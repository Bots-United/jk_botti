//
// JK_Botti - be more human!
//
// waypoint.h
//

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <limits.h>

#define MAX_WAYPOINTS 1024

#define REACHABLE_RANGE 400.0f

// defines for waypoint flags field (32 bits are available)
#define W_FL_CROUCH      (1<<1)  /* must crouch to reach this waypoint */
#define W_FL_JUMP        (1<<2) /* jump waypoint */
#define W_FL_LADDER      (1<<3)  /* waypoint on a ladder */
//#define W_FL_OLDLIFT     (1<<4)  /* wait for lift to be down before approaching this waypoint */
#define W_FL_DOOR        (1<<5)  /* wait for door to open */

#define W_FL_LONGJUMP    (1<<6)  /* item (longjump) */
#define W_FL_HEALTH      (1<<7)  /* health kit (or wall mounted) location */
#define W_FL_ARMOR       (1<<8)  /* armor (or HEV) location */
#define W_FL_AMMO        (1<<9)  /* ammo location */
#define W_FL_WEAPON      (1<<10) /* weapon_ entity location */

#define W_FL_SNIPER      (1<<11) /* sniper waypoint (a good sniper spot) */
#define W_FL_AIMING      (1<<12) /* aiming waypoint */

#define W_FL_SPAWNADD    (1<<13) /* waypoint was added by spawn-object, marked to not have these trimmed on wp-save */

#define W_FL_LIFT_START  (1<<14) /* handled as normal waypoint + one path to lift-end waypoint */
#define W_FL_LIFT_END    (1<<15) /* handled otherwise as normal waypoint except only one incoming path from lift-start,
                                    other incoming paths are ignored (at creating time of other waypoints). */

#define W_FL_DELETED     (1<<31) /* used by waypoint allocation code */


#define WAYPOINT_VERSION 5
#define WAYPOINT_MAGIC "jkbotti\0"


//
// Subversions
//
#define WPF_SUBVERSION_1_00 1 // jk_botti 0.30-1.00
#define WPF_SUBVERSION_1_02 2 // jk_botti 1.02-1.10
#define WPF_SUBVERSION_1_10 3 // jk_botti 1.10-


// define the waypoint file header structure...
typedef struct {
   char filetype[8];  // should be "jkbotti\0"
   int  waypoint_file_version;
   int  waypoint_file_subversion;
   int  waypoint_file_flags;
   int  number_of_waypoints;
   char mapname[32];  // name of map for these waypoints
} WAYPOINT_HDR;


// define the structure for waypoints... size must be 4*8 bytes
typedef struct {
   int flags;     // button, lift, flag, health, ammo, etc.
   Vector origin; // location
   int itemflags; // different flags for different ammo/weapons
   int __reserved[3];
} WAYPOINT;

#define WAYPOINT_UNREACHABLE  (USHRT_MAX)
#define WAYPOINT_MAX_DISTANCE (USHRT_MAX-1)

#define MAX_PATH_INDEX 4

// define the structure for waypoint paths (paths are connections between
// two waypoint nodes that indicates the bot can get from point A to point B.
// note that paths DON'T have to be two-way.  sometimes they are just one-way
// connections between two points.  There is an array called "paths" that
// contains head pointers to these structures for each waypoint index.
typedef struct path {
   short int index[MAX_PATH_INDEX];  // indexes of waypoints (index -1 means not used)
   struct path *next;   // link to next structure
} PATH;


// waypoint function prototypes...
void WaypointInit(void);
int  WaypointFindPath(PATH **pPath, int *path_index, int waypoint_index);
int  WaypointFindNearest(edict_t *pEntity, float distance);
int  WaypointFindNearest(Vector v_src, edict_t *pEntity, float range);
int  WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int itemflags, int exclude[]);
int  WaypointFindNearestGoal(Vector v_src, edict_t *pEntity, float range, int flags);
int  WaypointFindRandomGoal(edict_t *pEntity, int flags, int itemflags, int exclude[]);
int  WaypointFindRandomGoal(Vector v_src, edict_t *pEntity, float range, int flags);
int  WaypointFindNearestAiming(Vector v_origin);
void WaypointSearchItems(edict_t *pEntity, Vector origin, int wpt_index);
void WaypointAdd(edict_t *pEntity);
void WaypointAddAiming(edict_t *pEntity);
int  WaypointAddTesting(const Vector &vecOrigin, int flags, int itemflags, qboolean MakePaths);
void WaypointAddLift(edict_t * pent, const Vector &start, const Vector &end);
void WaypointDelete(edict_t *pEntity);
void WaypointUpdate(edict_t *pEntity);
void WaypointCreatePath(edict_t *pEntity, int cmd);
void WaypointRemovePath(edict_t *pEntity, int cmd);
qboolean WaypointLoad(edict_t *pEntity);
void WaypointSave(void);
qboolean WaypointReachable(const Vector &v_src, const Vector &v_dest, const int reachable_flags);
int  WaypointFindReachable(edict_t *pEntity, float range);
void WaypointPrintInfo(edict_t *pEntity);
void WaypointThink(edict_t *pEntity);
void WaypointFloyds(short *shortest_path, short *from_to);
void WaypointRouteInit(qboolean ForceRebuild);
void CollectMapSpawnItems(edict_t *pSpawn);
void WaypointAddSpawnObjects(void);
edict_t *WaypointFindItem( int wpt_index );
int WaypointRouteFromTo(int src, int dest);
float WaypointDistanceFromTo(int src, int dest);
void WaypointSaveFloydsMatrix(unsigned short *shortest_path, unsigned short *from_to);
void WaypointSaveFloydsMatrix(void);
void WaypointSlowFloydsStop(void);
int WaypointSlowFloydsState(void);
int WaypointSlowFloyds(void);
int WaypointSlowFloyds(unsigned short *shortest_path, unsigned short *from_to);
qboolean WaypointIsRouteValid(int src, int dest);
int WaypointFindRunawayPath(int runner, int enemy);

inline int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int exclude[])
{
   return WaypointFindNearestGoal(pEntity, src, flags, 0, exclude);
}

inline int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags, int itemflags)
{
   return WaypointFindNearestGoal(pEntity, src, flags, itemflags, NULL);
}

inline int WaypointFindNearestGoal(edict_t *pEntity, int src, int flags)
{
   return WaypointFindNearestGoal(pEntity, src, flags, 0, NULL);
}

inline int WaypointFindRandomGoal(edict_t *pEntity, int flags)
{
   return WaypointFindRandomGoal(pEntity, flags, 0, NULL); 
}

inline int WaypointFindRandomGoal(edict_t *pEntity, int flags, int exclude[]) 
{ 
   return WaypointFindRandomGoal(pEntity, flags, 0, exclude); 
}

// find the nearest waypoint to the player and return the index (-1 if not found)
inline int WaypointFindNearest(edict_t *pEntity, float range)
{
   return WaypointFindNearest(pEntity->v.origin + pEntity->v.view_ofs, pEntity, range);
}

#endif // WAYPOINT_H
