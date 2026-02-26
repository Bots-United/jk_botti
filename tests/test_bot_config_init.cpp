//
// JK_Botti - placeholder tests for bot_config_init.cpp
//
// test_bot_config_init.cpp
//

#include <stdlib.h>
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_config_init.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("bot_config_init.cpp placeholder");

   mock_reset();

   // Verify initial state of globals defined by bot_config_init.cpp
   ASSERT_INT(number_names, 0);
   ASSERT_INT(num_logos, 0);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_config_init:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
