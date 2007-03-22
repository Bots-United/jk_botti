typedef struct
{
   int iId;  // the weapon ID value
   char  weapon_name[64];  // name of the weapon when selecting it
   int   skill_level;   // bot skill must be less than or equal to this value
   float primary_min_distance;   // 0 = no minimum
   float primary_max_distance;   // 9999 = no maximum
   float secondary_min_distance; // 0 = no minimum    
   float secondary_max_distance; // 9999 = no maximum
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
   float opt_distance; // optimal distance from target
   qboolean  secondary_zooms; // zooming weapon?
   qboolean  not_on_severians;
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

   {VALVE_WEAPON_CROWBAR, "weapon_crowbar", 3, 0.0, 32.0, 0.0, 0.0,
    20, TRUE, 100, 0, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 30.0, FALSE, FALSE},

   {VALVE_WEAPON_HANDGRENADE, "weapon_handgrenade", 0, 250.0, 750.0, 300.0, 600.0,
    0, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 300.0, FALSE, FALSE}, //disabled (by skill and usage-%)

   {VALVE_WEAPON_SNARK, "weapon_snark", 3, 150.0, 600.0, 0.0, 0.0,
    20, FALSE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 300.0, FALSE, FALSE},

   {VALVE_WEAPON_EGON, "weapon_egon", 4, 200.0, 2000.0, 0.0, 0.0,
    50, FALSE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 250.0, FALSE, TRUE},

   {VALVE_WEAPON_GAUSS, "weapon_gauss", 1, 100.0, 750.0, 500.0, 9999.0,
    20, FALSE, 80, 1, 10, TRUE, FALSE, FALSE, TRUE, 0.0, 0.8, TRUE, 450.0, FALSE, FALSE},

   {VALVE_WEAPON_SHOTGUN, "weapon_shotgun", 5, 400.0, 600.0, 50.0, 400.0,
    55, FALSE, 70, 1, 2, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 200.0, FALSE, FALSE},

   {VALVE_WEAPON_PYTHON, "weapon_357", 2, 100.0, 9999, 0, 0,
    15, FALSE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 500.0, FALSE, FALSE},

   {VALVE_WEAPON_HORNETGUN, "weapon_hornetgun", 5, 180.0, 1700.0, 70.0, 180.1,
    10, TRUE, 50, 1, 4, TRUE, TRUE, FALSE, FALSE, 0.0, 0.0, TRUE, 300.0, FALSE, FALSE},

   {VALVE_WEAPON_MP5, "weapon_9mmAR", 4, 200.0, 2000.0, 300.0, 700.0,
    55, FALSE, 90, 1, 1, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 400.0, FALSE, FALSE},

   {VALVE_WEAPON_CROSSBOW, "weapon_crossbow", 2, 150.0, 500.0, 0, 0,
    5, TRUE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0, 0.0, TRUE, 1000.0, FALSE, FALSE},
   
   {VALVE_WEAPON_RPG, "weapon_rpg", 4, 300.0, 9999.0, 0.0, 0.0,
    60, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 500.0, FALSE, FALSE},
    
   {VALVE_WEAPON_GLOCK, "weapon_9mmhandgun", 5, 200.0, 3000.0, 80.0, 300.0,
    20, TRUE, 70, 1, 1, TRUE, TRUE, FALSE, FALSE, 0.0, 0.0, TRUE, 500.0, FALSE, FALSE},
   /* terminator */
   
   {0, "", 0, 0.0, 0.0, 0.0, 0.0, 0, TRUE, 0, 1, 1, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 0.0, FALSE, FALSE}
};

// weapon firing delay based on skill (min and max delay for each weapon)
// THESE MUST MATCH THE SAME ORDER AS THE WEAPON SELECT ARRAY!!!

bot_fire_delay_t valve_fire_delay[] = {
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

#endif
