// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log$
// Revision 1.5  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.4  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.3  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//     Common defines, utility function prototypes
//-----------------------------------------------------------------------------

#ifndef doomdef_h
#define doomdef_h 1

#include "m_swap.h"

#ifdef __WIN32__
# define ASMCALL __cdecl
#else
# define ASMCALL
#endif

#if defined(LINUX)
# define O_BINARY 0
char *strupr(char *n); 
char *strlwr(char *n); // in parser.cpp
#endif


// version control
extern const int  VERSION;
extern const char VERSIONSTRING[];


//#define RANGECHECK              // Uncheck this to compile debugging code
#define PARANOIA                // do some test that never happens but maybe
#define LOGMESSAGES             // write message in log.txt (win32 only for the moment)

// some tests, enable or desable it if it run or not
//#define HORIZONTALDRAW        // abandoned : too slow
//#define TILTVIEW              // not finished
//#define PERSPCORRECT          // not finished
#define SPLITSCREEN             
#define ABSOLUTEANGLE           // work fine, soon #ifdef and old code remove
#define NEWLIGHT                // compute lighting with bsp (in construction)
//#define OLDWATER                // SoM: Allow old legacy water.
#define FRAGGLESCRIPT

// =========================================================================


#define MAXPLAYERNAME           21
#define MAXSKINCOLORS           11
#define SAVESTRINGSIZE          24

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define OLDTICRATE       35
#define NEWTICRATERATIO   1  // try 4 for 140 fps :)
#define TICRATE         (OLDTICRATE*NEWTICRATERATIO) 



// commonly used routines - moved here for include convenience

// i_system.h
void I_Error(char *error, ...);

// console.h
void CONS_Printf(char *fmt, ...);

// m_misc.h
char *va(char *format, ...);
char *Z_StrDup (const char *in);

// i_system.c, replace getchar() once the keyboard has been appropriated
int I_GetKey();

#endif
