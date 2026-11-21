// Phase 0 smoke test: proves the native Unity test runner works. Removed in phase 1.
#include <unity.h>
void setUp() {}
void tearDown() {}
static void test_runner_works() { TEST_ASSERT_EQUAL_INT(4, 2 + 2); }
int main() { UNITY_BEGIN(); RUN_TEST(test_runner_works); return UNITY_END(); }
