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
//-----------------------------------------------------------------------------

/// \file
/// \brief Software renderer internal state variables.

#ifndef r_state_h
#define r_state_h 1

#include "screen.h"
#include "tables.h"

//
// Refresh internal data structures,
//  for rendering.
//

extern int              viewwidth;
extern int              scaledviewwidth;
extern int              viewheight;

extern int              firstflat;
extern int              firstwaterflat; //added:18-02-98:WATER!


//
// POV data.
//

// SoM: Portals require that certain functions use a different x and y pos
// than the actual view pos...
//extern fixed_t          bspx;
//extern fixed_t          bspy;
//extern angle_t          bspangle;


// ?
extern angle_t          clipangle;

extern int              viewangletox[FINEANGLES/2];
extern angle_t          xtoviewangle[MAXVIDWIDTH+1];

extern fixed_t          rw_distance;
extern angle_t          rw_normalangle;


// angle to line origin
extern int              rw_angle1;

#endif
