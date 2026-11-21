#include <unity.h>
#include <map>
#include "seri/alerts.h"
#include "seri/time_util.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static const int64_t kDay0 = fromIst(CivilTime{2026, 11, 21, 8, 0, 0});  // 08:00 IST
static Event start(EventType t, int64_t at) { return Event{t, Edge::Start, at, 300}; }
static Event clear(EventType t, int64_t at) { return Event{t, Edge::Clear, at, 280}; }

static int count(const std::vector<SmsJob>& v, SmsKind k, int rcpt = -1) {
  int n = 0;
  for (auto& j : v) if (j.kind == k && (rcpt < 0 || j.recipient == rcpt)) ++n;
  return n;
}
// run ticks every 60 s from t0 to t1 (exclusive)
static void ticks(AlertPolicy& p, int64_t t0, int64_t t1, uint8_t m, std::vector<SmsJob>& out) {
  for (int64_t t = t0; t < t1; t += 60) p.tick(t, m, out);
}

static void test_start_texts_every_registered_number_once() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempHigh, kDay0), 0b101, out);
  TEST_ASSERT_EQUAL(2, out.size());
  TEST_ASSERT_EQUAL(0, out[0].recipient);
  TEST_ASSERT_EQUAL(2, out[1].recipient);
  TEST_ASSERT_EQUAL(SmsKind::Start, out[0].kind);
}

static void test_repeat_every_30_minutes_while_active() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempHigh, kDay0), 1, out);
  out.clear();
  p.tick(kDay0 + 1799, 1, out);
  TEST_ASSERT_EQUAL(0, out.size());
  p.tick(kDay0 + 1800, 1, out);
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Repeat));
  TEST_ASSERT_EQUAL(kDay0, out[0].startedAt);
  p.tick(kDay0 + 1860, 1, out);
  TEST_ASSERT_EQUAL(1, out.size());
}

static void test_repeat_cap_then_one_limit_message_then_silence() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempHigh, kDay0), 1, out);
  ticks(p, kDay0, kDay0 + 10 * 3600, 1, out);  // 10 h of trouble (08:00-18:00)
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Start));
  TEST_ASSERT_EQUAL(6, count(out, SmsKind::Repeat));
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::LimitReached));
}

static void test_recovery_is_sent_even_after_cap() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempHigh, kDay0), 1, out);
  ticks(p, kDay0, kDay0 + 10 * 3600, 1, out);
  out.clear();
  p.onEvent(clear(EventType::TempHigh, kDay0 + 10 * 3600), 1, out);
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Recovery));
  TEST_ASSERT_EQUAL(EventType::TempHigh, out[0].type);
}

static void test_cap_resets_at_ist_midnight() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  const int64_t evening = fromIst(CivilTime{2026, 11, 21, 18, 0, 0});
  p.onEvent(start(EventType::TempLow, evening), 1, out);
  ticks(p, evening, fromIst(CivilTime{2026, 11, 22, 0, 0, 0}), 1, out);
  TEST_ASSERT_EQUAL(6, count(out, SmsKind::Repeat));
  out.clear();
  ticks(p, fromIst(CivilTime{2026, 11, 22, 0, 0, 0}), fromIst(CivilTime{2026, 11, 22, 1, 1, 0}), 1, out);
  // schedule keeps running while suppressed (..23:00, 23:30) so new-day repeats fall at 00:00, 00:30, 01:00
  TEST_ASSERT_EQUAL_MESSAGE(3, count(out, SmsKind::Repeat), "new day: repeats resume");
}

static void test_mains_fail_has_no_repeats_but_has_power_back_text() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::MainsFail, kDay0), 1, out);
  ticks(p, kDay0, kDay0 + 8 * 3600, 1, out);
  TEST_ASSERT_EQUAL(1, out.size());
  p.onEvent(clear(EventType::MainsFail, kDay0 + 8 * 3600), 1, out);
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Recovery));
}

static void test_paper_out_texts_once_and_no_recovery_text() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::PaperOut, kDay0), 1, out);
  ticks(p, kDay0, kDay0 + 3 * 3600, 1, out);
  p.onEvent(clear(EventType::PaperOut, kDay0 + 3 * 3600), 1, out);
  TEST_ASSERT_EQUAL(1, out.size());
  TEST_ASSERT_EQUAL(SmsKind::Start, out[0].kind);
}

static void test_silent_clear_sends_nothing_and_stops_repeats() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempLow, kDay0), 1, out);
  out.clear();
  TEST_ASSERT_EQUAL(0, out.size());
  p.tick(kDay0 + 1800, 1, out);
  TEST_ASSERT_EQUAL_MESSAGE(1, out.size(), "sanity: repeats run before the silent clear");
  out.clear();
  p.onEvent(Event{EventType::TempLow, Edge::SilentClear, kDay0 + 600, 0}, 1, out);
  ticks(p, kDay0 + 600, kDay0 + 4 * 3600, 1, out);
  TEST_ASSERT_EQUAL(0, out.size());
}

static void test_recurrence_within_hour_counts_against_cap() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  int64_t t = kDay0;
  p.onEvent(start(EventType::TempHigh, t), 1, out);
  p.onEvent(clear(EventType::TempHigh, t + 600), 1, out);
  p.onEvent(start(EventType::TempHigh, t + 1200), 1, out);  // back 10 min later
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Start));
  TEST_ASSERT_EQUAL_MESSAGE(1, count(out, SmsKind::Repeat), "recurrence is sent as a repeat");
  // 61 min after the end it's a fresh event again
  p.onEvent(clear(EventType::TempHigh, t + 1800), 1, out);
  p.onEvent(start(EventType::TempHigh, t + 1800 + 3660), 1, out);
  TEST_ASSERT_EQUAL(2, count(out, SmsKind::Start));
}

static void test_flapping_all_day_never_exceeds_hard_cap_and_recoveries_match() {
  // Worst case: temperature flaps every 6 min AND rh flaps AND mains flickers past debounce,
  // all day. Per number, per day: total <= 20, and every told episode gets its recovery.
  AlertPolicy p;
  std::vector<SmsJob> out;
  const int64_t d0 = fromIst(CivilTime{2026, 11, 21, 0, 0, 0});
  std::map<int64_t, int> perDay;
  int told = 0, recovered = 0;
  for (int64_t t = d0; t < d0 + 3 * 86400; t += 360) {
    std::vector<SmsJob> o;
    for (EventType ty : {EventType::TempHigh, EventType::RhLow, EventType::MainsFail, EventType::SensorFault}) {
      p.onEvent(start(ty, t), 0b111, o);
      p.tick(t + 60, 0b111, o);
      p.onEvent(clear(ty, t + 300), 0b111, o);
    }
    for (auto& j : o) {
      if (j.recipient == 1) {
        perDay[istDay(j.time)]++;
        if (j.kind == SmsKind::Start || j.kind == SmsKind::Repeat) told++;
        if (j.kind == SmsKind::Recovery) recovered++;
      }
    }
    out.insert(out.end(), o.begin(), o.end());
  }
  for (auto& kv : perDay) TEST_ASSERT_TRUE_MESSAGE(kv.second <= 20, "hard cap exceeded");
  TEST_ASSERT_EQUAL_MESSAGE(told, recovered, "every told episode must get its recovery text");
  TEST_ASSERT_TRUE(told > 0);
}

static void test_state_survives_reboot_so_cap_is_not_reset() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempHigh, kDay0), 1, out);
  ticks(p, kDay0, kDay0 + 5 * 3600, 1, out);  // 10 repeats due: 6 sent + limit message
  TEST_ASSERT_EQUAL(6, count(out, SmsKind::Repeat));
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::LimitReached));
  AlertPolicy rebooted(AlertConfig{}, p.state());  // state restored from flash
  out.clear();
  ticks(rebooted, kDay0 + 5 * 3600, kDay0 + 10 * 3600, 1, out);
  TEST_ASSERT_EQUAL_MESSAGE(0, count(out, SmsKind::Repeat), "cap must survive reboot");
  TEST_ASSERT_EQUAL_MESSAGE(0, count(out, SmsKind::LimitReached), "limit message only once per day");
}

static void test_number_added_mid_event_gets_repeats_and_recovery_only_if_told() {
  AlertPolicy p;
  std::vector<SmsJob> out;
  p.onEvent(start(EventType::TempHigh, kDay0), 0b001, out);
  out.clear();
  p.tick(kDay0 + 1800, 0b011, out);  // slot 1 registered meanwhile
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Repeat, 1));
  p.onEvent(clear(EventType::TempHigh, kDay0 + 2000), 0b111, out);  // slot 2 never told
  TEST_ASSERT_EQUAL(1, count(out, SmsKind::Recovery, 1));
  TEST_ASSERT_EQUAL(0, count(out, SmsKind::Recovery, 2));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_start_texts_every_registered_number_once);
  RUN_TEST(test_repeat_every_30_minutes_while_active);
  RUN_TEST(test_repeat_cap_then_one_limit_message_then_silence);
  RUN_TEST(test_recovery_is_sent_even_after_cap);
  RUN_TEST(test_cap_resets_at_ist_midnight);
  RUN_TEST(test_mains_fail_has_no_repeats_but_has_power_back_text);
  RUN_TEST(test_paper_out_texts_once_and_no_recovery_text);
  RUN_TEST(test_silent_clear_sends_nothing_and_stops_repeats);
  RUN_TEST(test_recurrence_within_hour_counts_against_cap);
  RUN_TEST(test_flapping_all_day_never_exceeds_hard_cap_and_recoveries_match);
  RUN_TEST(test_state_survives_reboot_so_cap_is_not_reset);
  RUN_TEST(test_number_added_mid_event_gets_repeats_and_recovery_only_if_told);
  return UNITY_END();
}
