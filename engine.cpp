//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
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


extern enginefuncs_t g_engfuncs;
extern bot_t bots[32];
extern int mod_id;
extern int team_allies[4];
extern char g_argv[1024];
extern bool isFakeClientCommand;
extern int fake_arg_count;


void (*botMsgFunction)(void *, int) = NULL;
void (*botMsgEndFunction)(void *, int) = NULL;
int botMsgIndex;

static FILE *fp;


void pfnChangeLevel(char* s1, char* s2)
{
   // kick any bot off of the server after time/frag limit...
   for (int index = 0; index < 32; index++)
   {
      if (bots[index].is_used)  // is this slot used?
      {
         char cmd[40];

         sprintf(cmd, "kick \"%s\"\n", bots[index].name);

         bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;

         SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
      }
   }

   RETURN_META (MRES_IGNORED);
}


edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
   if (gpGlobals->deathmatch)
   {
      int bot_index;
      bot_t *pBot;

      if (mod_id == CSTRIKE_DLL)
      {
         // new round in CS 1.5
         if (strcmp ("info_map_parameters", pszValue) == 0)
         {
            for (bot_index = 0; bot_index < 32; bot_index++)
            {
               pBot = &bots[bot_index];

               if (pBot->is_used)
                  BotSpawnInit (pBot); // reset bots for new round
            }
         }
      }
   }

   RETURN_META_VALUE (MRES_IGNORED, NULL);
}


void pfnEmitSound(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
   int index;
   float distance;
   Vector vecEnd;
   edict_t *pEdict;

   if (gpGlobals->deathmatch)
   {
      if (mod_id == TFC_DLL)
      {
         // is someone yelling for a medic?
         if ((strcmp(sample, "speech/saveme1.wav") == 0) ||
             (strcmp(sample, "speech/saveme2.wav") == 0))
         {
            for (index=0; index < 32; index++)
            {
               if (bots[index].is_used)  // is this slot used?
               {
                  pEdict = bots[index].pEdict;
                  
                  if ((pEdict->v.playerclass != TFC_CLASS_MEDIC) &&
                      (pEdict->v.playerclass != TFC_CLASS_ENGINEER))
                     continue;

                  int player_team = UTIL_GetTeam(entity);
                  int bot_team = UTIL_GetTeam(pEdict);

                  // don't heal your enemies...
                  if ((bot_team != player_team) &&
                      !(team_allies[bot_team] & (1<<player_team)))
                     continue;

                  distance = (pEdict->v.origin - entity->v.origin).Length();

                  vecEnd = entity->v.origin + entity->v.view_ofs;

                  if ((distance < 1000) && FVisible(vecEnd, pEdict) &&
                      (bots[index].pBotEnemy == NULL))
                  {
                     bots[index].pBotEnemy = entity;

                     bots[index].enemy_attack_count = 3;  // hit 'em 3 times

                     bots[index].pBotUser = NULL;  // don't follow user when enemy found
                  }
               }
            }
         }
      }
   }

   RETURN_META (MRES_IGNORED);
}


void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
{
   if ((pEdict->v.flags & FL_FAKECLIENT) || (pEdict->v.flags & FL_THIRDPARTYBOT))
      RETURN_META (MRES_SUPERCEDE);
   RETURN_META (MRES_IGNORED);
}


void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
   if (gpGlobals->deathmatch)
   {
      int index = -1;

      if (ed)
      {
         if ((mod_id == HOLYWARS_DLL) &&
             (msg_type == GET_USER_MSG_ID (PLID, "Halo", NULL)))
         {
            // catch this message for ALL players, NOT just bots...
            botMsgFunction = BotClient_HolyWars_Halo;

            botMsgIndex = (int)ed;  // save the edict instead of the bot index
         }
         else
         {
            index = UTIL_GetBotIndex(ed);

            // is this message for a bot?
            if (index != -1)
            {
               botMsgFunction = NULL;     // no msg function until known otherwise
               botMsgEndFunction = NULL;  // no msg end function until known otherwise
               botMsgIndex = index;       // index of bot receiving message

               if (mod_id == VALVE_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_Valve_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_Valve_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_Valve_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_Valve_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_Valve_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_Valve_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_Valve_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_Valve_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_Valve_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ScreenFade", NULL))
                     botMsgFunction = BotClient_Valve_ScreenFade;
               }
               else if (mod_id == TFC_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "VGUIMenu", NULL))
                     botMsgFunction = BotClient_TFC_VGUI;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_TFC_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_TFC_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_TFC_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_TFC_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "SecAmmoVal", NULL))
                     botMsgFunction = BotClient_TFC_SecAmmoVal;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_TFC_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_TFC_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_TFC_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_TFC_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_TFC_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "TextMsg", NULL))
                  {
                     botMsgFunction = BotClient_TFC_TextMsg;
                     botMsgEndFunction = BotClient_TFC_TextMsg;
                  }
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ScreenFade", NULL))
                     botMsgFunction = BotClient_TFC_ScreenFade;
               }
               else if (mod_id == CSTRIKE_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "VGUIMenu", NULL))
                     botMsgFunction = BotClient_CS_VGUI;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ShowMenu", NULL))
                     botMsgFunction = BotClient_CS_ShowMenu;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_CS_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_CS_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_CS_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_CS_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_CS_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_CS_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_CS_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_CS_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_CS_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Money", NULL))
                     botMsgFunction = BotClient_CS_Money;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ScreenFade", NULL))
                     botMsgFunction = BotClient_CS_ScreenFade;
               }
               else if (mod_id == GEARBOX_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "VGUIMenu", NULL))
                     botMsgFunction = BotClient_Gearbox_VGUI;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_Gearbox_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_Gearbox_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_Gearbox_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_Gearbox_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_Gearbox_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_Gearbox_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_Gearbox_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_Gearbox_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_Gearbox_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ScreenFade", NULL))
                     botMsgFunction = BotClient_Gearbox_ScreenFade;
               }
               else if (mod_id == FRONTLINE_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "VGUIMenu", NULL))
                  {
                     botMsgFunction = BotClient_FLF_VGUI;
                     botMsgEndFunction = BotClient_FLF_VGUI;
                  }
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_FLF_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_FLF_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_FLF_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_FLF_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_FLF_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_FLF_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_FLF_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_FLF_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_FLF_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "TextMsg", NULL))
                  {
                     botMsgFunction = BotClient_FLF_TextMsg;
                     botMsgEndFunction = BotClient_FLF_TextMsg;
                  }
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WarmUp", NULL))
                     botMsgFunction = BotClient_FLF_WarmUp;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ScreenFade", NULL))
                     botMsgFunction = BotClient_FLF_ScreenFade;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "HideWeapon", NULL))
                     botMsgFunction = BotClient_FLF_HideWeapon;
               }
               else if (mod_id == HOLYWARS_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_Valve_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_Valve_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_Valve_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_Valve_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_Valve_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_Valve_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_Valve_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_Valve_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_Valve_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ScreenFade", NULL))
                     botMsgFunction = BotClient_Valve_ScreenFade;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "GameMode", NULL))
                     botMsgFunction = BotClient_HolyWars_GameMode;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "HudText", NULL))
                     botMsgFunction = BotClient_HolyWars_HudText;
               }
               else if (mod_id == DMC_DLL)
               {
                  if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
                     botMsgFunction = BotClient_DMC_WeaponList;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "CurWeapon", NULL))
                     botMsgFunction = BotClient_DMC_CurrentWeapon;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoX", NULL))
                     botMsgFunction = BotClient_DMC_AmmoX;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "AmmoPickup", NULL))
                     botMsgFunction = BotClient_DMC_AmmoPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "WeapPickup", NULL))
                     botMsgFunction = BotClient_DMC_WeaponPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "ItemPickup", NULL))
                     botMsgFunction = BotClient_DMC_ItemPickup;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Health", NULL))
                     botMsgFunction = BotClient_DMC_Health;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Battery", NULL))
                     botMsgFunction = BotClient_DMC_Battery;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "Damage", NULL))
                     botMsgFunction = BotClient_DMC_Damage;
                  else if (msg_type == GET_USER_MSG_ID (PLID, "QItems", NULL))
                     botMsgFunction = BotClient_DMC_QItems;
               }
            }
         }
      }
      else if (msg_dest == MSG_ALL)
      {
         botMsgFunction = NULL;  // no msg function until known otherwise
         botMsgIndex = -1;       // index of bot receiving message (none)

         if (mod_id == VALVE_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_Valve_DeathMsg;
         }
         else if (mod_id == TFC_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_TFC_DeathMsg;
         }
         else if (mod_id == CSTRIKE_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_CS_DeathMsg;
         }
         else if (mod_id == GEARBOX_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_Gearbox_DeathMsg;
         }
         else if (mod_id == FRONTLINE_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_FLF_DeathMsg;
            else if (msg_type == GET_USER_MSG_ID (PLID, "WarmUp", NULL))
               botMsgFunction = BotClient_FLF_WarmUpAll;
            else if (msg_type == GET_USER_MSG_ID (PLID, "WinMessage", NULL))
               botMsgFunction = BotClient_FLF_WinMessage;
         }
         else if (mod_id == HOLYWARS_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_Valve_DeathMsg;
         }
         else if (mod_id == DMC_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "DeathMsg", NULL))
               botMsgFunction = BotClient_DMC_DeathMsg;
         }
      }
      else
      {
         // Steam makes the WeaponList message be sent differently

         botMsgFunction = NULL;  // no msg function until known otherwise
         botMsgIndex = -1;       // index of bot receiving message (none)

         if (mod_id == VALVE_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
               botMsgFunction = BotClient_Valve_WeaponList;
         }
         else if (mod_id == TFC_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
               botMsgFunction = BotClient_TFC_WeaponList;
         }
         else if (mod_id == CSTRIKE_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
               botMsgFunction = BotClient_CS_WeaponList;
            else if (msg_type == GET_USER_MSG_ID (PLID, "HLTV", NULL))
               botMsgFunction = BotClient_CS_HLTV;
         }
         else if (mod_id == GEARBOX_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
               botMsgFunction = BotClient_Gearbox_WeaponList;
         }
         else if (mod_id == FRONTLINE_DLL)
         {
            if (msg_type == GET_USER_MSG_ID (PLID, "WeaponList", NULL))
               botMsgFunction = BotClient_FLF_WeaponList;
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
   if ((pEdict->v.flags & FL_FAKECLIENT) || (pEdict->v.flags & FL_THIRDPARTYBOT))
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
         RETURN_META_VALUE (MRES_SUPERCEDE, &g_argv[64]);
      else if (argc == 1)
         RETURN_META_VALUE (MRES_SUPERCEDE, &g_argv[128]);
      else if (argc == 2)
         RETURN_META_VALUE (MRES_SUPERCEDE, &g_argv[192]);
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


int pfnGetPlayerUserId(edict_t *e )
{
   if (gpGlobals->deathmatch)
   {
      if (mod_id == GEARBOX_DLL)
      {
         // is this edict a bot?
         if (UTIL_GetBotPointer( e ))
            RETURN_META_VALUE (MRES_SUPERCEDE, 0);  // don't return a valid index (so bot won't get kicked)
      }
   }

   RETURN_META_VALUE (MRES_IGNORED, 0);
}


const char *pfnGetPlayerAuthId (edict_t *e)
{
   if ((e->v.flags & FL_FAKECLIENT) || (e->v.flags & FL_THIRDPARTYBOT))
      RETURN_META_VALUE (MRES_SUPERCEDE, "0");

   RETURN_META_VALUE (MRES_IGNORED, NULL);
}


C_DLLEXPORT int GetEngineFunctions (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion)
{
   meta_engfuncs.pfnChangeLevel = pfnChangeLevel;
   meta_engfuncs.pfnFindEntityByString = pfnFindEntityByString;
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
   meta_engfuncs.pfnGetPlayerUserId = pfnGetPlayerUserId;
   meta_engfuncs.pfnGetPlayerAuthId = pfnGetPlayerAuthId;

   memcpy (pengfuncsFromEngine, &meta_engfuncs, sizeof (enginefuncs_t));
   return TRUE;
}
