//
// JK_Botti - be more human!
//
// engine.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "bot_sound.h"
#include "bot_weapons.h"
#include "bot_weapon_select.h"

extern enginefuncs_t g_engfuncs;
extern bot_t bots[32];
extern qboolean isFakeClientCommand;
extern int fake_arg_count;

char g_argv[1024*3];
char g_arg1[1024];
char g_arg2[1024];
char g_arg3[1024];

extern int get_cvars;
extern qboolean aim_fix;
extern float turn_skill;

void (*botMsgFunction)(void *, int) = NULL;
void (*botMsgEndFunction)(void *, int) = NULL;
int botMsgIndex;

qboolean g_in_intermission = FALSE;


typedef struct event_info_s {
   int eventindex;
   const char * eventname;
   float volume;
   float recoil[2];
} event_info_t;


event_info_t g_event_info[] = {
//from valve
   {-1, "events/glock1.sc",		0.96,  {-2.0, -2.0}},
   {-1, "events/glock2.sc",		0.96,  {-2.0, -2.0}},
   {-1, "events/shotgun1.sc",		0.975, {-5.0, -5.0}},
   {-1, "events/shotgun2.sc",		0.99,  {-10.0, -10.0}},
   {-1, "events/mp5.sc",		1.0,   {-2.0, 2.0}},
   {-1, "events/mp52.sc",		1.0,   {-10.0, -10.0}},
   {-1, "events/python.sc",		0.85,  {-10.0, -10.0}},
   {-1, "events/gauss.sc",		1.0,   {-2.0, -2.0}},
   {-1, "events/gaussspin.sc",		1.0,   {0, 0}},
   {-1, "events/train.sc",		0.6,   {0, 0}},
   {-1, "events/crowbar.sc",		1.0,   {0, 0}},
   {-1, "events/crossbow1.sc",		1.0,   {-2.0, -2.0}},
   {-1, "events/crossbow2.sc",		1.0,   {0, 0}},
   {-1, "events/rpg.sc",		0.9,   {-5.0, -5.0}},
   {-1, "events/egon_fire.sc",		0.94,  {0, 0}},
   {-1, "events/egon_stop.sc",		0.98,  {0, 0}},
   {-1, "events/firehornet.sc",		1.0,   {0.0, 2.0}},
   {-1, "events/tripfire.sc",		0.0,   {0, 0}},
   {-1, "events/snarkfire.sc",		0.0,   {0, 0}},
//from gearbox
   {-1, "events/egon_effect.sc",	0.0,   {0, 0}},
   {-1, "events/eagle.sc",		0.90,  {-20.0, -20.0}},
   {-1, "events/pipewrench.sc",		0.8,   {0, 0}},
   {-1, "events/m249.sc",		0.96,  {-7.0, 7.0}},
   {-1, "events/shock.sc",		0.96,  {0.0, 2.0}},
   {-1, "events/sniper.sc",		1.00,  {-10.0, -10.0}},
   {-1, "events/knife.sc",		0.6,   {0, 0}},
   {-1, "events/penguinfire.sc",	0.85,  {0, 0}},
   {-1, "events/spore.sc",		0.96,  {-10.0, -10.0}},
   
   {-1, "", 0, {0, 0}} //NULL
};


//
unsigned short pfnPrecacheEvent_Post(int type, const char* psz)
{
   if (!gpGlobals->deathmatch)
      RETURN_META_VALUE (MRES_IGNORED, 0);
   
   unsigned short eventindex = (META_RESULT_STATUS == MRES_OVERRIDE || META_RESULT_STATUS == MRES_SUPERCEDE) ? META_RESULT_OVERRIDE_RET(unsigned short) : META_RESULT_ORIG_RET(unsigned short);

   event_info_t *pei;
   for(pei = g_event_info; pei->eventname && pei->eventname[0]; pei++)
   {
      if(strcmp(psz, pei->eventname) == 0)
      {
         // found event, update eventindex
         pei->eventindex = eventindex;
         break;
      }
   }
   
   if(!pei->eventname || !pei->eventname[0])
   {
      // unknown event
      //UTIL_ConsolePrintf("PrecacheEvent, new event type: \"%s\" : %d", psz, eventindex);
   }
   
   RETURN_META_VALUE (MRES_IGNORED, 0);
}


//
void pfnPlaybackEvent( int flags, const edict_t *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 ) 
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);

   // get event info
   event_info_t *pei;
   
   for(pei = g_event_info; pei->eventname; pei++)
   {
      if(eventindex == pei->eventindex)
         break;
   }
   
   if(!pei->eventname)
      RETURN_META (MRES_IGNORED);
   
   // event creates sound?
   if(pei->volume > 0.0f)
   {
      int ivolume = (int)(1000*pei->volume);
      
      SaveSound((edict_t*)pInvoker, pInvoker->v.origin, ivolume, CHAN_WEAPON, 5.0f);
   }
   
   // event causes client recoil?
   if(pei->recoil[0] != 0.0f && pei->recoil[1] != 0.0f)
   {
      // gauss uses bug fix that sends two events at same time,
      // ignore the duplicated one...
      if(strcmp("events/gauss.sc", pei->eventname) == 0 && delay > 0.0f && (flags & FEV_RELIABLE))
         RETURN_META (MRES_IGNORED);
      
      int index = UTIL_GetBotIndex(pInvoker);
      
      if(index != -1)
      {
         bots[index].f_recoil += RANDOM_FLOAT2(pei->recoil[0], pei->recoil[1]);
      }
   }
   
   RETURN_META (MRES_IGNORED);
}

void pfnEmitSound(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch)
{
   if (gpGlobals->deathmatch)
   {
      int ivolume = (int)(1000*volume);
      const char *classname = (const char *)STRING(entity->v.classname);
      float duration = 5.0f;
      
      if (strncmp("item_health", classname, 11) == 0)
         duration = 8.0f;
      else if (strncmp("item_battery", classname, 12) == 0)
         duration = 8.0f;
      else if (GetAmmoItemFlag(classname) != 0)
         duration = 8.0f;
      else if (GetWeaponItemFlag(classname) != 0 && (entity->v.owner == NULL))
         duration = 8.0f;
      else if (strcmp("item_longjump", classname) == 0)
         duration = 8.0f;
      
      SaveSound(entity, entity->v.origin, ivolume, channel, duration);
   }

   RETURN_META (MRES_IGNORED);
}

void pfnChangeLevel(char* s1, char* s2)
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   // kick any bot off of the server after time/frag limit...
   for (int index = 0; index < 32; index++)
      if (bots[index].is_used)  // is this slot used?
         BotKick(bots[index]);

   RETURN_META (MRES_IGNORED);
}


void pfnClientCommand(edict_t* pEdict, const char* szFmt, ...)
{
   if ((FBitSet(pEdict->v.flags, FL_FAKECLIENT) || FBitSet(pEdict->v.flags, FL_THIRDPARTYBOT)))
      RETURN_META (MRES_SUPERCEDE);
      
   RETURN_META (MRES_IGNORED);
}

int FAST_GET_USER_MSG_ID(plid_t plindex, int & value, const char * name, int * size) 
{
   return(value ? value : (value = GET_USER_MSG_ID(plindex, name, size)));
}

void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{   
   if (gpGlobals->deathmatch)
   {
      int index = -1;
      
      static int WeaponList = 0;
      static int CurWeapon = 0;
      static int AmmoX = 0;
      static int AmmoPickup = 0;
      static int WeapPickup = 0;
      static int ItemPickup = 0;
      static int Health = 0;
      static int Battery = 0;
      static int Damage = 0;
      static int ScreenFade = 0;
      static int DeathMsg = 0;
      
      if (ed)
      {
         index = UTIL_GetBotIndex(ed);

         // is this message for a bot?
         if (index != -1)
         {
            botMsgEndFunction = NULL;  // no msg end function until known otherwise
            botMsgIndex = index;       // index of bot receiving message

            if (msg_type == FAST_GET_USER_MSG_ID (PLID, WeaponList, "WeaponList", NULL))
               botMsgFunction = BotClient_Valve_WeaponList;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, CurWeapon, "CurWeapon", NULL))
               botMsgFunction = BotClient_Valve_CurrentWeapon;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, AmmoX, "AmmoX", NULL))
               botMsgFunction = BotClient_Valve_AmmoX;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, AmmoPickup, "AmmoPickup", NULL))
               botMsgFunction = BotClient_Valve_AmmoPickup;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, WeapPickup, "WeapPickup", NULL))
               botMsgFunction = BotClient_Valve_WeaponPickup;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, ItemPickup, "ItemPickup", NULL))
               botMsgFunction = BotClient_Valve_ItemPickup;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, Health, "Health", NULL))
               botMsgFunction = BotClient_Valve_Health;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, Battery, "Battery", NULL))
               botMsgFunction = BotClient_Valve_Battery;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, Damage, "Damage", NULL))
               botMsgFunction = BotClient_Valve_Damage;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, ScreenFade, "ScreenFade", NULL))
               botMsgFunction = BotClient_Valve_ScreenFade;
         }
         // is this message for a player?
         else 
         {
            index = ENTINDEX(ed) - 1;
            if(index > -1 && index < gpGlobals->maxClients)
            {
               botMsgFunction = NULL;     // no msg function until known otherwise
               botMsgEndFunction = NULL;  // no msg end function until known otherwise
               botMsgIndex = index;       // index of bot receiving message
            }
            
            if (msg_type == FAST_GET_USER_MSG_ID (PLID, CurWeapon, "CurWeapon", NULL))
               botMsgFunction = PlayerClient_Valve_CurrentWeapon;
         }
      }
      else if (msg_dest == MSG_ALL)
      {
         botMsgFunction = NULL;     // no msg function until known otherwise
         botMsgEndFunction = NULL;  // no msg end function until known otherwise
         botMsgIndex = -1;          // index of bot receiving message (none)

         if (msg_type == SVC_INTERMISSION)
            g_in_intermission = TRUE;
         else if (msg_type == FAST_GET_USER_MSG_ID (PLID, DeathMsg, "DeathMsg", NULL))
            botMsgFunction = BotClient_Valve_DeathMsg;
      }
      else
      {
         // Steam makes the WeaponList message be sent differently

         botMsgFunction = NULL;     // no msg function until known otherwise
         botMsgEndFunction = NULL;  // no msg end function until known otherwise
         botMsgIndex = -1;          // index of bot receiving message (none)

         if (msg_type == FAST_GET_USER_MSG_ID (PLID, WeaponList, "WeaponList", NULL))
            botMsgFunction = BotClient_Valve_WeaponList;
      }
   }

   RETURN_META (MRES_IGNORED);
}


void pfnMessageEnd(void)
{
   if (gpGlobals->deathmatch)
   {
      if (botMsgEndFunction)
         (*botMsgEndFunction)(NULL, botMsgIndex);  // NULL indicated msg end

      // clear out the bot message function pointers...
      botMsgFunction = NULL;
      botMsgEndFunction = NULL;
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteByte(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteChar(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteShort(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteLong(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteAngle(float flValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&flValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteCoord(float flValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&flValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteString(const char *sz)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)sz, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnWriteEntity(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   RETURN_META (MRES_IGNORED);
}


void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
   if ((FBitSet(pEdict->v.flags, FL_FAKECLIENT) || FBitSet(pEdict->v.flags, FL_THIRDPARTYBOT)))
      RETURN_META (MRES_SUPERCEDE);
      
   RETURN_META (MRES_IGNORED);
}


const char *pfnCmd_Args( void )
{
   if (isFakeClientCommand)
      RETURN_META_VALUE (MRES_SUPERCEDE, &g_argv[0]);

   RETURN_META_VALUE (MRES_IGNORED, NULL);
}


const char *pfnCmd_Argv( int argc )
{
   if (isFakeClientCommand)
   {
      if (argc == 0)
         RETURN_META_VALUE (MRES_SUPERCEDE, g_arg1);
      else if (argc == 1)
         RETURN_META_VALUE (MRES_SUPERCEDE, g_arg2);
      else if (argc == 2)
         RETURN_META_VALUE (MRES_SUPERCEDE, g_arg3);
      else
         RETURN_META_VALUE (MRES_SUPERCEDE, NULL);
   }

   RETURN_META_VALUE (MRES_IGNORED, NULL);
}


int pfnCmd_Argc( void )
{
   if (isFakeClientCommand)
      RETURN_META_VALUE (MRES_SUPERCEDE, fake_arg_count);

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed)
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
   int index;

   index = UTIL_GetBotIndex((edict_t *)pEdict);

   // is this message for a bot?
   if (index != -1)
      bots[index].f_max_speed = fNewMaxspeed;

   RETURN_META (MRES_IGNORED);
}


int pfnGetPlayerUserId(edict_t *e )
{
   if (gpGlobals->deathmatch)
   {
      if (submod_id == SUBMOD_OP4)
      {
         // is this edict a bot?
         if (UTIL_GetBotPointer( e ))
            RETURN_META_VALUE (MRES_SUPERCEDE, 0);  // don't return a valid index (so bot won't get kicked)
      }
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


C_DLLEXPORT int GetEngineFunctions (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
   memset(pengfuncsFromEngine, 0, sizeof(enginefuncs_t));
   
   pengfuncsFromEngine->pfnPlaybackEvent = pfnPlaybackEvent;
   pengfuncsFromEngine->pfnChangeLevel = pfnChangeLevel;
   pengfuncsFromEngine->pfnEmitSound = pfnEmitSound;
   pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
   pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
   pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
   pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
   pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
   pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
   pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
   pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
   pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
   pengfuncsFromEngine->pfnWriteString = pfnWriteString;
   pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;
   pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
   pengfuncsFromEngine->pfnCmd_Args = pfnCmd_Args;
   pengfuncsFromEngine->pfnCmd_Argv = pfnCmd_Argv;
   pengfuncsFromEngine->pfnCmd_Argc = pfnCmd_Argc;
   pengfuncsFromEngine->pfnSetClientMaxspeed = pfnSetClientMaxspeed;
   pengfuncsFromEngine->pfnGetPlayerUserId = pfnGetPlayerUserId;

   return TRUE;
}


C_DLLEXPORT int GetEngineFunctions_POST (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
   memset(pengfuncsFromEngine, 0, sizeof(enginefuncs_t));
   
   pengfuncsFromEngine->pfnPrecacheEvent = pfnPrecacheEvent_Post;
   
   return TRUE;
}
