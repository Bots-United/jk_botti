//
// JK_Botti - be more human!
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
#include "player.h"


extern bot_t bots[32];
extern qboolean is_team_play;

breakable_list_t *g_breakable_list = NULL;


// classic way of getting connected client count without using data collected from 'ClientPutInServer'/'ClientDisconnect'.
int UTIL_GetClientCount(void)
{
   int count = 0;
   
   for(int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t * pClient = INDEXENT(i);
      
      if(!pClient || pClient->free || FNullEnt(pClient) || GETPLAYERUSERID(pClient) <= 0 || STRING(pClient->v.netname)[0] == 0)
         continue;
      
      count++;
   }
   
   return(count);
}


// 
int UTIL_GetBotCount(void)
{
   int count = 0;
   
   for(int i = 0; i < 32; i++)
      if(bots[i].is_used)
         count++;
   
   return(count);
}


//
int UTIL_PickRandomBot(void)
{
   int bot_index_list[32];
   int num_bots = 0;
   
   for(int i = 0; i < 32; i++)
      if(bots[i].is_used)  // is this slot used?
         bot_index_list[num_bots++] = i;
   
   if(num_bots > 0)
   {
      if(num_bots == 1)
         return(bot_index_list[0]);
      
      int pick = RANDOM_LONG2(0, num_bots-1);
      
      JKASSERT(pick < 0 || pick > num_bots-1);
      
      return(bot_index_list[pick]);
   }
   
   return(-1);
}


//
void UTIL_DrawBeam(edict_t *pEnemy, const Vector &start, const Vector &end, int width,
    int noise, int red, int green, int blue, int brightness, int speed)
{
   if(pEnemy && (ENTINDEX(pEnemy)-1 < 0 || ENTINDEX(pEnemy)-1 >= gpGlobals->maxClients))
      return;
   
   if(pEnemy == NULL)
      MESSAGE_BEGIN(MSG_ALL, SVC_TEMPENTITY);
   else
      MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEnemy);
   
   WRITE_BYTE( TE_BEAMPOINTS);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_SHORT( m_spriteTexture );
   WRITE_BYTE( 1 ); // framestart
   WRITE_BYTE( 10 ); // framerate
   WRITE_BYTE( 10 ); // life in 0.1's
   WRITE_BYTE( width ); // width
   WRITE_BYTE( noise );  // noise

   WRITE_BYTE( red );   // r, g, b
   WRITE_BYTE( green );   // r, g, b
   WRITE_BYTE( blue );   // r, g, b

   WRITE_BYTE( brightness );   // brightness
   WRITE_BYTE( speed );    // speed
   
   MESSAGE_END();
}


// 
breakable_list_t * UTIL_AddFuncBreakable(edict_t *pEdict)
{
   // get end of list
   breakable_list_t *prev = NULL;
   breakable_list_t *next = g_breakable_list;
   while(next)
   {
      prev = next;
      next = next->next;
   }
   
   // malloc memory
   next = (breakable_list_t*)calloc(1, sizeof(breakable_list_t));
   
   // fill in data
   next->next = NULL;
   next->material_breakable = FALSE;
   next->pEdict = pEdict;
   
   //link end of list next
   if(prev)
      prev->next = next;
   else
      g_breakable_list = next;
   
   return(next);
}

typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial } Materials;

// called on DispatchKeyValue
void UTIL_UpdateFuncBreakable(edict_t *pEdict, const char * setting, const char * value)
{
   // find breakable
   breakable_list_t *plist = g_breakable_list;
   
   while(plist)
   {
      if(plist->pEdict == pEdict)
         break;
      plist = plist->next;
   }
   
   // not found?
   if(!plist)
   {
      // add new
      plist = UTIL_AddFuncBreakable(pEdict);
   }
   
   // check if interesting setting
   if(FStrEq(setting, "material"))
   {
      // update data value
      plist->material_breakable = (atoi(value) != matUnbreakableGlass);
   }
}

// called on ServerDeactivate
void UTIL_FreeFuncBreakables(void)
{
   // free linked list
   breakable_list_t *next = g_breakable_list;
   while(next)
   {
      breakable_list_t *pfree = next;
      next = next->next;
      
      free(pfree);
   }
   
   g_breakable_list = NULL;
}

//
breakable_list_t * UTIL_FindBreakable_Internal(breakable_list_t * pbreakable)
{
   if(unlikely(!pbreakable))
      return(g_breakable_list);
   else
      return(pbreakable->next);
}

//
breakable_list_t * UTIL_FindBreakable(breakable_list_t * pbreakable)
{
   do {
      pbreakable = UTIL_FindBreakable_Internal(pbreakable);
      
      if(unlikely(!pbreakable))
         return(NULL);
      
      // skip unbreakable glass
      if(!pbreakable->material_breakable)
         continue;
      
      // skip deleted entities
      if(FNullEnt(pbreakable->pEdict) || pbreakable->pEdict->v.health <= 0)
         continue;
   
   // skip reused/wrong name entities
   } while(!FIsClassname(pbreakable->pEdict, "func_breakable") && !FIsClassname(pbreakable->pEdict, "func_pushable"));
   
   return(pbreakable);
}


//
void SaveAliveStatus(edict_t * pPlayer)
{
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return;
   
   if(!IsAlive(pPlayer))
      players[idx].last_time_dead = gpGlobals->time;
}

//
float UTIL_GetTimeSinceRespawn(edict_t * pPlayer)
{
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return(-1.0);
   
   if(!IsAlive(pPlayer))
   {
      //we are dead, so time since respawn is... 
      return(-1.0);
   }
   else
   {
      return(gpGlobals->time - players[idx].last_time_dead);
   }
}


//
qboolean IsPlayerFacingWall(edict_t * pPlayer)
{
   TraceResult tr;
   Vector v_forward, EyePosition;
	
   EyePosition = pPlayer->v.origin + pPlayer->v.view_ofs;
   v_forward = UTIL_AnglesToForward(pPlayer->v.v_angle);
   
   UTIL_TraceLine(EyePosition, EyePosition + gpGlobals->v_forward * 48, ignore_monsters, ignore_glass, pPlayer, &tr);
   
   if (tr.flFraction >= 1.0f) 
      return(FALSE);

   if (DotProduct(gpGlobals->v_forward, tr.vecPlaneNormal) > -0.5f) //60deg
      return(FALSE);

   return(TRUE);
}

//
void CheckPlayerChatProtection(edict_t * pPlayer)
{
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return;
   
   // skip bots
   if (FBitSet(pPlayer->v.flags, FL_FAKECLIENT) || FBitSet(pPlayer->v.flags, FL_THIRDPARTYBOT))
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }
   
   // use of any buttons will reset protection
   if((pPlayer->v.button & ~(IN_SCORE | IN_DUCK)) != 0)
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }
   
   // is not facing wall?
   if(!IsPlayerFacingWall(pPlayer))
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }
   
   // This cannot be checked, because if someone accidentally shoots chatter, chatter will move abit -> resets protection
   /*
   // is moving
   if(pPlayer->v.velocity.Length() > 1.0)
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }*/
}

//
qboolean IsPlayerChatProtected(edict_t * pPlayer)
{
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return(FALSE);
   
   if(players[idx].last_time_not_facing_wall + 2.0 < gpGlobals->time)
   {
      return TRUE;
   }
   
   return FALSE;
}


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
   char  sender_teamstr[MAX_TEAMNAME_LENGTH];
   char  player_teamstr[MAX_TEAMNAME_LENGTH];

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

   UTIL_GetTeam(pEntity, sender_teamstr, sizeof(sender_teamstr));

   client = NULL;
   while((client = UTIL_FindEntityByClassname( client, "player" )) != NULL && !FNullEnt(client))
   {
      if ( client == pEntity )  // skip sender of message
         continue;

      UTIL_GetTeam(client, player_teamstr, sizeof(player_teamstr));

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

   // write to log file
   // team match?
   if ( is_team_play )
   {
      UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n", 
         STRING( pEntity->v.netname ), 
         GETPLAYERUSERID( pEntity ),
         (*g_engfuncs.pfnGetPlayerAuthId)( pEntity ),
         sender_teamstr,
         teamonly ? "say_team" : "say",
         message );
   }
   else
   {
      UTIL_LogPrintf( "\"%s<%i><%s><%i>\" %s \"%s\"\n", 
         STRING( pEntity->v.netname ), 
         GETPLAYERUSERID( pEntity ),
         (*g_engfuncs.pfnGetPlayerAuthId)( pEntity ),
         GETPLAYERUSERID( pEntity ),
         teamonly ? "say_team" : "say",
         message );
   }
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
char * UTIL_GetTeam(edict_t *pEntity, char *teamstr, size_t slen)
{
   safe_strcopy(teamstr, slen, INFOKEY_VALUE(GET_INFOKEYBUFFER(pEntity), "model"));
   
   return(teamstr);
}

qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t ** pHit )
{
   TraceResult tr;
   Vector      vecLookerOrigin;
   
   if(pHit)
      *pHit = NULL;
   
   // look through caller's eyes
   vecLookerOrigin = GetGunPosition(pEdict);

   int bInWater = (POINT_CONTENTS (vecOrigin) == CONTENTS_WATER);
   int bLookerInWater = (POINT_CONTENTS (vecLookerOrigin) == CONTENTS_WATER);
   
   // don't look through water
   if (bInWater != bLookerInWater)
      return FALSE;

   UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

   if(pHit)
      *pHit = tr.pHit;

   return(tr.flFraction >= 1.0f);
}

qboolean FVisibleEnemyOffset( const Vector &vecOrigin, const Vector &vecOffset, edict_t *pEdict, edict_t *pEnemy )
{
   edict_t * pHit = NULL;
   
   if(FVisible(vecOrigin + vecOffset, pEdict, &pHit) || (pEnemy != NULL && pHit == pEnemy))
      return(TRUE);
   
   if(FNullEnt(pHit))
      return(FALSE);
   
   if(!(pHit->v.flags & FL_MONSTER) && !FIsClassname(pHit, "player"))
      return(FALSE);
   
   if(!IsAlive (pHit))
      return(FALSE);
   
   return(TRUE);
}

qboolean FVisibleEnemy( const Vector &vecOrigin, edict_t *pEdict, edict_t *pEnemy )
{
   // only check center if cannot use extra information
   if(!pEnemy)
      return(FVisibleEnemyOffset( vecOrigin, Vector(0, 0, 0), pEdict, pEnemy ));
   
   // first check for if head is visible
   Vector head_offset = Vector(0, 0, pEnemy->v.maxs.z - 6);
   if(FVisibleEnemyOffset( vecOrigin, head_offset, pEdict, pEnemy ))
      return(TRUE);
   
   // then check if feet are visible
   Vector feet_offset = Vector(0, 0, pEnemy->v.mins.z - 6);
   if(FVisibleEnemyOffset( vecOrigin, feet_offset, pEdict, pEnemy ))
      return(TRUE);
   
   // check center
   return(FVisibleEnemyOffset( vecOrigin, Vector(0, 0, 0), pEdict, pEnemy ));
   
   // construct sideways vector
   Vector v_right = UTIL_AnglesToRight(UTIL_VecToAngles(vecOrigin - GetGunPosition(pEdict)));

   // check if right side of player is visible
   Vector right_offset = v_right * (pEnemy->v.maxs.x - 4);
   if(FVisibleEnemyOffset( vecOrigin, right_offset, pEdict, pEnemy ))
      return(TRUE);
   
   // check if left side of player is visible
   Vector left_offset = v_right * (pEnemy->v.mins.x - 4);
   if(FVisibleEnemyOffset( vecOrigin, left_offset, pEdict, pEnemy ))
      return(TRUE);
   
   return(FALSE);
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


void UTIL_BuildFileName_N(char *filename, int size, char *arg1, char *arg2)
{
   const char * mod_dir = (submod_id == SUBMOD_OP4) ? "gearbox" : "valve";
   
   if ((arg1 != NULL) && (arg2 != NULL))
   {
      if (*arg1 && *arg2)
      {
         safevoid_snprintf(filename, size, "%s/%s/%s", mod_dir, arg1, arg2);
         return;
      }
   }

   if (arg1 != NULL)
   {
      if (*arg1)
      {
         safevoid_snprintf(filename, size, "%s/%s", mod_dir, arg1);
         return;
      }
   }
   
   safevoid_snprintf(filename, size, "%s/", mod_dir);
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
   char string[512];
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

void UTIL_AssertConsolePrintf(const char *file, const char *str, int line)
{
   UTIL_ConsolePrintf("[ASSERT] '%s' : '%s' : 'line %d'", file, str, line);
   __asm__ ("int $3");
}

char* UTIL_VarArgs2( char * string, size_t strlen, char *format, ... )
{
   va_list argptr;
   
   va_start (argptr, format);
   safevoid_vsnprintf (string, strlen, format, argptr);
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
