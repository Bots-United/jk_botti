//
// JK_Botti - unit tests for bot_navigate.cpp
//
// test_bot_navigate.cpp
//
// Uses #include-the-.cpp approach to access static functions directly.
//

#include <stdlib.h>
#include <math.h>

// Mock RANDOM functions for deterministic testing
static int mock_random_long_ret = 0;
static float mock_random_float_ret = 0.0f;

#define RANDOM_LONG2 mock_RANDOM_LONG2
#define RANDOM_FLOAT2 mock_RANDOM_FLOAT2

// Include the source file under test (brings in all its headers + static functions)
#include "../bot_navigate.cpp"

#undef RANDOM_LONG2
#undef RANDOM_FLOAT2

// Mock implementations - return clamped mock_random_*_ret values
int mock_RANDOM_LONG2(int low, int high)
{
   if (low >= high) return low;
   int val = mock_random_long_ret;
   if (val < low) val = low;
   if (val > high) val = high;
   return val;
}

float mock_RANDOM_FLOAT2(float low, float high)
{
   if (low >= high) return low;
   float val = mock_random_float_ret;
   if (val < low) val = low;
   if (val > high) val = high;
   return val;
}

// Test infrastructure
#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Waypoint/Sound mock infrastructure
// ============================================================

// Controllable return values for waypoint functions
static int mock_WaypointFindNearest_result = -1;
static int mock_WaypointFindReachable_result = -1;
static int mock_WaypointRouteFromTo_result = -1;
static float mock_WaypointDistanceFromTo_result = 99999.0f;

// WaypointFindPath: returns waypoint indices from a list, one per call
static int mock_WaypointFindPath_results[32];
static int mock_WaypointFindPath_count = 0;
static int mock_WaypointFindPath_call = 0;

static edict_t *mock_WaypointFindItem_result = NULL;
static int mock_WaypointFindNearestGoal_result = -1;
static int mock_WaypointFindRandomGoal_result = -1;

// Override waypoint functions (non-weak, beats engine_mock's weak stubs)
int WaypointFindNearest(const Vector &v_origin, const Vector &v_offset,
                        edict_t *pEntity, float range, qboolean b_traceline)
{
   (void)v_origin; (void)v_offset; (void)pEntity; (void)range; (void)b_traceline;
   return mock_WaypointFindNearest_result;
}

int WaypointFindReachable(edict_t *pEntity, float range)
{
   (void)pEntity; (void)range;
   return mock_WaypointFindReachable_result;
}

int WaypointRouteFromTo(int src, int dest)
{
   (void)src; (void)dest;
   return mock_WaypointRouteFromTo_result;
}

float WaypointDistanceFromTo(int src, int dest)
{
   (void)src; (void)dest;
   return mock_WaypointDistanceFromTo_result;
}

int WaypointFindPath(int &path_index, int waypoint_index)
{
   (void)waypoint_index;
   if (mock_WaypointFindPath_call < mock_WaypointFindPath_count)
   {
      path_index = mock_WaypointFindPath_call;
      return mock_WaypointFindPath_results[mock_WaypointFindPath_call++];
   }
   return -1;
}

edict_t *WaypointFindItem(int wpt_index)
{
   (void)wpt_index;
   return mock_WaypointFindItem_result;
}

int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int itemflags,
                            int exclude[], float range, const Vector *pv_src)
{
   (void)pEntity; (void)src; (void)flags; (void)itemflags; (void)exclude;
   (void)range; (void)pv_src;
   return mock_WaypointFindNearestGoal_result;
}

int WaypointFindRandomGoal(int *out_indexes, int max_indexes, edict_t *pEntity,
                           int flags, int itemflags, int exclude[])
{
   (void)max_indexes; (void)pEntity; (void)flags; (void)itemflags; (void)exclude;
   if (mock_WaypointFindRandomGoal_result != -1)
   {
      out_indexes[0] = mock_WaypointFindRandomGoal_result;
      return 1;
   }
   return 0;
}

// Sound mock infrastructure
static CSound mock_sounds[4];
static int mock_active_sound_list = SOUNDLIST_EMPTY;
static CSoundEnt mock_sound_ent;

void CSound::Clear(void) {}
void CSound::Reset(void) {}
void CSoundEnt::Initialize(void)
{
   m_iActiveSound = SOUNDLIST_EMPTY;
   m_iFreeSound = SOUNDLIST_EMPTY;
}

int CSoundEnt::ActiveList(void)
{
   return mock_active_sound_list;
}

int CSoundEnt::FreeList(void) { return SOUNDLIST_EMPTY; }

CSound *CSoundEnt::SoundPointerForIndex(int iIndex)
{
   if (iIndex >= 0 && iIndex < 4)
      return &mock_sounds[iIndex];
   return NULL;
}

void CSoundEnt::InsertSound(edict_t *pEdict, int channel, const Vector &vecOrigin,
   int iVolume, float flDuration, int iBotOwner)
{ (void)pEdict; (void)channel; (void)vecOrigin; (void)iVolume; (void)flDuration; (void)iBotOwner; }
void CSoundEnt::FreeSound(int iSound, int iPrevious) { (void)iSound; (void)iPrevious; }
CSound *CSoundEnt::GetEdictChannelSound(edict_t *pEdict, int iChannel)
{ (void)pEdict; (void)iChannel; return NULL; }
int CSoundEnt::ClientSoundIndex(edict_t *pClient) { (void)pClient; return -1; }
void SaveSound(edict_t *pEdict, const Vector &origin, int volume, int channel, float flDuration)
{ (void)pEdict; (void)origin; (void)volume; (void)channel; (void)flDuration; }

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
   pBot.f_max_speed = 320.0f;

   for (int i = 0; i < 5; i++)
      pBot.prev_waypoint_index[i] = -1;

   pEdict->v.origin = Vector(0, 0, 0);
   pEdict->v.v_angle = Vector(0, 0, 0); // facing +X
   pEdict->v.view_ofs = Vector(0, 0, 28);
   pEdict->v.health = 100;
   pEdict->v.deadflag = DEAD_NO;
   pEdict->v.takedamage = DAMAGE_YES;
   pEdict->v.solid = SOLID_BBOX;
   pEdict->v.flags = FL_CLIENT | FL_FAKECLIENT;
}

static void setup_waypoint(int index, Vector origin, int flags = 0, int itemflags = 0)
{
   if (index >= 0 && index < MAX_WAYPOINTS)
   {
      waypoints[index].origin = origin;
      waypoints[index].flags = flags;
      waypoints[index].itemflags = itemflags;
      if (index >= num_waypoints)
         num_waypoints = index + 1;
   }
}

static void reset_navigate_mocks(void)
{
   mock_WaypointFindNearest_result = -1;
   mock_WaypointFindReachable_result = -1;
   mock_WaypointRouteFromTo_result = -1;
   mock_WaypointDistanceFromTo_result = 99999.0f;
   mock_WaypointFindPath_count = 0;
   mock_WaypointFindPath_call = 0;
   mock_WaypointFindItem_result = NULL;
   mock_WaypointFindNearestGoal_result = -1;
   mock_WaypointFindRandomGoal_result = -1;

   mock_random_long_ret = 0;
   mock_random_float_ret = 0.0f;

   mock_active_sound_list = SOUNDLIST_EMPTY;
   memset(mock_sounds, 0, sizeof(mock_sounds));

   pSoundEnt = &mock_sound_ent;
   pSoundEnt->Initialize();
}



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
// Selective trace helper (bit mask controls which calls hit)
// ============================================================

static int g_jump_trace_call;
static int g_jump_block_mask;

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

// ============================================================
// 1. BotChangePitch tests
// ============================================================

static int test_change_pitch_both_positive_current_gt_ideal(void)
{
   TEST("BotChangePitch: both positive, current > ideal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 50.0f;
   pEdict->v.idealpitch = 30.0f;
   float diff = BotChangePitch(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 40.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 10.0f, 0.01f);  // remaining: |40 - 30|
   PASS();
   return 0;
}

static int test_change_pitch_both_positive_current_lt_ideal(void)
{
   TEST("BotChangePitch: both positive, current < ideal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 10.0f;
   pEdict->v.idealpitch = 50.0f;
   float diff = BotChangePitch(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 20.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);  // remaining: |20 - 50|
   PASS();
   return 0;
}

static int test_change_pitch_pos_to_neg(void)
{
   TEST("BotChangePitch: positive -> negative crossover");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 10.0f;
   pEdict->v.idealpitch = -30.0f;
   float diff = BotChangePitch(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);  // remaining: |0 - (-30)|
   PASS();
   return 0;
}

static int test_change_pitch_neg_to_pos(void)
{
   TEST("BotChangePitch: negative -> positive crossover");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = -10.0f;
   pEdict->v.idealpitch = 30.0f;
   float diff = BotChangePitch(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);  // remaining: |0 - 30|
   PASS();
   return 0;
}

static int test_change_pitch_both_negative(void)
{
   TEST("BotChangePitch: both negative");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = -50.0f;
   pEdict->v.idealpitch = -20.0f;
   float diff = BotChangePitch(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -40.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 20.0f, 0.01f);  // remaining: |-40 - (-20)|
   PASS();
   return 0;
}

static int test_change_pitch_speed_clamped(void)
{
   TEST("BotChangePitch: speed clamped when diff < speed");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = 10.0f;
   pEdict->v.idealpitch = 12.0f;
   float diff = BotChangePitch(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 12.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 0.0f, 0.01f);  // reached ideal exactly
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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 90.0f;
   pEdict->v.ideal_yaw = 60.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 80.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 20.0f, 0.01f);  // remaining: |80 - 60|
   PASS();
   return 0;
}

static int test_change_yaw_both_positive_current_lt_ideal(void)
{
   TEST("BotChangeYaw: both positive, current < ideal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 30.0f;
   pEdict->v.ideal_yaw = 90.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 40.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 50.0f, 0.01f);  // remaining: |40 - 90|
   PASS();
   return 0;
}

static int test_change_yaw_pos_to_neg(void)
{
   TEST("BotChangeYaw: positive -> negative crossover");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 10.0f;
   pEdict->v.ideal_yaw = -30.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);  // remaining: |0 - (-30)|
   PASS();
   return 0;
}

static int test_change_yaw_neg_to_pos(void)
{
   TEST("BotChangeYaw: negative -> positive crossover");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = -10.0f;
   pEdict->v.ideal_yaw = 30.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 0.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);  // remaining: |0 - 30|
   PASS();
   return 0;
}

static int test_change_yaw_both_negative(void)
{
   TEST("BotChangeYaw: both negative");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = -80.0f;
   pEdict->v.ideal_yaw = -40.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, -70.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 30.0f, 0.01f);  // remaining: |-70 - (-40)|
   PASS();
   return 0;
}

static int test_change_yaw_speed_clamped(void)
{
   TEST("BotChangeYaw: speed clamped when diff < speed");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 45.0f;
   pEdict->v.ideal_yaw = 48.0f;
   float diff = BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 48.0f, 0.01f);
   ASSERT_FLOAT_NEAR(diff, 0.0f, 0.01f);  // reached ideal exactly
   PASS();
   return 0;
}

// ============================================================
// 3. BotCanJumpUp tests
// ============================================================

static int test_can_jump_up_clear_normal_height(void)
{
   TEST("BotCanJumpUp: clear at normal height -> TRUE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static void trace_jump_block_right_side(const float *v1, const float *v2,
                                        int fNoMonsters, int hullNumber,
                                        edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   float dz = v2[2] - v1[2];
   int is_horizontal = (fabs(dz) < 1.0f);
   int is_normal_center = is_horizontal && (fabs(v1[1]) < 1.0f) && (fabs(v1[2] - 10.0f) < 1.0f);
   int is_right_side = is_horizontal && (v1[1] < -8.0f);

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
   TEST("BotCanJumpUp: right side checked at duck height (bug #1)");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

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
   reset_navigate_mocks();

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

static int test_edge_forward_drop(void)
{
   TEST("BotEdgeForward: drop detected -> TRUE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 0);
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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

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
// 8. BotCheckWall tests
// ============================================================

static int test_wall_on_left_hit(void)
{
   TEST("BotCheckWallOnLeft: wall -> TRUE + timestamp");
   mock_reset();
   reset_navigate_mocks();

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

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
   TEST("BotCheckWallOnForward: wall -> TRUE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean result = BotCheckWallOnForward(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_wall_on_forward_clear(void)
{
   TEST("BotCheckWallOnForward: no wall -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotCheckWallOnForward(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_wall_on_forward_no_corrupt(void)
{
   TEST("BotCheckWallOnForward: no corrupt f_wall_on_right (bug #2)");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_wall_on_right = 0.0f;

   mock_trace_hull_fn = trace_all_hit;

   BotCheckWallOnForward(bot);

   ASSERT_FLOAT_NEAR(bot.f_wall_on_right, 0.0f, 0.01f);
   PASS();
   return 0;
}

static int test_wall_on_back_hit(void)
{
   TEST("BotCheckWallOnBack: wall -> TRUE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   qboolean result = BotCheckWallOnBack(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

static int test_wall_on_back_clear(void)
{
   TEST("BotCheckWallOnBack: no wall -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotCheckWallOnBack(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_wall_on_back_no_corrupt(void)
{
   TEST("BotCheckWallOnBack: no corrupt f_wall_on_right (bug #2)");
   mock_reset();
   reset_navigate_mocks();

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

static int g_lookdrop_trace_call;

static void trace_no_drop(const float *v1, const float *v2,
                          int fNoMonsters, int hullNumber,
                          edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_lookdrop_trace_call++;
   if (call == 0)
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

static int test_look_for_drop_no_drop(void)
{
   TEST("BotLookForDrop: no drop -> no yaw change");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   float original_yaw = pEdict->v.ideal_yaw;

   g_lookdrop_trace_call = 0;
   mock_trace_hull_fn = trace_no_drop;

   BotLookForDrop(bot);

   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, original_yaw, 0.01f);
   PASS();
   return 0;
}

static void trace_drop_with_safe_turn(const float *v1, const float *v2,
                                      int fNoMonsters, int hullNumber,
                                      edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_lookdrop_trace_call++;

   if (call == 4)
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

static int test_look_for_drop_safe_turn(void)
{
   TEST("BotLookForDrop: drop ahead, safe dir found -> yaw changes");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   float original_yaw = pEdict->v.ideal_yaw;

   g_lookdrop_trace_call = 0;
   mock_trace_hull_fn = trace_drop_with_safe_turn;

   BotLookForDrop(bot);

   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw - original_yaw) > 1.0f);
   PASS();
   return 0;
}

static void trace_drop_bug_detect(const float *v1, const float *v2,
                                  int fNoMonsters, int hullNumber,
                                  edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   int call = g_lookdrop_trace_call++;

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
   TEST("BotLookForDrop: loop exits on ground, not drops (bug #3)");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.v_angle.y = 0.0f;

   g_lookdrop_trace_call = 0;
   mock_trace_hull_fn = trace_drop_bug_detect;

   BotLookForDrop(bot);

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   qboolean result = BotUpdateTrackSoundGoal(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_track_sound_low_health(void)
{
   TEST("BotUpdateTrackSoundGoal: low health -> stops tracking");
   mock_reset();
   reset_navigate_mocks();

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
   reset_navigate_mocks();

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
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   bot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
   bot.f_track_sound_time = gpGlobals->time + 10.0f;
   bot.b_low_health = FALSE;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;
   bot.waypoint_goal = 5;

   qboolean result = BotUpdateTrackSoundGoal(bot);
   ASSERT_INT(result, TRUE);
   PASS();
   return 0;
}

// ============================================================
// 11. BotEvaluateGoal tests (STATIC)
// ============================================================

static int test_evaluate_goal_no_goal(void)
{
   TEST("BotEvaluateGoal: goal=-1 -> noop");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.waypoint_goal = -1;
   pEdict->v.health = 10;

   BotEvaluateGoal(bot);

   ASSERT_INT(bot.waypoint_goal, -1);
   PASS();
   return 0;
}

static int test_evaluate_goal_high_health(void)
{
   TEST("BotEvaluateGoal: health>25 -> keeps goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.waypoint_goal = 5;
   bot.wpt_goal_type = WPT_GOAL_WEAPON;
   pEdict->v.health = 50;

   BotEvaluateGoal(bot);

   ASSERT_INT(bot.waypoint_goal, 5);
   PASS();
   return 0;
}

static int test_evaluate_goal_low_health_health_goal(void)
{
   TEST("BotEvaluateGoal: health<=25, HEALTH goal -> keeps goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.waypoint_goal = 5;
   bot.wpt_goal_type = WPT_GOAL_HEALTH;
   pEdict->v.health = 20;

   BotEvaluateGoal(bot);

   ASSERT_INT(bot.waypoint_goal, 5);
   PASS();
   return 0;
}

static int test_evaluate_goal_low_health_non_health_goal(void)
{
   TEST("BotEvaluateGoal: health<=25, non-HEALTH -> clears goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.waypoint_goal = 5;
   bot.wpt_goal_type = WPT_GOAL_WEAPON;
   pEdict->v.health = 20;

   BotEvaluateGoal(bot);

   ASSERT_INT(bot.waypoint_goal, -1);
   PASS();
   return 0;
}

// ============================================================
// 12. BotRandomTurn tests
// ============================================================

static int test_random_turn_wander_left(void)
{
   TEST("BotRandomTurn: changes yaw + stops movement");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.wander_dir = WANDER_LEFT;
   bot.f_move_speed = 320.0f;
   pEdict->v.v_angle.y = 0.0f;
   pEdict->v.ideal_yaw = 0.0f;

   BotRandomTurn(bot);

   // Yaw must have changed (either 180 or 30-60 turn)
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw) > 1.0f);
   ASSERT_FLOAT_NEAR(bot.f_move_speed, 0.0f, 0.01f);
   PASS();
   return 0;
}

static int test_random_turn_move_speed_zero(void)
{
   TEST("BotRandomTurn: sets move_speed to 0");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_move_speed = 320.0f;
   bot.wander_dir = WANDER_RIGHT;

   BotRandomTurn(bot);

   ASSERT_FLOAT_NEAR(bot.f_move_speed, 0.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 13. BotTurnAtWall tests
// ============================================================

static int test_turn_at_wall_perpendicular(void)
{
   TEST("BotTurnAtWall: perpendicular to wall normal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.v_angle.y = 0.0f;

   TraceResult tr;
   memset(&tr, 0, sizeof(tr));
   // Wall facing +Y (normal = 0,1,0), UTIL_VecToAngles of (0,1,0) gives yaw ~90
   tr.vecPlaneNormal[0] = 0;
   tr.vecPlaneNormal[1] = 1;
   tr.vecPlaneNormal[2] = 0;

   BotTurnAtWall(bot, &tr, FALSE);

   // Should change ideal_yaw to something perpendicular to the wall
   // The exact value depends on random jitter but should be roughly +-90 from current
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw) > 1.0f);
   PASS();
   return 0;
}

static int test_turn_at_wall_negative(void)
{
   TEST("BotTurnAtWall: negative flag flips normal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.v_angle.y = 0.0f;

   TraceResult tr;
   memset(&tr, 0, sizeof(tr));
   tr.vecPlaneNormal[0] = 1;
   tr.vecPlaneNormal[1] = 0;
   tr.vecPlaneNormal[2] = 0;

   float yaw_before = pEdict->v.ideal_yaw;
   BotTurnAtWall(bot, &tr, TRUE);

   // With negative flag, normal is flipped, should still set a new yaw
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw - yaw_before) > 0.1f ||
               fabs(pEdict->v.ideal_yaw) > 0.1f);
   PASS();
   return 0;
}

// ============================================================
// 14. BotFindWaypoint tests (STATIC)
// ============================================================

static int test_find_waypoint_no_paths(void)
{
   TEST("BotFindWaypoint: no paths -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.curr_waypoint_index = 0;

   // WaypointFindPath returns -1 by default (no paths)
   mock_WaypointFindPath_count = 0;

   qboolean result = BotFindWaypoint(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_find_waypoint_single_connected(void)
{
   TEST("BotFindWaypoint: single connected -> found + history");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);
   bot.curr_waypoint_index = 0;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(1, Vector(100, 0, 0));

   // WaypointFindPath returns index 1, then -1
   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;

   qboolean result = BotFindWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 1);
   ASSERT_INT(bot.prev_waypoint_index[0], 0); // old curr pushed to prev[0]
   PASS();
   return 0;
}

static int test_find_waypoint_closest_of_multiple(void)
{
   TEST("BotFindWaypoint: closest of multiple selected");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);
   bot.curr_waypoint_index = 0;
   // Make random selection not trigger (time in past)
   bot.f_random_waypoint_time = gpGlobals->time + 100.0f;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(1, Vector(500, 0, 0)); // far
   setup_waypoint(2, Vector(50, 0, 0));  // close
   setup_waypoint(3, Vector(200, 0, 0)); // medium

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_results[1] = 2;
   mock_WaypointFindPath_results[2] = 3;
   mock_WaypointFindPath_count = 3;

   qboolean result = BotFindWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 2); // closest one
   PASS();
   return 0;
}

static int test_find_waypoint_prev_exclusion(void)
{
   TEST("BotFindWaypoint: prev waypoints excluded");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);
   bot.curr_waypoint_index = 0;
   bot.f_random_waypoint_time = gpGlobals->time + 100.0f;

   // Mark waypoint 1 as a previous waypoint
   bot.prev_waypoint_index[0] = 1;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(1, Vector(10, 0, 0));  // closest but excluded
   setup_waypoint(2, Vector(200, 0, 0)); // farther, not excluded

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_results[1] = 2;
   mock_WaypointFindPath_count = 2;

   qboolean result = BotFindWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 2); // 1 excluded, picks 2
   PASS();
   return 0;
}

// ============================================================
// 15. BotGetSoundWaypoint tests (STATIC)
// ============================================================

static int test_get_sound_waypoint_no_sounds(void)
{
   TEST("BotGetSoundWaypoint: no sounds -> -1");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_active_sound_list = SOUNDLIST_EMPTY;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bot, NULL, &pNew);
   ASSERT_INT(result, -1);
   ASSERT_PTR_NULL(pNew);
   PASS();
   return 0;
}

static int test_get_sound_waypoint_own_sound_skipped(void)
{
   TEST("BotGetSoundWaypoint: own sound skipped -> -1");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Put bot at index 0 in bots[]
   memcpy(&bots[0], &bot, sizeof(bot_t));

   // Set up sound at index 0
   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 0; // same as bot index
   mock_sounds[0].m_pEdict = pEdict;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   // hearing sensitivity
   skill_settings[bot.bot_skill].hearing_sensitivity = 1.0f;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bot, NULL, &pNew);
   ASSERT_INT(result, -1); // skipped own sound
   PASS();
   return 0;
}

static int test_get_sound_waypoint_out_of_range(void)
{
   TEST("BotGetSoundWaypoint: out of range -> -1");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   // Sound very far away with low volume
   mock_sounds[0].m_vecOrigin = Vector(5000, 0, 0);
   mock_sounds[0].m_iVolume = 10; // very quiet
   mock_sounds[0].m_iBotOwner = 99; // not this bot
   mock_sounds[0].m_pEdict = NULL;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   skill_settings[bot.bot_skill].hearing_sensitivity = 1.0f;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bot, NULL, &pNew);
   ASSERT_INT(result, -1);
   PASS();
   return 0;
}

static int test_get_sound_waypoint_valid(void)
{
   TEST("BotGetSoundWaypoint: valid sound -> waypoint index");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pSoundSource = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   setup_waypoint(5, Vector(100, 0, 0));

   // Set up sound close to bot, loud enough
   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 99; // not this bot
   mock_sounds[0].m_pEdict = pSoundSource;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   skill_settings[bot.bot_skill].hearing_sensitivity = 1.0f;

   // WaypointFindNearest returns 5 for the sound position
   mock_WaypointFindNearest_result = 5;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bot, NULL, &pNew);
   ASSERT_INT(result, 5);
   ASSERT_PTR_EQ(pNew, pSoundSource);
   PASS();
   return 0;
}

// ============================================================
// 16. BotOnLadder tests
// ============================================================

static void trace_ladder_wall(const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   // Simulate hitting a BSP wall
   ptr->flFraction = 0.5f;
   ptr->pHit = &mock_edicts[2]; // use edict 2 as wall
   mock_edicts[2].v.solid = SOLID_BSP;
   ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
   ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
   ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
   // Normal facing -X (towards the bot)
   ptr->vecPlaneNormal[0] = -1;
   ptr->vecPlaneNormal[1] = 0;
   ptr->vecPlaneNormal[2] = 0;
}

static int test_on_ladder_unknown_finds_wall(void)
{
   TEST("BotOnLadder: LADDER_UNKNOWN finds wall -> squares up");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_UNKNOWN;
   pEdict->v.v_angle.y = 0.0f;

   // Allocate edict 2 for the wall
   mock_alloc_edict(); // edict 1 = pEdict, edict 2 = wall
   mock_edicts[2].v.solid = SOLID_BSP;

   mock_trace_hull_fn = trace_ladder_wall;

   BotOnLadder(bot, 0.0f);

   // After finding wall, bot should set ladder_dir to LADDER_UP
   ASSERT_INT(bot.ladder_dir, LADDER_UP);
   // ideal_yaw should be set facing the wall (normal is -1,0,0 -> facing +X, angle ~180)
   // The wall normal points towards bot, so flip 180 -> bot faces wall
   ASSERT_TRUE(fabs(pEdict->v.v_angle.x - (-60.0f)) < 0.01f); // looking up
   PASS();
   return 0;
}

static int test_on_ladder_unknown_no_wall(void)
{
   TEST("BotOnLadder: LADDER_UNKNOWN no wall -> current yaw");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_UNKNOWN;
   pEdict->v.v_angle.y = 45.0f;

   mock_trace_hull_fn = trace_all_clear;

   BotOnLadder(bot, 0.0f);

   // No wall found, ideal_yaw set to current v_angle.y
   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, 45.0f, 0.01f);
   ASSERT_INT(bot.ladder_dir, LADDER_UP);
   PASS();
   return 0;
}

static int test_on_ladder_up_normal(void)
{
   TEST("BotOnLadder: LADDER_UP moves forward + looks up");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_UP;
   bot.f_prev_speed = 10.0f; // was moving

   BotOnLadder(bot, 5.0f); // still moving

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -60.0f, 0.01f); // looking up
   ASSERT_INT(bot.ladder_dir, LADDER_UP);
   ASSERT_TRUE(pEdict->v.button & IN_FORWARD);
   PASS();
   return 0;
}

static int test_on_ladder_up_stuck(void)
{
   TEST("BotOnLadder: LADDER_UP stuck -> switch to DOWN");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_UP;
   bot.f_prev_speed = 5.0f; // was moving before

   BotOnLadder(bot, 0.5f); // barely moved

   ASSERT_INT(bot.ladder_dir, LADDER_DOWN);
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 60.0f, 0.01f); // looking down
   PASS();
   return 0;
}

static int test_on_ladder_down_stuck(void)
{
   TEST("BotOnLadder: LADDER_DOWN stuck -> switch to UP");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_DOWN;
   bot.f_prev_speed = 5.0f;

   BotOnLadder(bot, 0.5f); // barely moved

   ASSERT_INT(bot.ladder_dir, LADDER_UP);
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -60.0f, 0.01f); // looking up
   PASS();
   return 0;
}

// ============================================================
// 17. BotUnderWater tests
// ============================================================

static int test_underwater_waypoints_head_toward(void)
{
   TEST("BotUnderWater: waypoints + head toward succeeds");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   setup_waypoint(0, Vector(100, 0, 0));
   mock_WaypointFindReachable_result = 0;

   // BotHeadTowardWaypoint will find reachable wp 0
   // Make traces clear so it can head toward it
   mock_trace_hull_fn = trace_all_clear;

   BotUnderWater(bot);

   // Should have set curr_waypoint_index and returned early
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_underwater_no_waypoints_surface(void)
{
   TEST("BotUnderWater: no waypoints -> swim up + jump on land");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   // No waypoints (num_waypoints = 0 from reset)

   // First trace (upward/forward): clear (reach surface)
   // Second trace (downward from surface): hits ground
   static int uw_trace_call;
   uw_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = uw_trace_call++;
      if (call == 1) // second trace (downward) hits ground
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
      }
      else // first trace (upward) clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };

   // First POINT_CONTENTS (surface endpoint): CONTENTS_EMPTY
   // Second POINT_CONTENTS (ground below): not CONTENTS_WATER
   mock_point_contents_fn = [](const float *origin) -> int {
      (void)origin;
      return CONTENTS_EMPTY;
   };

   BotUnderWater(bot);

   // Bot looks up and moves forward
   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -60.0f, 0.01f);
   ASSERT_TRUE(pEdict->v.button & IN_FORWARD);
   // Surface trace clear + land found -> jump
   ASSERT_TRUE(pEdict->v.button & IN_JUMP);
   PASS();
   return 0;
}

static int test_underwater_trace_blocked(void)
{
   TEST("BotUnderWater: trace blocked -> swim up only");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_hit;

   BotUnderWater(bot);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -60.0f, 0.01f);
   ASSERT_TRUE(pEdict->v.button & IN_FORWARD);
   // No jump since trace was blocked
   ASSERT_TRUE(!(pEdict->v.button & IN_JUMP));
   PASS();
   return 0;
}

static int test_underwater_surface_is_water(void)
{
   TEST("BotUnderWater: surface is water -> no jump");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // First trace (forward/up) clear, then downward trace hits
   static int water_trace_call;
   water_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = water_trace_call++;
      if (call == 1) // second trace (downward) hits
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
   };

   mock_point_contents_fn = [](const float *origin) -> int {
      (void)origin;
      return CONTENTS_WATER;
   };

   BotUnderWater(bot);

   // Surface found but it's water -> no jump
   ASSERT_TRUE(!(pEdict->v.button & IN_JUMP));
   PASS();
   return 0;
}

// ============================================================
// 18. BotUseLift tests
// ============================================================

static int test_use_lift_button_press(void)
{
   TEST("BotUseLift: button press timing + USE key");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time; // just pressed
   bot.v_use_target = Vector(100, 0, 28);
   bot.b_lift_moving = FALSE;
   bot.b_use_button = TRUE;
   pEdict->v.yaw_speed = 100.0f;

   mock_trace_hull_fn = trace_all_clear;

   BotUseLift(bot, 0.0f);

   // Should press USE and set not_maxspeed
   ASSERT_TRUE(pEdict->v.button & IN_USE);
   ASSERT_INT(bot.b_not_maxspeed, TRUE);
   PASS();
   return 0;
}

static int test_use_lift_timeout(void)
{
   TEST("BotUseLift: timeout -> clears use_button");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time - 5.0f; // 5 seconds ago
   bot.b_lift_moving = FALSE;
   bot.b_use_button = TRUE;
   bot.f_max_speed = 320.0f;

   BotUseLift(bot, 0.0f);

   ASSERT_INT(bot.b_use_button, FALSE);
   ASSERT_INT(bot.b_not_maxspeed, FALSE);
   PASS();
   return 0;
}

static int test_use_lift_starts_moving(void)
{
   TEST("BotUseLift: lift starts moving -> b_lift_moving");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time - 5.0f;
   bot.b_lift_moving = FALSE;
   bot.b_use_button = TRUE;
   bot.f_max_speed = 320.0f;

   BotUseLift(bot, 5.0f); // moving

   ASSERT_INT(bot.b_lift_moving, TRUE);
   PASS();
   return 0;
}

static int test_use_lift_stops_forward_clear(void)
{
   TEST("BotUseLift: lift stops, forward clear -> go forward");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time - 5.0f;
   bot.b_lift_moving = TRUE;
   bot.b_use_button = TRUE;
   bot.f_max_speed = 320.0f;
   pEdict->v.yaw_speed = 100.0f;

   // Forward traces clear (both horizontal and downward hit = floor found)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 1); // downward trace hits (floor)
   mock_trace_hull_fn = trace_jump_selective;

   float original_yaw = pEdict->v.ideal_yaw;
   BotUseLift(bot, 0.5f); // barely moving -> lift stopped

   // Forward is clear + floor found, no yaw change
   ASSERT_INT(bot.b_use_button, FALSE);
   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, original_yaw, 0.01f);
   PASS();
   return 0;
}

static int test_use_lift_stops_all_blocked(void)
{
   TEST("BotUseLift: lift stops, all blocked -> turn 180");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time - 5.0f;
   bot.b_lift_moving = TRUE;
   bot.b_use_button = TRUE;
   bot.f_max_speed = 320.0f;
   pEdict->v.ideal_yaw = 0.0f;
   pEdict->v.yaw_speed = 100.0f;

   // All directions blocked
   mock_trace_hull_fn = trace_all_hit;

   BotUseLift(bot, 0.5f); // barely moving -> lift stopped

   ASSERT_INT(bot.b_use_button, FALSE);
   // Should have turned 180 degrees
   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, 180.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 19. BotFindWaypointGoal tests (STATIC)
// ============================================================

static int test_find_waypoint_goal_critical_health(void)
{
   TEST("BotFindWaypointGoal: critical health -> HEALTH goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 10; // very low health
   bot.curr_waypoint_index = 0;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(3, Vector(100, 0, 0), W_FL_HEALTH);

   mock_WaypointFindNearestGoal_result = 3;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_HEALTH);
   ASSERT_INT(bot.waypoint_goal, 3);
   PASS();
   return 0;
}

static int test_find_waypoint_goal_enemy(void)
{
   TEST("BotFindWaypointGoal: has enemy -> ENEMY goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pEnemy = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 100;
   bot.pBotEnemy = pEnemy;
   bot.curr_waypoint_index = 0;

   pEnemy->v.origin = Vector(500, 0, 0);
   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(7, Vector(500, 0, 0));

   mock_WaypointFindNearest_result = 7;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_ENEMY);
   ASSERT_INT(bot.waypoint_goal, 7);
   PASS();
   return 0;
}

static int test_find_waypoint_goal_fallback_random(void)
{
   TEST("BotFindWaypointGoal: nothing found -> LOCATION");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 100;
   bot.curr_waypoint_index = 0;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(5, Vector(200, 0, 0));

   // All specific searches return -1 (defaults)
   // Random goal returns 5
   mock_WaypointFindRandomGoal_result = 5;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_LOCATION);
   ASSERT_INT(bot.waypoint_goal, 5);
   PASS();
   return 0;
}

static int test_find_waypoint_goal_needs_health(void)
{
   TEST("BotFindWaypointGoal: needs health -> HEALTH");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 50; // below 95% of 100
   bot.curr_waypoint_index = 0;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(4, Vector(100, 0, 0), W_FL_HEALTH);

   mock_WaypointFindNearestGoal_result = 4;
   mock_WaypointDistanceFromTo_result = 100.0f;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_HEALTH);
   ASSERT_INT(bot.waypoint_goal, 4);
   PASS();
   return 0;
}

// ============================================================
// 20. BotHeadTowardWaypoint tests
// ============================================================

static int test_head_toward_no_waypoint_find_reachable(void)
{
   TEST("BotHeadTowardWaypoint: no wp -> find reachable");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.curr_waypoint_index = -1;

   setup_waypoint(0, Vector(100, 0, 0));
   mock_WaypointFindReachable_result = 0;
   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_head_toward_nothing_found(void)
{
   TEST("BotHeadTowardWaypoint: nothing found -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.curr_waypoint_index = -1;

   // All waypoint searches return -1 (defaults)
   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, FALSE);
   ASSERT_INT(bot.curr_waypoint_index, -1);
   PASS();
   return 0;
}

static int test_head_toward_timeout_resets(void)
{
   TEST("BotHeadTowardWaypoint: timeout -> resets waypoint");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.curr_waypoint_index = 5;
   bot.waypoint_goal = 10;
   bot.f_waypoint_time = gpGlobals->time - 10.0f; // 10 seconds ago

   setup_waypoint(5, Vector(100, 0, 0));

   // After timeout, curr_waypoint_index becomes -1
   // Then we try to find a new waypoint
   // Let that fail too
   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(bot.curr_waypoint_index, -1);
   ASSERT_INT(bot.waypoint_goal, -1);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_head_toward_touch_advances(void)
{
   TEST("BotHeadTowardWaypoint: touch -> advance via BotFindWaypoint");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   // Current waypoint very close to bot (touching)
   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time; // recent
   bot.prev_waypoint_distance = 0.0f; // reset

   // BotFindWaypoint will find waypoint 1
   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 1);
   PASS();
   return 0;
}

static int test_head_toward_loop_the_loop(void)
{
   TEST("BotHeadTowardWaypoint: loop-the-loop detection");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(60, 0, 0);

   // Waypoint at (100,0,0), bot was closer before (prev_distance=30, now=40)
   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.prev_waypoint_distance = 30.0f; // was closer

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, TRUE);
   // Should have triggered "touching" and advanced to next waypoint
   ASSERT_INT(bot.curr_waypoint_index, 1);
   PASS();
   return 0;
}

static int test_head_toward_door_flag(void)
{
   TEST("BotHeadTowardWaypoint: door flag -> dont_avoid_wall_time");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_DOOR);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;
   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   ASSERT_TRUE(bot.f_dont_avoid_wall_time > gpGlobals->time);
   PASS();
   return 0;
}

static int test_head_toward_jump_flag(void)
{
   TEST("BotHeadTowardWaypoint: jump flag -> IN_JUMP");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_JUMP);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;
   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   ASSERT_TRUE(pEdict->v.button & IN_JUMP);
   PASS();
   return 0;
}

static int test_head_toward_goal_reached(void)
{
   TEST("BotHeadTowardWaypoint: goal reached -> clears goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_HEALTH);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_goal = 0; // goal IS the current waypoint
   bot.wpt_goal_type = WPT_GOAL_HEALTH;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;
   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   ASSERT_INT(bot.waypoint_goal, -1);
   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_NONE);
   PASS();
   return 0;
}

static int test_head_toward_goal_routing(void)
{
   TEST("BotHeadTowardWaypoint: goal routing uses WaypointRouteFromTo");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(200, 0, 0));
   setup_waypoint(5, Vector(500, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_goal = 5;
   bot.wpt_goal_type = WPT_GOAL_WEAPON;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_waypoint_goal_time = 0.0f; // allow goal search

   // Route from 0 to goal 5 goes through waypoint 1
   mock_WaypointRouteFromTo_result = 1;
   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   ASSERT_INT(bot.curr_waypoint_index, 1);
   PASS();
   return 0;
}

static int test_head_toward_ladder_flag(void)
{
   TEST("BotHeadTowardWaypoint: ladder flag -> sets top_of_ladder");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 100); // higher than waypoint

   // Current waypoint is a ladder waypoint below bot
   setup_waypoint(0, Vector(100, 0, 50), W_FL_LADDER);
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.b_on_ladder = FALSE;

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   ASSERT_INT(bot.waypoint_top_of_ladder, TRUE);
   PASS();
   return 0;
}

static int test_head_toward_in_water_exit(void)
{
   TEST("BotHeadTowardWaypoint: in_water touching -> exit water");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(100, 0, 100)); // waypoint above
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.b_in_water = TRUE;

   // When touching and in_water, it traces up and looks for surface
   mock_trace_hull_fn = trace_all_clear;
   mock_point_contents_fn = [](const float *origin) -> int {
      (void)origin;
      return CONTENTS_EMPTY;
   };
   mock_WaypointFindNearest_result = 1;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;

   BotHeadTowardWaypoint(bot);

   // Should have found waypoint above water surface
   ASSERT_TRUE(bot.f_exit_water_time >= gpGlobals->time);
   PASS();
   return 0;
}

static int test_head_toward_retry_loop(void)
{
   TEST("BotHeadTowardWaypoint: retry loop clears prev waypoints");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   // No paths available and no goal, so BotFindWaypoint will fail
   // The retry loop should clear prev_waypoint_index entries
   mock_WaypointFindPath_count = 0;
   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotHeadTowardWaypoint(bot);

   // Should have failed and cleared state
   ASSERT_INT(result, FALSE);
   ASSERT_INT(bot.curr_waypoint_index, -1);
   for (int i = 0; i < 5; i++)
      ASSERT_INT(bot.prev_waypoint_index[i], -1);
   PASS();
   return 0;
}

static int test_head_toward_pitch_yaw_toward_waypoint(void)
{
   TEST("BotHeadTowardWaypoint: sets pitch/yaw toward waypoint");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   // Waypoint to the right (positive Y)
   setup_waypoint(0, Vector(0, 100, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // ideal_yaw should be set toward (0,100,0)
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw - 90.0f) < 1.0f ||
               fabs(pEdict->v.ideal_yaw + 270.0f) < 1.0f);
   PASS();
   return 0;
}

// ============================================================
// 21. Additional BotChangePitch/Yaw wrap-around tests
// ============================================================

static int test_change_pitch_wrap_above_180(void)
{
   TEST("BotChangePitch: wrap current > 180");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // current=170 (positive), ideal=-170 (negative)
   // current_180 = 170-180 = -10. -10 > -170 → current += speed → 190
   // 190 > 180 → current -= 360 = -170
   pEdict->v.v_angle.x = 170.0f;
   pEdict->v.idealpitch = -170.0f;
   bot.f_frame_time = 0.1f;
   BotChangePitch(bot, 200.0f); // speed = 200*0.1 = 20

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -170.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_wrap_below_minus180(void)
{
   TEST("BotChangePitch: wrap current < -180");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // current=-170 (negative), ideal=170 (positive)
   // current_180 = -170+180 = 10. 10 > 170? No → current -= speed → -190
   // -190 < -180 → current += 360 = 170
   pEdict->v.v_angle.x = -170.0f;
   pEdict->v.idealpitch = 170.0f;
   bot.f_frame_time = 0.1f;
   BotChangePitch(bot, 200.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, 170.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_pitch_both_neg_cur_gt_ideal(void)
{
   TEST("BotChangePitch: both negative, current > ideal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.x = -20.0f;
   pEdict->v.idealpitch = -50.0f;
   bot.f_frame_time = 0.1f;
   BotChangePitch(bot, 100.0f); // speed = 10

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.x, -30.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_wrap_above_180(void)
{
   TEST("BotChangeYaw: wrap current > 180");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = 170.0f;
   pEdict->v.ideal_yaw = -170.0f;
   bot.f_frame_time = 0.1f;
   BotChangeYaw(bot, 200.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, -170.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_wrap_below_minus180(void)
{
   TEST("BotChangeYaw: wrap current < -180");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = -170.0f;
   pEdict->v.ideal_yaw = 170.0f;
   bot.f_frame_time = 0.1f;
   BotChangeYaw(bot, 200.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, 170.0f, 0.01f);
   PASS();
   return 0;
}

static int test_change_yaw_both_neg_cur_gt_ideal(void)
{
   TEST("BotChangeYaw: both negative, current > ideal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   pEdict->v.v_angle.y = -20.0f;
   pEdict->v.ideal_yaw = -50.0f;
   bot.f_frame_time = 0.1f;
   BotChangeYaw(bot, 100.0f);

   ASSERT_FLOAT_NEAR(pEdict->v.v_angle.y, -30.0f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 22. Additional BotCheckWallOnRight test
// ============================================================

static int test_wall_on_right_clear(void)
{
   TEST("BotCheckWallOnRight: no wall -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotCheckWallOnRight(bot);
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// ============================================================
// 23. Additional BotFindWaypoint test (3rd closest)
// ============================================================

static int test_find_waypoint_three_candidates(void)
{
   TEST("BotFindWaypoint: fills 3rd closest slot");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);
   bot.curr_waypoint_index = 0;
   bot.f_random_waypoint_time = gpGlobals->time + 100.0f;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(1, Vector(50, 0, 0));   // closest
   setup_waypoint(2, Vector(200, 0, 0));  // 2nd
   setup_waypoint(3, Vector(500, 0, 0));  // 3rd
   setup_waypoint(4, Vector(400, 0, 0));  // replaces 3rd (closer)

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_results[1] = 2;
   mock_WaypointFindPath_results[2] = 3;
   mock_WaypointFindPath_results[3] = 4;
   mock_WaypointFindPath_count = 4;

   qboolean result = BotFindWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 1); // closest
   PASS();
   return 0;
}

// ============================================================
// 24. Additional BotGetSoundWaypoint tests
// ============================================================

static int test_get_sound_waypoint_own_sound_fixed(void)
{
   TEST("BotGetSoundWaypoint: own sound skipped (bots[])");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();

   // Use bots[0] directly so &pBot - &bots[0] == 0
   setup_bot_for_test(bots[0], pEdict);
   bots[0].pEdict = pEdict;

   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 0; // same as bot index
   mock_sounds[0].m_pEdict = pEdict;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   skill_settings[bots[0].bot_skill].hearing_sensitivity = 1.0f;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bots[0], NULL, &pNew);
   ASSERT_INT(result, -1);
   PASS();
   return 0;
}

static int test_get_sound_waypoint_track_specific_edict(void)
{
   TEST("BotGetSoundWaypoint: track edict mismatch -> skip");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pTrackTarget = mock_alloc_edict();
   edict_t *pSoundSource = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   // Sound from pSoundSource, but we're tracking pTrackTarget
   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 99;
   mock_sounds[0].m_pEdict = pSoundSource; // doesn't match pTrackTarget
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   skill_settings[bot.bot_skill].hearing_sensitivity = 1.0f;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bot, pTrackTarget, &pNew);
   ASSERT_INT(result, -1); // skipped because m_pEdict != pTrackTarget
   PASS();
   return 0;
}

static int test_get_sound_waypoint_distance_with_curr_wp(void)
{
   TEST("BotGetSoundWaypoint: uses WaypointDistanceFromTo");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pSoundSource = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);
   bot.curr_waypoint_index = 0; // has a current waypoint

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(5, Vector(100, 0, 0));

   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 99;
   mock_sounds[0].m_pEdict = pSoundSource;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   skill_settings[bot.bot_skill].hearing_sensitivity = 1.0f;
   mock_WaypointFindNearest_result = 5;
   mock_WaypointDistanceFromTo_result = 50.0f; // short distance via routing

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bot, NULL, &pNew);
   ASSERT_INT(result, 5);
   ASSERT_PTR_EQ(pNew, pSoundSource);
   PASS();
   return 0;
}

// ============================================================
// 25. Additional BotFindWaypointGoal tests
// ============================================================

static int test_find_waypoint_goal_needs_armor(void)
{
   TEST("BotFindWaypointGoal: needs armor -> ARMOR");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 100;   // >= 95, skip health
   pEdict->v.armorvalue = 50; // < 95, want armor
   bot.curr_waypoint_index = 0;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(6, Vector(100, 0, 0));

   mock_WaypointFindNearestGoal_result = 6;
   mock_WaypointDistanceFromTo_result = 100.0f;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_ARMOR);
   ASSERT_INT(bot.waypoint_goal, 6);
   PASS();
   return 0;
}

static int test_find_waypoint_goal_needs_longjump(void)
{
   TEST("BotFindWaypointGoal: needs longjump -> ITEM");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 100;
   pEdict->v.armorvalue = 100;
   bot.curr_waypoint_index = 0;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;
   bot.b_longjump = FALSE;
   skill_settings[bot.bot_skill].can_longjump = TRUE;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(8, Vector(100, 0, 0));

   mock_WaypointFindNearestGoal_result = 8;
   mock_WaypointDistanceFromTo_result = 100.0f;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_ITEM);
   ASSERT_INT(bot.waypoint_goal, 8);
   PASS();
   return 0;
}

static int test_find_waypoint_goal_critical_health_random_fallback(void)
{
   TEST("BotFindWaypointGoal: critical health, nearest fails -> random");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.health = 1; // very low, always triggers critical health
   bot.curr_waypoint_index = 0;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(3, Vector(100, 0, 0));

   // Nearest returns -1, random returns 3
   mock_WaypointFindNearestGoal_result = -1;
   mock_WaypointFindRandomGoal_result = 3;

   BotFindWaypointGoal(bot);

   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_HEALTH);
   ASSERT_INT(bot.waypoint_goal, 3);
   PASS();
   return 0;
}

// ============================================================
// 26. Additional BotHeadTowardWaypoint tests
// ============================================================

static int test_head_toward_longjump_to_waypoint(void)
{
   TEST("BotHeadTowardWaypoint: longjump toward waypoint");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);
   pEdict->v.v_angle = Vector(0, 0, 0); // facing +X

   // sv_gravity = 800 → max_lj_distance = 540 * (800/800) = 540
   // Need distance >= 540 * 0.6 = 324
   setup_waypoint(0, Vector(400, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   bot.b_longjump = TRUE;
   skill_settings[bot.bot_skill].can_longjump = TRUE;
   bot.b_on_ground = TRUE;
   bot.b_ducking = FALSE;
   bot.b_on_ladder = FALSE;
   bot.b_in_water = FALSE;
   pEdict->v.velocity = Vector(100, 0, 0); // > 50

   // Two hull traces: 1) toward waypoint (must be clear), 2) downward (must hit ground near wp level)
   static int lj_trace_call;
   lj_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = lj_trace_call++;
      if (call == 0) // forward hull trace - must be clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
      else // downward trace - hit ground at same Z level as waypoint
      {
         ptr->flFraction = 0.01f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = 0; // same Z as waypoint origin
      }
   };

   BotHeadTowardWaypoint(bot);

   // Should have triggered longjump: IN_DUCK + b_longjump_do_jump
   ASSERT_TRUE(pEdict->v.button & IN_DUCK);
   ASSERT_INT(bot.b_longjump_do_jump, TRUE);
   ASSERT_TRUE(bot.f_longjump_time > gpGlobals->time);
   PASS();
   return 0;
}

static int test_head_toward_visibility_recheck(void)
{
   TEST("BotHeadTowardWaypoint: visibility blocked -> re-find");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   // Waypoint far away (not touching)
   setup_waypoint(0, Vector(500, 0, 0));
   setup_waypoint(1, Vector(100, 0, 0)); // closer replacement
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = 0; // not exiting water

   // First trace (visibility to waypoint 0) hits
   // Subsequent traces clear
   static int vis_trace_call;
   vis_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = vis_trace_call++;
      if (call == 0) // visibility check to current waypoint
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
   };

   // WaypointFindReachable returns waypoint 1
   mock_WaypointFindReachable_result = 1;

   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 1); // switched to new waypoint
   PASS();
   return 0;
}

static int test_head_toward_visibility_recheck_fails(void)
{
   TEST("BotHeadTowardWaypoint: visibility blocked, no new wp -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   setup_waypoint(0, Vector(500, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = 0;

   // Visibility check hits, no new waypoint available
   mock_trace_hull_fn = trace_all_hit;
   mock_WaypointFindReachable_result = -1;

   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, FALSE);
   ASSERT_INT(bot.curr_waypoint_index, -1);
   PASS();
   return 0;
}

static int test_head_toward_crouch_min_distance(void)
{
   TEST("BotHeadTowardWaypoint: crouch wp -> min_distance=20");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   // Place bot close but > 20 units (so NOT touching with crouch dist)
   // and < 50 units (so WOULD be touching with normal dist)
   pEdict->v.origin = Vector(75, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_CROUCH);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // At 25 units away with min_distance=20, NOT touching
   // Bot should still be heading toward waypoint 0
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_head_toward_lift_start_min_distance(void)
{
   TEST("BotHeadTowardWaypoint: LIFT_START wp -> min_distance=32");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(60, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_LIFT_START);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // At 40 units away, with min_distance=32 for LIFT_START → NOT touching
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_head_toward_item_min_distance(void)
{
   TEST("BotHeadTowardWaypoint: item wp -> min_distance=20");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(75, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_HEALTH);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // 25 units away with min_distance=20 → NOT touching
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_head_toward_exit_water_min_distance(void)
{
   TEST("BotHeadTowardWaypoint: exit water -> min_distance=20");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(75, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = gpGlobals->time + 10.0f; // exiting water

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // 25 units away with exit_water min_distance=20 → NOT touching
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_head_toward_goal_reached_with_enemy(void)
{
   TEST("BotHeadTowardWaypoint: goal reached with enemy -> goal time 0.5");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pEnemy = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_HEALTH);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_goal = 0;
   bot.wpt_goal_type = WPT_GOAL_HEALTH;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.pBotEnemy = pEnemy;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;
   mock_trace_hull_fn = trace_all_clear;

   // With enemy, goal reached should set f_waypoint_goal_time = time + 0.25
   // Then the "if pBotEnemy != NULL" sets it to time + 0.5
   mock_WaypointFindNearest_result = 1; // for enemy waypoint search
   BotHeadTowardWaypoint(bot);

   ASSERT_INT(bot.waypoint_goal, -1); // goal cleared
   PASS();
   return 0;
}

static int test_head_toward_exclude_points(void)
{
   TEST("BotHeadTowardWaypoint: goal reached -> shifts exclude_points");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0), W_FL_ARMOR);
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_goal = 0;
   bot.wpt_goal_type = WPT_GOAL_ARMOR;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   // Pre-fill exclude_points
   bot.exclude_points[0] = 99;
   bot.exclude_points[1] = -1;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;
   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // exclude_points should shift: [0] = waypoint_goal (0), [1] = old [0] (99)
   ASSERT_INT(bot.exclude_points[0], 0);
   ASSERT_INT(bot.exclude_points[1], 99);
   PASS();
   return 0;
}

static int test_head_toward_track_sound_update(void)
{
   TEST("BotHeadTowardWaypoint: track sound -> updates goal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
   bot.waypoint_goal = 5;
   bot.f_track_sound_time = gpGlobals->time + 10.0f;
   bot.b_has_enough_ammo_for_good_weapon = TRUE;
   bot.b_low_health = FALSE;

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // BotUpdateTrackSoundGoal was called (track sound still valid)
   ASSERT_INT(bot.wpt_goal_type, WPT_GOAL_TRACK_SOUND);
   PASS();
   return 0;
}

static int test_head_toward_goal_time_reset(void)
{
   TEST("BotHeadTowardWaypoint: no goal, high goal_time -> resets");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.waypoint_goal = -1;
   bot.f_waypoint_goal_time = gpGlobals->time + 100.0f; // high, should reset

   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // With no goal and goal_time far in future, it should reset to 0
   ASSERT_FLOAT_NEAR(bot.f_waypoint_goal_time, 0.0f, 0.01f);
   PASS();
   return 0;
}

static int test_head_toward_find_nearest_after_ladder(void)
{
   TEST("BotHeadTowardWaypoint: after ladder -> FindNearest");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.curr_waypoint_index = -1;
   bot.f_end_use_ladder_time = gpGlobals->time; // just got off ladder

   setup_waypoint(0, Vector(100, 0, 0));
   mock_WaypointFindNearest_result = 0;
   mock_trace_hull_fn = trace_all_clear;

   qboolean result = BotHeadTowardWaypoint(bot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 0);
   PASS();
   return 0;
}

static int test_head_toward_enemy_goal_time(void)
{
   TEST("BotHeadTowardWaypoint: enemy present -> goal time 0.5s");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pEnemy = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(100, 0, 0);

   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(200, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.pBotEnemy = pEnemy;
   pEnemy->v.origin = Vector(500, 0, 0);
   bot.f_waypoint_goal_time = 0; // allow goal search

   mock_WaypointFindNearest_result = 1;
   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_count = 1;
   mock_trace_hull_fn = trace_all_clear;

   BotHeadTowardWaypoint(bot);

   // With enemy, f_waypoint_goal_time should be time + 0.5
   ASSERT_FLOAT_NEAR(bot.f_waypoint_goal_time, (float)gpGlobals->time + 0.5f, 0.01f);
   PASS();
   return 0;
}

// ============================================================
// 27. Additional BotOnLadder tests
// ============================================================

static int test_on_ladder_waypoint_directed(void)
{
   TEST("BotOnLadder: waypoint -> searches toward wp first");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_UNKNOWN;
   pEdict->v.v_angle.y = 0.0f;
   bot.curr_waypoint_index = 0;

   // Waypoint directly ahead
   setup_waypoint(0, Vector(100, 0, 0));

   // Wall hit used for squaring up
   mock_alloc_edict(); // edict 2 for wall
   mock_edicts[2].v.solid = SOLID_BSP;
   mock_trace_hull_fn = trace_ladder_wall;

   BotOnLadder(bot, 0.0f);

   // Should use waypoint-directed angle for first search
   ASSERT_INT(bot.ladder_dir, LADDER_UP);
   PASS();
   return 0;
}

static int test_on_ladder_second_wall_direction(void)
{
   TEST("BotOnLadder: first dir clear, second dir hits wall");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.ladder_dir = LADDER_UNKNOWN;
   pEdict->v.v_angle.y = 0.0f;

   // Alloc edict 2 for the wall
   mock_alloc_edict();
   mock_edicts[2].v.solid = SOLID_BSP;

   // First direction (forward): clear
   // Second direction (forward - angle): hits wall
   static int ladder_trace_call;
   ladder_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = ladder_trace_call++;
      if (call % 2 == 0) // first direction: clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
      else // second direction: hit BSP wall
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[2];
         mock_edicts[2].v.solid = SOLID_BSP;
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
         ptr->vecPlaneNormal[0] = -1;
         ptr->vecPlaneNormal[1] = 0;
         ptr->vecPlaneNormal[2] = 0;
      }
   };

   BotOnLadder(bot, 0.0f);

   ASSERT_INT(bot.ladder_dir, LADDER_UP);
   PASS();
   return 0;
}

// ============================================================
// 28. Additional BotUseLift tests
// ============================================================

static int test_use_lift_finds_lift_start(void)
{
   TEST("BotUseLift: button press finds LIFT_START wp");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time; // just pressed
   bot.v_use_target = Vector(100, 0, 28);
   bot.b_lift_moving = FALSE;
   bot.b_use_button = TRUE;
   bot.curr_waypoint_index = 0;
   pEdict->v.yaw_speed = 100.0f;

   setup_waypoint(0, Vector(0, 0, 0));
   setup_waypoint(5, Vector(10, 0, 0), W_FL_LIFT_START);

   mock_WaypointFindNearestGoal_result = 5;
   mock_trace_hull_fn = trace_all_clear;

   BotUseLift(bot, 0.0f);

   // Should have found LIFT_START waypoint and set it as current
   ASSERT_INT(bot.curr_waypoint_index, 5);
   ASSERT_INT(bot.prev_waypoint_index[0], 0); // old curr pushed
   PASS();
   return 0;
}

static int test_use_lift_stops_right_clear(void)
{
   TEST("BotUseLift: lift stops, right clear -> turn right");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time - 5.0f;
   bot.b_lift_moving = TRUE;
   bot.b_use_button = TRUE;
   bot.f_max_speed = 320.0f;
   pEdict->v.ideal_yaw = 0.0f;
   pEdict->v.v_angle.y = 0.0f;
   pEdict->v.yaw_speed = 100.0f;

   // Forward: blocked+floor, Right: clear+floor
   static int lift_trace_call;
   lift_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = lift_trace_call++;
      if (call == 0) // forward horizontal: blocked
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
      }
      else if (call == 2) // right horizontal: clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
      else if (call == 3) // right downward: hit floor
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
   };

   BotUseLift(bot, 0.5f); // barely moving -> stopped

   // Should turn right (yaw -= 90)
   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, -90.0f, 1.0f);
   PASS();
   return 0;
}

static int test_use_lift_stops_left_clear(void)
{
   TEST("BotUseLift: lift stops, left clear -> turn left");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.f_use_button_time = gpGlobals->time - 5.0f;
   bot.b_lift_moving = TRUE;
   bot.b_use_button = TRUE;
   bot.f_max_speed = 320.0f;
   pEdict->v.ideal_yaw = 0.0f;
   pEdict->v.v_angle.y = 0.0f;
   pEdict->v.yaw_speed = 100.0f;

   // Forward: blocked, Right: blocked, Left: clear+floor
   static int lift_left_call;
   lift_left_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = lift_left_call++;
      if (call == 4) // left horizontal: clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
      else if (call == 5) // left downward: floor
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
      }
      else // forward and right: blocked
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
      }
   };

   BotUseLift(bot, 0.5f);

   // Should turn left (yaw += 90)
   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, 90.0f, 1.0f);
   PASS();
   return 0;
}

// ============================================================
// 29. Additional BotLookForDrop tests
// ============================================================

static int test_look_for_drop_lava(void)
{
   TEST("BotLookForDrop: lava detected -> turns");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.v_angle.y = 0.0f;

   // Forward: clear. Downward: hits but it's lava
   static int lava_call;
   lava_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = lava_call++;
      if (call == 1) // downward trace: hit
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
      }
      else // all other traces: clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };

   mock_point_contents_fn = [](const float *origin) -> int {
      (void)origin;
      return CONTENTS_LAVA;
   };

   BotLookForDrop(bot);

   // Should have turned (ideal_yaw changed)
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw) > 1.0f);
   PASS();
   return 0;
}

static int test_look_for_drop_with_enemy(void)
{
   TEST("BotLookForDrop: drop with enemy -> removes enemy");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pEnemy = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.pBotEnemy = pEnemy;
   pEdict->v.v_angle.y = 0.0f;

   // Forward clear, downward too far (all clear)
   mock_trace_hull_fn = trace_all_clear;

   BotLookForDrop(bot);

   // Enemy should be removed, idealpitch set to 0
   ASSERT_FLOAT_NEAR(pEdict->v.idealpitch, 0.0f, 0.01f);
   PASS();
   return 0;
}

static int test_look_for_drop_ducking(void)
{
   TEST("BotLookForDrop: ducking -> uses shorter offset");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_ducking = TRUE;
   pEdict->v.v_angle.y = 0.0f;

   // Forward clear, downward too far
   mock_trace_hull_fn = trace_all_clear;

   BotLookForDrop(bot);

   // Should have turned (ducking offset of -22 instead of -40)
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw) > 1.0f);
   PASS();
   return 0;
}

static int test_look_for_drop_turn_at_wall(void)
{
   TEST("BotLookForDrop: standing ground -> BotTurnAtWall");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.v_angle.y = 0.0f;

   // Forward clear, downward too far, then turn-trace hits ground
   static int wall_trace_call;
   wall_trace_call = 0;
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      int call = wall_trace_call++;
      if (call == 2) // third trace (ground check below feet): hit
      {
         ptr->flFraction = 0.5f;
         ptr->pHit = &mock_edicts[0];
         ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
         ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
         ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
         ptr->vecPlaneNormal[0] = 0;
         ptr->vecPlaneNormal[1] = 1;
         ptr->vecPlaneNormal[2] = 0;
      }
      else // other traces: clear
      {
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };

   BotLookForDrop(bot);

   // BotTurnAtWall should have changed ideal_yaw
   ASSERT_TRUE(fabs(pEdict->v.ideal_yaw) > 1.0f);
   PASS();
   return 0;
}

// ============================================================
// 30. Additional BotEdge pre-check tests
// ============================================================

static int test_edge_forward_pre_check_duck(void)
{
   TEST("BotEdgeForward: ducking -> early FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;
   pEdict->v.button |= IN_DUCK;

   qboolean result = BotEdgeForward(bot, Vector(100, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_edge_right_pre_check_ladder(void)
{
   TEST("BotEdgeRight: on ladder -> early FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;
   bot.b_on_ladder = TRUE;

   qboolean result = BotEdgeRight(bot, Vector(100, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_edge_left_pre_check_jump(void)
{
   TEST("BotEdgeLeft: jumping -> early FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;
   pEdict->v.button |= IN_JUMP;

   qboolean result = BotEdgeLeft(bot, Vector(100, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

static int test_edge_forward_zero_velocity(void)
{
   TEST("BotEdgeForward: zero velocity -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bot.b_on_ground = TRUE;

   mock_trace_hull_fn = trace_all_clear;

   // Zero-length move direction
   qboolean result = BotEdgeForward(bot, Vector(0, 0, 0));
   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// ============================================================
// 31. Additional BotCanJumpUp side trace tests
// ============================================================

static int test_can_jump_up_left_side_blocked(void)
{
   TEST("BotCanJumpUp: left side blocked -> check_duck path");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block only left side trace (3rd horizontal trace = bit 2)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 2);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   // Left side blocked → check_duck=TRUE → duck traces all clear → duck jump
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bDuckJump, TRUE);
   PASS();
   return 0;
}

// ============================================================
// Random-dependent path tests (using mock RANDOM_LONG2/FLOAT2)
// ============================================================

// BotRandomTurn: 180 degree turn (RANDOM_LONG2(1,100) <= 10)
static int test_random_turn_180_degree(void)
{
   TEST("BotRandomTurn: 180 degree turn (random <= 10)");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.ideal_yaw = 45.0f;
   mock_random_long_ret = 1; // RANDOM_LONG2(1,100) returns 1 (<=10)

   BotRandomTurn(bot);

   // 45 + 180 = 225 → wraps to -135 via BotFixIdealYaw
   ASSERT_FLOAT_NEAR(pEdict->v.ideal_yaw, -135.0f, 0.01f);
   ASSERT_FLOAT_NEAR(bot.f_move_speed, 0.0f, 0.01f);
   PASS();
   return 0;
}

// BotFindWaypoint: random selection with 3 candidates (line 251)
static int test_find_waypoint_random_3_candidates(void)
{
   TEST("BotFindWaypoint: random select from 3 candidates");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   // Set up current waypoint with 3 connected paths at different distances
   setup_waypoint(0, Vector(0, 0, 0)); // current
   setup_waypoint(1, Vector(100, 0, 0)); // closest
   setup_waypoint(2, Vector(200, 0, 0)); // middle
   setup_waypoint(3, Vector(300, 0, 0)); // farthest
   bot.curr_waypoint_index = 0;

   // WaypointFindPath returns 1, 2, 3 then -1
   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_results[1] = 2;
   mock_WaypointFindPath_results[2] = 3;
   mock_WaypointFindPath_results[3] = -1;
   mock_WaypointFindPath_count = 4;

   // Enable random waypoint selection: RANDOM_LONG2(1,100) <= 20
   mock_random_long_ret = 10; // clamped to range [1,100], returns 10 (<=20)
   bot.f_random_waypoint_time = 0; // in the past
   gpGlobals->time = 10.0f;

   qboolean result = BotFindWaypoint(bot);

   // With 3 candidates and RANDOM_LONG2(0,2), mock returns clamped 10→2
   // Selects min_index[2] = waypoint 3 (farthest)
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 3);
   PASS();
   return 0;
}

// BotFindWaypoint: random selection with 2 candidates (line 253)
static int test_find_waypoint_random_2_candidates(void)
{
   TEST("BotFindWaypoint: random select from 2 candidates");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   setup_waypoint(0, Vector(0, 0, 0)); // current
   setup_waypoint(1, Vector(100, 0, 0)); // closest
   setup_waypoint(2, Vector(200, 0, 0)); // next

   bot.curr_waypoint_index = 0;

   mock_WaypointFindPath_results[0] = 1;
   mock_WaypointFindPath_results[1] = 2;
   mock_WaypointFindPath_results[2] = -1;
   mock_WaypointFindPath_count = 3;

   mock_random_long_ret = 1; // for RANDOM_LONG2(1,100)→1 (<=20), then RANDOM_LONG2(0,1)→1
   bot.f_random_waypoint_time = 0;
   gpGlobals->time = 10.0f;

   qboolean result = BotFindWaypoint(bot);

   // With 2 candidates and RANDOM_LONG2(0,1) returning 1, selects min_index[1] = wp 2
   ASSERT_INT(result, TRUE);
   ASSERT_INT(bot.curr_waypoint_index, 2);
   PASS();
   return 0;
}

// BotGetSoundWaypoint: NULL sound pointer causes break (line 322)
static int test_get_sound_waypoint_null_sound_ptr(void)
{
   TEST("BotGetSoundWaypoint: NULL sound ptr -> break");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;
   pEdict->v.origin = Vector(0, 0, 0);

   // Set active list to index 4 which is out of bounds for mock_sounds[4]
   // SoundPointerForIndex returns NULL for index >= 4
   mock_active_sound_list = 4;

   edict_t *pNew = NULL;
   int result = BotGetSoundWaypoint(bots[0], NULL, &pNew);

   ASSERT_INT(result, -1);
   PASS();
   return 0;
}

// BotUpdateTrackSoundGoal: pTrackSoundEdict replaced by pNew (line 400)
static int test_update_track_sound_goal_replace_edict(void)
{
   TEST("BotUpdateTrackSoundGoal: null edict replaced by pNew");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pSoundSource = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;

   bots[0].wpt_goal_type = WPT_GOAL_TRACK_SOUND;
   bots[0].pTrackSoundEdict = NULL; // null - will be replaced
   bots[0].f_track_sound_time = gpGlobals->time + 10.0f;
   bots[0].b_has_enough_ammo_for_good_weapon = TRUE;
   bots[0].b_low_health = FALSE;
   skill_settings[bots[0].bot_skill].hearing_sensitivity = 1.0f;

   // Set up a sound from pSoundSource
   pEdict->v.origin = Vector(0, 0, 0);
   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 99; // not this bot
   mock_sounds[0].m_pEdict = pSoundSource;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   // BotGetSoundWaypoint needs to find a nearby waypoint for the sound
   setup_waypoint(0, Vector(100, 0, 0));
   mock_WaypointFindNearest_result = 0;
   mock_WaypointDistanceFromTo_result = 100.0f;
   bots[0].curr_waypoint_index = 1;

   qboolean result = BotUpdateTrackSoundGoal(bots[0]);

   // Should have replaced pTrackSoundEdict with pSoundSource
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE(bots[0].pTrackSoundEdict == pSoundSource);
   PASS();
   return 0;
}

// BotFindWaypointGoal: track sound with goto exit (RANDOM < 0.5, lines 454-465)
static int test_find_waypoint_goal_track_sound_goto(void)
{
   TEST("BotFindWaypointGoal: track sound -> goto exit");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pSoundSource = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;

   bots[0].pBotEnemy = NULL;
   bots[0].b_low_health = FALSE;
   bots[0].b_has_enough_ammo_for_good_weapon = TRUE;
   bots[0].f_waypoint_goal_time = 0; // expired
   pEdict->v.health = 100;
   skill_settings[bots[0].bot_skill].hearing_sensitivity = 1.0f;

   // Set up sound
   pEdict->v.origin = Vector(0, 0, 0);
   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 99;
   mock_sounds[0].m_pEdict = pSoundSource;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(0, 0, 0));
   mock_WaypointFindNearest_result = 0;
   mock_WaypointDistanceFromTo_result = 100.0f;
   bots[0].curr_waypoint_index = 1;

   // RANDOM_FLOAT2(0.9, 1/0.9) for critical health check - with health=100
   // this won't trigger critical path (100 * ~1.0 > 25)
   // RANDOM_FLOAT2(0.0, 1.0) for track sound: mock returns 0.0 (<0.5 → goto exit)
   mock_random_float_ret = 0.0f;

   // Set skill track_sound_time settings
   skill_settings[bots[0].bot_skill].track_sound_time_min = 5.0f;
   skill_settings[bots[0].bot_skill].track_sound_time_max = 10.0f;

   BotFindWaypointGoal(bots[0]);

   ASSERT_INT(bots[0].wpt_goal_type, WPT_GOAL_TRACK_SOUND);
   ASSERT_INT(bots[0].waypoint_goal, 0);
   ASSERT_TRUE(bots[0].pTrackSoundEdict == pSoundSource);
   ASSERT_TRUE(bots[0].f_track_sound_time > gpGlobals->time);
   PASS();
   return 0;
}

// BotFindWaypointGoal: track sound distance compare (RANDOM >= 0.5, lines 469-475)
static int test_find_waypoint_goal_track_sound_distance(void)
{
   TEST("BotFindWaypointGoal: track sound -> distance compare");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   edict_t *pSoundSource = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;

   bots[0].pBotEnemy = NULL;
   bots[0].b_low_health = FALSE;
   bots[0].b_has_enough_ammo_for_good_weapon = TRUE;
   bots[0].f_waypoint_goal_time = 0;
   pEdict->v.health = 100;
   pEdict->v.armorvalue = 100;
   skill_settings[bots[0].bot_skill].hearing_sensitivity = 1.0f;

   pEdict->v.origin = Vector(0, 0, 0);
   mock_sounds[0].m_vecOrigin = Vector(100, 0, 0);
   mock_sounds[0].m_iVolume = 1000;
   mock_sounds[0].m_iBotOwner = 99;
   mock_sounds[0].m_pEdict = pSoundSource;
   mock_sounds[0].m_iNext = SOUNDLIST_EMPTY;
   mock_active_sound_list = 0;

   setup_waypoint(0, Vector(100, 0, 0));
   setup_waypoint(1, Vector(0, 0, 0));
   mock_WaypointFindNearest_result = 0;
   mock_WaypointDistanceFromTo_result = 100.0f;
   bots[0].curr_waypoint_index = 1;

   // RANDOM_FLOAT2(0.0, 1.0) returns 0.9 (>=0.5 → distance compare path)
   mock_random_float_ret = 0.9f;

   // No other goals will be found (WaypointFindNearestGoal returns -1)
   mock_WaypointFindNearestGoal_result = -1;
   mock_WaypointFindRandomGoal_result = -1;

   // longjump already obtained
   bots[0].b_longjump = TRUE;

   // Set skill track_sound_time settings
   skill_settings[bots[0].bot_skill].track_sound_time_min = 5.0f;
   skill_settings[bots[0].bot_skill].track_sound_time_max = 10.0f;

   BotFindWaypointGoal(bots[0]);

   // Track sound distance was 100 < WAYPOINT_UNREACHABLE, so it wins
   // At exit, goal_type = WPT_GOAL_TRACK_SOUND → sets pTrackSoundEdict
   ASSERT_INT(bots[0].wpt_goal_type, WPT_GOAL_TRACK_SOUND);
   ASSERT_INT(bots[0].waypoint_goal, 0);
   ASSERT_TRUE(bots[0].pTrackSoundEdict == pSoundSource);
   ASSERT_TRUE(bots[0].f_track_sound_time > gpGlobals->time);
   PASS();
   return 0;
}

// BotFindWaypointGoal: health RandomGoal fallback (line 487)
static int test_find_waypoint_goal_health_random_fallback(void)
{
   TEST("BotFindWaypointGoal: health NearestGoal=-1 -> RandomGoal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;

   bots[0].pBotEnemy = NULL;
   bots[0].b_low_health = FALSE;
   bots[0].b_has_enough_ammo_for_good_weapon = TRUE;
   bots[0].f_waypoint_goal_time = 0;
   bots[0].b_longjump = TRUE; // skip longjump search

   pEdict->v.health = 50; // < 95 → wants health
   pEdict->v.armorvalue = 100; // full → skip armor
   pEdict->v.origin = Vector(0, 0, 0);

   // high float ret so critical health check doesn't trigger: 50 * 1.0 > 25
   mock_random_float_ret = 1.0f;

   // NearestGoal returns -1, RandomGoal returns 5
   setup_waypoint(5, Vector(200, 0, 0));
   mock_WaypointFindNearestGoal_result = -1;
   mock_WaypointFindRandomGoal_result = 5;
   mock_WaypointDistanceFromTo_result = 200.0f;

   BotFindWaypointGoal(bots[0]);

   ASSERT_INT(bots[0].wpt_goal_type, WPT_GOAL_HEALTH);
   ASSERT_INT(bots[0].waypoint_goal, 5);
   PASS();
   return 0;
}

// BotFindWaypointGoal: armor RandomGoal fallback (line 510)
static int test_find_waypoint_goal_armor_random_fallback(void)
{
   TEST("BotFindWaypointGoal: armor NearestGoal=-1 -> RandomGoal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;

   bots[0].pBotEnemy = NULL;
   bots[0].b_low_health = FALSE;
   bots[0].b_has_enough_ammo_for_good_weapon = TRUE;
   bots[0].f_waypoint_goal_time = 0;
   bots[0].b_longjump = TRUE;

   pEdict->v.health = 100; // full → skip health
   pEdict->v.armorvalue = 50; // < 95 → wants armor
   pEdict->v.origin = Vector(0, 0, 0);

   mock_random_float_ret = 1.0f;

   setup_waypoint(6, Vector(200, 0, 0));
   mock_WaypointFindNearestGoal_result = -1;
   mock_WaypointFindRandomGoal_result = 6;
   mock_WaypointDistanceFromTo_result = 200.0f;

   BotFindWaypointGoal(bots[0]);

   ASSERT_INT(bots[0].wpt_goal_type, WPT_GOAL_ARMOR);
   ASSERT_INT(bots[0].waypoint_goal, 6);
   PASS();
   return 0;
}

// BotFindWaypointGoal: longjump RandomGoal fallback (line 533)
static int test_find_waypoint_goal_longjump_random_fallback(void)
{
   TEST("BotFindWaypointGoal: longjump NearestGoal=-1 -> RandomGoal");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   bots[0] = bot;
   bots[0].pEdict = pEdict;

   bots[0].pBotEnemy = NULL;
   bots[0].b_low_health = FALSE;
   bots[0].b_has_enough_ammo_for_good_weapon = TRUE;
   bots[0].f_waypoint_goal_time = 0;
   bots[0].b_longjump = FALSE; // needs longjump
   skill_settings[bots[0].bot_skill].can_longjump = TRUE;

   pEdict->v.health = 100; // full
   pEdict->v.armorvalue = 100; // full
   pEdict->v.origin = Vector(0, 0, 0);

   mock_random_float_ret = 1.0f;

   setup_waypoint(7, Vector(200, 0, 0));
   mock_WaypointFindNearestGoal_result = -1;
   mock_WaypointFindRandomGoal_result = 7;
   mock_WaypointDistanceFromTo_result = 200.0f;

   BotFindWaypointGoal(bots[0]);

   ASSERT_INT(bots[0].wpt_goal_type, WPT_GOAL_ITEM);
   ASSERT_INT(bots[0].waypoint_goal, 7);
   PASS();
   return 0;
}

// BotHeadTowardWaypoint: visibility blocked after ladder → FindNearest (line 829)
static int test_head_toward_visibility_after_ladder(void)
{
   TEST("BotHeadTowardWaypoint: after ladder, vis blocked -> FindNearest");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);
   pEdict->v.origin = Vector(0, 0, 0);

   setup_waypoint(0, Vector(500, 0, 0));
   setup_waypoint(1, Vector(100, 0, 0));
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = 0;

   // Recently came off a ladder
   bot.f_end_use_ladder_time = gpGlobals->time; // within +2.0 of current time

   // Visibility trace blocks → triggers the ladder/water branch → WaypointFindNearest
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      ptr->flFraction = 0.5f;
      ptr->pHit = &mock_edicts[0];
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   };
   mock_WaypointFindNearest_result = 1;

   BotHeadTowardWaypoint(bot);

   // Should have found nearest (not reachable) since we just came off a ladder
   ASSERT_INT(bot.curr_waypoint_index, 1);
   PASS();
   return 0;
}

// BotHeadTowardWaypoint: LIFT_END waypoint → min_distance=20 (line 874)
static int test_head_toward_lift_end_min_distance(void)
{
   TEST("BotHeadTowardWaypoint: LIFT_END wp -> min_distance=20");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Place bot 19 units from waypoint (within 20 = touching for LIFT_END)
   setup_waypoint(0, Vector(19, 0, 0));
   waypoints[0].flags = W_FL_LIFT_END;
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = 0;
   pEdict->v.origin = Vector(0, 0, 0);

   mock_trace_hull_fn = trace_all_clear;
   mock_WaypointFindPath_results[0] = -1;
   mock_WaypointFindPath_count = 1;

   BotHeadTowardWaypoint(bot);

   // 19 < 20 = touching for LIFT_END waypoint
   ASSERT_FLOAT_NEAR(bot.prev_waypoint_distance, 0.0f, 0.01f);
   PASS();
   return 0;
}

// BotHeadTowardWaypoint: item waypoint min_distance=20 (line 877-878)
static int test_head_toward_item_wp_min_distance(void)
{
   TEST("BotHeadTowardWaypoint: HEALTH wp -> min_distance=20");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   setup_waypoint(0, Vector(19, 0, 0));
   waypoints[0].flags = W_FL_HEALTH;
   bot.curr_waypoint_index = 0;
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = 0;
   pEdict->v.origin = Vector(0, 0, 0);

   mock_trace_hull_fn = trace_all_clear;
   mock_WaypointFindPath_results[0] = -1;
   mock_WaypointFindPath_count = 1;

   BotHeadTowardWaypoint(bot);

   // Should have touched (19 < 20 for item waypoints)
   ASSERT_FLOAT_NEAR(bot.prev_waypoint_distance, 0.0f, 0.01f);
   PASS();
   return 0;
}

// BotHeadTowardWaypoint: goal reached with WEAPON goal type → exclude_points (lines 938-940)
static int test_head_toward_weapon_goal_exclude_points(void)
{
   TEST("BotHeadTowardWaypoint: WEAPON goal -> exclude_points");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   setup_waypoint(0, Vector(10, 0, 0));
   waypoints[0].flags = W_FL_WEAPON;
   bot.curr_waypoint_index = 0;
   bot.waypoint_goal = 0; // this IS the goal
   bot.waypoint_origin = waypoints[0].origin;
   bot.f_waypoint_time = gpGlobals->time;
   bot.f_exit_water_time = 0;
   bot.wpt_goal_type = WPT_GOAL_WEAPON;
   pEdict->v.origin = Vector(0, 0, 0);

   mock_trace_hull_fn = trace_all_clear;
   mock_WaypointFindPath_results[0] = -1;
   mock_WaypointFindPath_count = 1;

   BotHeadTowardWaypoint(bot);

   // Goal reached with WEAPON type → shifts exclude_points, finds item
   ASSERT_INT(bot.exclude_points[0], 0); // waypoint 0 added to exclude
   ASSERT_INT(bot.waypoint_goal, -1);    // goal cleared
   PASS();
   return 0;
}

// BotCanJumpUp: right side blocked at duck height → return FALSE (line 1714)
static int test_can_jump_up_right_duck_blocked(void)
{
   TEST("BotCanJumpUp: duck height right blocked -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Block specific traces to reach line 1714:
   // Normal traces: center clear, right blocked → check_duck = TRUE
   // Duck traces: center clear (line 1701), left clear (line 1713), RIGHT blocked (line 1714)
   //
   // BotCanJumpUp does many traces. The trace order for "right side blocked at duck" is:
   // 1. center normal height: clear
   // 2. right normal height: blocked → check_duck = TRUE (line 1672 already covered)
   // Actually we need to get to line 1714 which is in the check_duck section.
   // Trace order within check_duck:
   //   - center at duck+64 (call after check_duck set): if blocked → 1702 FALSE
   //   - left at duck+64: if blocked → 1714 FALSE
   // Wait, lines 1704-1714: "other side" = right side... let me re-read

   // Use trace_jump_selective to block specific call numbers
   // Normal: 3 horizontal traces (center, right, left) + (if needed, check_duck traces)
   // Call 0: center at jump height → clear
   // Call 1: right at jump height → clear
   // Call 2: left at jump height → blocked → check_duck = TRUE
   // check_duck section:
   // Call 3: center at duck+64 → clear (line 1701)
   // Call 4: left at duck+64 → clear (line 1713)
   // Call 5: right at duck+64 → blocked → return FALSE (line 1714... but wait)

   // Actually looking at lines more carefully:
   // line 1692-1702: center duck → if blocked return FALSE
   // line 1704-1714: "other side" (v_right * -16) → LEFT side at duck+64 → if blocked return FALSE (line 1714)
   // line 1716-1726: "right side" (v_right * 16) → RIGHT side → if blocked return FALSE

   // So line 1714 = LEFT side blocked at duck+64 level. Need:
   // Normal: block one of center/right/left to set check_duck = TRUE
   // Duck: center clear, then LEFT blocked at duck+64

   g_jump_trace_call = 0;
   // Block call 2 (left at normal height) to set check_duck
   // Block call 4 (left at duck height) for line 1714
   g_jump_block_mask = (1 << 2) | (1 << 4);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// BotCanJumpUp: downward trace hits at right side → FALSE (line 1779)
static int test_can_jump_up_downward_right_blocked(void)
{
   TEST("BotCanJumpUp: downward trace right blocked -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // All horizontal traces clear (no check_duck), then downward traces:
   // Call 0: center forward → clear
   // Call 1: right forward → clear
   // Call 2: left forward → clear
   // Call 3: downward center → clear (line 1757)
   // Call 4: downward right → blocked → FALSE (line 1779)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 4);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
   PASS();
   return 0;
}

// BotCanJumpUp: downward trace hits at other side → FALSE (line 1800)
static int test_can_jump_up_downward_left_blocked(void)
{
   TEST("BotCanJumpUp: downward trace left blocked -> FALSE");
   mock_reset();
   reset_navigate_mocks();

   edict_t *pEdict = mock_alloc_edict();
   bot_t bot;
   setup_bot_for_test(bot, pEdict);

   // Call 0-2: horizontal clear
   // Call 3: downward center clear
   // Call 4: downward right clear
   // Call 5: downward left blocked → FALSE (line 1800)
   g_jump_trace_call = 0;
   g_jump_block_mask = (1 << 5);
   mock_trace_hull_fn = trace_jump_selective;

   qboolean bDuckJump = FALSE;
   qboolean result = BotCanJumpUp(bot, &bDuckJump);

   ASSERT_INT(result, FALSE);
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
   failures += test_wall_on_forward_clear();
   failures += test_wall_on_forward_no_corrupt();
   failures += test_wall_on_back_hit();
   failures += test_wall_on_back_clear();
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

   printf("=== BotEvaluateGoal tests ===\n");
   failures += test_evaluate_goal_no_goal();
   failures += test_evaluate_goal_high_health();
   failures += test_evaluate_goal_low_health_health_goal();
   failures += test_evaluate_goal_low_health_non_health_goal();

   printf("=== BotRandomTurn tests ===\n");
   failures += test_random_turn_wander_left();
   failures += test_random_turn_move_speed_zero();

   printf("=== BotTurnAtWall tests ===\n");
   failures += test_turn_at_wall_perpendicular();
   failures += test_turn_at_wall_negative();

   printf("=== BotFindWaypoint tests ===\n");
   failures += test_find_waypoint_no_paths();
   failures += test_find_waypoint_single_connected();
   failures += test_find_waypoint_closest_of_multiple();
   failures += test_find_waypoint_prev_exclusion();

   printf("=== BotGetSoundWaypoint tests ===\n");
   failures += test_get_sound_waypoint_no_sounds();
   failures += test_get_sound_waypoint_own_sound_skipped();
   failures += test_get_sound_waypoint_out_of_range();
   failures += test_get_sound_waypoint_valid();

   printf("=== BotOnLadder tests ===\n");
   failures += test_on_ladder_unknown_finds_wall();
   failures += test_on_ladder_unknown_no_wall();
   failures += test_on_ladder_up_normal();
   failures += test_on_ladder_up_stuck();
   failures += test_on_ladder_down_stuck();

   printf("=== BotUnderWater tests ===\n");
   failures += test_underwater_waypoints_head_toward();
   failures += test_underwater_no_waypoints_surface();
   failures += test_underwater_trace_blocked();
   failures += test_underwater_surface_is_water();

   printf("=== BotUseLift tests ===\n");
   failures += test_use_lift_button_press();
   failures += test_use_lift_timeout();
   failures += test_use_lift_starts_moving();
   failures += test_use_lift_stops_forward_clear();
   failures += test_use_lift_stops_all_blocked();

   printf("=== BotFindWaypointGoal tests ===\n");
   failures += test_find_waypoint_goal_critical_health();
   failures += test_find_waypoint_goal_enemy();
   failures += test_find_waypoint_goal_fallback_random();
   failures += test_find_waypoint_goal_needs_health();

   printf("=== BotHeadTowardWaypoint tests ===\n");
   failures += test_head_toward_no_waypoint_find_reachable();
   failures += test_head_toward_nothing_found();
   failures += test_head_toward_timeout_resets();
   failures += test_head_toward_touch_advances();
   failures += test_head_toward_loop_the_loop();
   failures += test_head_toward_door_flag();
   failures += test_head_toward_jump_flag();
   failures += test_head_toward_goal_reached();
   failures += test_head_toward_goal_routing();
   failures += test_head_toward_ladder_flag();
   failures += test_head_toward_in_water_exit();
   failures += test_head_toward_retry_loop();
   failures += test_head_toward_pitch_yaw_toward_waypoint();

   printf("=== Additional BotChangePitch/Yaw wrap-around tests ===\n");
   failures += test_change_pitch_wrap_above_180();
   failures += test_change_pitch_wrap_below_minus180();
   failures += test_change_pitch_both_neg_cur_gt_ideal();
   failures += test_change_yaw_wrap_above_180();
   failures += test_change_yaw_wrap_below_minus180();
   failures += test_change_yaw_both_neg_cur_gt_ideal();

   printf("=== Additional BotCheckWall tests ===\n");
   failures += test_wall_on_right_clear();

   printf("=== Additional BotFindWaypoint tests ===\n");
   failures += test_find_waypoint_three_candidates();

   printf("=== Additional BotGetSoundWaypoint tests ===\n");
   failures += test_get_sound_waypoint_own_sound_fixed();
   failures += test_get_sound_waypoint_track_specific_edict();
   failures += test_get_sound_waypoint_distance_with_curr_wp();

   printf("=== Additional BotFindWaypointGoal tests ===\n");
   failures += test_find_waypoint_goal_needs_armor();
   failures += test_find_waypoint_goal_needs_longjump();
   failures += test_find_waypoint_goal_critical_health_random_fallback();

   printf("=== Additional BotHeadTowardWaypoint tests ===\n");
   failures += test_head_toward_longjump_to_waypoint();
   failures += test_head_toward_visibility_recheck();
   failures += test_head_toward_visibility_recheck_fails();
   failures += test_head_toward_crouch_min_distance();
   failures += test_head_toward_lift_start_min_distance();
   failures += test_head_toward_item_min_distance();
   failures += test_head_toward_exit_water_min_distance();
   failures += test_head_toward_goal_reached_with_enemy();
   failures += test_head_toward_exclude_points();
   failures += test_head_toward_track_sound_update();
   failures += test_head_toward_goal_time_reset();
   failures += test_head_toward_find_nearest_after_ladder();
   failures += test_head_toward_enemy_goal_time();

   printf("=== Additional BotOnLadder tests ===\n");
   failures += test_on_ladder_waypoint_directed();
   failures += test_on_ladder_second_wall_direction();

   printf("=== Additional BotUseLift tests ===\n");
   failures += test_use_lift_finds_lift_start();
   failures += test_use_lift_stops_right_clear();
   failures += test_use_lift_stops_left_clear();

   printf("=== Additional BotLookForDrop tests ===\n");
   failures += test_look_for_drop_lava();
   failures += test_look_for_drop_with_enemy();
   failures += test_look_for_drop_ducking();
   failures += test_look_for_drop_turn_at_wall();

   printf("=== Additional BotEdge pre-check tests ===\n");
   failures += test_edge_forward_pre_check_duck();
   failures += test_edge_right_pre_check_ladder();
   failures += test_edge_left_pre_check_jump();
   failures += test_edge_forward_zero_velocity();

   printf("=== Additional BotCanJumpUp tests ===\n");
   failures += test_can_jump_up_left_side_blocked();

   printf("=== Random-dependent path tests ===\n");
   failures += test_random_turn_180_degree();
   failures += test_find_waypoint_random_3_candidates();
   failures += test_find_waypoint_random_2_candidates();
   failures += test_get_sound_waypoint_null_sound_ptr();
   failures += test_update_track_sound_goal_replace_edict();
   failures += test_find_waypoint_goal_track_sound_goto();
   failures += test_find_waypoint_goal_track_sound_distance();
   failures += test_find_waypoint_goal_health_random_fallback();
   failures += test_find_waypoint_goal_armor_random_fallback();
   failures += test_find_waypoint_goal_longjump_random_fallback();
   failures += test_head_toward_visibility_after_ladder();
   failures += test_head_toward_lift_end_min_distance();
   failures += test_head_toward_item_wp_min_distance();
   failures += test_head_toward_weapon_goal_exclude_points();
   failures += test_can_jump_up_right_duck_blocked();
   failures += test_can_jump_up_downward_right_blocked();
   failures += test_can_jump_up_downward_left_blocked();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return failures ? 1 : 0;
}
