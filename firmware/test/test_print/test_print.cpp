#include <unity.h>
#include <string>
#include "seri/print.h"
#include "seri/sms_text.h"
#include "seri/time_util.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static int64_t ist(int d, int h, int m) { return fromIst(CivilTime{2026, 11, d, h, m, 0}); }
static int cols(const std::string& s) { return static_cast<int>(s.size()); }  // English = ASCII

static void test_event_lines_english_exact() {
  TEST_ASSERT_EQUAL_STRING("21/11 14:05 TEMP HIGH 31.2C",
                           printEventLine(Event{EventType::TempHigh, Edge::Start, ist(21, 14, 5), 312}, Language::English).c_str());
  TEST_ASSERT_EQUAL_STRING("21/11 14:40 TEMP OK 28.4C",
                           printEventLine(Event{EventType::TempHigh, Edge::Clear, ist(21, 14, 40), 284}, Language::English).c_str());
  TEST_ASSERT_EQUAL_STRING("21/11 09:00 RH OK 72%",
                           printEventLine(Event{EventType::RhHigh, Edge::SilentClear, ist(21, 9, 0), 720}, Language::English).c_str());
  TEST_ASSERT_EQUAL_STRING("22/11 02:10 POWER CUT",
                           printEventLine(Event{EventType::MainsFail, Edge::Start, ist(22, 2, 10), 0}, Language::English).c_str());
  TEST_ASSERT_EQUAL_STRING("22/11 07:45 BATTERY LOW 3.38V",
                           printEventLine(Event{EventType::LowBattery, Edge::Start, ist(22, 7, 45), 3380}, Language::English).c_str());
}

static void test_every_english_event_line_fits_32_columns() {
  for (int t = 0; t < kEventTypeCount; ++t)
    for (Edge e : {Edge::Start, Edge::Clear, Edge::SilentClear}) {
      std::string l = printEventLine(Event{static_cast<EventType>(t), e, ist(30, 23, 59), -105}, Language::English);
      TEST_ASSERT_TRUE_MESSAGE(cols(l) <= kPrintCols && cols(l) > 12, l.c_str());
    }
}

static void test_indian_language_event_line_uses_local_words_and_western_digits() {
  std::string l = printEventLine(Event{EventType::TempHigh, Edge::Start, ist(21, 14, 5), 312}, Language::Telugu);
  TEST_ASSERT_EQUAL_STRING("21/11 14:05 ఉష్ణోగ్రత ఎక్కువ 31.2C", l.c_str());
}

static void test_stage_change_line() {
  TEST_ASSERT_EQUAL_STRING("21/11 09:00 STAGE -> Instar 4", printStageLine(ist(21, 9, 0), Stage::Instar4, Language::English).c_str());
}

static void test_summary_min_max_and_out_of_range_time() {
  Band b = bandFor(Stage::Chawki, Tolerance{});  // 26-29 C, 75-95 %
  DailySummary acc(ist(21, 6, 0));
  int64_t t = ist(21, 6, 0);
  // 10 samples in band, 10 samples hot (30.5C), 4 invalid, 6 humid (97%)
  for (int i = 0; i < 10; ++i, t += 30) acc.addSample(t, Sample{true, 275, 850}, b);
  for (int i = 0; i < 10; ++i, t += 30) acc.addSample(t, Sample{true, 305, 850}, b);
  for (int i = 0; i < 4; ++i, t += 30) acc.addSample(t, Sample{false, 0, 0}, b);
  for (int i = 0; i < 6; ++i, t += 30) acc.addSample(t, Sample{true, 262, 970}, b);
  acc.addEvent(Event{EventType::TempHigh, Edge::Start, t, 305});
  acc.addEvent(Event{EventType::TempHigh, Edge::Clear, t, 280});  // clears are not counted
  DayReport r = acc.close(ist(22, 6, 0));
  TEST_ASSERT_EQUAL(30, r.samples);
  TEST_ASSERT_EQUAL(4, r.invalidSamples);
  TEST_ASSERT_EQUAL(262, r.tMin);
  TEST_ASSERT_EQUAL(305, r.tMax);
  TEST_ASSERT_EQUAL_INT64(ist(21, 6, 0) + 10 * 30, r.tMaxAt);
  TEST_ASSERT_EQUAL(850, r.rhMin);
  TEST_ASSERT_EQUAL(970, r.rhMax);
  TEST_ASSERT_EQUAL_INT64(10 * 30, r.tOutSec);
  TEST_ASSERT_EQUAL_INT64(6 * 30, r.rhOutSec);
  TEST_ASSERT_EQUAL_INT64(4 * 30, r.noDataSec);
  TEST_ASSERT_EQUAL(1, r.alerts);
  TEST_ASSERT_EQUAL_INT64(ist(22, 6, 0), r.to);
  // new period starts empty
  DayReport r2 = acc.close(ist(23, 6, 0));
  TEST_ASSERT_EQUAL(0, r2.samples);
  TEST_ASSERT_EQUAL_INT64(ist(22, 6, 0), r2.from);
}

static void test_gap_in_samples_is_capped_at_60_seconds() {
  // a 10-minute gap (e.g. reboot) must not count as 10 minutes out of range
  Band b = bandFor(Stage::Chawki, Tolerance{});
  DailySummary acc(ist(21, 6, 0));
  acc.addSample(ist(21, 7, 0), Sample{true, 305, 850}, b);
  acc.addSample(ist(21, 7, 10), Sample{true, 305, 850}, b);
  DayReport r = acc.close(ist(22, 6, 0));
  TEST_ASSERT_EQUAL_INT64(30 + 60, r.tOutSec);
}

static void test_summary_print_english_exact_and_fits() {
  DayReport r;
  r.from = ist(20, 6, 0); r.to = ist(21, 6, 0);
  r.samples = 2880; r.tMin = 241; r.tMinAt = ist(21, 3, 10); r.tMax = 302; r.tMaxAt = ist(20, 14, 20);
  r.rhMin = 610; r.rhMax = 930; r.tOutSec = 3900; r.rhOutSec = 1200; r.alerts = 3; r.noDataSec = 600;
  auto lines = printSummary(r, Stage::Chawki, bandFor(Stage::Chawki, Tolerance{}), Language::English, true);
  const char* expect[] = {
      "======= DAILY SUMMARY =======",
      "20/11 06:00 - 21/11 06:00",
      "Chawki 26-29C 75-95%",
      "Temp min 24.1C 03:10",
      "Temp max 30.2C 14:20",
      "Humidity 61% - 93%",
      "Temp out of range 1h05m",
      "Humidity out of range 0h20m",
      "No sensor data 0h10m",
      "Alerts 3",
      "PRINTED LATE (POWER CUT)",
      "--------------------------------",
  };
  TEST_ASSERT_EQUAL(12, lines.size());
  for (size_t i = 0; i < lines.size(); ++i) {
    TEST_ASSERT_EQUAL_STRING(expect[i], lines[i].c_str());
    TEST_ASSERT_TRUE(cols(lines[i]) <= kPrintCols);
  }
}

static void test_summary_without_data_says_so_and_skips_optional_lines() {
  DayReport r;
  r.from = ist(20, 6, 0); r.to = ist(21, 6, 0);
  auto lines = printSummary(r, Stage::Chawki, bandFor(Stage::Chawki, Tolerance{}), Language::English, false);
  bool sawNoData = false;
  for (auto& l : lines) {
    if (l == "Temp min --") sawNoData = true;
    TEST_ASSERT_TRUE(l.find("LATE") == std::string::npos);
  }
  TEST_ASSERT_TRUE(sawNoData);
}

static void test_summary_has_lines_in_every_language() {
  DayReport r;
  r.from = ist(20, 6, 0); r.to = ist(21, 6, 0); r.samples = 10; r.tMin = 250; r.tMax = 260;
  for (int l = 1; l < kLanguageCount; ++l) {
    auto lines = printSummary(r, Stage::Instar3, bandFor(Stage::Instar3, Tolerance{}), static_cast<Language>(l), true);
    TEST_ASSERT_TRUE(lines.size() >= 8);
    TEST_ASSERT_TRUE_MESSAGE(smsSegments(lines[0]).ucs2, "title must be in the local script");
  }
}

static void test_scheduler_prints_once_at_6am() {
  SummaryScheduler s;
  DailySummary acc(ist(21, 0, 0));
  TEST_ASSERT_EQUAL(0, s.tick(ist(21, 5, 59), false, acc).size());
  auto d = s.tick(ist(21, 6, 0), false, acc);
  TEST_ASSERT_EQUAL(1, d.size());
  TEST_ASSERT_FALSE(d[0].late);
  TEST_ASSERT_EQUAL(0, s.tick(ist(21, 6, 1), false, acc).size());
  TEST_ASSERT_EQUAL(0, s.tick(ist(21, 23, 0), false, acc).size());
  TEST_ASSERT_EQUAL(1, s.tick(ist(22, 6, 0), false, acc).size());
}

static void test_scheduler_holds_summary_on_battery_and_prints_late() {
  SummaryScheduler s;
  DailySummary acc(ist(20, 6, 0));
  TEST_ASSERT_EQUAL(0, s.tick(ist(21, 6, 0), true, acc).size());
  TEST_ASSERT_EQUAL(0, s.tick(ist(21, 9, 0), true, acc).size());
  TEST_ASSERT_EQUAL(0, s.tick(ist(22, 6, 0), true, acc).size());  // second day also on battery
  auto d = s.tick(ist(22, 8, 0), false, acc);                      // mains back
  TEST_ASSERT_EQUAL(2, d.size());
  TEST_ASSERT_TRUE(d[0].late);
  TEST_ASSERT_EQUAL_INT64(ist(20, 6, 0), d[0].report.from);
  TEST_ASSERT_EQUAL_INT64(ist(21, 6, 0), d[1].report.from);
  TEST_ASSERT_EQUAL(0, s.tick(ist(22, 9, 0), false, acc).size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_event_lines_english_exact);
  RUN_TEST(test_every_english_event_line_fits_32_columns);
  RUN_TEST(test_indian_language_event_line_uses_local_words_and_western_digits);
  RUN_TEST(test_stage_change_line);
  RUN_TEST(test_summary_min_max_and_out_of_range_time);
  RUN_TEST(test_gap_in_samples_is_capped_at_60_seconds);
  RUN_TEST(test_summary_print_english_exact_and_fits);
  RUN_TEST(test_summary_without_data_says_so_and_skips_optional_lines);
  RUN_TEST(test_summary_has_lines_in_every_language);
  RUN_TEST(test_scheduler_prints_once_at_6am);
  RUN_TEST(test_scheduler_holds_summary_on_battery_and_prints_late);
  return UNITY_END();
}
