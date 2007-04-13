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


void pfnSetSize(edict_t *e, const float *rgflMin, const float *rgflMax) 
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);

   int index = UTIL_GetBotIndex(e);
   if(index == -1)
      RETURN_META (MRES_IGNORED);
                
   bots[index].ducking = (Vector((float *)rgflMin) == VEC_DUCK_HULL_MIN && Vector((float *)rgflMax) == VEC_DUCK_HULL_MAX);
        
   RETURN_META (MRES_IGNORED);
}

void pfnPlaybackEvent( int flags, const edict_t *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 ) 
{
   //TODO:
   float volume = /*GetEventIndexVolume(eventindex)*/1.0;
   
   if (gpGlobals->deathmatch)
   {
      int ivolume = (int)(1000*volume);
      SaveSound((edict_t*)pInvoker, pInvoker->v.origin, ivolume, CHAN_WEAPON);
   }
   
   RETURN_META (MRES_IGNORED);
}

void pfnEmitSound(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
   if (gpGlobals->deathmatch)
   {
      int ivolume = (int)(1000*volume);
      SaveSound((edict_t*)entity, entity->v.origin, ivolume, channel);
   }

   RETURN_META (MRES_IGNORED);
}

void pfnChangeLevel(char* s1, char* s2)
{
   if (!gpGlobals->deathmatch)
      RETURN_META (MRES_IGNORED);
   
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

   RETURN_META (MRES_IGNORED);
}


void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
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
         {
            index = UTIL_GetBotIndex(ed);

            // is this message for a bot?
            if (index != -1)
            {
               botMsgFunction = NULL;     // no msg function until known otherwise
               botMsgEndFunction = NULL;  // no msg end function until known otherwise
               botMsgIndex = index;       // index of bot receiving message

               {
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
            }
         }
      }
      else if (msg_dest == MSG_ALL)
      {
         botMsgFunction = NULL;  // no msg function until known otherwise
         botMsgIndex = -1;       // index of bot receiving message (none)

         {
            if (msg_type == SVC_INTERMISSION)
               g_in_intermission = TRUE;
            else if (msg_type == FAST_GET_USER_MSG_ID (PLID, DeathMsg, "DeathMsg", NULL))
               botMsgFunction = BotClient_Valve_DeathMsg;
         }
      }
      else
      {
         // Steam makes the WeaponList message be sent differently

         botMsgFunction = NULL;  // no msg function until known otherwise
         botMsgIndex = -1;       // index of bot receiving message (none)

         {
            if (msg_type == FAST_GET_USER_MSG_ID (PLID, WeaponList, "WeaponList", NULL))
               botMsgFunction = BotClient_Valve_WeaponList;
         }
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
   int index;

   index = UTIL_GetBotIndex((edict_t *)pEdict);

   // is this message for a bot?
   if (index != -1)
      bots[index].f_max_speed = fNewMaxspeed;

   RETURN_META (MRES_IGNORED);
}


C_DLLEXPORT int GetEngineFunctions (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
   meta_engfuncs.pfnPlaybackEvent = pfnPlaybackEvent;
   meta_engfuncs.pfnChangeLevel = pfnChangeLevel;
   meta_engfuncs.pfnEmitSound = pfnEmitSound;
   meta_engfuncs.pfnClientCommand = pfnClientCommand;
   meta_engfuncs.pfnMessageBegin = pfnMessageBegin;
   meta_engfuncs.pfnMessageEnd = pfnMessageEnd;
   meta_engfuncs.pfnWriteByte = pfnWriteByte;
   meta_engfuncs.pfnWriteChar = pfnWriteChar;
   meta_engfuncs.pfnWriteShort = pfnWriteShort;
   meta_engfuncs.pfnWriteLong = pfnWriteLong;
   meta_engfuncs.pfnWriteAngle = pfnWriteAngle;
   meta_engfuncs.pfnWriteCoord = pfnWriteCoord;
   meta_engfuncs.pfnWriteString = pfnWriteString;
   meta_engfuncs.pfnWriteEntity = pfnWriteEntity;
   meta_engfuncs.pfnClientPrintf = pfnClientPrintf;
   meta_engfuncs.pfnCmd_Args = pfnCmd_Args;
   meta_engfuncs.pfnCmd_Argv = pfnCmd_Argv;
   meta_engfuncs.pfnCmd_Argc = pfnCmd_Argc;
   meta_engfuncs.pfnSetClientMaxspeed = pfnSetClientMaxspeed;
   meta_engfuncs.pfnSetSize = pfnSetSize;

   memcpy (pengfuncsFromEngine, &meta_engfuncs, sizeof (enginefuncs_t));
   return TRUE;
}
