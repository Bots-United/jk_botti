//
// JK_Botti - placeholder tests for bot_client.cpp
//
// test_bot_client.cpp
//

#include <stdlib.h>
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_client.h"
#include "bot_weapons.h"

#include "engine_mock.h"
#include "test_common.h"

// Stubs for symbols not in engine_mock
int bot_taunt_count = 0;
int recent_bot_taunt[5] = {};
bot_chat_t bot_taunt[MAX_BOT_CHAT];

qboolean FPredictedVisible(bot_t &pBot) { (void)pBot; return FALSE; }

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("bot_client.cpp placeholder");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   // Verify weapon_defs array exists (defined by bot_client.o)
   extern bot_weapon_t weapon_defs[MAX_WEAPONS];
   ASSERT_TRUE(sizeof(weapon_defs) > 0);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_client:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
