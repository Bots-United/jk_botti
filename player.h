//
// JK_Botti - be more human!
//
// player.h
//

#ifndef PLAYER_H
#define PLAYER_H

#include "posdata_list.h"

typedef struct player_s 
{
   edict_t * pEdict;
   
   //int current_weapon_id; // Was only needed for OP4 grapple detection (old method)

   int last_waypoint;

   float last_time_not_facing_wall;
   float last_time_dead;
   
   posdata_t * position_latest;
   posdata_t * position_oldest;
   posdata_t posdata_mem[POSDATA_SIZE];

} player_t;

extern player_t players[32];

#endif
