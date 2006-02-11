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
//
// DESCRIPTION:
//      Sky rendering.
//
//-----------------------------------------------------------------------------


#ifndef r_sky_h
#define r_sky_h 1

#include "m_fixed.h"


// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT         22

extern int            skytexturemid;
extern fixed_t        skyscale;
extern int            skymode;  //current sky old (0) or new(1),
                                  // see SCR_SetMode

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.
extern int              skyflatnum;

//added:12-02-98: declare the asm routine which draws the sky columns
void R_DrawSkyColumn();

// Called once at startup.
void R_InitSkyMap();

// call after skytexture is set to adapt for old/new skies
void R_SetupSkyDraw(int height);

void        R_StorePortalRange();
void        R_InitPortals();
void        R_ClearPortals();
void        R_DrawPortals();

void R_SetSkyScale();

#endif
