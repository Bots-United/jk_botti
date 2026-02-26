//
// JK_Botti - tests for h_export.cpp
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
// Tests for GiveFnptrsToDll
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
// Tests for __cxa_guard_* functions
// ============================================================

static int test_cxa_guard_acquire_uninitialized(void)
{
   TEST("__cxa_guard_acquire: uninitialized guard -> 1");

   __cxxabiv1::__guard g = 0;  // uninitialized
   int result = __cxxabiv1::__cxa_guard_acquire(&g);
   ASSERT_INT(result, 1);

   PASS();
   return 0;
}

static int test_cxa_guard_acquire_initialized(void)
{
   TEST("__cxa_guard_acquire: initialized guard -> 0");

   __cxxabiv1::__guard g = 0;
   // Simulate that guard_release has been called (first byte = 1)
   *(char *)&g = 1;
   int result = __cxxabiv1::__cxa_guard_acquire(&g);
   ASSERT_INT(result, 0);

   PASS();
   return 0;
}

static int test_cxa_guard_release(void)
{
   TEST("__cxa_guard_release: sets guard byte to 1");

   __cxxabiv1::__guard g = 0;
   __cxxabiv1::__cxa_guard_release(&g);
   ASSERT_INT(*(char *)&g, 1);

   // Subsequent acquire should return 0 (already initialized)
   int result = __cxxabiv1::__cxa_guard_acquire(&g);
   ASSERT_INT(result, 0);

   PASS();
   return 0;
}

static int test_cxa_guard_abort(void)
{
   TEST("__cxa_guard_abort: does not set guard byte");

   __cxxabiv1::__guard g = 0;
   __cxxabiv1::__cxa_guard_abort(&g);
   // Guard should remain uninitialized (first byte still 0)
   ASSERT_INT(*(char *)&g, 0);

   // acquire should still return 1 (not initialized)
   int result = __cxxabiv1::__cxa_guard_acquire(&g);
   ASSERT_INT(result, 1);

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
   fail |= test_cxa_guard_acquire_uninitialized();
   fail |= test_cxa_guard_acquire_initialized();
   fail |= test_cxa_guard_release();
   fail |= test_cxa_guard_abort();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
