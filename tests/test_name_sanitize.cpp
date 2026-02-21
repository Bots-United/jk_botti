//
// JK_Botti - unit tests for bot_name_sanitize.h
//

#include "test_common.h"

#include "../bot_name_sanitize.h"

// Helper: copy string into mutable buffer and sanitize
static int test_sanitize(void)
{
   char buf[128];

   printf("bot_name_sanitize:\n");

   TEST("spaces preserved");
   strcpy(buf, "John Doe");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "John Doe");
   PASS();

   TEST("control chars removed");
   strcpy(buf, "AB\x01""CD");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "ABCD");
   PASS();

   TEST("consecutive invalid chars removed (skip bug)");
   strcpy(buf, "A\x01\x02""B");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "AB");
   PASS();

   TEST("double quotes removed");
   strcpy(buf, "say \"hello\"");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "say hello");
   PASS();

   TEST("DEL (0x7F) removed");
   strcpy(buf, "AB\x7F""CD");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "ABCD");
   PASS();

   TEST("boundary: 0x1F removed");
   strcpy(buf, "A\x1F""B");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "AB");
   PASS();

   TEST("boundary: 0x20 (space) kept");
   strcpy(buf, "A B");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "A B");
   PASS();

   TEST("boundary: 0x7E (~) kept");
   strcpy(buf, "A~B");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "A~B");
   PASS();

   TEST("empty string unchanged");
   strcpy(buf, "");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "");
   PASS();

   TEST("all-invalid string becomes empty");
   strcpy(buf, "\x01\x02\x03");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "");
   PASS();

   TEST("printable ASCII unchanged");
   strcpy(buf, "Bot_Name-123");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "Bot_Name-123");
   PASS();

   TEST("mixed valid and invalid chars");
   strcpy(buf, "\x01""A\x02""B\x7F""C\x03");
   bot_name_sanitize(buf);
   ASSERT_STR(buf, "ABC");
   PASS();

   return 0;
}

static int test_strip_hlds_tag(void)
{
   char buf[128];

   printf("bot_name_strip_hlds_tag:\n");

   TEST("(1)Bot -> Bot");
   strcpy(buf, "(1)Bot");
   bot_name_strip_hlds_tag(buf);
   ASSERT_STR(buf, "Bot");
   PASS();

   TEST("(9)Bot -> Bot");
   strcpy(buf, "(9)Bot");
   bot_name_strip_hlds_tag(buf);
   ASSERT_STR(buf, "Bot");
   PASS();

   TEST("(0)Bot unchanged (0 not valid)");
   strcpy(buf, "(0)Bot");
   bot_name_strip_hlds_tag(buf);
   ASSERT_STR(buf, "(0)Bot");
   PASS();

   TEST("Bot(1) unchanged (not prefix)");
   strcpy(buf, "Bot(1)");
   bot_name_strip_hlds_tag(buf);
   ASSERT_STR(buf, "Bot(1)");
   PASS();

   TEST("(1) alone -> empty string");
   strcpy(buf, "(1)");
   bot_name_strip_hlds_tag(buf);
   ASSERT_STR(buf, "");
   PASS();

   TEST("no tag, unchanged");
   strcpy(buf, "PlainBot");
   bot_name_strip_hlds_tag(buf);
   ASSERT_STR(buf, "PlainBot");
   PASS();

   return 0;
}

static int test_strip_skill_tag(void)
{
   char buf[128];
   int skill;

   printf("bot_name_strip_skill_tag:\n");

   TEST("[lvl3]Bot -> Bot, skill=3");
   strcpy(buf, "[lvl3]Bot");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "Bot");
   ASSERT_INT(skill, 3);
   PASS();

   TEST("[lvl1]Bot -> Bot, skill=1");
   strcpy(buf, "[lvl1]Bot");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "Bot");
   ASSERT_INT(skill, 1);
   PASS();

   TEST("[lvl5]Bot -> Bot, skill=5");
   strcpy(buf, "[lvl5]Bot");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "Bot");
   ASSERT_INT(skill, 5);
   PASS();

   TEST("[lvl0]Bot unchanged (0 not valid)");
   strcpy(buf, "[lvl0]Bot");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "[lvl0]Bot");
   ASSERT_INT(skill, 0);
   PASS();

   TEST("[lvl6]Bot unchanged (6 not valid)");
   strcpy(buf, "[lvl6]Bot");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "[lvl6]Bot");
   ASSERT_INT(skill, 0);
   PASS();

   TEST("Bot[lvl3] unchanged (not prefix)");
   strcpy(buf, "Bot[lvl3]");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "Bot[lvl3]");
   ASSERT_INT(skill, 0);
   PASS();

   TEST("NULL skill_out (just strip tag)");
   strcpy(buf, "[lvl3]Bot");
   bot_name_strip_skill_tag(buf, NULL);
   ASSERT_STR(buf, "Bot");
   PASS();

   TEST("no tag, unchanged");
   strcpy(buf, "PlainBot");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "PlainBot");
   ASSERT_INT(skill, 0);
   PASS();

   TEST("[lvl] alone -> empty string, skill=3");
   strcpy(buf, "[lvl3]");
   skill = 0;
   bot_name_strip_skill_tag(buf, &skill);
   ASSERT_STR(buf, "");
   ASSERT_INT(skill, 3);
   PASS();

   return 0;
}

int main(void)
{
   int rc = 0;

   printf("=== bot_name_sanitize tests ===\n\n");

   rc |= test_sanitize();
   printf("\n");
   rc |= test_strip_hlds_tag();
   printf("\n");
   rc |= test_strip_skill_tag();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
