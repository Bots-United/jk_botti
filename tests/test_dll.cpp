//
// JK_Botti - comprehensive tests for dll.cpp
//
// test_dll.cpp
//

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <pm_defs.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "bot_sound.h"
#include "bot_config_init.h"
#include "bot_client.h"
#include "waypoint.h"
#include "player.h"
#include "bot_query_hook.h"

#include "engine_mock.h"
#include "test_common.h"

// Forward-declare the dll.cpp exported functions with exact names
extern "C" int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
extern "C" int GetEntityAPI2_POST(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
extern "C" int Meta_Query(char *ifvers, plugin_info_t **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs);
extern "C" int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs);
extern "C" int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason);

// ============================================================
// Stubs for symbols not in engine_mock or linked .o files
// ============================================================

// engine.cpp globals
qboolean g_in_intermission = FALSE;
char g_argv[1024*3];
char g_arg1[1024];
char g_arg2[1024];
char g_arg3[1024];
void (*botMsgFunction)(void *, int) = NULL;
void (*botMsgEndFunction)(void *, int) = NULL;
int botMsgIndex = 0;

// bot_chat.cpp globals
int bot_chat_count = 0;
int bot_taunt_count = 0;
int bot_whine_count = 0;
int bot_endgame_count = 0;
int recent_bot_chat[5] = {};
int recent_bot_taunt[5] = {};
int recent_bot_whine[5] = {};
int recent_bot_endgame[5] = {};
bot_chat_t bot_chat[MAX_BOT_CHAT];
bot_chat_t bot_whine[MAX_BOT_CHAT];

// bot_combat.cpp globals
char g_team_list[TEAMPLAY_TEAMLISTLENGTH];
char g_team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int g_team_scores[MAX_TEAMS];
int g_num_teams = 0;
qboolean g_team_limit = FALSE;

// commands.cpp globals
qboolean isFakeClientCommand = FALSE;
int fake_arg_count = 0;
int bot_conntimes = 0;

// waypoint globals
qboolean g_waypoint_on = FALSE;
qboolean g_auto_waypoint = FALSE;
qboolean g_path_waypoint = FALSE;
qboolean g_path_waypoint_enable = FALSE;
qboolean g_waypoint_updated = FALSE;
float wp_display_time[MAX_WAYPOINTS];
BOOL wp_matrix_save_on_mapend = FALSE;

// bot_query_hook
bool unhook_sendto_function(void) { return true; }

// bot_config_init.cpp functions
void BotNameInit(void) {}
void BotLogoInit(void) {}

// bot.cpp functions
char * GetSpecificTeam(char *teamstr, size_t slen, qboolean get_smallest, qboolean get_largest, qboolean only_count_bots)
{ (void)teamstr; (void)slen; (void)get_smallest; (void)get_largest; (void)only_count_bots; return NULL; }

// waypoint.cpp functions (additional)
static int collect_map_spawn_items_called = 0;
static edict_t *collect_map_spawn_items_arg = NULL;
void CollectMapSpawnItems(edict_t *pSpawn)
{
   collect_map_spawn_items_called++;
   collect_map_spawn_items_arg = pSpawn;
}

// bot_combat.cpp functions
static int free_posdata_list_called = 0;
void free_posdata_list(int idx) { (void)idx; free_posdata_list_called++; }
void GatherPlayerData(edict_t *pEdict) { (void)pEdict; }

// commands.cpp functions
static int process_bot_cfg_file_called = 0;
void ProcessBotCfgFile(void) { process_bot_cfg_file_called++; }
void jk_botti_ServerCommand(void) {}
void ClientCommand(edict_t *pEntity) { (void)pEntity; }
int AddToCfgBotRecord(const char *skin, const char *name, int skill, int top_color, int bottom_color)
{ (void)skin; (void)name; (void)skill; (void)top_color; (void)bottom_color; return 0; }

static int free_cfg_bot_record_called = 0;
void FreeCfgBotRecord(void) { free_cfg_bot_record_called++; }

// bot_client.cpp functions
void BotClient_Valve_WeaponList(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_CurrentWeapon(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_AmmoX(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_AmmoPickup(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_WeaponPickup(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_ItemPickup(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_Health(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_Battery(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_Damage(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_DeathMsg(void *p, int bot_index) { (void)p; (void)bot_index; }
void BotClient_Valve_ScreenFade(void *p, int bot_index) { (void)p; (void)bot_index; }

// waypoint.cpp functions
static int waypoint_init_called = 0;
void WaypointInit(void) { waypoint_init_called++; }
static int waypoint_add_spawn_objects_called = 0;
void WaypointAddSpawnObjects(void) { waypoint_add_spawn_objects_called++; }

static int waypoint_add_lift_called = 0;
static edict_t *waypoint_add_lift_ent = NULL;
static Vector waypoint_add_lift_start;
static Vector waypoint_add_lift_end;
void WaypointAddLift(edict_t *pent, const Vector &start, const Vector &end)
{
   waypoint_add_lift_called++;
   waypoint_add_lift_ent = pent;
   waypoint_add_lift_start = start;
   waypoint_add_lift_end = end;
}

static int waypoint_load_called = 0;
qboolean WaypointLoad(edict_t *pEntity) { (void)pEntity; waypoint_load_called++; return FALSE; }

static int waypoint_save_called = 0;
void WaypointSave(void) { waypoint_save_called++; }

static int waypoint_save_floyds_called = 0;
void WaypointSaveFloydsMatrix(void) { waypoint_save_floyds_called++; }

static int waypoint_slow_floyds_state_ret = -1;
int WaypointSlowFloyds(void) { return -1; }
int WaypointSlowFloydsState(void) { return waypoint_slow_floyds_state_ret; }

static int waypoint_think_called = 0;
void WaypointThink(edict_t *pEntity) { (void)pEntity; waypoint_think_called++; }

// engine.cpp functions (need extern "C" to match C_DLLEXPORT declarations in dll.cpp)
extern "C" int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{ (void)pengfuncsFromEngine; (void)interfaceVersion; return TRUE; }
extern "C" int GetEngineFunctions_POST(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{ (void)pengfuncsFromEngine; (void)interfaceVersion; return TRUE; }

// bot.cpp stubs that override the weak symbols in engine_mock.cpp
static int bot_check_teamplay_called = 0;
void BotCheckTeamplay(void) { bot_check_teamplay_called++; }

static int bot_kick_called = 0;
static int bot_kick_index = -1;
void BotKick(bot_t &pBot)
{
   bot_kick_called++;
   for (int i = 0; i < 32; i++)
      if (&bots[i] == &pBot) { bot_kick_index = i; break; }
}

static int bot_think_called = 0;
void BotThink(bot_t &pBot) { (void)pBot; bot_think_called++; }

static int bot_create_called = 0;
void BotCreate(const char *skin, const char *name, int skill, int top_color,
               int bottom_color, int cfg_bot_index)
{ (void)skin; (void)name; (void)skill; (void)top_color; (void)bottom_color; (void)cfg_bot_index;
  bot_create_called++; }

// CheckPlayerChatProtection and SaveAliveStatus are in util.o (linked)

// ============================================================
// Mock for MDLL_GetGameDescription
// ============================================================

static const char *mock_game_description = "Half-Life";

static const char *mock_pfnGetGameDescription(void)
{
   return mock_game_description;
}

// ============================================================
// Mock for CVAR_GET_POINTER
// ============================================================

static cvar_t mock_cvar_bm_ver;
static cvar_t mock_cvar_mp_giveweapons;
static cvar_t mock_cvar_mp_giveammo;

static int mock_cvar_bm_ver_exists = 0;
static int mock_cvar_mp_giveweapons_exists = 0;
static int mock_cvar_mp_giveammo_exists = 0;

static cvar_t *mock_pfnCVarGetPointer(const char *name)
{
   if (strcmp(name, "bm_ver") == 0 && mock_cvar_bm_ver_exists)
      return &mock_cvar_bm_ver;
   if (strcmp(name, "mp_giveweapons") == 0 && mock_cvar_mp_giveweapons_exists)
      return &mock_cvar_mp_giveweapons;
   if (strcmp(name, "mp_giveammo") == 0 && mock_cvar_mp_giveammo_exists)
      return &mock_cvar_mp_giveammo;
   return NULL;
}

// ============================================================
// Mock for PRECACHE_SOUND/MODEL
// ============================================================

static int mock_precache_count = 0;

static int mock_pfnPrecacheSound(char *s)
{
   (void)s;
   return ++mock_precache_count;
}

static int mock_pfnPrecacheModel(char *s)
{
   (void)s;
   return ++mock_precache_count;
}

// ============================================================
// Mock for CVAR_REGISTER, CVAR_SET_STRING, CVAR_GET_STRING
// ============================================================

static int mock_cvar_register_called = 0;
static void mock_pfnCVarRegister(cvar_t *pCvar)
{
   (void)pCvar;
   mock_cvar_register_called++;
}

static int mock_cvar_set_string_called = 0;
static char mock_cvar_set_string_name[128];
static char mock_cvar_set_string_value[128];
static void mock_pfnCVarSetString(const char *name, const char *value)
{
   mock_cvar_set_string_called++;
   if (name) strncpy(mock_cvar_set_string_name, name, sizeof(mock_cvar_set_string_name) - 1);
   if (value) strncpy(mock_cvar_set_string_value, value, sizeof(mock_cvar_set_string_value) - 1);
}

static const char *mock_pfnCVarGetString(const char *name)
{
   (void)name;
   return "";
}

// ============================================================
// Mock for REG_SVR_COMMAND (pfnAddServerCommand)
// ============================================================

static int mock_reg_svr_cmd_called = 0;
static void mock_pfnAddServerCommand(char *name, void (*function)(void))
{
   (void)name; (void)function;
   mock_reg_svr_cmd_called++;
}

// ============================================================
// Mock for pfnIsDedicatedServer
// ============================================================

static int mock_pfnIsDedicatedServer(void)
{
   return 0;
}

// ============================================================
// Mock for pfnGetCurrentPlayer
// ============================================================

static int mock_current_player = 0;

static int mock_pfnGetCurrentPlayer(void)
{
   return mock_current_player;
}

// ============================================================
// Metamod util stub functions for LOG_CONSOLE, LOG_MESSAGE, LOG_ERROR
// ============================================================

static void mock_mutil_LogConsole(plid_t plid, const char *fmt, ...)
{
   (void)plid; (void)fmt;
}

static void mock_mutil_LogMessage(plid_t plid, const char *fmt, ...)
{
   (void)plid; (void)fmt;
}

static void mock_mutil_LogError(plid_t plid, const char *fmt, ...)
{
   (void)plid; (void)fmt;
}

// ============================================================
// Static CSoundEnt instance for tests
// ============================================================

static CSoundEnt mock_sound_ent;

// ============================================================
// Test helpers
// ============================================================

static DLL_FUNCTIONS api_table;
static DLL_FUNCTIONS api_post_table;

// dll.o defines gpGamedllFuncs, gpMetaUtilFuncs, and gpMetaGlobals as
// non-weak NULL pointers (overriding engine_mock's weak versions).
// We need our own local structs so that MDLL_* macros and metamod
// utility function pointers have something valid to use.
static DLL_FUNCTIONS mock_gamedll_dll_functions;
static gamedll_funcs_t mock_gamedll_for_dll = { &mock_gamedll_dll_functions, NULL };
static mutil_funcs_t mock_mutil_funcs_for_dll;
static meta_globals_t mock_metaglobals_for_dll;

static void reset_test_state(void)
{
   mock_reset();

   // Point gpGamedllFuncs, gpMetaUtilFuncs, gpMetaGlobals at our local
   // structs (dll.o's non-weak NULL definitions override the weak ones
   // from engine_mock.cpp).
   memset(&mock_gamedll_dll_functions, 0, sizeof(mock_gamedll_dll_functions));
   gpGamedllFuncs = &mock_gamedll_for_dll;
   memset(&mock_mutil_funcs_for_dll, 0, sizeof(mock_mutil_funcs_for_dll));
   gpMetaUtilFuncs = &mock_mutil_funcs_for_dll;
   memset(&mock_metaglobals_for_dll, 0, sizeof(mock_metaglobals_for_dll));
   gpMetaGlobals = &mock_metaglobals_for_dll;

   // Set up gamedll funcs for MDLL_GetGameDescription
   gpGamedllFuncs->dllapi_table->pfnGetGameDescription = mock_pfnGetGameDescription;

   // Set up engine function pointers
   g_engfuncs.pfnCVarGetPointer = mock_pfnCVarGetPointer;
   g_engfuncs.pfnPrecacheSound = mock_pfnPrecacheSound;
   g_engfuncs.pfnPrecacheModel = mock_pfnPrecacheModel;
   g_engfuncs.pfnCVarRegister = mock_pfnCVarRegister;
   g_engfuncs.pfnCVarSetString = mock_pfnCVarSetString;
   g_engfuncs.pfnCVarGetString = mock_pfnCVarGetString;
   g_engfuncs.pfnAddServerCommand = mock_pfnAddServerCommand;
   g_engfuncs.pfnIsDedicatedServer = mock_pfnIsDedicatedServer;
   g_engfuncs.pfnGetCurrentPlayer = mock_pfnGetCurrentPlayer;

   // Set up metamod util functions
   gpMetaUtilFuncs->pfnLogConsole = mock_mutil_LogConsole;
   gpMetaUtilFuncs->pfnLogMessage = mock_mutil_LogMessage;
   gpMetaUtilFuncs->pfnLogError = mock_mutil_LogError;

   // Set up sound entity
   pSoundEnt = &mock_sound_ent;
   memset(&mock_sound_ent, 0, sizeof(mock_sound_ent));
   mock_sound_ent.m_iActiveSound = SOUNDLIST_EMPTY;
   mock_sound_ent.m_nextthink = 99999.0;

   // Reset mock state
   mock_game_description = "Half-Life";
   mock_cvar_bm_ver_exists = 0;
   mock_cvar_mp_giveweapons_exists = 0;
   mock_cvar_mp_giveammo_exists = 0;
   mock_precache_count = 0;
   mock_cvar_register_called = 0;
   mock_cvar_set_string_called = 0;
   memset(mock_cvar_set_string_name, 0, sizeof(mock_cvar_set_string_name));
   memset(mock_cvar_set_string_value, 0, sizeof(mock_cvar_set_string_value));
   mock_reg_svr_cmd_called = 0;
   mock_current_player = 0;

   // Reset tracking counters
   bot_check_teamplay_called = 0;
   collect_map_spawn_items_called = 0;
   collect_map_spawn_items_arg = NULL;
   waypoint_init_called = 0;
   waypoint_load_called = 0;
   waypoint_save_called = 0;
   waypoint_save_floyds_called = 0;
   waypoint_add_spawn_objects_called = 0;
   waypoint_add_lift_called = 0;
   waypoint_add_lift_ent = NULL;
   waypoint_add_lift_start = Vector(0, 0, 0);
   waypoint_add_lift_end = Vector(0, 0, 0);
   waypoint_think_called = 0;
   waypoint_slow_floyds_state_ret = -1;
   free_posdata_list_called = 0;
   free_cfg_bot_record_called = 0;
   process_bot_cfg_file_called = 0;
   bot_kick_called = 0;
   bot_kick_index = -1;
   bot_think_called = 0;
   bot_create_called = 0;

   // Enable deathmatch
   gpGlobals->deathmatch = 1;
   gpGlobals->time = 10.0;
   gpGlobals->maxClients = 32;

   // Get fresh function tables
   int version = 0;
   GetEntityAPI2(&api_table, &version);
   GetEntityAPI2_POST(&api_post_table, &version);
}

// ============================================================
// GetEntityAPI2 / GetEntityAPI2_POST tests
// ============================================================

static int test_GetEntityAPI2_returns_true(void)
{
   TEST("GetEntityAPI2 returns TRUE");

   DLL_FUNCTIONS table;
   int version = 0;
   int ret = GetEntityAPI2(&table, &version);
   ASSERT_INT(ret, TRUE);

   PASS();
   return 0;
}

static int test_GetEntityAPI2_fills_function_table(void)
{
   TEST("GetEntityAPI2 fills function pointers");

   DLL_FUNCTIONS table;
   int version = 0;
   GetEntityAPI2(&table, &version);

   ASSERT_PTR_NOT_NULL((void*)table.pfnGameInit);
   ASSERT_PTR_NOT_NULL((void*)table.pfnSpawn);
   ASSERT_PTR_NOT_NULL((void*)table.pfnClientConnect);
   ASSERT_PTR_NOT_NULL((void*)table.pfnClientPutInServer);
   ASSERT_PTR_NOT_NULL((void*)table.pfnClientDisconnect);
   ASSERT_PTR_NOT_NULL((void*)table.pfnClientCommand);
   ASSERT_PTR_NOT_NULL((void*)table.pfnStartFrame);
   ASSERT_PTR_NOT_NULL((void*)table.pfnServerDeactivate);
   ASSERT_PTR_NOT_NULL((void*)table.pfnPM_Move);
   ASSERT_PTR_NOT_NULL((void*)table.pfnCmdStart);

   PASS();
   return 0;
}

static int test_GetEntityAPI2_POST_returns_true(void)
{
   TEST("GetEntityAPI2_POST returns TRUE");

   DLL_FUNCTIONS table;
   int version = 0;
   int ret = GetEntityAPI2_POST(&table, &version);
   ASSERT_INT(ret, TRUE);

   PASS();
   return 0;
}

static int test_GetEntityAPI2_POST_fills_function_table(void)
{
   TEST("GetEntityAPI2_POST fills function pointers");

   DLL_FUNCTIONS table;
   int version = 0;
   GetEntityAPI2_POST(&table, &version);

   ASSERT_PTR_NOT_NULL((void*)table.pfnSpawn);
   ASSERT_PTR_NOT_NULL((void*)table.pfnPlayerPostThink);
   ASSERT_PTR_NOT_NULL((void*)table.pfnKeyValue);

   PASS();
   return 0;
}

// ============================================================
// CheckSubMod tests (accessed via GameDLLInit -> CheckSubMod)
// ============================================================

static int test_checksubmod_opposing_force(void)
{
   TEST("CheckSubMod: 'Opposing Force' -> SUBMOD_OP4");

   reset_test_state();
   mock_game_description = "Opposing Force";
   submod_id = SUBMOD_HLDM;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_OP4);

   PASS();
   return 0;
}

static int test_checksubmod_opfor_short(void)
{
   TEST("CheckSubMod: 'OpFor DM' -> SUBMOD_OP4");

   reset_test_state();
   mock_game_description = "OpFor Deathmatch";
   submod_id = SUBMOD_HLDM;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_OP4);

   PASS();
   return 0;
}

static int test_checksubmod_severians(void)
{
   TEST("CheckSubMod: 'Severian' -> SUBMOD_SEVS");

   reset_test_state();
   mock_game_description = "Severian MOD";
   submod_id = SUBMOD_HLDM;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_SEVS);

   PASS();
   return 0;
}

static int test_checksubmod_xdm(void)
{
   TEST("CheckSubMod: 'XDM' -> SUBMOD_XDM");

   reset_test_state();
   mock_game_description = "XDM Xtreme";
   submod_id = SUBMOD_HLDM;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_XDM);

   PASS();
   return 0;
}

static int test_checksubmod_bubblemod(void)
{
   TEST("CheckSubMod: bm_ver cvar -> SUBMOD_BUBBLEMOD");

   reset_test_state();
   mock_game_description = "Half-Life";
   mock_cvar_bm_ver_exists = 1;
   submod_id = SUBMOD_HLDM;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_BUBBLEMOD);

   PASS();
   return 0;
}

static int test_checksubmod_default_hldm(void)
{
   TEST("CheckSubMod: default -> SUBMOD_HLDM");

   reset_test_state();
   mock_game_description = "Half-Life";
   mock_cvar_bm_ver_exists = 0;
   submod_id = SUBMOD_OP4;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_HLDM);

   PASS();
   return 0;
}

static int test_checksubmod_hl_teamplay_sevs(void)
{
   TEST("CheckSubMod: 'HL Teamplay' + sevs cvars -> SUBMOD_SEVS");

   reset_test_state();
   mock_game_description = "HL Teamplay";
   mock_cvar_mp_giveweapons_exists = 1;
   mock_cvar_mp_giveammo_exists = 1;
   submod_id = SUBMOD_HLDM;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_SEVS);

   PASS();
   return 0;
}

static int test_checksubmod_hl_teamplay_hldm(void)
{
   TEST("CheckSubMod: 'HL Teamplay' no sevs cvars -> SUBMOD_HLDM");

   reset_test_state();
   mock_game_description = "HL Teamplay";
   mock_cvar_mp_giveweapons_exists = 0;
   mock_cvar_mp_giveammo_exists = 0;
   submod_id = SUBMOD_OP4;

   api_table.pfnGameInit();
   ASSERT_INT(submod_id, SUBMOD_HLDM);

   PASS();
   return 0;
}

// ============================================================
// GameDLLInit tests
// ============================================================

static int test_gamedllinit_sets_submod_weaponflag(void)
{
   TEST("GameDLLInit sets submod_weaponflag for OP4");

   reset_test_state();
   mock_game_description = "Opposing Force";
   extern int submod_weaponflag;

   api_table.pfnGameInit();
   ASSERT_INT(submod_weaponflag, WEAPON_SUBMOD_OP4);

   PASS();
   return 0;
}

static int test_gamedllinit_clears_players_and_bots(void)
{
   TEST("GameDLLInit clears players[] and bots[]");

   reset_test_state();
   players[5].pEdict = (edict_t*)0xDEADBEEF;
   bots[3].is_used = TRUE;

   api_table.pfnGameInit();
   ASSERT_PTR_NULL(players[5].pEdict);
   ASSERT_INT(bots[3].is_used, FALSE);

   PASS();
   return 0;
}

// ============================================================
// Spawn tests
// ============================================================

static int test_spawn_worldspawn(void)
{
   TEST("Spawn worldspawn: initializes state");

   reset_test_state();

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   extern int frame_count;
   frame_count = 999;
   extern int num_bots;
   num_bots = 10;
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   bots[0].is_used = TRUE;

   api_table.pfnSpawn(pWorld);

   ASSERT_INT(frame_count, 0);
   ASSERT_INT(bots[0].is_used, FALSE);
   ASSERT_INT(num_bots, 0);
   ASSERT_TRUE(need_to_open_cfg);
   ASSERT_TRUE(free_posdata_list_called > 0);
   ASSERT_TRUE(waypoint_init_called > 0);
   ASSERT_TRUE(waypoint_load_called > 0);
   ASSERT_INT(g_in_intermission, FALSE);

   PASS();
   return 0;
}

static int test_spawn_worldspawn_precaches(void)
{
   TEST("Spawn worldspawn: precaches sounds and models");

   reset_test_state();

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   mock_precache_count = 0;
   api_table.pfnSpawn(pWorld);

   // 7 PRECACHE_SOUND + 2 PRECACHE_MODEL = 9 precache calls
   ASSERT_TRUE(mock_precache_count >= 9);

   PASS();
   return 0;
}

static int test_spawn_worldspawn_resets_team_data(void)
{
   TEST("Spawn worldspawn: resets teamplay data");

   reset_test_state();

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   extern qboolean is_team_play;
   extern qboolean checked_teamplay;
   is_team_play = TRUE;
   checked_teamplay = TRUE;
   g_num_teams = 5;
   g_team_list[0] = 'x';

   api_table.pfnSpawn(pWorld);

   ASSERT_INT(is_team_play, FALSE);
   ASSERT_INT(checked_teamplay, FALSE);
   ASSERT_INT(g_num_teams, 0);
   ASSERT_INT(g_team_list[0], 0);

   PASS();
   return 0;
}

static int test_spawn_worldspawn_sets_bot_check_time(void)
{
   TEST("Spawn worldspawn: sets bot_check_time");

   reset_test_state();
   extern float bot_check_time;
   bot_check_time = 0.0;
   gpGlobals->time = 20.0;

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   api_table.pfnSpawn(pWorld);
   ASSERT_FLOAT(bot_check_time, 25.0);

   PASS();
   return 0;
}

static int test_spawn_worldspawn_sets_waypoint_time(void)
{
   TEST("Spawn worldspawn: sets waypoint_time to -1");

   reset_test_state();
   extern float waypoint_time;
   waypoint_time = 99.0;

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   api_table.pfnSpawn(pWorld);
   ASSERT_FLOAT(waypoint_time, -1.0);

   PASS();
   return 0;
}

static int test_spawn_func_plat_saves_origin(void)
{
   TEST("Spawn func_plat: saves m_origin (no crash)");

   reset_test_state();

   edict_t *pPlat = mock_alloc_edict();
   mock_set_classname(pPlat, "func_plat");
   pPlat->v.origin = Vector(100.0, 200.0, 300.0);

   api_table.pfnSpawn(pPlat);

   PASS();
   return 0;
}

static int test_spawn_func_door_saves_origin(void)
{
   TEST("Spawn func_door: saves m_origin (no crash)");

   reset_test_state();

   edict_t *pDoor = mock_alloc_edict();
   mock_set_classname(pDoor, "func_door");
   pDoor->v.origin = Vector(50.0, 60.0, 70.0);

   api_table.pfnSpawn(pDoor);

   PASS();
   return 0;
}

static int test_spawn_deathmatch_0_ignored(void)
{
   TEST("Spawn: deathmatch=0 -> no initialization");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   waypoint_init_called = 0;
   api_table.pfnSpawn(pWorld);

   ASSERT_INT(waypoint_init_called, 0);

   PASS();
   return 0;
}

// ============================================================
// Spawn_Post tests
// ============================================================

static int test_spawn_post_worldspawn_calls_bot_check_teamplay(void)
{
   TEST("Spawn_Post worldspawn: calls BotCheckTeamplay");

   reset_test_state();

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   api_post_table.pfnSpawn(pWorld);

   ASSERT_INT(bot_check_teamplay_called, 1);

   PASS();
   return 0;
}

static int test_spawn_post_func_plat_adds_lift_waypoint(void)
{
   TEST("Spawn_Post func_plat: adds lift waypoint");

   reset_test_state();

   edict_t *pPlat = mock_alloc_edict();
   mock_set_classname(pPlat, "func_plat");
   pPlat->v.origin = Vector(100.0, 200.0, 300.0);
   pPlat->v.size = Vector(64.0, 64.0, 128.0);
   pPlat->v.targetname = 0;

   api_table.pfnSpawn(pPlat);
   api_post_table.pfnSpawn(pPlat);

   ASSERT_INT(waypoint_add_lift_called, 1);
   ASSERT_PTR_EQ(waypoint_add_lift_ent, pPlat);

   PASS();
   return 0;
}

static int test_spawn_post_func_plat_uses_default_height(void)
{
   TEST("Spawn_Post func_plat: default height = size.z + 8");

   reset_test_state();

   edict_t *pPlat = mock_alloc_edict();
   mock_set_classname(pPlat, "func_plat");
   pPlat->v.origin = Vector(0.0, 0.0, 100.0);
   pPlat->v.size = Vector(64.0, 64.0, 200.0);
   pPlat->v.targetname = 0;

   api_table.pfnSpawn(pPlat);
   api_post_table.pfnSpawn(pPlat);

   ASSERT_INT(waypoint_add_lift_called, 1);
   // m_origin=(0,0,100), height=200+8=208
   // no targetname: start=position2(0,0,100-208), end=position1(0,0,100)
   ASSERT_FLOAT(waypoint_add_lift_start.z, -108.0);
   ASSERT_FLOAT(waypoint_add_lift_end.z, 100.0);

   PASS();
   return 0;
}

static int test_spawn_post_func_plat_with_targetname(void)
{
   TEST("Spawn_Post func_plat: targetname swaps start/end");

   reset_test_state();

   edict_t *pPlat = mock_alloc_edict();
   mock_set_classname(pPlat, "func_plat");
   pPlat->v.origin = Vector(0.0, 0.0, 100.0);
   pPlat->v.size = Vector(64.0, 64.0, 200.0);
   pPlat->v.targetname = (string_t)(long)"plat_target";

   api_table.pfnSpawn(pPlat);
   api_post_table.pfnSpawn(pPlat);

   ASSERT_INT(waypoint_add_lift_called, 1);
   // with targetname: start=position1(0,0,100), end=position2(0,0,-108)
   ASSERT_FLOAT(waypoint_add_lift_start.z, 100.0);
   ASSERT_FLOAT(waypoint_add_lift_end.z, -108.0);

   PASS();
   return 0;
}

static int test_spawn_post_func_door_adds_lift_waypoint(void)
{
   TEST("Spawn_Post func_door: adds lift waypoint");

   reset_test_state();

   edict_t *pDoor = mock_alloc_edict();
   mock_set_classname(pDoor, "func_door");
   pDoor->v.origin = Vector(0.0, 0.0, 0.0);
   pDoor->v.size = Vector(64.0, 64.0, 128.0);
   pDoor->v.movedir = Vector(0.0, 0.0, 1.0);
   pDoor->v.spawnflags = 0;

   api_table.pfnSpawn(pDoor);
   api_post_table.pfnSpawn(pDoor);

   ASSERT_INT(waypoint_add_lift_called, 1);
   ASSERT_PTR_EQ(waypoint_add_lift_ent, pDoor);

   PASS();
   return 0;
}

static int test_spawn_post_func_door_start_open(void)
{
   TEST("Spawn_Post func_door: SF_DOOR_START_OPEN swaps");

   reset_test_state();

   edict_t *pDoor = mock_alloc_edict();
   mock_set_classname(pDoor, "func_door");
   pDoor->v.origin = Vector(0.0, 0.0, 0.0);
   pDoor->v.size = Vector(100.0, 100.0, 100.0);
   pDoor->v.movedir = Vector(1.0, 0.0, 0.0);
   pDoor->v.spawnflags = 1; // SF_DOOR_START_OPEN

   api_table.pfnSpawn(pDoor);
   api_post_table.pfnSpawn(pDoor);

   ASSERT_INT(waypoint_add_lift_called, 1);
   // position2 = origin + movedir * (|1*(100-2)| - 0) = (98,0,0)
   // SF_DOOR_START_OPEN: swap -> start=(98,0,0), end=(0,0,0)
   ASSERT_FLOAT(waypoint_add_lift_start.x, 98.0);
   ASSERT_FLOAT(waypoint_add_lift_end.x, 0.0);

   PASS();
   return 0;
}

static int test_spawn_post_other_entity_collects_spawn_items(void)
{
   TEST("Spawn_Post other entity: calls CollectMapSpawnItems");

   reset_test_state();

   edict_t *pItem = mock_alloc_edict();
   mock_set_classname(pItem, "weapon_shotgun");

   api_post_table.pfnSpawn(pItem);

   ASSERT_INT(collect_map_spawn_items_called, 1);
   ASSERT_PTR_EQ(collect_map_spawn_items_arg, pItem);

   PASS();
   return 0;
}

static int test_spawn_post_deathmatch_0_ignored(void)
{
   TEST("Spawn_Post: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   edict_t *pWorld = mock_alloc_edict();
   mock_set_classname(pWorld, "worldspawn");

   api_post_table.pfnSpawn(pWorld);

   ASSERT_INT(bot_check_teamplay_called, 0);

   PASS();
   return 0;
}

static int test_spawn_post_resets_m_height_m_lip(void)
{
   TEST("Spawn_Post: resets m_height/m_lip after processing");

   reset_test_state();

   // First plat with custom height
   edict_t *pPlat = mock_alloc_edict();
   mock_set_classname(pPlat, "func_plat");
   pPlat->v.origin = Vector(0.0, 0.0, 50.0);
   pPlat->v.size = Vector(64.0, 64.0, 100.0);
   pPlat->v.targetname = 0;

   api_table.pfnSpawn(pPlat);

   KeyValueData kvd;
   kvd.szKeyName = (char*)"height";
   kvd.szValue = (char*)"200";
   kvd.szClassName = (char*)"func_plat";
   kvd.fHandled = 0;
   api_post_table.pfnKeyValue(pPlat, &kvd);

   api_post_table.pfnSpawn(pPlat);

   // Now second plat without keyvalue -> should get default height
   waypoint_add_lift_called = 0;

   edict_t *pPlat2 = mock_alloc_edict();
   mock_set_classname(pPlat2, "func_plat");
   pPlat2->v.origin = Vector(0.0, 0.0, 0.0);
   pPlat2->v.size = Vector(64.0, 64.0, 80.0);
   pPlat2->v.targetname = 0;

   api_table.pfnSpawn(pPlat2);
   api_post_table.pfnSpawn(pPlat2);

   ASSERT_INT(waypoint_add_lift_called, 1);
   // Default height = size.z + 8 = 88, start.z = 0 - 88 = -88
   ASSERT_FLOAT(waypoint_add_lift_start.z, -88.0);

   PASS();
   return 0;
}

// ============================================================
// DispatchKeyValue_Post tests
// ============================================================

static int test_dispatch_keyvalue_func_breakable(void)
{
   TEST("DispatchKeyValue_Post func_breakable: updates material");

   reset_test_state();

   edict_t *pBreak = mock_alloc_edict();
   mock_set_classname(pBreak, "func_breakable");
   pBreak->v.spawnflags = 0;
   pBreak->v.health = 100;

   KeyValueData kvd;
   kvd.szKeyName = (char*)"material";
   kvd.szValue = (char*)"0";
   kvd.szClassName = (char*)"func_breakable";
   kvd.fHandled = 0;

   api_post_table.pfnKeyValue(pBreak, &kvd);

   breakable_list_t *brk = UTIL_LookupBreakable(pBreak);
   ASSERT_PTR_NOT_NULL(brk);

   PASS();
   return 0;
}

static int test_dispatch_keyvalue_func_breakable_trigger_only(void)
{
   TEST("DispatchKeyValue_Post func_breakable: trigger_only skip");

   reset_test_state();

   edict_t *pBreak = mock_alloc_edict();
   mock_set_classname(pBreak, "func_breakable");
   pBreak->v.spawnflags = SF_BREAK_TRIGGER_ONLY;
   pBreak->v.health = 100;

   KeyValueData kvd;
   kvd.szKeyName = (char*)"material";
   kvd.szValue = (char*)"0";
   kvd.szClassName = (char*)"func_breakable";
   kvd.fHandled = 0;

   api_post_table.pfnKeyValue(pBreak, &kvd);

   breakable_list_t *brk = UTIL_LookupBreakable(pBreak);
   ASSERT_PTR_NULL(brk);

   PASS();
   return 0;
}

static int test_dispatch_keyvalue_func_pushable(void)
{
   TEST("DispatchKeyValue_Post func_pushable: breakable ok");

   reset_test_state();

   edict_t *pPush = mock_alloc_edict();
   mock_set_classname(pPush, "func_pushable");
   pPush->v.spawnflags = SF_PUSH_BREAKABLE;
   pPush->v.health = 100;

   KeyValueData kvd;
   kvd.szKeyName = (char*)"material";
   kvd.szValue = (char*)"0";
   kvd.szClassName = (char*)"func_pushable";
   kvd.fHandled = 0;

   api_post_table.pfnKeyValue(pPush, &kvd);

   breakable_list_t *brk = UTIL_LookupBreakable(pPush);
   ASSERT_PTR_NOT_NULL(brk);

   PASS();
   return 0;
}

static int test_dispatch_keyvalue_func_plat_height(void)
{
   TEST("DispatchKeyValue_Post func_plat: height key");

   reset_test_state();

   edict_t *pPlat = mock_alloc_edict();
   mock_set_classname(pPlat, "func_plat");
   pPlat->v.origin = Vector(0.0, 0.0, 100.0);
   pPlat->v.size = Vector(64.0, 64.0, 200.0);
   pPlat->v.targetname = 0;

   api_table.pfnSpawn(pPlat);

   KeyValueData kvd;
   kvd.szKeyName = (char*)"height";
   kvd.szValue = (char*)"500";
   kvd.szClassName = (char*)"func_plat";
   kvd.fHandled = 0;
   api_post_table.pfnKeyValue(pPlat, &kvd);

   api_post_table.pfnSpawn(pPlat);

   ASSERT_INT(waypoint_add_lift_called, 1);
   ASSERT_FLOAT(waypoint_add_lift_start.z, 100.0 - 500.0);
   ASSERT_FLOAT(waypoint_add_lift_end.z, 100.0);

   PASS();
   return 0;
}

static int test_dispatch_keyvalue_func_door_lip(void)
{
   TEST("DispatchKeyValue_Post func_door: lip key");

   reset_test_state();

   edict_t *pDoor = mock_alloc_edict();
   mock_set_classname(pDoor, "func_door");
   pDoor->v.origin = Vector(0.0, 0.0, 0.0);
   pDoor->v.size = Vector(100.0, 100.0, 100.0);
   pDoor->v.movedir = Vector(1.0, 0.0, 0.0);
   pDoor->v.spawnflags = 0;

   api_table.pfnSpawn(pDoor);

   KeyValueData kvd;
   kvd.szKeyName = (char*)"lip";
   kvd.szValue = (char*)"10";
   kvd.szClassName = (char*)"func_door";
   kvd.fHandled = 0;
   api_post_table.pfnKeyValue(pDoor, &kvd);

   api_post_table.pfnSpawn(pDoor);

   ASSERT_INT(waypoint_add_lift_called, 1);
   // end.x = |1*(100-2)| - 10 = 88
   ASSERT_FLOAT(waypoint_add_lift_end.x, 88.0);

   PASS();
   return 0;
}

static int test_dispatch_keyvalue_deathmatch_0_ignored(void)
{
   TEST("DispatchKeyValue_Post: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   edict_t *pBreak = mock_alloc_edict();
   mock_set_classname(pBreak, "func_breakable");
   pBreak->v.spawnflags = 0;

   KeyValueData kvd;
   kvd.szKeyName = (char*)"material";
   kvd.szValue = (char*)"0";
   kvd.szClassName = (char*)"func_breakable";
   kvd.fHandled = 0;

   api_post_table.pfnKeyValue(pBreak, &kvd);

   breakable_list_t *brk = UTIL_LookupBreakable(pBreak);
   ASSERT_PTR_NULL(brk);

   PASS();
   return 0;
}

// ============================================================
// ClientConnect tests
// ============================================================

static int test_client_connect_loopback(void)
{
   TEST("ClientConnect loopback -> listenserver_edict set");

   reset_test_state();
   extern edict_t *listenserver_edict;
   listenserver_edict = NULL;

   edict_t *pClient = mock_alloc_edict();
   char reject[128] = {};

   api_table.pfnClientConnect(pClient, "Player", "loopback", reject);
   ASSERT_PTR_EQ(listenserver_edict, pClient);

   PASS();
   return 0;
}

static int test_client_connect_bot_address(void)
{
   TEST("ClientConnect bot address -> bot_check_time updated");

   reset_test_state();
   extern float bot_check_time;
   bot_check_time = 0.0;
   gpGlobals->time = 10.0;

   edict_t *pBot = mock_alloc_edict();
   char reject[128] = {};

   api_table.pfnClientConnect(pBot, "BotName", "::::local:jk_botti", reject);
   ASSERT_FLOAT(bot_check_time, 11.0);

   PASS();
   return 0;
}

static int test_client_connect_other_bot_address(void)
{
   TEST("ClientConnect other_bot -> bot_check_time updated");

   reset_test_state();
   extern float bot_check_time;
   bot_check_time = 0.0;
   gpGlobals->time = 10.0;

   edict_t *pBot = mock_alloc_edict();
   char reject[128] = {};

   api_table.pfnClientConnect(pBot, "OtherBot", "::::local:other_bot", reject);
   ASSERT_FLOAT(bot_check_time, 11.0);

   PASS();
   return 0;
}

static int test_client_connect_deathmatch_0(void)
{
   TEST("ClientConnect: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;
   extern edict_t *listenserver_edict;
   listenserver_edict = NULL;

   edict_t *pClient = mock_alloc_edict();
   char reject[128] = {};

   api_table.pfnClientConnect(pClient, "Player", "loopback", reject);
   ASSERT_PTR_NULL(listenserver_edict);

   PASS();
   return 0;
}

// ============================================================
// ClientPutInServer tests
// ============================================================

static int test_client_put_in_server_valid_index(void)
{
   TEST("ClientPutInServer: index 1 -> players[0].pEdict set");

   reset_test_state();

   edict_t *pClient = &mock_edicts[1];
   memset(pClient, 0, sizeof(*pClient));
   pClient->v.pContainingEntity = pClient;

   api_table.pfnClientPutInServer(pClient);
   ASSERT_PTR_EQ(players[0].pEdict, pClient);

   PASS();
   return 0;
}

static int test_client_put_in_server_index_5(void)
{
   TEST("ClientPutInServer: index 5 -> players[4].pEdict set");

   reset_test_state();

   edict_t *pClient = &mock_edicts[5];
   memset(pClient, 0, sizeof(*pClient));
   pClient->v.pContainingEntity = pClient;

   api_table.pfnClientPutInServer(pClient);
   ASSERT_PTR_EQ(players[4].pEdict, pClient);

   PASS();
   return 0;
}

static int test_client_put_in_server_deathmatch_0(void)
{
   TEST("ClientPutInServer: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   edict_t *pClient = &mock_edicts[1];
   memset(pClient, 0, sizeof(*pClient));
   pClient->v.pContainingEntity = pClient;

   api_table.pfnClientPutInServer(pClient);
   ASSERT_PTR_NULL(players[0].pEdict);

   PASS();
   return 0;
}

// ============================================================
// CmdStart tests
// ============================================================

static int test_cmdstart_bot_fills_name(void)
{
   TEST("CmdStart: bot edict -> fills name");

   reset_test_state();

   edict_t *pBot = mock_alloc_edict();
   pBot->v.netname = (string_t)(long)"TestBot";
   int idx = 0;

   bots[idx].is_used = TRUE;
   bots[idx].pEdict = pBot;
   bots[idx].name[0] = 0;
   bots[idx].userid = 0;

   struct usercmd_s cmd;
   memset(&cmd, 0, sizeof(cmd));

   api_table.pfnCmdStart(pBot, &cmd, 0);

   ASSERT_STR(bots[idx].name, "TestBot");
   ASSERT_TRUE(bots[idx].userid > 0);

   PASS();
   return 0;
}

static int test_cmdstart_bot_already_has_name(void)
{
   TEST("CmdStart: bot with name -> not overwritten");

   reset_test_state();

   edict_t *pBot = mock_alloc_edict();
   pBot->v.netname = (string_t)(long)"NewName";
   int idx = 0;

   bots[idx].is_used = TRUE;
   bots[idx].pEdict = pBot;
   strcpy(bots[idx].name, "OldName");
   bots[idx].userid = 42;

   struct usercmd_s cmd;
   memset(&cmd, 0, sizeof(cmd));

   api_table.pfnCmdStart(pBot, &cmd, 0);

   ASSERT_STR(bots[idx].name, "OldName");
   ASSERT_INT(bots[idx].userid, 42);

   PASS();
   return 0;
}

static int test_cmdstart_third_party_bot_auto_connects(void)
{
   TEST("CmdStart: third-party bot -> auto-connect");

   reset_test_state();

   edict_t *pOther = mock_alloc_edict();
   pOther->v.netname = (string_t)(long)"ThirdPartyBot";

   struct usercmd_s cmd;
   memset(&cmd, 0, sizeof(cmd));

   api_table.pfnCmdStart(pOther, &cmd, 0);

   // CmdStart calls ClientConnect + ClientPutInServer for unknown edicts
   int edict_idx = 0;
   for (int i = 0; i < MOCK_MAX_EDICTS; i++)
      if (&mock_edicts[i] == pOther) { edict_idx = i; break; }

   ASSERT_PTR_EQ(players[edict_idx - 1].pEdict, pOther);

   PASS();
   return 0;
}

// ============================================================
// ClientDisconnect tests
// ============================================================

static int test_client_disconnect_removes_player(void)
{
   TEST("ClientDisconnect: removes player from players[]");

   reset_test_state();

   edict_t *pClient = &mock_edicts[3];
   memset(pClient, 0, sizeof(*pClient));
   pClient->v.pContainingEntity = pClient;
   players[2].pEdict = pClient;

   api_table.pfnClientDisconnect(pClient);
   ASSERT_PTR_NULL(players[2].pEdict);

   PASS();
   return 0;
}

static int test_client_disconnect_removes_bot(void)
{
   TEST("ClientDisconnect: removes bot from bots[]");

   reset_test_state();

   edict_t *pBot = mock_alloc_edict();
   int bot_idx = 5;

   bots[bot_idx].is_used = TRUE;
   bots[bot_idx].pEdict = pBot;
   bots[bot_idx].userid = 100;

   api_table.pfnClientDisconnect(pBot);

   ASSERT_INT(bots[bot_idx].is_used, FALSE);
   ASSERT_INT(bots[bot_idx].userid, 0);
   ASSERT_FLOAT(bots[bot_idx].f_kick_time, gpGlobals->time);

   PASS();
   return 0;
}

static int test_client_disconnect_no_matching_bot(void)
{
   TEST("ClientDisconnect: no matching bot -> no crash");

   reset_test_state();

   edict_t *pClient = &mock_edicts[5];
   memset(pClient, 0, sizeof(*pClient));
   pClient->v.pContainingEntity = pClient;
   players[4].pEdict = pClient;

   api_table.pfnClientDisconnect(pClient);
   ASSERT_PTR_NULL(players[4].pEdict);

   PASS();
   return 0;
}

static int test_client_disconnect_deathmatch_0(void)
{
   TEST("ClientDisconnect: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   edict_t *pClient = &mock_edicts[3];
   players[2].pEdict = pClient;

   api_table.pfnClientDisconnect(pClient);
   ASSERT_PTR_EQ(players[2].pEdict, pClient);

   PASS();
   return 0;
}

// ============================================================
// ServerDeactivate tests
// ============================================================

static int test_server_deactivate_frees_breakables(void)
{
   TEST("ServerDeactivate: calls UTIL_FreeFuncBreakables");

   reset_test_state();

   edict_t *pBreak = mock_alloc_edict();
   mock_set_classname(pBreak, "func_breakable");
   pBreak->v.health = 100;
   mock_add_breakable(pBreak, 1);
   ASSERT_PTR_NOT_NULL(UTIL_LookupBreakable(pBreak));

   api_table.pfnServerDeactivate();

   ASSERT_PTR_NULL(UTIL_LookupBreakable(pBreak));

   PASS();
   return 0;
}

static int test_server_deactivate_auto_waypoint_save(void)
{
   TEST("ServerDeactivate: auto_waypoint saves if updated");

   reset_test_state();
   g_auto_waypoint = TRUE;
   g_waypoint_updated = TRUE;

   api_table.pfnServerDeactivate();

   ASSERT_INT(waypoint_save_called, 1);

   PASS();
   return 0;
}

static int test_server_deactivate_no_save_without_update(void)
{
   TEST("ServerDeactivate: no save if not updated");

   reset_test_state();
   g_auto_waypoint = TRUE;
   g_waypoint_updated = FALSE;

   api_table.pfnServerDeactivate();

   ASSERT_INT(waypoint_save_called, 0);

   PASS();
   return 0;
}

static int test_server_deactivate_saves_floyds_matrix(void)
{
   TEST("ServerDeactivate: saves floyds matrix on mapend");

   reset_test_state();
   wp_matrix_save_on_mapend = TRUE;

   api_table.pfnServerDeactivate();

   ASSERT_INT(waypoint_save_floyds_called, 1);
   ASSERT_INT(wp_matrix_save_on_mapend, FALSE);

   PASS();
   return 0;
}

static int test_server_deactivate_deathmatch_0(void)
{
   TEST("ServerDeactivate: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;
   g_auto_waypoint = TRUE;
   g_waypoint_updated = TRUE;

   api_table.pfnServerDeactivate();

   ASSERT_INT(waypoint_save_called, 0);

   PASS();
   return 0;
}

// ============================================================
// PM_Move tests
// ============================================================

static void dummy_pm_playsound(int channel, const char *sample,
   float volume, float attenuation, int fFlags, int pitch)
{
   (void)channel; (void)sample; (void)volume;
   (void)attenuation; (void)fFlags; (void)pitch;
}

// Static so old_ppmove (dll.cpp static) doesn't dangle after test returns
static struct playermove_s pm_move_test_ppmove;

static int test_pm_move_hooks_playsound(void)
{
   TEST("PM_Move: hooks PM_PlaySound");

   reset_test_state();

   struct playermove_s &ppmove = pm_move_test_ppmove;
   memset(&ppmove, 0, sizeof(ppmove));
   ppmove.PM_PlaySound = dummy_pm_playsound;

   api_table.pfnPM_Move(&ppmove, TRUE);

   // PM_PlaySound should now be replaced with the hook
   ASSERT_TRUE(ppmove.PM_PlaySound != dummy_pm_playsound);
   ASSERT_PTR_NOT_NULL((void*)ppmove.PM_PlaySound);

   PASS();
   return 0;
}

static int test_pm_move_deathmatch_0(void)
{
   TEST("PM_Move: deathmatch=0 -> not hooked");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   struct playermove_s ppmove;
   memset(&ppmove, 0, sizeof(ppmove));
   ppmove.PM_PlaySound = dummy_pm_playsound;

   api_table.pfnPM_Move(&ppmove, TRUE);

   ASSERT_PTR_EQ((void*)ppmove.PM_PlaySound, (void*)dummy_pm_playsound);

   PASS();
   return 0;
}

// ============================================================
// StartFrame tests
// ============================================================

static int test_startframe_deathmatch_0_ignored(void)
{
   TEST("StartFrame: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = TRUE;

   api_table.pfnStartFrame();

   ASSERT_TRUE(need_to_open_cfg);

   PASS();
   return 0;
}

static int test_startframe_bot_stop_no_thinking(void)
{
   TEST("StartFrame: bot_stop=1 -> no bot thinking");

   reset_test_state();
   extern int bot_stop;
   bot_stop = 1;
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;

   bots[0].is_used = TRUE;
   bots[0].pEdict = mock_alloc_edict();
   bots[0].bot_think_time = 0.0;

   api_table.pfnStartFrame();

   ASSERT_INT(bot_think_called, 0);
   bot_stop = 0;

   PASS();
   return 0;
}

static int test_startframe_normal_with_bot(void)
{
   TEST("StartFrame: normal frame with active bot");

   reset_test_state();
   extern int bot_stop;
   bot_stop = 0;
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;

   edict_t *pBot = mock_alloc_edict();
   bots[0].is_used = TRUE;
   bots[0].pEdict = pBot;
   bots[0].bot_think_time = 0.0;

   gpGlobals->time = 10.0;

   api_table.pfnStartFrame();

   ASSERT_INT(bot_think_called, 1);

   PASS();
   return 0;
}

static int test_startframe_bot_think_time_not_reached(void)
{
   TEST("StartFrame: bot think time not reached -> skip");

   reset_test_state();
   extern int bot_stop;
   bot_stop = 0;
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;

   edict_t *pBot = mock_alloc_edict();
   bots[0].is_used = TRUE;
   bots[0].pEdict = pBot;
   bots[0].bot_think_time = 999.0;

   gpGlobals->time = 10.0;

   api_table.pfnStartFrame();

   ASSERT_INT(bot_think_called, 0);

   PASS();
   return 0;
}

static int test_startframe_config_file_processing(void)
{
   TEST("StartFrame: need_to_open_cfg triggers cfg search");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = TRUE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;

   gpGlobals->mapname = (string_t)(long)"testmap";
   gpGlobals->time = 10.0;

   api_table.pfnStartFrame();

   ASSERT_INT(need_to_open_cfg, FALSE);

   PASS();
   return 0;
}

static int test_startframe_waypoint_addspawnobjects(void)
{
   TEST("StartFrame: calls WaypointAddSpawnObjects");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;
   extern float waypoint_time;
   waypoint_time = 0.0;

   api_table.pfnStartFrame();

   ASSERT_TRUE(waypoint_add_spawn_objects_called > 0);

   PASS();
   return 0;
}

static int test_startframe_sound_entity_think(void)
{
   TEST("StartFrame: sound entity thinks when due");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;

   mock_sound_ent.m_nextthink = 0.0;
   gpGlobals->time = 10.0;

   // Should not crash
   api_table.pfnStartFrame();

   PASS();
   return 0;
}

static int test_startframe_num_bots_tracks_count(void)
{
   TEST("StartFrame: num_bots tracks active bot count");

   reset_test_state();
   extern int bot_stop;
   bot_stop = 0;
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   extern int num_bots;
   min_bots = -1;
   max_bots = -1;
   num_bots = 0;

   for (int i = 0; i < 3; i++)
   {
      bots[i].is_used = TRUE;
      bots[i].pEdict = mock_alloc_edict();
      bots[i].bot_think_time = 0.0;
   }

   gpGlobals->time = 10.0;

   api_table.pfnStartFrame();

   ASSERT_TRUE(num_bots >= 3);

   PASS();
   return 0;
}

// ============================================================
// Meta_Query tests
// ============================================================

static int test_meta_query_compatible_version(void)
{
   TEST("Meta_Query: compatible version -> TRUE");

   reset_test_state();

   plugin_info_t *pInfo = NULL;
   mutil_funcs_t mfuncs;
   memset(&mfuncs, 0, sizeof(mfuncs));
   mfuncs.pfnLogConsole = mock_mutil_LogConsole;
   mfuncs.pfnLogMessage = mock_mutil_LogMessage;
   mfuncs.pfnLogError = mock_mutil_LogError;
   mfuncs.pfnGetUserMsgID = gpMetaUtilFuncs->pfnGetUserMsgID;

   int ret = Meta_Query((char*)META_INTERFACE_VERSION, &pInfo, &mfuncs);
   ASSERT_INT(ret, TRUE);
   ASSERT_PTR_NOT_NULL(pInfo);
   ASSERT_PTR_NOT_NULL(pInfo->version);

   PASS();
   return 0;
}

static int test_meta_query_plugin_newer_than_metamod(void)
{
   TEST("Meta_Query: plugin newer than metamod -> FALSE");

   reset_test_state();

   plugin_info_t *pInfo = NULL;
   mutil_funcs_t mfuncs;
   memset(&mfuncs, 0, sizeof(mfuncs));
   mfuncs.pfnLogConsole = mock_mutil_LogConsole;
   mfuncs.pfnLogMessage = mock_mutil_LogMessage;
   mfuncs.pfnLogError = mock_mutil_LogError;
   mfuncs.pfnGetUserMsgID = gpMetaUtilFuncs->pfnGetUserMsgID;

   // "1:0" -> metamod major=1, plugin major=5 -> plugin is newer
   int ret = Meta_Query((char*)"1:0", &pInfo, &mfuncs);
   ASSERT_INT(ret, FALSE);

   PASS();
   return 0;
}

static int test_meta_query_metamod_newer_major(void)
{
   TEST("Meta_Query: metamod newer major -> FALSE");

   reset_test_state();

   plugin_info_t *pInfo = NULL;
   mutil_funcs_t mfuncs;
   memset(&mfuncs, 0, sizeof(mfuncs));
   mfuncs.pfnLogConsole = mock_mutil_LogConsole;
   mfuncs.pfnLogMessage = mock_mutil_LogMessage;
   mfuncs.pfnLogError = mock_mutil_LogError;
   mfuncs.pfnGetUserMsgID = gpMetaUtilFuncs->pfnGetUserMsgID;

   // "99:0" -> metamod major=99 -> plugin major=5, plugin outdated
   int ret = Meta_Query((char*)"99:0", &pInfo, &mfuncs);
   ASSERT_INT(ret, FALSE);

   PASS();
   return 0;
}

static int test_meta_query_same_major_newer_minor(void)
{
   TEST("Meta_Query: same major newer minor -> TRUE");

   reset_test_state();

   plugin_info_t *pInfo = NULL;
   mutil_funcs_t mfuncs;
   memset(&mfuncs, 0, sizeof(mfuncs));
   mfuncs.pfnLogConsole = mock_mutil_LogConsole;
   mfuncs.pfnLogMessage = mock_mutil_LogMessage;
   mfuncs.pfnLogError = mock_mutil_LogError;
   mfuncs.pfnGetUserMsgID = gpMetaUtilFuncs->pfnGetUserMsgID;

   // "5:99" -> same major, mm newer minor -> ok
   int ret = Meta_Query((char*)"5:99", &pInfo, &mfuncs);
   ASSERT_INT(ret, TRUE);

   PASS();
   return 0;
}

static int test_meta_query_sets_plugin_info(void)
{
   TEST("Meta_Query: sets Plugin_info.version");

   reset_test_state();

   plugin_info_t *pInfo = NULL;
   mutil_funcs_t mfuncs;
   memset(&mfuncs, 0, sizeof(mfuncs));
   mfuncs.pfnLogConsole = mock_mutil_LogConsole;
   mfuncs.pfnLogMessage = mock_mutil_LogMessage;
   mfuncs.pfnLogError = mock_mutil_LogError;
   mfuncs.pfnGetUserMsgID = gpMetaUtilFuncs->pfnGetUserMsgID;

   Meta_Query((char*)META_INTERFACE_VERSION, &pInfo, &mfuncs);

   ASSERT_PTR_NOT_NULL(pInfo);
   ASSERT_PTR_NOT_NULL(pInfo->version);
   ASSERT_TRUE(strlen(pInfo->version) > 0);

   PASS();
   return 0;
}

static int test_meta_query_sets_gpMetaUtilFuncs(void)
{
   TEST("Meta_Query: sets gpMetaUtilFuncs");

   reset_test_state();

   plugin_info_t *pInfo = NULL;
   mutil_funcs_t mfuncs;
   memset(&mfuncs, 0, sizeof(mfuncs));
   mfuncs.pfnLogConsole = mock_mutil_LogConsole;
   mfuncs.pfnLogMessage = mock_mutil_LogMessage;
   mfuncs.pfnLogError = mock_mutil_LogError;
   mfuncs.pfnGetUserMsgID = gpMetaUtilFuncs->pfnGetUserMsgID;

   Meta_Query((char*)META_INTERFACE_VERSION, &pInfo, &mfuncs);

   ASSERT_PTR_EQ(gpMetaUtilFuncs, &mfuncs);

   PASS();
   return 0;
}

// ============================================================
// Meta_Attach tests
// ============================================================

static int test_meta_attach_normal(void)
{
   TEST("Meta_Attach: normal attach -> TRUE");

   reset_test_state();

   META_FUNCTIONS funcs;
   memset(&funcs, 0, sizeof(funcs));
   meta_globals_t mglobals;
   memset(&mglobals, 0, sizeof(mglobals));
   gamedll_funcs_t gdll;
   DLL_FUNCTIONS dll_table;
   memset(&dll_table, 0, sizeof(dll_table));
   gdll.dllapi_table = &dll_table;
   gdll.newapi_table = NULL;

   int ret = Meta_Attach(PT_STARTUP, &funcs, &mglobals, &gdll);
   ASSERT_INT(ret, TRUE);
   ASSERT_PTR_NOT_NULL((void*)funcs.pfnGetEntityAPI2);
   ASSERT_PTR_NOT_NULL((void*)funcs.pfnGetEntityAPI2_Post);
   ASSERT_PTR_EQ(gpMetaGlobals, &mglobals);
   ASSERT_PTR_EQ(gpGamedllFuncs, &gdll);
   ASSERT_TRUE(mock_cvar_register_called > 0);
   ASSERT_TRUE(mock_reg_svr_cmd_called > 0);

   PASS();
   return 0;
}

static int test_meta_attach_too_late(void)
{
   TEST("Meta_Attach: too late to load -> FALSE");

   reset_test_state();

   META_FUNCTIONS funcs;
   memset(&funcs, 0, sizeof(funcs));
   meta_globals_t mglobals;
   memset(&mglobals, 0, sizeof(mglobals));
   gamedll_funcs_t gdll;
   DLL_FUNCTIONS dll_table;
   memset(&dll_table, 0, sizeof(dll_table));
   gdll.dllapi_table = &dll_table;
   gdll.newapi_table = NULL;

   // PT_ANYTIME (3) > PT_STARTUP (1) -> too late
   int ret = Meta_Attach(PT_ANYTIME, &funcs, &mglobals, &gdll);
   ASSERT_INT(ret, FALSE);

   PASS();
   return 0;
}

static int test_meta_attach_registers_cvar(void)
{
   TEST("Meta_Attach: registers jk_botti_version cvar");

   reset_test_state();

   META_FUNCTIONS funcs;
   memset(&funcs, 0, sizeof(funcs));
   meta_globals_t mglobals;
   memset(&mglobals, 0, sizeof(mglobals));
   gamedll_funcs_t gdll;
   DLL_FUNCTIONS dll_table;
   memset(&dll_table, 0, sizeof(dll_table));
   gdll.dllapi_table = &dll_table;
   gdll.newapi_table = NULL;

   mock_cvar_register_called = 0;
   mock_cvar_set_string_called = 0;

   Meta_Attach(PT_STARTUP, &funcs, &mglobals, &gdll);

   ASSERT_TRUE(mock_cvar_register_called > 0);
   ASSERT_TRUE(mock_cvar_set_string_called > 0);
   ASSERT_STR(mock_cvar_set_string_name, "jk_botti_version");

   PASS();
   return 0;
}

// ============================================================
// Meta_Detach tests
// ============================================================

static int test_meta_detach_normal(void)
{
   TEST("Meta_Detach: normal detach -> TRUE, kicks bots");

   reset_test_state();

   bots[2].is_used = TRUE;
   bots[2].pEdict = mock_alloc_edict();

   int ret = Meta_Detach(PT_ANYTIME, PNL_COMMAND);
   ASSERT_INT(ret, TRUE);
   ASSERT_TRUE(bot_kick_called > 0);

   PASS();
   return 0;
}

static int test_meta_detach_kicks_all_bots(void)
{
   TEST("Meta_Detach: kicks all active bots");

   reset_test_state();

   bots[0].is_used = TRUE;
   bots[0].pEdict = mock_alloc_edict();
   bots[5].is_used = TRUE;
   bots[5].pEdict = mock_alloc_edict();
   bots[31].is_used = TRUE;
   bots[31].pEdict = mock_alloc_edict();

   Meta_Detach(PT_ANYTIME, PNL_COMMAND);
   ASSERT_INT(bot_kick_called, 3);

   PASS();
   return 0;
}

static int test_meta_detach_too_early(void)
{
   TEST("Meta_Detach: too early to unload -> FALSE");

   reset_test_state();

   // unloadable = PT_ANYTIME = 3, now = PT_ANYPAUSE = 4 -> 4 > 3
   int ret = Meta_Detach(PT_ANYPAUSE, PNL_COMMAND);
   ASSERT_INT(ret, FALSE);

   PASS();
   return 0;
}

static int test_meta_detach_forced_overrides(void)
{
   TEST("Meta_Detach: PNL_CMD_FORCED overrides timing");

   reset_test_state();

   int ret = Meta_Detach(PT_ANYPAUSE, PNL_CMD_FORCED);
   ASSERT_INT(ret, TRUE);

   PASS();
   return 0;
}

static int test_meta_detach_frees_resources(void)
{
   TEST("Meta_Detach: frees breakables and cfg records");

   reset_test_state();

   edict_t *pBreak = mock_alloc_edict();
   mock_set_classname(pBreak, "func_breakable");
   pBreak->v.health = 100;
   mock_add_breakable(pBreak, 1);
   ASSERT_PTR_NOT_NULL(UTIL_LookupBreakable(pBreak));

   Meta_Detach(PT_ANYTIME, PNL_COMMAND);

   ASSERT_PTR_NULL(UTIL_LookupBreakable(pBreak));
   ASSERT_TRUE(waypoint_init_called > 0);
   ASSERT_TRUE(free_posdata_list_called > 0);
   ASSERT_TRUE(free_cfg_bot_record_called > 0);

   PASS();
   return 0;
}

// ============================================================
// ClientPutInServer additional tests
// ============================================================

static int test_client_put_in_server_invalid_index_0(void)
{
   TEST("ClientPutInServer: worldspawn (idx=0) -> error path");

   reset_test_state();

   // Worldspawn is edict index 0 -> idx = 0 - 1 = -1, fails idx >= 0
   edict_t *pWorld = &mock_edicts[0];

   // Save current players state
   edict_t *saved = players[0].pEdict;

   api_table.pfnClientPutInServer(pWorld);

   // players[0] should be unchanged (error path taken)
   ASSERT_PTR_EQ(players[0].pEdict, saved);

   PASS();
   return 0;
}

// ============================================================
// PlayerPostThink_Post tests
// ============================================================

static int test_playerpostthink_deathmatch_0(void)
{
   TEST("PlayerPostThink_Post: deathmatch=0 -> ignored");

   reset_test_state();
   gpGlobals->deathmatch = 0;

   edict_t *pPlayer = mock_alloc_edict();
   pPlayer->v.flags = FL_CLIENT;
   pPlayer->v.health = 100;

   // PlayerPostThink_Post calls CheckPlayerChatProtection (trace-based),
   // GatherPlayerData, SaveAliveStatus. In deathmatch=0 it should return
   // immediately. We verify by checking no crash occurs.
   api_post_table.pfnPlayerPostThink(pPlayer);

   PASS();
   return 0;
}

static int test_playerpostthink_full_path(void)
{
   TEST("PlayerPostThink_Post: deathmatch=1 -> runs callbacks");

   reset_test_state();

   // Set up a player edict at index 1 -> players[0]
   edict_t *pPlayer = &mock_edicts[1];
   memset(pPlayer, 0, sizeof(*pPlayer));
   pPlayer->v.pContainingEntity = pPlayer;
   pPlayer->v.flags = FL_CLIENT;
   pPlayer->v.health = 100;
   pPlayer->v.origin = Vector(0, 0, 0);
   pPlayer->v.view_ofs = Vector(0, 0, 28);
   pPlayer->v.v_angle = Vector(0, 0, 0);
   pPlayer->free = 0;
   players[0].pEdict = pPlayer;

   // Should call CheckPlayerChatProtection, GatherPlayerData, SaveAliveStatus
   // without crashing
   api_post_table.pfnPlayerPostThink(pPlayer);

   PASS();
   return 0;
}

// ============================================================
// new_PM_PlaySound tests
// ============================================================

static int old_pm_playsound_called = 0;
static void tracking_old_pm_playsound(int channel, const char *sample,
   float volume, float attenuation, int fFlags, int pitch)
{
   (void)channel; (void)sample; (void)volume;
   (void)attenuation; (void)fFlags; (void)pitch;
   old_pm_playsound_called++;
}

static int test_pm_playsound_deathmatch_0(void)
{
   TEST("new_PM_PlaySound: deathmatch=0 -> calls old fn");

   reset_test_state();

   // Install the hook via PM_Move
   struct playermove_s &ppmove = pm_move_test_ppmove;
   memset(&ppmove, 0, sizeof(ppmove));
   ppmove.PM_PlaySound = tracking_old_pm_playsound;
   api_table.pfnPM_Move(&ppmove, TRUE);

   // Switch to deathmatch=0
   gpGlobals->deathmatch = 0;
   old_pm_playsound_called = 0;

   // Call the hooked PM_PlaySound
   ppmove.PM_PlaySound(0, "player/step.wav", 1.0f, 1.0f, 0, 100);

   ASSERT_INT(old_pm_playsound_called, 1);

   PASS();
   return 0;
}

static int test_pm_playsound_invalid_player(void)
{
   TEST("new_PM_PlaySound: invalid player idx -> no crash");

   reset_test_state();

   // Install the hook
   struct playermove_s &ppmove = pm_move_test_ppmove;
   memset(&ppmove, 0, sizeof(ppmove));
   ppmove.PM_PlaySound = tracking_old_pm_playsound;
   api_table.pfnPM_Move(&ppmove, TRUE);

   old_pm_playsound_called = 0;
   mock_current_player = -1; // invalid player index

   ppmove.PM_PlaySound(0, "player/step.wav", 1.0f, 1.0f, 0, 100);

   // Should still call old PM_PlaySound
   ASSERT_INT(old_pm_playsound_called, 1);

   PASS();
   return 0;
}

static int test_pm_playsound_valid_player(void)
{
   TEST("new_PM_PlaySound: valid player -> full path");

   reset_test_state();

   // Install the hook
   struct playermove_s &ppmove = pm_move_test_ppmove;
   memset(&ppmove, 0, sizeof(ppmove));
   ppmove.PM_PlaySound = tracking_old_pm_playsound;
   api_table.pfnPM_Move(&ppmove, TRUE);

   // Set up a valid player edict at index 1 (ENGINE_CURRENT_PLAYER returns 0-based)
   mock_current_player = 0; // player 0 -> INDEXENT(0+1) = mock_edicts[1]
   edict_t *pPlayer = &mock_edicts[1];
   memset(pPlayer, 0, sizeof(*pPlayer));
   pPlayer->v.pContainingEntity = pPlayer;
   pPlayer->v.origin = Vector(100, 200, 300);

   old_pm_playsound_called = 0;

   // This calls SaveSound (from bot_sound.o) and then old_PM_PlaySound
   ppmove.PM_PlaySound(0, "player/step.wav", 1.0f, 1.0f, 0, 100);

   ASSERT_INT(old_pm_playsound_called, 1);

   PASS();
   return 0;
}

extern void new_PM_PlaySound(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch);

static int test_pm_playsound_volume_formula(void)
{
   TEST("new_PM_PlaySound: volume maps [0,1] to [0,1000]");

   reset_test_state();

   // Install the hook so old_PM_PlaySound is non-NULL
   struct playermove_s &ppmove = pm_move_test_ppmove;
   memset(&ppmove, 0, sizeof(ppmove));
   ppmove.PM_PlaySound = tracking_old_pm_playsound;
   api_table.pfnPM_Move(&ppmove, TRUE);

   // Set up valid player
   mock_current_player = 0;
   edict_t *pPlayer = &mock_edicts[1];
   memset(pPlayer, 0, sizeof(*pPlayer));
   pPlayer->v.pContainingEntity = pPlayer;
   pPlayer->v.origin = Vector(100, 200, 300);
   gpGlobals->maxClients = 1;

   // Set up CSoundEnt so SaveSound stores the sound
   static CSoundEnt soundEnt;
   pSoundEnt = &soundEnt;
   pSoundEnt->Initialize();

   // Call with volume=0 (silent)
   new_PM_PlaySound(1, "player/step.wav", 0.0f, 1.0f, 0, 100);

   // volume=0 should give ivolume=0, not 500
   CSound *snd = CSoundEnt::GetEdictChannelSound(pPlayer, 1);
   ASSERT_PTR_NOT_NULL(snd);
   ASSERT_INT(snd->m_iVolume, 0);

   pSoundEnt->Initialize();

   // Call with volume=1.0 (max)
   new_PM_PlaySound(2, "player/step.wav", 1.0f, 1.0f, 0, 100);

   snd = CSoundEnt::GetEdictChannelSound(pPlayer, 2);
   ASSERT_PTR_NOT_NULL(snd);
   ASSERT_INT(snd->m_iVolume, 1000);

   pSoundEnt = NULL;
   PASS();
   return 0;
}

// ============================================================
// StartFrame alive player waypoint loop tests
// ============================================================

static int test_startframe_waypoint_think_alive_player(void)
{
   TEST("StartFrame: alive player -> WaypointThink called");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;
   extern float waypoint_time;
   waypoint_time = 0.0;

   // Set up an alive real player at edict index 1
   // Must be: FL_CLIENT, NOT FL_FAKECLIENT, NOT FL_THIRDPARTYBOT, NOT FL_PROXY
   // IsAlive needs: health>0, deadflag=DEAD_NO, takedamage!=0, solid!=SOLID_NOT, !FL_NOTARGET
   edict_t *pPlayer = &mock_edicts[1];
   memset(pPlayer, 0, sizeof(*pPlayer));
   pPlayer->v.pContainingEntity = pPlayer;
   pPlayer->v.flags = FL_CLIENT | FL_ONGROUND;
   pPlayer->v.health = 100;
   pPlayer->v.deadflag = DEAD_NO;
   pPlayer->v.takedamage = DAMAGE_YES;
   pPlayer->v.solid = SOLID_BBOX;
   pPlayer->free = 0;

   waypoint_think_called = 0;

   api_table.pfnStartFrame();

   ASSERT_TRUE(waypoint_think_called > 0);

   PASS();
   return 0;
}

// ============================================================
// StartFrame ProcessBotCfgFile tests
// ============================================================

static int test_startframe_process_bot_cfg(void)
{
   TEST("StartFrame: bot_cfg_fp set -> ProcessBotCfgFile");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;
   extern FILE *bot_cfg_fp;
   extern float bot_cfg_pause_time;

   // Use a dummy non-NULL FILE pointer (we never actually read from it)
   bot_cfg_fp = (FILE*)1;
   bot_cfg_pause_time = gpGlobals->time - 1.0; // expired
   process_bot_cfg_file_called = 0;

   api_table.pfnStartFrame();

   ASSERT_TRUE(process_bot_cfg_file_called > 0);

   // Reset bot_cfg_fp to avoid dangling pointer in later tests
   bot_cfg_fp = NULL;

   PASS();
   return 0;
}

// ============================================================
// StartFrame bot add/remove tests
// ============================================================

static int test_startframe_bot_add(void)
{
   TEST("StartFrame: client_count < max_bots -> BotCreate");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   extern float bot_check_time;
   min_bots = 1;
   max_bots = 5;
   bot_check_time = 0.0; // expired

   // No clients connected (client_count=0 < max_bots=5)
   // Also need client_count < maxClients
   bot_create_called = 0;

   api_table.pfnStartFrame();

   ASSERT_TRUE(bot_create_called > 0);

   PASS();
   return 0;
}

static int test_startframe_bot_remove(void)
{
   TEST("StartFrame: client_count > max_bots -> BotKick");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   extern float bot_check_time;
   min_bots = 0;
   max_bots = 1;
   bot_check_time = 0.0; // expired

   // Set up 3 bots as connected clients that UTIL_GetClientCount sees.
   // mock_alloc_edict advances mock_next_edict so pfnGetPlayerUserId returns > 0.
   // We allocate edicts 1,2,3 via mock_alloc_edict to ensure they're in range.
   for (int i = 0; i < 3; i++)
   {
      edict_t *e = mock_alloc_edict(); // allocates next free edict (1, 2, 3)
      e->v.flags = FL_CLIENT | FL_FAKECLIENT;
      e->v.netname = (string_t)(long)"Bot";
      e->free = 0;

      int idx = ENTINDEX(e) - 1;
      players[idx].pEdict = e;

      bots[i].is_used = TRUE;
      bots[i].pEdict = e;
      bots[i].bot_think_time = 999.0; // not due yet
   }

   bot_kick_called = 0;

   api_table.pfnStartFrame();

   // client_count=3 > max_bots=1 && bot_count=3 > min_bots=0 -> BotKick
   ASSERT_TRUE(bot_kick_called > 0);

   PASS();
   return 0;
}

static int test_startframe_bot_no_add_no_remove(void)
{
   TEST("StartFrame: count in range -> no add/remove");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   extern float bot_check_time;

   // Set up 2 bots as connected clients
   for (int i = 0; i < 2; i++)
   {
      edict_t *e = mock_alloc_edict();
      e->v.flags = FL_CLIENT | FL_FAKECLIENT;
      e->v.netname = (string_t)(long)"Bot";
      e->free = 0;

      int idx = ENTINDEX(e) - 1;
      players[idx].pEdict = e;

      bots[i].is_used = TRUE;
      bots[i].pEdict = e;
      bots[i].bot_think_time = 999.0;
   }

   // client_count=2, bot_count=2
   // For "no add": need !(client_count < max_bots || bot_count < min_bots)
   //   => client_count >= max_bots AND bot_count >= min_bots
   //   => 2 >= 2 AND 2 >= 1
   // For "no remove": need !(client_count > max_bots && bot_count > min_bots)
   //   => either client_count <= max_bots OR bot_count <= min_bots
   //   => 2 <= 2 (true)
   min_bots = 1;
   max_bots = 2;
   bot_check_time = 0.0; // expired

   bot_create_called = 0;
   bot_kick_called = 0;

   api_table.pfnStartFrame();

   ASSERT_INT(bot_create_called, 0);
   ASSERT_INT(bot_kick_called, 0);

   PASS();
   return 0;
}

// ============================================================
// StartFrame WaypointSlowFloyds loop test
// ============================================================

static int test_startframe_waypoint_slow_floyds(void)
{
   TEST("StartFrame: floyds state != -1 -> loop runs");

   reset_test_state();
   extern qboolean need_to_open_cfg;
   need_to_open_cfg = FALSE;
   extern int min_bots;
   extern int max_bots;
   min_bots = -1;
   max_bots = -1;

   // Set floyds state to 0 (not -1) so the loop enters
   // WaypointSlowFloyds stub returns -1 so loop breaks immediately
   waypoint_slow_floyds_state_ret = 0;

   // Should not crash and the loop should enter
   api_table.pfnStartFrame();

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_dll:\n");

   printf("=== GetEntityAPI2 tests ===\n");
   fail |= test_GetEntityAPI2_returns_true();
   fail |= test_GetEntityAPI2_fills_function_table();
   fail |= test_GetEntityAPI2_POST_returns_true();
   fail |= test_GetEntityAPI2_POST_fills_function_table();

   printf("=== CheckSubMod tests ===\n");
   fail |= test_checksubmod_opposing_force();
   fail |= test_checksubmod_opfor_short();
   fail |= test_checksubmod_severians();
   fail |= test_checksubmod_xdm();
   fail |= test_checksubmod_bubblemod();
   fail |= test_checksubmod_default_hldm();
   fail |= test_checksubmod_hl_teamplay_sevs();
   fail |= test_checksubmod_hl_teamplay_hldm();

   printf("=== GameDLLInit tests ===\n");
   fail |= test_gamedllinit_sets_submod_weaponflag();
   fail |= test_gamedllinit_clears_players_and_bots();

   printf("=== Spawn tests ===\n");
   fail |= test_spawn_worldspawn();
   fail |= test_spawn_worldspawn_precaches();
   fail |= test_spawn_worldspawn_resets_team_data();
   fail |= test_spawn_worldspawn_sets_bot_check_time();
   fail |= test_spawn_worldspawn_sets_waypoint_time();
   fail |= test_spawn_func_plat_saves_origin();
   fail |= test_spawn_func_door_saves_origin();
   fail |= test_spawn_deathmatch_0_ignored();

   printf("=== Spawn_Post tests ===\n");
   fail |= test_spawn_post_worldspawn_calls_bot_check_teamplay();
   fail |= test_spawn_post_func_plat_adds_lift_waypoint();
   fail |= test_spawn_post_func_plat_uses_default_height();
   fail |= test_spawn_post_func_plat_with_targetname();
   fail |= test_spawn_post_func_door_adds_lift_waypoint();
   fail |= test_spawn_post_func_door_start_open();
   fail |= test_spawn_post_other_entity_collects_spawn_items();
   fail |= test_spawn_post_deathmatch_0_ignored();
   fail |= test_spawn_post_resets_m_height_m_lip();

   printf("=== DispatchKeyValue_Post tests ===\n");
   fail |= test_dispatch_keyvalue_func_breakable();
   fail |= test_dispatch_keyvalue_func_breakable_trigger_only();
   fail |= test_dispatch_keyvalue_func_pushable();
   fail |= test_dispatch_keyvalue_func_plat_height();
   fail |= test_dispatch_keyvalue_func_door_lip();
   fail |= test_dispatch_keyvalue_deathmatch_0_ignored();

   printf("=== ClientConnect tests ===\n");
   fail |= test_client_connect_loopback();
   fail |= test_client_connect_bot_address();
   fail |= test_client_connect_other_bot_address();
   fail |= test_client_connect_deathmatch_0();

   printf("=== ClientPutInServer tests ===\n");
   fail |= test_client_put_in_server_valid_index();
   fail |= test_client_put_in_server_index_5();
   fail |= test_client_put_in_server_deathmatch_0();
   fail |= test_client_put_in_server_invalid_index_0();

   printf("=== CmdStart tests ===\n");
   fail |= test_cmdstart_bot_fills_name();
   fail |= test_cmdstart_bot_already_has_name();
   fail |= test_cmdstart_third_party_bot_auto_connects();

   printf("=== ClientDisconnect tests ===\n");
   fail |= test_client_disconnect_removes_player();
   fail |= test_client_disconnect_removes_bot();
   fail |= test_client_disconnect_no_matching_bot();
   fail |= test_client_disconnect_deathmatch_0();

   printf("=== ServerDeactivate tests ===\n");
   fail |= test_server_deactivate_frees_breakables();
   fail |= test_server_deactivate_auto_waypoint_save();
   fail |= test_server_deactivate_no_save_without_update();
   fail |= test_server_deactivate_saves_floyds_matrix();
   fail |= test_server_deactivate_deathmatch_0();

   printf("=== PlayerPostThink_Post tests ===\n");
   fail |= test_playerpostthink_deathmatch_0();
   fail |= test_playerpostthink_full_path();

   printf("=== PM_Move tests ===\n");
   fail |= test_pm_move_hooks_playsound();
   fail |= test_pm_move_deathmatch_0();

   printf("=== new_PM_PlaySound tests ===\n");
   fail |= test_pm_playsound_deathmatch_0();
   fail |= test_pm_playsound_invalid_player();
   fail |= test_pm_playsound_valid_player();
   fail |= test_pm_playsound_volume_formula();

   printf("=== StartFrame tests ===\n");
   fail |= test_startframe_deathmatch_0_ignored();
   fail |= test_startframe_bot_stop_no_thinking();
   fail |= test_startframe_normal_with_bot();
   fail |= test_startframe_bot_think_time_not_reached();
   fail |= test_startframe_config_file_processing();
   fail |= test_startframe_waypoint_addspawnobjects();
   fail |= test_startframe_sound_entity_think();
   fail |= test_startframe_num_bots_tracks_count();
   fail |= test_startframe_waypoint_think_alive_player();
   fail |= test_startframe_process_bot_cfg();
   fail |= test_startframe_bot_add();
   fail |= test_startframe_bot_remove();
   fail |= test_startframe_bot_no_add_no_remove();
   fail |= test_startframe_waypoint_slow_floyds();

   printf("=== Meta_Query tests ===\n");
   fail |= test_meta_query_compatible_version();
   fail |= test_meta_query_plugin_newer_than_metamod();
   fail |= test_meta_query_metamod_newer_major();
   fail |= test_meta_query_same_major_newer_minor();
   fail |= test_meta_query_sets_plugin_info();
   fail |= test_meta_query_sets_gpMetaUtilFuncs();

   printf("=== Meta_Attach tests ===\n");
   fail |= test_meta_attach_normal();
   fail |= test_meta_attach_too_late();
   fail |= test_meta_attach_registers_cvar();

   printf("=== Meta_Detach tests ===\n");
   fail |= test_meta_detach_normal();
   fail |= test_meta_detach_kicks_all_bots();
   fail |= test_meta_detach_too_early();
   fail |= test_meta_detach_forced_overrides();
   fail |= test_meta_detach_frees_resources();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
