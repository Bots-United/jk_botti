//
// JK_Botti - unit tests for bot_skill.cpp
//
// test_bot_skill.cpp
//
// Uses the #include-the-.cpp approach to access default_skill_settings directly.
//

#include <stdlib.h>

// Include the source file under test (brings in all its headers + data)
#include "../bot_skill.cpp"

#include "test_common.h"

// ============================================================
// test_ResetSkillsToDefault
// ============================================================

static int test_copies_all_defaults(void)
{
   TEST("ResetSkillsToDefault copies all defaults");

   // Corrupt skill_settings first
   memset(skill_settings, 0xFF, sizeof(skill_settings));

   ResetSkillsToDefault();

   ASSERT_INT(memcmp(skill_settings, default_skill_settings, sizeof(skill_settings)), 0);
   PASS();
   return 0;
}

static int test_overwrites_modified_values(void)
{
   TEST("ResetSkillsToDefault overwrites modified values");

   ResetSkillsToDefault();

   // Corrupt one entry
   skill_settings[0].turn_skill = -999.0f;
   skill_settings[0].pause_frequency = 9999;
   skill_settings[0].can_longjump = FALSE;

   ResetSkillsToDefault();

   ASSERT_TRUE(skill_settings[0].turn_skill == default_skill_settings[0].turn_skill);
   ASSERT_INT(skill_settings[0].pause_frequency, default_skill_settings[0].pause_frequency);
   ASSERT_INT(skill_settings[0].can_longjump, default_skill_settings[0].can_longjump);
   PASS();
   return 0;
}

static int test_independent_from_defaults(void)
{
   TEST("ResetSkillsToDefault: runtime independent from defaults");

   ResetSkillsToDefault();

   // Modify runtime copy
   skill_settings[2].turn_skill = -1.0f;
   skill_settings[2].pause_frequency = 9999;

   // Default should be unchanged
   ASSERT_TRUE(default_skill_settings[2].turn_skill > 0);
   ASSERT_TRUE(default_skill_settings[2].pause_frequency != 9999);
   PASS();
   return 0;
}

static int test_idempotent(void)
{
   TEST("ResetSkillsToDefault is idempotent");

   ResetSkillsToDefault();
   bot_skill_settings_t snapshot[5];
   memcpy(snapshot, skill_settings, sizeof(skill_settings));

   ResetSkillsToDefault();

   ASSERT_INT(memcmp(skill_settings, snapshot, sizeof(skill_settings)), 0);
   PASS();
   return 0;
}

// ============================================================
// test_DefaultSkillData
// ============================================================

static int test_min_less_than_max(void)
{
   TEST("Default data: min < max for all min/max pairs");

   for (int i = 0; i < 5; i++)
   {
      const bot_skill_settings_t &s = default_skill_settings[i];

      ASSERT_TRUE(s.pause_time_min < s.pause_time_max);
      ASSERT_TRUE(s.react_delay_min < s.react_delay_max);
      ASSERT_TRUE(s.weaponchange_rate_min < s.weaponchange_rate_max);
      ASSERT_TRUE(s.track_sound_time_min < s.track_sound_time_max);
   }

   PASS();
   return 0;
}

static int test_pause_frequency_range(void)
{
   TEST("Default data: pause_frequency in range 0-1000");

   for (int i = 0; i < 5; i++)
   {
      ASSERT_TRUE(default_skill_settings[i].pause_frequency >= 0);
      ASSERT_TRUE(default_skill_settings[i].pause_frequency <= 1000);
   }

   PASS();
   return 0;
}

static int test_frequency_ints_range(void)
{
   TEST("Default data: frequency ints in range 0-100");

   for (int i = 0; i < 5; i++)
   {
      const bot_skill_settings_t &s = default_skill_settings[i];

      ASSERT_TRUE(s.keep_optimal_dist >= 0 && s.keep_optimal_dist <= 100);
      ASSERT_TRUE(s.random_jump_frequency >= 0 && s.random_jump_frequency <= 100);
      ASSERT_TRUE(s.random_jump_duck_frequency >= 0 && s.random_jump_duck_frequency <= 100);
      ASSERT_TRUE(s.random_duck_frequency >= 0 && s.random_duck_frequency <= 100);
      ASSERT_TRUE(s.random_longjump_frequency >= 0 && s.random_longjump_frequency <= 100);
   }

   PASS();
   return 0;
}

static int test_positive_aiming_values(void)
{
   TEST("Default data: positive aiming values");

   for (int i = 0; i < 5; i++)
   {
      const bot_skill_settings_t &s = default_skill_settings[i];

      ASSERT_TRUE(s.turn_skill > 0);
      ASSERT_TRUE(s.turn_slowness > 0);
      ASSERT_TRUE(s.updown_turn_ration > 0);
   }

   PASS();
   return 0;
}

static int test_nonneg_ping_emu_values(void)
{
   TEST("Default data: non-negative ping emu values");

   for (int i = 0; i < 5; i++)
   {
      const bot_skill_settings_t &s = default_skill_settings[i];

      ASSERT_TRUE(s.ping_emu_latency >= 0);
      ASSERT_TRUE(s.ping_emu_speed_varitation >= 0);
      ASSERT_TRUE(s.ping_emu_position_varitation >= 0);
   }

   PASS();
   return 0;
}

static int test_longjump_best_worst(void)
{
   TEST("Default data: best can longjump, worst cannot");

   ASSERT_INT(default_skill_settings[BEST_BOT_LEVEL].can_longjump, TRUE);
   ASSERT_INT(default_skill_settings[WORST_BOT_LEVEL].can_longjump, FALSE);
   PASS();
   return 0;
}

static int test_turn_skill_decreasing(void)
{
   TEST("Default data: turn_skill decreasing with level");

   for (int i = 0; i < 4; i++)
   {
      ASSERT_TRUE(default_skill_settings[i].turn_skill >
                  default_skill_settings[i + 1].turn_skill);
   }

   PASS();
   return 0;
}

static int test_react_delay_min_increasing(void)
{
   TEST("Default data: react_delay_min increasing with level");

   for (int i = 0; i < 4; i++)
   {
      ASSERT_TRUE(default_skill_settings[i].react_delay_min <
                  default_skill_settings[i + 1].react_delay_min);
   }

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_ResetSkillsToDefault:\n");
   fail |= test_copies_all_defaults();
   fail |= test_overwrites_modified_values();
   fail |= test_independent_from_defaults();
   fail |= test_idempotent();

   printf("test_DefaultSkillData:\n");
   fail |= test_min_less_than_max();
   fail |= test_pause_frequency_range();
   fail |= test_frequency_ints_range();
   fail |= test_positive_aiming_values();
   fail |= test_nonneg_ping_emu_values();
   fail |= test_longjump_best_worst();
   fail |= test_turn_skill_decreasing();
   fail |= test_react_delay_min_increasing();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);

   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
