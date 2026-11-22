#include "power_sense.h"
#include <Arduino.h>

void PowerSense::begin() {
  pinMode(mains_, INPUT);
  analogSetPinAttenuation(batt_, ADC_11db);  // 0-3.1 V input range
}

bool PowerSense::mainsPresent() { return digitalRead(mains_) == HIGH; }

int32_t PowerSense::batteryMv() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; ++i) sum += analogReadMilliVolts(batt_);  // eFuse-calibrated reading
  return static_cast<int32_t>(static_cast<float>(sum) / 8.0f * div_);
}
