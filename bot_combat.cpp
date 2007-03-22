//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_combat.cpp
//

#define BOTCOMBAT

#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"


extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern qboolean b_observer_mode;
extern qboolean is_team_play;
extern qboolean checked_teamplay;
extern int num_logos;
extern int submod_id;

float react_delay_min[3][5] = {
   {0.01, 0.02, 0.03, 0.04, 0.05},
   {0.07, 0.09, 0.12, 0.14, 0.17},
   {0.10, 0.12, 0.15, 0.18, 0.21}};
float react_delay_max[3][5] = {
   {0.04, 0.06, 0.08, 0.10, 0.12},
   {0.11, 0.14, 0.18, 0.21, 0.25},
   {0.15, 0.18, 0.22, 0.25, 0.30}};

float shootcone_values[5][2] = {
   {200.0, 10.0}, {250.0, 15.0}, {300.0, 25.0}, {350.0, 30.0}, {400.0, 30.0}};

float prediction_times[5][2] = {
   {0.150, 0.155}, {0.200, 0.210}, {0.250, 0.265}, {0.300, 0.320}, {0.350, 0.375}};

float turn_skills[5] = { 5.0, 3.0, 2.0, 1.0, 0.5 };


typedef struct posdata_s {
   float time;
   qboolean was_alive;
   Vector origin;
   Vector velocity;
   //Vector accel;
   posdata_s * older;
   posdata_s * newer;
} posdata_t;

posdata_t * pos_latest[32];
posdata_t * pos_oldest[32];

#include "bot_weapon_select.h"

int turn_method = AIM_RACC;
//float turn_skill = 4;
static const qboolean aim_fix = TRUE;

//
qboolean BotAimsAtSomething (bot_t &pBot)
{
   if(!pBot.pBotEnemy)
      return FALSE;
   
   float prediction_ammount = RANDOM_FLOAT2(prediction_times[pBot.bot_skill][0], prediction_times[pBot.bot_skill][1]);
   float shootcone_width    = shootcone_values[pBot.bot_skill][0];
   float shootcone_minangle = shootcone_values[pBot.bot_skill][1];
   
   if(FInShootCone(pBot.pBotEnemy->v.origin, pBot.pEdict, 
                   (GetPredictedPlayerPosition(pBot.pBotEnemy, gpGlobals->time - prediction_ammount) - pBot.pEdict->v.origin).Length(), shootcone_width, shootcone_minangle))
      return TRUE;
   
   return FALSE;
}

//
void BotPointGun (bot_t &pBot, qboolean save_v_angle)
{
   // this function is called every frame for every bot. Its purpose is to make the bot
   // move its crosshair to the direction where it wants to look. There is some kind of
   // filtering for the view, to make it human-like.
   edict_t *pEdict = pBot.pEdict;

   float speed; // speed : 0.1 - 1
   Vector v_deviation;
   float turn_skill = turn_skills[pBot.bot_skill];

   v_deviation = UTIL_WrapAngles (Vector (pEdict->v.idealpitch, pEdict->v.ideal_yaw, 0) - pBot.bot_v_angle);

   // do we want old RACC style aiming ?
   if (turn_method == AIM_RACC_OLD)
   {
      // limit the turn skill
      speed = (turn_skill * 3 < 1 / pBot.f_frame_time ? turn_skill * 3 : 1 / pBot.f_frame_time);

      // offset yaw and pitch at some percentage of the deviation, based on skill and frame time.
      pEdict->v.yaw_speed = v_deviation.y * pBot.f_frame_time * speed;
      pEdict->v.pitch_speed = v_deviation.x * pBot.f_frame_time * speed;
   }

   // else do we want new RACC style aiming ?
   else if (turn_method == AIM_RACC)
   {
      // if bot is aiming at something, aim fast, else take our time...
      if (BotAimsAtSomething (pBot))
         speed = 0.7 + (turn_skill - 1) / 10; // fast aim
      else
         speed = 0.2 + (turn_skill - 1) / 20; // slow aim

      // thanks Tobias "Killaruna" Heimann and Johannes "@$3.1415rin" Lampel for this one
      pEdict->v.yaw_speed = (pEdict->v.yaw_speed * exp (log (speed / 2) * pBot.f_frame_time * 20)
                             + speed * v_deviation.y * (1 - exp (log (speed / 2) * pBot.f_frame_time * 20)))
                            * pBot.f_frame_time * 20;
      pEdict->v.pitch_speed = (pEdict->v.pitch_speed * exp (log (speed / 2) * pBot.f_frame_time * 20)
                               + speed * v_deviation.x * (1 - exp (log (speed / 2) * pBot.f_frame_time * 20)))
                              * pBot.f_frame_time * 20;

      // influence of y movement on x axis, based on skill (less influence than x on y since it's
      // easier and more natural for the bot to "move its mouse" horizontally than vertically)
      if (pEdict->v.pitch_speed > 0)
         pEdict->v.pitch_speed += pEdict->v.yaw_speed / (1.5 * (1 + turn_skill));
      else
         pEdict->v.pitch_speed -= pEdict->v.yaw_speed / (1.5 * (1 + turn_skill));

      // influence of x movement on y axis, based on skill
      if (pEdict->v.yaw_speed > 0)
         pEdict->v.yaw_speed += pEdict->v.pitch_speed / (1 + turn_skill);
      else
         pEdict->v.yaw_speed -= pEdict->v.pitch_speed / (1 + turn_skill);
   }

   // move the aim cursor
   pEdict->v.v_angle = UTIL_WrapAngles (pBot.bot_v_angle + Vector (pEdict->v.pitch_speed, pEdict->v.yaw_speed, 0)); 

   // set the body angles to point the gun correctly
   pEdict->v.angles.x = UTIL_WrapAngle (pEdict->v.v_angle.x / 3);
   pEdict->v.angles.y = UTIL_WrapAngle (pEdict->v.v_angle.y);
   pEdict->v.angles.z = 0;

   // did the bot maker not get rid of Paulo-La-Frite's "bugfix" bug ?
   if (aim_fix)
      pEdict->v.angles.x = UTIL_WrapAngle (-pEdict->v.angles.x); // if the bug is still here, fix it again
   
   // save angles
   if( save_v_angle )
      pBot.bot_v_angle = UTIL_WrapAngles (pEdict->v.angles);
   
   return;
}

//
void BotAimThink( bot_t &pBot ) {   
   // make bot aim and turn
   BotPointGun(pBot, TRUE); // update and save this bot's view angles
   
   // systematically wrap all player angles to avoid engine freezes
   pBot.pEdict->v.angles = UTIL_WrapAngles(pBot.pEdict->v.angles);
   pBot.pEdict->v.v_angle = UTIL_WrapAngles(pBot.pEdict->v.v_angle);
   pBot.pEdict->v.idealpitch = UTIL_WrapAngle(pBot.pEdict->v.idealpitch);
   pBot.pEdict->v.ideal_yaw = UTIL_WrapAngle(pBot.pEdict->v.ideal_yaw);
}

//
void BotResetReactionTime(bot_t &pBot) {
   if (pBot.reaction_time)
   {
      float react_delay;

      int index = pBot.reaction_time - 1;

      int bot_skill = pBot.bot_skill;

      float delay_min = react_delay_min[index][bot_skill];
      float delay_max = react_delay_max[index][bot_skill];

      react_delay = RANDOM_FLOAT2(delay_min, delay_max);

      pBot.f_reaction_target_time = gpGlobals->time + react_delay;
   }
}

//
void BotCheckTeamplay(void)
{
   float f_team_play = 0.0;

   f_team_play = CVAR_GET_FLOAT("mp_teamplay");  // teamplay enabled?

   if (f_team_play > 0.0)
      is_team_play = TRUE;
   else
      is_team_play = FALSE;

   checked_teamplay = TRUE;
}

// called in clientdisconnect
void free_posdata_list(int idx) {
   posdata_t * next;
   
   next = pos_latest[idx];
   
   while(next) {
      posdata_t * curr = next;
      
      next = curr->older;
      free(curr);
   }
   
   pos_latest[idx] = 0;
   pos_oldest[idx] = 0;
}

//
void add_next_posdata(int idx, edict_t *pEdict) {
   posdata_t * curr_latest = pos_latest[idx];
   
   pos_latest[idx] = (posdata_t*)malloc(sizeof(posdata_t));
   
   if(curr_latest) {
      curr_latest->newer = pos_latest[idx];
   }
   
   pos_latest[idx]->older = curr_latest;
   pos_latest[idx]->newer = 0;
   
   pos_latest[idx]->origin = pEdict->v.origin;
   pos_latest[idx]->velocity = pEdict->v.basevelocity + pEdict->v.velocity;   
   
   pos_latest[idx]->was_alive = !!IsAlive(pEdict);
   
   pos_latest[idx]->time = gpGlobals->time;
   
   if(!pos_oldest[idx])
      pos_oldest[idx] = pos_latest[idx];
}

// remove data older than 500ms
void timetrim_posdata(int idx) {
   posdata_t * list;
   
   if(!(list = pos_oldest[idx]))
      return;
   
   while(list) {
      if(list->time + (prediction_times[5-1][1] + 0.1) <= gpGlobals->time) { //max + 100ms
      	 posdata_t * next = list->newer;
      	 
      	 free(list);
      	 
      	 list = next;
      	 list->older = 0;
      	 pos_oldest[idx] = list;
      }
      else {
         // new enough.. so are all the rest
         break;
      }
   }
   
   if(!pos_oldest[idx]) {
      pos_oldest[idx] = 0;
      pos_latest[idx] = 0;
   }
}

// called every 33ms (30fps) from startframe
void GatherPlayerData(void) {
   for(int i = 0; i < gpGlobals->maxClients; i++) {
      edict_t * pEdict = INDEXENT(i+1);
      
      if(fast_FNullEnt(pEdict) || pEdict->free) {
      	 if(pos_latest[i]) {
      	    free_posdata_list(i);
      	 }
         
      	 continue;
      }
      
      add_next_posdata(i, pEdict);
      timetrim_posdata(i);
   }
}

// used instead of using pBotEnemy->v.origin in aim code.
//  example: GetPredictedPlayerPosition(pBotEnemy, gpGlobals->time - 0.3) // use 300ms old position data
//  if bot's aim lags behind moving target increase value of AHEAD_MULTIPLIER.
#define AHEAD_MULTIPLIER 1.5
Vector GetPredictedPlayerPosition(edict_t * pPlayer, float time) {
   const float globaltime = gpGlobals->time;
   posdata_t * newer;
   posdata_t * older;
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients || !pos_latest[idx] || !pos_oldest[idx])
      return(pPlayer->v.origin);
   
   // find position data slots that are around 'time'
   newer = pos_latest[idx];
   older = 0;
   while(newer) {
      if(newer->time > time) {
         //this is too new for us.. proceed
         newer = newer->older;
         continue;
      }
      if(newer->time == time) {
         return(newer->origin + (fabs(globaltime - time) * AHEAD_MULTIPLIER) * newer->velocity); 
      }
      
      //this time is older than previous..
      older = newer;
      newer = older->newer;
      break;
   }
   
   if(!older) {
      return(pos_oldest[idx]->origin + (fabs(globaltime - time) * AHEAD_MULTIPLIER) * pos_oldest[idx]->velocity); 
   }
   
   if(!newer) {
      static posdata_t newertmp;
      
      newertmp.origin = pPlayer->v.origin;
      newertmp.velocity = pPlayer->v.basevelocity + pPlayer->v.velocity;   
      newertmp.time = gpGlobals->time;  
      newertmp.was_alive = !!IsAlive(pPlayer);
      
      newer = &newertmp;
   }
   
   float newer_diff = fabs(newer->time - time);
   float older_diff = fabs(older->time - time);
   float total_diff = newer_diff + older_diff;
   
   if(total_diff == 0.0) {
      // zero div would crash server.. 
      // zero diff means that both data are from same time
      return(newer->origin + (fabs(globaltime - time) * AHEAD_MULTIPLIER) * newer->velocity); 
   }
   
   // don't mix dead data with alive data
   if(!newer->was_alive && older->was_alive) {
      return(newer->origin + (fabs(globaltime - time) * AHEAD_MULTIPLIER) * newer->velocity); 
   }
   if(!older->was_alive && newer->was_alive) {
      return(older->origin + (fabs(globaltime - time) * AHEAD_MULTIPLIER) * older->velocity); 
   }
   
   // make weighted average
   Vector pred_origin = (older_diff/total_diff) * newer->origin + (newer_diff/total_diff) * older->origin;
   Vector pred_velocity = (older_diff/total_diff) * newer->velocity + (newer_diff/total_diff) * older->velocity;
   
   // use old origin and use old velocity to predict current position
   return(pred_origin + (fabs(globaltime - time) * AHEAD_MULTIPLIER) * pred_velocity);
}

//
qboolean GetPredictedIsAlive(edict_t * pPlayer, float time) {
   posdata_t * newer;
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients || !pos_latest[idx] || !pos_oldest[idx])
      return(!!IsAlive(pPlayer));
   
   // find position data slots that are around 'time'
   newer = pos_latest[idx];
   while(newer) {
      if(newer->time > time) {
         //this is too new for us.. proceed
         newer = newer->older;
         continue;
      }
      if(newer->time == time) {
         return(newer->was_alive);
      }
      
      //this time is older than previous..
      newer = newer->newer;
      break;
   }
   
   if(!newer) {
      return(!!IsAlive(pPlayer));
   }
   
   return(newer->was_alive);
}

//
qboolean HaveSameModels(edict_t * pEnt1, edict_t * pEnt2) {
   char *infobuffer;
   char model_name1[32];
   char model_name2[32];
   
   infobuffer = GET_INFOKEYBUFFER( pEnt1 );
   *model_name1=0; strncat(model_name1, INFOKEY_VALUE (infobuffer, "model"), sizeof(model_name1));
   
   infobuffer = GET_INFOKEYBUFFER( pEnt2 );
   *model_name2=0; strncat(model_name2, INFOKEY_VALUE (infobuffer, "model"), sizeof(model_name2));
   
   return(!stricmp(model_name1, model_name2));
}

edict_t *BotFindEnemy( bot_t &pBot )
{
   const float globaltime = gpGlobals->time;
   edict_t *pent = NULL;
   edict_t *pNewEnemy; 
   float nearestdistance;
   int i;

   edict_t *pEdict = pBot.pEdict;

   if (pBot.pBotEnemy != NULL)  // does the bot already have an enemy?
   {
      // if the enemy is dead?
      // is the enemy dead?, assume bot killed it
      if (!GetPredictedIsAlive(pBot.pBotEnemy, globaltime - prediction_times[pBot.bot_skill][0])) 
      {
         // the enemy is dead, jump for joy about 10% of the time
         if (RANDOM_LONG2(1, 100) <= 10)
            pEdict->v.button |= IN_JUMP;

         // don't have an enemy anymore so null out the pointer...
         pBot.pBotEnemy = NULL;
      }
      else  // enemy is still alive
      {
         Vector vecEnd;
         Vector vecPredEnemy;

         vecPredEnemy = GetPredictedPlayerPosition(pBot.pBotEnemy, globaltime - RANDOM_FLOAT2(prediction_times[pBot.bot_skill][0], prediction_times[pBot.bot_skill][1]));
         vecEnd = vecPredEnemy + pBot.pBotEnemy->v.view_ofs;

         if (!checked_teamplay)  // check for team play...
            BotCheckTeamplay();

         if( FInViewCone( vecEnd, pEdict ) &&
                  FVisible( vecEnd, pEdict ))
         {
            // face the enemy
            Vector v_enemy = vecPredEnemy - pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles( v_enemy );

            pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(pEdict);

            // keep track of when we last saw an enemy
            pBot.f_bot_see_enemy_time = globaltime;

            return (pBot.pBotEnemy);
         }
         else if( (pBot.f_bot_see_enemy_time > 0) && 
                  (pBot.f_bot_see_enemy_time + 2.0 >= globaltime) ) // enemy has gone out of bot's line of sight, remember enemy for 2 sec
         {
            // we remember this enemy.. keep tracking
            
            // face the enemy
            Vector v_enemy = vecPredEnemy - pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles( v_enemy );

            pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(pEdict);

            return (pBot.pBotEnemy);
         }
      }
   }

   pent = NULL;
   pNewEnemy = NULL;
   nearestdistance = 1500;

   pBot.enemy_attack_count = 0;  // don't limit number of attacks

   if (pNewEnemy == NULL)
   {
      edict_t *pMonster = NULL;
      Vector vecEnd;

      nearestdistance = 9999;

      // search the world for monsters...
      while (!fast_FNullEnt (pMonster = UTIL_FindEntityInSphere (pMonster, pEdict->v.origin, 1000)))
      {
         if (!(pMonster->v.flags & FL_MONSTER))
            continue; // discard anything that is not a monster

         if (!IsAlive (pMonster))
            continue; // discard dead or dying monsters

         if (strcmp ("hornet", STRING (pMonster->v.classname)) == 0)
            continue; // skip hornets
         
         vecEnd = pMonster->v.origin + pMonster->v.view_ofs;

         // see if bot can't see the player...
         if (!FInViewCone( vecEnd, pEdict ) ||
             !FVisible( vecEnd, pEdict ))
            continue;

         float distance = (pMonster->v.origin - pEdict->v.origin).Length();
         if (distance < nearestdistance)
         {
            nearestdistance = distance;
            pNewEnemy = pMonster;

            pBot.pBotUser = NULL;  // don't follow user when enemy found
         }
      }

      // search the world for players...
      for (i = 1; i <= gpGlobals->maxClients; i++)
      {
         edict_t *pPlayer = INDEXENT(i);

         // skip invalid players and skip self (i.e. this bot)
         if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict))
         {
            if ((b_observer_mode) && !(pPlayer->v.flags & (FL_FAKECLIENT | FL_THIRDPARTYBOT | FL_PROXY)))
               continue;
            
            // skip this player if not alive (i.e. dead or dying)
            if (!GetPredictedIsAlive(pPlayer, globaltime - prediction_times[pBot.bot_skill][0]))
               continue;

            // don't target teammates
            if(AreTeamMates(pPlayer, pEdict))
               continue;

            vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

            // see if bot can't see or hear the player...
            if (!FInViewCone( vecEnd, pEdict ) ||
                !FVisible( vecEnd, pEdict ) )
               continue;

            float distance = (pPlayer->v.origin - pEdict->v.origin).Length();
            if (distance < nearestdistance)
            {
               nearestdistance = distance;
               pNewEnemy = pPlayer;

               pBot.pBotUser = NULL;  // don't follow user when enemy found
            }
         }
      }
   }

   if (pNewEnemy)
   {
      // face the enemy
      Vector v_enemy = pNewEnemy->v.origin - pEdict->v.origin;
      Vector bot_angles = UTIL_VecToAngles( v_enemy );

      pEdict->v.ideal_yaw = bot_angles.y;

      BotFixIdealYaw(pEdict);

      // keep track of when we last saw an enemy
      pBot.f_bot_see_enemy_time = globaltime;

      BotResetReactionTime(pBot);
   }

   // has the bot NOT seen an ememy for at least 5 seconds (time to reload)?
   if ((pBot.f_bot_see_enemy_time > 0) &&
       ((pBot.f_bot_see_enemy_time + 5.0) <= globaltime))
   {
      pBot.f_bot_see_enemy_time = -1;  // so we won't keep reloading

      pEdict->v.button |= IN_RELOAD;  // press reload button

      // initialize aim tracking angles...
      pBot.f_aim_x_angle_delta = 5.0;
      pBot.f_aim_y_angle_delta = 5.0;
   }

   return (pNewEnemy);
}


//
qboolean BotFireSelectedWeapon(bot_t & pBot, const bot_weapon_select_t *pSelect, const bot_fire_delay_t *pDelay, const int select_index, const int iId, const qboolean use_primary, const qboolean use_secondary)
{
   const float globaltime = gpGlobals->time;
   edict_t *pEdict = pBot.pEdict;

   if (iId == VALVE_WEAPON_CROWBAR)
   {
      // check if bot needs to duck down to hit enemy...
      if (pBot.pBotEnemy->v.origin.z < (pEdict->v.origin.z - 30))
         pBot.f_duck_time = globaltime + 1.0;

      extern int bot_stop;
      if (bot_stop == 2)
         bot_stop = 1;
   }
      
   if (use_primary)
   {
      if (pSelect[select_index].secondary_zooms && pBot.zooming) {
      	 pEdict->v.button |= IN_ATTACK2;  // use secondary attack to zoom out
      	 pBot.zooming = FALSE;
      }
      else
      	 pEdict->v.button |= IN_ATTACK;  // use primary attack

      if (pSelect[select_index].primary_fire_charge)
      {
         pBot.charging_weapon_id = iId;

         // release primary fire after the appropriate delay...
         pBot.f_primary_charging = globaltime +
                        pSelect[select_index].primary_charge_delay;

         pBot.f_shoot_time = globaltime;  // keep charging
      }
      else
      {
         // set next time to shoot
         if (pSelect[select_index].primary_fire_hold)
            pBot.f_shoot_time = globaltime;  // don't let button up
         else
         {
            int skill = pBot.bot_skill;
            float base_delay, min_delay, max_delay;

            base_delay = pDelay[select_index].primary_base_delay;
            min_delay = pDelay[select_index].primary_min_delay[skill];
            max_delay = pDelay[select_index].primary_max_delay[skill];
            
            if(min_delay == 0 && max_delay == 0)
               pBot.f_shoot_time = globaltime + base_delay;
            else
               pBot.f_shoot_time = globaltime + base_delay + RANDOM_FLOAT2(min_delay, max_delay);
         }
      }
   }
   else  // MUST be use_secondary...
   {
      if (pSelect[select_index].secondary_zooms && !pBot.zooming) {
         pEdict->v.button |= IN_ATTACK2;  // use secondary attack to zoom in
         pBot.zooming = TRUE;
      }
      else if (pSelect[select_index].secondary_zooms && pBot.zooming) {
      	 pEdict->v.button |= IN_ATTACK;  // use primary attack
      }
      else
      	 pEdict->v.button |= IN_ATTACK2;  // use secondary attack

      if (pSelect[select_index].secondary_fire_charge)
      {
         pBot.charging_weapon_id = iId;

         // release secondary fire after the appropriate delay...
         pBot.f_secondary_charging = globaltime +
                        pSelect[select_index].secondary_charge_delay;

         pBot.f_shoot_time = globaltime;  // keep charging
      }
      else
      {
         // set next time to shoot
         if (pSelect[select_index].secondary_fire_hold)
            pBot.f_shoot_time = globaltime;  // don't let button up
         else
         {
            int skill = pBot.bot_skill;
            float base_delay, min_delay, max_delay;

            base_delay = pDelay[select_index].secondary_base_delay;
            min_delay = pDelay[select_index].secondary_min_delay[skill];
            max_delay = pDelay[select_index].secondary_max_delay[skill];

            if(min_delay == 0 && max_delay == 0)
               pBot.f_shoot_time = globaltime + base_delay;
            else
               pBot.f_shoot_time = globaltime + base_delay + RANDOM_FLOAT2(min_delay, max_delay);
         }
      }
   }

   return TRUE;  // weapon was fired
}


//
void GetWeaponSelect(bot_weapon_select_t **pSelect, bot_fire_delay_t **pDelay)
{
   *pSelect = &valve_weapon_select[0];
   *pDelay = &valve_fire_delay[0];
}


// specifing a weapon_choice allows you to choose the weapon the bot will
// use (assuming enough ammo exists for that weapon)
// BotFireWeapon will return TRUE if weapon was fired, FALSE otherwise

qboolean BotFireWeapon( const Vector & v_enemy, bot_t &pBot, int weapon_choice)
{
   const float globaltime = gpGlobals->time;
   bot_weapon_select_t *pSelect = NULL;
   bot_fire_delay_t *pDelay = NULL;
   int select_index;
   int iId, weapon_index;
   qboolean use_primary;
   qboolean use_secondary;
   int use_percent;
   int primary_percent;
   qboolean primary_in_range, secondary_in_range;

   edict_t *pEdict = pBot.pEdict;

   float distance = v_enemy.Length();  // how far away is the enemy?

   GetWeaponSelect(&pSelect, &pDelay);

   if (pSelect)
   {
      // are we charging the primary fire?
      if (pBot.f_primary_charging > 0)
      {
         iId = pBot.charging_weapon_id;

         // is it time to fire the charged weapon?
         if (pBot.f_primary_charging <= globaltime)
         {
            // we DON'T set pEdict->v.button here to release the
            // fire button which will fire the charged weapon

            pBot.f_primary_charging = -1;  // -1 means not charging

            // find the correct fire delay for this weapon
            select_index = 0;

            while ((pSelect[select_index].iId) &&
                   (pSelect[select_index].iId != iId))
               select_index++;

            // set next time to shoot
            int skill = pBot.bot_skill;
            float base_delay, min_delay, max_delay;

            base_delay = pDelay[select_index].primary_base_delay;
            min_delay = pDelay[select_index].primary_min_delay[skill];
            max_delay = pDelay[select_index].primary_max_delay[skill];

            pBot.f_shoot_time = globaltime + base_delay +
               RANDOM_FLOAT2(min_delay, max_delay);

            return TRUE;
         }
         else
         {
            pEdict->v.button |= IN_ATTACK;   // charge the weapon
            pBot.f_shoot_time = globaltime;  // keep charging

            return TRUE;
         }
      }

      // are we charging the secondary fire?
      if (pBot.f_secondary_charging > 0)
      {
         iId = pBot.charging_weapon_id;

         // is it time to fire the charged weapon?
         if (pBot.f_secondary_charging <= globaltime)
         {
            // we DON'T set pEdict->v.button here to release the
            // fire button which will fire the charged weapon

            pBot.f_secondary_charging = -1;  // -1 means not charging

            // find the correct fire delay for this weapon
            select_index = 0;

            while ((pSelect[select_index].iId) &&
                   (pSelect[select_index].iId != iId))
               select_index++;

            // set next time to shoot
            int skill = pBot.bot_skill;
            float base_delay, min_delay, max_delay;

            base_delay = pDelay[select_index].secondary_base_delay;
            min_delay = pDelay[select_index].secondary_min_delay[skill];
            max_delay = pDelay[select_index].secondary_max_delay[skill];

            pBot.f_shoot_time = globaltime + base_delay +
               RANDOM_FLOAT2(min_delay, max_delay);

            return TRUE;
         }
         else
         {
            pEdict->v.button |= IN_ATTACK2;  // charge the weapon
            pBot.f_shoot_time = globaltime;  // keep charging

            return TRUE;
         }
      }
      
      // check if we can reuse currently active weapon
      while(pBot.current_weapon_index >= 0) { //'while' instead of 'if' for breaking out
         select_index = pBot.current_weapon_index;
         
         // was a weapon choice specified? (and if so do they NOT match?)
         if ((weapon_choice != 0) &&
             (weapon_choice != pSelect[select_index].iId)) {
            break;
         }
         
         // is the bot NOT carrying this weapon?
         if (!(pBot.pEdict->v.weapons & (1<<pSelect[select_index].iId)))
            break;

         // is the bot NOT skilled enough to use this weapon?
         if ((pBot.bot_skill+1) > pSelect[select_index].skill_level)
         {
            break;
         }

         // is the bot underwater and does this weapon NOT work under water?
         if ((pEdict->v.waterlevel == 3) &&
             !(pSelect[select_index].can_use_underwater))
         {
            break;
         }
         
         iId = pSelect[select_index].iId;
         weapon_index = iId;
         use_primary = FALSE;
         use_secondary = FALSE;
         primary_percent = RANDOM_LONG2(1, 100);

         // is primary percent less than weapon primary percent AND
         // no ammo required for this weapon OR
            // enough ammo available to fire AND
         // the bot is far enough away to use primary fire AND
         // the bot is close enough to the enemy to use primary fire

         primary_in_range = (distance >= pSelect[select_index].primary_min_distance) &&
                            (distance <= pSelect[select_index].primary_max_distance);

         secondary_in_range = (distance >= pSelect[select_index].secondary_min_distance) &&
                              (distance <= pSelect[select_index].secondary_max_distance);

         if (weapon_choice != 0)
         {
            primary_in_range = TRUE;
            secondary_in_range = TRUE;
         }

         if ((primary_percent <= pSelect[select_index].primary_fire_percent) &&
             ((weapon_defs[weapon_index].iAmmo1 == -1) ||
              (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] >=
               pSelect[select_index].min_primary_ammo)) &&
             (primary_in_range))
         {
            use_primary = TRUE;
         }

         // otherwise see if there is enough secondary ammo AND
         // the bot is far enough away to use secondary fire AND
         // the bot is close enough to the enemy to use secondary fire

         else if (((weapon_defs[weapon_index].iAmmo2 == -1 && 
                    !pSelect[select_index].secondary_use_primary_ammo) ||
                   (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo2] >=
                    pSelect[select_index].min_secondary_ammo) ||
                   (pSelect[select_index].secondary_use_primary_ammo &&
                    ((weapon_defs[weapon_index].iAmmo1 == -1) ||
                     (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] >= 
                      pSelect[select_index].min_primary_ammo)))) &&
                  (secondary_in_range))
         {
            use_secondary = TRUE;
            
            // MP5 cannot use secondary if primary is empty
            if(iId == VALVE_WEAPON_MP5 &&
               (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] <
                pSelect[select_index].min_primary_ammo))
            {
               use_secondary = FALSE;
            }
         }
         
         //
         // Try primary again without primary vs secondary percent check
         //
         
         else if (((weapon_defs[weapon_index].iAmmo1 == -1) ||
                  (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] >=
                   pSelect[select_index].min_primary_ammo)) &&
                  (primary_in_range))
         {
            use_primary = TRUE;
         }

         // see if there wasn't enough ammo to fire the weapon...
         if ((use_primary == FALSE) && (use_secondary == FALSE))
         {
            break;
         }

         // select this weapon if it isn't already selected
         if (pBot.current_weapon.iId != iId)
         {
            UTIL_SelectItem(pEdict, (char*)pSelect[select_index].weapon_name);
            pBot.zooming = FALSE;
         }
         
         if (pDelay[select_index].iId != iId)
         {
            char msg[80];
            snprintf(msg, sizeof(msg), "fire_delay mismatch for weapon id=%d\n",iId);
            ALERT(at_console, "%s", msg);
            
            pBot.current_weapon_index = -1;
            
            return FALSE;
         }
         
         pBot.current_weapon_index = select_index;
         
         return(BotFireSelectedWeapon(pBot, pSelect, pDelay, select_index, iId, use_primary, use_secondary));
      }
      
      select_index = 0;

      // loop through all the weapons until terminator is found...
      while (pSelect[select_index].iId)
      {
      	 // skip currently selected weapon.. it wasn't good enough earlier so it isn't that now either
      	 if(pBot.current_weapon_index >= 0 && pBot.current_weapon_index == select_index)
      	 {
            select_index++;  // skip to next weapon
            pBot.current_weapon_index = -1; // forget current weapon
            continue;
         }
      	 
      	 // severians doesn't like egon
      	 if (pSelect[select_index].not_on_severians && submod_id == SUBMOD_SEVS)
         {
            select_index++;  // skip to next weapon
            continue;
         }
         
         // was a weapon choice specified? (and if so do they NOT match?)
         if ((weapon_choice != 0) &&
             (weapon_choice != pSelect[select_index].iId))
         {
            select_index++;  // skip to next weapon
            continue;
         }

         // is the bot NOT carrying this weapon?
         if (!(pBot.pEdict->v.weapons & (1<<pSelect[select_index].iId)))
         {
            select_index++;  // skip to next weapon
            continue;
         }

         // is the bot NOT skilled enough to use this weapon?
         if ((pBot.bot_skill+1) > pSelect[select_index].skill_level)
         {
            select_index++;  // skip to next weapon
            continue;
         }

         // is the bot underwater and does this weapon NOT work under water?
         if ((pEdict->v.waterlevel == 3) &&
             !(pSelect[select_index].can_use_underwater))
         {
            select_index++;  // skip to next weapon
            continue;
         }

         use_percent = RANDOM_LONG2(1, 100);

         // is use percent greater than weapon use percent?
         if (use_percent > pSelect[select_index].use_percent)
         {
            select_index++;  // skip to next weapon
            continue;
         }

         iId = pSelect[select_index].iId;
         weapon_index = iId;
         use_primary = FALSE;
         use_secondary = FALSE;
         primary_percent = RANDOM_LONG2(1, 100);

         // is primary percent less than weapon primary percent AND
         // no ammo required for this weapon OR
            // enough ammo available to fire AND
         // the bot is far enough away to use primary fire AND
         // the bot is close enough to the enemy to use primary fire

         primary_in_range = (distance >= pSelect[select_index].primary_min_distance) &&
                            (distance <= pSelect[select_index].primary_max_distance);

         secondary_in_range = (distance >= pSelect[select_index].secondary_min_distance) &&
                              (distance <= pSelect[select_index].secondary_max_distance);

         if (weapon_choice != 0)
         {
            primary_in_range = TRUE;
            secondary_in_range = TRUE;
         }

         if ((primary_percent <= pSelect[select_index].primary_fire_percent) &&
             ((weapon_defs[weapon_index].iAmmo1 == -1) ||
              (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] >=
               pSelect[select_index].min_primary_ammo)) &&
             (primary_in_range))
         {
            use_primary = TRUE;
         }

         // otherwise see if there is enough secondary ammo AND
         // the bot is far enough away to use secondary fire AND
         // the bot is close enough to the enemy to use secondary fire

         else if (((weapon_defs[weapon_index].iAmmo2 == -1 && 
                    !pSelect[select_index].secondary_use_primary_ammo) ||
                   (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo2] >=
                    pSelect[select_index].min_secondary_ammo) ||
                   (pSelect[select_index].secondary_use_primary_ammo &&
                    ((weapon_defs[weapon_index].iAmmo1 == -1) ||
                     (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] >= 
                      pSelect[select_index].min_primary_ammo)))) &&
                  (secondary_in_range))
         {
            use_secondary = TRUE;
            
            // MP5 cannot use secondary if primary is empty
            if(iId == VALVE_WEAPON_MP5 &&
               (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] <
                pSelect[select_index].min_primary_ammo))
            {
               use_secondary = FALSE;
            }
         }
         
         //
         // Try primary again without primary vs secondary percent check
         //
         
         else if (((weapon_defs[weapon_index].iAmmo1 == -1) ||
                  (pBot.m_rgAmmo[weapon_defs[weapon_index].iAmmo1] >=
                   pSelect[select_index].min_primary_ammo)) &&
                  (primary_in_range))
         {
            use_primary = TRUE;
         }
         
         // see if there wasn't enough ammo to fire the weapon...
         if ((use_primary == FALSE) && (use_secondary == FALSE))
         {
            select_index++;  // skip to next weapon
            continue;
         }

         // select this weapon if it isn't already selected
         if (pBot.current_weapon.iId != iId)
         {
            UTIL_SelectItem(pEdict, (char*)pSelect[select_index].weapon_name);
            pBot.zooming = FALSE;
         }
         
         if (pDelay[select_index].iId != iId)
         {
            char msg[80];
            snprintf(msg, sizeof(msg), "fire_delay mismatch for weapon id=%d\n",iId);
            ALERT(at_console, "%s", msg);
            
            pBot.current_weapon_index = -1;
            
            return FALSE;
         }
         
         pBot.current_weapon_index = select_index;
         
         return(BotFireSelectedWeapon(pBot, pSelect, pDelay, select_index, iId, use_primary, use_secondary));
      }
   }

   // didn't have any available weapons or ammo, return FALSE
   return FALSE;
}


qboolean AreTeamMates(edict_t * pOther, edict_t * pEdict) {
   if (!checked_teamplay)  // check for team play...
      BotCheckTeamplay();

   // is team play enabled?
   if (is_team_play)
   {
      char other_model[32];
      char edict_model[32];
      
      return(!stricmp(UTIL_GetTeam(pOther, other_model), UTIL_GetTeam(pEdict, edict_model)));
   }
   
   return FALSE;
}


void BotShootAtEnemy( bot_t &pBot )
{
   const float globaltime = gpGlobals->time;
   float f_distance;
   Vector v_enemy;
   Vector v_enemy_aimpos;
   Vector v_predicted_pos;

   edict_t *pEdict = pBot.pEdict;

   if (pBot.f_reaction_target_time > globaltime)
      return;

   v_predicted_pos = GetPredictedPlayerPosition(pBot.pBotEnemy, globaltime - RANDOM_FLOAT2(prediction_times[pBot.bot_skill][0], prediction_times[pBot.bot_skill][1]));

   // do we need to aim at the feet?
   if (pBot.current_weapon.iId == VALVE_WEAPON_RPG)
   {
      Vector v_src, v_dest;
      TraceResult tr;

      v_src = pEdict->v.origin + pEdict->v.view_ofs;  // bot's eyes
      v_dest = pBot.pBotEnemy->v.origin - pBot.pBotEnemy->v.view_ofs;

      UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // can the bot see the enemies feet?

      if ((tr.flFraction >= 1.0) ||
          ((tr.flFraction >= 0.95) &&
           (strcmp("player", STRING(tr.pHit->v.classname)) == 0)))
      {
         // aim at the feet for RPG type weapons
         v_enemy_aimpos = v_predicted_pos - pBot.pBotEnemy->v.view_ofs;
      }
      else
         v_enemy_aimpos = v_predicted_pos + pBot.pBotEnemy->v.view_ofs;
   }
   else
   {
      // aim for the head...
      v_enemy_aimpos = v_predicted_pos + pBot.pBotEnemy->v.view_ofs;
   }
   
   v_enemy = v_enemy_aimpos - GetGunPosition(pEdict);
   
   Vector enemy_angle = UTIL_VecToAngles( v_enemy );

   if (enemy_angle.x > 180)
      enemy_angle.x -=360;

   if (enemy_angle.y > 180)
      enemy_angle.y -=360;

   // adjust the view angle pitch to aim correctly
   enemy_angle.x = -enemy_angle.x;
   
   pEdict->v.idealpitch = enemy_angle.x;
   BotFixIdealPitch(pEdict);

   pEdict->v.ideal_yaw = enemy_angle.y;
   BotFixIdealYaw(pEdict);
   
   v_enemy.z = 0;  // ignore z component (up & down)

   f_distance = v_enemy.Length();  // how far away is the enemy scum?

   if (f_distance > 20)      // run if distance to enemy is far
      pBot.f_move_speed = pBot.f_max_speed;
   else                     // don't move if close enough
      pBot.f_move_speed =10.0;


   // is it time to shoot yet?
   if (pBot.f_shoot_time <= globaltime)
   {
      edict_t * pHit = 0;
      
      // only fire if we can see enemy and if aiming target circle with radius 150.0 
      if((FVisible(v_enemy_aimpos, pEdict, &pHit) || pHit == pBot.pBotEnemy) &&
          FInViewCone(v_enemy_aimpos, pEdict)) 
      {
         float shootcone_width    = shootcone_values[pBot.bot_skill][0];
         float shootcone_minangle = shootcone_values[pBot.bot_skill][1];
   
      	 // check if it is possible to hit target
         if(FInShootCone(v_enemy_aimpos, pEdict, f_distance, shootcone_width, shootcone_minangle)) 
         {
            // select the best weapon to use at this distance and fire...
            if(!BotFireWeapon(v_enemy, pBot, 0))
            {
               pBot.pBotEnemy = NULL;
               pBot.f_bot_find_enemy_time = globaltime + 3.0;
            }
         }
      } 
      else 
      {
         // not visible.. reset reaction times
         BotResetReactionTime(pBot);
      }
   }
}

qboolean BotShootTripmine( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;

   if (pBot.b_shoot_tripmine != TRUE)
      return FALSE;

   // aim at the tripmine and fire the glock...

   Vector v_enemy = pBot.v_tripmine - GetGunPosition( pEdict );

   pEdict->v.v_angle = UTIL_VecToAngles( v_enemy );

   if (pEdict->v.v_angle.y > 180)
      pEdict->v.v_angle.y -=360;

   // Paulo-La-Frite - START bot aiming bug fix
   if (pEdict->v.v_angle.x > 180)
      pEdict->v.v_angle.x -=360;

   // set the body angles to point the gun correctly
   pEdict->v.angles.x = pEdict->v.v_angle.x / 3;
   pEdict->v.angles.y = pEdict->v.v_angle.y;
   pEdict->v.angles.z = 0;

   // adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
   pEdict->v.v_angle.x = -pEdict->v.v_angle.x;
   // Paulo-La-Frite - END

   pEdict->v.ideal_yaw = pEdict->v.v_angle.y;

   BotFixIdealYaw(pEdict);

   return (BotFireWeapon( v_enemy, pBot, VALVE_WEAPON_GLOCK ));
}
