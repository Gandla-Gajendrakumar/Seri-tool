#pragma once
// SHT31 driver: single-shot, high repeatability, no clock stretching (command 0x2400).
// Per Sensirion SHT3x-DIS datasheet: measurement takes max 15 ms at high repeatability;
// each 16-bit word is followed by CRC-8 (poly 0x31, init 0xFF).
#include <Wire.h>
#include "seri/hal.h"

class Sht31Sensor : public seri::hal::Sensor {
 public:
  explicit Sht31Sensor(TwoWire& w) : w_(w) {}
  seri::RawReading read() override;
 private:
  TwoWire& w_;
};
