#pragma once
// ESP32-DevKitC (WROOM-32E) pin map for the version-1 carrier board. The KiCad schematic (phase 4)
// must match this file. Strapping pins (0, 2, 5, 12, 15) are only used where their boot level is safe.
#include <cstdint>

namespace board {
constexpr int kI2cSda = 21, kI2cScl = 22;          // SHT31 (0x44), DS3231 (0x68), OLED (0x3C)
constexpr uint32_t kI2cHz = 50000;                   // 2 m extended probe cable (D-052): half of standard mode
constexpr int kModemTx = 17, kModemRx = 16;          // UART2 -> A7670C RXD / from A7670C TXD
constexpr int kModemPwrKey = 27;                     // via NPN (Q8) on the carrier: HIGH = press
#ifdef SERI_SIM
constexpr bool kPwrKeyActiveHigh = false;            // Wokwi: GPIO wired straight to the module's PWRKEY
#else
constexpr bool kPwrKeyActiveHigh = true;
#endif
constexpr int kPrinterTx = 25, kPrinterRx = 39;      // UART1 (9600 or 19200, auto-detected). RX on
                                                     // input-only VN: the printer may drive it at boot (D-041)
constexpr int kPrinterPowerEn = 26;                  // TPS61088 EN, 100k pull-down: boost off until firmware
                                                     // turns it on; off between prints on battery (D-046)
constexpr int kSdSck = 18, kSdMiso = 19, kSdMosi = 23, kSdCs = 5;
constexpr int kSdCardDetect = 34;                    // input-only; LOW = card inserted (ext. pull-up)
constexpr int kBtnUp = 32, kBtnOk = 33, kBtnDown = 14;  // to GND, internal pull-ups
constexpr int kBuzzer = 13;
constexpr int kLedGreen = 2, kLedRed = 4;
constexpr int kMainsOk = 35;                         // input-only; HIGH = 12 V present
constexpr int kBattSense = 36;                       // ADC1_CH0 (VP), battery via 100k/100k divider
constexpr float kBattDivider = 2.0f;
constexpr int kDs18b20 = 15;                         // optional probe header (4.7k pull-up on board)
constexpr int kRelay = 12;                           // future relay header; needs ext. pull-down (strapping)
}  // namespace board

namespace cfg {
#ifdef SERI_SIM
constexpr bool kSim = true;
#else
constexpr bool kSim = false;
#endif
// MQTT broker: per device, imported from seri.cfg + ca.pem on the microSD card (D-060, docs/handoff/installer-guide.md).
constexpr const char* kFirmwareVersion = "0.3.1";
}  // namespace cfg
