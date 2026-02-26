//
// JK_Botti - placeholder tests for engine.cpp
//
// test_engine.cpp
//

#include <stdlib.h>
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "bot_sound.h"
#include "bot_weapons.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Stubs for symbols not in engine_mock or linked .o files
// ============================================================

// commands.cpp globals
qboolean isFakeClientCommand = FALSE;
int fake_arg_count = 0;

// engine.cpp declares these externs but may not use them all
int get_cvars = 0;
qboolean aim_fix = FALSE;
float turn_skill = 3.0;

// bot_client.cpp functions
void BotClient_Valve_WeaponList(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_CurrentWeapon(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_AmmoX(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_AmmoPickup(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_WeaponPickup(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_ItemPickup(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_Health(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_Battery(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_Damage(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_DeathMsg(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_ScreenFade(void *p, int bot_index) { (void)p; (void)bot_index; }

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("engine.cpp placeholder");

   mock_reset();

   // Verify engine.o exported function exists
   enginefuncs_t eng;
   int version = 0;
   memset(&eng, 0, sizeof(eng));
   extern int GetEngineFunctions(enginefuncs_t *, int *);
   int ret = GetEngineFunctions(&eng, &version);
   ASSERT_INT(ret, TRUE);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_engine:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
