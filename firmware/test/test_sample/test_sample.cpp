#include <unity.h>
#include "seri/sample.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static void test_median_of_five_good_reads() {
  RawReading r[5] = {{true, 27.0f, 85.0f}, {true, 27.4f, 84.0f}, {true, 26.9f, 86.0f},
                     {true, 27.2f, 85.5f}, {true, 27.1f, 83.0f}};
  Sample s = medianSample(r, 5);
  TEST_ASSERT_TRUE(s.valid);
  TEST_ASSERT_EQUAL(271, s.t);
  TEST_ASSERT_EQUAL(850, s.rh);
}

static void test_median_rejects_a_spike() {
  // door opened during one read: the spike must not move the sample
  RawReading r[5] = {{true, 27.0f, 85.0f}, {true, 35.0f, 50.0f}, {true, 27.0f, 85.0f},
                     {true, 27.1f, 85.0f}, {true, 26.9f, 85.0f}};
  Sample s = medianSample(r, 5);
  TEST_ASSERT_EQUAL(270, s.t);
  TEST_ASSERT_EQUAL(850, s.rh);
}

static void test_failed_and_impossible_reads_are_discarded() {
  RawReading r[5] = {{false, 0, 0}, {true, 130.0f, 50.0f}, {true, 27.0f, 101.0f},
                     {true, 27.0f, 80.0f}, {true, 27.2f, 82.0f}};
  // only 2 usable -> invalid
  TEST_ASSERT_FALSE(medianSample(r, 5).valid);
}

static void test_three_usable_reads_are_enough() {
  RawReading r[5] = {{false, 0, 0}, {true, -45.0f, 50.0f}, {true, 25.0f, 70.0f},
                     {true, 25.4f, 71.0f}, {true, 25.2f, 72.0f}};
  Sample s = medianSample(r, 5);
  TEST_ASSERT_TRUE(s.valid);
  TEST_ASSERT_EQUAL(252, s.t);
  TEST_ASSERT_EQUAL(710, s.rh);
}

static void test_even_count_uses_mean_of_middle_two() {
  RawReading r[5] = {{false, 0, 0}, {true, 25.0f, 70.0f}, {true, 25.2f, 72.0f},
                     {true, 25.6f, 74.0f}, {true, 26.0f, 76.0f}};
  Sample s = medianSample(r, 5);
  TEST_ASSERT_EQUAL(254, s.t);   // (25.2 + 25.6) / 2
  TEST_ASSERT_EQUAL(730, s.rh);  // (72 + 74) / 2
}

static void test_negative_temperature_rounds_correctly() {
  RawReading r[3] = {{true, -1.04f, 50.0f}, {true, -1.08f, 50.0f}, {true, -1.07f, 50.0f}};
  TEST_ASSERT_EQUAL(-11, medianSample(r, 3).t);  // -1.07 -> -1.1 (rounds to nearest tenth)
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_median_of_five_good_reads);
  RUN_TEST(test_median_rejects_a_spike);
  RUN_TEST(test_failed_and_impossible_reads_are_discarded);
  RUN_TEST(test_three_usable_reads_are_enough);
  RUN_TEST(test_even_count_uses_mean_of_middle_two);
  RUN_TEST(test_negative_temperature_rounds_correctly);
  return UNITY_END();
}
