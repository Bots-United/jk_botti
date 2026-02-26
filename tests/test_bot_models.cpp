//
// JK_Botti - placeholder tests for bot_models.cpp
//
// test_bot_models.cpp
//

#include <stdlib.h>
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"

#include "engine_mock.h"
#include "test_common.h"

// bot_models.cpp globals
extern int number_skins;
extern skin_t bot_skins[MAX_SKINS];

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("bot_models.cpp placeholder");

   mock_reset();

   // LoadBotModels touches the filesystem, so just verify linkage
   ASSERT_TRUE(sizeof(bot_skins) > 0);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_models:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
