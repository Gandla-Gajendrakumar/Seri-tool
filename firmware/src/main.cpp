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
#include "hal_esp32/persist_nvs.h"
#include "hal_esp32/power_sense.h"
#include "hal_esp32/printer_escpos.h"
#include "hal_esp32/sensor_sht31.h"
#include "hal_esp32/storage_sd.h"
#include "hal_esp32/trace_serial.h"
#include "hal_esp32/wifi_net.h"
#include "seri/monitor.h"

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
seri::Monitor* monitor = nullptr;

uint32_t bootMs = 0;
bool appConfirmed = false;
bool okPrev = true;
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
  modem.begin("seri-device");
  esp_task_wdt_config_t wdt = {.timeout_ms = 30000, .idle_core_mask = 0, .trigger_panic = true};
  esp_task_wdt_reconfigure(&wdt);
  esp_task_wdt_add(nullptr);  // R-09: reset if the loop stops for 30 s

  static seri::Hardware hw{rtc, sht, power, modem, wifi, printer, sd, indicators, persist, &trace};
  monitor = new seri::Monitor(hw);
  monitor->begin();
  wifi.configure(monitor->settings().wifiSsid, monitor->settings().wifiPass, monitor->settings().deviceName);
  bootMs = millis();
  Serial.println("READY");
}

void loop() {
  monitor->tick();
  esp_task_wdt_reset();

  // OK button: silence the buzzer (full menu: UI task)
  const bool ok = digitalRead(board::kBtnOk) == HIGH;
  if (!ok && okPrev) monitor->silenceBuzzer();
  okPrev = ok;

  wifi.configure(monitor->settings().wifiSsid, monitor->settings().wifiPass, monitor->settings().deviceName);

  // Confirm a new firmware image after 5 minutes of normal running (F-OTA-01).
  if (!appConfirmed && millis() - bootMs > 5UL * 60UL * 1000UL / (cfg::kSim ? 60 : 1)) {
    esp_ota_mark_app_valid_cancel_rollback();
    appConfirmed = true;
  }
  delay(cfg::kSim ? 5 : 20);
}
