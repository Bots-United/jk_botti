//
// JK_Botti - tests for bot_config_init.cpp
//
// test_bot_config_init.cpp
//
// Uses #include approach so we can override UTIL_BuildFileName_N
// to redirect config file paths to temp files.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_config_init.h"

#include "engine_mock.h"

// ============================================================
// Temp file helpers
// ============================================================

static char tmp_logo_path[256] = "";
static char tmp_name_path[256] = "";

static void create_temp_file(const char *content, char *path_out, size_t path_size)
{
   snprintf(path_out, path_size, "/tmp/jkbotti_test_XXXXXX");
   int fd = mkstemp(path_out);
   if (fd >= 0) {
      if (content) {
         size_t len = strlen(content);
         ssize_t wr = write(fd, content, len);
         (void)wr;
      }
      close(fd);
   }
}

static void cleanup_temp_files(void)
{
   if (tmp_logo_path[0]) { unlink(tmp_logo_path); tmp_logo_path[0] = 0; }
   if (tmp_name_path[0]) { unlink(tmp_name_path); tmp_name_path[0] = 0; }
}

// ============================================================
// Override UTIL_BuildFileName_N - must be defined before
// #including bot_config_init.cpp (which calls it).
// ============================================================

void UTIL_BuildFileName_N(char *filename, int size, char *arg1, char *arg2)
{
   (void)arg2;
   if (arg1 && strstr(arg1, "logo")) {
      safe_strcopy(filename, size, tmp_logo_path);
   } else if (arg1 && strstr(arg1, "names")) {
      safe_strcopy(filename, size, tmp_name_path);
   } else {
      safe_strcopy(filename, size, "/dev/null");
   }
}

// Stub for UTIL_ConsolePrintf (also used by bot_config_init.cpp)
void UTIL_ConsolePrintf(const char *fmt, ...) { (void)fmt; }

// Stubs for util.cpp functions used by engine_mock.cpp
// (we can't link util.o because UTIL_BuildFileName_N would conflict)
void UTIL_UpdateFuncBreakable(edict_t *, const char *, const char *) {}
void UTIL_FreeFuncBreakables(void) {}

// Include the source under test
#include "../bot_config_init.cpp"

#include "test_common.h"

// ============================================================
// Test helpers
// ============================================================

static void reset_config_state(void)
{
   mock_reset();
   number_names = 0;
   memset(bot_names, 0, sizeof(bot_names));
   num_logos = 0;
   memset(bot_logos, 0, sizeof(bot_logos));
   cleanup_temp_files();
}

// ============================================================
// BotLogoInit tests
// ============================================================

static int test_logo_file_missing(void)
{
   TEST("BotLogoInit: missing file -> num_logos stays 0");
   reset_config_state();
   snprintf(tmp_logo_path, sizeof(tmp_logo_path), "/tmp/jkbotti_nonexistent_XXXXXX");

   BotLogoInit();
   ASSERT_INT(num_logos, 0);

   PASS();
   return 0;
}

static int test_logo_valid_entries(void)
{
   TEST("BotLogoInit: reads valid logo entries");
   reset_config_state();
   create_temp_file("lambda\nskull\nhalf-life\n", tmp_logo_path, sizeof(tmp_logo_path));

   BotLogoInit();
   ASSERT_INT(num_logos, 3);
   ASSERT_STR(bot_logos[0], "lambda");
   ASSERT_STR(bot_logos[1], "skull");
   ASSERT_STR(bot_logos[2], "half-life");

   PASS();
   return 0;
}

static int test_logo_blank_lines_skipped(void)
{
   TEST("BotLogoInit: blank lines skipped");
   reset_config_state();
   create_temp_file("logo1\n\n\nlogo2\n", tmp_logo_path, sizeof(tmp_logo_path));

   BotLogoInit();
   ASSERT_INT(num_logos, 2);
   ASSERT_STR(bot_logos[0], "logo1");
   ASSERT_STR(bot_logos[1], "logo2");

   PASS();
   return 0;
}

static int test_logo_newline_strip(void)
{
   TEST("BotLogoInit: trailing newline stripped");
   reset_config_state();
   create_temp_file("mylogo\n", tmp_logo_path, sizeof(tmp_logo_path));

   BotLogoInit();
   ASSERT_INT(num_logos, 1);
   ASSERT_STR(bot_logos[0], "mylogo");

   PASS();
   return 0;
}

static int test_logo_no_trailing_newline(void)
{
   TEST("BotLogoInit: entry without trailing newline");
   reset_config_state();
   create_temp_file("notrail", tmp_logo_path, sizeof(tmp_logo_path));

   BotLogoInit();
   ASSERT_INT(num_logos, 1);
   ASSERT_STR(bot_logos[0], "notrail");

   PASS();
   return 0;
}

static int test_logo_truncation(void)
{
   TEST("BotLogoInit: long logo name truncated to 15 chars");
   reset_config_state();
   create_temp_file("1234567890123456789\n", tmp_logo_path, sizeof(tmp_logo_path));

   BotLogoInit();
   ASSERT_INT(num_logos, 1);
   ASSERT_INT((int)strlen(bot_logos[0]), 15);

   PASS();
   return 0;
}

static int test_logo_max_limit(void)
{
   TEST("BotLogoInit: stops at MAX_BOT_LOGOS");
   reset_config_state();

   char content[4096] = "";
   for (int i = 0; i < MAX_BOT_LOGOS + 5; i++) {
      char line[32];
      snprintf(line, sizeof(line), "logo%d\n", i);
      strncat(content, line, sizeof(content) - strlen(content) - 1);
   }
   create_temp_file(content, tmp_logo_path, sizeof(tmp_logo_path));

   BotLogoInit();
   ASSERT_INT(num_logos, MAX_BOT_LOGOS);

   PASS();
   return 0;
}

// ============================================================
// BotNameInit tests
// ============================================================

static int test_name_file_missing(void)
{
   TEST("BotNameInit: missing file -> number_names stays 0");
   reset_config_state();
   snprintf(tmp_name_path, sizeof(tmp_name_path), "/tmp/jkbotti_nonexistent_XXXXXX");

   BotNameInit();
   ASSERT_INT(number_names, 0);

   PASS();
   return 0;
}

static int test_name_valid_entries(void)
{
   TEST("BotNameInit: reads valid name entries");
   reset_config_state();
   create_temp_file("Alice\nBob\nCharlie\n", tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 3);
   ASSERT_STR(bot_names[0], "Alice");
   ASSERT_STR(bot_names[1], "Bob");
   ASSERT_STR(bot_names[2], "Charlie");

   PASS();
   return 0;
}

static int test_name_blank_lines_skipped(void)
{
   TEST("BotNameInit: blank lines skipped");
   reset_config_state();
   create_temp_file("Name1\n\n\nName2\n", tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 2);
   ASSERT_STR(bot_names[0], "Name1");
   ASSERT_STR(bot_names[1], "Name2");

   PASS();
   return 0;
}

static int test_name_newline_strip(void)
{
   TEST("BotNameInit: trailing newline stripped");
   reset_config_state();
   create_temp_file("TestBot\n", tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 1);
   ASSERT_STR(bot_names[0], "TestBot");

   PASS();
   return 0;
}

static int test_name_sanitization(void)
{
   TEST("BotNameInit: control chars sanitized from names");
   reset_config_state();
   create_temp_file("Bot\x01Name\n", tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 1);
   ASSERT_STR(bot_names[0], "BotName");

   PASS();
   return 0;
}

static int test_name_all_invalid_becomes_empty_skipped(void)
{
   TEST("BotNameInit: all-invalid name becomes empty, skipped");
   reset_config_state();
   create_temp_file("\x01\x02\x03\nValidBot\n", tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 1);
   ASSERT_STR(bot_names[0], "ValidBot");

   PASS();
   return 0;
}

static int test_name_truncation(void)
{
   TEST("BotNameInit: long name truncated to BOT_NAME_LEN");
   reset_config_state();
   create_temp_file("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890EXTRA\n",
                    tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 1);
   ASSERT_INT((int)strlen(bot_names[0]), BOT_NAME_LEN);

   PASS();
   return 0;
}

static int test_name_max_limit(void)
{
   TEST("BotNameInit: stops at MAX_BOT_NAMES");
   reset_config_state();

   char content[8192] = "";
   for (int i = 0; i < MAX_BOT_NAMES + 5; i++) {
      char line[64];
      snprintf(line, sizeof(line), "Bot%d\n", i);
      strncat(content, line, sizeof(content) - strlen(content) - 1);
   }
   create_temp_file(content, tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, MAX_BOT_NAMES);

   PASS();
   return 0;
}

static int test_name_high_ascii_removed(void)
{
   TEST("BotNameInit: high-ASCII chars removed by sanitize");
   reset_config_state();
   create_temp_file("Bot\x80\x90Name\n", tmp_name_path, sizeof(tmp_name_path));

   BotNameInit();
   ASSERT_INT(number_names, 1);
   ASSERT_STR(bot_names[0], "BotName");

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_config_init:\n");

   printf("  BotLogoInit:\n");
   fail |= test_logo_file_missing();
   fail |= test_logo_valid_entries();
   fail |= test_logo_blank_lines_skipped();
   fail |= test_logo_newline_strip();
   fail |= test_logo_no_trailing_newline();
   fail |= test_logo_truncation();
   fail |= test_logo_max_limit();

   printf("  BotNameInit:\n");
   fail |= test_name_file_missing();
   fail |= test_name_valid_entries();
   fail |= test_name_blank_lines_skipped();
   fail |= test_name_newline_strip();
   fail |= test_name_sanitization();
   fail |= test_name_all_invalid_becomes_empty_skipped();
   fail |= test_name_truncation();
   fail |= test_name_max_limit();
   fail |= test_name_high_ascii_removed();

   cleanup_temp_files();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
