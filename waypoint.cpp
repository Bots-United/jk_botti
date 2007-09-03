//
// JK_Botti - be more human!
//
// waypoint.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#ifndef __linux__
#include <io.h>
#endif

#include <fcntl.h>

#ifndef __linux__
#include <sys\stat.h>
#else
#include <string.h>
#include <sys/stat.h>
#endif

#include <malloc.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "waypoint.h"
#include "bot_weapons.h"
#include "bot_weapon_select.h"
#include "player.h"

#include "zlib/zlib.h"


extern int m_spriteTexture;


// waypoints with information bits (flags)
WAYPOINT waypoints[MAX_WAYPOINTS];

// number of waypoints currently in use
int num_waypoints;

// declare the array of head pointers to the path structures...
PATH *paths[MAX_WAYPOINTS];

// time that this waypoint was displayed (while editing)
float wp_display_time[MAX_WAYPOINTS];

// Collect max half of total waypoints
WAYPOINT spawnpoints[MAX_WAYPOINTS / 2];
int num_spawnpoints = 0;

// debuging settings
qboolean g_waypoint_on = FALSE;
qboolean g_path_waypoint = FALSE;

// 
qboolean g_waypoint_paths = FALSE;  // have any paths been allocated?
qboolean g_auto_waypoint = TRUE;
qboolean g_path_waypoint_enable = TRUE;
qboolean g_waypoint_updated = FALSE;
qboolean g_waypoint_testing = FALSE;
float f_path_time = 0.0;
int g_lifts_added = 0;

unsigned int route_num_waypoints;
unsigned short *shortest_path = NULL;
unsigned short *from_to = NULL;
BOOL wp_matrix_initialized = FALSE;
BOOL wp_matrix_save_on_mapend = FALSE;


static Vector * block_list = NULL;
static int block_list_endlist = 0;
static int block_list_size = 0;


void WaypointAddBlock(const Vector &origin)
{
   if(block_list_endlist == block_list_size)
   {
      //resize block_list
      int newsize = block_list_size + 64;
      
      Vector * temp = (Vector *)realloc(block_list, newsize * sizeof(Vector));
      if(!temp) 
      {
         UTIL_ConsolePrintf("Couldn't grow block list");
         return;
      }
      
      block_list = temp;
      block_list_size = newsize;
   }
   
   block_list[block_list_endlist++] = origin;
}


qboolean WaypointInBlockRadius(const Vector &origin)
{
   TraceResult tr;
   
   for(int i = 0; i < block_list_endlist; i++)
   {
      // if distance is enough, don't block
      if((block_list[i] - origin).Length() > 800.0f)
         continue;
      	
      // within block distance check if visible
      UTIL_TraceMove( block_list[i], origin, ignore_monsters, NULL, &tr );
      
      // block visible
      if(tr.fStartSolid || tr.fAllSolid || tr.flFraction >= 1.0f)
         return(TRUE);
   }
   
   return(FALSE);
}


void WaypointDebug(void)
{
   int y = 1, x = 1;

   FILE *fp=fopen("jk_botti.txt","a");
   fprintf(fp,"WaypointDebug: LINKED LIST ERROR!!!\n");
   fclose(fp);

   x = x - 1;  // x is zero
   y = y / x;  // cause an divide by zero exception

   return;
}


inline int GetReachableFlags(int i1, int i2)
{
   return((waypoints[i1].flags | waypoints[i2].flags) & (W_FL_LADDER | W_FL_CROUCH));
}


inline int GetReachableFlags(edict_t * pEntity, int i1)
{
   int flags = waypoints[i1].flags & (W_FL_LADDER | W_FL_CROUCH);
   
   if(FNullEnt(pEntity))
      return(flags);
   
   if(pEntity->v.flags & FL_DUCKING)
      flags |= W_FL_CROUCH;
   
   if(pEntity->v.movetype == MOVETYPE_FLY)
      flags |= W_FL_LADDER;
   
   return(flags);
}


inline int GetReachableFlags(int i1, edict_t * pEntity)
{
   return(GetReachableFlags(pEntity, i1));
}


// free the linked list of waypoint path nodes...
void WaypointFree(void)
{
   for (int i=0; i < MAX_WAYPOINTS; i++)
   {
      if (paths[i])
      {
         PATH *p = paths[i];
         PATH *p_next;

         while (p)  // free the linked list
         {
            p_next = p->next;  // save the link to next
            free(p);
            p = p_next;

#ifdef _DEBUG
            count++;
            if (count > 1000) WaypointDebug();
#endif
         }

         paths[i] = NULL;
      }
   }
}


// initialize the waypoint structures...
void WaypointInit(void)
{
   int i;

   // have any waypoint path nodes been allocated yet?
   if (g_waypoint_paths)
      WaypointFree();  // must free previously allocated path memory

   if (shortest_path != NULL)
      free(shortest_path);

   if (from_to != NULL)
      free(from_to);
   
   shortest_path = NULL;
   from_to = NULL;
   wp_matrix_initialized = FALSE;

   memset(waypoints, 0, sizeof(waypoints));
   
   for (i=0; i < MAX_WAYPOINTS; i++)
   {
      wp_display_time[i] = 0.0;
      paths[i] = NULL;  // no paths allocated yet
   }

   f_path_time = 0.0;  // reset waypoint path display time

   num_waypoints = 0;

   for(i = 0; i < 32; i++)
      players[i].last_waypoint = -1;
   
   memset(&spawnpoints, 0, sizeof(spawnpoints));
   num_spawnpoints = 0;
   
   if(block_list)
      free(block_list);
   
   block_list = NULL;
   block_list_endlist = 0;
   block_list_size = 0;
   
   g_lifts_added = 0;
}


void WaypointAddPath(short int add_index, short int path_index)
{
   PATH *p, *prev;
   int i;

   if((waypoints[add_index].flags | waypoints[path_index].flags) & W_FL_DELETED)
   {
      UTIL_ConsolePrintf("%s", "Tried to add path to deleted waypoint!");
      return;
   }
   
   p = paths[add_index];
   prev = NULL;

   // find an empty slot for new path_index...
   while (p != NULL)
   {
      i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] == -1)
         {
            p->index[i] = path_index;

            return;
         }

         i++;
      }

      prev = p;     // save the previous node in linked list
      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }

   p = (PATH *)calloc(1, sizeof(PATH));

   if (p == NULL)
   {
      UTIL_ConsolePrintf("%s", "Error allocating memory for path!");
   }

   p->index[0] = path_index;
   p->index[1] = -1;
   p->index[2] = -1;
   p->index[3] = -1;
   p->next = NULL;

   if (prev != NULL)
      prev->next = p;  // link new node into existing list

   if (paths[add_index] == NULL)
      paths[add_index] = p;  // save head point if necessary
}


void WaypointDeletePath(short int del_index)
{
   PATH *p;
   int index, i;

   // search all paths for this index...
   for (index=0; index < num_waypoints; index++)
   {
      p = paths[index];

      // search linked list for del_index...
      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] == del_index)
            {
               p->index[i] = -1;  // unassign this path
            }

            i++;
         }

         p = p->next;  // go to next node in linked list

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug();
#endif
      }
   }
}


void WaypointDeletePath(short int path_index, short int del_index)
{
   PATH *p;
   int i;

   p = paths[path_index];

   // search linked list for del_index...
   while (p != NULL)
   {
      i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] == del_index)
         {
            p->index[i] = -1;  // unassign this path
         }

         i++;
      }

      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }
}


// find a path from the current waypoint. (pPath MUST be NULL on the
// initial call. subsequent calls will return other paths if they exist.)
int WaypointFindPath(PATH **pPath, int *path_index, int waypoint_index)
{
   int index;

   if (*pPath == NULL)
   {
      *pPath = paths[waypoint_index];
      *path_index = 0;
   }

   if (*path_index == MAX_PATH_INDEX)
   {
      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list
   }

   while (*pPath != NULL)
   {
      while (*path_index < MAX_PATH_INDEX)
      {
         if ((*pPath)->index[*path_index] != -1)  // found a path?
         {
            // save the return value
            index = (*pPath)->index[*path_index];

            // set up stuff for subsequent calls...
            (*path_index)++;

            return index;
         }

         (*path_index)++;
      }

      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }

   return -1;
}


// calculate all the paths to this waypoint
void WaypointMakePathsWith(int index)
{
   // calculate all the paths to this new waypoint
   if(!g_path_waypoint_enable)
      return;
      	 
   for (int i=0; i < num_waypoints; i++)
   {
      if (i == index)
         continue;  // skip the waypoint that was just added

      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;  // skip any aiming waypoints
 
      // don't allow paths TO lift-end
      if(!(waypoints[i].flags & W_FL_LIFT_END))
      {
         // check if the waypoint is reachable from the new one (one-way)
         if ( WaypointReachable(waypoints[index].origin, waypoints[i].origin, GetReachableFlags(index, i)))
         {
            WaypointAddPath(index, i);
         }
      }

      // don't allow paths TO lift-end
      if(!(waypoints[index].flags & W_FL_LIFT_END))
      {
         // check if the new one is reachable from the waypoint (other way)
         if ( WaypointReachable(waypoints[i].origin, waypoints[index].origin, GetReachableFlags(i, index)))
         {
            WaypointAddPath(i, index);
         }
      }
   }
}


// Called when map-objects spawn
void CollectMapSpawnItems(edict_t *pSpawn)
{
   const char *classname = (const char *)STRING(pSpawn->v.classname);
   int flags = 0;
   int i;
   Vector SpawnOrigin;
   int itemflag = 0;
   static Vector CheckOffset;
   static int wall_count, wall_updated;
   static BOOL offset_set = FALSE;
   static const Vector offsets[8] = { Vector(60, 0, 0), Vector(0, 60, 0), Vector(-60, 0, 0), Vector(0, -60, 0), Vector(42, 42, 0), Vector(42, -42, 0), Vector(-42, 42, 0), Vector(-42, -42, 0) };
   
   if(num_spawnpoints >= MAX_WAYPOINTS / 2)
      return;
      
   Vector EntOrigin = pSpawn->v.origin;
   
   if (strncmp("item_health", classname, 11) == 0)
      flags |= W_FL_HEALTH;
   else if (strncmp("item_battery", classname, 12) == 0)
      flags |= W_FL_ARMOR;
   else if ((itemflag = GetAmmoItemFlag(classname)) != 0)
      flags |= W_FL_AMMO;
   else if ((itemflag = GetWeaponItemFlag(classname)) != 0 && (pSpawn->v.owner == NULL))
      flags |= W_FL_WEAPON;
   else if (strcmp("item_longjump", classname) == 0)
      flags |= W_FL_LONGJUMP;
   else
   {
      // check for func_recharge and func_healthcharger
      if(strncmp("func_recharge", classname, 13) == 0 || strncmp("func_healthcharger", classname, 18) == 0)
      {
         flags |= (classname[5] == 'r') ? W_FL_ARMOR : W_FL_HEALTH;
         
         if(offset_set)
         {
            // check if there is room around charger
            EntOrigin = VecBModelOrigin(pSpawn) + CheckOffset;
         }
         else
         {
            wall_count = 0;
            wall_updated = 0;
            
            // check if there is room around charger
            for(int i = 0; i < 8; i++)
            {
               CheckOffset = offsets[i];
               offset_set = TRUE;
               CollectMapSpawnItems(pSpawn);
               offset_set = FALSE;
            }
            
            //UTIL_ConsolePrintf("Total %d waypoints add around %s and %d waypoints updated with new flags\n", wall_count, classname, wall_updated);
            
            return;
         }
      }
      else
         return;
   }
   
   // Trace item on ground
   TraceResult tr;
   UTIL_TraceHull( EntOrigin + Vector(0, 0, 36 / 2), EntOrigin - Vector( 0, 0, 10240 ), 
      ignore_monsters, head_hull, pSpawn->v.pContainingEntity, &tr );
   
   //stuck in world?
   if(tr.fStartSolid || tr.fAllSolid) 
   {
      //UTIL_ConsolePrintf("stuck in world\n");
      return;
   }
   
   SpawnOrigin = tr.vecEndPos - Vector(0, 0, 36 / 2) + Vector(0, 0, 72 / 2);
   
   // Check if there is enough space to stand
   TraceResult tr2;
   UTIL_TraceHull( SpawnOrigin, SpawnOrigin + Vector(0, 0, 1),
      ignore_monsters, human_hull, pSpawn->v.pContainingEntity, &tr2 );
      
   if(tr2.fStartSolid || tr2.flFraction < 1.0f )
   {
      // Not enough space, make this duck waypoint
      SpawnOrigin = tr.vecEndPos;
      flags |= W_FL_CROUCH;
      
      //UTIL_ConsolePrintf("adding crouch waypoint\n");
   }
   
   // Check if any spawnpoints near already (distance 50.0)
   for(i = 0; i < num_spawnpoints; i++)
   {
      if((flags & W_FL_CROUCH) != (spawnpoints[i].flags & W_FL_CROUCH))
         continue;
      
      if((spawnpoints[i].origin - SpawnOrigin).Length() < 50.0)
      {
         spawnpoints[i].flags |= flags;
         spawnpoints[i].itemflags |= itemflag;
         
         wall_updated++;
         return;
      }
   }
   
   // Add new
   spawnpoints[num_spawnpoints].origin = SpawnOrigin;
   spawnpoints[num_spawnpoints].flags = flags | W_FL_SPAWNADD;
   spawnpoints[num_spawnpoints].itemflags = itemflag;
   num_spawnpoints++;
   
   wall_count++;
}


//
void WaypointAddSpawnObjects(void)
{
   int index;
   int i,j;
   qboolean done;
   int count = 0;
   int flags_updated = 0;
   int itemflags_updated = 0;

   if (!g_auto_waypoint || num_spawnpoints == 0)
      return;

   for(i = 0; i < num_spawnpoints; i++) 
   {
      done = FALSE;
      
      // Check if there is waypoint near already
      for(j = 0; j < num_waypoints; j++) {
         if (!(waypoints[j].flags & (W_FL_DELETED | W_FL_AIMING))) 
         {
            if((waypoints[j].origin - spawnpoints[i].origin).Length() < 50.0) 
            {
               // just add flags and return
               if(waypoints[j].flags != spawnpoints[i].flags)
                  flags_updated++;
               if(waypoints[j].itemflags != spawnpoints[i].itemflags)
                  itemflags_updated++;
               
               waypoints[j].flags |= spawnpoints[i].flags;
               waypoints[j].itemflags |= spawnpoints[i].itemflags;
               done = TRUE;
            }
         }
      }
      
      if(done)
         continue;
      
      index = 0;
      
      // find the next available slot for the new waypoint...
      while (index < num_waypoints)
      {
         if (waypoints[index].flags & W_FL_DELETED)
            break;

         index++;
      }
      
      if (index >= MAX_WAYPOINTS) // out of space
         break;
   
      waypoints[index].flags = spawnpoints[i].flags;
      waypoints[index].itemflags = spawnpoints[i].itemflags;

      // store the origin (location) of this waypoint
      waypoints[index].origin = spawnpoints[i].origin;

      // set the time that this waypoint was originally displayed...
      wp_display_time[index] = 0.0;
      
      // increment total number of waypoints if adding at end of array...
      if (index == num_waypoints)
         num_waypoints++;

      count++;

      // calculate all the paths to this new waypoint
      WaypointMakePathsWith(index);
   }
   
   if(count || flags_updated || itemflags_updated) 
   {
      UTIL_ConsolePrintf("Added total %d map-object based waypoints and updated flags for %d!\n", count, flags_updated + itemflags_updated);
      
      g_waypoint_updated = !!count;
   }
   
   num_spawnpoints = 0;
   memset(&spawnpoints, 0, sizeof(spawnpoints));
   
   if(g_lifts_added || count)
   {
      WaypointSlowFloydsStop();
      
      if (shortest_path != NULL)
         free(shortest_path);

      if (from_to != NULL)
         free(from_to);
   
      shortest_path = NULL;
      from_to = NULL;
      wp_matrix_initialized = FALSE;
      
      WaypointRouteInit(TRUE);
   }
}


//
void WaypointAddLift(edict_t * pent, const Vector &start, const Vector &end)
{
   int index_start, index_end;
   
   if(end.z - start.z < 16.0f)
      return;
   
   TraceResult tr;
   Vector origin = VecBModelOrigin(pent);
   
   Vector move = end - start;
   
   // test if this is trap
   Vector stand_start = Vector(origin.x, origin.y, pent->v.absmax.z + 72/2);
   Vector stand_end = stand_start + move;
   
   UTIL_TraceDuck(stand_start, stand_end, ignore_monsters, NULL, &tr);
   
   if(tr.fAllSolid || tr.fStartSolid || tr.flFraction < 1.0f)
      return;
   
   // Detect if waypoints already exists for this lift
   for(int i = 0; i < num_waypoints; i++)
   {
      if(waypoints[i].flags & W_FL_DELETED)
         continue;
      
      //check start point
      if(waypoints[i].flags & W_FL_LIFT_START)
      {
         if((stand_start - waypoints[i].origin).Length() < 100.0f)
            return;
      }
      
      //check end point
      if(waypoints[i].flags & W_FL_LIFT_END)
      {
         if((stand_end - waypoints[i].origin).Length() < 100.0f)
            return;
      }
   }

   // Get two free waypoint indexes
   index_start = 0;
   index_end = 0;
   
   // find the next available slot for the new waypoint...
   while (index_start < num_waypoints)
   {
      if (waypoints[index_start].flags & W_FL_DELETED)
         break;

      index_start++;
   }
   
   if (index_start >= MAX_WAYPOINTS) // out of space
      return;

   // increment total number of waypoints if adding at end of array...
   if (index_start == num_waypoints)
      num_waypoints++;
   
   // find the next available slot for the new waypoint...
   while (index_end < num_waypoints)
   {
      if ((waypoints[index_end].flags & W_FL_DELETED) && index_end != index_start)
         break;

      index_end++;
   }

   if (index_end >= MAX_WAYPOINTS) // out of space
      return;

   // increment total number of waypoints if adding at end of array...
   if (index_end == num_waypoints)
      num_waypoints++;
   
   // Add waypoints (W_FL_LIFT_START -> W_FL_LIFT_END)
   waypoints[index_start].origin = stand_start;
   waypoints[index_start].flags = W_FL_LIFT_START;
   waypoints[index_start].itemflags = 0;
   
   waypoints[index_end].origin = stand_end;
   waypoints[index_end].flags = W_FL_LIFT_END;
   waypoints[index_end].itemflags = 0;
   
   // Add one way path
   WaypointAddPath(index_start, index_end);
   
   // Make path links with start point
   WaypointMakePathsWith(index_start);
   
   // Move lift to upper position and make path links with end point
   SET_ORIGIN(pent, end);
   WaypointMakePathsWith(index_end);
   SET_ORIGIN(pent, start);
   
   // done
   g_lifts_added++;
   g_waypoint_updated = TRUE;

   if(0)
   {
      UTIL_ConsolePrintf("%s\n", STRING(pent->v.classname));
      UTIL_ConsolePrintf(" - start : %4.1f, %4.1f, %4.1f", start.x, start.y, start.z);
      UTIL_ConsolePrintf(" - end   : %4.1f, %4.1f, %4.1f", end.x, end.y, end.z);
      UTIL_ConsolePrintf(" - move  : %4.1f, %4.1f, %4.1f", move.x, move.y, move.z);
      UTIL_ConsolePrintf(" - origin: %4.1f, %4.1f, %4.1f\n", origin.x, origin.y, origin.z);
   }
}


// find the nearest waypoint to the source postition and return the index
int WaypointFindNearest(const Vector &v_origin, const Vector &v_offset, edict_t *pEntity, float range, qboolean b_traceline)
{
   int index, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 99999.0;

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      distance = (waypoints[index].origin - v_origin).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from source position...
         if(b_traceline)
            UTIL_TraceLine( v_origin + v_offset, waypoints[index].origin, ignore_monsters, pEntity->v.pContainingEntity, &tr );
         else
            UTIL_TraceMove( v_origin + v_offset, waypoints[index].origin, ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0f)
         {
            min_index = index;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


//
int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int itemflags, int exclude[], float range, const Vector *pv_src)
{
   int index, min_index;
   float distance, min_distance;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   
   if(pv_src != NULL)
   {
      JKASSERT(src != -1);
      min_distance = 99999;
   }
   else
      min_distance = WAYPOINT_UNREACHABLE;

   for (index=0; index < num_waypoints; index++)
   {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;  // skip any deleted && aiming waypoints

      if (flags && !(waypoints[index].flags & flags))
         continue;  // skip this waypoint if the flags don't match

      if (itemflags && !(waypoints[index].itemflags & itemflags))
      	 continue;  // skip this weapoint if no match for itemflags
      
      if (exclude != NULL)
      {
         int exclude_index = 0;
         while (exclude[exclude_index])
         {
            if (index == exclude[exclude_index])
               break;  // found a match, break out of while loop

            exclude_index++;
         }
         
         if (index == exclude[exclude_index])
            continue;  // skip any index that matches exclude list
      }

      if (waypoints[index].flags & (W_FL_LONGJUMP | W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }

      if (pv_src != NULL)
         distance = (waypoints[index].origin - *pv_src).Length();
      else
         distance = WaypointDistanceFromTo(src, index);
      
      if (range > 0.0f && distance >= range)
         continue;

      if (distance < min_distance)
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


//
int WaypointFindRandomGoal(int *out_indexes, int max_indexes, edict_t *pEntity, int flags, int itemflags, int exclude[])
{
   int index;
   int * indexes;
   int count = 0;

   if (num_waypoints < 1 || max_indexes <= 0 || out_indexes == NULL)
      return 0;

   indexes = (int*)alloca(sizeof(int)*num_waypoints);

   // find all the waypoints with the matching flags...
   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;  // skip any deleted&aiming waypoints

      if (flags && !(waypoints[index].flags & flags))
         continue;  // skip this waypoint if the flags don't match

      if (itemflags && !(waypoints[index].itemflags & itemflags))
      	 continue;  // skip this weapoint if no match for itemflags
      
      if (exclude != NULL)
      {
         int exclude_index = 0;
         while (exclude[exclude_index])
         {
            if (index == exclude[exclude_index])
               break;  // found a match, break out of while loop

            exclude_index++;
         }
         
         if (index == exclude[exclude_index])
            continue;  // skip any index that matches exclude list
      }
      
      if (waypoints[index].flags & (W_FL_LONGJUMP | W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);   
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }

      if (count < num_waypoints)
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return 0;

   if (count <= max_indexes)
   {
      // simplest case, we just enough or less indexes to give
      for(int i = 0; i < count; i++)
         out_indexes[i] = indexes[i];
      
      return(count);
   }

   // we have extra.. take randomly
   int out_count = 0;
   
   for(int i = 0; i < max_indexes; i++)
      out_indexes[out_count] = indexes[RANDOM_LONG2(0, count - 1)];

   return max_indexes;
}


// find the nearest waypoint to the source postition and return the index
int WaypointFindRunawayPath(int runner, int enemy)
{
   int index, max_index;
   float runner_distance;
   float enemy_distance;
   float max_difference;
   float difference;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   max_difference = 0.0;
   max_index = -1;

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & (W_FL_DELETED|W_FL_AIMING))
         continue;  // skip any deleted&aiming waypoints

      runner_distance = WaypointDistanceFromTo(runner, index);
      enemy_distance = WaypointDistanceFromTo(runner, index);
      
      if(runner_distance == WAYPOINT_MAX_DISTANCE)
         continue;
      
      difference = enemy_distance - runner_distance;
      if(difference > max_difference) 
      {
         max_difference = difference;
         max_index = index;
      }
   }

   return max_index;
}


//
int WaypointFindNearestAiming(Vector v_origin)
{
   int index;
   int min_index = -1;
   float min_distance = 99999.0;
   float distance;

   if (num_waypoints < 1)
      return -1;

   // search for nearby aiming waypoint...
   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (!(waypoints[index].flags & W_FL_AIMING))
         continue;  // skip any NON aiming waypoints

      distance = (v_origin - waypoints[index].origin).Length();

      if ((distance < min_distance) && (distance < 40))
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


//
void WaypointDrawBeam(edict_t *pEntity, const Vector &start, const Vector &end, int width,
        int noise, int red, int green, int blue, int brightness, int speed)
{
   // should waypoints be visible?
   if(!g_waypoint_on)
      return;
   
   MESSAGE_BEGIN(FNullEnt(pEntity)?MSG_ALL:MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE( TE_BEAMPOINTS);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_SHORT( m_spriteTexture );
   WRITE_BYTE( 1 ); // framestart
   WRITE_BYTE( 10 ); // framerate
   WRITE_BYTE( 10 ); // life in 0.1's
   WRITE_BYTE( width ); // width
   WRITE_BYTE( noise );  // noise

   WRITE_BYTE( red );   // r, g, b
   WRITE_BYTE( green );   // r, g, b
   WRITE_BYTE( blue );   // r, g, b

   WRITE_BYTE( brightness );   // brightness
   WRITE_BYTE( speed );    // speed
   MESSAGE_END();
}


//
void WaypointSearchItems(edict_t *pEntity, const Vector &v_origin, int wpt_index)
{
   edict_t *pent = NULL;
   float radius = 40;
   TraceResult tr;
   float distance;
   float min_distance;
   char item_name[64];
   char nearest_name[64];
   edict_t *nearest_pent;
   int itemflag;
   int flags_before;
   int itemflags_before;
   
   flags_before = waypoints[wpt_index].flags;
   itemflags_before = waypoints[wpt_index].itemflags;

   nearest_name[0] = 0;      // null out nearest_name string
   nearest_pent = NULL;

   min_distance = 99999.0;

   //********************************************************
   // look for the nearest health, armor, ammo, weapon, etc.
   //********************************************************

   while ((pent = UTIL_FindEntityInSphere( pent, v_origin, radius )) != NULL)
   {
      UTIL_TraceMove( v_origin, pent->v.origin, ignore_monsters,  
                      pEntity ? pEntity->v.pContainingEntity : NULL, &tr );

      // make sure entity is visible...
      if (tr.flFraction >= 1.0f)
      {
         safevoid_snprintf(item_name, sizeof(item_name), "%s", STRING(pent->v.classname));

         if ((strncmp("item_health", item_name, 11) == 0) || strncmp("func_healthcharger", item_name, 18) == 0 ||
             (strncmp("item_battery", item_name, 12) == 0) || strncmp("func_recharge", item_name, 13) == 0||
             (GetAmmoItemFlag(item_name) != 0) ||
             (strcmp("item_longjump", item_name) == 0) ||
             ((GetWeaponItemFlag(item_name) != 0) && (pent->v.owner == NULL)))
         {
            distance = (pent->v.origin - v_origin).Length();

            if (distance < min_distance)
            {
               safevoid_snprintf(nearest_name, sizeof(nearest_name), "%s", item_name);

               nearest_pent = pent;

               min_distance = distance;
            }
         }
      }
   }

   if (nearest_name[0])  // found an entity name
   {
      char buf[128];
      
      if (strncmp("item_health", nearest_name, 11) == 0 || strncmp("func_healthcharger", nearest_name, 18) == 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs2(buf, sizeof(buf), "found a healthkit! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_HEALTH;
      }

      if (strncmp("item_battery", nearest_name, 12) == 0 || strncmp("func_recharge", nearest_name, 13) == 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs2(buf, sizeof(buf), "found some armor! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_ARMOR;
      }

      if ((itemflag = GetAmmoItemFlag(nearest_name)) != 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs2(buf, sizeof(buf), "found some ammo! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_AMMO;
         waypoints[wpt_index].itemflags |= itemflag;
      }

      if ((itemflag = GetWeaponItemFlag(nearest_name)) != 0 && nearest_pent->v.owner == NULL)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs2(buf, sizeof(buf), "found a weapon! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_WEAPON;
         waypoints[wpt_index].itemflags |= itemflag;
      }
      
      if ((strcmp("item_longjump", nearest_name) == 0))
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs2(buf, sizeof(buf), "found a longjump! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_LONGJUMP;
      }
   }
   
   // don't check flags, they can be rechecked on next mapload
   /*if(flags_before != waypoints[wpt_index].flags || itemflags_before != waypoints[wpt_index].itemflags)
   {  // save on mapchange
      g_waypoint_updated = TRUE;
   }*/
}


edict_t *WaypointFindItem( int wpt_index )
{
   edict_t *pent = NULL;
   float radius = 40;
   TraceResult tr;
   float distance;
   float min_distance;
   char item_name[64];
   char nearest_name[64];
   edict_t *nearest_pent = NULL;
   
   nearest_name[0] = 0;      // null out nearest_name string
   nearest_pent = NULL;
   
   min_distance = 99999.0;
   
   Vector v_origin = waypoints[wpt_index].origin;
   //********************************************************
   // look for the nearest health, armor, ammo, weapon, etc.
   //********************************************************
   while ((pent = UTIL_FindEntityInSphere( pent, v_origin, radius )) != NULL)
   {
      UTIL_TraceMove( v_origin, pent->v.origin, ignore_monsters, NULL, &tr );
      
      // make sure entity is visible...
      if ((tr.flFraction >= 1.0f) || (tr.pHit == pent) || !(pent->v.effects & EF_NODRAW) || !(pent->v.frame > 0))
      {
         safevoid_snprintf(item_name, sizeof(item_name), "%s", STRING(pent->v.classname));
         
         if(((waypoints[wpt_index].flags & W_FL_HEALTH) && (strncmp("item_health", item_name, 11) == 0) || strncmp("func_healthcharger", item_name, 18) == 0) ||
            ((waypoints[wpt_index].flags & W_FL_ARMOR) && (strncmp("item_battery", item_name, 12) == 0) || strncmp("func_recharge", item_name, 13) == 0) ||
            ((waypoints[wpt_index].flags & W_FL_AMMO) && GetAmmoItemFlag(item_name) != 0) ||
            ((waypoints[wpt_index].flags & W_FL_LONGJUMP) && strcmp("item_longjump", item_name) == 0) ||
            ((waypoints[wpt_index].flags & W_FL_WEAPON) && GetWeaponItemFlag(item_name) != 0 && pent->v.owner == NULL))
         {
            distance = (pent->v.origin - v_origin).Length();
            
            if (distance < min_distance)
            {   
               nearest_pent = pent;
               min_distance = distance;
            }
         }
      }
   }

   return nearest_pent;
}


void WaypointAdd(edict_t *pEntity)
{
   int index;
   int player_index = ENTINDEX(pEntity) - 1;
   
   index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         break;

      index++;
   }

   if (index >= MAX_WAYPOINTS)
      return;
   
   // save on mapchange
   g_waypoint_updated = TRUE;

   // ladder check
   waypoints[index].flags = (pEntity->v.movetype == MOVETYPE_FLY) ? W_FL_LADDER : 0;

   waypoints[index].itemflags = 0;

   // store the origin (location) of this waypoint (use entity origin)
   waypoints[index].origin = pEntity->v.origin;

   // store the last used waypoint for the auto waypoint code...
   if(player_index >= 0 && player_index < gpGlobals->maxClients)
      players[player_index].last_waypoint = index;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = gpGlobals->time;


   Vector start, end;

   start = pEntity->v.origin - Vector(0, 0, 34);
   end = start + Vector(0, 0, 68);

   if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING && (pEntity->v.flags & FL_ONGROUND) == FL_ONGROUND)
   {
      TraceResult tr;
      
      Vector v_src = waypoints[index].origin;
      Vector v_dest = waypoints[index].origin;
      
      v_dest.z += 36/2 + 72/2 + 1;
      
      UTIL_TraceMove(v_src, v_dest, ignore_monsters, NULL, &tr);
      
      // no room to stand up
      if (tr.flFraction < 1.0f)
      {
         waypoints[index].flags |= W_FL_CROUCH;  // crouching waypoint

         start = pEntity->v.origin - Vector(0, 0, 17);
         end = start + Vector(0, 0, 34);
      }
      else
      {
         waypoints[index].origin.z += 36/2;
         
         start = waypoints[index].origin - Vector(0, 0, 34);
         end = start + Vector(0, 0, 68);
      }
   }

   if (pEntity->v.movetype == MOVETYPE_FLY)
      waypoints[index].flags |= W_FL_LADDER;  // waypoint on a ladder


   // search the area near the waypoint for items (HEALTH, AMMO, WEAPON, etc.)
   WaypointSearchItems(pEntity, waypoints[index].origin, index);


   // draw a blue waypoint
   WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

   if(g_waypoint_on)
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0,
                   ATTN_NORM, 0, 100);

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;

   // calculate all the paths to this new waypoint
   WaypointMakePathsWith(index);
}


int WaypointAddTesting(const Vector &vecOrigin, int flags, int itemflags, qboolean MakePaths)
{
   int index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         break;

      index++;
   }

   if (index >= MAX_WAYPOINTS)
      return(-1);
   
   // block saving test waypoints
   g_waypoint_testing = TRUE;

   // ladder check
   waypoints[index].flags = flags;

   waypoints[index].itemflags = itemflags;

   // store the origin (location) of this waypoint 
   waypoints[index].origin = vecOrigin;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = gpGlobals->time - 10.0f;

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;

   // calculate all the paths to this new waypoint
   if(MakePaths)
      WaypointMakePathsWith(index);
   
   return(index);
}


void WaypointAddAiming(edict_t *pEntity)
{
   int index;

   index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         break;

      index++;
   }

   if (index >= MAX_WAYPOINTS)
      return;
      
   // save on mapchange
   g_waypoint_updated = TRUE;

   //..
   waypoints[index].flags = W_FL_AIMING;  // aiming waypoint

   waypoints[index].itemflags = 0; 

   Vector v_angle;
   
   v_angle.y = pEntity->v.v_angle.y;
   v_angle.x = 0;  // reset pitch to horizontal
   v_angle.z = 0;  // reset roll to level

   // store the origin (location) of this waypoint (use entity origin)
   waypoints[index].origin = pEntity->v.origin + UTIL_AnglesToForward(v_angle) * 25;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = gpGlobals->time;


   Vector start, end;

   start = pEntity->v.origin - Vector(0, 0, 10);
   end = start + Vector(0, 0, 14);

   // draw a blue waypoint
   WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

   if(g_waypoint_on)
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0,
                   ATTN_NORM, 0, 100);

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;
}


void WaypointDelete(edict_t *pEntity)
{
   int index;

   if (num_waypoints < 1)
      return;

   index = WaypointFindNearest(pEntity, 50.0);

   if (index == -1)
      return;

   //..

   if (/*waypoints[index].flags & W_FL_SNIPER ||*/ waypoints[index].flags & W_FL_JUMP)
   {
      int i;
      int min_index = -1;
      float min_distance = 99999.0;
      float distance;

      // search for nearby aiming waypoint and delete it also...
      for (i=0; i < num_waypoints; i++)
      {
         if (waypoints[i].flags & W_FL_DELETED)
            continue;  // skip any deleted waypoints

         if ((waypoints[i].flags & W_FL_AIMING) == 0)
            continue;  // skip any NON aiming waypoints

         distance = (waypoints[i].origin - waypoints[index].origin).Length();

         if ((distance < min_distance) && (distance < 40))
         {
            min_index = i;
            min_distance = distance;
         }
      }

      if (min_index != -1)
      {
         // save on mapchange
         g_waypoint_updated = TRUE;
   
         waypoints[min_index].flags = W_FL_DELETED;  // not being used
         waypoints[min_index].origin = Vector(0,0,0);
         waypoints[min_index].itemflags = 0;

         wp_display_time[min_index] = 0.0;
      }
   }

   // delete any paths that lead to this index...
   WaypointDeletePath(index);

   // free the path for this index...

   if (paths[index] != NULL)
   {
      PATH *p = paths[index];
      PATH *p_next;

      while (p)  // free the linked list
      {
         p_next = p->next;  // save the link to next
         free(p);
         p = p_next;

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug();
#endif
      }

      paths[index] = NULL;
   }

   // save on mapchange
   g_waypoint_updated = TRUE;

   waypoints[index].flags = W_FL_DELETED;  // not being used
   waypoints[index].origin = Vector(0,0,0);
   waypoints[index].itemflags = 0;

   wp_display_time[index] = 0.0;

   if(g_waypoint_on)
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0,
                   ATTN_NORM, 0, 100);
}


void WaypointUpdate(edict_t *pEntity)
{
   int index;
   int mask;

   mask = W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON | W_FL_LONGJUMP;

   for (index=0; index < num_waypoints; index++)
   {
      int oldflags = waypoints[index].flags;
      
      waypoints[index].flags &= ~mask;  // clear the mask bits

      WaypointSearchItems(NULL, waypoints[index].origin, index);
      
      // flags can be recheck on mapchange
      if(oldflags != waypoints[index].flags)
         g_waypoint_updated = TRUE;
   }
}


// allow player to manually create a path from one waypoint to another
void WaypointCreatePath(edict_t *pEntity, int cmd)
{
   static int waypoint1 = -1;  // initialized to unassigned
   static int waypoint2 = -1;  // initialized to unassigned

   if (cmd == 1)  // assign source of path
   {
      waypoint1 = WaypointFindNearest(pEntity, 50.0);

      if (waypoint1 == -1)
      {
         if(g_waypoint_on) {
            // play "cancelled" sound...
            EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0,
                         ATTN_NORM, 0, 100);
         }
         
         return;
      }

      // play "start" sound...
      if(g_waypoint_on)
          EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0,
                      ATTN_NORM, 0, 100);

      return;
   }

   if (cmd == 2)  // assign dest of path and make path
   {
      waypoint2 = WaypointFindNearest(pEntity, 50.0);

      if ((waypoint1 == -1) || (waypoint2 == -1))
      {
         // play "error" sound...
         if(g_waypoint_on)
            EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      g_waypoint_updated = TRUE;
      WaypointAddPath(waypoint1, waypoint2);

      // play "done" sound...
      if(g_waypoint_on)
          EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
                      ATTN_NORM, 0, 100);
   }
}


// allow player to manually remove a path from one waypoint to another
void WaypointRemovePath(edict_t *pEntity, int cmd)
{
   static int waypoint1 = -1;  // initialized to unassigned
   static int waypoint2 = -1;  // initialized to unassigned

   if (cmd == 1)  // assign source of path
   {
      waypoint1 = WaypointFindNearest(pEntity, 50.0);

      if (waypoint1 == -1)
      {
         // play "cancelled" sound...
         if(g_waypoint_on)
            EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      // play "start" sound...
      if(g_waypoint_on)
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0,
                      ATTN_NORM, 0, 100);

      return;
   }

   if (cmd == 2)  // assign dest of path and make path
   {
      waypoint2 = WaypointFindNearest(pEntity, 50.0);

      if ((waypoint1 == -1) || (waypoint2 == -1))
      {
         // play "error" sound...
         if(g_waypoint_on)
            EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      g_waypoint_updated = TRUE;
      WaypointDeletePath(waypoint1, waypoint2);

      // play "done" sound...
      if(g_waypoint_on)
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
                      ATTN_NORM, 0, 100);
   }
}


qboolean WaypointFixOldWaypoints(void)
{
   int k;
   qboolean Changed = FALSE;
   TraceResult tr;
   
   int crouch_to_normal_count = 0;
   int midair_count = 0;
   int unreachable_path_count = 0;
   
   //make crouch waypoints normal waypoints where possible
   for(k = 0; k < num_waypoints; k++)
   {
      if(waypoints[k].flags & W_FL_AIMING || waypoints[k].flags & W_FL_DELETED)
         continue;
      
      if(!(waypoints[k].flags & W_FL_CROUCH))
         continue;
      
      Vector v_src = waypoints[k].origin;
      Vector v_dest = waypoints[k].origin;
      
      v_dest.z += 36 + 1;
      
      UTIL_TraceDuck(v_src, v_dest, ignore_monsters, NULL, &tr);
      
      // check if we didn't hit anything, if not then there is room to stand up
      if (!tr.fAllSolid && !tr.fStartSolid && tr.flFraction >= 1.0f)
      {
         crouch_to_normal_count++;
         
         waypoints[k].flags &= ~W_FL_CROUCH;
         waypoints[k].origin.z += 36/2;

         g_waypoint_updated = TRUE;
         Changed = TRUE;
      }
   }
   
   //remove all midair waypoints
   for(k = 0; k < num_waypoints; k++)
   {
      if(waypoints[k].flags & W_FL_AIMING || waypoints[k].flags & W_FL_DELETED || waypoints[k].flags & W_FL_LADDER)
         continue;
      
      if(POINT_CONTENTS(waypoints[k].origin) == CONTENTS_WATER)
         continue;
      
      Vector v_src = waypoints[k].origin;
      Vector v_dest = waypoints[k].origin;
      
      if(waypoints[k].flags & W_FL_CROUCH)
      {
         v_dest.z -= 72/2 + 1;

         UTIL_TraceMove(v_src, v_dest, ignore_monsters, NULL, &tr);
      }
      else
      {
         v_dest.z -= 72/2 - 36/2 + 1;
         
         UTIL_TraceDuck(v_src, v_dest, ignore_monsters, NULL, &tr);
      }

      // check if we didn't hit anything, if not then it's in mid-air
      if (!tr.fAllSolid && !tr.fStartSolid && tr.flFraction >= 1.0f)
      {
         int index = k;
         
         // delete any paths that lead to this index...
         WaypointDeletePath(index);

         // free the path for this index...
         if (paths[index] != NULL)
         {
            PATH *p = paths[index];
            PATH *p_next;

            while (p)  // free the linked list
            {
               p_next = p->next;  // save the link to next
               free(p);
               p = p_next;

#ifdef _DEBUG
               count++;
               if (count > 100) WaypointDebug();
#endif
            }

            paths[index] = NULL;
         }

         // save on mapchange
         g_waypoint_updated = TRUE;
         Changed = TRUE;

         waypoints[index].flags = W_FL_DELETED;  // not being used
         waypoints[index].origin = Vector(0,0,0);
         waypoints[index].itemflags = 0;
         
         midair_count++;
      }
   }
 
   //Remove invalid paths
   for(k = 0; k < num_waypoints; k++)
   {
      if(waypoints[k].flags & (W_FL_AIMING | W_FL_DELETED))
         continue;
      
      PATH *p = paths[k];
      qboolean Modified = FALSE;
      while(p)
      {
         for(int i = 0; i < MAX_PATH_INDEX; i++)
         {
            if(p->index[i] == -1)
               continue;
            
            if(waypoints[p->index[i]].flags & W_FL_DELETED || p->index[i] == k || 
                !WaypointReachable(waypoints[k].origin, waypoints[p->index[i]].origin, GetReachableFlags(k, p->index[i])))
            {
               unreachable_path_count++;
               
               // remove
               p->index[i] = -1;
               Modified = TRUE;
            }
         }
         
         p = p->next;
      }
      
      if(Modified)
      {
      	 g_waypoint_updated = TRUE;
      	 Changed = TRUE;
      }
   }
   
   if(Changed)
   {
      UTIL_ConsolePrintf("Detected pre-1.10 waypoint file, fixed buggy autowaypoints:");
      
      if(crouch_to_normal_count)
         UTIL_ConsolePrintf("- Convert %d crouch waypoints to normal waypoints.", crouch_to_normal_count);
         
      if(midair_count)
         UTIL_ConsolePrintf("- Removed %d mid-air waypoints.", midair_count);
         
      if(unreachable_path_count)
         UTIL_ConsolePrintf("- Removed %d broken waypoint paths.", unreachable_path_count);
   }
   
   return(Changed);
}


qboolean WaypointLoad(edict_t *pEntity)
{
   gzFile bfp;
   char mapname[64];
   char filename[256];
   WAYPOINT_HDR header;
   int index, i;
   short int num;
   short int path_index;
   qboolean Changed;
   
   g_waypoint_updated = FALSE;
   g_waypoint_testing = FALSE;
   
   safevoid_snprintf(mapname, sizeof(mapname), "%s.wpt", STRING(gpGlobals->mapname));

   UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);

   UTIL_ConsolePrintf("Loading waypoint file: %s\n", filename);

   bfp = gzopen(filename, "rb");

   // if file exists, read the waypoint structure from it
   if (bfp != NULL)
   {
      gzread(bfp, &header, sizeof(header));

      header.filetype[7] = 0;
      if (strcmp(header.filetype, WAYPOINT_MAGIC) == 0)
      {
         if (header.waypoint_file_version != WAYPOINT_VERSION)
         {
            UTIL_ConsolePrintf("Incompatible jk_botti waypoint file version!\n");
            UTIL_ConsolePrintf("Waypoints not loaded!\n");

            gzclose(bfp);
            return FALSE;
         }

         header.mapname[31] = 0;

#ifdef WPT_MUST_MATCH_MAPNAME
         if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
#endif
         {
            WaypointInit();  // remove any existing waypoints
            
            int count_wp = 0, count_paths = 0;

            for (i=0; i < header.number_of_waypoints; i++)
            {
               gzread(bfp, &waypoints[i], sizeof(waypoints[0]));
               num_waypoints++;
               
               if(!(waypoints[i].flags & W_FL_DELETED))
                  count_wp++;
            }

            // read and add waypoint paths...
            for (index=0; index < num_waypoints; index++)
            {
               // read the number of paths from this node...
               gzread(bfp, &num, sizeof(num));
               count_paths += num;

               for (i=0; i < num; i++)
               {
                  gzread(bfp, &path_index, sizeof(path_index));

                  WaypointAddPath(index, path_index);
               }
            }

            g_waypoint_paths = TRUE;  // keep track so path can be freed
            
            UTIL_ConsolePrintf("- loaded: %d waypoints, %d paths\n", count_wp, count_paths);
         }
#ifdef WPT_MUST_MATCH_MAPNAME
         else
         {
            UTIL_ConsolePrintf("%s jk_botti waypoints are not for this map!\n", filename);

            gzclose(bfp);
            return FALSE;
         }
#endif
      }
      else
      {
         UTIL_ConsolePrintf("%s is not a jk_botti waypoint file!\n", filename);

         gzclose(bfp);
         return FALSE;
      }

      gzclose(bfp);
      
      Changed = false;
      
      // very early pre-1.00 version waypoints(v0) had __reserved[4] and no itemflags
      if(header.waypoint_file_subversion < WPF_SUBVERSION_1_00)
      {
         for(int i = 0; i < num_waypoints; i++)
         {
            waypoints[i].itemflags = 0;
         }
      }
      
      // Check if this waypoint file is already fixed
      if(header.waypoint_file_subversion < WPF_SUBVERSION_1_10)
      {
         Changed = WaypointFixOldWaypoints();
      }
      
      // Reserved slots in waypoints are not used in WPF_SUBVERSION_1_00 and WPF_SUBVERSION_1_10
      // clean up
      if(header.waypoint_file_subversion <= WPF_SUBVERSION_1_10)
      {
         for(int i = 0; i < num_waypoints; i++)
         {
            waypoints[i].__reserved[0] = 0;
            waypoints[i].__reserved[1] = 0;
            waypoints[i].__reserved[2] = 0;
         }
      }

      WaypointRouteInit(Changed);
   }
   else
   {
      UTIL_ConsolePrintf("Waypoint file %s not found!\n", filename);

      return FALSE;
   }

   return TRUE;
}


int WaypointNumberOfPaths(int index)
{
   // count the number of paths from this node...
   PATH *p = paths[index];
   int num = 0;

   while (p != NULL)
   {
      int i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] != -1)
            num++; // count path node if it's used

         i++;
      }

      p = p->next; // go to next node in linked list
   }
   
   return(num);
}


int WaypointIncomingPathsCount(int index, int exit_count)
{
   int num = 0;
   
   for(int k = 0; k < num_waypoints; k++)
   {
      PATH *p = paths[k];
      while (p != NULL)
      {
         int i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] == index)
            {
               num++; // count path 
               goto found_already;
            }

            i++;
         }

         p = p->next; // go to next node in linked list
      }
      
found_already:
      
      // exit early if we just want to know if specific number of path point to this waypoint
      if(exit_count > 0 && num >= exit_count)
         return(num);
      
      continue;
   }
   
   return(num);
}


// remove dummy waypoints, waypoints that have no paths in or out
void WaypointTrim(void)
{
   int orphan_count = 0;
	
   for(int i = 0; i < num_waypoints; i++)
   {
      if(waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_SPAWNADD) )
         continue;
      
      // get number of paths out from wp
      if(WaypointNumberOfPaths(i) > 0)
         continue;
      
      // get number of paths to wp
      if(WaypointIncomingPathsCount(i, 1) > 0)
         continue;
      
      g_waypoint_updated = TRUE;
   
      waypoints[i].flags = W_FL_DELETED;  // not being used
      waypoints[i].origin = Vector(0,0,0);
      waypoints[i].itemflags = 0;
      
      orphan_count++;
   }
   
   if(orphan_count)
      UTIL_ConsolePrintf("Removed %d orphan waypoints.\n", orphan_count);
}


void WaypointSave(void)
{
   char filename[256];
   char mapname[64];
   WAYPOINT_HDR header;
   int index, i;
   short int num;
   PATH *p;

   if(g_waypoint_testing || !g_waypoint_updated)
      return;

   //
   WaypointTrim();

   safevoid_snprintf(header.filetype, sizeof(header.filetype), "%s", WAYPOINT_MAGIC);

   header.waypoint_file_version = WAYPOINT_VERSION;
   header.waypoint_file_subversion = WPF_SUBVERSION_1_10;
   header.waypoint_file_flags = 0;

   header.number_of_waypoints = num_waypoints;

   memset(header.mapname, 0, sizeof(header.mapname));
   safevoid_snprintf(header.mapname, sizeof(header.mapname), "%s", STRING(gpGlobals->mapname));

   safevoid_snprintf(mapname, sizeof(mapname), "%s.wpt", STRING(gpGlobals->mapname));

   UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);

   UTIL_ConsolePrintf("Saving waypoint file: %s\n", filename);

   gzFile bfp = gzopen(filename, "wb6");

   // write the waypoint header to the file...
   gzwrite(bfp, &header, sizeof(header));

   int count_wp=0, count_paths=0;

   // write the waypoint data to the file...
   for (index=0; index < num_waypoints; index++)
   {
      gzwrite(bfp, &waypoints[index], sizeof(waypoints[0]));
      
      if(!(waypoints[index].flags & W_FL_DELETED))
         count_wp++;
   }

   // save the waypoint paths...
   for (index=0; index < num_waypoints; index++)
   {
      // count the number of paths from this node...
      num = WaypointNumberOfPaths(index);
      count_paths += num;

      gzwrite(bfp, &num, sizeof(num));  // write the count

      // now write out each path index...

      p = paths[index];

      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] != -1)  // save path node if it's used
               gzwrite(bfp, &p->index[i], sizeof(p->index[0]));

            i++;
         }

         p = p->next;  // go to next node in linked list
      }
   }
   
   UTIL_ConsolePrintf("- saved: %d waypoints, %d paths\n", count_wp, count_paths);

   gzclose(bfp);
}


qboolean WaypointReachable(const Vector &v_src, const Vector &v_dest, const int reachable_flags)
{
   TraceResult tr;
   float curr_height, last_height, waterlevel;
   Vector v_check_dest, v_check, v_direction, v_down;
   
   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < REACHABLE_RANGE)
   {
      // check if this waypoint is "visible"...
      if(reachable_flags & (W_FL_LADDER | W_FL_CROUCH))
         UTIL_TraceMove( v_src, v_dest, ignore_monsters, NULL, &tr );
      else
         UTIL_TraceHull( v_src, v_dest, ignore_monsters, head_hull, NULL, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (!tr.fAllSolid && !tr.fStartSolid && tr.flFraction >= 1.0f)
      {
      	 // special ladder case, if both on same ladder -> ok
      	 if((reachable_flags & W_FL_LADDER) && (v_src.Make2D() - v_dest.Make2D()).Length() <= 32.0f)
      	 {
      	    return TRUE;
      	 }
      	 
      	 // very close.. 
         if((v_dest - v_src).Length() <= 32.0f)
            return(TRUE);
      	
         // check for special case of both waypoints being underwater...
         if ((POINT_CONTENTS( v_src ) == CONTENTS_WATER) &&
             (POINT_CONTENTS( v_dest ) == CONTENTS_WATER))
         {
            return TRUE;
         }

         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > (v_src.z + 45.0))
         {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceMove(v_new_src, v_new_dest, ignore_monsters, NULL, &tr);
         
            if(tr.fAllSolid || tr.fStartSolid)
            {
               //BUG BUG! what should we do?
               return FALSE;
            }
            
            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0f)
            {
               return FALSE;  // can't reach this one
            }
         }

         // get source water level
         if(POINT_CONTENTS( v_src ) == CONTENTS_WATER)
         {
            waterlevel = -99999;
            
            v_direction = (v_dest - v_src).Normalize();  // 1 unit long
            v_check = v_src;
            
            if(v_direction.z > 0.0f)
            {
               // go upwards until get to air
               while((v_dest - v_check).Length() < (v_dest - v_src).Length())
               {
                  // advance one unit upwards
                  v_check = v_check + v_direction * (1.0f / v_direction.z);
               
                  if(POINT_CONTENTS( v_check ) == CONTENTS_EMPTY)
                  {
                     waterlevel = round(v_check.z);
                     break;
                  }
               }
            }
         }
         else
         {
            // outside map area (lowest level on map is -4096)
            waterlevel = -99999;
         }

         // check if distance to ground increases more than jump height
         // at points between source and destination...

         v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         v_check = v_src + v_direction * 16.0f;
         v_down = v_check;
         v_check_dest = v_dest - v_direction * 16.0f;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceMove(v_check, v_down, ignore_monsters, NULL, &tr);
         
         if(tr.fAllSolid || tr.fStartSolid)
         {
            //BUG BUG! what should we do?
            return FALSE;
         }

         last_height = tr.flFraction * 1000.0;  // height from ground
         
         //check against waterlevel
         if(v_check.z - last_height < waterlevel)
         {
            last_height = v_check.z - waterlevel;
         }
         
         last_height += v_direction.z * 16.0f;  // we skipped 16 units, add height to calculations

         distance = (v_check_dest - v_check).Length();  // distance from goal

         while (distance > 10.0f)
         {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0f);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceMove(v_check, v_down, ignore_monsters, NULL, &tr);

            if(tr.fAllSolid || tr.fStartSolid)
            {
               //BUG BUG! what should we do?
               return FALSE;
            }

            curr_height = tr.flFraction * 1000.0;  // height from ground

            //check against waterlevel
            if(v_check.z - curr_height < waterlevel)
            {
               curr_height = v_check.z - waterlevel;
            }

            // is the difference in the last height and the current height
            // higher that the jump height?
            if ((last_height - curr_height) > 45.0f)
            {
               // can't get there from here...
               return FALSE;
            }

            last_height = curr_height;

            distance = (v_check_dest - v_check).Length();  // distance from goal
         }
         
         // we skipped last 16 units and there still is some distance left
         v_check = v_dest;
         
         v_down = v_check;
         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceMove(v_check, v_down, ignore_monsters, NULL, &tr);   
         
         if(tr.fAllSolid || tr.fStartSolid)
         {
            //BUG BUG! what should we do?
            return FALSE;
         }
         
         curr_height = tr.flFraction * 1000.0;  // height from ground

         // is the difference in the last height and the current height
         // higher that the jump height?
         if ((last_height - curr_height) > 45.0f)
         {
            // can't get there from here...
            return FALSE;
         }
         
         return TRUE;
      }
   }

   return FALSE;
}


// find the nearest reachable waypoint
int WaypointFindReachable(edict_t *pEntity, float range)
{
   int i, min_index=0;
   float distance;
   float min_distance;
   TraceResult tr;

   // find the nearest waypoint...

   min_distance = 99999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;  // skip any deleted & aiming waypoints

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if (distance < min_distance)
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceMove( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0f)
         {
            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, GetReachableFlags(pEntity, i)))
            {
               min_index = i;
               min_distance = distance;
            }
         }
      }
   }

   // if not close enough to a waypoint then just return
   if (min_distance > range)
      return -1;

   return min_index;

}


#if _DEBUG
void WaypointPrintInfo(edict_t *pEntity)
{
   char msg[80];
   int index;
   int flags;

   // find the nearest waypoint...
   index = WaypointFindNearest(pEntity, 50.0);

   if (index == -1)
      return;

   safevoid_snprintf(msg, sizeof(msg), "Waypoint %d of %d total\n", index, num_waypoints);
   ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

   flags = waypoints[index].flags;

   if (flags & W_FL_LIFT_START)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This waypoint is start of lift\n");
      
   if (flags & W_FL_LIFT_END)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This waypoint is end of lift\n");
      
   if (flags & W_FL_LADDER)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This waypoint is on a ladder\n");

   if (flags & W_FL_DOOR)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This is a door waypoint\n");

   if (flags & W_FL_HEALTH)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is health near this waypoint\n");

   if (flags & W_FL_ARMOR)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is armor near this waypoint\n");

   if (flags & W_FL_AMMO)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is ammo near this waypoint\n");

   if (flags & W_FL_WEAPON)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "There is a weapon near this waypoint\n");

   if (flags & W_FL_JUMP)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Bot will jump here\n");

//   if (flags & W_FL_SNIPER)
//      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This is a sniper waypoint\n");
}
#endif


void WaypointAutowaypointing(int idx, edict_t *pEntity)
{
   float distance, min_distance;
   int i;
   qboolean DidAddAlready = FALSE;
   
   // only if on ground or on ladder or under water
   if((pEntity->v.flags & FL_ONGROUND) != FL_ONGROUND && pEntity->v.movetype != MOVETYPE_FLY && pEntity->v.waterlevel == 0)
      return;
   
   // on moving platform or train?
   if(!FNullEnt(pEntity->v.groundentity) && (pEntity->v.groundentity->v.speed > 0.0f || pEntity->v.groundentity->v.avelocity != Vector(0, 0, 0)))
      return;
   
   // moving with grapple (op4)
   if(pEntity->v.movetype == MOVETYPE_FLY && players[idx].current_weapon_id == GEARBOX_WEAPON_GRAPPLE)
      return;

   // more waypoints on ladders
   float target_distance = (pEntity->v.movetype == MOVETYPE_FLY) ? 65.0f : 200.0f;

   // find the distance from the last used waypoint
   if(players[idx].last_waypoint == -1)
      distance = 99999.0f;
   else
      distance = (waypoints[players[idx].last_waypoint].origin - pEntity->v.origin).Length();

   if (distance >= target_distance)
   {
      int onepath_wpt = -1;
      min_distance = 99999.0f;

      // check that no other reachable waypoints are nearby...
      for (i=0; i < num_waypoints; i++)
      {
         if (waypoints[i].flags & (W_FL_DELETED|W_FL_AIMING|W_FL_LIFT_END))
            continue;

         if (!WaypointReachable(pEntity->v.origin, waypoints[i].origin, GetReachableFlags(pEntity, i)))
            continue;
         
         distance = (waypoints[i].origin - pEntity->v.origin).Length();
         if (distance < min_distance) 
         {
            min_distance = distance;
         }
         
         // special code that does linking of two waypoints that are unreachable to each other
         // and are reachable to this location. Other end is waypoint with one or none path.
         if(distance < target_distance && distance >= 64.0f && onepath_wpt == -1 && i != players[idx].last_waypoint && WaypointNumberOfPaths(i) <= 1)
         {
            onepath_wpt = i;
         }
      }

      // make sure nearest waypoint is far enough away...
      if (min_distance >= target_distance) 
      {
         WaypointAdd(pEntity);
         WaypointAddBlock(pEntity->v.origin);
         DidAddAlready = TRUE;
      }
      else if(onepath_wpt != -1 && min_distance >= 64.0f)
      {
         // special code that does linking of two waypoints that are unreachable to each other
         // and are reachable to this location. Other end is waypoint with one or none path.
         for (i=0; i < num_waypoints; i++)
         {
            if (waypoints[i].flags & (W_FL_DELETED|W_FL_AIMING|W_FL_LIFT_END))
               continue;

            // distance between isn't too much
            if ((waypoints[i].origin - waypoints[onepath_wpt].origin).Length() >= 350.0f)

            // need to be reachable to current player location
            if (!WaypointReachable(pEntity->v.origin, waypoints[i].origin, GetReachableFlags(pEntity, i)))
               continue;

            // waypoints are unreachable to each other
            if (!WaypointReachable(waypoints[i].origin, waypoints[onepath_wpt].origin, GetReachableFlags(onepath_wpt, i)))
            {
               WaypointAdd(pEntity);
               WaypointAddBlock(pEntity->v.origin);
               DidAddAlready = TRUE;
                  
               break;
            }
         }
      }
      
      // checking index 0 twice -> is matrix initialized?
      // make sure we are not too close to resently added route/path-cutting waypoint
      if(!DidAddAlready && WaypointIsRouteValid(0, 0) && !WaypointInBlockRadius(pEntity->v.origin))
      {
         // check waypoints around this area, then add waypoint if route between waypoints is more than 1000.0 units
         for (i=0; i < num_waypoints; i++)
         {
            if (waypoints[i].flags & (W_FL_DELETED|W_FL_AIMING|W_FL_LIFT_END))
               continue;

            if((waypoints[i].origin - pEntity->v.origin).Length() < 64.0f)
               continue;

            // need to be reachable to current player location
            if (!WaypointReachable(pEntity->v.origin, waypoints[i].origin, GetReachableFlags(pEntity, i)) ||
                !WaypointReachable(waypoints[i].origin, pEntity->v.origin, GetReachableFlags(i, pEntity)))
               continue;

            // distance between isn't too much
            for (int k=0; k < num_waypoints; k++)
            {
               if (waypoints[k].flags & (W_FL_DELETED|W_FL_AIMING|W_FL_LIFT_END))
                  continue;
               
               if((waypoints[k].origin - pEntity->v.origin).Length() < 64.0f)
                  continue;
               
               // valid?
               if (!WaypointIsRouteValid(i, k))
                  continue;
               
               // is it long route?
               if (WaypointDistanceFromTo(i, k) < 1600.0f)
                  continue;
               
               // need to be reachable to current player location
               if (!WaypointReachable(pEntity->v.origin, waypoints[k].origin, GetReachableFlags(pEntity, k)) ||
                   !WaypointReachable(waypoints[k].origin, pEntity->v.origin, GetReachableFlags(k, pEntity)))
                  continue;
               
               WaypointAdd(pEntity);
               WaypointAddBlock(pEntity->v.origin);
               DidAddAlready = TRUE;
               
               break;
            }
            
            if(DidAddAlready)
               break;
         }
      }
   }
}


void WaypointThink(edict_t *pEntity)
{
   float distance, min_distance;
   Vector start, end;
   int i, idx, index=0;

   idx = ENTINDEX(pEntity) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return;

   // is auto waypoint on?
   // is player on ground, underwater or ladder?
   if(g_auto_waypoint)
   {
      WaypointAutowaypointing(idx, pEntity);
   }

   min_distance = 99999.0f;

   if (g_waypoint_on)  // display the waypoints if turned on...
   {
      for (i=0; i < num_waypoints; i++)
      {
         if (waypoints[i].flags & (W_FL_DELETED|W_FL_AIMING))
            continue;

         distance = (waypoints[i].origin - pEntity->v.origin).Length();

         if (distance < 500.0f)
         {
            if (distance < min_distance)
            {
               index = i; // store index of nearest waypoint
               min_distance = distance;
            }

            if ((wp_display_time[i] + 1.0) < gpGlobals->time)
            {
               if (waypoints[i].flags & W_FL_CROUCH)
               {
                  start = waypoints[i].origin - Vector(0, 0, 17);
                  end = start + Vector(0, 0, 34);
               }
               else if (waypoints[i].flags & W_FL_AIMING)
               {
                  start = waypoints[i].origin + Vector(0, 0, 10);
                  end = start + Vector(0, 0, 14);
               }
               else
               {
                  start = waypoints[i].origin - Vector(0, 0, 34);
                  end = start + Vector(0, 0, 68);
               }

               if(waypoints[i].flags & W_FL_CROUCH)
               {
                  // draw a yellow waypoint
                  WaypointDrawBeam(pEntity, start, end, 30, 0, 250, 250, 0, 250, 5);
               }
               else if(waypoints[i].flags & W_FL_LADDER)
               {
                  // draw a purple waypoint
                  WaypointDrawBeam(pEntity, start, end, 30, 0, 250, 0, 250, 250, 5);
               }
               else if(waypoints[i].flags & W_FL_LIFT_START)
               {
                  // draw a red waypoint
                  WaypointDrawBeam(pEntity, start, end, 30, 0, 255, 0, 0, 250, 5);
               }
               else if(waypoints[i].flags & W_FL_LIFT_END)
               {
                  // draw a green waypoint
                  WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 255, 0, 250, 5);
               }
               else
               {
                  // draw a blue waypoint
                  WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);
               }

               wp_display_time[i] = gpGlobals->time;
            }
         }
      }

      // check if path waypointing is on...
      if (g_path_waypoint)
      {
         // check if player is close enough to a waypoint and time to draw path...
         if ((min_distance <= 50.0f) && (f_path_time <= gpGlobals->time))
         {
            PATH *p;

            f_path_time = gpGlobals->time + 1.0;

            p = paths[index];

            while (p != NULL)
            {
               i = 0;

               while (i < MAX_PATH_INDEX)
               {
                  if (p->index[i] != -1)
                  {
                     Vector v_src = waypoints[index].origin;
                     Vector v_dest = waypoints[p->index[i]].origin;

                     // draw a white line to this index's waypoint
                     WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 250, 250, 250, 200, 10);
                  }

                  i++;
               }

               p = p->next;  // go to next node in linked list
            }
         }
      }
   }
}


void WaypointSaveFloydsMatrix(void)
{
   return WaypointSaveFloydsMatrix(shortest_path, from_to);
}


void WaypointSaveFloydsMatrix(unsigned short *shortest_path, unsigned short *from_to)
{
   char filename[256];
   char mapname[64];
   int num_items;
   gzFile bfp;
   int array_size;
   
   array_size = route_num_waypoints * route_num_waypoints;
   
   //
   safevoid_snprintf(mapname, sizeof(mapname), "%s.matrix", STRING(gpGlobals->mapname));
   
   UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);
   
   bfp = gzopen(filename, "wb6");

   if (bfp != NULL)
   {
      num_items = gzwrite(bfp, "jkbotti_matrixA\0", 16);
      if (num_items != 16)
      {
         // if couldn't write enough data, close file and delete it
         gzclose(bfp);
         unlink(filename);
         
         UTIL_ConsolePrintf("[matrix save] - Error writing waypoint matrix (code: %d)!\n", 1);
      }
      else
      {
         num_items = gzwrite(bfp, shortest_path, sizeof(unsigned short) * array_size) / sizeof(unsigned short);

         if (num_items != array_size)
         {
            // if couldn't write enough data, close file and delete it
            gzclose(bfp);
            unlink(filename);
            
            UTIL_ConsolePrintf("[matrix save] - Error writing waypoint matrix (code: %d)!\n", 2);
         }
         else
         {
            num_items = gzwrite(bfp, "jkbotti_matrixB\0", 16);
            if (num_items != 16)
            {
               // if couldn't write enough data, close file and delete it
               gzclose(bfp);
               unlink(filename);
               
               UTIL_ConsolePrintf("[matrix save] - Error writing waypoint matrix (code: %d)!\n", 3);
            }
            else
            {            	
               num_items = gzwrite(bfp, from_to, sizeof(unsigned short) * array_size) / sizeof(unsigned short);

               gzclose(bfp);

               if (num_items != array_size)
               {
                  // if couldn't write enough data, delete file
                  unlink(filename);
                  
                  UTIL_ConsolePrintf("[matrix save] - Error writing waypoint matrix (code: %d)!\n", 4);
               }
               else
               {
                  UTIL_ConsolePrintf("[matrix save] Waypoint matrix '%s' saved.\n", filename);
               }
            }
         }
      }
   }
   else
   {
      UTIL_ConsolePrintf("[matrix save] - Error writing waypoint matrix (code: %d)!\n", 0);
   }
}


int WaypointSlowFloyds(void)
{
   return WaypointSlowFloyds(shortest_path, from_to);
}


static struct {
   int state;
   unsigned int x, y, z;
   int changed;
   int escaped;
} slow_floyds = {-1,0,0,0,0,0};

void WaypointSlowFloydsStop(void)
{
   slow_floyds.state = -1;
}

int WaypointSlowFloydsState(void)
{
   return(slow_floyds.state);
}

int WaypointSlowFloyds(unsigned short *shortest_path, unsigned short *from_to)
{
   unsigned int x, y, z;
   int changed;
   int distance;
   
   if(slow_floyds.state == -1)
      return -1;
   
   if(slow_floyds.state == 0)
   {
      // the begining
      wp_matrix_initialized = FALSE;
      slow_floyds.x = 0;
      slow_floyds.y = 0;
      slow_floyds.z = 0;
      slow_floyds.changed = 1;
      slow_floyds.escaped = 0;
      
      slow_floyds.state = 1;
      
      for (y=0; y < route_num_waypoints; y++)
      {
         for (z=0; z < route_num_waypoints; z++)
         {
            from_to[y * route_num_waypoints + z] = z;
         }
      }
      
      return 1;
   }
   
   if(slow_floyds.state == 1)
   {
      // middle, the slowest part
      int max_count = 50;
      
      x = slow_floyds.x;
      y = slow_floyds.y;
      z = slow_floyds.z;
      changed = slow_floyds.changed;
      
      if(slow_floyds.escaped)
      {
         slow_floyds.escaped = 0;
         goto escape_return;
      }
      
      while(changed)
      {
         changed = 0;

         for (x=0; x < route_num_waypoints; x++)
         {
            for (y=0; y < route_num_waypoints; y++)
            {
               if(max_count-- <= 0)
               {
                  slow_floyds.x = x;
                  slow_floyds.y = y;
                  slow_floyds.z = z;
                  slow_floyds.changed = changed;
                  slow_floyds.escaped = 1;
                  return 1;
               }
escape_return:

               for (z=0; z < route_num_waypoints; z++)
               {
                  if ((shortest_path[y * route_num_waypoints + x] == WAYPOINT_UNREACHABLE) ||
                      (shortest_path[x * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
                     continue;

                  distance = shortest_path[y * route_num_waypoints + x] +
                             shortest_path[x * route_num_waypoints + z];

                  if (distance > WAYPOINT_MAX_DISTANCE)
                     distance = WAYPOINT_MAX_DISTANCE;

                  if ((distance < shortest_path[y * route_num_waypoints + z]) ||
                      (shortest_path[y * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
                  {
                     shortest_path[y * route_num_waypoints + z] = distance;
                     from_to[y * route_num_waypoints + z] = from_to[y * route_num_waypoints + x];
                     changed = 1;
                  }
               }
            }
         }
      }
      
      if(changed == 0)
      {
         // we have hit the end
         slow_floyds.state = -1;

         for (x=0; x < route_num_waypoints; x++)
         {
            for (y=0; y < route_num_waypoints; y++)
               if (shortest_path[x * route_num_waypoints + y] == WAYPOINT_UNREACHABLE)
                  from_to[x * route_num_waypoints + y] = WAYPOINT_UNREACHABLE;
         }
         
         wp_matrix_initialized = TRUE;
         wp_matrix_save_on_mapend = TRUE;
         
         UTIL_ConsolePrintf("[matrix calc] - waypoint path calculations complete!\n");
         
         return -1;
      }
      
      slow_floyds.x = x;
      slow_floyds.y = y;
      slow_floyds.z = z;
      slow_floyds.changed = changed;
      
      return 1;
   }
   
   slow_floyds.state = -1;
   return -1;
}


void WaypointRouteInit(qboolean ForceRebuild)
{
   unsigned int index;
   unsigned int array_size;
   unsigned int row;
   int i, offset;
   float distance;
   unsigned short *pShortestPath, *pFromTo;
   unsigned int num_items;
   gzFile bfp;
   char filename2[256];
   char mapname[64];
   int file1, file2;
   struct stat stat1, stat2;
   char header[16];

   wp_matrix_initialized = FALSE;
   wp_matrix_save_on_mapend = FALSE;

   if (num_waypoints == 0)
      return;

   // save number of current waypoints in case waypoints get added later
   route_num_waypoints = num_waypoints;
   array_size = route_num_waypoints * route_num_waypoints;

   //
   safevoid_snprintf(mapname, sizeof(mapname), "%s.matrix", STRING(gpGlobals->mapname));
   
   UTIL_BuildFileName_N(filename2, sizeof(filename2), "addons/jk_botti/waypoints", mapname);

   // does the ".matrix" file exist?
   if (!ForceRebuild && access(filename2, 0) == 0)
   {
      char filename[256];
      
      //
      safevoid_snprintf(mapname, sizeof(mapname), "%s.wpt", STRING(gpGlobals->mapname));
      
      UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);

      file1 = open(filename, O_RDONLY);
      file2 = open(filename2, O_RDONLY);

      fstat(file1, &stat1);
      fstat(file2, &stat2);

      close(file1);
      close(file2);

      if (stat1.st_mtime <= stat2.st_mtime)  // is ".wpt" older than or as old as ".matrix" file?
      {
         UTIL_ConsolePrintf("[matrix load] - loading jk_botti waypoint path matrix\n");

         shortest_path = (unsigned short *)calloc(1, sizeof(unsigned short) * array_size);

         if (shortest_path == NULL)
            UTIL_ConsolePrintf("[matrix load] - Error allocating memory for shortest path!\n");

         from_to = (unsigned short *)calloc(1, sizeof(unsigned short) * array_size);

         if (from_to == NULL)
            UTIL_ConsolePrintf("[matrix load] - Error allocating memory for from to matrix!\n");

         bfp = gzopen(filename2, "rb");

         if (bfp != NULL)
         {
            // first try read header 'jkbotti_matrixA\0', 16 bytes
            num_items = gzread(bfp, header, 16);
            if(num_items != 16 || strcmp(header, "jkbotti_matrixA\0") != 0)
            {
               // if couldn't read enough data, free memory to recalculate it
               UTIL_ConsolePrintf("[matrix load] - error reading first matrix file header, recalculating...\n");

               free(shortest_path);
               shortest_path = NULL;
               
               free(from_to);
               from_to = NULL;
            }
            else
            {
               num_items = gzread(bfp, shortest_path, sizeof(unsigned short) * array_size) / sizeof(unsigned short);

               if (num_items != array_size)
               {
                  // if couldn't read enough data, free memory to recalculate it
                  UTIL_ConsolePrintf("[matrix load] - error reading enough path items, recalculating...\n");

                  free(shortest_path);
                  shortest_path = NULL;

                  free(from_to);
                  from_to = NULL;
               }
               else
               {
                  // first try read header 'jkbotti_matrixB\0', 16 bytes
                  num_items = gzread(bfp, header, 16);
                  if(num_items != 16 || strcmp(header, "jkbotti_matrixB\0") != 0)
                  {
                     // if couldn't read enough data, free memory to recalculate it
                     UTIL_ConsolePrintf("[matrix load] - error reading second matrix file header, recalculating...\n");

                     free(shortest_path);
                     shortest_path = NULL;
                     
                     free(from_to);
                     from_to = NULL;
                  }
                  else
                  {
                     num_items = gzread(bfp, from_to, sizeof(unsigned short) * array_size) / sizeof(unsigned short);
   
                     if (num_items != array_size)
                     {
                        // if couldn't read enough data, free memory to recalculate it
                        UTIL_ConsolePrintf("[matrix load] - error reading enough path items, recalculating...\n");

                        free(shortest_path);
                        shortest_path = NULL;

                        free(from_to);
                        from_to = NULL;
                     }
                     
                     wp_matrix_initialized = TRUE;
                     wp_matrix_save_on_mapend = FALSE;
                  }
               }
            }
            
            gzclose(bfp);
         }
         else
         {
            UTIL_ConsolePrintf("[matrix load] - Error reading waypoint paths!\n");

            free(shortest_path);
            shortest_path = NULL;

            free(from_to);
            from_to = NULL;
         }
      }
      else
      {
      	 UTIL_ConsolePrintf("[matrix load] - Waypoint file is newer than matrix file, recalculating...\n");
      }
   }

   if (shortest_path == NULL)
   {
      UTIL_ConsolePrintf("[matrix calc] - calculating jk_botti waypoint path matrix\n");

      shortest_path = (unsigned short *)calloc(1, sizeof(unsigned short) * array_size);

      if (shortest_path == NULL)
         UTIL_ConsolePrintf("[matrix calc] - Error allocating memory for shortest path!\n");

      from_to = (unsigned short *)calloc(1, sizeof(unsigned short) * array_size);

      if (from_to == NULL)
         UTIL_ConsolePrintf("[matrix calc] - Error allocating memory for from to matrix!\n");

      pShortestPath = shortest_path;
      pFromTo = from_to;

      for (index=0; index < array_size; index++)
         pShortestPath[index] = WAYPOINT_UNREACHABLE;

      for (index=0; index < route_num_waypoints; index++)
         pShortestPath[index * route_num_waypoints + index] = 0;  // zero diagonal

      for (row=0; row < route_num_waypoints; row++)
      {
         if (paths[row] != NULL)
         {
            PATH *p = paths[row];

            while (p)
            {
               i = 0;

               while (i < MAX_PATH_INDEX)
               {
                  if (p->index[i] != -1)
                  {
                     index = p->index[i];

                     distance = (waypoints[row].origin - waypoints[index].origin).Length();
                     
                     if(waypoints[row].flags & W_FL_DELETED || waypoints[index].flags & W_FL_DELETED)
                     {
                        UTIL_ConsolePrintf("[matrix calc] - ERROR: Path to deleted waypoint detected.\n");
                     }

                     if (distance > WAYPOINT_MAX_DISTANCE)
                        distance = WAYPOINT_MAX_DISTANCE;

                     if (!(waypoints[row].flags & W_FL_LIFT_START && waypoints[index].flags & W_FL_LIFT_END) &&
                     	distance > REACHABLE_RANGE)
                     {
                        UTIL_ConsolePrintf("[matrix calc] - Waypoint path distance %4.1f > %4.1f at from %d to %d:\n", distance, (float)REACHABLE_RANGE, row, index);
                     }
                     else
                     {
                        offset = row * route_num_waypoints + index;
                        pShortestPath[offset] = (unsigned short)distance;
                     }
                  }

                  i++;
               }

               p = p->next;  // go to next node in linked list
            }
         }
      }

      // Initialize run of Floyd's Algorithm to generate the from_to matrix...
      wp_matrix_save_on_mapend = FALSE;
      wp_matrix_initialized = FALSE;
      
      // activate state machine
      slow_floyds.state = 0; 
   }
}


qboolean WaypointIsRouteValid(int src, int dest)
{
   if(unlikely(!wp_matrix_initialized) || 
      unlikely(from_to == NULL) || 
      unlikely(src < 0) ||
      unlikely(dest < 0) ||
      unlikely(src >= (int)route_num_waypoints) || 
      unlikely(dest >= (int)route_num_waypoints))
      return(FALSE);
   
   return(TRUE);
}


int WaypointRouteFromTo(int src, int dest)
{
   // new waypoints come effective on mapchange
   if(!WaypointIsRouteValid(src, dest))
      return WAYPOINT_UNREACHABLE;
   
   return (int)from_to[src * route_num_waypoints + dest];
}


float WaypointDistanceFromTo(int src, int dest)
{
   // new waypoints come effective on mapchange
   if(!WaypointIsRouteValid(src, dest))
      return (float)WAYPOINT_MAX_DISTANCE;
   
   return (float)(shortest_path[src * route_num_waypoints + dest]);
}
