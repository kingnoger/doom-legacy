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
// Revision 1.8  2004/11/19 16:51:06  smite-meister
// cleanup
//
// Revision 1.7  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.6  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Common defines and utility function prototypes.

#ifndef doomdef_h
#define doomdef_h 1

/// version control
extern const int  VERSION;
extern const char VERSIONSTRING[];


//#define RANGECHECK              // Uncheck this to compile debugging code
#define PARANOIA                // do some test that never happens but maybe
//#define OLDWATER                // SoM: Allow old legacy water.

#define MAXPLAYERNAME           21
#define MAXSKINCOLORS           11
#define SAVESTRINGSIZE          24

/// Frame rate, number of game tics / second.
#define OLDTICRATE       35
#define NEWTICRATERATIO   1  // try 4 for 140 fps :)
#define TICRATE         (OLDTICRATE*NEWTICRATERATIO) 


/// development mode (-devparm)
extern bool devparm;


/// commonly used routines - moved here for include convenience
void  I_Error(char *error, ...);
void  CONS_Printf(char *fmt, ...);
char *va(char *format, ...);
char *Z_StrDup(const char *in);
int   I_GetKey();

#endif
