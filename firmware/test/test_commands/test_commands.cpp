#include <unity.h>
#include "seri/commands.h"
#include "seri/sms_text.h"
#include "seri/time_util.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static const char* kOwner = "+919876543210";
static const char* kHelper = "9123456780";
static const char* kStranger = "+917000000001";

static Settings fresh() {
  Settings s;
  s.pin = "482913";
  s.phone[0] = "9876543210";
  return s;
}
static StatusInfo status() {
  return StatusInfo{Sample{true, 275, 850}, false, 100, fromIst(CivilTime{2026, 11, 21, 14, 5, 0})};
}

static void test_normalize_indian_numbers() {
  TEST_ASSERT_EQUAL_STRING("9876543210", normalizeNumber("+919876543210").c_str());
  TEST_ASSERT_EQUAL_STRING("9876543210", normalizeNumber("919876543210").c_str());
  TEST_ASSERT_EQUAL_STRING("9876543210", normalizeNumber("09876543210").c_str());
  TEST_ASSERT_EQUAL_STRING("9876543210", normalizeNumber("+91 98765-43210").c_str());
  TEST_ASSERT_EQUAL_STRING("", normalizeNumber("5876543210").c_str());   // mobiles start 6-9
  TEST_ASSERT_EQUAL_STRING("", normalizeNumber("987654321").c_str());    // too short
  TEST_ASSERT_EQUAL_STRING("", normalizeNumber("+449876543210").c_str()); // not India
  TEST_ASSERT_EQUAL_STRING("", normalizeNumber("VK-JIOINF").c_str());     // operator sender id
}

static void test_parse_commands() {
  Command c = parseCommand("11 4");
  TEST_ASSERT_EQUAL(CmdType::SetStage, c.type);
  TEST_ASSERT_EQUAL(4, c.a);
  c = parseCommand(" 13*15#3 ");
  TEST_ASSERT_EQUAL(CmdType::SetTolerance, c.type);
  TEST_ASSERT_EQUAL(15, c.a);
  TEST_ASSERT_EQUAL(3, c.b);
  TEST_ASSERT_EQUAL(CmdType::Status, parseCommand("20").type);
  TEST_ASSERT_EQUAL(6, parseCommand("11 6").a);  // 6 = spinning
  c = parseCommand("21 +91 91234 56780");
  TEST_ASSERT_EQUAL(CmdType::AddNumber, c.type);
  TEST_ASSERT_EQUAL_STRING("9123456780", c.s1.c_str());
  c = parseCommand("30 FarmNet my pass 1");
  TEST_ASSERT_EQUAL(CmdType::SetWifi, c.type);
  TEST_ASSERT_EQUAL_STRING("FarmNet", c.s1.c_str());
  TEST_ASSERT_EQUAL_STRING("my pass 1", c.s2.c_str());  // password may contain spaces
  c = parseCommand("10 482913");
  TEST_ASSERT_EQUAL(CmdType::ClaimOwner, c.type);
  TEST_ASSERT_EQUAL_STRING("482913", c.s1.c_str());
}

static void test_parse_rejects_bad_input() {
  const char* bad[] = {"", "hello", "11", "11 7", "11 0", "11 x", "11 3 4", "12 5", "13 4 5", "13 10 16",
                       "13 10", "22 0", "22 3", "41 2", "99", "21 12345", "30", "30 onlyname", "10 12345"};
  for (const char* b : bad) TEST_ASSERT_EQUAL_MESSAGE(CmdType::Invalid, parseCommand(b).type, b);
}

static void test_stranger_is_ignored_silently() {
  Settings s = fresh();
  CmdOutcome o = applyCommand(s, kStranger, "11 5");
  TEST_ASSERT_EQUAL(CmdResult::Ignored, o.result);
  TEST_ASSERT_EQUAL(Stage::Chawki, s.stage);
  TEST_ASSERT_EQUAL_STRING("", formatReply(o, s, status()).c_str());
}

static void test_member_sets_stage_and_gets_confirmation() {
  Settings s = fresh();
  CmdOutcome o = applyCommand(s, kOwner, "11 4");
  TEST_ASSERT_EQUAL(CmdResult::Ok, o.result);
  TEST_ASSERT_EQUAL(Stage::Instar4, s.stage);
  TEST_ASSERT_EQUAL_STRING("Shed-1: OK - Instar 4", formatReply(o, s, status()).c_str());
}

static void test_tolerance_and_buzzer() {
  Settings s = fresh();
  applyCommand(s, kOwner, "13 15 3");
  TEST_ASSERT_EQUAL(15, s.tol.t);
  TEST_ASSERT_EQUAL(30, s.tol.rh);
  CmdOutcome o = applyCommand(s, kOwner, "41 0");
  TEST_ASSERT_FALSE(s.buzzerOn);
  TEST_ASSERT_EQUAL(CmdResult::Ok, o.result);
}

static void test_only_owner_manages_numbers_and_wifi() {
  Settings s = fresh();
  CmdOutcome added = applyCommand(s, kOwner, "21 9123456780");
  TEST_ASSERT_EQUAL(CmdResult::Ok, added.result);
  TEST_ASSERT_EQUAL_STRING("Shed-1: OK - 9123456780", formatReply(added, s, status()).c_str());
  TEST_ASSERT_EQUAL(1, slotOf(s, kHelper));
  TEST_ASSERT_EQUAL(0b011, recipientMask(s));
  CmdOutcome o = applyCommand(s, kHelper, "21 9000000009");
  TEST_ASSERT_EQUAL(CmdResult::NotAllowed, o.result);
  TEST_ASSERT_EQUAL_STRING("Shed-1: Only the owner number can do this.", formatReply(o, s, status()).c_str());
  TEST_ASSERT_EQUAL(CmdResult::NotAllowed, applyCommand(s, kHelper, "30 Net pw").result);
  TEST_ASSERT_EQUAL(CmdResult::Ok, applyCommand(s, kHelper, "11 3").result);  // helper may set stage
  TEST_ASSERT_EQUAL(CmdResult::Ok, applyCommand(s, kOwner, "21 9000000009").result);
  TEST_ASSERT_EQUAL(CmdResult::Full, applyCommand(s, kOwner, "21 9000000008").result);
  TEST_ASSERT_EQUAL(CmdResult::Ok, applyCommand(s, kOwner, "22 1").result);
  TEST_ASSERT_EQUAL(-1, slotOf(s, kHelper));
  TEST_ASSERT_EQUAL(CmdResult::Ok, applyCommand(s, kOwner, "30 FarmNet secret").result);
  TEST_ASSERT_EQUAL_STRING("FarmNet", s.wifiSsid.c_str());
}

static void test_adding_existing_number_is_not_duplicated() {
  Settings s = fresh();
  applyCommand(s, kOwner, "21 9123456780");
  TEST_ASSERT_EQUAL(CmdResult::Ok, applyCommand(s, kOwner, "21 9123456780").result);
  TEST_ASSERT_EQUAL(0b011, recipientMask(s));
}

static void test_language_is_per_number() {
  Settings s = fresh();
  applyCommand(s, kOwner, "21 9123456780");
  CmdOutcome o = applyCommand(s, kHelper, "12 3");
  TEST_ASSERT_EQUAL(Language::Telugu, s.lang[1]);
  TEST_ASSERT_EQUAL(Language::English, s.lang[0]);
  TEST_ASSERT_EQUAL_STRING("Shed-1: OK - తెలుగు", formatReply(o, s, status()).c_str());
}

static void test_owner_claim_with_pin_and_lockout() {
  Settings s = fresh();
  for (int i = 0; i < 5; ++i) TEST_ASSERT_EQUAL(CmdResult::Ignored, applyCommand(s, kStranger, "10 111111").result);
  TEST_ASSERT_EQUAL_MESSAGE(CmdResult::Ignored, applyCommand(s, kStranger, "10 482913").result,
                            "locked after 5 wrong PINs");
  s.pinFailuresToday = 0;  // next day
  CmdOutcome o = applyCommand(s, kStranger, "10 482913");
  TEST_ASSERT_EQUAL(CmdResult::Ok, o.result);
  TEST_ASSERT_EQUAL(0, slotOf(s, kStranger));  // new owner (e.g. owner's phone lost)
  TEST_ASSERT_EQUAL(-1, slotOf(s, kOwner));
}

static void test_bad_code_from_member_gets_help_reply_in_their_language() {
  Settings s = fresh();
  s.lang[0] = Language::Hindi;
  CmdOutcome o = applyCommand(s, kOwner, "77");
  TEST_ASSERT_EQUAL(CmdResult::BadCode, o.result);
  TEST_ASSERT_EQUAL_STRING("Shed-1: कोड समझ नहीं आया। कार्ड देखें।", formatReply(o, s, status()).c_str());
}

static void test_status_reply() {
  Settings s = fresh();
  CmdOutcome o = applyCommand(s, kOwner, "20");
  TEST_ASSERT_EQUAL_STRING("Shed-1: 27.5C 85% Chawki 14:05", formatReply(o, s, status()).c_str());
  StatusInfo st = status();
  st.onBattery = true;
  st.batteryPct = 64;
  TEST_ASSERT_EQUAL_STRING("Shed-1: 27.5C 85% Chawki Power cut, on battery 64% 14:05", formatReply(o, s, st).c_str());
  st.sample.valid = false;
  TEST_ASSERT_EQUAL_STRING("Shed-1: Sensor fault, check probe cable Chawki Power cut, on battery 64% 14:05",
                           formatReply(o, s, st).c_str());
}

static void test_every_reply_fits_one_sms_in_every_language() {
  const char* cmds[] = {"11 1", "12 1", "13 30 15", "20", "21 9123456780", "22 1", "30 a b", "41 1", "77", "10 482913"};
  for (int l = 0; l < kLanguageCount; ++l)
    for (const char* c : cmds)
      for (bool batt : {false, true}) {
        Settings s = fresh();
        s.deviceName = "ABCDEFGHIJ";
        s.lang[0] = static_cast<Language>(l);
        s.phone[1] = "9000000001"; s.phone[2] = "9000000002";  // so 21 hits "Full"
        StatusInfo st = status();
        st.onBattery = batt;
        st.sample = Sample{true, -105, 1000};
        if (c[0] == '1' && c[1] == '2') continue;  // would change the language under test
        CmdOutcome o = applyCommand(s, kOwner, c);
        std::string r = formatReply(o, s, st);
        TEST_ASSERT_TRUE_MESSAGE(!r.empty(), c);
        TEST_ASSERT_EQUAL_MESSAGE(1, smsSegments(r).segments, r.c_str());
      }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normalize_indian_numbers);
  RUN_TEST(test_parse_commands);
  RUN_TEST(test_parse_rejects_bad_input);
  RUN_TEST(test_stranger_is_ignored_silently);
  RUN_TEST(test_member_sets_stage_and_gets_confirmation);
  RUN_TEST(test_tolerance_and_buzzer);
  RUN_TEST(test_only_owner_manages_numbers_and_wifi);
  RUN_TEST(test_adding_existing_number_is_not_duplicated);
  RUN_TEST(test_language_is_per_number);
  RUN_TEST(test_owner_claim_with_pin_and_lockout);
  RUN_TEST(test_bad_code_from_member_gets_help_reply_in_their_language);
  RUN_TEST(test_status_reply);
  RUN_TEST(test_every_reply_fits_one_sms_in_every_language);
  return UNITY_END();
}
