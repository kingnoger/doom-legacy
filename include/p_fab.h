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
// Revision 1.2  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.4  2002/08/06 13:14:28  vberghol
// ...
//
// Revision 1.3  2002/07/01 21:00:51  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:26  vberghol
// Version 133 Experimental!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//
//
//-----------------------------------------------------------------------------


#ifndef __P_FAB__
#define __P_FAB__

#include "doomtype.h"
#include "command.h"

extern consvar_t cv_solidcorpse;        //p_enemy
extern consvar_t cv_bloodtime;

class DActor;

// spawn smoke trails behind rockets and skull head attacks
void A_SmokeTrailer(DActor* actor);

// hack the states table to set Doom Legacy's default translucency on sprites
void P_SetTranslucencies (void);

// add commands for deathmatch rules and style (like more blood) :)
void D_AddDeathmatchCommands (void);

#endif
