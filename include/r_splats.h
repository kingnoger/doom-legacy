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
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1.1.1  2002/11/16 14:18:27  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.5  2002/08/19 18:06:43  vberghol
// renderer somewhat fixed
//
// Revision 1.4  2002/08/13 19:47:46  vberghol
// p_inter.cpp done
//
// Revision 1.3  2002/07/01 21:00:55  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:29  vberghol
// Version 133 Experimental!
//
// Revision 1.4  2000/11/02 19:49:37  bpereira
// no message
//
// Revision 1.3  2000/04/30 10:30:10  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      flat sprites & blood splats effects
//
//-----------------------------------------------------------------------------


#ifndef r_splats_h
#define r_splats_h 1

#include "r_defs.h"

#define WALLSPLATS      // comment this out to compile without splat effects
//#define FLOORSPLATS

#define MAXLEVELSPLATS      1024

// splat flags
#define SPLATDRAWMODE_MASK   0x03       // mask to get drawmode from flags
#define SPLATDRAWMODE_OPAQUE 0x00
#define SPLATDRAWMODE_SHADE  0x01
#define SPLATDRAWMODE_TRANS  0x02
/*
#define SPLATUPPER           0x04
#define SPLATLOWER           0x08
*/
// ==========================================================================
// DEFINITIONS
// ==========================================================================

// WALL SPLATS are patches drawn on top of wall segs
struct wallsplat_t
{
  class Texture *tex;  // texture
  vertex_t    v1;      // vertices along the linedef
  vertex_t    v2;
  fixed_t     top;
  fixed_t     offset;     // offset in columns<<FRACBITS from start of linedef to start of splat
  int         flags;
  int*        yoffset;
  //short       xofs, yofs;
  //int         tictime;
  line_t      *line;       // the parent line of the splat seg
  wallsplat_t *next;
};

// FLOOR SPLATS are pic_t (raw horizontally stored) drawn on top of the floor or ceiling
struct floorsplat_t
{
  Texture *tex; // texture
  int           flags;
  vertex_t      verts[4];   // (x,y) as viewn from above on map
  fixed_t       z;          //     z (height) is constant for all the floorsplat
  subsector_t  *subsector;       // the parent subsector
  floorsplat_t *next;
  floorsplat_t *nextvis;
};

//p_setup.c
extern float P_SegLength (seg_t* seg);

// call at P_SetupLevel()
void R_ClearLevelSplats();

//void R_AddWallSplat(line_t* wallline, int sectorside, char* patchname, fixed_t top, fixed_t wallfrac, int flags);
void R_AddFloorSplat(subsector_t* subsec, char* picname, fixed_t x, fixed_t y, fixed_t z, int flags);

void R_ClearVisibleFloorSplats();
void R_AddVisibleFloorSplats(subsector_t* subsec);
void R_DrawVisibleFloorSplats();


#endif
