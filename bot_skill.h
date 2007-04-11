//
// JK_Botti - be more human!
//
// bot_skill.h
//

typedef struct
{
   int pause_frequency; // how often (out of 1000 times) the bot will pause, based on bot skill
   float pause_time[2]; // how long bot pauses (min, max)
   
   float normal_strafe; // how much bot straifes when walking around
   float battle_strafe; // how much bot straifes when attacking enemy
   
   int keep_optimal_dist; // how often bot (out of 100 times) the bot try to keep at optimum distance of weapon when attacking
   
   float react_delay_min[3]; // reaction delay settings (first is for bot_reaction 1, second for 2, etc)
   float react_delay_max[3]; 
   
   float weaponchange_rate[2]; // how fast changing weapons (min, max)
   
   float shootcone_diameter; // bot tries to fire when aim line is less than [diameter / 2] apart from target 
   float shootcone_minangle; // OR angle between bot aim line and line to target is less than angle set here
      
   // Bot doesn't use real origin of target player but instead use data from earlier
   // and does latency prediction based on this data to get player origin. These settings
   // specify ammount of latency used at this skill level.
   float prediction_latency;
   float turn_skill; // BotAim turn_skill, how good bot is at aiming on enemy origin.
   float prediction_velocity_varitation;
   
   int hear_frequency; // how often (out of 100 times) the bot will hear what happens around it.
   
   qboolean can_longjump; // and can longjump.
   
   int random_jump_frequency; // how often (out of 100 times) the bot will do random jump
   int random_jump_duck_frequency; // how often (out of 100 times) the bot will do random duck when random jumping
   int random_duck_frequency; // how often (out of 100 times) the bot will do random duck jumping in combat mode
   int random_longjump_frequency; // how often (out of 100 times) the bot will do random longjump instead of random jump
   
   qboolean can_taujump; // can tau jump? (waypoint taujump, attack/flee taujump)
   
   int attack_taujump_frequency; // how often (out of 100 times) the bot will do tau jump at far away enemy
   int flee_taujump_frequency; // how often (out of 100 times) the bot will taujump away from enemy
   
   float attack_taujump_distance; // how far enemy have to be to bot to use tau jump
   float flee_taujump_distance; // max distance to flee enemy from
   float flee_taujump_health; // how much bot has health left when tries to escape
   float flee_taujump_escape_distance; // how long way bot tries to move away
   
   qboolean can_shoot_through_walls; // can shoot through walls by sound
   int wallshoot_frequency; // how often (out of 100 times) the bot will try attack enemy behind wall
   
} bot_skill_settings_t;

extern bot_skill_settings_t skill_settings[5];

void ResetSkillsToDefault(void);

