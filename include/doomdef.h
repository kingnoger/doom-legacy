// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.9  2005/01/25 18:29:15  smite-meister
// preparing for alpha
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
extern const int  SUBVERSION;
extern const char VERSIONSTRING[];
#define VERSION_BANNER "Doom Legacy %d.%d.%d %s"


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
