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
// Revision 1.7  2004/10/27 17:37:09  smite-meister
// netcode update
//
// Revision 1.6  2004/09/13 20:43:31  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.5  2004/07/07 17:27:19  smite-meister
// bugfixes
//
// Revision 1.4  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.3  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.2  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief ticcmd_t definition

#ifndef d_ticcmd_h
#define d_ticcmd_h 1

#include "m_fixed.h" // fixed_t
#include "doomtype.h"

/// \brief Player input.
///
/// Client input data gathered during a tick and sent
/// to server for processing.

struct ticcmd_t
{
  /// Button-type actions
  enum buttoncode_t
  {
    BT_ATTACK   = 1, ///< Press "Fire".
    BT_USE      = 2, ///< Use button, to open doors, activate switches.
    BT_JUMP     = 4, ///< Jump or fly/swim up.
    BT_FLYDOWN  = 8, ///< Fly/swim down.

    // The weapon mask and shift, for convenience.
    WEAPONMASK  = 0xFFF0,
    WEAPONSHIFT = 4,
  };

  short buttons; ///< buttons and weapon changes
  char  item;    ///< if nonzero, using an inventory item
  char  forward; ///< "push" forward-backward
  char  side;    ///< "push" right-left

  // rotation: << 16 for angle_t
  short yaw;   ///< left-right   
  short pitch; ///< up-down



  ticcmd_t() : buttons(0), item(0), forward(0), side(0), yaw(0), pitch(0) {};

  /// sets the ticcmd into a neutral state
  void Clear();

  /// Fills the ticcmd_t with local input data.
  void Build(int playernumber, int elapsedtics);
};

#endif
