#include <unity.h>
#include "seri/persist.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static Settings sample() {
  Settings s;
  s.phone[0] = "9876543210"; s.phone[2] = "9123456780";
  s.lang[0] = Language::Telugu; s.lang[2] = Language::Kannada;
  s.stage = Stage::Instar4; s.tol = Tolerance{15, 30}; s.buzzerOn = false;
  s.deviceName = "Shed-7"; s.pin = "482913"; s.wifiSsid = "FarmNet"; s.wifiPass = "p#ss w*rd";
  return s;
}

static void test_crc32_known_value() {
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926, crc32("123456789"));  // standard CRC-32 check value
}

static void test_settings_round_trip() {
  Settings in = sample(), out;
  TEST_ASSERT_TRUE(decodeSettings(encodeSettings(in), out));
  TEST_ASSERT_EQUAL_STRING("9876543210", out.phone[0].c_str());
  TEST_ASSERT_EQUAL_STRING("", out.phone[1].c_str());
  TEST_ASSERT_EQUAL_STRING("9123456780", out.phone[2].c_str());
  TEST_ASSERT_EQUAL(Language::Telugu, out.lang[0]);
  TEST_ASSERT_EQUAL(Language::Kannada, out.lang[2]);
  TEST_ASSERT_EQUAL(Stage::Instar4, out.stage);
  TEST_ASSERT_EQUAL(15, out.tol.t);
  TEST_ASSERT_EQUAL(30, out.tol.rh);
  TEST_ASSERT_FALSE(out.buzzerOn);
  TEST_ASSERT_EQUAL_STRING("Shed-7", out.deviceName.c_str());
  TEST_ASSERT_EQUAL_STRING("482913", out.pin.c_str());
  TEST_ASSERT_EQUAL_STRING("p#ss w*rd", out.wifiPass.c_str());
}

static void test_corrupt_or_foreign_blob_is_rejected_and_output_untouched() {
  std::string b = encodeSettings(sample());
  Settings out;
  out.deviceName = "keep";
  std::string flipped = b; flipped[b.size() / 2] ^= 0x01;
  TEST_ASSERT_FALSE(decodeSettings(flipped, out));
  TEST_ASSERT_FALSE(decodeSettings(b.substr(0, b.size() - 1), out));
  TEST_ASSERT_FALSE(decodeSettings("", out));
  std::string other = b; other[2] = 99;  // version byte
  TEST_ASSERT_FALSE(decodeSettings(other, out));
  TEST_ASSERT_EQUAL_STRING("keep", out.deviceName.c_str());
}

static void test_out_of_range_values_are_rejected() {
  Settings bad = sample();
  bad.stage = static_cast<Stage>(42);
  Settings out;
  TEST_ASSERT_FALSE(decodeSettings(encodeSettings(bad), out));
}

static void test_alert_state_round_trip() {
  AlertState a;
  a.day = 20413;
  a.r[1].repeatsToday = 6; a.r[1].totalToday = 17; a.r[1].reserved = 2; a.r[1].limitNotified = true;
  auto& ep = a.ep[static_cast<int>(EventType::RhHigh)];
  ep.active = true; ep.haveEnded = true; ep.start = 1795219200; ep.lastRepeat = 1795221000;
  ep.lastEnd = 1795210000; ep.lastValue = 965; ep.toldMask = 0b101;
  AlertState out;
  TEST_ASSERT_TRUE(decodeAlertState(encodeAlertState(a), out));
  TEST_ASSERT_EQUAL_INT64(20413, out.day);
  TEST_ASSERT_EQUAL(6, out.r[1].repeatsToday);
  TEST_ASSERT_EQUAL(17, out.r[1].totalToday);
  TEST_ASSERT_EQUAL(2, out.r[1].reserved);
  TEST_ASSERT_TRUE(out.r[1].limitNotified);
  const auto& o = out.ep[static_cast<int>(EventType::RhHigh)];
  TEST_ASSERT_TRUE(o.active);
  TEST_ASSERT_EQUAL_INT64(1795221000, o.lastRepeat);
  TEST_ASSERT_EQUAL_INT64(965, o.lastValue);
  TEST_ASSERT_EQUAL(0b101, o.toldMask);
  std::string b = encodeAlertState(a);
  b[5] ^= 0x10;
  TEST_ASSERT_FALSE(decodeAlertState(b, out));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_known_value);
  RUN_TEST(test_settings_round_trip);
  RUN_TEST(test_corrupt_or_foreign_blob_is_rejected_and_output_untouched);
  RUN_TEST(test_out_of_range_values_are_rejected);
  RUN_TEST(test_alert_state_round_trip);
  return UNITY_END();
}
