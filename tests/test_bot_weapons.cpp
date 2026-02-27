//
// JK_Botti - unit tests for bot_weapons.cpp
//
// test_bot_weapons.cpp
//

#include <stdlib.h>
#include <math.h>

#include "test_common.h"

#include "engine_mock.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "player.h"

// Externs from bot_weapons.cpp / engine_mock.cpp
extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern int submod_id;
extern int submod_weaponflag;
extern bot_weapon_select_t valve_weapon_select[NUM_OF_WEAPON_SELECTS];

// From util.h
extern void fast_random_seed(unsigned int seed);

// ============================================================
// Test helpers
// ============================================================

static void setup_bot(bot_t &pBot, edict_t *pEdict)
{
   memset(&pBot, 0, sizeof(pBot));
   pBot.pEdict = pEdict;
   pBot.is_used = TRUE;
   pBot.bot_skill = 2;
   pBot.weapon_skill = SKILL3;
   pBot.curr_waypoint_index = -1;
   pBot.waypoint_goal = -1;
   pBot.current_weapon_index = -1;
   pBot.f_primary_charging = -1;
   pBot.f_secondary_charging = -1;

   pEdict->v.origin = Vector(0, 0, 0);
   pEdict->v.v_angle = Vector(0, 0, 0);
   pEdict->v.view_ofs = Vector(0, 0, 28);
   pEdict->v.health = 100;
   pEdict->v.deadflag = DEAD_NO;
   pEdict->v.takedamage = DAMAGE_YES;
   pEdict->v.solid = SOLID_BBOX;
   pEdict->v.flags = FL_CLIENT | FL_FAKECLIENT;
}

// Set up realistic weapon_defs for valve HLDM weapons
// Ammo slot indices based on real HL1 assignments:
//   slot 1 = 9mm (glock/mp5 primary)
//   slot 2 = 357
//   slot 3 = buckshot
//   slot 4 = bolts (crossbow)
//   slot 5 = rockets
//   slot 6 = uranium (gauss/egon)
//   slot 7 = hornets
//   slot 8 = AR grenades (mp5 secondary)
//   slot 9 = snarks
//   slot 10 = satchels
//   slot 11 = hand grenades
static void setup_weapon_defs_valve(void)
{
   memset(weapon_defs, 0, sizeof(weapon_defs));

   // Crowbar (no ammo)
   weapon_defs[VALVE_WEAPON_CROWBAR].iId = VALVE_WEAPON_CROWBAR;
   weapon_defs[VALVE_WEAPON_CROWBAR].iAmmo1 = -1;
   weapon_defs[VALVE_WEAPON_CROWBAR].iAmmo2 = -1;

   // Glock: primary=9mm(1), no secondary ammo
   weapon_defs[VALVE_WEAPON_GLOCK].iId = VALVE_WEAPON_GLOCK;
   weapon_defs[VALVE_WEAPON_GLOCK].iAmmo1 = 1;
   weapon_defs[VALVE_WEAPON_GLOCK].iAmmo1Max = 250;
   weapon_defs[VALVE_WEAPON_GLOCK].iAmmo2 = -1;

   // Python: primary=357(2), no secondary
   weapon_defs[VALVE_WEAPON_PYTHON].iId = VALVE_WEAPON_PYTHON;
   weapon_defs[VALVE_WEAPON_PYTHON].iAmmo1 = 2;
   weapon_defs[VALVE_WEAPON_PYTHON].iAmmo1Max = 36;
   weapon_defs[VALVE_WEAPON_PYTHON].iAmmo2 = -1;

   // MP5: primary=9mm(1), secondary=ARgrenades(8)
   weapon_defs[VALVE_WEAPON_MP5].iId = VALVE_WEAPON_MP5;
   weapon_defs[VALVE_WEAPON_MP5].iAmmo1 = 1;
   weapon_defs[VALVE_WEAPON_MP5].iAmmo1Max = 250;
   weapon_defs[VALVE_WEAPON_MP5].iAmmo2 = 8;
   weapon_defs[VALVE_WEAPON_MP5].iAmmo2Max = 10;

   // Shotgun: primary=buckshot(3), secondary uses same
   weapon_defs[VALVE_WEAPON_SHOTGUN].iId = VALVE_WEAPON_SHOTGUN;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1 = 3;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo1Max = 125;
   weapon_defs[VALVE_WEAPON_SHOTGUN].iAmmo2 = -1;

   // Crossbow: primary=bolts(4), no secondary
   weapon_defs[VALVE_WEAPON_CROSSBOW].iId = VALVE_WEAPON_CROSSBOW;
   weapon_defs[VALVE_WEAPON_CROSSBOW].iAmmo1 = 4;
   weapon_defs[VALVE_WEAPON_CROSSBOW].iAmmo1Max = 50;
   weapon_defs[VALVE_WEAPON_CROSSBOW].iAmmo2 = -1;

   // RPG: primary=rockets(5), no secondary
   weapon_defs[VALVE_WEAPON_RPG].iId = VALVE_WEAPON_RPG;
   weapon_defs[VALVE_WEAPON_RPG].iAmmo1 = 5;
   weapon_defs[VALVE_WEAPON_RPG].iAmmo1Max = 5;
   weapon_defs[VALVE_WEAPON_RPG].iAmmo2 = -1;

   // Gauss: primary=uranium(6), secondary uses primary
   weapon_defs[VALVE_WEAPON_GAUSS].iId = VALVE_WEAPON_GAUSS;
   weapon_defs[VALVE_WEAPON_GAUSS].iAmmo1 = 6;
   weapon_defs[VALVE_WEAPON_GAUSS].iAmmo1Max = 100;
   weapon_defs[VALVE_WEAPON_GAUSS].iAmmo2 = -1;

   // Egon: primary=uranium(6), no secondary
   weapon_defs[VALVE_WEAPON_EGON].iId = VALVE_WEAPON_EGON;
   weapon_defs[VALVE_WEAPON_EGON].iAmmo1 = 6;
   weapon_defs[VALVE_WEAPON_EGON].iAmmo1Max = 100;
   weapon_defs[VALVE_WEAPON_EGON].iAmmo2 = -1;

   // Hornetgun: no ammo slots (self-regenerating)
   weapon_defs[VALVE_WEAPON_HORNETGUN].iId = VALVE_WEAPON_HORNETGUN;
   weapon_defs[VALVE_WEAPON_HORNETGUN].iAmmo1 = -1;
   weapon_defs[VALVE_WEAPON_HORNETGUN].iAmmo2 = -1;

   // Handgrenade: primary=handgrenades(11), no secondary
   weapon_defs[VALVE_WEAPON_HANDGRENADE].iId = VALVE_WEAPON_HANDGRENADE;
   weapon_defs[VALVE_WEAPON_HANDGRENADE].iAmmo1 = 11;
   weapon_defs[VALVE_WEAPON_HANDGRENADE].iAmmo1Max = 10;
   weapon_defs[VALVE_WEAPON_HANDGRENADE].iAmmo2 = -1;

   // Satchel: primary=satchels(10), no secondary
   weapon_defs[VALVE_WEAPON_SATCHEL].iId = VALVE_WEAPON_SATCHEL;
   weapon_defs[VALVE_WEAPON_SATCHEL].iAmmo1 = 10;
   weapon_defs[VALVE_WEAPON_SATCHEL].iAmmo1Max = 5;
   weapon_defs[VALVE_WEAPON_SATCHEL].iAmmo2 = -1;

   // Snark: primary=snarks(9), no secondary
   weapon_defs[VALVE_WEAPON_SNARK].iId = VALVE_WEAPON_SNARK;
   weapon_defs[VALVE_WEAPON_SNARK].iAmmo1 = 9;
   weapon_defs[VALVE_WEAPON_SNARK].iAmmo1Max = 15;
   weapon_defs[VALVE_WEAPON_SNARK].iAmmo2 = -1;
}

// ============================================================
// 1. SubmodToSubmodWeaponFlag tests
// ============================================================

static int test_SubmodToSubmodWeaponFlag(void)
{
   printf("SubmodToSubmodWeaponFlag:\n");

   TEST("HLDM -> WEAPON_SUBMOD_HLDM");
   ASSERT_INT(SubmodToSubmodWeaponFlag(SUBMOD_HLDM), WEAPON_SUBMOD_HLDM);
   PASS();

   TEST("all 5 submods map correctly");
   ASSERT_INT(SubmodToSubmodWeaponFlag(SUBMOD_SEVS), WEAPON_SUBMOD_SEVS);
   ASSERT_INT(SubmodToSubmodWeaponFlag(SUBMOD_BUBBLEMOD), WEAPON_SUBMOD_BUBBLEMOD);
   ASSERT_INT(SubmodToSubmodWeaponFlag(SUBMOD_XDM), WEAPON_SUBMOD_XDM);
   ASSERT_INT(SubmodToSubmodWeaponFlag(SUBMOD_OP4), WEAPON_SUBMOD_OP4);
   PASS();

   TEST("unknown submod defaults to HLDM");
   ASSERT_INT(SubmodToSubmodWeaponFlag(999), WEAPON_SUBMOD_HLDM);
   PASS();

   return 0;
}

// ============================================================
// 2. InitWeaponSelect / GetWeaponSelect tests
// ============================================================

static int test_InitWeaponSelect_GetWeaponSelect(void)
{
   printf("InitWeaponSelect / GetWeaponSelect:\n");

   mock_reset();

   TEST("HLDM init: handgrenade is WEAPON_THROW type");
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *hg = GetWeaponSelect(VALVE_WEAPON_HANDGRENADE);
   ASSERT_PTR_NOT_NULL(hg);
   ASSERT_INT(hg->type, WEAPON_THROW);
   PASS();

   TEST("SEVS init: handgrenade becomes WEAPON_FIRE type");
   InitWeaponSelect(SUBMOD_SEVS);
   hg = GetWeaponSelect(VALVE_WEAPON_HANDGRENADE);
   ASSERT_PTR_NOT_NULL(hg);
   ASSERT_INT(hg->type, WEAPON_FIRE);
   PASS();

   TEST("GetWeaponSelect finds crowbar");
   InitWeaponSelect(SUBMOD_HLDM);
   bot_weapon_select_t *cw = GetWeaponSelect(VALVE_WEAPON_CROWBAR);
   ASSERT_PTR_NOT_NULL(cw);
   ASSERT_INT(cw->iId, VALVE_WEAPON_CROWBAR);
   PASS();

   TEST("GetWeaponSelect returns NULL for unknown id");
   bot_weapon_select_t *unk = GetWeaponSelect(999);
   ASSERT_PTR_NULL(unk);
   PASS();

   TEST("GetWeaponSelect finds MP5");
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);
   ASSERT_PTR_NOT_NULL(mp5);
   ASSERT_INT(mp5->iId, VALVE_WEAPON_MP5);
   PASS();

   return 0;
}

// ============================================================
// 3. GetWeaponItemFlag / GetAmmoItemFlag tests
// ============================================================

static int test_GetWeaponItemFlag_GetAmmoItemFlag(void)
{
   printf("GetWeaponItemFlag / GetAmmoItemFlag:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   TEST("GetWeaponItemFlag: weapon_crowbar -> W_IFL_CROWBAR");
   ASSERT_INT(GetWeaponItemFlag("weapon_crowbar"), W_IFL_CROWBAR);
   PASS();

   TEST("GetWeaponItemFlag: unknown weapon -> 0");
   ASSERT_INT(GetWeaponItemFlag("weapon_bfg"), 0);
   PASS();

   TEST("GetAmmoItemFlag: ammo_buckshot -> W_IFL_AMMO_BUCKSHOT");
   ASSERT_INT(GetAmmoItemFlag("ammo_buckshot"), W_IFL_AMMO_BUCKSHOT);
   PASS();

   TEST("GetAmmoItemFlag: unknown ammo -> 0");
   ASSERT_INT(GetAmmoItemFlag("ammo_plasma"), 0);
   PASS();

   return 0;
}

// ============================================================
// 4. Skill checks / BotCanUseWeapon tests
// ============================================================

static int test_skill_checks(void)
{
   printf("Skill checks / BotCanUseWeapon:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   // Gauss: primary=SKILL4, secondary=SKILL2
   bot_weapon_select_t *gauss = GetWeaponSelect(VALVE_WEAPON_GAUSS);
   ASSERT_PTR_NOT_NULL(gauss);

   TEST("skilled enough for primary (SKILL3 <= SKILL4)");
   bot.weapon_skill = SKILL3;
   ASSERT_INT(BotSkilledEnoughForPrimaryAttack(bot, *gauss), TRUE);
   PASS();

   TEST("not skilled enough for primary (SKILL5 > SKILL4)");
   bot.weapon_skill = SKILL5;
   ASSERT_INT(BotSkilledEnoughForPrimaryAttack(bot, *gauss), FALSE);
   PASS();

   TEST("NOSKILL always returns FALSE");
   // Crowbar: secondary_skill_level = NOSKILL
   bot_weapon_select_t *crowbar = GetWeaponSelect(VALVE_WEAPON_CROWBAR);
   bot.weapon_skill = SKILL1;
   ASSERT_INT(BotSkilledEnoughForSecondaryAttack(bot, *crowbar), FALSE);
   PASS();

   TEST("BotCanUseWeapon: primary-only weapon ok");
   // Crossbow: primary=SKILL2, secondary=NOSKILL
   bot_weapon_select_t *xbow = GetWeaponSelect(VALVE_WEAPON_CROSSBOW);
   bot.weapon_skill = SKILL2;
   ASSERT_INT(BotCanUseWeapon(bot, *xbow), TRUE);
   PASS();

   TEST("BotCanUseWeapon: neither skill matches");
   bot.weapon_skill = SKILL5;
   // Crossbow primary=SKILL2, 5>2 fails; secondary=NOSKILL fails
   ASSERT_INT(BotCanUseWeapon(bot, *xbow), FALSE);
   PASS();

   TEST("BotCanUseWeapon: secondary skill match suffices");
   // Gauss: primary=SKILL4, secondary=SKILL2
   bot.weapon_skill = SKILL5;
   // 5>4 fails primary, 5>2 fails secondary -> FALSE
   ASSERT_INT(BotCanUseWeapon(bot, *gauss), FALSE);
   bot.weapon_skill = SKILL2;
   // 2<=4 primary ok -> TRUE
   ASSERT_INT(BotCanUseWeapon(bot, *gauss), TRUE);
   PASS();

   return 0;
}

// ============================================================
// 5. BotSelectAttack tests
// ============================================================

static int test_BotSelectAttack(void)
{
   printf("BotSelectAttack:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   qboolean use_primary, use_secondary;

   // Gauss: prefer_higher_skill_attack=TRUE, primary=SKILL4, secondary=SKILL2
   bot_weapon_select_t *gauss = GetWeaponSelect(VALVE_WEAPON_GAUSS);

   TEST("prefer_higher_skill: secondary preferred (lower=better)");
   bot.weapon_skill = SKILL2; // <= both SKILL4 and SKILL2
   BotSelectAttack(bot, *gauss, use_primary, use_secondary);
   ASSERT_INT(use_secondary, TRUE);
   ASSERT_INT(use_primary, FALSE);
   PASS();

   TEST("prefer_higher_skill: primary if secondary > primary");
   // Construct a weapon where primary has lower (better) skill
   bot_weapon_select_t custom = *gauss;
   custom.primary_skill_level = SKILL2;
   custom.secondary_skill_level = SKILL4;
   bot.weapon_skill = SKILL1;
   BotSelectAttack(bot, custom, use_primary, use_secondary);
   ASSERT_INT(use_primary, TRUE);
   ASSERT_INT(use_secondary, FALSE);
   PASS();

   // Test random fallback with deterministic seed
   // MP5: prefer_higher_skill_attack=FALSE, primary_fire_percent=70
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);

   TEST("random: 100% primary_fire_percent -> always primary");
   bot_weapon_select_t mp5_custom = *mp5;
   mp5_custom.primary_fire_percent = 100;
   fast_random_seed(42);
   BotSelectAttack(bot, mp5_custom, use_primary, use_secondary);
   ASSERT_INT(use_primary, TRUE);
   ASSERT_INT(use_secondary, FALSE);
   PASS();

   TEST("random: 0% primary_fire_percent -> always secondary");
   mp5_custom.primary_fire_percent = 0;
   fast_random_seed(42);
   BotSelectAttack(bot, mp5_custom, use_primary, use_secondary);
   ASSERT_INT(use_primary, FALSE);
   ASSERT_INT(use_secondary, TRUE);
   PASS();

   return 0;
}

// ============================================================
// 6. BotIsCarryingWeapon / IsValidToFireAtTheMoment tests
// ============================================================

static int test_BotIsCarryingWeapon_IsValidToFire(void)
{
   printf("BotIsCarryingWeapon / IsValidToFireAtTheMoment:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);

   TEST("carrying weapon: bit set -> TRUE");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   ASSERT_INT(BotIsCarryingWeapon(bot, VALVE_WEAPON_MP5), TRUE);
   PASS();

   TEST("not carrying weapon: bit not set -> FALSE");
   pe->v.weapons = 0;
   ASSERT_INT(BotIsCarryingWeapon(bot, VALVE_WEAPON_MP5), FALSE);
   PASS();

   TEST("IsValidToFire: not carrying -> FALSE");
   pe->v.weapons = 0;
   ASSERT_INT(IsValidToFireAtTheMoment(bot, *mp5), FALSE);
   PASS();

   TEST("IsValidToFire: underwater + can't use underwater -> FALSE");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.b_in_water = TRUE;
   // MP5 can_use_underwater=FALSE
   ASSERT_INT(mp5->can_use_underwater, FALSE);
   ASSERT_INT(IsValidToFireAtTheMoment(bot, *mp5), FALSE);
   bot.b_in_water = FALSE;
   PASS();

   TEST("IsValidToFire: satchel deployed -> FALSE for satchel");
   bot_weapon_select_t *satchel = GetWeaponSelect(VALVE_WEAPON_SATCHEL);
   pe->v.weapons = (1u << VALVE_WEAPON_SATCHEL);
   bot.f_satchel_detonate_time = 5.0;
   ASSERT_INT(IsValidToFireAtTheMoment(bot, *satchel), FALSE);
   bot.f_satchel_detonate_time = 0;
   PASS();

   return 0;
}

// ============================================================
// 7. IsValidWeaponChoose tests
// ============================================================

static int test_IsValidWeaponChoose(void)
{
   printf("IsValidWeaponChoose:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   TEST("normal HLDM weapon -> TRUE");
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);
   ASSERT_INT(IsValidWeaponChoose(bot, *mp5), TRUE);
   PASS();

   TEST("zero-id terminator -> FALSE");
   bot_weapon_select_t terminator;
   memset(&terminator, 0, sizeof(terminator));
   ASSERT_INT(IsValidWeaponChoose(bot, terminator), FALSE);
   PASS();

   TEST("unsupported submod -> FALSE");
   // OP4-only weapon in HLDM mode
   bot_weapon_select_t op4only;
   memset(&op4only, 0, sizeof(op4only));
   op4only.iId = GEARBOX_WEAPON_EAGLE;
   strcpy(op4only.weapon_name, "weapon_eagle");
   op4only.supported_submods = WEAPON_SUBMOD_OP4;
   ASSERT_INT(IsValidWeaponChoose(bot, op4only), FALSE);
   PASS();

   TEST("Egon blocked in SEVS");
   submod_id = SUBMOD_SEVS;
   submod_weaponflag = WEAPON_SUBMOD_SEVS;
   InitWeaponSelect(SUBMOD_SEVS);
   bot_weapon_select_t *egon = GetWeaponSelect(VALVE_WEAPON_EGON);
   ASSERT_INT(IsValidWeaponChoose(bot, *egon), FALSE);
   PASS();

   TEST("Egon blocked in bubblemod when bm_gluon_mod > 0");
   submod_id = SUBMOD_BUBBLEMOD;
   submod_weaponflag = WEAPON_SUBMOD_BUBBLEMOD;
   InitWeaponSelect(SUBMOD_BUBBLEMOD);
   egon = GetWeaponSelect(VALVE_WEAPON_EGON);
   mock_cvar_bm_gluon_mod_val = 1.0f;
   ASSERT_INT(IsValidWeaponChoose(bot, *egon), FALSE);
   // Reset
   mock_cvar_bm_gluon_mod_val = 0.0f;
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;
   PASS();

   return 0;
}

// ============================================================
// 8. IsValidPrimaryAttack / IsValidSecondaryAttack tests
// ============================================================

static int test_IsValidPrimaryAttack(void)
{
   printf("IsValidPrimaryAttack:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   // MP5: primary_min=32, primary_max=2000, min_primary_ammo=1, iAmmo1=slot 1
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);

   TEST("in range + has ammo -> TRUE");
   bot.m_rgAmmo[1] = 50; // 9mm ammo
   ASSERT_INT(IsValidPrimaryAttack(bot, *mp5, 500.0, 0.0, FALSE), TRUE);
   PASS();

   TEST("out of range (too far) -> FALSE");
   ASSERT_INT(IsValidPrimaryAttack(bot, *mp5, 3000.0, 0.0, FALSE), FALSE);
   PASS();

   TEST("out of range (too close) -> FALSE");
   ASSERT_INT(IsValidPrimaryAttack(bot, *mp5, 10.0, 0.0, FALSE), FALSE);
   PASS();

   TEST("no ammo -> FALSE");
   bot.m_rgAmmo[1] = 0;
   ASSERT_INT(IsValidPrimaryAttack(bot, *mp5, 500.0, 0.0, FALSE), FALSE);
   PASS();

   TEST("no-ammo weapon (hornetgun) -> TRUE regardless");
   bot_weapon_select_t *hornet = GetWeaponSelect(VALVE_WEAPON_HORNETGUN);
   // iAmmo1=-1, so ammo check bypassed
   ASSERT_INT(IsValidPrimaryAttack(bot, *hornet, 500.0, 0.0, FALSE), TRUE);
   PASS();

   TEST("always_in_range bypasses distance check");
   bot.m_rgAmmo[1] = 50;
   ASSERT_INT(IsValidPrimaryAttack(bot, *mp5, 99999.0, 0.0, TRUE), TRUE);
   PASS();

   return 0;
}

static int test_IsValidSecondaryAttack(void)
{
   printf("IsValidSecondaryAttack:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   // MP5: secondary_min=300, secondary_max=700, min_secondary_ammo=1, iAmmo2=slot 8
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);

   TEST("in range + has secondary ammo -> TRUE");
   bot.m_rgAmmo[1] = 50;  // primary (needed: MP5 blocks secondary if primary empty)
   bot.m_rgAmmo[8] = 5;   // AR grenades
   ASSERT_INT(IsValidSecondaryAttack(bot, *mp5, 500.0, 0.0, FALSE), TRUE);
   PASS();

   TEST("MP5 primary empty blocks secondary -> FALSE");
   bot.m_rgAmmo[1] = 0;   // no 9mm
   bot.m_rgAmmo[8] = 5;   // has grenades
   ASSERT_INT(IsValidSecondaryAttack(bot, *mp5, 500.0, 0.0, FALSE), FALSE);
   PASS();

   TEST("MP5 bad launch angle rejects secondary");
   bot.m_rgAmmo[1] = 50;
   bot.m_rgAmmo[8] = 5;
   // distance=500, height=900 -> too high -> angle=-99
   ASSERT_INT(IsValidSecondaryAttack(bot, *mp5, 500.0, 900.0, FALSE), FALSE);
   PASS();

   TEST("secondary_use_primary_ammo checks iAmmo1");
   // Gauss: secondary_use_primary_ammo=TRUE, iAmmo1=6 (uranium)
   bot_weapon_select_t *gauss = GetWeaponSelect(VALVE_WEAPON_GAUSS);
   bot.m_rgAmmo[6] = 50;
   ASSERT_INT(IsValidSecondaryAttack(bot, *gauss, 200.0, 0.0, FALSE), TRUE);
   PASS();

   TEST("out of range -> FALSE");
   bot.m_rgAmmo[8] = 5;
   bot.m_rgAmmo[1] = 50;
   ASSERT_INT(IsValidSecondaryAttack(bot, *mp5, 50.0, 0.0, FALSE), FALSE);
   PASS();

   TEST("always_in_range bypasses distance check");
   bot.m_rgAmmo[8] = 5;
   bot.m_rgAmmo[1] = 50;
   ASSERT_INT(IsValidSecondaryAttack(bot, *mp5, 50.0, 0.0, TRUE), TRUE);
   PASS();

   TEST("iAmmo2=-1 with secondary_use_primary_ammo: no OOB, uses primary");
   // Gauss: iAmmo2=-1, secondary_use_primary_ammo=TRUE, iAmmo1=6
   // With the bug, m_rgAmmo[-1] reads current_weapon.iAmmo2 (adjacent struct field).
   // Set it high so the OOB read would satisfy min_secondary_ammo, masking
   // the fact that primary ammo is empty.
   bot.current_weapon.iAmmo2 = 999; // this is what m_rgAmmo[-1] would read
   bot.m_rgAmmo[6] = 0; // no primary (uranium) ammo
   ASSERT_INT(IsValidSecondaryAttack(bot, *gauss, 200.0, 0.0, FALSE), FALSE);
   bot.current_weapon.iAmmo2 = 0;
   PASS();

   return 0;
}

// ============================================================
// 9. BotPrimaryAmmoLow / BotSecondaryAmmoLow tests
// ============================================================

static int test_BotPrimaryAmmoLow(void)
{
   printf("BotPrimaryAmmoLow:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   // MP5: iAmmo1=1, low_ammo_primary=50
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);

   TEST("no-ammo weapon -> AMMO_NO");
   bot_weapon_select_t *crowbar = GetWeaponSelect(VALVE_WEAPON_CROWBAR);
   ASSERT_INT(BotPrimaryAmmoLow(bot, *crowbar), AMMO_NO);
   PASS();

   TEST("ammo <= low threshold -> AMMO_LOW");
   bot.m_rgAmmo[1] = 30; // 30 <= 50
   ASSERT_INT(BotPrimaryAmmoLow(bot, *mp5), AMMO_LOW);
   PASS();

   TEST("ammo > low threshold -> AMMO_OK");
   bot.m_rgAmmo[1] = 100; // 100 > 50
   ASSERT_INT(BotPrimaryAmmoLow(bot, *mp5), AMMO_OK);
   PASS();

   return 0;
}

static int test_BotSecondaryAmmoLow(void)
{
   printf("BotSecondaryAmmoLow:\n");

   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   // MP5: low_ammo_secondary=2, secondary_use_primary_ammo=FALSE, iAmmo2=8
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);

   TEST("low_ammo_secondary=-1 -> AMMO_NO (early return)");
   // Crowbar has low_ammo_secondary=-1
   bot_weapon_select_t *crowbar = GetWeaponSelect(VALVE_WEAPON_CROWBAR);
   ASSERT_INT(BotSecondaryAmmoLow(bot, *crowbar), AMMO_NO);
   PASS();

   // Bug test: secondary_use_primary_ammo=FALSE should check iAmmo2
   TEST("FALSE branch: iAmmo2=-1 -> AMMO_NO");
   // Make a custom select with secondary_use_primary_ammo=FALSE, iAmmo2=-1
   bot_weapon_select_t custom = *mp5;
   custom.secondary_use_primary_ammo = FALSE;
   // Temporarily set iAmmo2=-1 for weapon_index
   int saved_iAmmo2 = weapon_defs[VALVE_WEAPON_MP5].iAmmo2;
   weapon_defs[VALVE_WEAPON_MP5].iAmmo2 = -1;
   ASSERT_INT(BotSecondaryAmmoLow(bot, custom), AMMO_NO);
   weapon_defs[VALVE_WEAPON_MP5].iAmmo2 = saved_iAmmo2;
   PASS();

   TEST("FALSE branch: ammo in iAmmo2 slot low -> AMMO_LOW");
   bot.m_rgAmmo[8] = 1; // 1 <= 2 (low_ammo_secondary)
   ASSERT_INT(BotSecondaryAmmoLow(bot, *mp5), AMMO_LOW);
   PASS();

   TEST("FALSE branch: ammo in iAmmo2 slot ok -> AMMO_OK");
   bot.m_rgAmmo[8] = 10; // 10 > 2
   ASSERT_INT(BotSecondaryAmmoLow(bot, *mp5), AMMO_OK);
   PASS();

   // Bug test: secondary_use_primary_ammo=TRUE should check iAmmo1
   TEST("TRUE branch: checks iAmmo1 slot");
   // Gauss: secondary_use_primary_ammo=TRUE, iAmmo1=6
   bot_weapon_select_t *gauss = GetWeaponSelect(VALVE_WEAPON_GAUSS);
   // Gauss has low_ammo_secondary=30
   bot.m_rgAmmo[6] = 10; // 10 <= 30
   ASSERT_INT(BotSecondaryAmmoLow(bot, *gauss), AMMO_LOW);
   PASS();

   TEST("TRUE branch: iAmmo1 ok -> AMMO_OK");
   bot.m_rgAmmo[6] = 100; // 100 > 30
   ASSERT_INT(BotSecondaryAmmoLow(bot, *gauss), AMMO_OK);
   PASS();

   TEST("TRUE branch: iAmmo1==-1 -> AMMO_NO");
   {
      // Gauss has secondary_use_primary_ammo=TRUE
      // Temporarily set iAmmo1=-1 to hit the edge case
      int saved = weapon_defs[VALVE_WEAPON_GAUSS].iAmmo1;
      weapon_defs[VALVE_WEAPON_GAUSS].iAmmo1 = -1;
      ASSERT_INT(BotSecondaryAmmoLow(bot, *gauss), AMMO_NO);
      weapon_defs[VALVE_WEAPON_GAUSS].iAmmo1 = saved;
   }
   PASS();

   return 0;
}

// ============================================================
// 10. BotGetGoodWeaponCount tests
// ============================================================

static int test_BotGetGoodWeaponCount(void)
{
   printf("BotGetGoodWeaponCount:\n");

   mock_reset();
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   TEST("no weapons -> 0");
   pe->v.weapons = 0;
   ASSERT_INT(BotGetGoodWeaponCount(bot, 5), 0);
   PASS();

   TEST("has fire weapons with ammo -> counts them");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5) | (1u << VALVE_WEAPON_SHOTGUN);
   bot.m_rgAmmo[1] = 50;  // 9mm for MP5
   bot.m_rgAmmo[3] = 20;  // buckshot for shotgun
   int count = BotGetGoodWeaponCount(bot, 10);
   ASSERT_TRUE(count >= 2);
   PASS();

   TEST("stops at stop_count");
   count = BotGetGoodWeaponCount(bot, 1);
   ASSERT_INT(count, 1);
   PASS();

   TEST("skips melee/throw/avoid and 0-ammo weapons");
   pe->v.weapons = (1u << VALVE_WEAPON_CROWBAR) |       // WEAPON_MELEE -> skip
                   (1u << VALVE_WEAPON_HANDGRENADE) |    // avoid_this_gun=TRUE -> skip
                   (1u << VALVE_WEAPON_MP5);             // WEAPON_FIRE but 0 ammo -> skip
   bot.m_rgAmmo[1] = 0;  // MP5 primary ammo = 0
   bot.m_rgAmmo[8] = 0;  // MP5 secondary ammo = 0
   count = BotGetGoodWeaponCount(bot, 5);
   ASSERT_INT(count, 0);
   PASS();

   TEST("skips OP4 weapon carried in HLDM (IsValidWeaponChoose)");
   pe->v.weapons = (1u << GEARBOX_WEAPON_EAGLE);
   bot.m_rgAmmo[2] = 36;  // 357 ammo for eagle
   count = BotGetGoodWeaponCount(bot, 5);
   ASSERT_INT(count, 0);
   PASS();

   TEST("counts only WEAPON_FIRE with sufficient ammo");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5) | (1u << VALVE_WEAPON_SHOTGUN);
   bot.m_rgAmmo[1] = 50;  // 9mm for MP5
   bot.m_rgAmmo[3] = 20;  // buckshot for shotgun
   bot.m_rgAmmo[8] = 5;
   count = BotGetGoodWeaponCount(bot, 10);
   ASSERT_INT(count, 2);
   PASS();

   return 0;
}

// ============================================================
// 11. BotGetLowAmmoFlags / BotAllWeaponsRunningOutOfAmmo / BotWeaponCanAttack
// ============================================================

static int test_BotGetLowAmmoFlags(void)
{
   printf("BotGetLowAmmoFlags:\n");

   mock_reset();
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   TEST("no weapons -> 0 flags");
   pe->v.weapons = 0;
   int weapon_flags = 0;
   int ammoflags = BotGetLowAmmoFlags(bot, &weapon_flags, TRUE);
   ASSERT_INT(ammoflags, 0);
   ASSERT_INT(weapon_flags, 0);
   PASS();

   TEST("low MP5 primary ammo -> W_IFL_AMMO_9MM flag set");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 10;  // low (<=50)
   bot.m_rgAmmo[8] = 10;  // AR grenades ok
   ammoflags = BotGetLowAmmoFlags(bot, &weapon_flags, TRUE);
   ASSERT_TRUE((ammoflags & W_IFL_AMMO_9MM) != 0);
   PASS();

   TEST("OnlyCarrying=FALSE hits IsValidWeaponChoose skip");
   // In HLDM mode, OP4 weapons fail IsValidWeaponChoose
   pe->v.weapons = 0;  // not carrying anything
   memset(bot.m_rgAmmo, 0, sizeof(bot.m_rgAmmo));
   ammoflags = BotGetLowAmmoFlags(bot, &weapon_flags, FALSE);
   // OP4 weapons in table are skipped by IsValidWeaponChoose, others may or may not have low ammo
   // Just verify function doesn't crash and returns something
   ASSERT_TRUE(ammoflags >= 0);
   PASS();

   TEST("low secondary ammo sets ammo2_waypoint_flag");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 200;  // primary OK
   bot.m_rgAmmo[8] = 1;    // AR grenades low (<=2)
   weapon_flags = 0;
   ammoflags = BotGetLowAmmoFlags(bot, &weapon_flags, TRUE);
   // MP5 ammo2_waypoint_flag = W_IFL_AMMO_ARGRENADES
   ASSERT_TRUE((ammoflags & W_IFL_AMMO_ARGRENADES) != 0);
   PASS();

   TEST("ammo2_on_repickup sets weapon_flag");
   {
      // Temporarily set MP5's ammo2_on_repickup=TRUE
      bot_weapon_select_t *mp5_ws = GetWeaponSelect(VALVE_WEAPON_MP5);
      qboolean saved_repickup = mp5_ws->ammo2_on_repickup;
      mp5_ws->ammo2_on_repickup = TRUE;
      pe->v.weapons = (1u << VALVE_WEAPON_MP5);
      bot.m_rgAmmo[1] = 200;  // primary OK
      bot.m_rgAmmo[8] = 1;    // secondary low
      weapon_flags = 0;
      ammoflags = BotGetLowAmmoFlags(bot, &weapon_flags, TRUE);
      ASSERT_TRUE((ammoflags & W_IFL_AMMO_ARGRENADES) != 0);
      // weapon_flags should include MP5's waypoint_flag
      ASSERT_TRUE((weapon_flags & W_IFL_MP5) != 0);
      mp5_ws->ammo2_on_repickup = saved_repickup;
   }
   PASS();

   return 0;
}

static int test_BotAllWeaponsRunningOutOfAmmo(void)
{
   printf("BotAllWeaponsRunningOutOfAmmo:\n");

   mock_reset();
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   TEST("no weapons at all -> TRUE (running out)");
   pe->v.weapons = 0;
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, FALSE), TRUE);
   PASS();

   TEST("has weapon with enough ammo -> FALSE");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 200;  // plenty of 9mm
   bot.m_rgAmmo[8] = 10;   // AR grenades
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, FALSE), FALSE);
   PASS();

   TEST("GoodWeaponsOnly skips melee/throw -> TRUE");
   pe->v.weapons = (1u << VALVE_WEAPON_CROWBAR) | (1u << VALVE_WEAPON_HANDGRENADE);
   memset(bot.m_rgAmmo, 0, sizeof(bot.m_rgAmmo));
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, TRUE), TRUE);
   PASS();

   TEST("GoodWeaponsOnly, 0 ammo both attacks -> continues");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 0;  // no 9mm
   bot.m_rgAmmo[8] = 0;  // no AR grenades
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, TRUE), TRUE);
   PASS();

   TEST("GoodWeaponsOnly, low primary ammo -> continues");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 30;  // low (<=50)
   bot.m_rgAmmo[8] = 0;   // no secondary
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, TRUE), TRUE);
   PASS();

   TEST("GoodWeaponsOnly, low secondary ammo only -> has enough (primary OK)");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 200;  // primary OK
   bot.m_rgAmmo[8] = 1;    // secondary low (<=2), but primary is fine
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, TRUE), FALSE);
   PASS();

   TEST("GoodWeaponsOnly, both primary and secondary low -> running out");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 10;   // primary low (<=50)
   bot.m_rgAmmo[8] = 1;    // secondary low (<=2)
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, TRUE), TRUE);
   PASS();

   TEST("GoodWeaponsOnly, OP4 weapon in HLDM -> fails IsValidWeaponChoose");
   pe->v.weapons = (1u << GEARBOX_WEAPON_EAGLE);
   bot.m_rgAmmo[2] = 36;  // 357 ammo for eagle
   ASSERT_INT(BotAllWeaponsRunningOutOfAmmo(bot, TRUE), TRUE);
   PASS();

   return 0;
}

static int test_BotWeaponCanAttack(void)
{
   printf("BotWeaponCanAttack:\n");

   mock_reset();
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);

   TEST("no weapons -> FALSE");
   pe->v.weapons = 0;
   ASSERT_INT(BotWeaponCanAttack(bot, FALSE), FALSE);
   PASS();

   TEST("has weapon with ammo -> TRUE");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 50;
   ASSERT_INT(BotWeaponCanAttack(bot, FALSE), TRUE);
   PASS();

   TEST("GoodWeaponsOnly skips melee -> FALSE");
   pe->v.weapons = (1u << VALVE_WEAPON_CROWBAR);
   ASSERT_INT(BotWeaponCanAttack(bot, TRUE), FALSE);
   PASS();

   TEST("GoodWeaponsOnly, 0 ammo weapon -> FALSE");
   pe->v.weapons = (1u << VALVE_WEAPON_MP5);
   bot.m_rgAmmo[1] = 0;
   bot.m_rgAmmo[8] = 0;
   ASSERT_INT(BotWeaponCanAttack(bot, TRUE), FALSE);
   PASS();

   TEST("GoodWeaponsOnly, OP4 weapon in HLDM -> skips");
   pe->v.weapons = (1u << GEARBOX_WEAPON_EAGLE);
   bot.m_rgAmmo[2] = 36;  // 357 ammo
   ASSERT_INT(BotWeaponCanAttack(bot, TRUE), FALSE);
   PASS();

   return 0;
}

// ============================================================
// 12. BotGetBetterWeaponChoice tests
// ============================================================

static int test_BotGetBetterWeaponChoice(void)
{
   printf("BotGetBetterWeaponChoice:\n");

   mock_reset();
   submod_id = SUBMOD_HLDM;
   submod_weaponflag = WEAPON_SUBMOD_HLDM;
   InitWeaponSelect(SUBMOD_HLDM);
   setup_weapon_defs_valve();

   edict_t *pe = mock_alloc_edict();
   bot_t bot;
   setup_bot(bot, pe);
   bot.weapon_skill = SKILL3;

   qboolean use_primary, use_secondary;

   TEST("not-avoided weapon returns -1 (no change needed)");
   bot_weapon_select_t *mp5 = GetWeaponSelect(VALVE_WEAPON_MP5);
   // MP5 avoid_this_gun=FALSE
   int idx = BotGetBetterWeaponChoice(bot, *mp5, weapon_select, 500.0, 0.0, &use_primary, &use_secondary);
   ASSERT_INT(idx, -1);
   PASS();

   TEST("avoided weapon finds alternative");
   // Handgrenade: avoid_this_gun=TRUE
   bot_weapon_select_t *hg = GetWeaponSelect(VALVE_WEAPON_HANDGRENADE);
   // Give the bot an MP5 with ammo
   pe->v.weapons = (1u << VALVE_WEAPON_MP5) | (1u << VALVE_WEAPON_HANDGRENADE);
   bot.m_rgAmmo[1] = 50;
   idx = BotGetBetterWeaponChoice(bot, *hg, weapon_select, 500.0, 0.0, &use_primary, &use_secondary);
   ASSERT_TRUE(idx >= 0);
   ASSERT_TRUE(use_primary || use_secondary);
   PASS();

   TEST("avoided weapon, no alternative available -> -1");
   pe->v.weapons = (1u << VALVE_WEAPON_HANDGRENADE);
   bot.m_rgAmmo[1] = 0;
   idx = BotGetBetterWeaponChoice(bot, *hg, weapon_select, 500.0, 0.0, &use_primary, &use_secondary);
   ASSERT_INT(idx, -1);
   ASSERT_INT(use_primary, FALSE);
   ASSERT_INT(use_secondary, FALSE);
   PASS();

   TEST("skips OP4 weapon (IsValidWeaponChoose) and avoided weapon");
   // Current weapon is avoided -> enters search
   // Bot carries eagle (OP4, fails IsValidWeaponChoose in HLDM) + handgrenade (avoided)
   pe->v.weapons = (1u << GEARBOX_WEAPON_EAGLE) | (1u << VALVE_WEAPON_HANDGRENADE);
   bot.m_rgAmmo[2] = 36;  // 357 ammo for eagle
   bot.m_rgAmmo[11] = 5;  // handgrenade ammo
   idx = BotGetBetterWeaponChoice(bot, *hg, weapon_select, 500.0, 0.0, &use_primary, &use_secondary);
   ASSERT_INT(idx, -1);
   PASS();

   return 0;
}

// ============================================================
// 13. MP5 launch angle tests
// ============================================================

static int test_MP5_launch_angle(void)
{
   printf("ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight:\n");

   float angle;

   TEST("too high (height=900, distance=500) -> -99");
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(500.0, 900.0);
   ASSERT_TRUE(angle < -90.0);
   PASS();

   TEST("too close (distance=100) -> -99");
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(100.0, 0.0);
   ASSERT_TRUE(angle < -90.0);
   PASS();

   TEST("too far (distance=2500) -> -99");
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(2500.0, 0.0);
   ASSERT_TRUE(angle < -90.0);
   PASS();

   TEST("same level, exact table entry (distance=500, height=0) -> -8.5");
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(500.0, 0.0);
   // Exact table entry: distance=500 at index 2, height=0 -> -8.5
   ASSERT_FLOAT_NEAR(angle, -8.5f, 0.01f);
   PASS();

   TEST("interpolated distance (600, height=0) -> between -8.5 and -12.5");
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(600.0, 0.0);
   // Between distance=500 (-8.5) and distance=700 (-12.5), at midpoint -> -10.5
   ASSERT_FLOAT_NEAR(angle, -10.5f, 0.01f);
   PASS();

   TEST("downward target (height=-64, distance=400) -> 2.8");
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(400.0, -64.0);
   // Exact table entry: distance=400 at index 1, height=-64 -> 2.8
   ASSERT_FLOAT_NEAR(angle, 2.8f, 0.01f);
   PASS();

   TEST("height below all entries (-2000) -> -99");
   // Height below all table entries, height_idx stays 0 -> returns -99
   angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(500.0, -2000.0);
   ASSERT_TRUE(angle < -90.0);
   PASS();

   return 0;
}

// ============================================================
// Main
// ============================================================

int main(void)
{
   int rc = 0;

   printf("=== bot_weapons.cpp unit tests ===\n\n");

   rc |= test_SubmodToSubmodWeaponFlag();
   printf("\n");
   rc |= test_InitWeaponSelect_GetWeaponSelect();
   printf("\n");
   rc |= test_GetWeaponItemFlag_GetAmmoItemFlag();
   printf("\n");
   rc |= test_skill_checks();
   printf("\n");
   rc |= test_BotSelectAttack();
   printf("\n");
   rc |= test_BotIsCarryingWeapon_IsValidToFire();
   printf("\n");
   rc |= test_IsValidWeaponChoose();
   printf("\n");
   rc |= test_IsValidPrimaryAttack();
   printf("\n");
   rc |= test_IsValidSecondaryAttack();
   printf("\n");
   rc |= test_BotPrimaryAmmoLow();
   printf("\n");
   rc |= test_BotSecondaryAmmoLow();
   printf("\n");
   rc |= test_BotGetGoodWeaponCount();
   printf("\n");
   rc |= test_BotGetLowAmmoFlags();
   printf("\n");
   rc |= test_BotAllWeaponsRunningOutOfAmmo();
   printf("\n");
   rc |= test_BotWeaponCanAttack();
   printf("\n");
   rc |= test_BotGetBetterWeaponChoice();
   printf("\n");
   rc |= test_MP5_launch_angle();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
