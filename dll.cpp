//
// JK_Botti - be more human!
//
// dll.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>
#include <pm_defs.h>
#include <time.h>

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"

#define VER_MAJOR 0
#define VER_MINOR 20


#define MENU_NONE  0
#define MENU_1     1
#define MENU_2     2
#define MENU_3     3
#define MENU_4     4

extern DLL_FUNCTIONS gFunctionTable;
extern enginefuncs_t g_engfuncs;
extern globalvars_t  *gpGlobals;
extern char g_argv[1024*3];
extern char g_arg1[1024];
extern char g_arg2[1024];
extern char g_arg3[1024];
extern qboolean g_waypoint_on;
extern qboolean g_auto_waypoint;
extern qboolean g_path_waypoint;
extern qboolean g_path_waypoint_enable;
extern qboolean g_waypoint_updated;
extern int num_waypoints;  // number of waypoints currently in use
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];
extern bot_t bots[32];
extern qboolean b_observer_mode;
extern qboolean b_botdontshoot;
extern char welcome_msg[80];

#include "bot_weapon_select.h"

int submod_id = SUBMOD_HLDM;

int m_spriteTexture = 0;
int default_bot_skill = 2;
int bot_add_level_tag = 0;   // use [lvl%d] for bots (where %d is skill level of bot)
//int bot_strafe_percent = 20; // percent of time to strafe
int bot_chat_percent = 10;   // percent of time to chat
int bot_taunt_percent = 20;  // percent of time to taunt after kill
int bot_whine_percent = 10;  // percent of time to whine after death
int bot_logo_percent = 40;   // percent of time to spray logo after kill

int bot_chat_tag_percent = 80;   // percent of the time to drop clan tag
int bot_chat_drop_percent = 10;  // percent of the time to drop characters
int bot_chat_swap_percent = 10;  // percent of the time to swap characters
int bot_chat_lower_percent = 50; // percent of the time to lowercase chat

float bot_think_spf = 1/30; // == 1 / (30 fps)

qboolean b_random_color = TRUE;
qboolean isFakeClientCommand = FALSE;
int fake_arg_count;
int IsDedicatedServer;
float bot_check_time = 60.0;
int bot_reaction_time = 2;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
qboolean g_GameRules = FALSE;
edict_t *clients[32];
edict_t *listenserver_edict = NULL;
float welcome_time = 0.0;
qboolean welcome_sent = FALSE;
int g_menu_waypoint;
int g_menu_state = 0;
int bot_stop = 0;
int randomize_bots_on_mapchange = 0;

extern int turn_method;

qboolean is_team_play = FALSE;
qboolean checked_teamplay = FALSE;

FILE *bot_cfg_fp = NULL;
qboolean need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
float respawn_time = 0.0;
float gather_data_time = 0.0;
qboolean spawn_time_reset = FALSE;
float waypoint_time = 0.0;

char *show_menu_none = {" "};
char *show_menu_1 =
   {"Waypoint Tags\n\n1. Team Specific\n2. Wait for Lift\n3. Door\n4. Sniper Spot\n5. More..."};
char *show_menu_2 =
   {"Waypoint Tags\n\n1. Team 1\n2. Team 2\n3. Team 3\n4. Team 4\n5. CANCEL"};
char *show_menu_2_flf =
   {"Waypoint Tags\n\n1. Attackers\n2. Defenders\n\n5. CANCEL"};
char *show_menu_3 =
   {"Waypoint Tags\n\n1. Flag Location\n2. Flag Goal Location\n3. Sentry gun\n4. Dispenser\n5. More"};
char *show_menu_3_flf =
   {"Waypoint Tags\n\n1. Capture Point\n2. Defend Point\n3. Prone\n\n5. CANCEL"};
char *show_menu_3_hw =
   {"Waypoint Tags\n\n1. Halo Location\n\n\n\n5. More"};
char *show_menu_4 =
   {"Waypoint Tags\n\n1. Health\n2. Armor\n3. Ammo\n4. Jump\n5. CANCEL"};


void BotNameInit(void);
void BotLogoInit(void);
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
void ProcessBotCfgFile(void);
void jk_botti_ServerCommand (void);



// START of Metamod stuff

enginefuncs_t meta_engfuncs;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;
meta_globals_t *gpMetaGlobals;

META_FUNCTIONS gMetaFunctionTable =
{
   NULL, // pfnGetEntityAPI()
   NULL, // pfnGetEntityAPI_Post()
   GetEntityAPI2, // pfnGetEntityAPI2()
   NULL, // pfnGetEntityAPI2_Post()
   NULL, // pfnGetNewDLLFunctions()
   NULL, // pfnGetNewDLLFunctions_Post()
   GetEngineFunctions, // pfnGetEngineFunctions()
   NULL, // pfnGetEngineFunctions_Post()
};


static char plugver[32];
plugin_info_t Plugin_info = {
   META_INTERFACE_VERSION, // interface version
   "JK_Botti", // plugin name
   "", // plugin version
   "11/12/05 (dd/mm/yy)", // date of creation
   "Jussi Kivilinna", // plugin author
   "http://koti.mbnet.fi/axh/", // plugin URL
   "JK_BOTTI", // plugin logtag
   PT_STARTUP, // when loadable
   PT_ANYTIME, // when unloadable
};


C_DLLEXPORT int Meta_Query (char *ifvers, plugin_info_t **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs)
{
   // this function is the first function ever called by metamod in the plugin DLL. Its purpose
   // is for metamod to retrieve basic information about the plugin, such as its meta-interface
   // version, for ensuring compatibility with the current version of the running metamod.

   // keep track of the pointers to metamod function tables metamod gives us
   gpMetaUtilFuncs = pMetaUtilFuncs;
   *pPlugInfo = &Plugin_info;

   snprintf(plugver, sizeof(plugver), "%d.%d", VER_MAJOR, VER_MINOR);
   Plugin_info.version = plugver;

   // check for interface version compatibility
   if (strcmp (ifvers, Plugin_info.ifvers) != 0)
   {
      int mmajor = 0, mminor = 0, pmajor = 0, pminor = 0;

      LOG_CONSOLE (PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);
      LOG_MESSAGE (PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);

      // if plugin has later interface version, it's incompatible (update metamod)
      sscanf (ifvers, "%d:%d", &mmajor, &mminor);
      sscanf (META_INTERFACE_VERSION, "%d:%d", &pmajor, &pminor);

      if ((pmajor > mmajor) || ((pmajor == mmajor) && (pminor > mminor)))
      {
         LOG_CONSOLE (PLID, "metamod version is too old for this plugin; update metamod");
         LOG_ERROR (PLID, "metamod version is too old for this plugin; update metamod");
         return (FALSE);
      }

      // if plugin has older major interface version, it's incompatible (update plugin)
      else if (pmajor < mmajor)
      {
         LOG_CONSOLE (PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
         LOG_ERROR (PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
         return (FALSE);
      }
   }

   return (TRUE); // tell metamod this plugin looks safe
}

C_DLLEXPORT int Meta_Attach (PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
   // this function is called when metamod attempts to load the plugin. Since it's the place
   // where we can tell if the plugin will be allowed to run or not, we wait until here to make
   // our initialization stuff, like registering CVARs and dedicated server commands.

   // are we allowed to load this plugin now ?
   if (now > Plugin_info.loadable)
   {
      LOG_CONSOLE (PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
      LOG_ERROR (PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
      return (FALSE); // returning FALSE prevents metamod from attaching this plugin
   }

   // keep track of the pointers to engine function tables metamod gives us
   gpMetaGlobals = pMGlobals;
   memcpy (pFunctionTable, &gMetaFunctionTable, sizeof (META_FUNCTIONS));
   gpGamedllFuncs = pGamedllFuncs;

   // print a message to notify about plugin attaching
   LOG_CONSOLE (PLID, "%s: plugin attaching", Plugin_info.name);
   LOG_MESSAGE (PLID, "%s: plugin attaching", Plugin_info.name);

   // ask the engine to register the server commands this plugin uses
   REG_SVR_COMMAND ("jk_botti", jk_botti_ServerCommand);
   
   return (TRUE); // returning TRUE enables metamod to attach this plugin
}


C_DLLEXPORT int Meta_Detach (PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
   // this function is called when metamod unloads the plugin. A basic check is made in order
   // to prevent unloading the plugin if its processing should not be interrupted.

   // is metamod allowed to unload the plugin ?
   if ((now > Plugin_info.unloadable) && (reason != PNL_CMD_FORCED))
   {
      LOG_CONSOLE (PLID, "%s: plugin NOT detaching (can't unload plugin right now)", Plugin_info.name);
      LOG_ERROR (PLID, "%s: plugin NOT detaching (can't unload plugin right now)", Plugin_info.name);
      return (FALSE); // returning FALSE prevents metamod from unloading this plugin
   }

   // kick any bot off of the server after time/frag limit...
   for (int index = 0; index < 32; index++)
   {
      if (bots[index].is_used)  // is this slot used?
      {
         char cmd[40];

         snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bots[index].name);

         bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
         bots[index].is_used = false;

         SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
      }
   }

   return (TRUE); // returning TRUE enables metamod to unload this plugin
}

// END of Metamod stuff


void GameDLLInit( void )
{
   int i;

   IsDedicatedServer = IS_DEDICATED_SERVER();

   for (i=0; i<32; i++)
      clients[i] = NULL;

   // initialize the bots array of structures...
   memset(bots, 0, sizeof(bots));

   BotNameInit();
   BotLogoInit();

   LoadBotChat();
   LoadBotModels();

   RETURN_META (MRES_IGNORED);
}


int Spawn( edict_t *pent )
{
   if (gpGlobals->deathmatch)
   {
      char *pClassname = (char *)STRING(pent->v.classname);

      if (strcmp(pClassname, "worldspawn") == 0)
      {
         // do level initialization stuff here...
         for(int i = 0; i < 32; i++)
            free_posdata_list(i);

         WaypointInit();
         WaypointLoad(NULL);

         PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
         PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
         PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
         PRECACHE_SOUND("common/wpn_hudon.wav");       // path add/delete done
         PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel
         PRECACHE_SOUND("common/wpn_denyselect.wav");  // path add/delete error
         PRECACHE_SOUND("player/sprayer.wav");         // logo spray sound

         m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr");

         g_GameRules = TRUE;

         is_team_play = FALSE;
         checked_teamplay = FALSE;

         bot_cfg_pause_time = 0.0;
         respawn_time = 0.0;
         gather_data_time = 0.0;
         waypoint_time = -1.0;
         spawn_time_reset = FALSE;

         prev_num_bots = num_bots;
         num_bots = 0;

         bot_check_time = gpGlobals->time + 60.0;
      }
      else
         CollectMapSpawnItems(pent);
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{ 
   if (gpGlobals->deathmatch)
   {
      int i;
      int count = 0;

      // check if this client is the listen server client
      if (strcmp(pszAddress, "loopback") == 0)
      {
         // save the edict of the listen server client...
         listenserver_edict = pEntity;
      }

      // check if this is NOT a bot joining the server...
      if (strcmp(pszAddress, "127.0.0.1") != 0)
      {
         // don't try to add bots for 60 seconds, give client time to get added
         bot_check_time = gpGlobals->time + 60.0;

         for (i=0; i < 32; i++)
         {
            if (bots[i].is_used)  // count the number of bots in use
               count++;
         }

         // if there are currently more than the minimum number of bots running
         // then kick one of the bots off the server...
         if ((count > min_bots) && (min_bots != -1))
         {
            for (i=0; i < 32; i++)
            {
               if (bots[i].is_used)  // is this slot used?
               {
                  char cmd[80];

                  snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bots[i].name);

                  SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

                  break;
               }
            }
         }
      }
      
      SaveSound(pEntity, 0, Vector(0,0,0), 0, 0, 0);
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


void ClientDisconnect( edict_t *pEntity )
{
   if (gpGlobals->deathmatch)
   {
      int i;

      i = 0;
      while ((i < 32) && (clients[i] != pEntity))
         i++;

      if (i < 32)
         clients[i] = NULL;


      for (i=0; i < 32; i++)
      {
         if (bots[i].pEdict == pEntity)
         {
            // someone kicked this bot off of the server...

            bots[i].is_used = FALSE;  // this slot is now free to use

            bots[i].f_kick_time = gpGlobals->time;  // save the kicked time

            break;
         }
      }
      
      SaveSound(pEntity, 0, Vector(0,0,0), 0, 0, 0);
   }

   RETURN_META (MRES_IGNORED);
}


void ClientPutInServer( edict_t *pEntity )
{
   int i = 0;

   while ((i < 32) && (clients[i] != NULL))
      i++;

   if (i < 32)
      clients[i] = pEntity;  // store this clients edict in the clients array
   
   SaveSound(pEntity, 0, Vector(0,0,0), 0, 0, 0);
   
   RETURN_META (MRES_IGNORED);
}


static void print_to_client(void *arg, char *msg) {
   ClientPrint((edict_t *)arg, HUD_PRINTNOTIFY, msg);
}

qboolean ProcessCommand(void (*printfunc)(void *arg, char *msg), void * arg, const char * pcmd, const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, qboolean is_cfgcmd) 
{
   const float globaltime = gpGlobals->time;
   char msg[128];
   qboolean is_clientcmd;
   qboolean is_servercmd;
   
   is_clientcmd = !is_cfgcmd && !!arg; //arg is pEntity on clientcmd
   is_servercmd = !is_clientcmd && !is_cfgcmd;
   
   if (FStrEq(pcmd, "info"))
   {
      UTIL_PrintBotInfo(printfunc, arg);
      
      return TRUE;
   }
   else if (FStrEq(pcmd, "addbot"))
   {
      BotCreate( (edict_t *)arg, arg1, arg2, arg3, arg4, arg5 );

      bot_check_time = globaltime + 5.0;
      if(is_cfgcmd)
         bot_cfg_pause_time = globaltime + 2.0;

      return TRUE;
   }
   else if (FStrEq(pcmd, "observer"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);
         if (temp)
            b_observer_mode = TRUE;
         else
            b_observer_mode = FALSE;
      }

      if (b_observer_mode)
         printfunc(arg, "observer mode ENABLED\n");
      else
         printfunc(arg, "observer mode DISABLED\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "botskill"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 1) || (temp > 5))
            printfunc(arg, "invalid botskill value!\n");
         else
            default_bot_skill = temp;
      }

      snprintf(msg, sizeof(msg), "botskill is %d\n", default_bot_skill);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "randomize_bots_on_mapchange"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         randomize_bots_on_mapchange = !!atoi(arg1);
      }

      snprintf(msg, sizeof(msg), "randomize_bots_on_mapchange is %s\n", (randomize_bots_on_mapchange?"on":"off"));
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_add_level_tag"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         bot_add_level_tag = !!atoi(arg1);
      }

      snprintf(msg, sizeof(msg), "bot_add_level_tag is %s\n", (bot_add_level_tag?"on":"off"));
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "turn_method"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if(temp != AIM_RACC_OLD && temp != AIM_RACC) 
            printfunc(arg, "invalid turn_method value!\n");
         else
            turn_method = temp;
      }

      snprintf(msg, sizeof(msg), "turn_method is %d\n", turn_method);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "botthinkfps"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 1) || (temp > 100))
            printfunc(arg, "invalid botthinkfps value!\n");
         else
            bot_think_spf = 1.0 / temp;
      }

      snprintf(msg, sizeof(msg), "botthinkfps is %.2f\n", 1.0 / bot_think_spf);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_percent value!\n");
         else
            bot_chat_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_chat_percent is %d\n", bot_chat_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_taunt_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_taunt_percent value!\n");
         else
            bot_taunt_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_taunt_percent is %d\n", bot_taunt_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_whine_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_whine_percent value!\n");
         else
            bot_whine_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_whine_percent is %d\n", bot_whine_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_tag_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_tag_percent value!\n");
         else
            bot_chat_tag_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_chat_tag_percent is %d\n", bot_chat_tag_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_drop_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_drop_percent value!\n");
         else
            bot_chat_drop_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_chat_drop_percent is %d\n", bot_chat_drop_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_swap_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_swap_percent value!\n");
         else
            bot_chat_swap_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_chat_swap_percent is %d\n", bot_chat_swap_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_lower_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_lower_percent value!\n");
         else
            bot_chat_lower_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_chat_lower_percent is %d\n", bot_chat_lower_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_logo_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_logo_percent value!\n");
         else
            bot_logo_percent = temp;
      }

      snprintf(msg, sizeof(msg), "bot_logo_percent is %d\n", bot_logo_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_reaction_time"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 3))
            printfunc(arg, "invalid bot_reaction_time value!\n");
         else
            bot_reaction_time = temp;
      }

      if (bot_reaction_time)
         snprintf(msg, sizeof(msg), "bot_reaction_time is %d\n", bot_reaction_time);
      else
         snprintf(msg, sizeof(msg), "bot_reaction_time is DISABLED\n");

      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "random_color"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if (temp)
            b_random_color = TRUE;
         else
            b_random_color = FALSE;
      }

      if (b_random_color)
         printfunc(arg, "random_color ENABLED\n");
      else
         printfunc(arg, "random_color DISABLED\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "botdontshoot"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);
         if (temp)
            b_botdontshoot = TRUE;
         else
            b_botdontshoot = FALSE;
      }

      if (b_botdontshoot)
         printfunc(arg, "botdontshoot ENABLED\n");
      else
         printfunc(arg, "botdontshoot DISABLED\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "min_bots"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         min_bots = atoi( arg1 );

         if ((min_bots < 0) || (min_bots > 31))
            min_bots = 1;
      }
      
      snprintf(msg, sizeof(msg), "min_bots is set to %d\n", min_bots);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "max_bots"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         max_bots = atoi( arg1 );

         if ((max_bots < 0) || (max_bots > 31))
            max_bots = 1;
      }
      
      snprintf(msg, sizeof(msg), "max_bots is set to %d\n", max_bots);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "pause") && is_cfgcmd)
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         bot_cfg_pause_time = globaltime + atoi( arg1 );
      }

      return TRUE;
   }
   else if (FStrEq(pcmd, "autowaypoint"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         if (FStrEq(arg1, "on"))
         {
            g_auto_waypoint = TRUE;
         }
         else if (FStrEq(arg1, "off"))
         {
            g_auto_waypoint = FALSE;
         }
         else
         {
            g_auto_waypoint = !!atoi(arg1);
         }

         if (g_auto_waypoint)
         {
            if(is_clientcmd)
               g_waypoint_on = TRUE; //just in case
            printfunc(arg, "autowaypoint is ON\n");
         }
         else
            printfunc(arg, "autowaypoint is OFF\n");
      }
      
      return TRUE;
   }
   else if (FStrEq(pcmd, "botweapon"))
   {  // this command allows editing of weapon information
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int select_index = -1;
         bot_weapon_select_t *pSelect = &valve_weapon_select[0];
         
         while ((arg2 != NULL) && (*arg2 != 0) && pSelect[++select_index].iId)
            if (FStrEq(pSelect[select_index].weapon_name, arg1))
               break;
         
         if((arg2 != NULL) && (*arg2 != 0) && pSelect[select_index].iId)
         {
#define CHECK_AND_SET_BOTWEAPON_INT(setting) \
   if (FStrEq(arg2, #setting)) { \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         int temp = atoi(arg3); \
         pSelect[select_index].setting = temp; \
         snprintf(msg, sizeof(msg), "%s for weapon %s is now %i.\n", arg2, arg1, temp); \
      } else \
         snprintf(msg, sizeof(msg), "%s for weapon %s is %i.\n", arg2, arg1, pSelect[select_index].setting); \
   }
#define CHECK_AND_SET_BOTWEAPON_QBOOLEAN(setting) \
   if (FStrEq(arg2, #setting)) { \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         qboolean temp = atoi(arg3) ? TRUE : FALSE; \
         pSelect[select_index].setting = temp; \
         snprintf(msg, sizeof(msg), "%s for weapon %s is now %s.\n", arg2, arg1, temp ? "TRUE" : "FALSE"); \
      } else \
         snprintf(msg, sizeof(msg), "%s for weapon %s is %s.\n", arg2, arg1, pSelect[select_index].setting ? "TRUE" : "FALSE"); \
   }
#define CHECK_AND_SET_BOTWEAPON_FLOAT(setting) \
   if (FStrEq(arg2, #setting)) { \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         float temp = atof(arg3); \
         pSelect[select_index].setting = temp; \
         snprintf(msg, sizeof(msg), "%s for weapon %s is now %f.\n", arg2, arg1, temp); \
      } else \
         snprintf(msg, sizeof(msg), "%s for weapon %s is %f.\n", arg2, arg1, pSelect[select_index].setting); \
   }
                 CHECK_AND_SET_BOTWEAPON_INT(primary_skill_level)   // bot skill must be less than or equal to this value
            else CHECK_AND_SET_BOTWEAPON_INT(secondary_skill_level) // bot skill must be less than or equal to this value
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(avoid_this_gun)  // bot avoids using this weapon if possible
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(prefer_higher_skill_attack) // bot uses better of primary and secondary attacks if both avaible
            else CHECK_AND_SET_BOTWEAPON_FLOAT(primary_min_distance)   // 0 = no minimum
            else CHECK_AND_SET_BOTWEAPON_FLOAT(primary_max_distance)   // 9999 = no maximum
            else CHECK_AND_SET_BOTWEAPON_FLOAT(secondary_min_distance) // 0 = no minimum    
            else CHECK_AND_SET_BOTWEAPON_FLOAT(secondary_max_distance) // 9999 = no maximum
            else CHECK_AND_SET_BOTWEAPON_FLOAT(opt_distance) // optimal distance from target
            else CHECK_AND_SET_BOTWEAPON_INT(use_percent)   // times out of 100 to use this weapon when available
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(can_use_underwater)     // can use this weapon underwater
            else CHECK_AND_SET_BOTWEAPON_INT(primary_fire_percent)   // times out of 100 to use primary fire
            else CHECK_AND_SET_BOTWEAPON_INT(min_primary_ammo)       // minimum ammout of primary ammo needed to fire
            else CHECK_AND_SET_BOTWEAPON_INT(min_secondary_ammo)     // minimum ammout of seconday ammo needed to fire
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(primary_fire_hold)      // hold down primary fire button to use?
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(secondary_fire_hold)    // hold down secondary fire button to use?
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(primary_fire_charge)    // charge weapon using primary fire?
            else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(secondary_fire_charge)  // charge weapon using secondary fire?
            else CHECK_AND_SET_BOTWEAPON_FLOAT(primary_charge_delay)   // time to charge weapon
            else CHECK_AND_SET_BOTWEAPON_FLOAT(secondary_charge_delay) // time to charge weapon
            else CHECK_AND_SET_BOTWEAPON_INT(low_ammo_primary)        // low ammo level
            else CHECK_AND_SET_BOTWEAPON_INT(low_ammo_secondary)      // low ammo level
            //else CHECK_AND_SET_BOTWEAPON_QBOOLEAN(secondary_use_primary_ammo; //does secondary fire use primary ammo?
            else {
              snprintf(msg, sizeof(msg), "unknown weapon setting %s.\n", arg2);
              printfunc(arg, msg);
            }
         }
         else if(FStrEq("list", arg1)) {
            printfunc(arg, "List of available settings:\n");
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "primary_skill_level", "int", "primary", "bot skill must be less than or equal to this value");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "secondary_skill_level", "int", "secondary", "bot skill must be less than or equal to this value");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   %s\n", "avoid_this_gun", "0/1", "bot avoids using this weapon if possible");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   %s\n", "prefer_higher_skill_attack", "0/1", "bot uses higher skill attack of primary and secondary attacks if both available");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "primary_min_distance", "float", "primary", "minimum distance for using this weapon; 0 = no minimum");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "primary_max_distance", "float", "primary", "maximum distance for using this weapon; 9999 = no maximum");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "secondary_min_distance", "float", "secondary", "minimum distance for using this weapon; 0 = no minimum");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "secondary_max_distance", "float", "secondary", "maximum distance for using this weapon; 9999 = no maximum");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   %s\n", "opt_distance", "float", "optimal distance from target");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   %s\n", "use_percent", "int", "times out of 100 to use this weapon when available");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   %s\n", "can_use_underwater", "0/1", "can use this weapon underwater");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   %s\n", "primary_fire_percent", "int", "times out of 100 to use primary fire");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "min_primary_ammo", "int", "primary", "minimum ammouth of ammo needed to fire");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "min_secondary_ammo", "int", "secondary", "minimum ammouth of ammo needed to fire");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "primary_fire_hold", "0/1", "primary", "hold down fire button to use?");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "secondary_fire_hold", "0/1", "secondary", "hold down fire button to use?");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "primary_fire_charge", "0/1", "primary", "charge weapon using fire?");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "secondary_fire_charge", "0/1", "secondary", "charge weapon using fire?");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "primary_charge_delay", "float", "primary", "time to charge weapon");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "secondary_charge_delay", "float", "secondary", "time to charge weapon");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "low_ammo_primary", "int", "primary", "running out of ammo level");
            printfunc(arg, msg);
            snprintf(msg, sizeof(msg), " %s(%s):\n   [%s] %s\n", "low_ammo_secondary", "int", "secondary", "secondary out of ammo level");
            printfunc(arg, msg);
         }
         else if(FStrEq("weapons", arg1)) {
            printfunc(arg, "List of available weapons:\n");
            
            select_index = -1;
            pSelect = &valve_weapon_select[0];

            while (pSelect[++select_index].iId) {
               snprintf(msg, sizeof(msg), "  %s\n", pSelect[select_index].weapon_name);
               printfunc(arg, msg);
            }
         }
         else 
            printfunc(arg, "Could not complete request! (unknown arg1)\n");
      }
      else
         printfunc(arg, "Could not complete request! (arg1 not given)\n");
      
      return TRUE;
   }

   return FALSE;
}


void ClientCommand( edict_t *pEntity )
{
   const char *pcmd = CMD_ARGV (0);
   const char *arg1 = CMD_ARGV (1);
   const char *arg2 = CMD_ARGV (2);
   const char *arg3 = CMD_ARGV (3);
   const char *arg4 = CMD_ARGV (4);
   const char *arg5 = CMD_ARGV (5);

   // only allow custom commands if deathmatch mode and NOT dedicated server and
   // client sending command is the listen server client...

   if ((gpGlobals->deathmatch) && (!IsDedicatedServer) && (pEntity == listenserver_edict))
   {
      if(ProcessCommand(print_to_client, pEntity, pcmd, arg1, arg2, arg3, arg4, arg5, FALSE))
      {
         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "waypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_waypoint_on = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_waypoint_on = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }
         else if (FStrEq(arg1, "add"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointAdd(pEntity);
         }
         else if (FStrEq(arg1, "delete"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointDelete(pEntity);
         }
         else if (FStrEq(arg1, "save"))
         {
            WaypointSave();

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints saved\n");
         }
         else if (FStrEq(arg1, "load"))
         {
            if (WaypointLoad(pEntity))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");
         }
         else if (FStrEq(arg1, "menu"))
         {
            int index;

            if (num_waypoints < 1)
               RETURN_META (MRES_SUPERCEDE);

            index = WaypointFindNearest(pEntity, 50.0);

            if (index == -1)
               RETURN_META (MRES_SUPERCEDE);

            g_menu_waypoint = index;
            g_menu_state = MENU_1;

            UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_1);
         }
         else if (FStrEq(arg1, "info"))
         {
            WaypointPrintInfo(pEntity);
         }
         else if (FStrEq(arg1, "update"))
         {
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "updating waypoint tags...\n");

            WaypointUpdate(pEntity);

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "...update done!  (don't forget to save!)\n");
         }
         else
         {
            if (g_waypoint_on)
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
            else
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "pathwaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_path_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_path_waypoint = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
         }
         else if (FStrEq(arg1, "enable"))
         {
            g_path_waypoint_enable = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ENABLED\n");
         }
         else if (FStrEq(arg1, "disable"))
         {
            g_path_waypoint_enable = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is DISABLED\n");
         }
         else if (FStrEq(arg1, "create1"))
         {
            WaypointCreatePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "create2"))
         {
            WaypointCreatePath(pEntity, 2);
         }
         else if (FStrEq(arg1, "remove1"))
         {
            WaypointRemovePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "remove2"))
         {
            WaypointRemovePath(pEntity, 2);
         }

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "menuselect") && (g_menu_state != MENU_NONE))
      {
         if (g_menu_state == MENU_1)  // main menu...
         {
            if (FStrEq(arg1, "1"))  // team specific...
            {
               g_menu_state = MENU_2;  // display team specific menu...

               UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_2);

               RETURN_META (MRES_SUPERCEDE);
            }
            else if (FStrEq(arg1, "2"))  // wait for lift...
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_LIFT)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_LIFT;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_LIFT;  // on
            }
            else if (FStrEq(arg1, "3"))  // door waypoint
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_DOOR)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_DOOR;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_DOOR;  // on
            }
            else if (FStrEq(arg1, "4"))  // sniper spot
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_SNIPER)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_SNIPER;  // off
               else
               {
                  waypoints[g_menu_waypoint].flags |= W_FL_SNIPER;  // on

                  // set the aiming waypoint...

                  WaypointAddAiming(pEntity);
               }
            }
            else if (FStrEq(arg1, "5"))  // more...
            {
               g_menu_state = MENU_3;

               UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_3);

               RETURN_META (MRES_SUPERCEDE);
            }
         }
         else if (g_menu_state == MENU_2)  // team specific menu
         {
                
         }
         else if (g_menu_state == MENU_3)  // third menu...
         {
            {
               if (FStrEq(arg1, "5"))
               {
                  g_menu_state = MENU_4;

                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_4);

                  RETURN_META (MRES_SUPERCEDE);
               }
            }
         }
         else if (g_menu_state == MENU_4)  // fourth menu...
         {
            if (FStrEq(arg1, "1"))  // health
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_HEALTH)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_HEALTH;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_HEALTH;  // on
            }
            else if (FStrEq(arg1, "2"))  // armor
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_ARMOR)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_ARMOR;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_ARMOR;  // on
            }
            else if (FStrEq(arg1, "3"))  // ammo
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_AMMO)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_AMMO;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_AMMO;  // on
            }
            else if (FStrEq(arg1, "4"))  // jump
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_JUMP)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_JUMP;  // off
               else
               {
                  waypoints[g_menu_waypoint].flags |= W_FL_JUMP;  // on

                  // set the aiming waypoint...

                  WaypointAddAiming(pEntity);
               }
            }
         }

         g_menu_state = MENU_NONE;

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "search"))
      {
         edict_t *pent = NULL;
         float radius = 100;
         char str[80];

         ClientPrint(pEntity, HUD_PRINTNOTIFY, "searching...\n");

         while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
         {
            snprintf(str, sizeof(str), "Found %s at %5.2f %5.2f %5.2f\n",
                       STRING(pent->v.classname),
                       pent->v.origin.x, pent->v.origin.y,
                       pent->v.origin.z);
            ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

            FILE *fp=fopen("jk_botti.txt", "a");
            fprintf(fp, "ClientCommmand: search %s", str);
            fclose(fp);
         }

         RETURN_META (MRES_SUPERCEDE);
      }
#if _DEBUG
      else if (FStrEq(pcmd, "botstop"))
      {
         bot_stop = 1;

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "botstart"))
      {
         bot_stop = 0;

         RETURN_META (MRES_SUPERCEDE);
      }
#endif
   }

   RETURN_META (MRES_IGNORED);
}


static void (*old_PM_PlaySound)(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch);

void new_PM_PlaySound(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch) {
        const float globaltime = gpGlobals->time;
        edict_t * pPlayer;
        int idx;
        
        idx = ENGINE_CURRENT_PLAYER();
        if(idx < 0 || idx >= gpGlobals->maxClients)
                goto _exit;
        
        pPlayer = INDEXENT(idx+1);
        if(!pPlayer || pPlayer->free)
                goto _exit;
        
        SaveSound(pPlayer, globaltime, pPlayer->v.origin, volume, attenuation, 1);
                
_exit:
        (*old_PM_PlaySound)(channel, sample, volume, attenuation, fFlags, pitch);
}


void PM_Move(struct playermove_s *ppmove, qboolean server) {
        //
        if(ppmove && gpGlobals->deathmatch) {
                //hook footstep sound function
                if(ppmove->PM_PlaySound != &new_PM_PlaySound) {
                        old_PM_PlaySound = ppmove->PM_PlaySound;
                        ppmove->PM_PlaySound = &new_PM_PlaySound;
                        
                        RETURN_META (MRES_HANDLED);
                }
        }
        
        RETURN_META (MRES_IGNORED);
}


void CheckSubMod(void)
{
   static int checked = 0;
   
   if(checked > 100)
      return;

   // Check if Severians, XDM or Bubblemod
   const char * desc = MDLL_GetGameDescription();
   
   if(!strnicmp(desc, "Sev", 3))
      submod_id = SUBMOD_SEVS;
   else if(!strnicmp(desc, "XDM", 3))
      submod_id = SUBMOD_XDM;
   else if(CVAR_GET_POINTER("bm_ver") != NULL)
      submod_id = SUBMOD_BUBBLEMOD;
   else
      submod_id = SUBMOD_HLDM;
      
   checked++;
}


void StartFrame( void )
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);

   edict_t *pPlayer;
   static int i, index, bot_index;
   static float previous_time = -1.0;
   char msg[256];
   int count;
   
   const float globaltime = gpGlobals->time;

   CheckSubMod();

   // if a new map has started then (MUST BE FIRST IN StartFrame)...
   if ((globaltime + 0.1) < previous_time)
   {
      char filename[256];
      char mapname[64];

      // check if mapname_bot.cfg file exists...
      strcpy(mapname, STRING(gpGlobals->mapname));
      strcat(mapname, "_jk_botti.cfg");

      UTIL_BuildFileName_N(filename, sizeof(filename), "maps", mapname);

      if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
      {
         snprintf(msg, sizeof(msg), "Executing %s\n", filename);
         ALERT( at_console, "%s", msg );

         for (index = 0; index < 32; index++)
         {
            bots[index].is_used = FALSE;
            bots[index].respawn_state = 0;
            bots[index].f_kick_time = 0.0;
         }

         if (IsDedicatedServer)
            bot_cfg_pause_time = globaltime + 5.0;
         else
            bot_cfg_pause_time = globaltime + 20.0;
      }
      else
      {
         count = 0;

         // mark the bots as needing to be respawned...
         for (index = 0; index < 32; index++)
         {
            if (count >= prev_num_bots)
            {
               bots[index].is_used = FALSE;
               bots[index].respawn_state = 0;
               bots[index].f_kick_time = 0.0;
            }

            if (bots[index].is_used)  // is this slot used?
            {
               bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
               count++;
            }

            // check for any bots that were very recently kicked...
            if ((bots[index].f_kick_time + 5.0) > previous_time)
            {
               bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
               count++;
            }
            else
               bots[index].f_kick_time = 0.0;  // reset to prevent false spawns later
         }

         // set the respawn time
         if (IsDedicatedServer)
            respawn_time = globaltime + 5.0;
         else
            respawn_time = globaltime + 20.0;
      }

      bot_check_time = globaltime + 60.0;
   }

   if (!IsDedicatedServer)
   {
      if ((listenserver_edict != NULL) && (welcome_sent == FALSE) &&
          (welcome_time < 1.0))
      {
         // are they out of observer mode yet?
         if (IsAlive(listenserver_edict))
            welcome_time = globaltime + 5.0;  // welcome in 5 seconds
      }

      if ((welcome_time > 0.0) && (welcome_time < globaltime) &&
          (welcome_sent == FALSE))
      {
         char version[80];

         snprintf(version, sizeof(version), "%s Version %d.%d\n", welcome_msg, VER_MAJOR, VER_MINOR);

         // let's send a welcome message to this client...
         UTIL_SayText(version, listenserver_edict);

         welcome_sent = TRUE;  // clear this so we only do it once
      }
   }
   
   static char skip_think_frame = 0;
   if(!!(skip_think_frame = !skip_think_frame))
   {
      count = 0;
      
      if (globaltime >= gather_data_time) {
         GatherPlayerData();
         gather_data_time = globaltime + (1.0 / 30); //30 fps
      }
      
      if (bot_stop == 0)
      {            
         for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
         {
            if ((bots[bot_index].is_used) &&  // is this slot used AND
               (bots[bot_index].respawn_state == RESPAWN_IDLE))  // not respawning
            {
               if (globaltime >= bots[bot_index].bot_think_time)
               {
                  BotThink(bots[bot_index]);
                  
                  do {
                     bots[bot_index].bot_think_time = globaltime + bot_think_spf * RANDOM_FLOAT2(0.95, 1.05);
                  } while( globaltime + 1.0 < bots[bot_index].bot_think_time );
               }
               
               count++;
            }
         }
      }

      if (count > num_bots)
         num_bots = count;

      // Autowaypointing engine and else, limit to half of botthink rate
      if (globaltime >= waypoint_time)
      {
         int waypoint_player_count = 0;
         int waypoint_player_index = 1;
         
         WaypointAddSpawnObjects();
         
         // 10 times / sec, note: this is extremely slow, do checking only for max 4 players on one frame
         waypoint_time = globaltime + (1.0/10.0);
         
         while(waypoint_player_index <= gpGlobals->maxClients)
         {
            pPlayer = INDEXENT(waypoint_player_index++);

            if (pPlayer && !pPlayer->free)
            {
               // Is player alive?
               if(!IsAlive(pPlayer))
                  continue;
               
               if (FBitSet(pPlayer->v.flags, FL_CLIENT) && !FBitSet(pPlayer->v.flags, FL_FAKECLIENT) && !FBitSet(pPlayer->v.flags, FL_THIRDPARTYBOT))
               {
                  WaypointThink(pPlayer);
                  
                  if(++waypoint_player_count >= 4)
                     break;
               }
            }
         }
      }
   }

   // are we currently respawning bots and is it time to spawn one yet?
   if ((respawn_time > 1.0) && (respawn_time <= globaltime))
   {
      int index = 0;

      // find bot needing to be respawned...
      while ((index < 32) &&
             (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
         index++;

      if (index < 32)
      {
         //int strafe = bot_strafe_percent;  // save global strafe percent
         int chat = bot_chat_percent;    // save global chat percent
         int taunt = bot_taunt_percent;  // save global taunt percent
         int whine = bot_whine_percent;  // save global whine percent
         int logo = bot_logo_percent;    // save global logo percent
         int tag = bot_chat_tag_percent;    // save global clan tag percent
         int drop = bot_chat_drop_percent;  // save global chat drop percent
         int swap = bot_chat_swap_percent;  // save global chat swap percent
         int lower = bot_chat_lower_percent; // save global chat lower percent
         int react = bot_reaction_time;

         bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
         bots[index].is_used = FALSE;      // free up this slot

         //bot_strafe_percent = bots[index].strafe_percent;
         bot_chat_percent = bots[index].chat_percent;
         bot_taunt_percent = bots[index].taunt_percent;
         bot_whine_percent = bots[index].whine_percent;
         bot_logo_percent = bots[index].logo_percent;
         bot_chat_tag_percent = bots[index].chat_tag_percent;
         bot_chat_drop_percent = bots[index].chat_drop_percent;
         bot_chat_swap_percent = bots[index].chat_swap_percent;
         bot_chat_lower_percent = bots[index].chat_lower_percent;
         bot_reaction_time = bots[index].reaction_time;

         // respawn 1 bot then wait a while (otherwise engine crashes)
         {
            char c_skill[2];
            char c_topcolor[4];
            char c_bottomcolor[4];

            snprintf(c_skill, sizeof(c_skill), "%d", bots[index].bot_skill+1);
            snprintf(c_topcolor, sizeof(c_topcolor), "%d", bots[index].top_color);
            snprintf(c_bottomcolor, sizeof(c_bottomcolor), "%d", bots[index].bottom_color);
//UTIL_ServerPrintf("c_skill: %s\n", c_skill);               
            if(randomize_bots_on_mapchange)
               BotCreate(NULL, NULL, NULL, c_skill, NULL, NULL);
            else
               BotCreate(NULL, bots[index].skin, bots[index].name, c_skill, c_topcolor, c_bottomcolor);
         }

         //bot_strafe_percent = strafe;  // restore global strafe percent
         bot_chat_percent = chat;    // restore global chat percent
         bot_taunt_percent = taunt;  // restore global taunt percent
         bot_whine_percent = whine;  // restore global whine percent
         bot_logo_percent = logo;  // restore global logo percent
         bot_chat_tag_percent = tag;    // restore global chat percent
         bot_chat_drop_percent = drop;    // restore global chat percent
         bot_chat_swap_percent = swap;    // restore global chat percent
         bot_chat_lower_percent = lower;    // restore global chat percent
         bot_reaction_time = react;

         respawn_time = globaltime + 2.0;  // set next respawn time

         bot_check_time = globaltime + 5.0;
      }
      else
      {
         respawn_time = 0.0;
      }
   }

   if (g_GameRules)
   {
      if (need_to_open_cfg)  // have we open jk_botti.cfg file yet?
      {
         char filename[256];
         char mapname[64];

         need_to_open_cfg = FALSE;  // only do this once!!!

         // check if mapname_jk_botti.cfg file exists...

         strcpy(mapname, STRING(gpGlobals->mapname));
         strcat(mapname, "_jk_botti.cfg");

         UTIL_BuildFileName_N(filename, sizeof(filename), "maps", mapname);

         if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
         {
            snprintf(msg, sizeof(msg), "Executing %s\n", filename);
            ALERT( at_console, "%s", msg );
         }
         else
         {
            UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/jk_botti.cfg", NULL);

            snprintf(msg, sizeof(msg), "Executing %s\n", filename);
            ALERT( at_console, "%s", msg );

            bot_cfg_fp = fopen(filename, "r");

            if (bot_cfg_fp == NULL)
               ALERT( at_console, "%s", "jk_botti.cfg file not found\n" );
         }

         if (IsDedicatedServer)
            bot_cfg_pause_time = globaltime + 5.0;
         else
            bot_cfg_pause_time = globaltime + 20.0;
      }

      if (!IsDedicatedServer && !spawn_time_reset)
      {
         if (listenserver_edict != NULL)
         {
            if (IsAlive(listenserver_edict))
            {
               spawn_time_reset = TRUE;

               if (respawn_time >= 1.0)
                  respawn_time = min(respawn_time, globaltime + (float)1.0);

               if (bot_cfg_pause_time >= 1.0)
                  bot_cfg_pause_time = min(bot_cfg_pause_time, globaltime + (float)1.0);
            }
         }
      }

      if ((bot_cfg_fp) &&
          (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= globaltime))
      {
         // process jk_botti.cfg file options...
         ProcessBotCfgFile();
      }

   }      

   // check if time to see if a bot needs to be created...
   if (bot_check_time < globaltime)
   {
      count = 0;

      bot_check_time = globaltime + 5.0;

      for (i = 0; i < 32; i++)
      {
         if (clients[i] != NULL)
            count++;
      }

      // if there are currently less than the maximum number of "players"
      // then add another bot using the default skill level...
      if ((count < max_bots) && (max_bots != -1))
      {
         BotCreate( NULL, NULL, NULL, NULL, NULL, NULL );
      }
   }

   previous_time = globaltime;

   RETURN_META (MRES_HANDLED);
}


void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3)
{
   int length;

   g_argv[0] = 0;
   g_arg1[0] = 0;
   g_arg2[0] = 0;
   g_arg3[0] = 0;

   isFakeClientCommand = TRUE;

   if ((arg1 == NULL) || (*arg1 == 0))
      return;

   if ((arg2 == NULL) || (*arg2 == 0))
   {
      length = snprintf(g_argv, sizeof(g_argv), "%s", arg1);
      fake_arg_count = 1;
   }
   else if ((arg3 == NULL) || (*arg3 == 0))
   {
      length = snprintf(g_argv, sizeof(g_argv), "%s %s", arg1, arg2);
      fake_arg_count = 2;
   }
   else
   {
      length = snprintf(g_argv, sizeof(g_argv), "%s %s %s", arg1, arg2, arg3);
      fake_arg_count = 3;
   }

   g_argv[length] = 0;  // null terminate just in case

   g_arg1[snprintf(g_arg1, sizeof(g_arg1), "%s", arg1)] = 0;
   
   if (arg2)
      g_arg2[snprintf(g_arg2, sizeof(g_arg2), "%s", arg2)] = 0;
   
   if (arg3)
      g_arg3[snprintf(g_arg3, sizeof(g_arg3), "%s", arg3)] = 0;

   // allow the MOD DLL to execute the ClientCommand...
   MDLL_ClientCommand(pBot);

   isFakeClientCommand = FALSE;
}


static void print_to_null(void *, char *) 
{
}


void ProcessBotCfgFile(void)
{
   const float globaltime = gpGlobals->time;
   int ch;
   char cmd_line[256];
   int cmd_index;
   static char server_cmd[80];
   char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5;
   char msg[80];

   if (bot_cfg_pause_time > globaltime)
      return;

   if (bot_cfg_fp == NULL)
      return;

   cmd_index = 0;
   cmd_line[cmd_index] = 0;

   ch = fgetc(bot_cfg_fp);

   // skip any leading blanks
   while (ch == ' ')
      ch = fgetc(bot_cfg_fp);

   while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
   {
      if (ch == '\t')  // convert tabs to spaces
         ch = ' ';

      cmd_line[cmd_index] = ch;

      ch = fgetc(bot_cfg_fp);

      // skip multiple spaces in input file
      while ((cmd_line[cmd_index] == ' ') &&
             (ch == ' '))      
         ch = fgetc(bot_cfg_fp);

      cmd_index++;
   }

   if (ch == '\r')  // is it a carriage return?
   {
      ch = fgetc(bot_cfg_fp);  // skip the linefeed
   }

   // if reached end of file, then close it
   if (ch == EOF)
   {
      fclose(bot_cfg_fp);

      bot_cfg_fp = NULL;

      bot_cfg_pause_time = 0.0;
   }

   cmd_line[cmd_index] = 0;  // terminate the command line

   // copy the command line to a server command buffer...
   strcpy(server_cmd, cmd_line);
   strcat(server_cmd, "\n");

   cmd_index = 0;
   cmd = cmd_line;
   arg1 = arg2 = arg3 = arg4 = arg5 = NULL;

   // skip to blank or end of string...
   while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
      cmd_index++;

   if (cmd_line[cmd_index] == ' ')
   {
      cmd_line[cmd_index++] = 0;
      arg1 = &cmd_line[cmd_index];

      // skip to blank or end of string...
      while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
         cmd_index++;

      if (cmd_line[cmd_index] == ' ')
      {
         cmd_line[cmd_index++] = 0;
         arg2 = &cmd_line[cmd_index];

         // skip to blank or end of string...
         while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
            cmd_index++;

         if (cmd_line[cmd_index] == ' ')
         {
            cmd_line[cmd_index++] = 0;
            arg3 = &cmd_line[cmd_index];

            // skip to blank or end of string...
            while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
               cmd_index++;

            if (cmd_line[cmd_index] == ' ')
            {
               cmd_line[cmd_index++] = 0;
               arg4 = &cmd_line[cmd_index];

               // skip to blank or end of string...
               while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
                  cmd_index++;

               if (cmd_line[cmd_index] == ' ')
               {
                  cmd_line[cmd_index++] = 0;
                  arg5 = &cmd_line[cmd_index];
               }
            }
         }
      }
   }

   if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
      return;  // return if comment or blank line

   if(arg1 && !strcmp(arg1, "\"\""))
      *arg1=0;
   if(arg2 && !strcmp(arg2, "\"\""))
      *arg2=0;
   if(arg3 && !strcmp(arg3, "\"\""))
      *arg3=0;
   if(arg4 && !strcmp(arg4, "\"\""))
      *arg4=0;
   if(arg5 && !strcmp(arg5, "\"\""))
      *arg5=0;

   if(ProcessCommand(print_to_null, NULL, cmd, arg1, arg2, arg3, arg4, arg5, TRUE))
      return;
   
   snprintf(msg, sizeof(msg), "executing server command: %s\n", server_cmd);
   ALERT( at_console, "%s", msg );

   if (IsDedicatedServer)
      UTIL_ServerPrintf(msg);

   SERVER_COMMAND(server_cmd);
}


void ServerDeactivate(void)
{
   if(!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   // automatic saving in autowaypoint mode, only if there are changes!
   if(g_auto_waypoint && g_waypoint_updated)
      WaypointSave();
   
   RETURN_META (MRES_HANDLED);
}


static void print_to_server_output(void *, char * msg) 
{
   SERVER_PRINT(msg);
}


void jk_botti_ServerCommand (void)
{
   if(!ProcessCommand(print_to_server_output, NULL, CMD_ARGV (1), CMD_ARGV (2), CMD_ARGV (3), CMD_ARGV (4), CMD_ARGV (5), CMD_ARGV (6), FALSE)) {
      SERVER_PRINT(CMD_ARGV (1)); SERVER_PRINT(": Unknown command \'"); SERVER_PRINT(CMD_ARGV (2)); SERVER_PRINT("\'\n");
   }
}


C_DLLEXPORT int GetEntityAPI2 (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
   gFunctionTable.pfnGameInit = GameDLLInit;
   gFunctionTable.pfnSpawn = Spawn;
   gFunctionTable.pfnClientConnect = ClientConnect;
   gFunctionTable.pfnClientDisconnect = ClientDisconnect;
   gFunctionTable.pfnClientPutInServer = ClientPutInServer;
   gFunctionTable.pfnClientCommand = ClientCommand;
   gFunctionTable.pfnStartFrame = StartFrame;
   gFunctionTable.pfnServerDeactivate = ServerDeactivate;

   memcpy (pFunctionTable, &gFunctionTable, sizeof (DLL_FUNCTIONS));
   return (TRUE);
}
