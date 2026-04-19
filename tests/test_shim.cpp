//
// JK_Botti - tests for shim.c
//
// Compiles shim.c as the Win32 variant on Linux, mocking LoadLibraryA /
// GetProcAddress / OutputDebugStringA and the CPU-feature detection helpers.
// Build with: -D_WIN32 -DSHIM_TEST -I. (tests dir for mock windows.h)
//

#ifndef _WIN32
#define _WIN32
#endif

#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>

#include "../shim.c"

#include "test_common.h"

// ============================================================
// Mock state
// ============================================================

struct MockLib {
   bool load_ok         = true;
   bool have_all_syms   = true;
};

static std::map<std::string, MockLib> mock_libs;
static std::vector<std::string>       load_attempts;
static std::vector<MockLib *>         free_attempts;
static int mock_has_avx2_fma_val = 0;
static int mock_has_sse3_val     = 0;
static std::string mock_self_dir = "/mock/";

// ============================================================
// Mocks of shim.c externals (has_avx2_fma / has_sse3 / self_directory)
// ============================================================

int has_avx2_fma(void) { return mock_has_avx2_fma_val; }
int has_sse3(void)     { return mock_has_sse3_val; }

int self_directory(char *out, size_t cap)
{
   size_t n = mock_self_dir.size();
   if (n + 1 > cap) return 0;
   memcpy(out, mock_self_dir.c_str(), n + 1);
   return 1;
}

// ============================================================
// Mock Win32 API implementations
// ============================================================

HMODULE LoadLibraryA(const char *name)
{
   load_attempts.push_back(name);
   auto it = mock_libs.find(name);
   if (it == mock_libs.end() || !it->second.load_ok)
      return NULL;
   return reinterpret_cast<HMODULE>(&it->second);
}

FARPROC GetProcAddress(HMODULE module, const char *name)
{
   (void)name;
   MockLib *lib = reinterpret_cast<MockLib *>(module);
   if (!lib || !lib->have_all_syms)
      return NULL;
   // Non-null sentinel address — shim stores it into its function-pointer slot
   // but never calls through during these tests.
   static int fake;
   return reinterpret_cast<FARPROC>(&fake);
}

BOOL FreeLibrary(HMODULE module)
{
   free_attempts.push_back(reinterpret_cast<MockLib *>(module));
   return 1;
}

DWORD GetLastError(void) { return 0; }
void OutputDebugStringA(const char *msg) { (void)msg; }

// ============================================================
// Helpers
// ============================================================

static void reset_all(void)
{
   shim_test_reset();
   mock_libs.clear();
   load_attempts.clear();
   free_attempts.clear();
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 0;
   mock_self_dir         = "/mock/";
}

static void add_lib(const char *leaf, bool load_ok, bool have_all_syms)
{
   MockLib m;
   m.load_ok       = load_ok;
   m.have_all_syms = have_all_syms;
   mock_libs[std::string("/mock/") + leaf] = m;
}

// ============================================================
// Tests
// ============================================================

static int test_picks_avx2fma_when_available(void)
{
   TEST("ensure_loaded picks avx2fma when available");
   reset_all();
   add_lib("jk_botti_mm_avx2fma.dll", true, true);
   add_lib("jk_botti_mm_sse3.dll",    true, true);
   add_lib("jk_botti_mm_x87.dll",     true, true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_avx2fma.dll");
   PASS();
   return 0;
}

static int test_picks_sse3_when_no_avx2(void)
{
   TEST("ensure_loaded picks sse3 when avx2 unavailable");
   reset_all();
   add_lib("jk_botti_mm_sse3.dll", true, true);
   add_lib("jk_botti_mm_x87.dll",  true, true);
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_sse3.dll");
   PASS();
   return 0;
}

static int test_picks_x87_when_no_sse3(void)
{
   TEST("ensure_loaded picks x87 when sse3 unavailable");
   reset_all();
   add_lib("jk_botti_mm_x87.dll", true, true);
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 0;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_x87.dll");
   PASS();
   return 0;
}

static int test_fallback_avx2_load_fails(void)
{
   TEST("ensure_loaded falls back sse3 when avx2 load fails");
   reset_all();
   add_lib("jk_botti_mm_avx2fma.dll", false, false);  // load fails
   add_lib("jk_botti_mm_sse3.dll",    true,  true);
   add_lib("jk_botti_mm_x87.dll",     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 2);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_avx2fma.dll");
   ASSERT_STR(load_attempts[1].c_str(), "/mock/jk_botti_mm_sse3.dll");
   PASS();
   return 0;
}

static int test_fallback_missing_symbols(void)
{
   TEST("ensure_loaded falls back when symbol missing");
   reset_all();
   add_lib("jk_botti_mm_avx2fma.dll", true,  false); // loads but missing syms
   add_lib("jk_botti_mm_sse3.dll",    true,  true);
   add_lib("jk_botti_mm_x87.dll",     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 2);
   ASSERT_STR(load_attempts[1].c_str(), "/mock/jk_botti_mm_sse3.dll");
   PASS();
   return 0;
}

static int test_all_fall_back_to_x87(void)
{
   TEST("ensure_loaded falls through avx2->sse3->x87");
   reset_all();
   add_lib("jk_botti_mm_avx2fma.dll", false, false);
   add_lib("jk_botti_mm_sse3.dll",    false, false);
   add_lib("jk_botti_mm_x87.dll",     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 3);
   ASSERT_STR(load_attempts[2].c_str(), "/mock/jk_botti_mm_x87.dll");
   PASS();
   return 0;
}

static int test_all_loads_fail(void)
{
   TEST("ensure_loaded returns 0 when every variant fails");
   reset_all();
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 0);
   ASSERT_INT((int)load_attempts.size(), 3);
   PASS();
   return 0;
}

static int test_init_is_sticky(void)
{
   TEST("ensure_loaded returns cached result on second call");
   reset_all();
   add_lib("jk_botti_mm_sse3.dll", true, true);
   add_lib("jk_botti_mm_x87.dll",  true, true);
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);

   // Second call must not retry loads
   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   PASS();
   return 0;
}

static int test_load_variant_direct_ok(void)
{
   TEST("load_variant succeeds when library + symbols present");
   reset_all();
   add_lib("jk_botti_mm_sse3.dll", true, true);

   ASSERT_INT(shim_test_load_variant("/mock/", "jk_botti_mm_sse3.dll"), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   PASS();
   return 0;
}

static int test_load_variant_direct_missing_sym(void)
{
   TEST("load_variant fails when a symbol is missing");
   reset_all();
   add_lib("jk_botti_mm_sse3.dll", true, false);

   ASSERT_INT(shim_test_load_variant("/mock/", "jk_botti_mm_sse3.dll"), 0);
   PASS();
   return 0;
}

static int test_load_variant_closes_handle_on_sym_failure(void)
{
   TEST("load_variant closes module handle when symbol resolution fails");
   reset_all();
   add_lib("jk_botti_mm_sse3.dll", true, false);

   ASSERT_INT(shim_test_load_variant("/mock/", "jk_botti_mm_sse3.dll"), 0);
   ASSERT_INT((int)free_attempts.size(), 1);
   MockLib *loaded = &mock_libs["/mock/jk_botti_mm_sse3.dll"];
   if (free_attempts[0] != loaded) {
      printf("  FreeLibrary called on wrong handle\n");
      return 1;
   }
   PASS();
   return 0;
}

static int test_fallback_frees_handle_on_sym_failure(void)
{
   TEST("fallback path frees handle from variant with missing symbols");
   reset_all();
   add_lib("jk_botti_mm_avx2fma.dll", true,  false);  // loads, no syms
   add_lib("jk_botti_mm_sse3.dll",    true,  true);
   add_lib("jk_botti_mm_x87.dll",     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)free_attempts.size(), 1);
   MockLib *avx = &mock_libs["/mock/jk_botti_mm_avx2fma.dll"];
   if (free_attempts[0] != avx) {
      printf("  expected avx2fma handle to be freed\n");
      return 1;
   }
   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_shim:\n");
   fail |= test_picks_avx2fma_when_available();
   fail |= test_picks_sse3_when_no_avx2();
   fail |= test_picks_x87_when_no_sse3();
   fail |= test_fallback_avx2_load_fails();
   fail |= test_fallback_missing_symbols();
   fail |= test_all_fall_back_to_x87();
   fail |= test_all_loads_fail();
   fail |= test_init_is_sticky();
   fail |= test_load_variant_direct_ok();
   fail |= test_load_variant_direct_missing_sym();
   fail |= test_load_variant_closes_handle_on_sym_failure();
   fail |= test_fallback_frees_handle_on_sym_failure();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
