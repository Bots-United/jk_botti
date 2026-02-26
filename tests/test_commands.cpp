//
// JK_Botti - comprehensive tests for commands.cpp
//
// test_commands.cpp
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
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "bot_query_hook.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Stubs for symbols not in engine_mock or linked .o files
// ============================================================

// dll.cpp globals
int default_bot_skill = 2;
int bot_add_level_tag = 0;
int bot_chat_percent = 10;
int bot_taunt_percent = 20;
int bot_whine_percent = 10;
int bot_endgame_percent = 40;
int bot_logo_percent = 40;
int bot_chat_tag_percent = 80;
int bot_chat_drop_percent = 10;
int bot_chat_swap_percent = 10;
int bot_chat_lower_percent = 50;
qboolean b_random_color = TRUE;
int debug_minmax = 0;
float bot_think_spf = 1.0f/30.0f;
float bot_check_time = 60.0;
int min_bots = -1;
int max_bots = -1;
int randomize_bots_on_mapchange = 0;
FILE *bot_cfg_fp = NULL;
int bot_cfg_linenumber = 0;
int team_balancetype = 1;
char *team_blockedlist = NULL;
float bot_cfg_pause_time = 0.0;
int bot_stop = 0;

// engine.cpp globals
qboolean g_in_intermission = FALSE;
char g_argv[1024*3];
char g_arg1[1024];
char g_arg2[1024];
char g_arg3[1024];

// waypoint globals
qboolean g_waypoint_on = FALSE;
qboolean g_auto_waypoint = FALSE;
qboolean g_path_waypoint = FALSE;
qboolean g_path_waypoint_enable = FALSE;
qboolean g_waypoint_updated = FALSE;
float wp_display_time[MAX_WAYPOINTS];
BOOL wp_matrix_save_on_mapend = FALSE;

// bot_query_hook
bool hook_sendto_function(void) { return true; }
bool unhook_sendto_function(void) { return true; }

// waypoint.cpp functions
void WaypointAdd(edict_t *pEntity, int flags) { (void)pEntity; (void)flags; }
void WaypointDelete(edict_t *pEntity) { (void)pEntity; }
void WaypointCreatePath(edict_t *pEntity, int cmd) { (void)pEntity; (void)cmd; }
void WaypointRemovePath(edict_t *pEntity, int cmd) { (void)pEntity; (void)cmd; }
qboolean WaypointLoad(edict_t *pEntity) { (void)pEntity; return FALSE; }
void WaypointSave(void) {}
void WaypointPrintInfo(edict_t *pEntity) { (void)pEntity; }
void WaypointUpdate(edict_t *pEntity) { (void)pEntity; }

// ============================================================
// Externs from commands.cpp
// ============================================================

extern qboolean isFakeClientCommand;
extern int bot_conntimes;
extern int cfg_bot_record_size;
extern cfg_bot_record_t *cfg_bot_record;
extern int fake_arg_count;

// ============================================================
// Externs from engine_mock.cpp (weak symbols)
// ============================================================

extern qboolean b_observer_mode;
extern qboolean b_botdontshoot;
extern int bot_shoot_breakables;
extern bot_skill_settings_t skill_settings[5];

// ============================================================
// Mock Argv/Args/Argc for jk_botti_ServerCommand
// ============================================================

static const char *mock_argv_values[8];
static int mock_argc_value;

static const char *mock_Cmd_Argv(int argc)
{
   if (argc >= 0 && argc < 8 && mock_argv_values[argc])
      return mock_argv_values[argc];
   return "";
}

static const char *mock_Cmd_Args(void)
{
   return mock_argv_values[1] ? mock_argv_values[1] : "";
}

static int mock_Cmd_Argc(void)
{
   return mock_argc_value;
}

// ============================================================
// Mock for SERVER_COMMAND and SERVER_EXECUTE
// ============================================================

static char mock_server_cmd_buf[256];
static int mock_server_execute_count;

static void mock_pfnServerCommand(const char *str)
{
   if (str)
      safe_strcopy(mock_server_cmd_buf, sizeof(mock_server_cmd_buf), str);
}

static void mock_pfnServerExecute(void)
{
   mock_server_execute_count++;
}

// ============================================================
// Mock for IS_DEDICATED_SERVER
// ============================================================

static int mock_dedicated_server_value;

static int mock_pfnIsDedicatedServer(void)
{
   return mock_dedicated_server_value;
}

// ============================================================
// Mock for BotCreate tracking
// ============================================================

static int mock_bot_create_count;
static char mock_bot_create_last_skin[64];
static char mock_bot_create_last_name[64];
static int mock_bot_create_last_skill;

void BotCreate(const char *skin, const char *name, int skill, int top_color,
               int bottom_color, int cfg_bot_index)
{
   mock_bot_create_count++;
   if (skin)
      safe_strcopy(mock_bot_create_last_skin, sizeof(mock_bot_create_last_skin), skin);
   else
      mock_bot_create_last_skin[0] = 0;
   if (name)
      safe_strcopy(mock_bot_create_last_name, sizeof(mock_bot_create_last_name), name);
   else
      mock_bot_create_last_name[0] = 0;
   mock_bot_create_last_skill = skill;
   (void)top_color; (void)bottom_color; (void)cfg_bot_index;
}

// ============================================================
// Mock for BotKick tracking
// ============================================================

static int mock_bot_kick_count;

void BotKick(bot_t &pBot)
{
   mock_bot_kick_count++;
   pBot.is_used = FALSE;
}

// ============================================================
// Stubs
// ============================================================

void BotThink(bot_t &pBot) { (void)pBot; }
void BotCheckTeamplay(void) {}
void LoadBotChat(void) {}
void LoadBotModels(void) {}
void ResetSkillsToDefault(void) {}
void jkbotti_ClientPutInServer(edict_t *pEntity) { (void)pEntity; }
BOOL jkbotti_ClientConnect(edict_t *pEntity, const char *pszName,
                           const char *pszAddress, char szRejectReason[128])
{ (void)pEntity; (void)pszName; (void)pszAddress; (void)szRejectReason; return TRUE; }

// Mock MDLL callback (FakeClientCommand calls MDLL_ClientCommand via
// gpGamedllFuncs->dllapi_table->pfnClientCommand)
static void mock_pfnClientCommand(edict_t *pEntity)
{
   (void)pEntity;
}

// ============================================================
// Test helpers
// ============================================================

static void setup_mock_argv(const char *a0, const char *a1, const char *a2,
                             const char *a3, const char *a4, const char *a5,
                             const char *a6)
{
   mock_argv_values[0] = a0;
   mock_argv_values[1] = a1;
   mock_argv_values[2] = a2;
   mock_argv_values[3] = a3;
   mock_argv_values[4] = a4;
   mock_argv_values[5] = a5;
   mock_argv_values[6] = a6;
   mock_argv_values[7] = NULL;

   mock_argc_value = 0;
   for (int i = 0; i < 8; i++) {
      if (mock_argv_values[i] && mock_argv_values[i][0])
         mock_argc_value = i + 1;
      else
         break;
   }
}

static void reset_test_state(void)
{
   mock_reset();

   // Set up MDLL callback (FakeClientCommand calls MDLL_ClientCommand)
   gpGamedllFuncs->dllapi_table->pfnClientCommand = mock_pfnClientCommand;

   // Install Argv/Args/Argc mocks
   g_engfuncs.pfnCmd_Argv = mock_Cmd_Argv;
   g_engfuncs.pfnCmd_Args = mock_Cmd_Args;
   g_engfuncs.pfnCmd_Argc = mock_Cmd_Argc;
   g_engfuncs.pfnServerCommand = (void (*)(char *))mock_pfnServerCommand;
   g_engfuncs.pfnServerExecute = mock_pfnServerExecute;
   g_engfuncs.pfnIsDedicatedServer = mock_pfnIsDedicatedServer;

   memset(mock_argv_values, 0, sizeof(mock_argv_values));
   mock_argc_value = 0;

   mock_server_cmd_buf[0] = 0;
   mock_server_execute_count = 0;
   mock_dedicated_server_value = 1; // default: dedicated

   mock_bot_create_count = 0;
   mock_bot_create_last_skin[0] = 0;
   mock_bot_create_last_name[0] = 0;
   mock_bot_create_last_skill = 0;
   mock_bot_kick_count = 0;

   // Reset commands.cpp globals
   isFakeClientCommand = FALSE;
   bot_conntimes = 0;
   FreeCfgBotRecord();

   // Reset dll.cpp globals
   default_bot_skill = 2;
   bot_add_level_tag = 0;
   bot_chat_percent = 10;
   bot_taunt_percent = 20;
   bot_whine_percent = 10;
   bot_endgame_percent = 40;
   bot_logo_percent = 40;
   bot_chat_tag_percent = 80;
   bot_chat_drop_percent = 10;
   bot_chat_swap_percent = 10;
   bot_chat_lower_percent = 50;
   b_random_color = TRUE;
   debug_minmax = 0;
   bot_think_spf = 1.0f / 30.0f;
   bot_check_time = 60.0;
   min_bots = -1;
   max_bots = -1;
   randomize_bots_on_mapchange = 0;
   bot_cfg_fp = NULL;
   bot_cfg_linenumber = 0;
   team_balancetype = 1;
   if (team_blockedlist) {
      free(team_blockedlist);
      team_blockedlist = NULL;
   }
   bot_cfg_pause_time = 0.0;

   // Reset waypoint globals
   g_waypoint_on = FALSE;
   g_auto_waypoint = FALSE;
   g_path_waypoint = FALSE;
   g_path_waypoint_enable = FALSE;

   g_argv[0] = 0;
   g_arg1[0] = 0;
   g_arg2[0] = 0;
   g_arg3[0] = 0;
}

// ============================================================
// CfgBotRecord CRUD tests
// ============================================================

static int test_AddToCfgBotRecord_basic(void)
{
   TEST("AddToCfgBotRecord: adds entry, returns index");
   reset_test_state();

   int idx = AddToCfgBotRecord("skin1", "Bot1", 3, 10, 20);
   ASSERT_INT(idx, 0);
   ASSERT_INT(cfg_bot_record_size, 1);
   ASSERT_STR(cfg_bot_record[0].skin, "skin1");
   ASSERT_STR(cfg_bot_record[0].name, "Bot1");
   ASSERT_INT(cfg_bot_record[0].skill, 3);
   ASSERT_INT(cfg_bot_record[0].top_color, 10);
   ASSERT_INT(cfg_bot_record[0].bottom_color, 20);
   ASSERT_INT(cfg_bot_record[0].index, 0);

   FreeCfgBotRecord();
   PASS();
   return 0;
}

static int test_AddToCfgBotRecord_multiple(void)
{
   TEST("AddToCfgBotRecord: multiple entries");
   reset_test_state();

   int idx0 = AddToCfgBotRecord("skin1", "Bot1", 1, 0, 0);
   int idx1 = AddToCfgBotRecord("skin2", "Bot2", 2, 5, 5);
   int idx2 = AddToCfgBotRecord(NULL, NULL, 3, -1, -1);

   ASSERT_INT(idx0, 0);
   ASSERT_INT(idx1, 1);
   ASSERT_INT(idx2, 2);
   ASSERT_INT(cfg_bot_record_size, 3);
   ASSERT_STR(cfg_bot_record[0].skin, "skin1");
   ASSERT_STR(cfg_bot_record[1].name, "Bot2");
   ASSERT_PTR_NULL(cfg_bot_record[2].skin);
   ASSERT_PTR_NULL(cfg_bot_record[2].name);
   ASSERT_INT(cfg_bot_record[2].skill, 3);

   FreeCfgBotRecord();
   PASS();
   return 0;
}

static int test_GetUnusedCfgBotRecord_no_records(void)
{
   TEST("GetUnusedCfgBotRecord: no records -> NULL");
   reset_test_state();

   const cfg_bot_record_t *rec = GetUnusedCfgBotRecord();
   ASSERT_PTR_NULL(rec);

   PASS();
   return 0;
}

static int test_GetUnusedCfgBotRecord_with_unused(void)
{
   TEST("GetUnusedCfgBotRecord: unused record -> returns it");
   reset_test_state();

   AddToCfgBotRecord("skin1", "Bot1", 1, 0, 0);

   const cfg_bot_record_t *rec = GetUnusedCfgBotRecord();
   ASSERT_PTR_NOT_NULL(rec);
   ASSERT_STR(rec->name, "Bot1");

   FreeCfgBotRecord();
   PASS();
   return 0;
}

static int test_GetUnusedCfgBotRecord_all_used(void)
{
   TEST("GetUnusedCfgBotRecord: all used -> NULL");
   reset_test_state();

   AddToCfgBotRecord("skin1", "Bot1", 1, 0, 0);
   bots[0].is_used = TRUE;
   bots[0].cfg_bot_index = 0;

   const cfg_bot_record_t *rec = GetUnusedCfgBotRecord();
   ASSERT_PTR_NULL(rec);

   FreeCfgBotRecord();
   PASS();
   return 0;
}

static int test_GetUnusedCfgBotRecord_some_used(void)
{
   TEST("GetUnusedCfgBotRecord: some used -> returns unused");
   reset_test_state();

   AddToCfgBotRecord("skin1", "Bot1", 1, 0, 0);
   AddToCfgBotRecord("skin2", "Bot2", 2, 0, 0);
   bots[0].is_used = TRUE;
   bots[0].cfg_bot_index = 0;

   const cfg_bot_record_t *rec = GetUnusedCfgBotRecord();
   ASSERT_PTR_NOT_NULL(rec);
   ASSERT_STR(rec->name, "Bot2");
   ASSERT_INT(rec->index, 1);

   FreeCfgBotRecord();
   PASS();
   return 0;
}

static int test_FreeCfgBotRecord_frees_all(void)
{
   TEST("FreeCfgBotRecord: frees everything");
   reset_test_state();

   AddToCfgBotRecord("skin1", "Bot1", 1, 0, 0);
   AddToCfgBotRecord("skin2", "Bot2", 2, 0, 0);
   ASSERT_INT(cfg_bot_record_size, 2);
   ASSERT_PTR_NOT_NULL(cfg_bot_record);

   FreeCfgBotRecord();
   ASSERT_INT(cfg_bot_record_size, 0);
   ASSERT_PTR_NULL(cfg_bot_record);

   PASS();
   return 0;
}

static int test_FreeCfgBotRecord_empty(void)
{
   TEST("FreeCfgBotRecord: empty is safe");
   reset_test_state();

   FreeCfgBotRecord();
   ASSERT_INT(cfg_bot_record_size, 0);
   ASSERT_PTR_NULL(cfg_bot_record);

   PASS();
   return 0;
}

// ============================================================
// FakeClientCommand tests
// ============================================================

static int test_FakeClientCommand_one_arg(void)
{
   TEST("FakeClientCommand: 1 arg");
   reset_test_state();
   edict_t *pBot = mock_alloc_edict();

   FakeClientCommand(pBot, "say", NULL, NULL);

   ASSERT_STR(g_argv, "say");
   ASSERT_STR(g_arg1, "say");
   ASSERT_STR(g_arg2, "");
   ASSERT_STR(g_arg3, "");
   ASSERT_INT(fake_arg_count, 1);

   PASS();
   return 0;
}

static int test_FakeClientCommand_two_args(void)
{
   TEST("FakeClientCommand: 2 args");
   reset_test_state();
   edict_t *pBot = mock_alloc_edict();

   FakeClientCommand(pBot, "say", "hello", NULL);

   ASSERT_STR(g_argv, "say hello");
   ASSERT_STR(g_arg1, "say");
   ASSERT_STR(g_arg2, "hello");
   ASSERT_STR(g_arg3, "");
   ASSERT_INT(fake_arg_count, 2);

   PASS();
   return 0;
}

static int test_FakeClientCommand_three_args(void)
{
   TEST("FakeClientCommand: 3 args");
   reset_test_state();
   edict_t *pBot = mock_alloc_edict();

   FakeClientCommand(pBot, "say", "hello", "world");

   ASSERT_STR(g_argv, "say hello world");
   ASSERT_STR(g_arg1, "say");
   ASSERT_STR(g_arg2, "hello");
   ASSERT_STR(g_arg3, "world");
   ASSERT_INT(fake_arg_count, 3);

   PASS();
   return 0;
}

static int test_FakeClientCommand_null_arg1(void)
{
   TEST("FakeClientCommand: NULL arg1 -> no crash, empty");
   reset_test_state();
   edict_t *pBot = mock_alloc_edict();

   g_argv[0] = 'X';
   g_argv[1] = 0;

   FakeClientCommand(pBot, NULL, NULL, NULL);

   ASSERT_STR(g_argv, "");
   ASSERT_STR(g_arg1, "");

   PASS();
   return 0;
}

static int test_FakeClientCommand_empty_arg1(void)
{
   TEST("FakeClientCommand: empty arg1 -> no crash");
   reset_test_state();
   edict_t *pBot = mock_alloc_edict();

   g_argv[0] = 'X';
   g_argv[1] = 0;

   FakeClientCommand(pBot, "", NULL, NULL);

   ASSERT_STR(g_argv, "");

   PASS();
   return 0;
}

// ============================================================
// ProcessCommand tests via jk_botti_ServerCommand
//
// jk_botti_ServerCommand reads CMD_ARGV(1) as the command,
// CMD_ARGV(2..6) as args.
// ============================================================

static int test_srv_botskill_set(void)
{
   TEST("ProcessCommand: botskill set to 3");
   reset_test_state();
   default_bot_skill = 2;
   setup_mock_argv("jk_botti", "botskill", "3", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(default_bot_skill, 3);
   PASS();
   return 0;
}

static int test_srv_botskill_invalid(void)
{
   TEST("ProcessCommand: botskill invalid value");
   reset_test_state();
   default_bot_skill = 2;
   setup_mock_argv("jk_botti", "botskill", "6", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(default_bot_skill, 2);
   PASS();
   return 0;
}

static int test_srv_botskill_zero(void)
{
   TEST("ProcessCommand: botskill 0 invalid");
   reset_test_state();
   default_bot_skill = 3;
   setup_mock_argv("jk_botti", "botskill", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(default_bot_skill, 3);
   PASS();
   return 0;
}

static int test_srv_botskill_query(void)
{
   TEST("ProcessCommand: botskill with no value queries");
   reset_test_state();
   default_bot_skill = 4;
   setup_mock_argv("jk_botti", "botskill", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(default_bot_skill, 4);
   PASS();
   return 0;
}

static int test_srv_botskill_all_valid(void)
{
   TEST("ProcessCommand: botskill all valid values 1-5");
   reset_test_state();
   for (int i = 1; i <= 5; i++) {
      char val[4];
      snprintf(val, sizeof(val), "%d", i);
      setup_mock_argv("jk_botti", "botskill", val, NULL, NULL, NULL, NULL);
      jk_botti_ServerCommand();
      ASSERT_INT(default_bot_skill, i);
   }
   PASS();
   return 0;
}

static int test_srv_min_bots_set(void)
{
   TEST("ProcessCommand: min_bots set to 5");
   reset_test_state();
   min_bots = -1;
   setup_mock_argv("jk_botti", "min_bots", "5", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, 5);
   PASS();
   return 0;
}

static int test_srv_min_bots_invalid(void)
{
   TEST("ProcessCommand: min_bots 32 -> -1");
   reset_test_state();
   min_bots = 5;
   setup_mock_argv("jk_botti", "min_bots", "32", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, -1);
   PASS();
   return 0;
}

static int test_srv_min_bots_negative(void)
{
   TEST("ProcessCommand: min_bots -2 -> -1");
   reset_test_state();
   min_bots = 5;
   setup_mock_argv("jk_botti", "min_bots", "-2", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, -1);
   PASS();
   return 0;
}

static int test_srv_min_bots_boundary(void)
{
   TEST("ProcessCommand: min_bots boundary values 0 and 31");
   reset_test_state();
   setup_mock_argv("jk_botti", "min_bots", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, 0);
   setup_mock_argv("jk_botti", "min_bots", "31", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, 31);
   PASS();
   return 0;
}

static int test_srv_max_bots_set(void)
{
   TEST("ProcessCommand: max_bots set to 10");
   reset_test_state();
   max_bots = -1;
   setup_mock_argv("jk_botti", "max_bots", "10", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(max_bots, 10);
   PASS();
   return 0;
}

static int test_srv_max_bots_invalid(void)
{
   TEST("ProcessCommand: max_bots 32 -> -1");
   reset_test_state();
   max_bots = 5;
   setup_mock_argv("jk_botti", "max_bots", "32", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(max_bots, -1);
   PASS();
   return 0;
}

static int test_srv_max_bots_boundary(void)
{
   TEST("ProcessCommand: max_bots boundary values 0 and 31");
   reset_test_state();
   setup_mock_argv("jk_botti", "max_bots", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(max_bots, 0);
   setup_mock_argv("jk_botti", "max_bots", "31", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(max_bots, 31);
   PASS();
   return 0;
}

static int test_srv_bot_conntimes_on(void)
{
   TEST("ProcessCommand: bot_conntimes enable (dedicated)");
   reset_test_state();
   mock_dedicated_server_value = 1;
   bot_conntimes = 0;
   setup_mock_argv("jk_botti", "bot_conntimes", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_conntimes, 1);
   PASS();
   return 0;
}

static int test_srv_bot_conntimes_off(void)
{
   TEST("ProcessCommand: bot_conntimes disable");
   reset_test_state();
   mock_dedicated_server_value = 1;
   bot_conntimes = 1;
   setup_mock_argv("jk_botti", "bot_conntimes", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_conntimes, 0);
   PASS();
   return 0;
}

static int test_srv_bot_conntimes_listenserver(void)
{
   TEST("ProcessCommand: bot_conntimes forced off on listen srv");
   reset_test_state();
   mock_dedicated_server_value = 0;
   bot_conntimes = 0;
   setup_mock_argv("jk_botti", "bot_conntimes", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_conntimes, 0);
   PASS();
   return 0;
}

static int test_srv_bot_conntimes_invalid(void)
{
   TEST("ProcessCommand: bot_conntimes invalid value 2");
   reset_test_state();
   mock_dedicated_server_value = 1;
   bot_conntimes = 0;
   setup_mock_argv("jk_botti", "bot_conntimes", "2", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_conntimes, 0);
   PASS();
   return 0;
}

static int test_srv_botdontshoot_on(void)
{
   TEST("ProcessCommand: botdontshoot enable");
   reset_test_state();
   b_botdontshoot = FALSE;
   setup_mock_argv("jk_botti", "botdontshoot", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(b_botdontshoot, TRUE);
   PASS();
   return 0;
}

static int test_srv_botdontshoot_off(void)
{
   TEST("ProcessCommand: botdontshoot disable");
   reset_test_state();
   b_botdontshoot = TRUE;
   setup_mock_argv("jk_botti", "botdontshoot", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(b_botdontshoot, FALSE);
   PASS();
   return 0;
}

static int test_srv_observer_on(void)
{
   TEST("ProcessCommand: observer enable");
   reset_test_state();
   b_observer_mode = FALSE;
   setup_mock_argv("jk_botti", "observer", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(b_observer_mode, TRUE);
   PASS();
   return 0;
}

static int test_srv_observer_off(void)
{
   TEST("ProcessCommand: observer disable");
   reset_test_state();
   b_observer_mode = TRUE;
   setup_mock_argv("jk_botti", "observer", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(b_observer_mode, FALSE);
   PASS();
   return 0;
}

static int test_srv_bot_shoot_breakables_set(void)
{
   TEST("ProcessCommand: bot_shoot_breakables set to 2");
   reset_test_state();
   bot_shoot_breakables = 0;
   setup_mock_argv("jk_botti", "bot_shoot_breakables", "2", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_shoot_breakables, 2);
   PASS();
   return 0;
}

static int test_srv_bot_shoot_breakables_invalid(void)
{
   TEST("ProcessCommand: bot_shoot_breakables invalid value 3");
   reset_test_state();
   bot_shoot_breakables = 1;
   setup_mock_argv("jk_botti", "bot_shoot_breakables", "3", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_shoot_breakables, 1);
   PASS();
   return 0;
}

static int test_srv_bot_shoot_breakables_boundary(void)
{
   TEST("ProcessCommand: bot_shoot_breakables boundary 0,1,2");
   reset_test_state();
   for (int i = 0; i <= 2; i++) {
      char val[4];
      snprintf(val, sizeof(val), "%d", i);
      setup_mock_argv("jk_botti", "bot_shoot_breakables", val, NULL, NULL, NULL, NULL);
      jk_botti_ServerCommand();
      ASSERT_INT(bot_shoot_breakables, i);
   }
   PASS();
   return 0;
}

static int test_srv_botthinkfps_set(void)
{
   TEST("ProcessCommand: botthinkfps set to 60");
   reset_test_state();
   bot_think_spf = 1.0f / 30.0f;
   setup_mock_argv("jk_botti", "botthinkfps", "60", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(bot_think_spf, 1.0f / 60.0f, 0.001f);
   PASS();
   return 0;
}

static int test_srv_botthinkfps_invalid_zero(void)
{
   TEST("ProcessCommand: botthinkfps 0 invalid");
   reset_test_state();
   float orig = bot_think_spf;
   setup_mock_argv("jk_botti", "botthinkfps", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(bot_think_spf, orig, 0.001f);
   PASS();
   return 0;
}

static int test_srv_botthinkfps_invalid_101(void)
{
   TEST("ProcessCommand: botthinkfps 101 invalid");
   reset_test_state();
   float orig = bot_think_spf;
   setup_mock_argv("jk_botti", "botthinkfps", "101", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(bot_think_spf, orig, 0.001f);
   PASS();
   return 0;
}

static int test_srv_botthinkfps_boundary(void)
{
   TEST("ProcessCommand: botthinkfps boundary values 1 and 100");
   reset_test_state();
   setup_mock_argv("jk_botti", "botthinkfps", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(bot_think_spf, 1.0f, 0.01f);
   setup_mock_argv("jk_botti", "botthinkfps", "100", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(bot_think_spf, 0.01f, 0.001f);
   PASS();
   return 0;
}

static int test_srv_team_balancetype_set(void)
{
   TEST("ProcessCommand: team_balancetype set to 0");
   reset_test_state();
   team_balancetype = 1;
   setup_mock_argv("jk_botti", "team_balancetype", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(team_balancetype, 0);
   PASS();
   return 0;
}

static int test_srv_team_balancetype_clamped(void)
{
   TEST("ProcessCommand: team_balancetype clamped to 0-1");
   reset_test_state();
   team_balancetype = 0;
   setup_mock_argv("jk_botti", "team_balancetype", "5", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(team_balancetype, 1);
   setup_mock_argv("jk_botti", "team_balancetype", "-1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(team_balancetype, 0);
   PASS();
   return 0;
}

static int test_srv_team_blockedlist_set(void)
{
   TEST("ProcessCommand: team_blockedlist set");
   reset_test_state();
   setup_mock_argv("jk_botti", "team_blockedlist", "team1,team2", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_PTR_NOT_NULL(team_blockedlist);
   ASSERT_STR(team_blockedlist, "team1,team2");
   PASS();
   return 0;
}

static int test_srv_team_blockedlist_query(void)
{
   TEST("ProcessCommand: team_blockedlist query (no arg)");
   reset_test_state();
   setup_mock_argv("jk_botti", "team_blockedlist", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_PTR_NOT_NULL(team_blockedlist);
   ASSERT_STR(team_blockedlist, "");
   PASS();
   return 0;
}

static int test_srv_randomize_bots_on(void)
{
   TEST("ProcessCommand: randomize_bots_on_mapchange on");
   reset_test_state();
   randomize_bots_on_mapchange = 0;
   setup_mock_argv("jk_botti", "randomize_bots_on_mapchange", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(randomize_bots_on_mapchange, 1);
   PASS();
   return 0;
}

static int test_srv_randomize_bots_off(void)
{
   TEST("ProcessCommand: randomize_bots_on_mapchange off");
   reset_test_state();
   randomize_bots_on_mapchange = 1;
   setup_mock_argv("jk_botti", "randomize_bots_on_mapchange", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(randomize_bots_on_mapchange, 0);
   PASS();
   return 0;
}

static int test_srv_bot_add_level_tag_on(void)
{
   TEST("ProcessCommand: bot_add_level_tag on");
   reset_test_state();
   bot_add_level_tag = 0;
   setup_mock_argv("jk_botti", "bot_add_level_tag", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_add_level_tag, 1);
   PASS();
   return 0;
}

static int test_srv_bot_chat_percent_set(void)
{
   TEST("ProcessCommand: bot_chat_percent set to 50");
   reset_test_state();
   bot_chat_percent = 10;
   setup_mock_argv("jk_botti", "bot_chat_percent", "50", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_percent, 50);
   PASS();
   return 0;
}

static int test_srv_bot_chat_percent_invalid(void)
{
   TEST("ProcessCommand: bot_chat_percent 101 invalid");
   reset_test_state();
   bot_chat_percent = 10;
   setup_mock_argv("jk_botti", "bot_chat_percent", "101", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_percent, 10);
   PASS();
   return 0;
}

static int test_srv_bot_taunt_percent_set(void)
{
   TEST("ProcessCommand: bot_taunt_percent set to 30");
   reset_test_state();
   bot_taunt_percent = 20;
   setup_mock_argv("jk_botti", "bot_taunt_percent", "30", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_taunt_percent, 30);
   PASS();
   return 0;
}

static int test_srv_bot_whine_percent_set(void)
{
   TEST("ProcessCommand: bot_whine_percent set to 5");
   reset_test_state();
   bot_whine_percent = 10;
   setup_mock_argv("jk_botti", "bot_whine_percent", "5", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_whine_percent, 5);
   PASS();
   return 0;
}

static int test_srv_bot_endgame_percent_set(void)
{
   TEST("ProcessCommand: bot_endgame_percent set to 60");
   reset_test_state();
   bot_endgame_percent = 40;
   setup_mock_argv("jk_botti", "bot_endgame_percent", "60", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_endgame_percent, 60);
   PASS();
   return 0;
}

static int test_srv_bot_chat_tag_percent_set(void)
{
   TEST("ProcessCommand: bot_chat_tag_percent set to 90");
   reset_test_state();
   bot_chat_tag_percent = 80;
   setup_mock_argv("jk_botti", "bot_chat_tag_percent", "90", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_tag_percent, 90);
   PASS();
   return 0;
}

static int test_srv_bot_chat_drop_percent_set(void)
{
   TEST("ProcessCommand: bot_chat_drop_percent set to 25");
   reset_test_state();
   bot_chat_drop_percent = 10;
   setup_mock_argv("jk_botti", "bot_chat_drop_percent", "25", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_drop_percent, 25);
   PASS();
   return 0;
}

static int test_srv_bot_chat_swap_percent_set(void)
{
   TEST("ProcessCommand: bot_chat_swap_percent set to 15");
   reset_test_state();
   bot_chat_swap_percent = 10;
   setup_mock_argv("jk_botti", "bot_chat_swap_percent", "15", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_swap_percent, 15);
   PASS();
   return 0;
}

static int test_srv_bot_chat_lower_percent_set(void)
{
   TEST("ProcessCommand: bot_chat_lower_percent set to 75");
   reset_test_state();
   bot_chat_lower_percent = 50;
   setup_mock_argv("jk_botti", "bot_chat_lower_percent", "75", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_lower_percent, 75);
   PASS();
   return 0;
}

static int test_srv_bot_logo_percent_set(void)
{
   TEST("ProcessCommand: bot_logo_percent set to 55");
   reset_test_state();
   bot_logo_percent = 40;
   setup_mock_argv("jk_botti", "bot_logo_percent", "55", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_logo_percent, 55);
   PASS();
   return 0;
}

static int test_srv_bot_logo_percent_invalid(void)
{
   TEST("ProcessCommand: bot_logo_percent 101 invalid");
   reset_test_state();
   bot_logo_percent = 40;
   setup_mock_argv("jk_botti", "bot_logo_percent", "101", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_logo_percent, 40);
   PASS();
   return 0;
}

static int test_srv_random_color_on(void)
{
   TEST("ProcessCommand: random_color enable");
   reset_test_state();
   b_random_color = FALSE;
   setup_mock_argv("jk_botti", "random_color", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(b_random_color, TRUE);
   PASS();
   return 0;
}

static int test_srv_random_color_off(void)
{
   TEST("ProcessCommand: random_color disable");
   reset_test_state();
   b_random_color = TRUE;
   setup_mock_argv("jk_botti", "random_color", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(b_random_color, FALSE);
   PASS();
   return 0;
}

static int test_srv_show_waypoints_on(void)
{
   TEST("ProcessCommand: show_waypoints enable");
   reset_test_state();
   g_waypoint_on = FALSE;
   g_path_waypoint = FALSE;
   setup_mock_argv("jk_botti", "show_waypoints", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(g_waypoint_on, TRUE);
   ASSERT_INT(g_path_waypoint, TRUE);
   PASS();
   return 0;
}

static int test_srv_show_waypoints_off(void)
{
   TEST("ProcessCommand: show_waypoints disable");
   reset_test_state();
   g_waypoint_on = TRUE;
   g_path_waypoint = TRUE;
   setup_mock_argv("jk_botti", "show_waypoints", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(g_waypoint_on, FALSE);
   ASSERT_INT(g_path_waypoint, FALSE);
   PASS();
   return 0;
}

static int test_srv_debug_minmax_on(void)
{
   TEST("ProcessCommand: debug_minmax enable");
   reset_test_state();
   debug_minmax = 0;
   setup_mock_argv("jk_botti", "debug_minmax", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(debug_minmax, TRUE);
   PASS();
   return 0;
}

static int test_srv_debug_minmax_off(void)
{
   TEST("ProcessCommand: debug_minmax disable");
   reset_test_state();
   debug_minmax = TRUE;
   setup_mock_argv("jk_botti", "debug_minmax", "0", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(debug_minmax, FALSE);
   PASS();
   return 0;
}

static int test_srv_autowaypoint_on(void)
{
   TEST("ProcessCommand: autowaypoint on");
   reset_test_state();
   g_auto_waypoint = FALSE;
   setup_mock_argv("jk_botti", "autowaypoint", "on", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(g_auto_waypoint, TRUE);
   PASS();
   return 0;
}

static int test_srv_autowaypoint_off(void)
{
   TEST("ProcessCommand: autowaypoint off");
   reset_test_state();
   g_auto_waypoint = TRUE;
   setup_mock_argv("jk_botti", "autowaypoint", "off", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(g_auto_waypoint, FALSE);
   PASS();
   return 0;
}

static int test_srv_autowaypoint_numeric(void)
{
   TEST("ProcessCommand: autowaypoint with numeric arg");
   reset_test_state();
   g_auto_waypoint = FALSE;
   setup_mock_argv("jk_botti", "autowaypoint", "1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(g_auto_waypoint, TRUE);
   PASS();
   return 0;
}

static int test_srv_bot_reaction_time_ignored(void)
{
   TEST("ProcessCommand: bot_reaction_time is ignored");
   reset_test_state();
   setup_mock_argv("jk_botti", "bot_reaction_time", "5", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_unknown_command(void)
{
   TEST("ProcessCommand: unknown command");
   reset_test_state();
   setup_mock_argv("jk_botti", "nonexistent_cmd", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_percent_settings_negative(void)
{
   TEST("ProcessCommand: percent settings reject negative");
   reset_test_state();
   bot_chat_percent = 10;
   setup_mock_argv("jk_botti", "bot_chat_percent", "-1", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_chat_percent, 10);
   bot_taunt_percent = 20;
   setup_mock_argv("jk_botti", "bot_taunt_percent", "-5", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(bot_taunt_percent, 20);
   PASS();
   return 0;
}

static int test_srv_addbot_default(void)
{
   TEST("ProcessCommand: addbot with defaults");
   reset_test_state();
   default_bot_skill = 3;
   max_bots = -1;
   setup_mock_argv("jk_botti", "addbot", "", "", "", "", "");
   jk_botti_ServerCommand();
   ASSERT_INT(mock_bot_create_count, 1);
   ASSERT_INT(mock_bot_create_last_skill, 3);
   PASS();
   return 0;
}

static int test_srv_addbot_with_args(void)
{
   TEST("ProcessCommand: addbot with skin, name, skill");
   reset_test_state();
   max_bots = -1;
   setup_mock_argv("jk_botti", "addbot", "barney", "TestBot", "5", "", "");
   jk_botti_ServerCommand();
   ASSERT_INT(mock_bot_create_count, 1);
   ASSERT_STR(mock_bot_create_last_skin, "barney");
   ASSERT_STR(mock_bot_create_last_name, "TestBot");
   ASSERT_INT(mock_bot_create_last_skill, 5);
   PASS();
   return 0;
}

static int test_srv_addbot_max_bots_reached(void)
{
   TEST("ProcessCommand: addbot blocked by max_bots");
   reset_test_state();
   max_bots = 0;
   setup_mock_argv("jk_botti", "addbot", "", "", "", "", "");
   jk_botti_ServerCommand();
   ASSERT_INT(mock_bot_create_count, 0);
   PASS();
   return 0;
}

static int test_srv_info_no_bots(void)
{
   TEST("ProcessCommand: info with no bots");
   reset_test_state();
   setup_mock_argv("jk_botti", "info", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_info_with_bots(void)
{
   TEST("ProcessCommand: info with active bots");
   reset_test_state();
   bots[0].is_used = TRUE;
   safe_strcopy(bots[0].name, sizeof(bots[0].name), "TestBot1");
   safe_strcopy(bots[0].skin, sizeof(bots[0].skin), "barney");
   bots[0].bot_skill = 2;
   setup_mock_argv("jk_botti", "info", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

// ============================================================
// jk_botti_ServerCommand kickall
// ============================================================

static int test_srv_kickall_no_bots(void)
{
   TEST("jk_botti_ServerCommand: kickall with no bots");
   reset_test_state();
   setup_mock_argv("jk_botti", "kickall", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(mock_bot_kick_count, 0);
   PASS();
   return 0;
}

static int test_srv_kickall_with_bots(void)
{
   TEST("jk_botti_ServerCommand: kickall with bots");
   reset_test_state();
   bots[0].is_used = TRUE;
   bots[3].is_used = TRUE;
   bots[7].is_used = TRUE;
   setup_mock_argv("jk_botti", "kickall", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(mock_bot_kick_count, 3);
   PASS();
   return 0;
}

static int test_srv_kickall_resets_minmax(void)
{
   TEST("jk_botti_ServerCommand: kickall resets min/max bots");
   reset_test_state();
   min_bots = 5;
   max_bots = 10;
   setup_mock_argv("jk_botti", "kickall", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, -1);
   ASSERT_INT(max_bots, -1);
   PASS();
   return 0;
}

static int test_srv_kickall_no_reset_when_minmax_neg(void)
{
   TEST("jk_botti_ServerCommand: kickall no reset if -1");
   reset_test_state();
   min_bots = -1;
   max_bots = -1;
   bots[0].is_used = TRUE;
   setup_mock_argv("jk_botti", "kickall", "", NULL, NULL, NULL, NULL);
   jk_botti_ServerCommand();
   ASSERT_INT(min_bots, -1);
   ASSERT_INT(max_bots, -1);
   PASS();
   return 0;
}

// ============================================================
// bot_skill_setup tests
// ============================================================

static int test_srv_bot_skill_setup_set(void)
{
   TEST("ProcessCommand: bot_skill_setup pause_frequency");
   reset_test_state();
   memset(skill_settings, 0, sizeof(skill_settings));
   setup_mock_argv("jk_botti", "bot_skill_setup", "1", "pause_frequency", "42", "", "");
   jk_botti_ServerCommand();
   ASSERT_INT(skill_settings[0].pause_frequency, 42);
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_invalid_skill(void)
{
   TEST("ProcessCommand: bot_skill_setup invalid skill 6");
   reset_test_state();
   setup_mock_argv("jk_botti", "bot_skill_setup", "6", "pause_frequency", "42", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_reset(void)
{
   TEST("ProcessCommand: bot_skill_setup reset");
   reset_test_state();
   setup_mock_argv("jk_botti", "bot_skill_setup", "reset", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_no_arg(void)
{
   TEST("ProcessCommand: bot_skill_setup no arg");
   reset_test_state();
   setup_mock_argv("jk_botti", "bot_skill_setup", "", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_float(void)
{
   TEST("ProcessCommand: bot_skill_setup set turn_skill");
   reset_test_state();
   memset(skill_settings, 0, sizeof(skill_settings));
   setup_mock_argv("jk_botti", "bot_skill_setup", "3", "turn_skill", "0.75", "", "");
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(skill_settings[2].turn_skill, 0.75f, 0.01f);
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_bool(void)
{
   TEST("ProcessCommand: bot_skill_setup set can_longjump");
   reset_test_state();
   memset(skill_settings, 0, sizeof(skill_settings));
   setup_mock_argv("jk_botti", "bot_skill_setup", "5", "can_longjump", "on", "", "");
   jk_botti_ServerCommand();
   ASSERT_INT(skill_settings[4].can_longjump, TRUE);
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_float100(void)
{
   TEST("ProcessCommand: bot_skill_setup normal_strafe");
   reset_test_state();
   memset(skill_settings, 0, sizeof(skill_settings));
   setup_mock_argv("jk_botti", "bot_skill_setup", "2", "normal_strafe", "50", "", "");
   jk_botti_ServerCommand();
   ASSERT_FLOAT_NEAR(skill_settings[1].normal_strafe, 0.5f, 0.01f);
   PASS();
   return 0;
}

static int test_srv_bot_skill_setup_unknown(void)
{
   TEST("ProcessCommand: bot_skill_setup unknown setting");
   reset_test_state();
   setup_mock_argv("jk_botti", "bot_skill_setup", "1", "nonexistent_setting", "5", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

// ============================================================
// botweapon command tests
// ============================================================

static int test_srv_botweapon_reset(void)
{
   TEST("ProcessCommand: botweapon reset");
   reset_test_state();
   setup_mock_argv("jk_botti", "botweapon", "reset", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_botweapon_weapons_list(void)
{
   TEST("ProcessCommand: botweapon weapons list");
   reset_test_state();
   setup_mock_argv("jk_botti", "botweapon", "weapons", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_botweapon_no_arg(void)
{
   TEST("ProcessCommand: botweapon no arg");
   reset_test_state();
   setup_mock_argv("jk_botti", "botweapon", "", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_botweapon_unknown(void)
{
   TEST("ProcessCommand: botweapon unknown weapon");
   reset_test_state();
   setup_mock_argv("jk_botti", "botweapon", "nonexistent_weapon", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_botweapon_show_all_settings(void)
{
   TEST("ProcessCommand: botweapon show all settings for weapon");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   // weapon name only, no setting -> dumps all settings
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun", "", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_botweapon_set_int(void)
{
   TEST("ProcessCommand: botweapon set INT setting");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   int old_val = sg->primary_skill_level;
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "primary_skill_level", "3", "", "");
   jk_botti_ServerCommand();
   ASSERT_INT(sg->primary_skill_level, 3);
   // Restore
   sg->primary_skill_level = old_val;
   PASS();
   return 0;
}

static int test_srv_botweapon_read_int(void)
{
   TEST("ProcessCommand: botweapon read INT setting (no value)");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   // No value arg -> just prints current value
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "primary_skill_level", "", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

static int test_srv_botweapon_set_float(void)
{
   TEST("ProcessCommand: botweapon set FLOAT setting");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "aim_speed", "0.75", "", "");
   jk_botti_ServerCommand();
   ASSERT_TRUE(sg->aim_speed > 0.74f && sg->aim_speed < 0.76f);
   PASS();
   return 0;
}

static int test_srv_botweapon_set_bool_on(void)
{
   TEST("ProcessCommand: botweapon set QBOOLEAN 'on'");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "avoid_this_gun", "on", "", "");
   jk_botti_ServerCommand();
   ASSERT_TRUE(sg->avoid_this_gun == TRUE);
   PASS();
   return 0;
}

static int test_srv_botweapon_set_bool_off(void)
{
   TEST("ProcessCommand: botweapon set QBOOLEAN 'off'");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   sg->avoid_this_gun = TRUE;
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "avoid_this_gun", "off", "", "");
   jk_botti_ServerCommand();
   ASSERT_TRUE(sg->avoid_this_gun == FALSE);
   PASS();
   return 0;
}

static int test_srv_botweapon_set_bool_true(void)
{
   TEST("ProcessCommand: botweapon set QBOOLEAN 'true'");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "can_use_underwater", "true", "", "");
   jk_botti_ServerCommand();
   ASSERT_TRUE(sg->can_use_underwater == TRUE);
   PASS();
   return 0;
}

static int test_srv_botweapon_set_bool_false(void)
{
   TEST("ProcessCommand: botweapon set QBOOLEAN 'false'");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   sg->can_use_underwater = TRUE;
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "can_use_underwater", "false", "", "");
   jk_botti_ServerCommand();
   ASSERT_TRUE(sg->can_use_underwater == FALSE);
   PASS();
   return 0;
}

static int test_srv_botweapon_set_bool_numeric(void)
{
   TEST("ProcessCommand: botweapon set QBOOLEAN numeric '1'");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *sg = GetWeaponSelect(VALVE_WEAPON_SHOTGUN);
   ASSERT_TRUE(sg != NULL);
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "prefer_higher_skill_attack", "1", "", "");
   jk_botti_ServerCommand();
   ASSERT_TRUE(sg->prefer_higher_skill_attack == TRUE);
   PASS();
   return 0;
}

static int test_srv_botweapon_unknown_setting(void)
{
   TEST("ProcessCommand: botweapon unknown setting name");
   reset_test_state();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_mock_argv("jk_botti", "botweapon", "weapon_shotgun",
                    "nonexistent_field", "42", "", "");
   jk_botti_ServerCommand();
   PASS();
   return 0;
}

// ============================================================
// ProcessBotCfgFile tests
// ============================================================

static int test_ProcessBotCfgFile_null_fp(void)
{
   TEST("ProcessBotCfgFile: NULL bot_cfg_fp -> no crash");
   reset_test_state();
   bot_cfg_fp = NULL;
   ProcessBotCfgFile();
   ASSERT_PTR_NULL(bot_cfg_fp);
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_pause_time(void)
{
   TEST("ProcessBotCfgFile: pause_time not elapsed -> skip");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 20.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "botskill 3\n");
   rewind(fp);
   bot_cfg_fp = fp;
   default_bot_skill = 2;
   ProcessBotCfgFile();
   ASSERT_INT(default_bot_skill, 2);
   ASSERT_PTR_NOT_NULL(bot_cfg_fp);
   fclose(bot_cfg_fp);
   bot_cfg_fp = NULL;
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_comment_skipped(void)
{
   TEST("ProcessBotCfgFile: # comment line skipped");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "# this is a comment\nbotskill 4\n");
   rewind(fp);
   bot_cfg_fp = fp;
   default_bot_skill = 2;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(default_bot_skill, 2);
   ProcessBotCfgFile();
   ASSERT_INT(default_bot_skill, 4);
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_slashslash_comment(void)
{
   TEST("ProcessBotCfgFile: // comment line skipped");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "// this is a comment\nmin_bots 5\n");
   rewind(fp);
   bot_cfg_fp = fp;
   min_bots = -1;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(min_bots, -1);
   ProcessBotCfgFile();
   ASSERT_INT(min_bots, 5);
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_blank_line(void)
{
   TEST("ProcessBotCfgFile: blank lines skipped");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "\nmax_bots 8\n");
   rewind(fp);
   bot_cfg_fp = fp;
   max_bots = -1;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(max_bots, -1);
   ProcessBotCfgFile();
   ASSERT_INT(max_bots, 8);
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_eof_closes(void)
{
   TEST("ProcessBotCfgFile: EOF closes file");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "botskill 5");
   rewind(fp);
   bot_cfg_fp = fp;
   default_bot_skill = 2;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(default_bot_skill, 5);
   ASSERT_PTR_NULL(bot_cfg_fp);
   ASSERT_FLOAT_NEAR(bot_cfg_pause_time, 0.0f, 0.01f);
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_addbot_as_cfg(void)
{
   TEST("ProcessBotCfgFile: addbot adds to cfg record");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   max_bots = -1;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "addbot barney TestBot 3\n");
   rewind(fp);
   bot_cfg_fp = fp;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(cfg_bot_record_size, 1);
   ASSERT_STR(cfg_bot_record[0].skin, "barney");
   ASSERT_STR(cfg_bot_record[0].name, "TestBot");
   ASSERT_INT(cfg_bot_record[0].skill, 3);
   ASSERT_INT(mock_bot_create_count, 1);
   FreeCfgBotRecord();
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_pause_cmd(void)
{
   TEST("ProcessBotCfgFile: pause command sets pause time");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "pause 5\nbotskill 4\n");
   rewind(fp);
   bot_cfg_fp = fp;
   default_bot_skill = 2;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_FLOAT_NEAR(bot_cfg_pause_time, 15.0f, 0.01f);
   ASSERT_INT(default_bot_skill, 2);
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_unknown_cmd(void)
{
   TEST("ProcessBotCfgFile: unknown cmd -> server cmd");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "sv_cheats 1\n");
   rewind(fp);
   bot_cfg_fp = fp;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(mock_server_execute_count, 1);
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_quoted_args(void)
{
   TEST("ProcessBotCfgFile: quoted args trimmed");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   max_bots = -1;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "addbot \"barney\" \"TestBot\" 2\n");
   rewind(fp);
   bot_cfg_fp = fp;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(cfg_bot_record_size, 1);
   ASSERT_STR(cfg_bot_record[0].skin, "barney");
   ASSERT_STR(cfg_bot_record[0].name, "TestBot");
   FreeCfgBotRecord();
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_empty_quoted_arg(void)
{
   TEST("ProcessBotCfgFile: \"\" arg becomes empty");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   max_bots = -1;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "addbot \"\" \"\" 3\n");
   rewind(fp);
   bot_cfg_fp = fp;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(cfg_bot_record_size, 1);
   ASSERT_PTR_NULL(cfg_bot_record[0].skin);
   ASSERT_PTR_NULL(cfg_bot_record[0].name);
   FreeCfgBotRecord();
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

static int test_ProcessBotCfgFile_tabs_converted(void)
{
   TEST("ProcessBotCfgFile: tabs converted to spaces");
   reset_test_state();
   gpGlobals->time = 10.0;
   bot_cfg_pause_time = 0.0;
   FILE *fp = tmpfile();
   ASSERT_PTR_NOT_NULL(fp);
   fprintf(fp, "botskill\t5\n");
   rewind(fp);
   bot_cfg_fp = fp;
   default_bot_skill = 2;
   bot_cfg_linenumber = 0;
   ProcessBotCfgFile();
   ASSERT_INT(default_bot_skill, 5);
   if (bot_cfg_fp) { fclose(bot_cfg_fp); bot_cfg_fp = NULL; }
   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_commands:\n");

   // CfgBotRecord CRUD
   printf(" CfgBotRecord:\n");
   fail |= test_AddToCfgBotRecord_basic();
   fail |= test_AddToCfgBotRecord_multiple();
   fail |= test_GetUnusedCfgBotRecord_no_records();
   fail |= test_GetUnusedCfgBotRecord_with_unused();
   fail |= test_GetUnusedCfgBotRecord_all_used();
   fail |= test_GetUnusedCfgBotRecord_some_used();
   fail |= test_FreeCfgBotRecord_frees_all();
   fail |= test_FreeCfgBotRecord_empty();

   // FakeClientCommand
   printf(" FakeClientCommand:\n");
   fail |= test_FakeClientCommand_one_arg();
   fail |= test_FakeClientCommand_two_args();
   fail |= test_FakeClientCommand_three_args();
   fail |= test_FakeClientCommand_null_arg1();
   fail |= test_FakeClientCommand_empty_arg1();

   // ProcessCommand via jk_botti_ServerCommand
   printf(" ProcessCommand (via ServerCommand):\n");
   fail |= test_srv_botskill_set();
   fail |= test_srv_botskill_invalid();
   fail |= test_srv_botskill_zero();
   fail |= test_srv_botskill_query();
   fail |= test_srv_botskill_all_valid();
   fail |= test_srv_min_bots_set();
   fail |= test_srv_min_bots_invalid();
   fail |= test_srv_min_bots_negative();
   fail |= test_srv_min_bots_boundary();
   fail |= test_srv_max_bots_set();
   fail |= test_srv_max_bots_invalid();
   fail |= test_srv_max_bots_boundary();
   fail |= test_srv_bot_conntimes_on();
   fail |= test_srv_bot_conntimes_off();
   fail |= test_srv_bot_conntimes_listenserver();
   fail |= test_srv_bot_conntimes_invalid();
   fail |= test_srv_botdontshoot_on();
   fail |= test_srv_botdontshoot_off();
   fail |= test_srv_observer_on();
   fail |= test_srv_observer_off();
   fail |= test_srv_bot_shoot_breakables_set();
   fail |= test_srv_bot_shoot_breakables_invalid();
   fail |= test_srv_bot_shoot_breakables_boundary();
   fail |= test_srv_botthinkfps_set();
   fail |= test_srv_botthinkfps_invalid_zero();
   fail |= test_srv_botthinkfps_invalid_101();
   fail |= test_srv_botthinkfps_boundary();
   fail |= test_srv_team_balancetype_set();
   fail |= test_srv_team_balancetype_clamped();
   fail |= test_srv_team_blockedlist_set();
   fail |= test_srv_team_blockedlist_query();
   fail |= test_srv_randomize_bots_on();
   fail |= test_srv_randomize_bots_off();
   fail |= test_srv_bot_add_level_tag_on();
   fail |= test_srv_bot_chat_percent_set();
   fail |= test_srv_bot_chat_percent_invalid();
   fail |= test_srv_bot_taunt_percent_set();
   fail |= test_srv_bot_whine_percent_set();
   fail |= test_srv_bot_endgame_percent_set();
   fail |= test_srv_bot_chat_tag_percent_set();
   fail |= test_srv_bot_chat_drop_percent_set();
   fail |= test_srv_bot_chat_swap_percent_set();
   fail |= test_srv_bot_chat_lower_percent_set();
   fail |= test_srv_bot_logo_percent_set();
   fail |= test_srv_bot_logo_percent_invalid();
   fail |= test_srv_random_color_on();
   fail |= test_srv_random_color_off();
   fail |= test_srv_show_waypoints_on();
   fail |= test_srv_show_waypoints_off();
   fail |= test_srv_debug_minmax_on();
   fail |= test_srv_debug_minmax_off();
   fail |= test_srv_autowaypoint_on();
   fail |= test_srv_autowaypoint_off();
   fail |= test_srv_autowaypoint_numeric();
   fail |= test_srv_bot_reaction_time_ignored();
   fail |= test_srv_percent_settings_negative();
   fail |= test_srv_addbot_default();
   fail |= test_srv_addbot_with_args();
   fail |= test_srv_addbot_max_bots_reached();
   fail |= test_srv_info_no_bots();
   fail |= test_srv_info_with_bots();
   fail |= test_srv_unknown_command();

   // kickall
   printf(" jk_botti_ServerCommand kickall:\n");
   fail |= test_srv_kickall_no_bots();
   fail |= test_srv_kickall_with_bots();
   fail |= test_srv_kickall_resets_minmax();
   fail |= test_srv_kickall_no_reset_when_minmax_neg();

   // bot_skill_setup
   printf(" bot_skill_setup:\n");
   fail |= test_srv_bot_skill_setup_set();
   fail |= test_srv_bot_skill_setup_invalid_skill();
   fail |= test_srv_bot_skill_setup_reset();
   fail |= test_srv_bot_skill_setup_no_arg();
   fail |= test_srv_bot_skill_setup_float();
   fail |= test_srv_bot_skill_setup_bool();
   fail |= test_srv_bot_skill_setup_float100();
   fail |= test_srv_bot_skill_setup_unknown();

   // botweapon
   printf(" botweapon:\n");
   fail |= test_srv_botweapon_reset();
   fail |= test_srv_botweapon_weapons_list();
   fail |= test_srv_botweapon_no_arg();
   fail |= test_srv_botweapon_unknown();
   fail |= test_srv_botweapon_show_all_settings();
   fail |= test_srv_botweapon_set_int();
   fail |= test_srv_botweapon_read_int();
   fail |= test_srv_botweapon_set_float();
   fail |= test_srv_botweapon_set_bool_on();
   fail |= test_srv_botweapon_set_bool_off();
   fail |= test_srv_botweapon_set_bool_true();
   fail |= test_srv_botweapon_set_bool_false();
   fail |= test_srv_botweapon_set_bool_numeric();
   fail |= test_srv_botweapon_unknown_setting();

   // ProcessBotCfgFile
   printf(" ProcessBotCfgFile:\n");
   fail |= test_ProcessBotCfgFile_null_fp();
   fail |= test_ProcessBotCfgFile_pause_time();
   fail |= test_ProcessBotCfgFile_comment_skipped();
   fail |= test_ProcessBotCfgFile_slashslash_comment();
   fail |= test_ProcessBotCfgFile_blank_line();
   fail |= test_ProcessBotCfgFile_eof_closes();
   fail |= test_ProcessBotCfgFile_addbot_as_cfg();
   fail |= test_ProcessBotCfgFile_pause_cmd();
   fail |= test_ProcessBotCfgFile_unknown_cmd();
   fail |= test_ProcessBotCfgFile_quoted_args();
   fail |= test_ProcessBotCfgFile_empty_quoted_arg();
   fail |= test_ProcessBotCfgFile_tabs_converted();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
