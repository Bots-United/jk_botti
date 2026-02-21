//
// JK_Botti - common test framework macros
//

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
   tests_run++; \
   printf("  %-60s ", name); \
} while(0)

#define PASS() do { \
   tests_passed++; \
   printf("OK\n"); \
} while(0)

#define ASSERT_STR(actual, expected) do { \
   if (strcmp((actual), (expected)) != 0) { \
      printf("FAIL\n    expected: \"%s\"\n    got:      \"%s\"\n", \
             (expected), (actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_INT(actual, expected) do { \
   if ((actual) != (expected)) { \
      printf("FAIL\n    expected: %d\n    got:      %d\n", \
             (expected), (actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_EQ(actual, expected) do { \
   if ((actual) != (expected)) { \
      printf("FAIL\n    expected: %p\n    got:      %p\n", \
             (void*)(expected), (void*)(actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_NULL(actual) do { \
   if ((actual) != NULL) { \
      printf("FAIL\n    expected: NULL\n    got:      %p\n", \
             (void*)(actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_NOT_NULL(actual) do { \
   if ((actual) == NULL) { \
      printf("FAIL\n    expected: non-NULL\n    got:      NULL\n"); \
      return 1; \
   } \
} while(0)

#define ASSERT_TRUE(cond) do { \
   if (!(cond)) { \
      printf("FAIL\n    condition false: %s\n", #cond); \
      return 1; \
   } \
} while(0)

#endif // TEST_COMMON_H
