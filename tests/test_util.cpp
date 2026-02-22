//
// JK_Botti - tests for util.cpp bug fixes
//

#include <string.h>
#include <math.h>

#include "engine_mock.h"
#include "player.h"
#include "test_common.h"

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
// main
// ============================================================

int main(void)
{
   printf("=== util bug fix tests ===\n\n");

   mock_reset();

   int fail = 0;
   fail |= test_getgamedir();
   fail |= test_chat_protection_facing_wall();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
   if (tests_passed == tests_run)
      printf("All tests passed.\n");
   else
      printf("SOME TESTS FAILED!\n");

   return (tests_passed == tests_run) ? 0 : 1;
}
