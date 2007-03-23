//
// JK_Botti - be more human!
//
// bot_inline_funcs.h
//

#ifndef BOT_INLINE_FUNCS
#define BOT_INLINE_FUNCS

#include <inttypes.h>

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

inline void WRAP_TraceLine(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr) {
   TRACE_LINE(v1, v2, fNoMonsters, pentToSkip, ptr);
}

// Overloaded to add IGNORE_GLASS
inline void UTIL_TraceLine( const Vector & vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
   WRAP_TraceLine( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}

inline void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   WRAP_TraceLine( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}

inline void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr );
}

inline void UTIL_TraceMove( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, /*int ducking,*/ edict_t *pentIgnore, TraceResult *ptr ) {
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), /*((!ducking) ? head_hull :*/ point_hull/*)*/, pentIgnore, ptr );
}

inline edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (!fast_FNullEnt(pentEntity))
      return pentEntity;

   return NULL;
}

inline edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
   edict_t *pentEntity;

   pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

   if (!fast_FNullEnt(pentEntity))
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
   Vector vForward, vRight, vUp;
   UTIL_MakeVectorsPrivate( pEdict->v.angles, vForward, vRight, vUp);
   return (DotProduct((Origin - pEdict->v.origin).Normalize(), vForward) > cos(deg2rad(80)));
}

extern void init_genrand(unsigned long s);
extern void mt_next_state(void);
extern int mt_left;
extern unsigned long *mt_next;

/* generates a random number on [0,1] with 53-bit resolution*/
inline double genrand_real1_res53(void)
{
   unsigned long a,b;

   if (mt_left > 2)
   {
      mt_left-=2;
      a = *mt_next++;
      b = *mt_next++;
   }
   else if (mt_left == 2)
   {
      a = *mt_next++;
      mt_next_state();
      b = *mt_next++;
   }
   else
   {
      mt_next_state();
      mt_left--;
      a = *mt_next++;
      b = *mt_next++;
   }

   /* Tempering */
   a ^= (a >> 11);
   b ^= (b >> 11);
   a ^= (a << 7) & 0x9d2c5680UL;
   b ^= (b << 7) & 0x9d2c5680UL;
   a ^= (a << 15) & 0xefc60000UL;
   b ^= (b << 15) & 0xefc60000UL;
   a ^= (a >> 18);
   b ^= (b >> 18);
   a >>= 5;
   b >>= 6;
   
   return(a*67108864.0+b)*(1.0/9007199254740991.0);
   /* divided by 2^53-1 */
}

inline int RANDOM_LONG2(int lLow, int lHigh) 
{
   double rnd_diff, add;
   
   if(lLow <= lHigh)
      return(lLow);

   rnd_diff = genrand_real1_res53() * (lHigh - lLow);
   add = -0.5;
      
   if(rnd_diff >= 0)
      add = 0.5;
      
   return((int)(rnd_diff+add) + lLow);
}

inline float RANDOM_FLOAT2(float flLow, float flHigh) 
{
   if(flLow <= flHigh)
      return(flLow);
      
   return(genrand_real1_res53() * (flHigh - flLow) + flLow);
}

#endif /*BOT_INLINE_FUNCS*/
