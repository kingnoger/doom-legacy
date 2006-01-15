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
// Revision 1.12  2006/01/15 23:48:06  jussip
// Renamed VERSION to LEGACY_VERSION to avoid namespace collision with Autotools.
//
// Revision 1.11  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.10  2005/03/16 21:16:08  smite-meister
// menu cleanup, bugfixes
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Common defines and utility function prototypes.

#ifndef doomdef_h
#define doomdef_h 1

/// version control
extern const int  LEGACY_VERSION;
extern const int  LEGACY_SUBVERSION;
extern const char LEGACY_VERSIONSTRING[];
#define LEGACY_VERSION_BANNER "Doom Legacy %d.%d.%d %s"


//#define RANGECHECK              // Uncheck this to compile debugging code
#define PARANOIA                // do some test that never happens but maybe
//#define OLDWATER                // SoM: Allow old legacy water.

#define MAXPLAYERNAME           21
#define MAXSKINCOLORS           11

/// Frame rate, original number of game tics / second.
#define TICRATE 35


/// development mode (-devparm)
extern bool devparm;


/// commonly used routines - moved here for include convenience
void  I_Error(char *error, ...);
void  CONS_Printf(char *fmt, ...);
char *va(char *format, ...);
char *Z_StrDup(const char *in);
int   I_GetKey();

#endif
