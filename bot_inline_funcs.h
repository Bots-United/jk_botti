//
// JK_Botti - be more human!
//
// bot_inline_funcs.h
//

#ifndef BOT_INLINE_FUNCS
#define BOT_INLINE_FUNCS

#include <inttypes.h>
#include <sys/time.h>

// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
	#define likely(x) (x)
	#define unlikely(x) (x)
#else
	#define likely(x) __builtin_expect((long int)!!(x), true)
	#define unlikely(x) __builtin_expect((long int)!!(x), false)
#endif

//
inline double UTIL_GetSecs(void) 
{
#ifdef _WIN32
   LARGE_INTEGER count, freq;

   count.QuadPart = 0;
   freq.QuadPart = 0;

   QueryPerformanceFrequency(&freq);
   QueryPerformanceCounter(&count);

   return (double)count.QuadPart / (double)freq.QuadPart;
#else
   struct timeval tv;
   
   gettimeofday (&tv, NULL);
   
   return (double) tv.tv_sec + ((double) tv.tv_usec) / 1000000.0;
#endif
}

#ifdef __GNUC__
inline void fsincos(double x, double &s, double &c) 
{
   __asm__ ("fsincos;" : "=t" (c), "=u" (s) : "0" (x) : "st(7)");
}
#else
inline void fsincos(double x, double &s, double &c)
{
   s = sin(x);
   c = cos(x);
}
#endif

inline Vector UTIL_AnglesToForward(const Vector &angles)
{
   // from pm_shared/pm_math.h
   double sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   fsincos(pitch, sp, cp);

   yaw = angles.y * (M_PI*2 / 360);
   fsincos(yaw, sy, cy);

   return(Vector(cp*cy, cp*sy, -sp));
}

inline Vector UTIL_AnglesToRight(const Vector &angles)
{
   // from pm_shared/pm_math.h
   double sr, cr, roll, sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   fsincos(pitch, sp, cp);

   yaw = angles.y * (M_PI*2 / 360);
   fsincos(yaw, sy, cy);

   roll = angles.z * (M_PI*2 / 360);
   fsincos(roll, sr, cr);

   return(Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp));
}

inline void UTIL_MakeVectorsPrivate( const Vector &angles, Vector &v_forward, Vector &v_right, Vector &v_up )
{
   // from pm_shared/pm_math.h
   double sr, cr, roll, sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   fsincos(pitch, sp, cp);

   yaw = angles.y * (M_PI*2 / 360);
   fsincos(yaw, sy, cy);

   roll = angles.z * (M_PI*2 / 360);
   fsincos(roll, sr, cr);

   v_forward = Vector(cp*cy, cp*sy, -sp);
   v_right   = Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp);
   v_up      = Vector(cr*sp*cy+-sr*-sy, cr*sp*sy+-sr*cy, cr*cp);
}

inline Vector UTIL_VecToAngles(const Vector &forward)
{
   // from pm_shared/pm_math.h
   float tmp, yaw, pitch;
   
   if (unlikely(forward.y == 0) && unlikely(forward.x == 0))
   {
      yaw = 0;
      pitch = (forward.z > 0) ? 90 : -90;
   }
   else
   {
      // atan2 returns values in range [-pi < x < +pi]
      yaw = (atan2(forward.y, forward.x) * 180 / M_PI);
      tmp = sqrt(forward.x * forward.x + forward.y * forward.y);
      pitch = (atan2(forward.z, tmp) * 180 / M_PI);
   }
   
   return(Vector(pitch, yaw, 0));
}

inline Vector GetGunPosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}

inline Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

inline void UTIL_SelectItem(edict_t *pEdict, const char *item_name)
{
   FakeClientCommand(pEdict, item_name, NULL, NULL);
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

inline void UTIL_TraceDuck( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr ) 
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), head_hull, pentIgnore, ptr );
}

inline edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (likely(!FNullEnt(pentEntity)))
      return pentEntity;

   return NULL;
}

inline edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
   edict_t *pentEntity;

   pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

   if (likely(!FNullEnt(pentEntity)))
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

inline void null_terminate_buffer(char *buf, const size_t maxlen)
{
   for(size_t i = 0; i < maxlen; i++)
      if(buf[i] == 0)
         return;
   buf[maxlen-1] = 0;
}

inline double deg2rad(double deg) 
{
   return(deg * (M_PI / 180));
}

inline qboolean IsAlive(const edict_t *pEdict) 
{
   qboolean ret0,ret1,ret2,ret3,ret4;
   
   ret0 = (pEdict->v.deadflag == DEAD_NO);
   ret1 = (pEdict->v.health > 0);
   ret2 = !(pEdict->v.flags & FL_NOTARGET);
   ret3 = (pEdict->v.takedamage != 0);
   ret4 = (pEdict->v.solid != SOLID_NOT);
   
   return(ret0 & ret1 & ret2 & ret3 & ret4);
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
//   if(strncmp(STRING(pEdict->v.classname), "func_", 5) == 0)
   if(unlikely(pEdict->v.solid == SOLID_BSP))
      return VecBModelOrigin(pEdict);

   return pEdict->v.origin; 
}

inline Vector UTIL_AdjustOriginWithExtent(bot_t &pBot, const Vector & v_target_origin, edict_t *pTarget)
{
   // get smallest extent of bots mins/maxs
   float smallest_extent = -pTarget->v.mins[0];
   if(-pTarget->v.mins[1] < smallest_extent)
      smallest_extent = -pTarget->v.mins[1];
   if(-pTarget->v.mins[2] < smallest_extent)
      smallest_extent = -pTarget->v.mins[2];
   
   if(pTarget->v.maxs[0] < smallest_extent)
      smallest_extent = pTarget->v.maxs[0];
   if(pTarget->v.maxs[1] < smallest_extent)
      smallest_extent = pTarget->v.maxs[1];
   if(pTarget->v.maxs[2] < smallest_extent)
      smallest_extent = pTarget->v.maxs[2];
   
   if(smallest_extent <= 0.0f)
      return(v_target_origin);
   
   // extent origin towards bot
   Vector v_extent_dir = (GetGunPosition(pBot.pEdict) - v_target_origin).Normalize();
   
   return(v_target_origin + v_extent_dir * smallest_extent);
}

inline Vector UTIL_GetOriginWithExtent(bot_t &pBot, edict_t *pTarget)
{
   return(UTIL_AdjustOriginWithExtent(pBot, UTIL_GetOrigin(pTarget), pTarget));
}

inline void UTIL_ParticleEffect( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount )
{
   PARTICLE_EFFECT( vecOrigin, vecDirection, (float)ulColor, (float)ulCount );
}

inline int UTIL_GetBotIndex(const edict_t *pEdict)
{
   int index;

   for (index=0; index < 32; index++)
      if (bots[index].pEdict == pEdict)
         return index;

   return -1;  // return -1 if edict is not a bot
}

inline bot_t *UTIL_GetBotPointer(const edict_t *pEdict)
{
   int index = UTIL_GetBotIndex(pEdict);
   
   if(index == -1)
      return NULL; // return NULL if edict is not a bot
   
   return(&bots[index]);
}

inline qboolean FInViewCone(const Vector & Origin, edict_t *pEdict)
{
   const float fov_angle = 80;
   
   return(DotProduct((Origin - pEdict->v.origin).Normalize(), UTIL_AnglesToForward(pEdict->v.v_angle)) > cos(deg2rad(fov_angle)));
}

inline qboolean FIsClassname(edict_t * pent, const char * cname)
{
   return(likely(pent)?strcmp(STRING(pent->v.classname), cname)==0:FALSE);
}

inline qboolean FIsClassname(const char * cname, edict_t * pent)
{
   return(FIsClassname(pent, cname));
}

extern unsigned int rnd_idnum[2];

/* generates a random 32bit integer */
inline unsigned int fast_generate_random(void)
{
   extern unsigned int rnd_idnum[2];
   
   rnd_idnum[0] ^= rnd_idnum[1] << 5;
   
   rnd_idnum[0] *= 1664525L;
   rnd_idnum[0] += 1013904223L;
   
   rnd_idnum[1] *= 1664525L;
   rnd_idnum[1] += 1013904223L;
   
   rnd_idnum[1] ^= rnd_idnum[0] << 3;
   
   return rnd_idnum[0];
}

inline void fast_random_seed(unsigned int seed)
{
   rnd_idnum[0] = seed;
   rnd_idnum[1] = ~(seed + 6);
   rnd_idnum[1] = fast_generate_random();
}

/* supports range INT_MIN, INT_MAX */
inline int RANDOM_LONG2(int lLow, int lHigh) 
{
   double rnd;
   
   if(unlikely(lLow >= lHigh))
      return(lLow);
   
   rnd = fast_generate_random();
   rnd = (rnd * ((double)lHigh - (double)lLow + 1.0)) / 4294967296.0; // div by (1<<32)
   
   return (int)(rnd + lLow);
}

inline float RANDOM_FLOAT2(float flLow, float flHigh) 
{
   double rnd;
   
   if(unlikely(flLow >= flHigh))
      return(flLow);
   
   rnd = fast_generate_random();
   rnd = rnd * (flHigh - flLow) / 4294967295.0; // div by (1<<32)-1
   
   return (float)(rnd + flLow);
}


inline qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t * pOrigin )
{
   edict_t * pHit = NULL;
   return(FVisible(vecOrigin, pEdict, &pHit) || (pOrigin != NULL && pHit == pOrigin));
}

inline qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict )
{
   return(FVisible(vecOrigin, pEdict, (edict_t **)NULL));
}

#endif /*BOT_INLINE_FUNCS*/
