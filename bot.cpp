//
// JK_Botti - be more human!
//
// bot.cpp
//

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
#include "bot_weapon_select.h"

#include <sys/types.h>
#include <sys/stat.h>


extern edict_t *clients[32];
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use
extern int default_bot_skill;
//extern int bot_strafe_percent;
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

extern qboolean g_in_intermission;

extern bot_chat_t bot_chat[MAX_BOT_CHAT];
extern bot_chat_t bot_whine[MAX_BOT_CHAT];
extern int bot_chat_count;
extern int bot_whine_count;
extern int recent_bot_chat[];
extern int recent_bot_whine[];
extern qboolean checked_teamplay;
extern qboolean is_team_play;

extern int number_skins;
extern skin_t bot_skins[MAX_SKINS];

#define MAX_BOT_NAMES 100
int number_names = 0;
char bot_names[MAX_BOT_NAMES][BOT_NAME_LEN+1];

#define MAX_BOT_LOGOS 100
int num_logos = 0;
char bot_logos[MAX_BOT_LOGOS][16];

bot_t bots[32];   // max of 32 bots in a game
qboolean b_observer_mode = FALSE;
qboolean b_botdontshoot = FALSE;


void BotSpawnInit( bot_t &pBot )
{
   pBot.bot_think_time = -1.0;
   
   pBot.v_prev_origin = Vector(9999.0, 9999.0, 9999.0);
   pBot.f_speed_check_time = gpGlobals->time;

   pBot.waypoint_origin = Vector(0, 0, 0);
   pBot.f_waypoint_time = 0.0;
   pBot.curr_waypoint_index = -1;
   pBot.prev_waypoint_index[0] = -1;
   pBot.prev_waypoint_index[1] = -1;
   pBot.prev_waypoint_index[2] = -1;
   pBot.prev_waypoint_index[3] = -1;
   pBot.prev_waypoint_index[4] = -1;

   pBot.f_random_waypoint_time = gpGlobals->time;
   pBot.waypoint_goal = -1;
   pBot.f_waypoint_goal_time = 0.0;
   pBot.prev_waypoint_distance = 0.0;
   pBot.f_last_item_found = 0.0;

   pBot.exclude_points[0] = 0;
   pBot.exclude_points[1] = 0;
   pBot.exclude_points[2] = 0;
   pBot.exclude_points[3] = 0;
   pBot.exclude_points[4] = 0;
   pBot.exclude_points[5] = 0;

   pBot.blinded_time = 0.0;
   pBot.f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");
   pBot.f_prev_speed = 0.0;  // fake "paused" since bot is NOT stuck
   pBot.f_find_item = 0.0;

   pBot.ladder_dir = LADDER_UNKNOWN;
   pBot.f_start_use_ladder_time = 0.0;
   pBot.f_end_use_ladder_time = 0.0;
   pBot.waypoint_top_of_ladder = FALSE;

   pBot.f_wall_check_time = 0.0;
   pBot.f_wall_on_right = 0.0;
   pBot.f_wall_on_left = 0.0;
   pBot.f_dont_avoid_wall_time = 0.0;
   pBot.f_look_for_waypoint_time = 0.0;
   pBot.f_jump_time = 0.0;
   pBot.f_drop_check_time = 0.0;

   // pick a wander direction (50% of the time to the left, 50% to the right)
   pBot.wander_dir = (RANDOM_LONG2(1, 100) <= 50) ? WANDER_LEFT : WANDER_RIGHT;

   pBot.f_exit_water_time = 0.0;

   pBot.pBotEnemy = NULL;
   pBot.f_bot_see_enemy_time = gpGlobals->time;
   pBot.f_bot_find_enemy_time = gpGlobals->time;
   pBot.f_aim_tracking_time = 0.0;
   pBot.f_aim_x_angle_delta = 0.0;
   pBot.f_aim_y_angle_delta = 0.0;

   pBot.wpt_goal_type = WPT_GOAL_NONE;
   pBot.f_evaluate_goal_time = 0.0;

   pBot.pBotUser = NULL;
   pBot.f_bot_use_time = 0.0;
   pBot.b_bot_say = FALSE;
   pBot.f_bot_say = 0.0;
   pBot.bot_say_msg[0] = 0;
   pBot.f_bot_chat_time = gpGlobals->time;
   pBot.enemy_attack_count = 0;
   pBot.f_duck_time = 0.0;
   
   pBot.f_random_jump_time = 0.0;
   pBot.f_random_jump_duck_time = 0.0;
   pBot.f_random_jump_duck_end = 0.0;
   pBot.f_random_duck_time = 0.0;
   pBot.prev_random_type = 0;
   
   pBot.f_sniper_aim_time = 0.0;
   
   pBot.zooming = 0;
   
   pBot.b_longjump_do_jump = FALSE;
   pBot.b_longjump = FALSE;
   pBot.f_combat_longjump = 0.0;
   pBot.f_longjump_time = 0.0;
   pBot.b_combat_longjump = FALSE;
   
   pBot.f_shoot_time = gpGlobals->time;
   pBot.f_primary_charging = -1.0;
   pBot.f_secondary_charging = -1.0;
   pBot.charging_weapon_id = 0;
   pBot.f_grenade_search_time = 0.0;
   pBot.f_grenade_found_time = 0.0;
   pBot.current_weapon_index = -1;
   pBot.current_opt_distance = 99999.0;

   pBot.f_pause_time = 0.0;
   pBot.f_sound_update_time = 0.0;

   pBot.b_see_tripmine = FALSE;
   pBot.b_shoot_tripmine = FALSE;
   pBot.v_tripmine = Vector(0,0,0);

   pBot.b_use_health_station = FALSE;
   pBot.f_use_health_time = 0.0;
   pBot.b_use_HEV_station = FALSE;
   pBot.f_use_HEV_time = 0.0;

   pBot.b_use_button = FALSE;
   pBot.f_use_button_time = 0;
   pBot.b_lift_moving = FALSE;

   pBot.b_use_capture = FALSE;
   pBot.f_use_capture_time = 0.0;
   pBot.pCaptureEdict = NULL;

   pBot.b_spray_logo = FALSE;

   pBot.f_reaction_target_time = 0.0;
   pBot.ducking = 0;

   pBot.set_special_shoot_angle = FALSE;
   pBot.special_shoot_angle = 0.0;

   pBot.f_weaponchange_time = 0.0;

   memset(&(pBot.current_weapon), 0, sizeof(pBot.current_weapon));
   memset(&(pBot.m_rgAmmo), 0, sizeof(pBot.m_rgAmmo));
}


void BotNameInit( void )
{
   FILE *bot_name_fp;
   char bot_name_filename[256];
   int str_index;
   char name_buffer[80];
   int length, index;

   UTIL_BuildFileName_N(bot_name_filename, sizeof(bot_name_filename), "addons/jk_botti/jk_botti_names.txt", NULL);

   bot_name_fp = fopen(bot_name_filename, "r");

   if (bot_name_fp != NULL)
   {
      UTIL_ConsolePrintf("Loading %s...\n", bot_name_filename);
      
      while ((number_names < MAX_BOT_NAMES) &&
             (fgets(name_buffer, 80, bot_name_fp) != NULL))
      {
         length = strlen(name_buffer);

         if (name_buffer[length-1] == '\n')
         {
            name_buffer[length-1] = 0;  // remove '\n'
            length--;
         }

         str_index = 0;
         while (str_index < length)
         {
            if ((name_buffer[str_index] < ' ') || (name_buffer[str_index] > '~') ||
                (name_buffer[str_index] == '"'))
            for (index=str_index; index < length; index++)
               name_buffer[index] = name_buffer[index+1];

            str_index++;
         }

         if (name_buffer[0] != 0)
         {
            strncpy(bot_names[number_names], name_buffer, BOT_NAME_LEN);

            number_names++;
         }
      }

      fclose(bot_name_fp);
   }
}


void BotPickName( char *name_buffer )
{
   int name_index, index;
   qboolean used;
   edict_t *pPlayer;
   int attempts = 0;

   name_index = RANDOM_LONG2(1, number_names) - 1;  // zero based

   // check make sure this name isn't used
   used = TRUE;

   while (used)
   {
      used = FALSE;

      for (index = 1; index <= gpGlobals->maxClients; index++)
      {
         pPlayer = INDEXENT(index);

         if (pPlayer && !pPlayer->free && !FBitSet(pPlayer->v.flags, FL_PROXY))
         {
            if (strcmp(bot_names[name_index], STRING(pPlayer->v.netname)) == 0)
            {
               used = TRUE;
               break;
            }
         }
      }

      if (used)
      {
         name_index++;

         if (name_index == number_names)
            name_index = 0;

         attempts++;

         if (attempts == number_names)
            used = FALSE;  // break out of loop even if already used
      }
   }

   strcpy(name_buffer, bot_names[name_index]);
}


void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4, const char *arg5 )
{
   edict_t *BotEnt;
   char c_skin[BOT_SKIN_LEN+1];
   char c_name[BOT_NAME_LEN+1];
   int skill;
   int index;
   int i, j, length;
   qboolean found = FALSE;
   qboolean got_skill_arg = FALSE;
   int top_color, bottom_color;
   char c_topcolor[4], c_bottomcolor[4];

   top_color = -1;
   bottom_color = -1;

   {
      int  max_skin_index;

      max_skin_index = number_skins;
      
      if ((arg1 == NULL) || (*arg1 == 0))
      {
         index = RANDOM_LONG2(0, number_skins-1);

         // check if this skin has already been used...
         while (bot_skins[index].skin_used == TRUE)
         {
            index++;

            if (index == max_skin_index)
               index = 0;
         }

         bot_skins[index].skin_used = TRUE;

         // check if all skins are now used...
         for (i = 0; i < max_skin_index; i++)
         {
            if (bot_skins[i].skin_used == FALSE)
               break;
         }

         // if all skins are used, reset used to FALSE for next selection
         if (i == max_skin_index)
         {
            for (i = 0; i < max_skin_index; i++)
               bot_skins[i].skin_used = FALSE;
         }

         strcpy(c_skin, bot_skins[index].model_name);
      }
      else
      {
         strncpy( c_skin, arg1, BOT_SKIN_LEN-1 );
         c_skin[BOT_SKIN_LEN] = 0;  // make sure c_skin is null terminated
      }

      for (i = 0; c_skin[i] != 0; i++)
         c_skin[i] = tolower( c_skin[i] );  // convert to all lowercase

      index = 0;

      while ((!found) && (index < max_skin_index))
      {
         if (strcmp(c_skin, bot_skins[index].model_name) == 0)
            found = TRUE;
         else
            index++;
      }

      if (found == TRUE)
      {
         if ((arg2 != NULL) && (*arg2 != 0))
         {
            strncpy( c_name, arg2, BOT_SKIN_LEN-1 );
            c_name[BOT_SKIN_LEN] = 0;  // make sure c_name is null terminated
         }
         else
         {
            if (number_names > 0)
               BotPickName( c_name );
            else
               strcpy(c_name, bot_skins[index].bot_name);
         }
      }
      else
      {
         char dir_name[32];
         char filename[128];

         struct stat stat_str;

         GetGameDir(dir_name);

         safevoid_snprintf(filename, sizeof(filename), "%s/models/player/%s", dir_name, c_skin);

         if (stat(filename, &stat_str) != 0)
         {
            safevoid_snprintf(filename, sizeof(filename), "valve/models/player/%s", c_skin);

            if (stat(filename, &stat_str) != 0)
            {
               UTIL_ConsolePrintf("model \"%s\" is unknown.\n", c_skin );
               UTIL_ConsolePrintf("use barney, gina, gman, gordon, helmet, hgrunt,\n");
               UTIL_ConsolePrintf("    recon, robo, scientist, or zombie\n");
               return;
            }
         }

         if ((arg2 != NULL) && (*arg2 != 0))
         {
            strncpy( c_name, arg2, BOT_NAME_LEN-1 );
            c_name[BOT_NAME_LEN] = 0;  // make sure c_name is null terminated
         }
         else
         {
            if (number_names > 0)
               BotPickName( c_name );
            else
            {
               // copy the name of the model to the bot's name...
               strncpy( c_name, arg1, BOT_NAME_LEN-1 );
               c_name[BOT_NAME_LEN] = 0;  // make sure c_skin is null terminated
            }
         }
      }

      skill = 0;
      
      if ((arg3 != NULL) && (*arg3 != 0))
      {
         skill = atoi(arg3);
         got_skill_arg = TRUE;
      }
      if ((skill < 1) || (skill > 5))
      {
         skill = default_bot_skill;
         got_skill_arg = FALSE;
      }

      if ((arg4 != NULL) && (*arg4 != 0))
         top_color = atoi(arg4);

      if ((top_color < 0) || (top_color > 255))
         top_color = -1;
      else
         safevoid_snprintf(c_topcolor, sizeof(c_topcolor), "%d", top_color);

      if ((arg5 != NULL) && (*arg5 != 0))
         bottom_color = atoi(arg5);

      if ((bottom_color < 0) || (bottom_color > 255))
         bottom_color = -1;
      else
         safevoid_snprintf(c_bottomcolor, sizeof(c_bottomcolor), "%d", bottom_color);

      if ((top_color == -1) && (bottom_color == -1) && (b_random_color))
      {
         top_color = RANDOM_LONG2(0, 255);
         safevoid_snprintf(c_topcolor, sizeof(c_topcolor), "%d", top_color);

         bottom_color = RANDOM_LONG2(0, 255);
         safevoid_snprintf(c_bottomcolor, sizeof(c_bottomcolor), "%d", bottom_color);
      }
   }

   length = strlen(c_name);

   // remove any illegal characters from name...
   for (i = 0; i < length; i++)
   {
      if ((c_name[i] <= ' ') || (c_name[i] > '~') ||
          (c_name[i] == '"'))
      {
         for (j = i; j < length; j++)  // shuffle chars left (and null)
            c_name[j] = c_name[j+1];
         length--;
      }
   }
   
   //
   // Bug fix: remove "(1)" tags from name, HLDS adds "(1)" on duplicated names (and "(2), (3), ...")
   //
   if(c_name[0] == '(' && (c_name[1] >= '1' && c_name[1] <= '9') && c_name[2] == ')') 
   {
      length = strlen(&c_name[3]) + 1; // str+null
      for(i = 0; i < length; i++)
         c_name[i] = (&c_name[3])[i];
   }
   
   //
   // Bug fix: remove [lvlX] tags always
   //
   if(!strncmp(c_name, "[lvl", 4) && (c_name[4] >= '1' && c_name[4] <= '5') && c_name[5] == ']')
   {
      // Read skill from config file name
      if(!got_skill_arg)
      	 skill = c_name[4] - '0';
      
      length = strlen(&c_name[6]) + 1; // str+null
      for(i = 0; i < length; i++)
         c_name[i] = (&c_name[6])[i];
   }
   
   //
   if(bot_add_level_tag) 
   {
      char tmp[sizeof(c_name)];
      
      safevoid_snprintf(tmp, sizeof(tmp), "[lvl%d]%s", skill, c_name);
      tmp[sizeof(tmp)-1] = 0;  // make sure c_name is null terminated
      strcpy(c_name, tmp);
      c_name[sizeof(c_name)-1] = 0;  // make sure c_name is null terminated
   }
   
   BotEnt = (*g_engfuncs.pfnCreateFakeClient)( c_name );

   if (FNullEnt( BotEnt ))
   {
      if (pPlayer)
         ClientPrint( pPlayer, HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot (jk_botti)!\n");
   }
   else
   {
      char ptr[128];  // allocate space for message from ClientConnect
      char *infobuffer;
      int clientIndex;
      int index;

      UTIL_ConsolePrintf("Creating bot...\n");

      index = 0;
      while ((bots[index].is_used) && (index < 32))
         index++;

      if (index == 32)
      {
         UTIL_ConsolePrintf("Can't create bot!\n");
         return;
      }

      // create the player entity by calling MOD's player function
      // (from LINK_ENTITY_TO_CLASS for player object)

      CALL_GAME_ENTITY (PLID, "player", VARS(BotEnt));

      infobuffer = GET_INFOKEYBUFFER( BotEnt );
      clientIndex = ENTINDEX( BotEnt );

      if (!checked_teamplay)  // check for team play...
         BotCheckTeamplay();

      // is this a MOD that supports model colors AND it's not teamplay?
      SET_CLIENT_KEYVALUE( clientIndex, infobuffer, "model", c_skin );

      if (top_color != -1)
         SET_CLIENT_KEYVALUE( clientIndex, infobuffer, "topcolor", c_topcolor );

      if (bottom_color != -1)
         SET_CLIENT_KEYVALUE( clientIndex, infobuffer, "bottomcolor", c_bottomcolor );


      MDLL_ClientConnect( BotEnt, c_name, "127.0.0.1", ptr );

      // HPB_bot metamod fix - START

      // we have to do the ClientPutInServer() hook's job ourselves since calling the MDLL_
      // function calls directly the gamedll one, and not ours. You are allowed to call this
      // an "awful hack".

      while ((i < 32) && (clients[i] != NULL))
         i++;

      if (i < 32)
         clients[i] = BotEnt;  // store this clients edict in the clients array

      // HPB_bot metamod fix - END

      // Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
      MDLL_ClientPutInServer( BotEnt );

      BotEnt->v.flags |= FL_THIRDPARTYBOT | FL_FAKECLIENT;

      // initialize all the variables for this bot...

      bot_t &pBot = bots[index];
      memset(&pBot, 0, sizeof(pBot));

      pBot.is_used = TRUE;
      pBot.respawn_state = RESPAWN_IDLE;
      pBot.f_create_time = gpGlobals->time;
      pBot.name[0] = 0;  // name not set by server yet

      strcpy(pBot.skin, c_skin);

      pBot.top_color = top_color;
      pBot.bottom_color = bottom_color;

      pBot.pEdict = BotEnt;

      BotPickLogo(pBot);

      pBot.not_started = 1;  // hasn't joined game yet

      pBot.start_action = 0;  // not needed for non-team MODs


      BotSpawnInit(pBot);

      pBot.need_to_initialize = FALSE;  // don't need to initialize yet

      BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
      BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;

      // these should REALLY be MOD dependant...
      BotEnt->v.pitch_speed = 270;  // slightly faster than HLDM of 225
      BotEnt->v.yaw_speed = 250; // slightly faster than HLDM of 210

      pBot.idle_angle = 0.0;
      pBot.idle_angle_time = 0.0;
      pBot.round_end = 0;

//      pBot.strafe_percent = bot_strafe_percent;
      pBot.chat_percent = bot_chat_percent;
      pBot.taunt_percent = bot_taunt_percent;
      pBot.whine_percent = bot_whine_percent;
      pBot.chat_tag_percent = bot_chat_tag_percent;
      pBot.chat_drop_percent = bot_chat_drop_percent;
      pBot.chat_swap_percent = bot_chat_swap_percent;
      pBot.chat_lower_percent = bot_chat_lower_percent;
      pBot.logo_percent = bot_logo_percent;
      pBot.f_strafe_direction = 0.0;  // not strafing
      pBot.f_strafe_time = 0.0;
      pBot.reaction_time = bot_reaction_time;

      pBot.f_start_vote_time = gpGlobals->time + RANDOM_LONG2(120, 600);
      pBot.vote_in_progress = FALSE;
      pBot.f_vote_time = 0.0;

      pBot.bot_skill = skill - 1;  // 0 based for array indexes

      pBot.bot_team = -1;
      pBot.bot_class = -1;
   }
}


int BotInFieldOfView(bot_t &pBot, const Vector & dest)
{
   // find angles from source to destination...
   Vector entity_angles = UTIL_VecToAngles( dest );

   // make yaw angle 0 to 360 degrees if negative...
   if (entity_angles.y < 0)
      entity_angles.y += 360;

   // get bot's current view angle...
   float view_angle = pBot.pEdict->v.v_angle.y;

   // make view angle 0 to 360 degrees if negative...
   if (view_angle < 0)
      view_angle += 360;

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead,  45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle

   // rsm - START angle bug fix
   int angle = abs((int)view_angle - (int)entity_angles.y);

   if (angle > 180)
      angle = 360 - angle;

   return angle;
   // rsm - END
}


qboolean BotEntityIsVisible( bot_t &pBot, const Vector & dest )
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine( pBot.pEdict->v.origin + pBot.pEdict->v.view_ofs,
                   dest, ignore_monsters,
                   pBot.pEdict->v.pContainingEntity, &tr );

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction >= 1.0)
      return TRUE;
   else
      return FALSE;
}

void BotLogoInit(void)
{
   FILE *bot_logo_fp;
   char bot_logo_filename[256];
   char logo_buffer[80];
   int length;

   UTIL_BuildFileName_N(bot_logo_filename, sizeof(bot_logo_filename), "addons/jk_botti/jk_botti_logo.cfg", NULL);

   bot_logo_fp = fopen(bot_logo_filename, "r");

   if (bot_logo_fp != NULL)
   {
      UTIL_ConsolePrintf("Loading %s...\n", bot_logo_filename);
      
      while ((num_logos < MAX_BOT_LOGOS) &&
             (fgets(logo_buffer, 80, bot_logo_fp) != NULL))
      {
         length = strlen(logo_buffer);

         if (logo_buffer[length-1] == '\n')
         {
            logo_buffer[length-1] = 0;  // remove '\n'
            length--;
         }

         if (logo_buffer[0] != 0)
         {
            strncpy(bot_logos[num_logos], logo_buffer, 16);

            num_logos++;
         }
      }

      fclose(bot_logo_fp);
   }
}


void BotPickLogo(bot_t &pBot)
{
   qboolean used;
   int logo_index;
   int check_count;
   int index;

   pBot.logo_name[0] = 0;

   if (num_logos == 0)
      return;

   logo_index = RANDOM_LONG2(1, num_logos) - 1;  // zero based

   // check make sure this name isn't used
   used = TRUE;
   check_count = 0;

   while ((used) && (check_count < MAX_BOT_LOGOS))
   {
      used = FALSE;

      for (index=0; index < 32; index++)
      {
         if (bots[index].is_used)
         {
            if (strcmp(bots[index].logo_name, bot_logos[logo_index]) == 0)
            {
               used = TRUE;
            }
         }
      }

      if (used)
         logo_index++;

      if (logo_index == MAX_BOT_LOGOS)
         logo_index = 0;

      check_count++;
   }

   strcpy(pBot.logo_name, bot_logos[logo_index]);
}


void BotSprayLogo(edict_t *pEntity, char *logo_name)
{
   int index;
   TraceResult pTrace;
   Vector v_src, v_dest;
   
   v_src = pEntity->v.origin + pEntity->v.view_ofs;
   v_dest = v_src + UTIL_AnglesToForward(pEntity->v.v_angle) * 80;
   UTIL_TraceMove( v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &pTrace );

   index = DECAL_INDEX(logo_name);

   if (index < 0)
      return;

   if ((pTrace.pHit) && (pTrace.flFraction < 1.0))
   {
      if (pTrace.pHit->v.solid != SOLID_BSP)
         return;

      MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

      if (index > 255)
      {
         WRITE_BYTE( TE_WORLDDECALHIGH);
         index -= 256;
      }
      else
         WRITE_BYTE( TE_WORLDDECAL );

      WRITE_COORD( pTrace.vecEndPos.x );
      WRITE_COORD( pTrace.vecEndPos.y );
      WRITE_COORD( pTrace.vecEndPos.z );
      WRITE_BYTE( index );

      MESSAGE_END();

      EMIT_SOUND_DYN2(pEntity, CHAN_VOICE, "player/sprayer.wav", 1.0, ATTN_NORM, 0, 100);
   }
}

void BotFindItem( bot_t &pBot )
{
   edict_t *pent = NULL;
   edict_t *pPickupEntity = NULL;
   Vector pickup_origin;
   Vector entity_origin;
   float radius = 500;
   qboolean can_pickup;
   float min_distance;
   char item_name[40];
   TraceResult tr;
   Vector vecStart;
   Vector vecEnd;
   int angle_to_entity;
   int itemflag, select_index;
   bot_weapon_select_t *pSelect = &weapon_select[0];
   edict_t *pEdict = pBot.pEdict;

   pBot.pBotPickupItem = NULL;

   // forget about our item if it's been three seconds
   // forget about item if it we picked it up
   if (pBot.f_last_item_found > 0 && pBot.f_last_item_found < (gpGlobals->time - 5.0))
   {
      pBot.f_find_item = gpGlobals->time + 2.0;
      pBot.f_last_item_found = -1;
      pBot.pBotPickupItem = NULL;
   }

   if (pBot.pBotPickupItem && ((pBot.pBotPickupItem->v.effects & EF_NODRAW) ||
      !BotEntityIsVisible(pBot, UTIL_GetOrigin(pBot.pBotPickupItem))))
   {
      pBot.f_last_item_found = -1;
      pBot.pBotPickupItem = NULL;
   }

   // halt the rest of the function
   if (pBot.f_find_item > gpGlobals->time || pBot.pBotPickupItem)
      return;

   // use a MUCH smaller search radius when waypoints are available
   if ((num_waypoints > 0) && (pBot.curr_waypoint_index != -1))
      radius = 260.0;
   else
      radius = 520.0;

   min_distance = radius + 1.0;

   while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, radius )) != NULL)
   {
      can_pickup = FALSE;  // assume can't use it until known otherwise

      strcpy(item_name, STRING(pent->v.classname));

      // see if this is a "func_" type of entity (func_button, etc.)...
      if (strncmp("func_", item_name, 5) == 0)
      {
         // BModels have 0,0,0 for origin so must use VecBModelOrigin...
         entity_origin = VecBModelOrigin(pent);

         vecStart = pEdict->v.origin + pEdict->v.view_ofs;
         vecEnd = entity_origin;

         angle_to_entity = BotInFieldOfView( pBot, vecEnd - vecStart );

         // check if entity is outside field of view (+/- 45 degrees)
         if (angle_to_entity > 45)
            continue;  // skip this item if bot can't "see" it

         // check if entity is a ladder (ladders are a special case)
         // DON'T search for ladders if there are waypoints in this level...
         if ((strcmp("func_ladder", item_name) == 0) && (num_waypoints == 0))
         {
            // force ladder origin to same z coordinate as bot since
            // the VecBModelOrigin is the center of the ladder.  For
            // LONG ladders, the center MAY be hundreds of units above
            // the bot.  Fake an origin at the same level as the bot...

            entity_origin.z = pEdict->v.origin.z;
            vecEnd = entity_origin;

            // trace a line from bot's eyes to func_ladder entity...
            UTIL_TraceMove( vecStart, vecEnd, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr);

            // check if traced all the way up to the entity (didn't hit wall)
            if (tr.flFraction >= 1.0)
            {
               // find distance to item for later use...
               float distance = (vecEnd - vecStart).Length( );

               // use the ladder about 100% of the time, if haven't
               // used a ladder in at least 5 seconds...
               if ((RANDOM_LONG2(1, 100) <= 100) &&
                   ((pBot.f_end_use_ladder_time + 5.0) < gpGlobals->time))
               {
                  // if close to ladder...
                  if (distance < 100)
                  {
                     // don't avoid walls for a while
                     pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                  }

                  can_pickup = TRUE;
               }
            }
         }
         else
         {
            // trace a line from bot's eyes to func_ entity...
            UTIL_TraceMove( vecStart, vecEnd, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr);

            // check if traced all the way up to the entity (didn't hit wall)
            if (strcmp(item_name, STRING(tr.pHit->v.classname)) == 0)
            {
               // find distance to item for later use...
               float distance = (vecEnd - vecStart).Length( );

               // check if entity is wall mounted health charger...
               if (strcmp("func_healthcharger", item_name) == 0)
               {
                  // check if the bot can use this item and
                  // check if the recharger is ready to use (has power left)...
                  if ((pEdict->v.health < 100) && (pent->v.frame == 0))
                  {
                     // check if flag not set...
                     if (!pBot.b_use_health_station)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           pBot.b_use_health_station = TRUE;
                           pBot.f_use_health_time = gpGlobals->time;
                        }

                        // if close to health station...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                  }
                  else
                  {
                     // don't need or can't use this item...
                     pBot.b_use_health_station = FALSE;
                  }
               }

               // check if entity is wall mounted HEV charger...
               else if (strcmp("func_recharge", item_name) == 0)
               {
                  // check if the bot can use this item and
                  // check if the recharger is ready to use (has power left)...
                  if ((pEdict->v.armorvalue < VALVE_MAX_NORMAL_BATTERY) &&
                      (pent->v.frame == 0))
                  {
                     // check if flag not set and facing it...
                     if (!pBot.b_use_HEV_station)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           pBot.b_use_HEV_station = TRUE;
                           pBot.f_use_HEV_time = gpGlobals->time;
                        }

                        // if close to HEV recharger...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                  }
                  else
                  {
                     // don't need or can't use this item...
                     pBot.b_use_HEV_station = FALSE;
                  }
               }

               // check if entity is a button...
               else if (strcmp("func_button", item_name) == 0)
               {
                  // use the button about 100% of the time, if haven't
                  // used a button in at least 5 seconds...
                  if ((RANDOM_LONG2(1, 100) <= 100) &&
                      ((pBot.f_use_button_time + 5) < gpGlobals->time))
                  {
                     // check if flag not set and facing it...
                     if (!pBot.b_use_button)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           pBot.b_use_button = TRUE;
                           pBot.b_lift_moving = FALSE;
                           pBot.f_use_button_time = gpGlobals->time;
                        }

                        // if close to button...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                  }
                  else
                  {
                     // don't need or can't use this item...
                     pBot.b_use_button = FALSE;
                  }
               }
            }
         }
      }
      else  // everything else...
      {
         entity_origin = pent->v.origin;

         vecStart = pEdict->v.origin + pEdict->v.view_ofs;
         vecEnd = entity_origin;

         // find angles from bot origin to entity...
         angle_to_entity = BotInFieldOfView( pBot, vecEnd - vecStart );

         // check if entity is outside field of view (+/- 45 degrees)
         if (angle_to_entity > 45)
            continue;  // skip this item if bot can't "see" it

         // check if line of sight to object is not blocked (i.e. visible)
         if (BotEntityIsVisible( pBot, vecEnd ))
         {
            // check if entity is a weapon...
            if ((itemflag = GetWeaponItemFlag(item_name)) != 0)
            {
               if (pent->v.effects & EF_NODRAW)
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }
               
               // check if player has already
               select_index = -1;
               qboolean found = FALSE;
               while(pSelect[++select_index].iId)
               {
                  if((itemflag & pSelect[select_index].waypoint_flag) == 0)
                     continue;
                  
                  found = TRUE;
                  break;
               }
               
               if(found)
               {
                  //does player have this weapon
                  if(pBot.pEdict->v.weapons & (1<<pSelect[select_index].iId))
                  {
                     // is ammo low?
                     if(BotPrimaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                  }
               }
               else
                    can_pickup = TRUE;
            }

            // pick up ammo
            else if ((itemflag = GetAmmoItemFlag(item_name)) != 0)
            {
               if (pent->v.effects & EF_NODRAW)
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }
               
               // check if player is running out of this ammo on any weapon
               select_index = -1;
               while(pSelect[++select_index].iId)
               {
                  if(itemflag & pSelect[select_index].ammo1_waypoint_flag)
                  {
                     if(BotPrimaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                     else if(BotSecondaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                  }
                  else if(itemflag & pSelect[select_index].ammo2_waypoint_flag)
                  {
                     if(BotPrimaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                     else if(BotSecondaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                  }
                  
                  if(can_pickup)
                     break;
               }
            }
            
            // pick up longjump
            else if (strcmp("item_longjump", item_name) == 0)
            {
               if (pent->v.effects & EF_NODRAW)
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }
               
               // check if the bot can use this item...
               if (!pBot.b_longjump && skill_settings[pBot.bot_skill].can_longjump)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a battery...
            else if (strcmp("item_battery", item_name) == 0)
            {
               // check if the item is not visible (i.e. has not respawned)
               if (pent->v.effects & EF_NODRAW)
                  continue;

               // check if the bot can use this item...
               if (pEdict->v.armorvalue < VALVE_MAX_NORMAL_BATTERY)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a healthkit...
            else if (strcmp("item_healthkit", item_name) == 0)
            {
               // check if the item is not visible (i.e. has not respawned)
               if (pent->v.effects & EF_NODRAW)
                  continue;

               // check if the bot can use this item...
               if (pEdict->v.health < VALVE_MAX_NORMAL_HEALTH)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a packed up weapons box...
            else if (strcmp("weaponbox", item_name) == 0)
            {
               can_pickup = TRUE;
            }

            // check if entity is the spot from RPG laser
            else if (strcmp("laser_spot", item_name) == 0)
            {
            }

            // check if entity is an armed tripmine
            else if (strcmp("monster_tripmine", item_name) == 0)
            {
               float distance = (pent->v.origin - pEdict->v.origin).Length( );

               if (pBot.b_see_tripmine)
               {
                  // see if this tripmine is closer to bot...
                  if (distance < (pBot.v_tripmine - pEdict->v.origin).Length())
                  {
                     pBot.v_tripmine = pent->v.origin;
                     pBot.b_shoot_tripmine = FALSE;

                     // see if bot is far enough to shoot the tripmine...
                     if (distance >= 375)
                     {
                        pBot.b_shoot_tripmine = TRUE;
                     }
                  }
               }
               else
               {
                  pBot.b_see_tripmine = TRUE;
                  pBot.v_tripmine = pent->v.origin;
                  pBot.b_shoot_tripmine = FALSE;

                  // see if bot is far enough to shoot the tripmine...
                  if (distance >= 375)  // 375 is damage radius
                  {
                     pBot.b_shoot_tripmine = TRUE;
                  }
               }
            }

            // check if entity is an armed satchel charge
            else if (strcmp("monster_satchel", item_name) == 0)
            {
            }

            // check if entity is a snark (squeak grenade)
            else if (strcmp("monster_snark", item_name) == 0)
            {
            }
            
         }  // end if object is visible
      }  // end else not "func_" entity

      if (can_pickup) // if the bot found something it can pickup...
      {
         float distance = (entity_origin - pEdict->v.origin).Length( );

         // see if it's the closest item so far...
         if (distance < min_distance)
         {
            min_distance = distance;        // update the minimum distance
            pPickupEntity = pent;        // remember this entity
            pickup_origin = entity_origin;  // remember location of entity
         }
      }
   }  // end while loop

   if (pPickupEntity != NULL)
   {
      // let's head off toward that item...
      Vector v_item = pickup_origin - pEdict->v.origin;

      Vector bot_angles = UTIL_VecToAngles( v_item );

      pEdict->v.ideal_yaw = bot_angles.y;

      BotFixIdealYaw(pEdict);

      pBot.pBotPickupItem = pPickupEntity;  // save the item bot is trying to get
   }
}


qboolean BotLookForGrenades( bot_t &pBot )
{
   static const char * grenade_names[] = {
      "grenade", "monster_satchel", "monster_snark", "rpg_rocket", "hvr_rocket",
      NULL,
   };

   edict_t *pent;
   Vector entity_origin;
   float radius = 500;
   char classname[40];
   edict_t *pEdict = pBot.pEdict;

   pent = NULL;
   while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, radius )) != NULL)
   {
      strcpy(classname, STRING(pent->v.classname));

      entity_origin = pent->v.origin;

      if (FInViewCone( entity_origin, pEdict ) &&
          FVisible( entity_origin, pEdict ))
      {
          const char ** p_grna = grenade_names;
          
         while(*p_grna) {
            if (strcmp(*p_grna, classname) == 0)
               return TRUE;
            p_grna++;
         }
      }
   }

   return FALSE;  // no grenades were found
}


static void HandleWallOnLeft( bot_t &pBot ) 
{
   edict_t *pEdict = pBot.pEdict;
        
   // if there was a wall on the left over 1/2 a second ago then
   // 20% of the time randomly turn between 45 and 60 degrees

   if ((pBot.f_wall_on_left != 0) &&
       (pBot.f_wall_on_left <= gpGlobals->time - 0.5) &&
       (RANDOM_LONG2(1, 100) <= 20))
   {
      pEdict->v.ideal_yaw += RANDOM_LONG2(45, 60);

      BotFixIdealYaw(pEdict);

      pBot.f_move_speed /= 3;  // move slowly while turning
      pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
   }

   pBot.f_wall_on_left = 0;  // reset wall detect time
}


static void HandleWallOnRight( bot_t &pBot ) 
{
   edict_t *pEdict = pBot.pEdict;
        
   // if there was a wall on the right over 1/2 a second ago then
   // 20% of the time randomly turn between 45 and 60 degrees

   if ((pBot.f_wall_on_right != 0) &&
       (pBot.f_wall_on_right <= gpGlobals->time - 0.5) &&
       (RANDOM_LONG2(1, 100) <= 20))
   {
      pEdict->v.ideal_yaw -= RANDOM_LONG2(45, 60);

      BotFixIdealYaw(pEdict);

      pBot.f_move_speed /= 3;  // move slowly while turning
      pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
   }

   pBot.f_wall_on_right = 0;  // reset wall detect time
}


void BotThink( bot_t &pBot )
{
   Vector v_diff;             // vector from previous to current location
   float pitch_degrees;
   float yaw_degrees;
   float moved_distance;      // length of v_diff vector (distance bot moved)
   TraceResult tr;
   qboolean found_waypoint;
   qboolean is_idle;
   float f_strafe_speed;
   qboolean bCrouchJump;
   char chat_text[81];
   char chat_name[64];
   char temp_name[64];
   const char *bot_name;

   edict_t *pEdict = pBot.pEdict;


   pEdict->v.flags |= FL_THIRDPARTYBOT | FL_FAKECLIENT;

   if (pBot.name[0] == 0)  // name filled in yet?
      strcpy(pBot.name, STRING(pBot.pEdict->v.netname));


// TheFatal - START from Advanced Bot Framework (Thanks Rich!)

   // adjust the millisecond delay based on the frame rate interval...
   if (pBot.msecdel <= gpGlobals->time)
   {
      pBot.msecdel = gpGlobals->time + 0.5;
      if (pBot.msecnum > 0)
         pBot.msecval = 450.0/pBot.msecnum;
      pBot.msecnum = 0;
   }
   else
      pBot.msecnum++;

   if (pBot.msecval < 1)    // don't allow msec to be less than 1...
      pBot.msecval = 1;

   if (pBot.msecval > 100)  // ...or greater than 100
      pBot.msecval = 100;

// TheFatal - END

   pBot.f_frame_time = pBot.msecval / 1000;  // calculate frame time

   
   pEdict->v.button = 0;
   pBot.f_move_speed = 0.0;

   // if the bot hasn't selected stuff to start the game yet, go do that...
   if (pBot.not_started)
   {
      BotStartGame( pBot );
      
      g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot.f_move_speed,
                                   0, 0, pEdict->v.button, 0, (byte)pBot.msecval);

      return;
   }

   // does bot need to say a message and time to say a message?
   if ((pBot.b_bot_say) && (pBot.f_bot_say < gpGlobals->time))
   {
      pBot.b_bot_say = FALSE;

      UTIL_HostSay(pEdict, 0, pBot.bot_say_msg);
   }
   
   // in intermission.. don't do anything, freeze bot
   if(g_in_intermission)
   {
      g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, 0, 0, 0, 0, 0, (byte)pBot.msecval);
      return;
   }

   // if the bot is dead, randomly press fire to respawn...
   if ((pEdict->v.health < 1) || (pEdict->v.deadflag != DEAD_NO))
   {
      if (pBot.need_to_initialize)
      {
         BotSpawnInit(pBot);

         pBot.need_to_initialize = FALSE;

         // did another player kill this bot AND bot whine messages loaded AND
         // has the bot been alive for at least 15 seconds AND
         if ((pBot.killer_edict != NULL) && (bot_whine_count > 0) &&
             ((pBot.f_bot_spawn_time + 15.0) <= gpGlobals->time))
         {
            int whine_index;
            qboolean used;
            int i, recent_count;

            if ((RANDOM_LONG2(1,100) <= pBot.whine_percent))
            {
               // set chat flag and time to chat...
               pBot.b_bot_say = TRUE;
               pBot.f_bot_say = gpGlobals->time + 5.0 + RANDOM_FLOAT2(0.0, 5.0);

               recent_count = 0;

               while (recent_count < 5)
               {
                  whine_index = RANDOM_LONG2(0, bot_whine_count-1);

                  used = FALSE;

                  for (i=0; i < 5; i++)
                  {
                     if (recent_bot_whine[i] == whine_index)
                        used = TRUE;
                  }

                  if (used)
                     recent_count++;
                  else
                     break;
               }

               for (i=4; i > 0; i--)
                  recent_bot_whine[i] = recent_bot_whine[i-1];

               recent_bot_whine[0] = whine_index;

               if (bot_whine[whine_index].can_modify)
                  BotChatText(bot_whine[whine_index].text, chat_text);
               else
                  strcpy(chat_text, bot_whine[whine_index].text);

               if (pBot.killer_edict->v.netname)
               {
                  strncpy(temp_name, STRING(pBot.killer_edict->v.netname), 31);
                  temp_name[31] = 0;

                  BotChatName(temp_name, chat_name);
               }
               else
                  strcpy(chat_name, "NULL");

               bot_name = STRING(pEdict->v.netname);

               BotChatFillInName(pBot.bot_say_msg, chat_text, chat_name, bot_name);
            }
         }
      }

      if (RANDOM_LONG2(1, 100) <= 50)
         pEdict->v.button = IN_ATTACK;
      
      BotAimPre(pBot);
      g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot.f_move_speed,
                                   0, 0, pEdict->v.button, 0, (byte)pBot.msecval);
      BotAimPost(pBot);

      return;
   }

   if ((bot_chat_count > 0) && (pBot.f_bot_chat_time < gpGlobals->time))
   {
      pBot.f_bot_chat_time = gpGlobals->time + 30.0;

      if (RANDOM_LONG2(1,100) <= pBot.chat_percent)
      {
         int chat_index;
         qboolean used;
         int i, recent_count;

         // set chat flag and time to chat...
         pBot.b_bot_say = TRUE;
         pBot.f_bot_say = gpGlobals->time + 5.0 + RANDOM_FLOAT2(0.0, 5.0);

         recent_count = 0;

         while (recent_count < 5)
         {
            chat_index = RANDOM_LONG2(0, bot_chat_count-1);

            used = FALSE;

            for (i=0; i < 5; i++)
            {
               if (recent_bot_chat[i] == chat_index)
                  used = TRUE;
            }

            if (used)
               recent_count++;
            else
               break;
         }

         for (i=4; i > 0; i--)
            recent_bot_chat[i] = recent_bot_chat[i-1];

         recent_bot_chat[0] = chat_index;

         if (bot_chat[chat_index].can_modify)
            BotChatText(bot_chat[chat_index].text, chat_text);
         else
            strcpy(chat_text, bot_chat[chat_index].text);

         strcpy(chat_name, STRING(pBot.pEdict->v.netname));

         bot_name = STRING(pEdict->v.netname);

         BotChatFillInName(pBot.bot_say_msg, chat_text, chat_name, bot_name);
      }
   }


   // set this for the next time the bot dies so it will initialize stuff
   if (pBot.need_to_initialize == FALSE)
   {
      pBot.need_to_initialize = TRUE;
      pBot.f_bot_spawn_time = gpGlobals->time;
   }

   is_idle = FALSE;

   if (pBot.blinded_time > gpGlobals->time)
   {
      is_idle = TRUE;  // don't do anything while blinded
   }

   if (is_idle)
   {
      if (pBot.idle_angle_time <= gpGlobals->time)
      {
         pBot.idle_angle_time = gpGlobals->time + RANDOM_FLOAT2(0.5, 2.0);

         pEdict->v.ideal_yaw = pBot.idle_angle + RANDOM_FLOAT2(0.0, 60.0) - 30.0;

         BotFixIdealYaw(pEdict);
      }

      // turn towards ideal_yaw by yaw_speed degrees (slower than normal)
      BotChangeYaw( pBot, pEdict->v.yaw_speed / 2 );
      
      BotAimPre(pBot);
      g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot.f_move_speed,
                                   0, 0, pEdict->v.button, 0, (byte)pBot.msecval);
      BotAimPost(pBot);

      return;
   }
   else
   {
      pBot.idle_angle = pEdict->v.v_angle.y;
   }

   pBot.f_move_speed = pBot.f_max_speed;  // set to max speed

   if (pBot.f_speed_check_time <= gpGlobals->time)
   {
      // see how far bot has moved since the previous position...
      v_diff = pBot.v_prev_origin - pEdict->v.origin;
      moved_distance = v_diff.Length();

      // save current position as previous
      pBot.v_prev_origin = pEdict->v.origin;
      pBot.f_speed_check_time = gpGlobals->time + 0.2;
   }
   else
   {
      moved_distance = 2.0;
   }

   // if the bot is not underwater AND not in the air (or on ladder),
   // check if the bot is about to fall off high ledge or into water...
   if ((pEdict->v.waterlevel != 3) &&
       (pEdict->v.flags & FL_ONGROUND) &&
       (pEdict->v.movetype != MOVETYPE_FLY) &&
       (pBot.f_drop_check_time < gpGlobals->time))
   {
      pBot.f_drop_check_time = gpGlobals->time + 0.05;

      BotLookForDrop( pBot );
   }

   // turn towards ideal_pitch by pitch_speed degrees
   pitch_degrees = BotChangePitch( pBot, pEdict->v.pitch_speed );

   // turn towards ideal_yaw by yaw_speed degrees
   yaw_degrees = BotChangeYaw( pBot, pEdict->v.yaw_speed );

   {
      if (b_botdontshoot == 0)
      {
         pBot.pBotEnemy = BotFindEnemy( pBot );
      }
      else
         pBot.pBotEnemy = NULL;  // clear enemy pointer (no ememy for you!)

      if (pBot.pBotEnemy != NULL)  // does an enemy exist?
      {
         BotShootAtEnemy( pBot );  // shoot at the enemy
         
         pBot.f_pause_time = 0;  // dont't pause if enemy exists
         
         // check if bot is on a ladder
         if (pEdict->v.movetype == MOVETYPE_FLY)
         {
            // bot is stuck on a ladder... jump off ladder
            pEdict->v.button |= IN_JUMP;
         }
      }

      else if (pBot.f_pause_time > gpGlobals->time)  // is bot "paused"?
      {
         // you could make the bot look left then right, or look up
         // and down, to make it appear that the bot is hunting for
         // something (don't do anything right now)
      }

      // is bot being "used" and can still follow "user"?
      else if ((pBot.pBotUser != NULL) && BotFollowUser( pBot ))
      {
         // do nothing here!
         ;
      }

      else
      {
         // no enemy, let's just wander around...

         // took too long trying to spray logo?...
         if ((pBot.b_spray_logo) &&
             ((pBot.f_spray_logo_time + 3.0) < gpGlobals->time))
         {
            pBot.b_spray_logo = FALSE;
            pEdict->v.idealpitch = 0.0f;
         }

         if (pBot.b_spray_logo)  // trying to spray a logo?
         {
            Vector v_src, v_dest, angle;
            TraceResult tr;

            // find the nearest wall to spray logo on (or floor)...

            angle = pEdict->v.v_angle;
            angle.x = 0;  // pitch is level horizontally

            v_src = pEdict->v.origin + pEdict->v.view_ofs;
            v_dest = v_src + UTIL_AnglesToForward(angle) * 100;

            UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);

            if (tr.flFraction < 1.0)
            {
               // already facing the correct yaw, just set pitch...
               pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
            }
            else
            {
               v_dest = v_src + gpGlobals->v_right * 100;  // to the right

               UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                               pEdict->v.pContainingEntity, &tr);

               if (tr.flFraction < 1.0)
               {
                  // set the ideal yaw and pitch...
                  Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);

                  pEdict->v.ideal_yaw = bot_angles.y;

                  BotFixIdealYaw(pEdict);

                  pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
               }
               else
               {
                  v_dest = v_src + gpGlobals->v_right * -100;  // to the left

                  UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                                  pEdict->v.pContainingEntity, &tr);

                  if (tr.flFraction < 1.0)
                  {
                     // set the ideal yaw and pitch...
                     Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);

                     pEdict->v.ideal_yaw = bot_angles.y;

                     BotFixIdealYaw(pEdict);

                     pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
                  }
                  else
                  {
                     v_dest = v_src + gpGlobals->v_forward * -100;  // behind

                     UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                                     pEdict->v.pContainingEntity, &tr);

                     if (tr.flFraction < 1.0)
                     {
                        // set the ideal yaw and pitch...
                        Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);

                        pEdict->v.ideal_yaw = bot_angles.y;

                        BotFixIdealYaw(pEdict);

                        pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
                     }
                     else
                     {
                        // on the ground...

                        angle = pEdict->v.v_angle;
                        angle.x = 85.0f;  // 85 degrees is downward

                        v_src = pEdict->v.origin + pEdict->v.view_ofs;
                        v_dest = v_src + UTIL_AnglesToForward(angle) * 80;

                        UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                                        pEdict->v.pContainingEntity, &tr);

                        if (tr.flFraction < 1.0)
                        {
                           // set the pitch...

                           pEdict->v.idealpitch = 85.0f;

                           BotFixIdealPitch(pEdict);
                        }
                     }
                  }
               }
            }

            pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;

            // is there a wall close to us?

            v_src = pEdict->v.origin + pEdict->v.view_ofs;
            v_dest = v_src + UTIL_AnglesToForward(pEdict->v.v_angle) * 80;

            UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);

            if (tr.flFraction < 1.0)
            {
               BotSprayLogo(pEdict, pBot.logo_name);

               pBot.b_spray_logo = FALSE;

               pEdict->v.idealpitch = 0.0f;
            }
         }

         if ((pEdict->v.waterlevel != 2) &&  // is bot NOT under water?
             (pEdict->v.waterlevel != 3) &&
             (!pBot.b_spray_logo))          // AND not trying to spray a logo
         {
            // reset pitch to 0 (level horizontally)
            pEdict->v.idealpitch = 0;
            pEdict->v.v_angle.x = 0;
         }

         // check if bot should look for items now or not...
         if (pBot.f_find_item <= gpGlobals->time)
         {
            pBot.f_find_item = gpGlobals->time + 0.1;

            BotFindItem( pBot );  // see if there are any visible items
         }

         // check if bot sees a tripmine...
         if (pBot.b_see_tripmine)
         {
            // check if bot can shoot the tripmine...
            if ((pBot.b_shoot_tripmine) && BotShootTripmine( pBot ))
            {
               // shot at tripmine, may or may not have hit it, clear
               // flags anyway, next BotFindItem will see it again if
               // it is still there...

               pBot.b_shoot_tripmine = FALSE;
               pBot.b_see_tripmine = FALSE;

               // pause for a while to allow tripmine to explode...
               pBot.f_pause_time = gpGlobals->time + 0.5;
            }
            else  // run away!!!
            {
               Vector tripmine_angles;

               tripmine_angles = UTIL_VecToAngles( pBot.v_tripmine - pEdict->v.origin );

               // face away from the tripmine
               pEdict->v.ideal_yaw += 180;  // rotate 180 degrees

               BotFixIdealYaw(pEdict);

               pBot.b_see_tripmine = FALSE;

               pBot.f_move_speed = 0;  // don't run while turning
            }
         }

         // check if should use wall mounted health station...
         else if (pBot.b_use_health_station)
         {
            if ((pBot.f_use_health_time + 10.0) > gpGlobals->time)
            {
               pBot.f_move_speed = 0;  // don't move while using health station

               pEdict->v.button = IN_USE;
            }
            else
            {
               // bot is stuck trying to "use" a health station...

               pBot.b_use_health_station = FALSE;

               // don't look for items for a while since the bot
               // could be stuck trying to get to an item
               pBot.f_find_item = gpGlobals->time + 0.5;
            }
         }

         // check if should use wall mounted HEV station...
         else if (pBot.b_use_HEV_station)
         {
            if ((pBot.f_use_HEV_time + 10.0) > gpGlobals->time)
            {
               pBot.f_move_speed = 0;  // don't move while using HEV station

               pEdict->v.button = IN_USE;
            }
            else
            {
               // bot is stuck trying to "use" a HEV station...

               pBot.b_use_HEV_station = FALSE;

               // don't look for items for a while since the bot
               // could be stuck trying to get to an item
               pBot.f_find_item = gpGlobals->time + 0.5;
            }
         }

         // check if should capture a point by using it...
         else if (pBot.b_use_capture)
         {
            char teamstr[32];
            UTIL_GetTeam(pEdict, teamstr);  // skin and team must match

            // still capturing and hasn't captured yet...
            if ((pBot.f_use_capture_time > gpGlobals->time) &&
                (pBot.pCaptureEdict->v.skin == (1 - atoi(teamstr))))
            {
               pBot.f_move_speed = 0;  // don't move while capturing

               pEdict->v.button = IN_USE;
            }
            else
            {
               // bot is stuck trying to "use" a capture point...

               pBot.b_use_capture = FALSE;
               pBot.f_use_capture_time = 0.0;

               // don't look for items for a while since the bot
               // could be stuck trying to get to an item
               pBot.f_find_item = gpGlobals->time + 0.5;
            }
         }

         else if (pBot.b_use_button)
         {
            pBot.f_move_speed = 0;  // don't move while using elevator

            BotUseLift( pBot, moved_distance );
         }

         else
         {
            if (pEdict->v.waterlevel == 3)  // check if the bot is underwater...
            {
               BotUnderWater( pBot );
            }

            found_waypoint = FALSE;

            // if the bot is not trying to get to something AND
            // it is time to look for a waypoint AND
            // there are waypoints in this level...

            if ((pBot.pBotPickupItem == NULL) &&
                (pBot.f_look_for_waypoint_time <= gpGlobals->time) &&
                (num_waypoints != 0))
            {
               found_waypoint = BotHeadTowardWaypoint(pBot);
            }

            // check if the bot is on a ladder...
            if (pEdict->v.movetype == MOVETYPE_FLY)
            {
               // check if bot JUST got on the ladder...
               if ((pBot.f_end_use_ladder_time + 1.0) < gpGlobals->time)
                  pBot.f_start_use_ladder_time = gpGlobals->time;

               // go handle the ladder movement
               BotOnLadder( pBot, moved_distance );

               pBot.f_dont_avoid_wall_time = gpGlobals->time + 2.0;
               pBot.f_end_use_ladder_time = gpGlobals->time;
            }
            else
            {
               // check if the bot JUST got off the ladder...
               if ((pBot.f_end_use_ladder_time + 1.0) > gpGlobals->time)
               {
                  pBot.ladder_dir = LADDER_UNKNOWN;
               }
            }

            // if the bot isn't headed toward a waypoint...
            if (found_waypoint == FALSE)
            {
               TraceResult tr;

               // check if we should be avoiding walls
               if (pBot.f_dont_avoid_wall_time <= gpGlobals->time)
               {
                  // let's just randomly wander around
                  if (BotStuckInCorner( pBot ))
                  {
                     pEdict->v.ideal_yaw += 180;  // turn 180 degrees

                     BotFixIdealYaw(pEdict);

                     pBot.f_move_speed /= 3;  // move slowly while turning
                     pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;

                     moved_distance = 2.0;  // dont use bot stuck code
                  }
                  else
                  {
                     if(RANDOM_LONG2(0,1)) {
                        // check if there is a wall on the left first...
                        if (!BotCheckWallOnLeft( pBot ))
                        {
                           HandleWallOnLeft(pBot);
                        }
                        else if (!BotCheckWallOnRight( pBot ))
                        {
                           HandleWallOnRight(pBot);
                        }
                     }
                     else {
                        // check if there is a wall on the right first...
                        if (!BotCheckWallOnRight( pBot ))
                        {
                           HandleWallOnRight(pBot);
                        }
                        else if (!BotCheckWallOnLeft( pBot ))
                        {
                           HandleWallOnLeft(pBot);
                        }
                     }
                  }
               }

               // check if bot is about to hit a wall.  TraceResult gets returned
               if ((pBot.f_dont_avoid_wall_time <= gpGlobals->time) &&
                   BotCantMoveForward( pBot, &tr ))
               {
                  // ADD LATER
                  // need to check if bot can jump up or duck under here...
                  // ADD LATER

                  BotTurnAtWall( pBot, &tr, TRUE );
               }
            }

            // check if bot is on a ladder and has been on a ladder for
            // more than 5 seconds...
            if ((pEdict->v.movetype == MOVETYPE_FLY) &&
                (pBot.f_start_use_ladder_time > 0.0) &&
                ((pBot.f_start_use_ladder_time + 5.0) <= gpGlobals->time))
            {
               // bot is stuck on a ladder...

               BotRandomTurn(pBot);

               // don't look for items for 2 seconds
               pBot.f_find_item = gpGlobals->time + 2.0;

               pBot.f_start_use_ladder_time = 0.0;  // reset start ladder time
            }

            // check if the bot hasn't moved much since the last location
            // (and NOT on a ladder since ladder stuck handled elsewhere)
            if ((moved_distance <= 1.0) && (pBot.f_prev_speed >= 1.0) &&
                (pEdict->v.movetype != MOVETYPE_FLY))
            {
               // the bot must be stuck!

               pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
               pBot.f_look_for_waypoint_time = gpGlobals->time + 1.0;

               if (BotCanJumpUp( pBot, &bCrouchJump ))  // can the bot jump onto something?
               {
                  if ((pBot.f_jump_time + 2.0) <= gpGlobals->time)
                  {
                     pBot.f_jump_time = gpGlobals->time;
                     pEdict->v.button |= IN_JUMP;  // jump up and move forward

                     if (bCrouchJump)
                        pEdict->v.button |= IN_DUCK;  // also need to duck
                  }
                  else
                  {
                     // bot already tried jumping less than two seconds ago, just turn
                     BotRandomTurn(pBot);
                  }
               }
               else if (BotCanDuckUnder( pBot ))  // can the bot duck under something?
               {
                  pEdict->v.button |= IN_DUCK;  // duck down and move forward
               }
               else
               {
                  BotRandomTurn(pBot);

                  // is the bot trying to get to an item?...
                  if (pBot.pBotPickupItem != NULL)
                  {
                     // don't look for items for a while since the bot
                     // could be stuck trying to get to an item
                     pBot.f_find_item = gpGlobals->time + 0.5;
                  }
               }
            }

            // should the bot pause for a while here?
            // (don't pause on ladders or while being "used"...
            if ((RANDOM_LONG2(1, 1000) <= skill_settings[pBot.bot_skill].pause_frequency) &&
                (pEdict->v.movetype != MOVETYPE_FLY) &&
                (pBot.pBotUser == NULL))
            {
               // set the time that the bot will stop "pausing"
               pBot.f_pause_time = gpGlobals->time +
                  RANDOM_FLOAT2(skill_settings[pBot.bot_skill].pause_time[0],
                               skill_settings[pBot.bot_skill].pause_time[1]);
            }
         }
      }
   
      // if has zoom weapon and zooming click off zoom
      if(pBot.pBotEnemy == NULL && 
         pBot.current_weapon_index != -1 && 
         weapon_select[pBot.current_weapon_index].type == WEAPON_FIRE_ZOOM && 
         pEdict->v.fov != 0 &&
         !(pEdict->v.button & (IN_ATTACK|IN_ATTACK2)))
      {
         pEdict->v.button |= IN_ATTACK2;
      }
   }

   if (pBot.curr_waypoint_index != -1)  // does the bot have a waypoint?
   {
      // check if the next waypoint is a door waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_DOOR)
      {
         pBot.f_move_speed = pBot.f_max_speed / 3;  // slow down for doors
      }

      // check if the next waypoint is a ladder waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_LADDER)
      {
         // check if the waypoint is at the top of a ladder AND
         // the bot isn't currenly on a ladder...
         if ((pBot.waypoint_top_of_ladder) &&
             (pEdict->v.movetype != MOVETYPE_FLY))
         {
            // is the bot on "ground" above the ladder?
            if (pEdict->v.flags & FL_ONGROUND)
            {
               float waypoint_distance = (pEdict->v.origin - pBot.waypoint_origin).Length();

               if (waypoint_distance <= 20.0)  // if VERY close...
                  pBot.f_move_speed = 20.0;  // go VERY slow
               else if (waypoint_distance <= 100.0)  // if fairly close...
                  pBot.f_move_speed = 50.0;  // go fairly slow

               pBot.ladder_dir = LADDER_DOWN;
            }
            else  // bot must be in mid-air, go BACKWARDS to touch ladder...
            {
               pBot.f_move_speed = -pBot.f_max_speed;
            }
         }
         else
         {
            // don't avoid walls for a while
            pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;

            pBot.waypoint_top_of_ladder = FALSE;
         }
      }

      // check if the next waypoint is a crouch waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_CROUCH)
         pEdict->v.button |= IN_DUCK;  // duck down while moving forward

      // check if the waypoint is a sniper waypoint AND
      // bot isn't currently aiming at an ememy...
      if ((waypoints[pBot.curr_waypoint_index].flags & W_FL_SNIPER) && (pBot.pBotEnemy == NULL))
      {
         {
            // check if the bot need to move back closer to the waypoint...

            float distance = (pEdict->v.origin - waypoints[pBot.curr_waypoint_index].origin).Length();

            if (distance > 40)
            {
               // turn towards the sniper waypoint and move there...

               Vector v_direction = waypoints[pBot.curr_waypoint_index].origin - pEdict->v.origin;

               Vector bot_angles = UTIL_VecToAngles( v_direction );

               pEdict->v.ideal_yaw = bot_angles.y;

               BotFixIdealYaw(pEdict);

               // go slow to prevent the "loop the loop" problem...
               pBot.f_move_speed = pBot.f_max_speed / 3;

               pBot.f_sniper_aim_time = 0.0;  // reset aiming time
               
               // save the previous speed (for checking if stuck)
               pBot.f_prev_speed = pBot.f_move_speed;

               f_strafe_speed = 0.0;
               
               BotAimPre(pBot);
               g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot.f_move_speed,
                                            f_strafe_speed, 0, pEdict->v.button, 0, (byte)pBot.msecval);
               BotAimPost(pBot);
               
               return;
            }

            // check if it's time to adjust aim yet...
            if (pBot.f_sniper_aim_time <= gpGlobals->time)
            {
               int aim_index;

               aim_index = WaypointFindNearestAiming(waypoints[pBot.curr_waypoint_index].origin);

               if (aim_index != -1)
               {
                  Vector v_aim = waypoints[aim_index].origin - waypoints[pBot.curr_waypoint_index].origin;

                  Vector aim_angles = UTIL_VecToAngles( v_aim );

                  aim_angles.y += RANDOM_LONG2(0, 30) - 15;

                  pEdict->v.ideal_yaw = aim_angles.y;

                  BotFixIdealYaw(pEdict);
               }

               // don't adjust aim again until after a few seconds...
               pBot.f_sniper_aim_time = gpGlobals->time + RANDOM_FLOAT2(3.0, 5.0);
            }
         }
      }
   }
   
   // don't go too close to enemy
   // strafe instead
   if(pBot.pBotEnemy != NULL && 
      FInViewCone(pBot.pBotEnemy->v.origin, pEdict) && 
      (pBot.pBotEnemy->v.origin - pEdict->v.origin).Length() < pBot.current_opt_distance *
       RANDOM_FLOAT2((1000-skill_settings[pBot.bot_skill].keep_optimal_distance) / -1000.0, 
                     (1000-skill_settings[pBot.bot_skill].keep_optimal_distance) / 1000.0))
   {
      if(RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].battle_strafe) 
      {
         if(pBot.f_strafe_time <= gpGlobals->time) {
            pBot.f_strafe_time = gpGlobals->time + RANDOM_FLOAT2(1.0, 2.0);
            pBot.f_strafe_direction = (RANDOM_LONG2(1, 100) <= 50) ? 1.0 : -1.0;
         }
         
         pBot.f_move_speed = 1.0;
      }
      else
      {
         pBot.f_move_speed = 32.0;
      }
   }
   else if (pBot.f_strafe_time < gpGlobals->time)  // time to strafe yet?
   {
      pBot.f_strafe_time = gpGlobals->time + RANDOM_FLOAT2(0.1, 1.0);

      if (RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].normal_strafe)
      {
         if (RANDOM_LONG2(1, 100) <= 50)
            pBot.f_strafe_direction = -1.0;
         else
            pBot.f_strafe_direction = 1.0;
      }
      else
         pBot.f_strafe_direction = 0.0 * RANDOM_FLOAT2(-0.005, 0.005);
   }
        
   if (pBot.f_duck_time > gpGlobals->time)
      pEdict->v.button |= IN_DUCK;  // need to duck (crowbar attack, and random combat ducking)
   
   if (pBot.f_grenade_search_time <= gpGlobals->time)
   {
      pBot.f_grenade_search_time = gpGlobals->time + 0.1;

      // does the bot see any grenades laying/flying about?
      if (BotLookForGrenades( pBot ))
         pBot.f_grenade_found_time = gpGlobals->time;
   }

   if (pBot.f_grenade_found_time + 1.0 > gpGlobals->time)
   {
      // move backwards for 1.0 second after seeing a grenade...
      if(pBot.f_move_speed <= 0.1)
         pBot.f_move_speed = (-2.0/3.0) * pBot.f_max_speed;
      else
         pBot.f_move_speed = -pBot.f_move_speed;
   }

   // is the bot "paused"? or longjumping?
   if (pBot.f_pause_time > gpGlobals->time || pBot.f_longjump_time > gpGlobals->time) 
      pBot.f_move_speed = pBot.f_strafe_direction = 0;  // don't move while pausing
  
   BotDoRandomJumpingAndDuckingAndLongJumping(pBot, moved_distance);
  
   // save the previous speed (for checking if stuck)
   pBot.f_prev_speed = pBot.f_move_speed;
   
   // full strafe if bot isn't moving much else
   if(pBot.f_move_speed <= 1.0)
      f_strafe_speed = pBot.f_strafe_direction * pBot.f_max_speed;
   else
      f_strafe_speed = pBot.f_strafe_direction * pBot.f_move_speed;
   
   // normalize strafe and move to keep speed at normal level
   if(f_strafe_speed != 0.0 || pBot.f_move_speed != 0.0) 
   {
      Vector2D calc;
      
      calc.x = f_strafe_speed;
      calc.y = pBot.f_move_speed;
      
      calc = calc.Normalize() * pBot.f_max_speed;
      
      f_strafe_speed = calc.x;
      pBot.f_move_speed = calc.y;
   }
              
   BotAimPre(pBot);
   g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot.f_move_speed,
                                f_strafe_speed, 0, pEdict->v.button, 0, (byte)pBot.msecval);
   BotAimPost(pBot);
   
   return;
}

void BotDoRandomJumpingAndDuckingAndLongJumping(bot_t &pBot, float moved_distance)
{
   edict_t *pEdict = pBot.pEdict;
   float max_lj_distance = 0.0, target_distance;

   // 
   // PART1 -- handle actions that are not finished
   // 

   // fix bot animation on longjumping
   if (pBot.b_longjump_do_jump)
   {
      pBot.b_longjump_do_jump = FALSE;
      pEdict->v.button |= IN_DUCK;
      pEdict->v.button |= IN_JUMP;
      
      pBot.f_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.3, 0.5); //stay duck for some time to fix animation
      
      return;
   }

   // duck in mid-air after random jump!
   if (pBot.f_random_jump_duck_time > 0.0 && pBot.f_random_jump_duck_time <= gpGlobals->time) 
   {
       pEdict->v.button |= IN_DUCK;  // need to duck (random duckjump)
       
       if(pBot.f_random_jump_duck_end > 0.0 && pBot.f_random_jump_duck_end <= gpGlobals->time) 
       {
          pBot.f_random_jump_duck_time = 0.0;
          pBot.f_random_jump_duck_end = 0.0;
          pBot.f_random_jump_time = gpGlobals->time + RANDOM_FLOAT2(0.2, 0.4);
       }
       
       return;
   }

   // stop trying to longjump after half a second
   if (pBot.f_combat_longjump < gpGlobals->time - 0.5 && pBot.b_combat_longjump)
      pBot.b_combat_longjump = FALSE;

   // longjump
   if (pBot.b_longjump && pEdict->v.waterlevel == 0 && (pEdict->v.flags & FL_ONGROUND) == FL_ONGROUND &&
       !FBitSet(pEdict->v.button, IN_DUCK) && !FBitSet(pEdict->v.button, IN_JUMP) &&
       pEdict->v.velocity.Length() > 50 && pBot.b_combat_longjump &&
       fabs(pEdict->v.v_angle.y - pEdict->v.ideal_yaw) <= 10)
   {
      // don't try to move for 1.0 seconds, otherwise the longjump is fucked up	
      pBot.f_longjump_time = gpGlobals->time + 1.0;
      
      // we're done trying to do a longjump
      pBot.b_combat_longjump = FALSE;
      
      // don't do another one for a certain amount of time
      if (RANDOM_LONG2(1,100) > 10)
         pBot.f_combat_longjump = gpGlobals->time + 1.0;
      else
         pBot.f_combat_longjump = gpGlobals->time + RANDOM_FLOAT2(3.0, 5.0);
      
      // actually do the longjump
      pEdict->v.button |= IN_DUCK;
      pBot.b_longjump_do_jump = TRUE;
      
      //UTIL_ConsolePrintf("%s doing longjump! - combat\n", STRING(pEdict->v.netname));
      return;
   }

   //
   // PART2 -- return here if actions unfinished
   //
   
   // random jump or duck or longjump?
   if (pBot.f_random_jump_time > gpGlobals->time || 
       pBot.f_random_duck_time > gpGlobals->time ||
       pBot.f_combat_longjump > gpGlobals->time ||
       pBot.b_combat_longjump) 
   {
      return;
   }
   
   // Already jumping or ducking, or not on ground, or on ladder
   if(FBitSet(pEdict->v.button, IN_DUCK) || FBitSet(pEdict->v.button, IN_JUMP) ||
       !FBitSet(pEdict->v.flags, FL_ONGROUND) || pEdict->v.movetype == MOVETYPE_FLY)
   {
      return;
   }
   
   //
   // PART3 -- start new actions, first check if contitions are right, then select, then act
   //
   
   qboolean jump, duck, lj;
   
   // if in combat mode jump more
   jump = (pBot.prev_random_type != 1 && pBot.f_random_jump_time <= gpGlobals->time &&
           (moved_distance >= 10.0 || pBot.pBotEnemy != NULL) && pBot.f_move_speed > 1 &&
            RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_jump_frequency);
   
   duck = (pBot.prev_random_type != 2 && pBot.f_random_duck_time <= gpGlobals->time &&
           pBot.pBotEnemy != NULL && 
           RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_duck_frequency);
   
   lj = (pBot.prev_random_type != 3 && pBot.b_longjump && pBot.f_combat_longjump <= gpGlobals->time && !pBot.b_combat_longjump && 
         skill_settings[pBot.bot_skill].can_longjump &&
         pBot.pBotEnemy != NULL && fabs(pEdict->v.v_angle.y - pEdict->v.ideal_yaw) <= 30 &&
         RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_longjump_frequency);
   
   if(lj)
   {
      max_lj_distance = LONGJUMP_DISTANCE * (800 / CVAR_GET_FLOAT("sv_gravity"));
      target_distance = (pBot.pBotEnemy->v.origin - pBot.pEdict->v.origin).Length();
      
      lj = (target_distance > 128 && target_distance < max_lj_distance);
   }
   
   // ofcourse if we get multiple matches here we need to select
   if(lj)
   {
      // lj happens so rarely that it overrides others
      jump = FALSE;
      duck = FALSE;
   }
   else if(jump || duck)
   {
      if(jump && duck)
      {
         jump = RANDOM_FLOAT2(0, 100) <= 50.0;
         duck = !jump;
      }
   }
   else if(!jump && !duck && !lj)
   {
      switch(pBot.prev_random_type)
      {
      case 1:
         pBot.f_random_jump_time = gpGlobals->time + RANDOM_FLOAT2(0.3,0.5);
         break;
      case 2:
         pBot.f_random_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.3,0.5);
         break;
      case 3:
         pBot.f_combat_longjump = gpGlobals->time + RANDOM_FLOAT2(0.3,0.5);
         break;
      default:
         break;
      }
      
      pBot.prev_random_type = 0;
      return;
   }
   
   // if in combat mode jump more
   if(jump)
   {
      pEdict->v.button |= IN_JUMP;
      
      pBot.prev_random_type = 1;
      //UTIL_ConsolePrintf("%s - Random jump\n", STRING(pBot.pEdict->v.netname));
      
      // duck mid-air?
      if(RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_jump_duck_frequency)
      {
         //UTIL_ConsolePrintf("%s - Random duck-jump\n", STRING(pBot.pEdict->v.netname));
         pBot.f_random_jump_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.15, 0.25);
         pBot.f_random_jump_duck_end = pBot.f_random_jump_duck_time + RANDOM_FLOAT2(0.3, 0.5);
         pBot.f_random_jump_time = pBot.f_random_jump_duck_end + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      }
      else
         pBot.f_random_jump_time = pBot.f_random_jump_duck_end + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      
      return;
   }
   
   // combat mode random ducking
   if(duck)
   {
      pEdict->v.button |= IN_DUCK;
      
      pBot.prev_random_type = 2;
      //UTIL_ConsolePrintf("%s - Random duck\n", STRING(pBot.pEdict->v.netname));
      
      pBot.f_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.3, 0.45);
      pBot.f_random_duck_time = pBot.f_duck_time + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      
      return;
   }
   
   // combat mode random longjumping
   if(lj)
   {
      Vector vecSrc, target_angle;
      TraceResult tr;
      int mod;
         
      vecSrc = pEdict->v.origin;
      mod = RANDOM_LONG2(1, 100) <= 50 ? -1 : 1;
      
      // get a random angle (-30 or 30)
      for (int i = 1; i >= -1; i-=2)
      {
         target_angle.x = -pBot.pEdict->v.v_angle.x;
         target_angle.y = UTIL_WrapAngle(pBot.pEdict->v.v_angle.y + 30 * (mod * i));
         target_angle.z = pBot.pEdict->v.v_angle.z;

         // trace a hull toward the current waypoint the distance of a longjump (depending on gravity)
         UTIL_TraceMove(vecSrc, vecSrc + UTIL_AnglesToForward(target_angle) * max_lj_distance, dont_ignore_monsters, pEdict, &tr);
            
         // make sure it's clear
         if (tr.flFraction >= 1.0)
         {
            //UTIL_ConsolePrintf("%s - Clear longjump path found\n", STRING(pBot.pEdict->v.netname));
            pBot.b_combat_longjump = TRUE;
               
            pEdict->v.ideal_yaw += 30 * mod;
               
            BotFixIdealYaw(pEdict);
               
            break;
         }
      }
         
      pBot.f_combat_longjump = gpGlobals->time + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
         
      if(pBot.b_combat_longjump)
      {
         pBot.prev_random_type = 3;
         return;
      }
   }
}

