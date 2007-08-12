//
// JK_Botti - be more human!
//
// dll.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <malloc.h>

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
#include "bot_sound.h"
#include "version.h"

#include "bot_query_hook.h"

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
extern float last_time_not_facing_wall[32];
extern float last_time_dead[32];

extern char g_team_list[TEAMPLAY_TEAMLISTLENGTH];
extern char g_team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
extern int g_team_scores[MAX_TEAMS];
extern int g_num_teams;
extern qboolean g_team_limit;

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

int team_balancetype = 1;
char *team_blockedlist;

float bot_think_spf = 1.0/30.0; // == 1 / (30 fps)

qboolean b_random_color = TRUE;
float bot_check_time = 60.0;
int bot_reaction_time = 2;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
edict_t *clients[32];
char *client_addresses[32];
edict_t *listenserver_edict = NULL;
float welcome_time = 0.0;
qboolean welcome_sent = FALSE;
int bot_stop = 0;
int frame_count = 0;
int randomize_bots_on_mapchange = 0;
int debug_minmax = 0;

qboolean is_team_play = FALSE;
qboolean checked_teamplay = FALSE;

FILE *bot_cfg_fp = NULL;
int bot_cfg_linenumber = 0;
qboolean need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
qboolean spawn_time_reset = FALSE;
float waypoint_time = 0.0;
char * client_address[32];

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
C_DLLEXPORT int GetEngineFunctions_POST (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion);

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
   GetEngineFunctions_POST, // pfnGetEngineFunctions_Post()
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

   safevoid_snprintf(plugver, sizeof(plugver), "%d.%02d%s", VER_MAJOR, VER_MINOR, VER_NOTE);
   Plugin_info.version = plugver;

   // check for interface version compatibility
   if (strcmp (ifvers, Plugin_info.ifvers) != 0)
   {
      int mmajor = 0, mminor = 0, pmajor = 0, pminor = 0;

      //LOG_CONSOLE (PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);
      //LOG_MESSAGE (PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);

      // if plugin has later interface version, it's incompatible (update metamod)
      sscanf (ifvers, "%d:%d", &mmajor, &mminor);
      sscanf (META_INTERFACE_VERSION, "%d:%d", &pmajor, &pminor);

      if ((pmajor > mmajor) || ((pmajor == mmajor) && (pminor > mminor)))
      {
         LOG_CONSOLE (PLID, "%s", "metamod version is too old for this plugin; update metamod");
         LOG_ERROR (PLID, "%s", "metamod version is too old for this plugin; update metamod");
         return (FALSE);
      }

      // if plugin has older major interface version, it's incompatible (update plugin)
      else if (pmajor < mmajor)
      {
         LOG_CONSOLE (PLID, "%s", "metamod version is incompatible with this plugin; please find a newer version of this plugin");
         LOG_ERROR (PLID, "%s", "metamod version is incompatible with this plugin; please find a newer version of this plugin");
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
      if (bots[index].is_used)  // is this slot used?
         BotKick(bots[index]);
   
   // free memory
   WaypointInit();
   for(int i = 0; i < 32; i++)
      free_posdata_list(i);
   UTIL_FreeFuncBreakables();
   FreeCfgBotRecord();
   
   // try remove hook
   if(!unhook_sendto_function())
      return(FALSE);
   
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
   else if(!strnicmp(desc, "HL Teamplay", 11))
   {
      // this is a bit of hack, sevs uses "HL Teamplay" string for teamplay so we need alternative way of detecting
      // Ofcourse all other submods with same problem will be detected as sevs now too, oh well.. got to live with it :S
      if(CVAR_GET_POINTER("mp_giveweapons") != NULL && CVAR_GET_POINTER("mp_giveammo") != NULL)
         submod_id = SUBMOD_SEVS;
      else
         submod_id = SUBMOD_HLDM;
   }
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
   default:
      submod_id = SUBMOD_HLDM;
   case SUBMOD_HLDM:
      UTIL_ConsolePrintf("Standard HL1DM assumed.");
      break;
   }
}


static float m_height = 0;
static float m_lip = 0;
static Vector m_origin;


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
         
         // init sound system
         *pSoundEnt = CSoundEnt();
         pSoundEnt->Spawn();
         
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

         *g_team_list = 0;
         memset(g_team_names, 0, sizeof(g_team_names));
         memset(g_team_scores, 0, sizeof(g_team_scores));
         g_num_teams = 0;
         g_team_limit = FALSE;

         frame_count = 0;

         bot_cfg_pause_time = 0.0;
         waypoint_time = -1.0;
         spawn_time_reset = FALSE;

         for(int i = 0; i < 32; i++)
            bots[i].is_used = FALSE;
         
         num_bots = 0;
         need_to_open_cfg = TRUE;
         FreeCfgBotRecord();//reset on mapchange

         bot_check_time = gpGlobals->time + 5.0;
         
         memset(last_time_not_facing_wall, 0, sizeof(last_time_not_facing_wall));
         memset(last_time_dead, 0, sizeof(last_time_dead));
      }
      else if(FIsClassname(pent, "func_plat") || FIsClassname(pent, "func_door"))
      {
         m_origin = pent->v.origin;
      }
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


int Spawn_Post( edict_t *pent )
{
   if(!gpGlobals->deathmatch)
      RETURN_META_VALUE (MRES_IGNORED, 0);
   
   if (FIsClassname(pent, "worldspawn"))
   {
      // check for team play... gamedll check it in 'worldspawn' spawn and doesn't recheck
      BotCheckTeamplay();
   }
   else if(FIsClassname(pent, "func_plat"))
   {
      if(m_height == 0)
         m_height = pent->v.size.z + 8;
      
      Vector v_position1 = m_origin;
      Vector v_position2 = m_origin;
      v_position2.z -= m_height;
      
      Vector start, end;
      if(!FStringNull(pent->v.targetname))
      {
         start = v_position1;
         end = v_position2;
      }
      else
      {
         start = v_position2;
         end = v_position1;
      }
      
      WaypointAddLift(pent, start, end);
   }
   else if(FIsClassname(pent, "func_door"))
   {
      const int SF_DOOR_START_OPEN = 1;
      
      Vector v_position1 = m_origin;
      // Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
      Vector v_position2 = m_origin + (pent->v.movedir * (fabs( pent->v.movedir.x * (pent->v.size.x-2) ) + fabs( pent->v.movedir.y * (pent->v.size.y-2) ) + fabs( pent->v.movedir.z * (pent->v.size.z-2) ) - m_lip));

      if ( FBitSet (pent->v.spawnflags, SF_DOOR_START_OPEN) )
      {
         Vector swap = v_position1;
         v_position1 = v_position2;
         v_position2 = swap;
      }
      
      Vector start = v_position1;
      Vector end = v_position2;
      
      WaypointAddLift(pent, start, end);
   }
   else
      CollectMapSpawnItems(pent);
   
   m_height = 0;
   m_lip = 0;
   
   RETURN_META_VALUE (MRES_IGNORED, 0);
}


void DispatchKeyValue_Post( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
   if(!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   if(pkvd && pkvd->szKeyName && pkvd->szValue)
   {
      if(FIsClassname(pentKeyvalue, "func_breakable") && !(pentKeyvalue->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
      	 UTIL_UpdateFuncBreakable(pentKeyvalue, pkvd->szKeyName, pkvd->szValue);
      
      else if(FIsClassname(pentKeyvalue, "func_pushable") && pentKeyvalue->v.spawnflags & SF_PUSH_BREAKABLE && !(pentKeyvalue->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
         UTIL_UpdateFuncBreakable(pentKeyvalue, pkvd->szKeyName, pkvd->szValue);
      
      else if(FIsClassname(pentKeyvalue, "func_plat"))
      {
         // save height?
         if(FStrEq(pkvd->szKeyName, "height"))
            m_height = atof(pkvd->szValue);
      }
      else if(FIsClassname(pentKeyvalue, "func_door"))
      {
         // save lip?
         if (FStrEq(pkvd->szKeyName, "lip"))
            m_lip = atof(pkvd->szValue);
      }
   }
   
   RETURN_META (MRES_HANDLED);
}


BOOL jkbotti_ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{ 
   if (!gpGlobals->deathmatch)
      RETURN_META_VALUE (MRES_IGNORED, 0);

   // check if this client is the listen server client
   if (strcmp(pszAddress, "loopback") == 0)
   {
      // save the edict of the listen server client...
      listenserver_edict = pEntity;
   }
   else if(strcmp(pszAddress, "::::local:jk_botti") == 0)
   {
      // don't try to add bots for 1 second, give client time to get added
      bot_check_time = gpGlobals->time + 1.0;
   }
   else if(strcmp(pszAddress, "::::local:other_bot") == 0)
   {
      // don't try to add bots for 1 second, give client time to get added
      bot_check_time = gpGlobals->time + 1.0;
   }
   
   RETURN_META_VALUE (MRES_IGNORED, 0);
}


void jkbotti_ClientPutInServer( edict_t *pEntity )
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   int idx = ENTINDEX(pEntity) - 1;

   if (idx < gpGlobals->maxClients && idx >= 0)
      clients[idx] = pEntity;  // store this clients edict in the clients array
   else
   {
      UTIL_ConsolePrintf("Error! ClientPutInServer pEntity invalid!");
      RETURN_META (MRES_IGNORED);
   }
   
   ResetSound(pEntity);

   RETURN_META (MRES_IGNORED);
}


void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
   // check if is our bot
   int bot_index = UTIL_GetBotIndex(player);
   
   if(bot_index != -1)
   {
      if(bots[bot_index].name[0] == 0)  // name filled in yet?
         safevoid_snprintf(bots[bot_index].name, sizeof(bots[bot_index].name), "%s", STRING(bots[bot_index].pEdict->v.netname));
      
      if(bots[bot_index].userid <= 0)  // user id filled in yet?
         bots[bot_index].userid = GETPLAYERUSERID(bots[bot_index].pEdict);
      
      JKASSERT(bots[bot_index].name[0] == 0);
      JKASSERT(bots[bot_index].userid <= 0);
      
      RETURN_META (MRES_IGNORED);
   }
	
   // This is to detect other third party bots
   
   // check if connected
   for(int i = 0; i < gpGlobals->maxClients; i++)
   {
      if(player == clients[i])
         RETURN_META (MRES_IGNORED);
   }
   
   char szRejectReason[ 128 ];
   
   memset(szRejectReason, 0, sizeof(szRejectReason));
   
   jkbotti_ClientConnect((edict_t*)player, STRING(player->v.netname), "::::local:other_bot", szRejectReason);
   jkbotti_ClientPutInServer( (edict_t*)player );
   
   RETURN_META (MRES_IGNORED);
}


void ClientDisconnect( edict_t *pEntity )
{
   if (gpGlobals->deathmatch)
   {
      int i;
      int idx = ENTINDEX(pEntity) - 1;

      if (idx < gpGlobals->maxClients && idx >= 0)
         clients[idx] = NULL;

      for (i=0; i < 32; i++)
      {
         if (bots[i].pEdict == pEntity)
         {
            // someone kicked this bot off of the server...

            bots[i].is_used = FALSE;  // this slot is now free to use
            bots[i].f_kick_time = gpGlobals->time;  // save the kicked time
            bots[i].userid = 0;

            break;
         }
      }
      
      ResetSound(pEntity);
   }

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


void PlayerPostThink_Post( edict_t *pEdict )
{
   if (!gpGlobals->deathmatch) 
      RETURN_META(MRES_IGNORED);
   
   //check if player is facing wall
   CheckPlayerChatProtection(pEdict);
      
   //Gather player positions for ping prediction
   GatherPlayerData(pEdict);
      
   //Sound update
   UpdatePlayerSound(pEdict);
   
   //Store check IsAlive
   SaveAliveStatus(pEdict);
   
   RETURN_META (MRES_HANDLED);
}


static void (*old_PM_PlaySound)(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch);

void new_PM_PlaySound(int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch) 
{
   if (gpGlobals->deathmatch)
   {
      edict_t * pPlayer;
      int idx;
   
      idx = ENGINE_CURRENT_PLAYER();
   
      if(idx >= 0 && idx < gpGlobals->maxClients)
      {
         pPlayer = INDEXENT(idx+1);
   
         if(pPlayer && !pPlayer->free)
         {
            int ivolume = (int)(1000*volume);
            SaveSound(pPlayer, pPlayer->v.origin, ivolume, CHAN_BODY);
         }
      }
   }
   
   (*old_PM_PlaySound)(channel, sample, volume, attenuation, fFlags, pitch);
}

void PM_Move(struct playermove_s *ppmove, qboolean server) 
{
   if (!gpGlobals->deathmatch) 
      RETURN_META(MRES_IGNORED);
      
   //
   if(ppmove) 
   {
      //hook footstep sound function
      if(ppmove->PM_PlaySound != &new_PM_PlaySound) 
      {
         old_PM_PlaySound = ppmove->PM_PlaySound;
         ppmove->PM_PlaySound = &new_PM_PlaySound;
      }
   }
   
   RETURN_META (MRES_HANDLED);
}

void StartFrame( void )
{
   edict_t *pPlayer;
   int bot_index;
   int count;
   double begin_time;
   
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   begin_time = UTIL_GetSecs();
   
   // sound system
   if (pSoundEnt->m_nextthink <= gpGlobals->time)
      pSoundEnt->Think();
   
   // bot system
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
                  
               // randomness prevents all bots to run on same frame -> less laggy server
               bots[bot_index].bot_think_time = gpGlobals->time + bot_think_spf * RANDOM_FLOAT2(0.95, 1.05);
            }

            count++;
         }
      }
         
      if (count > num_bots)
         num_bots = count;
   }
   
   // Autowaypointing engine
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
   
   if (need_to_open_cfg)  // have we open jk_botti.cfg file yet?
   {
      char filename[256];
      char mapname[64];

      need_to_open_cfg = FALSE;  // only do this once!!!

      // check if jk_botti_mapname.cfg file exists...
      if(stricmp(STRING(gpGlobals->mapname), "logo") == 0)
         safevoid_snprintf(mapname, sizeof(mapname), "%s", "_jk_botti_logo.cfg");
      else
         safevoid_snprintf(mapname, sizeof(mapname), "jk_botti_%s.cfg", STRING(gpGlobals->mapname));
      
      UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti", mapname);

      if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
      {
         UTIL_ConsolePrintf("Executing %s\n", filename);
         bot_cfg_linenumber = 0;
      }
      else
      {
         UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/jk_botti.cfg", NULL);

         if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
         {
            UTIL_ConsolePrintf("Executing %s\n", filename);
            bot_cfg_linenumber = 0;
         }
         else
         {
            UTIL_ConsolePrintf("jk_botti.cfg file not found\n" );
         }
      }

      bot_cfg_pause_time = gpGlobals->time + 0.1;
      bot_check_time = gpGlobals->time + 1.0;
   }

   if ((bot_cfg_fp) &&
       (bot_cfg_pause_time > 0.0f) && (bot_cfg_pause_time <= gpGlobals->time))
   {
      // process jk_botti.cfg file options...
      ProcessBotCfgFile();
      
      bot_check_time = gpGlobals->time + 1.0;
   }

   // check if time to see if a bot needs to be created...
   if (bot_check_time < gpGlobals->time && min_bots != -1 && max_bots != -1 && min_bots <= max_bots)
   {
      int client_count = UTIL_GetClientCount();
      
      if(debug_minmax)
      {
         UTIL_ConsolePrintf("client count: %d, bot count: %d, max_bots: %d, min_bots: %d\n", 
            client_count, UTIL_GetBotCount(), max_bots, min_bots);
         
         UTIL_ConsolePrintf("client_count < max_bots: %s", client_count < max_bots ? "TRUE":"FALSE");
         UTIL_ConsolePrintf("client_count > max_bots: %s", client_count > max_bots ? "TRUE":"FALSE");
         UTIL_ConsolePrintf("bot_count > min_bots: %s", UTIL_GetBotCount() > min_bots ? "TRUE":"FALSE");
         
         UTIL_ConsolePrintf("Should add bot: %s", client_count < max_bots ? "TRUE":"FALSE");
         UTIL_ConsolePrintf("Should remove bot: %s", (client_count > max_bots && UTIL_GetBotCount() > min_bots) ? "TRUE":"FALSE");
         
         if(client_count > max_bots && UTIL_GetBotCount() > min_bots)
            UTIL_ConsolePrintf("Test UTIL_PickRandomBot(), return value: %d", UTIL_PickRandomBot());
      }
      
      // need more clients
      if(client_count < max_bots)
      {
         const cfg_bot_record_t * record = GetUnusedCfgBotRecord();
         
         if(record)
            BotCreate( record->skin, record->name, record->skill, record->top_color, record->bottom_color, record->index );
         else
            BotCreate( NULL, NULL, -1, -1, -1, -1 );
         
         bot_check_time = gpGlobals->time + 0.5;
      }
      // more than minimum count of bots and need to lower client count
      else if(client_count > max_bots && UTIL_GetBotCount() > min_bots)
      {
         int pick = UTIL_PickRandomBot();
         
         if(is_team_play && team_balancetype >= 1 && g_team_limit)
         {
            char balanceskin[MAX_TEAMNAME_LENGTH];

            // get largest team, count by all players
            if( GetSpecificTeam(balanceskin, sizeof(balanceskin), FALSE, TRUE, FALSE) ||
            // get largest team, count by bots only
                GetSpecificTeam(balanceskin, sizeof(balanceskin), FALSE, TRUE, TRUE) )
            {
               for(int i = 0; i < 32; i++)
               {
                  char teamstr[MAX_TEAMNAME_LENGTH];
                  
                  if(bots[i].is_used)
                  {
                     if(!stricmp(UTIL_GetTeam(bots[i].pEdict, teamstr, sizeof(teamstr)), balanceskin))
                     {
                        pick = i;
                        
                        if(debug_minmax)
                           UTIL_ConsolePrintf("Teambalance override, kicking from team: %s", balanceskin);
                        
                        break;
                     }
                  }
               }
            }
         }
            
         if(pick != -1)
            BotKick(bots[pick]);
         
         bot_check_time = gpGlobals->time + 0.5;
      }
      else
         bot_check_time = gpGlobals->time + ((!!debug_minmax) ? 5.0 : 0.5);
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
   memset(pFunctionTable, 0, sizeof (DLL_FUNCTIONS));
   
   pFunctionTable->pfnGameInit = GameDLLInit;
   pFunctionTable->pfnSpawn = Spawn;
   pFunctionTable->pfnClientConnect = jkbotti_ClientConnect;
   pFunctionTable->pfnClientPutInServer = jkbotti_ClientPutInServer;
   pFunctionTable->pfnClientDisconnect = ClientDisconnect;
   pFunctionTable->pfnClientCommand = ClientCommand;
   pFunctionTable->pfnStartFrame = StartFrame;
   pFunctionTable->pfnServerDeactivate = ServerDeactivate;
   pFunctionTable->pfnPM_Move = PM_Move;
   pFunctionTable->pfnCmdStart = CmdStart;

   return (TRUE);
}

C_DLLEXPORT int GetEntityAPI2_POST (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
   memset(pFunctionTable, 0, sizeof (DLL_FUNCTIONS));
   
   pFunctionTable->pfnSpawn = Spawn_Post;
   pFunctionTable->pfnPlayerPostThink = PlayerPostThink_Post;
   pFunctionTable->pfnKeyValue = DispatchKeyValue_Post;

   return (TRUE);
}
