//
// JK_Botti - tests for bot_trace.cpp
//
// test_bot_trace.cpp
//

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_trace.h"

#include "engine_mock.h"
#include "test_common.h"

extern qboolean is_team_play;

// ============================================================
// Capture buffer for ALERT(at_logged, ...) output
// ============================================================

static char alert_log_buf[2048];
static int alert_log_count;

static void mock_pfnAlertMessage_capture(ALERT_TYPE atype, char *szFmt, ...)
{
   if (atype == at_logged)
   {
      va_list ap;
      va_start(ap, szFmt);
      vsnprintf(alert_log_buf, sizeof(alert_log_buf), szFmt, ap);
      va_end(ap);
      alert_log_count++;
   }
}

// ============================================================
// Mock cvar storage for jk_botti_trace
// ============================================================

static float mock_jk_botti_trace_val = 0.0f;
static int mock_cvar_register_count = 0;
static int mock_cvar_set_float_count = 0;
static float mock_cvar_set_float_last = 0.0f;

static float mock_pfnCVarGetFloat_trace(const char *szVarName)
{
   if (strcmp(szVarName, "jk_botti_trace") == 0)
      return mock_jk_botti_trace_val;
   return 0.0f;
}

static void mock_pfnCVarRegister_trace(cvar_t *pCvar)
{
   (void)pCvar;
   mock_cvar_register_count++;
}

static void mock_pfnCVarSetFloat_trace(const char *szVarName, float val)
{
   if (strcmp(szVarName, "jk_botti_trace") == 0)
      mock_jk_botti_trace_val = val;
   mock_cvar_set_float_count++;
   mock_cvar_set_float_last = val;
}

// ============================================================
// Test helpers
// ============================================================

static void reset_trace_test(void)
{
   mock_reset();
   g_engfuncs.pfnAlertMessage = mock_pfnAlertMessage_capture;
   g_engfuncs.pfnCVarGetFloat = mock_pfnCVarGetFloat_trace;
   g_engfuncs.pfnCVarRegister = mock_pfnCVarRegister_trace;
   g_engfuncs.pfnCVarSetFloat = mock_pfnCVarSetFloat_trace;

   alert_log_buf[0] = 0;
   alert_log_count = 0;
   mock_jk_botti_trace_val = 0.0f;
   mock_cvar_register_count = 0;
   mock_cvar_set_float_count = 0;
   mock_cvar_set_float_last = 0.0f;

   bot_trace_level = BOT_TRACE_OFF;
}

static bot_t *setup_bot(const char *name)
{
   edict_t *e = mock_alloc_edict();
   e->v.netname = (string_t)(long)name;
   e->v.flags = FL_FAKECLIENT;

   int idx = -1;
   for (int i = 0; i < 32; i++)
   {
      if (!bots[i].is_used)
      {
         idx = i;
         break;
      }
   }
   if (idx < 0)
      return NULL;

   memset(&bots[idx], 0, sizeof(bot_t));
   bots[idx].is_used = TRUE;
   bots[idx].pEdict = e;
   return &bots[idx];
}

// ============================================================
// BotTraceRegisterCvar tests
// ============================================================

static int test_register_cvar(void)
{
   TEST("BotTraceRegisterCvar: registers cvar");
   reset_trace_test();

   // Reset internal state by calling with fresh mock
   BotTraceRegisterCvar();

   ASSERT_INT(mock_cvar_register_count, 1);
   PASS();
   return 0;
}

// ============================================================
// BotTraceUpdateCache tests
// ============================================================

static int test_update_cache_off(void)
{
   TEST("BotTraceUpdateCache: cvar=0 -> BOT_TRACE_OFF");
   reset_trace_test();
   BotTraceRegisterCvar();
   mock_jk_botti_trace_val = 0.0f;
   bot_trace_level = 99;

   BotTraceUpdateCache();

   ASSERT_INT(bot_trace_level, BOT_TRACE_OFF);
   PASS();
   return 0;
}

static int test_update_cache_log(void)
{
   TEST("BotTraceUpdateCache: cvar=1 -> BOT_TRACE_LOG");
   reset_trace_test();
   BotTraceRegisterCvar();
   mock_jk_botti_trace_val = 1.0f;

   BotTraceUpdateCache();

   ASSERT_INT(bot_trace_level, BOT_TRACE_LOG);
   PASS();
   return 0;
}

static int test_update_cache_say(void)
{
   TEST("BotTraceUpdateCache: cvar=2 -> BOT_TRACE_SAY");
   reset_trace_test();
   BotTraceRegisterCvar();
   mock_jk_botti_trace_val = 2.0f;

   BotTraceUpdateCache();

   ASSERT_INT(bot_trace_level, BOT_TRACE_SAY);
   PASS();
   return 0;
}

static int test_update_cache_clamp_negative(void)
{
   TEST("BotTraceUpdateCache: cvar=-1 -> clamped to 0");
   reset_trace_test();
   BotTraceRegisterCvar();
   mock_jk_botti_trace_val = -1.0f;

   BotTraceUpdateCache();

   ASSERT_INT(bot_trace_level, BOT_TRACE_OFF);
   PASS();
   return 0;
}

static int test_update_cache_clamp_high(void)
{
   TEST("BotTraceUpdateCache: cvar=5 -> clamped to 2");
   reset_trace_test();
   BotTraceRegisterCvar();
   mock_jk_botti_trace_val = 5.0f;

   BotTraceUpdateCache();

   ASSERT_INT(bot_trace_level, BOT_TRACE_SAY);
   PASS();
   return 0;
}

static int test_update_cache_before_register(void)
{
   TEST("BotTraceUpdateCache: before register -> no-op");
   reset_trace_test();
   // Don't call BotTraceRegisterCvar
   mock_jk_botti_trace_val = 1.0f;
   bot_trace_level = BOT_TRACE_OFF;

   BotTraceUpdateCache();

   // Should remain OFF because cvar not registered yet
   ASSERT_INT(bot_trace_level, BOT_TRACE_OFF);
   PASS();
   return 0;
}

// ============================================================
// BotTrace macro tests
// ============================================================

static int test_macro_skip_when_off(void)
{
   TEST("BotTrace macro: level=0 -> no call, no log output");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_OFF;

   bot_t *pBot = setup_bot("TestBot");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTrace(*pBot, "should not appear %d", 42);

   ASSERT_INT(alert_log_count, 0);
   ASSERT_STR(alert_log_buf, "");
   PASS();
   return 0;
}

static int test_macro_calls_when_log(void)
{
   TEST("BotTrace macro: level=1 -> produces log output");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_LOG;

   bot_t *pBot = setup_bot("TestBot");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTrace(*pBot, "hello %s", "world");

   ASSERT_INT(alert_log_count, 1);
   ASSERT_TRUE(strstr(alert_log_buf, "[TRACE] hello world") != NULL);
   PASS();
   return 0;
}

// ============================================================
// BotTracePrintf tests - LOG mode
// ============================================================

static int test_log_format_ffa(void)
{
   TEST("BotTracePrintf LOG: FFA format with [TRACE] prefix");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_LOG;
   is_team_play = FALSE;

   bot_t *pBot = setup_bot("FragBot");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTracePrintf(*pBot, "enemy found dist=%d", 420);

   ASSERT_INT(alert_log_count, 1);
   // Should contain bot name, say keyword, [TRACE] prefix, message
   ASSERT_TRUE(strstr(alert_log_buf, "FragBot") != NULL);
   ASSERT_TRUE(strstr(alert_log_buf, "say") != NULL);
   ASSERT_TRUE(strstr(alert_log_buf, "[TRACE] enemy found dist=420") != NULL);
   ASSERT_TRUE(strstr(alert_log_buf, "\n") != NULL);
   PASS();
   return 0;
}

static int test_log_format_teamplay(void)
{
   TEST("BotTracePrintf LOG: teamplay format");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_LOG;
   is_team_play = TRUE;

   bot_t *pBot = setup_bot("TeamBot");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTracePrintf(*pBot, "weapon switch");

   ASSERT_INT(alert_log_count, 1);
   ASSERT_TRUE(strstr(alert_log_buf, "TeamBot") != NULL);
   ASSERT_TRUE(strstr(alert_log_buf, "[TRACE] weapon switch") != NULL);
   PASS();
   return 0;
}

static int test_log_format_printf_args(void)
{
   TEST("BotTracePrintf LOG: multiple format args");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_LOG;

   bot_t *pBot = setup_bot("Bot1");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTracePrintf(*pBot, "wp %d -> %d len=%d goal=%s", 42, 55, 5, "item");

   ASSERT_TRUE(strstr(alert_log_buf, "[TRACE] wp 42 -> 55 len=5 goal=item") != NULL);
   PASS();
   return 0;
}

// ============================================================
// BotTracePrintf tests - SAY mode
// ============================================================

// For SAY mode, BotTraceSay calls UTIL_HostSay which calls
// UTIL_LogPrintf (-> ALERT) AND MESSAGE_BEGIN/END.
// We verify the ALERT output contains the trace message.

static int test_say_mode_produces_log(void)
{
   TEST("BotTracePrintf SAY: produces log output via HostSay");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_SAY;

   bot_t *pBot = setup_bot("SayBot");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTracePrintf(*pBot, "test say msg");

   // UTIL_HostSay calls UTIL_LogPrintf which calls ALERT
   ASSERT_TRUE(alert_log_count > 0);
   ASSERT_TRUE(strstr(alert_log_buf, "[TRACE] test say msg") != NULL);
   ASSERT_TRUE(strstr(alert_log_buf, "say") != NULL);
   PASS();
   return 0;
}

// ============================================================
// BotTracePrintf tests - defensive: invalid level
// ============================================================

static int test_printf_level_zero_no_output(void)
{
   TEST("BotTracePrintf: level=0 -> no output (defensive)");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_OFF;

   bot_t *pBot = setup_bot("DefBot");
   ASSERT_PTR_NOT_NULL(pBot);

   // Call BotTracePrintf directly, bypassing the macro guard
   BotTracePrintf(*pBot, "should not appear");

   ASSERT_INT(alert_log_count, 0);
   ASSERT_STR(alert_log_buf, "");
   PASS();
   return 0;
}

// ============================================================
// BotTrace macro with no variadic args
// ============================================================

static int test_macro_no_varargs(void)
{
   TEST("BotTrace macro: no variadic args (plain string)");
   reset_trace_test();
   BotTraceRegisterCvar();
   bot_trace_level = BOT_TRACE_LOG;

   bot_t *pBot = setup_bot("Bot2");
   ASSERT_PTR_NOT_NULL(pBot);

   BotTrace(*pBot, "simple message");

   ASSERT_TRUE(strstr(alert_log_buf, "[TRACE] simple message") != NULL);
   PASS();
   return 0;
}

// ============================================================
// Main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("=== bot_trace tests ===\n\n");

   printf(" BotTraceUpdateCache (before register):\n");
   fail |= test_update_cache_before_register();

   printf(" BotTraceRegisterCvar:\n");
   fail |= test_register_cvar();

   printf(" BotTraceUpdateCache:\n");
   fail |= test_update_cache_off();
   fail |= test_update_cache_log();
   fail |= test_update_cache_say();
   fail |= test_update_cache_clamp_negative();
   fail |= test_update_cache_clamp_high();

   printf(" BotTrace macro:\n");
   fail |= test_macro_skip_when_off();
   fail |= test_macro_calls_when_log();
   fail |= test_macro_no_varargs();

   printf(" BotTracePrintf LOG mode:\n");
   fail |= test_log_format_ffa();
   fail |= test_log_format_teamplay();
   fail |= test_log_format_printf_args();

   printf(" BotTracePrintf SAY mode:\n");
   fail |= test_say_mode_produces_log();

   printf(" BotTracePrintf defensive:\n");
   fail |= test_printf_level_zero_no_output();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail;
}
