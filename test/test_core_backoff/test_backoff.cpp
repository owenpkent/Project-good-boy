#include <unity.h>
#include <stdint.h>
#include "backoff.h"

void setUp() {}
void tearDown() {}

void test_growth_no_jitter() {
  core::Backoff b(100, 800, 2.0f, 0.0f);
  TEST_ASSERT_EQUAL_UINT32(100, b.next());
  TEST_ASSERT_EQUAL_UINT32(200, b.next());
  TEST_ASSERT_EQUAL_UINT32(400, b.next());
  TEST_ASSERT_EQUAL_UINT32(800, b.next());
  // capped at max
  TEST_ASSERT_EQUAL_UINT32(800, b.next());
}

void test_reset() {
  core::Backoff b(250, 1000, 2.0f, 0.0f);
  (void)b.next();
  (void)b.next();
  b.reset();
  TEST_ASSERT_EQUAL_UINT32(250, b.next());
}

void test_jitter_bounds() {
  core::Backoff b(1000, 30000, 2.0f, 0.2f); // 20% jitter
  b.setSeed(42);
  for (int i = 0; i < 50; ++i) {
    uint32_t d = b.next();
    // First call base = 1000, so bounds = [800, 1200]
    // Next calls will change base, but still check proportional bounds of previous base
    // Here, to keep simple, re-create expected bounds from last base
    // However, since jitter already applied to 'base', we only check for the first 3 samples
    if (i == 0) {
      TEST_ASSERT_TRUE(d >= 800 && d <= 1200);
    }
    if (i == 1) {
      TEST_ASSERT_TRUE(d >= 1600 && d <= 2400); // base 2000 +-20%
    }
    if (i == 2) {
      TEST_ASSERT_TRUE(d >= 3200 && d <= 4800); // base 4000 +-20%
    }
  }
}

void test_deterministic_seed() {
  core::Backoff a(500, 10000, 2.0f, 0.3f);
  core::Backoff b(500, 10000, 2.0f, 0.3f);
  a.setSeed(123);
  b.setSeed(123);
  
  // Test first few values match
  for (int i = 0; i < 5; ++i) {
    uint32_t valA = a.next();
    uint32_t valB = b.next();
    TEST_ASSERT_EQUAL_UINT32(valA, valB);
  }
}

void test_factor_less_than_one_clamps_to_initial() {
  core::Backoff b(300, 5000, 0.5f, 0.0f); // factor < 1
  TEST_ASSERT_EQUAL_UINT32(300, b.next());
  // Since factor < 1, implementation keeps current at >= initial
  TEST_ASSERT_EQUAL_UINT32(300, b.next());
  TEST_ASSERT_EQUAL_UINT32(300, b.next());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_growth_no_jitter);
  RUN_TEST(test_reset);
  RUN_TEST(test_jitter_bounds);
  RUN_TEST(test_deterministic_seed);
  RUN_TEST(test_factor_less_than_one_clamps_to_initial);
  return UNITY_END();
}
