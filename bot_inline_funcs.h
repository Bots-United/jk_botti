//
// JK_Botti - be more human!
//
// bot_inline_funcs.h
//

#ifndef BOT_INLINE_FUNCS
#define BOT_INLINE_FUNCS

#include <inttypes.h>

inline Vector UTIL_AnglesToForward(const Vector &angles)
{
   // from pm_shared/pm_math.h
   float /*sr, cr, roll,*/ sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   sp = sin(pitch);
   cp = cos(pitch);

   yaw = angles.y * (M_PI*2 / 360);
   sy = sin(yaw);
   cy = cos(yaw);

   //roll = angles.z * (M_PI*2 / 360);
   //sr = sin(roll);
   //cr = cos(roll);

   return(Vector(cp*cy, cp*sy, -sp));
}

inline Vector UTIL_AnglesToRight(const Vector &angles)
{
   // from pm_shared/pm_math.h
   float sr, cr, roll, sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   sp = sin(pitch);
   cp = cos(pitch);

   yaw = angles.y * (M_PI*2 / 360);
   sy = sin(yaw);
   cy = cos(yaw);

   roll = angles.z * (M_PI*2 / 360);
   sr = sin(roll);
   cr = cos(roll);

   return(Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp));
}

inline void UTIL_MakeVectorsPrivate( const Vector &angles, Vector &v_forward, Vector &v_right, Vector &v_up )
{
   // from pm_shared/pm_math.h
   float sr, cr, roll, sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   sp = sin(pitch);
   cp = cos(pitch);

   yaw = angles.y * (M_PI*2 / 360);
   sy = sin(yaw);
   cy = cos(yaw);

   roll = angles.z * (M_PI*2 / 360);
   sr = sin(roll);
   cr = cos(roll);

   v_forward = Vector(cp*cy, cp*sy, -sp);
   v_right   = Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp);
   v_up      = Vector(cr*sp*cy+-sr*-sy, cr*sp*sy+-sr*cy, cr*cp);
}

inline Vector GetGunPosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}

inline void UTIL_SelectItem(edict_t *pEdict, char *item_name)
{
   FakeClientCommand(pEdict, item_name, NULL, NULL);
}

inline Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

// Overloaded to add IGNORE_GLASS
inline void UTIL_TraceLine( const Vector & vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}

inline void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}

inline void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr );
}

inline void UTIL_TraceMove( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr ) 
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), point_hull, pentIgnore, ptr );
}

inline edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (!FNullEnt(pentEntity))
      return pentEntity;

   return NULL;
}

inline edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
   edict_t *pentEntity;

   pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

   if (!FNullEnt(pentEntity))
      return pentEntity;
   
   return NULL;
}

inline edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "classname", szName );
}

inline edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "target", szName );
}

inline edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "targetname", szName );
}

inline double deg2rad(double deg) 
{
   return(deg * (M_PI / 180));
}

inline qboolean IsAlive(edict_t *pEdict) 
{
   return ((pEdict->v.deadflag == DEAD_NO) &&
           (pEdict->v.health > 0) &&
           !(pEdict->v.flags & FL_NOTARGET) &&
           (pEdict->v.takedamage != 0));
}

inline float UTIL_WrapAngle360(float angle)
{
   // this function returns an angle normalized to the range [0 <= angle < 360]
   const unsigned int bits = 0x80000000;
   return ((360.0 / bits) * ((int64_t)(angle * (bits / 360.0)) & (bits-1)));
}

inline float UTIL_WrapAngle(float angle)
{
   // this function returns angle normalized to the range [-180 < angle <= 180]
   angle = UTIL_WrapAngle360(angle);

   if (angle > 180.0)
      angle -= 360.0;

   return (angle);
}

inline Vector UTIL_WrapAngles(const Vector & angles)
{
   // check for wraparound of angles
   return Vector( UTIL_WrapAngle(angles.x), UTIL_WrapAngle(angles.y), UTIL_WrapAngle(angles.z) );
}

inline Vector UTIL_VecToAngles( const Vector & vec )
{
   Vector VecOut;
   VEC_TO_ANGLES(vec, VecOut);
   return UTIL_WrapAngles(VecOut);
}

inline void BotFixIdealPitch(edict_t *pEdict)
{
   pEdict->v.idealpitch = UTIL_WrapAngle(pEdict->v.idealpitch);
}

inline void BotFixIdealYaw(edict_t *pEdict)
{
   pEdict->v.ideal_yaw = UTIL_WrapAngle(pEdict->v.ideal_yaw);
}

inline Vector UTIL_GetOrigin(edict_t *pEdict)
{
   if (strncmp(STRING(pEdict->v.classname), "func_", 5) == 0)
           return VecBModelOrigin(pEdict);

   return pEdict->v.origin; 
}

inline int UTIL_GetBotIndex(edict_t *pEdict)
{
   int index;

   for (index=0; index < 32; index++)
      if (bots[index].pEdict == pEdict)
         return index;

   return -1;  // return -1 if edict is not a bot
}

inline bot_t *UTIL_GetBotPointer(edict_t *pEdict)
{
   int index;

   for (index=0; index < 32; index++)
      if (bots[index].pEdict == pEdict)
         return (&bots[index]);
   
   return NULL;  // return NULL if edict is not a bot
}

inline qboolean FInViewCone(const Vector & Origin, edict_t *pEdict)
{
   return(DotProduct((Origin - pEdict->v.origin).Normalize(), UTIL_AnglesToForward(pEdict->v.v_angle)) > cos(deg2rad(80)));
}

/* generates a random number on [0,0x7fffffff]-interval */
inline long genrand_int31(void)
{
   extern unsigned long minimalistic_randomness_idnum;
   minimalistic_randomness_idnum = 1664525L * minimalistic_randomness_idnum + 1013904223L;
   return(long)(minimalistic_randomness_idnum>>1);
}

/* generates a random number on [0,1]-real-interval */
inline double genrand_real1()
{
   return (double)genrand_int31() * (1.0/2147483647.0); 
   /* divided by 2^31-1 */ 
}

inline int RANDOM_LONG2(int lLow, int lHigh) 
{
   double rnd_diff, add;
   
   if(lLow >= lHigh)
      return(lLow);

   rnd_diff = genrand_real1() * (lHigh - lLow);
   add = 0.5;
      
   if(rnd_diff < 0)
      add = -0.5;
      
   return((int)(rnd_diff+add) + lLow);
}

inline float RANDOM_FLOAT2(float flLow, float flHigh) 
{
   if(flLow >= flHigh)
      return(flLow);
      
   return(genrand_real1() * (flHigh - flLow) + flLow);
}

#endif /*BOT_INLINE_FUNCS*/
