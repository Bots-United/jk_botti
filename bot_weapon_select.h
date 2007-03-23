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

// waypoint item flags
#define W_IFL_CROWBAR         (1<<0)
#define W_IFL_HANDGRENADE     (1<<1)
#define W_IFL_SNARK           (1<<2)
#define W_IFL_EGON            (1<<3)
#define W_IFL_GAUSS           (1<<4)
#define W_IFL_SHOTGUN         (1<<5)
#define W_IFL_PYTHON          (1<<6)
#define W_IFL_HORNETGUN       (1<<7)
#define W_IFL_MP5             (1<<8)
#define W_IFL_CROSSBOW        (1<<9)
#define W_IFL_RPG             (1<<10)
#define W_IFL_GLOCK           (1<<11)

#define W_IFL_AMMO_GAUSS      (1<<12)
#define W_IFL_AMMO_BUCKSHOT   (1<<13)
#define W_IFL_AMMO_357        (1<<14)
#define W_IFL_AMMO_9MM        (1<<15)
#define W_IFL_AMMO_ARGRENADES (1<<16)
#define W_IFL_AMMO_CROSSBOW   (1<<17)
#define W_IFL_AMMO_RPG        (1<<18)

// weapon types
#define WEAPON_FIRE  1
#define WEAPON_MELEE 2
#define WEAPON_THROW 3

typedef struct
{
   int iId;  // the weapon ID value
   char  weapon_name[64];  // name of the weapon when selecting it
   int type;
   
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
   
   int  waypoint_flag;
   int  ammo1_waypoint_flag;
   int  ammo2_waypoint_flag;
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

typedef struct
{
   int waypoint_flag;
   char ammoName[64];
} bot_ammo_names_t;

#ifdef BOTWEAPONS

// weapons are stored in priority order, most desired weapon should be at
// the start of the array and least desired should be at the end

bot_weapon_select_t weapon_select[] = 
{
   {VALVE_WEAPON_CROWBAR, "weapon_crowbar", WEAPON_MELEE,
    SKILL4, NOSKILL, FALSE, FALSE,
    0.0, 64.0, 0, 0, 1.0,
    20, TRUE, 100, 0, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, -1, -1,
    W_IFL_CROWBAR, 0, 0 },

   {VALVE_WEAPON_HANDGRENADE, "weapon_handgrenade", WEAPON_THROW,
    SKILL4, NOSKILL, TRUE, FALSE,
    250.0, 750.0, 300.0, 600.0, 300.0,
    10, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, -1, -1,
    W_IFL_HANDGRENADE, 0, 0 },

   {VALVE_WEAPON_SNARK, "weapon_snark", WEAPON_THROW,
    SKILL3, NOSKILL, FALSE, FALSE,
    150.0, 600.0, 0, 0, 300.0,
    20, FALSE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, -1, -1,
    W_IFL_SNARK, 0, 0 },

   {VALVE_WEAPON_EGON, "weapon_egon", WEAPON_FIRE,
    SKILL3, NOSKILL, FALSE, FALSE,
    200.0, 2000.0, 0, 0, 350.0,
    50, FALSE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 20, -1,
    W_IFL_EGON, W_IFL_AMMO_GAUSS, 0 },

   {VALVE_WEAPON_GAUSS, "weapon_gauss", WEAPON_FIRE,
    SKILL4, SKILL1, FALSE, TRUE,
    100.0, 500.0, 200.0, 9999.0, 500.0,
    20, FALSE, 80, 1, 10, TRUE, FALSE, FALSE, TRUE, 0.0, 0.8, TRUE, 30, 30,
    W_IFL_GAUSS, W_IFL_AMMO_GAUSS, 0 },

   {VALVE_WEAPON_SHOTGUN, "weapon_shotgun", WEAPON_FIRE,
    SKILL5, SKILL3, FALSE, TRUE,
    400.0, 800.0, 50.0, 600.0, 400.0,
    55, FALSE, 70, 1, 2, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 12, 12,
    W_IFL_SHOTGUN, W_IFL_AMMO_BUCKSHOT, 0 },

   {VALVE_WEAPON_PYTHON, "weapon_357", WEAPON_FIRE,
    SKILL3, NOSKILL, FALSE, FALSE,
    300.0, 9999, 0, 0, 750.0,
    15, FALSE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 12, 12,
    W_IFL_PYTHON, W_IFL_AMMO_357, 0 },

   {VALVE_WEAPON_HORNETGUN, "weapon_hornetgun", WEAPON_FIRE,
    SKILL4, SKILL3, FALSE, FALSE,
    200.0, 1500.0, 70.0, 250.0, 300.0,
    10, TRUE, 50, 1, 4, TRUE, TRUE, FALSE, FALSE, 0.0, 0.0, TRUE, -1, -1,
    W_IFL_HORNETGUN, 0, 0 },

   {VALVE_WEAPON_MP5, "weapon_9mmAR", WEAPON_FIRE,
    SKILL5, SKILL2, FALSE, FALSE,
    200.0, 2000.0, 300.0, 700.0, 600.0,
    55, FALSE, 90, 1, 1, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 50, 2,
    W_IFL_MP5, W_IFL_AMMO_9MM, 0 },

   {VALVE_WEAPON_CROSSBOW, "weapon_crossbow", WEAPON_FIRE,
    SKILL5, SKILL2, TRUE, TRUE,
    150.0, 500.0, 800, 9999.0, 1000.0,
    10, TRUE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 5, -1,
    W_IFL_CROSSBOW, W_IFL_AMMO_CROSSBOW, 0 },
   
   {VALVE_WEAPON_RPG, "weapon_rpg", WEAPON_FIRE,
    SKILL3, NOSKILL, FALSE, FALSE,
    500.0, 9999.0, 0.0, 0.0, 700.0,
    60, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 2, -1,
    W_IFL_RPG, W_IFL_AMMO_RPG, 0 },
    
   {VALVE_WEAPON_GLOCK, "weapon_9mmhandgun", WEAPON_FIRE,
    SKILL5, SKILL3, TRUE, TRUE,
    300.0, 3000.0, 80.0, 350.0, 300.0,
    20, TRUE, 70, 1, 1, TRUE, TRUE, FALSE, FALSE, 0.0, 0.0, TRUE, 30, -1,
    W_IFL_GLOCK, W_IFL_AMMO_9MM, 0 },
    
   /* terminator */   
   {0, "", 0,
    NOSKILL, NOSKILL, FALSE, FALSE, 
    0.0, 0.0, 0.0, 0.0, 0.0,
    0, FALSE, 0, 1, 1, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 0, 0, 
    0, 0, 0 },
};

// List of different ammo we are looking for
bot_ammo_names_t ammo_names[] = {
   { W_IFL_AMMO_GAUSS     , "ammo_gaussclip" },
   { W_IFL_AMMO_BUCKSHOT  , "ammo_buckshot" },
   { W_IFL_AMMO_357       , "ammo_357" },
   { W_IFL_AMMO_9MM       , "ammo_9mmclip" },
   { W_IFL_AMMO_9MM       , "ammo_9mmAR" },
   { W_IFL_AMMO_ARGRENADES, "ammo_ARgrenades" },
   { W_IFL_AMMO_CROSSBOW  , "ammo_crossbow" },
   { W_IFL_AMMO_RPG       , "ammo_rpgclip" },
   { 0, "" },
};

// weapon firing delay based on skill (min and max delay for each weapon)
// THESE MUST MATCH THE SAME ORDER AS THE WEAPON SELECT ARRAY!!!

bot_fire_delay_t fire_delay[] = {
   {VALVE_WEAPON_CROWBAR,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
   {VALVE_WEAPON_HANDGRENADE,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
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
  {VALVE_WEAPON_CROSSBOW,
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}},
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

extern bot_weapon_select_t weapon_select[];
extern bot_fire_delay_t fire_delay[];
extern bot_ammo_names_t ammo_names[];

#endif

enum ammo_low_t {
   AMMO_NO = 1,
   AMMO_LOW = 2,
   AMMO_OK = 3,
};

extern int GetAmmoItemFlag(const char * classname);
extern int GetWeaponItemFlag(const char * classname);
extern qboolean BotCanUseWeapon(bot_t &pBot, const bot_weapon_select_t &select);
extern ammo_low_t BotPrimaryAmmoLow(bot_t &pBot, const bot_weapon_select_t &select);
extern ammo_low_t BotSecondaryAmmoLow(bot_t &pBot, const bot_weapon_select_t &select);
extern void BotSelectAttack(bot_t &pBot, const bot_weapon_select_t &select, qboolean &use_primary, qboolean &use_secondary);
extern qboolean IsValidWeaponChoose(bot_t &pBot, const bot_weapon_select_t &select);
extern qboolean IsValidPrimaryAttack(bot_t &pBot, const bot_weapon_select_t &select, const float distance, const qboolean always_in_range);
extern qboolean IsValidSecondaryAttack(bot_t &pBot, const bot_weapon_select_t &select, const float distance, const qboolean always_in_range);
extern qboolean BotGetGoodWeaponCount(bot_t &pBot, const int stop_count);
extern int BotGetLowAmmoFlags(bot_t &pBot);
extern int BotGetBetterWeaponChoice(bot_t &pBot, const bot_weapon_select_t &current, const bot_weapon_select_t *pSelect, const float distance, qboolean *use_primary, qboolean *use_secondary);
extern qboolean BotAllWeaponsRunningOutOfAmmo(bot_t &pBot);
extern qboolean IsValidToFireAtTheMoment(bot_t &pBot, const bot_weapon_select_t &select) ;
