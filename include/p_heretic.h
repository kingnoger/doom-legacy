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
// Revision 1.2  2002/12/16 22:05:02  smite-meister
// Actor / DActor separation done!
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
// Revision 1.2  2001/02/10 13:20:55  hurdler
// update license
//
//
//
// DESCRIPTION:
//     Misc. routines from Heretic
//-----------------------------------------------------------------------------

class Actor;

void P_MinotaurSlam(Actor *source, Actor *target);
bool P_TouchWhirlwind(Actor *target);
void DoomPatchEngine();
void HereticPatchEngine();
