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
// Revision 1.4  2004/11/04 21:12:54  smite-meister
// save/load fixed
//
// Revision 1.3  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.2  2004/01/06 14:37:45  smite-meister
// six bugfixes, cleanup
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Lookup tables.
///
/// fixed_t finetangent[FINEANGLES/2]   - tangens LUT
///  Maps fineangle_t(alpha + pi/2) to fixed_t(tan(angle)).
///  Should work with BAM fairly well (12 of 16bit, effectively, by shifting).
///
/// fixed_t finesine[FINEANGLES * 5/4] - sine/cosine LUT
///  Maps fineangle_t(alpha) to fixed_t(sin(angle)).
///  Remarkable thing is, how to use BAMs with this?
///
/// angle_t tantoangle[SLOPERANGE+1]   - arctan LUT
///  Maps (tan(alpha) * SLOPERANGE) to angle_t(alpha).
///
///  finetangent[i] == FRACUNIT * tan((i - FINEANGLES/4 + 0.5) * (2*pi / FINEANGLES))
///  finesine[i]    == FRACUNIT * sin((i + 0.5) * (2*pi / FINEANGLES))
///  tantoangle[i]  == atan(i/SLOPERANGE) * (2^32 / (2*pi))

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


angle_t R_PointToAngle2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);
fixed_t R_PointToDist2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);

#endif
