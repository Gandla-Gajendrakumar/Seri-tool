#include "sensor_sht31.h"
#include <Arduino.h>

namespace {
constexpr uint8_t kAddr = 0x44;
uint8_t crc8(const uint8_t* d, int n) {
  uint8_t c = 0xFF;
  for (int i = 0; i < n; ++i) {
    c ^= d[i];
    for (int b = 0; b < 8; ++b) c = (c & 0x80) ? static_cast<uint8_t>((c << 1) ^ 0x31) : static_cast<uint8_t>(c << 1);
  }
  return c;
}
}  // namespace

seri::RawReading Sht31Sensor::read() {
  w_.beginTransmission(kAddr);
  w_.write(0x24);
  w_.write(0x00);
  if (w_.endTransmission() != 0) return {false, 0, 0};
  delay(16);
  uint8_t r[6];
  if (w_.requestFrom(static_cast<int>(kAddr), 6) != 6) return {false, 0, 0};
  for (int i = 0; i < 6; ++i) r[i] = static_cast<uint8_t>(w_.read());
  if (crc8(r, 2) != r[2] || crc8(r + 3, 2) != r[5]) return {false, 0, 0};
  const float t = -45.0f + 175.0f * static_cast<float>((r[0] << 8) | r[1]) / 65535.0f;
  const float rh = 100.0f * static_cast<float>((r[3] << 8) | r[4]) / 65535.0f;
  return {true, t, rh};
}
