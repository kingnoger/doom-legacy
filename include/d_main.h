// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
// $Log$
// Revision 1.3  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.2  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.5  2002/08/24 11:57:27  vberghol
// d_main.cpp is better
//
// Revision 1.4  2002/07/04 18:02:26  vberghol
// Pientä fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.3  2002/07/01 21:00:43  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:21  vberghol
// Version 133 Experimental!
//
// Revision 1.6  2001/08/20 20:40:39  metzgermeister
// *** empty log message ***
//
// Revision 1.5  2000/10/21 08:43:28  bpereira
// no message
//
// Revision 1.4  2000/04/23 16:19:52  bpereira
// no message
//
// Revision 1.3  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      game startup, and main loop code
//-----------------------------------------------------------------------------


#ifndef d_main_h
#define d_main_h 1

#include "doomtype.h"

// make sure not to write back the config until it's been correctly loaded
extern tic_t rendergametic;

// for dedicated server
extern bool dedicated;

// the infinite loop of D_DoomLoop() called from win_main for windows version
void D_DoomLoop();

//
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
void D_DoomMain();


// Called by IO functions when input is detected.
void D_PostEvent(const struct event_t* ev);
void D_PostEvent_end();    // delimiter for locking memory
void D_ProcessEvents();

// pagename is lumpname of a 320x200 patch to fill the screen
void D_PageDrawer(char* pagename);


#endif
