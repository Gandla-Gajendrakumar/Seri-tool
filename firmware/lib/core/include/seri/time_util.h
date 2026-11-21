#pragma once
// Calendar helpers. All stored times are UTC seconds since 1970-01-01 (Unix time).
// India has one time zone, IST = UTC+5:30, with no daylight saving (decision D-005).
#include <cstdint>

namespace seri {

constexpr int32_t kIstOffsetSec = 5 * 3600 + 30 * 60;

struct CivilTime {
  int year, month, day;      // month 1-12, day 1-31
  int hour, minute, second;  // 24-hour clock
};

// UTC seconds -> wall-clock time in IST.
CivilTime toIst(int64_t utc);
// Wall-clock time in IST -> UTC seconds.
int64_t fromIst(const CivilTime& ist);
// Day number (days since 1970-01-01) of the IST calendar date. Changes at IST midnight;
// used to reset daily SMS caps and to close the daily summary.
int64_t istDay(int64_t utc);

}  // namespace seri
