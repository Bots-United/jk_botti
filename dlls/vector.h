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

class Vector;

//=========================================================
// 2DVector - used for many pathfinding and many other 
// operations that are treated as planar rather than 3d.
//=========================================================
class Vector2D
{
public:
        inline Vector2D(void): x(0.0), y(0.0) { }
        inline Vector2D(float X, float Y): x(X), y(Y) { }
        inline Vector2D operator+(const Vector2D& v)    const   { return Vector2D(x+v.x, y+v.y);        }
        inline Vector2D operator-(const Vector2D& v)    const   { return Vector2D(x-v.x, y-v.y);        }
        inline Vector2D operator*(float fl)             const   { return Vector2D(x*fl, y*fl);  }
        inline Vector2D operator/(float fl)             const   { return Vector2D(x/fl, y/fl);  }
        
        inline double Length(void)                      const   { return sqrt(x*x + y*y );              }

        inline Vector2D Normalize ( void ) const
        {
                double flLen = Length();
                if ( (float)flLen == 0.0f )
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
        inline Vector(void): x(0.0), y(0.0), z(0.0) { }
        inline Vector(float X, float Y, float Z): x(X), y(Y), z(Z) { }
        inline Vector(const Vector& v): x(v.x), y(v.y), z(v.z) { } 
        inline Vector(const float rgfl[3]): x(rgfl[0]), y(rgfl[1]), z(rgfl[2]) { }
        inline Vector(const Vector2D& v): x(v.x), y(v.y), z(0.0) { }

        // Operators
        inline Vector operator-(void) const                     { return Vector(-x,-y,-z);              }
        inline int operator==(const Vector& v) const            { return x==v.x && y==v.y && z==v.z;    }
        inline int operator!=(const Vector& v) const            { return !(*this==v);                   }
        inline Vector operator+(const Vector& v) const          { return Vector(x+v.x, y+v.y, z+v.z);   }
        inline Vector operator-(const Vector& v) const          { return Vector(x-v.x, y-v.y, z-v.z);   }
        inline Vector operator*(float fl) const                 { return Vector(x*fl, y*fl, z*fl);      }
        inline Vector operator/(float _fl) const                { double fl = 1.0 / _fl; return Vector(x*fl, y*fl, z*fl); }
        inline Vector& operator=(const Vector& v) 		{ x=v.x; y=v.y; z=v.z; return *this;    }
        inline Vector& operator+=(const Vector& v) 		{ x+=v.x; y+=v.y; z+=v.z; return *this; }
        inline Vector& operator-=(const Vector& v) 		{ x-=v.x; y-=v.y; z-=v.z; return *this; }
        inline Vector& operator*=(float fl) 			{ x*=fl; y*=fl; z*=fl; return *this;    }
        inline Vector& operator/=(float _fl) 			{ double fl = 1.0 / _fl; x=x*fl; y=y*fl; z=z*fl; return *this; }
        
        // Methods
        inline void CopyToArray(float* rgfl) const              { rgfl[0] = x; rgfl[1] = y; rgfl[2] = z; }
        inline double Length(void) const                        { return sqrt(x*x + y*y + z*z);         }
        inline operator float *()                               { return &x; } // Vectors will now automatically convert to float * when needed
        inline operator Vector2D () const                       { return (*this).Make2D(); }

#if !defined(__GNUC__) || (__GNUC__ >= 3)
        inline operator const float *() const                   { return &x; } // Vectors will now automatically convert to float * when needed
#endif

        inline Vector Normalize(void) const
        {
                double flLen = Length();
                if ((float)flLen == 0.0f) return Vector(0, 0, 1); // ????
                flLen = 1.0 / flLen;
                return Vector(x * flLen, y * flLen, z * flLen);
        }

        inline Vector2D Make2D ( void ) const
        {
                return Vector2D(x,y);
        }
        inline double Length2D(void) const                       { return sqrt(x*x + y*y); }

        inline bool is_zero_vector(void) const
        {
		const double diff = 0.0000001;
		if (fabs(x) > diff)
			return false;
		if (fabs(y) > diff)
			return false;
		if (fabs(z) > diff)
			return false;
		return true;
	}

        // Members
        vec_t x, y, z;
};

inline Vector operator*(float fl, const Vector& v)      { return v * fl; }
inline float DotProduct(const Vector& a, const Vector& b) { return(a.x*b.x+a.y*b.y+a.z*b.z); }
inline Vector CrossProduct(const Vector& a, const Vector& b) { return Vector( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x ); }

#endif

