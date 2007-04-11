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
#include "bot_skill.h"
#include "bot_weapon_select.h"

#define VER_MAJOR 0
#define VER_MINOR 44
#define VER_NOTE "beta"

extern DLL_FUNCTIONS gFunctionTable;
extern DLL_FUNCTIONS gFunctionTable_POST;
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
extern qboolean g_in_intermission;
extern BOOL wp_matrix_save_on_mapend;

int submod_id = SUBMOD_HLDM;

int m_spriteTexture = 0;
int default_bot_skill = 2;
int bot_add_level_tag = 0;   // use [lvl%d] for bots (where %d is skill level of bot)
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
float bot_check_time = 60.0;
int bot_reaction_time = 2;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
edict_t *clients[32];
edict_t *listenserver_edict = NULL;
float welcome_time = 0.0;
qboolean welcome_sent = FALSE;
int bot_stop = 0;
int frame_count = 0;
int randomize_bots_on_mapchange = 0;

qboolean is_team_play = FALSE;
qboolean checked_teamplay = FALSE;

FILE *bot_cfg_fp = NULL;
qboolean need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
qboolean spawn_time_reset = FALSE;
float waypoint_time = 0.0;

unsigned int rnd_idnum[2] = {1, 1};
cvar_t jk_botti_version = { "jk_botti_version", "", FCVAR_EXTDLL|FCVAR_SERVER, 0, NULL};


void BotNameInit(void);
void BotLogoInit(void);
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
void ProcessBotCfgFile(void);
void jk_botti_ServerCommand (void);
void CheckSubMod(void);



// START of Metamod stuff
C_DLLEXPORT int GetEntityAPI2_POST (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);

enginefuncs_t meta_engfuncs;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;
meta_globals_t *gpMetaGlobals;

META_FUNCTIONS gMetaFunctionTable =
{
   NULL, // pfnGetEntityAPI()
   NULL, // pfnGetEntityAPI_Post()
   GetEntityAPI2, // pfnGetEntityAPI2()
   GetEntityAPI2_POST, // pfnGetEntityAPI2_Post()
   NULL, // pfnGetNewDLLFunctions()
   NULL, // pfnGetNewDLLFunctions_Post()
   GetEngineFunctions, // pfnGetEngineFunctions()
   NULL, // pfnGetEngineFunctions_Post()
};


static char plugver[128];
plugin_info_t Plugin_info = {
   META_INTERFACE_VERSION, // interface version
   "JK_Botti", // plugin name
   "", // plugin version
   __DATE__, // date of creation
   "Jussi Kivilinna", // plugin author
   "http://forums.bots-united.com/forumdisplay.php?f=83", // plugin URL
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

   safevoid_snprintf(plugver, sizeof(plugver), "%d.%d%s", VER_MAJOR, VER_MINOR, VER_NOTE);
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
   
   // init random
   fast_random_seed(time(0) ^ (unsigned long)&bots[0] ^ sizeof(bots));
  
   CVAR_REGISTER(&jk_botti_version);
   CVAR_SET_STRING("jk_botti_version", Plugin_info.version);
      
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

         safevoid_snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bots[index].name);

         SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
      }
   }
   
   // free memory
   WaypointInit();
   for(int i = 0; i < 32; i++)
      free_posdata_list(i);
   UTIL_FreeFuncBreakables();
   
   return (TRUE); // returning TRUE enables metamod to unload this plugin
}

// END of Metamod stuff


void GameDLLInit( void )
{
   memset(clients, 0, sizeof(clients));

   // initialize the bots array of structures...
   memset(bots, 0, sizeof(bots));

   //skill init
   ResetSkillsToDefault();

   //weapon select init
   CheckSubMod();
   InitWeaponSelect(submod_id);

   BotNameInit();
   BotLogoInit();

   LoadBotChat();
   LoadBotModels();

   RETURN_META (MRES_IGNORED);
}


void CheckSubMod(void)
{
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
   
   switch(submod_id)
   {
   case SUBMOD_SEVS:
      UTIL_ConsolePrintf("Severians MOD detected.");
      break;
   case SUBMOD_XDM:
      UTIL_ConsolePrintf("Xtreme Deathmatch MOD detected.");
      break;
   case SUBMOD_BUBBLEMOD:
      UTIL_ConsolePrintf("BubbleMod MOD detected.");
      break;
   case SUBMOD_HLDM:
      UTIL_ConsolePrintf("Standard HL1DM assumed.");
      break;
   }
}


int Spawn( edict_t *pent )
{
   if (gpGlobals->deathmatch)
   {
      if (FIsClassname(pent, "worldspawn"))
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
         PRECACHE_MODEL( "models/w_chainammo.mdl");

         g_in_intermission = FALSE;

         is_team_play = FALSE;
         checked_teamplay = FALSE;

         frame_count = 0;

         bot_cfg_pause_time = 0.0;
         waypoint_time = -1.0;
         spawn_time_reset = FALSE;

         for(int i = 0; i < 32; i++)
            bots[i].is_used = 0;
         
         num_bots = 0;
         need_to_open_cfg = TRUE;

         bot_check_time = gpGlobals->time + 5.0;
      }
      else
         CollectMapSpawnItems(pent);
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


int Spawn_Post( edict_t *pent )
{
   if (gpGlobals->deathmatch)
   {
      if (FIsClassname(pent, "worldspawn"))
      {
         // check for team play... gamedll check it in 'worldspawn' spawn and doesn't recheck
         BotCheckTeamplay();
      }
   }
   
   RETURN_META_VALUE (MRES_IGNORED, 0);
}


void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
   if(!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   if(pkvd && pkvd->szKeyName && pkvd->szValue)
      if(FIsClassname(pentKeyvalue, "func_breakable") || (FIsClassname(pentKeyvalue, "func_pushable") && pentKeyvalue->v.spawnflags & SF_PUSH_BREAKABLE))
         UTIL_UpdateFuncBreakable(pentKeyvalue, pkvd->szKeyName, pkvd->szValue);
   
   RETURN_META (MRES_HANDLED);
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
         bot_check_time = gpGlobals->time + 5.0;

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

                  safevoid_snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bots[i].name);

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


void ServerDeactivate(void)
{
   if(!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   // automatic saving in autowaypoint mode, only if there are changes!
   if(wp_matrix_save_on_mapend)
   {
      WaypointSaveFloydsMatrix();
      wp_matrix_save_on_mapend = FALSE;
   }
   if(g_auto_waypoint && g_waypoint_updated)
      WaypointSave();
   
   UTIL_FreeFuncBreakables();
   
   RETURN_META (MRES_HANDLED);
}


void PlayerPostThink( edict_t *pEdict )
{
   if (!gpGlobals->deathmatch) 
      RETURN_META(MRES_IGNORED);
   
   if(!FNullEnt(pEdict))
   {
      //check if player is facing wall
      CheckPlayerChatProtection(pEdict);
      
      //Gather player positions for ping prediction
      GatherPlayerData(pEdict);
   }
   
   RETURN_META (MRES_HANDLED);
}


static void (*old_PM_PlaySound)(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch);

void new_PM_PlaySound(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch) 
{
   edict_t * pPlayer;
   int idx;
   
   idx = ENGINE_CURRENT_PLAYER();
   if(idx < 0 || idx >= gpGlobals->maxClients)
      goto _exit;
   
   pPlayer = INDEXENT(idx+1);
   if(!pPlayer || pPlayer->free || FBitSet(pPlayer->v.flags, FL_PROXY))
      goto _exit;
   
   SaveSound(pPlayer, gpGlobals->time, pPlayer->v.origin, volume, attenuation, 1);
      
_exit:
   (*old_PM_PlaySound)(channel, sample, volume, attenuation, fFlags, pitch);
}


void PM_Move(struct playermove_s *ppmove, qboolean server) 
{
   //
   if(ppmove && gpGlobals->deathmatch) 
   {
      //hook footstep sound function
      if(ppmove->PM_PlaySound != &new_PM_PlaySound) 
      {
         old_PM_PlaySound = ppmove->PM_PlaySound;
         ppmove->PM_PlaySound = &new_PM_PlaySound;
         
         RETURN_META (MRES_HANDLED);
      }
   }
   
   RETURN_META (MRES_IGNORED);
}


void StartFrame( void )
{
   edict_t *pPlayer;
   int i, bot_index;
   int count;
   double begin_time;
   
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   begin_time = UTIL_GetSecs();
   
   // Don't allow BotThink and WaypointThink run on same frame,
   // because this would KILL bot aim (for reasons still unknown)
   if(frame_count++ & 1)
   {
      if (bot_stop == 0)
      {            
         count = 0;
         
         for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
         {
            if (bots[bot_index].is_used)   // is this slot used 
            {
               if (gpGlobals->time >= bots[bot_index].bot_think_time)
               {
                  BotThink(bots[bot_index]);
               
                  bots[bot_index].bot_think_time = gpGlobals->time + bot_think_spf * RANDOM_FLOAT2(0.95, 1.05);
               }
            
               count++;
            }
         }
         
         if (count > num_bots)
            num_bots = count;
      }
   }
   else
   {
      // Autowaypointing engine and else, limit to half of botthink rate
      if (gpGlobals->time >= waypoint_time)
      {
         int waypoint_player_count = 0;
         int waypoint_player_index = 1;
      
         WaypointAddSpawnObjects();
      
         // 10 times / sec, note: this is extremely slow, do checking only for max 4 players on one frame
         waypoint_time = gpGlobals->time + (1.0/10.0);
      
         while(waypoint_player_index <= gpGlobals->maxClients)
         {
            pPlayer = INDEXENT(waypoint_player_index++);

            if (pPlayer && !pPlayer->free && !FBitSet(pPlayer->v.flags, FL_PROXY))
            {
               // Is player alive?
               if(!IsAlive(pPlayer))
                  continue;
            
               if (FBitSet(pPlayer->v.flags, FL_CLIENT) && !FBitSet(pPlayer->v.flags, FL_PROXY) && 
                  !(FBitSet(pPlayer->v.flags, FL_FAKECLIENT) || FBitSet(pPlayer->v.flags, FL_THIRDPARTYBOT)))
               {
                  WaypointThink(pPlayer);
               
                  if(++waypoint_player_count >= 2)
                     break;
               }
            }
         }
      }
   }

   if (need_to_open_cfg)  // have we open jk_botti.cfg file yet?
   {
      char filename[256];
      char mapname[64];

      need_to_open_cfg = FALSE;  // only do this once!!!

      // check if mapname_jk_botti.cfg file exists...

      strcpy(mapname, STRING(gpGlobals->mapname));
      strcat(mapname, "_jk_botti.cfg");

      UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti", mapname);

      if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
      {
         UTIL_ConsolePrintf("Executing %s\n", filename);
      }
      else
      {
         UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/jk_botti.cfg", NULL);

         UTIL_ConsolePrintf("Executing %s\n", filename);

         bot_cfg_fp = fopen(filename, "r");

         if (bot_cfg_fp == NULL)
         {
            UTIL_ConsolePrintf("jk_botti.cfg file not found\n" );
         }
      }

      bot_cfg_pause_time = gpGlobals->time + 5.0;
   }

   if ((bot_cfg_fp) &&
       (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time))
   {
      // process jk_botti.cfg file options...
      ProcessBotCfgFile();
   }

   // check if time to see if a bot needs to be created...
   if (bot_check_time < gpGlobals->time)
   {
      count = 0;

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
   
   // -- Run Floyds for creating waypoint path matrix
   //
   //    keep going for 10ms (hlds sleeps 10ms after every frame on win32, thus 10ms+10ms = 20ms -> 50fps
   if(WaypointSlowFloydsState() != -1)
   {
      count=0;
      while(begin_time + 0.010 >= UTIL_GetSecs())
      {
         count++;
         if(WaypointSlowFloyds() == -1)
            break;
      }
   }
   
   RETURN_META (MRES_HANDLED);
}


extern void ClientCommand( edict_t *pEntity );

C_DLLEXPORT int GetEntityAPI2 (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
   gFunctionTable.pfnGameInit = GameDLLInit;
   gFunctionTable.pfnSpawn = Spawn;
   gFunctionTable.pfnKeyValue = DispatchKeyValue;
   gFunctionTable.pfnClientConnect = ClientConnect;
   gFunctionTable.pfnClientDisconnect = ClientDisconnect;
   gFunctionTable.pfnClientPutInServer = ClientPutInServer;
   gFunctionTable.pfnClientCommand = ClientCommand;
   gFunctionTable.pfnStartFrame = StartFrame;
   gFunctionTable.pfnServerDeactivate = ServerDeactivate;
   gFunctionTable.pfnPlayerPostThink = PlayerPostThink;

   memcpy (pFunctionTable, &gFunctionTable, sizeof (DLL_FUNCTIONS));
   return (TRUE);
}

C_DLLEXPORT int GetEntityAPI2_POST (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
   gFunctionTable_POST.pfnSpawn = Spawn_Post;
   
   memcpy (pFunctionTable, &gFunctionTable_POST, sizeof (DLL_FUNCTIONS));
   return (TRUE);
}
