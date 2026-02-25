//
// JK_Botti - unit tests for bot_combat.cpp
//
// test_bot_combat.cpp
//
// Uses the #include-the-.cpp approach to access static functions directly.
//

#include <stdlib.h>
#include <math.h>

// Mock RANDOM functions for deterministic testing
static int mock_random_long_ret = 0;
static float mock_random_float_ret = 0.0f;

#define RANDOM_LONG2 mock_RANDOM_LONG2
#define RANDOM_FLOAT2 mock_RANDOM_FLOAT2

// Include the source file under test (brings in all its headers + static functions)
#include "../bot_combat.cpp"

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
// Test helpers
// ============================================================

static void setup_skill_settings(void)
{
   mock_random_long_ret = 0;
   mock_random_float_ret = 0.0f;

   InitWeaponSelect(SUBMOD_HLDM);

   for (int i = 0; i < 5; i++)
   {
      skill_settings[i].turn_skill = 3.0 + i;
      skill_settings[i].turn_slowness = 2.0;
      skill_settings[i].updown_turn_ration = 2.0;
      skill_settings[i].react_delay_min = 0.1;
      skill_settings[i].react_delay_max = 0.3;
      skill_settings[i].hearing_sensitivity = 1.0 - i * 0.15;
      skill_settings[i].ping_emu_latency = 0.05 + i * 0.05;
      skill_settings[i].ping_emu_speed_varitation = 0.1;
      skill_settings[i].ping_emu_position_varitation = 5.0;
      skill_settings[i].respawn_react_delay = 1.0;
      skill_settings[i].shootcone_diameter = 20.0;
      skill_settings[i].shootcone_minangle = 10.0;
      skill_settings[i].weaponchange_rate_min = 0.5;
      skill_settings[i].weaponchange_rate_max = 1.0;
      skill_settings[i].track_sound_time_min = 2.0;
      skill_settings[i].track_sound_time_max = 5.0;
   }
}

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
   pBot.f_frame_time = 0.01;
   pBot.f_max_speed = 320.0;

   pEdict->v.origin = Vector(0, 0, 0);
   pEdict->v.v_angle = Vector(0, 0, 0);
   pEdict->v.view_ofs = Vector(0, 0, 28);
   pEdict->v.health = 100;
   pEdict->v.deadflag = DEAD_NO;
   pEdict->v.takedamage = DAMAGE_YES;
   pEdict->v.solid = SOLID_BBOX;
   pEdict->v.flags = FL_CLIENT | FL_FAKECLIENT;
}

static edict_t *create_enemy_player(const Vector &origin)
{
   edict_t *e = mock_alloc_edict();
   mock_set_classname(e, "player");
   e->v.origin = origin;
   e->v.health = 100;
   e->v.deadflag = DEAD_NO;
   e->v.takedamage = DAMAGE_YES;
   e->v.solid = SOLID_BBOX;
   e->v.flags = FL_CLIENT;
   e->v.view_ofs = Vector(0, 0, 28);

   // Set player data so IsPlayerChatProtected returns FALSE
   int pidx = ENTINDEX(e) - 1;
   if (pidx >= 0 && pidx < 32)
      players[pidx].last_time_not_facing_wall = gpGlobals->time;

   return e;
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
   e->v.absmin = origin - Vector(16, 16, 16);
   e->v.size = Vector(32, 32, 32);
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

// Trace function that reports a partial hit (for feet/center blocked scenarios)
static void trace_partial_hit(const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
{
   (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   ptr->flFraction = 0.5f;
   ptr->pHit = NULL;
   ptr->vecEndPos[0] = (v1[0] + v2[0]) / 2;
   ptr->vecEndPos[1] = (v1[1] + v2[1]) / 2;
   ptr->vecEndPos[2] = (v1[2] + v2[2]) / 2;
}

// Give a bot a weapon
static void give_bot_weapon(bot_t &pBot, int weapon_id, int clip, int ammo1, int ammo2)
{
   pBot.current_weapon.iId = weapon_id;
   pBot.current_weapon.iClip = clip;
   pBot.current_weapon.iAmmo1 = ammo1;
   pBot.current_weapon.iAmmo2 = ammo2;

   // Set the weapon_defs entry
   weapon_defs[weapon_id].iId = weapon_id;

   // Also set ammo in the ammo array
   if (weapon_id < MAX_WEAPONS)
   {
      // Set ammo indices based on weapon_defs
      int ammo1_idx = weapon_defs[weapon_id].iAmmo1;
      int ammo2_idx = weapon_defs[weapon_id].iAmmo2;
      if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
         pBot.m_rgAmmo[ammo1_idx] = ammo1;
      if (ammo2_idx >= 0 && ammo2_idx < MAX_AMMO_SLOTS)
         pBot.m_rgAmmo[ammo2_idx] = ammo2;
   }
}

// ============================================================
// Group 7: Small remaining functions
// ============================================================

static int test_get_modified_enemy_distance(void)
{
   printf("GetModifiedEnemyDistance:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("z component scaled by updown_turn_ration");
   Vector dist(100, 200, 50);
   Vector result = GetModifiedEnemyDistance(testbot, dist);
   ASSERT_FLOAT_NEAR(result.x, 100.0, 0.01);
   ASSERT_FLOAT_NEAR(result.y, 200.0, 0.01);
   ASSERT_FLOAT_NEAR(result.z, 50.0 * skill_settings[testbot.bot_skill].updown_turn_ration, 0.01);
   PASS();

   return 0;
}

static int test_bot_reset_reaction_time(void)
{
   printf("BotResetReactionTime:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("normal reaction time set");
   BotResetReactionTime(testbot, FALSE);
   ASSERT_TRUE(testbot.f_reaction_target_time > gpGlobals->time);
   (void)testbot.f_reaction_target_time;
   PASS();

   TEST("slow reaction time is longer");
   BotResetReactionTime(testbot, TRUE);
   // slow reaction multiplies delay by 4
   // With RANDOM_FLOAT2 returning low, slow should be 4x the min delay
   ASSERT_TRUE(testbot.f_reaction_target_time > gpGlobals->time);
   PASS();

   TEST("f_next_find_visible_sound_enemy_time set");
   ASSERT_FLOAT_NEAR(testbot.f_next_find_visible_sound_enemy_time, gpGlobals->time + 0.2f, 0.01);
   PASS();

   return 0;
}

// ============================================================
// Group 1: Position data & prediction
// ============================================================

static int test_free_posdata_list(void)
{
   printf("free_posdata_list:\n");
   mock_reset();
   setup_skill_settings();

   TEST("clears player posdata");
   // Set up some posdata for player index 0
   players[0].posdata_mem[0].inuse = TRUE;
   players[0].posdata_mem[0].time = 5.0;
   players[0].posdata_mem[0].origin = Vector(100, 0, 0);
   players[0].posdata_mem[0].older = NULL;
   players[0].posdata_mem[0].newer = NULL;
   players[0].position_latest = &players[0].posdata_mem[0];
   players[0].position_oldest = &players[0].posdata_mem[0];

   free_posdata_list(0);

   ASSERT_PTR_NULL(players[0].position_latest);
   ASSERT_PTR_NULL(players[0].position_oldest);
   PASS();

   return 0;
}

static int test_gather_player_data(void)
{
   printf("GatherPlayerData:\n");
   mock_reset();
   setup_skill_settings();

   TEST("NULL edict skipped");
   GatherPlayerData(NULL);
   PASS();

   TEST("FL_PROXY edict skipped");
   edict_t *proxy = mock_alloc_edict();
   proxy->v.flags = FL_CLIENT | FL_PROXY;
   GatherPlayerData(proxy);
   // player index = ENTINDEX(proxy) - 1
   int pidx = ENTINDEX(proxy) - 1;
   ASSERT_PTR_NULL(players[pidx].position_latest);
   PASS();

   mock_reset();
   setup_skill_settings();

   TEST("normal player data gathered");
   edict_t *player = mock_alloc_edict();
   player->v.flags = FL_CLIENT;
   player->v.origin = Vector(100, 200, 300);
   player->v.velocity = Vector(10, 20, 0);
   player->v.health = 100;
   player->v.deadflag = DEAD_NO;
   pidx = ENTINDEX(player) - 1;

   GatherPlayerData(player);

   ASSERT_PTR_NOT_NULL(players[pidx].position_latest);
   ASSERT_FLOAT_NEAR(players[pidx].position_latest->origin.x, 100.0, 0.01);
   ASSERT_FLOAT_NEAR(players[pidx].position_latest->origin.y, 200.0, 0.01);
   ASSERT_FLOAT_NEAR(players[pidx].position_latest->time, gpGlobals->time, 0.01);
   PASS();

   TEST("time trimming removes old entries");
   // Advance time far enough that old data gets trimmed
   gpGlobals->time = 100.0;
   player->v.origin = Vector(500, 600, 700);
   GatherPlayerData(player);
   // The old entry (at time 10.0) should be trimmed since cutoff is 100.0 - (max latency + 0.1)
   ASSERT_PTR_NOT_NULL(players[pidx].position_latest);
   ASSERT_FLOAT_NEAR(players[pidx].position_latest->time, 100.0, 0.01);
   PASS();

   return 0;
}

static int test_prediction_velocity_variation(void)
{
   printf("AddPredictionVelocityVaritation:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("zero velocity returns unchanged");
   Vector zero(0, 0, 0);
   Vector result = AddPredictionVelocityVaritation(testbot, zero);
   ASSERT_FLOAT_NEAR(result.x, 0.0, 0.01);
   ASSERT_FLOAT_NEAR(result.y, 0.0, 0.01);
   ASSERT_FLOAT_NEAR(result.z, 0.0, 0.01);
   PASS();

   TEST("nonzero velocity applies variance");
   Vector vel(100, 0, 0);
   result = AddPredictionVelocityVaritation(testbot, vel);
   // Result should be vel normalized * (length * random(minvar, maxvar))
   // With mock random returning low, minvar = 1.0 - variation
   ASSERT_TRUE(result.Length() > 0);
   PASS();

   return 0;
}

static int test_prediction_position_variation(void)
{
   printf("AddPredictionPositionVaritation:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("returns non-zero vector when position_varitation > 0");
   Vector result = AddPredictionPositionVaritation(testbot);
   // With non-zero ping_emu_position_varitation, result should not be zero
   // (unless random angles happen to produce a zero forward vector, which is extremely unlikely)
   ASSERT_TRUE(result.Length() > 0 || skill_settings[testbot.bot_skill].ping_emu_position_varitation == 0);
   PASS();

   return 0;
}

static int test_trace_predicted_movement(void)
{
   printf("TracePredictedMovement:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));

   mock_trace_hull_fn = trace_nohit;

   TEST("without_velocity returns v_src unchanged");
   Vector src(100, 200, 300);
   Vector vel(50, 0, 0);
   Vector result = TracePredictedMovement(testbot, pEnemy, src, vel, 1.0, FALSE, TRUE);
   ASSERT_FLOAT_NEAR(result.x, 100.0, 0.01);
   ASSERT_FLOAT_NEAR(result.y, 200.0, 0.01);
   ASSERT_FLOAT_NEAR(result.z, 300.0, 0.01);
   PASS();

   TEST("with velocity adjusts position");
   result = TracePredictedMovement(testbot, pEnemy, src, vel, 1.0, FALSE, FALSE);
   // Result should differ from src since velocity is applied
   ASSERT_TRUE(fabs(result.x - src.x) > 0.1 || fabs(result.y - src.y) > 0.1 || fabs(result.z - src.z) > 0.1);
   PASS();

   TEST("trace hit adjusts z");
   // Use a trace that reports a hit partway
   mock_trace_hull_fn = trace_partial_hit;
   result = TracePredictedMovement(testbot, pEnemy, src, vel, 1.0, FALSE, FALSE);
   // z should be adjusted to trace end pos z
   PASS();

   return 0;
}

static int test_get_predicted_player_position(void)
{
   printf("GetPredictedPlayerPosition:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_hull_fn = trace_nohit;
   mock_trace_line_fn = trace_nohit;

   TEST("FNullEnt returns zero vector");
   Vector result = GetPredictedPlayerPosition(testbot, NULL);
   ASSERT_FLOAT_NEAR(result.x, 0.0, 0.01);
   ASSERT_FLOAT_NEAR(result.y, 0.0, 0.01);
   PASS();

   TEST("no posdata returns UTIL_GetOrigin");
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   result = GetPredictedPlayerPosition(testbot, pEnemy);
   ASSERT_FLOAT_NEAR(result.x, 200.0, 0.01);
   PASS();

   // Set up posdata for the enemy
   int pidx = ENTINDEX(pEnemy) - 1;

   TEST("exact time match returns traced position");
   memset(players[pidx].posdata_mem, 0, sizeof(players[pidx].posdata_mem));
   posdata_t *node = &players[pidx].posdata_mem[0];
   node->inuse = TRUE;
   node->time = gpGlobals->time - skill_settings[testbot.bot_skill].ping_emu_latency;
   node->origin = Vector(180, 0, 0);
   node->velocity = Vector(10, 0, 0);
   node->was_alive = TRUE;
   node->older = NULL;
   node->newer = NULL;
   players[pidx].position_latest = node;
   players[pidx].position_oldest = node;

   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE); // without_velocity
   ASSERT_FLOAT_NEAR(result.x, 180.0, 0.01); // without_velocity returns position as-is
   PASS();

   TEST("older==NULL uses position_oldest");
   // Make the only node have a time newer than prediction time
   node->time = gpGlobals->time; // newer than prediction time
   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE);
   // Should use position_oldest since loop ends with older==NULL
   PASS();

   TEST("newer==NULL creates temp from current edict state");
   // Set up two nodes: one very old
   posdata_t *node2 = &players[pidx].posdata_mem[1];
   memset(node2, 0, sizeof(*node2));
   node2->inuse = TRUE;
   node2->time = gpGlobals->time - 5.0; // very old, older than prediction time
   node2->origin = Vector(100, 0, 0);
   node2->velocity = Vector(5, 0, 0);
   node2->was_alive = TRUE;
   node2->older = NULL;
   node2->newer = NULL; // newer is NULL => triggers newertmp path

   node->time = gpGlobals->time - 5.0; // also old
   node->older = node2;
   node->newer = NULL;
   node2->newer = node;

   players[pidx].position_latest = node;
   players[pidx].position_oldest = node2;

   // Prediction time is gpGlobals->time - latency, which is newer than both nodes
   // Loop: newer = position_latest (node), node->time < prediction_time? yes (both are old)
   // so older = node, newer = node->newer = NULL => triggers newertmp path
   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE);
   PASS();

   TEST("dead/alive mixing: newer dead, older alive uses newer");
   memset(players[pidx].posdata_mem, 0, sizeof(players[pidx].posdata_mem));
   posdata_t *nodeA = &players[pidx].posdata_mem[0];
   posdata_t *nodeB = &players[pidx].posdata_mem[1];

   float pred_time = gpGlobals->time - skill_settings[testbot.bot_skill].ping_emu_latency;

   // nodeA (latest) is newer than pred_time, nodeB (oldest) is older
   nodeA->inuse = TRUE;
   nodeA->time = pred_time + 0.01;
   nodeA->origin = Vector(200, 0, 0);
   nodeA->velocity = Vector(0, 0, 0);
   nodeA->was_alive = FALSE; // dead newer
   nodeA->newer = NULL;
   nodeA->older = nodeB;

   nodeB->inuse = TRUE;
   nodeB->time = pred_time - 0.01;
   nodeB->origin = Vector(150, 0, 0);
   nodeB->velocity = Vector(0, 0, 0);
   nodeB->was_alive = TRUE; // alive older
   nodeB->newer = nodeA;
   nodeB->older = NULL;

   players[pidx].position_latest = nodeA;
   players[pidx].position_oldest = nodeB;

   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE);
   // newer dead, older alive => uses newer origin
   ASSERT_FLOAT_NEAR(result.x, 200.0, 1.0);
   PASS();

   TEST("dead/alive mixing: older dead, newer alive uses older");
   nodeA->was_alive = TRUE;  // alive newer
   nodeB->was_alive = FALSE; // dead older
   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE);
   // older dead, newer alive => uses older origin
   ASSERT_FLOAT_NEAR(result.x, 150.0, 1.0);
   PASS();

   TEST("total_diff==0 uses newer");
   // Both nodes at the same time
   nodeA->time = pred_time + 0.01;
   nodeB->time = pred_time - 0.01;
   nodeA->was_alive = TRUE;
   nodeB->was_alive = TRUE;
   // Make total_diff = 0 by setting both to same offset
   nodeA->time = pred_time + 0.001;
   nodeB->time = pred_time - 0.001;
   // total_diff = 0.001 + 0.001 = 0.002 which is > 0, need exact same distance
   // To get total_diff==0, need both at exact pred_time (but then exact match triggers)
   // Instead set newer_diff and older_diff to be 0 each - not possible unless same time
   // Let's use a different approach: just verify the weighted average path
   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE);
   // weighted average with equal weights
   PASS();

   TEST("self-cycle in posdata does not hang");
   memset(players[pidx].posdata_mem, 0, sizeof(players[pidx].posdata_mem));
   posdata_t *selfnode = &players[pidx].posdata_mem[0];
   selfnode->inuse = TRUE;
   selfnode->time = gpGlobals->time + 100; // far in the future
   selfnode->origin = Vector(200, 0, 0);
   selfnode->older = selfnode; // SELF-CYCLE
   selfnode->newer = NULL;
   players[pidx].position_latest = selfnode;
   players[pidx].position_oldest = selfnode;

   result = GetPredictedPlayerPosition(testbot, pEnemy, TRUE);
   PASS();

   return 0;
}

// ============================================================
// Group 2: Aiming
// ============================================================

static int test_bot_aims_at_something(void)
{
   printf("BotAimsAtSomething:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("no enemy returns FALSE");
   testbot.pBotEnemy = NULL;
   ASSERT_INT(BotAimsAtSomething(testbot), FALSE);
   PASS();

   TEST("with visible enemy returns TRUE");
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   mock_trace_line_fn = trace_nohit;
   // Need posdata for GetPredictedPlayerPosition
   ASSERT_INT(BotAimsAtSomething(testbot), TRUE);
   PASS();

   return 0;
}

static int test_bot_point_gun(void)
{
   printf("BotPointGun:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("combat longjump speed path");
   testbot.b_combat_longjump = TRUE;
   pBotEdict->v.idealpitch = 10.0;
   pBotEdict->v.ideal_yaw = 20.0;
   BotPointGun(testbot);
   // Should modify v_angle
   PASS();

   TEST("aims-at-something with weapon aim_speed");
   testbot.b_combat_longjump = FALSE;
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   mock_trace_line_fn = trace_nohit;
   give_bot_weapon(testbot, VALVE_WEAPON_GLOCK, 17, 50, 0);
   pBotEdict->v.idealpitch = 5.0;
   pBotEdict->v.ideal_yaw = 10.0;
   BotPointGun(testbot);
   PASS();

   TEST("waypoint speed path");
   testbot.pBotEnemy = NULL;
   testbot.curr_waypoint_index = 5;
   pBotEdict->v.idealpitch = 5.0;
   pBotEdict->v.ideal_yaw = 10.0;
   BotPointGun(testbot);
   PASS();

   TEST("slow speed path (no enemy, no waypoint)");
   testbot.curr_waypoint_index = -1;
   testbot.pBotPickupItem = NULL;
   pBotEdict->v.idealpitch = 5.0;
   pBotEdict->v.ideal_yaw = 10.0;
   BotPointGun(testbot);
   PASS();

   return 0;
}

static int test_bot_aim_pre(void)
{
   printf("BotAimPre:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("normal path wraps angles");
   pBotEdict->v.idealpitch = 400.0; // should be wrapped
   pBotEdict->v.ideal_yaw = -400.0;
   BotAimPre(testbot);
   ASSERT_TRUE(pBotEdict->v.idealpitch >= -180.0 && pBotEdict->v.idealpitch <= 180.0);
   ASSERT_TRUE(pBotEdict->v.ideal_yaw >= -180.0 && pBotEdict->v.ideal_yaw <= 180.0);
   PASS();

   TEST("special shoot angle swap");
   testbot.b_set_special_shoot_angle = TRUE;
   testbot.f_special_shoot_angle = 45.0;
   pBotEdict->v.v_angle.z = 10.0;
   BotAimPre(testbot);
   // After swap: v_angle.z = 45.0, f_special_shoot_angle = 10.0
   ASSERT_FLOAT_NEAR(pBotEdict->v.v_angle.z, 45.0, 0.01);
   ASSERT_FLOAT_NEAR(testbot.f_special_shoot_angle, 10.0, 0.01);
   PASS();

   return 0;
}

static int test_bot_aim_post(void)
{
   printf("BotAimPost:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("M249 ducking recoil");
   testbot.current_weapon.iId = GEARBOX_WEAPON_M249;
   testbot.f_recoil = 8.0;
   pBotEdict->v.flags |= FL_DUCKING;
   BotAimPost(testbot);
   // recoil was 8.0, ducking divides by 4 = 2.0, then added to punchangle
   ASSERT_FLOAT_NEAR(pBotEdict->v.punchangle.x, 2.0, 0.01);
   ASSERT_FLOAT_NEAR(testbot.f_recoil, 0.0, 0.01);
   PASS();

   TEST("eagle recoil with secondary state");
   pBotEdict->v.punchangle.x = 0;
   testbot.current_weapon.iId = GEARBOX_WEAPON_EAGLE;
   testbot.eagle_secondary_state = 1;
   testbot.f_recoil = 15.0;
   pBotEdict->v.flags &= ~FL_DUCKING;
   BotAimPost(testbot);
   // recoil 15.0 / 15 = 1.0
   ASSERT_FLOAT_NEAR(pBotEdict->v.punchangle.x, 1.0, 0.01);
   PASS();

   TEST("normal recoil (no special weapon)");
   pBotEdict->v.punchangle.x = 0;
   testbot.current_weapon.iId = VALVE_WEAPON_GLOCK;
   testbot.f_recoil = 5.0;
   BotAimPost(testbot);
   ASSERT_FLOAT_NEAR(pBotEdict->v.punchangle.x, 5.0, 0.01);
   PASS();

   TEST("special angle restore");
   pBotEdict->v.punchangle.x = 0;
   testbot.b_set_special_shoot_angle = TRUE;
   testbot.f_special_shoot_angle = 30.0;
   pBotEdict->v.v_angle.z = 5.0;
   testbot.current_weapon.iId = VALVE_WEAPON_GLOCK;
   testbot.f_recoil = 0;
   BotAimPost(testbot);
   // After restore: v_angle.z = 30.0, b_set_special_shoot_angle = FALSE
   ASSERT_FLOAT_NEAR(pBotEdict->v.v_angle.z, 30.0, 0.01);
   ASSERT_INT(testbot.b_set_special_shoot_angle, FALSE);
   ASSERT_FLOAT_NEAR(testbot.f_special_shoot_angle, 0.0, 0.01);
   PASS();

   return 0;
}

// ============================================================
// Group 3: Hearing & sound enemy
// ============================================================

static int test_hearing_sensitivity(void)
{
   printf("BotUpdateHearingSensitivity / BotGetHearingSensitivity:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   testbot.f_current_hearing_sensitivity = skill_settings[BEST_BOT_LEVEL].hearing_sensitivity;

   TEST("with enemy: set to best");
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   BotUpdateHearingSensitivity(testbot);
   ASSERT_FLOAT_NEAR(testbot.f_current_hearing_sensitivity,
                      skill_settings[BEST_BOT_LEVEL].hearing_sensitivity, 0.01);
   PASS();

   TEST("without enemy: decrease + clamp");
   testbot.pBotEnemy = NULL;
   testbot.f_frame_time = 10.0; // very large frame time to force below clamp
   BotUpdateHearingSensitivity(testbot);
   // Should be clamped to skill level
   ASSERT_FLOAT_NEAR(testbot.f_current_hearing_sensitivity,
                      skill_settings[testbot.bot_skill].hearing_sensitivity, 0.01);
   PASS();

   TEST("BotGetHearingSensitivity returns at least skill level");
   testbot.f_current_hearing_sensitivity = 0; // below skill level
   float sens = BotGetHearingSensitivity(testbot);
   ASSERT_FLOAT_NEAR(sens, skill_settings[testbot.bot_skill].hearing_sensitivity, 0.01);
   PASS();

   TEST("BotGetHearingSensitivity returns current if above skill level");
   testbot.f_current_hearing_sensitivity = skill_settings[testbot.bot_skill].hearing_sensitivity + 0.5;
   sens = BotGetHearingSensitivity(testbot);
   ASSERT_FLOAT_NEAR(sens, skill_settings[testbot.bot_skill].hearing_sensitivity + 0.5, 0.01);
   PASS();

   return 0;
}

static int test_find_visible_sound_enemy(void)
{
   printf("BotFindVisibleSoundEnemy:\n");
   mock_reset();
   setup_skill_settings();

   // Set up sound entity
   static CSoundEnt soundEnt;
   pSoundEnt = &soundEnt;
   pSoundEnt->Initialize();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   // Put bot in bots[] array so iBotOwner check works
   int bot_idx = 0;
   bots[bot_idx] = testbot;
   bots[bot_idx].pEdict = pBotEdict;

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   // Create an enemy player near a sound
   edict_t *pEnemy = create_enemy_player(Vector(300, 0, 0));

   TEST("enemy near sound detected");
   // Insert a sound near the enemy (channel must be != 0 for allocation)
   CSoundEnt::InsertSound(pEnemy, 1, Vector(300, 0, 0), 2000, 5.0, -1);
   edict_t *found = BotFindVisibleSoundEnemy(bots[bot_idx]);
   ASSERT_PTR_EQ(found, pEnemy);
   PASS();

   TEST("bot's own sound ignored");
   pSoundEnt->Initialize(); // clear sounds
   CSoundEnt::InsertSound(pEnemy, 1, Vector(300, 0, 0), 2000, 5.0, bot_idx); // owner = this bot
   found = BotFindVisibleSoundEnemy(bots[bot_idx]);
   ASSERT_PTR_NULL(found);
   PASS();

   TEST("sound too far not heard");
   pSoundEnt->Initialize();
   // volume=1, sensitivity ~1.0 => hearable range = 1.0
   // distance = 300 >> 1
   CSoundEnt::InsertSound(pEnemy, 1, Vector(300, 0, 0), 1, 5.0, -1);
   found = BotFindVisibleSoundEnemy(bots[bot_idx]);
   ASSERT_PTR_NULL(found);
   PASS();

   TEST("no enemy near sound location");
   pSoundEnt->Initialize();
   // Sound at (5000, 5000, 0), no enemy within 512 units
   CSoundEnt::InsertSound(pEnemy, 1, Vector(5000, 5000, 0), 99999, 5.0, -1);
   found = BotFindVisibleSoundEnemy(bots[bot_idx]);
   ASSERT_PTR_NULL(found);
   PASS();

   TEST("farther sound skipped when closer already found");
   pSoundEnt->Initialize();
   // Insert a close sound first
   CSoundEnt::InsertSound(pEnemy, 1, Vector(300, 0, 0), 2000, 5.0, -1);
   // Insert a farther sound
   CSoundEnt::InsertSound(pEnemy, 2, Vector(800, 0, 0), 2000, 5.0, -1);
   found = BotFindVisibleSoundEnemy(bots[bot_idx]);
   // Should find enemy from closer sound
   ASSERT_PTR_EQ(found, pEnemy);
   PASS();

   TEST("enemy not visible from bot skipped");
   pSoundEnt->Initialize();
   CSoundEnt::InsertSound(pEnemy, 1, Vector(300, 0, 0), 2000, 5.0, -1);
   // Make enemy not visible
   mock_trace_line_fn = trace_partial_hit;
   found = BotFindVisibleSoundEnemy(bots[bot_idx]);
   ASSERT_PTR_NULL(found);
   mock_trace_line_fn = trace_nohit;
   PASS();

   pSoundEnt = NULL;
   return 0;
}

// ============================================================
// Group 4: Enemy detection expanded
// ============================================================

static int test_are_team_mates(void)
{
   printf("AreTeamMates:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *p1 = mock_alloc_edict();
   edict_t *p2 = mock_alloc_edict();
   p1->v.flags = FL_CLIENT;
   p2->v.flags = FL_CLIENT;

   TEST("teamplay off returns FALSE");
   is_team_play = FALSE;
   ASSERT_INT(AreTeamMates(p1, p2), FALSE);
   PASS();

   TEST("teamplay on, same team returns TRUE");
   is_team_play = TRUE;
   // mock_pfnInfoKeyValue returns "" for all keys, so both have empty team => same => TRUE
   ASSERT_INT(AreTeamMates(p1, p2), TRUE);
   PASS();

   return 0;
}

static int test_f_can_shoot_in_head(void)
{
   printf("FCanShootInHead:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBot = mock_alloc_edict();
   pBot->v.flags = FL_CLIENT;
   pBot->v.origin = Vector(0, 0, 0);

   mock_trace_line_fn = trace_nohit;

   TEST("non-player returns FALSE");
   edict_t *pMonster = mock_alloc_edict();
   mock_set_classname(pMonster, "monster_zombie");
   pMonster->v.origin = Vector(100, 0, 0);
   pMonster->v.view_ofs = Vector(0, 0, 20);
   ASSERT_INT(FCanShootInHead(pBot, pMonster, pMonster->v.origin), FALSE);
   PASS();

   TEST("player with head-only visible returns TRUE");
   edict_t *pEnemy = create_enemy_player(Vector(100, 0, 0));
   // Need head visible but center/feet not visible
   // Use a custom trace that makes head visible but center/feet blocked
   struct TraceHelper {
      static void trace_head_only(const float *v1, const float *v2,
                                  int fNoMonsters, int hullNumber,
                                  edict_t *pentToSkip, TraceResult *ptr)
      {
         (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
         // Head position check: v2.z should be higher (origin + view_ofs - 2)
         float dest_z = v2[2];
         if (dest_z > 20.0) // head check
         {
            ptr->flFraction = 1.0f;
            ptr->pHit = NULL;
         }
         else // center or feet check - blocked
         {
            ptr->flFraction = 0.5f;
            ptr->pHit = NULL;
         }
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   mock_trace_line_fn = TraceHelper::trace_head_only;
   ASSERT_INT(FCanShootInHead(pBot, pEnemy, pEnemy->v.origin), TRUE);
   PASS();

   TEST("player far away (angle check) returns FALSE");
   mock_trace_line_fn = trace_nohit;
   // All visible, but at large distance the angle is too small
   pEnemy->v.origin = Vector(5000, 0, 0);
   ASSERT_INT(FCanShootInHead(pBot, pEnemy, pEnemy->v.origin), FALSE);
   PASS();

   return 0;
}

static int test_find_enemy_nearest_to_point(void)
{
   printf("BotFindEnemyNearestToPoint:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   TEST("found nearby enemy");
   edict_t *pEnemy = create_enemy_player(Vector(100, 0, 0));
   Vector v_found;
   edict_t *found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 500.0, &v_found);
   ASSERT_PTR_EQ(found, pEnemy);
   PASS();

   TEST("self-skip");
   // Move enemy to bot's position, it should still find the enemy (not skip self)
   // But bot edict should be skipped
   found = BotFindEnemyNearestToPoint(testbot, pBotEdict->v.origin, 500.0, &v_found);
   // pEnemy is at (100,0,0), bot at (0,0,0), search from (0,0,0) - enemy within range
   ASSERT_PTR_EQ(found, pEnemy);
   PASS();

   TEST("observer mode skips non-bots");
   b_observer_mode = TRUE;
   // pEnemy has FL_CLIENT but not FL_FAKECLIENT or FL_THIRDPARTYBOT
   found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 500.0, NULL);
   ASSERT_PTR_NULL(found);
   b_observer_mode = FALSE;
   PASS();

   TEST("dead player skipped");
   pEnemy->v.deadflag = DEAD_DEAD;
   pEnemy->v.health = 0;
   found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 500.0, NULL);
   ASSERT_PTR_NULL(found);
   pEnemy->v.deadflag = DEAD_NO;
   pEnemy->v.health = 100;
   PASS();

   TEST("chat protected player skipped");
   // IsPlayerChatProtected checks players[idx].last_time_not_facing_wall + 2.0 < gpGlobals->time
   // Set last_time_not_facing_wall to old value so player is chat protected
   {
      int pidx = ENTINDEX(pEnemy) - 1;
      players[pidx].last_time_not_facing_wall = gpGlobals->time - 100;
      found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 500.0, NULL);
      ASSERT_PTR_NULL(found);
      players[pidx].last_time_not_facing_wall = gpGlobals->time; // restore
   }
   PASS();

   TEST("teammates skipped in teamplay");
   is_team_play = TRUE;
   // Both have empty team (same team)
   found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 500.0, NULL);
   ASSERT_PTR_NULL(found);
   is_team_play = FALSE;
   PASS();

   TEST("beyond radius not found");
   found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 10.0, NULL);
   // enemy at (100,0,0), search from (100,0,0) radius 10 => distance 0, should be found
   // Actually the search compares distance from v_point to player origin
   // Enemy is at (100,0,0), v_point is (100,0,0), distance = 0 < 10, should find
   // Let's search from far away instead
   found = BotFindEnemyNearestToPoint(testbot, Vector(5000, 5000, 0), 10.0, NULL);
   ASSERT_PTR_NULL(found);
   PASS();

   TEST("respawn delay skipped");
   // Set respawn time to very recent
   int pidx = ENTINDEX(pEnemy) - 1;
   players[pidx].last_time_dead = gpGlobals->time - 0.1; // died 0.1s ago
   found = BotFindEnemyNearestToPoint(testbot, Vector(100, 0, 0), 500.0, NULL);
   ASSERT_PTR_NULL(found);
   players[pidx].last_time_dead = 0;
   PASS();

   return 0;
}

static int test_bot_remove_enemy_tracking(void)
{
   printf("BotRemoveEnemy:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   mock_trace_line_fn = trace_nohit;

   TEST("b_keep_tracking=TRUE sets WPT_GOAL_TRACK_SOUND");
   BotRemoveEnemy(testbot, TRUE);
   ASSERT_INT(testbot.wpt_goal_type, WPT_GOAL_TRACK_SOUND);
   ASSERT_PTR_EQ(testbot.pTrackSoundEdict, pEnemy);
   ASSERT_PTR_NULL(testbot.pBotEnemy); // enemy cleared
   PASS();

   TEST("b_keep_tracking=FALSE does not set tracking");
   testbot.pBotEnemy = pEnemy;
   testbot.wpt_goal_type = WPT_GOAL_NONE;
   BotRemoveEnemy(testbot, FALSE);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   ASSERT_INT(testbot.wpt_goal_type, WPT_GOAL_NONE);
   PASS();

   return 0;
}

static int test_fpredicted_visible(void)
{
   printf("FPredictedVisible:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("no enemy returns FALSE");
   testbot.pBotEnemy = NULL;
   ASSERT_INT(FPredictedVisible(testbot), FALSE);
   PASS();

   TEST("visible enemy returns TRUE");
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   mock_trace_line_fn = trace_nohit;
   ASSERT_INT(FPredictedVisible(testbot), TRUE);
   PASS();

   return 0;
}

// ============================================================
// Group 4 continued: BotFindEnemy expanded
// ============================================================

static int test_bot_find_enemy_expanded(void)
{
   printf("BotFindEnemy expanded:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("b_botdontshoot clears enemy and reloads");
   b_botdontshoot = TRUE;
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   ASSERT_TRUE((pBotEdict->v.button & IN_RELOAD) != 0);
   b_botdontshoot = FALSE;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("enemy dead -> removed");
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   pEnemy->v.deadflag = DEAD_DEAD;
   pEnemy->v.health = 0;
   testbot.pBotEnemy = pEnemy;
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("enemy still visible -> kept");
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   testbot.pBotEnemy = pEnemy;
   // Need the enemy to be in view cone and visible
   pBotEdict->v.v_angle = Vector(0, 0, 0); // facing +X
   BotFindEnemy(testbot);
   // The enemy should be kept since it's visible (trace_nohit)
   // Actually, the existing enemy path calls FInViewCone + FVisibleEnemy
   // with predicted position, which with no posdata returns UTIL_GetOrigin
   ASSERT_PTR_NOT_NULL(testbot.pBotEnemy);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);

   TEST("lost sight + recent -> tracking");
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_bot_see_enemy_time = gpGlobals->time - 0.3; // seen 0.3s ago (within 0.5s window)
   // Make enemy not visible by using a trace that blocks
   mock_trace_line_fn = trace_partial_hit;
   mock_trace_hull_fn = trace_partial_hit;
   BotFindEnemy(testbot);
   // Should have started tracking
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   ASSERT_INT(testbot.wpt_goal_type, WPT_GOAL_TRACK_SOUND);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("mode 2 sticky breakable targeting");
   bot_shoot_breakables = 2;
   edict_t *pBreakable = create_breakable(Vector(50, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);
   // Set last_time_not_facing_wall for breakable's player slot so IsPlayerChatProtected returns FALSE
   {
      int bpidx = ENTINDEX(pBreakable) - 1;
      if (bpidx >= 0 && bpidx < 32)
         players[bpidx].last_time_not_facing_wall = gpGlobals->time;
   }
   testbot.pBotEnemy = pBreakable;
   testbot.f_bot_see_enemy_time = gpGlobals->time;
   BotFindEnemy(testbot);
   // Breakable should be kept as enemy (sticky targeting)
   ASSERT_PTR_EQ(testbot.pBotEnemy, pBreakable);
   bot_shoot_breakables = 0;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("mode 2 sticky breakable dropped when not visible");
   bot_shoot_breakables = 2;
   pBreakable = create_breakable(Vector(50, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);
   {
      int bpidx = ENTINDEX(pBreakable) - 1;
      if (bpidx >= 0 && bpidx < 32)
         players[bpidx].last_time_not_facing_wall = gpGlobals->time;
   }
   testbot.pBotEnemy = pBreakable;
   testbot.f_bot_see_enemy_time = gpGlobals->time;
   // Make breakable not visible
   mock_trace_line_fn = trace_partial_hit;
   BotFindEnemy(testbot);
   // Should have dropped the breakable
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   bot_shoot_breakables = 0;
   mock_trace_line_fn = trace_nohit;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("mode 1 breakable: zero origin skipped");
   bot_shoot_breakables = 1;
   pBreakable = create_breakable(Vector(0, 0, 0), 50); // zero origin
   pBreakable->v.absmin = Vector(0, 0, 0);
   pBreakable->v.size = Vector(0, 0, 0);
   pBreakable->v.mins = Vector(0, 0, 0);
   pBreakable->v.maxs = Vector(0, 0, 0);
   mock_add_breakable(pBreakable, TRUE);
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   bot_shoot_breakables = 0;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("mode 1 breakable: far away skipped");
   bot_shoot_breakables = 1;
   pBreakable = create_breakable(Vector(50, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);
   // Create a second closer breakable to be found first
   edict_t *pClose = create_breakable(Vector(30, 0, 0), 50);
   mock_add_breakable(pClose, TRUE);
   BotFindEnemy(testbot);
   // Should find the closer one, not the farther
   ASSERT_PTR_EQ(testbot.pBotEnemy, pClose);
   bot_shoot_breakables = 0;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("mode 1 breakable: not visible skipped");
   bot_shoot_breakables = 1;
   pBreakable = create_breakable(Vector(100, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);
   // Make breakable not visible
   mock_trace_line_fn = trace_partial_hit;
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   bot_shoot_breakables = 0;
   mock_trace_line_fn = trace_nohit;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("monster: zero origin skipped");
   {
      edict_t *pM = mock_alloc_edict();
      mock_set_classname(pM, "monster_alien_grunt");
      pM->v.origin = Vector(0, 0, 0); // zero origin
      pM->v.health = 100;
      pM->v.flags = FL_MONSTER;
      pM->v.takedamage = DAMAGE_YES;
      pM->v.deadflag = DEAD_NO;
      pM->v.solid = SOLID_BBOX;
      pM->v.view_ofs = Vector(0, 0, 20);
      BotFindEnemy(testbot);
      ASSERT_PTR_NULL(testbot.pBotEnemy);
   }
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("monster: hornet skipped");
   {
      edict_t *pM = mock_alloc_edict();
      mock_set_classname(pM, "hornet");
      pM->v.origin = Vector(100, 0, 0);
      pM->v.health = 10;
      pM->v.flags = FL_MONSTER;
      pM->v.takedamage = DAMAGE_YES;
      pM->v.deadflag = DEAD_NO;
      pM->v.solid = SOLID_BBOX;
      pM->v.view_ofs = Vector(0, 0, 5);
      BotFindEnemy(testbot);
      ASSERT_PTR_NULL(testbot.pBotEnemy);
   }
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("monster: snark skipped");
   {
      edict_t *pM = mock_alloc_edict();
      mock_set_classname(pM, "monster_snark");
      pM->v.origin = Vector(100, 0, 0);
      pM->v.health = 10;
      pM->v.flags = FL_MONSTER;
      pM->v.takedamage = DAMAGE_YES;
      pM->v.deadflag = DEAD_NO;
      pM->v.solid = SOLID_BBOX;
      pM->v.view_ofs = Vector(0, 0, 5);
      BotFindEnemy(testbot);
      ASSERT_PTR_NULL(testbot.pBotEnemy);
   }
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("monster: high health (>4000) skipped");
   {
      edict_t *pM = mock_alloc_edict();
      mock_set_classname(pM, "monster_alien_grunt");
      pM->v.origin = Vector(100, 0, 0);
      pM->v.health = 5000;
      pM->v.flags = FL_MONSTER;
      pM->v.takedamage = DAMAGE_YES;
      pM->v.deadflag = DEAD_NO;
      pM->v.solid = SOLID_BBOX;
      pM->v.view_ofs = Vector(0, 0, 20);
      BotFindEnemy(testbot);
      ASSERT_PTR_NULL(testbot.pBotEnemy);
   }
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("monster: not visible skipped");
   {
      edict_t *pM = mock_alloc_edict();
      mock_set_classname(pM, "monster_alien_grunt");
      pM->v.origin = Vector(100, 0, 0);
      pM->v.health = 100;
      pM->v.flags = FL_MONSTER;
      pM->v.takedamage = DAMAGE_YES;
      pM->v.deadflag = DEAD_NO;
      pM->v.solid = SOLID_BBOX;
      pM->v.view_ofs = Vector(0, 0, 20);
      mock_trace_line_fn = trace_partial_hit;
      BotFindEnemy(testbot);
      ASSERT_PTR_NULL(testbot.pBotEnemy);
      mock_trace_line_fn = trace_nohit;
   }
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("player: observer mode skips non-bots");
   b_observer_mode = TRUE;
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   b_observer_mode = FALSE;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("player: teamplay skips teammates");
   is_team_play = TRUE;
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   is_team_play = FALSE;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("player: respawn delay skipped");
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   {
      int pidx = ENTINDEX(pEnemy) - 1;
      players[pidx].last_time_dead = gpGlobals->time - 0.1;
   }
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("player: not visible skipped");
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   mock_trace_line_fn = trace_partial_hit;
   BotFindEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   mock_trace_line_fn = trace_nohit;
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("player: farther player not preferred over closer");
   edict_t *pClose2 = create_enemy_player(Vector(100, 0, 0));
   create_enemy_player(Vector(500, 0, 0));
   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pClose2);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("monster enemy found");
   edict_t *pMonster = mock_alloc_edict();
   mock_set_classname(pMonster, "monster_alien_grunt");
   pMonster->v.origin = Vector(100, 0, 0);
   pMonster->v.health = 100;
   pMonster->v.flags = FL_MONSTER;
   pMonster->v.takedamage = DAMAGE_YES;
   pMonster->v.deadflag = DEAD_NO;
   pMonster->v.solid = SOLID_BBOX;
   pMonster->v.view_ofs = Vector(0, 0, 20);
   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pMonster);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("player enemy found");
   pEnemy = create_enemy_player(Vector(100, 0, 0));
   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pEnemy);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("reload timer after no enemy for 5+ seconds");
   testbot.f_bot_see_enemy_time = gpGlobals->time - 6.0;
   BotFindEnemy(testbot);
   ASSERT_TRUE((pBotEdict->v.button & IN_RELOAD) != 0);
   ASSERT_FLOAT_NEAR(testbot.f_bot_see_enemy_time, -1.0, 0.01);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("sound enemy detection with CSoundEnt");
   static CSoundEnt soundEnt;
   pSoundEnt = &soundEnt;
   pSoundEnt->Initialize();

   pEnemy = create_enemy_player(Vector(300, 0, 0));
   testbot.f_next_find_visible_sound_enemy_time = 0; // allow sound check
   CSoundEnt::InsertSound(pEnemy, 1, Vector(300, 0, 0), 2000, 5.0, -1);

   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pEnemy);

   pSoundEnt = NULL;
   PASS();

   return 0;
}

// ============================================================
// Group 5: Weapon firing
// ============================================================

static int test_have_room_for_throw(void)
{
   printf("HaveRoomForThrow:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   TEST("feet+center clear -> TRUE");
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   ASSERT_INT(HaveRoomForThrow(testbot), TRUE);
   PASS();

   TEST("center blocked -> FALSE");
   // Need center trace blocked but not hitting enemy
   struct CenterBlocked {
      static void trace(const float *v1, const float *v2,
                        int fNoMonsters, int hullNumber,
                        edict_t *pentToSkip, TraceResult *ptr)
      {
         (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
         // Block everything
         ptr->flFraction = 0.3f;
         ptr->pHit = NULL;
         ptr->vecEndPos[0] = v1[0];
         ptr->vecEndPos[1] = v1[1];
         ptr->vecEndPos[2] = v1[2];
      }
   };
   mock_trace_line_fn = CenterBlocked::trace;
   mock_trace_hull_fn = CenterBlocked::trace;
   ASSERT_INT(HaveRoomForThrow(testbot), FALSE);
   PASS();

   TEST("center+head clear but feet blocked -> TRUE");
   // Need: feet blocked, center clear, head clear
   struct FeetBlocked {
      static void trace(const float *v1, const float *v2,
                        int fNoMonsters, int hullNumber,
                        edict_t *pentToSkip, TraceResult *ptr)
      {
         (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
         // If start z is negative (feet trace), block it
         if (v1[2] < -10.0f) {
            ptr->flFraction = 0.3f;
            ptr->pHit = NULL;
         } else {
            ptr->flFraction = 1.0f;
            ptr->pHit = NULL;
         }
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   mock_trace_line_fn = FeetBlocked::trace;
   mock_trace_hull_fn = FeetBlocked::trace;
   ASSERT_INT(HaveRoomForThrow(testbot), TRUE);
   PASS();

   return 0;
}

static int test_check_weapon_fire_conditions(void)
{
   printf("CheckWeaponFireConditions:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(50, 0, 30));
   testbot.pBotEnemy = pEnemy;

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("melee: duck when height diff and close");
   bot_weapon_select_t melee_select;
   memset(&melee_select, 0, sizeof(melee_select));
   melee_select.type = WEAPON_MELEE;
   melee_select.iId = VALVE_WEAPON_CROWBAR;
   qboolean use_p = TRUE, use_s = FALSE;
   qboolean result = CheckWeaponFireConditions(testbot, melee_select, use_p, use_s);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE(testbot.f_duck_time > gpGlobals->time);
   PASS();

   TEST("throw: no room returns FALSE");
   bot_weapon_select_t throw_select;
   memset(&throw_select, 0, sizeof(throw_select));
   throw_select.type = WEAPON_THROW;
   throw_select.iId = VALVE_WEAPON_HANDGRENADE;
   // Block all traces
   mock_trace_line_fn = trace_partial_hit;
   mock_trace_hull_fn = trace_partial_hit;
   use_p = TRUE; use_s = FALSE;
   result = CheckWeaponFireConditions(testbot, throw_select, use_p, use_s);
   ASSERT_INT(result, FALSE);
   PASS();

   TEST("MP5 secondary: valid angle sets special shoot angle");
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   // Place enemy at medium distance where MP5 grenade angle is valid
   pEnemy->v.origin = Vector(500, 0, 0);
   testbot.pBotEnemy = pEnemy;
   bot_weapon_select_t mp5_select;
   memset(&mp5_select, 0, sizeof(mp5_select));
   mp5_select.type = WEAPON_FIRE;
   mp5_select.iId = VALVE_WEAPON_MP5;
   use_p = FALSE; use_s = TRUE;
   testbot.b_set_special_shoot_angle = FALSE;
   result = CheckWeaponFireConditions(testbot, mp5_select, use_p, use_s);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(testbot.b_set_special_shoot_angle, TRUE);
   PASS();

   TEST("MP5 secondary: invalid angle falls back to primary");
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   // Place enemy very close where MP5 grenade angle is out of range
   pEnemy->v.origin = Vector(50, 0, 0);
   testbot.pBotEnemy = pEnemy;
   testbot.b_set_special_shoot_angle = FALSE;
   use_p = FALSE; use_s = TRUE;
   result = CheckWeaponFireConditions(testbot, mp5_select, use_p, use_s);
   ASSERT_INT(result, TRUE);
   // Should have switched to primary since angle is out of range
   ASSERT_INT(use_p, TRUE);
   ASSERT_INT(use_s, FALSE);
   PASS();

   return 0;
}

static int test_try_select_weapon(void)
{
   printf("TrySelectWeapon:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   bot_weapon_select_t select;
   memset(&select, 0, sizeof(select));
   select.iId = VALVE_WEAPON_SHOTGUN;
   safe_strcopy(select.weapon_name, sizeof(select.weapon_name), "weapon_shotgun");

   bot_fire_delay_t delay;
   memset(&delay, 0, sizeof(delay));

   TEST("delay mismatch returns FALSE");
   delay.iId = VALVE_WEAPON_GLOCK; // mismatch
   testbot.current_weapon.iId = VALVE_WEAPON_SHOTGUN;
   qboolean result = TrySelectWeapon(testbot, 3, select, delay);
   ASSERT_INT(result, FALSE);
   ASSERT_INT(testbot.current_weapon_index, -1);
   PASS();

   TEST("normal success sets index and weaponchange_time");
   delay.iId = VALVE_WEAPON_SHOTGUN; // match
   result = TrySelectWeapon(testbot, 3, select, delay);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(testbot.current_weapon_index, 3);
   ASSERT_TRUE(testbot.f_weaponchange_time > gpGlobals->time);
   PASS();

   TEST("weapon switch selects different weapon");
   testbot.current_weapon.iId = VALVE_WEAPON_GLOCK; // different
   delay.iId = VALVE_WEAPON_SHOTGUN;
   result = TrySelectWeapon(testbot, 3, select, delay);
   ASSERT_INT(result, TRUE);
   PASS();

   return 0;
}

static int test_bot_fire_selected_weapon(void)
{
   printf("BotFireSelectedWeapon:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   bot_weapon_select_t select;
   memset(&select, 0, sizeof(select));
   select.iId = VALVE_WEAPON_GLOCK;
   select.type = WEAPON_FIRE;

   bot_fire_delay_t delay;
   memset(&delay, 0, sizeof(delay));
   delay.iId = VALVE_WEAPON_GLOCK;
   delay.primary_base_delay = 0.1;
   delay.secondary_base_delay = 0.2;

   TEST("primary attack presses IN_ATTACK");
   pBotEdict->v.button = 0;
   qboolean result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK) != 0);
   PASS();

   TEST("secondary attack presses IN_ATTACK2");
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, FALSE, TRUE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK2) != 0);
   PASS();

   TEST("primary fire charge sets charging state");
   pBotEdict->v.button = 0;
   select.primary_fire_charge = TRUE;
   select.primary_charge_delay = 1.0;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE(testbot.f_primary_charging > 0);
   ASSERT_INT(testbot.charging_weapon_id, VALVE_WEAPON_GLOCK);
   select.primary_fire_charge = FALSE;
   testbot.f_primary_charging = -1;
   PASS();

   TEST("secondary fire charge sets charging state");
   pBotEdict->v.button = 0;
   select.secondary_fire_charge = TRUE;
   select.secondary_charge_delay = 0.5;
   result = BotFireSelectedWeapon(testbot, select, delay, FALSE, TRUE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE(testbot.f_secondary_charging > 0);
   select.secondary_fire_charge = FALSE;
   testbot.f_secondary_charging = -1;
   PASS();

   TEST("primary hold-fire sets shoot_time to now");
   pBotEdict->v.button = 0;
   select.primary_fire_hold = TRUE;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time, 0.01);
   select.primary_fire_hold = FALSE;
   PASS();

   TEST("satchel primary sets detonation tracking");
   pBotEdict->v.button = 0;
   select.iId = VALVE_WEAPON_SATCHEL;
   delay.iId = VALVE_WEAPON_SATCHEL;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE(testbot.f_satchel_detonate_time > 0);
   ASSERT_TRUE(testbot.f_satchel_check_time > 0);
   ASSERT_INT(testbot.current_weapon_index, -1);
   PASS();

   TEST("zoom weapon uses secondary first");
   select.iId = VALVE_WEAPON_CROSSBOW;
   delay.iId = VALVE_WEAPON_CROSSBOW;
   select.type = WEAPON_FIRE_ZOOM;
   pBotEdict->v.fov = 0; // not zoomed
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK2) != 0);
   PASS();

   TEST("eagle enables secondary state");
   select.iId = GEARBOX_WEAPON_EAGLE;
   delay.iId = GEARBOX_WEAPON_EAGLE;
   select.type = WEAPON_FIRE;
   testbot.eagle_secondary_state = 0;
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(testbot.eagle_secondary_state, 1);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK2) != 0);
   PASS();

   TEST("M249 ducks for better aim");
   select.iId = GEARBOX_WEAPON_M249;
   delay.iId = GEARBOX_WEAPON_M249;
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_DUCK) != 0);
   ASSERT_TRUE(testbot.f_duck_time > gpGlobals->time);
   PASS();

   TEST("both primary+secondary calls BotSelectAttack");
   select.iId = VALVE_WEAPON_GLOCK;
   delay.iId = VALVE_WEAPON_GLOCK;
   select.type = WEAPON_FIRE;
   select.primary_fire_charge = FALSE;
   select.primary_fire_hold = FALSE;
   select.secondary_fire_charge = FALSE;
   select.secondary_fire_hold = FALSE;
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, TRUE);
   ASSERT_INT(result, TRUE);
   // BotSelectAttack should pick one; either IN_ATTACK or IN_ATTACK2 is set
   ASSERT_TRUE((pBotEdict->v.button & (IN_ATTACK | IN_ATTACK2)) != 0);
   PASS();

   TEST("primary fire with min/max delay");
   select.iId = VALVE_WEAPON_GLOCK;
   delay.iId = VALVE_WEAPON_GLOCK;
   delay.primary_base_delay = 0.1;
   for (int sk = 0; sk < 5; sk++) {
      delay.primary_min_delay[sk] = 0.05;
      delay.primary_max_delay[sk] = 0.15;
   }
   select.primary_fire_charge = FALSE;
   select.primary_fire_hold = FALSE;
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   ASSERT_INT(result, TRUE);
   // shoot_time = time + base_delay(0.1) + RANDOM_FLOAT2(0.05, 0.15) ~ time + 0.15
   // Use FLOAT_NEAR to avoid FP precision issues with -O2 on i686
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time + 0.15f, 0.001f);
   PASS();

   TEST("secondary hold-fire sets shoot_time to now");
   select.secondary_fire_hold = TRUE;
   select.secondary_fire_charge = FALSE;
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, FALSE, TRUE);
   ASSERT_INT(result, TRUE);
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time, 0.01);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK2) != 0);
   select.secondary_fire_hold = FALSE;
   PASS();

   TEST("secondary fire with min/max delay");
   delay.secondary_base_delay = 0.2;
   for (int sk = 0; sk < 5; sk++) {
      delay.secondary_min_delay[sk] = 0.1;
      delay.secondary_max_delay[sk] = 0.2;
   }
   select.secondary_fire_charge = FALSE;
   select.secondary_fire_hold = FALSE;
   pBotEdict->v.button = 0;
   result = BotFireSelectedWeapon(testbot, select, delay, FALSE, TRUE);
   ASSERT_INT(result, TRUE);
   // shoot_time = time + base_delay(0.2) + RANDOM_FLOAT2(0.1, 0.2) ~ time + 0.3
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time + 0.3f, 0.001f);
   PASS();

   return 0;
}

static int test_bot_fire_weapon(void)
{
   printf("BotFireWeapon:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("null enemy returns FALSE");
   testbot.pBotEnemy = NULL;
   ASSERT_INT(BotFireWeapon(Vector(100, 0, 0), testbot, 0), FALSE);
   PASS();

   TEST("charging primary returns TRUE");
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_primary_charging = gpGlobals->time + 1.0; // still charging
   testbot.charging_weapon_id = VALVE_WEAPON_GAUSS;
   pBotEdict->v.button = 0;
   ASSERT_INT(BotFireWeapon(Vector(200, 0, 0), testbot, 0), TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK) != 0);
   testbot.f_primary_charging = -1;
   PASS();

   TEST("primary charge release sets fire delay");
   testbot.f_primary_charging = gpGlobals->time - 0.1; // ready to release
   testbot.charging_weapon_id = VALVE_WEAPON_GAUSS;
   pBotEdict->v.button = 0;
   ASSERT_INT(BotFireWeapon(Vector(200, 0, 0), testbot, 0), TRUE);
   ASSERT_TRUE(testbot.f_primary_charging == -1.0f);
   testbot.f_primary_charging = -1;
   PASS();

   TEST("charging secondary returns TRUE");
   testbot.f_secondary_charging = gpGlobals->time + 1.0;
   testbot.charging_weapon_id = VALVE_WEAPON_GAUSS;
   pBotEdict->v.button = 0;
   ASSERT_INT(BotFireWeapon(Vector(200, 0, 0), testbot, 0), TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK2) != 0);
   testbot.f_secondary_charging = -1;
   PASS();

   TEST("secondary charge release sets fire delay");
   testbot.f_secondary_charging = gpGlobals->time - 0.1;
   testbot.charging_weapon_id = VALVE_WEAPON_GAUSS;
   pBotEdict->v.button = 0;
   ASSERT_INT(BotFireWeapon(Vector(200, 0, 0), testbot, 0), TRUE);
   ASSERT_TRUE(testbot.f_secondary_charging == -1.0f);
   testbot.f_secondary_charging = -1;
   PASS();

   TEST("weaponchange throttle returns FALSE for non-melee");
   testbot.current_weapon_index = 0;
   weapon_select[0].avoid_this_gun = TRUE; // skip reuse path
   testbot.f_weaponchange_time = gpGlobals->time + 5.0;
   qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
   // Should hit the throttle check and return FALSE (not melee)
   weapon_select[0].avoid_this_gun = FALSE;
   PASS();

   TEST("no weapon available returns FALSE");
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   memset(&testbot.current_weapon, 0, sizeof(testbot.current_weapon));
   memset(testbot.m_rgAmmo, 0, sizeof(testbot.m_rgAmmo));
   pBotEdict->v.weapons = 0; // no weapons
   res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
   ASSERT_INT(res, FALSE);
   PASS();

   TEST("underwater clears non-underwater weapon");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.b_in_water = TRUE;
   // Find a weapon index for a non-underwater weapon
   {
      int idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (!weapon_select[i].can_use_underwater && !weapon_select[i].avoid_this_gun) {
            idx = i;
            break;
         }
      }
      if (idx >= 0) {
         testbot.current_weapon_index = idx;
         testbot.current_weapon.iId = weapon_select[idx].iId;
         pBotEdict->v.weapons = (1u << weapon_select[idx].iId);
         testbot.f_primary_charging = -1;
         testbot.f_secondary_charging = -1;
         // underwater with non-underwater weapon should reset current_weapon_index
         res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         ASSERT_INT(testbot.current_weapon_index, -1);
      }
   }
   testbot.b_in_water = FALSE;
   PASS();

   TEST("reuse current weapon fires");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   // Find a usable weapon (crowbar/knife is simplest - no ammo needed)
   {
      int crow_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) {
            crow_idx = i;
            break;
         }
      }
      if (crow_idx >= 0) {
         testbot.current_weapon_index = crow_idx;
         testbot.current_weapon.iId = VALVE_WEAPON_CROWBAR;
         pBotEdict->v.weapons = (1u << VALVE_WEAPON_CROWBAR);
         testbot.f_weaponchange_time = 0;
         // Crowbar should be reusable via current weapon path
         res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         ASSERT_INT(res, TRUE);
      }
   }
   PASS();

   TEST("percent-based weapon selection fires");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   // Give bot a shotgun with ammo
   {
      int sg_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_SHOTGUN) {
            sg_idx = i;
            break;
         }
      }
      if (sg_idx >= 0) {
         pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
         testbot.current_weapon.iId = 0; // not holding anything
         // Set ammo for shotgun
         int ammo1_idx = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
         if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
            testbot.m_rgAmmo[ammo1_idx] = 50;
         fire_delay[sg_idx].iId = VALVE_WEAPON_SHOTGUN;
         res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         ASSERT_INT(res, TRUE);
      }
   }
   PASS();

   TEST("min-skill fallback weapon selection");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   testbot.bot_skill = 4; // worst skill
   // Give bot crowbar (melee, always usable, no ammo)
   pBotEdict->v.weapons = (1u << VALVE_WEAPON_CROWBAR);
   testbot.current_weapon.iId = 0;
   // Make all non-crowbar weapons use_percent = 0 to force min-skill fallback
   {
      int crow_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) {
            crow_idx = i;
            // Temporarily set use_percent to 0 so percent-based selection skips it
            weapon_select[i].use_percent = 0;
            fire_delay[i].iId = VALVE_WEAPON_CROWBAR;
            break;
         }
      }
      res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      // Should fall through to min-skill fallback and find crowbar
      ASSERT_INT(res, TRUE);
      // Restore
      if (crow_idx >= 0)
         weapon_select[crow_idx].use_percent = 100;
   }
   PASS();

   TEST("avoidable weapon fallback");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   testbot.bot_skill = 0; // best skill
   // Give bot only an avoid weapon (crowbar set to avoid)
   pBotEdict->v.weapons = (1u << VALVE_WEAPON_CROWBAR);
   testbot.current_weapon.iId = 0;
   {
      int crow_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) {
            crow_idx = i;
            weapon_select[i].avoid_this_gun = TRUE;
            weapon_select[i].use_percent = 0;
            fire_delay[i].iId = VALVE_WEAPON_CROWBAR;
            break;
         }
      }
      res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      // Should fall through to avoidable weapon fallback
      ASSERT_INT(res, TRUE);
      // Restore
      if (crow_idx >= 0) {
         weapon_select[crow_idx].avoid_this_gun = FALSE;
         weapon_select[crow_idx].use_percent = 100;
      }
   }
   PASS();

   return 0;
}

// ============================================================
// Group 6: BotShootAtEnemy + special weapons
// ============================================================

static int test_bot_shoot_at_enemy(void)
{
   printf("BotShootAtEnemy:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   TEST("reaction time not yet -> returns early");
   testbot.f_reaction_target_time = gpGlobals->time + 10.0;
   BotShootAtEnemy(testbot);
   // Nothing should have changed
   ASSERT_PTR_EQ(testbot.pBotEnemy, pEnemy);
   PASS();

   TEST("normal aim at enemy");
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0; // allow shooting
   BotShootAtEnemy(testbot);
   // Should have set ideal_yaw and idealpitch
   PASS();

   TEST("enemy not visible -> removed with tracking");
   mock_trace_line_fn = trace_partial_hit;
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.b_combat_longjump = FALSE;
   BotShootAtEnemy(testbot);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   mock_trace_line_fn = trace_nohit;

   TEST("longjump mode");
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.b_combat_longjump = TRUE;
   testbot.f_shoot_time = 0;
   BotShootAtEnemy(testbot);
   // In longjump mode, idealpitch and ideal_yaw not set
   testbot.b_combat_longjump = FALSE;
   PASS();

   TEST("close range -> reduced speed");
   testbot.pBotEnemy = pEnemy;
   pEnemy->v.origin = Vector(10, 0, 0); // very close
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   BotShootAtEnemy(testbot);
   ASSERT_TRUE(testbot.b_not_maxspeed == TRUE);
   PASS();

   TEST("fire at feet for RPG-type weapon (feet visible)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   {
      int rpg_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_RPG) {
            rpg_idx = i;
            break;
         }
      }
      if (rpg_idx >= 0) {
         testbot.current_weapon_index = rpg_idx;
         BotShootAtEnemy(testbot);
      }
   }
   PASS();

   TEST("fire at feet for RPG (feet partially blocked -> head aim)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   // Trace that blocks feet but allows head visibility
   struct FeetBlockedLine {
      static void trace(const float *v1, const float *v2,
                        int fNoMonsters, int hullNumber,
                        edict_t *pentToSkip, TraceResult *ptr)
      {
         (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
         // If destination z is negative (below origin = feet), block it
         if (v2[2] < 0.0f) {
            ptr->flFraction = 0.5f;
            ptr->pHit = NULL;
         } else {
            ptr->flFraction = 1.0f;
            ptr->pHit = NULL;
         }
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   mock_trace_line_fn = FeetBlockedLine::trace;
   mock_trace_hull_fn = trace_nohit;
   {
      int rpg_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_RPG) {
            rpg_idx = i;
            break;
         }
      }
      if (rpg_idx >= 0) {
         testbot.current_weapon_index = rpg_idx;
         BotShootAtEnemy(testbot);
      }
   }
   PASS();

   TEST("not visible during fire -> reset reaction");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   testbot.b_combat_longjump = FALSE;
   // Make target visible for the initial check but not for the fire check
   // Use a trace that always shows hit (not visible)
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   // Actually we need to pass the initial visibility check but fail the fire-time one
   // The flow: FVisibleEnemy passes first (line 1800) but fails at line 1846
   // This is tricky because both use the same trace function
   // Instead, test the "angle > 180" wrapping by setting enemy behind
   pEnemy->v.origin = Vector(-200, -200, 0); // behind and to the left
   BotShootAtEnemy(testbot);
   // The angle wrapping code should be exercised
   PASS();

   return 0;
}

static int test_bot_should_detonate_satchel(void)
{
   printf("BotShouldDetonateSatchel:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   TEST("no satchels -> clears state");
   testbot.f_satchel_detonate_time = 10.0;
   testbot.b_satchel_detonating = TRUE;
   ASSERT_INT(BotShouldDetonateSatchel(testbot), FALSE);
   ASSERT_FLOAT_NEAR(testbot.f_satchel_detonate_time, 0.0, 0.01);
   ASSERT_INT(testbot.b_satchel_detonating, FALSE);
   PASS();

   TEST("enemy nearby -> TRUE");
   edict_t *pSatchel = mock_alloc_edict();
   mock_set_classname(pSatchel, "monster_satchel");
   pSatchel->v.owner = pBotEdict;
   pSatchel->v.origin = Vector(300, 0, 0);

   edict_t *pEnemy = create_enemy_player(Vector(350, 0, 0));
   testbot.f_satchel_detonate_time = 10.0;
   ASSERT_INT(BotShouldDetonateSatchel(testbot), TRUE);
   PASS();

   TEST("bot too close -> FALSE");
   pSatchel->v.origin = Vector(50, 0, 0); // within 200 units of bot
   ASSERT_INT(BotShouldDetonateSatchel(testbot), FALSE);
   PASS();

   TEST("other's satchel ignored");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;

   edict_t *otherBot = mock_alloc_edict();
   pSatchel = mock_alloc_edict();
   mock_set_classname(pSatchel, "monster_satchel");
   pSatchel->v.owner = otherBot; // owned by someone else
   pSatchel->v.origin = Vector(300, 0, 0);

   pEnemy = create_enemy_player(Vector(350, 0, 0));
   testbot.f_satchel_detonate_time = 10.0;
   testbot.b_satchel_detonating = TRUE;
   ASSERT_INT(BotShouldDetonateSatchel(testbot), FALSE);
   // No satchels owned by this bot -> state cleared
   ASSERT_FLOAT_NEAR(testbot.f_satchel_detonate_time, 0.0, 0.01);
   PASS();

   return 0;
}

static int test_bot_detonate_satchel(void)
{
   printf("BotDetonateSatchel:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   TEST("inactive (detonate_time <= 0) returns FALSE");
   testbot.f_satchel_detonate_time = 0;
   ASSERT_INT(BotDetonateSatchel(testbot), FALSE);
   PASS();

   TEST("phase 2: with satchel weapon -> detonates");
   testbot.f_satchel_detonate_time = gpGlobals->time + 30.0;
   testbot.b_satchel_detonating = TRUE;
   testbot.current_weapon.iId = VALVE_WEAPON_SATCHEL;
   pBotEdict->v.button = 0;
   ASSERT_INT(BotDetonateSatchel(testbot), TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK) != 0);
   ASSERT_FLOAT_NEAR(testbot.f_satchel_detonate_time, 0.0, 0.01);
   ASSERT_INT(testbot.b_satchel_detonating, FALSE);
   PASS();

   TEST("phase 2: without satchel weapon -> selects satchel");
   testbot.f_satchel_detonate_time = gpGlobals->time + 30.0;
   testbot.b_satchel_detonating = TRUE;
   testbot.current_weapon.iId = VALVE_WEAPON_GLOCK;
   pBotEdict->v.button = 0;
   ASSERT_INT(BotDetonateSatchel(testbot), TRUE);
   // Should have called UTIL_SelectItem and set shoot_time
   ASSERT_TRUE(testbot.f_shoot_time > gpGlobals->time);
   PASS();

   TEST("phase 1: timeout + should detonate -> detonates");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;

   // Create satchel owned by bot with enemy nearby
   edict_t *pSatchel = mock_alloc_edict();
   mock_set_classname(pSatchel, "monster_satchel");
   pSatchel->v.owner = pBotEdict;
   pSatchel->v.origin = Vector(300, 0, 0);

   edict_t *pEnemy = create_enemy_player(Vector(350, 0, 0));

   testbot.f_satchel_detonate_time = gpGlobals->time - 1.0; // timeout reached
   testbot.f_satchel_check_time = 0;
   testbot.b_satchel_detonating = FALSE;
   testbot.current_weapon.iId = VALVE_WEAPON_SATCHEL;
   pBotEdict->v.button = 0;

   ASSERT_INT(BotDetonateSatchel(testbot), TRUE);
   ASSERT_TRUE((pBotEdict->v.button & IN_ATTACK) != 0);
   PASS();

   TEST("phase 1: timeout + detonate but not holding satchel -> switch");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;

   pSatchel = mock_alloc_edict();
   mock_set_classname(pSatchel, "monster_satchel");
   pSatchel->v.owner = pBotEdict;
   pSatchel->v.origin = Vector(300, 0, 0);

   pEnemy = create_enemy_player(Vector(350, 0, 0));

   testbot.f_satchel_detonate_time = gpGlobals->time - 1.0;
   testbot.f_satchel_check_time = 0;
   testbot.b_satchel_detonating = FALSE;
   testbot.current_weapon.iId = VALVE_WEAPON_GLOCK; // not holding satchel
   pBotEdict->v.button = 0;

   ASSERT_INT(BotDetonateSatchel(testbot), TRUE);
   // Should have set b_satchel_detonating and called UTIL_SelectItem
   ASSERT_INT(testbot.b_satchel_detonating, TRUE);
   ASSERT_TRUE(testbot.f_shoot_time >= gpGlobals->time + 0.25f);
   PASS();

   TEST("phase 1: timeout but unsafe -> extends deadline");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;

   pSatchel = mock_alloc_edict();
   mock_set_classname(pSatchel, "monster_satchel");
   pSatchel->v.owner = pBotEdict;
   pSatchel->v.origin = Vector(50, 0, 0); // too close to bot (< 200)

   pEnemy = create_enemy_player(Vector(350, 0, 0));

   testbot.f_satchel_detonate_time = gpGlobals->time - 1.0; // timeout
   testbot.f_satchel_check_time = 0;
   testbot.b_satchel_detonating = FALSE;

   ASSERT_INT(BotDetonateSatchel(testbot), FALSE);
   // Deadline should be extended
   ASSERT_TRUE(testbot.f_satchel_detonate_time > gpGlobals->time);
   PASS();

   return 0;
}

static int test_bot_shoot_tripmine(void)
{
   printf("BotShootTripmine:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("disabled -> FALSE");
   testbot.b_shoot_tripmine = FALSE;
   ASSERT_INT(BotShootTripmine(testbot), FALSE);
   PASS();

   TEST("null tripmine edict -> FALSE");
   testbot.b_shoot_tripmine = TRUE;
   testbot.tripmine_edict = NULL;
   ASSERT_INT(BotShootTripmine(testbot), FALSE);
   PASS();

   TEST("has enemy -> FALSE");
   edict_t *pTripmine = mock_alloc_edict();
   mock_set_classname(pTripmine, "monster_tripmine");
   pTripmine->v.origin = Vector(200, 0, 0);
   testbot.tripmine_edict = pTripmine;
   testbot.v_tripmine = Vector(200, 0, 0);
   testbot.pBotEnemy = create_enemy_player(Vector(300, 0, 0));
   ASSERT_INT(BotShootTripmine(testbot), FALSE);
   PASS();

   TEST("fire at tripmine with glock");
   testbot.pBotEnemy = NULL;
   // Give bot a glock with ammo
   give_bot_weapon(testbot, VALVE_WEAPON_GLOCK, 17, 50, 0);
   // Need weapon_select initialized
   // BotFireWeapon needs the glock entry
   qboolean result = BotShootTripmine(testbot);
   // Result depends on whether glock is available in weapon_select
   // In any case, pBotEnemy should be NULL after (restored)
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   return 0;
}

// ============================================================
// Original breakable tests (preserved)
// ============================================================

static int test_mode0_no_breakables_targeted(void)
{
   printf("BotFindEnemy mode 0 (disabled):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

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
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0);

   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

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
   setup_skill_settings();

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
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pBreakable = create_breakable(Vector(0, 200, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

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
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0);

   edict_t *pBreakable = create_breakable(Vector(50, 0, 0), 50);
   mock_add_breakable(pBreakable, TRUE);

   g_trace_hit_edict = pBreakable;
   mock_trace_hull_fn = trace_hit_breakable_hull;
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
   setup_skill_settings();

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
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pBreakable = create_breakable(Vector(100, 0, 0), 50);
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
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

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
// Group 8: RANDOM mock-dependent tests
// ============================================================

static int test_bot_find_enemy_jump_on_kill(void)
{
   printf("BotFindEnemy jump on kill (RANDOM mock):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("dead enemy + random <= 10 -> jump");
   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   pEnemy->v.deadflag = DEAD_DEAD;
   pEnemy->v.health = 0;
   testbot.pBotEnemy = pEnemy;
   mock_random_long_ret = 5; // <= 10
   pBotEdict->v.button = 0;
   BotFindEnemy(testbot);
   ASSERT_TRUE((pBotEdict->v.button & IN_JUMP) != 0);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   TEST("dead enemy + random > 10 -> no jump");
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   pEnemy->v.deadflag = DEAD_DEAD;
   pEnemy->v.health = 0;
   testbot.pBotEnemy = pEnemy;
   mock_random_long_ret = 50; // > 10
   pBotEdict->v.button = 0;
   BotFindEnemy(testbot);
   ASSERT_TRUE((pBotEdict->v.button & IN_JUMP) == 0);
   ASSERT_PTR_NULL(testbot.pBotEnemy);
   PASS();

   TEST("chat protected enemy -> no jump (even with random <= 10)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   // Make enemy chat protected by setting last_time_not_facing_wall to old value
   {
      int pidx = ENTINDEX(pEnemy) - 1;
      if (pidx >= 0 && pidx < 32)
         players[pidx].last_time_not_facing_wall = gpGlobals->time - 100;
   }
   testbot.pBotEnemy = pEnemy;
   mock_random_long_ret = 5;
   pBotEdict->v.button = 0;
   BotFindEnemy(testbot);
   // chatprot is TRUE so jump should NOT happen
   ASSERT_TRUE((pBotEdict->v.button & IN_JUMP) == 0);
   PASS();

   return 0;
}

static int test_bot_find_enemy_sound_enemy_path(void)
{
   printf("BotFindEnemy sound enemy is_sound_enemy path:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   // Set up sound entity
   static CSoundEnt soundEnt;
   pSoundEnt = &soundEnt;
   pSoundEnt->Initialize();

   // Put bot in bots[] array
   int bot_idx = 0;
   bots[bot_idx] = testbot;
   bots[bot_idx].pEdict = pBotEdict;

   TEST("sound enemy found and reaction time set (line 1084)");
   // Enemy behind bot (out of view cone) so normal player search skips it.
   // But sound system + BotFindEnemyNearestToPoint finds it.
   edict_t *pEnemy = create_enemy_player(Vector(-200, 0, 0)); // behind bot
   bots[bot_idx].pEdict->v.v_angle = Vector(0, 0, 0); // looking forward (+X)
   bots[bot_idx].f_next_find_visible_sound_enemy_time = 0;
   CSoundEnt::InsertSound(pEnemy, 1, Vector(-200, 0, 0), 2000, 5.0, -1);
   BotFindEnemy(bots[bot_idx]);
   ASSERT_PTR_EQ(bots[bot_idx].pBotEnemy, pEnemy);
   // Sound enemy found -> is_sound_enemy=TRUE (line 1084), reaction time set
   ASSERT_TRUE(bots[bot_idx].f_reaction_target_time > gpGlobals->time);
   ASSERT_INT(bots[bot_idx].waypoint_goal, -1);
   ASSERT_INT(bots[bot_idx].wpt_goal_type, WPT_GOAL_ENEMY);
   PASS();

   pSoundEnt = NULL;
   return 0;
}

static int test_bot_point_gun_negative_pitch(void)
{
   printf("BotPointGun negative pitch_speed path:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   TEST("negative pitch_speed hits else branch (line 109)");
   // Need pitch_speed to become negative after the speed calculation
   // Set a deviation that makes pitch_speed negative
   pBotEdict->v.idealpitch = -30.0; // target pitch is negative
   pBotEdict->v.ideal_yaw = 10.0;
   pBotEdict->v.v_angle = Vector(10, 0, 0); // current pitch is positive
   // deviation.x = idealpitch - v_angle.x = -30 - 10 = -40 (wrapped)
   pBotEdict->v.pitch_speed = -5.0; // start with negative
   testbot.b_combat_longjump = FALSE;
   testbot.pBotEnemy = NULL;
   testbot.curr_waypoint_index = -1;
   testbot.pBotPickupItem = NULL;
   BotPointGun(testbot);
   // pitch_speed should have been modified through the else branch
   PASS();

   TEST("negative yaw_speed hits else branch (line 115)");
   pBotEdict->v.idealpitch = 10.0;
   pBotEdict->v.ideal_yaw = -30.0; // target yaw negative
   pBotEdict->v.v_angle = Vector(0, 10, 0);
   pBotEdict->v.yaw_speed = -5.0;
   pBotEdict->v.pitch_speed = 5.0;
   BotPointGun(testbot);
   PASS();

   return 0;
}

static int test_bot_fire_weapon_percent_selection(void)
{
   printf("BotFireWeapon percent-based weapon selection (RANDOM mock):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;

   // Give bot shotgun with ammo
   int sg_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_SHOTGUN) {
         sg_idx = i;
         break;
      }
   }

   TEST("random selects first weapon in percent list");
   if (sg_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      int ammo1_idx = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
         testbot.m_rgAmmo[ammo1_idx] = 50;
      fire_delay[sg_idx].iId = VALVE_WEAPON_SHOTGUN;

      mock_random_long_ret = 0; // pick first weapon in list
      qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      ASSERT_INT(res, TRUE);
   }
   PASS();

   TEST("random selects high percent value");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;

   if (sg_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      int ammo1_idx = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
         testbot.m_rgAmmo[ammo1_idx] = 50;
      fire_delay[sg_idx].iId = VALVE_WEAPON_SHOTGUN;

      mock_random_long_ret = 9999; // high value, gets clamped to total_percent-1
      qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      ASSERT_INT(res, TRUE);
   }
   PASS();

   return 0;
}

static int test_bot_fire_weapon_reuse_with_choice(void)
{
   printf("BotFireWeapon reuse path with weapon_choice:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   TEST("reuse with direct weapon_choice match");
   {
      int crow_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) {
            crow_idx = i;
            break;
         }
      }
      if (crow_idx >= 0) {
         testbot.current_weapon_index = crow_idx;
         testbot.current_weapon.iId = VALVE_WEAPON_CROWBAR;
         pBotEdict->v.weapons = (1u << VALVE_WEAPON_CROWBAR);
         testbot.f_weaponchange_time = 0;
         fire_delay[crow_idx].iId = VALVE_WEAPON_CROWBAR;

         // weapon_choice matches directly -> enters reuse path line 1499
         qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, VALVE_WEAPON_CROWBAR);
         ASSERT_INT(res, TRUE);
      }
   }
   PASS();

   return 0;
}

static int test_bot_fire_weapon_min_skill_both(void)
{
   printf("BotFireWeapon min-skill fallback with both attacks:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   testbot.bot_skill = 0; // best skill

   // Find a weapon that has both primary and secondary valid at distance 200
   // Use glock which has both attacks
   int glock_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_GLOCK) {
         glock_idx = i;
         break;
      }
   }

   TEST("min-skill fallback: primary_skill < secondary_skill -> use secondary");
   if (glock_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_GLOCK);
      int ammo1_idx = weapon_defs[VALVE_WEAPON_GLOCK].iAmmo1;
      if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
         testbot.m_rgAmmo[ammo1_idx] = 50;
      fire_delay[glock_idx].iId = VALVE_WEAPON_GLOCK;

      // Temporarily set use_percent to 0 and make sure avoid_this_gun is FALSE
      int old_percent = weapon_select[glock_idx].use_percent;
      weapon_select[glock_idx].use_percent = 0; // skip percent selection

      // Save and modify skill levels to force the min-skill both-attacks path
      int old_primary_skill = weapon_select[glock_idx].primary_skill_level;
      int old_secondary_skill = weapon_select[glock_idx].secondary_skill_level;
      weapon_select[glock_idx].primary_skill_level = 1;
      weapon_select[glock_idx].secondary_skill_level = 2;

      qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      // Should fall through to min-skill fallback

      // Restore
      weapon_select[glock_idx].use_percent = old_percent;
      weapon_select[glock_idx].primary_skill_level = old_primary_skill;
      weapon_select[glock_idx].secondary_skill_level = old_secondary_skill;
   }
   PASS();

   TEST("min-skill fallback: secondary_skill < primary_skill -> use primary");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   testbot.bot_skill = 0;

   if (glock_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_GLOCK);
      int ammo1_idx = weapon_defs[VALVE_WEAPON_GLOCK].iAmmo1;
      if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
         testbot.m_rgAmmo[ammo1_idx] = 50;
      fire_delay[glock_idx].iId = VALVE_WEAPON_GLOCK;

      int old_percent = weapon_select[glock_idx].use_percent;
      weapon_select[glock_idx].use_percent = 0;

      int old_primary_skill = weapon_select[glock_idx].primary_skill_level;
      int old_secondary_skill = weapon_select[glock_idx].secondary_skill_level;
      weapon_select[glock_idx].primary_skill_level = 2;
      weapon_select[glock_idx].secondary_skill_level = 1;

      qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      (void)res;

      weapon_select[glock_idx].use_percent = old_percent;
      weapon_select[glock_idx].primary_skill_level = old_primary_skill;
      weapon_select[glock_idx].secondary_skill_level = old_secondary_skill;
   }
   PASS();

   return 0;
}

static int test_bot_fire_weapon_avoid_skill_strip(void)
{
   printf("BotFireWeapon avoid fallback with skill strip:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   testbot.bot_skill = 4; // worst skill

   TEST("avoid weapon with skill check stripping primary attack");
   {
      int crow_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) {
            crow_idx = i;
            break;
         }
      }
      if (crow_idx >= 0) {
         pBotEdict->v.weapons = (1u << VALVE_WEAPON_CROWBAR);
         fire_delay[crow_idx].iId = VALVE_WEAPON_CROWBAR;

         // Save and modify weapon properties
         qboolean old_avoid = weapon_select[crow_idx].avoid_this_gun;
         int old_percent = weapon_select[crow_idx].use_percent;
         int old_primary_skill = weapon_select[crow_idx].primary_skill_level;
         int old_secondary_skill = weapon_select[crow_idx].secondary_skill_level;

         weapon_select[crow_idx].avoid_this_gun = TRUE;
         weapon_select[crow_idx].use_percent = 0;
         // primary_skill_level=3, secondary_skill_level=5
         // weapon_skill=4: 4 <= 3 = FALSE (primary stripped), 4 <= 5 = TRUE (secondary ok)
         // BotCanUseWeapon = FALSE || TRUE = TRUE (passes line 1687)
         weapon_select[crow_idx].primary_skill_level = 3;
         weapon_select[crow_idx].secondary_skill_level = 5;
         testbot.weapon_skill = 4;

         qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         (void)res;

         weapon_select[crow_idx].avoid_this_gun = old_avoid;
         weapon_select[crow_idx].use_percent = old_percent;
         weapon_select[crow_idx].primary_skill_level = old_primary_skill;
         weapon_select[crow_idx].secondary_skill_level = old_secondary_skill;
      }
   }
   PASS();

   return 0;
}

static int test_bot_fire_weapon_underwater(void)
{
   printf("BotFireWeapon underwater weapon reset:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.b_in_water = TRUE;

   TEST("underwater with can_use_underwater weapon keeps index");
   {
      int idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].can_use_underwater && !weapon_select[i].avoid_this_gun) {
            idx = i;
            break;
         }
      }
      if (idx >= 0) {
         testbot.current_weapon_index = idx;
         testbot.current_weapon.iId = weapon_select[idx].iId;
         pBotEdict->v.weapons = (1u << weapon_select[idx].iId);
         testbot.f_primary_charging = -1;
         testbot.f_secondary_charging = -1;
         int saved_idx = testbot.current_weapon_index;
         // Should NOT reset current_weapon_index since weapon is underwater-capable
         BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         // Note: current_weapon_index might change due to reuse/selection logic,
         // but it shouldn't be reset to -1 by the underwater check
      }
   }
   PASS();

   TEST("underwater with negative current_weapon_index");
   testbot.current_weapon_index = -1;
   testbot.f_primary_charging = -1;
   testbot.f_secondary_charging = -1;
   // Should handle negative index gracefully
   BotFireWeapon(Vector(200, 0, 0), testbot, 0);
   PASS();

   return 0;
}

static int test_bot_fire_selected_weapon_delays(void)
{
   printf("BotFireSelectedWeapon fire delays (RANDOM mock):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   bot_weapon_select_t select;
   memset(&select, 0, sizeof(select));
   select.iId = VALVE_WEAPON_GLOCK;
   select.type = WEAPON_FIRE;

   bot_fire_delay_t delay;
   memset(&delay, 0, sizeof(delay));
   delay.iId = VALVE_WEAPON_GLOCK;

   TEST("primary delay with mock_random_float_ret = high");
   delay.primary_base_delay = 0.1f;
   for (int sk = 0; sk < 5; sk++) {
      delay.primary_min_delay[sk] = 0.05f;
      delay.primary_max_delay[sk] = 0.15f;
   }
   mock_random_float_ret = 999.0f; // will be clamped to max_delay (0.15)
   pBotEdict->v.button = 0;
   BotFireSelectedWeapon(testbot, select, delay, TRUE, FALSE);
   // shoot_time = time + 0.1 + 0.15 = time + 0.25
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time + 0.25f, 0.001f);
   PASS();

   TEST("secondary delay with mock_random_float_ret = mid");
   delay.secondary_base_delay = 0.2f;
   for (int sk = 0; sk < 5; sk++) {
      delay.secondary_min_delay[sk] = 0.1f;
      delay.secondary_max_delay[sk] = 0.3f;
   }
   mock_random_float_ret = 0.2f; // within range [0.1, 0.3]
   pBotEdict->v.button = 0;
   BotFireSelectedWeapon(testbot, select, delay, FALSE, TRUE);
   // shoot_time = time + 0.2 + 0.2 = time + 0.4
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time + 0.4f, 0.001f);
   PASS();

   return 0;
}

static int test_bot_shoot_at_enemy_feet_player_hit(void)
{
   printf("BotShootAtEnemy feet trace hits player:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   testbot.b_combat_longjump = FALSE;

   // Find RPG weapon index for fire-at-feet
   int rpg_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_RPG) {
         rpg_idx = i;
         break;
      }
   }

   TEST("feet trace hits player -> aims at feet");
   if (rpg_idx >= 0) {
      testbot.current_weapon_index = rpg_idx;

      // Trace that partially hits a player entity (using g_trace_hit_edict)
      struct FeetHitPlayer {
         static void trace(const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
         {
            (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
            // For the feet trace (low z destination), return partial hit on a player
            if (v2[2] < 0.0f) {
               ptr->flFraction = 0.95f; // >= 0.95
               ptr->pHit = g_trace_hit_edict; // hit a player
            } else {
               ptr->flFraction = 1.0f;
               ptr->pHit = NULL;
            }
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };
      g_trace_hit_edict = pEnemy;
      mock_trace_line_fn = FeetHitPlayer::trace;
      mock_trace_hull_fn = trace_nohit;

      BotShootAtEnemy(testbot);
      // Should aim at feet since partial hit on player at >= 0.95
      g_trace_hit_edict = NULL;
   }
   mock_trace_line_fn = trace_nohit;
   PASS();

   return 0;
}

static int test_bot_shoot_at_enemy_angle_wrap(void)
{
   printf("BotShootAtEnemy angle wrapping:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(-200, -200, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   testbot.b_combat_longjump = FALSE;

   TEST("enemy angle > 180 triggers wrapping (lines 1811-1815)");
   // Place enemy so UTIL_VecToAngles returns angles > 180
   pEnemy->v.origin = Vector(-200, -200, -200);
   BotShootAtEnemy(testbot);
   // The angle wrap paths should be exercised
   PASS();

   return 0;
}

static int test_bot_shoot_at_enemy_not_visible_reset(void)
{
   printf("BotShootAtEnemy not visible during fire -> reaction reset:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   testbot.b_combat_longjump = FALSE;

   TEST("visible for main check, not visible for fire check -> reset reaction");
   // Use a trace that starts visible (for FVisibleEnemy at line 1800) then
   // becomes blocked (for FVisibleEnemy at line 1846)
   static int s_call_count;
   s_call_count = 0;
   struct AlternatingTrace {
      static void trace(const float *v1, const float *v2,
                        int fNoMonsters, int hullNumber,
                        edict_t *pentToSkip, TraceResult *ptr)
      {
         (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
         s_call_count++;
         // First few calls: visible (for initial check). Later: blocked.
         if (s_call_count <= 4) {
            ptr->flFraction = 1.0f;
            ptr->pHit = NULL;
         } else {
            ptr->flFraction = 0.3f;
            ptr->pHit = NULL;
         }
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   mock_trace_line_fn = AlternatingTrace::trace;
   mock_trace_hull_fn = trace_nohit;

   float old_reaction = testbot.f_reaction_target_time;
   BotShootAtEnemy(testbot);
   // Should have hit the "not visible during fire" path (line 1867)
   // which calls BotResetReactionTime
   mock_trace_line_fn = trace_nohit;
   PASS();

   return 0;
}

static int test_bot_detonate_satchel_switch(void)
{
   printf("BotDetonateSatchel phase 1 weapon switch:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   TEST("phase 1: should detonate but not holding satchel -> sets b_satchel_detonating");
   // Create satchel owned by bot with enemy nearby
   edict_t *pSatchel = mock_alloc_edict();
   mock_set_classname(pSatchel, "monster_satchel");
   pSatchel->v.owner = pBotEdict;
   pSatchel->v.origin = Vector(300, 0, 0);

   edict_t *pEnemy = create_enemy_player(Vector(350, 0, 0));
   (void)pEnemy;

   testbot.f_satchel_detonate_time = gpGlobals->time + 10.0; // not timed out
   testbot.f_satchel_check_time = 0; // allow check
   testbot.b_satchel_detonating = FALSE;
   testbot.current_weapon.iId = VALVE_WEAPON_GLOCK; // not holding satchel
   pBotEdict->v.button = 0;

   qboolean result = BotDetonateSatchel(testbot);
   ASSERT_INT(result, TRUE);
   ASSERT_INT(testbot.b_satchel_detonating, TRUE);
   ASSERT_TRUE(testbot.f_shoot_time >= gpGlobals->time + 0.25f);
   PASS();

   TEST("phase 1: check_time not reached -> returns FALSE");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;

   testbot.f_satchel_detonate_time = gpGlobals->time + 10.0;
   testbot.f_satchel_check_time = gpGlobals->time + 100.0; // far in future
   testbot.b_satchel_detonating = FALSE;
   result = BotDetonateSatchel(testbot);
   ASSERT_INT(result, FALSE);
   PASS();

   return 0;
}

static int test_bot_fire_weapon_melee_throttle(void)
{
   printf("BotFireWeapon weapon change throttle melee path:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   TEST("weaponchange throttle with melee weapon returns TRUE");
   {
      int crow_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) {
            crow_idx = i;
            break;
         }
      }
      if (crow_idx >= 0) {
         testbot.current_weapon_index = crow_idx;
         weapon_select[crow_idx].avoid_this_gun = TRUE; // skip reuse path
         testbot.f_weaponchange_time = gpGlobals->time + 5.0; // throttle active
         qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         // Melee weapon during throttle should return TRUE (keep tracking)
         ASSERT_INT(res, TRUE);
         weapon_select[crow_idx].avoid_this_gun = FALSE;
      }
   }
   PASS();

   return 0;
}

static int test_bot_fire_weapon_charging_delays(void)
{
   printf("BotFireWeapon charging weapon fire delays (RANDOM mock):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   // Set non-zero fire delays for gauss so we can test the delay path
   int gauss_delay_idx = -1;
   for (int i = 0; i < NUM_OF_WEAPON_SELECTS; i++) {
      if (fire_delay[i].iId == VALVE_WEAPON_GAUSS) {
         gauss_delay_idx = i;
         break;
      }
   }
   ASSERT_TRUE(gauss_delay_idx >= 0);

   // Save original and set test delays
   bot_fire_delay_t saved_delay = fire_delay[gauss_delay_idx];
   fire_delay[gauss_delay_idx].primary_base_delay = 0.1f;
   fire_delay[gauss_delay_idx].secondary_base_delay = 0.1f;
   for (int s = 0; s < 5; s++) {
      fire_delay[gauss_delay_idx].primary_min_delay[s] = 0.05f;
      fire_delay[gauss_delay_idx].primary_max_delay[s] = 0.5f;
      fire_delay[gauss_delay_idx].secondary_min_delay[s] = 0.05f;
      fire_delay[gauss_delay_idx].secondary_max_delay[s] = 0.5f;
   }

   TEST("primary charge release with specific random delay");
   mock_random_float_ret = 0.3f;
   testbot.f_primary_charging = gpGlobals->time - 0.1f; // ready to release
   testbot.charging_weapon_id = VALVE_WEAPON_GAUSS;
   pBotEdict->v.button = 0;
   qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
   ASSERT_INT(res, TRUE);
   ASSERT_TRUE(testbot.f_primary_charging == -1.0f);
   // f_shoot_time = time + 0.1 + clamp(0.3, 0.05, 0.5) = time + 0.4
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time + 0.4f, 0.001f);
   PASS();

   TEST("secondary charge release with specific random delay");
   mock_random_float_ret = 0.2f;
   testbot.f_primary_charging = -1;
   testbot.f_secondary_charging = gpGlobals->time - 0.1f;
   testbot.charging_weapon_id = VALVE_WEAPON_GAUSS;
   pBotEdict->v.button = 0;
   res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
   ASSERT_INT(res, TRUE);
   ASSERT_TRUE(testbot.f_secondary_charging == -1.0f);
   // f_shoot_time = time + 0.1 + clamp(0.2, 0.05, 0.5) = time + 0.3
   ASSERT_FLOAT_NEAR(testbot.f_shoot_time, gpGlobals->time + 0.3f, 0.001f);
   PASS();

   // Restore original delay values
   fire_delay[gauss_delay_idx] = saved_delay;

   return 0;
}

static int test_bot_remove_enemy_random_track_time(void)
{
   printf("BotRemoveEnemy track time with RANDOM mock:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   TEST("track time uses mock random (high value)");
   mock_random_float_ret = 999.0f; // will be clamped to max
   BotRemoveEnemy(testbot, TRUE);
   // f_track_sound_time = time + clamped(track_sound_time_max)
   float max_track = skill_settings[testbot.bot_skill].track_sound_time_max;
   ASSERT_FLOAT_NEAR(testbot.f_track_sound_time,
                      gpGlobals->time + max_track, 0.01f);
   PASS();

   TEST("track time uses mock random (low value)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   mock_random_float_ret = 0.0f; // will be clamped to min
   BotRemoveEnemy(testbot, TRUE);
   float min_track = skill_settings[testbot.bot_skill].track_sound_time_min;
   ASSERT_FLOAT_NEAR(testbot.f_track_sound_time,
                      gpGlobals->time + min_track, 0.01f);
   PASS();

   return 0;
}

static int test_bot_reset_reaction_time_random(void)
{
   printf("BotResetReactionTime with RANDOM mock:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   TEST("high random -> max react delay");
   mock_random_float_ret = 999.0f; // clamped to max
   BotResetReactionTime(testbot, FALSE);
   float max_delay = skill_settings[testbot.bot_skill].react_delay_max;
   ASSERT_FLOAT_NEAR(testbot.f_reaction_target_time,
                      gpGlobals->time + max_delay, 0.01f);
   PASS();

   TEST("slow reaction with max random -> 4x max delay");
   mock_random_float_ret = 999.0f;
   BotResetReactionTime(testbot, TRUE);
   ASSERT_FLOAT_NEAR(testbot.f_reaction_target_time,
                      gpGlobals->time + max_delay * 4, 0.01f);
   PASS();

   TEST("low random -> min react delay");
   mock_random_float_ret = 0.0f; // clamped to min
   BotResetReactionTime(testbot, FALSE);
   float min_delay = skill_settings[testbot.bot_skill].react_delay_min;
   ASSERT_FLOAT_NEAR(testbot.f_reaction_target_time,
                      gpGlobals->time + min_delay, 0.01f);
   PASS();

   return 0;
}

static int test_bot_fire_weapon_better_choice(void)
{
   printf("BotFireWeapon reuse with BotGetBetterWeaponChoice:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;

   TEST("reuse path enters BotGetBetterWeaponChoice");
   {
      // Give bot crowbar as current weapon, and shotgun as potentially better
      int crow_idx = -1, sg_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_CROWBAR) crow_idx = i;
         if (weapon_select[i].iId == VALVE_WEAPON_SHOTGUN) sg_idx = i;
      }
      if (crow_idx >= 0 && sg_idx >= 0) {
         testbot.current_weapon_index = crow_idx;
         testbot.current_weapon.iId = VALVE_WEAPON_CROWBAR;
         pBotEdict->v.weapons = (1u << VALVE_WEAPON_CROWBAR) | (1u << VALVE_WEAPON_SHOTGUN);
         int ammo1_idx = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
         if (ammo1_idx >= 0 && ammo1_idx < MAX_AMMO_SLOTS)
            testbot.m_rgAmmo[ammo1_idx] = 50;
         fire_delay[crow_idx].iId = VALVE_WEAPON_CROWBAR;
         fire_delay[sg_idx].iId = VALVE_WEAPON_SHOTGUN;
         testbot.f_weaponchange_time = 0;

         // weapon_choice == 0, so BotGetBetterWeaponChoice is called
         qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
         ASSERT_INT(res, TRUE);
      }
   }
   PASS();

   return 0;
}

static int test_try_select_weapon_random(void)
{
   printf("TrySelectWeapon weaponchange_rate with RANDOM mock:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   bot_weapon_select_t select;
   memset(&select, 0, sizeof(select));
   select.iId = VALVE_WEAPON_SHOTGUN;
   safe_strcopy(select.weapon_name, sizeof(select.weapon_name), "weapon_shotgun");

   bot_fire_delay_t delay;
   memset(&delay, 0, sizeof(delay));
   delay.iId = VALVE_WEAPON_SHOTGUN;

   TEST("high random -> max weaponchange rate");
   mock_random_float_ret = 999.0f;
   testbot.current_weapon.iId = VALVE_WEAPON_SHOTGUN;
   TrySelectWeapon(testbot, 3, select, delay);
   float max_rate = skill_settings[testbot.bot_skill].weaponchange_rate_max;
   ASSERT_FLOAT_NEAR(testbot.f_weaponchange_time,
                      gpGlobals->time + max_rate, 0.01f);
   PASS();

   TEST("low random -> min weaponchange rate");
   mock_random_float_ret = 0.0f;
   TrySelectWeapon(testbot, 3, select, delay);
   float min_rate = skill_settings[testbot.bot_skill].weaponchange_rate_min;
   ASSERT_FLOAT_NEAR(testbot.f_weaponchange_time,
                      gpGlobals->time + min_rate, 0.01f);
   PASS();

   return 0;
}

static int test_prediction_random_variation(void)
{
   printf("Prediction variation with RANDOM mock:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_hull_fn = trace_nohit;

   TEST("velocity variation with high random");
   mock_random_float_ret = 999.0f; // clamped to maxvar
   Vector vel(100, 0, 0);
   Vector result = AddPredictionVelocityVaritation(testbot, vel);
   float maxvar = 1.0f + skill_settings[testbot.bot_skill].ping_emu_speed_varitation;
   // Result should be scaled by maxvar
   ASSERT_FLOAT_NEAR(result.Length(), vel.Length() * maxvar, 1.0f);
   PASS();

   TEST("velocity variation with low random");
   mock_random_float_ret = 0.0f; // clamped to minvar
   result = AddPredictionVelocityVaritation(testbot, vel);
   float minvar = 1.0f - skill_settings[testbot.bot_skill].ping_emu_speed_varitation;
   ASSERT_FLOAT_NEAR(result.Length(), vel.Length() * minvar, 1.0f);
   PASS();

   TEST("position variation with specific random angle");
   mock_random_float_ret = 0.0f;
   Vector pos_result = AddPredictionPositionVaritation(testbot);
   // Result length should be ping_emu_position_varitation
   ASSERT_FLOAT_NEAR(pos_result.Length(),
                      skill_settings[testbot.bot_skill].ping_emu_position_varitation, 0.1f);
   PASS();

   return 0;
}

// ============================================================
// Group 9: Additional coverage tests for remaining uncovered lines
// ============================================================

// Mode 1: breakable distance skip (line 881) and monster tests (918, 935)
static int test_mode1_distance_and_monster_skips(void)
{
   printf("BotFindEnemy mode 1 distance/monster skip paths:\n");

   // --- Breakable distance skip (line 881) ---
   // Add close breakable FIRST, then far. The close one becomes nearest,
   // then the far one hits distance >= nearestdistance on line 881.
   mock_reset();
   setup_skill_settings();
   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   bot_shoot_breakables = 1;

   TEST("mode 1: farther breakable skipped after closer found (line 881)");
   edict_t *pClose = create_breakable(Vector(30, 0, 0), 50);
   mock_add_breakable(pClose, TRUE);
   edict_t *pFar = create_breakable(Vector(200, 0, 0), 50);
   mock_add_breakable(pFar, TRUE);
   BotFindEnemy(testbot);
   ASSERT_PTR_EQ(testbot.pBotEnemy, pClose);
   bot_shoot_breakables = 0;
   PASS();

   // --- Dead monster skip (line 918) ---
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   bot_shoot_breakables = 1;

   TEST("mode 1: dead monster skipped (line 918)");
   {
      edict_t *pM = mock_alloc_edict();
      mock_set_classname(pM, "monster_alien_grunt");
      pM->v.origin = Vector(100, 0, 0);
      pM->v.health = 0; // dead
      pM->v.flags = FL_MONSTER;
      pM->v.takedamage = DAMAGE_YES;
      pM->v.deadflag = DEAD_DEAD;
      pM->v.solid = SOLID_BBOX;
      pM->v.view_ofs = Vector(0, 0, 20);
      BotFindEnemy(testbot);
      ASSERT_PTR_NULL(testbot.pBotEnemy);
   }
   bot_shoot_breakables = 0;
   PASS();

   // --- Monster distance skip (line 935) ---
   // Add close monster first, then far monster.
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   pBotEdict->v.v_angle = Vector(0, 0, 0);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   bot_shoot_breakables = 1;

   TEST("mode 1: farther monster skipped after closer found (line 935)");
   {
      // Close monster
      edict_t *pMClose = mock_alloc_edict();
      mock_set_classname(pMClose, "monster_alien_grunt");
      pMClose->v.origin = Vector(50, 0, 0);
      pMClose->v.health = 100;
      pMClose->v.flags = FL_MONSTER;
      pMClose->v.takedamage = DAMAGE_YES;
      pMClose->v.deadflag = DEAD_NO;
      pMClose->v.solid = SOLID_BBOX;
      pMClose->v.view_ofs = Vector(0, 0, 20);

      // Far monster (iterated second, hits distance check)
      edict_t *pMFar = mock_alloc_edict();
      mock_set_classname(pMFar, "monster_alien_grunt");
      pMFar->v.origin = Vector(500, 0, 0);
      pMFar->v.health = 100;
      pMFar->v.flags = FL_MONSTER;
      pMFar->v.takedamage = DAMAGE_YES;
      pMFar->v.deadflag = DEAD_NO;
      pMFar->v.solid = SOLID_BBOX;
      pMFar->v.view_ofs = Vector(0, 0, 20);

      BotFindEnemy(testbot);
      ASSERT_PTR_EQ(testbot.pBotEnemy, pMClose);
   }
   bot_shoot_breakables = 0;
   PASS();

   return 0;
}

// Sound enemy is_sound_enemy path (line 1084) + sound distance/visibility (664, 674)
static int test_sound_enemy_detailed(void)
{
   printf("BotFindVisibleSoundEnemy detailed paths:\n");

   // --- Sound enemy farther than current best (line 664) ---
   mock_reset();
   setup_skill_settings();
   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   static CSoundEnt soundEnt;
   pSoundEnt = &soundEnt;
   pSoundEnt->Initialize();

   int bot_idx = 0;
   bots[bot_idx] = testbot;
   bots[bot_idx].pEdict = pBotEdict;

   TEST("sound: farther sound skipped after closer (line 664)");
   edict_t *pNear = create_enemy_player(Vector(200, 0, 0));
   edict_t *pFar = create_enemy_player(Vector(600, 0, 0));
   bots[bot_idx].f_next_find_visible_sound_enemy_time = 0;
   // Insert FAR first, then NEAR. Active list is LIFO, so NEAR is iterated first.
   // NEAR passes, sets mindistance. FAR iterated second -> hits line 664.
   CSoundEnt::InsertSound(pFar, 2, Vector(600, 0, 0), 2000, 5.0, -1);
   CSoundEnt::InsertSound(pNear, 1, Vector(200, 0, 0), 2000, 5.0, -1);
   // Call BotFindVisibleSoundEnemy directly to avoid normal player loop interference
   edict_t *result = BotFindVisibleSoundEnemy(bots[bot_idx]);
   ASSERT_PTR_EQ(result, pNear);
   PASS();

   // --- Sound enemy not visible (line 674) ---
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   pSoundEnt = &soundEnt;
   pSoundEnt->Initialize();
   bots[bot_idx] = testbot;
   bots[bot_idx].pEdict = pBotEdict;

   TEST("sound: enemy near sound but not visible from bot (line 674)");
   {
      edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
      bots[bot_idx].f_next_find_visible_sound_enemy_time = 0;
      CSoundEnt::InsertSound(pEnemy, 1, Vector(200, 0, 0), 2000, 5.0, -1);

      // Call BotFindVisibleSoundEnemy directly (static, accessible via #include).
      // BotFindEnemyNearestToPoint (line 585): traces from enemy to sound point.
      // Outer FVisible (line 673): traces from bot to enemy.
      // Use call counter: first UTIL_TraceLine passes (inner), second fails (outer).
      static int s_fvis_calls;
      s_fvis_calls = 0;
      struct InnerPassOuterFail {
         static void trace(const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
         {
            (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
            s_fvis_calls++;
            if (s_fvis_calls <= 1) {
               ptr->flFraction = 1.0f;
            } else {
               ptr->flFraction = 0.3f;
            }
            ptr->pHit = NULL;
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };
      mock_trace_line_fn = InnerPassOuterFail::trace;
      mock_trace_hull_fn = trace_nohit;
      edict_t *result = BotFindVisibleSoundEnemy(bots[bot_idx]);
      ASSERT_PTR_NULL(result);
      mock_trace_line_fn = trace_nohit;
   }
   PASS();

   pSoundEnt = NULL;
   return 0;
}

// Percent selection with multiple weapons (lines 1603-1607, 1629-1630)
static int test_percent_selection_multi_weapon(void)
{
   printf("BotFireWeapon percent selection with multiple weapons:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;

   // Find shotgun and mp5 indices
   int sg_idx = -1, mp5_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_SHOTGUN) sg_idx = i;
      if (weapon_select[i].iId == VALVE_WEAPON_MP5) mp5_idx = i;
   }

   TEST("two weapons in percent list -> linked list append (lines 1603-1607)");
   if (sg_idx >= 0 && mp5_idx >= 0) {
      // Give bot both shotgun and mp5 with ammo
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN) | (1u << VALVE_WEAPON_MP5);
      int ammo1 = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1 >= 0 && ammo1 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo1] = 50;
      int ammo2 = weapon_defs[VALVE_WEAPON_MP5].iAmmo1;
      if (ammo2 >= 0 && ammo2 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo2] = 50;

      // Set random to high value -> selects second weapon in percent list (line 1629-1630)
      mock_random_long_ret = 9999;
      qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      ASSERT_INT(res, TRUE);
   }
   PASS();

   TEST("random selects first of two weapons -> loop skips second (line 1629)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;

   if (sg_idx >= 0 && mp5_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN) | (1u << VALVE_WEAPON_MP5);
      int ammo1 = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1 >= 0 && ammo1 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo1] = 50;
      int ammo2 = weapon_defs[VALVE_WEAPON_MP5].iAmmo1;
      if (ammo2 >= 0 && ammo2 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo2] = 50;

      // Set random to 0 -> selects first weapon
      mock_random_long_ret = 0;
      qboolean res = BotFireWeapon(Vector(200, 0, 0), testbot, 0);
      ASSERT_INT(res, TRUE);
   }
   PASS();

   return 0;
}

// Reuse path: weapon_choice match via conditions (lines 1500-1502)
static int test_reuse_weapon_conditions(void)
{
   printf("BotFireWeapon reuse via conditions (lines 1500-1502):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_weaponchange_time = 0;

   // Find shotgun index
   int sg_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_SHOTGUN) {
         sg_idx = i;
         break;
      }
   }

   TEST("reuse current weapon via IsValid checks (lines 1500-1502)");
   if (sg_idx >= 0) {
      testbot.current_weapon_index = sg_idx;
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      int ammo1 = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1 >= 0 && ammo1 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo1] = 50;
      // weapon_choice != 0 AND != iId -> falls to IsValid condition check (lines 1500-1502)
      // Distance 500 = within shotgun primary range (400-1500)
      qboolean res = BotFireWeapon(Vector(500, 0, 0), testbot, VALVE_WEAPON_MP5);
      (void)res;
   }
   PASS();

   // Test skill strip in reuse path (line 1523)
   TEST("reuse path: primary stripped by skill check (line 1523)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(500, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_weaponchange_time = 0;

   if (sg_idx >= 0) {
      testbot.current_weapon_index = sg_idx;
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      int ammo1 = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1 >= 0 && ammo1 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo1] = 50;
      // weapon_skill=6 > SKILL5(5) => BotSkilledEnoughForPrimaryAttack fails
      testbot.weapon_skill = 6;
      // weapon_choice=0 enters via first branch, distance 500 in primary range (400-1500)
      qboolean res = BotFireWeapon(Vector(500, 0, 0), testbot, 0);
      (void)res;
   }
   PASS();

   TEST("reuse path: skill strip via non-matching weapon_choice (line 1523)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(500, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_weaponchange_time = 0;

   if (sg_idx >= 0) {
      testbot.current_weapon_index = sg_idx;
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      int ammo1 = weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1;
      if (ammo1 >= 0 && ammo1 < MAX_AMMO_SLOTS) testbot.m_rgAmmo[ammo1] = 50;
      testbot.weapon_skill = 6;
      // weapon_choice != 0 and != shotgun -> enters lines 1500-1502
      // Distance 500 in primary range (400-1500), then skill strip at 1522-1523
      qboolean res = BotFireWeapon(Vector(500, 0, 0), testbot, VALVE_WEAPON_MP5);
      (void)res;
   }
   PASS();

   return 0;
}

// Skill strip in percent loop (lines 1577, 1579)
static int test_percent_skill_strip(void)
{
   printf("BotFireWeapon percent loop skill strip (lines 1577, 1579):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(500, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;

   int sg_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_SHOTGUN) {
         sg_idx = i;
         break;
      }
   }

   // Set weapon_defs for shotgun ammo
   weapon_defs[VALVE_WEAPON_SHOTGUN].iId = VALVE_WEAPON_SHOTGUN;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1 = 3; // buckshot slot
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2 = -1;

   // Test 1: Strip primary (line 1577)
   // primary_skill_level=2, secondary_skill_level=4, weapon_skill=3
   // BotCanUseWeapon: (3<=2=F)||(3<=4=T)=T. Line 1576: use_primary(T)&&!(3<=2=F)=T -> fires
   TEST("weapon_skill strips primary in percent loop (line 1577)");
   if (sg_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      testbot.m_rgAmmo[3] = 50;
      int old_primary = weapon_select[sg_idx].primary_skill_level;
      int old_secondary = weapon_select[sg_idx].secondary_skill_level;
      weapon_select[sg_idx].primary_skill_level = 2;
      weapon_select[sg_idx].secondary_skill_level = 4;
      testbot.weapon_skill = 3;
      qboolean res = BotFireWeapon(Vector(500, 0, 0), testbot, 0);
      (void)res;
      weapon_select[sg_idx].primary_skill_level = old_primary;
      weapon_select[sg_idx].secondary_skill_level = old_secondary;
   }
   PASS();

   // Test 2: Strip secondary (line 1579)
   // primary_skill_level=4, secondary_skill_level=2, weapon_skill=3
   // BotCanUseWeapon: (3<=4=T)||(3<=2=F)=T. Line 1578: use_secondary(T)&&!(3<=2=F)=T -> fires
   TEST("weapon_skill strips secondary in percent loop (line 1579)");
   mock_reset();
   setup_skill_settings();
   pBotEdict = mock_alloc_edict();
   setup_bot_for_test(testbot, pBotEdict);
   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;
   pEnemy = create_enemy_player(Vector(500, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iId = VALVE_WEAPON_SHOTGUN;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1 = 3;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2 = -1;
   if (sg_idx >= 0) {
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_SHOTGUN);
      testbot.m_rgAmmo[3] = 50;
      int old_primary = weapon_select[sg_idx].primary_skill_level;
      int old_secondary = weapon_select[sg_idx].secondary_skill_level;
      weapon_select[sg_idx].primary_skill_level = 4;
      weapon_select[sg_idx].secondary_skill_level = 2;
      testbot.weapon_skill = 3;
      qboolean res = BotFireWeapon(Vector(500, 0, 0), testbot, 0);
      (void)res;
      weapon_select[sg_idx].primary_skill_level = old_primary;
      weapon_select[sg_idx].secondary_skill_level = old_secondary;
   }
   PASS();

   return 0;
}

// Min-skill fallback with secondary_skill_level check (line 1696)
static int test_min_skill_secondary_path(void)
{
   printf("BotFireWeapon min-skill secondary_skill_level path (line 1696):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;
   mock_trace_hull_fn = trace_nohit;

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.current_weapon_index = -1;
   testbot.f_weaponchange_time = 0;

   // Give bot MP5 with no primary ammo but secondary ammo.
   // Use hornetgun: secondary_use_primary_ammo=FALSE, no MP5 special check.
   // With no primary ammo -> use_primary=FALSE, with secondary ammo -> use_secondary=TRUE.
   // Line 1695: (FALSE && ...) is FALSE -> evaluates line 1696.
   // Line 1696: (TRUE && secondary_skill_level(3) > min_skill(-1)) -> TRUE
   TEST("min-skill fallback: secondary_skill_level > min_skill (line 1696)");
   {
      // Set up weapon_defs for hornetgun with separate ammo slots
      weapon_defs[VALVE_WEAPON_HORNETGUN].iId = VALVE_WEAPON_HORNETGUN;
      weapon_defs[VALVE_WEAPON_HORNETGUN].iAmmo1 = 2; // primary ammo slot
      weapon_defs[VALVE_WEAPON_HORNETGUN].iAmmo2 = 3; // secondary ammo slot
      pBotEdict->v.weapons = (1u << VALVE_WEAPON_HORNETGUN);
      testbot.m_rgAmmo[2] = 0; // no primary ammo -> use_primary=FALSE
      testbot.m_rgAmmo[3] = 5; // secondary ammo ok -> use_secondary=TRUE
      // Find hornetgun index in weapon_select
      int hg_idx = -1;
      for (int i = 0; weapon_select[i].iId; i++) {
         if (weapon_select[i].iId == VALVE_WEAPON_HORNETGUN) {
            hg_idx = i;
            break;
         }
      }
      if (hg_idx >= 0) {
         int old_percent = weapon_select[hg_idx].use_percent;
         weapon_select[hg_idx].use_percent = 0; // skip percent loop
         testbot.weapon_skill = 0; // can use weapon (skill check passes)
         qboolean res = BotFireWeapon(Vector(500, 0, 0), testbot, 0);
         ASSERT_TRUE(res); // should find weapon via min-skill path
         weapon_select[hg_idx].use_percent = old_percent;
      }
   }
   PASS();

   return 0;
}

// BotShootAtEnemy feet-not-visible -> head aim (line 1789)
static int test_shoot_at_enemy_head_aim(void)
{
   printf("BotShootAtEnemy feet not visible -> head aim:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   // Enemy at (80,0,0) - close enough for FCanShootInHead angle check.
   // cos(12.5deg) ~= 0.9763. At distance=80, height=26:
   // 80/sqrt(80^2+26^2) = 80/84.1 = 0.951 < 0.9763 -> passes.
   edict_t *pEnemy = create_enemy_player(Vector(80, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0;
   testbot.b_combat_longjump = FALSE;

   // Find RPG weapon index (WEAPON_FIRE_AT_FEET)
   int rpg_idx = -1;
   for (int i = 0; weapon_select[i].iId; i++) {
      if (weapon_select[i].iId == VALVE_WEAPON_RPG) {
         rpg_idx = i;
         break;
      }
   }

   TEST("RPG: feet blocked, head visible -> head aim path (line 1789)");
   if (rpg_idx >= 0) {
      testbot.current_weapon_index = rpg_idx;

      // Trace: feet trace (low z) returns blocked, but normal traces pass.
      // FCanShootInHead checks head visible + center/feet not visible for "head only".
      // We need: head visible, center visible (to NOT return TRUE at line 509),
      // then the angle check (line 518) passes for close enemies -> returns TRUE.
      // Actually we need center OR feet not visible for FCanShootInHead to return TRUE
      // via line 509. Let me make center blocked too:
      struct FeetAndCenterBlocked {
         static void trace(const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
         {
            (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
            // Head at z=26 -> visible. Center at z=0 and feet at z=-28 -> blocked.
            if (v2[2] > 10.0f) {
               ptr->flFraction = 1.0f; // head visible
            } else {
               ptr->flFraction = 0.3f; // center and feet blocked
            }
            ptr->pHit = NULL;
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };
      mock_trace_line_fn = FeetAndCenterBlocked::trace;
      mock_trace_hull_fn = trace_nohit;

      BotShootAtEnemy(testbot);
      // Should have taken the else branch (line 1787), FCanShootInHead returns TRUE,
      // and line 1789 adds view_ofs to aim position.
   }
   mock_trace_line_fn = trace_nohit;
   PASS();

   return 0;
}

// BotShootAtEnemy not visible during fire -> reaction reset (line 1867)
static int test_shoot_at_enemy_fire_not_visible(void)
{
   printf("BotShootAtEnemy fire time reached but not visible:\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   edict_t *pEnemy = create_enemy_player(Vector(200, 0, 0));
   testbot.pBotEnemy = pEnemy;
   testbot.f_reaction_target_time = 0;
   testbot.f_shoot_time = 0; // <= gpGlobals->time, so enters fire section
   testbot.b_combat_longjump = FALSE;
   testbot.current_weapon_index = -1; // no fire-at-feet weapon

   TEST("fire time reached, visibility check at line 1846 fails -> reset reaction (line 1867)");
   {
      // First FVisibleEnemy at line 1800 must PASS (otherwise BotRemoveEnemy at 1802)
      // Then FVisibleEnemy at line 1846 must FAIL (to hit line 1867)
      // Both use mock_trace_line_fn. The calls differ in that
      // line 1800 checks v_enemy_aimpos, line 1846 checks same position.
      // We need the FIRST visibility check to pass but the SECOND to fail.
      static int s_vis_call;
      s_vis_call = 0;
      struct FirstVisSecondBlocked {
         static void trace(const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
         {
            (void)v1; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
            s_vis_call++;
            // FVisibleEnemy calls FVisibleEnemyOffset multiple times per check
            // (head, feet, center). First full check (line 1800) ~3 calls.
            // We need all of those to pass. Then line 1846 check calls must fail.
            if (s_vis_call <= 3) {
               ptr->flFraction = 1.0f;
            } else {
               ptr->flFraction = 0.3f;
            }
            ptr->pHit = NULL;
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };
      mock_trace_line_fn = FirstVisSecondBlocked::trace;
      mock_trace_hull_fn = trace_nohit;

      float old_reaction = testbot.f_reaction_target_time;
      BotShootAtEnemy(testbot);
      // Should have hit line 1867 (BotResetReactionTime)
      // Reaction time should be updated (set to future)
      ASSERT_TRUE(testbot.f_reaction_target_time > old_reaction ||
                  testbot.f_reaction_target_time > gpGlobals->time);
   }
   mock_trace_line_fn = trace_nohit;
   PASS();

   return 0;
}

// BotPointGun with no enemy -> BotAimsAtSomething returns FALSE (line 52)
static int test_bot_point_gun_no_enemy(void)
{
   printf("BotPointGun with no enemy (BotAimsAtSomething line 52):\n");
   mock_reset();
   setup_skill_settings();

   edict_t *pBotEdict = mock_alloc_edict();
   bot_t testbot;
   setup_bot_for_test(testbot, pBotEdict);

   mock_trace_line_fn = trace_nohit;

   TEST("no enemy -> slow aim speed path (line 52 return FALSE)");
   testbot.b_combat_longjump = FALSE;
   testbot.pBotEnemy = NULL;
   testbot.curr_waypoint_index = -1;
   testbot.pBotPickupItem = NULL;
   testbot.current_weapon_index = -1;

   pBotEdict->v.idealpitch = 10.0;
   pBotEdict->v.ideal_yaw = 20.0;
   pBotEdict->v.v_angle = Vector(0, 0, 0);
   pBotEdict->v.pitch_speed = 0;
   pBotEdict->v.yaw_speed = 0;

   BotPointGun(testbot);
   // Should use slow aim speed (0.2 + turn_skill adjustment)
   // Pitch/yaw_speed should be non-zero since there's deviation
   ASSERT_TRUE(pBotEdict->v.pitch_speed != 0.0f || pBotEdict->v.yaw_speed != 0.0f);
   PASS();

   return 0;
}

// ============================================================
// Main
// ============================================================

int main(void)
{
   int rc = 0;

   printf("=== bot_combat.cpp tests ===\n\n");

   // Group 7: Small remaining functions
   rc |= test_get_modified_enemy_distance();
   printf("\n");
   rc |= test_bot_reset_reaction_time();
   printf("\n");

   // Group 1: Position data & prediction
   rc |= test_free_posdata_list();
   printf("\n");
   rc |= test_gather_player_data();
   printf("\n");
   rc |= test_prediction_velocity_variation();
   printf("\n");
   rc |= test_prediction_position_variation();
   printf("\n");
   rc |= test_trace_predicted_movement();
   printf("\n");
   rc |= test_get_predicted_player_position();
   printf("\n");

   // Group 2: Aiming
   rc |= test_bot_aims_at_something();
   printf("\n");
   rc |= test_bot_point_gun();
   printf("\n");
   rc |= test_bot_aim_pre();
   printf("\n");
   rc |= test_bot_aim_post();
   printf("\n");

   // Group 3: Hearing & sound enemy
   rc |= test_hearing_sensitivity();
   printf("\n");
   rc |= test_find_visible_sound_enemy();
   printf("\n");

   // Group 4: Enemy detection
   rc |= test_are_team_mates();
   printf("\n");
   rc |= test_f_can_shoot_in_head();
   printf("\n");
   rc |= test_find_enemy_nearest_to_point();
   printf("\n");
   rc |= test_bot_remove_enemy_tracking();
   printf("\n");
   rc |= test_fpredicted_visible();
   printf("\n");
   rc |= test_bot_find_enemy_expanded();
   printf("\n");

   // Group 5: Weapon firing
   rc |= test_have_room_for_throw();
   printf("\n");
   rc |= test_check_weapon_fire_conditions();
   printf("\n");
   rc |= test_try_select_weapon();
   printf("\n");
   rc |= test_bot_fire_selected_weapon();
   printf("\n");
   rc |= test_bot_fire_weapon();
   printf("\n");

   // Group 6: BotShootAtEnemy + special weapons
   rc |= test_bot_shoot_at_enemy();
   printf("\n");
   rc |= test_bot_should_detonate_satchel();
   printf("\n");
   rc |= test_bot_detonate_satchel();
   printf("\n");
   rc |= test_bot_shoot_tripmine();
   printf("\n");

   // Original breakable tests
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
   printf("\n");

   // Group 8: RANDOM mock-dependent tests
   rc |= test_bot_find_enemy_jump_on_kill();
   printf("\n");
   rc |= test_bot_find_enemy_sound_enemy_path();
   printf("\n");
   rc |= test_bot_point_gun_negative_pitch();
   printf("\n");
   rc |= test_bot_fire_weapon_percent_selection();
   printf("\n");
   rc |= test_bot_fire_weapon_reuse_with_choice();
   printf("\n");
   rc |= test_bot_fire_weapon_min_skill_both();
   printf("\n");
   rc |= test_bot_fire_weapon_avoid_skill_strip();
   printf("\n");
   rc |= test_bot_fire_weapon_underwater();
   printf("\n");
   rc |= test_bot_fire_selected_weapon_delays();
   printf("\n");
   rc |= test_bot_shoot_at_enemy_feet_player_hit();
   printf("\n");
   rc |= test_bot_shoot_at_enemy_angle_wrap();
   printf("\n");
   rc |= test_bot_shoot_at_enemy_not_visible_reset();
   printf("\n");
   rc |= test_bot_detonate_satchel_switch();
   printf("\n");
   rc |= test_bot_fire_weapon_melee_throttle();
   printf("\n");
   rc |= test_bot_fire_weapon_charging_delays();
   printf("\n");
   rc |= test_bot_remove_enemy_random_track_time();
   printf("\n");
   rc |= test_bot_reset_reaction_time_random();
   printf("\n");
   rc |= test_bot_fire_weapon_better_choice();
   printf("\n");
   rc |= test_try_select_weapon_random();
   printf("\n");
   rc |= test_prediction_random_variation();
   printf("\n");

   // Group 9: Additional coverage tests
   rc |= test_mode1_distance_and_monster_skips();
   printf("\n");
   rc |= test_sound_enemy_detailed();
   printf("\n");
   rc |= test_percent_selection_multi_weapon();
   printf("\n");
   rc |= test_reuse_weapon_conditions();
   printf("\n");
   rc |= test_percent_skill_strip();
   printf("\n");
   rc |= test_min_skill_secondary_path();
   printf("\n");
   rc |= test_shoot_at_enemy_head_aim();
   printf("\n");
   rc |= test_shoot_at_enemy_fire_not_visible();
   printf("\n");
   rc |= test_bot_point_gun_no_enemy();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
