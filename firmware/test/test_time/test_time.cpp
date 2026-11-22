#include <unity.h>
#include "seri/time_util.h"
using namespace seri;
void setUp() {}
void tearDown() {}

// 2026-11-21 00:00:00 UTC = 1795219200 (checked with `date -u -d 2026-11-21 +%s`)
static const int64_t k20261121Utc = 1795219200;

static void test_utc_midnight_is_0530_ist_same_day() {
  CivilTime t = toIst(k20261121Utc);
  TEST_ASSERT_EQUAL(2026, t.year);
  TEST_ASSERT_EQUAL(11, t.month);
  TEST_ASSERT_EQUAL(21, t.day);
  TEST_ASSERT_EQUAL(5, t.hour);
  TEST_ASSERT_EQUAL(30, t.minute);
  TEST_ASSERT_EQUAL(0, t.second);
}

static void test_late_utc_evening_rolls_to_next_ist_day() {
  // 2026-11-21 20:00:00 UTC = 2026-11-22 01:30 IST
  CivilTime t = toIst(k20261121Utc + 20 * 3600);
  TEST_ASSERT_EQUAL(22, t.day);
  TEST_ASSERT_EQUAL(1, t.hour);
  TEST_ASSERT_EQUAL(30, t.minute);
}

static void test_leap_day_and_year_end() {
  // 2028-02-29 12:00 IST and 2026-12-31 23:59:59 IST round-trip
  CivilTime a{2028, 2, 29, 12, 0, 0};
  CivilTime b{2026, 12, 31, 23, 59, 59};
  CivilTime ra = toIst(fromIst(a));
  CivilTime rb = toIst(fromIst(b));
  TEST_ASSERT_EQUAL(29, ra.day);
  TEST_ASSERT_EQUAL(2, ra.month);
  TEST_ASSERT_EQUAL(2028, ra.year);
  TEST_ASSERT_EQUAL(31, rb.day);
  TEST_ASSERT_EQUAL(59, rb.second);
  // one second later is new year
  CivilTime n = toIst(fromIst(b) + 1);
  TEST_ASSERT_EQUAL(2027, n.year);
  TEST_ASSERT_EQUAL(1, n.month);
  TEST_ASSERT_EQUAL(1, n.day);
  TEST_ASSERT_EQUAL(0, n.hour);
}

static void test_from_ist_matches_known_epoch() {
  CivilTime t{2026, 11, 21, 5, 30, 0};
  TEST_ASSERT_EQUAL_INT64(k20261121Utc, fromIst(t));
}

static void test_ist_day_changes_exactly_at_ist_midnight() {
  int64_t midnight = fromIst(CivilTime{2026, 11, 22, 0, 0, 0});
  TEST_ASSERT_EQUAL_INT64(istDay(midnight - 1) + 1, istDay(midnight));
  TEST_ASSERT_EQUAL_INT64(istDay(midnight), istDay(midnight + 86399));
  // day numbering: 1970-01-01 IST is day 0
  TEST_ASSERT_EQUAL_INT64(0, istDay(fromIst(CivilTime{1970, 1, 1, 0, 0, 0})));
}

static void test_utc_calendar_conversion_round_trip() {
  CivilTime u = toUtc(k20261121Utc + 3600 + 61);
  TEST_ASSERT_EQUAL(2026, u.year); TEST_ASSERT_EQUAL(11, u.month); TEST_ASSERT_EQUAL(21, u.day);
  TEST_ASSERT_EQUAL(1, u.hour); TEST_ASSERT_EQUAL(1, u.minute); TEST_ASSERT_EQUAL(1, u.second);
  TEST_ASSERT_EQUAL_INT64(k20261121Utc + 3661, fromUtc(u));
  TEST_ASSERT_EQUAL_INT64(fromIst(CivilTime{2026, 11, 21, 5, 30, 0}), fromUtc(CivilTime{2026, 11, 21, 0, 0, 0}));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_utc_midnight_is_0530_ist_same_day);
  RUN_TEST(test_late_utc_evening_rolls_to_next_ist_day);
  RUN_TEST(test_leap_day_and_year_end);
  RUN_TEST(test_from_ist_matches_known_epoch);
  RUN_TEST(test_ist_day_changes_exactly_at_ist_midnight);
  RUN_TEST(test_utc_calendar_conversion_round_trip);
  return UNITY_END();
}
