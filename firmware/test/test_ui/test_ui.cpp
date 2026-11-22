#include <unity.h>
#include "../fakes/fakes.h"
#include "seri/persist.h"
#include "seri/sms_text.h"
#include "seri/time_util.h"
#include "seri/ui.h"
using namespace seri;

static const int64_t kT = fromIst(CivilTime{2026, 11, 21, 14, 5, 0});

struct Rig {
  fakes::Clock clock; fakes::Sensor sensor; fakes::Power power; fakes::Modem modem; fakes::Wifi wifi;
  fakes::Printer printer; fakes::Storage storage; fakes::Indicators ind; fakes::Persist persist;
  Monitor m{Hardware{clock, sensor, power, modem, wifi, printer, storage, ind, persist}};
  Ui ui{m};
  int64_t now = kT;
  int64_t uiSec = 1000;  // UI timer (wall-clock seconds), deliberately unrelated to the device clock
  Rig() {
    clock.t = kT;
    Settings s;
    s.phone[0] = "9876543210";
    persist.kv["settings"] = encodeSettings(s);
    m.begin();
    run(60);
  }
  void run(int secs) { for (int i = 0; i < secs; ++i) { clock.t += 1; ++uiSec; m.tick(); ui.tick(uiSec); } now = clock.t; }
  void press(Button b) { ui.press(b, uiSec); }
  Screen screen() { return ui.render(UiStatus{true, false, clock.t}); }
  bool shows(const std::string& s) {
    for (auto& l : screen().lines) if (l.find(s) != std::string::npos) return true;
    return false;
  }
};

void setUp() {}
void tearDown() {}

static void test_home_screen_shows_reading_stage_status_power() {
  Rig r;
  Screen s = r.screen();
  TEST_ASSERT_EQUAL(4, s.lines.size());
  TEST_ASSERT_TRUE(s.bigFirstLine);
  TEST_ASSERT_EQUAL_STRING("27.5C 85%", s.lines[0].c_str());
  TEST_ASSERT_EQUAL_STRING("Chawki 4G", s.lines[1].c_str());
  TEST_ASSERT_EQUAL_STRING("All OK", s.lines[2].c_str());
  TEST_ASSERT_EQUAL_STRING("Mains 14:06", s.lines[3].c_str());
  r.power.mains = false;
  r.power.mv = 3800;
  r.run(2);
  TEST_ASSERT_EQUAL_STRING("Battery 50% 14:06", r.screen().lines[3].c_str());
}

static void test_home_shows_active_alarm_and_ok_silences_buzzer() {
  Rig r;
  r.sensor.t = 31.0f;
  r.run(6 * 60);
  TEST_ASSERT_TRUE(r.m.buzzing());
  TEST_ASSERT_EQUAL_STRING("Temp HIGH", r.screen().lines[2].c_str());
  r.press(Button::Ok);
  TEST_ASSERT_FALSE(r.m.buzzing());
  TEST_ASSERT_EQUAL(Ui::Page::Message, r.ui.page());
  TEST_ASSERT_TRUE(r.shows("Alarm silenced"));
}

static void test_menu_navigation_and_stage_change() {
  Rig r;
  r.press(Button::Ok);
  TEST_ASSERT_EQUAL(Ui::Page::Menu, r.ui.page());
  TEST_ASSERT_TRUE(r.shows("Stage"));
  TEST_ASSERT_EQUAL(0, r.screen().highlight);
  r.press(Button::Ok);  // Stage
  TEST_ASSERT_EQUAL(Ui::Page::Stage, r.ui.page());
  TEST_ASSERT_EQUAL_MESSAGE(1, r.screen().highlight, "cursor starts on the current stage (chawki)");
  r.press(Button::Down);
  r.press(Button::Ok);  // Instar 3
  TEST_ASSERT_EQUAL(Stage::Instar3, r.m.settings().stage);
  TEST_ASSERT_TRUE(r.shows("Saved"));
  TEST_ASSERT_EQUAL(1, r.printer.count("STAGE -> Instar 3"));
}

static void test_menu_scrolls_beyond_four_lines() {
  Rig r;
  r.press(Button::Ok);
  for (int i = 0; i < 6; ++i) r.press(Button::Down);  // to "Back"
  Screen s = r.screen();
  TEST_ASSERT_EQUAL(4, s.lines.size());
  TEST_ASSERT_TRUE(r.shows("Back"));
  TEST_ASSERT_EQUAL(3, s.highlight);
  r.press(Button::Down);  // wraps to the top
  TEST_ASSERT_EQUAL(0, r.screen().highlight);
  TEST_ASSERT_TRUE(r.shows("Stage"));
}

static void test_language_change_switches_the_whole_ui() {
  Rig r;
  r.press(Button::Ok);
  r.press(Button::Down);
  r.press(Button::Ok);  // Language
  TEST_ASSERT_EQUAL(Ui::Page::Language, r.ui.page());
  TEST_ASSERT_TRUE(r.shows("తెలుగు"));
  r.press(Button::Down); r.press(Button::Down);
  r.press(Button::Ok);  // Telugu
  TEST_ASSERT_EQUAL(Language::Telugu, r.m.settings().lang[0]);
  r.run(5);
  TEST_ASSERT_EQUAL(Ui::Page::Home, r.ui.page());
  Screen home = r.screen();  // Telugu: large mode (D-033), one item at a time
  TEST_ASSERT_TRUE(home.large);
  bool sawOk = false, sawStage = false;
  for (int i = 0; i < 9; ++i) {
    sawOk |= r.screen().body == "అన్నీ సరి";
    sawStage |= r.screen().body == "చాకీ 4G";
    r.run(1);
  }
  TEST_ASSERT_TRUE(sawOk);
  TEST_ASSERT_TRUE(sawStage);
}

static void test_range_edit_changes_tolerance() {
  Rig r;
  r.press(Button::Ok);
  r.press(Button::Down); r.press(Button::Down);
  r.press(Button::Ok);  // Range: temperature first
  TEST_ASSERT_TRUE(r.shows("+/-1.0C"));
  r.press(Button::Up);  // +0.5
  TEST_ASSERT_TRUE(r.shows("+/-1.5C"));
  r.press(Button::Ok);  // then humidity
  TEST_ASSERT_TRUE(r.shows("+/-5%"));
  r.press(Button::Down); r.press(Button::Down);
  r.press(Button::Ok);
  TEST_ASSERT_EQUAL(15, r.m.settings().tol.t);
  TEST_ASSERT_EQUAL(30, r.m.settings().tol.rh);
  TEST_ASSERT_TRUE(r.shows("Saved"));
}

static void test_range_limits_are_enforced() {
  Rig r;
  r.press(Button::Ok); r.press(Button::Down); r.press(Button::Down); r.press(Button::Ok);
  for (int i = 0; i < 20; ++i) r.press(Button::Down);
  TEST_ASSERT_TRUE(r.shows("+/-0.5C"));
  for (int i = 0; i < 20; ++i) r.press(Button::Up);
  TEST_ASSERT_TRUE(r.shows("+/-3.0C"));
}

static void test_add_phone_number_with_digit_editor() {
  Rig r;
  r.press(Button::Ok);
  for (int i = 0; i < 3; ++i) r.press(Button::Down);
  r.press(Button::Ok);  // Phone numbers
  TEST_ASSERT_TRUE(r.shows("9876543210"));
  TEST_ASSERT_TRUE(r.shows("Empty"));
  r.press(Button::Down);
  r.press(Button::Ok);  // slot 2 -> Change
  r.press(Button::Ok);  // Change -> editor starts at 6000000000
  TEST_ASSERT_EQUAL(Ui::Page::PhoneEdit, r.ui.page());
  TEST_ASSERT_TRUE(r.shows("6000000000"));
  // type 9123456780: first digit 6 -> 9 (UP x3), then each digit
  const char* target = "9123456780";
  for (int d = 0; d < 10; ++d) {
    const int from = d == 0 ? 6 : 0, to = target[d] - '0';
    for (int k = from; k < to; ++k) r.press(Button::Up);
    r.press(Button::Ok);
  }
  TEST_ASSERT_EQUAL_STRING("9123456780", r.m.settings().phone[1].c_str());
  TEST_ASSERT_TRUE(r.shows("Saved"));
}

static void test_remove_extra_number_but_owner_cannot_be_removed() {
  Rig r;
  Settings s = r.m.settings();
  s.phone[1] = "9123456780";
  r.m.changeSettings(s);
  r.press(Button::Ok);
  for (int i = 0; i < 3; ++i) r.press(Button::Down);
  r.press(Button::Ok);  // Phones
  r.press(Button::Ok);  // owner slot
  TEST_ASSERT_FALSE_MESSAGE(r.shows("Remove"), "owner slot offers Change only");
  r.press(Button::Down);  // Back
  r.press(Button::Ok);
  r.press(Button::Down);
  r.press(Button::Ok);    // slot 2
  r.press(Button::Down);  // Remove
  r.press(Button::Ok);
  TEST_ASSERT_EQUAL_STRING("", r.m.settings().phone[1].c_str());
}

static void test_buzzer_toggle_and_print_test() {
  Rig r;
  r.press(Button::Ok);
  for (int i = 0; i < 4; ++i) r.press(Button::Down);
  TEST_ASSERT_TRUE(r.shows("Buzzer: On"));
  r.press(Button::Ok);
  TEST_ASSERT_FALSE(r.m.settings().buzzerOn);
  TEST_ASSERT_TRUE(r.shows("Buzzer: Off"));
  r.press(Button::Down);
  r.press(Button::Ok);  // Print test
  TEST_ASSERT_EQUAL(1, r.printer.count("Printer test OK"));
  TEST_ASSERT_EQUAL_MESSAGE(1, r.printer.count("21/11 14:06 Printer test OK"), "stamped with the device clock, not the UI's timer");
}

static void test_idle_returns_home() {
  Rig r;
  r.press(Button::Ok);
  r.run(59);
  TEST_ASSERT_EQUAL(Ui::Page::Menu, r.ui.page());
  r.run(2);
  TEST_ASSERT_EQUAL(Ui::Page::Home, r.ui.page());
}

static void switchToTelugu(Rig& r) {
  Settings s = r.m.settings();
  s.lang[0] = Language::Telugu;
  r.m.changeSettings(s);
}

static void test_indian_language_home_uses_large_mode_and_rotates() {
  Rig r;
  switchToTelugu(r);
  Screen s = r.screen();
  TEST_ASSERT_TRUE(s.large);
  TEST_ASSERT_EQUAL_STRING("27.5C 85% 14:06", s.header.c_str());
  std::vector<std::string> seen;
  for (int i = 0; i < 9; ++i) { seen.push_back(r.screen().body); r.run(1); }
  auto has = [&](const char* x) { for (auto& b : seen) if (b == x) return true; return false; };
  TEST_ASSERT_TRUE_MESSAGE(has("అన్నీ సరి"), "status shown");
  TEST_ASSERT_TRUE_MESSAGE(has("చాకీ 4G"), "stage shown");
  TEST_ASSERT_TRUE_MESSAGE(has("కరెంట్"), "power shown");
}

static void test_indian_language_menu_shows_one_large_item_with_position() {
  Rig r;
  switchToTelugu(r);
  r.press(Button::Ok);
  Screen s = r.screen();
  TEST_ASSERT_TRUE(s.large);
  TEST_ASSERT_EQUAL_STRING("దశ", s.body.c_str());
  TEST_ASSERT_EQUAL_STRING("1/7", s.header.c_str());
  r.press(Button::Down);
  TEST_ASSERT_EQUAL_STRING("భాష", r.screen().body.c_str());
  TEST_ASSERT_EQUAL_STRING("2/7", r.screen().header.c_str());
}

static void test_english_keeps_four_line_mode() {
  Rig r;
  TEST_ASSERT_FALSE(r.screen().large);
}

static void test_phone_editor_stays_small_in_every_language() {
  Rig r;
  switchToTelugu(r);
  r.press(Button::Ok);
  for (int i = 0; i < 3; ++i) r.press(Button::Down);
  r.press(Button::Ok); r.press(Button::Ok); r.press(Button::Ok);
  TEST_ASSERT_EQUAL(Ui::Page::PhoneEdit, r.ui.page());
  TEST_ASSERT_FALSE(r.screen().large);
  TEST_ASSERT_TRUE(r.shows("9876543210"));
}

static void test_every_ui_text_exists_in_every_language() {
  for (int l = 0; l < kLanguageCount; ++l)
    for (int t = 0; t < kUiTextCount; ++t)
      TEST_ASSERT_TRUE(std::string(uiText(static_cast<UiText>(t), static_cast<Language>(l))).size() > 0);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_home_screen_shows_reading_stage_status_power);
  RUN_TEST(test_home_shows_active_alarm_and_ok_silences_buzzer);
  RUN_TEST(test_menu_navigation_and_stage_change);
  RUN_TEST(test_menu_scrolls_beyond_four_lines);
  RUN_TEST(test_language_change_switches_the_whole_ui);
  RUN_TEST(test_range_edit_changes_tolerance);
  RUN_TEST(test_range_limits_are_enforced);
  RUN_TEST(test_add_phone_number_with_digit_editor);
  RUN_TEST(test_remove_extra_number_but_owner_cannot_be_removed);
  RUN_TEST(test_buzzer_toggle_and_print_test);
  RUN_TEST(test_idle_returns_home);
  RUN_TEST(test_every_ui_text_exists_in_every_language);
  RUN_TEST(test_indian_language_home_uses_large_mode_and_rotates);
  RUN_TEST(test_indian_language_menu_shows_one_large_item_with_position);
  RUN_TEST(test_english_keeps_four_line_mode);
  RUN_TEST(test_phone_editor_stays_small_in_every_language);
  return UNITY_END();
}
