//
// JK_Botti - tests for shim.cpp
//
// Tests the CPU dispatch and variant loading logic by mocking
// dl_open/dl_sym/dl_close and the CPU detection helpers.
// Build with: -DSHIM_TEST
//

#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>

#include "../shim.cpp"

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
static std::vector<MockLib *>         close_attempts;
static int mock_has_avx2_fma_val = 0;
static int mock_has_sse3_val     = 0;
static std::string mock_self_dir = "/mock/";

// ============================================================
// Mocks of shim.cpp externals (has_avx2_fma / has_sse3 / self_directory)
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
// Mock dl_open / dl_sym / dl_close
// ============================================================

dl_handle_t dl_open(const char *path, char *errbuf, size_t errcap)
{
   load_attempts.push_back(path);
   auto it = mock_libs.find(path);
   if (it == mock_libs.end() || !it->second.load_ok) {
      snprintf(errbuf, errcap, "mock: load failed for %s", path);
      return NULL;
   }
   return reinterpret_cast<dl_handle_t>(&it->second);
}

void *dl_sym(dl_handle_t h, const char *name)
{
   (void)name;
   MockLib *lib = reinterpret_cast<MockLib *>(h);
   if (!lib || !lib->have_all_syms)
      return NULL;
   static int fake;
   return reinterpret_cast<void *>(&fake);
}

void dl_close(dl_handle_t h)
{
   close_attempts.push_back(reinterpret_cast<MockLib *>(h));
}

// ============================================================
// Helpers
// ============================================================

static void reset_all(void)
{
   shim_test_reset();
   mock_libs.clear();
   load_attempts.clear();
   close_attempts.clear();
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 0;
   mock_self_dir         = "/mock/";
}

#ifdef _WIN32
#define VARIANT_EXT ".dll"
#else
#define VARIANT_EXT "_i386.so"
#endif

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
   add_lib("jk_botti_mm_avx2fma" VARIANT_EXT, true, true);
   add_lib("jk_botti_mm_sse3" VARIANT_EXT,    true, true);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,     true, true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_avx2fma" VARIANT_EXT);
   PASS();
   return 0;
}

static int test_picks_sse3_when_no_avx2(void)
{
   TEST("ensure_loaded picks sse3 when avx2 unavailable");
   reset_all();
   add_lib("jk_botti_mm_sse3" VARIANT_EXT, true, true);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,  true, true);
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_sse3" VARIANT_EXT);
   PASS();
   return 0;
}

static int test_picks_x87_when_no_sse3(void)
{
   TEST("ensure_loaded picks x87 when sse3 unavailable");
   reset_all();
   add_lib("jk_botti_mm_x87" VARIANT_EXT, true, true);
   mock_has_avx2_fma_val = 0;
   mock_has_sse3_val     = 0;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_x87" VARIANT_EXT);
   PASS();
   return 0;
}

static int test_fallback_avx2_load_fails(void)
{
   TEST("ensure_loaded falls back sse3 when avx2 load fails");
   reset_all();
   add_lib("jk_botti_mm_avx2fma" VARIANT_EXT, false, false);
   add_lib("jk_botti_mm_sse3" VARIANT_EXT,    true,  true);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 2);
   ASSERT_STR(load_attempts[0].c_str(), "/mock/jk_botti_mm_avx2fma" VARIANT_EXT);
   ASSERT_STR(load_attempts[1].c_str(), "/mock/jk_botti_mm_sse3" VARIANT_EXT);
   PASS();
   return 0;
}

static int test_fallback_missing_symbols(void)
{
   TEST("ensure_loaded falls back when symbol missing");
   reset_all();
   add_lib("jk_botti_mm_avx2fma" VARIANT_EXT, true,  false);
   add_lib("jk_botti_mm_sse3" VARIANT_EXT,    true,  true);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 2);
   ASSERT_STR(load_attempts[1].c_str(), "/mock/jk_botti_mm_sse3" VARIANT_EXT);
   PASS();
   return 0;
}

static int test_all_fall_back_to_x87(void)
{
   TEST("ensure_loaded falls through avx2->sse3->x87");
   reset_all();
   add_lib("jk_botti_mm_avx2fma" VARIANT_EXT, false, false);
   add_lib("jk_botti_mm_sse3" VARIANT_EXT,    false, false);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)load_attempts.size(), 3);
   ASSERT_STR(load_attempts[2].c_str(), "/mock/jk_botti_mm_x87" VARIANT_EXT);
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
   add_lib("jk_botti_mm_sse3" VARIANT_EXT, true, true);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,  true, true);
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
   add_lib("jk_botti_mm_sse3" VARIANT_EXT, true, true);

   ASSERT_INT(shim_test_load_variant("/mock/", "jk_botti_mm_sse3" VARIANT_EXT), 1);
   ASSERT_INT((int)load_attempts.size(), 1);
   PASS();
   return 0;
}

static int test_load_variant_direct_missing_sym(void)
{
   TEST("load_variant fails when a symbol is missing");
   reset_all();
   add_lib("jk_botti_mm_sse3" VARIANT_EXT, true, false);

   ASSERT_INT(shim_test_load_variant("/mock/", "jk_botti_mm_sse3" VARIANT_EXT), 0);
   PASS();
   return 0;
}

static int test_load_variant_closes_handle_on_sym_failure(void)
{
   TEST("load_variant closes module handle when symbol resolution fails");
   reset_all();
   add_lib("jk_botti_mm_sse3" VARIANT_EXT, true, false);

   ASSERT_INT(shim_test_load_variant("/mock/", "jk_botti_mm_sse3" VARIANT_EXT), 0);
   ASSERT_INT((int)close_attempts.size(), 1);
   MockLib *loaded = &mock_libs["/mock/jk_botti_mm_sse3" VARIANT_EXT];
   if (close_attempts[0] != loaded) {
      printf("  dl_close called on wrong handle\n");
      return 1;
   }
   PASS();
   return 0;
}

static int test_fallback_frees_handle_on_sym_failure(void)
{
   TEST("fallback path frees handle from variant with missing symbols");
   reset_all();
   add_lib("jk_botti_mm_avx2fma" VARIANT_EXT, true,  false);
   add_lib("jk_botti_mm_sse3" VARIANT_EXT,    true,  true);
   add_lib("jk_botti_mm_x87" VARIANT_EXT,     true,  true);
   mock_has_avx2_fma_val = 1;
   mock_has_sse3_val     = 1;

   ASSERT_INT(shim_test_ensure_loaded(), 1);
   ASSERT_INT((int)close_attempts.size(), 1);
   MockLib *avx = &mock_libs["/mock/jk_botti_mm_avx2fma" VARIANT_EXT];
   if (close_attempts[0] != avx) {
      printf("  expected avx2fma handle to be freed\n");
      return 1;
   }
   PASS();
   return 0;
}

// ============================================================
// Proxy tests - cvar registration
// ============================================================

static std::vector<cvar_t *> mock_registered_cvars;

static void mock_real_CVarRegister(cvar_t *cv)
{
   mock_registered_cvars.push_back(cv);
}

static void reset_proxy_state(void)
{
   shim_test_reset();
   mock_registered_cvars.clear();
   shim_test_set_real_CVarRegister(mock_real_CVarRegister);
}

static int test_cvar_register_copies_to_local_storage(void)
{
   TEST("shim_CVarRegister copies cvar to shim-local storage");
   reset_proxy_state();

   cvar_t original = {};
   original.name = "jk_botti_skill";
   original.string = "3";
   original.value = 3.0f;

   shim_test_call_CVarRegister(&original);

   ASSERT_INT(shim_test_get_proxy_cvar_count(), 1);
   cvar_t *local = shim_test_get_proxy_cvar(0);
   ASSERT_STR(local->name, "jk_botti_skill");
   ASSERT_INT((int)local->value, 3);
   // Must have been registered with the LOCAL copy, not the original
   ASSERT_INT((int)mock_registered_cvars.size(), 1);
   if (mock_registered_cvars[0] != local) {
      printf("  real CVarRegister called with original ptr, not local copy\n");
      return 1;
   }
   if (mock_registered_cvars[0] == &original) {
      printf("  real CVarRegister should not get the original pointer\n");
      return 1;
   }
   PASS();
   return 0;
}

static int test_cvar_register_multiple(void)
{
   TEST("shim_CVarRegister handles multiple cvars");
   reset_proxy_state();

   cvar_t cv1 = {}; cv1.name = "cvar_one";
   cvar_t cv2 = {}; cv2.name = "cvar_two";
   cvar_t cv3 = {}; cv3.name = "cvar_three";

   shim_test_call_CVarRegister(&cv1);
   shim_test_call_CVarRegister(&cv2);
   shim_test_call_CVarRegister(&cv3);

   ASSERT_INT(shim_test_get_proxy_cvar_count(), 3);
   ASSERT_STR(shim_test_get_proxy_cvar(0)->name, "cvar_one");
   ASSERT_STR(shim_test_get_proxy_cvar(1)->name, "cvar_two");
   ASSERT_STR(shim_test_get_proxy_cvar(2)->name, "cvar_three");
   ASSERT_INT((int)mock_registered_cvars.size(), 3);
   PASS();
   return 0;
}

static int test_cvar_register_overflow_passes_original(void)
{
   TEST("shim_CVarRegister passes original ptr when proxy slots full");
   reset_proxy_state();

   cvar_t cvars[MAX_PROXY_CVARS + 1];
   for (int i = 0; i <= MAX_PROXY_CVARS; i++) {
      memset(&cvars[i], 0, sizeof(cvar_t));
      cvars[i].name = "overflow_test";
      shim_test_call_CVarRegister(&cvars[i]);
   }

   ASSERT_INT(shim_test_get_proxy_cvar_count(), MAX_PROXY_CVARS);
   ASSERT_INT((int)mock_registered_cvars.size(), MAX_PROXY_CVARS + 1);
   // Last one should be the original pointer (overflow case)
   if (mock_registered_cvars[MAX_PROXY_CVARS] != &cvars[MAX_PROXY_CVARS]) {
      printf("  overflow cvar should pass original pointer\n");
      return 1;
   }
   PASS();
   return 0;
}

static int test_cvar_register_noop_without_real(void)
{
   TEST("shim_CVarRegister is no-op when g_real_CVarRegister is NULL");
   shim_test_reset();
   // Don't set g_real_CVarRegister

   cvar_t cv = {};
   cv.name = "test";
   shim_test_call_CVarRegister(&cv);

   ASSERT_INT(shim_test_get_proxy_cvar_count(), 0);
   PASS();
   return 0;
}

// ============================================================
// Proxy tests - command registration
// ============================================================

struct CmdRecord {
   std::string name;
   void (*handler)(void);
};
static std::vector<CmdRecord> mock_registered_cmds;

static void mock_real_AddServerCommand(char *name, void (*fn)(void))
{
   mock_registered_cmds.push_back({name, fn});
}

static int cmd_call_count;
static void test_cmd_handler_a(void) { cmd_call_count += 1; }
static void test_cmd_handler_b(void) { cmd_call_count += 10; }

static void reset_cmd_state(void)
{
   shim_test_reset();
   mock_registered_cmds.clear();
   cmd_call_count = 0;
   shim_test_set_real_AddServerCommand(mock_real_AddServerCommand);
}

static int test_cmd_register_uses_trampoline(void)
{
   TEST("shim_AddServerCommand registers trampoline, not original");
   reset_cmd_state();

   shim_test_call_AddServerCommand((char *)"jk_botti", test_cmd_handler_a);

   ASSERT_INT(shim_test_get_proxy_cmd_count(), 1);
   ASSERT_INT((int)mock_registered_cmds.size(), 1);
   ASSERT_STR(mock_registered_cmds[0].name.c_str(), "jk_botti");
   // Handler should be the trampoline, not the original
   if (mock_registered_cmds[0].handler == test_cmd_handler_a) {
      printf("  registered handler is original, not trampoline\n");
      return 1;
   }
   // Trampoline should be the proxy handler for slot 0
   if (mock_registered_cmds[0].handler != shim_test_get_proxy_cmd_handler(0)) {
      printf("  registered handler is not proxy_cmd_handler_0\n");
      return 1;
   }
   PASS();
   return 0;
}

static int test_cmd_trampoline_calls_target(void)
{
   TEST("command trampoline calls the real handler function");
   reset_cmd_state();

   shim_test_call_AddServerCommand((char *)"cmd1", test_cmd_handler_a);
   shim_test_call_AddServerCommand((char *)"cmd2", test_cmd_handler_b);

   // Call through the trampolines
   mock_registered_cmds[0].handler();
   ASSERT_INT(cmd_call_count, 1);

   mock_registered_cmds[1].handler();
   ASSERT_INT(cmd_call_count, 11);
   PASS();
   return 0;
}

static int test_cmd_register_overflow_passes_original(void)
{
   TEST("shim_AddServerCommand passes original fn when proxy slots full");
   reset_cmd_state();

   void (*handlers[])(void) = {
      test_cmd_handler_a, test_cmd_handler_a,
      test_cmd_handler_a, test_cmd_handler_a,
      test_cmd_handler_b,  // 5th = overflow
   };

   for (int i = 0; i < 5; i++)
      shim_test_call_AddServerCommand((char *)"cmd", handlers[i]);

   ASSERT_INT(shim_test_get_proxy_cmd_count(), MAX_PROXY_CMDS);
   ASSERT_INT((int)mock_registered_cmds.size(), 5);
   // 5th (overflow) should be the original function pointer
   if (mock_registered_cmds[4].handler != test_cmd_handler_b) {
      printf("  overflow cmd should pass original handler\n");
      return 1;
   }
   PASS();
   return 0;
}

static int test_cmd_register_noop_without_real(void)
{
   TEST("shim_AddServerCommand is no-op when g_real_AddServerCommand is NULL");
   shim_test_reset();
   mock_registered_cmds.clear();

   shim_test_call_AddServerCommand((char *)"test", test_cmd_handler_a);

   ASSERT_INT(shim_test_get_proxy_cmd_count(), 0);
   ASSERT_INT((int)mock_registered_cmds.size(), 0);
   PASS();
   return 0;
}

// ============================================================
// GiveFnptrsToDll patching test
// ============================================================

static enginefuncs_t *captured_engfuncs;
static globalvars_t *captured_globals;

static void WINAPI mock_variant_GiveFnptrsToDll(enginefuncs_t *eng, globalvars_t *glob)
{
   captured_engfuncs = eng;
   captured_globals = glob;
}

static void mock_engine_CVarRegister(cvar_t *cv) { (void)cv; }
static void mock_engine_AddServerCommand(char *name, void (*fn)(void)) { (void)name; (void)fn; }

static int test_give_fnptrs_patches_engfuncs(void)
{
   TEST("GiveFnptrsToDll patches cvar/cmd functions in engine table");
   reset_all();
   add_lib("jk_botti_mm_sse3" VARIANT_EXT, true, true);
   mock_has_sse3_val = 1;
   captured_engfuncs = NULL;
   captured_globals = NULL;

   // Force load so ensure_loaded() succeeds
   shim_test_ensure_loaded();

   // Set up the variant's GiveFnptrsToDll to our mock
   p_GiveFnptrsToDll = (GIVE_ENGINE_FUNCTIONS_FN)mock_variant_GiveFnptrsToDll;

   // Set up a fake engine function table
   enginefuncs_t fake_eng;
   memset(&fake_eng, 0, sizeof(fake_eng));
   fake_eng.pfnCVarRegister = mock_engine_CVarRegister;
   fake_eng.pfnCvar_RegisterVariable = mock_engine_CVarRegister;
   fake_eng.pfnAddServerCommand = mock_engine_AddServerCommand;

   globalvars_t fake_globals;
   memset(&fake_globals, 0, sizeof(fake_globals));

   GiveFnptrsToDll(&fake_eng, &fake_globals);

   // Variant should have received the patched copy, not the original
   if (captured_engfuncs == NULL) {
      printf("  variant GiveFnptrsToDll was not called\n");
      return 1;
   }
   if (captured_engfuncs == &fake_eng) {
      printf("  variant got original engfuncs, not the patched copy\n");
      return 1;
   }
   // The patched table should have our interceptors
   if ((void *)captured_engfuncs->pfnCVarRegister == (void *)mock_engine_CVarRegister) {
      printf("  pfnCVarRegister was not replaced with shim interceptor\n");
      return 1;
   }
   if ((void *)captured_engfuncs->pfnCvar_RegisterVariable == (void *)mock_engine_CVarRegister) {
      printf("  pfnCvar_RegisterVariable was not replaced\n");
      return 1;
   }
   if ((void *)captured_engfuncs->pfnAddServerCommand == (void *)mock_engine_AddServerCommand) {
      printf("  pfnAddServerCommand was not replaced with shim interceptor\n");
      return 1;
   }
   // Globals should pass through
   if (captured_globals != &fake_globals) {
      printf("  globals pointer was not passed through\n");
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
   fail |= test_cvar_register_copies_to_local_storage();
   fail |= test_cvar_register_multiple();
   fail |= test_cvar_register_overflow_passes_original();
   fail |= test_cvar_register_noop_without_real();
   fail |= test_cmd_register_uses_trampoline();
   fail |= test_cmd_trampoline_calls_target();
   fail |= test_cmd_register_overflow_passes_original();
   fail |= test_cmd_register_noop_without_real();
   fail |= test_give_fnptrs_patches_engfuncs();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
