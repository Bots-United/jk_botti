/***
*
*  Copyright (c) 1999, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*  All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// util.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "usercmd.h"
#include "bot.h"
#include "bot_func.h"


extern bot_t bots[32];
extern qboolean is_team_play;

trigger_sound_t trigger_sounds[32];


void ClientPrint( edict_t *pEntity, int msg_dest, const char *msg_name)
{       
   if (GET_USER_MSG_ID (PLID, "TextMsg", NULL) <= 0)
      REG_USER_MSG ("TextMsg", -1);

   MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "TextMsg", NULL), NULL, pEntity );

   WRITE_BYTE( msg_dest );
   WRITE_STRING( msg_name );
   MESSAGE_END();
}

void UTIL_SayText( const char *pText, edict_t *pEdict )
{
   if (GET_USER_MSG_ID (PLID, "SayText", NULL) <= 0)
      REG_USER_MSG ("SayText", -1);

   MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "SayText", NULL), NULL, pEdict );
      WRITE_BYTE( ENTINDEX(pEdict) );
      WRITE_STRING( pText );
   MESSAGE_END();
}

void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message )
{
   int   j;
   char  text[128];
   char *pc;
   edict_t *client;
   char  sender_teamstr[32];
   char  player_teamstr[32];

   // make sure the text has content
   for ( pc = message; pc != NULL && *pc != 0; pc++ )
   {
      if ( isprint( *pc ) && !isspace( *pc ) )
      {
         pc = NULL;   // we've found an alphanumeric character,  so text is valid
         break;
      }
   }

   if ( pc != NULL )
      return;  // no character found, so say nothing

   // turn on color set 2  (color on,  no sound)
   if ( teamonly )
      safevoid_snprintf( text, sizeof(text), "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
   else
      safevoid_snprintf( text, sizeof(text), "%c%s: ", 2, STRING( pEntity->v.netname ) );

   j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
   if ( (int)strlen(message) > j )
      message[j] = 0;

   strcat( text, message );
   strcat( text, "\n" );

   // loop through all players
   // Start with the first player.
   // This may return the world in single player if the client types something between levels or during spawn
   // so check it, or it will infinite loop

   if (GET_USER_MSG_ID (PLID, "SayText", NULL) <= 0)
      REG_USER_MSG ("SayText", -1);

   UTIL_GetTeam(pEntity, sender_teamstr);

   client = NULL;
   while((client = UTIL_FindEntityByClassname( client, "player" )) != NULL && !FNullEnt(client))
   {
      if ( client == pEntity )  // skip sender of message
         continue;

      UTIL_GetTeam(client, player_teamstr);

      if ( teamonly && is_team_play && stricmp(sender_teamstr, player_teamstr) != 0 )
         continue;

      MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "SayText", NULL), NULL, client );
         WRITE_BYTE( ENTINDEX(pEntity) );
         WRITE_STRING( text );
      MESSAGE_END();
   }

   // print to the sending client
   MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "SayText", NULL), NULL, pEntity );
      WRITE_BYTE( ENTINDEX(pEntity) );
      WRITE_STRING( text );
   MESSAGE_END();

   // echo to server console
   SERVER_PRINT( text );
}

#ifdef   DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
   if (pev->pContainingEntity != NULL)
      return pev->pContainingEntity;
   
   UTIL_ConsolePrintf("%s", "entvars_t pContainingEntity is NULL, calling into engine");
   edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
   if (pent == NULL)
      UTIL_ConsolePrintf("%s", "DAMN!  Even the engine couldn't FindEntityByVars!");
   ((entvars_t *)pev)->pContainingEntity = pent;
   return pent;
}
#endif //DEBUG

// return team string 0 through 3 based what MOD uses for team numbers
char * UTIL_GetTeam(edict_t *pEntity, char teamstr[32])
{
   char *infobuffer;
   char model_name[32];

   infobuffer = GET_INFOKEYBUFFER( pEntity );
   *model_name = 0; 
   strncat(model_name, INFOKEY_VALUE (infobuffer, "model"), sizeof(model_name));

   strcpy(teamstr, model_name);
   return(teamstr);
}

qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t ** pHit )
{
   TraceResult tr;
   Vector      vecLookerOrigin;

   // look through caller's eyes
   vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

   int bInWater = (POINT_CONTENTS (vecOrigin) == CONTENTS_WATER);
   int bLookerInWater = (POINT_CONTENTS (vecLookerOrigin) == CONTENTS_WATER);

   // don't look through water
   if (bInWater != bLookerInWater)
      return FALSE;

   UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

   if (tr.flFraction != 1.0)
   {
      if(pHit)
         *pHit = 0;
         
      return FALSE;  // Line of sight is not established
   }
   else
   {
      if(pHit)
         *pHit = tr.pHit;
         
      return TRUE;  // line of sight is valid.
   }
}

qboolean FInShootCone(const Vector & Origin, edict_t *pEdict, float distance, float diameter, float min_angle)
{
   /*
   
      <----- distance ---->
      
                  ___....->T^           ^
       ...----''''          | <- radius |
     O ------------------->Qv           |
     ^ '''----....___                   | <- diameter
     |               ''''->             v
     |
     Bot(pEdict)
   
    T: Target (Origin)
    
    if angle Q-O-T is less than min_angle, always return true.
   
   */
   
   if(distance < 0.01)
      return TRUE;
   
   // angle between forward-view-vector and vector to player (as cos(angle))
   float flDot = DotProduct( (Origin - (pEdict->v.origin + pEdict->v.view_ofs)).Normalize(), UTIL_AnglesToForward(pEdict->v.v_angle) );
   if(flDot > cos(deg2rad(min_angle))) // smaller angle, bigger cosine
      return TRUE;
   
   Vector2D triangle;
   triangle.x = distance;
   triangle.y = diameter / 2.0;
   
   // full angle of shootcode at this distance (as cos(angle))   
   if(flDot > (distance / triangle.Length())) // smaller angle, bigger cosine
      return TRUE;
      
   return FALSE;
}

void UTIL_SelectWeapon(edict_t *pEdict, int weapon_index)
{
   usercmd_t user;

   user.lerp_msec = 0;
   user.msec = 0;
   user.viewangles = pEdict->v.v_angle;
   user.forwardmove = 0;
   user.sidemove = 0;
   user.upmove = 0;
   user.lightlevel = 127;
   user.buttons = 0;
   user.impulse = 0;
   user.weaponselect = weapon_index;
   user.impact_index = 0;
   user.impact_position = Vector(0, 0, 0);

   MDLL_CmdStart(pEdict, &user, 0);
   MDLL_CmdEnd(pEdict);
}

void SaveSound(edict_t * pPlayer, float time, const Vector & origin, float volume, float attenuation, int used) 
{
   int i = ENTINDEX(pPlayer) - 1;
   if(i < 0 || i >= gpGlobals->maxClients)
      return;
   
   // importance value for new sound
   float importance = 0.0;
   
   if(attenuation > 0)
   {
      importance = volume * (1024 / attenuation);
   }
   
   if(trigger_sounds[i].used && importance < trigger_sounds[i].importance)
   {
      //decrease remembered volume over time
      if((time - trigger_sounds[i].time) >= 0.1)
      {
         trigger_sounds[i].time += 0.1;
         trigger_sounds[i].importance *= 0.95;
      }
   }
   else
   {
      trigger_sounds[i].time = time;
      trigger_sounds[i].origin = origin;
      trigger_sounds[i].volume = volume;
      trigger_sounds[i].attenuation = attenuation;
      trigger_sounds[i].used = used;
      trigger_sounds[i].importance = importance;
   }
}

void UTIL_PrintBotInfo(void(*printfunc)(void *, char*), void * arg) {
   //print out bot info
   char msg[80];
   int bot_index, count = 0;
   
   for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
   {
      if (bots[bot_index].is_used) {
         count++;
         safevoid_snprintf(msg, sizeof(msg), "Bot #%d\n", count);
         printfunc(arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " name: %s\n", bots[bot_index].name);
         printfunc(arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " skin: %s\n", bots[bot_index].skin);
         printfunc(arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " skill: %d\n", bots[bot_index].bot_skill + 1);
         printfunc(arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " got enemy: %s\n", (bots[bot_index].pBotEnemy != 0) ? "true" : "false"); 
         printfunc(arg, msg);
         safevoid_snprintf(msg, sizeof(msg), "---\n"); 
         printfunc(arg, msg);
      }
   }
      
   safevoid_snprintf(msg, sizeof(msg), "Total Bots: %d\n", count);
   printfunc(arg, msg);
}

void UTIL_BuildFileName_N(char *filename, int size, char *arg1, char *arg2)
{
   if ((arg1 != NULL) && (arg2 != NULL))
   {
      if (*arg1 && *arg2)
      {
         safevoid_snprintf(filename, size, "valve/%s/%s", arg1, arg2);
         return;
      }
   }

   if (arg1 != NULL)
   {
      if (*arg1)
      {
         safevoid_snprintf(filename, size, "valve/%s", arg1);
         return;
      }
   }
   
   safevoid_snprintf(filename, size, "valve/");
   return;
}


//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
   va_list argptr;
   char string[1024];

   va_start( argptr, fmt );
   safevoid_vsnprintf( string, sizeof(string), fmt, argptr );
   va_end( argptr );

   // Print to server console
   ALERT( at_logged, "%s", string );
}

void UTIL_ServerPrintf( char *fmt, ... )
{
   va_list argptr;
   char string[512];
   
   va_start( argptr, fmt );
   safevoid_vsnprintf( string, sizeof(string), fmt, argptr );
   va_end( argptr );

   // Print to server console
   SERVER_PRINT( string );
}

void UTIL_ConsolePrintf( char *fmt, ... )
{
   va_list argptr;
   char string[1024];
   size_t len;
   
   strcpy(string, "[jk_botti] ");
   len = strlen(string);
   
   va_start( argptr, fmt );
   safevoid_vsnprintf( string+len, sizeof(string)-len, fmt, argptr );
   va_end( argptr );

   // end msg with newline if not already
   len = strlen(string);
   if(string[len-1] != '\n')
   {
      if(len < sizeof(string)-2)// -1 null, -1 for newline
         strcat(string, "\n");
      else
         string[len-1] = '\n';
   }

   // Print to server console
   SERVER_PRINT( string );
}

char* UTIL_VarArgs( char *format, ... )
{
   va_list	    argptr;
   static char string[1024];
	
   va_start (argptr, format);
   safevoid_vsnprintf (string, sizeof(string), format,argptr);
   va_end (argptr);

   return string;	
}


void GetGameDir (char *game_dir)
{
   // This function fixes the erratic behaviour caused by the use of the GET_GAME_DIR engine
   // macro, which returns either an absolute directory path, or a relative one, depending on
   // whether the game server is run standalone or not. This one always return a RELATIVE path.

   unsigned char length, fieldstart, fieldstop;

   GET_GAME_DIR (game_dir); // call the engine macro and let it mallocate for the char pointer

   length = strlen (game_dir); // get the length of the returned string
   length--; // ignore the trailing string terminator

   // format the returned string to get the last directory name
   fieldstop = length;
   while (((game_dir[fieldstop] == '\\') || (game_dir[fieldstop] == '/')) && (fieldstop > 0))
      fieldstop--; // shift back any trailing separator

   fieldstart = fieldstop;
   while ((game_dir[fieldstart] != '\\') && (game_dir[fieldstart] != '/') && (fieldstart > 0))
      fieldstart--; // shift back to the start of the last subdirectory name

   if ((game_dir[fieldstart] == '\\') || (game_dir[fieldstart] == '/'))
      fieldstart++; // if we reached a separator, step over it

   // now copy the formatted string back onto itself character per character
   for (length = fieldstart; length <= fieldstop; length++)
      game_dir[length - fieldstart] = game_dir[length];
   game_dir[length - fieldstart] = 0; // terminate the string

   return;
}
