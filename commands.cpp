//
// JK_Botti - be more human!
//
// commands.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>
#include <pm_defs.h>
#include <time.h>

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"
#include "bot_skill.h"
#include "bot_weapon_select.h"

extern char g_argv[1024*3];
extern char g_arg1[1024];
extern char g_arg2[1024];
extern char g_arg3[1024];
extern qboolean g_waypoint_on;
extern qboolean g_auto_waypoint;
extern qboolean g_path_waypoint;
extern qboolean g_path_waypoint_enable;
extern qboolean g_waypoint_updated;
extern int num_waypoints;  // number of waypoints currently in use
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];
extern bot_t bots[32];
extern qboolean b_observer_mode;
extern qboolean b_botdontshoot;
extern qboolean g_in_intermission;
extern BOOL wp_matrix_save_on_mapend;

extern float bot_check_time;
extern float bot_cfg_pause_time;
extern int randomize_bots_on_mapchange;
extern float bot_think_spf;
extern int min_bots;
extern int max_bots;
extern FILE *bot_cfg_fp;
extern int submod_id;

extern int default_bot_skill;
extern int bot_add_level_tag;
extern int bot_chat_percent;
extern int bot_taunt_percent;
extern int bot_whine_percent;
extern int bot_logo_percent;
extern int bot_chat_tag_percent;
extern int bot_chat_drop_percent;
extern int bot_chat_swap_percent;
extern int bot_chat_lower_percent;
extern qboolean b_random_color;
extern int bot_reaction_time;

qboolean isFakeClientCommand = FALSE;
int fake_arg_count;


qboolean ProcessCommand(void (*printfunc)(void *arg, char *msg), void * arg, const char * pcmd, const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, qboolean is_cfgcmd) 
{
   char msg[128];
   qboolean is_clientcmd;
   qboolean is_servercmd;
   
   is_clientcmd = !is_cfgcmd && !!arg; //arg is pEntity on clientcmd
   is_servercmd = !is_clientcmd && !is_cfgcmd;
   
   if (FStrEq(pcmd, "info"))
   {
      UTIL_PrintBotInfo(printfunc, arg);
      
      return TRUE;
   }
   else if (FStrEq(pcmd, "addbot"))
   {
      BotCreate( (edict_t *)arg, arg1, arg2, arg3, arg4, arg5 );

      bot_check_time = gpGlobals->time + 5.0;
      if(is_cfgcmd)
         bot_cfg_pause_time = gpGlobals->time + 2.0;

      return TRUE;
   }
   else if (FStrEq(pcmd, "observer"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);
         if (temp)
            b_observer_mode = TRUE;
         else
            b_observer_mode = FALSE;
      }

      if (b_observer_mode)
         printfunc(arg, "observer mode ENABLED\n");
      else
         printfunc(arg, "observer mode DISABLED\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "botskill"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 1) || (temp > 5))
            printfunc(arg, "invalid botskill value!\n");
         else
            default_bot_skill = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "botskill is %d\n", default_bot_skill);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "randomize_bots_on_mapchange"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         randomize_bots_on_mapchange = !!atoi(arg1);
      }

      safevoid_snprintf(msg, sizeof(msg), "randomize_bots_on_mapchange is %s\n", (randomize_bots_on_mapchange?"on":"off"));
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_add_level_tag"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         bot_add_level_tag = !!atoi(arg1);
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_add_level_tag is %s\n", (bot_add_level_tag?"on":"off"));
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "botthinkfps"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 1) || (temp > 100))
            printfunc(arg, "invalid botthinkfps value!\n");
         else
            bot_think_spf = 1.0 / temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "botthinkfps is %.2f\n", 1.0 / bot_think_spf);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_percent value!\n");
         else
            bot_chat_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_chat_percent is %d\n", bot_chat_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_taunt_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_taunt_percent value!\n");
         else
            bot_taunt_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_taunt_percent is %d\n", bot_taunt_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_whine_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_whine_percent value!\n");
         else
            bot_whine_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_whine_percent is %d\n", bot_whine_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_tag_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_tag_percent value!\n");
         else
            bot_chat_tag_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_chat_tag_percent is %d\n", bot_chat_tag_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_drop_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_drop_percent value!\n");
         else
            bot_chat_drop_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_chat_drop_percent is %d\n", bot_chat_drop_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_swap_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_swap_percent value!\n");
         else
            bot_chat_swap_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_chat_swap_percent is %d\n", bot_chat_swap_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_lower_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_chat_lower_percent value!\n");
         else
            bot_chat_lower_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_chat_lower_percent is %d\n", bot_chat_lower_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_logo_percent"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 100))
            printfunc(arg, "invalid bot_logo_percent value!\n");
         else
            bot_logo_percent = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_logo_percent is %d\n", bot_logo_percent);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_reaction_time"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 3))
            printfunc(arg, "invalid bot_reaction_time value!\n");
         else
            bot_reaction_time = temp;
      }

      if (bot_reaction_time)
         safevoid_snprintf(msg, sizeof(msg), "bot_reaction_time is %d\n", bot_reaction_time);
      else
         safevoid_snprintf(msg, sizeof(msg), "bot_reaction_time is DISABLED\n");

      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "random_color"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if (temp)
            b_random_color = TRUE;
         else
            b_random_color = FALSE;
      }

      if (b_random_color)
         printfunc(arg, "random_color ENABLED\n");
      else
         printfunc(arg, "random_color DISABLED\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "botdontshoot"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);
         if (temp)
            b_botdontshoot = TRUE;
         else
            b_botdontshoot = FALSE;
      }

      if (b_botdontshoot)
         printfunc(arg, "botdontshoot ENABLED\n");
      else
         printfunc(arg, "botdontshoot DISABLED\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "min_bots"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         min_bots = atoi( arg1 );

         if ((min_bots < 0) || (min_bots > 31))
            min_bots = 1;
      }
      
      safevoid_snprintf(msg, sizeof(msg), "min_bots is set to %d\n", min_bots);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "max_bots"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         max_bots = atoi( arg1 );

         if ((max_bots < 0) || (max_bots > 31))
            max_bots = 1;
      }
      
      safevoid_snprintf(msg, sizeof(msg), "max_bots is set to %d\n", max_bots);
      printfunc(arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "pause") && is_cfgcmd)
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         bot_cfg_pause_time = gpGlobals->time + atoi( arg1 );
      }

      return TRUE;
   }
   else if (FStrEq(pcmd, "autowaypoint"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         if (FStrEq(arg1, "on"))
         {
            g_auto_waypoint = TRUE;
         }
         else if (FStrEq(arg1, "off"))
         {
            g_auto_waypoint = FALSE;
         }
         else
         {
            g_auto_waypoint = !!atoi(arg1);
         }

         if (g_auto_waypoint)
         {
            if(is_clientcmd)
               g_waypoint_on = TRUE; //just in case
            printfunc(arg, "autowaypoint is ON\n");
         }
         else
            printfunc(arg, "autowaypoint is OFF\n");
      }
      
      return TRUE;
   }
   else if (FStrEq(pcmd, "botweapon"))
   {  // this command allows editing of weapon information
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int select_index = -1;
         bot_weapon_select_t *pSelect = &weapon_select[0];
         
         while (pSelect[++select_index].iId)
            if (FStrEq(pSelect[select_index].weapon_name, arg1))
               break;
         
         if((arg2 == NULL) || (*arg2 == 0) || pSelect[select_index].iId)
         {
            qboolean got_match = FALSE;
            
#define CHECK_AND_SET_BOTWEAPON_INT(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         int temp = atoi(arg3); \
         pSelect[select_index].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for %s is %s'%i'.\n", #setting, arg1, "now ", temp); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for %s is %s'%i'.\n", #setting, arg1, "", pSelect[select_index].setting); \
      printfunc(arg, msg); \
   }
#define CHECK_AND_SET_BOTWEAPON_QBOOLEAN(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         qboolean temp; \
         if(!stricmp(arg3, "on") || !stricmp(arg3, "true")) \
            temp = TRUE; \
         else if(!stricmp(arg3, "off") || !stricmp(arg3, "false")) \
            temp = FALSE; \
         else \
            temp = atoi(arg3) ? TRUE : FALSE; \
         pSelect[select_index].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for %s is %s'%s'.\n", #setting, arg1, "now ", temp ? "on" : "off"); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for %s is %s'%s'.\n", #setting, arg1, "", pSelect[select_index].setting ? "on" : "off"); \
      printfunc(arg, msg); \
   }
#define CHECK_AND_SET_BOTWEAPON_FLOAT(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         float temp = atof(arg3); \
         pSelect[select_index].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for %s is %s'%.4f'.\n", #setting, arg1, "now ", temp); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for %s is %s'%.4f'.\n", #setting, arg1, "", pSelect[select_index].setting); \
      printfunc(arg, msg); \
   }
            CHECK_AND_SET_BOTWEAPON_INT(primary_skill_level)   // bot skill must be less than or equal to this value
            CHECK_AND_SET_BOTWEAPON_INT(secondary_skill_level) // bot skill must be less than or equal to this value
            CHECK_AND_SET_BOTWEAPON_FLOAT(aim_speed)           // aim speed, 0.0 worst, 1.0 best.
            CHECK_AND_SET_BOTWEAPON_QBOOLEAN(avoid_this_gun)  // bot avoids using this weapon if possible
            CHECK_AND_SET_BOTWEAPON_QBOOLEAN(prefer_higher_skill_attack) // bot uses better of primary and secondary attacks if both avaible
            CHECK_AND_SET_BOTWEAPON_FLOAT(primary_min_distance)   // 0 = no minimum
            CHECK_AND_SET_BOTWEAPON_FLOAT(primary_max_distance)   // 9999 = no maximum
            CHECK_AND_SET_BOTWEAPON_FLOAT(secondary_min_distance) // 0 = no minimum    
            CHECK_AND_SET_BOTWEAPON_FLOAT(secondary_max_distance) // 9999 = no maximum
            CHECK_AND_SET_BOTWEAPON_FLOAT(opt_distance) // optimal distance from target
            CHECK_AND_SET_BOTWEAPON_INT(use_percent)   // times out of 100 to use this weapon when available
            CHECK_AND_SET_BOTWEAPON_QBOOLEAN(can_use_underwater)     // can use this weapon underwater
            CHECK_AND_SET_BOTWEAPON_INT(primary_fire_percent)   // times out of 100 to use primary fire
            CHECK_AND_SET_BOTWEAPON_INT(low_ammo_primary)        // low ammo level
            CHECK_AND_SET_BOTWEAPON_INT(low_ammo_secondary)      // low ammo level
            if(!got_match) 
            {
               snprintf(msg, sizeof(msg), "unknown weapon setting %s.\n", arg2);
               printfunc(arg, msg);
               printfunc(arg, "Usage: botweapon <weapon-name> <setting> <value> - set value\n");
               printfunc(arg, "       botweapon <weapon-name> <setting> - shows setting value\n");
               printfunc(arg, "       botweapon <weapon-name> - shows all setting values\n");
               printfunc(arg, "       botweapon weapons - shows weapon-names\n");
            }
         }
         else if(FStrEq("weapons", arg1)) {
            printfunc(arg, "List of available weapons:\n");
            
            select_index = -1;
            pSelect = &weapon_select[0];

            while (pSelect[++select_index].iId) {
               safevoid_snprintf(msg, sizeof(msg), "  %s\n", pSelect[select_index].weapon_name);
               printfunc(arg, msg);
            }
         }
         else
         { 
            snprintf(msg, sizeof(msg), "Could not complete request! (unknown arg1: '%s')\n", arg1);
            printfunc(arg, msg);
            printfunc(arg, "Usage: botweapon <weapon-name> <setting> <value> - set value\n");
            printfunc(arg, "       botweapon <weapon-name> <setting> - shows setting value\n");
            printfunc(arg, "       botweapon <weapon-name> - shows all setting values\n");
            printfunc(arg, "       botweapon weapons - shows weapon-names\n");
            printfunc(arg, "       botweapon reset - reset values back to defaults for all skills\n");
         }
      }
      else if((arg1 != NULL) && (*arg1 != 0) && FStrEq(arg1, "reset"))
      {
         //weapon select init
         InitWeaponSelect(submod_id);
         printfunc(arg, "Bot weapon settings reset default.\n");
      }
      else
      {
      	 printfunc(arg, "Could not complete request! (arg1 not given)\n");
      	 printfunc(arg, "Usage: botweapon <weapon-name> <setting> <value> - set value\n");
         printfunc(arg, "       botweapon <weapon-name> <setting> - shows setting value\n");
         printfunc(arg, "       botweapon <weapon-name> - shows all setting values\n");
         printfunc(arg, "       botweapon weapons - shows weapon-names\n");
         printfunc(arg, "       botweapon reset - reset values back to defaults for all skills\n");
      }
      
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_skill_setup"))
   {  // this command allows editing of botskill settings, 
      // bot_skill_setup <skill>, bot_skill_setup <skill> <setting>, bot_skill_setup <skill> <setting> <value>
      if ((arg1 != NULL) && (*arg1 != 0) && atoi(arg1) >= 1 && atoi(arg1) <= 5)
      {
         int skill_idx = atoi(arg1)-1;
         qboolean got_match = FALSE;
         
#define CHECK_AND_SET_BOTSKILL_INT(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         int temp = atoi(arg3); \
         skill_settings[skill_idx].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%i'.\n", #setting, skill_idx+1, "now ", temp); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%i'.\n", #setting, skill_idx+1, "", skill_settings[skill_idx].setting); \
      printfunc(arg, msg); \
   }
#define CHECK_AND_SET_BOTSKILL_QBOOLEAN(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         qboolean temp; \
         if(!stricmp(arg3, "on") || !stricmp(arg3, "true")) \
            temp = TRUE; \
         else if(!stricmp(arg3, "off") || !stricmp(arg3, "false")) \
            temp = FALSE; \
         else \
            temp = atoi(arg3) ? TRUE : FALSE; \
         skill_settings[skill_idx].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%s'.\n", #setting, skill_idx+1, "now ", temp ? "on" : "off"); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%s'.\n", #setting, skill_idx+1, "", skill_settings[skill_idx].setting ? "on" : "off"); \
      printfunc(arg, msg); \
   }
#define CHECK_AND_SET_BOTSKILL_FLOAT(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         float temp = atof(arg3); \
         skill_settings[skill_idx].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%.4f'.\n", #setting, skill_idx+1, "now ", temp); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%.4f'.\n", #setting, skill_idx+1, "", skill_settings[skill_idx].setting); \
      printfunc(arg, msg); \
   }
#define CHECK_AND_SET_BOTSKILL_FLOAT100(setting) \
   if ((arg2 == NULL) || (*arg2 == 0) || FStrEq(arg2, #setting)) { \
      got_match = TRUE; \
      if ((arg3 != NULL) && (*arg3 != 0)) { \
         float temp = atof(arg3)/100; \
         skill_settings[skill_idx].setting = temp; \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%.0f'.\n", #setting, skill_idx+1, "now ", temp*100); \
      } else \
         safevoid_snprintf(msg, sizeof(msg), "'%s' for skill[%d] is %s'%.0f'.\n", #setting, skill_idx+1, "", skill_settings[skill_idx].setting*100); \
      printfunc(arg, msg); \
   }
         CHECK_AND_SET_BOTSKILL_INT(pause_frequency) // how often (out of 1000 times) the bot will pause, based on bot skill
         CHECK_AND_SET_BOTSKILL_FLOAT100(normal_strafe) // how much bot straifes when walking around
         CHECK_AND_SET_BOTSKILL_FLOAT100(battle_strafe) // how much bot straifes when attacking enemy
         CHECK_AND_SET_BOTSKILL_INT(keep_optimal_distance) // how often bot (out of 1000 times) the bot try to keep at optimum distance of weapon when attacking
         CHECK_AND_SET_BOTSKILL_FLOAT(shootcone_diameter) // bot tries to fire when aim line is less than [diameter / 2] apart from target 
         CHECK_AND_SET_BOTSKILL_FLOAT(shootcone_minangle) // OR angle between bot aim line and line to target is less than angle set here
         CHECK_AND_SET_BOTSKILL_FLOAT(turn_skill) // BotAim turn_skill, how good bot is at aiming on enemy origin.
         CHECK_AND_SET_BOTSKILL_INT(hear_frequency) // how often (out of 100 times) the bot will hear what happens around it.

         CHECK_AND_SET_BOTSKILL_QBOOLEAN(can_longjump) // and can longjump.
         CHECK_AND_SET_BOTSKILL_INT(random_jump_frequency) // how often (out of 100 times) the bot will do random jump
         CHECK_AND_SET_BOTSKILL_INT(random_jump_duck_frequency) // how often (out of 100 times) the bot will do random duck when random jumping
         CHECK_AND_SET_BOTSKILL_INT(random_duck_frequency) // how often (out of 100 times) the bot will do random duck jumping in combat mode
         CHECK_AND_SET_BOTSKILL_INT(random_longjump_frequency) // how often (out of 100 times) the bot will do random longjump instead of random jump

         CHECK_AND_SET_BOTSKILL_QBOOLEAN(can_taujump) // can tau jump?
         CHECK_AND_SET_BOTSKILL_INT(attack_taujump_frequency) // how often (out of 100 times) the bot will do tau jump at far away enemy
         CHECK_AND_SET_BOTSKILL_INT(flee_taujump_frequency) // how often (out of 100 times) the bot will taujump away from enemy
         CHECK_AND_SET_BOTSKILL_FLOAT(attack_taujump_distance) // how far enemy have to be to bot to use tau jump
         CHECK_AND_SET_BOTSKILL_FLOAT(flee_taujump_distance) // max distance to flee enemy from
         CHECK_AND_SET_BOTSKILL_FLOAT(flee_taujump_health) // how much bot has health left when tries to escape
         CHECK_AND_SET_BOTSKILL_FLOAT(flee_taujump_escape_distance) // how long way bot tries to move away
         
         CHECK_AND_SET_BOTSKILL_QBOOLEAN(can_shoot_through_walls) // can shoot through walls by sound
         CHECK_AND_SET_BOTSKILL_INT(wallshoot_frequency) // how often (out of 100 times) the bot will try attack enemy behind wall

         if(!got_match) 
         {
            snprintf(msg, sizeof(msg), "unknown skill setting %s.\n", arg2);
            printfunc(arg, msg);
      	    printfunc(arg, "Usage: bot_skill_setup <skill> <setting> <value> - set value\n");
            printfunc(arg, "       bot_skill_setup <skill> <setting> - shows setting value\n");
            printfunc(arg, "       bot_skill_setup <skill> - shows all setting values\n");
            printfunc(arg, "       bot_skill_setup reset - reset values back to defaults for all skills\n");
         }
      }
      else if((arg1 != NULL) && (*arg1 != 0) && FStrEq(arg1, "reset"))
      {
         ResetSkillsToDefault();
         printfunc(arg, "Bot skill settings reset default.\n");
      }
      else
      {
         if((arg1 == NULL) || (*arg1 == 0))
      	    printfunc(arg, "Could not complete request! (arg1 not given)\n");
      	 else
      	 {
      	    snprintf(msg, sizeof(msg), "Could not complete request! (invalid skill %d, use 1-5)\n", atoi(arg1));
      	    printfunc(arg, msg);
      	 }
      	 printfunc(arg, "Usage: bot_skill_setup <skill> <setting> <value> - set value\n");
         printfunc(arg, "       bot_skill_setup <skill> <setting> - shows setting value\n");
         printfunc(arg, "       bot_skill_setup <skill> - shows all setting values\n");
         printfunc(arg, "       bot_skill_setup reset - reset values back to defaults for all skills\n");
      }
      
      return TRUE;
   }

   return FALSE;
}

#if _DEBUG  
static void print_to_client(void *arg, char *msg) 
{
   ClientPrint((edict_t *)arg, HUD_PRINTNOTIFY, msg);
}
#endif

void ClientCommand( edict_t *pEntity )
{
   // only allow custom commands if deathmatch mode and NOT dedicated server and
   // client sending command is the listen server client...

#if _DEBUG   
   if ((gpGlobals->deathmatch))
   {
      const char *pcmd = CMD_ARGV (0);
      const char *arg1 = CMD_ARGV (1);
      const char *arg2 = CMD_ARGV (2);
      const char *arg3 = CMD_ARGV (3);
      const char *arg4 = CMD_ARGV (4);
      const char *arg5 = CMD_ARGV (5);
   
      if(ProcessCommand(print_to_client, pEntity, pcmd, arg1, arg2, arg3, arg4, arg5, FALSE))
      {
         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "waypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_waypoint_on = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_waypoint_on = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }
         else if (FStrEq(arg1, "add"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointAdd(pEntity);
         }
         else if (FStrEq(arg1, "delete"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointDelete(pEntity);
         }
         else if (FStrEq(arg1, "save"))
         {
            WaypointSave();

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints saved\n");
         }
         else if (FStrEq(arg1, "load"))
         {
            if (WaypointLoad(pEntity))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");
         }
         else if (FStrEq(arg1, "info"))
         {
            WaypointPrintInfo(pEntity);
         }
         else if (FStrEq(arg1, "update"))
         {
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "updating waypoint tags...\n");

            WaypointUpdate(pEntity);

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "...update done!  (don't forget to save!)\n");
         }
         else
         {
            if (g_waypoint_on)
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
            else
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "pathwaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_path_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_path_waypoint = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
         }
         else if (FStrEq(arg1, "enable"))
         {
            g_path_waypoint_enable = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ENABLED\n");
         }
         else if (FStrEq(arg1, "disable"))
         {
            g_path_waypoint_enable = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is DISABLED\n");
         }
         else if (FStrEq(arg1, "create1"))
         {
            WaypointCreatePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "create2"))
         {
            WaypointCreatePath(pEntity, 2);
         }
         else if (FStrEq(arg1, "remove1"))
         {
            WaypointRemovePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "remove2"))
         {
            WaypointRemovePath(pEntity, 2);
         }

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "search"))
      {
         edict_t *pent = NULL;
         float radius = 100;
         char str[80];

         ClientPrint(pEntity, HUD_PRINTNOTIFY, "searching...\n");

         while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
         {
            safevoid_snprintf(str, sizeof(str), "Found %s at %5.2f %5.2f %5.2f\n",
                       STRING(pent->v.classname),
                       pent->v.origin.x, pent->v.origin.y,
                       pent->v.origin.z);
            ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

            FILE *fp=fopen("jk_botti.txt", "a");
            fprintf(fp, "ClientCommmand: search %s", str);
            fclose(fp);
         }

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "botstop"))
      {
         bot_stop = 1;

         RETURN_META (MRES_SUPERCEDE);
      }
      else if (FStrEq(pcmd, "botstart"))
      {
         bot_stop = 0;

         RETURN_META (MRES_SUPERCEDE);
      }
   }
#endif

   RETURN_META (MRES_IGNORED);
}


void FakeClientCommand(edict_t *pBot, const char *arg1, const char *arg2, const char *arg3)
{
   g_argv[0] = 0;
   g_arg1[0] = 0;
   g_arg2[0] = 0;
   g_arg3[0] = 0;

   if (!arg1 || !*arg1)
   {
      return;
   }

   if (!arg2 || !*arg2)
   {
      safevoid_snprintf(g_argv, sizeof(g_argv), "%s", arg1);
      null_terminate_buffer(g_argv, sizeof(g_argv));
      
      fake_arg_count = 1;
   }
   else if (!arg3 || !*arg3)
   {
      safevoid_snprintf(g_argv, sizeof(g_argv), "%s %s", arg1, arg2);
      null_terminate_buffer(g_argv, sizeof(g_argv));
      
      fake_arg_count = 2;
   }
   else
   {
      safevoid_snprintf(g_argv, sizeof(g_argv), "%s %s %s", arg1, arg2, arg3);
      null_terminate_buffer(g_argv, sizeof(g_argv));
      
      fake_arg_count = 3;
   }
   
   safevoid_snprintf(g_arg1, sizeof(g_arg1), "%s", arg1);
   null_terminate_buffer(g_arg1, sizeof(g_arg1));
   
   if (arg2 && *arg2)
   {
      safevoid_snprintf(g_arg2, sizeof(g_arg2), "%s", arg2);
      null_terminate_buffer(g_arg2, sizeof(g_arg2));
   }
   
   if (arg3 && *arg3)
   {
      safevoid_snprintf(g_arg3, sizeof(g_arg3), "%s", arg3);
      null_terminate_buffer(g_arg3, sizeof(g_arg3));
   }
   
   // allow the MOD DLL to execute the ClientCommand...
   isFakeClientCommand = TRUE;
   MDLL_ClientCommand(pBot);
   isFakeClientCommand = FALSE;
}


static void print_to_null(void *, char *) 
{
}


void ProcessBotCfgFile(void)
{
   int ch;
   char cmd_line[256];
   int cmd_index;
   static char server_cmd[80];
   char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5;

   if (bot_cfg_pause_time > gpGlobals->time)
      return;

   if (bot_cfg_fp == NULL)
      return;

   cmd_index = 0;
   cmd_line[cmd_index] = 0;

   ch = fgetc(bot_cfg_fp);

   // skip any leading blanks
   while (ch == ' ')
      ch = fgetc(bot_cfg_fp);

   while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
   {
      if (ch == '\t')  // convert tabs to spaces
         ch = ' ';

      cmd_line[cmd_index] = ch;

      ch = fgetc(bot_cfg_fp);

      // skip multiple spaces in input file
      while ((cmd_line[cmd_index] == ' ') &&
             (ch == ' '))      
         ch = fgetc(bot_cfg_fp);

      cmd_index++;
   }

   if (ch == '\r')  // is it a carriage return?
   {
      ch = fgetc(bot_cfg_fp);  // skip the linefeed
   }

   // if reached end of file, then close it
   if (ch == EOF)
   {
      fclose(bot_cfg_fp);

      bot_cfg_fp = NULL;

      bot_cfg_pause_time = 0.0;
   }

   cmd_line[cmd_index] = 0;  // terminate the command line

   // copy the command line to a server command buffer...
   strcpy(server_cmd, cmd_line);
   strcat(server_cmd, "\n");

   cmd_index = 0;
   cmd = cmd_line;
   arg1 = arg2 = arg3 = arg4 = arg5 = NULL;

   // skip to blank or end of string...
   while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
      cmd_index++;

   if (cmd_line[cmd_index] == ' ')
   {
      cmd_line[cmd_index++] = 0;
      arg1 = &cmd_line[cmd_index];

      // skip to blank or end of string...
      while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
         cmd_index++;

      if (cmd_line[cmd_index] == ' ')
      {
         cmd_line[cmd_index++] = 0;
         arg2 = &cmd_line[cmd_index];

         // skip to blank or end of string...
         while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
            cmd_index++;

         if (cmd_line[cmd_index] == ' ')
         {
            cmd_line[cmd_index++] = 0;
            arg3 = &cmd_line[cmd_index];

            // skip to blank or end of string...
            while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
               cmd_index++;

            if (cmd_line[cmd_index] == ' ')
            {
               cmd_line[cmd_index++] = 0;
               arg4 = &cmd_line[cmd_index];

               // skip to blank or end of string...
               while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
                  cmd_index++;

               if (cmd_line[cmd_index] == ' ')
               {
                  cmd_line[cmd_index++] = 0;
                  arg5 = &cmd_line[cmd_index];
               }
            }
         }
      }
   }

   if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
      return;  // return if comment or blank line

   if(arg1 && !strcmp(arg1, "\"\""))
      *arg1=0;
   if(arg2 && !strcmp(arg2, "\"\""))
      *arg2=0;
   if(arg3 && !strcmp(arg3, "\"\""))
      *arg3=0;
   if(arg4 && !strcmp(arg4, "\"\""))
      *arg4=0;
   if(arg5 && !strcmp(arg5, "\"\""))
      *arg5=0;

   if(ProcessCommand(print_to_null, NULL, cmd, arg1, arg2, arg3, arg4, arg5, TRUE))
      return;
   
   UTIL_ConsolePrintf("executing server command: %s\n", server_cmd);

   SERVER_COMMAND(server_cmd);
}


static void print_to_server_output(void *, char * msg) 
{
   UTIL_ConsolePrintf(msg);
}


void jk_botti_ServerCommand (void)
{
   if(!ProcessCommand(print_to_server_output, NULL, CMD_ARGV (1), CMD_ARGV (2), CMD_ARGV (3), CMD_ARGV (4), CMD_ARGV (5), CMD_ARGV (6), FALSE)) {
      UTIL_ConsolePrintf("%s: Unknown command \'%s\'\n", CMD_ARGV(1), CMD_ARGV(2));
   }
}
