//
// JK_Botti - unit tests for bot_navigate.cpp
//
// test_bot_navigate.cpp
//

#include <stdlib.h>
#include <math.h>

#include "test_common.h"

#include "engine_mock.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "player.h"

// Externs from bot_navigate.cpp (not in any header)
extern int num_waypoints;
extern qboolean is_team_play;
extern qboolean checked_teamplay;

// ============================================================
// Test helpers
// ============================================================

static void setup_bot_for_test(bot_t &pBot, edict_t *pEdict)
{
   memset(&pBot, 0, sizeof(pBot));
   pBot.pEdict = pEdict;
   pBot.is_used = TRUE;
   pBot.bot_skill = 2;
   pBot.curr_waypoint_index = -1;
   pBot.waypoint_goal = -1;
   pBot.current_weapon_index = -1;
   pBot.f_primary_charging = -1;
   pBot.f_secondary_charging = -1;
   pBot.f_bot_see_enemy_time = -1;
   pBot.v_bot_see_enemy_origin = Vector(-99999, -99999, -99999);
   pBot.f_frame_time = 0.1f; // 100ms frame

   pEdict->v.origin = Vector(0, 0, 0);
   pEdict->v.v_angle = Vector(0, 0, 0); // facing +X
   pEdict->v.view_ofs = Vector(0, 0, 28);
   pEdict->v.health = 100;
   pEdict->v.deadflag = DEAD_NO;
   pEdict->v.takedamage = DAMAGE_YES;
   pEdict->v.solid = SOLID_BBOX;
   pEdict->v.flags = FL_CLIENT | FL_FAKECLIENT;
}

#define ASSERT_FLOAT_NEAR(actual, expected, eps) do { \
   float _a = (actual), _e = (expected); \
   if (fabs(_a - _e) > (eps)) { \
      printf("FAIL\n    expected: %f (+-%.6f)\n    got:      %f\n", \
             (double)_e, (double)(eps), (double)_a); \
      return 1; \
   } \
} while(0)

// ============================================================
// Trace helpers
// ============================================================

// Default: all traces clear (no hit)
static void trace_all_clear(const float *v1, const float *v2,
                            int fNoMonsters, int hullNumber,
                            edict_t *pentToSkip, TraceResult *ptr)
{
   (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   ptr->flFraction = 1.0f;
   ptr->pHit = NULL;
   ptr->vecEndPos[0] = v2[0];
   ptr->vecEndPos[1] = v2[1];
   ptr->vecEndPos[2] = v2[2];
}

// All traces hit (wall)
static void trace_all_hit(const float *v1, const float *v2,
                          int fNoMonsters, int hullNumber,
                          edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   ptr->flFraction = 0.5f;
   ptr->pHit = &mock_edicts[0]; // worldspawn
   ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
   ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
   ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   ptr->vecPlaneNormal[0] = 0;
   ptr->vecPlaneNormal[1] = 0;
   ptr->vecPlaneNormal[2] = 1;
}

// ============================================================
// 1. BotChangePitch tests
// ============================================================

static int test_change_pitch_both_positive_current_gt_ideal(void)
{
   TEST("BotChangePitch: both positive, current > ideal");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 50.0f;
   pEdict->v.idealpitch = 30.0f;
   float diff = BotChangePitch(bot, 100.0f);

   // speed = 100 * 0.1 = 10, diff = 20, speed < diff so speed = 10
   // current > ideal: current -= speed -> 50 - 10 = 40
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 40.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 20.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_both_positive_current_lt_ideal(void)
{
   TEST("BotChangePitch: both positive, current < ideal");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 10.0f;
   pEdict->v.idealpitch = 50.0f;
   float diff = BotChangePitch(bot, 100.0f);

   // speed = 10, diff = 40 -> speed stays 10
   // current < ideal: current += speed -> 10 + 10 = 20
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 20.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 40.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_pos_to_neg(void)
{
   TEST("BotChangePitch: positive -> negative crossover");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 10.0f;
   pEdict->v.idealpitch = -30.0f;
   float diff = BotChangePitch(bot, 100.0f);

   // current >= 0, ideal < 0
   // current_180 = 10 - 180 = -170
   // -170 > -30? NO -> current -= speed -> 10 - 10 = 0
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 40.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_neg_to_pos(void)
{
   TEST("BotChangePitch: negative -> positive crossover");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = -10.0f;
   pEdict->v.idealpitch = 30.0f;
   float diff = BotChangePitch(bot, 100.0f);

   // current < 0, ideal >= 0
   // current_180 = -10 + 180 = 170
   // 170 > 30? YES -> current += speed -> -10 + 10 = 0
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 40.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_both_negative(void)
{
   TEST("BotChangePitch: both negative");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = -50.0f;
   pEdict->v.idealpitch = -20.0f;
   float diff = BotChangePitch(bot, 100.0f);

   // both negative, current < ideal: current += speed -> -50 + 10 = -40
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -40.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_speed_clamped(void)
{
   TEST("BotChangePitch: speed clamped when diff < speed");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 10.0f;
   pEdict->v.idealpitch = 12.0f;
   float diff = BotChangePitch(bot, 100.0f);

   // speed = 10, diff = 2, diff < speed -> speed clamped to 2
   // current < ideal: current += 2 -> 12
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 12.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 2.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 2. BotChangeYaw tests
// ============================================================

static int test_change_yaw_both_positive_current_gt_ideal(void)
{
   TEST("BotChangeYaw: both positive, current > ideal");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 90.0f;
   pEdict->v.ideal_yaw = 60.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   // speed = 10, diff = 30 -> speed = 10
   // current > ideal: current -= speed -> 90 - 10 = 80
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 80.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_both_positive_current_lt_ideal(void)
{
   TEST("BotChangeYaw: both positive, current < ideal");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 30.0f;
   pEdict->v.ideal_yaw = 90.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 40.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 60.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_pos_to_neg(void)
{
   TEST("BotChangeYaw: positive -> negative crossover");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 10.0f;
   pEdict->v.ideal_yaw = -30.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   // current_180 = 10 - 180 = -170
   // -170 > -30? NO -> current -= speed -> 10 - 10 = 0
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 40.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_neg_to_pos(void)
{
   TEST("BotChangeYaw: negative -> positive crossover");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = -10.0f;
   pEdict->v.ideal_yaw = 30.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   // current_180 = -10 + 180 = 170
   // 170 > 30? YES -> current += speed -> -10 + 10 = 0
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 40.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_both_negative(void)
{
   TEST("BotChangeYaw: both negative");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = -80.0f;
   pEdict->v.ideal_yaw = -40.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   // both negative, current < ideal: current += speed -> -80 + 10 = -70
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, -70.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 40.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_speed_clamped(void)
{
   TEST("BotChangeYaw: speed clamped when diff < speed");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 45.0f;
   pEdict->v.ideal_yaw = 48.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   // speed = 10, diff = 3, diff < speed -> speed = 3
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 48.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 3.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 3. BotCanJumpUp tests
// ============================================================

// Trace context for BotCanJumpUp: block based on direction
// We use v_angle.y=0, so v_forward=(1,0,0), v_right=(0,-1,0)

// For BotCanJumpUp, horizontal forward traces check if path is blocked.
// If clear at normal height -> test downward trace.
// We need to control which traces hit and which don't.

static int g_jump_trace_call;
static int g_jump_block_mask; // bit mask for which trace calls to block

static void trace_jump_selective(const float *v1, const float *v2,
                                 int fNoMonsters, int hullNumber,
                                 edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_jump_trace_call++;
   if (g_jump_block_mask & (1 << call))
   {
      ptr->flFraction = 0.5f;
      ptr->pHit = &mock_edicts[0];
      ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
      ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
      ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   }
   else
   {
      ptr->flFraction = 1.0f;
      ptr->pHit = NULL;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   }
}

static int test_can_jump_up_clear_normal_height(void)
{
   TEST("BotCanJumpUp: clear at normal height -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // All traces clear -> no check_duck path
   // Trace order for normal (no duck):
   //  0: center horizontal forward
   //  1: right side horizontal forward
   //  2: left side horizontal forward
   //  3: center downward
   //  4: right side downward
   //  5: left side downward
   // All clear except downward traces should NOT hit (fraction=1.0 means
   // no obstruction above jump point -> good)
   mock_trace_hull_fn = trace_all_clear;
   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, TRUE);
   ASSERT_INT(bDuckJump, FALSE);
   PASS();
   return 0;
}

static int test_can_jump_up_blocked_normal_clear_duck(void)
{
   TEST("BotCanJumpUp: blocked normal, clear duck -> TRUE + duck");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block center at normal height (trace 0), triggering duck check
   // Duck traces (3 horizontal + 3 downward) all clear
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0); // block first trace only
   mock_trace_hull_fn = trace_jump_selective;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, TRUE);
   ASSERT_INT(bDuckJump, TRUE);
   PASS();
   return 0;
}

static int test_can_jump_up_all_blocked(void)
{
   TEST("BotCanJumpUp: blocked at all heights -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block everything
   mock_trace_hull_fn = trace_all_hit;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// Trace for BotCanJumpUp bug #1 test: block based on source position.
// With v_angle.y=0: v_forward=(1,0,0), v_right=(0,-1,0)
// v_right * 16 = (0,-16,0) => right side source Y = -16
// v_right * -16 = (0,16,0) => left side source Y = 16
// We block normal-height center (Z=10, Y=0) to enter duck path,
// then block any horizontal trace from the right side (source Y = -16).
// With the bug, the right side is never checked (duplicate left),
// so the function returns TRUE. After fix, it returns FALSE.
static void trace_jump_block_right_side(const float *v1, const float *v2,
                                        int fNoMonsters, int hullNumber,
                                        edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   float dz = v2[2] - v1[2];
   int is_horizontal = (fabs(dz) < 1.0f);
   int is_normal_center = is_horizontal && (fabs(v1[1]) < 1.0f) && (fabs(v1[2] - 10.0f) < 1.0f);
   int is_right_side = is_horizontal && (v1[1] < -8.0f); // Y = -16 for right

   if (is_normal_center || is_right_side)
   {
      ptr->flFraction = 0.5f;
      ptr->pHit = &mock_edicts[0];
      ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
      ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
      ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   }
   else
   {
      ptr->flFraction = 1.0f;
      ptr->pHit = NULL;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   }
}

static int test_can_jump_up_right_side_checked_duck(void)
{
   // Bug-detecting test: verify that the duck-jump path checks both
   // left AND right sides (not left twice). With the bug, the right
   // side (v_right * 16, source Y=-16) is never traced, so blocking
   // it has no effect and the function returns TRUE. After fix, the
   // right side IS traced and blocked, returning FALSE.
   TEST("BotCanJumpUp: right side checked at duck height (bug #1)");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   // face +X, so v_forward=(1,0,0), v_right=(0,-1,0)

   mock_trace_hull_fn = trace_jump_block_right_side;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_can_jump_up_downward_blocked(void)
{
   TEST("BotCanJumpUp: downward trace blocked -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // All horizontal clear, but first downward blocked
   // Non-duck path: traces 0,1,2 horizontal, 3,4,5 downward
   // Block trace 3 (center downward)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 3);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// ============================================================
// 4. BotCanDuckUnder tests
// ============================================================

static int test_can_duck_under_clear(void)
{
   TEST("BotCanDuckUnder: clear at duck + ceiling -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Duck path:
   // Traces 0,1,2: horizontal at duck height -> must be clear
   // Traces 3,4,5: vertical upward (ceiling check) -> must HIT
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 3) | (1 << 4) | (1 << 5);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotCanDuckUnder(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_can_duck_under_blocked_horizontal(void)
{
   TEST("BotCanDuckUnder: blocked horizontally -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block first horizontal trace (center)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotCanDuckUnder(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_can_duck_under_no_ceiling(void)
{
   TEST("BotCanDuckUnder: no ceiling -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // All traces clear -> horizontal OK but ceiling check fails
   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotCanDuckUnder(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_can_duck_under_one_side_blocked(void)
{
   TEST("BotCanDuckUnder: one horizontal side blocked -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block right side horizontal (trace 1)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 1);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotCanDuckUnder(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// ============================================================
// 5. BotCantMoveForward tests
// ============================================================

static int test_cant_move_forward_clear(void)
{
   TEST("BotCantMoveForward: clear -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_clear;
   TraceResult tr;
   qboolean result = BotCantMoveForward(bot, &tr);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_cant_move_forward_eye_blocked(void)
{
   TEST("BotCantMoveForward: blocked at eye level -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block first trace (eye level)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
   mock_trace_hull_fn = trace_jump_selective;

   TraceResult tr;
   qboolean result = BotCantMoveForward(bot, &tr);

   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_cant_move_forward_waist_blocked(void)
{
   TEST("BotCantMoveForward: blocked at waist -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Clear eye level (trace 0), block waist (trace 1)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 1);
   mock_trace_hull_fn = trace_jump_selective;

   TraceResult tr;
   qboolean result = BotCantMoveForward(bot, &tr);

   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

// ============================================================
// 6. BotStuckInCorner tests
// ============================================================

static int test_stuck_in_corner_both_blocked(void)
{
   TEST("BotStuckInCorner: both sides blocked -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean result = BotStuckInCorner(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_stuck_in_corner_one_open(void)
{
   TEST("BotStuckInCorner: one side open -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // First trace (right-ish) blocked, second (left-ish) clear
   // RANDOM_LONG2(0,1) returns 0 (mock always returns low), so right_first=0
   // Trace 0: forward+left blocked, trace 1: forward+right clear
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotStuckInCorner(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_stuck_in_corner_both_open(void)
{
   TEST("BotStuckInCorner: both sides open -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotStuckInCorner(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// ============================================================
// 7. BotEdgeForward/Right/Left tests
// ============================================================

// BotEdge functions use UTIL_TraceDuck (head_hull=1) which still goes
// through mock_trace_hull_fn.
// Each BotEdge function does 3 traces:
//   0: trace down to ground (must hit to get ground pos)
//   1: trace forward/right/left from ground (must clear)
//   2: trace downward from offset (clear = edge, hit = no edge)

static int test_edge_forward_drop(void)
{
   TEST("BotEdgeForward: drop detected -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   // Trace 0: ground hit, trace 1: clear ahead, trace 2: clear down (edge!)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0); // only ground trace hits
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotEdgeForward(bot, Vector(100, 0, 0));
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_edge_forward_solid(void)
{
   TEST("BotEdgeForward: solid ground -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   // Trace 0: ground hit, trace 1: clear ahead, trace 2: hit (ground solid)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0) | (1 << 2);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotEdgeForward(bot, Vector(100, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_edge_right_drop(void)
{
   TEST("BotEdgeRight: drop detected -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotEdgeRight(bot, Vector(100, 0, 0));
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_edge_right_solid(void)
{
   TEST("BotEdgeRight: solid ground -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0) | (1 << 2);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotEdgeRight(bot, Vector(100, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_edge_left_drop(void)
{
   TEST("BotEdgeLeft: drop detected -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotEdgeLeft(bot, Vector(100, 0, 0));
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_edge_left_solid(void)
{
   TEST("BotEdgeLeft: solid ground -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0) | (1 << 2);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean result = BotEdgeLeft(bot, Vector(100, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// ============================================================
// 8. BotCheckWallOnLeft/Right/Forward/Back tests
// ============================================================

static int test_wall_on_left_hit(void)
{
   TEST("BotCheckWallOnLeft: wall -> TRUE + timestamp");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean result = BotCheckWallOnLeft(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_FLOAT_NEAR(bot.f_wall_on_left, (float)gpGlobals->time, 0.01f);
   PASS();
   return 0;
}

static int test_wall_on_left_clear(void)
{
   TEST("BotCheckWallOnLeft: no wall -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotCheckWallOnLeft(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_wall_on_right_hit(void)
{
   TEST("BotCheckWallOnRight: wall -> TRUE + timestamp");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean result = BotCheckWallOnRight(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_FLOAT_NEAR(bot.f_wall_on_right, (float)gpGlobals->time, 0.01f);
   PASS();
   return 0;
}

static int test_wall_on_forward_hit(void)
{
   TEST("BotCheckWallOnForward: wall -> TRUE (no timestamp)");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean result = BotCheckWallOnForward(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_wall_on_forward_no_corrupt(void)
{
   // Bug-detecting test: BotCheckWallOnForward should NOT modify
   // f_wall_on_right. Before the fix, it copied the wall-on-right
   // timestamp code from BotCheckWallOnRight.
   TEST("BotCheckWallOnForward: doesn't corrupt f_wall_on_right (bug #2)");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_wall_on_right = 0.0f; // not set

   mock_trace_hull_fn = trace_all_hit;

   BotCheckWallOnForward(bot);

   // f_wall_on_right should remain 0, not be set to gpGlobals->time
   ASSERT_FLOAT_NEAR(bot.f_wall_on_right, 0.0f, 0.01f);
   PASS();
   return 0;
}

static int test_wall_on_back_no_corrupt(void)
{
   // Bug-detecting test: BotCheckWallOnBack should NOT modify
   // f_wall_on_right either.
   TEST("BotCheckWallOnBack: doesn't corrupt f_wall_on_right (bug #2)");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_wall_on_right = 0.0f;

   mock_trace_hull_fn = trace_all_hit;

   BotCheckWallOnBack(bot);

   ASSERT_FLOAT_NEAR(bot.f_wall_on_right, 0.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 9. BotLookForDrop tests
// ============================================================

// BotLookForDrop trace sequence (facing +X):
// Trace 0: horizontal forward (clear -> continue to drop check)
// Trace 1: downward from forward point (clear = too far drop)
// If drop too far:
//   Trace 2: downward from directly below bot (checking ground)
//   If ground below:
//     call BotTurnAtWall
//   else:
//     Turning loop: up to 6 iterations, each with:
//       Trace N: horizontal in new direction
//       Trace N+1: downward from that point (hit = safe ground found)

static int g_lookdrop_trace_call;

// Trace for BotLookForDrop: no drop scenario - ground ahead is fine
static void trace_no_drop(const float *v1, const float *v2,
                          int fNoMonsters, int hullNumber,
                          edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_lookdrop_trace_call++;
   if (call == 0)
   {
      // horizontal forward: blocked (something in the way -> no drop)
      ptr->flFraction = 0.5f;
      ptr->pHit = &mock_edicts[0];
      ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
      ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
      ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   }
   else
   {
      ptr->flFraction = 1.0f;
      ptr->pHit = NULL;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   }
}

static int test_look_for_drop_no_drop(void)
{
   TEST("BotLookForDrop: no drop -> no yaw change");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_max_speed = 100.0f;

   float original_yaw = pEdict->v.ideal_yaw;

   g_lookdrop_trace_call = 0;
   mock_trace_hull_fn = trace_no_drop;

   BotLookForDrop(bot);

   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, original_yaw, 0.01f);
   PASS();
   return 0;
}

// Drop ahead, turning loop finds safe ground.
// The turning loop checks: for each turned direction, trace horizontal
// (clear?), then trace down (hit ground = safe). The bug had the
// condition inverted: it exited on MORE drops instead of on finding ground.
static void trace_drop_with_safe_turn(const float *v1, const float *v2,
                                      int fNoMonsters, int hullNumber,
                                      edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_lookdrop_trace_call++;

   // Call 0: horizontal forward -> clear (can check for drop)
   // Call 1: downward -> clear (too far drop! need_to_turn = TRUE)
   // Call 2: downward from below bot -> clear (no ground directly below,
   //         enter turning loop)
   // Turning loop iteration 1:
   //   Call 3: horizontal in turned direction -> clear
   //   Call 4: downward -> HIT (found safe ground! should exit)
   if (call == 4)
   {
      // Ground found in turned direction
      ptr->flFraction = 0.5f;
      ptr->pHit = &mock_edicts[0];
      ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
      ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
      ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   }
   else
   {
      // Everything else clear
      ptr->flFraction = 1.0f;
      ptr->pHit = NULL;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   }
}

static int test_look_for_drop_safe_turn(void)
{
   TEST("BotLookForDrop: drop ahead, safe dir found -> yaw changes");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_max_speed = 100.0f;

   float original_yaw = pEdict->v.ideal_yaw;

   g_lookdrop_trace_call = 0;
   mock_trace_hull_fn = trace_drop_with_safe_turn;

   BotLookForDrop(bot);

   // Yaw should have changed (turned to avoid drop)
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw - original_yaw) > 1.0f);
   PASS();
   return 0;
}

// Bug-detecting test: loop should exit when it finds ground (fraction < 1),
// NOT when it finds another drop (fraction > 0.999999)
static void trace_drop_bug_detect(const float *v1, const float *v2,
                                  int fNoMonsters, int hullNumber,
                                  edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_lookdrop_trace_call++;

   // Call 0: horizontal forward -> clear
   // Call 1: downward -> clear (drop too far)
   // Call 2: downward from below bot -> clear (enter turning loop)
   // Turning loop:
   //   Call 3: horiz turn1 -> clear
   //   Call 4: down turn1 -> clear (still a drop - should NOT exit)
   //   Call 5: horiz turn2 -> clear
   //   Call 6: down turn2 -> HIT (safe ground found - should exit)
   if (call == 6)
   {
      ptr->flFraction = 0.5f;
      ptr->pHit = &mock_edicts[0];
      ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
      ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
      ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   }
   else
   {
      ptr->flFraction = 1.0f;
      ptr->pHit = NULL;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   }
}

static int test_look_for_drop_exit_on_ground(void)
{
   // With the bug, the loop exits on call 4 (more drops = fraction > 0.999999)
   // and turns only 30 degrees. After fix, it continues past call 4, exits
   // on call 6 (ground found = fraction < 1.0), and turns 60 degrees.
   TEST("BotLookForDrop: loop exits on ground, not drops (bug #3)");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_max_speed = 100.0f;
   pEdict->v.v_angle.y = 0.0f;

   g_lookdrop_trace_call = 0;
   mock_trace_hull_fn = trace_drop_bug_detect;

   BotLookForDrop(bot);

   // With fix: loop runs 2 iterations (2 * 30 = 60 degrees turn)
   // Direction is random (+1 or -1), so ideal_yaw is +60 or -60
   float turned = fabs(pEdict->v.ideal_yaw);
   ASSERT_FLOAT_NEAR(turned, 60.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 10. BotUpdateTrackSoundGoal tests
// ============================================================

static int test_track_sound_not_tracking(void)
{
   TEST("BotUpdateTrackSoundGoal: not tracking -> FALSE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // wpt_goal_type defaults to 0 (WPT_GOAL_NONE)
   qboolean result = BotUpdateTrackSoundGoal(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_track_sound_low_health(void)
{
   TEST("BotUpdateTrackSoundGoal: low health -> stops tracking");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   bot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
   bot.f_track_sound_time = gpGlobals->time + 10.0f;
   bot.b_low_health = TRUE;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;
   bot.waypoint_goal = 5;

   qboolean result = BotUpdateTrackSoundGoal(bot);
   ASSERT_INT(result, FALSE);
   ASSERT_INT(bot.waypoint_goal, -1);
   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_NONE);
   PASS();
   return 0;
}

static int test_track_sound_no_ammo(void)
{
   TEST("BotUpdateTrackSoundGoal: no ammo -> stops tracking");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   bot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
   bot.f_track_sound_time = gpGlobals->time + 10.0f;
   bot.b_low_health = FALSE;
   bot.b_has_enough_ammo_for_good_weapon = FALSE;
   bot.waypoint_goal = 5;

   qboolean result = BotUpdateTrackSoundGoal(bot);
   ASSERT_INT(result, FALSE);
   ASSERT_INT(bot.waypoint_goal, -1);
   PASS();
   return 0;
}

static int test_track_sound_still_valid(void)
{
   TEST("BotUpdateTrackSoundGoal: still valid -> TRUE");
   mock_reset();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   bot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
   bot.f_track_sound_time = gpGlobals->time + 10.0f;
   bot.b_low_health = FALSE;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;
   bot.waypoint_goal = 5;

   // CSoundEnt::ActiveList returns SOUNDLIST_EMPTY,
   // so BotGetSoundWaypoint returns -1, waypoint_goal becomes -1
   // but still returns TRUE
   qboolean result = BotUpdateTrackSoundGoal(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

// ============================================================
// Main
// ============================================================

int main(void)
{
   int failures = 0;

   printf("=== BotChangePitch tests ===\n");
   failures += test_change_pitch_both_positive_current_gt_ideal();
   failures += test_change_pitch_both_positive_current_lt_ideal();
   failures += test_change_pitch_pos_to_neg();
   failures += test_change_pitch_neg_to_pos();
   failures += test_change_pitch_both_negative();
   failures += test_change_pitch_speed_clamped();

   printf("=== BotChangeYaw tests ===\n");
   failures += test_change_yaw_both_positive_current_gt_ideal();
   failures += test_change_yaw_both_positive_current_lt_ideal();
   failures += test_change_yaw_pos_to_neg();
   failures += test_change_yaw_neg_to_pos();
   failures += test_change_yaw_both_negative();
   failures += test_change_yaw_speed_clamped();

   printf("=== BotCanJumpUp tests ===\n");
   failures += test_can_jump_up_clear_normal_height();
   failures += test_can_jump_up_blocked_normal_clear_duck();
   failures += test_can_jump_up_all_blocked();
   failures += test_can_jump_up_right_side_checked_duck();
   failures += test_can_jump_up_downward_blocked();

   printf("=== BotCanDuckUnder tests ===\n");
   failures += test_can_duck_under_clear();
   failures += test_can_duck_under_blocked_horizontal();
   failures += test_can_duck_under_no_ceiling();
   failures += test_can_duck_under_one_side_blocked();

   printf("=== BotCantMoveForward tests ===\n");
   failures += test_cant_move_forward_clear();
   failures += test_cant_move_forward_eye_blocked();
   failures += test_cant_move_forward_waist_blocked();

   printf("=== BotStuckInCorner tests ===\n");
   failures += test_stuck_in_corner_both_blocked();
   failures += test_stuck_in_corner_one_open();
   failures += test_stuck_in_corner_both_open();

   printf("=== BotEdge tests ===\n");
   failures += test_edge_forward_drop();
   failures += test_edge_forward_solid();
   failures += test_edge_right_drop();
   failures += test_edge_right_solid();
   failures += test_edge_left_drop();
   failures += test_edge_left_solid();

   printf("=== BotCheckWall tests ===\n");
   failures += test_wall_on_left_hit();
   failures += test_wall_on_left_clear();
   failures += test_wall_on_right_hit();
   failures += test_wall_on_forward_hit();
   failures += test_wall_on_forward_no_corrupt();
   failures += test_wall_on_back_no_corrupt();

   printf("=== BotLookForDrop tests ===\n");
   failures += test_look_for_drop_no_drop();
   failures += test_look_for_drop_safe_turn();
   failures += test_look_for_drop_exit_on_ground();

   printf("=== BotUpdateTrackSoundGoal tests ===\n");
   failures += test_track_sound_not_tracking();
   failures += test_track_sound_low_health();
   failures += test_track_sound_no_ammo();
   failures += test_track_sound_still_valid();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return failures ? 1 : 0;
}
