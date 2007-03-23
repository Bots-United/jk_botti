//
// JK_Botti - be more human!
//
// bot_weapon_select.h
//

#define NOSKILL 0 // better than best required == N/A

#define SKILL1 1 // best
#define SKILL2 2 
#define SKILL3 3
#define SKILL4 4
#define SKILL5 5 // worst

typedef struct
{
   int iId;  // the weapon ID value
   char  weapon_name[64];  // name of the weapon when selecting it
   
   int   primary_skill_level;   // bot skill must be less than or equal to this value
   int   secondary_skill_level; // bot skill must be less than or equal to this value
   qboolean  avoid_this_gun; // bot avoids using this weapon if possible
   qboolean  prefer_higher_skill_attack; // bot uses better of primary and secondary attacks if both avaible
      
   float primary_min_distance;   // 0 = no minimum
   float primary_max_distance;   // 9999 = no maximum
   float secondary_min_distance; // 0 = no minimum    
   float secondary_max_distance; // 9999 = no maximum
   float opt_distance; // optimal distance from target
   
   int   use_percent;   // times out of 100 to use this weapon when available
   qboolean  can_use_underwater;     // can use this weapon underwater
   int   primary_fire_percent;   // times out of 100 to use primary fire
   int   min_primary_ammo;       // minimum ammout of primary ammo needed to fire
   int   min_secondary_ammo;     // minimum ammout of seconday ammo needed to fire
   qboolean  primary_fire_hold;      // hold down primary fire button to use?
   qboolean  secondary_fire_hold;    // hold down secondary fire button to use?
   qboolean  primary_fire_charge;    // charge weapon using primary fire?
   qboolean  secondary_fire_charge;  // charge weapon using secondary fire?
   float primary_charge_delay;   // time to charge weapon
   float secondary_charge_delay; // time to charge weapon
   qboolean  secondary_use_primary_ammo; //does secondary fire use primary ammo?
   int   low_ammo_primary;
   int   low_ammo_secondary;
} bot_weapon_select_t;

typedef struct
{
   int iId;
   float primary_base_delay;
   float primary_min_delay[5];
   float primary_max_delay[5];
   float secondary_base_delay;
   float secondary_min_delay[5];
   float secondary_max_delay[5];
} bot_fire_delay_t;

void GetWeaponSelect(bot_weapon_select_t **pSelect, bot_fire_delay_t **pDelay);

#ifdef BOTCOMBAT

// weapons are stored in priority order, most desired weapon should be at
// the start of the array and least desired should be at the end

bot_weapon_select_t valve_weapon_select[] = {

   {VALVE_WEAPON_CROWBAR, "weapon_crowbar", 
    SKILL4, NOSKILL, FALSE, FALSE,
    0.0, 64.0, 0, 0, 1.0,
    20, TRUE, 100, 0, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, -1, -1},

//disabled
//   {VALVE_WEAPON_HANDGRENADE, "weapon_handgrenade", 
//    SKILL3, NOSKILL,
//    250.0, 750.0, 300.0, 600.0, 300.0
//    5, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, -1, -1},

   {VALVE_WEAPON_SNARK, "weapon_snark", 
    SKILL3, NOSKILL, FALSE, FALSE,
    150.0, 600.0, 0, 0, 300.0,
    20, FALSE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, -1, -1},

   {VALVE_WEAPON_EGON, "weapon_egon", 
    SKILL3, NOSKILL, FALSE, FALSE,
    200.0, 2000.0, 0, 0, 350.0,
    50, FALSE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 20, -1},

   {VALVE_WEAPON_GAUSS, "weapon_gauss", 
    SKILL4, SKILL1, FALSE, TRUE,
    100.0, 500.0, 200.0, 9999.0, 500.0,
    20, FALSE, 80, 1, 10, TRUE, FALSE, FALSE, TRUE, 0.0, 0.8, TRUE, 30, 30},

   {VALVE_WEAPON_SHOTGUN, "weapon_shotgun", 
    SKILL5, SKILL3, FALSE, TRUE,
    400.0, 800.0, 50.0, 600.0, 400.0,
    55, FALSE, 70, 1, 2, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 12, 12},

   {VALVE_WEAPON_PYTHON, "weapon_357", 
    SKILL3, NOSKILL, FALSE, FALSE,
    300.0, 9999, 0, 0, 750.0,
    15, FALSE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 12, 12},

   {VALVE_WEAPON_HORNETGUN, "weapon_hornetgun", 
    SKILL4, SKILL3, FALSE, FALSE,
    200.0, 1500.0, 70.0, 250.0, 300.0,
    10, TRUE, 50, 1, 4, TRUE, TRUE, FALSE, FALSE, 0.0, 0.0, TRUE, -1, -1},

   {VALVE_WEAPON_MP5, "weapon_9mmAR", 
    SKILL5, SKILL2, FALSE, FALSE,
    200.0, 2000.0, 300.0, 700.0, 600.0,
    55, FALSE, 90, 1, 1, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 50, 2},

//disabled
//   {VALVE_WEAPON_CROSSBOW, "weapon_crossbow", 
//    SKILL5, SKILL1,
//    150.0, 500.0, 800, 9999.0, 1000.0
//    5, TRUE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 5, -1},
   
   {VALVE_WEAPON_RPG, "weapon_rpg", 
    SKILL3, NOSKILL, FALSE, FALSE,
    500.0, 9999.0, 0.0, 0.0, 700.0,
    60, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 2, -1},
    
   {VALVE_WEAPON_GLOCK, "weapon_9mmhandgun", 
    SKILL5, SKILL3, TRUE, TRUE,
    300.0, 3000.0, 80.0, 350.0, 300.0,
    20, TRUE, 70, 1, 1, TRUE, TRUE, FALSE, FALSE, 0.0, 0.0, TRUE, 30, -1},

   /* terminator */   
   {0, "", NOSKILL, NOSKILL, FALSE, FALSE, 0.0, 0.0, 0.0, 0.0, 0.0, 0, FALSE, 0, 1, 1, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 0, 0},
};

// weapon firing delay based on skill (min and max delay for each weapon)
// THESE MUST MATCH THE SAME ORDER AS THE WEAPON SELECT ARRAY!!!

bot_fire_delay_t valve_fire_delay[] = {
   {VALVE_WEAPON_CROWBAR,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
//   {VALVE_WEAPON_HANDGRENADE,
//    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
//    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_SNARK,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_EGON,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_GAUSS,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_SHOTGUN,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_PYTHON,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_HORNETGUN,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_MP5,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
//   {VALVE_WEAPON_CROSSBOW,
//    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
//    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_RPG,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_GLOCK,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   /* terminator */
   {0, 0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
       0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}}
};

#else

extern bot_weapon_select_t valve_weapon_select[];
extern bot_fire_delay_t valve_fire_delay[];

#endif
