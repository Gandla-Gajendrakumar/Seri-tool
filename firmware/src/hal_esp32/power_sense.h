#pragma once
#include "seri/hal.h"

class PowerSense : public seri::hal::Power {
 public:
  PowerSense(int mainsPin, int battPin, float divider) : mains_(mainsPin), batt_(battPin), div_(divider) {}
  void begin();
  bool mainsPresent() override;
  int32_t batteryMv() override;
 private:
  int mains_, batt_;
  float div_;
};
