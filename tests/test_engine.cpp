//
// JK_Botti - comprehensive tests for engine.cpp
//
// test_engine.cpp
//

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "bot_sound.h"
#include "bot_weapons.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Stubs for symbols not in engine_mock or linked .o files
// ============================================================

// commands.cpp globals
qboolean isFakeClientCommand = FALSE;
int fake_arg_count = 0;

// engine.cpp declares these externs
int get_cvars = 0;
qboolean aim_fix = FALSE;
float turn_skill = 3.0;

// ============================================================
// Tracking stubs for BotClient_Valve_* functions
// ============================================================

static int stub_call_count = 0;
static int stub_last_int = 0;
static float stub_last_float = 0;
static const char *stub_last_str = NULL;
static int stub_last_bot_index = -1;
static void (*stub_last_function)(void *, int) = NULL;

static void reset_stubs(void)
{
   stub_call_count = 0;
   stub_last_int = 0;
   stub_last_float = 0;
   stub_last_str = NULL;
   stub_last_bot_index = -1;
   stub_last_function = NULL;
}

#define DEFINE_TRACKING_STUB(name) \
   void name(void *p, int bot_index) { \
      stub_call_count++; \
      stub_last_int = p ? *(int*)p : 0; \
      stub_last_float = p ? *(float*)p : 0; \
      stub_last_str = (const char*)p; \
      stub_last_bot_index = bot_index; \
      stub_last_function = (void(*)(void*,int))name; \
   }

DEFINE_TRACKING_STUB(BotClient_Valve_WeaponList)
DEFINE_TRACKING_STUB(BotClient_Valve_CurrentWeapon)
DEFINE_TRACKING_STUB(BotClient_Valve_AmmoX)
DEFINE_TRACKING_STUB(BotClient_Valve_AmmoPickup)
DEFINE_TRACKING_STUB(BotClient_Valve_WeaponPickup)
DEFINE_TRACKING_STUB(BotClient_Valve_ItemPickup)
DEFINE_TRACKING_STUB(BotClient_Valve_Health)
DEFINE_TRACKING_STUB(BotClient_Valve_Battery)
DEFINE_TRACKING_STUB(BotClient_Valve_Damage)
DEFINE_TRACKING_STUB(BotClient_Valve_DeathMsg)
DEFINE_TRACKING_STUB(BotClient_Valve_ScreenFade)

// ============================================================
// Tracking stub for BotKick (engine_mock has weak definition)
// ============================================================

static int botkick_call_count = 0;
static int botkick_last_index = -1;

void BotKick(bot_t &pBot)
{
   botkick_call_count++;
   for (int i = 0; i < 32; i++)
   {
      if (&bots[i] == &pBot)
      {
         botkick_last_index = i;
         break;
      }
   }
}

static void reset_botkick(void)
{
   botkick_call_count = 0;
   botkick_last_index = -1;
}

// ============================================================
// Custom mock for GetUserMsgID that returns unique IDs
// ============================================================

#define MOCK_MSG_WEAPONLIST   101
#define MOCK_MSG_CURWEAPON    102
#define MOCK_MSG_AMMOX        103
#define MOCK_MSG_AMMOPICKUP   104
#define MOCK_MSG_WEAPPICKUP   105
#define MOCK_MSG_ITEMPICKUP   106
#define MOCK_MSG_HEALTH       107
#define MOCK_MSG_BATTERY      108
#define MOCK_MSG_DAMAGE       109
#define MOCK_MSG_SCREENFADE   110
#define MOCK_MSG_DEATHMSG     111

static int custom_GetUserMsgID(plid_t plid, const char *msgname, int *size)
{
   (void)plid;
   if (size) *size = -1;

   if (strcmp(msgname, "WeaponList") == 0) return MOCK_MSG_WEAPONLIST;
   if (strcmp(msgname, "CurWeapon") == 0) return MOCK_MSG_CURWEAPON;
   if (strcmp(msgname, "AmmoX") == 0) return MOCK_MSG_AMMOX;
   if (strcmp(msgname, "AmmoPickup") == 0) return MOCK_MSG_AMMOPICKUP;
   if (strcmp(msgname, "WeapPickup") == 0) return MOCK_MSG_WEAPPICKUP;
   if (strcmp(msgname, "ItemPickup") == 0) return MOCK_MSG_ITEMPICKUP;
   if (strcmp(msgname, "Health") == 0) return MOCK_MSG_HEALTH;
   if (strcmp(msgname, "Battery") == 0) return MOCK_MSG_BATTERY;
   if (strcmp(msgname, "Damage") == 0) return MOCK_MSG_DAMAGE;
   if (strcmp(msgname, "ScreenFade") == 0) return MOCK_MSG_SCREENFADE;
   if (strcmp(msgname, "DeathMsg") == 0) return MOCK_MSG_DEATHMSG;
   return 0;
}

// ============================================================
// Externs from engine.cpp
// ============================================================

extern "C" int GetEngineFunctions(enginefuncs_t *, int *);
extern "C" int GetEngineFunctions_POST(enginefuncs_t *, int *);

extern void (*botMsgFunction)(void *, int);
extern void (*botMsgEndFunction)(void *, int);
extern int botMsgIndex;
extern qboolean g_in_intermission;
extern char g_argv[1024*3];
extern char g_arg1[1024];
extern char g_arg2[1024];
extern char g_arg3[1024];

// ============================================================
// Helper functions
// ============================================================

static enginefuncs_t g_eng_hooks;
static enginefuncs_t g_eng_hooks_post;

static void init_engine_tables(void)
{
   int ver = 0;
   memset(&g_eng_hooks, 0, sizeof(g_eng_hooks));
   memset(&g_eng_hooks_post, 0, sizeof(g_eng_hooks_post));
   GetEngineFunctions(&g_eng_hooks, &ver);
   GetEngineFunctions_POST(&g_eng_hooks_post, &ver);
}

// Set up a bot at a given slot with an edict
static edict_t *setup_bot(int index)
{
   edict_t *e = mock_alloc_edict();
   e->v.flags = FL_FAKECLIENT | FL_CLIENT;
   bots[index].is_used = TRUE;
   bots[index].pEdict = e;
   bots[index].f_recoil = 0.0f;
   bots[index].f_max_speed = 0.0f;
   return e;
}

// Set up a non-bot player edict
static edict_t *setup_player(void)
{
   edict_t *e = mock_alloc_edict();
   e->v.flags = FL_CLIENT;
   return e;
}

// Count active sounds in CSoundEnt
static int count_active_sounds(void)
{
   if (!pSoundEnt)
      return 0;
   return pSoundEnt->ISoundsInList(SOUNDLISTTYPE_ACTIVE);
}

// Get the first active sound (head of active list)
static CSound *get_first_active_sound(void)
{
   int idx = CSoundEnt::ActiveList();
   if (idx == SOUNDLIST_EMPTY)
      return NULL;
   return CSoundEnt::SoundPointerForIndex(idx);
}

// Common mock_reset that also sets up our custom GetUserMsgID
static void test_reset(void)
{
   mock_reset();
   // Re-apply custom GetUserMsgID after mock_reset resets mutil_funcs
   gpMetaUtilFuncs->pfnGetUserMsgID = custom_GetUserMsgID;
}

// ============================================================
// Tests: GetEngineFunctions
// ============================================================

static int test_GetEngineFunctions_returns_TRUE(void)
{
   TEST("GetEngineFunctions returns TRUE, table populated");

   enginefuncs_t eng;
   int ver = 0;
   memset(&eng, 0, sizeof(eng));
   int ret = GetEngineFunctions(&eng, &ver);
   ASSERT_INT(ret, TRUE);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnPlaybackEvent);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnChangeLevel);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnEmitSound);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnClientCommand);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnMessageBegin);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnMessageEnd);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteByte);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteChar);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteShort);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteLong);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteAngle);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteCoord);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteString);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnWriteEntity);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnClientPrintf);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnCmd_Args);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnCmd_Argv);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnCmd_Argc);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnSetClientMaxspeed);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnGetPlayerUserId);

   PASS();
   return 0;
}

static int test_GetEngineFunctions_POST_returns_TRUE(void)
{
   TEST("GetEngineFunctions_POST returns TRUE, PrecacheEvent set");

   enginefuncs_t eng;
   int ver = 0;
   memset(&eng, 0, sizeof(eng));
   int ret = GetEngineFunctions_POST(&eng, &ver);
   ASSERT_INT(ret, TRUE);
   ASSERT_PTR_NOT_NULL((void*)eng.pfnPrecacheEvent);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnPrecacheEvent_Post
// ============================================================

static int test_PrecacheEvent_Post_known_event(void)
{
   TEST("PrecacheEvent_Post: known event updates eventindex");

   test_reset();
   gpGlobals->deathmatch = 1;

   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short fake_eventindex = 42;
   gpMetaGlobals->orig_ret = (void *)&fake_eventindex;

   g_eng_hooks_post.pfnPrecacheEvent(1, "events/glock1.sc");

   // Verify by playing back eventindex=42 and checking sound was created
   // glock1 volume=0.96 -> ivolume=960
   edict_t *invoker = mock_alloc_edict();
   invoker->v.origin[0] = 100.0f;

   g_eng_hooks.pfnPlaybackEvent(0, invoker, 42, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   ASSERT_INT(s->m_iVolume, 959); // (int)(1000*0.96f) on x87

   PASS();
   return 0;
}

static int test_PrecacheEvent_Post_unknown_event(void)
{
   TEST("PrecacheEvent_Post: unknown event (no match)");

   test_reset();
   gpGlobals->deathmatch = 1;

   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short fake_eventindex = 99;
   gpMetaGlobals->orig_ret = (void *)&fake_eventindex;

   // Unknown event name - should not crash, no eventindex update
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/unknown_event.sc");

   // Playing eventindex=99 should find no match, no sound created
   edict_t *invoker = mock_alloc_edict();
   g_eng_hooks.pfnPlaybackEvent(0, invoker, 99, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);
   ASSERT_INT(count_active_sounds(), 0);

   PASS();
   return 0;
}

static int test_PrecacheEvent_Post_deathmatch_0(void)
{
   TEST("PrecacheEvent_Post: deathmatch=0 -> ignored");

   test_reset();
   gpGlobals->deathmatch = 0;

   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short fake_eventindex = 77;
   gpMetaGlobals->orig_ret = (void *)&fake_eventindex;

   g_eng_hooks_post.pfnPrecacheEvent(1, "events/glock2.sc");

   // Verify it was NOT registered
   gpGlobals->deathmatch = 1;
   edict_t *invoker = mock_alloc_edict();
   g_eng_hooks.pfnPlaybackEvent(0, invoker, 77, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);
   ASSERT_INT(count_active_sounds(), 0);

   PASS();
   return 0;
}

static int test_PrecacheEvent_Post_override_ret(void)
{
   TEST("PrecacheEvent_Post: MRES_OVERRIDE uses override_ret");

   test_reset();
   gpGlobals->deathmatch = 1;

   gpMetaGlobals->status = MRES_OVERRIDE;
   unsigned short override_idx = 55;
   gpMetaGlobals->override_ret = (void *)&override_idx;

   g_eng_hooks_post.pfnPrecacheEvent(1, "events/shotgun1.sc");

   // Verify eventindex=55 triggers shotgun1 sound (volume=0.975)
   edict_t *invoker = mock_alloc_edict();
   g_eng_hooks.pfnPlaybackEvent(0, invoker, 55, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);
   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   // shotgun1 volume=0.975 -> ivolume=975
   ASSERT_INT(s->m_iVolume, 975);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnPlaybackEvent
// ============================================================

static int test_PlaybackEvent_volume_calls_SaveSound(void)
{
   TEST("PlaybackEvent: event with volume > 0 calls SaveSound");

   test_reset();
   gpGlobals->deathmatch = 1;

   // Register mp5 event (volume=1.0) with eventindex=10
   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short idx = 10;
   gpMetaGlobals->orig_ret = (void *)&idx;
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/mp5.sc");

   edict_t *invoker = mock_alloc_edict();
   invoker->v.origin[0] = 200.0f;
   invoker->v.origin[1] = 300.0f;
   invoker->v.origin[2] = 50.0f;

   g_eng_hooks.pfnPlaybackEvent(0, invoker, 10, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   // mp5 volume=1.0 -> ivolume=1000
   ASSERT_INT(s->m_iVolume, 1000);
   ASSERT_INT(s->m_iChannel, CHAN_WEAPON);

   PASS();
   return 0;
}

static int test_PlaybackEvent_recoil_updates_bot(void)
{
   TEST("PlaybackEvent: event with recoil updates bot f_recoil");

   test_reset();
   gpGlobals->deathmatch = 1;

   // Register glock1 (recoil={-2.0, -2.0}) with eventindex=20
   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short idx = 20;
   gpMetaGlobals->orig_ret = (void *)&idx;
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/glock1.sc");

   edict_t *bot_edict = setup_bot(3);
   bots[3].f_recoil = 0.0f;

   g_eng_hooks.pfnPlaybackEvent(0, bot_edict, 20, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);

   // RANDOM_FLOAT2(-2.0, -2.0) = -2.0
   ASSERT_TRUE(bots[3].f_recoil != 0.0f);
   ASSERT_FLOAT(bots[3].f_recoil, -2.0f);

   PASS();
   return 0;
}

static int test_PlaybackEvent_gauss_duplicate_ignored(void)
{
   TEST("PlaybackEvent: gauss + delay>0 + FEV_RELIABLE -> no recoil");

   test_reset();
   gpGlobals->deathmatch = 1;

   // Register gauss event with eventindex=30
   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short idx = 30;
   gpMetaGlobals->orig_ret = (void *)&idx;
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/gauss.sc");

   edict_t *bot_edict = setup_bot(5);
   bots[5].f_recoil = 0.0f;

   // delay > 0 and FEV_RELIABLE -> duplicate gauss event
   g_eng_hooks.pfnPlaybackEvent(FEV_RELIABLE, bot_edict, 30, 0.01f,
                                 NULL, NULL, 0.0f, 0.0f, 0, 0, 0, 0);

   // Sound should still be created (volume check is before recoil check)
   ASSERT_TRUE(count_active_sounds() > 0);
   // But recoil should NOT be applied (gauss duplicate fix)
   ASSERT_FLOAT(bots[5].f_recoil, 0.0f);

   PASS();
   return 0;
}

static int test_PlaybackEvent_gauss_normal_applies_recoil(void)
{
   TEST("PlaybackEvent: gauss with delay=0 -> recoil applied");

   test_reset();
   gpGlobals->deathmatch = 1;

   // Register gauss event with eventindex=31
   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short idx = 31;
   gpMetaGlobals->orig_ret = (void *)&idx;
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/gauss.sc");

   edict_t *bot_edict = setup_bot(5);
   bots[5].f_recoil = 0.0f;

   // delay=0 -> not duplicate -> recoil applied
   g_eng_hooks.pfnPlaybackEvent(0, bot_edict, 31, 0.0f,
                                 NULL, NULL, 0.0f, 0.0f, 0, 0, 0, 0);

   // gauss recoil={-2.0, -2.0}, RANDOM_FLOAT2(-2.0, -2.0) = -2.0
   ASSERT_FLOAT(bots[5].f_recoil, -2.0f);

   PASS();
   return 0;
}

static int test_PlaybackEvent_non_bot_no_recoil(void)
{
   TEST("PlaybackEvent: non-bot invoker -> no recoil applied");

   test_reset();
   gpGlobals->deathmatch = 1;

   // Register mp5 event (recoil={-2.0, 2.0}) with eventindex=11
   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short idx = 11;
   gpMetaGlobals->orig_ret = (void *)&idx;
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/mp5.sc");

   edict_t *player = setup_player();

   g_eng_hooks.pfnPlaybackEvent(0, player, 11, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);

   // Sound should be generated (volume > 0)
   ASSERT_TRUE(count_active_sounds() > 0);
   // No bots should have recoil
   for (int i = 0; i < 32; i++)
      ASSERT_FLOAT(bots[i].f_recoil, 0.0f);

   PASS();
   return 0;
}

static int test_PlaybackEvent_deathmatch_0(void)
{
   TEST("PlaybackEvent: deathmatch=0 -> ignored");

   test_reset();
   gpGlobals->deathmatch = 0;

   edict_t *invoker = mock_alloc_edict();
   g_eng_hooks.pfnPlaybackEvent(0, invoker, 10, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);

   ASSERT_INT(count_active_sounds(), 0);

   PASS();
   return 0;
}

static int test_PlaybackEvent_zero_volume_no_sound(void)
{
   TEST("PlaybackEvent: event with volume=0 -> no SaveSound");

   test_reset();
   gpGlobals->deathmatch = 1;

   // Register tripfire event (volume=0.0) with eventindex=12
   gpMetaGlobals->status = MRES_IGNORED;
   unsigned short idx = 12;
   gpMetaGlobals->orig_ret = (void *)&idx;
   g_eng_hooks_post.pfnPrecacheEvent(1, "events/tripfire.sc");

   edict_t *invoker = mock_alloc_edict();
   g_eng_hooks.pfnPlaybackEvent(0, invoker, 12, 0.0f, NULL, NULL,
                                 0.0f, 0.0f, 0, 0, 0, 0);

   // tripfire volume=0, recoil={0,0} -> no sound, no recoil
   ASSERT_INT(count_active_sounds(), 0);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnEmitSound
// ============================================================

static int test_EmitSound_item_health(void)
{
   TEST("pfnEmitSound: item_health -> 8s duration");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *ent = mock_alloc_edict();
   mock_set_classname(ent, "item_health");
   ent->v.origin[0] = 10.0f;

   g_eng_hooks.pfnEmitSound(ent, CHAN_ITEM, "items/smallmedkit1.wav",
                             0.5f, 1.0f, 0, 100);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   ASSERT_INT(s->m_iVolume, 500); // 1000 * 0.5
   // duration = 8s -> expire = gpGlobals->time + 8.0
   ASSERT_FLOAT(s->m_flExpireTime, gpGlobals->time + 8.0f);

   PASS();
   return 0;
}

static int test_EmitSound_item_battery(void)
{
   TEST("pfnEmitSound: item_battery -> 8s duration");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *ent = mock_alloc_edict();
   mock_set_classname(ent, "item_battery");

   g_eng_hooks.pfnEmitSound(ent, CHAN_ITEM, "items/gunpickup2.wav",
                             0.8f, 1.0f, 0, 100);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   ASSERT_FLOAT(s->m_flExpireTime, gpGlobals->time + 8.0f);

   PASS();
   return 0;
}

static int test_EmitSound_item_longjump(void)
{
   TEST("pfnEmitSound: item_longjump -> 8s duration");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *ent = mock_alloc_edict();
   mock_set_classname(ent, "item_longjump");

   g_eng_hooks.pfnEmitSound(ent, CHAN_ITEM, "items/pickup.wav",
                             1.0f, 1.0f, 0, 100);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   ASSERT_FLOAT(s->m_flExpireTime, gpGlobals->time + 8.0f);

   PASS();
   return 0;
}

static int test_EmitSound_weapon_item(void)
{
   TEST("pfnEmitSound: weapon item (no owner) -> 8s duration");

   test_reset();
   gpGlobals->deathmatch = 1;
   InitWeaponSelect(SUBMOD_HLDM);

   edict_t *ent = mock_alloc_edict();
   mock_set_classname(ent, "weapon_crowbar");
   ent->v.owner = NULL; // no owner - pickup item on ground

   g_eng_hooks.pfnEmitSound(ent, CHAN_WEAPON, "weapons/crowbar.wav",
                             1.0f, 1.0f, 0, 100);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   ASSERT_FLOAT(s->m_flExpireTime, gpGlobals->time + 8.0f);

   PASS();
   return 0;
}

static int test_EmitSound_other_entity(void)
{
   TEST("pfnEmitSound: other entity -> 5s duration");

   test_reset();
   gpGlobals->deathmatch = 1;
   InitWeaponSelect(SUBMOD_HLDM);

   edict_t *ent = mock_alloc_edict();
   mock_set_classname(ent, "func_door");

   g_eng_hooks.pfnEmitSound(ent, CHAN_BODY, "doors/doormove1.wav",
                             0.7f, 1.0f, 0, 100);

   ASSERT_TRUE(count_active_sounds() > 0);
   CSound *s = get_first_active_sound();
   ASSERT_PTR_NOT_NULL(s);
   ASSERT_INT(s->m_iVolume, 699); // (int)(1000*0.7f) on x87
   ASSERT_FLOAT(s->m_flExpireTime, gpGlobals->time + 5.0f);

   PASS();
   return 0;
}

static int test_EmitSound_deathmatch_0(void)
{
   TEST("pfnEmitSound: deathmatch=0 -> no SaveSound");

   test_reset();
   gpGlobals->deathmatch = 0;

   edict_t *ent = mock_alloc_edict();
   mock_set_classname(ent, "item_health");

   g_eng_hooks.pfnEmitSound(ent, CHAN_ITEM, "items/smallmedkit1.wav",
                             1.0f, 1.0f, 0, 100);

   ASSERT_INT(count_active_sounds(), 0);

   PASS();
   return 0;
}

static int test_EmitSound_null_entity(void)
{
   TEST("pfnEmitSound: null entity (worldspawn) -> no SaveSound");

   test_reset();
   gpGlobals->deathmatch = 1;

   // worldspawn edict (index 0) -> FNullEnt returns TRUE
   g_eng_hooks.pfnEmitSound(&mock_edicts[0], CHAN_ITEM, "test.wav",
                             1.0f, 1.0f, 0, 100);

   ASSERT_INT(count_active_sounds(), 0);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnChangeLevel
// ============================================================

static int test_ChangeLevel_kicks_all_bots(void)
{
   TEST("pfnChangeLevel: kicks all active bots");

   test_reset();
   gpGlobals->deathmatch = 1;

   setup_bot(0);
   setup_bot(5);
   setup_bot(31);

   reset_botkick();

   char s1[] = "map1";
   char s2[] = "map2";
   g_eng_hooks.pfnChangeLevel(s1, s2);

   ASSERT_INT(botkick_call_count, 3);

   PASS();
   return 0;
}

static int test_ChangeLevel_deathmatch_0(void)
{
   TEST("pfnChangeLevel: deathmatch=0 -> no kicks");

   test_reset();
   gpGlobals->deathmatch = 0;

   setup_bot(0);
   reset_botkick();

   char s1[] = "map1";
   char s2[] = "map2";
   g_eng_hooks.pfnChangeLevel(s1, s2);

   ASSERT_INT(botkick_call_count, 0);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnClientCommand
// ============================================================

static int test_ClientCommand_bot_superceded(void)
{
   TEST("pfnClientCommand: bot edict -> MRES_SUPERCEDE");

   test_reset();

   edict_t *bot = mock_alloc_edict();
   bot->v.flags = FL_FAKECLIENT;

   gpMetaGlobals->mres = MRES_UNSET;
   g_eng_hooks.pfnClientCommand(bot, (char*)"test %s", (char*)"arg");

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);

   PASS();
   return 0;
}

static int test_ClientCommand_thirdpartybot_superceded(void)
{
   TEST("pfnClientCommand: thirdparty bot -> MRES_SUPERCEDE");

   test_reset();

   edict_t *bot = mock_alloc_edict();
   bot->v.flags = FL_THIRDPARTYBOT;

   gpMetaGlobals->mres = MRES_UNSET;
   g_eng_hooks.pfnClientCommand(bot, (char*)"test");

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);

   PASS();
   return 0;
}

static int test_ClientCommand_normal_player(void)
{
   TEST("pfnClientCommand: normal player -> MRES_IGNORED");

   test_reset();

   edict_t *player = setup_player();

   gpMetaGlobals->mres = MRES_UNSET;
   g_eng_hooks.pfnClientCommand(player, (char*)"test");

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnMessageBegin
// ============================================================

static int test_MessageBegin_bot_WeaponList(void)
{
   TEST("pfnMessageBegin: bot edict + WeaponList msg -> sets fn");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *bot_edict = setup_bot(2);
   reset_stubs();
   botMsgFunction = NULL;
   botMsgEndFunction = NULL;

   g_eng_hooks.pfnMessageBegin(MSG_ONE, MOCK_MSG_WEAPONLIST, NULL, bot_edict);

   ASSERT_PTR_EQ((void*)botMsgFunction, (void*)BotClient_Valve_WeaponList);
   ASSERT_INT(botMsgIndex, 2);

   PASS();
   return 0;
}

static int test_MessageBegin_bot_CurWeapon(void)
{
   TEST("pfnMessageBegin: bot edict + CurWeapon msg -> sets fn");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *bot_edict = setup_bot(7);
   botMsgFunction = NULL;

   g_eng_hooks.pfnMessageBegin(MSG_ONE, MOCK_MSG_CURWEAPON, NULL, bot_edict);

   ASSERT_PTR_EQ((void*)botMsgFunction, (void*)BotClient_Valve_CurrentWeapon);
   ASSERT_INT(botMsgIndex, 7);

   PASS();
   return 0;
}

static int test_MessageBegin_bot_AmmoX(void)
{
   TEST("pfnMessageBegin: bot edict + AmmoX msg -> sets fn");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *bot_edict = setup_bot(0);
   botMsgFunction = NULL;

   g_eng_hooks.pfnMessageBegin(MSG_ONE, MOCK_MSG_AMMOX, NULL, bot_edict);

   ASSERT_PTR_EQ((void*)botMsgFunction, (void*)BotClient_Valve_AmmoX);

   PASS();
   return 0;
}

static int test_MessageBegin_bot_Health(void)
{
   TEST("pfnMessageBegin: bot edict + Health msg -> sets fn");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *bot_edict = setup_bot(1);
   botMsgFunction = NULL;

   g_eng_hooks.pfnMessageBegin(MSG_ONE, MOCK_MSG_HEALTH, NULL, bot_edict);

   ASSERT_PTR_EQ((void*)botMsgFunction, (void*)BotClient_Valve_Health);

   PASS();
   return 0;
}

static int test_MessageBegin_MSG_ALL_DeathMsg(void)
{
   TEST("pfnMessageBegin: MSG_ALL + DeathMsg -> sets botMsgFn");

   test_reset();
   gpGlobals->deathmatch = 1;

   botMsgFunction = NULL;
   botMsgEndFunction = NULL;

   g_eng_hooks.pfnMessageBegin(MSG_ALL, MOCK_MSG_DEATHMSG, NULL, NULL);

   ASSERT_PTR_EQ((void*)botMsgFunction, (void*)BotClient_Valve_DeathMsg);
   ASSERT_INT(botMsgIndex, -1);

   PASS();
   return 0;
}

static int test_MessageBegin_MSG_ALL_SVC_INTERMISSION(void)
{
   TEST("pfnMessageBegin: MSG_ALL + SVC_INTERMISSION -> intermission");

   test_reset();
   gpGlobals->deathmatch = 1;

   g_in_intermission = FALSE;
   g_eng_hooks.pfnMessageBegin(MSG_ALL, SVC_INTERMISSION, NULL, NULL);

   ASSERT_INT(g_in_intermission, TRUE);

   PASS();
   return 0;
}

static int test_MessageBegin_non_bot_player(void)
{
   TEST("pfnMessageBegin: non-bot player -> clears botMsgFn");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *player = setup_player();

   botMsgFunction = BotClient_Valve_WeaponList;
   botMsgEndFunction = BotClient_Valve_WeaponList;

   g_eng_hooks.pfnMessageBegin(MSG_ONE, MOCK_MSG_CURWEAPON, NULL, player);

   // For non-bot players, botMsgFunction is cleared
   ASSERT_PTR_NULL((void*)botMsgFunction);
   ASSERT_PTR_NULL((void*)botMsgEndFunction);

   PASS();
   return 0;
}

static int test_MessageBegin_other_dest_WeaponList(void)
{
   TEST("pfnMessageBegin: other dest + WeaponList -> Steam compat");

   test_reset();
   gpGlobals->deathmatch = 1;

   botMsgFunction = NULL;

   // Steam sends WeaponList with different msg_dest, no entity
   g_eng_hooks.pfnMessageBegin(MSG_ONE_UNRELIABLE, MOCK_MSG_WEAPONLIST, NULL, NULL);

   ASSERT_PTR_EQ((void*)botMsgFunction, (void*)BotClient_Valve_WeaponList);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnMessageEnd
// ============================================================

static int test_MessageEnd_calls_botMsgEndFunction(void)
{
   TEST("pfnMessageEnd: calls botMsgEndFunction and clears");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgEndFunction = BotClient_Valve_WeaponList;
   botMsgFunction = BotClient_Valve_Health;
   botMsgIndex = 4;

   g_eng_hooks.pfnMessageEnd();

   // botMsgEndFunction should have been called
   ASSERT_INT(stub_call_count, 1);
   ASSERT_INT(stub_last_bot_index, 4);
   ASSERT_PTR_NULL(stub_last_str); // NULL indicates msg end
   // Both function pointers should be cleared
   ASSERT_PTR_NULL((void*)botMsgFunction);
   ASSERT_PTR_NULL((void*)botMsgEndFunction);

   PASS();
   return 0;
}

static int test_MessageEnd_no_end_function(void)
{
   TEST("pfnMessageEnd: no botMsgEndFunction -> just clears");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgEndFunction = NULL;
   botMsgFunction = BotClient_Valve_Health;

   g_eng_hooks.pfnMessageEnd();

   ASSERT_INT(stub_call_count, 0);
   ASSERT_PTR_NULL((void*)botMsgFunction);
   ASSERT_PTR_NULL((void*)botMsgEndFunction);

   PASS();
   return 0;
}

static int test_MessageEnd_deathmatch_0(void)
{
   TEST("pfnMessageEnd: deathmatch=0 -> no action");

   test_reset();
   gpGlobals->deathmatch = 0;

   reset_stubs();
   botMsgEndFunction = BotClient_Valve_WeaponList;
   botMsgFunction = BotClient_Valve_Health;

   g_eng_hooks.pfnMessageEnd();

   // Should not call end function when deathmatch=0
   ASSERT_INT(stub_call_count, 0);
   // Pointers NOT cleared (block skipped)
   ASSERT_PTR_NOT_NULL((void*)botMsgFunction);
   ASSERT_PTR_NOT_NULL((void*)botMsgEndFunction);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnWrite* functions
// ============================================================

static int test_WriteByte_with_botMsgFunction(void)
{
   TEST("pfnWriteByte: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_Health;
   botMsgIndex = 3;

   g_eng_hooks.pfnWriteByte(42);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_INT(stub_last_bot_index, 3);
   ASSERT_INT(stub_last_int, 42);

   PASS();
   return 0;
}

static int test_WriteChar_with_botMsgFunction(void)
{
   TEST("pfnWriteChar: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_Damage;
   botMsgIndex = 5;

   g_eng_hooks.pfnWriteChar(99);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_INT(stub_last_int, 99);

   PASS();
   return 0;
}

static int test_WriteShort_with_botMsgFunction(void)
{
   TEST("pfnWriteShort: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_AmmoX;
   botMsgIndex = 1;

   g_eng_hooks.pfnWriteShort(1234);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_INT(stub_last_int, 1234);

   PASS();
   return 0;
}

static int test_WriteLong_with_botMsgFunction(void)
{
   TEST("pfnWriteLong: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_Battery;
   botMsgIndex = 0;

   g_eng_hooks.pfnWriteLong(56789);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_INT(stub_last_int, 56789);

   PASS();
   return 0;
}

static int test_WriteAngle_with_botMsgFunction(void)
{
   TEST("pfnWriteAngle: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_ScreenFade;
   botMsgIndex = 2;

   g_eng_hooks.pfnWriteAngle(45.5f);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_FLOAT(stub_last_float, 45.5f);

   PASS();
   return 0;
}

static int test_WriteCoord_with_botMsgFunction(void)
{
   TEST("pfnWriteCoord: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_Damage;
   botMsgIndex = 6;

   g_eng_hooks.pfnWriteCoord(123.456f);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_FLOAT(stub_last_float, 123.456f);

   PASS();
   return 0;
}

static int test_WriteString_with_botMsgFunction(void)
{
   TEST("pfnWriteString: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_ItemPickup;
   botMsgIndex = 4;

   g_eng_hooks.pfnWriteString("test_string");

   ASSERT_INT(stub_call_count, 1);
   ASSERT_STR(stub_last_str, "test_string");

   PASS();
   return 0;
}

static int test_WriteEntity_with_botMsgFunction(void)
{
   TEST("pfnWriteEntity: active botMsgFunction -> calls it");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = BotClient_Valve_DeathMsg;
   botMsgIndex = 7;

   g_eng_hooks.pfnWriteEntity(15);

   ASSERT_INT(stub_call_count, 1);
   ASSERT_INT(stub_last_int, 15);

   PASS();
   return 0;
}

static int test_WriteByte_no_botMsgFunction(void)
{
   TEST("pfnWriteByte: no botMsgFunction -> no call");

   test_reset();
   gpGlobals->deathmatch = 1;

   reset_stubs();
   botMsgFunction = NULL;

   g_eng_hooks.pfnWriteByte(42);

   ASSERT_INT(stub_call_count, 0);

   PASS();
   return 0;
}

static int test_WriteByte_deathmatch_0(void)
{
   TEST("pfnWriteByte: deathmatch=0 -> no call");

   test_reset();
   gpGlobals->deathmatch = 0;

   reset_stubs();
   botMsgFunction = BotClient_Valve_Health;

   g_eng_hooks.pfnWriteByte(42);

   ASSERT_INT(stub_call_count, 0);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnClientPrintf
// ============================================================

static int test_ClientPrintf_bot_superceded(void)
{
   TEST("pfnClientPrintf: bot -> MRES_SUPERCEDE");

   test_reset();

   edict_t *bot = mock_alloc_edict();
   bot->v.flags = FL_FAKECLIENT;

   gpMetaGlobals->mres = MRES_UNSET;
   g_eng_hooks.pfnClientPrintf(bot, print_console, "test message");

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);

   PASS();
   return 0;
}

static int test_ClientPrintf_thirdpartybot_superceded(void)
{
   TEST("pfnClientPrintf: thirdparty bot -> MRES_SUPERCEDE");

   test_reset();

   edict_t *bot = mock_alloc_edict();
   bot->v.flags = FL_THIRDPARTYBOT;

   gpMetaGlobals->mres = MRES_UNSET;
   g_eng_hooks.pfnClientPrintf(bot, print_console, "test");

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);

   PASS();
   return 0;
}

static int test_ClientPrintf_normal_player(void)
{
   TEST("pfnClientPrintf: normal player -> MRES_IGNORED");

   test_reset();

   edict_t *player = setup_player();

   gpMetaGlobals->mres = MRES_UNSET;
   g_eng_hooks.pfnClientPrintf(player, print_console, "test");

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnCmd_Args/Argv/Argc
// ============================================================

static int test_Cmd_Args_with_isFakeClientCommand(void)
{
   TEST("pfnCmd_Args: isFakeClientCommand -> returns g_argv");

   test_reset();

   isFakeClientCommand = TRUE;
   strcpy(g_argv, "test_arg1 test_arg2");

   const char *result = g_eng_hooks.pfnCmd_Args();

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);
   ASSERT_STR(result, "test_arg1 test_arg2");

   isFakeClientCommand = FALSE;

   PASS();
   return 0;
}

static int test_Cmd_Args_without_isFakeClientCommand(void)
{
   TEST("pfnCmd_Args: not fake -> MRES_IGNORED, returns NULL");

   test_reset();

   isFakeClientCommand = FALSE;

   const char *result = g_eng_hooks.pfnCmd_Args();

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);
   ASSERT_PTR_NULL((void*)result);

   PASS();
   return 0;
}

static int test_Cmd_Argv_with_isFakeClientCommand(void)
{
   TEST("pfnCmd_Argv: isFakeClientCommand -> returns g_arg1/2/3");

   test_reset();

   isFakeClientCommand = TRUE;
   strcpy(g_arg1, "command");
   strcpy(g_arg2, "param1");
   strcpy(g_arg3, "param2");

   const char *r0 = g_eng_hooks.pfnCmd_Argv(0);
   ASSERT_STR(r0, "command");
   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);

   const char *r1 = g_eng_hooks.pfnCmd_Argv(1);
   ASSERT_STR(r1, "param1");

   const char *r2 = g_eng_hooks.pfnCmd_Argv(2);
   ASSERT_STR(r2, "param2");

   const char *r3 = g_eng_hooks.pfnCmd_Argv(3);
   ASSERT_PTR_NULL((void*)r3);
   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);

   isFakeClientCommand = FALSE;

   PASS();
   return 0;
}

static int test_Cmd_Argv_without_isFakeClientCommand(void)
{
   TEST("pfnCmd_Argv: not fake -> MRES_IGNORED");

   test_reset();

   isFakeClientCommand = FALSE;

   const char *result = g_eng_hooks.pfnCmd_Argv(0);

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);
   ASSERT_PTR_NULL((void*)result);

   PASS();
   return 0;
}

static int test_Cmd_Argc_with_isFakeClientCommand(void)
{
   TEST("pfnCmd_Argc: isFakeClientCommand -> returns fake_arg_count");

   test_reset();

   isFakeClientCommand = TRUE;
   fake_arg_count = 3;

   int result = g_eng_hooks.pfnCmd_Argc();

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);
   ASSERT_INT(result, 3);

   isFakeClientCommand = FALSE;
   fake_arg_count = 0;

   PASS();
   return 0;
}

static int test_Cmd_Argc_without_isFakeClientCommand(void)
{
   TEST("pfnCmd_Argc: not fake -> MRES_IGNORED, returns 0");

   test_reset();

   isFakeClientCommand = FALSE;

   int result = g_eng_hooks.pfnCmd_Argc();

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);
   ASSERT_INT(result, 0);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnSetClientMaxspeed
// ============================================================

static int test_SetClientMaxspeed_bot(void)
{
   TEST("pfnSetClientMaxspeed: bot -> updates f_max_speed");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *bot_edict = setup_bot(4);
   bots[4].f_max_speed = 0.0f;

   g_eng_hooks.pfnSetClientMaxspeed(bot_edict, 320.0f);

   ASSERT_FLOAT(bots[4].f_max_speed, 320.0f);

   PASS();
   return 0;
}

static int test_SetClientMaxspeed_non_bot(void)
{
   TEST("pfnSetClientMaxspeed: non-bot -> no change to bots");

   test_reset();
   gpGlobals->deathmatch = 1;

   edict_t *player = setup_player();

   for (int i = 0; i < 32; i++)
      bots[i].f_max_speed = 100.0f;

   g_eng_hooks.pfnSetClientMaxspeed(player, 500.0f);

   for (int i = 0; i < 32; i++)
      ASSERT_FLOAT(bots[i].f_max_speed, 100.0f);

   PASS();
   return 0;
}

static int test_SetClientMaxspeed_deathmatch_0(void)
{
   TEST("pfnSetClientMaxspeed: deathmatch=0 -> ignored");

   test_reset();
   gpGlobals->deathmatch = 0;

   edict_t *bot_edict = setup_bot(2);
   bots[2].f_max_speed = 100.0f;

   g_eng_hooks.pfnSetClientMaxspeed(bot_edict, 500.0f);

   ASSERT_FLOAT(bots[2].f_max_speed, 100.0f);

   PASS();
   return 0;
}

// ============================================================
// Tests: pfnGetPlayerUserId
// ============================================================

static int test_GetPlayerUserId_OP4_bot(void)
{
   TEST("pfnGetPlayerUserId: OP4 bot -> returns 0");

   test_reset();
   gpGlobals->deathmatch = 1;
   submod_id = SUBMOD_OP4;

   edict_t *bot_edict = setup_bot(0);

   int result = g_eng_hooks.pfnGetPlayerUserId(bot_edict);

   ASSERT_INT(gpMetaGlobals->mres, MRES_SUPERCEDE);
   ASSERT_INT(result, 0);

   submod_id = SUBMOD_HLDM;

   PASS();
   return 0;
}

static int test_GetPlayerUserId_HLDM_bot(void)
{
   TEST("pfnGetPlayerUserId: HLDM bot -> MRES_IGNORED");

   test_reset();
   gpGlobals->deathmatch = 1;
   submod_id = SUBMOD_HLDM;

   edict_t *bot_edict = setup_bot(0);

   int result = g_eng_hooks.pfnGetPlayerUserId(bot_edict);

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);
   ASSERT_INT(result, 0);
   (void)bot_edict;

   PASS();
   return 0;
}

static int test_GetPlayerUserId_OP4_non_bot(void)
{
   TEST("pfnGetPlayerUserId: OP4 non-bot -> MRES_IGNORED");

   test_reset();
   gpGlobals->deathmatch = 1;
   submod_id = SUBMOD_OP4;

   edict_t *player = setup_player();

   int result = g_eng_hooks.pfnGetPlayerUserId(player);

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);
   ASSERT_INT(result, 0);

   submod_id = SUBMOD_HLDM;

   PASS();
   return 0;
}

static int test_GetPlayerUserId_deathmatch_0(void)
{
   TEST("pfnGetPlayerUserId: deathmatch=0 -> MRES_IGNORED");

   test_reset();
   gpGlobals->deathmatch = 0;
   submod_id = SUBMOD_OP4;

   edict_t *bot_edict = setup_bot(0);
   (void)bot_edict;

   int result = g_eng_hooks.pfnGetPlayerUserId(bot_edict);

   ASSERT_INT(gpMetaGlobals->mres, MRES_IGNORED);
   ASSERT_INT(result, 0);

   submod_id = SUBMOD_HLDM;

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   // Set up custom GetUserMsgID BEFORE any pfnMessageBegin call,
   // because pfnMessageBegin has static locals that cache IDs.
   gpMetaUtilFuncs->pfnGetUserMsgID = custom_GetUserMsgID;

   // Initialize engine function tables
   test_reset();
   init_engine_tables();

   printf("test_engine:\n");

   // GetEngineFunctions table tests
   fail |= test_GetEngineFunctions_returns_TRUE();
   fail |= test_GetEngineFunctions_POST_returns_TRUE();

   // PrecacheEvent_Post tests
   fail |= test_PrecacheEvent_Post_known_event();
   fail |= test_PrecacheEvent_Post_unknown_event();
   fail |= test_PrecacheEvent_Post_deathmatch_0();
   fail |= test_PrecacheEvent_Post_override_ret();

   // PlaybackEvent tests
   fail |= test_PlaybackEvent_volume_calls_SaveSound();
   fail |= test_PlaybackEvent_recoil_updates_bot();
   fail |= test_PlaybackEvent_gauss_duplicate_ignored();
   fail |= test_PlaybackEvent_gauss_normal_applies_recoil();
   fail |= test_PlaybackEvent_non_bot_no_recoil();
   fail |= test_PlaybackEvent_deathmatch_0();
   fail |= test_PlaybackEvent_zero_volume_no_sound();

   // EmitSound tests
   fail |= test_EmitSound_item_health();
   fail |= test_EmitSound_item_battery();
   fail |= test_EmitSound_item_longjump();
   fail |= test_EmitSound_weapon_item();
   fail |= test_EmitSound_other_entity();
   fail |= test_EmitSound_deathmatch_0();
   fail |= test_EmitSound_null_entity();

   // ChangeLevel tests
   fail |= test_ChangeLevel_kicks_all_bots();
   fail |= test_ChangeLevel_deathmatch_0();

   // ClientCommand tests
   fail |= test_ClientCommand_bot_superceded();
   fail |= test_ClientCommand_thirdpartybot_superceded();
   fail |= test_ClientCommand_normal_player();

   // MessageBegin tests
   fail |= test_MessageBegin_bot_WeaponList();
   fail |= test_MessageBegin_bot_CurWeapon();
   fail |= test_MessageBegin_bot_AmmoX();
   fail |= test_MessageBegin_bot_Health();
   fail |= test_MessageBegin_MSG_ALL_DeathMsg();
   fail |= test_MessageBegin_MSG_ALL_SVC_INTERMISSION();
   fail |= test_MessageBegin_non_bot_player();
   fail |= test_MessageBegin_other_dest_WeaponList();

   // MessageEnd tests
   fail |= test_MessageEnd_calls_botMsgEndFunction();
   fail |= test_MessageEnd_no_end_function();
   fail |= test_MessageEnd_deathmatch_0();

   // Write* tests
   fail |= test_WriteByte_with_botMsgFunction();
   fail |= test_WriteChar_with_botMsgFunction();
   fail |= test_WriteShort_with_botMsgFunction();
   fail |= test_WriteLong_with_botMsgFunction();
   fail |= test_WriteAngle_with_botMsgFunction();
   fail |= test_WriteCoord_with_botMsgFunction();
   fail |= test_WriteString_with_botMsgFunction();
   fail |= test_WriteEntity_with_botMsgFunction();
   fail |= test_WriteByte_no_botMsgFunction();
   fail |= test_WriteByte_deathmatch_0();

   // ClientPrintf tests
   fail |= test_ClientPrintf_bot_superceded();
   fail |= test_ClientPrintf_thirdpartybot_superceded();
   fail |= test_ClientPrintf_normal_player();

   // Cmd_Args/Argv/Argc tests
   fail |= test_Cmd_Args_with_isFakeClientCommand();
   fail |= test_Cmd_Args_without_isFakeClientCommand();
   fail |= test_Cmd_Argv_with_isFakeClientCommand();
   fail |= test_Cmd_Argv_without_isFakeClientCommand();
   fail |= test_Cmd_Argc_with_isFakeClientCommand();
   fail |= test_Cmd_Argc_without_isFakeClientCommand();

   // SetClientMaxspeed tests
   fail |= test_SetClientMaxspeed_bot();
   fail |= test_SetClientMaxspeed_non_bot();
   fail |= test_SetClientMaxspeed_deathmatch_0();

   // GetPlayerUserId tests
   fail |= test_GetPlayerUserId_OP4_bot();
   fail |= test_GetPlayerUserId_HLDM_bot();
   fail |= test_GetPlayerUserId_OP4_non_bot();
   fail |= test_GetPlayerUserId_deathmatch_0();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
