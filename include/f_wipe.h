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
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Mission start screen wipe/melt, special effects.
//      
//-----------------------------------------------------------------------------


#ifndef f_wipe_h
#define f_wipe_h 1

void wipe_StartScreen(int x, int y, int width, int height);
void wipe_EndScreen(int x, int y, int width, int height);
bool wipe_ScreenWipe(int x, int y, int width, int height, int ticks);

#endif
