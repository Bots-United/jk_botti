// jk_botti - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// waypoint.cpp
//

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

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "waypoint.h"
#include "bot_weapon_select.h"


extern int m_spriteTexture;
extern int IsDedicatedServer;


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

qboolean g_waypoint_paths = FALSE;  // have any paths been allocated?
qboolean g_waypoint_on = FALSE;
qboolean g_auto_waypoint = FALSE;
qboolean g_path_waypoint = FALSE;
qboolean g_path_waypoint_enable = TRUE;
qboolean g_waypoint_updated = FALSE;
int last_waypoint[32];
float f_path_time = 0.0;

unsigned int route_num_waypoints;
unsigned short *shortest_path = NULL;
unsigned short *from_to = NULL;

static FILE *fp;


void WaypointDebug(void)
{
   int y = 1, x = 1;

   fp=fopen("jk_botti.txt","a");
   fprintf(fp,"WaypointDebug: LINKED LIST ERROR!!!\n");
   fclose(fp);

   x = x - 1;  // x is zero
   y = y / x;  // cause an divide by zero exception

   return;
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

   for (i=0; i < MAX_WAYPOINTS; i++)
   {
      waypoints[i].flags = 0;
      waypoints[i].origin = Vector(0,0,0);
      waypoints[i].itemflags = 0;

      wp_display_time[i] = 0.0;

      paths[i] = NULL;  // no paths allocated yet
   }

   f_path_time = 0.0;  // reset waypoint path display time

   num_waypoints = 0;

   for(i = 0; i < 32; i++)
      last_waypoint[i] = -1;

   shortest_path = NULL;
   from_to = NULL;
   
   memset(&spawnpoints, 0, sizeof(spawnpoints));
   num_spawnpoints = 0;
}


void WaypointAddPath(short int add_index, short int path_index)
{
   PATH *p, *prev;
   int i;

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

   p = (PATH *)malloc(sizeof(PATH));

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


// Called when map-objects spawn
void CollectMapSpawnItems(edict_t *pSpawn)
{
   char *classname = (char *)STRING(pSpawn->v.classname);
   int flags = 0;
   int i;
   Vector SpawnOrigin;
   int itemflag = 0;
   
   if(num_spawnpoints >= MAX_WAYPOINTS / 2)
      return;
   
   if (strncmp("item_health", classname, 11) == 0)
      flags |= W_FL_HEALTH;
   else if (strncmp("item_armor", classname, 10) == 0)
      flags |= W_FL_ARMOR;
   else if ((itemflag = GetAmmoItemFlag(classname)) != 0)
      flags |= W_FL_AMMO;
   else if ((itemflag = GetWeaponItemFlag(classname)) != 0 && (pSpawn->v.owner == NULL))
      flags |= W_FL_WEAPON;
   else if (strcmp("item_longjump", classname) == 0)
      flags |= W_FL_LONGJUMP;
   else
      return;
   
   // Trace item on ground
   TraceResult tr;
   UTIL_TraceHull( pSpawn->v.origin + Vector(0, 0, 36 / 2), pSpawn->v.origin - Vector( 0, 0, 10240 ), 
      ignore_monsters, head_hull, pSpawn->v.pContainingEntity, &tr );
   
   //stuck in world?
   if(tr.fStartSolid) 
   {
      //SERVER_PRINT("stuck in world\n");
      return;
   }
   
   SpawnOrigin = tr.vecEndPos - Vector(0, 0, 36 / 2) + Vector(0, 0, 72 / 2);
   
   // Check if there is enough space to stand
   TraceResult tr2;
   UTIL_TraceHull( SpawnOrigin, SpawnOrigin + Vector(0, 0, 1),
      ignore_monsters, human_hull, pSpawn->v.pContainingEntity, &tr2 );
      
   if(tr2.fStartSolid || tr2.flFraction < 0.9999 )
   {
      // Not enough space, make this duck waypoint
      SpawnOrigin = tr.vecEndPos;
      flags |= W_FL_CROUCH;
      
      //SERVER_PRINT("add crouch waypoint\n");
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
         return;
      }
   }
   
   // Add new
   spawnpoints[num_spawnpoints].origin = SpawnOrigin;
   spawnpoints[num_spawnpoints].flags = flags;
   spawnpoints[num_spawnpoints].itemflags = itemflag;
   num_spawnpoints++;
}


//
void WaypointAddSpawnObjects(void)
{
   int index;
   int i,j,k;
   qboolean done;
   int count = 0;
   int updated_count = 0;

   if (!g_auto_waypoint || num_spawnpoints == 0)
      return;

   for(i = 0; i < num_spawnpoints; i++) 
   {
      done = FALSE;
      
      // Check if there is waypoint near already
      for(j = 0; j < num_waypoints; j++) {
         if (!(waypoints[j].flags & W_FL_DELETED)) 
         {
            if((waypoints[j].origin - spawnpoints[i].origin).Length() < 50.0) 
            {
               // just add flags and return
               if(waypoints[j].flags != spawnpoints[i].flags || waypoints[j].itemflags != spawnpoints[i].itemflags)
               	  updated_count++;
               
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
      if(!g_path_waypoint_enable)
      	 continue;
      
      for (k=0; k < num_waypoints; k++)
      {
         if (k == index)
            continue;  // skip the waypoint that was just added

         if (waypoints[k].flags & W_FL_AIMING)
            continue;  // skip any aiming waypoints

         // check if the waypoint is reachable from the new one (one-way)
         if ( WaypointReachable(waypoints[index].origin, waypoints[k].origin, NULL) )
         {
            WaypointAddPath(index, k);
         }
 
         // check if the new one is reachable from the waypoint (other way)
         if ( WaypointReachable(waypoints[k].origin, waypoints[index].origin, NULL) )
         {
            WaypointAddPath(k, index);
         }
      }
   }
   
   if(count || updated_count) 
   {
      UTIL_ConsolePrintf("Added total %d map-object based waypoints and updated flags for %d!\n", count, updated_count);
      
      g_waypoint_updated = TRUE;
   }
   
   num_spawnpoints = 0;
   memset(&spawnpoints, 0, sizeof(spawnpoints));
   
   if(count && shortest_path == NULL && from_to == NULL)
      WaypointRouteInit();
}


// find the nearest waypoint to the player and return the index (-1 if not found)
int WaypointFindNearest(edict_t *pEntity, float range)
{
   int i, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 9999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[i].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceMove( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            min_index = i;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


// find the nearest waypoint to the source postition and return the index
int WaypointFindNearest(Vector v_src, edict_t *pEntity, float range)
{
   int index, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 9999.0;

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from source position...
         UTIL_TraceMove( v_src, waypoints[index].origin, ignore_monsters,
                              pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            min_index = index;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags)
{
   int index, min_index;
   int distance, min_distance;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      if (waypoints[index].flags & (W_FL_LONGJUMP | W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }

      distance = WaypointDistanceFromTo(src, index);

      if (distance < min_distance)
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int itemflags)
{
   int index, min_index;
   int distance, min_distance;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      if (itemflags && !(waypoints[index].itemflags & itemflags))
      	 continue;  // skip this weapoint if no match for itemflags

      if (waypoints[index].flags & (W_FL_LONGJUMP | W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }

      distance = WaypointDistanceFromTo(src, index);

      if (distance < min_distance)
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int exclude[])
{
   int index, min_index;
   int distance, min_distance;
   int exclude_index;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      exclude_index = 0;
      while (exclude[exclude_index])
      {
         if (index == exclude[exclude_index])
            break;  // found a match, break out of while loop

         exclude_index++;
      }

      if (index == exclude[exclude_index])
         continue;  // skip any index that matches exclude list
      
      if (waypoints[index].flags & (W_FL_LONGJUMP| W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);   
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }
      
      distance = WaypointDistanceFromTo(src, index);

      if (distance < min_distance)
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindNearestGoal(Vector v_src, edict_t *pEntity, float range, int flags)
{
   int index, min_index;
   float distance, min_distance;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint with the matching flags...

   min_index = -1;
   min_distance = 99999;

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match
      
      if (waypoints[index].flags & (W_FL_LONGJUMP| W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);   
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }
      
      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < range) && (distance < min_distance))
      {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}


int WaypointFindRandomGoal(edict_t *pEntity, int flags)
{
   int index;
   int indexes[50];
   int count = 0;

   if (num_waypoints < 1)
      return -1;

   // find all the waypoints with the matching flags...

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      if (waypoints[index].flags & (W_FL_LONGJUMP| W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);   
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }
      
      if (count < 50)
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return -1;

   index = RANDOM_LONG2(1, count) - 1;

   return indexes[index];
}


int WaypointFindRandomGoal(edict_t *pEntity, int flags, int exclude[])
{
   int index;
   int indexes[200];
   int count = 0;
   int exclude_index;

   if (num_waypoints < 1)
      return -1;

   // find all the waypoints with the matching flags...

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match
      
      if (waypoints[index].flags & (W_FL_LONGJUMP| W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);   
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }
      
      exclude_index = 0;
      while (exclude[exclude_index])
      {
         if (index == exclude[exclude_index])
            break;  // found a match, break out of while loop

         exclude_index++;
      }

      if (index == exclude[exclude_index])
         continue;  // skip any index that matches exclude list

      if (count < 200)
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return -1;

   index = RANDOM_LONG2(1, count) - 1;

   return indexes[index];
}


int WaypointFindRandomGoal(Vector v_src, edict_t *pEntity, float range, int flags)
{
   int index;
   int indexes[200];
   int count = 0;
   float distance;

   if (num_waypoints < 1)
      return -1;

   // find all the waypoints with the matching flags...

   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[index].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match
      
      if (waypoints[index].flags & (W_FL_LONGJUMP| W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      {
         edict_t *wpt_item = WaypointFindItem(index);   
         if ((wpt_item == NULL) || (wpt_item->v.effects & EF_NODRAW) || (wpt_item->v.frame > 0))
            continue;
      }
      
      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < range) && (count < 200))
      {
         indexes[count] = index;

         count++;
      }
   }

   if (count == 0)  // no matching waypoints found
      return -1;

   index = RANDOM_LONG2(1, count) - 1;

   return indexes[index];
}


int WaypointFindNearestAiming(Vector v_origin)
{
   int index;
   int min_index = -1;
   float min_distance = 9999.0;
   float distance;

   if (num_waypoints < 1)
      return -1;

   // search for nearby aiming waypoint...
   for (index=0; index < num_waypoints; index++)
   {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if ((waypoints[index].flags & W_FL_AIMING) == 0)
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


void WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed)
{
   // should waypoints be visible?
   if(!g_waypoint_on)
      return;
   
   MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
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


void WaypointSearchItems(edict_t *pEntity, Vector origin, int wpt_index)
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

   min_distance = 9999.0;

   //********************************************************
   // look for the nearest health, armor, ammo, weapon, etc.
   //********************************************************

   while ((pent = UTIL_FindEntityInSphere( pent, origin, radius )) != NULL)
   {
      UTIL_TraceMove( origin, pent->v.origin, ignore_monsters,  
                      pEntity ? pEntity->v.pContainingEntity : NULL, &tr );

      // make sure entity is visible...
      if (tr.flFraction >= 1.0)
      {
         strcpy(item_name, STRING(pent->v.classname));

         if ((strncmp("item_health", item_name, 11) == 0) ||
             (strncmp("item_armor", item_name, 10) == 0) ||
             (GetAmmoItemFlag(item_name) != 0) ||
             (strcmp("item_longjump", item_name) == 0) ||
             ((GetWeaponItemFlag(item_name) != 0) &&
              (pent->v.owner == NULL)))
         {
            distance = (pent->v.origin - origin).Length();

            if (distance < min_distance)
            {
               strcpy(nearest_name, item_name);

               nearest_pent = pent;

               min_distance = distance;
            }
         }
      }
   }

   if (nearest_name[0])  // found an entity name
   {
      if (strncmp("item_health", nearest_name, 11) == 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs("found a healthkit! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_HEALTH;
      }

      if (strncmp("item_armor", nearest_name, 10) == 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs("found some armor! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_ARMOR;
      }

      if ((itemflag = GetAmmoItemFlag(nearest_name)) != 0)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs("found some ammo! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_AMMO;
         waypoints[wpt_index].itemflags |= itemflag;
      }

      if ((itemflag = GetWeaponItemFlag(nearest_name)) != 0 && nearest_pent->v.owner == NULL)
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs("found a weapon! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_WEAPON;
         waypoints[wpt_index].itemflags |= itemflag;
      }
      
      if ((strcmp("item_longjump", nearest_name) == 0))
      {
         if (pEntity)
            ClientPrint(pEntity, HUD_PRINTCONSOLE, UTIL_VarArgs("found a longjump! (%s)\n", nearest_name));
         waypoints[wpt_index].flags |= W_FL_LONGJUMP;
      }
   }
   
   if(flags_before != waypoints[wpt_index].flags || itemflags_before != waypoints[wpt_index].itemflags)
   {  // save on mapchange
      g_waypoint_updated = TRUE;
   }
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
   
   min_distance = 9999.0;
   
   Vector origin = waypoints[wpt_index].origin;
   //********************************************************
   // look for the nearest health, armor, ammo, weapon, etc.
   //********************************************************
   while ((pent = UTIL_FindEntityInSphere( pent, origin, radius )) != NULL)
   {
      UTIL_TraceMove( origin, pent->v.origin, ignore_monsters, NULL, &tr );
      
      // make sure entity is visible...
      if ((tr.flFraction >= 1.0) || (tr.pHit == pent) || !(pent->v.effects & EF_NODRAW) || !(pent->v.frame > 0))
      {
         strcpy(item_name, STRING(pent->v.classname));
         
         if(((waypoints[wpt_index].flags & W_FL_HEALTH) && strncmp("item_health", item_name, 11) == 0) ||
            ((waypoints[wpt_index].flags & W_FL_ARMOR) && strncmp("item_armor", item_name, 10) == 0) ||
            ((waypoints[wpt_index].flags & W_FL_AMMO) && GetAmmoItemFlag(item_name) != 0) ||
            ((waypoints[wpt_index].flags & W_FL_LONGJUMP) && strcmp("item_longjump", item_name) == 0) ||
            ((waypoints[wpt_index].flags & W_FL_WEAPON) && GetWeaponItemFlag(item_name) != 0 && pent->v.owner == NULL))
         {
            distance = (pent->v.origin - origin).Length();
            
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
   const float globaltime = gpGlobals->time;
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
      last_waypoint[player_index] = index;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = globaltime;


   Vector start, end;

   start = pEntity->v.origin - Vector(0, 0, 34);
   end = start + Vector(0, 0, 68);

   if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING)
   {
      waypoints[index].flags |= W_FL_CROUCH;  // crouching waypoint

      start = pEntity->v.origin - Vector(0, 0, 17);
      end = start + Vector(0, 0, 34);
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
   if(!g_path_waypoint_enable)
      return;
   
   for (int i=0; i < num_waypoints; i++)
   {
      if (i == index)
         continue;  // skip the waypoint that was just added

      if (waypoints[i].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      // check if the waypoint is reachable from the new one (one-way)
      if ( WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity) )
      {
         WaypointAddPath(index, i);
      }

      // check if the new one is reachable from the waypoint (other way)
      if ( WaypointReachable(waypoints[i].origin, pEntity->v.origin, pEntity) )
      {
         WaypointAddPath(i, index);
      }
   }
}


void WaypointAddAiming(edict_t *pEntity)
{
   const float globaltime = gpGlobals->time;
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
   wp_display_time[index] = globaltime;


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

   if ((waypoints[index].flags & W_FL_SNIPER) ||
       (waypoints[index].flags & W_FL_JUMP))
   {
      int i;
      int min_index = -1;
      float min_distance = 9999.0;
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


qboolean WaypointLoad(edict_t *pEntity)
{
   FILE *bfp;
   char mapname[64];
   char filename[256];
   WAYPOINT_HDR header;
   char msg[80];
   int index, i;
   short int num;
   short int path_index;
   
   g_waypoint_updated = FALSE;
   
   strcpy(mapname, STRING(gpGlobals->mapname));
   strcat(mapname, ".wpt");

   UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);

   UTIL_ConsolePrintf("loading waypoint file: %s\n", filename);

   bfp = fopen(filename, "rb");

   // if file exists, read the waypoint structure from it
   if (bfp != NULL)
   {
      fread(&header, sizeof(header), 1, bfp);

      header.filetype[7] = 0;
      if (strcmp(header.filetype, WAYPOINT_MAGIC) == 0)
      {
         if (header.waypoint_file_version != WAYPOINT_VERSION)
         {
            UTIL_ConsolePrintf("Incompatible jk_botti waypoint file version!\n");
            UTIL_ConsolePrintf("Waypoints not loaded!\n");

            fclose(bfp);
            return FALSE;
         }
         
         // Handle subversions here (header.waypoint_file_subversion), fixup old versions
         //  1: Original, didn't use __reserved slots (int __reserved[4])

         header.mapname[31] = 0;

         if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
         {
            WaypointInit();  // remove any existing waypoints

            for (i=0; i < header.number_of_waypoints; i++)
            {
               fread(&waypoints[i], sizeof(waypoints[0]), 1, bfp);
               num_waypoints++;
            }

            // read and add waypoint paths...
            for (index=0; index < num_waypoints; index++)
            {
               // read the number of paths from this node...
               fread(&num, sizeof(num), 1, bfp);

               for (i=0; i < num; i++)
               {
                  fread(&path_index, sizeof(path_index), 1, bfp);

                  WaypointAddPath(index, path_index);
               }
            }

            g_waypoint_paths = TRUE;  // keep track so path can be freed
         }
         else
         {
            if (pEntity)
            {
               snprintf(msg, sizeof(msg), "%s jk_botti waypoints are not for this map!\n", filename);
               ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
            }

            fclose(bfp);
            return FALSE;
         }
      }
      else
      {
         if (pEntity)
         {
            snprintf(msg, sizeof(msg), "%s is not a jk_botti waypoint file!\n", filename);
            ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
         }

         fclose(bfp);
         return FALSE;
      }

      fclose(bfp);

      WaypointRouteInit();
   }
   else
   {
      if (pEntity)
      {
         snprintf(msg, sizeof(msg), "Waypoint file %s does not exist!\n", filename);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
      }

      if (IsDedicatedServer)
         UTIL_ConsolePrintf("waypoint file %s not found!\n", filename);

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


void WaypointSave(void)
{
   char filename[256];
   char mapname[64];
   WAYPOINT_HDR header;
   int index, i;
   short int num;
   PATH *p;

   strcpy(header.filetype, WAYPOINT_MAGIC);

   header.waypoint_file_version = WAYPOINT_VERSION;
   header.waypoint_file_subversion = 1;

   header.waypoint_file_flags = 0;  // not currently used

   header.number_of_waypoints = num_waypoints;

   memset(header.mapname, 0, sizeof(header.mapname));
   strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
   header.mapname[31] = 0;

   strcpy(mapname, STRING(gpGlobals->mapname));
   strcat(mapname, ".wpt");

   UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);

   UTIL_ConsolePrintf("saving waypoint file: %s\n", filename);

   FILE *bfp = fopen(filename, "wb");

   // write the waypoint header to the file...
   fwrite(&header, sizeof(header), 1, bfp);

   // write the waypoint data to the file...
   for (index=0; index < num_waypoints; index++)
   {
      fwrite(&waypoints[index], sizeof(waypoints[0]), 1, bfp);
   }

   // save the waypoint paths...
   for (index=0; index < num_waypoints; index++)
   {
      // count the number of paths from this node...
      num = WaypointNumberOfPaths(index);

      fwrite(&num, sizeof(num), 1, bfp);  // write the count

      // now write out each path index...

      p = paths[index];

      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] != -1)  // save path node if it's used
               fwrite(&p->index[i], sizeof(p->index[0]), 1, bfp);

            i++;
         }

         p = p->next;  // go to next node in linked list
      }
   }

   fclose(bfp);
}


qboolean WaypointReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
   TraceResult tr;
   float curr_height, last_height;
   qboolean on_ladder = pEntity ? ((pEntity->v.movetype == MOVETYPE_FLY) ? TRUE : FALSE) : FALSE;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < REACHABLE_RANGE)
   {
      // check if this waypoint is "visible"...
      if(on_ladder)
         UTIL_TraceMove( v_src, v_dest, ignore_monsters,  
                      !pEntity ? NULL : pEntity->v.pContainingEntity, &tr );
      else
         UTIL_TraceHull( v_src, v_dest, ignore_monsters, head_hull, 
                      !pEntity ? NULL : pEntity->v.pContainingEntity, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0)
      {
      	 // special ladder case, if both on same ladder -> ok
      	 if(on_ladder && (v_src.Make2D() - v_dest.Make2D()).Length() <= 32.0)
      	 {
      	    return TRUE;
      	 }
      	
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

            UTIL_TraceHull(v_new_src, v_new_dest, dont_ignore_monsters, head_hull, 
                           !pEntity ? NULL : pEntity->v.pContainingEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
            {
               return FALSE;  // can't reach this one
            }
         }

         // check if distance to ground increases more than jump height
         // at points between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceMove(v_check, v_down, ignore_monsters,  
                        !pEntity ? NULL : pEntity->v.pContainingEntity, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0)
         {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceMove(v_check, v_down, ignore_monsters, 
                           !pEntity ? NULL : pEntity->v.pContainingEntity, &tr);

            curr_height = tr.flFraction * 1000.0;  // height from ground

            // is the difference in the last height and the current height
            // higher that the jump height?
            if ((last_height - curr_height) > 45.0)
            {
               // can't get there from here...
               return FALSE;
            }

            last_height = curr_height;

            distance = (v_dest - v_check).Length();  // distance from goal
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

   min_distance = 9999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if (waypoints[i].flags & W_FL_AIMING)
         continue;  // skip any aiming waypoints

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if (distance < min_distance)
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceMove( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
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


void WaypointPrintInfo(edict_t *pEntity)
{
   char msg[80];
   int index;
   int flags;

   // find the nearest waypoint...
   index = WaypointFindNearest(pEntity, 50.0);

   if (index == -1)
      return;

   snprintf(msg, sizeof(msg), "Waypoint %d of %d total\n", index, num_waypoints);
   ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

   flags = waypoints[index].flags;

   if (flags & W_FL_LIFT)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "Bot will wait for lift before approaching\n");

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

   if (flags & W_FL_SNIPER)
      ClientPrint(pEntity, HUD_PRINTNOTIFY, "This is a sniper waypoint\n");
}


void WaypointThink(edict_t *pEntity)
{
   const float globaltime = gpGlobals->time;
   float distance, min_distance;
   Vector start, end;
   int i, idx, index=0;

   idx = ENTINDEX(pEntity) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return;

   if (g_auto_waypoint)  // is auto waypoint on?
   {
      // find the distance from the last used waypoint
      if(last_waypoint[idx] == -1)
         distance = 9999.0;
      else
      	 distance = (waypoints[last_waypoint[idx]].origin - pEntity->v.origin).Length();

      if (distance >= 200.0)
      {
         int onepath_wpt = -1;
         min_distance = 9999.0;

         // check that no other reachable waypoints are nearby...
         for (i=0; i < num_waypoints; i++)
         {
            if (waypoints[i].flags & W_FL_DELETED)
               continue;

            if (waypoints[i].flags & W_FL_AIMING)
               continue;

            if (!WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
               continue;
            
            distance = (waypoints[i].origin - pEntity->v.origin).Length();
            if (distance < min_distance) 
            {
               min_distance = distance;
            }
            
            // special code that does linking of two waypoints that are unreachable to each other
            // and are reachable to this location. Other end is waypoint with one or none path.
            if(distance < 200.0 && distance >= 64.0 && onepath_wpt == -1 && i != last_waypoint[idx] && WaypointNumberOfPaths(i) <= 1)
            {
               onepath_wpt = i;
            }
         }

         // make sure nearest waypoint is far enough away...
         if (min_distance >= 200.0) 
         {
            WaypointAdd(pEntity);
         }
         else if(onepath_wpt != -1 && min_distance >= 64.0)
         {
            // special code that does linking of two waypoints that are unreachable to each other
            // and are reachable to this location. Other end is waypoint with one or none path.
            for (i=0; i < num_waypoints; i++)
            {
               if (waypoints[i].flags & W_FL_DELETED)
                  continue;

               if (waypoints[i].flags & W_FL_AIMING)
                  continue;

               // distance between isn't too much
               if ((waypoints[i].origin - waypoints[onepath_wpt].origin).Length() >= 350.0)

               // need to be reachable to current player location
               if (!WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
                  continue;

               // waypoints are unreachable to each other
               if (!WaypointReachable(waypoints[i].origin, waypoints[onepath_wpt].origin, pEntity))
               {
                  WaypointAdd(pEntity);
                     
                  break;
               }
            }
         }
      }
   }

   min_distance = 9999.0;

   if (g_waypoint_on)  // display the waypoints if turned on...
   {
      for (i=0; i < num_waypoints; i++)
      {
         if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
            continue;

         distance = (waypoints[i].origin - pEntity->v.origin).Length();

         if (distance < 500)
         {
            if (distance < min_distance)
            {
               index = i; // store index of nearest waypoint
               min_distance = distance;
            }

            if ((wp_display_time[i] + 1.0) < globaltime)
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

               // draw a blue waypoint
               WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

               wp_display_time[i] = globaltime;
            }
         }
      }

      // check if path waypointing is on...
      if (g_path_waypoint)
      {
         // check if player is close enough to a waypoint and time to draw path...
         if ((min_distance <= 50) && (f_path_time <= globaltime))
         {
            PATH *p;

            f_path_time = globaltime + 1.0;

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


void WaypointFloyds(unsigned short *shortest_path, unsigned short *from_to)
{
   unsigned int x, y, z;
   int changed = 1;
   int distance;

   for (y=0; y < route_num_waypoints; y++)
   {
      for (z=0; z < route_num_waypoints; z++)
      {
         from_to[y * route_num_waypoints + z] = z;
      }
   }

   while (changed)
   {
      changed = 0;

      for (x=0; x < route_num_waypoints; x++)
      {
         for (y=0; y < route_num_waypoints; y++)
         {
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
}


void WaypointRouteInit(void)
{
   unsigned int index;
   unsigned int array_size;
   unsigned int row;
   int i, offset;
   unsigned int a, b;
   float distance;
   unsigned short *pShortestPath, *pFromTo;
   unsigned int num_items;
   FILE *bfp;
   char filename2[256];
   char mapname[64];
   int file1, file2;
   struct stat stat1, stat2;
   char header[16];

   if (num_waypoints == 0)
      return;

   // save number of current waypoints in case waypoints get added later
   route_num_waypoints = num_waypoints;
   array_size = route_num_waypoints * route_num_waypoints;

   //
   strcpy(mapname, STRING(gpGlobals->mapname));
   strcat(mapname, ".matrix");
   UTIL_BuildFileName_N(filename2, sizeof(filename2), "addons/jk_botti/waypoints", mapname);

   if (access(filename2, 0) == 0)  // does the ".matrix" file exist?
   {
      char filename[256];
      
      //
      strcpy(mapname, STRING(gpGlobals->mapname));
      strcat(mapname, ".wpt");
      UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", mapname);

      file1 = open(filename, O_RDONLY);
      file2 = open(filename2, O_RDONLY);

      fstat(file1, &stat1);
      fstat(file2, &stat2);

      close(file1);
      close(file2);

      if (stat1.st_mtime < stat2.st_mtime)  // is ".wpt" older than ".matrix" file?
      {
         UTIL_ConsolePrintf("[matrix load] - loading jk_botti waypoint path matrix\n");

         shortest_path = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

         if (shortest_path == NULL)
            UTIL_ConsolePrintf("[matrix load] - Error allocating memory for shortest path!\n");

         from_to = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

         if (from_to == NULL)
            UTIL_ConsolePrintf("[matrix load] - Error allocating memory for from to matrix!\n");

         bfp = fopen(filename2, "rb");

         if (bfp != NULL)
         {
            // first try read header 'jkbotti_matrixA\0', 16 bytes
            num_items = fread(header, 1, 16, bfp);
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
               num_items = fread(shortest_path, sizeof(unsigned short), array_size, bfp);

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
                  num_items = fread(header, 1, 16, bfp);
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
                     num_items = fread(from_to, sizeof(unsigned short), array_size, bfp);
   
                     if (num_items != array_size)
                     {
                        // if couldn't read enough data, free memory to recalculate it
                        UTIL_ConsolePrintf("[matrix load] - error reading enough path items, recalculating...\n");

                        free(shortest_path);
                        shortest_path = NULL;

                        free(from_to);
                        from_to = NULL;
                     }
                  }
               }
            }
         }
         else
         {
            UTIL_ConsolePrintf("[matrix load] - Error reading waypoint paths!\n");

            free(shortest_path);
            shortest_path = NULL;

            free(from_to);
            from_to = NULL;
         }

         fclose(bfp);
      }
      else
      {
      	 UTIL_ConsolePrintf("[matrix load] - Waypoint file is newer than matrix file, recalculating...\n");
      }
   }

   if (shortest_path == NULL)
   {
      UTIL_ConsolePrintf("[matrix calc] - calculating jk_botti waypoint path matrix\n");

      shortest_path = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

      if (shortest_path == NULL)
         UTIL_ConsolePrintf("[matrix calc] - Error allocating memory for shortest path!\n");

      from_to = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

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

                     if (distance > (float)WAYPOINT_MAX_DISTANCE)
                        distance = (float)WAYPOINT_MAX_DISTANCE;

                     if (distance > REACHABLE_RANGE)
                     {
                        UTIL_ConsolePrintf("[matrix calc] - Waypoint path distance > %4.1f at from %d to %d\n",
                                     REACHABLE_RANGE, row, index);
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

      // run Floyd's Algorithm to generate the from_to matrix...
      WaypointFloyds(pShortestPath, pFromTo);

      for (a=0; a < route_num_waypoints; a++)
      {
         for (b=0; b < route_num_waypoints; b++)
            if (pShortestPath[a * route_num_waypoints + b] == WAYPOINT_UNREACHABLE)
              pFromTo[a * route_num_waypoints + b] = WAYPOINT_UNREACHABLE;
      }

      bfp = fopen(filename2, "wb");

      if (bfp != NULL)
      {
      	 num_items = fwrite("jkbotti_matrixA\0", 1, 16, bfp);
      	 if (num_items != 16)
      	 {
            // if couldn't write enough data, close file and delete it
            fclose(bfp);
            unlink(filename2);
         }
         else
         {
            num_items = fwrite(shortest_path, sizeof(unsigned short), array_size, bfp);

            if (num_items != array_size)
            {
               // if couldn't write enough data, close file and delete it
               fclose(bfp);
               unlink(filename2);
            }
            else
            {
               num_items = fwrite("jkbotti_matrixB\0", 1, 16, bfp);
               if (num_items != 16)
      	       {
                  // if couldn't write enough data, close file and delete it
                  fclose(bfp);
                  unlink(filename2);
               }
               else
               {            	
                  num_items = fwrite(from_to, sizeof(unsigned short), array_size, bfp);

                  fclose(bfp);

                  if (num_items != array_size)
                  {
                     // if couldn't write enough data, delete file
                     unlink(filename2);
                  }
               }
            }
         }
      }
      else
      {
         UTIL_ConsolePrintf("[matrix calc] - Error writing waypoint paths!\n");
      }

      UTIL_ConsolePrintf("[matrix calc] - waypoint path calculations complete!\n");
   }
}


unsigned short WaypointRouteFromTo(int src, int dest)
{
   // new waypoints come effective on mapchange
   if(from_to == NULL || src >= (int)route_num_waypoints || dest >= (int)route_num_waypoints)
      return WAYPOINT_UNREACHABLE;
   
   return from_to[src * route_num_waypoints + dest];
}


int WaypointDistanceFromTo(int src, int dest)
{
   // new waypoints come effective on mapchange
   if(shortest_path == NULL || src >= (int)route_num_waypoints || dest >= (int)route_num_waypoints)
      return WAYPOINT_MAX_DISTANCE;
   
   return (int)(shortest_path[src * route_num_waypoints + dest]);
}
