// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.4  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.3  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1.1.1  2002/11/16 14:18:27  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Decals, wall and floor splats.

#ifndef r_splats_h
#define r_splats_h 1

#include "m_fixed.h"
#include "r_defs.h"


#define MAXLEVELSPLATS 1024

/// splat flags
enum splatdrawmode_e
{
  SPLATDRAWMODE_OPAQUE = 0x00,
  SPLATDRAWMODE_SHADE  = 0x01,
  SPLATDRAWMODE_TRANS  = 0x02,

  SPLATDRAWMODE_MASK =  0x03 // mask to get drawmode from flags
};


/// \brief Textures drawn on top of wall segs
struct wallsplat_t
{
  class Texture *tex;  ///< splat texture
  vertex_t    v1, v2;  ///< vertices along the linedef
  fixed_t     top;
  fixed_t     offset;  ///< offset in columns<<FRACBITS from start of linedef to start of splat
  int         flags;
  int        *yoffset;
  //short       xofs, yofs;
  //int         tictime;
  line_t      *line;  ///< parent line of the splat seg
  wallsplat_t *next;
};


void R_ClearLevelSplats();

//void R_AddWallSplat(line_t* wallline, int sectorside, char* patchname, fixed_t top, fixed_t wallfrac, int flags);



//#define FLOORSPLATS
#ifdef FLOORSPLATS

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

void R_AddFloorSplat(subsector_t* subsec, char* picname, fixed_t x, fixed_t y, fixed_t z, int flags);
void R_ClearVisibleFloorSplats();
void R_AddVisibleFloorSplats(subsector_t* subsec);
void R_DrawVisibleFloorSplats();
#endif


#endif
