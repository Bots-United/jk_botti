//
// JK_Botti - be more human!
//
// h_export.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"


DLL_FUNCTIONS gFunctionTable;
DLL_FUNCTIONS gFunctionTable_POST;
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
   char game_dir[256];
   char mod_name[32];
   char game_dll_filename[256];

   // get the engine functions from the engine...

   memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
   gpGlobals = pGlobals;

   // find the directory name of the currently running MOD...
   GetGameDir (game_dir);

   strcpy(mod_name, game_dir);

   game_dll_filename[0] = 0;

   if (strcmpi(mod_name, "valve") == 0)
   {
#ifndef __linux__
      strcpy(game_dll_filename, "valve\\dlls\\hl.dll");
#else
      strcpy(game_dll_filename, "valve/dlls/hl_i386.so");
#endif
   }
}

