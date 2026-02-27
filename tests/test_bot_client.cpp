//
// JK_Botti - tests for bot_client.cpp
//
// test_bot_client.cpp
//
// Tests the state-machine message parsers: WeaponList, CurrentWeapon,
// AmmoX, AmmoPickup, ItemPickup, Damage, DeathMsg, ScreenFade.
//

#include <stdlib.h>
#include <math.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_client.h"
#include "bot_weapons.h"

#include "engine_mock.h"
#include "test_common.h"

// Stubs for symbols not in engine_mock
int bot_taunt_count = 0;
int recent_bot_taunt[5] = {};
bot_chat_t bot_taunt[MAX_BOT_CHAT];

qboolean FPredictedVisible(bot_t &pBot) { (void)pBot; return FALSE; }

// weapon_defs is defined in bot_client.o
extern bot_weapon_t weapon_defs[MAX_WEAPONS];

// ============================================================
// Helpers for feeding state-machine messages
// ============================================================

// Send an int value as a message parameter (casts to void*)
static void msg_int(void (*fn)(void *, int), int bot_index, int val)
{
   fn((void *)&val, bot_index);
}

// Send a string value as a message parameter
static void msg_str(void (*fn)(void *, int), int bot_index, const char *val)
{
   fn((void *)val, bot_index);
}

// Send a float value as a message parameter
static void msg_float(void (*fn)(void *, int), int bot_index, float val)
{
   fn((void *)&val, bot_index);
}

// ============================================================
// WeaponList tests
// ============================================================

static int test_WeaponList_basic(void)
{
   TEST("WeaponList: registers weapon definition");
   mock_reset();

   int bi = 0;  // bot_index (not used by WeaponList but required param)

   // WeaponList expects 9 calls: classname, ammo1, ammo1max, ammo2, ammo2max,
   // slot, position, id, flags
   msg_str(BotClient_Valve_WeaponList, bi, "weapon_shotgun");  // state 0: classname
   msg_int(BotClient_Valve_WeaponList, bi, 3);                 // state 1: iAmmo1
   msg_int(BotClient_Valve_WeaponList, bi, 125);               // state 2: iAmmo1Max
   msg_int(BotClient_Valve_WeaponList, bi, -1);                // state 3: iAmmo2
   msg_int(BotClient_Valve_WeaponList, bi, -1);                // state 4: iAmmo2Max
   msg_int(BotClient_Valve_WeaponList, bi, 2);                 // state 5: iSlot
   msg_int(BotClient_Valve_WeaponList, bi, 1);                 // state 6: iPosition
   msg_int(BotClient_Valve_WeaponList, bi, VALVE_WEAPON_SHOTGUN); // state 7: iId
   msg_int(BotClient_Valve_WeaponList, bi, 0);                 // state 8: iFlags

   ASSERT_STR(weapon_defs[VALVE_WEAPON_SHOTGUN].szClassname, "weapon_shotgun");
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1, 3);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1Max, 125);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2, -1);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2Max, -1);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iSlot, 2);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iPosition, 1);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iId, VALVE_WEAPON_SHOTGUN);
   ASSERT_INT(weapon_defs[VALVE_WEAPON_SHOTGUN].iFlags, 0);

   PASS();
   return 0;
}

static int test_WeaponList_multiple(void)
{
   TEST("WeaponList: multiple weapons registered correctly");
   mock_reset();

   int bi = 0;

   // Register glock
   msg_str(BotClient_Valve_WeaponList, bi, "weapon_9mmhandgun");
   msg_int(BotClient_Valve_WeaponList, bi, 1);   // ammo1
   msg_int(BotClient_Valve_WeaponList, bi, 250);  // ammo1max
   msg_int(BotClient_Valve_WeaponList, bi, -1);   // ammo2
   msg_int(BotClient_Valve_WeaponList, bi, -1);   // ammo2max
   msg_int(BotClient_Valve_WeaponList, bi, 1);    // slot
   msg_int(BotClient_Valve_WeaponList, bi, 0);    // position
   msg_int(BotClient_Valve_WeaponList, bi, VALVE_WEAPON_GLOCK); // id
   msg_int(BotClient_Valve_WeaponList, bi, 0);    // flags

   // Register crowbar
   msg_str(BotClient_Valve_WeaponList, bi, "weapon_crowbar");
   msg_int(BotClient_Valve_WeaponList, bi, -1);   // ammo1
   msg_int(BotClient_Valve_WeaponList, bi, -1);   // ammo1max
   msg_int(BotClient_Valve_WeaponList, bi, -1);   // ammo2
   msg_int(BotClient_Valve_WeaponList, bi, -1);   // ammo2max
   msg_int(BotClient_Valve_WeaponList, bi, 0);    // slot
   msg_int(BotClient_Valve_WeaponList, bi, 0);    // position
   msg_int(BotClient_Valve_WeaponList, bi, VALVE_WEAPON_CROWBAR); // id
   msg_int(BotClient_Valve_WeaponList, bi, 0);    // flags

   ASSERT_STR(weapon_defs[VALVE_WEAPON_GLOCK].szClassname, "weapon_9mmhandgun");
   ASSERT_INT(weapon_defs[VALVE_WEAPON_GLOCK].iAmmo1, 1);

   ASSERT_STR(weapon_defs[VALVE_WEAPON_CROWBAR].szClassname, "weapon_crowbar");
   ASSERT_INT(weapon_defs[VALVE_WEAPON_CROWBAR].iAmmo1, -1);

   PASS();
   return 0;
}

// ============================================================
// CurrentWeapon tests
// ============================================================

static int test_CurrentWeapon_active(void)
{
   TEST("CurrentWeapon: active weapon sets bot state");
   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   int bi = 0;

   // Setup: register shotgun in weapon_defs
   memset(&weapon_defs[VALVE_WEAPON_SHOTGUN], 0, sizeof(weapon_defs[0]));
   weapon_defs[VALVE_WEAPON_SHOTGUN].iId = VALVE_WEAPON_SHOTGUN;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1 = 3;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2 = -1;

   // Give the bot some ammo
   bots[bi].m_rgAmmo[3] = 40;  // ammo index 3 = buckshot

   // Setup edict for the bot
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;

   // Send CurrentWeapon: iState=1 (active), iId=SHOTGUN, iClip=8
   msg_int(BotClient_Valve_CurrentWeapon, bi, 1);   // state 0: iState = active
   msg_int(BotClient_Valve_CurrentWeapon, bi, VALVE_WEAPON_SHOTGUN); // state 1: iId
   msg_int(BotClient_Valve_CurrentWeapon, bi, 8);    // state 2: iClip

   ASSERT_INT(bots[bi].current_weapon.iId, VALVE_WEAPON_SHOTGUN);
   ASSERT_INT(bots[bi].current_weapon.iClip, 8);
   ASSERT_INT(bots[bi].current_weapon.iAmmo1, 40);

   PASS();
   return 0;
}

static int test_CurrentWeapon_inactive(void)
{
   TEST("CurrentWeapon: inactive weapon (state=0) no update");
   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;

   // Set initial weapon
   bots[bi].current_weapon.iId = VALVE_WEAPON_CROWBAR;
   bots[bi].current_weapon.iClip = 0;

   // Send CurrentWeapon with iState=0 (inactive)
   msg_int(BotClient_Valve_CurrentWeapon, bi, 0);   // state 0: iState = inactive
   msg_int(BotClient_Valve_CurrentWeapon, bi, VALVE_WEAPON_SHOTGUN);
   msg_int(BotClient_Valve_CurrentWeapon, bi, 8);

   // Should NOT update (iState != 1)
   ASSERT_INT(bots[bi].current_weapon.iId, VALVE_WEAPON_CROWBAR);

   PASS();
   return 0;
}

// ============================================================
// AmmoX tests
// ============================================================

static int test_AmmoX_updates_reserve(void)
{
   TEST("AmmoX: updates ammo reserve and current weapon ammo");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;

   // Setup: bot has glock (ammo1 index = 1)
   bots[bi].current_weapon.iId = VALVE_WEAPON_GLOCK;
   weapon_defs[VALVE_WEAPON_GLOCK].iAmmo1 = 1;
   weapon_defs[VALVE_WEAPON_GLOCK].iAmmo2 = -1;

   // Send AmmoX: index=1, amount=200
   msg_int(BotClient_Valve_AmmoX, bi, 1);    // state 0: ammo index
   msg_int(BotClient_Valve_AmmoX, bi, 200);  // state 1: amount

   ASSERT_INT(bots[bi].m_rgAmmo[1], 200);
   ASSERT_INT(bots[bi].current_weapon.iAmmo1, 200);

   PASS();
   return 0;
}

// ============================================================
// AmmoPickup tests
// ============================================================

static int test_AmmoPickup_updates(void)
{
   TEST("AmmoPickup: updates ammo reserve");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;

   bots[bi].current_weapon.iId = VALVE_WEAPON_SHOTGUN;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1 = 3;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2 = -1;

   // Send AmmoPickup: index=3, amount=24
   msg_int(BotClient_Valve_AmmoPickup, bi, 3);   // state 0: ammo index
   msg_int(BotClient_Valve_AmmoPickup, bi, 24);  // state 1: amount

   ASSERT_INT(bots[bi].m_rgAmmo[3], 24);
   ASSERT_INT(bots[bi].current_weapon.iAmmo1, 24);

   PASS();
   return 0;
}

static int test_AmmoPickup_adds_to_existing(void)
{
   TEST("AmmoPickup: adds to existing ammo, not replaces");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;

   bots[bi].current_weapon.iId = VALVE_WEAPON_SHOTGUN;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1 = 3;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2 = -1;

   // Bot already has 10 shells
   bots[bi].m_rgAmmo[3] = 10;

   // Pick up 8 more shells
   msg_int(BotClient_Valve_AmmoPickup, bi, 3);   // ammo index
   msg_int(BotClient_Valve_AmmoPickup, bi, 8);   // amount picked up

   // Should be 10 + 8 = 18, not just 8
   ASSERT_INT(bots[bi].m_rgAmmo[3], 18);
   ASSERT_INT(bots[bi].current_weapon.iAmmo1, 18);

   PASS();
   return 0;
}

// ============================================================
// ItemPickup tests
// ============================================================

static int test_ItemPickup_longjump(void)
{
   TEST("ItemPickup: item_longjump sets b_longjump");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;
   bots[bi].b_longjump = FALSE;

   msg_str(BotClient_Valve_ItemPickup, bi, "item_longjump");

   ASSERT_INT(bots[bi].b_longjump, TRUE);
   ASSERT_TRUE(bots[bi].f_combat_longjump > 0.0f);

   PASS();
   return 0;
}

static int test_ItemPickup_other(void)
{
   TEST("ItemPickup: non-longjump item does nothing");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;
   bots[bi].b_longjump = FALSE;

   msg_str(BotClient_Valve_ItemPickup, bi, "item_battery");

   ASSERT_INT(bots[bi].b_longjump, FALSE);

   PASS();
   return 0;
}

// ============================================================
// Damage tests
// ============================================================

static int test_Damage_from_enemy(void)
{
   TEST("Damage: non-ignore damage sets f_last_time_attacked");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;
   bots[bi].pBotEnemy = NULL;
   bots[bi].f_last_time_attacked = 0.0f;

   gpGlobals->time = 25.0f;

   // Damage message: armor=0, damage_taken=10, damage_bits=0 (bullet),
   // damage_origin = (100, 200, 300)
   int armor = 0, taken = 10, bits = 0;
   float ox = 100.0f, oy = 200.0f, oz = 300.0f;

   msg_int(BotClient_Valve_Damage, bi, armor);  // state 0
   msg_int(BotClient_Valve_Damage, bi, taken);   // state 1
   msg_int(BotClient_Valve_Damage, bi, bits);    // state 2
   msg_float(BotClient_Valve_Damage, bi, ox);    // state 3
   msg_float(BotClient_Valve_Damage, bi, oy);    // state 4
   msg_float(BotClient_Valve_Damage, bi, oz);    // state 5

   ASSERT_FLOAT_NEAR(bots[bi].f_last_time_attacked, 25.0f, 0.01f);

   PASS();
   return 0;
}

static int test_Damage_ignored_type(void)
{
   TEST("Damage: DMG_CRUSH is ignored");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;
   bots[bi].pBotEnemy = NULL;
   bots[bi].f_last_time_attacked = 0.0f;

   gpGlobals->time = 25.0f;

   int armor = 0, taken = 10, bits = (1 << 0); // DMG_CRUSH
   float ox = 100.0f, oy = 200.0f, oz = 300.0f;

   msg_int(BotClient_Valve_Damage, bi, armor);
   msg_int(BotClient_Valve_Damage, bi, taken);
   msg_int(BotClient_Valve_Damage, bi, bits);
   msg_float(BotClient_Valve_Damage, bi, ox);
   msg_float(BotClient_Valve_Damage, bi, oy);
   msg_float(BotClient_Valve_Damage, bi, oz);

   // Should NOT update f_last_time_attacked (damage type is ignored)
   ASSERT_FLOAT_NEAR(bots[bi].f_last_time_attacked, 0.0f, 0.01f);

   PASS();
   return 0;
}

static int test_Damage_zero_both(void)
{
   TEST("Damage: zero armor and zero taken -> no update");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;
   bots[bi].pBotEnemy = NULL;
   bots[bi].f_last_time_attacked = 0.0f;

   gpGlobals->time = 25.0f;

   int armor = 0, taken = 0, bits = 0;
   float ox = 100.0f, oy = 200.0f, oz = 300.0f;

   msg_int(BotClient_Valve_Damage, bi, armor);
   msg_int(BotClient_Valve_Damage, bi, taken);
   msg_int(BotClient_Valve_Damage, bi, bits);
   msg_float(BotClient_Valve_Damage, bi, ox);
   msg_float(BotClient_Valve_Damage, bi, oy);
   msg_float(BotClient_Valve_Damage, bi, oz);

   // Both 0 -> no update
   ASSERT_FLOAT_NEAR(bots[bi].f_last_time_attacked, 0.0f, 0.01f);

   PASS();
   return 0;
}

// ============================================================
// DeathMsg tests
// ============================================================

static int test_DeathMsg_bot_killed(void)
{
   TEST("DeathMsg: bot killed by player -> stores killer_edict");
   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   // Setup: bot at index 2, killer at index 1
   int bi = 2;
   edict_t *bot_edict = mock_alloc_edict();  // index 1 - killer
   edict_t *victim_edict = mock_alloc_edict();  // index 2 - victim bot

   bots[bi].is_used = TRUE;
   bots[bi].pEdict = victim_edict;
   bots[bi].killer_edict = NULL;

   int killer_idx = 1;
   int victim_idx = 2;

   msg_int(BotClient_Valve_DeathMsg, bi, killer_idx);  // state 0: killer index
   msg_int(BotClient_Valve_DeathMsg, bi, victim_idx);  // state 1: victim index
   msg_str(BotClient_Valve_DeathMsg, bi, "weapon_shotgun"); // state 2: weapon name (ignored as char*)

   // Victim bot should have killer_edict set
   ASSERT_TRUE(bots[bi].killer_edict == bot_edict);

   PASS();
   return 0;
}

static int test_DeathMsg_bot_suicide(void)
{
   TEST("DeathMsg: bot suicide -> killer_edict = NULL");
   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   int bi = 1;
   edict_t *bot_edict = mock_alloc_edict();  // index 1 - both killer and victim

   bots[bi].is_used = TRUE;
   bots[bi].pEdict = bot_edict;
   bots[bi].killer_edict = (edict_t *)0xDEAD;  // set to non-null

   // Same killer and victim index = suicide
   msg_int(BotClient_Valve_DeathMsg, bi, 1);  // killer = 1
   msg_int(BotClient_Valve_DeathMsg, bi, 1);  // victim = 1
   msg_str(BotClient_Valve_DeathMsg, bi, "world");

   ASSERT_PTR_NULL(bots[bi].killer_edict);

   PASS();
   return 0;
}

static int test_DeathMsg_world_kill(void)
{
   TEST("DeathMsg: killed by world (index 0) -> killer_edict = NULL");
   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   int bi = 1;
   edict_t *bot_edict = mock_alloc_edict();  // index 1

   bots[bi].is_used = TRUE;
   bots[bi].pEdict = bot_edict;
   bots[bi].killer_edict = (edict_t *)0xDEAD;

   msg_int(BotClient_Valve_DeathMsg, bi, 0);  // killer = world
   msg_int(BotClient_Valve_DeathMsg, bi, 1);  // victim = bot
   msg_str(BotClient_Valve_DeathMsg, bi, "world");

   ASSERT_PTR_NULL(bots[bi].killer_edict);

   PASS();
   return 0;
}

// ============================================================
// ScreenFade tests
// ============================================================

static int test_ScreenFade_sets_blinded_time(void)
{
   TEST("ScreenFade: sets blinded_time from duration+hold");
   mock_reset();

   int bi = 0;
   edict_t *bot_edict = mock_alloc_edict();
   bots[bi].pEdict = bot_edict;
   bots[bi].blinded_time = 0.0f;

   gpGlobals->time = 10.0f;

   // ScreenFade message:
   // state 0: duration (in 1/4096 sec units)
   // state 1: hold_time
   // state 2: fade_flags
   // states 3-5: RGBA (just increments state)
   // state 6: final -> compute blinded_time
   int duration = 4096 * 4;  // 4 seconds
   int hold_time = 4096 * 2; // 2 seconds
   int fade_flags = 0;
   int r = 255, g = 255, b = 255;
   // Dummy value for state 6
   int a = 200;

   msg_int(BotClient_Valve_ScreenFade, bi, duration);    // state 0
   msg_int(BotClient_Valve_ScreenFade, bi, hold_time);   // state 1
   msg_int(BotClient_Valve_ScreenFade, bi, fade_flags);  // state 2
   msg_int(BotClient_Valve_ScreenFade, bi, r);           // state 3
   msg_int(BotClient_Valve_ScreenFade, bi, g);           // state 4
   msg_int(BotClient_Valve_ScreenFade, bi, b);           // state 5
   msg_int(BotClient_Valve_ScreenFade, bi, a);           // state 6

   // length = (duration + hold_time) / 4096 = (16384 + 8192) / 4096 = 6
   // blinded_time = gpGlobals->time + length - 2.0 = 10 + 6 - 2 = 14
   ASSERT_FLOAT_NEAR(bots[bi].blinded_time, 14.0f, 0.01f);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_client:\n");

   fail |= test_WeaponList_basic();
   fail |= test_WeaponList_multiple();
   fail |= test_CurrentWeapon_active();
   fail |= test_CurrentWeapon_inactive();
   fail |= test_AmmoX_updates_reserve();
   fail |= test_AmmoPickup_updates();
   fail |= test_AmmoPickup_adds_to_existing();
   fail |= test_ItemPickup_longjump();
   fail |= test_ItemPickup_other();
   fail |= test_Damage_from_enemy();
   fail |= test_Damage_ignored_type();
   fail |= test_Damage_zero_both();
   fail |= test_DeathMsg_bot_killed();
   fail |= test_DeathMsg_bot_suicide();
   fail |= test_DeathMsg_world_kill();
   fail |= test_ScreenFade_sets_blinded_time();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
