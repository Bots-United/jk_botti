//
// JK_Botti - be more human!
//
// bot_start.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"


void BotStartGame( bot_t &pBot )
{
   // don't need to do anything to start game...
   pBot.not_started = 0;
}

