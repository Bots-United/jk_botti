//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
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

char g_argv[1024];

DLL_FUNCTIONS gFunctionTable;
enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;

static FILE *fp;
char z_welcome_msg[] = "HPB bot - http://planethalflife.com/botman";


extern int mod_id;


#ifndef __linux__

// Required DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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
      mod_id = VALVE_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "valve\\dlls\\hl.dll");
#else
      strcpy(game_dll_filename, "valve/dlls/hl_i386.so");
#endif
   }
   else if (strcmpi(mod_name, "tfc") == 0)
   {
      mod_id = TFC_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "tfc\\dlls\\tfc.dll");
#else
      strcpy(game_dll_filename, "tfc/dlls/tfc_i386.so");
#endif
   }
   else if (strcmpi(mod_name, "cstrike") == 0)
   {
      mod_id = CSTRIKE_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "cstrike\\dlls\\mp.dll");
#else
      strcpy(game_dll_filename, "cstrike/dlls/cs_i386.so");
#endif
   }
   else if (strcmpi(mod_name, "gearbox") == 0)
   {
      mod_id = GEARBOX_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "gearbox\\dlls\\opfor.dll");
#else
      strcpy(game_dll_filename, "gearbox/dlls/opfor_i386.so");
#endif
   }
   else if (strcmpi(mod_name, "frontline") == 0)
   {
      mod_id = FRONTLINE_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "frontline\\dlls\\frontline.dll");
#else
      strcpy(game_dll_filename, "frontline/dlls/front_i386.so");
#endif
   }
   else if (strcmpi(mod_name, "holywars") == 0)
   {
      mod_id = HOLYWARS_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "holywars\\dlls\\holywars.dll");
#else
      strcpy(game_dll_filename, "holywars/dlls/holywars_i386.so");
#endif
   }
   else if (strcmpi(mod_name, "dmc") == 0)
   {
      mod_id = DMC_DLL;
#ifndef __linux__
      strcpy(game_dll_filename, "dmc\\dlls\\dmc.dll");
#else
      strcpy(game_dll_filename, "dmc/dlls/dmc_i386.so");
#endif
   }
}

