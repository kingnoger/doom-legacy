// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log$
// Revision 1.2  2002/12/16 22:05:17  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.4  2002/07/23 19:21:46  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:56  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:30  vberghol
// Version 133 Experimental!
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------


#ifndef t_func_h
#define t_func_h 1

#include "t_parse.h"
#include "p_camera.h"

extern Camera script_camera;
extern bool  script_camera_on;

void init_functions();

#define AngleToFixed(x)  (((double) x) / ((double) ANG45/45)) * FRACUNIT
#define FixedToAngle(x)  (((double) x) / FRACUNIT) * ANG45/45;

#endif
