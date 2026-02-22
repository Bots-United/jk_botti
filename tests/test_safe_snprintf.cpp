//
// JK_Botti - tests for safe_snprintf and safe_strcopy
//

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "test_common.h"

#include "safe_snprintf.h"

// helper: fill buffer with sentinel value
static void fill_buf(char *buf, size_t size, char val)
{
   memset(buf, val, size);
}

// ============================================================
// safe_strcopy tests
// ============================================================

static int test_strcopy_basic(void)
{
   char buf[64];

   printf("safe_strcopy basic:\n");

   TEST("copy simple string");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, sizeof(buf), "hello");
   ASSERT_STR(buf, "hello");
   PASS();

   TEST("copy empty string");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, sizeof(buf), "");
   ASSERT_STR(buf, "");
   PASS();

   TEST("NULL src writes (null)");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, sizeof(buf), NULL);
   ASSERT_STR(buf, "(null)");
   PASS();

   TEST("returns dst pointer");
   char *ret = safe_strcopy(buf, sizeof(buf), "test");
   ASSERT_TRUE(ret == buf);
   PASS();

   return 0;
}

static int test_strcopy_truncation(void)
{
   char buf[8];

   printf("safe_strcopy truncation:\n");

   TEST("exact fit (src length == dst_size - 1)");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, 6, "hello"); // 5 chars + null = 6
   ASSERT_STR(buf, "hello");
   PASS();

   TEST("truncation (src longer than dst_size)");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, 4, "hello");
   ASSERT_STR(buf, "hel");
   PASS();

   TEST("dst_size 1 produces empty string");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, 1, "hello");
   ASSERT_STR(buf, "");
   PASS();

   TEST("dst_size 2 copies one char");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, 2, "hello");
   ASSERT_STR(buf, "h");
   PASS();

   return 0;
}

static int test_strcopy_zero_size(void)
{
   printf("safe_strcopy zero size:\n");

   // BUG: dst_size == 0 causes write to dst[SIZE_MAX] (underflow of i-1).
   // Use heap buffer so valgrind catches the out-of-bounds write.
   TEST("dst_size 0 does not crash or modify buffer");
   char *hbuf = (char *)malloc(8);
   fill_buf(hbuf, 8, 'X');
   safe_strcopy(hbuf, 0, "hello");
   ASSERT_TRUE(hbuf[0] == 'X');
   free(hbuf);
   PASS();

   TEST("dst_size 0 with NULL src does not crash");
   hbuf = (char *)malloc(8);
   fill_buf(hbuf, 8, 'X');
   safe_strcopy(hbuf, 0, NULL);
   ASSERT_TRUE(hbuf[0] == 'X');
   free(hbuf);
   PASS();

   return 0;
}

static int test_strcopy_null_truncation(void)
{
   char buf[8];

   printf("safe_strcopy NULL truncation:\n");

   // "(null)" is 6 chars, test truncation of that
   TEST("NULL src truncated to dst_size 4");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, 4, NULL);
   ASSERT_STR(buf, "(nu");
   PASS();

   TEST("NULL src truncated to dst_size 1");
   fill_buf(buf, sizeof(buf), 'X');
   safe_strcopy(buf, 1, NULL);
   ASSERT_STR(buf, "");
   PASS();

   return 0;
}

// ============================================================
// safevoid_snprintf tests
// ============================================================

static int test_snprintf_basic(void)
{
   char buf[64];

   printf("safevoid_snprintf basic:\n");

   TEST("simple string format");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, sizeof(buf), "hello %s", "world");
   ASSERT_STR(buf, "hello world");
   PASS();

   TEST("integer format");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, sizeof(buf), "%d", 42);
   ASSERT_STR(buf, "42");
   PASS();

   TEST("empty format string");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, sizeof(buf), "");
   ASSERT_STR(buf, "");
   PASS();

   TEST("NULL format string");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, sizeof(buf), NULL);
   ASSERT_STR(buf, "");
   PASS();

   return 0;
}

static int test_snprintf_truncation(void)
{
   char buf[8];

   printf("safevoid_snprintf truncation:\n");

   TEST("output truncated to buffer size");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, 4, "hello world");
   ASSERT_INT((int)strlen(buf), 3);
   ASSERT_STR(buf, "hel");
   PASS();

   TEST("exact fit");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, 6, "hello");
   ASSERT_STR(buf, "hello");
   PASS();

   TEST("buffer size 1 produces empty string");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, 1, "hello");
   ASSERT_STR(buf, "");
   PASS();

   return 0;
}

static int test_snprintf_edge_cases(void)
{
   char buf[64];

   printf("safevoid_snprintf edge cases:\n");

   TEST("NULL buffer pointer does not crash");
   safevoid_snprintf(NULL, 10, "hello");
   PASS();

   TEST("zero size does not crash");
   fill_buf(buf, sizeof(buf), 'X');
   safevoid_snprintf(buf, 0, "hello");
   // buffer should be untouched
   ASSERT_TRUE(buf[0] == 'X');
   PASS();

   TEST("NULL buffer with zero size does not crash");
   safevoid_snprintf(NULL, 0, "hello");
   PASS();

   TEST("multiple format arguments");
   safevoid_snprintf(buf, sizeof(buf), "%s=%d, %s=%d", "a", 1, "b", 2);
   ASSERT_STR(buf, "a=1, b=2");
   PASS();

   TEST("percent literal");
   safevoid_snprintf(buf, sizeof(buf), "100%%");
   ASSERT_STR(buf, "100%");
   PASS();

   return 0;
}

static int test_snprintf_always_null_terminated(void)
{
   char buf[8];

   printf("safevoid_snprintf null termination:\n");

   TEST("always null-terminated when truncated");
   // fill with non-zero to detect missing null
   fill_buf(buf, sizeof(buf), 'Z');
   safevoid_snprintf(buf, 4, "abcdefghij");
   ASSERT_TRUE(buf[3] == '\0');
   ASSERT_INT((int)strlen(buf), 3);
   PASS();

   TEST("always null-terminated at exact boundary");
   fill_buf(buf, sizeof(buf), 'Z');
   safevoid_snprintf(buf, 4, "abc");
   ASSERT_TRUE(buf[3] == '\0');
   ASSERT_STR(buf, "abc");
   PASS();

   return 0;
}

int main(void)
{
   int rc = 0;

   printf("=== safe_snprintf tests ===\n\n");

   rc |= test_strcopy_basic();
   printf("\n");
   rc |= test_strcopy_truncation();
   printf("\n");
   rc |= test_strcopy_zero_size();
   printf("\n");
   rc |= test_strcopy_null_truncation();
   printf("\n");
   rc |= test_snprintf_basic();
   printf("\n");
   rc |= test_snprintf_truncation();
   printf("\n");
   rc |= test_snprintf_edge_cases();
   printf("\n");
   rc |= test_snprintf_always_null_terminated();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
