// Seri-tool firmware entry point: builds the ESP32 drivers, hands them to the portable Monitor,
// and runs it. All decisions live in lib/core (tested natively); this file is wiring only.
#include <Arduino.h>
#include <Wire.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include "board.h"
#include "hal_esp32/clock_ds3231.h"
#include "hal_esp32/indicators.h"
#include "hal_esp32/modem_a7670.h"
#include "hal_esp32/oled.h"
#include "hal_esp32/persist_nvs.h"
#include "hal_esp32/power_sense.h"
#include "hal_esp32/printer_escpos.h"
#include "hal_esp32/sensor_sht31.h"
#include "hal_esp32/storage_sd.h"
#include "hal_esp32/trace_serial.h"
#include "hal_esp32/wifi_net.h"
#include "seri/monitor.h"
#include "seri/text_render.h"
#include "seri/ui.h"

namespace {
Ds3231Clock rtc(Wire);
Sht31Sensor sht(Wire);
PowerSense power(board::kMainsOk, board::kBattSense, board::kBattDivider);
A7670Modem modem(Serial2, board::kModemRx, board::kModemTx, board::kModemPwrKey);
WifiNet wifi;
EscPosPrinter printer(Serial1, board::kPrinterRx, board::kPrinterTx);
SdStorage sd(board::kSdCs, board::kSdCardDetect);
GpioIndicators indicators(board::kBuzzer, board::kLedGreen, board::kLedRed);
NvsPersist persist;
SerialTrace trace;
Oled oled;
seri::Monitor* monitor = nullptr;
seri::Ui* ui = nullptr;

uint32_t bootMs = 0, lastDraw = 0;
bool appConfirmed = false, dirty = true;

struct Btn {
  int pin;
  seri::Button id;
  bool down = false;
  uint32_t changedMs = 0;
};
Btn buttons[] = {{board::kBtnUp, seri::Button::Up}, {board::kBtnOk, seri::Button::Ok}, {board::kBtnDown, seri::Button::Down}};

void pollButtons() {
  for (Btn& b : buttons) {
    const bool down = digitalRead(b.pin) == LOW;
    if (down != b.down && millis() - b.changedMs > 30) {  // 30 ms debounce
      b.down = down;
      b.changedMs = millis();
      if (down) { ui->press(b.id, millis() / 1000); dirty = true; }
    }
  }
}

// Indian-script printer lines: 24-dot raster, 384 dots wide (58 mm head).
bool rasterise(const std::string& utf8, seri::Language, std::vector<uint8_t>& bits, int& widthBytes, int& height) {
  seri::Bitmap b = seri::renderText(utf8, seri::kPrintCell, seri::kPrintWidth);
  bits = std::move(b.bits);
  widthBytes = b.widthBytes();
  height = b.height;
  return true;
}
}  // namespace

// Arduino-ESP32 hook: keep a freshly updated image "pending" until we confirm it is healthy,
// so the bootloader rolls back if it crashes or hangs (F-OTA-01).
extern "C" bool verifyRollbackLater() { return true; }

void setup() {
  Serial.begin(115200);
  Serial.printf("seri-tool %s%s booting\n", cfg::kFirmwareVersion, cfg::kSim ? " (SIM x60)" : "");
  Wire.begin(board::kI2cSda, board::kI2cScl, board::kI2cHz);
  pinMode(board::kBtnUp, INPUT_PULLUP);
  pinMode(board::kBtnOk, INPUT_PULLUP);
  pinMode(board::kBtnDown, INPUT_PULLUP);
  indicators.begin();
  power.begin();
  persist.begin();
  sd.begin();
  printer.begin();
  printer.setRasteriser(rasterise);
  oled.begin();
  modem.begin("seri-device");
  esp_task_wdt_config_t wdt = {.timeout_ms = 30000, .idle_core_mask = 0, .trigger_panic = true};
  esp_task_wdt_reconfigure(&wdt);
  esp_task_wdt_add(nullptr);  // R-09: reset if the loop stops for 30 s

  static seri::Hardware hw{rtc, sht, power, modem, wifi, printer, sd, indicators, persist, &trace};
  monitor = new seri::Monitor(hw);
  monitor->begin();
  ui = new seri::Ui(*monitor);
  wifi.configure(monitor->settings().wifiSsid, monitor->settings().wifiPass, monitor->settings().deviceName);
  bootMs = millis();
  Serial.println("READY");
}

void loop() {
  monitor->tick();
  esp_task_wdt_reset();

  pollButtons();
  ui->tick(millis() / 1000);
  if (dirty || millis() - lastDraw > 1000) {  // full frame over 100 kHz I2C takes ~90 ms
    oled.show(ui->render(seri::UiStatus{modem.lastKnownRegistered(), wifi.connected(), rtc.nowUtc()}));
    lastDraw = millis();
    dirty = false;
  }

  wifi.configure(monitor->settings().wifiSsid, monitor->settings().wifiPass, monitor->settings().deviceName);

  // Confirm a new firmware image after 5 minutes of normal running (F-OTA-01).
  if (!appConfirmed && millis() - bootMs > 5UL * 60UL * 1000UL / (cfg::kSim ? 60 : 1)) {
    esp_ota_mark_app_valid_cancel_rollback();
    appConfirmed = true;
  }
  delay(cfg::kSim ? 5 : 20);
}
