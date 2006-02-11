// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
//     Misc. routines from Heretic
//-----------------------------------------------------------------------------

#ifndef p_heretic_h
#define p_heretic_h 1

class Actor;

void P_MinotaurSlam(Actor *source, Actor *target);
bool P_TouchWhirlwind(Actor *target);
void DoomPatchEngine();
void HereticPatchEngine();
void HexenPatchEngine();

#endif
