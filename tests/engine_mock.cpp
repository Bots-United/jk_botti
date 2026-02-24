//
// JK_Botti - engine mock for unit testing
//
// engine_mock.cpp
//
// Provides stub implementations of all engine, metamod, and extern
// globals/functions needed to link bot_combat.o + util.o for testing.
//

#ifndef _WIN32
#include <string.h>
#endif
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "bot_sound.h"
#include "player.h"

#include "engine_mock.h"

// ============================================================
// Mock cvar values
// ============================================================

float mock_cvar_bm_gluon_mod_val = 0.0f;

// ============================================================
// Mock edict pool
// ============================================================

edict_t mock_edicts[MOCK_MAX_EDICTS];
static int mock_next_edict = 1; // 0 = worldspawn, start allocating from 1

edict_t *mock_alloc_edict(void)
{
   if (mock_next_edict >= MOCK_MAX_EDICTS)
      return NULL;
   edict_t *e = &mock_edicts[mock_next_edict++];
   memset(e, 0, sizeof(*e));
   e->v.pContainingEntity = e;
   return e;
}

void mock_set_classname(edict_t *e, const char *name)
{
   // STRING(offset) = (const char*)(gpGlobals->pStringBase + offset)
   // With pStringBase=NULL, STRING(x) = (const char*)(0 + x) = (const char*)x
   // So we store the raw pointer as the string_t value.
   e->v.classname = (string_t)(long)name;
}

// ============================================================
// Mock trace hooks
// ============================================================

mock_trace_fn mock_trace_hull_fn = NULL;
mock_trace_fn mock_trace_line_fn = NULL;
mock_point_contents_fn_t mock_point_contents_fn = NULL;

// ============================================================
// Engine globals
// ============================================================

static globalvars_t mock_globalvars;
globalvars_t *gpGlobals = &mock_globalvars;

enginefuncs_t g_engfuncs;

// Metamod globals
static meta_globals_t mock_metaglobals;
meta_globals_t *gpMetaGlobals = &mock_metaglobals;

static mutil_funcs_t mock_mutil_funcs;
mutil_funcs_t *gpMetaUtilFuncs = &mock_mutil_funcs;

static DLL_FUNCTIONS mock_dll_functions;
static gamedll_funcs_t mock_gamedll_funcs = { &mock_dll_functions, NULL };
gamedll_funcs_t *gpGamedllFuncs = &mock_gamedll_funcs;

plugin_info_t Plugin_info;

// ============================================================
// Extern globals from production .cpp files
// ============================================================

bot_weapon_t weapon_defs[MAX_WEAPONS];
bot_t bots[32];
player_t players[32];
qboolean b_observer_mode = FALSE;
qboolean is_team_play = FALSE;
qboolean checked_teamplay = FALSE;
qboolean b_botdontshoot = FALSE;
int num_logos = 0;
int submod_id = SUBMOD_HLDM;
int submod_weaponflag = WEAPON_SUBMOD_HLDM;
int bot_shoot_breakables = 0;
int m_spriteTexture = 0;
__attribute__((weak)) int num_waypoints = 0;
__attribute__((weak)) WAYPOINT waypoints[MAX_WAYPOINTS];
bot_skill_settings_t skill_settings[5];
__attribute__((weak)) CSoundEnt *pSoundEnt = NULL;

// ============================================================
// Engine function stubs
// ============================================================

static int mock_pfnEntOffsetOfPEntity(const edict_t *pEdict)
{
   // Return byte offset from mock_edicts[0], so worldspawn (edict 0) returns 0.
   // This makes FNullEnt(NULL) and FNullEnt(&mock_edicts[0]) both return TRUE,
   // matching real engine behavior.
   if (!pEdict) return 0;
   return (int)((const char *)pEdict - (const char *)&mock_edicts[0]);
}

static edict_t *mock_pfnPEntityOfEntOffset(int offset)
{
   return (edict_t *)((char *)&mock_edicts[0] + offset);
}

static int mock_pfnIndexOfEdict(const edict_t *pEdict)
{
   if (!pEdict) return 0;
   for (int i = 0; i < MOCK_MAX_EDICTS; i++)
      if (&mock_edicts[i] == pEdict)
         return i;
   return 0;
}

static edict_t *mock_pfnPEntityOfEntIndex(int iEntIndex)
{
   if (iEntIndex < 0 || iEntIndex >= MOCK_MAX_EDICTS)
      return NULL;
   return &mock_edicts[iEntIndex];
}

static void mock_pfnTraceLine(const float *v1, const float *v2,
                              int fNoMonsters, edict_t *pentToSkip,
                              TraceResult *ptr)
{
   memset(ptr, 0, sizeof(*ptr));
   if (mock_trace_line_fn)
   {
      mock_trace_line_fn(v1, v2, fNoMonsters, 0, pentToSkip, ptr);
      return;
   }
   // Default: no hit
   ptr->flFraction = 1.0f;
   ptr->pHit = NULL;
   ptr->vecEndPos[0] = v2[0];
   ptr->vecEndPos[1] = v2[1];
   ptr->vecEndPos[2] = v2[2];
}

static void mock_pfnTraceHull(const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
{
   memset(ptr, 0, sizeof(*ptr));
   if (mock_trace_hull_fn)
   {
      mock_trace_hull_fn(v1, v2, fNoMonsters, hullNumber, pentToSkip, ptr);
      return;
   }
   // Default: no hit
   ptr->flFraction = 1.0f;
   ptr->pHit = NULL;
   ptr->vecEndPos[0] = v2[0];
   ptr->vecEndPos[1] = v2[1];
   ptr->vecEndPos[2] = v2[2];
}

static edict_t *mock_pfnFindEntityInSphere(edict_t *pentStart,
                                           const float *origin,
                                           float radius)
{
   // Start scanning from pentStart+1 (or edict 1 if pentStart is NULL/worldspawn)
   int start = 1;
   if (pentStart)
   {
      for (int i = 0; i < MOCK_MAX_EDICTS; i++)
         if (&mock_edicts[i] == pentStart)
         { start = i + 1; break; }
   }

   for (int i = start; i < mock_next_edict; i++)
   {
      edict_t *e = &mock_edicts[i];
      if (e->v.classname == 0) continue;

      float dx = e->v.origin[0] - origin[0];
      float dy = e->v.origin[1] - origin[1];
      float dz = e->v.origin[2] - origin[2];
      float dist = sqrtf(dx*dx + dy*dy + dz*dz);

      if (dist <= radius)
         return e;
   }

   return NULL;
}

static edict_t *mock_pfnFindEntityByString(edict_t *pentStart,
                                           const char *szKeyword,
                                           const char *szValue)
{
   // Search mock_edicts by classname
   int start = 0;
   if (pentStart)
   {
      for (int i = 0; i < MOCK_MAX_EDICTS; i++)
         if (&mock_edicts[i] == pentStart)
         { start = i + 1; break; }
   }

   if (strcmp(szKeyword, "classname") == 0)
   {
      for (int i = start; i < MOCK_MAX_EDICTS; i++)
      {
         edict_t *e = &mock_edicts[i];
         if (e->v.classname == 0) continue;
         if (strcmp(STRING(e->v.classname), szValue) == 0)
            return e;
      }
   }

   // Return NULL to signal end of search
   return NULL;
}

static int mock_pfnPointContents(const float *origin)
{
   if (mock_point_contents_fn)
      return mock_point_contents_fn(origin);
   return CONTENTS_EMPTY;
}

static void mock_pfnSetOrigin(edict_t *e, const float *rgflOrigin)
{
   if (e)
   {
      e->v.origin[0] = rgflOrigin[0];
      e->v.origin[1] = rgflOrigin[1];
      e->v.origin[2] = rgflOrigin[2];
   }
}

static void mock_pfnEmitSound(edict_t *entity, int channel, const char *sample,
                               float volume, float attenuation, int flags, int pitch)
{
   (void)entity; (void)channel; (void)sample; (void)volume;
   (void)attenuation; (void)flags; (void)pitch;
}

static void mock_pfnAlertMessage(ALERT_TYPE atype, char *szFmt, ...)
{
   (void)atype; (void)szFmt;
}

static void mock_pfnMessageBegin(int msg_dest, int msg_type,
                                 const float *pOrigin, edict_t *ed)
{
   (void)msg_dest; (void)msg_type; (void)pOrigin; (void)ed;
}

static void mock_pfnMessageEnd(void) {}
static void mock_pfnWriteByte(int val) { (void)val; }
static void mock_pfnWriteChar(int val) { (void)val; }
static void mock_pfnWriteShort(int val) { (void)val; }
static void mock_pfnWriteLong(int val) { (void)val; }
static void mock_pfnWriteAngle(float val) { (void)val; }
static void mock_pfnWriteCoord(float val) { (void)val; }
static void mock_pfnWriteString(const char *s) { (void)s; }
static void mock_pfnWriteEntity(int val) { (void)val; }

static char *mock_pfnGetInfoKeyBuffer(edict_t *e)
{
   static char buf[256] = "";
   (void)e;
   return buf;
}

static char *mock_pfnInfoKeyValue(char *infobuffer, char *key)
{
   static char empty[] = "";
   (void)infobuffer; (void)key;
   return empty;
}

static int mock_pfnGetPlayerUserId(edict_t *e)
{
   if (!e) return 0;
   // Return ENTINDEX if edict is allocated (within mock_next_edict range)
   for (int i = 1; i < mock_next_edict && i < MOCK_MAX_EDICTS; i++)
      if (&mock_edicts[i] == e)
         return i; // non-zero = valid user
   return 0;
}

static void mock_pfnMakeVectors(const float *angles)
{
   // Simple stub: set v_forward to 1,0,0
   gpGlobals->v_forward[0] = 1; gpGlobals->v_forward[1] = 0; gpGlobals->v_forward[2] = 0;
   gpGlobals->v_right[0] = 0;   gpGlobals->v_right[1] = 1;   gpGlobals->v_right[2] = 0;
   gpGlobals->v_up[0] = 0;      gpGlobals->v_up[1] = 0;      gpGlobals->v_up[2] = 1;
   (void)angles;
}

static void mock_pfnServerPrint(const char *msg)
{
   (void)msg;
}

static void mock_pfnClientPrintf(edict_t *pEdict, PRINT_TYPE ptype, const char *szMsg)
{
   (void)pEdict; (void)ptype; (void)szMsg;
}

static int mock_pfnRegUserMsg(const char *name, int size)
{
   (void)name; (void)size;
   return 1;
}

static const char *mock_pfnGetPlayerAuthId(edict_t *e)
{
   static char auth[] = "MOCK";
   (void)e;
   return auth;
}

static int mock_pfnRandomLong(int low, int high)
{
   if (low >= high) return low;
   return low;
}

static float mock_pfnRandomFloat(float low, float high)
{
   if (low >= high) return low;
   return low;
}

static void mock_pfnParticleEffect(const float *org, const float *dir, float color, float count)
{
   (void)org; (void)dir; (void)color; (void)count;
}

static float mock_pfnCVarGetFloat(const char *szVarName)
{
   if (strcmp(szVarName, "bm_gluon_mod") == 0)
      return mock_cvar_bm_gluon_mod_val;
   return 0.0f;
}

// DLL function stubs (MDLL_CmdStart/CmdEnd for UTIL_SelectWeapon)
static void mock_pfnCmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed)
{ (void)player; (void)cmd; (void)random_seed; }
static void mock_pfnCmdEnd(const edict_t *player)
{ (void)player; }

// Metamod util stubs
static int mock_mutil_GetUserMsgID(plid_t plid, const char *msgname, int *size)
{
   (void)plid; (void)msgname;
   if (size) *size = -1;
   return 1;
}

// ============================================================
// Stub functions from other .cpp files
// ============================================================

// waypoint.cpp (weak: overridden by test_waypoint.cpp which #includes waypoint.cpp)
__attribute__((weak)) int WaypointFindNearest(const Vector &v_origin, const Vector &v_offset,
                        edict_t *pEntity, float range, qboolean b_traceline)
{ (void)v_origin; (void)v_offset; (void)pEntity; (void)range; (void)b_traceline; return -1; }
__attribute__((weak)) int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int itemflags,
                            int exclude[], float range, const Vector *pv_src)
{ (void)pEntity; (void)src; (void)flags; (void)itemflags; (void)exclude;
  (void)range; (void)pv_src; return -1; }
__attribute__((weak)) int WaypointFindRandomGoal(int *out_indexes, int max_indexes, edict_t *pEntity,
                           int flags, int itemflags, int exclude[])
{ (void)out_indexes; (void)max_indexes; (void)pEntity; (void)flags;
  (void)itemflags; (void)exclude; return 0; }
__attribute__((weak)) int WaypointFindReachable(edict_t *pEntity, float range)
{ (void)pEntity; (void)range; return -1; }
__attribute__((weak)) edict_t *WaypointFindItem(int wpt_index)
{ (void)wpt_index; return NULL; }
__attribute__((weak)) int WaypointRouteFromTo(int src, int dest)
{ (void)src; (void)dest; return -1; }
__attribute__((weak)) float WaypointDistanceFromTo(int src, int dest)
{ (void)src; (void)dest; return 99999.0f; }
__attribute__((weak)) int WaypointFindPath(int &path_index, int waypoint_index)
{ (void)path_index; (void)waypoint_index; return -1; }

// bot_navigate.cpp (weak: overridden by test_bot_navigate.cpp which links bot_navigate.o)
__attribute__((weak)) qboolean BotUpdateTrackSoundGoal(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) float BotChangeYaw(bot_t &pBot, float speed) { (void)pBot; (void)speed; return 0; }
__attribute__((weak)) float BotChangePitch(bot_t &pBot, float speed) { (void)pBot; (void)speed; return 0; }
__attribute__((weak)) qboolean BotHeadTowardWaypoint(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) qboolean BotStuckInCorner(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) qboolean BotCantMoveForward(bot_t &pBot, TraceResult *tr) { (void)pBot; (void)tr; return FALSE; }
__attribute__((weak)) qboolean BotCanJumpUp(bot_t &pBot, qboolean *bDuckJump) { (void)pBot; (void)bDuckJump; return FALSE; }
__attribute__((weak)) qboolean BotCanDuckUnder(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) qboolean BotEdgeForward(bot_t &pBot, const Vector &v) { (void)pBot; (void)v; return FALSE; }
__attribute__((weak)) qboolean BotEdgeRight(bot_t &pBot, const Vector &v) { (void)pBot; (void)v; return FALSE; }
__attribute__((weak)) qboolean BotEdgeLeft(bot_t &pBot, const Vector &v) { (void)pBot; (void)v; return FALSE; }
__attribute__((weak)) qboolean BotCheckWallOnLeft(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) qboolean BotCheckWallOnRight(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) qboolean BotCheckWallOnForward(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) qboolean BotCheckWallOnBack(bot_t &pBot) { (void)pBot; return FALSE; }
__attribute__((weak)) void BotLookForDrop(bot_t &pBot) { (void)pBot; }
__attribute__((weak)) void BotRemoveEnemy(bot_t &pBot, qboolean b_keep_tracking)
{ (void)pBot; (void)b_keep_tracking; }

// bot_sound.cpp (weak: overridden by test_bot_sound.cpp which links bot_sound.o)
__attribute__((weak)) void CSound::Clear(void) {}
__attribute__((weak)) void CSound::Reset(void) {}
__attribute__((weak)) void CSoundEnt::Initialize(void) {}
__attribute__((weak)) int CSoundEnt::ActiveList(void) { return SOUNDLIST_EMPTY; }
__attribute__((weak)) int CSoundEnt::FreeList(void) { return SOUNDLIST_EMPTY; }
__attribute__((weak)) CSound *CSoundEnt::SoundPointerForIndex(int iIndex) { (void)iIndex; return NULL; }
__attribute__((weak)) void CSoundEnt::InsertSound(edict_t *pEdict, int channel, const Vector &vecOrigin,
   int iVolume, float flDuration, int iBotOwner)
{ (void)pEdict; (void)channel; (void)vecOrigin; (void)iVolume; (void)flDuration; (void)iBotOwner; }
__attribute__((weak)) void CSoundEnt::FreeSound(int iSound, int iPrevious) { (void)iSound; (void)iPrevious; }
__attribute__((weak)) CSound *CSoundEnt::GetEdictChannelSound(edict_t *pEdict, int iChannel)
{ (void)pEdict; (void)iChannel; return NULL; }
__attribute__((weak)) int CSoundEnt::ClientSoundIndex(edict_t *pClient) { (void)pClient; return -1; }
__attribute__((weak)) void SaveSound(edict_t *pEdict, const Vector &origin, int volume, int channel, float flDuration)
{ (void)pEdict; (void)origin; (void)volume; (void)channel; (void)flDuration; }

// commands.cpp
void FakeClientCommand(edict_t *pBot, const char *arg1, const char *arg2, const char *arg3)
{ (void)pBot; (void)arg1; (void)arg2; (void)arg3; }
const cfg_bot_record_t *GetUnusedCfgBotRecord(void) { return NULL; }

// dll.cpp
void jkbotti_ClientPutInServer(edict_t *pEntity) { (void)pEntity; }
BOOL jkbotti_ClientConnect(edict_t *pEntity, const char *pszName,
                           const char *pszAddress, char szRejectReason[128])
{ (void)pEntity; (void)pszName; (void)pszAddress; (void)szRejectReason; return TRUE; }

// bot.cpp
void BotCreate(const char *skin, const char *name, int skill, int top_color,
               int bottom_color, int cfg_bot_index)
{ (void)skin; (void)name; (void)skill; (void)top_color; (void)bottom_color; (void)cfg_bot_index; }
void BotThink(bot_t &pBot) { (void)pBot; }
void BotCheckTeamplay(void) {}
void BotKick(bot_t &pBot) { (void)pBot; }

// bot_chat.cpp (weak: overridden by test_bot_chat.cpp which #includes bot_chat.cpp)
__attribute__((weak)) void LoadBotChat(void) {}
__attribute__((weak)) void BotChatTaunt(bot_t &pBot, edict_t *victim_edict) { (void)pBot; (void)victim_edict; }
__attribute__((weak)) void BotChatWhine(bot_t &pBot) { (void)pBot; }
__attribute__((weak)) void BotChatTalk(bot_t &pBot) { (void)pBot; }
__attribute__((weak)) void BotChatEndGame(bot_t &pBot) { (void)pBot; }

// bot_skill.cpp
void ResetSkillsToDefault(void) {}

// bot_models.cpp
void LoadBotModels(void) {}

// dlls/util.h (EMIT_SOUND_DYN not used by bot_combat.o/util.o but may be needed)
void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample,
                    float volume, float attenuation, int flags, int pitch)
{ (void)entity; (void)channel; (void)sample; (void)volume;
  (void)attenuation; (void)flags; (void)pitch; }

// ============================================================
// mock_add_breakable - register a breakable via real UTIL_UpdateFuncBreakable
// ============================================================

void mock_add_breakable(edict_t *pEdict, int material_breakable)
{
   // The breakable list is managed by UTIL_UpdateFuncBreakable in util.cpp.
   // We call it with "material" key to register the breakable.
   // material value: 7 = matUnbreakableGlass, anything else = breakable
   const char *mat_val = material_breakable ? "0" : "7"; // 0=matGlass (breakable)
   UTIL_UpdateFuncBreakable(pEdict, "material", mat_val);
}

// ============================================================
// mock_reset - reset all mock state between tests
// ============================================================

void mock_reset(void)
{
   // Clear edict pool
   memset(mock_edicts, 0, sizeof(mock_edicts));
   mock_next_edict = 1;

   // Worldspawn edict (index 0) - acts as "null"
   mock_edicts[0].v.pContainingEntity = &mock_edicts[0];

   // Clear trace hooks
   mock_trace_hull_fn = NULL;
   mock_trace_line_fn = NULL;
   mock_point_contents_fn = NULL;

   // Clear mock cvar values
   mock_cvar_bm_gluon_mod_val = 0.0f;

   // Clear extern globals
   memset(bots, 0, sizeof(bots));
   memset(players, 0, sizeof(players));
   num_waypoints = 0;
   memset(waypoints, 0, sizeof(waypoints));
   memset(skill_settings, 0, sizeof(skill_settings));
   b_observer_mode = FALSE;
   is_team_play = FALSE;
   b_botdontshoot = FALSE;
   bot_shoot_breakables = 0;

   // Reset breakable list
   UTIL_FreeFuncBreakables();

   // Reset globalvars
   memset(&mock_globalvars, 0, sizeof(mock_globalvars));
   mock_globalvars.maxClients = 32;
   // pStringBase = NULL (0) so STRING(offset) = (const char*)(0 + offset) = raw pointer
   mock_globalvars.pStringBase = NULL;
   mock_globalvars.time = 10.0; // some non-zero time

   // Initialize engine function pointers
   memset(&g_engfuncs, 0, sizeof(g_engfuncs));
   g_engfuncs.pfnEntOffsetOfPEntity = mock_pfnEntOffsetOfPEntity;
   g_engfuncs.pfnPEntityOfEntOffset = mock_pfnPEntityOfEntOffset;
   g_engfuncs.pfnIndexOfEdict = mock_pfnIndexOfEdict;
   g_engfuncs.pfnPEntityOfEntIndex = mock_pfnPEntityOfEntIndex;
   g_engfuncs.pfnTraceLine = mock_pfnTraceLine;
   g_engfuncs.pfnTraceHull = mock_pfnTraceHull;
   g_engfuncs.pfnFindEntityInSphere = mock_pfnFindEntityInSphere;
   g_engfuncs.pfnFindEntityByString = mock_pfnFindEntityByString;
   g_engfuncs.pfnPointContents = mock_pfnPointContents;
   g_engfuncs.pfnSetOrigin = mock_pfnSetOrigin;
   g_engfuncs.pfnEmitSound = mock_pfnEmitSound;
   g_engfuncs.pfnAlertMessage = mock_pfnAlertMessage;
   g_engfuncs.pfnMessageBegin = mock_pfnMessageBegin;
   g_engfuncs.pfnMessageEnd = mock_pfnMessageEnd;
   g_engfuncs.pfnWriteByte = mock_pfnWriteByte;
   g_engfuncs.pfnWriteChar = mock_pfnWriteChar;
   g_engfuncs.pfnWriteShort = mock_pfnWriteShort;
   g_engfuncs.pfnWriteLong = mock_pfnWriteLong;
   g_engfuncs.pfnWriteAngle = mock_pfnWriteAngle;
   g_engfuncs.pfnWriteCoord = mock_pfnWriteCoord;
   g_engfuncs.pfnWriteString = mock_pfnWriteString;
   g_engfuncs.pfnWriteEntity = mock_pfnWriteEntity;
   g_engfuncs.pfnGetInfoKeyBuffer = mock_pfnGetInfoKeyBuffer;
   g_engfuncs.pfnInfoKeyValue = mock_pfnInfoKeyValue;
   g_engfuncs.pfnGetPlayerUserId = mock_pfnGetPlayerUserId;
   g_engfuncs.pfnMakeVectors = mock_pfnMakeVectors;
   g_engfuncs.pfnServerPrint = mock_pfnServerPrint;
   g_engfuncs.pfnClientPrintf = mock_pfnClientPrintf;
   g_engfuncs.pfnRegUserMsg = mock_pfnRegUserMsg;
   g_engfuncs.pfnGetPlayerAuthId = mock_pfnGetPlayerAuthId;
   g_engfuncs.pfnRandomLong = mock_pfnRandomLong;
   g_engfuncs.pfnRandomFloat = mock_pfnRandomFloat;
   g_engfuncs.pfnCVarGetFloat = mock_pfnCVarGetFloat;
   g_engfuncs.pfnParticleEffect = mock_pfnParticleEffect;

   // Re-initialize sound entity if present
   if (pSoundEnt)
      pSoundEnt->Initialize();

   // Initialize metamod util function pointers
   memset(&mock_mutil_funcs, 0, sizeof(mock_mutil_funcs));
   mock_mutil_funcs.pfnGetUserMsgID = mock_mutil_GetUserMsgID;

   // Initialize gamedll funcs (for MDLL_CmdStart/CmdEnd)
   memset(&mock_dll_functions, 0, sizeof(mock_dll_functions));
   mock_dll_functions.pfnCmdStart = mock_pfnCmdStart;
   mock_dll_functions.pfnCmdEnd = mock_pfnCmdEnd;
}
