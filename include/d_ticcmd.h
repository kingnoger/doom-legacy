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
// Revision 1.3  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.2  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.3  2002/07/01 21:00:44  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:22  vberghol
// Version 133 Experimental!
//
// Revision 1.4  2001/03/03 06:17:33  bpereira
// no message
//
// Revision 1.3  2000/08/31 14:30:55  bpereira
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
//     Player movement commands in one game tic
//
//-----------------------------------------------------------------------------


#ifndef d_ticcmd_h
#define d_ticcmd_h 1

#include "m_fixed.h" // fixed_t
#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif


//
// Button/action code definitions.
//

//added:16-02-98: bit of value 64 doesnt seem to be used,
//                now its used to jump

enum buttoncode_t
{
  // Press "Fire".
  BT_ATTACK           = 1,
  // Use button, to open doors, activate switches.
  BT_USE              = 2,

  // Flag, weapon change pending.
  // If true, the next 3 bits hold weapon num.
  BT_CHANGE           = 4,
  // The 3bit weapon mask and shift, convenience.
  BT_WEAPONMASK       = (8+16+32),
  BT_WEAPONSHIFT      = 3,

  // Jump button.
  BT_JUMP             = 64,
  BT_EXTRAWEAPON      = 128
};


// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.

// bits in angleturn
#define TICCMD_RECEIVED 1      
#define TICCMD_XY       2
#define BT_FLYDOWN      4
struct ticcmd_t
{
  char         forwardmove;    // *2048 for move
  char         sidemove;       // *2048 for move
  short        angleturn;      // <<16 for angle delta
                                 // SAVED AS A BYTE into demos
  signed short aiming;    //added:16-02-98:mouse aiming, see G_BuildTicCmd
  byte         buttons;
};


#endif
