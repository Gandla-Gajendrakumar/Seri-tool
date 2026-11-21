#include <unity.h>
#include "seri/events.h"
using namespace seri;
void setUp() {}
void tearDown() {}

// Chawki band with default tolerance: 26.0-29.0 C, 75-95 %
static Band chawki() { return bandFor(Stage::Chawki, Tolerance{}); }
static Sample S(Deci t, Deci rh) { return Sample{true, t, rh}; }
static const Sample kInvalid{false, 0, 0};

struct Rig {
  EventEngine eng{chawki()};
  std::vector<Event> ev;
  int64_t now = 1795219200;
  // feed one 30 s sample, return events produced by it
  std::vector<Event>& sample(Sample s) { ev.clear(); eng.onSample(now, s, ev); now += 30; return ev; }
};

static void test_temp_high_starts_after_exactly_5_minutes_out() {
  Rig r;
  for (int i = 0; i < 10; ++i) TEST_ASSERT_EQUAL_MESSAGE(0, r.sample(S(295, 850)).size(), "too early");
  auto& e = r.sample(S(296, 850));  // 11th sample: 300 s after the first out-of-band one
  TEST_ASSERT_EQUAL(1, e.size());
  TEST_ASSERT_EQUAL(EventType::TempHigh, e[0].type);
  TEST_ASSERT_EQUAL(Edge::Start, e[0].edge);
  TEST_ASSERT_EQUAL(296, e[0].value);
  TEST_ASSERT_TRUE(r.eng.active(EventType::TempHigh));
}

static void test_value_on_band_edge_is_not_out_of_band() {
  Rig r;
  for (int i = 0; i < 20; ++i) TEST_ASSERT_EQUAL(0, r.sample(S(290, 950)).size());
}

static void test_one_in_band_sample_restarts_the_5_minute_timer() {
  Rig r;
  for (int i = 0; i < 9; ++i) r.sample(S(300, 850));
  r.sample(S(285, 850));  // back in band once
  for (int i = 0; i < 10; ++i) TEST_ASSERT_EQUAL(0, r.sample(S(300, 850)).size());
  TEST_ASSERT_EQUAL(1, r.sample(S(300, 850)).size());
}

static void test_invalid_sample_neither_resets_nor_confirms() {
  Rig r;
  for (int i = 0; i < 5; ++i) r.sample(S(300, 850));  // 0..120 s
  TEST_ASSERT_EQUAL(0, r.sample(kInvalid).size());    // 150 s
  for (int i = 0; i < 4; ++i) TEST_ASSERT_EQUAL(0, r.sample(S(300, 850)).size());  // 180..270
  TEST_ASSERT_EQUAL(1, r.sample(S(300, 850)).size());  // 300 s
}

static void start_temp_high(Rig& r) {
  for (int i = 0; i < 11; ++i) r.sample(S(305, 850));
  TEST_ASSERT_TRUE(r.eng.active(EventType::TempHigh));
}

static void test_clear_needs_half_degree_hysteresis() {
  Rig r;
  start_temp_high(r);
  for (int i = 0; i < 10; ++i) TEST_ASSERT_EQUAL_MESSAGE(0, r.sample(S(286, 850)).size(), "28.6 is inside band but not 0.5 below 29.0");
  r.sample(S(285, 850));
  auto& e = r.sample(S(285, 850));  // second consecutive sample at 28.5
  TEST_ASSERT_EQUAL(1, e.size());
  TEST_ASSERT_EQUAL(Edge::Clear, e[0].edge);
  TEST_ASSERT_EQUAL(EventType::TempHigh, e[0].type);
  TEST_ASSERT_FALSE(r.eng.active(EventType::TempHigh));
}

static void test_single_good_sample_does_not_clear() {
  Rig r;
  start_temp_high(r);
  r.sample(S(280, 850));
  TEST_ASSERT_EQUAL(0, r.sample(S(300, 850)).size());
  TEST_ASSERT_TRUE(r.eng.active(EventType::TempHigh));
}

static void test_temp_low_symmetric() {
  Rig r;
  for (int i = 0; i < 10; ++i) r.sample(S(255, 850));
  auto& e = r.sample(S(255, 850));
  TEST_ASSERT_EQUAL(1, e.size());
  TEST_ASSERT_EQUAL(EventType::TempLow, e[0].type);
  r.sample(S(264, 850));  // above 26.0 but not by 0.5
  TEST_ASSERT_EQUAL(0, r.sample(S(264, 850)).size());
  r.sample(S(265, 850));
  TEST_ASSERT_EQUAL(Edge::Clear, r.sample(S(265, 850))[0].edge);
}

static void test_rh_high_and_low_with_2_percent_hysteresis() {
  Rig r;
  for (int i = 0; i < 11; ++i) r.sample(S(275, 960));
  TEST_ASSERT_TRUE(r.eng.active(EventType::RhHigh));
  r.sample(S(275, 940)); r.sample(S(275, 940));
  TEST_ASSERT_TRUE_MESSAGE(r.eng.active(EventType::RhHigh), "94 % is not 2 % below 95");
  r.sample(S(275, 930));
  TEST_ASSERT_EQUAL(Edge::Clear, r.sample(S(275, 930))[0].edge);
  for (int i = 0; i < 11; ++i) r.sample(S(275, 700));
  TEST_ASSERT_TRUE(r.eng.active(EventType::RhLow));
}

static void test_temp_and_rh_events_are_independent() {
  Rig r;
  for (int i = 0; i < 10; ++i) r.sample(S(300, 970));
  auto& e = r.sample(S(300, 970));
  TEST_ASSERT_EQUAL(2, e.size());
}

static void test_sensor_fault_after_3_invalid_samples_and_clears_after_3_valid() {
  Rig r;
  TEST_ASSERT_EQUAL(0, r.sample(kInvalid).size());
  TEST_ASSERT_EQUAL(0, r.sample(kInvalid).size());
  auto& e = r.sample(kInvalid);
  TEST_ASSERT_EQUAL(1, e.size());
  TEST_ASSERT_EQUAL(EventType::SensorFault, e[0].type);
  TEST_ASSERT_EQUAL(Edge::Start, e[0].edge);
  r.sample(S(275, 850)); r.sample(S(275, 850));
  TEST_ASSERT_TRUE(r.eng.active(EventType::SensorFault));
  auto& c = r.sample(S(275, 850));
  TEST_ASSERT_EQUAL(1, c.size());
  TEST_ASSERT_EQUAL(Edge::Clear, c[0].edge);
}

static void test_mains_flicker_is_ignored_and_real_cut_is_reported() {
  EventEngine eng(chawki());
  std::vector<Event> ev;
  int64_t t = 1000;
  eng.onMains(t, false, ev); eng.onMains(t + 60, false, ev); eng.onMains(t + 90, true, ev);
  TEST_ASSERT_EQUAL_MESSAGE(0, ev.size(), "90 s flicker must not alert");
  t = 2000;
  eng.onMains(t, false, ev); eng.onMains(t + 119, false, ev);
  TEST_ASSERT_EQUAL(0, ev.size());
  eng.onMains(t + 120, false, ev);
  TEST_ASSERT_EQUAL(1, ev.size());
  TEST_ASSERT_EQUAL(EventType::MainsFail, ev[0].type);
  ev.clear();
  eng.onMains(t + 500, true, ev); eng.onMains(t + 560, false, ev);  // short return
  eng.onMains(t + 600, true, ev); eng.onMains(t + 719, true, ev);
  TEST_ASSERT_EQUAL(0, ev.size());
  eng.onMains(t + 720, true, ev);
  TEST_ASSERT_EQUAL(1, ev.size());
  TEST_ASSERT_EQUAL(Edge::Clear, ev[0].edge);  // = "mains return"
}

static void test_low_battery_only_on_battery_and_ignores_short_sag() {
  EventEngine eng(chawki());
  std::vector<Event> ev;
  eng.onBattery(0, 3300, false, ev); eng.onBattery(100, 3300, false, ev);
  TEST_ASSERT_EQUAL_MESSAGE(0, ev.size(), "charging on mains is not an alert");
  eng.onBattery(200, 3350, true, ev); eng.onBattery(230, 3800, true, ev);  // modem-burst sag
  eng.onBattery(260, 3390, true, ev); eng.onBattery(319, 3390, true, ev);
  TEST_ASSERT_EQUAL(0, ev.size());
  eng.onBattery(320, 3380, true, ev);
  TEST_ASSERT_EQUAL(1, ev.size());
  TEST_ASSERT_EQUAL(EventType::LowBattery, ev[0].type);
  TEST_ASSERT_EQUAL(3380, ev[0].value);
  ev.clear();
  eng.onBattery(400, 3500, false, ev);  // mains back: clears
  TEST_ASSERT_EQUAL(1, ev.size());
  TEST_ASSERT_EQUAL(Edge::Clear, ev[0].edge);
}

static void test_paper_sd_and_clock_events() {
  EventEngine eng(chawki());
  std::vector<Event> ev;
  eng.onPaper(0, false, ev);
  TEST_ASSERT_EQUAL(EventType::PaperOut, ev.at(0).type);
  eng.onPaper(10, true, ev);
  TEST_ASSERT_EQUAL(Edge::Clear, ev.at(1).edge);
  ev.clear();
  eng.onSd(0, false, ev);
  TEST_ASSERT_EQUAL_MESSAGE(0, ev.size(), "one failed check is not enough");
  eng.onSd(30, false, ev);
  TEST_ASSERT_EQUAL(EventType::SdFault, ev.at(0).type);
  eng.onSd(60, true, ev);
  TEST_ASSERT_EQUAL(Edge::Clear, ev.at(1).edge);
  ev.clear();
  eng.onClock(0, false, ev);
  TEST_ASSERT_EQUAL(EventType::ClockInvalid, ev.at(0).type);
}

static void test_stage_change_silently_clears_event_now_in_band() {
  Rig r;
  for (int i = 0; i < 11; ++i) r.sample(S(255, 850));  // TempLow in chawki (26-29)
  TEST_ASSERT_TRUE(r.eng.active(EventType::TempLow));
  r.ev.clear();
  r.eng.setBand(r.now, bandFor(Stage::Instar4, Tolerance{}), r.ev);  // 24-26 C, 65-75 %
  // 25.5 is fine for instar IV -> TempLow silently cleared.
  // 85 % is above instar IV's 75 % -> that is a new condition and must wait 5 min, not alert now.
  TEST_ASSERT_EQUAL(1, r.ev.size());
  TEST_ASSERT_EQUAL(EventType::TempLow, r.ev[0].type);
  TEST_ASSERT_EQUAL(Edge::SilentClear, r.ev[0].edge);
  TEST_ASSERT_FALSE(r.eng.active(EventType::TempLow));
}

static void test_stage_change_keeps_event_still_out_of_band() {
  Rig r;
  for (int i = 0; i < 11; ++i) r.sample(S(310, 850));
  r.ev.clear();
  r.eng.setBand(r.now, bandFor(Stage::Instar3, Tolerance{}), r.ev);  // 25-27 C: 31 still high
  TEST_ASSERT_EQUAL(0, r.ev.size());
  TEST_ASSERT_TRUE(r.eng.active(EventType::TempHigh));
}

static void test_stage_change_restarts_pending_onset_timer() {
  Rig r;
  for (int i = 0; i < 9; ++i) r.sample(S(300, 850));  // 4 min high, not yet an event
  r.eng.setBand(r.now, bandFor(Stage::Instar3, Tolerance{}), r.ev);  // still high in new band
  for (int i = 0; i < 10; ++i) TEST_ASSERT_EQUAL(0, r.sample(S(300, 850)).size());
  TEST_ASSERT_EQUAL(1, r.sample(S(300, 850)).size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_temp_high_starts_after_exactly_5_minutes_out);
  RUN_TEST(test_value_on_band_edge_is_not_out_of_band);
  RUN_TEST(test_one_in_band_sample_restarts_the_5_minute_timer);
  RUN_TEST(test_invalid_sample_neither_resets_nor_confirms);
  RUN_TEST(test_clear_needs_half_degree_hysteresis);
  RUN_TEST(test_single_good_sample_does_not_clear);
  RUN_TEST(test_temp_low_symmetric);
  RUN_TEST(test_rh_high_and_low_with_2_percent_hysteresis);
  RUN_TEST(test_temp_and_rh_events_are_independent);
  RUN_TEST(test_sensor_fault_after_3_invalid_samples_and_clears_after_3_valid);
  RUN_TEST(test_mains_flicker_is_ignored_and_real_cut_is_reported);
  RUN_TEST(test_low_battery_only_on_battery_and_ignores_short_sag);
  RUN_TEST(test_paper_sd_and_clock_events);
  RUN_TEST(test_stage_change_silently_clears_event_now_in_band);
  RUN_TEST(test_stage_change_keeps_event_still_out_of_band);
  RUN_TEST(test_stage_change_restarts_pending_onset_timer);
  return UNITY_END();
}
