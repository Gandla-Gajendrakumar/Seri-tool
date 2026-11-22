// Application-level tests: the whole device logic running on fake hardware.
// These mirror the phase-2 Wokwi scenarios so failures show up here first, in milliseconds.
#include <unity.h>
#include "../fakes/fakes.h"
#include "seri/monitor.h"
#include "seri/persist.h"
#include "seri/time_util.h"
using namespace seri;

static const int64_t kStart = fromIst(CivilTime{2026, 11, 21, 8, 0, 0});

struct Rig {
  fakes::Clock clock; fakes::Sensor sensor; fakes::Power power; fakes::Modem modem; fakes::Wifi wifi;
  fakes::Printer printer; fakes::Storage storage; fakes::Indicators ind; fakes::Persist persist;
  Monitor* m = nullptr;
  explicit Rig(bool withOwner = true) {
    clock.t = kStart;
    modem.clock = &clock;
    printer.clock = &clock;
    if (withOwner) {
      Settings s;
      s.phone[0] = "9876543210";
      s.stage = Stage::Chawki;  // 26-29 C, 75-95 %
      persist.kv["settings"] = encodeSettings(s);
    }
    boot();
  }
  ~Rig() { delete m; }
  void boot() {
    delete m;
    m = new Monitor(Hardware{clock, sensor, power, modem, wifi, printer, storage, ind, persist});
    m->begin();
  }
  // advance simulated time, ticking 10x per second of device time is overkill: 1 tick/second
  void run(int64_t seconds) {
    for (int64_t i = 0; i < seconds; ++i) { clock.t += 1; m->tick(); }
  }
  int smsContaining(const std::string& s) const {
    int n = 0;
    for (auto& x : modem.sent) if (x.text.find(s) != std::string::npos) ++n;
    return n;
  }
};

void setUp() {}
void tearDown() {}

static void test_normal_day_sends_no_sms_and_samples_every_30s() {
  Rig r;
  r.run(3600);
  TEST_ASSERT_EQUAL(0, r.modem.sent.size());
  TEST_ASSERT_TRUE(r.sensor.reads >= 595 && r.sensor.reads <= 605);  // every 6 s
  TEST_ASSERT_EQUAL(275, r.m->lastSample().t);
  TEST_ASSERT_TRUE(r.ind.g);
  TEST_ASSERT_FALSE(r.ind.r);
  TEST_ASSERT_FALSE(r.ind.buzz);
}

static void test_heat_spike_alerts_prints_logs_buzzes_then_recovers() {
  Rig r;
  r.run(60);
  r.sensor.t = 31.0f;
  r.run(4 * 60);
  TEST_ASSERT_EQUAL_MESSAGE(0, r.modem.sent.size(), "no alert before 5 minutes");
  r.run(2 * 60);
  TEST_ASSERT_EQUAL(1, r.modem.sent.size());
  TEST_ASSERT_EQUAL_STRING("9876543210", r.modem.sent[0].to.c_str());
  TEST_ASSERT_TRUE(r.modem.sent[0].text.find("Temp HIGH 31.0C") != std::string::npos);
  TEST_ASSERT_EQUAL(1, r.printer.count("TEMP HIGH 31.0C"));
  TEST_ASSERT_EQUAL(1, r.storage.count("TempHigh,Start"));
  TEST_ASSERT_EQUAL_MESSAGE(1, r.storage.count(",Start,TempHigh,0"), "policy decision logged: Q,<t>,Start,TempHigh,<slot>");
  TEST_ASSERT_TRUE(r.ind.buzz);
  TEST_ASSERT_TRUE(r.ind.r);
  r.m->silenceBuzzer();
  r.run(1);
  TEST_ASSERT_FALSE(r.ind.buzz);
  r.run(30 * 60);
  TEST_ASSERT_EQUAL_MESSAGE(1, r.smsContaining("STILL"), "one repeat after 30 min");
  r.sensor.t = 27.0f;
  r.run(2 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Temp normal again"));
  TEST_ASSERT_EQUAL(1, r.printer.count("TEMP OK"));
  TEST_ASSERT_FALSE(r.ind.r);
}

static void test_cold_night_low_temperature_alert() {
  Rig r;
  r.sensor.t = 24.0f;
  r.run(6 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Temp LOW 24.0C"));
}

static void test_power_cut_alerts_after_debounce_and_returns() {
  Rig r;
  r.power.mains = false;
  r.power.mv = 3900;
  r.run(90);
  TEST_ASSERT_EQUAL(0, r.modem.sent.size());
  r.run(60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Power cut, on battery"));
  TEST_ASSERT_TRUE(r.m->onBattery());
  TEST_ASSERT_EQUAL(1, r.printer.count("POWER CUT"));
  r.power.mains = true;
  r.run(3 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Power back"));
}

static void test_low_battery_alert_at_3_70_v_so_the_modem_can_still_send_it() {
  Rig r;
  r.power.mains = false;
  r.power.mv = 3750;
  r.run(5 * 60);
  TEST_ASSERT_EQUAL(0, r.smsContaining("Battery low"));
  r.power.mv = 3690;
  r.run(2 * 60);
  TEST_ASSERT_EQUAL_MESSAGE(1, r.smsContaining("Battery low"), "D-038: alert at <= 3.70 V");
}

static void test_modem_does_not_transmit_below_3_60_v_on_battery() {
  Rig r;
  r.power.mains = false;
  r.power.mv = 3550;  // below the modem cut-off (A7670 needs >= 3.4 V during 2 A bursts)
  r.sensor.t = 31.0f;
  int before = r.modem.sendAttempts;
  r.run(15 * 60);
  TEST_ASSERT_TRUE(r.m->active(EventType::TempHigh));
  TEST_ASSERT_EQUAL_MESSAGE(before, r.modem.sendAttempts, "no SMS attempts below cut-off");
  TEST_ASSERT_EQUAL(0, r.modem.published.size());
  TEST_ASSERT_TRUE_MESSAGE(r.printer.count("TEMP HIGH") == 1, "printing and logging continue");
  r.power.mains = true;  // mains back: queued alert goes out
  r.run(4 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Temp HIGH"));
}

static void test_on_battery_modem_waits_2_s_after_printing() {
  Rig r;
  r.power.mains = false;
  r.power.mv = 3900;
  r.run(10);
  TEST_ASSERT_TRUE_MESSAGE(r.printer.lowPower, "printer in low-power mode on battery (D-039)");
  r.sensor.t = 31.0f;
  r.run(8 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Temp HIGH"));
  int64_t printed = 0;
  for (size_t i = 0; i < r.printer.lines.size(); ++i)
    if (r.printer.lines[i].find("TEMP HIGH") != std::string::npos) printed = r.printer.times[i];
  int64_t sent = 0;
  for (auto& s : r.modem.sent) if (s.text.find("Temp HIGH") != std::string::npos) sent = s.at;
  TEST_ASSERT_TRUE_MESSAGE(sent - printed >= 2, "D-039: no modem burst while the printer heats");
  r.power.mains = true;
  r.run(5);
  TEST_ASSERT_FALSE(r.printer.lowPower);
}

static void test_on_battery_uploads_every_15_minutes_instead_of_5() {
  Rig r;
  r.run(30 * 60);
  size_t mains = r.modem.published.size();
  TEST_ASSERT_TRUE(mains >= 5 && mains <= 6);
  r.power.mains = false;
  r.run(60 * 60);
  size_t batt = r.modem.published.size() - mains;
  TEST_ASSERT_TRUE_MESSAGE(batt >= 3 && batt <= 5, "about 4 uploads in an hour on battery");
}

static void test_network_loss_retries_sms_and_queues_uploads_without_stopping_monitoring() {
  Rig r;
  r.modem.network = false;
  r.sensor.t = 31.0f;
  r.run(20 * 60);
  TEST_ASSERT_EQUAL(0, r.modem.sent.size());
  TEST_ASSERT_TRUE(r.m->active(EventType::TempHigh));
  TEST_ASSERT_TRUE_MESSAGE(r.modem.sendAttempts >= 3, "SMS retried with back-off");
  TEST_ASSERT_TRUE(r.storage.queue.size() >= 3);
  TEST_ASSERT_EQUAL(0, r.modem.published.size());
  r.modem.network = true;
  r.run(5 * 60);
  TEST_ASSERT_EQUAL_MESSAGE(1, r.smsContaining("Temp HIGH"), "delayed alert delivered once");
  TEST_ASSERT_EQUAL_MESSAGE(0, r.storage.queue.size(), "queued uploads flushed");
  TEST_ASSERT_TRUE(r.modem.published.size() >= 4);
}

static void test_sms_given_up_after_an_hour_is_printed() {
  Rig r;
  r.modem.network = false;
  r.sensor.t = 31.0f;
  r.run(75 * 60);
  TEST_ASSERT_TRUE(r.printer.count("SMS NOT SENT") >= 1);
}

static void test_long_outage_sends_one_offline_summary_when_network_returns() {
  Rig r;
  r.modem.network = false;
  r.sensor.t = 31.0f;
  r.run(30 * 60);
  r.sensor.t = 27.0f;  // problem over while still offline
  r.run(2 * 3600);
  TEST_ASSERT_EQUAL(0, r.modem.sent.size());
  TEST_ASSERT_TRUE_MESSAGE(r.printer.count("SMS NOT SENT") >= 1, "given up after 1 h, printed");
  r.modem.network = true;
  r.run(10 * 60);
  TEST_ASSERT_EQUAL_MESSAGE(1, r.smsContaining("No signal"), "exactly one offline summary");
  TEST_ASSERT_EQUAL(1, r.m->offlineSummariesSent());
  TEST_ASSERT_TRUE(r.smsContaining("alerts not sent. Now 27.0C 85%") == 1);
  r.run(3600);
  TEST_ASSERT_EQUAL_MESSAGE(1, r.smsContaining("No signal"), "not repeated");
}

static void test_short_outage_without_give_up_sends_no_offline_summary() {
  Rig r;
  r.modem.network = false;
  r.sensor.t = 31.0f;
  r.run(20 * 60);
  r.modem.network = true;
  r.run(10 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Temp HIGH"));
  TEST_ASSERT_EQUAL(0, r.smsContaining("No signal"));
}

static void test_sd_removed_keeps_monitoring_alerts_and_keeps_data_in_ram() {
  Rig r;
  r.run(10 * 60);
  r.storage.present = false;
  r.modem.data = false;  // also offline, so records must be held somewhere
  r.run(5 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Memory card fault"));
  TEST_ASSERT_EQUAL(1, r.printer.count("SD CARD FAULT"));
  r.sensor.t = 31.0f;
  r.run(10 * 60);
  TEST_ASSERT_EQUAL_MESSAGE(1, r.smsContaining("Temp HIGH"), "alerts still work without SD");
  TEST_ASSERT_TRUE(r.m->ramRecords() >= 2);
  r.storage.present = true;
  r.run(2 * 60);
  TEST_ASSERT_EQUAL(0, r.m->ramRecords());
  TEST_ASSERT_TRUE_MESSAGE(r.storage.count("TempHigh,Start") == 1, "RAM log flushed to SD");
  r.modem.data = true;
  r.run(6 * 60);
  TEST_ASSERT_EQUAL(0, r.storage.queue.size());
}

static void test_sensor_unplugged_alerts_and_recovers() {
  Rig r;
  r.run(60);
  r.sensor.connected = false;
  r.run(2 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Sensor fault"));
  TEST_ASSERT_TRUE(r.m->active(EventType::SensorFault));
  r.sensor.connected = true;
  r.run(3 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Sensor OK again"));
}

static void test_paper_out_queues_lines_and_prints_them_later() {
  Rig r;
  r.printer.paper = false;
  r.sensor.t = 31.0f;
  r.run(7 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Paper out, refill printer"));
  TEST_ASSERT_TRUE(r.m->printBacklog() >= 1);
  r.printer.paper = true;
  r.run(10);
  TEST_ASSERT_EQUAL(1, r.printer.count("TEMP HIGH"));
  TEST_ASSERT_EQUAL(0, r.m->printBacklog());
}

static void test_sms_command_changes_stage_replies_prints_and_persists() {
  Rig r;
  r.modem.inbox.push_back({"+919876543210", "11 5"});
  r.run(10);
  TEST_ASSERT_EQUAL(Stage::Instar5, r.m->settings().stage);
  TEST_ASSERT_EQUAL(1, r.smsContaining("OK - Instar 5"));
  TEST_ASSERT_EQUAL(1, r.printer.count("STAGE -> Instar 5"));
  Settings saved;
  TEST_ASSERT_TRUE(decodeSettings(r.persist.kv["settings"], saved));
  TEST_ASSERT_EQUAL(Stage::Instar5, saved.stage);
  // 27.5 C is now high for instar V (23-25) -> alert after 5 min
  r.run(6 * 60);
  TEST_ASSERT_EQUAL(1, r.smsContaining("Temp HIGH"));
}

static void test_stranger_sms_is_ignored() {
  Rig r;
  r.modem.inbox.push_back({"+917000000001", "11 5"});
  r.run(10);
  TEST_ASSERT_EQUAL(Stage::Chawki, r.m->settings().stage);
  TEST_ASSERT_EQUAL(0, r.modem.sent.size());
}

static void test_reboot_keeps_sms_cap_and_settings() {
  Rig r;
  r.sensor.t = 31.0f;
  r.run(5 * 3600);  // 1 start + 6 repeats + limit message used up
  int before = static_cast<int>(r.modem.sent.size());
  TEST_ASSERT_EQUAL(8, before);
  r.boot();  // power fully drained and restored, same day
  r.run(2 * 3600);
  TEST_ASSERT_EQUAL_MESSAGE(before, static_cast<int>(r.modem.sent.size()), "no new texts after reboot: cap remembered");
}

static void test_clock_corrected_from_network_only_when_off_by_more_than_2s() {
  Rig r;
  r.modem.netTimeOffset = 1;
  r.boot();
  TEST_ASSERT_EQUAL(0, r.clock.sets.size());
  r.modem.netTimeOffset = 10;
  r.run(86400 + 5);
  TEST_ASSERT_EQUAL(1, r.clock.sets.size());
  TEST_ASSERT_TRUE(r.storage.count("CLOCK,corrected") >= 1);
}

static void test_daily_summary_printed_at_6am() {
  Rig r;
  int64_t to6 = fromIst(CivilTime{2026, 11, 22, 6, 0, 30}) - r.clock.t;
  r.run(to6);
  TEST_ASSERT_EQUAL(1, r.printer.count("DAILY SUMMARY"));
  TEST_ASSERT_EQUAL_MESSAGE(1, r.storage.count(",SUMMARY,"), "summary recorded in the log (and trace)");
}

static void test_upload_record_contains_samples() {
  Rig r;
  r.run(5 * 60 + 5);
  TEST_ASSERT_EQUAL(1, r.modem.published.size());
  const std::string& p = r.modem.published[0].second;
  TEST_ASSERT_EQUAL_STRING("seri/Shed-1/data", r.modem.published[0].first.c_str());
  TEST_ASSERT_TRUE(p.find("\"d\":\"Shed-1\"") != std::string::npos);
  TEST_ASSERT_TRUE(p.find("275,850") != std::string::npos);
}

static void test_wifi_preferred_over_cellular_for_upload() {
  Rig r;
  r.wifi.up = true;
  r.run(5 * 60 + 5);
  TEST_ASSERT_EQUAL(1, r.wifi.published.size());
  TEST_ASSERT_EQUAL(0, r.modem.published.size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normal_day_sends_no_sms_and_samples_every_30s);
  RUN_TEST(test_heat_spike_alerts_prints_logs_buzzes_then_recovers);
  RUN_TEST(test_cold_night_low_temperature_alert);
  RUN_TEST(test_power_cut_alerts_after_debounce_and_returns);
  RUN_TEST(test_on_battery_uploads_every_15_minutes_instead_of_5);
  RUN_TEST(test_low_battery_alert_at_3_70_v_so_the_modem_can_still_send_it);
  RUN_TEST(test_modem_does_not_transmit_below_3_60_v_on_battery);
  RUN_TEST(test_on_battery_modem_waits_2_s_after_printing);
  RUN_TEST(test_network_loss_retries_sms_and_queues_uploads_without_stopping_monitoring);
  RUN_TEST(test_sms_given_up_after_an_hour_is_printed);
  RUN_TEST(test_long_outage_sends_one_offline_summary_when_network_returns);
  RUN_TEST(test_short_outage_without_give_up_sends_no_offline_summary);
  RUN_TEST(test_sd_removed_keeps_monitoring_alerts_and_keeps_data_in_ram);
  RUN_TEST(test_sensor_unplugged_alerts_and_recovers);
  RUN_TEST(test_paper_out_queues_lines_and_prints_them_later);
  RUN_TEST(test_sms_command_changes_stage_replies_prints_and_persists);
  RUN_TEST(test_stranger_sms_is_ignored);
  RUN_TEST(test_reboot_keeps_sms_cap_and_settings);
  RUN_TEST(test_clock_corrected_from_network_only_when_off_by_more_than_2s);
  RUN_TEST(test_daily_summary_printed_at_6am);
  RUN_TEST(test_upload_record_contains_samples);
  RUN_TEST(test_wifi_preferred_over_cellular_for_upload);
  return UNITY_END();
}
