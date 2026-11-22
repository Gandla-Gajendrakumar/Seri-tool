#include <unity.h>
#include "seri/print.h"
#include "seri/sms_text.h"
#include "seri/text_render.h"
#include "seri/time_util.h"
#include "seri/ui.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static int64_t T() { return fromIst(CivilTime{2026, 12, 31, 23, 59, 0}); }

static bool isAscii(const std::string& s) {
  for (unsigned char c : s) if (c >= 0x80) return false;
  return true;
}
// Printer: ASCII lines go out as ESC/POS text (32 columns); other lines as a 384-dot raster image.
static void check_print_line(const std::string& s);

static void check_line(const std::string& s, int cell, int maxWidth) {
  std::vector<std::string> missing;
  int w = measureText(s, cell, &missing);
  if (!missing.empty()) {
    std::string m = "word not in glyph atlas: '" + missing[0] + "' in: " + s;
    TEST_FAIL_MESSAGE(m.c_str());
  }
  if (w > maxWidth || w <= 0) {
    std::string m = "line is " + std::to_string(w) + " px (max " + std::to_string(maxWidth) + "): " + s;
    TEST_FAIL_MESSAGE(m.c_str());
  }
}

static void check_print_line(const std::string& s) {
  if (isAscii(s)) {
    if (static_cast<int>(s.size()) > kPrintCols) TEST_FAIL_MESSAGE(("over 32 columns: " + s).c_str());
    return;
  }
  check_line(s, kPrintCell, kPrintWidth);
}

static void test_ascii_renders_at_both_sizes() {
  for (int cell : {kOledCell, kPrintCell}) {
    Bitmap b = renderText("27.5C 85%", cell);
    TEST_ASSERT_EQUAL(cell, b.height);
    TEST_ASSERT_TRUE(b.width > 20);
    int set = 0;
    for (int y = 0; y < b.height; ++y) for (int x = 0; x < b.width; ++x) set += b.get(x, y);
    TEST_ASSERT_TRUE(set > 30);
  }
}

static void test_unknown_word_is_reported() {
  std::vector<std::string> missing;
  measureText("abc ॐॐॐ", kOledCell, &missing);
  TEST_ASSERT_EQUAL(1, missing.size());
}

static void test_padding_for_printer() {
  Bitmap b = renderText("तापमान ज्यादा 31.2C", kPrintCell, kPrintWidth);
  TEST_ASSERT_EQUAL(kPrintWidth, b.width);
  TEST_ASSERT_EQUAL(48 * kPrintCell, static_cast<int>(b.bits.size()));
}

static void test_every_oled_text_exists_and_fits_128px() {
  for (int l = 0; l < kLanguageCount; ++l) {
    const Language L = static_cast<Language>(l);
    for (int t = 0; t < kUiTextCount; ++t) check_line(uiText(static_cast<UiText>(t), L), kOledCell, kOledWidth);
    for (int s = 0; s < kStageCount; ++s) check_line(stageName(static_cast<Stage>(s), L), kOledCell, kOledWidth);
    check_line(languageName(L), kOledCell, kOledWidth);
    for (int e = 0; e < kEventTypeCount; ++e) check_line(shortEventPhrase(static_cast<EventType>(e), false, L), kOledCell, kOledWidth);
    // worst-case home status line and menu item
    check_line(std::string(uiText(kUiBattery, L)) + " 100% 23:59", kOledCell, kOledWidth);
    for (int s = 0; s < kStageCount; ++s) check_line(std::string(stageName(static_cast<Stage>(s), L)) + " WiFi", kOledCell, kOledWidth);
    check_line(std::string(uiText(kUiBuzzer, L)) + ": " + uiText(kUiOff, L), kOledCell, kOledWidth);
    check_line("-10.5C 100%", kOledCell, kOledWidth);
  }
}

static void test_every_printed_line_exists_and_fits_384_dots() {
  for (int l = 0; l < kLanguageCount; ++l) {
    const Language L = static_cast<Language>(l);
    for (int e = 0; e < kEventTypeCount; ++e)
      for (Edge ed : {Edge::Start, Edge::Clear}) check_print_line(printEventLine(Event{static_cast<EventType>(e), ed, T(), -105}, L));
    for (int s = 0; s < kStageCount; ++s) check_print_line(printStageLine(T(), static_cast<Stage>(s), L));
    DayReport r;
    r.from = T() - 86400; r.to = T(); r.samples = 10; r.tMin = -105; r.tMax = 455; r.rhMin = 0; r.rhMax = 1000;
    r.tOutSec = 86399; r.rhOutSec = 86399; r.noDataSec = 600; r.alerts = 99;
    for (int s = 0; s < kStageCount; ++s)
      for (const auto& line : printSummary(r, static_cast<Stage>(s), bandFor(static_cast<Stage>(s), Tolerance{30, 150}), L, true))
        check_print_line(line);
    check_print_line("31/12 23:59 " + std::string(uiText(kUiPrinterOk, L)));
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ascii_renders_at_both_sizes);
  RUN_TEST(test_unknown_word_is_reported);
  RUN_TEST(test_padding_for_printer);
  RUN_TEST(test_every_oled_text_exists_and_fits_128px);
  RUN_TEST(test_every_printed_line_exists_and_fits_384_dots);
  return UNITY_END();
}
