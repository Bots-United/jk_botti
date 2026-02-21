//
// JK_Botti - unit tests for BotFindEnemy() breakable targeting modes
//
// test_bot_combat.cpp
//

#include <stdlib.h>

#include "test_common.h"

#include "engine_mock.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "player.h"

// These are declared extern in bot_combat.cpp (not in any header)
extern int bot_shoot_breakables;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern qboolean b_botdontshoot;

// ============================================================
// Test helpers
// ============================================================

static void setup_bot_for_test(bot_t &pBot, edict_t *pEdict)
{
   memset(&pBot, 0, sizeof(pBot));
   pBot.pEdict = pEdict;
   pBot.is_used = TRUE;
   pBot.bot_skill = 2; // mid skill
   pBot.curr_waypoint_index = -1;
   pBot.waypoint_goal = -1;
   pBot.current_weapon_index = -1;
   pBot.f_primary_charging = -1;
   pBot.f_secondary_charging = -1;
   pBot.f_bot_see_enemy_time = -1;
   pBot.v_bot_see_enemy_origin = Vector(-99999, -99999, -99999);

   pEdict->v.origin = Vector(0, 0, 0);
   pEdict->v.v_angle = Vector(0, 0, 0);
   pEdict->v.view_ofs = Vector(0, 0, 28);
   pEdict->v.health = 100;
   pEdict->v.deadflag = DEAD_NO;
   pEdict->v.takedamage = DAMAGE_YES;
   pEdict->v.solid = SOLID_BBOX;
   pEdict->v.flags = FL_CLIENT | FL_FAKECLIENT;
}

static edict_t *create_breakable(const Vector &origin, float health)
{
   edict_t *e = mock_alloc_edict();
   mock_set_classname(e, "func_breakable");
   e->v.origin = origin;
   e->v.health = health;
   e->v.takedamage = DAMAGE_YES;
   e->v.solid = SOLID_BSP;
   e->v.deadflag = DEAD_NO;
   // For BSP entities, UTIL_GetOrigin uses VecBModelOrigin (absmin + size/2)
   // Set absmin/size so the computed origin matches what we want
   e->v.absmin = origin - Vector(16, 16, 16);
   e->v.size = Vector(32, 32, 32);
   // Also set mins/maxs for BSP (absolute coordinates)
   e->v.mins = origin - Vector(16, 16, 16);
   e->v.maxs = origin + Vector(16, 16, 16);
   return e;
}

// Trace function that always reports "no hit"
static void trace_nohit(const float *v1, const float *v2,
                        int fNoMonsters, int hullNumber,
                        edict_t *pentToSkip, TraceResult *ptr)
{
   (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   ptr->flFraction = 1.0f;
   ptr->pHit = NULL;
   ptr->vecEndPos[0] = v2[0];
   ptr->vecEndPos[1] = v2[1];
   ptr->vecEndPos[2] = v2[2];
}

// Trace context for targeted breakable
static edict_t *g_trace_hit_edict = NULL;

// Trace function that reports hitting a specific edict on hull traces
static void trace_hit_breakable_hull(const float *v1, const float *v2,
                                     int fNoMonsters, int hullNumber,
                                     edict_t *pentToSkip, TraceResult *ptr)
{
   (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   ptr->flFraction = 0.5f;
   ptr->pHit = g_trace_hit_edict;
   ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
   ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
   ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
}

// ============================================================
// Test cases
// ============================================================

static int test_mode0_no_breakables_targeted(void)
{
   printf("BotFindEnemy mode 0 (disabled):\n");
   mock_reset();

   // Create bot
   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   // Create a breakable in view
   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   // Set trace hooks to make everything visible
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   // Mode 0: breakable targeting disabled
   bot_shoot_breakables = 0;

   TEST("mode 0: breakable not targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

static int test_mode1_breakable_targeted(void)
{
   printf("BotFindEnemy mode 1 (all breakables):\n");
   mock_reset();

   // Create bot facing toward breakable
   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0); // facing +X

   // Create a breakable ahead of bot
   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   // Traces pass through (visible)
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   // Mode 1: target all visible breakables
   bot_shoot_breakables = 1;

   TEST("mode 1: visible breakable targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pBreakable);
   PASS();

   return 0;
}

static int test_mode1_dead_breakable_not_targeted(void)
{
   printf("BotFindEnemy mode 1 (dead breakable):\n");
   mock_reset();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
   pBreakable->v.deadflag = DEAD_DEAD;
   mock_add_breakable(pBreakable, TRUE);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   bot_shoot_breakables = 1;

   TEST("mode 1: dead breakable not targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

static int test_mode2_breakable_not_blocking(void)
{
   printf("BotFindEnemy mode 2 (path-only, not blocking):\n");
   mock_reset();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   // Breakable off to the side (not in path)
   edict_t *pBreakable = create_breakable(Vector(0, 200, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   // Hull trace doesn't hit anything (no blocking)
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   // Mode 2: only target breakables blocking path
   bot_shoot_breakables = 2;

   TEST("mode 2: non-blocking breakable not targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

static int test_mode2_breakable_blocking_path(void)
{
   printf("BotFindEnemy mode 2 (path-only, blocking):\n");
   mock_reset();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0); // facing +X

   // Breakable ahead in path
   edict_t *pBreakable = create_breakable(Vector(50, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   // Hull trace hits the breakable
   g_trace_hit_edict = pBreakable;
   mock_trace_hull_fn = trace_hit_breakable_hull;
   // Line trace (for FVisible) passes through
   mock_trace_line_fn = trace_nohit;

   bot_shoot_breakables = 2;

   TEST("mode 2: blocking breakable targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pBreakable);
   PASS();

   return 0;
}

static int test_mode2_dead_breakable_blocking(void)
{
   printf("BotFindEnemy mode 2 (dead breakable blocking):\n");
   mock_reset();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pBreakable = create_breakable(Vector(50, 0, 0), 50);
   pBreakable->v.deadflag = DEAD_DEAD;
   mock_add_breakable(pBreakable, TRUE);

   g_trace_hit_edict = pBreakable;
   mock_trace_hull_fn = trace_hit_breakable_hull;
   mock_trace_line_fn = trace_nohit;

   bot_shoot_breakables = 2;

   TEST("mode 2: dead breakable blocking not targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

static int test_mode1_unbreakable_glass_not_targeted(void)
{
   printf("BotFindEnemy mode 1 (unbreakable glass):\n");
   mock_reset();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
   // Register as unbreakable glass (material_breakable = FALSE)
   mock_add_breakable(pBreakable, FALSE);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   bot_shoot_breakables = 1;

   TEST("mode 1: unbreakable glass not targeted");
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

static int test_mode1_high_health_breakable_skipped(void)
{
   printf("BotFindEnemy mode 1 (high health breakable):\n");
   mock_reset();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   // Breakable with health > 8000 should be skipped
   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 9000);
   mock_add_breakable(pBreakable, TRUE);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   bot_shoot_breakables = 1;

   TEST("mode 1: high health breakable (>8000) skipped");
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

// ============================================================
// Main
// ============================================================

int main(void)
{
   int rc = 0;

   printf("=== BotFindEnemy breakable mode tests ===\n\n");

   rc |= test_mode0_no_breakables_targeted();
   printf("\n");
   rc |= test_mode1_breakable_targeted();
   printf("\n");
   rc |= test_mode1_dead_breakable_not_targeted();
   printf("\n");
   rc |= test_mode1_unbreakable_glass_not_targeted();
   printf("\n");
   rc |= test_mode1_high_health_breakable_skipped();
   printf("\n");
   rc |= test_mode2_breakable_not_blocking();
   printf("\n");
   rc |= test_mode2_breakable_blocking_path();
   printf("\n");
   rc |= test_mode2_dead_breakable_blocking();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
