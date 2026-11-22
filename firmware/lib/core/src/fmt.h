#pragma once
// Internal formatting helpers shared by SMS, printer and command replies.
#include <cstdio>
#include <string>
#include "seri/time_util.h"
#include "seri/types.h"

namespace seri {
namespace fmt {

inline std::string signedTenths(Deci v, bool alwaysDecimal) {
  char b[32];
  const Deci a = v < 0 ? -v : v;
  if (!alwaysDecimal && a % 10 == 0)
    std::snprintf(b, sizeof b, "%s%ld", v < 0 ? "-" : "", static_cast<long>(a / 10));
  else
    std::snprintf(b, sizeof b, "%s%ld.%ld", v < 0 ? "-" : "", static_cast<long>(a / 10), static_cast<long>(a % 10));
  return b;
}
inline std::string temp(Deci v) { return signedTenths(v, true) + "C"; }   // reading: 31.2C
inline std::string bandTemp(Deci v) { return signedTenths(v, false); }    // band edge: 26 / 25.5
inline std::string rhInt(Deci v) { return std::to_string((v + 5) / 10); }  // 85
inline std::string rh(Deci v) { return rhInt(v) + "%"; }
inline std::string hhmm(int64_t utc) {
  CivilTime t = toIst(utc);
  char b[32];
  std::snprintf(b, sizeof b, "%02d:%02d", t.hour, t.minute);
  return b;
}
inline std::string ddmmHhmm(int64_t utc) {
  CivilTime t = toIst(utc);
  char b[32];
  std::snprintf(b, sizeof b, "%02d/%02d %02d:%02d", t.day, t.month, t.hour, t.minute);
  return b;
}

}  // namespace fmt
}  // namespace seri
