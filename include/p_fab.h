// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// DESCRIPTION:
//
//
//-----------------------------------------------------------------------------


#ifndef __P_FAB__
#define __P_FAB__

#include "doomtype.h"

// spawn smoke trails behind rockets and skull head attacks
void A_SmokeTrailer(class DActor *actor);

// hack the states table to set Doom Legacy's default translucency on sprites
void P_SetTranslucencies();

#endif
