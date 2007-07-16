//
// JK_Botti - be more human!
//
// h_export.cpp
//

#ifndef _WIN32
#include <string.h>
#endif
#include "asm_string.h"

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"


enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;


#ifndef __linux__

// Required DLL entry point
extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   return TRUE;
}

#endif

void WINAPI GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
{
   // get the engine functions from the engine...
   memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
   gpGlobals = pGlobals;
}

