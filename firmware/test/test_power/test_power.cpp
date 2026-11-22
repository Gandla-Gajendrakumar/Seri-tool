#include <unity.h>
#include "seri/power.h"
using namespace seri;
void setUp() {}
void tearDown() {}

static void test_battery_percent_endpoints_and_clamping() {
  TEST_ASSERT_EQUAL(100, batteryPercent(4200));
  TEST_ASSERT_EQUAL(100, batteryPercent(4350));
  TEST_ASSERT_EQUAL(0, batteryPercent(3000));
  TEST_ASSERT_EQUAL(0, batteryPercent(2500));
  TEST_ASSERT_EQUAL(50, batteryPercent(3800));
}

static void test_battery_percent_interpolates_and_rounds() {
  TEST_ASSERT_EQUAL(58, batteryPercent(3850));  // halfway 3.80 (50) - 3.90 (65) = 57.5 -> 58
}

static void test_battery_percent_never_decreases_with_voltage() {
  int prev = -1;
  for (int mv = 2800; mv <= 4400; mv += 5) {
    int p = batteryPercent(mv);
    TEST_ASSERT_TRUE(p >= prev);
    TEST_ASSERT_TRUE(p >= 0 && p <= 100);
    prev = p;
  }
}

static void test_upload_interval() {
  TEST_ASSERT_EQUAL_INT64(300, uploadIntervalSec(false));
  TEST_ASSERT_EQUAL_INT64(900, uploadIntervalSec(true));
}

static void test_ota_only_on_mains_with_half_battery() {
  TEST_ASSERT_TRUE(otaAllowed(false, 50));
  TEST_ASSERT_FALSE(otaAllowed(false, 49));
  TEST_ASSERT_FALSE(otaAllowed(true, 100));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_battery_percent_endpoints_and_clamping);
  RUN_TEST(test_battery_percent_interpolates_and_rounds);
  RUN_TEST(test_battery_percent_never_decreases_with_voltage);
  RUN_TEST(test_upload_interval);
  RUN_TEST(test_ota_only_on_mains_with_half_battery);
  return UNITY_END();
}
