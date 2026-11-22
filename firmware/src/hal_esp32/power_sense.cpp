#include "power_sense.h"
#include <Arduino.h>

void PowerSense::begin() {
  pinMode(mains_, INPUT);
  analogSetPinAttenuation(batt_, ADC_11db);  // 0-3.1 V input range
}

bool PowerSense::mainsPresent() { return digitalRead(mains_) == HIGH; }

int32_t PowerSense::batteryMv() {
  uint32_t sum = 0;
#ifdef SERI_SIM
  // Wokwi's ESP32 ADC is linear with 0-4095 spanning 0-5 V (measured in CI runs 8 and 9: a chip
  // output of 1.95 V read raw ~1597 = 1.95/5*4095, and 1.65 V read ~1351), and it does not emulate
  // the eFuse calibration used by analogReadMilliVolts(). So the simulation build converts raw counts.
  for (int i = 0; i < 8; ++i) sum += static_cast<uint32_t>(analogRead(batt_)) * 5000u / 4095u;
#else
  // Real ESP32: eFuse-calibrated reading. Accuracy is checked and trimmed on the bench (test plan).
  for (int i = 0; i < 8; ++i) sum += analogReadMilliVolts(batt_);
#endif
  return static_cast<int32_t>(static_cast<float>(sum) / 8.0f * div_);
}
