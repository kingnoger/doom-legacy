// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log$
// Revision 1.4  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.3  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.2  2003/04/14 08:58:30  smite-meister
// Hexen maps load.
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Setup a game, startup stuff.
//
//-----------------------------------------------------------------------------


#ifndef p_setup_h
#define p_setup_h 1

struct mapthing_t;

typedef unsigned char byte;

// linedef conversion lookup table entry
struct xtable_t
{
  byte type;
  byte args[5];
  byte trigger;
};

// used in linedef conversion table
enum trigger_e
{
  T_REPEAT = 0x01, // bit 0

  T_AMASK  = 0x0e, // bits 1--3
  T_ASHIFT = 1,

  T_CROSS  = 0, // W
  T_USE    = 1,	// P, S
  T_MCROSS = 2,	// when monster crosses line
  T_IMPACT = 3,	// G
  T_PUSH   = 4,	// when player/monster pushes line
  T_PCROSS = 5, // when projectile crosses line

  T_ALLOWMONSTER = 0x10, // bit 4

  //T_TAG_1 = 0x20 // bit 5
};


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


#endif
