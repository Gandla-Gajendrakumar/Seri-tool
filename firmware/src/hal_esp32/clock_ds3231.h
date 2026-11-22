#pragma once
// DS3231 RTC driver. Keeps UTC. Register map per Maxim DS3231 datasheet (time 0x00-0x06 BCD,
// status 0x0F bit 7 = OSF "oscillator stop flag": time is not trustworthy).
#include <Wire.h>
#include "seri/hal.h"

class Ds3231Clock : public seri::hal::Clock {
 public:
  explicit Ds3231Clock(TwoWire& w) : w_(w) {}
  int64_t nowUtc() override;
  bool valid() override;
  void set(int64_t utc) override;
 private:
  bool readRegs(uint8_t reg, uint8_t* buf, size_t n);
  TwoWire& w_;
  int64_t last_ = 0;
  uint32_t lastMs_ = 0;
};
