//
// JK_Botti - placeholder tests for dll.cpp
//
// test_dll.cpp
//

#include <stdlib.h>
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

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
void CollectMapSpawnItems(edict_t *pSpawn) { (void)pSpawn; }

// bot_combat.cpp functions
void free_posdata_list(int idx) { (void)idx; }
void GatherPlayerData(edict_t *pEdict) { (void)pEdict; }

// commands.cpp functions
void ProcessBotCfgFile(void) {}
void jk_botti_ServerCommand(void) {}
void ClientCommand(edict_t *pEntity) { (void)pEntity; }
int AddToCfgBotRecord(const char *skin, const char *name, int skill, int top_color, int bottom_color)
{ (void)skin; (void)name; (void)skill; (void)top_color; (void)bottom_color; return 0; }
void FreeCfgBotRecord(void) {}

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
void WaypointInit(void) {}
void WaypointAddSpawnObjects(void) {}
void WaypointAddLift(edict_t *pent, const Vector &start, const Vector &end) { (void)pent; (void)start; (void)end; }
qboolean WaypointLoad(edict_t *pEntity) { (void)pEntity; return FALSE; }
void WaypointSave(void) {}
void WaypointSaveFloydsMatrix(void) {}
int WaypointSlowFloyds(void) { return 0; }
int WaypointSlowFloydsState(void) { return 0; }
void WaypointThink(edict_t *pEntity) { (void)pEntity; }

// engine.cpp functions (need extern "C" to match C_DLLEXPORT declarations in dll.cpp)
extern "C" int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{ (void)pengfuncsFromEngine; (void)interfaceVersion; return TRUE; }
extern "C" int GetEngineFunctions_POST(enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{ (void)pengfuncsFromEngine; (void)interfaceVersion; return TRUE; }

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("dll.cpp placeholder");

   mock_reset();

   // Verify dll.o globals are accessible
   ASSERT_INT(submod_id, SUBMOD_HLDM);

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
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
