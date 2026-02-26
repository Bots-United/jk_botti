//
// JK_Botti - placeholder tests for bot.cpp
//
// test_bot.cpp
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
#include "bot_config_init.h"
#include "bot_client.h"
#include "waypoint.h"
#include "player.h"

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
int team_balancetype = 1;
char *team_blockedlist = NULL;
float bot_think_spf = 1.0f/30.0f;
float bot_check_time = 60.0;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
float welcome_time = 0.0;
qboolean welcome_sent = FALSE;
int bot_stop = 0;
int frame_count = 0;
int randomize_bots_on_mapchange = 0;
float bot_cfg_pause_time = 0.0;
FILE *bot_cfg_fp = NULL;
int bot_cfg_linenumber = 0;
qboolean need_to_open_cfg = TRUE;
qboolean spawn_time_reset = FALSE;
float waypoint_time = 0.0;

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

// bot_combat.cpp functions
void BotAimPre(bot_t &pBot) { (void)pBot; }
void BotAimPost(bot_t &pBot) { (void)pBot; }
void free_posdata_list(int idx) { (void)idx; }
void GatherPlayerData(edict_t *pEdict) { (void)pEdict; }
qboolean FPredictedVisible(bot_t &pBot) { (void)pBot; return FALSE; }
void BotUpdateHearingSensitivity(bot_t &pBot) { (void)pBot; }
void BotFindEnemy(bot_t &pBot) { (void)pBot; }
void BotShootAtEnemy(bot_t &pBot) { (void)pBot; }
qboolean BotShootTripmine(bot_t &pBot) { (void)pBot; return FALSE; }
qboolean BotDetonateSatchel(bot_t &pBot) { (void)pBot; return FALSE; }

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

// bot_navigate.cpp functions not already in engine_mock (weak stubs)
void BotOnLadder(bot_t &pBot, float moved_distance) { (void)pBot; (void)moved_distance; }
void BotUnderWater(bot_t &pBot) { (void)pBot; }
void BotUseLift(bot_t &pBot, float moved_distance) { (void)pBot; (void)moved_distance; }
void BotTurnAtWall(bot_t &pBot, TraceResult *tr, qboolean negative) { (void)pBot; (void)tr; (void)negative; }
void BotRandomTurn(bot_t &pBot) { (void)pBot; }

// waypoint.cpp functions
void WaypointInit(void) {}
int WaypointFindRunawayPath(int runner, int enemy) { (void)runner; (void)enemy; return -1; }
float wp_display_time[MAX_WAYPOINTS];
qboolean g_waypoint_on = FALSE;
qboolean g_auto_waypoint = FALSE;
qboolean g_path_waypoint = FALSE;
qboolean g_path_waypoint_enable = FALSE;
qboolean g_waypoint_updated = FALSE;
BOOL wp_matrix_save_on_mapend = FALSE;

// bot_models.cpp globals
int number_skins = 0;
skin_t bot_skins[MAX_SKINS];

// bot_query_hook
bool hook_sendto_function(void) { return true; }
bool unhook_sendto_function(void) { return true; }

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("bot.cpp placeholder");

   mock_reset();

   // Verify bots array defined by bot.o is accessible
   ASSERT_TRUE(sizeof(bots) == sizeof(bot_t) * 32);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
