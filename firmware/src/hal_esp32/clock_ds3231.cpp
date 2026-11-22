#include "clock_ds3231.h"
#include <Arduino.h>
#include "seri/time_util.h"

namespace {
constexpr uint8_t kAddr = 0x68;
uint8_t bcd(int v) { return static_cast<uint8_t>(((v / 10) << 4) | (v % 10)); }
int unbcd(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
}  // namespace

bool Ds3231Clock::readRegs(uint8_t reg, uint8_t* buf, size_t n) {
  w_.beginTransmission(kAddr);
  w_.write(reg);
  if (w_.endTransmission(false) != 0) return false;
  if (w_.requestFrom(static_cast<int>(kAddr), static_cast<int>(n)) != static_cast<int>(n)) return false;
  for (size_t i = 0; i < n; ++i) buf[i] = static_cast<uint8_t>(w_.read());
  return true;
}

int64_t Ds3231Clock::nowUtc() {
  uint8_t r[7];
  if (!readRegs(0x00, r, 7)) {  // bus error: carry on from the last good reading
    return last_ + static_cast<int64_t>((millis() - lastMs_) / 1000);
  }
  seri::CivilTime t{2000 + unbcd(r[6]) + ((r[5] & 0x80) ? 100 : 0), unbcd(r[5] & 0x1F), unbcd(r[4]),
                    unbcd(r[2] & 0x3F), unbcd(r[1] & 0x7F), unbcd(r[0] & 0x7F)};
  last_ = seri::fromUtc(t);
  lastMs_ = millis();
  return last_;
}

bool Ds3231Clock::valid() {
  uint8_t st;
  if (!readRegs(0x0F, &st, 1)) return false;
  return (st & 0x80) == 0 && nowUtc() > 1700000000;  // OSF clear and after Nov 2023
}

void Ds3231Clock::set(int64_t utc) {
  seri::CivilTime t = seri::toUtc(utc);
  w_.beginTransmission(kAddr);
  w_.write(0x00);
  w_.write(bcd(t.second));
  w_.write(bcd(t.minute));
  w_.write(bcd(t.hour));  // 24-hour mode (bit 6 = 0)
  w_.write(static_cast<uint8_t>(((utc / 86400 + 4) % 7) + 1));
  w_.write(bcd(t.day));
  w_.write(static_cast<uint8_t>(bcd(t.month) | (t.year >= 2100 ? 0x80 : 0)));
  w_.write(bcd(t.year % 100));
  w_.endTransmission();
  w_.beginTransmission(kAddr);  // clear OSF
  w_.write(0x0F);
  w_.write(0x00);
  w_.endTransmission();
}
