// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.12  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.11  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.10  2001/04/01 17:35:06  bpereira
// no message
//
// Revision 1.9  2001/02/24 13:35:19  bpereira
// no message
//
// Revision 1.8  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.7  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.6  2000/10/21 08:43:28  bpereira
// no message
//
// Revision 1.5  2000/08/31 14:30:55  bpereira
// no message
//
// Revision 1.4  2000/08/10 19:58:04  bpereira
// no message
//
// Revision 1.3  2000/08/10 14:53:10  ydario
// OS/2 port
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
// Just debug stuff here anymore.
//
//-----------------------------------------------------------------------------


#ifndef d_debug_h
#define d_debug_h 1

#include <stdio.h>


// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

#define localgametic  cmap.leveltic


#ifdef __MACOS__
# define DEBFILE(msg) I_OutputMsg(msg)
extern  FILE*           debugfile;
#else
# define DEBUGFILE
# ifdef DEBUGFILE
#  define DEBFILE(msg) { if(debugfile) fputs(msg,debugfile); }
extern  FILE*           debugfile;
# else
#  define DEBFILE(msg) {}
extern  FILE*           debugfile;
# endif
#endif //__MACOS__


#endif // d_debug_h
