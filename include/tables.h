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
// Revision 1.2  2004/01/06 14:37:45  smite-meister
// six bugfixes, cleanup
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Lookup tables.
//      Do not try to look them up :-).
//      In the order of appearance: 
//
//      int finetangent[4096]   - Tangens LUT.
//       Should work with BAM fairly well (12 of 16bit,
//      effectively, by shifting).
//
//      int finesine[10240]             - Sine lookup.
//
//      int tantoangle[2049]    - ArcTan LUT,
//        maps tan(angle) to angle fast. Gotta search.
//    
//-----------------------------------------------------------------------------


#ifndef tables_h
#define tables_h 1

#include "m_fixed.h"

typedef unsigned int angle_t;


#define FINEANGLES              8192
#define FINEMASK                (FINEANGLES-1)
#define ANGLETOFINESHIFT        19      // 0x100000000 to 0x2000
#define AIMINGTOSLOPE(aiming)   finesine[((aiming) >> ANGLETOFINESHIFT) & FINEMASK]


// Effective size is 10240.
extern fixed_t  finesine[5*FINEANGLES/4];

// Re-use data, is just PI/2 phase shift.
extern fixed_t *finecosine;

// Effective size is 4096.
extern fixed_t  finetangent[FINEANGLES/2];


const angle_t ANG45  = 0x20000000;
const angle_t ANG90  = 0x40000000;
const angle_t ANG180 = 0x80000000;
const angle_t ANG270 = 0xc0000000;

const angle_t ANGLE_MAX = 0xffffffff;
const angle_t ANGLE_1   = ANG45 / 45;
const angle_t ANGLE_60  = ANG180 / 3;


// to get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system, then
// the y (<=x) is scaled and divided by x to get a tangent (slope) value
// which is looked up in the tantoangle[] table.
#define SLOPERANGE  2048
#define SLOPEBITS   11
#define DBITS       (FRACBITS-SLOPEBITS)

// The +1 size is to handle the case when x==y without additional checking.
extern  angle_t     tantoangle[SLOPERANGE+1];

// Utility function, called by R_PointToAngle.
int SlopeDiv(unsigned num, unsigned  den);


#endif
