//
// JK_Botti - be more human!
//
// commands.cpp
//

#include <string.h>

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
#include "bot_weapons.h"

#include "bot_query_hook.h"

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

extern int team_balancetype;
extern char *team_blockedlist;

extern float bot_check_time;
extern float bot_cfg_pause_time;
extern int randomize_bots_on_mapchange;
extern float bot_think_spf;
extern int min_bots;
extern int max_bots;
extern FILE *bot_cfg_fp;
extern int bot_cfg_linenumber;
extern int submod_id;
extern int debug_minmax;

extern int default_bot_skill;
extern int bot_add_level_tag;
extern int bot_chat_percent;
extern int bot_taunt_percent;
extern int bot_whine_percent;
extern int bot_endgame_percent;
extern int bot_logo_percent;
extern int bot_chat_tag_percent;
extern int bot_chat_drop_percent;
extern int bot_chat_swap_percent;
extern int bot_chat_lower_percent;
extern qboolean b_random_color;
extern int bot_shoot_breakables;

qboolean isFakeClientCommand = FALSE;
int fake_arg_count;

int bot_conntimes = 0;
int cfg_bot_record_size = 0;
cfg_bot_record_t * cfg_bot_record;

#define CFGCMD_TYPE 1
#define SRVCMD_TYPE 2
#define CLICMD_TYPE 3

#define PRINTFUNC_INFO 1
#define PRINTFUNC_ERROR 2

typedef void (*printfunc_t)(int printtype, void *arg, char *msg);


static qboolean check_and_set_int(const char *name, int *value,
   const char *arg2, const char *arg3,
   const char *label, char *msg, size_t msgsize,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg2 != NULL) && (*arg2 != 0) && !FStrEq(arg2, name))
      return FALSE;
   if ((arg3 != NULL) && (*arg3 != 0)) {
      int temp = atoi(arg3);
      *value = temp;
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%i'.\n", name, label, "now ", temp);
   } else {
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%i'.\n", name, label, "", *value);
   }
   printfunc(PRINTFUNC_INFO, pfarg, msg);
   return TRUE;
}

static qboolean check_and_set_qboolean(const char *name, qboolean *value,
   const char *arg2, const char *arg3,
   const char *label, char *msg, size_t msgsize,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg2 != NULL) && (*arg2 != 0) && !FStrEq(arg2, name))
      return FALSE;
   if ((arg3 != NULL) && (*arg3 != 0)) {
      qboolean temp;
      if(!stricmp(arg3, "on") || !stricmp(arg3, "true"))
         temp = TRUE;
      else if(!stricmp(arg3, "off") || !stricmp(arg3, "false"))
         temp = FALSE;
      else
         temp = atoi(arg3) ? TRUE : FALSE;
      *value = temp;
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%s'.\n", name, label, "now ", temp ? "on" : "off");
   } else {
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%s'.\n", name, label, "", *value ? "on" : "off");
   }
   printfunc(PRINTFUNC_INFO, pfarg, msg);
   return TRUE;
}

static qboolean check_and_set_float(const char *name, float *value,
   const char *arg2, const char *arg3,
   const char *label, char *msg, size_t msgsize,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg2 != NULL) && (*arg2 != 0) && !FStrEq(arg2, name))
      return FALSE;
   if ((arg3 != NULL) && (*arg3 != 0)) {
      float temp = atof(arg3);
      *value = temp;
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%.4f'.\n", name, label, "now ", temp);
   } else {
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%.4f'.\n", name, label, "", *value);
   }
   printfunc(PRINTFUNC_INFO, pfarg, msg);
   return TRUE;
}

static qboolean check_and_set_float100(const char *name, float *value,
   const char *arg2, const char *arg3,
   const char *label, char *msg, size_t msgsize,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg2 != NULL) && (*arg2 != 0) && !FStrEq(arg2, name))
      return FALSE;
   if ((arg3 != NULL) && (*arg3 != 0)) {
      float temp = atof(arg3)/100;
      *value = temp;
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%.0f'.\n", name, label, "now ", temp*100);
   } else {
      safevoid_snprintf(msg, msgsize, "'%s' for %s is %s'%.0f'.\n", name, label, "", (*value)*100);
   }
   printfunc(PRINTFUNC_INFO, pfarg, msg);
   return TRUE;
}

static void set_bool_toggle(int *value, const char *arg1,
   char *enabled_msg, char *disabled_msg,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg1 != NULL) && (*arg1 != 0))
      *value = atoi(arg1) ? TRUE : FALSE;
   printfunc(PRINTFUNC_INFO, pfarg, *value ? enabled_msg : disabled_msg);
}

static void set_percent_value(const char *name, int *value, const char *arg1,
   char *msg, size_t msgsize,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg1 != NULL) && (*arg1 != 0))
   {
      int temp = atoi(arg1);

      if ((temp < 0) || (temp > 100))
      {
         safevoid_snprintf(msg, msgsize, "invalid %s value!\n", name);
         printfunc(PRINTFUNC_ERROR, pfarg, msg);
      }
      else
         *value = temp;
   }

   safevoid_snprintf(msg, msgsize, "%s is %d\n", name, *value);
   printfunc(PRINTFUNC_INFO, pfarg, msg);
}

static void set_onoff_value(const char *name, int *value, const char *arg1,
   char *msg, size_t msgsize,
   const printfunc_t printfunc, void *pfarg)
{
   if ((arg1 != NULL) && (*arg1 != 0))
      *value = !!atoi(arg1);
   safevoid_snprintf(msg, msgsize, "%s is %s\n", name, (*value ? "on" : "off"));
   printfunc(PRINTFUNC_INFO, pfarg, msg);
}


//
const cfg_bot_record_t * GetUnusedCfgBotRecord(void)
{
   if(cfg_bot_record_size <= 0)
      return (const cfg_bot_record_t *)NULL;

   //
   int *record_indexes = (int *)malloc(cfg_bot_record_size * sizeof(int));
   if(!record_indexes)
      return (const cfg_bot_record_t *)NULL;
   int num_records = 0;

   record_indexes[0] = 0;

   // collect unused records
   for(int cfgindex = 0; cfgindex < cfg_bot_record_size; cfgindex++)
   {
      int used = 0;

      for(int index = 0; index < 32; index++)
      {
         if(bots[index].is_used)
         {
            if(bots[index].cfg_bot_index == cfgindex)
            {
               used = 1;
               break;
            }
         }
      }

      if(used)
         continue;

      record_indexes[num_records++] = cfgindex;
   }

   if(num_records <= 0)
   {
      free(record_indexes);
      return (const cfg_bot_record_t *)NULL;
   }

   // random pick one of unused records
   int pick = RANDOM_LONG2(0, num_records-1);

   //
   const cfg_bot_record_t *result = &cfg_bot_record[record_indexes[pick]];
   free(record_indexes);
   return(result);
}

//
void FreeCfgBotRecord(void)
{
   if(cfg_bot_record_size > 0 && cfg_bot_record_size)
   {
      for(int i = 0; i < cfg_bot_record_size; i++)
      {
         if(cfg_bot_record[i].name)
            free(cfg_bot_record[i].name);
         if(cfg_bot_record[i].skin)
            free(cfg_bot_record[i].skin);
      }

      free(cfg_bot_record);
   }

   cfg_bot_record = NULL;
   cfg_bot_record_size = 0;
}

//
int AddToCfgBotRecord(const char *skin, const char *name, int skill, int top_color, int bottom_color)
{
   int index = cfg_bot_record_size;

   cfg_bot_record_t *tmp = (cfg_bot_record_t*)realloc(cfg_bot_record, sizeof(cfg_bot_record_t) * (cfg_bot_record_size + 1));
   if (!tmp)
      return -1;
   cfg_bot_record = tmp;
   cfg_bot_record_size++;

   cfg_bot_record[index].index = index;
   cfg_bot_record[index].skin = (skin) ? strdup(skin) : NULL;
   cfg_bot_record[index].name = (name) ? strdup(name) : NULL;
   cfg_bot_record[index].skill = skill;
   cfg_bot_record[index].top_color = top_color;
   cfg_bot_record[index].bottom_color = bottom_color;

   return(index);
}

//
static void UTIL_PrintBotInfo(const printfunc_t printfunc, void * arg)
{
   //print out bot info
   char msg[80];
   int bot_index, count = 0;

   for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
   {
      if (bots[bot_index].is_used) {
         count++;
         safevoid_snprintf(msg, sizeof(msg), "Bot #%d\n", count);
         printfunc(PRINTFUNC_INFO, arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " name: %s\n", bots[bot_index].name);
         printfunc(PRINTFUNC_INFO, arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " skin: %s\n", bots[bot_index].skin);
         printfunc(PRINTFUNC_INFO, arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " skill: %d\n", bots[bot_index].bot_skill + 1);
         printfunc(PRINTFUNC_INFO, arg, msg);
         safevoid_snprintf(msg, sizeof(msg), " got enemy: %s\n", (bots[bot_index].pBotEnemy != 0) ? "true" : "false");
         printfunc(PRINTFUNC_INFO, arg, msg);
         safevoid_snprintf(msg, sizeof(msg), "---\n");
         printfunc(PRINTFUNC_INFO, arg, msg);
      }
   }

   safevoid_snprintf(msg, sizeof(msg), "Total Bots: %d\n", count);
   printfunc(PRINTFUNC_INFO, arg, msg);
}

static void ProcessBotWeaponCommand(const int cmdtype, const printfunc_t printfunc, void * arg, const char * arg1, const char * arg2, const char * arg3)
{
   char msg[128];

   if ((arg1 != NULL) && (*arg1 != 0))
   {
      int select_index = -1;
      bot_weapon_select_t *pSelect = &weapon_select[0];

      while (pSelect[++select_index].iId)
         if (FStrEq(pSelect[select_index].weapon_name, arg1))
            break;

      if(pSelect[select_index].iId)
      {
         qboolean got_match = FALSE;

         got_match |= check_and_set_int("primary_skill_level", &pSelect[select_index].primary_skill_level, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_int("secondary_skill_level", &pSelect[select_index].secondary_skill_level, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_float("aim_speed", &pSelect[select_index].aim_speed, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_qboolean("avoid_this_gun", &pSelect[select_index].avoid_this_gun, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_qboolean("prefer_higher_skill_attack", &pSelect[select_index].prefer_higher_skill_attack, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_float("primary_min_distance", &pSelect[select_index].primary_min_distance, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_float("primary_max_distance", &pSelect[select_index].primary_max_distance, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_float("secondary_min_distance", &pSelect[select_index].secondary_min_distance, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_float("secondary_max_distance", &pSelect[select_index].secondary_max_distance, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_float("opt_distance", &pSelect[select_index].opt_distance, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_int("use_percent", &pSelect[select_index].use_percent, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_qboolean("can_use_underwater", &pSelect[select_index].can_use_underwater, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_int("primary_fire_percent", &pSelect[select_index].primary_fire_percent, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_int("low_ammo_primary", &pSelect[select_index].low_ammo_primary, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         got_match |= check_and_set_int("low_ammo_secondary", &pSelect[select_index].low_ammo_secondary, arg2, arg3, arg1, msg, sizeof(msg), printfunc, arg);
         if(!got_match)
         {
            snprintf(msg, sizeof(msg), "unknown weapon setting %s.\n", arg2);
            printfunc(PRINTFUNC_ERROR, arg, msg);
            if(cmdtype == CLICMD_TYPE || cmdtype == SRVCMD_TYPE)
            {
               printfunc(PRINTFUNC_INFO, arg, "Usage: botweapon <weapon-name> <setting> <value> - set value\n");
               printfunc(PRINTFUNC_INFO, arg, "       botweapon <weapon-name> <setting> - shows setting value\n");
               printfunc(PRINTFUNC_INFO, arg, "       botweapon <weapon-name> - shows all setting values\n");
               printfunc(PRINTFUNC_INFO, arg, "       botweapon weapons - shows weapon-names\n");
            }
         }
      }
      else if(FStrEq("weapons", arg1))
      {
         printfunc(PRINTFUNC_INFO, arg, "List of available weapons:\n");

         select_index = -1;
         pSelect = &weapon_select[0];

         while (pSelect[++select_index].iId)
         {
            safevoid_snprintf(msg, sizeof(msg), "  %s\n", pSelect[select_index].weapon_name);
            printfunc(PRINTFUNC_INFO, arg, msg);
         }
      }
      else if((arg1 != NULL) && (*arg1 != 0) && FStrEq(arg1, "reset"))
      {
         //weapon select init
         InitWeaponSelect(submod_id);
         printfunc(PRINTFUNC_INFO, arg, "Bot weapon settings reset default.\n");
      }
      else
      {
         snprintf(msg, sizeof(msg), "Could not complete request! (unknown arg1: '%s')\n", arg1);
         printfunc(PRINTFUNC_ERROR, arg, msg);
         if(cmdtype == CLICMD_TYPE || cmdtype == SRVCMD_TYPE)
         {
            printfunc(PRINTFUNC_INFO, arg, "Usage: botweapon <weapon-name> <setting> <value> - set value\n");
            printfunc(PRINTFUNC_INFO, arg, "       botweapon <weapon-name> <setting> - shows setting value\n");
            printfunc(PRINTFUNC_INFO, arg, "       botweapon <weapon-name> - shows all setting values\n");
            printfunc(PRINTFUNC_INFO, arg, "       botweapon weapons - shows weapon-names\n");
            printfunc(PRINTFUNC_INFO, arg, "       botweapon reset - reset values back to defaults for all skills\n");
         }
      }
   }
   else
   {
       printfunc(PRINTFUNC_ERROR, arg, "Could not complete request! (arg1 not given)\n");
      if(cmdtype == CLICMD_TYPE || cmdtype == SRVCMD_TYPE)
      {
          printfunc(PRINTFUNC_INFO, arg, "Usage: botweapon <weapon-name> <setting> <value> - set value\n");
         printfunc(PRINTFUNC_INFO, arg, "       botweapon <weapon-name> <setting> - shows setting value\n");
         printfunc(PRINTFUNC_INFO, arg, "       botweapon <weapon-name> - shows all setting values\n");
         printfunc(PRINTFUNC_INFO, arg, "       botweapon weapons - shows weapon-names\n");
         printfunc(PRINTFUNC_INFO, arg, "       botweapon reset - reset values back to defaults for all skills\n");
      }
   }
}

static void ProcessBotSkillSetupCommand(const int cmdtype, const printfunc_t printfunc, void * arg, const char * arg1, const char * arg2, const char * arg3)
{
   char msg[128];

   if ((arg1 != NULL) && (*arg1 != 0) && atoi(arg1) >= 1 && atoi(arg1) <= 5)
   {
      int skill_idx = atoi(arg1)-1;
      qboolean got_match = FALSE;
      char skill_label[16];
      safevoid_snprintf(skill_label, sizeof(skill_label), "skill[%d]", skill_idx+1);

      got_match |= check_and_set_int("pause_frequency", &skill_settings[skill_idx].pause_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float("pause_time_min", &skill_settings[skill_idx].pause_time_min, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("pause_time_max", &skill_settings[skill_idx].pause_time_max, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float100("normal_strafe", &skill_settings[skill_idx].normal_strafe, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float100("battle_strafe", &skill_settings[skill_idx].battle_strafe, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_int("keep_optimal_dist", &skill_settings[skill_idx].keep_optimal_dist, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("shootcone_diameter", &skill_settings[skill_idx].shootcone_diameter, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("shootcone_minangle", &skill_settings[skill_idx].shootcone_minangle, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float("turn_skill", &skill_settings[skill_idx].turn_skill, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("turn_slowness", &skill_settings[skill_idx].turn_slowness, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("updown_turn_ration", &skill_settings[skill_idx].updown_turn_ration, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float("ping_emu_latency", &skill_settings[skill_idx].ping_emu_latency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("ping_emu_speed_varitation", &skill_settings[skill_idx].ping_emu_speed_varitation, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("ping_emu_position_varitation", &skill_settings[skill_idx].ping_emu_position_varitation, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float("hearing_sensitivity", &skill_settings[skill_idx].hearing_sensitivity, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("track_sound_time_min", &skill_settings[skill_idx].track_sound_time_min, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("track_sound_time_max", &skill_settings[skill_idx].track_sound_time_max, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float("respawn_react_delay", &skill_settings[skill_idx].respawn_react_delay, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("react_delay_min", &skill_settings[skill_idx].react_delay_min, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("react_delay_max", &skill_settings[skill_idx].react_delay_max, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_float("weaponchange_rate_min", &skill_settings[skill_idx].weaponchange_rate_min, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("weaponchange_rate_max", &skill_settings[skill_idx].weaponchange_rate_max, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_qboolean("can_longjump", &skill_settings[skill_idx].can_longjump, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("random_jump_frequency", &skill_settings[skill_idx].random_jump_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("random_jump_duck_frequency", &skill_settings[skill_idx].random_jump_duck_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("random_duck_frequency", &skill_settings[skill_idx].random_duck_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("random_longjump_frequency", &skill_settings[skill_idx].random_longjump_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

#if 0
      got_match |= check_and_set_qboolean("can_taujump", &skill_settings[skill_idx].can_taujump, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("attack_taujump_frequency", &skill_settings[skill_idx].attack_taujump_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("flee_taujump_frequency", &skill_settings[skill_idx].flee_taujump_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("attack_taujump_distance", &skill_settings[skill_idx].attack_taujump_distance, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("flee_taujump_distance", &skill_settings[skill_idx].flee_taujump_distance, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("flee_taujump_health", &skill_settings[skill_idx].flee_taujump_health, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_float("flee_taujump_escape_distance", &skill_settings[skill_idx].flee_taujump_escape_distance, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);

      got_match |= check_and_set_qboolean("can_shoot_through_walls", &skill_settings[skill_idx].can_shoot_through_walls, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
      got_match |= check_and_set_int("wallshoot_frequency", &skill_settings[skill_idx].wallshoot_frequency, arg2, arg3, skill_label, msg, sizeof(msg), printfunc, arg);
#endif

      if(!got_match)
      {
         snprintf(msg, sizeof(msg), "unknown skill setting %s.\n", arg2);
         printfunc(PRINTFUNC_ERROR, arg, msg);
         if(cmdtype == CLICMD_TYPE || cmdtype == SRVCMD_TYPE)
         {
             printfunc(PRINTFUNC_INFO, arg, "Usage: bot_skill_setup <skill> <setting> <value> - set value\n");
            printfunc(PRINTFUNC_INFO, arg, "       bot_skill_setup <skill> <setting> - shows setting value\n");
            printfunc(PRINTFUNC_INFO, arg, "       bot_skill_setup <skill> - shows all setting values\n");
            printfunc(PRINTFUNC_INFO, arg, "       bot_skill_setup reset - reset values back to defaults for all skills\n");
         }
      }
   }
   else if((arg1 != NULL) && (*arg1 != 0) && FStrEq(arg1, "reset"))
   {
      ResetSkillsToDefault();
      printfunc(PRINTFUNC_INFO, arg, "Bot skill settings reset default.\n");
   }
   else
   {
      if((arg1 == NULL) || (*arg1 == 0))
          printfunc(PRINTFUNC_ERROR, arg, "Could not complete request! (arg1 not given)\n");
       else
       {
          snprintf(msg, sizeof(msg), "Could not complete request! (invalid skill %d, use 1-5)\n", atoi(arg1));
          printfunc(PRINTFUNC_ERROR, arg, msg);
       }

      if(cmdtype == CLICMD_TYPE || cmdtype == SRVCMD_TYPE)
      {
          printfunc(PRINTFUNC_INFO, arg, "Usage: bot_skill_setup <skill> <setting> <value> - set value\n");
         printfunc(PRINTFUNC_INFO, arg, "       bot_skill_setup <skill> <setting> - shows setting value\n");
         printfunc(PRINTFUNC_INFO, arg, "       bot_skill_setup <skill> - shows all setting values\n");
         printfunc(PRINTFUNC_INFO, arg, "       bot_skill_setup reset - reset values back to defaults for all skills\n");
      }
   }
}

//
static qboolean ProcessCommand(const int cmdtype, const printfunc_t printfunc, void * arg, const char * pcmd, const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5)
{
   char msg[128];

   switch(cmdtype) {
      default:
         return FALSE;
      case CFGCMD_TYPE:
      case SRVCMD_TYPE:
      case CLICMD_TYPE:
         break;
   }

   if (FStrEq(pcmd, "info"))
   {
      UTIL_PrintBotInfo(printfunc, arg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "addbot"))
   {
      int cfg_bot_index = -1;
      const char * skin = (arg1 && *arg1) ? arg1 : NULL;
      const char * name = (arg2 && *arg2) ? arg2 : NULL;
      int skill = (arg3 && *arg3) ? atoi(arg3) : default_bot_skill;
      int top_color = (arg4 && *arg4) ? atoi(arg4) : -1;
      int bottom_color = (arg5 && *arg5) ? atoi(arg5) : -1;

      // save bots from config to special buffer
      if(cmdtype == CFGCMD_TYPE)
         cfg_bot_index = AddToCfgBotRecord(skin, name, skill, top_color, bottom_color);

      // only add bots if max_bots not reached
      if(max_bots == -1 || UTIL_GetClientCount() < max_bots)
      {
         BotCreate(skin, name, skill, top_color, bottom_color, cfg_bot_index);

         if(cmdtype == CFGCMD_TYPE)
            bot_cfg_pause_time = gpGlobals->time + 0.5;
      }

      bot_check_time = gpGlobals->time + 1.0;

      return TRUE;
   }
   else if (FStrEq(pcmd, "show_waypoints"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);
         if (temp)
            g_waypoint_on = g_path_waypoint = TRUE;
         else
            g_waypoint_on = g_path_waypoint = FALSE;
      }

      g_path_waypoint = g_waypoint_on;

      if (g_waypoint_on && g_path_waypoint)
         printfunc(PRINTFUNC_INFO, arg, "waypoints are VISIBLE\n");
      else
         printfunc(PRINTFUNC_INFO, arg, "waypoints are HIDDEN\n");

      return TRUE;
   }
   else if (FStrEq(pcmd, "debug_minmax"))
   {
      set_bool_toggle(&debug_minmax, arg1, "debug_minmax mode ENABLED\n", "debug_minmax mode DISABLED\n", printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "observer"))
   {
      set_bool_toggle((int *)&b_observer_mode, arg1, "observer mode ENABLED\n", "observer mode DISABLED\n", printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "team_balancetype"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);
         if (temp < 0)
            temp = 0;
         if (temp > 1)
            temp = 1;

         team_balancetype = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "team_balancetype is %d\n", team_balancetype);
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }

   else if (FStrEq(pcmd, "team_blockedlist"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         if(team_blockedlist)
            free(team_blockedlist);

         team_blockedlist = strdup(arg1);
      }

      if(!team_blockedlist)
         team_blockedlist = strdup("");

      safevoid_snprintf(msg, sizeof(msg), "team_blockedlist: %s\n", team_blockedlist ? team_blockedlist : "");
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }

   else if (FStrEq(pcmd, "botskill"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 1) || (temp > 5))
            printfunc(PRINTFUNC_ERROR, arg, "invalid botskill value!\n");
         else
            default_bot_skill = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "botskill is %d\n", default_bot_skill);
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_conntimes"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 1))
            printfunc(PRINTFUNC_ERROR, arg, "invalid bot_conntimes value!\n");
         else
            bot_conntimes = temp;
      }

      if(!IS_DEDICATED_SERVER() && bot_conntimes != 0)
      {
         bot_conntimes = 0;

         printfunc(PRINTFUNC_INFO, arg, "bot_conntimes is not supported on listenserver!");
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_conntimes is %d (%s)\n", bot_conntimes, (bot_conntimes==0?"disabled":"enabled"));
      printfunc(PRINTFUNC_INFO, arg, msg);

      if(bot_conntimes == 0)
         unhook_sendto_function();
      else
         hook_sendto_function();

      return TRUE;
   }
   else if (FStrEq(pcmd, "randomize_bots_on_mapchange"))
   {
      set_onoff_value("randomize_bots_on_mapchange", &randomize_bots_on_mapchange, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_add_level_tag"))
   {
      set_onoff_value("bot_add_level_tag", &bot_add_level_tag, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "botthinkfps"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 1) || (temp > 100))
            printfunc(PRINTFUNC_ERROR, arg, "invalid botthinkfps value!\n");
         else
            bot_think_spf = 1.0 / (float)temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "botthinkfps is %.2f\n", 1.0 / bot_think_spf);
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_percent"))
   {
      set_percent_value("bot_chat_percent", &bot_chat_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_taunt_percent"))
   {
      set_percent_value("bot_taunt_percent", &bot_taunt_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_whine_percent"))
   {
      set_percent_value("bot_whine_percent", &bot_whine_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_endgame_percent"))
   {
      set_percent_value("bot_endgame_percent", &bot_endgame_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_tag_percent"))
   {
      set_percent_value("bot_chat_tag_percent", &bot_chat_tag_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_drop_percent"))
   {
      set_percent_value("bot_chat_drop_percent", &bot_chat_drop_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_swap_percent"))
   {
      set_percent_value("bot_chat_swap_percent", &bot_chat_swap_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_chat_lower_percent"))
   {
      set_percent_value("bot_chat_lower_percent", &bot_chat_lower_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_logo_percent"))
   {
      set_percent_value("bot_logo_percent", &bot_logo_percent, arg1, msg, sizeof(msg), printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_shoot_breakables"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         int temp = atoi(arg1);

         if ((temp < 0) || (temp > 2))
            printfunc(PRINTFUNC_ERROR, arg, "invalid bot_shoot_breakables value! (0=never, 1=always, 2=path-blocking only)\n");
         else
            bot_shoot_breakables = temp;
      }

      safevoid_snprintf(msg, sizeof(msg), "bot_shoot_breakables is %d\n", bot_shoot_breakables);
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_reaction_time"))
   {
      safevoid_snprintf(msg, sizeof(msg), "bot_reaction_time setting was removed in jk_botti v1.42, ignoring setting\n");

      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "random_color"))
   {
      set_bool_toggle(&b_random_color, arg1, "random_color ENABLED\n", "random_color DISABLED\n", printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "botdontshoot"))
   {
      set_bool_toggle(&b_botdontshoot, arg1, "botdontshoot ENABLED\n", "botdontshoot DISABLED\n", printfunc, arg);
      return TRUE;
   }
   else if (FStrEq(pcmd, "min_bots"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         min_bots = atoi( arg1 );

         if ((min_bots < 0) || (min_bots > 31))
            min_bots = -1;
      }

      safevoid_snprintf(msg, sizeof(msg), "min_bots is set to %d\n", min_bots);
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "max_bots"))
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         max_bots = atoi( arg1 );

         if ((max_bots < 0) || (max_bots > 31))
            max_bots = -1;
      }

      safevoid_snprintf(msg, sizeof(msg), "max_bots is set to %d\n", max_bots);
      printfunc(PRINTFUNC_INFO, arg, msg);

      return TRUE;
   }
   else if (FStrEq(pcmd, "pause") && cmdtype == CFGCMD_TYPE)
   {
      if ((arg1 != NULL) && (*arg1 != 0))
      {
         bot_cfg_pause_time = gpGlobals->time + atoi( arg1 );

         if(bot_cfg_pause_time <= 0.0f)
            bot_cfg_pause_time = 0.1f;
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
            if(cmdtype == CLICMD_TYPE)
               g_waypoint_on = TRUE; //just in case
            printfunc(PRINTFUNC_INFO, arg, "autowaypoint is ON\n");
         }
         else
            printfunc(PRINTFUNC_INFO, arg, "autowaypoint is OFF\n");
      }

      return TRUE;
   }
   else if (FStrEq(pcmd, "botweapon"))
   {
      ProcessBotWeaponCommand(cmdtype, printfunc, arg, arg1, arg2, arg3);
      return TRUE;
   }
   else if (FStrEq(pcmd, "bot_skill_setup"))
   {
      ProcessBotSkillSetupCommand(cmdtype, printfunc, arg, arg1, arg2, arg3);
      return TRUE;
   }

   return FALSE;
}

#if _DEBUG
static void print_to_client(int, void *arg, char *msg)
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

      if(ProcessCommand(CLICMD_TYPE, print_to_client, pEntity, pcmd, arg1, arg2, arg3, arg4, arg5))
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
      safe_strcopy(g_argv, sizeof(g_argv), arg1);
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

   safe_strcopy(g_arg1, sizeof(g_arg1), arg1);
   null_terminate_buffer(g_arg1, sizeof(g_arg1));

   if (arg2 && *arg2)
   {
      safe_strcopy(g_arg2, sizeof(g_arg2), arg2);
      null_terminate_buffer(g_arg2, sizeof(g_arg2));
   }

   if (arg3 && *arg3)
   {
      safe_strcopy(g_arg3, sizeof(g_arg3), arg3);
      null_terminate_buffer(g_arg3, sizeof(g_arg3));
   }

   // allow the MOD DLL to execute the ClientCommand...
   isFakeClientCommand = TRUE;
   MDLL_ClientCommand(pBot);
   isFakeClientCommand = FALSE;
}


static char * parse_next_token(char *cmd_line, int *cmd_index)
{
   if (cmd_line[*cmd_index] != ' ')
      return NULL;
   cmd_line[(*cmd_index)++] = 0;
   char *token = &cmd_line[*cmd_index];
   while (cmd_line[*cmd_index] != ' ' && cmd_line[*cmd_index] != 0)
      (*cmd_index)++;
   return token;
}

static char * cfg_trim_quotes(char *arg)
{
   if (!arg)
      return arg;
   if (!strcmp(arg, "\"\""))
   {
      *arg = 0;
      return arg;
   }
   if (*arg == '\"')
      arg++;
   size_t len = strlen(arg);
   if (len > 0 && arg[len-1] == '\"')
      arg[len-1] = '\0';
   return arg;
}

static void print_to_console_config(int printtype, void *, char * msg)
{
   if(printtype == PRINTFUNC_ERROR)
      UTIL_ConsolePrintf("line[%d] config error: %s", bot_cfg_linenumber, msg);
}


void ProcessBotCfgFile(void)
{
   int ch;
   char cmd_line[256];
   int cmd_index;
   char server_cmd[80], server_cmd_print[80-1];
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

      // prevent buffer overflow on long lines
      if (cmd_index >= (int)sizeof(cmd_line) - 1)
         break;
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
   safevoid_snprintf(server_cmd, sizeof(server_cmd), "%s\n", cmd_line);
   safevoid_snprintf(server_cmd_print, sizeof(server_cmd_print), "%s", cmd_line);
   bot_cfg_linenumber++;

   cmd_index = 0;
   cmd = cmd_line;

   // skip command token
   while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
      cmd_index++;

   arg1 = parse_next_token(cmd_line, &cmd_index);
   arg2 = parse_next_token(cmd_line, &cmd_index);
   arg3 = parse_next_token(cmd_line, &cmd_index);
   arg4 = parse_next_token(cmd_line, &cmd_index);
   arg5 = parse_next_token(cmd_line, &cmd_index);

   if ((cmd_line[0] == '#') || (cmd_line[0] == '/' && cmd_line[1] == '/') || (cmd_line[0] == 0))
      return;  // return if comment or blank line

   arg1 = cfg_trim_quotes(arg1);
   arg2 = cfg_trim_quotes(arg2);
   arg3 = cfg_trim_quotes(arg3);
   arg4 = cfg_trim_quotes(arg4);
   arg5 = cfg_trim_quotes(arg5);

   if(ProcessCommand(CFGCMD_TYPE, print_to_console_config, NULL, cmd, arg1, arg2, arg3, arg4, arg5))
      return;

   UTIL_ConsolePrintf("line[%d] unknown command: '%s' (trying to execute as server command)\n", bot_cfg_linenumber, server_cmd_print);

   SERVER_COMMAND(server_cmd);
   SERVER_EXECUTE();
}


static void print_to_server_output(int, void *, char * msg)
{
   UTIL_ConsolePrintf("%s", msg);
}


void jk_botti_ServerCommand (void)
{
   if(FStrEq(CMD_ARGV(1), "kickall"))
   {
      int count = 0;

      // kick all bots.
      for (int index = 0; index < 32; index++)
      {
         if (bots[index].is_used)  // is this slot used?
         {
            BotKick(bots[index]);
            count++;
         }
      }

      if(count>0)
         UTIL_ConsolePrintf("Kicked %d bots.", count);
      else
         UTIL_ConsolePrintf("No bots on server to be kicked.");

      if(max_bots != -1 && min_bots != -1)
      {
         max_bots = min_bots = -1;
         UTIL_ConsolePrintf("Disabled min_bots/max_bots.");
      }
   }
   else if(!ProcessCommand(SRVCMD_TYPE, print_to_server_output, NULL, CMD_ARGV (1), CMD_ARGV (2), CMD_ARGV (3), CMD_ARGV (4), CMD_ARGV (5), CMD_ARGV (6)))
   {
      UTIL_ConsolePrintf("%s: Unknown command \'%s\'\n", CMD_ARGV(0), CMD_ARGS());
   }
}
