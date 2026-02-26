//
// JK_Botti - placeholder tests for h_export.cpp
//
// test_h_export.cpp
//
// Uses the #include-the-.cpp approach (h_export.cpp is self-contained).
//

#include <stdlib.h>
#include <string.h>

// Include the source file under test
#include "../h_export.cpp"

#include "test_common.h"

// ============================================================
// Tests
// ============================================================

static int test_GiveFnptrsToDll_copies_engfuncs(void)
{
   TEST("GiveFnptrsToDll copies engine funcs and sets gpGlobals");

   enginefuncs_t mock_eng;
   globalvars_t mock_globals;
   memset(&mock_eng, 0, sizeof(mock_eng));
   memset(&mock_globals, 0, sizeof(mock_globals));
   memset(&g_engfuncs, 0, sizeof(g_engfuncs));
   gpGlobals = NULL;

   // Set a recognizable value
   mock_eng.pfnPrecacheModel = (int (*)(char *))0x12345678;
   mock_globals.maxClients = 16;

   GiveFnptrsToDll(&mock_eng, &mock_globals);

   ASSERT_TRUE(g_engfuncs.pfnPrecacheModel == mock_eng.pfnPrecacheModel);
   ASSERT_TRUE(gpGlobals == &mock_globals);
   ASSERT_INT(gpGlobals->maxClients, 16);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_h_export:\n");
   fail |= test_GiveFnptrsToDll_copies_engfuncs();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
