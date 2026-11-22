#pragma once
#include <Arduino.h>
#include "seri/hal.h"

class GpioIndicators : public seri::hal::Indicators {
 public:
  GpioIndicators(int buzzer, int green, int red) : bz_(buzzer), g_(green), r_(red) {}
  void begin() { pinMode(bz_, OUTPUT); pinMode(g_, OUTPUT); pinMode(r_, OUTPUT); }
  void buzzer(bool on) override {
    if (on == on_) return;
    on_ = on;
    if (on) tone(bz_, 2700);  // piezo resonance; an active buzzer simply sees the pin high
    else noTone(bz_);
  }
  void green(bool on) override { digitalWrite(g_, on ? HIGH : LOW); }
  void red(bool on) override { digitalWrite(r_, on ? HIGH : LOW); }
 private:
  int bz_, g_, r_;
  bool on_ = false;
};
