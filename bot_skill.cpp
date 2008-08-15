//
// JK_Botti - be more human!
//
// bot_skill.cpp
//

#define BOTSKILL

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"
#include "bot_weapons.h"

#include "bot_skill.h"

#if 0
COPY+PASTE from bot_skill.h, for explaning values in following table:
typedef struct
{

0:
   int pause_frequency; // how often (out of 1000 times) the bot will pause, based on bot skill
   float pause_time_min; // how long bot pauses (min, max)
   float pause_time_max; 
   
   float normal_strafe; // how much bot strafes when walking around
   float battle_strafe; // how much bot strafes when attacking enemy
  
   int keep_optimal_dist; // how often bot (out of 100 times) the bot try to keep at optimum distance of weapon when attacking

6: 
   float respawn_react_delay; // delay on players after respawn
   float react_delay_min[3]; // reaction delay settings (first is for bot_reaction 1, second for 2, etc)
   float react_delay_max[3]; 

9:
   float weaponchange_rate_min; // how fast changing weapons (min, max)
   float weaponchange_rate_max; 

11: 
   float shootcone_diameter; // bot tries to fire when aim line is less than [diameter / 2] apart from target 
   float shootcone_minangle; // OR angle between bot aim line and line to target is less than angle set here

13:   
   float turn_skill; // BotAim turn_skill, how good bot is at aiming on enemy origin.
   float updown_turn_ration; // how much slower bots aims up and down than side ways?
   //float aimangle_varitation; // how steady bot's hand is (how much randomness we add to bot aim angle?)

16:
   // Bot doesn't use real origin of target player but instead use ping emulation based on recorded old position data of player. 
   // These settings specify ammount of latency and randomness used at different skill levels.
   float ping_emu_latency; // ping emulation in seconds
   float ping_emu_speed_varitation; // percent
   float ping_emu_position_varitation; // units from target center

19:
   qboolean can_longjump; // and can longjump.
   
   int random_jump_frequency; // how often (out of 100 times) the bot will do random jump
   int random_jump_duck_frequency; // how often (out of 100 times) the bot will do random duck when random jumping
   int random_duck_frequency; // how often (out of 100 times) the bot will do random duck jumping in combat mode
   int random_longjump_frequency; // how often (out of 100 times) the bot will do random longjump instead of random jump

#if 0
24:
   qboolean can_taujump; // can tau jump? (waypoint taujump, attack/flee taujump)
   
   int attack_taujump_frequency; // how often (out of 100 times) the bot will do tau jump at far away enemy
   int flee_taujump_frequency; // how often (out of 100 times) the bot will taujump away from enemy
   
   float attack_taujump_distance; // how far enemy have to be to bot to use tau jump
   float flee_taujump_distance; // max distance to flee enemy from
   float flee_taujump_health; // how much bot has health left when tries to escape
   float flee_taujump_escape_distance; // how long way bot tries to move away

   qboolean can_shoot_through_walls; // can shoot through walls by sound
   int wallshoot_frequency; // how often (out of 100 times) the bot will try attack enemy behind wall
#endif

33:
   float hearing_sensitivity; // how well bot hears sounds
   float track_sound_time_min; // how long bot tracks one sound
   float track_sound_time_max;

} bot_skill_settings_t;
#endif

bot_skill_settings_t default_skill_settings[5] = {
   // best skill (lvl1)
   {
//0:
     1, 0.10, 0.25, 10.0, 50.0, 80, 
//6:
     0.4, {0.01, 0.07, 0.10}, {0.04, 0.11, 0.15}, 
//9:
     0.1, 0.3,
//11:
     150.0, 12.5, 
//13:
     4.0, 2.0, //0.1, 
//16:
     0.060, 0.03, 2.0, 
//19:
     TRUE, 50, 75, 50, 100,
//24:
     //TRUE, 100, 100, 1000.0, 400.0, 20.0, 1000.0,
     //TRUE, 99,
//33:
     1.5, 20.0, 40.0 },

   // lvl2
   {
//0:
     5, 0.25, 0.5, 8.0, 30.0, 70,
//6:
     0.5, {0.02, 0.09, 0.12}, {0.06, 0.14, 0.18},
//9:
     0.2, 0.5,
//11:
     175.0, 20.0, 
//13:
     3.0, 2.25, //0.2, 
//16:
     0.120, 0.04, 3.0, 
//19:
     TRUE, 35, 60, 35, 90,
//24:
     //TRUE, 50, 50, 1000.0, 400.0, 10.0, 1000.0,
     //TRUE, 66,
//33:
     1.25, 15.0, 30.0 },

   // lvl3
   {
//0:
     10, 0.35, 0.75, 6.0, 15.0, 60,
//6:
     0.6, {0.03, 0.12, 0.15}, {0.08, 0.18, 0.22},
//9:
     0.3, 0.7,
//11:
     200.0, 25.0, 
//13:
     2.0, 2.50, //0.3, 
//16:
     0.180, 0.05, 4.0, 
//19:
     TRUE, 20, 40, 20, 70,
//24:
     //TRUE, 20, 20, 1000.0, 400.0, 10.0, 1000.0,
     //TRUE, 33,
//33:
     1.0, 10.0, 20.0 },

   // lvl4
   {
//0:
     20, 0.45, 1.5, 4.0, 5.0, 50,
//6:
     0.7, {0.04, 0.14, 0.18}, {0.10, 0.21, 0.25},
//9:
     0.6, 1.4,
//11:
     250.0, 30.0, 
//13:
     1.25, 2.75, //0.4, 
//16:
     0.240, 0.075, 6.0, 
//19:
     TRUE, 10, 25, 10, 40,
//24:
     //TRUE, 0, 0, 0.0, 0.0, 0.0, 0.0,
     //FALSE, 0,
//33:
     0.75, 7.5, 15.0 },

   // worst skill (lvl5)
   {
//0:
     50, 0.55, 2.5, 0.5, 1.0, 40,
//6:
     0.8, {0.05, 0.17, 0.21}, {0.12, 0.25, 0.30},
//9:
     1.2, 2.8,
//11:
     300.0, 35.0, 
//13:
     0.75, 3.0, //0.5, 
//16:
     0.300, 0.10, 8.0, 
//19:
     FALSE, 5, 15, 5, 0,
//24:
     //FALSE, 0, 0, 0.0, 0.0, 0.0, 0.0,
     //FALSE, 0,
//33:
     0.5, 5.0, 10.0 },
};

bot_skill_settings_t skill_settings[5];

void ResetSkillsToDefault(void)
{
   memcpy(skill_settings, default_skill_settings, sizeof(skill_settings));
}
