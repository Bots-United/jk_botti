/***
*
*       Copyright (c) 1999, Valve LLC. All rights reserved.
*       
*       This product contains software technology licensed from Id 
*       Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*       All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
	#define likely(x) (x)
	#define unlikely(x) (x)
	#define __func_pure
#else
	#define likely(x) __builtin_expect((long int)!!(x), true)
	#define unlikely(x) __builtin_expect((long int)!!(x), false)
	#define __func_pure __attribute__((pure))
#endif

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

class Vector;

//=========================================================
// 2DVector - used for many pathfinding and many other 
// operations that are treated as planar rather than 3d.
//=========================================================
class Vector2D
{
public:
        inline Vector2D(void): x(0.0), y(0.0)                                                   { }
        inline Vector2D(float X, float Y): x(0.0), y(0.0)                               { x = X; y = Y; }
        inline Vector2D operator+(const Vector2D& v)    const   { return Vector2D(x+v.x, y+v.y);        }
        inline Vector2D operator-(const Vector2D& v)    const   { return Vector2D(x-v.x, y-v.y);        }
        inline Vector2D operator*(float fl)                             const   { return Vector2D(x*fl, y*fl);  }
        inline Vector2D operator/(float fl)                             const   { return Vector2D(x/fl, y/fl);  }
        
        inline float Length(void)                                               const   { return (float)sqrt(x*x + y*y );              }

        inline Vector2D Normalize ( void ) const
        {
                // Vector2D vec2;

                float flLen = Length();
                if ( flLen == 0 )
                        return Vector2D( 0, 0 );
                else
                        return Vector2D( x / flLen, y / flLen );
        }

        vec_t   x, y;
};

inline float DotProduct(const Vector2D& a, const Vector2D& b) { return( a.x*b.x + a.y*b.y ); }
inline Vector2D operator*(float fl, const Vector2D& v)  { return v * fl; }

//=========================================================
// 3D Vector
//=========================================================
class Vector                                            // same data-layout as engine's vec3_t,
{                                                               //              which is a vec_t[3]
public:
        // Construction/destruction
        inline Vector(void): x(0.0), y(0.0), z(0.0)                                     { }
        inline Vector(float X, float Y, float Z): x(0.0), y(0.0), z(0.0)        { x = X; y = Y; z = Z;                          }
        //inline Vector(double X, double Y, double Z)                   { x = (float)X; y = (float)Y; z = (float)Z;     }
        //inline Vector(int X, int Y, int Z)                            { x = (float)X; y = (float)Y; z = (float)Z;     }
        inline Vector(const Vector& v): x(0.0), y(0.0), z(0.0)  { x = v.x; y = v.y; z = v.z;                            } 
        inline Vector(const float rgfl[3]): x(0.0), y(0.0), z(0.0)    { x = rgfl[0]; y = rgfl[1]; z = rgfl[2];        }
        inline Vector(const Vector2D& v): x(0.0), y(0.0), z(0.0){ x = v.x; y = v.y; }

        // Operators
        inline Vector operator-(void) const                     { return Vector(-x,-y,-z);                              }
        inline int operator==(const Vector& v) const            { return x==v.x && y==v.y && z==v.z;    }
        inline int operator!=(const Vector& v) const            { return !(*this==v);                                   }
        inline Vector operator+(const Vector& v) const          { return Vector(x+v.x, y+v.y, z+v.z);   }
        inline Vector operator-(const Vector& v) const          { return Vector(x-v.x, y-v.y, z-v.z);   }
        inline Vector operator*(float fl) const                 { return Vector(x*fl, y*fl, z*fl);              }
        inline Vector operator/(float fl) const                 { return Vector(x/fl, y/fl, z/fl);              }
        inline Vector operator=(const Vector& v) 		{ x=v.x; y=v.y; z=v.z; return *this; }
        inline Vector operator+=(const Vector& v) 		{ x+=v.x; y+=v.y; z+=v.z; return *this; }
        inline Vector operator-=(const Vector& v) 		{ x-=v.x; y-=v.y; z-=v.z; return *this; }
        inline Vector operator*=(float fl) 			{ x*=fl; y*=fl; z*=fl; return *this; }
        inline Vector operator/=(float fl) 			{ x/=fl; y/=fl; z/=fl; return *this; }
        
        // Methods
        inline void CopyToArray(float* rgfl) const              { rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; }
        inline float Length(void) const                         { return (float)sqrt(x*x + y*y + z*z); }
        inline operator float *()                               { return &x; } // Vectors will now automatically convert to float * when needed
        inline operator Vector2D () const                       { return (*this).Make2D();      }

#if !defined(__GNUC__) || (__GNUC__ >= 3)
        inline operator const float *() const                   { return &x; } // Vectors will now automatically convert to float * when needed
#endif

        inline Vector Normalize(void) const
        {
                float flLen = Length();
                if (flLen == 0) return Vector(0,0,1); // ????
                return Vector(x / flLen, y / flLen, z / flLen);
        }

        inline Vector2D Make2D ( void ) const
        {
                Vector2D        Vec2;

                Vec2.x = x;
                Vec2.y = y;

                return Vec2;
        }
        inline float Length2D(void) const                       { return (float)sqrt(x*x + y*y); }

        // Members
        vec_t x, y, z;
};
inline Vector operator*(float fl, const Vector& v)      { return v * fl; }
inline float DotProduct(const Vector& a, const Vector& b) { return(a.x*b.x+a.y*b.y+a.z*b.z); }
inline Vector CrossProduct(const Vector& a, const Vector& b) { return Vector( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x ); }


//=========================================================
// Angular Vectors, for engine safe angle calculations
//=========================================================
class angle_t // same data-layout as engine's vec_t
{
public:
	// Helper functions
	static inline float wrap_angle(float ang) __func_pure
	{
		// this function normalises angle to the range [-180 < angle <= 180]
		ang += 180.0;
		const unsigned int bits = 0x80000000;
		ang = -180.0 + ((360.0 / bits) * ((int64_t)(ang * (bits / 360.0)) & (bits-1)));
		if (unlikely(ang == -180.0f))
			ang = 180.0;
		return ang;
	}
	inline void wrap_angle(void) { ang = wrap_angle(ang); }

	// Construction/destruction
	inline angle_t(void): ang(0.0) { }
	inline angle_t(float fl): ang(0.0) { ang = fl; wrap_angle(); }

        // Operators
        inline angle_t operator-(void) const		{ angle_t a; a.ang = -ang; return a; }
        inline int operator==(const angle_t& a) const	{ return ang == a.ang; }
        inline int operator!=(const angle_t& a) const	{ return !(*this==a); }
	inline int operator<(const angle_t& a) const	{ return ang < a.ang; }
	inline int operator<=(const angle_t& a) const	{ return ang <= a.ang; }
	inline int operator>(const angle_t& a) const	{ return ang > a.ang; }
	inline int operator>=(const angle_t& a) const	{ return ang >= a.ang; }
	inline int operator<(float fl) const		{ return ang < fl; }
	inline int operator<=(float fl) const		{ return ang <= fl; }
	inline int operator>(float fl) const		{ return ang > fl; }
	inline int operator>=(float fl) const		{ return ang >= fl; }
        inline angle_t operator+(const angle_t& a) const{ return angle_t(ang + a.ang); }
        inline angle_t operator-(const angle_t& a) const{ return angle_t(ang - a.ang); }
        inline angle_t operator=(const angle_t& a)	{ ang = a.ang; return *this; }
        inline angle_t operator+(float fl) const	{ return angle_t(ang + fl); }
        inline angle_t operator-(float fl) const	{ return angle_t(ang - fl); }
        inline angle_t operator*(float fl) const	{ return angle_t(ang * fl); }
        inline angle_t operator/(float fl) const	{ return angle_t(ang / fl); }
        inline angle_t operator=(float fl)		{ ang = fl; wrap_angle(); return *this; }
        inline angle_t operator+=(const angle_t& a)	{ ang += a.ang; wrap_angle(); return *this; }
        inline angle_t operator-=(const angle_t& a)	{ ang -= a.ang; wrap_angle();return *this; }
        inline angle_t operator+=(float fl)		{ ang += fl; wrap_angle(); return *this; }
        inline angle_t operator-=(float fl)		{ ang -= fl; wrap_angle(); return *this; }
        inline angle_t operator*=(float fl)		{ ang *= fl; wrap_angle(); return *this; }
        inline angle_t operator/=(float fl)		{ ang /= fl; wrap_angle(); return *this; }

	// Degrees to radians, for math.h functions
	inline double rad(void) const { return (double)ang * (M_PI*2 / 360); }

	// math functions
	inline double cos(void) const { return ::cos(rad()); }
	inline double sin(void) const { return ::sin(rad()); }
	inline double tan(void) const { return ::tan(rad()); }
	inline double abs(void) const { return ::fabs(ang); }

	// helper functions
	static inline angle_t rad_angle(double rad) { return angle_t(rad / (M_PI*2 / 360)); }
	static inline angle_t between_vectors(const Vector &a, const Vector &b)
	{
		return rad_angle(acos(DotProduct(a, b)));
	}

	// Member
	vec_t ang;
};

class ang3_t // same data-layout as engine's vec3_t, which is a vec_t[3]
{
public:
        // Construction/destruction
        inline ang3_t(void): pitch(0.0), yaw(0.0), roll(0.0) { }
	inline ang3_t(const angle_t p, const angle_t y, const angle_t r): pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(float p, float y, float r):			pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(const angle_t p, const angle_t y, float r):	pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(const angle_t p, float y, float r):		pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(const angle_t p, float y, const angle_t r):	pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(float p, const angle_t y, const angle_t r):	pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(float p, float y, const angle_t r):		pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
	inline ang3_t(float p, const angle_t y, float r):		pitch(0.0), yaw(0.0), roll(0.0) { pitch = p; yaw = y; roll = r; }
        inline ang3_t(const ang3_t& v): pitch(0.0), yaw(0.0), roll(0.0)
	{
		pitch = v.pitch;
		yaw   = v.yaw;
		roll  = v.roll;
	}
        inline ang3_t(const float rgfl[3]): pitch(0.0), yaw(0.0), roll(0.0)
	{
		pitch = rgfl[0]; 
		yaw   = rgfl[1]; 
		roll  = rgfl[2];
	}
        inline ang3_t(const Vector& v): pitch(0.0), yaw(0.0), roll(0.0)
	{
   		float tmp;

		if (unlikely(v.y == 0) && unlikely(v.x == 0))
		{
			pitch = (v.z > 0) ? 90 : -90;
		}
		else
		{
			// atan2 returns values in range [-pi < x < +pi]
			yaw = (atan2(v.y, v.x) * 180 / M_PI);
			tmp = sqrt(v.x * v.x + v.y * v.y);
			pitch = (atan2(v.z, tmp) * 180 / M_PI);
		}
	}

        // Operators
        inline ang3_t operator-(void) const                     { return ang3_t(-pitch,-yaw,-roll); }
        inline int operator==(const ang3_t& a) const            { return pitch==a.pitch && yaw==a.yaw && roll==a.roll; }
        inline int operator!=(const ang3_t& a) const            { return !(*this==a);                                   }
        inline ang3_t operator+(const ang3_t& a) const          { return ang3_t(pitch+a.pitch, yaw+a.yaw, roll+a.roll);   }
        inline ang3_t operator-(const ang3_t& a) const          { return ang3_t(pitch-a.pitch, yaw-a.yaw, roll-a.roll);   }
        /*inline ang3_t operator*(float fl) const                 { return ang3_t(x*fl, y*fl, z*fl);              }
        inline ang3_t operator/(float fl) const                 { return ang3_t(x/fl, y/fl, z/fl);              }
        inline ang3_t operator=(const ang3_t& v) 		{ x=v.x; y=v.y; z=v.z; return *this; }
        inline ang3_t operator+=(const ang3_t& v) 		{ x+=v.x; y+=v.y; z+=v.z; return *this; }
        inline ang3_t operator-=(const ang3_t& v) 		{ x-=v.x; y-=v.y; z-=v.z; return *this; }
        inline ang3_t operator*=(float fl) 			{ x*=fl; y*=fl; z*=fl; return *this; }
        inline ang3_t operator/=(float fl) 			{ x/=fl; y/=fl; z/=fl; return *this; }*/

	// Angles to vectors conversion, forward, right, up
	inline Vector forward(void)
	{
		double sp, cp, sy, cy;

		fsincos(pitch.rad(), sp, cp);
		fsincos(yaw.rad(), sy, cy);

		return(Vector(cp*cy, cp*sy, -sp));
	}
	inline Vector right(void)
	{
		double sr, cr, sp, cp, sy, cy;

		fsincos(pitch.rad(), sp, cp);
		fsincos(yaw.rad(), sy, cy);
		fsincos(roll.rad(), sr, cr);

		return(Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp));
	}
	inline Vector up(void)
	{
		double sr, cr, sp, cp, sy, cy;

		fsincos(pitch.rad(), sp, cp);
		fsincos(yaw.rad(), sy, cy);
		fsincos(roll.rad(), sr, cr);

		return(Vector(cr*sp*cy+-sr*-sy, cr*sp*sy+-sr*cy, cr*cp));
	}
	inline void forward_right_up(Vector &forward, Vector &right, Vector &up)
	{
		double sr, cr, sp, cp, sy, cy;

		fsincos(pitch.rad(), sp, cp);
		fsincos(yaw.rad(), sy, cy);
		fsincos(roll.rad(), sr, cr);
		
		forward = Vector(cp*cy, cp*sy, -sp);
		right   = Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp);
		up      = Vector(cr*sp*cy+-sr*-sy, cr*sp*sy+-sr*cy, cr*cp);
	}

	// Members
	angle_t pitch, yaw, roll;
};


#endif

