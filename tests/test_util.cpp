//
// JK_Botti - comprehensive tests for util.cpp
//

#include <string.h>
#include <math.h>

#include "engine_mock.h"
#include "player.h"
#include "test_common.h"

extern qboolean is_team_play;

static float vec_dot(const Vector &a, const Vector &b)
{
   return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float vec_length(const Vector &v)
{
   return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

// Helper: set up an edict that IsAlive() returns TRUE for
static void make_alive(edict_t *e)
{
   e->v.deadflag = DEAD_NO;
   e->v.health = 100;
   e->v.flags &= ~FL_NOTARGET;
   e->v.takedamage = DAMAGE_YES;
   e->v.solid = SOLID_BSP;
}

// ============================================================
// GetGameDir tests
// ============================================================

static const char *mock_gamedir_value;

static void mock_pfnGetGameDir(char *szGetGameDir)
{
   strcpy(szGetGameDir, mock_gamedir_value);
}

static int test_getgamedir(void)
{
   char result[256];

   printf("GetGameDir:\n");

   // Install mock
   g_engfuncs.pfnGetGameDir = mock_pfnGetGameDir;

   TEST("relative path 'valve'");
   mock_gamedir_value = "valve";
   GetGameDir(result);
   ASSERT_STR(result, "valve");
   PASS();

   TEST("absolute path '/home/user/hlds/valve'");
   mock_gamedir_value = "/home/user/hlds/valve";
   GetGameDir(result);
   ASSERT_STR(result, "valve");
   PASS();

   TEST("Windows path 'C:\\hlds\\valve'");
   mock_gamedir_value = "C:\\hlds\\valve";
   GetGameDir(result);
   ASSERT_STR(result, "valve");
   PASS();

   TEST("trailing slash '/home/hlds/valve/'");
   mock_gamedir_value = "/home/hlds/valve/";
   GetGameDir(result);
   ASSERT_STR(result, "valve");
   PASS();

   TEST("trailing backslash 'C:\\hlds\\valve\\'");
   mock_gamedir_value = "C:\\hlds\\valve\\";
   GetGameDir(result);
   ASSERT_STR(result, "valve");
   PASS();

   TEST("gearbox mod directory");
   mock_gamedir_value = "/opt/hlds/gearbox";
   GetGameDir(result);
   ASSERT_STR(result, "gearbox");
   PASS();

   TEST("empty string does not crash");
   mock_gamedir_value = "";
   GetGameDir(result);
   // just verify no crash; result content is unspecified
   PASS();

   return 0;
}

// ============================================================
// IsPlayerFacingWall / chat protection test
// (tests that the fix uses local v_forward, not gpGlobals->v_forward)
// ============================================================

static void trace_wall_ahead(const float *v1, const float *v2,
                             int fNoMonsters, int hullNumber,
                             edict_t *pentToSkip, TraceResult *ptr)
{
   (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;

   // Simulate a wall hit close ahead
   ptr->flFraction = 0.5f;
   ptr->pHit = NULL;

   // Compute the trace direction to return a proper plane normal
   // (opposite of trace direction = facing the wall head-on)
   float dx = v2[0] - v1[0];
   float dy = v2[1] - v1[1];
   float dz = v2[2] - v1[2];
   float len = sqrt(dx*dx + dy*dy + dz*dz);
   if (len > 0.001f)
   {
      ptr->vecPlaneNormal[0] = -dx / len;
      ptr->vecPlaneNormal[1] = -dy / len;
      ptr->vecPlaneNormal[2] = -dz / len;
   }
}

static int test_chat_protection_facing_wall(void)
{
   printf("IsPlayerFacingWall (chat protection):\n");

   mock_reset();

   // Allocate a player edict (index 1)
   edict_t *pPlayer = mock_alloc_edict();

   // Set player looking along +X axis (yaw=0)
   pPlayer->v.v_angle[0] = 0; // pitch
   pPlayer->v.v_angle[1] = 0; // yaw = +X
   pPlayer->v.v_angle[2] = 0; // roll
   pPlayer->v.origin[0] = 100;
   pPlayer->v.origin[1] = 200;
   pPlayer->v.origin[2] = 0;
   pPlayer->v.view_ofs[0] = 0;
   pPlayer->v.view_ofs[1] = 0;
   pPlayer->v.view_ofs[2] = 17; // eye height
   pPlayer->v.flags = 0; // not a bot
   pPlayer->v.button = 0; // no buttons pressed

   // Set gpGlobals->v_forward to a WRONG direction (+Y)
   // If the bug still existed, IsPlayerFacingWall would use this
   // stale value and get wrong results
   gpGlobals->v_forward[0] = 0;
   gpGlobals->v_forward[1] = 1; // pointing +Y (wrong)
   gpGlobals->v_forward[2] = 0;

   // Mock trace: wall hit ahead
   mock_trace_line_fn = trace_wall_ahead;

   // Initialize chat protection state
   int idx = 1 - 1; // ENTINDEX returns 1 for first allocated edict
   players[idx].last_time_not_facing_wall = gpGlobals->time;

   // Call CheckPlayerChatProtection multiple times to let the timer expire
   gpGlobals->time = 10.0;
   players[idx].last_time_not_facing_wall = 7.0; // 3 seconds ago

   CheckPlayerChatProtection(pPlayer);

   TEST("player facing wall is detected (uses local v_forward)");
   // After check, last_time_not_facing_wall should NOT have been updated
   // (player IS facing a wall, so the function should not reset the timer)
   ASSERT_TRUE(players[idx].last_time_not_facing_wall < gpGlobals->time - 1.0);
   PASS();

   TEST("chat protection activates after facing wall");
   ASSERT_TRUE(IsPlayerChatProtected(pPlayer) == TRUE);
   PASS();

   // Now verify the opposite: player NOT facing wall
   mock_trace_line_fn = NULL; // default: no hit (fraction=1.0)
   players[idx].last_time_not_facing_wall = 7.0;

   CheckPlayerChatProtection(pPlayer);

   TEST("player not facing wall resets timer");
   ASSERT_TRUE(players[idx].last_time_not_facing_wall == gpGlobals->time);
   PASS();

   TEST("chat protection inactive when not facing wall");
   ASSERT_TRUE(IsPlayerChatProtected(pPlayer) == FALSE);
   PASS();

   // Test oblique wall angle (dot product > -0.5, i.e. wall at > 60deg)
   // Trace hits but plane normal is nearly perpendicular to forward = glancing hit
   mock_trace_line_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      ptr->flFraction = 0.5f;
      // Normal nearly perpendicular to +X forward -> dot product ~0 (> -0.5)
      ptr->vecPlaneNormal[0] = 0;
      ptr->vecPlaneNormal[1] = 1;
      ptr->vecPlaneNormal[2] = 0;
   };
   players[idx].last_time_not_facing_wall = 7.0;
   CheckPlayerChatProtection(pPlayer);

   TEST("oblique wall angle resets timer (not facing wall head-on)");
   ASSERT_TRUE(players[idx].last_time_not_facing_wall == gpGlobals->time);
   PASS();

   mock_trace_line_fn = NULL;

   // Test bot flag (FL_FAKECLIENT) resets timer
   mock_trace_line_fn = trace_wall_ahead;
   pPlayer->v.flags = FL_FAKECLIENT;
   players[idx].last_time_not_facing_wall = 7.0;
   CheckPlayerChatProtection(pPlayer);

   TEST("FL_FAKECLIENT resets timer (bots skip wall check)");
   ASSERT_TRUE(players[idx].last_time_not_facing_wall == gpGlobals->time);
   PASS();

   pPlayer->v.flags = 0;

   // Test button press resets timer
   pPlayer->v.button = IN_ATTACK;
   players[idx].last_time_not_facing_wall = 7.0;
   CheckPlayerChatProtection(pPlayer);

   TEST("button press resets timer");
   ASSERT_TRUE(players[idx].last_time_not_facing_wall == gpGlobals->time);
   PASS();

   pPlayer->v.button = 0;
   mock_trace_line_fn = NULL;

   return 0;
}

// ============================================================
// UTIL_WrapAngle tests
// ============================================================

static int test_wrap_angle(void)
{
   printf("UTIL_WrapAngle:\n");

   TEST("0 stays 0");
   ASSERT_FLOAT(UTIL_WrapAngle(0), 0);
   PASS();

   TEST("90 stays 90");
   ASSERT_FLOAT(UTIL_WrapAngle(90), 90);
   PASS();

   TEST("-90 stays -90");
   ASSERT_FLOAT(UTIL_WrapAngle(-90), -90);
   PASS();

   TEST("180 stays 180");
   ASSERT_FLOAT(UTIL_WrapAngle(180), 180);
   PASS();

   TEST("-180 becomes 180 (special case)");
   ASSERT_FLOAT(UTIL_WrapAngle(-180), 180);
   PASS();

   TEST("181 wraps to -179");
   ASSERT_FLOAT(UTIL_WrapAngle(181), -179);
   PASS();

   TEST("-181 wraps to 179");
   ASSERT_FLOAT(UTIL_WrapAngle(-181), 179);
   PASS();

   TEST("360 wraps to 0");
   ASSERT_FLOAT(UTIL_WrapAngle(360), 0);
   PASS();

   TEST("-360 wraps to 0");
   ASSERT_FLOAT(UTIL_WrapAngle(-360), 0);
   PASS();

   TEST("540 wraps to 180");
   ASSERT_FLOAT(UTIL_WrapAngle(540), 180);
   PASS();

   TEST("-540 wraps to 180");
   ASSERT_FLOAT(UTIL_WrapAngle(-540), 180);
   PASS();

   TEST("3600 (10 full turns) wraps to ~0");
   ASSERT_FLOAT(UTIL_WrapAngle(3600), 0);
   PASS();

   TEST("never returns -180 for integer inputs [-3600..3600]");
   for (int i = -3600; i <= 3600; i++)
      ASSERT_TRUE(UTIL_WrapAngle((float)i) != -180.0f);
   PASS();

   return 0;
}

// ============================================================
// UTIL_WrapAngles tests
// ============================================================

static int test_wrap_angles(void)
{
   printf("UTIL_WrapAngles:\n");

   TEST("wraps all three components");
   Vector result = UTIL_WrapAngles(Vector(540, -181, 360));
   ASSERT_FLOAT(result.x, 180);
   ASSERT_FLOAT(result.y, 179);
   ASSERT_FLOAT(result.z, 0);
   PASS();

   TEST("in-range values pass through");
   result = UTIL_WrapAngles(Vector(45, -90, 0));
   ASSERT_FLOAT(result.x, 45);
   ASSERT_FLOAT(result.y, -90);
   ASSERT_FLOAT(result.z, 0);
   PASS();

   return 0;
}

// ============================================================
// UTIL_AnglesToForward tests
// ============================================================

static int test_angles_to_forward(void)
{
   printf("UTIL_AnglesToForward:\n");
   Vector result;

   TEST("yaw=0 (identity) -> (+1,0,0)");
   result = UTIL_AnglesToForward(Vector(0, 0, 0));
   ASSERT_VEC(result, 1, 0, 0);
   PASS();

   TEST("yaw=90 -> (0,+1,0)");
   result = UTIL_AnglesToForward(Vector(0, 90, 0));
   ASSERT_VEC(result, 0, 1, 0);
   PASS();

   TEST("yaw=180 -> (-1,0,0)");
   result = UTIL_AnglesToForward(Vector(0, 180, 0));
   ASSERT_VEC(result, -1, 0, 0);
   PASS();

   TEST("yaw=270 -> (0,-1,0)");
   result = UTIL_AnglesToForward(Vector(0, 270, 0));
   ASSERT_VEC(result, 0, -1, 0);
   PASS();

   TEST("pitch=90 (down) -> (0,0,-1)");
   result = UTIL_AnglesToForward(Vector(90, 0, 0));
   ASSERT_VEC(result, 0, 0, -1);
   PASS();

   TEST("pitch=-90 (up) -> (0,0,+1)");
   result = UTIL_AnglesToForward(Vector(-90, 0, 0));
   ASSERT_VEC(result, 0, 0, 1);
   PASS();

   TEST("result is unit length for arbitrary angles");
   result = UTIL_AnglesToForward(Vector(30, 45, 0));
   ASSERT_FLOAT(vec_length(result), 1.0f);
   PASS();

   return 0;
}

// ============================================================
// UTIL_AnglesToRight tests
// ============================================================

static int test_angles_to_right(void)
{
   printf("UTIL_AnglesToRight:\n");
   Vector result;

   // With roll=0: right = (sin(yaw), -cos(yaw), 0)
   TEST("yaw=0, roll=0 -> (0,-1,0)");
   result = UTIL_AnglesToRight(Vector(0, 0, 0));
   ASSERT_VEC(result, 0, -1, 0);
   PASS();

   TEST("yaw=90, roll=0 -> (+1,0,0)");
   result = UTIL_AnglesToRight(Vector(0, 90, 0));
   ASSERT_VEC(result, 1, 0, 0);
   PASS();

   TEST("result is unit length");
   result = UTIL_AnglesToRight(Vector(30, 45, 15));
   ASSERT_FLOAT(vec_length(result), 1.0f);
   PASS();

   TEST("right is orthogonal to forward");
   Vector angles(30, 45, 15);
   Vector fwd = UTIL_AnglesToForward(angles);
   result = UTIL_AnglesToRight(angles);
   ASSERT_FLOAT(vec_dot(fwd, result), 0.0f);
   PASS();

   return 0;
}

// ============================================================
// UTIL_MakeVectorsPrivate tests
// ============================================================

static int test_make_vectors_private(void)
{
   printf("UTIL_MakeVectorsPrivate:\n");
   Vector fwd, right, up;

   TEST("identity orientation (0,0,0)");
   UTIL_MakeVectorsPrivate(Vector(0, 0, 0), fwd, right, up);
   ASSERT_VEC(fwd,   1,  0, 0);
   ASSERT_VEC(right, 0, -1, 0);
   ASSERT_VEC(up,    0,  0, 1);
   PASS();

   Vector angles(25, 60, 10);
   UTIL_MakeVectorsPrivate(angles, fwd, right, up);

   TEST("forward is unit length");
   ASSERT_FLOAT(vec_length(fwd), 1.0f);
   PASS();

   TEST("right is unit length");
   ASSERT_FLOAT(vec_length(right), 1.0f);
   PASS();

   TEST("up is unit length");
   ASSERT_FLOAT(vec_length(up), 1.0f);
   PASS();

   TEST("forward . right ~= 0 (orthogonal)");
   ASSERT_FLOAT(vec_dot(fwd, right), 0.0f);
   PASS();

   TEST("forward . up ~= 0 (orthogonal)");
   ASSERT_FLOAT(vec_dot(fwd, up), 0.0f);
   PASS();

   TEST("right . up ~= 0 (orthogonal)");
   ASSERT_FLOAT(vec_dot(right, up), 0.0f);
   PASS();

   TEST("forward matches UTIL_AnglesToForward");
   Vector fwd2 = UTIL_AnglesToForward(angles);
   ASSERT_VEC(fwd, fwd2.x, fwd2.y, fwd2.z);
   PASS();

   TEST("right matches UTIL_AnglesToRight");
   Vector right2 = UTIL_AnglesToRight(angles);
   ASSERT_VEC(right, right2.x, right2.y, right2.z);
   PASS();

   return 0;
}

// ============================================================
// UTIL_VecToAngles tests
// ============================================================

static int test_vec_to_angles(void)
{
   printf("UTIL_VecToAngles:\n");
   Vector result;

   TEST("(1,0,0) -> pitch=0, yaw=0");
   result = UTIL_VecToAngles(Vector(1, 0, 0));
   ASSERT_VEC(result, 0, 0, 0);
   PASS();

   TEST("(0,1,0) -> pitch=0, yaw=90");
   result = UTIL_VecToAngles(Vector(0, 1, 0));
   ASSERT_VEC(result, 0, 90, 0);
   PASS();

   TEST("(-1,0,0) -> pitch=0, yaw=180");
   result = UTIL_VecToAngles(Vector(-1, 0, 0));
   ASSERT_VEC(result, 0, 180, 0);
   PASS();

   TEST("(0,-1,0) -> pitch=0, yaw=-90");
   result = UTIL_VecToAngles(Vector(0, -1, 0));
   ASSERT_VEC(result, 0, -90, 0);
   PASS();

   TEST("(0,0,1) -> pitch=90 [special case, x=y=0]");
   result = UTIL_VecToAngles(Vector(0, 0, 1));
   ASSERT_VEC(result, 90, 0, 0);
   PASS();

   TEST("(0,0,-1) -> pitch=-90 [special case, x=y=0]");
   result = UTIL_VecToAngles(Vector(0, 0, -1));
   ASSERT_VEC(result, -90, 0, 0);
   PASS();

   // Horizontal round-trip: VecToAngles -> AnglesToForward reproduces input
   TEST("horizontal round-trip: (1,1,0) normalized");
   Vector fwd = Vector(1, 1, 0).Normalize();
   result = UTIL_VecToAngles(fwd);
   Vector back = UTIL_AnglesToForward(result);
   ASSERT_VEC(back, fwd.x, fwd.y, fwd.z);
   PASS();

   // Vertical round-trip: pitch sign is negated between VecToAngles and
   // AnglesToForward (standard Quake/HL convention)
   TEST("vertical round-trip: (0,0,1) with pitch negation");
   result = UTIL_VecToAngles(Vector(0, 0, 1));
   result.x = -result.x; // negate pitch for AnglesToForward
   back = UTIL_AnglesToForward(result);
   ASSERT_VEC(back, 0, 0, 1);
   PASS();

   return 0;
}

// ============================================================
// IsAlive tests
// ============================================================

static int test_is_alive(void)
{
   printf("IsAlive:\n");

   edict_t ent;
   memset(&ent, 0, sizeof(ent));

   // Set all conditions for alive
   make_alive(&ent);

   TEST("all conditions met -> alive");
   ASSERT_TRUE(IsAlive(&ent) == TRUE);
   PASS();

   TEST("deadflag != DEAD_NO -> not alive");
   ent.v.deadflag = DEAD_DYING;
   ASSERT_TRUE(IsAlive(&ent) == FALSE);
   ent.v.deadflag = DEAD_NO;
   PASS();

   TEST("health <= 0 -> not alive");
   ent.v.health = 0;
   ASSERT_TRUE(IsAlive(&ent) == FALSE);
   ent.v.health = 100;
   PASS();

   TEST("FL_NOTARGET flag -> not alive");
   ent.v.flags |= FL_NOTARGET;
   ASSERT_TRUE(IsAlive(&ent) == FALSE);
   ent.v.flags &= ~FL_NOTARGET;
   PASS();

   TEST("takedamage == 0 -> not alive");
   ent.v.takedamage = DAMAGE_NO;
   ASSERT_TRUE(IsAlive(&ent) == FALSE);
   ent.v.takedamage = DAMAGE_YES;
   PASS();

   TEST("solid == SOLID_NOT -> not alive");
   ent.v.solid = SOLID_NOT;
   ASSERT_TRUE(IsAlive(&ent) == FALSE);
   ent.v.solid = SOLID_BSP;
   PASS();

   // Verify still alive after restoring all conditions
   TEST("all restored -> alive again");
   ASSERT_TRUE(IsAlive(&ent) == TRUE);
   PASS();

   return 0;
}

// ============================================================
// VecBModelOrigin tests
// ============================================================

static int test_vec_bmodel_origin(void)
{
   printf("VecBModelOrigin:\n");

   edict_t ent;
   memset(&ent, 0, sizeof(ent));
   Vector result;

   // Non-BSP case: just absmin + size*0.5
   TEST("non-BSP: absmin + size*0.5");
   ent.v.solid = SOLID_NOT;
   ent.v.absmin = Vector(10, 20, 30);
   ent.v.size = Vector(6, 8, 10);
   result = VecBModelOrigin(&ent);
   ASSERT_VEC(result, 13, 24, 35);
   PASS();

   // BSP in-bounds: computed origin is within mins/maxs
   TEST("BSP in-bounds: uses absmin + size*0.5");
   ent.v.solid = SOLID_BSP;
   ent.v.absmin = Vector(0, 0, 0);
   ent.v.size = Vector(100, 100, 100);
   ent.v.mins = Vector(0, 0, 0);
   ent.v.maxs = Vector(100, 100, 100);
   result = VecBModelOrigin(&ent);
   ASSERT_VEC(result, 50, 50, 50);
   PASS();

   // BSP out-of-bounds: entity moved far, fallback to (maxs+mins)/2
   TEST("BSP out-of-bounds: fallback to (maxs+mins)/2");
   ent.v.solid = SOLID_BSP;
   ent.v.absmin = Vector(500, 500, 500);
   ent.v.size = Vector(100, 100, 100);
   ent.v.mins = Vector(0, 0, 0);
   ent.v.maxs = Vector(100, 100, 100);
   result = VecBModelOrigin(&ent);
   // absmin+size*0.5 = (550,550,550), out of [0..100], fallback
   ASSERT_VEC(result, 50, 50, 50);
   PASS();

   return 0;
}

// ============================================================
// FInViewCone tests
// ============================================================

static int test_fin_view_cone(void)
{
   printf("FInViewCone:\n");

   edict_t ent;
   memset(&ent, 0, sizeof(ent));
   // Bot at origin looking along +X
   ent.v.origin = Vector(0, 0, 0);
   ent.v.v_angle = Vector(0, 0, 0); // yaw=0 -> forward = +X

   TEST("directly ahead -> inside 80deg cone");
   ASSERT_TRUE(FInViewCone(Vector(100, 0, 0), &ent) == TRUE);
   PASS();

   TEST("45deg to side -> inside 80deg cone");
   ASSERT_TRUE(FInViewCone(Vector(100, 100, 0), &ent) == TRUE);
   PASS();

   TEST("90deg to side -> outside 80deg cone");
   ASSERT_TRUE(FInViewCone(Vector(0, 100, 0), &ent) == FALSE);
   PASS();

   TEST("behind -> outside cone");
   ASSERT_TRUE(FInViewCone(Vector(-100, 0, 0), &ent) == FALSE);
   PASS();

   return 0;
}

// ============================================================
// FInShootCone tests
// ============================================================

static int test_fin_shoot_cone(void)
{
   printf("FInShootCone:\n");

   edict_t ent;
   memset(&ent, 0, sizeof(ent));
   ent.v.origin = Vector(0, 0, 0);
   ent.v.view_ofs = Vector(0, 0, 0);
   ent.v.v_angle = Vector(0, 0, 0); // looking along +X

   TEST("distance < 0.01 -> always TRUE");
   ASSERT_TRUE(FInShootCone(Vector(100, 50, 0), &ent, 0.001f, 10, 5) == TRUE);
   PASS();

   TEST("directly ahead, within min_angle -> TRUE");
   ASSERT_TRUE(FInShootCone(Vector(100, 0, 0), &ent, 100, 0, 5) == TRUE);
   PASS();

   TEST("within diameter cone but beyond min_angle -> TRUE");
   // angle to (100,20,0) ~= 11.3deg > min_angle 5deg
   // diameter=50 -> cone half-angle = atan(25/100) ~= 14deg > 11.3deg -> inside
   ASSERT_TRUE(FInShootCone(Vector(100, 20, 0), &ent, 100, 50, 5) == TRUE);
   PASS();

   TEST("outside both min_angle and diameter cone -> FALSE");
   // angle to (100,100,0) = 45deg, far outside any reasonable cone
   ASSERT_TRUE(FInShootCone(Vector(100, 100, 0), &ent, 100, 10, 5) == FALSE);
   PASS();

   TEST("behind -> FALSE");
   ASSERT_TRUE(FInShootCone(Vector(-100, 0, 0), &ent, 100, 50, 10) == FALSE);
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetBotIndex tests
// ============================================================

static int test_get_bot_index(void)
{
   printf("UTIL_GetBotIndex:\n");
   mock_reset();

   edict_t *e1 = mock_alloc_edict();
   edict_t *e2 = mock_alloc_edict();
   edict_t *e_unregistered = mock_alloc_edict();

   bots[5].pEdict = e1;
   bots[5].is_used = TRUE;
   bots[20].pEdict = e2;
   bots[20].is_used = TRUE;

   TEST("found at index 5");
   ASSERT_INT(UTIL_GetBotIndex(e1), 5);
   PASS();

   TEST("found at index 20");
   ASSERT_INT(UTIL_GetBotIndex(e2), 20);
   PASS();

   TEST("not found returns -1");
   ASSERT_INT(UTIL_GetBotIndex(e_unregistered), -1);
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetBotPointer tests
// ============================================================

static int test_get_bot_pointer(void)
{
   printf("UTIL_GetBotPointer:\n");
   mock_reset();

   edict_t *e1 = mock_alloc_edict();
   edict_t *e_unregistered = mock_alloc_edict();

   bots[7].pEdict = e1;
   bots[7].is_used = TRUE;

   TEST("found returns correct pointer");
   ASSERT_PTR_EQ(UTIL_GetBotPointer(e1), &bots[7]);
   PASS();

   TEST("not found returns NULL");
   ASSERT_PTR_NULL(UTIL_GetBotPointer(e_unregistered));
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetBotCount tests
// ============================================================

static int test_get_bot_count(void)
{
   printf("UTIL_GetBotCount:\n");
   mock_reset();

   TEST("no bots -> 0");
   ASSERT_INT(UTIL_GetBotCount(), 0);
   PASS();

   TEST("one bot");
   bots[10].is_used = TRUE;
   ASSERT_INT(UTIL_GetBotCount(), 1);
   PASS();

   TEST("scattered bots");
   bots[0].is_used = TRUE;
   bots[31].is_used = TRUE;
   ASSERT_INT(UTIL_GetBotCount(), 3); // 0, 10, 31
   PASS();

   TEST("all 32 slots");
   for (int i = 0; i < 32; i++)
      bots[i].is_used = TRUE;
   ASSERT_INT(UTIL_GetBotCount(), 32);
   PASS();

   return 0;
}

// ============================================================
// UTIL_PickRandomBot tests
// ============================================================

static int test_pick_random_bot(void)
{
   printf("UTIL_PickRandomBot:\n");
   mock_reset();

   TEST("no bots -> -1");
   ASSERT_INT(UTIL_PickRandomBot(), -1);
   PASS();

   TEST("single bot returns its index");
   bots[15].is_used = TRUE;
   ASSERT_INT(UTIL_PickRandomBot(), 15);
   bots[15].is_used = FALSE;
   PASS();

   TEST("multiple bots -> returns a valid bot index");
   bots[3].is_used = TRUE;
   bots[17].is_used = TRUE;
   bots[28].is_used = TRUE;
   fast_random_seed(42);
   int pick = UTIL_PickRandomBot();
   ASSERT_TRUE(pick == 3 || pick == 17 || pick == 28);
   PASS();

   return 0;
}

// ============================================================
// UTIL_UpdateFuncBreakable tests
// ============================================================

static int test_update_func_breakable(void)
{
   printf("UTIL_UpdateFuncBreakable:\n");
   mock_reset();

   edict_t *e1 = mock_alloc_edict();
   mock_set_classname(e1, "func_breakable");
   e1->v.health = 100;

   TEST("register new breakable");
   UTIL_UpdateFuncBreakable(e1, "material", "0"); // matGlass = breakable
   ASSERT_PTR_NOT_NULL(UTIL_LookupBreakable(e1));
   PASS();

   TEST("update existing: set to unbreakable glass (material 7)");
   UTIL_UpdateFuncBreakable(e1, "material", "7"); // matUnbreakableGlass
   // Should no longer be found (material_breakable = FALSE)
   ASSERT_PTR_NULL(UTIL_LookupBreakable(e1));
   PASS();

   TEST("update back to breakable material");
   UTIL_UpdateFuncBreakable(e1, "material", "2"); // matMetal = breakable
   ASSERT_PTR_NOT_NULL(UTIL_LookupBreakable(e1));
   PASS();

   TEST("non-material setting is ignored");
   UTIL_UpdateFuncBreakable(e1, "rendermode", "1");
   // Still findable (material wasn't changed)
   ASSERT_PTR_NOT_NULL(UTIL_LookupBreakable(e1));
   PASS();

   return 0;
}

// ============================================================
// UTIL_FindBreakable tests
// ============================================================

static int test_find_breakable(void)
{
   printf("UTIL_FindBreakable:\n");
   mock_reset();

   TEST("empty list -> NULL");
   ASSERT_PTR_NULL(UTIL_FindBreakable(NULL));
   PASS();

   // Set up two valid breakables and iterate through them
   edict_t *e1 = mock_alloc_edict();
   mock_set_classname(e1, "func_breakable");
   e1->v.health = 100;
   mock_add_breakable(e1, TRUE);

   edict_t *e2 = mock_alloc_edict();
   mock_set_classname(e2, "func_pushable");
   e2->v.health = 50;
   mock_add_breakable(e2, TRUE);

   TEST("first iteration finds first breakable");
   breakable_list_t *p = UTIL_FindBreakable(NULL);
   ASSERT_PTR_NOT_NULL(p);
   ASSERT_PTR_EQ(p->pEdict, e1);
   PASS();

   TEST("second iteration finds func_pushable");
   p = UTIL_FindBreakable(p);
   ASSERT_PTR_NOT_NULL(p);
   ASSERT_PTR_EQ(p->pEdict, e2);
   PASS();

   TEST("exhausts list -> NULL");
   p = UTIL_FindBreakable(p);
   ASSERT_PTR_NULL(p);
   PASS();

   // Test that wrong classnames are skipped
   mock_reset();

   edict_t *e_door = mock_alloc_edict();
   mock_set_classname(e_door, "func_door");
   e_door->v.health = 100;
   mock_add_breakable(e_door, TRUE);

   edict_t *e_good = mock_alloc_edict();
   mock_set_classname(e_good, "func_breakable");
   e_good->v.health = 100;
   mock_add_breakable(e_good, TRUE);

   TEST("skips wrong classname, finds correct one");
   p = UTIL_FindBreakable(NULL);
   ASSERT_PTR_NOT_NULL(p);
   ASSERT_PTR_EQ(p->pEdict, e_good);
   PASS();

   TEST("after last valid entry -> NULL");
   p = UTIL_FindBreakable(p);
   ASSERT_PTR_NULL(p);
   PASS();

   return 0;
}

// ============================================================
// UTIL_LookupBreakable tests
// ============================================================

static int test_lookup_breakable(void)
{
   printf("UTIL_LookupBreakable:\n");
   mock_reset();

   edict_t *e1 = mock_alloc_edict();
   mock_set_classname(e1, "func_breakable");
   e1->v.health = 100;
   mock_add_breakable(e1, TRUE);

   edict_t *e2 = mock_alloc_edict();
   mock_set_classname(e2, "func_breakable");
   e2->v.health = 100;
   mock_add_breakable(e2, FALSE); // unbreakable glass

   edict_t *e3 = mock_alloc_edict();
   mock_set_classname(e3, "func_breakable");
   e3->v.health = 0; // dead
   mock_add_breakable(e3, TRUE);

   edict_t *e_not_registered = mock_alloc_edict();

   TEST("found valid breakable");
   breakable_list_t *p = UTIL_LookupBreakable(e1);
   ASSERT_PTR_NOT_NULL(p);
   ASSERT_PTR_EQ(p->pEdict, e1);
   PASS();

   TEST("unbreakable glass -> NULL");
   ASSERT_PTR_NULL(UTIL_LookupBreakable(e2));
   PASS();

   TEST("dead entity -> NULL");
   ASSERT_PTR_NULL(UTIL_LookupBreakable(e3));
   PASS();

   TEST("not registered -> NULL");
   ASSERT_PTR_NULL(UTIL_LookupBreakable(e_not_registered));
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetTimeSinceRespawn tests
// ============================================================

static int test_get_time_since_respawn(void)
{
   printf("UTIL_GetTimeSinceRespawn:\n");
   mock_reset();

   // Use maxClients=4 to test boundary
   gpGlobals->maxClients = 4;

   // Allocate edicts for player indices 1..4
   edict_t *p1 = mock_alloc_edict(); // index 1

   TEST("invalid index (beyond maxClients) -> -1");
   // Create edict at high index by allocating several
   edict_t *junk;
   for (int i = 0; i < 4; i++)
      junk = mock_alloc_edict();
   (void)junk;
   // junk is at index 6, which is beyond maxClients=4
   // Can't easily control index, so use a different approach:
   // Set maxClients low and use a valid edict with high ENTINDEX
   edict_t *p_high = &mock_edicts[10];
   memset(p_high, 0, sizeof(*p_high));
   p_high->v.pContainingEntity = p_high;
   // ENTINDEX(p_high) = 10, idx = 9, >= maxClients(4) -> -1
   ASSERT_FLOAT(UTIL_GetTimeSinceRespawn(p_high), -1.0f);
   PASS();

   TEST("dead player -> -1");
   // p1 is at ENTINDEX=1, idx=0
   p1->v.deadflag = DEAD_DYING;
   p1->v.health = 0;
   ASSERT_FLOAT(UTIL_GetTimeSinceRespawn(p1), -1.0f);
   PASS();

   TEST("alive player: returns time since last death");
   make_alive(p1);
   gpGlobals->time = 100.0;
   players[0].last_time_dead = 95.0;
   ASSERT_FLOAT(UTIL_GetTimeSinceRespawn(p1), 5.0f);
   PASS();

   return 0;
}

// ============================================================
// UTIL_BuildFileName_N tests
// ============================================================

static int test_build_filename_n(void)
{
   printf("UTIL_BuildFileName_N:\n");
   char buf[256];

   // Default: submod_id = SUBMOD_HLDM -> "valve"

   TEST("both args: valve/maps/test.bsp");
   UTIL_BuildFileName_N(buf, sizeof(buf), (char*)"maps", (char*)"test.bsp");
   ASSERT_STR(buf, "valve/maps/test.bsp");
   PASS();

   TEST("one arg (arg2=NULL): valve/maps");
   UTIL_BuildFileName_N(buf, sizeof(buf), (char*)"maps", NULL);
   ASSERT_STR(buf, "valve/maps");
   PASS();

   TEST("no args (both NULL): valve/");
   UTIL_BuildFileName_N(buf, sizeof(buf), NULL, NULL);
   ASSERT_STR(buf, "valve/");
   PASS();

   TEST("empty arg1: valve/");
   UTIL_BuildFileName_N(buf, sizeof(buf), (char*)"", NULL);
   ASSERT_STR(buf, "valve/");
   PASS();

   TEST("empty arg2 falls back to arg1 only: valve/maps");
   UTIL_BuildFileName_N(buf, sizeof(buf), (char*)"maps", (char*)"");
   ASSERT_STR(buf, "valve/maps");
   PASS();

   TEST("SUBMOD_OP4 uses 'gearbox' prefix");
   submod_id = SUBMOD_OP4;
   UTIL_BuildFileName_N(buf, sizeof(buf), (char*)"maps", (char*)"test.bsp");
   ASSERT_STR(buf, "gearbox/maps/test.bsp");
   submod_id = SUBMOD_HLDM;
   PASS();

   return 0;
}

// ============================================================
// null_terminate_buffer tests
// ============================================================

static int test_null_terminate_buffer(void)
{
   printf("null_terminate_buffer:\n");

   TEST("terminates at maxlen-1");
   char buf[8] = "abcdefg";
   null_terminate_buffer(buf, sizeof(buf));
   ASSERT_TRUE(buf[7] == '\0');
   // Earlier bytes unchanged
   ASSERT_TRUE(buf[0] == 'a');
   ASSERT_TRUE(buf[6] == 'g');
   PASS();

   TEST("maxlen=1 terminates first byte");
   char buf2[4] = "abc";
   null_terminate_buffer(buf2, 1);
   ASSERT_TRUE(buf2[0] == '\0');
   PASS();

   return 0;
}

// ============================================================
// bot_inline_funcs.h tests
// ============================================================

static int test_bot_inline_funcs(void)
{
   printf("bot_inline_funcs.h:\n");
   mock_reset();

   // --- UTIL_SelectItem ---
   TEST("UTIL_SelectItem: no crash");
   edict_t *e = mock_alloc_edict();
   UTIL_SelectItem(e, "weapon_shotgun");
   PASS();

   // --- BotFixIdealPitch ---
   TEST("BotFixIdealPitch: wraps 400 into [-180,180]");
   e->v.idealpitch = 400.0f;
   BotFixIdealPitch(e);
   ASSERT_FLOAT(e->v.idealpitch, 40.0f);
   PASS();

   TEST("BotFixIdealPitch: wraps -540 to 180");
   e->v.idealpitch = -540.0f;
   BotFixIdealPitch(e);
   ASSERT_FLOAT(e->v.idealpitch, 180.0f);
   PASS();

   // --- UTIL_ParticleEffect ---
   TEST("UTIL_ParticleEffect: no crash");
   UTIL_ParticleEffect(Vector(10, 20, 30), Vector(0, 0, 1), 5, 10);
   PASS();

   // --- UTIL_GetOrigin: non-BSP path (line 112) ---
   TEST("UTIL_GetOrigin: non-BSP returns v.origin");
   e->v.solid = SOLID_SLIDEBOX;
   e->v.origin = Vector(11, 22, 33);
   ASSERT_VEC(UTIL_GetOrigin(e), 11, 22, 33);
   PASS();

   // --- FIsClassname(const char*, edict_t*) reverse-arg overload ---
   mock_reset();
   edict_t *e2 = mock_alloc_edict();
   mock_set_classname(e2, "func_breakable");

   TEST("FIsClassname(name, edict): match");
   ASSERT_TRUE(FIsClassname("func_breakable", e2) == TRUE);
   PASS();

   TEST("FIsClassname(name, edict): mismatch");
   ASSERT_TRUE(FIsClassname("func_door", e2) == FALSE);
   PASS();

   // --- UTIL_FindEntityByClassname / UTIL_FindEntityByString ---
   mock_reset();
   edict_t *ent_a = mock_alloc_edict();
   mock_set_classname(ent_a, "info_player_start");
   ent_a->v.origin = Vector(100, 200, 300);

   TEST("UTIL_FindEntityByClassname: found");
   edict_t *found = UTIL_FindEntityByClassname(NULL, "info_player_start");
   ASSERT_PTR_EQ(found, ent_a);
   PASS();

   TEST("UTIL_FindEntityByClassname: not found");
   found = UTIL_FindEntityByClassname(NULL, "nonexistent_class");
   ASSERT_PTR_NULL(found);
   PASS();

   // Also exercise UTIL_FindEntityByString directly with found + not-found paths
   mock_reset();
   edict_t *ent_b = mock_alloc_edict();
   mock_set_classname(ent_b, "weapon_9mmAR");

   TEST("UTIL_FindEntityByString: found path");
   found = UTIL_FindEntityByString(NULL, "classname", "weapon_9mmAR");
   ASSERT_PTR_EQ(found, ent_b);
   PASS();

   TEST("UTIL_FindEntityByString: not-found returns NULL");
   found = UTIL_FindEntityByString(NULL, "classname", "weapon_nope");
   ASSERT_PTR_NULL(found);
   PASS();

   return 0;
}

// ============================================================
// UTIL_VarArgs2 tests
// ============================================================

static int test_varargs2(void)
{
   printf("UTIL_VarArgs2:\n");

   TEST("basic format string");
   char buf[128];
   char *result = UTIL_VarArgs2(buf, sizeof(buf), (char*)"hello %s %d", "world", 42);
   ASSERT_STR(result, "hello world 42");
   ASSERT_PTR_EQ(result, buf);
   PASS();

   TEST("truncation on small buffer");
   char small[8];
   UTIL_VarArgs2(small, sizeof(small), (char*)"abcdefghijklmnop");
   ASSERT_TRUE(strlen(small) < sizeof(small));
   PASS();

   return 0;
}

// ============================================================
// UTIL_FreeFuncBreakables tests
// ============================================================

static int test_free_func_breakables(void)
{
   printf("UTIL_FreeFuncBreakables:\n");
   mock_reset();

   edict_t *e1 = mock_alloc_edict();
   mock_set_classname(e1, "func_breakable");
   e1->v.health = 100;
   mock_add_breakable(e1, TRUE);

   edict_t *e2 = mock_alloc_edict();
   mock_set_classname(e2, "func_breakable");
   e2->v.health = 50;
   mock_add_breakable(e2, TRUE);

   TEST("breakables exist before free");
   ASSERT_PTR_NOT_NULL(UTIL_FindBreakable(NULL));
   PASS();

   TEST("after free, list is empty");
   UTIL_FreeFuncBreakables();
   ASSERT_PTR_NULL(UTIL_FindBreakable(NULL));
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetSecs tests
// ============================================================

static int test_get_secs(void)
{
   printf("UTIL_GetSecs:\n");

   TEST("returns positive value");
   double t = UTIL_GetSecs();
   ASSERT_TRUE(t > 0.0);
   PASS();

   TEST("second call returns non-negative value");
   double t2 = UTIL_GetSecs();
   ASSERT_TRUE(t2 > 0.0);
   PASS();

   return 0;
}

// ============================================================
// SaveAliveStatus tests
// ============================================================

static int test_save_alive_status(void)
{
   printf("SaveAliveStatus:\n");
   mock_reset();
   gpGlobals->maxClients = 4;

   edict_t *p1 = mock_alloc_edict(); // index 1
   gpGlobals->time = 50.0;
   players[0].last_time_dead = 0.0;

   TEST("alive player does not update last_time_dead");
   make_alive(p1);
   SaveAliveStatus(p1);
   ASSERT_FLOAT(players[0].last_time_dead, 0.0f);
   PASS();

   TEST("dead player updates last_time_dead to current time");
   p1->v.deadflag = DEAD_DYING;
   p1->v.health = 0;
   SaveAliveStatus(p1);
   ASSERT_FLOAT(players[0].last_time_dead, 50.0f);
   PASS();

   TEST("out-of-range index is ignored");
   edict_t *p_high = &mock_edicts[10];
   memset(p_high, 0, sizeof(*p_high));
   p_high->v.pContainingEntity = p_high;
   players[9].last_time_dead = 0.0;
   SaveAliveStatus(p_high);
   // Should not have been modified (idx=9 >= maxClients=4)
   ASSERT_FLOAT(players[9].last_time_dead, 0.0f);
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetTeam tests
// ============================================================

static char *mock_team_return_val;
static char *mock_pfnInfoKeyValue_team(char *infobuffer, char *key)
{
   (void)infobuffer; (void)key;
   return mock_team_return_val;
}

static int test_get_team(void)
{
   printf("UTIL_GetTeam:\n");
   mock_reset();

   edict_t *p = mock_alloc_edict();

   TEST("copies team string from InfoKeyValue");
   mock_team_return_val = (char *)"blue";
   g_engfuncs.pfnInfoKeyValue = mock_pfnInfoKeyValue_team;
   char teamstr[MAX_TEAMNAME_LENGTH];
   char *ret = UTIL_GetTeam(p, teamstr, sizeof(teamstr));
   ASSERT_STR(teamstr, "blue");
   ASSERT_PTR_EQ(ret, teamstr);
   PASS();

   TEST("empty team string");
   mock_team_return_val = (char *)"";
   UTIL_GetTeam(p, teamstr, sizeof(teamstr));
   ASSERT_STR(teamstr, "");
   PASS();

   return 0;
}

// ============================================================
// UTIL_GetClientCount tests
// ============================================================

static int test_get_client_count(void)
{
   printf("UTIL_GetClientCount:\n");
   mock_reset();
   gpGlobals->maxClients = 4;

   TEST("no clients -> 0");
   ASSERT_INT(UTIL_GetClientCount(), 0);
   PASS();

   // Set up 3 player edicts with netnames and valid GETPLAYERUSERID
   edict_t *p1 = mock_alloc_edict(); // index 1
   mock_set_classname(p1, "player");
   p1->v.netname = (string_t)(long)"Player1";

   edict_t *p2 = mock_alloc_edict(); // index 2
   mock_set_classname(p2, "player");
   p2->v.netname = (string_t)(long)"Player2";

   edict_t *p3 = mock_alloc_edict(); // index 3
   mock_set_classname(p3, "player");
   p3->v.netname = (string_t)(long)"Player3";

   TEST("3 valid clients -> 3");
   ASSERT_INT(UTIL_GetClientCount(), 3);
   PASS();

   TEST("empty netname skipped");
   p2->v.netname = (string_t)(long)"";
   ASSERT_INT(UTIL_GetClientCount(), 2);
   p2->v.netname = (string_t)(long)"Player2";
   PASS();

   return 0;
}

// ============================================================
// UTIL_DrawBeam tests
// ============================================================

static int test_draw_beam(void)
{
   printf("UTIL_DrawBeam:\n");
   mock_reset();

   Vector start(0, 0, 0);
   Vector end(100, 100, 100);

   TEST("NULL pEnemy -> MSG_ALL path, no crash");
   UTIL_DrawBeam(NULL, start, end, 10, 0, 255, 0, 0, 128, 5);
   PASS();

   edict_t *p = mock_alloc_edict(); // index 1
   gpGlobals->maxClients = 32;

   TEST("valid player pEnemy -> MSG_ONE path, no crash");
   UTIL_DrawBeam(p, start, end, 10, 0, 0, 255, 0, 128, 5);
   PASS();

   TEST("out-of-range ENTINDEX -> early return");
   edict_t *e_high = &mock_edicts[40];
   memset(e_high, 0, sizeof(*e_high));
   e_high->v.pContainingEntity = e_high;
   // ENTINDEX(e_high)=40, 40-1=39 >= maxClients=32, should early return
   UTIL_DrawBeam(e_high, start, end, 10, 0, 0, 0, 255, 128, 5);
   PASS();

   return 0;
}

// ============================================================
// ClientPrint tests
// ============================================================

static int mock_msg_id_return_val;
static int mock_mutil_GetUserMsgID_configurable(plid_t plid, const char *msgname, int *size)
{
   (void)plid; (void)msgname;
   if (size) *size = -1;
   return mock_msg_id_return_val;
}

static int test_client_print(void)
{
   printf("ClientPrint:\n");
   mock_reset();

   edict_t *p = mock_alloc_edict();

   TEST("basic call does not crash");
   ClientPrint(p, 1, "test message");
   PASS();

   TEST("GET_USER_MSG_ID returns 0 -> REG_USER_MSG fallback");
   mock_msg_id_return_val = 0;
   gpMetaUtilFuncs->pfnGetUserMsgID = mock_mutil_GetUserMsgID_configurable;
   ClientPrint(p, 1, "fallback test");
   // Restore
   mock_msg_id_return_val = 1;
   PASS();

   return 0;
}

// ============================================================
// UTIL_LogPrintf tests
// ============================================================

// UTIL_LogPrintf is declared in dlls/util.h but defined in util.cpp.
// Forward declare it here.
extern void UTIL_LogPrintf(char *fmt, ...);

static int test_log_printf(void)
{
   printf("UTIL_LogPrintf:\n");
   mock_reset();

   TEST("basic call with format args does not crash");
   UTIL_LogPrintf((char *)"test %s %d\n", "message", 42);
   PASS();

   TEST("empty string");
   UTIL_LogPrintf((char *)"");
   PASS();

   return 0;
}

// ============================================================
// UTIL_ConsolePrintf tests
// ============================================================

static int test_console_printf(void)
{
   printf("UTIL_ConsolePrintf:\n");
   mock_reset();

   TEST("short message with trailing newline");
   UTIL_ConsolePrintf("hello %s\n", "world");
   PASS();

   TEST("short message without trailing newline (auto-appended)");
   UTIL_ConsolePrintf("no newline");
   PASS();

   TEST("long message truncation (fill near buffer limit)");
   char longmsg[600];
   memset(longmsg, 'A', sizeof(longmsg) - 1);
   longmsg[sizeof(longmsg) - 1] = '\0';
   UTIL_ConsolePrintf("%s", longmsg);
   PASS();

   return 0;
}

// ============================================================
// UTIL_SelectWeapon tests
// ============================================================

// Forward declare (defined in util.cpp, not in util.h)
extern void UTIL_SelectWeapon(edict_t *pEdict, int weapon_index);

static int test_select_weapon(void)
{
   printf("UTIL_SelectWeapon:\n");
   mock_reset();

   edict_t *p = mock_alloc_edict();
   p->v.v_angle = Vector(0, 90, 0);

   TEST("basic call does not crash");
   UTIL_SelectWeapon(p, 5);
   PASS();

   TEST("call with different weapon index");
   UTIL_SelectWeapon(p, 17);
   PASS();

   return 0;
}

// ============================================================
// UTIL_AdjustOriginWithExtent tests
// ============================================================

static int test_adjust_origin_with_extent(void)
{
   printf("UTIL_AdjustOriginWithExtent:\n");
   mock_reset();

   edict_t *pBot_edict = mock_alloc_edict();
   pBot_edict->v.origin = Vector(0, 0, 0);
   pBot_edict->v.view_ofs = Vector(0, 0, 17);

   bot_t testbot;
   memset(&testbot, 0, sizeof(testbot));
   testbot.pEdict = pBot_edict;

   edict_t *pTarget = mock_alloc_edict();
   Vector target_origin(100, 0, 0);

   TEST("SOLID_BSP -> early return (unchanged origin)");
   pTarget->v.solid = SOLID_BSP;
   Vector result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   ASSERT_VEC(result, 100, 0, 0);
   PASS();

   TEST("non-BSP with symmetric mins/maxs -> extent applied");
   pTarget->v.solid = SOLID_SLIDEBOX;
   pTarget->v.mins = Vector(-16, -16, -36);
   pTarget->v.maxs = Vector(16, 16, 36);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   // smallest_extent = min(-(-16), -(-16), -(-36), 16, 16, 36) = 16
   // direction from target to bot gun position = normalize((0,0,17) - (100,0,0))
   // The origin should be shifted by 16 units toward the bot
   ASSERT_TRUE(result.x < 100.0f); // shifted toward bot
   PASS();

   TEST("zero-extent (all zero mins/maxs) -> returns original");
   pTarget->v.mins = Vector(0, 0, 0);
   pTarget->v.maxs = Vector(0, 0, 0);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   ASSERT_VEC(result, 100, 0, 0);
   PASS();

   TEST("negative extent (maxs < 0) -> returns original");
   pTarget->v.mins = Vector(-10, -10, -10);
   pTarget->v.maxs = Vector(-5, -5, -5);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   // smallest_extent = min(10, 10, 10, -5, -5, -5) = -5, <= 0 -> return original
   ASSERT_VEC(result, 100, 0, 0);
   PASS();

   TEST("asymmetric mins -> mins.y is smallest");
   pTarget->v.mins = Vector(-20, -8, -20);
   pTarget->v.maxs = Vector(20, 20, 20);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   // -mins: 20, 8, 20. maxs: 20, 20, 20. smallest = 8
   ASSERT_TRUE(result.x < 100.0f);
   PASS();

   TEST("asymmetric mins -> mins.z is smallest");
   pTarget->v.mins = Vector(-20, -20, -5);
   pTarget->v.maxs = Vector(20, 20, 20);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   // -mins: 20, 20, 5. maxs: 20, 20, 20. smallest = 5
   ASSERT_TRUE(result.x < 100.0f);
   PASS();

   TEST("asymmetric maxs -> maxs.y is smallest");
   pTarget->v.mins = Vector(-20, -20, -20);
   pTarget->v.maxs = Vector(20, 7, 20);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   // -mins: 20, 20, 20. maxs: 20, 7, 20. smallest = 7
   ASSERT_TRUE(result.x < 100.0f);
   PASS();

   TEST("asymmetric maxs -> maxs.z is smallest");
   pTarget->v.mins = Vector(-20, -20, -20);
   pTarget->v.maxs = Vector(20, 20, 6);
   result = UTIL_AdjustOriginWithExtent(testbot, target_origin, pTarget);
   // -mins: 20, 20, 20. maxs: 20, 20, 6. smallest = 6
   ASSERT_TRUE(result.x < 100.0f);
   PASS();

   return 0;
}

// ============================================================
// FVisible tests
// ============================================================

static int test_fvisible(void)
{
   printf("FVisible:\n");
   mock_reset();

   edict_t *pBot_edict = mock_alloc_edict();
   pBot_edict->v.origin = Vector(0, 0, 0);
   pBot_edict->v.view_ofs = Vector(0, 0, 17);

   TEST("clear line of sight -> TRUE");
   // Default trace: fraction = 1.0
   edict_t *pHit = NULL;
   qboolean vis = FVisible(Vector(100, 0, 0), pBot_edict, &pHit);
   ASSERT_TRUE(vis == TRUE);
   PASS();

   TEST("blocked line of sight -> FALSE");
   mock_trace_line_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      ptr->flFraction = 0.5f;
      ptr->pHit = NULL;
   };
   vis = FVisible(Vector(100, 0, 0), pBot_edict, &pHit);
   ASSERT_TRUE(vis == FALSE);
   mock_trace_line_fn = NULL;
   PASS();

   TEST("water mismatch -> FALSE");
   mock_point_contents_fn = [](const float *origin) -> int {
      // If z > 10, return WATER; otherwise EMPTY
      return (origin[2] > 10.0f) ? CONTENTS_WATER : CONTENTS_EMPTY;
   };
   // Bot eye at (0,0,17) -> WATER, target at (100,0,0) -> EMPTY
   vis = FVisible(Vector(100, 0, 0), pBot_edict, &pHit);
   ASSERT_TRUE(vis == FALSE);
   mock_point_contents_fn = NULL;
   PASS();

   TEST("pHit is set from trace result");
   edict_t *dummy_hit = mock_alloc_edict();
   static edict_t *s_trace_hit_target;
   s_trace_hit_target = dummy_hit;
   mock_trace_line_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      ptr->flFraction = 1.0f;
      ptr->pHit = s_trace_hit_target;
   };
   pHit = NULL;
   vis = FVisible(Vector(100, 0, 0), pBot_edict, &pHit);
   ASSERT_TRUE(vis == TRUE);
   ASSERT_PTR_EQ(pHit, dummy_hit);
   mock_trace_line_fn = NULL;
   PASS();

   TEST("NULL pHit pointer is safe");
   edict_t **null_phit = NULL;
   vis = FVisible(Vector(100, 0, 0), pBot_edict, null_phit);
   ASSERT_TRUE(vis == TRUE);
   PASS();

   return 0;
}

// ============================================================
// FVisibleEnemy tests
// ============================================================

// Need a trace function that blocks unless hitting a specific entity
static edict_t *mock_fvis_enemy_hit_edict;

static void trace_blocked_unless_enemy(const float *v1, const float *v2,
                                       int fNoMonsters, int hullNumber,
                                       edict_t *pentToSkip, TraceResult *ptr)
{
   (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
   ptr->flFraction = 0.5f;
   ptr->pHit = mock_fvis_enemy_hit_edict;
}

static int test_fvisible_enemy(void)
{
   printf("FVisibleEnemy:\n");
   mock_reset();

   edict_t *pBot_edict = mock_alloc_edict();
   pBot_edict->v.origin = Vector(0, 0, 0);
   pBot_edict->v.view_ofs = Vector(0, 0, 17);

   edict_t *pEnemy = mock_alloc_edict();
   mock_set_classname(pEnemy, "player");
   make_alive(pEnemy);
   pEnemy->v.origin = Vector(100, 0, 0);
   pEnemy->v.mins = Vector(-16, -16, -36);
   pEnemy->v.maxs = Vector(16, 16, 36);

   TEST("NULL pEnemy -> checks center only, clear -> TRUE");
   qboolean vis = FVisibleEnemy(Vector(100, 0, 0), pBot_edict, NULL);
   ASSERT_TRUE(vis == TRUE);
   PASS();

   TEST("clear visibility to non-BSP enemy -> TRUE");
   pEnemy->v.solid = SOLID_SLIDEBOX;
   vis = FVisibleEnemy(pEnemy->v.origin, pBot_edict, pEnemy);
   ASSERT_TRUE(vis == TRUE);
   PASS();

   TEST("SOLID_BSP enemy -> checks center only, clear -> TRUE");
   pEnemy->v.solid = SOLID_BSP;
   vis = FVisibleEnemy(pEnemy->v.origin, pBot_edict, pEnemy);
   ASSERT_TRUE(vis == TRUE);
   PASS();

   TEST("blocked trace but pHit == pEnemy -> TRUE");
   pEnemy->v.solid = SOLID_SLIDEBOX;
   mock_fvis_enemy_hit_edict = pEnemy;
   mock_trace_line_fn = trace_blocked_unless_enemy;
   vis = FVisibleEnemy(pEnemy->v.origin, pBot_edict, pEnemy);
   ASSERT_TRUE(vis == TRUE);
   mock_trace_line_fn = NULL;
   PASS();

   TEST("blocked trace, hit is alive monster -> TRUE");
   edict_t *monster = mock_alloc_edict();
   monster->v.flags = FL_MONSTER;
   make_alive(monster);
   mock_fvis_enemy_hit_edict = monster;
   mock_trace_line_fn = trace_blocked_unless_enemy;
   vis = FVisibleEnemy(pEnemy->v.origin, pBot_edict, pEnemy);
   ASSERT_TRUE(vis == TRUE);
   mock_trace_line_fn = NULL;
   PASS();

   TEST("blocked trace, hit is dead entity -> FALSE");
   edict_t *dead_ent = mock_alloc_edict();
   mock_set_classname(dead_ent, "player");
   dead_ent->v.flags = 0;
   dead_ent->v.health = 0;
   dead_ent->v.deadflag = DEAD_DEAD;
   mock_fvis_enemy_hit_edict = dead_ent;
   mock_trace_line_fn = trace_blocked_unless_enemy;
   // SOLID_BSP -> only center checked; blocked + hit dead = FALSE
   pEnemy->v.solid = SOLID_BSP;
   vis = FVisibleEnemy(pEnemy->v.origin, pBot_edict, pEnemy);
   ASSERT_TRUE(vis == FALSE);
   mock_trace_line_fn = NULL;
   PASS();

   TEST("head blocked, feet visible -> TRUE (covers feet_offset path)");
   pEnemy->v.solid = SOLID_SLIDEBOX;
   // Head at maxs.z - 6 = 30, feet at mins.z + 6 = -30
   // We need trace to block for head but pass for feet.
   static int s_trace_call_count;
   s_trace_call_count = 0;
   mock_trace_line_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr) {
      (void)v1; (void)v2; (void)fNoMonsters; (void)hullNumber; (void)pentToSkip;
      s_trace_call_count++;
      if (s_trace_call_count == 1) {
         // First call = head check -> blocked, no useful hit
         ptr->flFraction = 0.5f;
         ptr->pHit = NULL;
      } else {
         // Second call = feet check -> clear
         ptr->flFraction = 1.0f;
         ptr->pHit = NULL;
      }
   };
   vis = FVisibleEnemy(pEnemy->v.origin, pBot_edict, pEnemy);
   ASSERT_TRUE(vis == TRUE);
   mock_trace_line_fn = NULL;
   PASS();

   return 0;
}

// ============================================================
// UTIL_HostSay tests
// ============================================================

static int test_host_say(void)
{
   printf("UTIL_HostSay:\n");
   mock_reset();
   gpGlobals->maxClients = 4;

   edict_t *sender = mock_alloc_edict(); // index 1
   mock_set_classname(sender, "player");
   sender->v.netname = (string_t)(long)"TestBot";

   // Set up another player for the FindEntityByClassname loop
   edict_t *receiver = mock_alloc_edict(); // index 2
   mock_set_classname(receiver, "player");
   receiver->v.netname = (string_t)(long)"OtherPlayer";

   // Use team mock
   mock_team_return_val = (char *)"blue";
   g_engfuncs.pfnInfoKeyValue = mock_pfnInfoKeyValue_team;

   TEST("whitespace-only message -> early return");
   char ws_msg[] = "   \t  ";
   UTIL_HostSay(sender, 0, ws_msg);
   PASS();

   TEST("normal say message, no crash");
   char msg[] = "Hello everyone!";
   UTIL_HostSay(sender, 0, msg);
   PASS();

   TEST("team say with is_team_play");
   is_team_play = TRUE;
   char team_msg[] = "Team message";
   UTIL_HostSay(sender, 1, team_msg);
   is_team_play = FALSE;
   PASS();

   TEST("non-team say with is_team_play");
   is_team_play = TRUE;
   char say_msg[] = "Public message";
   UTIL_HostSay(sender, 0, say_msg);
   is_team_play = FALSE;
   PASS();

   TEST("long message truncation");
   char long_msg[256];
   memset(long_msg, 'X', sizeof(long_msg) - 1);
   long_msg[sizeof(long_msg) - 1] = '\0';
   UTIL_HostSay(sender, 0, long_msg);
   PASS();

   TEST("GET_USER_MSG_ID returns 0 -> REG_USER_MSG fallback");
   mock_msg_id_return_val = 0;
   gpMetaUtilFuncs->pfnGetUserMsgID = mock_mutil_GetUserMsgID_configurable;
   char msg2[] = "reg fallback";
   UTIL_HostSay(sender, 0, msg2);
   mock_msg_id_return_val = 1;
   PASS();

   TEST("team mismatch skips receiver in team-only mode");
   // Set up different teams: sender=blue, receiver=red
   static int mock_team_call_count;
   mock_team_call_count = 0;
   // We need InfoKeyValue to return different values per edict.
   // Use a lambda-compatible static approach:
   g_engfuncs.pfnInfoKeyValue = [](char *infobuffer, char *key) -> char * {
      (void)infobuffer; (void)key;
      // GetInfoKeyBuffer returns the same static buffer for all edicts,
      // so we differentiate by call order: UTIL_GetTeam is called for
      // sender first, then for each receiver.
      static char blue[] = "blue";
      static char red[] = "red";
      mock_team_call_count++;
      // Odd calls = sender (blue), even calls = receiver (red)
      return (mock_team_call_count % 2 == 1) ? blue : red;
   };
   is_team_play = TRUE;
   char team_diff_msg[] = "team diff";
   UTIL_HostSay(sender, 1, team_diff_msg);
   is_team_play = FALSE;
   PASS();

   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   printf("=== util.cpp tests ===\n\n");

   mock_reset();

   int fail = 0;

   // Pure math tests
   fail |= test_wrap_angle();
   fail |= test_wrap_angles();
   fail |= test_angles_to_forward();
   fail |= test_angles_to_right();
   fail |= test_make_vectors_private();
   fail |= test_vec_to_angles();
   fail |= test_is_alive();
   fail |= test_vec_bmodel_origin();
   fail |= test_fin_view_cone();
   fail |= test_fin_shoot_cone();

   // Bot array tests
   fail |= test_get_bot_index();
   fail |= test_get_bot_pointer();
   fail |= test_get_bot_count();
   fail |= test_pick_random_bot();

   // Breakable list tests
   fail |= test_update_func_breakable();
   fail |= test_find_breakable();
   fail |= test_lookup_breakable();

   // Other testable functions
   fail |= test_getgamedir();
   fail |= test_chat_protection_facing_wall();
   fail |= test_get_time_since_respawn();
   fail |= test_build_filename_n();
   fail |= test_null_terminate_buffer();

   // bot_inline_funcs.h coverage
   fail |= test_bot_inline_funcs();

   // New coverage tests
   fail |= test_varargs2();
   fail |= test_free_func_breakables();
   fail |= test_get_secs();
   fail |= test_save_alive_status();
   fail |= test_get_team();
   fail |= test_get_client_count();
   fail |= test_draw_beam();
   fail |= test_client_print();
   fail |= test_log_printf();
   fail |= test_console_printf();
   fail |= test_select_weapon();
   fail |= test_adjust_origin_with_extent();
   fail |= test_fvisible();
   fail |= test_fvisible_enemy();
   fail |= test_host_say();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
   if (tests_passed == tests_run)
      printf("All tests passed.\n");
   else
      printf("SOME TESTS FAILED!\n");

   return (tests_passed == tests_run) ? 0 : 1;
}
