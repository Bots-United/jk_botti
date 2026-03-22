//
// JK_Botti - be more human!
//
// util.cpp
//

#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "usercmd.h"
#include "bot.h"
#include "bot_func.h"
#include "player.h"

#define BREAKABLE_LIST_MAX 1024

extern bot_t bots[32];
extern qboolean is_team_play;

static breakable_list_t *g_breakable_list = NULL;
static breakable_list_t breakable_list_memarray[BREAKABLE_LIST_MAX];

// Polynomial sincos using only SSE-friendly operations. Avoids x87/SSE
// register transition penalty. Uses Cephes-style range reduction to
// [-pi/4, pi/4] and minimax polynomials. Max error vs libm: < 2e-16.
inline void fsincos_sse(double x, double &s, double &c)
{
   double abs_x = __builtin_fabs(x);

   // Find octant: j = floor(|x| * 4/pi), round up to even
   double y = __builtin_floor(abs_x * (4.0 / M_PI));
   int j = (int)y;
   int odd = j & 1;
   j = (j + odd) & 7;
   y += odd;

   // Extended precision reduction: z = |x| - y * (pi/4)
   // DP1+DP2+DP3 = pi/4 in triple-double precision
   static const double DP1 = 7.85398125648498535156e-1;
   static const double DP2 = 3.77489470793079817668e-8;
   static const double DP3 = 2.69515142907905952645e-15;
   double z = ((abs_x - y * DP1) - y * DP2) - y * DP3;
   double zz = z * z;

   // Minimax polynomial coefficients (Cephes double precision)
   double sin_p = z + z * zz * (-1.66666666666666307295e-1 + zz *
      (8.33333333332211858878e-3 + zz * (-1.98412698295895385996e-4 + zz *
      (2.75573136213857245213e-6 + zz * (-2.50507477628578072866e-8 + zz *
      1.58962301576546568060e-10)))));
   double cos_p = 1.0 - 0.5 * zz + zz * zz * (4.16666666666665929218e-2 + zz *
      (-1.38888888888730564116e-3 + zz * (2.48015872888517045348e-5 + zz *
      (-2.75573141792967388112e-7 + zz * (2.08757008419747316778e-9 + zz *
      (-1.13585365213876817300e-11))))));

   // Map octant to sin/cos:
   //   j=0: s= sin_p, c= cos_p    (angle near 0)
   //   j=2: s= cos_p, c=-sin_p    (angle near pi/2)
   //   j=4: s=-sin_p, c=-cos_p    (angle near pi)
   //   j=6: s=-cos_p, c= sin_p    (angle near 3pi/2)
   switch(j)
   {
   case 0:
      s =  sin_p;
      c =  cos_p;
      break;
   case 2:
      s =  cos_p;
      c = -sin_p;
      break;
   case 4:
      s = -sin_p;
      c = -cos_p;
      break;
   case 6:
      s = -cos_p;
      c =  sin_p;
      break;
   default:
      s = c = 0;
      break;
   }

   // sin is odd, cos is even
   if (x < 0)
      s = -s;
}

// Polynomial cos using only SSE-friendly operations (cos-only variant of
// fsincos_sse). cos is even: cos(-x) = cos(x), so no sign fixup needed.
inline double fcos_sse(double x)
{
   double abs_x = __builtin_fabs(x);

   // Find octant: j = floor(|x| * 4/pi), round up to even
   double y = __builtin_floor(abs_x * (4.0 / M_PI));
   int j = (int)y;
   int odd = j & 1;
   j = (j + odd) & 7;
   y += odd;

   // Extended precision reduction: z = |x| - y * (pi/4)
   // DP1+DP2+DP3 = pi/4 in triple-double precision
   static const double DP1 = 7.85398125648498535156e-1;
   static const double DP2 = 3.77489470793079817668e-8;
   static const double DP3 = 2.69515142907905952645e-15;
   double z = ((abs_x - y * DP1) - y * DP2) - y * DP3;
   double zz = z * z;

   // Only one polynomial needed per octant (Cephes double precision):
   //   j=0: +cos_p, j=2: -sin_p, j=4: -cos_p, j=6: +sin_p
   double p;
   if ((j & 2) == 0)
   {
      // j=0,4: cos polynomial
      p = 1.0 - 0.5 * zz + zz * zz * (4.16666666666665929218e-2 + zz *
         (-1.38888888888730564116e-3 + zz * (2.48015872888517045348e-5 + zz *
         (-2.75573141792967388112e-7 + zz * (2.08757008419747316778e-9 + zz *
         (-1.13585365213876817300e-11))))));
      if (j == 4)
         p = -p;
   }
   else
   {
      // j=2,6: sin polynomial
      p = z + z * zz * (-1.66666666666666307295e-1 + zz *
         (8.33333333332211858878e-3 + zz * (-1.98412698295895385996e-4 + zz *
         (2.75573136213857245213e-6 + zz * (-2.50507477628578072866e-8 + zz *
         1.58962301576546568060e-10)))));
      if (j == 2)
         p = -p;
   }

   return p;
}

// x87 fsincos asm: ~2.4x faster than glibc sin()+cos().
inline void fsincos_x87(double x, double &s, double &c)
{
   __asm__ ("fsincos;" : "=t" (c), "=u" (s) : "0" (x) : "st(7)");
}

inline void fsincos(double x, double &s, double &c)
{
#if defined(__SSE_MATH__)
   fsincos_sse(x, s, c);
#else
   fsincos_x87(x, s, c);
#endif
}

// Non-constant cos for SSE builds. Called via fcos() in util.h when
// __builtin_constant_p fails (runtime arguments).
inline double fcos_runtime(double x)
{
#if defined(__SSE_MATH__)
   return fcos_sse(x);
#else
   return cos(x);
#endif
}

// Polynomial atan using only SSE-friendly operations. Cephes-style
// three-interval range reduction to [0, 0.66] and rational polynomial
// P(x^2)/Q(x^2). Max error vs libm: < 2e-16.
inline double fatan_sse(double x)
{
   // Work with |x|, restore sign at end
   double abs_x = __builtin_fabs(x);

   // Range reduction:
   //   abs_x > tan(3*pi/8) (~2.414): z = -1/abs_x, base = pi/2
   //   abs_x > 0.66:                 z = (abs_x-1)/(abs_x+1), base = pi/4
   //   otherwise:                    z = abs_x, base = 0
   static const double T3P8 = 2.41421356237309504880; // tan(3*pi/8) = 1+sqrt(2)
   static const double MOREBITS = 6.123233995736765886130e-17; // pi/2 trailing bits

   double z, base, morebits_scale;
   if (abs_x > T3P8)
   {
      z = -1.0 / abs_x;
      base = M_PI_2;
      morebits_scale = 1.0;
   }
   else if (abs_x > 0.66)
   {
      z = (abs_x - 1.0) / (abs_x + 1.0);
      base = M_PI_4;
      morebits_scale = 0.5;
   }
   else
   {
      z = abs_x;
      base = 0.0;
      morebits_scale = 0.0;
   }

   // Rational polynomial: atan(z) = z + z^3 * P(z^2) / Q(z^2)
   // Cephes double precision coefficients, Horner form (high-to-low degree)
   double zz = z * z;

   // Numerator: zz * P(zz), where P is degree 4
   double p = ((((-8.750608600031904122785e-1 * zz
      - 1.615753718733365076637e1) * zz
      - 7.500855792314704667340e1) * zz
      - 1.228866684490136173410e2) * zz
      - 6.485021904942025371773e1) * zz;

   // Denominator: Q(zz), degree 5 with leading coeff 1.0
   double q = ((((zz
      + 2.485846490142306297962e1) * zz
      + 1.650270098316988542046e2) * zz
      + 4.328810604912902668951e2) * zz
      + 4.853903996359136964868e2) * zz
      + 1.945506571482613964425e2;

   z = z + z * (p / q) + morebits_scale * MOREBITS + base;

   // Restore sign: atan is odd
   if (x < 0)
      z = -z;

   return z;
}

// Polynomial atan2 using only SSE-friendly operations. Computes
// atan(y/x) with quadrant reconstruction from signs of x and y.
inline double fatan2_sse(double y, double x)
{
   // y == 0: copysign preserves sign of zero through to result
   if (y == 0.0)
   {
      if (x >= 0.0)
         return y;
      else
         return __builtin_copysign(M_PI, y);
   }
   if (x == 0.0)
      return (y > 0.0) ? M_PI_2 : -M_PI_2;

   double r = fatan_sse(y / x);

   // Quadrant adjustment for x < 0
   if (x < 0.0)
   {
      if (y >= 0.0)
         r += M_PI;
      else
         r -= M_PI;
   }

   return r;
}

// SSE/libm dispatch for atan2.
inline double fatan2(double y, double x)
{
#if defined(__SSE_MATH__)
   return fatan2_sse(y, x);
#else
   return atan2(y, x);
#endif
}


void null_terminate_buffer(char *buf, const size_t maxlen)
{
   buf[maxlen-1] = '\0';
}


double UTIL_GetSecs(void)
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


/*static float UTIL_WrapAngle360(float angle)
{
   // this function returns an angle normalized to the range [0 <= angle < 360]
   const unsigned int bits = 0x80000000;
   return ((360.0 / bits) * ((int64_t)(angle * (bits / 360.0)) & (bits-1)));
}*/


float UTIL_WrapAngle(float angle)
{
   // this function returns an angle normalized to the range (-180, 180]

   angle += 180.0;
   const unsigned int bits = 0x80000000;
   angle = -180.0 + ((360.0 / bits) * ((int64_t)(angle * (bits / 360.0)) & (bits-1)));

   if (angle == -180.0f)
      angle = 180.0;

   return(angle);
}


Vector UTIL_WrapAngles(const Vector & angles)
{
   // check for wraparound of angles
   return Vector( UTIL_WrapAngle(angles.x), UTIL_WrapAngle(angles.y), UTIL_WrapAngle(angles.z) );
}


Vector UTIL_AnglesToForward(const Vector &angles)
{
   // from pm_shared/pm_math.h
   double sp, cp, pitch, sy, cy, yaw;

   pitch = angles.x * (M_PI*2 / 360);
   fsincos(pitch, sp, cp);

   yaw = angles.y * (M_PI*2 / 360);
   fsincos(yaw, sy, cy);

   return(Vector(cp*cy, cp*sy, -sp));
}


Vector UTIL_AnglesToRight(const Vector &angles)
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


void UTIL_MakeVectorsPrivate( const Vector &angles, Vector &v_forward, Vector &v_right, Vector &v_up )
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


Vector UTIL_VecToAngles(const Vector &forward)
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
      yaw = (fatan2(forward.y, forward.x) * 180 / M_PI);
      tmp = sqrt(forward.x * forward.x + forward.y * forward.y);
      pitch = (fatan2(forward.z, tmp) * 180 / M_PI);
   }

   return(Vector(pitch, yaw, 0));
}


int UTIL_GetBotIndex(const edict_t *pEdict)
{
   int index;

   for (index=0; index < 32; index++)
      if (bots[index].pEdict == pEdict)
         return index;

   return -1;  // return -1 if edict is not a bot
}


bot_t *UTIL_GetBotPointer(const edict_t *pEdict)
{
   int index = UTIL_GetBotIndex(pEdict);

   if (index == -1)
      return NULL; // return NULL if edict is not a bot

   return(&bots[index]);
}


Vector UTIL_AdjustOriginWithExtent(bot_t &pBot, const Vector & v_target_origin, edict_t *pTarget)
{
   // mins/maxs are absolute values for SOLID_BSP
   if (pTarget->v.solid == SOLID_BSP)
      return v_target_origin;

   // get smallest extent of bots mins/maxs
   float smallest_extent = -pTarget->v.mins[0];
   if (-pTarget->v.mins[1] < smallest_extent)
      smallest_extent = -pTarget->v.mins[1];
   if (-pTarget->v.mins[2] < smallest_extent)
      smallest_extent = -pTarget->v.mins[2];

   if (pTarget->v.maxs[0] < smallest_extent)
      smallest_extent = pTarget->v.maxs[0];
   if (pTarget->v.maxs[1] < smallest_extent)
      smallest_extent = pTarget->v.maxs[1];
   if (pTarget->v.maxs[2] < smallest_extent)
      smallest_extent = pTarget->v.maxs[2];

   if (smallest_extent <= 0.0f)
      return(v_target_origin);

   // extent origin towards bot
   Vector v_extent_dir = (GetGunPosition(pBot.pEdict) - v_target_origin).Normalize();

   return(v_target_origin + v_extent_dir * smallest_extent);
}


// classic way of getting connected client count without using data collected from 'ClientPutInServer'/'ClientDisconnect'.
int UTIL_GetClientCount(void)
{
   int count = 0;

   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t * pClient = INDEXENT(i);

      if (!pClient || pClient->free || FNullEnt(pClient) || GETPLAYERUSERID(pClient) <= 0 || STRING(pClient->v.netname)[0] == 0)
         continue;

      count++;
   }

   return(count);
}


//
int UTIL_GetBotCount(void)
{
   int count = 0;

   for (int i = 0; i < 32; i++)
      count += bots[i].is_used;

   return(count);
}


//
int UTIL_PickRandomBot(void)
{
   int bot_index_list[32];
   int num_bots = 0;

   for (int i = 0; i < 32; i++)
      if (bots[i].is_used)  // is this slot used?
         bot_index_list[num_bots++] = i;

   if (num_bots > 0)
   {
      if (num_bots == 1)
         return(bot_index_list[0]);

      int pick = RANDOM_LONG2(0, num_bots-1);

      JKASSERT(pick < 0 || pick > num_bots-1);

      return(bot_index_list[pick]);
   }

   return(-1);
}


//
void UTIL_DrawBeam(edict_t *pEnemy, const Vector &start, const Vector &end, int width,
    int noise, int red, int green, int blue, int brightness, int speed)
{
   if (pEnemy && (ENTINDEX(pEnemy)-1 < 0 || ENTINDEX(pEnemy)-1 >= gpGlobals->maxClients))
      return;

   if (pEnemy == NULL)
      MESSAGE_BEGIN(MSG_ALL, SVC_TEMPENTITY);
   else
      MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEnemy);

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
static breakable_list_t * UTIL_AddFuncBreakable(edict_t *pEdict)
{
   int i;

   // get end of list
   breakable_list_t *prev = NULL;
   breakable_list_t *next = g_breakable_list;
   while (next)
   {
      prev = next;
      next = next->next;
   }

   // get unused slot
   for (i = 0; i < BREAKABLE_LIST_MAX; i++)
      if (!breakable_list_memarray[i].inuse)
         break;
   if (i >= BREAKABLE_LIST_MAX)
      return(NULL);

   next = &breakable_list_memarray[i];
   memset(next, 0, sizeof(breakable_list_t));
   next->inuse = TRUE;

   // fill in data
   next->next = NULL;
   next->material_breakable = FALSE;
   next->pEdict = pEdict;

   //link end of list next
   if (prev)
      prev->next = next;
   else
      g_breakable_list = next;

   return(next);
}

typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial } Materials;

// called on DispatchKeyValue
void UTIL_UpdateFuncBreakable(edict_t *pEdict, const char * setting, const char * value)
{
   // find breakable
   breakable_list_t *plist = g_breakable_list;

   while (plist)
   {
      if (plist->pEdict == pEdict)
         break;
      plist = plist->next;
   }

   // not found?
   if (!plist)
   {
      // add new
      plist = UTIL_AddFuncBreakable(pEdict);

      JKASSERT(plist == NULL);
      if (!plist)
         return;
   }

   // check if interesting setting
   if (FStrEq(setting, "material"))
   {
      // update data value
      plist->material_breakable = (atoi(value) != matUnbreakableGlass);
   }
}

// called on ServerDeactivate
void UTIL_FreeFuncBreakables(void)
{
   memset(breakable_list_memarray, 0, sizeof(breakable_list_memarray));
   g_breakable_list = NULL;
}

//
static breakable_list_t * UTIL_FindBreakable_Internal(breakable_list_t * pbreakable)
{
   if (unlikely(!pbreakable))
      return(g_breakable_list);
   else
      return(pbreakable->next);
}

//
breakable_list_t * UTIL_FindBreakable(breakable_list_t * pbreakable)
{
   do {
      pbreakable = UTIL_FindBreakable_Internal(pbreakable);

      if (unlikely(!pbreakable))
         return(NULL);

      // skip unbreakable glass
      if (!pbreakable->material_breakable)
         continue;

      // skip deleted entities
      if (FNullEnt(pbreakable->pEdict) || pbreakable->pEdict->v.health <= 0)
         continue;

   // skip reused/wrong name entities
   } while (!FIsClassname(pbreakable->pEdict, "func_breakable") && !FIsClassname(pbreakable->pEdict, "func_pushable"));

   return(pbreakable);
}


//
breakable_list_t * UTIL_LookupBreakable(edict_t *pEdict)
{
   breakable_list_t *plist = g_breakable_list;

   while (plist)
   {
      if (plist->pEdict == pEdict &&
         plist->material_breakable &&
         !FNullEnt(plist->pEdict) &&
         plist->pEdict->v.health > 0 &&
         (FIsClassname(plist->pEdict, "func_breakable") || FIsClassname(plist->pEdict, "func_pushable")))
      {
         return(plist);
      }

      plist = plist->next;
   }

   return(NULL);
}

//
void SaveAliveStatus(edict_t * pPlayer)
{
   int idx;

   idx = ENTINDEX(pPlayer) - 1;
   if (idx < 0 || idx >= gpGlobals->maxClients)
      return;

   if (!IsAlive(pPlayer))
      players[idx].last_time_dead = gpGlobals->time;
}

//
float UTIL_GetTimeSinceRespawn(edict_t * pPlayer)
{
   int idx;

   idx = ENTINDEX(pPlayer) - 1;
   if (idx < 0 || idx >= gpGlobals->maxClients)
      return(-1.0);

   if (!IsAlive(pPlayer))
   {
      //we are dead, so time since respawn is...
      return(-1.0);
   }
   else
   {
      return(gpGlobals->time - players[idx].last_time_dead);
   }
}


//
static qboolean IsPlayerFacingWall(edict_t * pPlayer)
{
   TraceResult tr;
   Vector v_forward, EyePosition;

   EyePosition = pPlayer->v.origin + pPlayer->v.view_ofs;
   v_forward = UTIL_AnglesToForward(pPlayer->v.v_angle);

   UTIL_TraceLine(EyePosition, EyePosition + v_forward * 48, ignore_monsters, ignore_glass, pPlayer, &tr);

   if (tr.flFraction > 0.999999f)
      return(FALSE);

   if (DotProduct(v_forward, tr.vecPlaneNormal) > -0.5f) //60deg
      return(FALSE);

   return(TRUE);
}

//
void CheckPlayerChatProtection(edict_t * pPlayer)
{
   int idx;

   idx = ENTINDEX(pPlayer) - 1;
   if (idx < 0 || idx >= gpGlobals->maxClients)
      return;

   // skip bots
   if (FBitSet(pPlayer->v.flags, FL_FAKECLIENT) || FBitSet(pPlayer->v.flags, FL_THIRDPARTYBOT))
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }

   // use of any buttons will reset protection
   if ((pPlayer->v.button & ~(IN_SCORE | IN_DUCK)) != 0)
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }

   // is not facing wall?
   if (!IsPlayerFacingWall(pPlayer))
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }

   // This cannot be checked, because if someone accidentally shoots chatter, chatter will move abit -> resets protection
   /*
   // is moving
   if (pPlayer->v.velocity.Length() > 1.0)
   {
      players[idx].last_time_not_facing_wall = gpGlobals->time;
      return;
   }*/
}

//
qboolean IsPlayerChatProtected(edict_t * pPlayer)
{
   int idx;

   idx = ENTINDEX(pPlayer) - 1;
   if (idx < 0 || idx >= gpGlobals->maxClients)
      return(FALSE);

   if (players[idx].last_time_not_facing_wall + 2.0 < gpGlobals->time)
   {
      return TRUE;
   }

   return FALSE;
}


void ClientPrint( edict_t *pEntity, int msg_dest, const char *msg_name)
{
   if (GET_USER_MSG_ID (PLID, "TextMsg", NULL) <= 0)
      REG_USER_MSG ("TextMsg", -1);

   MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "TextMsg", NULL), NULL, pEntity );

   WRITE_BYTE( msg_dest );
   WRITE_STRING( msg_name );
   MESSAGE_END();
}


#if 0
static void UTIL_SayText( const char *pText, edict_t *pEdict )
{
   if (GET_USER_MSG_ID (PLID, "SayText", NULL) <= 0)
      REG_USER_MSG ("SayText", -1);

   MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "SayText", NULL), NULL, pEdict );
      WRITE_BYTE( ENTINDEX(pEdict) );
      WRITE_STRING( pText );
   MESSAGE_END();
}
#endif


void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message )
{
   int   j;
   char  text[128];
   char *pc;
   edict_t *client;
   char  sender_teamstr[MAX_TEAMNAME_LENGTH];
   char  player_teamstr[MAX_TEAMNAME_LENGTH];

   // make sure the text has content
   for ( pc = message; pc != NULL && *pc != 0; pc++ )
   {
      if ( isprint( *pc ) && !isspace( *pc ) )
      {
         pc = NULL;   // we've found an alphanumeric character,  so text is valid
         break;
      }
   }

   if ( pc != NULL )
      return;  // no character found, so say nothing

   // turn on color set 2  (color on,  no sound)
   if ( teamonly )
      safevoid_snprintf( text, sizeof(text), "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
   else
      safevoid_snprintf( text, sizeof(text), "%c%s: ", 2, STRING( pEntity->v.netname ) );

   j = sizeof(text) - 2 - strlen(text);  // -2 for \n and null terminator
   if ( j < 0 )
      j = 0;
   if ( (int)strlen(message) > j )
      message[j] = 0;

   strcat( text, message );
   if ( strlen(text) < sizeof(text) - 2 )
      strcat( text, "\n" );

   // loop through all players
   // Start with the first player.
   // This may return the world in single player if the client types something between levels or during spawn
   // so check it, or it will infinite loop

   if (GET_USER_MSG_ID (PLID, "SayText", NULL) <= 0)
      REG_USER_MSG ("SayText", -1);

   UTIL_GetTeam(pEntity, sender_teamstr, sizeof(sender_teamstr));

   client = NULL;
   while ((client = UTIL_FindEntityByClassname( client, "player" )) != NULL && !FNullEnt(client))
   {
      if ( client == pEntity )  // skip sender of message
         continue;

      UTIL_GetTeam(client, player_teamstr, sizeof(player_teamstr));

      if ( teamonly && is_team_play && stricmp(sender_teamstr, player_teamstr) != 0 )
         continue;

      MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "SayText", NULL), NULL, client );
         WRITE_BYTE( ENTINDEX(pEntity) );
         WRITE_STRING( text );
      MESSAGE_END();
   }

   // print to the sending client
   MESSAGE_BEGIN( MSG_ONE, GET_USER_MSG_ID (PLID, "SayText", NULL), NULL, pEntity );
      WRITE_BYTE( ENTINDEX(pEntity) );
      WRITE_STRING( text );
   MESSAGE_END();

   // echo to server console
   SERVER_PRINT( text );

   // write to log file
   // team match?
   if ( is_team_play )
   {
      UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n",
         STRING( pEntity->v.netname ),
         GETPLAYERUSERID( pEntity ),
         (*g_engfuncs.pfnGetPlayerAuthId)( pEntity ),
         sender_teamstr,
         teamonly ? "say_team" : "say",
         message );
   }
   else
   {
      UTIL_LogPrintf( "\"%s<%i><%s><%i>\" %s \"%s\"\n",
         STRING( pEntity->v.netname ),
         GETPLAYERUSERID( pEntity ),
         (*g_engfuncs.pfnGetPlayerAuthId)( pEntity ),
         GETPLAYERUSERID( pEntity ),
         teamonly ? "say_team" : "say",
         message );
   }
}

#ifdef   DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
   if (pev->pContainingEntity != NULL)
      return pev->pContainingEntity;

   UTIL_ConsolePrintf("%s", "entvars_t pContainingEntity is NULL, calling into engine");
   edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
   if (pent == NULL)
      UTIL_ConsolePrintf("%s", "DAMN!  Even the engine couldn't FindEntityByVars!");
   ((entvars_t *)pev)->pContainingEntity = pent;
   return pent;
}
#endif //DEBUG

// return team string 0 through 3 based what MOD uses for team numbers
char * UTIL_GetTeam(edict_t *pEntity, char *teamstr, size_t slen)
{
   safe_strcopy(teamstr, slen, INFOKEY_VALUE(GET_INFOKEYBUFFER(pEntity), "model"));

   return(teamstr);
}

qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t ** pHit )
{
   TraceResult tr;
   Vector      vecLookerOrigin;

   if (pHit)
      *pHit = NULL;

   // look through caller's eyes
   vecLookerOrigin = GetGunPosition(pEdict);

   int bInWater = (POINT_CONTENTS (vecOrigin) == CONTENTS_WATER);
   int bLookerInWater = (POINT_CONTENTS (vecLookerOrigin) == CONTENTS_WATER);

   // don't look through water
   if (bInWater != bLookerInWater)
      return FALSE;

   UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

   if (pHit)
      *pHit = tr.pHit;

   return(tr.flFraction > 0.999999f);
}

static qboolean FVisibleEnemyOffset( const Vector &vecOrigin, const Vector &vecOffset, edict_t *pEdict, edict_t *pEnemy )
{
   edict_t * pHit = NULL;

   if (FVisible(vecOrigin + vecOffset, pEdict, &pHit) || (pEnemy != NULL && pHit == pEnemy))
      return(TRUE);

   if (FNullEnt(pHit))
      return(FALSE);

   if (!(pHit->v.flags & FL_MONSTER) && !FIsClassname(pHit, "player"))
      return(FALSE);

   if (!IsAlive (pHit))
      return(FALSE);

   return(TRUE);
}

qboolean FVisibleEnemy( const Vector &vecOrigin, edict_t *pEdict, edict_t *pEnemy )
{
   // only check center if cannot use extra information
   if (!pEnemy)
      return(FVisibleEnemyOffset( vecOrigin, Vector(0, 0, 0), pEdict, pEnemy ));

   if (pEnemy->v.solid != SOLID_BSP) {
      // first check for if head is visible
      Vector head_offset = Vector(0, 0, pEnemy->v.maxs.z - 6);
      if (FVisibleEnemyOffset( vecOrigin, head_offset, pEdict, pEnemy ))
         return(TRUE);

      // then check if feet are visible
      Vector feet_offset = Vector(0, 0, pEnemy->v.mins.z + 6);
      if (FVisibleEnemyOffset( vecOrigin, feet_offset, pEdict, pEnemy ))
         return(TRUE);
   }

   // check center
   if (FVisibleEnemyOffset( vecOrigin, Vector(0, 0, 0), pEdict, pEnemy ))
      return(TRUE);

#if 0
   /*
    * For long time this part was unintentionally disabled. Everything worked
    * just fine. And enabling this appears to increase CPU usage quite abit.
    * So keep this disabled after all.
    */
   if (pEnemy->v.solid != SOLID_BSP) {
      // construct sideways vector
      Vector v_right = UTIL_AnglesToRight(UTIL_VecToAngles(vecOrigin - GetGunPosition(pEdict)));

      // check if right side of player is visible
      Vector right_offset = v_right * (pEnemy->v.maxs.x - 4);
      if (FVisibleEnemyOffset( vecOrigin, right_offset, pEdict, pEnemy ))
         return(TRUE);

      // check if left side of player is visible
      Vector left_offset = v_right * (pEnemy->v.mins.x - 4);
      if (FVisibleEnemyOffset( vecOrigin, left_offset, pEdict, pEnemy ))
         return(TRUE);
   }
#endif

   return(FALSE);
}

qboolean FInShootCone(const Vector & Origin, edict_t *pEdict, float distance, float diameter, float min_angle)
{
   /*

      <----- distance ---->

                  ___....->T^           ^
       ...----''''          | <- radius |
     O ------------------->Qv           |
     ^ '''----....___                   | <- diameter
     |               ''''->             v
     |
     Bot(pEdict)

    T: Target (Origin)

    if angle Q-O-T is less than min_angle, always return true.

   */

   if (distance < 0.01)
      return TRUE;

   // angle between forward-view-vector and vector to player (as cos(angle))
   float flDot = DotProduct( (Origin - (pEdict->v.origin + pEdict->v.view_ofs)).Normalize(), UTIL_AnglesToForward(pEdict->v.v_angle) );
   if (flDot > fcos(deg2rad(min_angle))) // smaller angle, bigger cosine
      return TRUE;

   Vector2D triangle;
   triangle.x = distance;
   triangle.y = diameter / 2.0;

   // full angle of shootcode at this distance (as cos(angle))
   if (flDot > (distance / triangle.Length())) // smaller angle, bigger cosine
      return TRUE;

   return FALSE;
}

void UTIL_SelectWeapon(edict_t *pEdict, int weapon_index)
{
   usercmd_t user;

   user.lerp_msec = 0;
   user.msec = 0;
   user.viewangles = pEdict->v.v_angle;
   user.forwardmove = 0;
   user.sidemove = 0;
   user.upmove = 0;
   user.lightlevel = 127;
   user.buttons = 0;
   user.impulse = 0;
   user.weaponselect = weapon_index;
   user.impact_index = 0;
   user.impact_position = Vector(0, 0, 0);

   MDLL_CmdStart(pEdict, &user, 0);
   MDLL_CmdEnd(pEdict);
}


void UTIL_BuildFileName_N(char *filename, int size, char *arg1, char *arg2)
{
   const char * mod_dir = (submod_id == SUBMOD_OP4) ? "gearbox" : "valve";

   if ((arg1 != NULL) && (arg2 != NULL))
   {
      if (*arg1 && *arg2)
      {
         safevoid_snprintf(filename, size, "%s/%s/%s", mod_dir, arg1, arg2);
         return;
      }
   }

   if (arg1 != NULL)
   {
      if (*arg1)
      {
         safevoid_snprintf(filename, size, "%s/%s", mod_dir, arg1);
         return;
      }
   }

   safevoid_snprintf(filename, size, "%s/", mod_dir);
   return;
}


//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
   va_list argptr;
   char string[1024];

   va_start( argptr, fmt );
   safevoid_vsnprintf( string, sizeof(string), fmt, argptr );
   va_end( argptr );

   // Print to server console
   ALERT( at_logged, "%s", string );
}

#if 0
static void UTIL_ServerPrintf( char *fmt, ... )
{
   va_list argptr;
   char string[512];

   va_start( argptr, fmt );
   safevoid_vsnprintf( string, sizeof(string), fmt, argptr );
   va_end( argptr );

   // Print to server console
   SERVER_PRINT( string );
}
#endif

void UTIL_ConsolePrintf( const char *fmt, ... )
{
   va_list argptr;
   char string[512];
   size_t len;

   strcpy(string, "[jk_botti] ");
   len = strlen(string);

   va_start( argptr, fmt );
   safevoid_vsnprintf( string+len, sizeof(string)-len, fmt, argptr );
   va_end( argptr );

   // end msg with newline if not already
   len = strlen(string);
   if (string[len-1] != '\n')
   {
      if (len < sizeof(string)-2)// -1 null, -1 for newline
         strcat(string, "\n");
      else
         string[len-1] = '\n';
   }

   // Print to server console
   SERVER_PRINT( string );
}

void UTIL_AssertConsolePrintf(const char *file, const char *str, int line)
{
   UTIL_ConsolePrintf("[ASSERT] '%s' : '%s' : 'line %d'", file, str, line);
   __asm__ ("int $3");
}

char* UTIL_VarArgs2( char * string, size_t strlen, char *format, ... )
{
   va_list argptr;

   va_start (argptr, format);
   safevoid_vsnprintf (string, strlen, format, argptr);
   va_end (argptr);

   return string;
}

void GetGameDir (char *game_dir)
{
   // This function fixes the erratic behaviour caused by the use of the GET_GAME_DIR engine
   // macro, which returns either an absolute directory path, or a relative one, depending on
   // whether the game server is run standalone or not. This one always return a RELATIVE path.

   int length, fieldstart, fieldstop;

   GET_GAME_DIR (game_dir); // call the engine macro and let it mallocate for the char pointer

   length = strlen (game_dir); // get the length of the returned string
   if (length == 0)
      return;
   length--; // point to last character

   // format the returned string to get the last directory name
   fieldstop = length;
   while (((game_dir[fieldstop] == '\\') || (game_dir[fieldstop] == '/')) && (fieldstop > 0))
      fieldstop--; // shift back any trailing separator

   fieldstart = fieldstop;
   while ((game_dir[fieldstart] != '\\') && (game_dir[fieldstart] != '/') && (fieldstart > 0))
      fieldstart--; // shift back to the start of the last subdirectory name

   if ((game_dir[fieldstart] == '\\') || (game_dir[fieldstart] == '/'))
      fieldstart++; // if we reached a separator, step over it

   // now copy the formatted string back onto itself character per character
   for (length = fieldstart; length <= fieldstop; length++)
      game_dir[length - fieldstart] = game_dir[length];
   game_dir[length - fieldstart] = 0; // terminate the string

   return;
}

Vector VecBModelOrigin(edict_t *pEdict)
{
   Vector v_origin = pEdict->v.absmin + (pEdict->v.size * 0.5);

   if (likely(pEdict->v.solid == SOLID_BSP)) {
      if (unlikely((v_origin.x > pEdict->v.maxs.x || v_origin.x < pEdict->v.mins.x) ||
                   (v_origin.y > pEdict->v.maxs.y || v_origin.y < pEdict->v.mins.y) ||
                   (v_origin.z > pEdict->v.maxs.z || v_origin.z < pEdict->v.mins.z)))
         v_origin = (pEdict->v.maxs + pEdict->v.mins) / 2;
   }

   return v_origin;
}

qboolean IsAlive(const edict_t *pEdict)
{
   return (pEdict->v.deadflag == DEAD_NO) && (pEdict->v.health > 0) &&
      !(pEdict->v.flags & FL_NOTARGET) && ((int)pEdict->v.takedamage != 0) &&
      (pEdict->v.solid != SOLID_NOT);
}

qboolean FInViewCone(const Vector & Origin, edict_t *pEdict)
{
   const float fov_angle = 80;

   return(DotProduct((Origin - pEdict->v.origin).Normalize(), UTIL_AnglesToForward(pEdict->v.v_angle)) > fcos(deg2rad(fov_angle)));
}
