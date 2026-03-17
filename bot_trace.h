//
// JK_Botti - be more human!
//
// bot_trace.h
//

#ifndef BOT_TRACE_H
#define BOT_TRACE_H

#include "safe_snprintf.h"

// Trace logging levels
#define BOT_TRACE_OFF      0
#define BOT_TRACE_LOG      1  // log to server log only (UTIL_LogPrintf)
#define BOT_TRACE_SAY      2  // log via bot say (visible to players + server log)

// Cached trace level, updated once per frame in StartFrame.
// All trace check sites use this variable, never the cvar directly.
extern int bot_trace_level;

// Register the jk_botti_trace cvar. Call once in Meta_Attach.
void BotTraceRegisterCvar(void);

// Update cached trace level from cvar. Call once per frame in StartFrame.
void BotTraceUpdateCache(void);

// Log a trace message for a bot. Format string should not include newline.
// Use the BotTrace() macro at call sites — it checks bot_trace_level before
// calling the function, avoiding all call overhead when tracing is off.
void BotTracePrintf(bot_t &pBot, const char *fmt, ...);

// Convert WPT_GOAL_* constant to readable string
const char *BotTraceGoalTypeName(int goal_type);

// Build compact "weapon:ammo weapon:ammo ..." summary of bot's inventory
void BotTraceAmmoSummary(bot_t &pBot, char *buf, size_t bufsize);

#define BotTrace(pBot, fmt, ...) \
   do { if (bot_trace_level > 0) BotTracePrintf(pBot, fmt, ##__VA_ARGS__); } while(0)

// Format a string only when tracing is active. Skips snprintf overhead when off.
#define BotTraceFormat(buf, bufsize, fmt, ...) \
   do { if (bot_trace_level > 0) safevoid_snprintf(buf, bufsize, fmt, ##__VA_ARGS__); \
        else buf[0] = '\0'; } while(0)

#endif // BOT_TRACE_H
