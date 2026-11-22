#include <unity.h>
#include <cstring>
#include "seri/profiles.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static void test_every_stage_has_a_profile_with_a_source() {
  for (int i = 0; i < kStageCount; ++i) {
    const Profile& p = profileFor(static_cast<Stage>(i));
    TEST_ASSERT_EQUAL(i, static_cast<int>(p.stage));
    TEST_ASSERT_NOT_NULL(p.source);
    TEST_ASSERT_TRUE_MESSAGE(std::strlen(p.source) > 10, "each profile must cite a source");
    TEST_ASSERT_TRUE(std::strlen(p.key) > 0);
    TEST_ASSERT_TRUE(p.tMin <= p.tMax);
    TEST_ASSERT_TRUE(p.rhMin <= p.rhMax);
    // sanity: silkworm rearing is never outside 15-35 C / 40-100 %
    TEST_ASSERT_TRUE(p.tMin >= 150 && p.tMax <= 350);
    TEST_ASSERT_TRUE(p.rhMin >= 400 && p.rhMax <= 1000);
  }
}

static void test_approved_values_are_loaded() {
  // Brief values, confirmed against CSB/CSRTI; incubation RH widened and Spinning added on
  // product-owner approval 2026-11-22 (docs/stage_profile_sources.md).
  const Profile& inc = profileFor(Stage::Incubation);
  TEST_ASSERT_EQUAL(250, inc.tMin); TEST_ASSERT_EQUAL(260, inc.tMax);
  TEST_ASSERT_EQUAL(700, inc.rhMin); TEST_ASSERT_EQUAL(800, inc.rhMax);
  const Profile& ch = profileFor(Stage::Chawki);
  TEST_ASSERT_EQUAL(270, ch.tMin); TEST_ASSERT_EQUAL(280, ch.tMax);
  TEST_ASSERT_EQUAL(800, ch.rhMin); TEST_ASSERT_EQUAL(900, ch.rhMax);
  TEST_ASSERT_EQUAL(260, profileFor(Stage::Instar3).tMax);
  TEST_ASSERT_EQUAL(800, profileFor(Stage::Instar3).rhMax);
  TEST_ASSERT_EQUAL(250, profileFor(Stage::Instar4).tMin);
  TEST_ASSERT_EQUAL(700, profileFor(Stage::Instar4).rhMin);
  TEST_ASSERT_EQUAL(240, profileFor(Stage::Instar5).tMin);
  TEST_ASSERT_EQUAL(700, profileFor(Stage::Instar5).rhMax);
  const Profile& sp = profileFor(Stage::Spinning);
  TEST_ASSERT_EQUAL(240, sp.tMin); TEST_ASSERT_EQUAL(280, sp.tMax);
  TEST_ASSERT_EQUAL(600, sp.rhMin); TEST_ASSERT_EQUAL(700, sp.rhMax);
  TEST_ASSERT_EQUAL(6, kStageCount);
}

static void test_band_widens_target_range_by_tolerance() {
  // Q-3: chawki 27-28 C, 80-90 % with +/-1 C, +/-5 % -> 26-29 C, 75-95 %
  Band b = bandFor(Stage::Chawki, Tolerance{});
  TEST_ASSERT_EQUAL(260, b.tLo);
  TEST_ASSERT_EQUAL(290, b.tHi);
  TEST_ASSERT_EQUAL(750, b.rhLo);
  TEST_ASSERT_EQUAL(950, b.rhHi);
}

static void test_band_uses_custom_tolerance() {
  Band b = bandFor(Stage::Instar5, Tolerance{15, 30});  // 24 C, 70 % +/-1.5, +/-3
  TEST_ASSERT_EQUAL(225, b.tLo);
  TEST_ASSERT_EQUAL(255, b.tHi);
  TEST_ASSERT_EQUAL(670, b.rhLo);
  TEST_ASSERT_EQUAL(730, b.rhHi);
}

static void test_band_rh_never_exceeds_100_percent() {
  Band b = bandFor(Stage::Chawki, Tolerance{10, 150});  // 90 + 15 = 105 -> clamp 100
  TEST_ASSERT_EQUAL(1000, b.rhHi);
}

static void test_tolerance_limits() {
  TEST_ASSERT_TRUE(toleranceValid(Tolerance{}));
  TEST_ASSERT_TRUE(toleranceValid(Tolerance{5, 20}));    // 0.5 C, 2 %
  TEST_ASSERT_TRUE(toleranceValid(Tolerance{30, 150}));  // 3.0 C, 15 %
  TEST_ASSERT_FALSE(toleranceValid(Tolerance{4, 50}));
  TEST_ASSERT_FALSE(toleranceValid(Tolerance{31, 50}));
  TEST_ASSERT_FALSE(toleranceValid(Tolerance{10, 19}));
  TEST_ASSERT_FALSE(toleranceValid(Tolerance{10, 151}));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_stage_has_a_profile_with_a_source);
  RUN_TEST(test_approved_values_are_loaded);
  RUN_TEST(test_band_widens_target_range_by_tolerance);
  RUN_TEST(test_band_uses_custom_tolerance);
  RUN_TEST(test_band_rh_never_exceeds_100_percent);
  RUN_TEST(test_tolerance_limits);
  return UNITY_END();
}
