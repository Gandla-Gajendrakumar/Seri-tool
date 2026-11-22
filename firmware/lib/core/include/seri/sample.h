#pragma once
// Turns 5 raw sensor reads into one 30 s sample (requirement F-MEAS-01/03).
#include <cstddef>
#include "seri/types.h"

namespace seri {

struct RawReading {
  bool ok;   // false = I2C error / CRC failure / timeout
  float t;   // degC
  float rh;  // %RH
};

// Discards failed or physically impossible reads, then takes the median of what is left.
// Fewer than 3 usable reads -> invalid sample.
Sample medianSample(const RawReading* reads, size_t n);

}  // namespace seri
