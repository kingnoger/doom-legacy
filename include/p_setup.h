// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.1  2002/11/16 14:18:25  hurdler
// Initial revision
//
// Revision 1.8  2002/08/20 13:57:01  vberghol
// sdfgsd
//
// Revision 1.7  2002/08/13 19:47:46  vberghol
// p_inter.cpp done
//
// Revision 1.6  2002/08/08 18:36:26  vberghol
// p_spec.cpp fixed
//
// Revision 1.5  2002/07/26 19:23:06  vberghol
// a little something
//
// Revision 1.4  2002/07/01 21:00:53  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:58  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.4  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.3  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Setup a game, startup stuff.
//
//-----------------------------------------------------------------------------


#ifndef p_setup_h
#define p_setup_h 1

struct mapthing_t;

extern int        lastloadedmaplumpnum; // for comparative savegame
//
// MAP used flats lookup table
//
struct levelflat_t
{
  char        name[8];        // resource name from wad
  int         lumpnum;        // lump number of the flat

  // for flat animation
  int         baselumpnum;
  int         animseq;        // start pos. in the anim sequence
  int         numpics;
  int         speed;
};

extern int             numlevelflats;
extern levelflat_t*    levelflats;

int P_AddLevelFlat (char* flatname, levelflat_t* levelflat);
char *P_FlatNameForNum(int num);


extern bool  newlevel;

#endif
