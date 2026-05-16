/*
 * jk_botti Metamod plugin shim.
 *
 * Detects CPU features at runtime and forwards the Metamod plugin entry
 * points to the matching optimized variant (SSE3 or AVX2+FMA) loaded as
 * a sibling DLL/shared object.
 *
 * MUST be built with plain -march=i686 (no SSE/AVX enabled), since it
 * runs before we have proven the CPU supports those instruction sets.
 */

#include <string.h>
#include <stdint.h>
#ifndef SHIM_TEST
#include <cpuid.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

/* ---- variant filenames (siblings in the shim's own directory) ---------- */

#ifdef _WIN32
#  define VARIANT_X87     "jk_botti_mm_x87.dll"
#  define VARIANT_SSE3    "jk_botti_mm_sse3.dll"
#  define VARIANT_AVX2FMA "jk_botti_mm_avx2fma.dll"
#else
#  define VARIANT_X87     "jk_botti_mm_x87_i386.so"
#  define VARIANT_SSE3    "jk_botti_mm_sse3_i386.so"
#  define VARIANT_AVX2FMA "jk_botti_mm_avx2fma_i386.so"
#endif

/* ---- CPU feature detection --------------------------------------------- */

#ifndef SHIM_TEST

static inline uint64_t xgetbv_xcr0(void)
{
   uint32_t lo, hi;
   /* xgetbv with ECX=0 -> (EDX:EAX) = XCR0. */
   __asm__ volatile ("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
   return ((uint64_t)hi << 32) | lo;
}

static int has_avx2_fma(void)
{
   uint32_t a, b, c, d;

   if (!__get_cpuid(1, &a, &b, &c, &d))
      return 0;
   if (!(c & (1u << 27)))               /* OSXSAVE: xgetbv is safe to run */
      return 0;
   if ((xgetbv_xcr0() & 0x6u) != 0x6u)  /* OS saves XMM (bit 1) + YMM (bit 2) */
      return 0;
   if (!(c & (1u << 28)))               /* AVX */
      return 0;
   if (!(c & (1u << 12)))               /* FMA */
      return 0;

   if (!__get_cpuid_count(7, 0, &a, &b, &c, &d))
      return 0;
   if (!(b & (1u << 5)))                /* AVX2 */
      return 0;
   return 1;
}

static int has_sse3(void)
{
   uint32_t a, b, c, d;
   if (!__get_cpuid(1, &a, &b, &c, &d))
      return 0;
   return (c & 1u) != 0;                /* SSE3 (implies SSE/SSE2) */
}

#else /* SHIM_TEST: provided by test harness */

int has_avx2_fma(void);
int has_sse3(void);

#endif

/* ---- dlopen / LoadLibrary abstraction ---------------------------------- */

typedef void *dl_handle_t;

#ifndef SHIM_TEST

static dl_handle_t dl_open(const char *path, char *errbuf, size_t errcap)
{
#ifdef _WIN32
   HMODULE h = LoadLibraryA(path);
   if (!h)
      snprintf(errbuf, errcap, "LoadLibrary(%s) failed, GetLastError=%lu",
               path, (unsigned long)GetLastError());
   return (dl_handle_t)h;
#else
   void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      const char *e = dlerror();
      snprintf(errbuf, errcap, "%s", e ? e : "dlopen failed");
   }
   return h;
#endif
}

static void *dl_sym(dl_handle_t h, const char *name)
{
#ifdef _WIN32
   return (void *)GetProcAddress((HMODULE)h, name);
#else
   return dlsym(h, name);
#endif
}

static void dl_close(dl_handle_t h)
{
#ifdef _WIN32
   FreeLibrary((HMODULE)h);
#else
   dlclose(h);
#endif
}

#else /* SHIM_TEST: provided by test harness */

dl_handle_t dl_open(const char *path, char *errbuf, size_t errcap);
void *dl_sym(dl_handle_t h, const char *name);
void dl_close(dl_handle_t h);

#endif

/* ---- Metamod plugin ownership proxy ----------------------------------- */
/*
 * Metamod identifies which plugin owns a cvar/command by calling dladdr()
 * on the cvar_t pointer or command-handler function pointer.  Since the
 * shim loads the variant with dlopen(), those addresses resolve to the
 * variant .so, not the shim .so that metamod actually loaded.  The fix:
 * intercept pfnCVarRegister and pfnAddServerCommand in the engine function
 * table, copying data into shim-local storage so dladdr() resolves to us.
 */

typedef __typeof__(((enginefuncs_t*)NULL)->pfnCVarRegister) pfn_CVarRegister_t;
typedef __typeof__(((enginefuncs_t*)NULL)->pfnAddServerCommand) pfn_AddServerCommand_t;

#define MAX_PROXY_CVARS 8
static cvar_t g_proxy_cvars[MAX_PROXY_CVARS];
static int g_proxy_cvar_count;
static pfn_CVarRegister_t g_real_CVarRegister;

#define MAX_PROXY_CMDS 4
static void (*g_proxy_cmd_targets[MAX_PROXY_CMDS])(void);

static void FORCE_STACK_ALIGN proxy_cmd_handler_0(void) { if (g_proxy_cmd_targets[0]) g_proxy_cmd_targets[0](); }
static void FORCE_STACK_ALIGN proxy_cmd_handler_1(void) { if (g_proxy_cmd_targets[1]) g_proxy_cmd_targets[1](); }
static void FORCE_STACK_ALIGN proxy_cmd_handler_2(void) { if (g_proxy_cmd_targets[2]) g_proxy_cmd_targets[2](); }
static void FORCE_STACK_ALIGN proxy_cmd_handler_3(void) { if (g_proxy_cmd_targets[3]) g_proxy_cmd_targets[3](); }

typedef void (*pfn_void_void)(void);
static const pfn_void_void g_proxy_cmd_handlers[MAX_PROXY_CMDS] = {
   proxy_cmd_handler_0, proxy_cmd_handler_1,
   proxy_cmd_handler_2, proxy_cmd_handler_3,
};
static int g_proxy_cmd_count;
static pfn_AddServerCommand_t g_real_AddServerCommand;

static void FORCE_STACK_ALIGN shim_CVarRegister(cvar_t *pCvar)
{
   if (!g_real_CVarRegister)
      return;
   if (g_proxy_cvar_count < MAX_PROXY_CVARS) {
      cvar_t *local = &g_proxy_cvars[g_proxy_cvar_count++];
      *local = *pCvar;
      g_real_CVarRegister(local);
   } else {
      g_real_CVarRegister(pCvar);
   }
}

static void FORCE_STACK_ALIGN shim_AddServerCommand(char *cmd_name, void (*function)(void))
{
   if (!g_real_AddServerCommand)
      return;
   if (g_proxy_cmd_count < MAX_PROXY_CMDS) {
      int idx = g_proxy_cmd_count++;
      g_proxy_cmd_targets[idx] = function;
      g_real_AddServerCommand(cmd_name, g_proxy_cmd_handlers[idx]);
   } else {
      g_real_AddServerCommand(cmd_name, function);
   }
}

static enginefuncs_t g_engfuncs_copy;

/* ---- forward-declaration of variant entry points ----------------------- */

static dl_handle_t              g_variant;
static GIVE_ENGINE_FUNCTIONS_FN p_GiveFnptrsToDll;
static META_QUERY_FN            p_Meta_Query;
static META_ATTACH_FN           p_Meta_Attach;
static META_DETACH_FN           p_Meta_Detach;
static GETENTITYAPI2_FN         p_GetEntityAPI2;
static GETENTITYAPI2_FN         p_GetEntityAPI2_POST;
static GET_ENGINE_FUNCTIONS_FN  p_GetEngineFunctions;
static GET_ENGINE_FUNCTIONS_FN  p_GetEngineFunctions_POST;

/* ---- self path resolution ---------------------------------------------- */

#ifndef SHIM_TEST

static int self_directory(char *out, size_t cap)
{
#ifdef _WIN32
   HMODULE self = NULL;
   if (!GetModuleHandleExA(
           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
           (LPCSTR)(void *)&self_directory, &self))
      return 0;
   DWORD n = GetModuleFileNameA(self, out, (DWORD)cap);
   if (n == 0 || n >= cap)
      return 0;
#else
   Dl_info info;
   if (!dladdr((void *)&self_directory, &info) || !info.dli_fname)
      return 0;
   size_t n = strlen(info.dli_fname);
   if (n + 1 > cap)
      return 0;
   memcpy(out, info.dli_fname, n + 1);
#endif

   /* strip filename, keep trailing separator */
   size_t i = strlen(out);
   while (i > 0) {
      char ch = out[i - 1];
      if (ch == '/' || ch == '\\') break;
      --i;
   }
   if (i == 0) {
      if (cap < 3) return 0;
      out[0] = '.';
      out[1] = '/';
      out[2] = 0;
   } else {
      out[i] = 0;
   }
   return 1;
}

#else /* SHIM_TEST: provided by test harness */

int self_directory(char *out, size_t cap);

#endif

/* ---- one-shot init ----------------------------------------------------- */

static int g_init_done;
static int g_init_ok;

static void log_err(const char *msg)
{
#ifdef SHIM_TEST
   (void)msg;  /* tests would drown in detection noise */
#else
   fprintf(stderr, "jk_botti shim: %s\n", msg);
   fflush(stderr);
#  ifdef _WIN32
   OutputDebugStringA("jk_botti shim: ");
   OutputDebugStringA(msg);
   OutputDebugStringA("\r\n");
#  endif
#endif
}

struct resolve_entry {
   const char *name;
   void      **slot;
};

static int resolve_symbol(dl_handle_t h, const struct resolve_entry *e,
                          const char *path, char *errbuf, size_t errcap)
{
   void *s = dl_sym(h, e->name);
   if (!s) {
      snprintf(errbuf, errcap, "symbol %s missing in %s", e->name, path);
      log_err(errbuf);
      return 0;
   }
   *e->slot = s;
   return 1;
}

static int load_variant(const char *dir, const char *leaf)
{
   static const struct resolve_entry table[] = {
      { "GiveFnptrsToDll",         (void **)&p_GiveFnptrsToDll },
      { "Meta_Query",              (void **)&p_Meta_Query },
      { "Meta_Attach",             (void **)&p_Meta_Attach },
      { "Meta_Detach",             (void **)&p_Meta_Detach },
      { "GetEntityAPI2",           (void **)&p_GetEntityAPI2 },
      { "GetEntityAPI2_POST",      (void **)&p_GetEntityAPI2_POST },
      { "GetEngineFunctions",      (void **)&p_GetEngineFunctions },
      { "GetEngineFunctions_POST", (void **)&p_GetEngineFunctions_POST },
   };

   char path[1024];
   char errbuf[2048];

   int n = snprintf(path, sizeof(path), "%s%s", dir, leaf);
   if (n < 0 || (size_t)n >= sizeof(path)) {
      log_err("variant path too long");
      return 0;
   }

   g_variant = dl_open(path, errbuf, sizeof(errbuf));
   if (!g_variant) {
      log_err(errbuf);
      return 0;
   }

   for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
      if (!resolve_symbol(g_variant, &table[i], path, errbuf, sizeof(errbuf))) {
         dl_close(g_variant);
         g_variant = NULL;
         for (size_t j = 0; j < sizeof(table) / sizeof(table[0]); ++j)
            *table[j].slot = NULL;
         return 0;
      }
   }

#ifndef SHIM_TEST
   fprintf(stderr, "jk_botti shim: loaded variant %s\n", path);
   fflush(stderr);
#endif
   return 1;
}

static int ensure_loaded(void)
{
   if (g_init_done)
      return g_init_ok;
   g_init_done = 1;

   char dir[1024];
   if (!self_directory(dir, sizeof(dir))) {
      log_err("could not determine own path");
      return 0;
   }

   /* Try the best supported variant first, fall back step by step. */
   const char *candidates[3];
   int n = 0;
   if (has_avx2_fma()) candidates[n++] = VARIANT_AVX2FMA;
   if (has_sse3())     candidates[n++] = VARIANT_SSE3;
   candidates[n++] = VARIANT_X87;   /* always safe on -march=i686 hardware */

   for (int i = 0; i < n; ++i) {
      if (i > 0) {
         char msg[128];
         snprintf(msg, sizeof(msg), "falling back to %s", candidates[i]);
         log_err(msg);
      }
      if (load_variant(dir, candidates[i])) {
         g_init_ok = 1;
         return 1;
      }
   }
   return 0;
}

/* ---- exported Metamod entry points ------------------------------------- */

C_DLLEXPORT FORCE_STACK_ALIGN void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine,
							  globalvars_t *pGlobals)
{
   if (!ensure_loaded())
      return;

   g_engfuncs_copy = *pengfuncsFromEngine;

   g_real_CVarRegister = (pfn_CVarRegister_t)g_engfuncs_copy.pfnCVarRegister;
   g_engfuncs_copy.pfnCVarRegister = shim_CVarRegister;
   g_engfuncs_copy.pfnCvar_RegisterVariable = shim_CVarRegister;

   g_real_AddServerCommand = (pfn_AddServerCommand_t)g_engfuncs_copy.pfnAddServerCommand;
   g_engfuncs_copy.pfnAddServerCommand = shim_AddServerCommand;

   p_GiveFnptrsToDll(&g_engfuncs_copy, pGlobals);
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Query(char *interfaceVersion,
					     plugin_info_t **plinfo,
					     mutil_funcs_t *pMetaUtilFuncs)
{
   if (!ensure_loaded())
      return 0;
   return p_Meta_Query(interfaceVersion, plinfo, pMetaUtilFuncs);
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Attach(PLUG_LOADTIME now,
					      META_FUNCTIONS *pFunctionTable,
					      meta_globals_t *pMGlobals,
					      gamedll_funcs_t *pGamedllFuncs)
{
   if (!ensure_loaded())
      return 0;
   return p_Meta_Attach(now, pFunctionTable, pMGlobals, pGamedllFuncs);
}

C_DLLEXPORT FORCE_STACK_ALIGN int Meta_Detach(PLUG_LOADTIME now,
					      PL_UNLOAD_REASON reason)
{
   if (!g_init_ok)
      return 1;
   return p_Meta_Detach(now, reason);
}

C_DLLEXPORT FORCE_STACK_ALIGN int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable,
						int *interfaceVersion)
{
   if (!ensure_loaded())
      return 0;
   return p_GetEntityAPI2(pFunctionTable, interfaceVersion);
}

C_DLLEXPORT FORCE_STACK_ALIGN int GetEntityAPI2_POST(DLL_FUNCTIONS *pFunctionTable,
						     int *interfaceVersion)
{
   if (!ensure_loaded())
      return 0;
   return p_GetEntityAPI2_POST(pFunctionTable, interfaceVersion);
}

C_DLLEXPORT FORCE_STACK_ALIGN int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine,
						     int *interfaceVersion)
{
   if (!ensure_loaded())
      return 0;
   return p_GetEngineFunctions(pengfuncsFromEngine, interfaceVersion);
}

C_DLLEXPORT FORCE_STACK_ALIGN int GetEngineFunctions_POST(enginefuncs_t *pengfuncsFromEngine,
							  int *interfaceVersion)
{
   if (!ensure_loaded())
      return 0;
   return p_GetEngineFunctions_POST(pengfuncsFromEngine, interfaceVersion);
}

#if defined(_WIN32) && !defined(SHIM_TEST)
FORCE_STACK_ALIGN BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
   (void)hinst; (void)reason; (void)reserved;
   return TRUE;
}
#endif

#ifdef SHIM_TEST
/* Test-only accessors for unit-testing the shim. */
void shim_test_reset(void)
{
   g_init_done = 0;
   g_init_ok = 0;
   g_variant = NULL;
   p_GiveFnptrsToDll = NULL;
   p_Meta_Query = NULL;
   p_Meta_Attach = NULL;
   p_Meta_Detach = NULL;
   p_GetEntityAPI2 = NULL;
   p_GetEntityAPI2_POST = NULL;
   p_GetEngineFunctions = NULL;
   p_GetEngineFunctions_POST = NULL;
   g_real_CVarRegister = NULL;
   g_real_AddServerCommand = NULL;
   g_proxy_cvar_count = 0;
   g_proxy_cmd_count = 0;
   memset(g_proxy_cvars, 0, sizeof(g_proxy_cvars));
   memset(g_proxy_cmd_targets, 0, sizeof(g_proxy_cmd_targets));
   memset(&g_engfuncs_copy, 0, sizeof(g_engfuncs_copy));
}

int shim_test_ensure_loaded(void)             { return ensure_loaded(); }
int shim_test_load_variant(const char *dir,
                           const char *leaf)  { return load_variant(dir, leaf); }

/* Proxy test accessors */
cvar_t *shim_test_get_proxy_cvar(int idx)     { return &g_proxy_cvars[idx]; }
int shim_test_get_proxy_cvar_count(void)      { return g_proxy_cvar_count; }
int shim_test_get_proxy_cmd_count(void)       { return g_proxy_cmd_count; }
pfn_void_void shim_test_get_proxy_cmd_handler(int idx) { return g_proxy_cmd_handlers[idx]; }
void shim_test_call_CVarRegister(cvar_t *cv)  { shim_CVarRegister(cv); }
void shim_test_call_AddServerCommand(char *name, void (*fn)(void)) { shim_AddServerCommand(name, fn); }
void shim_test_set_real_CVarRegister(pfn_CVarRegister_t fn) { g_real_CVarRegister = fn; }
void shim_test_set_real_AddServerCommand(pfn_AddServerCommand_t fn) { g_real_AddServerCommand = fn; }
enginefuncs_t *shim_test_get_engfuncs_copy(void) { return &g_engfuncs_copy; }
#endif
