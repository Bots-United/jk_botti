//
// JK_Botti - tests for random number generator
//

#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include "test_common.h"

extern void fast_random_seed(unsigned int seed);
extern int RANDOM_LONG2(int lLow, int lHigh);
extern float RANDOM_FLOAT2(float flLow, float flHigh);

// ============================================================
// RANDOM_LONG2 tests
// ============================================================

static int test_random_long_basic(void)
{
   int i;

   printf("RANDOM_LONG2 basic:\n");

   fast_random_seed(123);

   TEST("equal low and high returns low");
   ASSERT_INT(RANDOM_LONG2(5, 5), 5);
   PASS();

   TEST("low > high returns low");
   ASSERT_INT(RANDOM_LONG2(10, 5), 10);
   PASS();

   TEST("range 0,1 produces only 0 or 1");
   fast_random_seed(42);
   int saw_0 = 0, saw_1 = 0;
   for (i = 0; i < 10000; i++) {
      int v = RANDOM_LONG2(0, 1);
      ASSERT_TRUE(v >= 0 && v <= 1);
      if (v == 0) saw_0 = 1;
      if (v == 1) saw_1 = 1;
   }
   ASSERT_TRUE(saw_0 && saw_1);
   PASS();

   return 0;
}

static int test_random_long_bounds(void)
{
   int i;

   printf("RANDOM_LONG2 bounds:\n");

   TEST("stays within [0, 100] over 100k samples");
   fast_random_seed(1);
   for (i = 0; i < 100000; i++) {
      int v = RANDOM_LONG2(0, 100);
      ASSERT_TRUE(v >= 0 && v <= 100);
   }
   PASS();

   TEST("stays within [-50, 50] over 100k samples");
   fast_random_seed(2);
   for (i = 0; i < 100000; i++) {
      int v = RANDOM_LONG2(-50, 50);
      ASSERT_TRUE(v >= -50 && v <= 50);
   }
   PASS();

   TEST("stays within [-1000000, 1000000] over 100k samples");
   fast_random_seed(3);
   for (i = 0; i < 100000; i++) {
      int v = RANDOM_LONG2(-1000000, 1000000);
      ASSERT_TRUE(v >= -1000000 && v <= 1000000);
   }
   PASS();

   TEST("hits both endpoints of [0, 10] over 100k samples");
   fast_random_seed(7);
   int saw_lo = 0, saw_hi = 0;
   for (i = 0; i < 100000; i++) {
      int v = RANDOM_LONG2(0, 10);
      if (v == 0) saw_lo = 1;
      if (v == 10) saw_hi = 1;
   }
   ASSERT_TRUE(saw_lo && saw_hi);
   PASS();

   TEST("inclusive: hits lHigh with range [0, 1] over 1M samples");
   saw_hi = 0;
   for (i = 0; i < 1000000; i++) {
      fast_random_seed(i);
      if (RANDOM_LONG2(0, 1) == 1) { saw_hi = 1; break; }
   }
   ASSERT_TRUE(saw_hi);
   PASS();

   return 0;
}

static int test_random_long_distribution(void)
{
   int i;
   int buckets[10];

   printf("RANDOM_LONG2 distribution:\n");

   TEST("roughly uniform over [0, 9] (100k samples, 10 buckets)");
   fast_random_seed(99);
   for (i = 0; i < 10; i++)
      buckets[i] = 0;
   for (i = 0; i < 100000; i++) {
      int v = RANDOM_LONG2(0, 9);
      buckets[v]++;
   }
   // expect ~10000 per bucket, allow 10% deviation
   for (i = 0; i < 10; i++)
      ASSERT_TRUE(buckets[i] > 9000 && buckets[i] < 11000);
   PASS();

   return 0;
}

// ============================================================
// RANDOM_FLOAT2 tests
// ============================================================

static int test_random_float_basic(void)
{
   printf("RANDOM_FLOAT2 basic:\n");

   fast_random_seed(456);

   TEST("equal low and high returns low");
   ASSERT_TRUE(RANDOM_FLOAT2(3.0f, 3.0f) == 3.0f);
   PASS();

   TEST("low > high returns low");
   ASSERT_TRUE(RANDOM_FLOAT2(10.0f, 5.0f) == 10.0f);
   PASS();

   return 0;
}

static int test_random_float_bounds(void)
{
   int i;

   printf("RANDOM_FLOAT2 bounds:\n");

   TEST("stays within [0.0, 1.0] over 100k samples");
   fast_random_seed(10);
   for (i = 0; i < 100000; i++) {
      float v = RANDOM_FLOAT2(0.0f, 1.0f);
      ASSERT_TRUE(v >= 0.0f && v <= 1.0f);
   }
   PASS();

   TEST("stays within [-100.0, 100.0] over 100k samples");
   fast_random_seed(11);
   for (i = 0; i < 100000; i++) {
      float v = RANDOM_FLOAT2(-100.0f, 100.0f);
      ASSERT_TRUE(v >= -100.0f && v <= 100.0f);
   }
   PASS();

   TEST("stays within [0.0, 0.001] over 100k samples");
   fast_random_seed(12);
   for (i = 0; i < 100000; i++) {
      float v = RANDOM_FLOAT2(0.0f, 0.001f);
      ASSERT_TRUE(v >= 0.0f && v <= 0.001f);
   }
   PASS();

   TEST("max value approaches flHigh with range [0.0, 1.0]");
   float max_seen = 0.0f;
   fast_random_seed(13);
   for (i = 0; i < 100000; i++) {
      float v = RANDOM_FLOAT2(0.0f, 1.0f);
      if (v > max_seen) max_seen = v;
   }
   // with 100k samples, max should be very close to 1.0
   ASSERT_TRUE(max_seen > 0.999f);
   PASS();

   return 0;
}

static int test_random_float_distribution(void)
{
   int i;
   int buckets[10];

   printf("RANDOM_FLOAT2 distribution:\n");

   TEST("roughly uniform over [0.0, 10.0] (100k samples, 10 buckets)");
   fast_random_seed(200);
   for (i = 0; i < 10; i++)
      buckets[i] = 0;
   for (i = 0; i < 100000; i++) {
      float v = RANDOM_FLOAT2(0.0f, 10.0f);
      int bucket = (int)v;
      if (bucket >= 10) bucket = 9;
      buckets[bucket]++;
   }
   // expect ~10000 per bucket, allow 10% deviation
   for (i = 0; i < 10; i++)
      ASSERT_TRUE(buckets[i] > 9000 && buckets[i] < 11000);
   PASS();

   return 0;
}

// ============================================================
// Seed determinism test
// ============================================================

static int test_seed_determinism(void)
{
   int i;

   printf("seed determinism:\n");

   TEST("same seed produces same sequence");
   fast_random_seed(777);
   int seq1[100];
   for (i = 0; i < 100; i++)
      seq1[i] = RANDOM_LONG2(0, 1000000);
   fast_random_seed(777);
   int match = 1;
   for (i = 0; i < 100; i++) {
      if (RANDOM_LONG2(0, 1000000) != seq1[i]) {
         match = 0;
         break;
      }
   }
   ASSERT_TRUE(match);
   PASS();

   TEST("different seeds produce different sequences");
   fast_random_seed(100);
   int a1 = RANDOM_LONG2(0, 1000000);
   int a2 = RANDOM_LONG2(0, 1000000);
   fast_random_seed(200);
   int b1 = RANDOM_LONG2(0, 1000000);
   int b2 = RANDOM_LONG2(0, 1000000);
   ASSERT_TRUE(a1 != b1 || a2 != b2);
   PASS();

   return 0;
}

int main(void)
{
   int rc = 0;

   printf("=== random_num tests ===\n\n");

   rc |= test_random_long_basic();
   printf("\n");
   rc |= test_random_long_bounds();
   printf("\n");
   rc |= test_random_long_distribution();
   printf("\n");
   rc |= test_random_float_basic();
   printf("\n");
   rc |= test_random_float_bounds();
   printf("\n");
   rc |= test_random_float_distribution();
   printf("\n");
   rc |= test_seed_determinism();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
