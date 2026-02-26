//
// JK_Botti - placeholder tests for commands.cpp
//
// test_commands.cpp
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
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("commands.cpp placeholder");

   mock_reset();

   // Verify commands.o globals are accessible
   extern qboolean isFakeClientCommand;
   extern int bot_conntimes;
   ASSERT_INT(isFakeClientCommand, FALSE);
   ASSERT_INT(bot_conntimes, 0);

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
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
