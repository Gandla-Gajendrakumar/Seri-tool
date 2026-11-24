#include <unity.h>
#include <string>
#include "seri/sms_text.h"
#include "seri/time_util.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static const int64_t k1405 = fromIst(CivilTime{2026, 11, 21, 14, 5, 0});

static SmsContext ctx(Language l, Stage st = Stage::Chawki) {
  return SmsContext{"Shed-1", st, bandFor(st, Tolerance{}), l, k1405, 312, 87};
}
static SmsJob job(SmsKind k, EventType t, Deci v = 312, int64_t started = k1405) {
  return SmsJob{0, k, t, k1405, v, started};
}

static void test_english_start_message_exact() {
  TEST_ASSERT_EQUAL_STRING("Shed-1: Temp HIGH 31.2C (safe 26-29C) Chawki 14:05",
                           formatSms(job(SmsKind::Start, EventType::TempHigh), ctx(Language::English)).c_str());
  TEST_ASSERT_EQUAL_STRING("Shed-1: Humidity LOW 70% (safe 75-95%) Chawki 14:05",
                           formatSms(job(SmsKind::Start, EventType::RhLow, 700), ctx(Language::English)).c_str());
}

static void test_half_degree_band_edges_are_shown() {
  SmsContext c = ctx(Language::English);
  c.band = bandFor(Stage::Instar5, Tolerance{15, 50});  // 22.5-25.5
  TEST_ASSERT_EQUAL_STRING("Shed-1: Temp HIGH 31.2C (safe 22.5-25.5C) Chawki 14:05",
                           formatSms(job(SmsKind::Start, EventType::TempHigh), c).c_str());
}

static void test_hindi_start_message_exact() {
  TEST_ASSERT_EQUAL_STRING("Shed-1: तापमान ज्यादा 31.2C (सही 26-29C) चॉकी 14:05",
                           formatSms(job(SmsKind::Start, EventType::TempHigh), ctx(Language::Hindi)).c_str());
}

static void test_repeat_uses_current_value_and_duration() {
  SmsContext c = ctx(Language::English);
  c.currentValue = 305;
  c.now = k1405 + 95 * 60;
  SmsJob j = job(SmsKind::Repeat, EventType::TempHigh);
  j.time = c.now;
  TEST_ASSERT_EQUAL_STRING("Shed-1: STILL Temp HIGH 30.5C for 95min 15:40",
                           formatSms(j, c).c_str());
  c.now = j.time = k1405 + 26 * 3600;  // long events shown in hours
  TEST_ASSERT_EQUAL_STRING("Shed-1: STILL Temp HIGH 30.5C for 26h 16:05", formatSms(j, c).c_str());
}

static void test_recovery_and_system_messages_english() {
  TEST_ASSERT_EQUAL_STRING("Shed-1: Temp normal again 28.4C 14:05",
                           formatSms(job(SmsKind::Recovery, EventType::TempHigh, 284), ctx(Language::English)).c_str());
  TEST_ASSERT_EQUAL_STRING("Shed-1: Power cut, on battery 87% 14:05",
                           formatSms(job(SmsKind::Start, EventType::MainsFail), ctx(Language::English)).c_str());
  TEST_ASSERT_EQUAL_STRING("Shed-1: Power back 14:05",
                           formatSms(job(SmsKind::Recovery, EventType::MainsFail), ctx(Language::English)).c_str());
  TEST_ASSERT_EQUAL_STRING("Shed-1: Sensor fault, check probe cable 14:05",
                           formatSms(job(SmsKind::Start, EventType::SensorFault), ctx(Language::English)).c_str());
  TEST_ASSERT_EQUAL_STRING("Shed-1: SMS limit for today reached. Device keeps monitoring and printing.",
                           formatSms(job(SmsKind::LimitReached, EventType::TempHigh), ctx(Language::English)).c_str());
}

static void test_segment_counting() {
  SegmentInfo a = smsSegments(std::string(160, 'a'));
  TEST_ASSERT_FALSE(a.ucs2);
  TEST_ASSERT_EQUAL(160, a.units);
  TEST_ASSERT_EQUAL(1, a.segments);
  TEST_ASSERT_EQUAL(2, smsSegments(std::string(161, 'a')).segments);  // 153 per part
  TEST_ASSERT_EQUAL(3, smsSegments(std::string(307, 'a')).segments);
  TEST_ASSERT_EQUAL(84, smsSegments(std::string(80, 'a') + "{}").units);  // { } are GSM-7 escapes: 2 each
  SegmentInfo t = smsSegments("తేమ");  // 3 code points
  TEST_ASSERT_TRUE(t.ucs2);
  TEST_ASSERT_EQUAL(3, t.units);
  std::string s70, s71;
  for (int i = 0; i < 70; ++i) s70 += "क";
  s71 = s70 + "क";
  TEST_ASSERT_EQUAL(1, smsSegments(s70).segments);
  TEST_ASSERT_EQUAL(2, smsSegments(s71).segments);  // 67 per part
  TEST_ASSERT_TRUE_MESSAGE(smsSegments("Temp 25°C").ucs2, "degree sign is not in GSM-7: it forces UCS-2, so we write C");
}

static void test_every_message_fits_one_segment_in_every_language() {
  // Worst cases: longest device name, negative 3-digit values, long durations, every stage.
  const EventType types[] = {EventType::TempHigh, EventType::TempLow, EventType::RhHigh, EventType::RhLow,
                             EventType::SensorFault, EventType::MainsFail, EventType::LowBattery,
                             EventType::PaperOut, EventType::SdFault, EventType::ClockInvalid};
  const SmsKind kinds[] = {SmsKind::Start, SmsKind::Repeat, SmsKind::Recovery, SmsKind::LimitReached};
  int checked = 0;
  int worst[kLanguageCount] = {0};
  for (int l = 0; l < kLanguageCount; ++l)
    for (int st = 0; st < kStageCount; ++st)
      for (EventType t : types)
        for (SmsKind k : kinds) {
          SmsContext c = ctx(static_cast<Language>(l), static_cast<Stage>(st));
          c.deviceName = "ABCDEFGHIJ";
          c.band = Band{-105, -105, 1000, 1000};
          c.currentValue = -105;
          c.batteryPct = 100;
          c.now = k1405 + 999 * 3600;
          SmsJob j = job(k, t, -105, k1405);
          j.time = c.now;
          std::string m = formatSms(j, c);
          SegmentInfo si = smsSegments(m);
          if (si.segments != 1) {
            std::string msg = "does not fit 1 SMS (" + std::to_string(si.units) + " units): " + m;
            TEST_FAIL_MESSAGE(msg.c_str());
          }
          TEST_ASSERT_TRUE(m.size() > 8);
          if (si.units > worst[l]) worst[l] = si.units;
          ++checked;
        }
  TEST_ASSERT_EQUAL(kLanguageCount * kStageCount * 10 * 4, checked);
  std::string rep = "longest message (limit 160 English / 70 others): ";
  for (int l = 0; l < kLanguageCount; ++l) rep += std::string(languageName(static_cast<Language>(l))) + "=" + std::to_string(worst[l]) + " ";
  TEST_MESSAGE(rep.c_str());
}

static void test_indian_language_messages_are_not_english() {
  for (int l = 1; l < kLanguageCount; ++l) {
    std::string m = formatSms(job(SmsKind::Start, EventType::RhHigh, 960), ctx(static_cast<Language>(l)));
    TEST_ASSERT_TRUE_MESSAGE(smsSegments(m).ucs2, m.c_str());
    TEST_ASSERT_TRUE_MESSAGE(m.find("96%") != std::string::npos, "Western digits expected");
    TEST_ASSERT_TRUE(m.find("Humidity") == std::string::npos);
  }
}

static void test_every_stage_and_language_has_a_name() {
  for (int l = 0; l < kLanguageCount; ++l) {
    TEST_ASSERT_TRUE(std::string(languageName(static_cast<Language>(l))).size() > 0);
    for (int s = 0; s < kStageCount; ++s)
      TEST_ASSERT_TRUE(std::string(stageName(static_cast<Stage>(s), static_cast<Language>(l))).size() > 0);
  }
}

static void test_device_name_rules() {
  TEST_ASSERT_TRUE(deviceNameValid("Shed-1"));
  TEST_ASSERT_TRUE(deviceNameValid("ABCDEFGHIJ"));
  TEST_ASSERT_FALSE(deviceNameValid(""));
  TEST_ASSERT_FALSE(deviceNameValid("ABCDEFGHIJK"));
  TEST_ASSERT_FALSE(deviceNameValid("Shed 1"));
  TEST_ASSERT_FALSE(deviceNameValid("शेड"));
}

static void test_offline_summary_text_and_fit() {
  const int64_t from = fromIst(CivilTime{2026, 12, 22, 7, 0, 0}), to = fromIst(CivilTime{2026, 12, 22, 19, 0, 0});
  TEST_ASSERT_EQUAL_STRING("Shed-1: No signal 07:00-19:00. 2 alerts not sent. Now 27.5C 85%",
                           formatOfflineSummary("Shed-1", Language::English, from, to, 2, Sample{true, 275, 850}).c_str());
  for (int l = 0; l < kLanguageCount; ++l) {
    std::string m = formatOfflineSummary("ABCDEFGHIJ", static_cast<Language>(l), from, to, 99, Sample{true, -105, 1000});
    TEST_ASSERT_EQUAL_MESSAGE(1, smsSegments(m).segments, m.c_str());
    if (l > 0) TEST_ASSERT_TRUE_MESSAGE(smsSegments(m).ucs2, "local script");
  }
  // sensor not working at the moment: reading omitted
  std::string m = formatOfflineSummary("Shed-1", Language::English, from, to, 1, Sample{false, 0, 0});
  TEST_ASSERT_TRUE(m.find("Now") == std::string::npos);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_english_start_message_exact);
  RUN_TEST(test_half_degree_band_edges_are_shown);
  RUN_TEST(test_hindi_start_message_exact);
  RUN_TEST(test_repeat_uses_current_value_and_duration);
  RUN_TEST(test_recovery_and_system_messages_english);
  RUN_TEST(test_segment_counting);
  RUN_TEST(test_every_message_fits_one_segment_in_every_language);
  RUN_TEST(test_indian_language_messages_are_not_english);
  RUN_TEST(test_every_stage_and_language_has_a_name);
  RUN_TEST(test_device_name_rules);
  RUN_TEST(test_offline_summary_text_and_fit);
  return UNITY_END();
}
