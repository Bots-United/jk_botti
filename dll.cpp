//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
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

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"

#define VER_MAJOR 4
#define VER_MINOR 0


#define MENU_NONE  0
#define MENU_1     1
#define MENU_2     2
#define MENU_3     3
#define MENU_4     4


extern DLL_FUNCTIONS gFunctionTable;
extern enginefuncs_t g_engfuncs;
extern globalvars_t  *gpGlobals;
extern char g_argv[1024];
extern bool g_waypoint_on;
extern bool g_auto_waypoint;
extern bool g_path_waypoint;
extern bool g_path_waypoint_enable;
extern int num_waypoints;  // number of waypoints currently in use
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];
extern bot_t bots[32];
extern bool b_observer_mode;
extern bool b_botdontshoot;
extern char welcome_msg[80];

static FILE *fp;

int mod_id = 0;
int m_spriteTexture = 0;
int default_bot_skill = 2;
int bot_strafe_percent = 20; // percent of time to strafe
int bot_chat_percent = 10;   // percent of time to chat
int bot_taunt_percent = 20;  // percent of time to taunt after kill
int bot_whine_percent = 10;  // percent of time to whine after death
int bot_grenade_time = 15;   // seconds between grenade throws
int bot_logo_percent = 40;   // percent of time to spray logo after kill

int bot_chat_tag_percent = 80;   // percent of the time to drop clan tag
int bot_chat_drop_percent = 10;  // percent of the time to drop characters
int bot_chat_swap_percent = 10;  // percent of the time to swap characters
int bot_chat_lower_percent = 50; // percent of the time to lowercase chat

bool b_random_color = TRUE;
bool isFakeClientCommand = FALSE;
int fake_arg_count;
int IsDedicatedServer;
float bot_check_time = 60.0;
int bot_reaction_time = 2;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = FALSE;
edict_t *clients[32];
edict_t *listenserver_edict = NULL;
float welcome_time = 0.0;
bool welcome_sent = FALSE;
int g_menu_waypoint;
int g_menu_state = 0;
int bot_stop = 0;
int jumppad_off = 0;

bool is_team_play = FALSE;
char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int num_teams = 0;
bool checked_teamplay = FALSE;
edict_t *pent_info_tfdetect = NULL;
edict_t *pent_info_ctfdetect = NULL;
edict_t *pent_info_frontline = NULL;
edict_t *pent_item_tfgoal = NULL;
edict_t *pent_info_tfgoal = NULL;
int max_team_players[4];
int team_class_limits[4];
int team_allies[4];  // bit mapped allies BLUE, RED, YELLOW, and GREEN
int max_teams = 0;
int num_flags = 0;
FLAG_S flags[MAX_FLAGS];
int num_backpacks = 0;
BACKPACK_S backpacks[MAX_BACKPACKS];

FILE *bot_cfg_fp = NULL;
bool need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
float respawn_time = 0.0;
bool spawn_time_reset = FALSE;

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
void HPB_Bot_ServerCommand (void);

extern void welcome_init(void);



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

plugin_info_t Plugin_info = {
   META_INTERFACE_VERSION, // interface version
   "HPB_Bot", // plugin name
   "4.0.4", // plugin version
   "09/11/2004", // date of creation
   "botman && Pierre-Marie Baty", // plugin author
   "http://hpb-bot.bots-united.com/", // plugin URL
   "HPB_BOT", // plugin logtag
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
   REG_SVR_COMMAND ("HPB_Bot", HPB_Bot_ServerCommand);

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

   return (TRUE); // returning TRUE enables metamod to unload this plugin
}

// END of Metamod stuff



void GameDLLInit( void )
{
   int i;

   IsDedicatedServer = IS_DEDICATED_SERVER();

   for (i=0; i<32; i++)
      clients[i] = NULL;

   welcome_init();

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
   int index;

   if (gpGlobals->deathmatch)
   {
      char *pClassname = (char *)STRING(pent->v.classname);

      if (strcmp(pClassname, "worldspawn") == 0)
      {
         // do level initialization stuff here...

         WaypointInit();
         WaypointLoad(NULL);

         pent_info_tfdetect = NULL;
         pent_info_ctfdetect = NULL;
         pent_info_frontline = NULL;
         pent_item_tfgoal = NULL;
         pent_info_tfgoal = NULL;

         for (index=0; index < 4; index++)
         {
            max_team_players[index] = 0;  // no player limit
            team_class_limits[index] = 0;  // no class limits
            team_allies[index] = 0;
         }

         max_teams = 0;
         num_flags = 0;

         for (index=0; index < MAX_FLAGS; index++)
         {
            flags[index].edict = NULL;
            flags[index].team_no = 0;  // any team unless specified
         }

         num_backpacks = 0;

         for (index=0; index < MAX_BACKPACKS; index++)
         {
            backpacks[index].edict = NULL;
            backpacks[index].armor = 0;
            backpacks[index].health = 0;
            backpacks[index].ammo = 0;
            backpacks[index].team = 0;  // any team unless specified
         }

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
         memset(team_names, 0, sizeof(team_names));
         num_teams = 0;
         checked_teamplay = FALSE;

         bot_cfg_pause_time = 0.0;
         respawn_time = 0.0;
         spawn_time_reset = FALSE;

         prev_num_bots = num_bots;
         num_bots = 0;

         bot_check_time = gpGlobals->time + 60.0;
      }

      if ((mod_id == HOLYWARS_DLL) && (jumppad_off) &&
          (strcmp(pClassname, "trigger_jumppad") == 0))
      {
         RETURN_META_VALUE (MRES_SUPERCEDE, -1);  // disable jumppads
      }
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}

void KeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
   static edict_t *temp_pent;
   static int flag_index;
   static int backpack_index;

   if (mod_id == TFC_DLL)
   {
      if (pentKeyvalue == pent_info_tfdetect)
      {
         if (strcmp(pkvd->szKeyName, "ammo_medikit") == 0)  // max BLUE players
            max_team_players[0] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "ammo_detpack") == 0)  // max RED players
            max_team_players[1] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_medikit") == 0)  // max YELLOW players
            max_team_players[2] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_detpack") == 0)  // max GREEN players
            max_team_players[3] = atoi(pkvd->szValue);

         else if (strcmp(pkvd->szKeyName, "maxammo_shells") == 0)  // BLUE class limits
            team_class_limits[0] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_nails") == 0)  // RED class limits
            team_class_limits[1] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_rockets") == 0)  // YELLOW class limits
            team_class_limits[2] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "maxammo_cells") == 0)  // GREEN class limits
            team_class_limits[3] = atoi(pkvd->szValue);

         else if (strcmp(pkvd->szKeyName, "team1_allies") == 0)  // BLUE allies
            team_allies[0] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "team2_allies") == 0)  // RED allies
            team_allies[1] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "team3_allies") == 0)  // YELLOW allies
            team_allies[2] = atoi(pkvd->szValue);
         else if (strcmp(pkvd->szKeyName, "team4_allies") == 0)  // GREEN allies
            team_allies[3] = atoi(pkvd->szValue);
      }
      else if (pent_info_tfdetect == NULL)
      {
         if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
             (strcmp(pkvd->szValue, "info_tfdetect") == 0))
         {
            pent_info_tfdetect = pentKeyvalue;
         }
      }

      if (pentKeyvalue == pent_item_tfgoal)
      {
         if (strcmp(pkvd->szKeyName, "team_no") == 0)
            flags[flag_index].team_no = atoi(pkvd->szValue);

         if ((strcmp(pkvd->szKeyName, "mdl") == 0) &&
             ((strcmp(pkvd->szValue, "models/flag.mdl") == 0) ||
              (strcmp(pkvd->szValue, "models/keycard.mdl") == 0) ||
              (strcmp(pkvd->szValue, "models/ball.mdl") == 0)))
         {
            num_flags++;
         }
      }
      else if (pent_item_tfgoal == NULL)
      {
         if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
             (strcmp(pkvd->szValue, "item_tfgoal") == 0))
         {
            if (num_flags < MAX_FLAGS)
            {
               pent_item_tfgoal = pentKeyvalue;

               flags[num_flags].edict = pentKeyvalue;

               flag_index = num_flags;  // in case the mdl comes before team_no
            }
         }
      }
      else
      {
         pent_item_tfgoal = NULL;  // reset for non-flag item_tfgoal's
      }


      if (pentKeyvalue != pent_info_tfgoal)  // different edict?
      {
         pent_info_tfgoal = NULL;  // reset
      }

      if (pentKeyvalue == pent_info_tfgoal)
      {
         if (strcmp(pkvd->szKeyName, "team_no") == 0)
            backpacks[backpack_index].team = atoi(pkvd->szValue);

         if (strcmp(pkvd->szKeyName, "armorvalue") == 0)
            backpacks[backpack_index].armor = atoi(pkvd->szValue);

         if (strcmp(pkvd->szKeyName, "health") == 0)
            backpacks[backpack_index].health = atoi(pkvd->szValue);

         if ((strcmp(pkvd->szKeyName, "ammo_nails") == 0) ||
             (strcmp(pkvd->szKeyName, "ammo_rockets") == 0) ||
             (strcmp(pkvd->szKeyName, "ammo_cells") == 0) ||
             (strcmp(pkvd->szKeyName, "ammo_shells") == 0))
            backpacks[backpack_index].ammo = atoi(pkvd->szValue);

         if ((strcmp(pkvd->szKeyName, "mdl") == 0) &&
             (strcmp(pkvd->szValue, "models/backpack.mdl") == 0))
         {
            num_backpacks++;
         }
      }
      else if (pent_info_tfgoal == NULL)
      {
         if (((strcmp(pkvd->szKeyName, "classname") == 0) &&
              (strcmp(pkvd->szValue, "info_tfgoal") == 0)) ||
             ((strcmp(pkvd->szKeyName, "classname") == 0) &&
              (strcmp(pkvd->szValue, "i_t_g") == 0)))
         {
            if (num_backpacks < MAX_BACKPACKS)
            {
               pent_info_tfgoal = pentKeyvalue;

               backpacks[num_backpacks].edict = pentKeyvalue;

               // in case the mdl comes before the other fields
               backpack_index = num_backpacks;
            }
         }
      }

      if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
          ((strcmp(pkvd->szValue, "info_player_teamspawn") == 0) ||
           (strcmp(pkvd->szValue, "i_p_t") == 0)))
      {
         temp_pent = pentKeyvalue;
      }
      else if (pentKeyvalue == temp_pent)
      {
         if (strcmp(pkvd->szKeyName, "team_no") == 0)
         {
            int value = atoi(pkvd->szValue);

            if (value > max_teams)
               max_teams = value;
         }
      }
   }
   else if (mod_id == GEARBOX_DLL)
   {
      if (pent_info_ctfdetect == NULL)
      {
         if ((strcmp(pkvd->szKeyName, "classname") == 0) &&
             (strcmp(pkvd->szValue, "info_ctfdetect") == 0))
         {
            pent_info_ctfdetect = pentKeyvalue;
         }
      }
   }

   RETURN_META (MRES_IGNORED);
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

                  sprintf(cmd, "kick \"%s\"\n", bots[i].name);

                  SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

                  break;
               }
            }
         }
      }
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

   RETURN_META (MRES_IGNORED);
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

   if ((gpGlobals->deathmatch) && (!IsDedicatedServer) &&
       (pEntity == listenserver_edict))
   {
      char msg[80];

      if (FStrEq(pcmd, "addbot"))
      {
         BotCreate( pEntity, arg1, arg2, arg3, arg4, arg5 );

         bot_check_time = gpGlobals->time + 5.0;

         RETURN_META (MRES_SUPERCEDE);
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
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "botskill"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 1) || (temp > 5))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid botskill value!\n");
            else
               default_bot_skill = temp;
         }

         sprintf(msg, "botskill is %d\n", default_bot_skill);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_strafe_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_strafe_percent value!\n");
            else
               bot_strafe_percent = temp;
         }

         sprintf(msg, "bot_strafe_percent is %d\n", bot_strafe_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_chat_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_percent value!\n");
            else
               bot_chat_percent = temp;
         }

         sprintf(msg, "bot_chat_percent is %d\n", bot_chat_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_taunt_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_taunt_percent value!\n");
            else
               bot_taunt_percent = temp;
         }

         sprintf(msg, "bot_taunt_percent is %d\n", bot_taunt_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_whine_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_whine_percent value!\n");
            else
               bot_whine_percent = temp;
         }

         sprintf(msg, "bot_whine_percent is %d\n", bot_whine_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_chat_tag_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_tag_percent value!\n");
            else
               bot_chat_tag_percent = temp;
         }

         sprintf(msg, "bot_chat_tag_percent is %d\n", bot_chat_tag_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_chat_drop_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_drop_percent value!\n");
            else
               bot_chat_drop_percent = temp;
         }

         sprintf(msg, "bot_chat_drop_percent is %d\n", bot_chat_drop_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_chat_swap_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_swap_percent value!\n");
            else
               bot_chat_swap_percent = temp;
         }

         sprintf(msg, "bot_chat_swap_percent is %d\n", bot_chat_swap_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_chat_lower_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_chat_lower_percent value!\n");
            else
               bot_chat_lower_percent = temp;
         }

         sprintf(msg, "bot_chat_lower_percent is %d\n", bot_chat_lower_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_grenade_time"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 60))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_grenade_time value!\n");
            else
               bot_grenade_time = temp;
         }

         sprintf(msg, "bot_grenade_time is %d\n", bot_grenade_time);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_logo_percent"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_logo_percent value!\n");
            else
               bot_logo_percent = temp;
         }

         sprintf(msg, "bot_logo_percent is %d\n", bot_logo_percent);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "bot_reaction_time"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 0) || (temp > 3))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid bot_reaction_time value!\n");
            else
               bot_reaction_time = temp;
         }

         if (bot_reaction_time)
            sprintf(msg, "bot_reaction_time is %d\n", bot_reaction_time);
         else
            sprintf(msg, "bot_reaction_time is DISABLED\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         RETURN_META (MRES_SUPERCEDE);
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
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "random_color ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "random_color DISABLED\n");

         RETURN_META (MRES_SUPERCEDE);
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
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot DISABLED\n");

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

            index = WaypointFindNearest(pEntity, 50.0, -1);

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
      else if (FStrEq(pcmd, "autowaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_auto_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case
         }
         else if (FStrEq(arg1, "off"))
         {
            g_auto_waypoint = FALSE;
         }

         if (g_auto_waypoint)
            sprintf(msg, "autowaypoint is ON\n");
         else
            sprintf(msg, "autowaypoint is OFF\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

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

               if (mod_id == FRONTLINE_DLL)
                  UTIL_ShowMenu(pEntity, 0x13, -1, FALSE, show_menu_2_flf);
               else
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

               if (mod_id == FRONTLINE_DLL)
                  UTIL_ShowMenu(pEntity, 0x17, -1, FALSE, show_menu_3_flf);
               else if (mod_id == HOLYWARS_DLL)
                  UTIL_ShowMenu(pEntity, 0x11, -1, FALSE, show_menu_3_hw);
               else
                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_3);

               RETURN_META (MRES_SUPERCEDE);
            }
         }
         else if (g_menu_state == MENU_2)  // team specific menu
         {
            if (waypoints[g_menu_waypoint].flags & W_FL_TEAM_SPECIFIC)
            {
               waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM;
               waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM_SPECIFIC; // off
            }
            else
            {
               int team = atoi(arg1);

               team--;  // make 0 to 3

               // this is kind of a kludge (team bits MUST be LSB!!!)
               waypoints[g_menu_waypoint].flags |= team;
               waypoints[g_menu_waypoint].flags |= W_FL_TEAM_SPECIFIC; // on
            }
         }
         else if (g_menu_state == MENU_3)  // third menu...
         {
            if (mod_id == FRONTLINE_DLL)
            {
               if (FStrEq(arg1, "1"))  // capture point
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLF_CAP)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLF_CAP;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLF_CAP;  // on
               }
               else if (FStrEq(arg1, "2"))  // defend point
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLF_DEFEND)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLF_DEFEND;  // off
                  else
                  {
                     waypoints[g_menu_waypoint].flags |= W_FL_FLF_DEFEND;  // on

                     // set the aiming waypoint...

                     WaypointAddAiming(pEntity);
                  }
               }
               else if (FStrEq(arg1, "3"))  // go prone
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_PRONE)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_PRONE;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_PRONE;  // on
               }
            }
            else if (mod_id == HOLYWARS_DLL)
            {
               if (FStrEq(arg1, "1"))  // flag location
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLAG)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLAG;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLAG;  // on
               }
               else if (FStrEq(arg1, "5"))
               {
                  g_menu_state = MENU_4;

                  UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_4);

                  RETURN_META (MRES_SUPERCEDE);
               }
            }
            else
            {
               if (FStrEq(arg1, "1"))  // flag location
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLAG)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLAG;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLAG;  // on
               }
               else if (FStrEq(arg1, "2"))  // flag goal
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_FLAG_GOAL)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_FLAG_GOAL;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_FLAG_GOAL;  // on
               }
               else if (FStrEq(arg1, "3"))  // sentry gun
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_SENTRYGUN)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_SENTRYGUN;  // off
                  else
                  {
                     waypoints[g_menu_waypoint].flags |= W_FL_SENTRYGUN;  // on

                     // set the aiming waypoint...

                     WaypointAddAiming(pEntity);
                  }
               }
               else if (FStrEq(arg1, "4"))  // dispenser
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_DISPENSER)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_DISPENSER;  // off
                  else
                  {
                     waypoints[g_menu_waypoint].flags |= W_FL_DISPENSER;  // on

                     // set the aiming waypoint...

                     WaypointAddAiming(pEntity);
                  }
               }
               else if (FStrEq(arg1, "5"))
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

         // turn off the text menu if the mod doesn't do this automatically
         if ((mod_id == HOLYWARS_DLL) || (mod_id == DMC_DLL))
            UTIL_ShowMenu(pEntity, 0x0, -1, FALSE, show_menu_none);

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
            sprintf(str, "Found %s at %5.2f %5.2f %5.2f\n",
                       STRING(pent->v.classname),
                       pent->v.origin.x, pent->v.origin.y,
                       pent->v.origin.z);
            ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

            FILE *fp=fopen("HPB_bot.txt", "a");
            fprintf(fp, "ClientCommmand: search %s", str);
            fclose(fp);
         }

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "jumppad"))
      {
         char str[80];

         if (FStrEq(arg1, "off"))
            jumppad_off = 1;
         if (FStrEq(arg1, "on"))
            jumppad_off = 0;

         if (jumppad_off)
            sprintf(str,"jumppad is OFF\n");
         else
            sprintf(str,"jumppad is ON\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

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

void StartFrame( void )
{
   if (gpGlobals->deathmatch)
   {
      edict_t *pPlayer;
      static int i, index, player_index, bot_index;
      static float previous_time = -1.0;
      char msg[256];
      int count;

      // if a new map has started then (MUST BE FIRST IN StartFrame)...
      if ((gpGlobals->time + 0.1) < previous_time)
      {
         char filename[256];
         char mapname[64];

         // check if mapname_bot.cfg file exists...

         strcpy(mapname, STRING(gpGlobals->mapname));
         strcat(mapname, "_HPB_bot.cfg");

         UTIL_BuildFileName(filename, "maps", mapname);

         if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
         {
            sprintf(msg, "Executing %s\n", filename);
            ALERT( at_console, msg );

            for (index = 0; index < 32; index++)
            {
               bots[index].is_used = FALSE;
               bots[index].respawn_state = 0;
               bots[index].f_kick_time = 0.0;
            }

            if (IsDedicatedServer)
               bot_cfg_pause_time = gpGlobals->time + 5.0;
            else
               bot_cfg_pause_time = gpGlobals->time + 20.0;
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
               respawn_time = gpGlobals->time + 5.0;
            else
               respawn_time = gpGlobals->time + 20.0;
         }

         bot_check_time = gpGlobals->time + 60.0;
      }

      if (!IsDedicatedServer)
      {
         if ((listenserver_edict != NULL) && (welcome_sent == FALSE) &&
             (welcome_time < 1.0))
         {
            // are they out of observer mode yet?
            if (IsAlive(listenserver_edict))
               welcome_time = gpGlobals->time + 5.0;  // welcome in 5 seconds
         }

         if ((welcome_time > 0.0) && (welcome_time < gpGlobals->time) &&
             (welcome_sent == FALSE))
         {
            char version[80];

            sprintf(version,"%s Version %d.%d\n", welcome_msg, VER_MAJOR, VER_MINOR);

            // let's send a welcome message to this client...
            UTIL_SayText(version, listenserver_edict);

            welcome_sent = TRUE;  // clear this so we only do it once
         }
      }

      count = 0;

      if (bot_stop == 0)
      {
         for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
         {
            if ((bots[bot_index].is_used) &&  // is this slot used AND
                (bots[bot_index].respawn_state == RESPAWN_IDLE))  // not respawning
            {
               BotThink(&bots[bot_index]);

               count++;
            }
         }
      }

      if (count > num_bots)
         num_bots = count;

      for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
      {
         pPlayer = INDEXENT(player_index);

         if (pPlayer && !pPlayer->free)
         {
            if ((g_waypoint_on) &&
                FBitSet(pPlayer->v.flags, FL_CLIENT) && !FBitSet(pPlayer->v.flags, FL_FAKECLIENT))
            {
                  WaypointThink(pPlayer);
            }
         }
      }

      // are we currently respawning bots and is it time to spawn one yet?
      if ((respawn_time > 1.0) && (respawn_time <= gpGlobals->time))
      {
         int index = 0;

         // find bot needing to be respawned...
         while ((index < 32) &&
                (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
            index++;

         if (index < 32)
         {
            int strafe = bot_strafe_percent;  // save global strafe percent
            int chat = bot_chat_percent;    // save global chat percent
            int taunt = bot_taunt_percent;  // save global taunt percent
            int whine = bot_whine_percent;  // save global whine percent
            int grenade = bot_grenade_time; // save global grenade time
            int logo = bot_logo_percent;    // save global logo percent
            int tag = bot_chat_tag_percent;    // save global clan tag percent
            int drop = bot_chat_drop_percent;  // save global chat drop percent
            int swap = bot_chat_swap_percent;  // save global chat swap percent
            int lower = bot_chat_lower_percent; // save global chat lower percent
            int react = bot_reaction_time;

            bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
            bots[index].is_used = FALSE;      // free up this slot

            bot_strafe_percent = bots[index].strafe_percent;
            bot_chat_percent = bots[index].chat_percent;
            bot_taunt_percent = bots[index].taunt_percent;
            bot_whine_percent = bots[index].whine_percent;
            bot_grenade_time = bots[index].grenade_time;
            bot_logo_percent = bots[index].logo_percent;
            bot_chat_tag_percent = bots[index].chat_tag_percent;
            bot_chat_drop_percent = bots[index].chat_drop_percent;
            bot_chat_swap_percent = bots[index].chat_swap_percent;
            bot_chat_lower_percent = bots[index].chat_lower_percent;
            bot_reaction_time = bots[index].reaction_time;

            // respawn 1 bot then wait a while (otherwise engine crashes)
            if ((mod_id == VALVE_DLL) ||
                ((mod_id == GEARBOX_DLL) && (pent_info_ctfdetect == NULL)) ||
                (mod_id == HOLYWARS_DLL) || (mod_id == DMC_DLL))
            {
               char c_skill[2];
               char c_topcolor[4];
               char c_bottomcolor[4];

               sprintf(c_skill, "%d", bots[index].bot_skill);
               sprintf(c_topcolor, "%d", bots[index].top_color);
               sprintf(c_bottomcolor, "%d", bots[index].bottom_color);
               
               BotCreate(NULL, bots[index].skin, bots[index].name, c_skill, c_topcolor, c_bottomcolor);
            }
            else
            {
               char c_skill[2];
               char c_team[2];
               char c_class[3];

               sprintf(c_skill, "%d", bots[index].bot_skill);
               sprintf(c_team, "%d", bots[index].bot_team);
               sprintf(c_class, "%d", bots[index].bot_class);

               if ((mod_id == TFC_DLL) || (mod_id == GEARBOX_DLL))
                  BotCreate(NULL, NULL, NULL, bots[index].name, c_skill, NULL);
               else
                  BotCreate(NULL, c_team, c_class, bots[index].name, c_skill, NULL);
            }

            bot_strafe_percent = strafe;  // restore global strafe percent
            bot_chat_percent = chat;    // restore global chat percent
            bot_taunt_percent = taunt;  // restore global taunt percent
            bot_whine_percent = whine;  // restore global whine percent
            bot_grenade_time = grenade;  // restore global grenade time
            bot_logo_percent = logo;  // restore global logo percent
            bot_chat_tag_percent = tag;    // restore global chat percent
            bot_chat_drop_percent = drop;    // restore global chat percent
            bot_chat_swap_percent = swap;    // restore global chat percent
            bot_chat_lower_percent = lower;    // restore global chat percent
            bot_reaction_time = react;

            respawn_time = gpGlobals->time + 2.0;  // set next respawn time

            bot_check_time = gpGlobals->time + 5.0;
         }
         else
         {
            respawn_time = 0.0;
         }
      }

      if (g_GameRules)
      {
         if (need_to_open_cfg)  // have we open HPB_bot.cfg file yet?
         {
            char filename[256];
            char mapname[64];

            need_to_open_cfg = FALSE;  // only do this once!!!

            // check if mapname_HPB_bot.cfg file exists...

            strcpy(mapname, STRING(gpGlobals->mapname));
            strcat(mapname, "_HPB_bot.cfg");

            UTIL_BuildFileName(filename, "maps", mapname);

            if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
            {
               sprintf(msg, "Executing %s\n", filename);
               ALERT( at_console, msg );
            }
            else
            {
               UTIL_BuildFileName(filename, "HPB_bot.cfg", NULL);

               sprintf(msg, "Executing %s\n", filename);
               ALERT( at_console, msg );

               bot_cfg_fp = fopen(filename, "r");

               if (bot_cfg_fp == NULL)
                  ALERT( at_console, "HPB_bot.cfg file not found\n" );
            }

            if (IsDedicatedServer)
               bot_cfg_pause_time = gpGlobals->time + 5.0;
            else
               bot_cfg_pause_time = gpGlobals->time + 20.0;
         }

         if (!IsDedicatedServer && !spawn_time_reset)
         {
            if (listenserver_edict != NULL)
            {
               if (IsAlive(listenserver_edict))
               {
                  spawn_time_reset = TRUE;

                  if (respawn_time >= 1.0)
                     respawn_time = min(respawn_time, gpGlobals->time + (float)1.0);

                  if (bot_cfg_pause_time >= 1.0)
                     bot_cfg_pause_time = min(bot_cfg_pause_time, gpGlobals->time + (float)1.0);
               }
            }
         }

         if ((bot_cfg_fp) &&
             (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time))
         {
            // process HPB_bot.cfg file options...
            ProcessBotCfgFile();
         }

      }      

      // check if time to see if a bot needs to be created...
      if (bot_check_time < gpGlobals->time)
      {
         int count = 0;

         bot_check_time = gpGlobals->time + 5.0;

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

      previous_time = gpGlobals->time;
   }

   RETURN_META (MRES_IGNORED);
}

void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3)
{
   int length;

   memset(g_argv, 0, 1024);

   isFakeClientCommand = TRUE;

   if ((arg1 == NULL) || (*arg1 == 0))
      return;

   if ((arg2 == NULL) || (*arg2 == 0))
   {
      length = sprintf(&g_argv[0], "%s", arg1);
      fake_arg_count = 1;
   }
   else if ((arg3 == NULL) || (*arg3 == 0))
   {
      length = sprintf(&g_argv[0], "%s %s", arg1, arg2);
      fake_arg_count = 2;
   }
   else
   {
      length = sprintf(&g_argv[0], "%s %s %s", arg1, arg2, arg3);
      fake_arg_count = 3;
   }

   g_argv[length] = 0;  // null terminate just in case

   strcpy(&g_argv[64], arg1);

   if (arg2)
      strcpy(&g_argv[128], arg2);

   if (arg3)
      strcpy(&g_argv[192], arg3);

   // allow the MOD DLL to execute the ClientCommand...
   MDLL_ClientCommand(pBot);

   isFakeClientCommand = FALSE;
}


void ProcessBotCfgFile(void)
{
   int ch;
   char cmd_line[256];
   int cmd_index;
   static char server_cmd[80];
   char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5;
   char msg[80];

   if (bot_cfg_pause_time > gpGlobals->time)
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

   if (strcmp(cmd, "addbot") == 0)
   {
      BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );

      // have to delay here or engine gives "Tried to write to
      // uninitialized sizebuf_t" error and crashes...

      bot_cfg_pause_time = gpGlobals->time + 2.0;
      bot_check_time = gpGlobals->time + 5.0;

      return;
   }

   if (strcmp(cmd, "botskill") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 1) && (temp <= 5))
         default_bot_skill = atoi( arg1 );  // set default bot skill level

      return;
   }

   if (strcmp(cmd, "random_color") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_random_color = TRUE;
      else
         b_random_color = FALSE;

      return;
   }

   if (strcmp(cmd, "bot_strafe_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_strafe_percent = atoi( arg1 );  // set bot strafe percent

      return;
   }

   if (strcmp(cmd, "bot_chat_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_taunt_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_taunt_percent = atoi( arg1 );  // set bot taunt percent

      return;
   }

   if (strcmp(cmd, "bot_whine_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_whine_percent = atoi( arg1 );  // set bot whine percent

      return;
   }

   if (strcmp(cmd, "bot_chat_tag_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_tag_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_chat_drop_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_drop_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_chat_swap_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_swap_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_chat_lower_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_chat_lower_percent = atoi( arg1 );  // set bot chat percent

      return;
   }

   if (strcmp(cmd, "bot_grenade_time") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 60))
         bot_grenade_time = atoi( arg1 );  // set bot grenade time

      return;
   }

   if (strcmp(cmd, "bot_logo_percent") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 100))
         bot_logo_percent = atoi( arg1 );  // set bot strafe percent

      return;
   }

   if (strcmp(cmd, "bot_reaction_time") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 0) && (temp <= 3))
         bot_reaction_time = atoi( arg1 );  // set bot reaction time

      return;
   }

   if (strcmp(cmd, "observer") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_observer_mode = TRUE;
      else
         b_observer_mode = FALSE;

      return;
   }

   if (strcmp(cmd, "botdontshoot") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_botdontshoot = TRUE;
      else
         b_botdontshoot = FALSE;

      return;
   }

   if (strcmp(cmd, "min_bots") == 0)
   {
      min_bots = atoi( arg1 );

      if ((min_bots < 0) || (min_bots > 31))
         min_bots = 1;

      if (IsDedicatedServer)
      {
         sprintf(msg, "min_bots set to %d\n", min_bots);
         printf(msg);
      }

      return;
   }

   if (strcmp(cmd, "max_bots") == 0)
   {
      max_bots = atoi( arg1 );

      if ((max_bots < 0) || (max_bots > 31)) 
         max_bots = 1;

      if (IsDedicatedServer)
      {
         sprintf(msg, "max_bots set to %d\n", max_bots);
         printf(msg);
      }

      return;
   }

   if (strcmp(cmd, "pause") == 0)
   {
      bot_cfg_pause_time = gpGlobals->time + atoi( arg1 );

      return;
   }

   sprintf(msg, "executing server command: %s\n", server_cmd);
   ALERT( at_console, msg );

   if (IsDedicatedServer)
      printf(msg);

   SERVER_COMMAND(server_cmd);
}


void HPB_Bot_ServerCommand (void)
{
   char msg[128];

   if (strcmp(CMD_ARGV (1), "addbot") == 0)
   {
      BotCreate( NULL, CMD_ARGV (2), CMD_ARGV (3), CMD_ARGV (4), CMD_ARGV (5), CMD_ARGV (6) );

      bot_check_time = gpGlobals->time + 5.0;
   }
   else if (strcmp(CMD_ARGV (1), "min_bots") == 0)
   {
      min_bots = atoi( CMD_ARGV (2) );

      if ((min_bots < 0) || (min_bots > 31))
         min_bots = 1;

      sprintf(msg, "min_bots set to %d\n", min_bots);
      SERVER_PRINT(msg);
   }
   else if (strcmp(CMD_ARGV (1), "max_bots") == 0)
   {
      max_bots = atoi( CMD_ARGV (2) );

      if ((max_bots < 0) || (max_bots > 31)) 
         max_bots = 1;

      sprintf(msg, "max_bots set to %d\n", max_bots);
      SERVER_PRINT(msg);
   }
}


C_DLLEXPORT int GetEntityAPI2 (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
   gFunctionTable.pfnGameInit = GameDLLInit;
   gFunctionTable.pfnSpawn = Spawn;
   gFunctionTable.pfnKeyValue = KeyValue;
   gFunctionTable.pfnClientConnect = ClientConnect;
   gFunctionTable.pfnClientDisconnect = ClientDisconnect;
   gFunctionTable.pfnClientPutInServer = ClientPutInServer;
   gFunctionTable.pfnClientCommand = ClientCommand;
   gFunctionTable.pfnStartFrame = StartFrame;

   memcpy (pFunctionTable, &gFunctionTable, sizeof (DLL_FUNCTIONS));
   return (TRUE);
}
