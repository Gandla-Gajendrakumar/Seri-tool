#include "seri/time_util.h"

namespace seri {
namespace {
// Howard Hinnant's days_from_civil / civil_from_days (public domain), proleptic Gregorian.
int64_t daysFromCivil(int64_t y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<int64_t>(doe) - 719468;
}
void civilFromDays(int64_t z, int& y, int& m, int& d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = static_cast<unsigned>(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  y = static_cast<int>(static_cast<int64_t>(yoe) + era * 400 + (m <= 2));
}
int64_t floorDiv(int64_t a, int64_t b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }
}  // namespace

CivilTime toIst(int64_t utc) {
  const int64_t local = utc + kIstOffsetSec;
  const int64_t days = floorDiv(local, 86400);
  const int64_t secs = local - days * 86400;
  CivilTime t{};
  civilFromDays(days, t.year, t.month, t.day);
  t.hour = static_cast<int>(secs / 3600);
  t.minute = static_cast<int>((secs % 3600) / 60);
  t.second = static_cast<int>(secs % 60);
  return t;
}

int64_t fromIst(const CivilTime& t) {
  const int64_t days = daysFromCivil(t.year, static_cast<unsigned>(t.month), static_cast<unsigned>(t.day));
  return days * 86400 + t.hour * 3600 + t.minute * 60 + t.second - kIstOffsetSec;
}

int64_t istDay(int64_t utc) { return floorDiv(utc + kIstOffsetSec, 86400); }
CivilTime toUtc(int64_t utc) { return toIst(utc - kIstOffsetSec); }
int64_t fromUtc(const CivilTime& t) { return fromIst(t) + kIstOffsetSec; }

}  // namespace seri
