//
// JK_Botti - comprehensive tests for util.cpp
//

#include <string.h>
#include <math.h>

#include "engine_mock.h"
#include "player.h"
#include "test_common.h"

// Tolerance for floating-point comparisons
#define EPSILON 1e-4f

#define ASSERT_FLOAT(actual, expected) do { \
   float _a = (actual), _e = (expected); \
   if (fabsf(_a - _e) > EPSILON) { \
      printf("FAIL\n    expected: %f\n    got:      %f\n", _e, _a); \
      return 1; \
   } \
} while(0)

#define ASSERT_VEC(v, ex, ey, ez) do { \
   if (fabsf((v).x - (float)(ex)) > EPSILON || \
       fabsf((v).y - (float)(ey)) > EPSILON || \
       fabsf((v).z - (float)(ez)) > EPSILON) { \
      printf("FAIL\n    expected: (%f, %f, %f)\n    got:      (%f, %f, %f)\n", \
             (float)(ex), (float)(ey), (float)(ez), (v).x, (v).y, (v).z); \
      return 1; \
   } \
} while(0)

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

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
   if (tests_passed == tests_run)
      printf("All tests passed.\n");
   else
      printf("SOME TESTS FAILED!\n");

   return (tests_passed == tests_run) ? 0 : 1;
}
