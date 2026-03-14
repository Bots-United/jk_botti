//
// JK_Botti - be more human!
//
// bot_trace.cpp
//

#include <string.h>
#include <stdarg.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_trace.h"
#include "safe_snprintf.h"

extern qboolean is_team_play;

int bot_trace_level = BOT_TRACE_OFF;

static cvar_t jk_botti_trace_cvar = { "jk_botti_trace", "0", FCVAR_EXTDLL|FCVAR_SERVER, 0, NULL};
static qboolean trace_cvar_registered = FALSE;


void BotTraceRegisterCvar(void)
{
   CVAR_REGISTER(&jk_botti_trace_cvar);
   trace_cvar_registered = TRUE;
}


void BotTraceUpdateCache(void)
{
   if (!trace_cvar_registered)
      return;

   int val = (int)CVAR_GET_FLOAT("jk_botti_trace");

   if (val < BOT_TRACE_OFF)
      val = BOT_TRACE_OFF;
   if (val > BOT_TRACE_SAY)
      val = BOT_TRACE_SAY;

   bot_trace_level = val;
}


static void BotTraceLog(bot_t &pBot, const char *message)
{
   edict_t *pEdict = pBot.pEdict;

   if (is_team_play)
   {
      char teamstr[MAX_TEAMNAME_LENGTH];

      UTIL_GetTeam(pEdict, teamstr, sizeof(teamstr));

      UTIL_LogPrintf( "\"%s<%i><%s><%s>\" say \"[TRACE] %s\"\n",
         STRING( pEdict->v.netname ),
         GETPLAYERUSERID( pEdict ),
         (*g_engfuncs.pfnGetPlayerAuthId)( pEdict ),
         teamstr,
         message );
   }
   else
   {
      UTIL_LogPrintf( "\"%s<%i><%s><%i>\" say \"[TRACE] %s\"\n",
         STRING( pEdict->v.netname ),
         GETPLAYERUSERID( pEdict ),
         (*g_engfuncs.pfnGetPlayerAuthId)( pEdict ),
         GETPLAYERUSERID( pEdict ),
         message );
   }
}


static void BotTraceSay(bot_t &pBot, const char *message)
{
   char say_msg[128];

   safevoid_snprintf(say_msg, sizeof(say_msg), "[TRACE] %s", message);

   UTIL_HostSay(pBot.pEdict, 0, say_msg);
}


void BotTracePrintf(bot_t &pBot, const char *fmt, ...)
{
   va_list argptr;
   char message[256];

   va_start(argptr, fmt);
   safevoid_vsnprintf(message, sizeof(message), fmt, argptr);
   va_end(argptr);

   if (bot_trace_level == BOT_TRACE_SAY)
      BotTraceSay(pBot, message);
   else if (bot_trace_level == BOT_TRACE_LOG)
      BotTraceLog(pBot, message);
}
