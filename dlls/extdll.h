/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef EXTDLL_H
#define EXTDLL_H


//
// Global header file for extension DLLs
//

// Allow "DEBUG" in addition to default "_DEBUG"
#ifdef _DEBUG
#define DEBUG 1
#endif



// Prevent tons of unused windows definitions
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOWINRES
	#define NOSERVICE
	#define NOMCX
	#define NOIME
	#include "windows.h"
#else // _WIN32
	#define FALSE 0
	#define TRUE (!FALSE)
	typedef unsigned int ULONG;
	typedef unsigned char BYTE;
	typedef int BOOL;
	#define MAX_PATH PATH_MAX
	#include <limits.h>
	#include <stdarg.h>
	#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#endif //_WIN32

#ifndef MIN
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif

// Misc C-runtime library headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Header file containing definition of globalvars_t and entvars_t
typedef int	func_t;					//
typedef int	string_t;				// from engine's pr_comp.h;
typedef float vec_t;				// needed before including progdefs.h

// Vector class
#include "vector.h"

// Defining it as a (bogus) struct helps enforce type-checking
#define vec3_t Vector

// Shared engine/DLL constants
#include "const.h"
#include "progdefs.h"
#include "edict.h"

// Shared header describing protocol between engine and DLLs
#include "eiface.h"

// Shared header between the client DLL and the game DLLs
#include "cdll_dll.h"

#endif //EXTDLL_H
