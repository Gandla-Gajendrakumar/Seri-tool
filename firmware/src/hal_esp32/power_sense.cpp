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
  // Wokwi models the ADC as linear 0-3.3 V over 0-4095 and does not emulate the chip's eFuse
  // calibration, so analogReadMilliVolts() reads about 24 % low there (seen in CI run 8).
  for (int i = 0; i < 8; ++i) sum += static_cast<uint32_t>(analogRead(batt_)) * 3300u / 4095u;
#else
  // Real ESP32: eFuse-calibrated reading. Accuracy is checked and trimmed on the bench (test plan).
  for (int i = 0; i < 8; ++i) sum += analogReadMilliVolts(batt_);
#endif
  return static_cast<int32_t>(static_cast<float>(sum) / 8.0f * div_);
}
