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
// $Log$
// Revision 1.4  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.3  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.2  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
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
