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
// Revision 1.1  2002/11/16 14:18:22  hurdler
// Initial revision
//
// Revision 1.9  2002/09/06 17:18:35  vberghol
// added most of the changes up to RC2
//
// Revision 1.8  2002/08/19 18:06:41  vberghol
// renderer somewhat fixed
//
// Revision 1.7  2002/08/16 20:49:26  vberghol
// engine ALMOST done!
//
// Revision 1.6  2002/08/08 18:36:26  vberghol
// p_spec.cpp fixed
//
// Revision 1.5  2002/07/18 19:16:39  vberghol
// renamed a few files
//
// Revision 1.4  2002/07/10 19:57:02  vberghol
// g_pawn.cpp tehty
//
// Revision 1.3  2002/07/01 21:00:44  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:22  vberghol
// Version 133 Experimental!
//
// Revision 1.4  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      all external data is defined here
//      most of the data is loaded into different structures at run time
//      some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------

#ifndef __DOOMDATA__
#define __DOOMDATA__

// The most basic types we use, portability.
#include "doomtype.h"

// Some global defines, that configure the game.
#include "doomdef.h"



//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
  ML_LABEL,             // A separator, name, ExMx or MAPxx
  ML_THINGS,            // Monsters, items..
  ML_LINEDEFS,          // LineDefs, from editing
  ML_SIDEDEFS,          // SideDefs, from editing
  ML_VERTEXES,          // Vertices, edited and BSP splits generated
  ML_SEGS,              // LineSegs, from LineDefs split by BSP
  ML_SSECTORS,          // SubSectors, list of LineSegs
  ML_NODES,             // BSP nodes
  ML_SECTORS,           // Sectors, from editing
  ML_REJECT,            // LUT, sector-sector visibility        
  ML_BLOCKMAP           // LUT, motion clipping, walls/grid element
};


// A single Vertex.
struct mapvertex_t
{
  short         x;
  short         y;
};


// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
struct mapsidedef_t
{
  short         textureoffset;
  short         rowoffset;
  char          toptexture[8];
  char          bottomtexture[8];
  char          midtexture[8];
  // Front sector, towards viewer.
  short         sector;
};



// A LineDef, as used for editing, and as input
// to the BSP builder.
struct maplinedef_t
{
  short         v1;
  short         v2;
  short         flags;
  short         special;
  short         tag;
  // sidenum[1] will be -1 if one sided
  short         sidenum[2];             
};


//
// LineDef attributes.
//

enum {
  // Solid, is an obstacle.
  ML_BLOCKING = 1,

  // Blocks monsters only.
  ML_BLOCKMONSTERS = 2,

  // Backside will not be present at all
  //  if not two sided.
  ML_TWOSIDED = 4,

  // If a texture is pegged, the texture will have
  // the end exposed to air held constant at the
  // top or bottom of the texture (stairs or pulled
  // down things) and will move with a height change
  // of one of the neighbor sectors.
  // Unpegged textures allways have the first row of
  // the texture at the top pixel of the line for both
  // top and bottom textures (use next to windows).

  // upper texture unpegged
  ML_DONTPEGTOP = 8,

  // lower texture unpegged
  ML_DONTPEGBOTTOM = 16,

  // In AutoMap: don't map as two sided: IT'S A SECRET!
  ML_SECRET = 32,

  // Sound rendering: don't let sound cross two of these.
  ML_SOUNDBLOCK = 64,

  // Don't draw on the automap at all.
  ML_DONTDRAW = 128,

  // Set if already seen, thus drawn in automap.
  ML_MAPPED = 256,

  //SoM: 3/29/2000: If flag is set, the player can use through it.
  ML_PASSUSE = 512,

  //SoM: 4/1/2000: If flag is set, anything can trigger the line.
  ML_ALLTRIGGER = 1024,
};


// Sector definition, from editing.
struct mapsector_t
{
  short         floorheight;
  short         ceilingheight;
  char          floorpic[8];
  char          ceilingpic[8];
  short         lightlevel;
  short         special;
  short         tag;
};

// SubSector, as generated by BSP.
struct mapsubsector_t
{
  short         numsegs;
  // Index of first one, segs are stored sequentially.
  short         firstseg;       
};


// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
struct mapseg_t
{
  short         v1;
  short         v2;
  short         angle;          
  short         linedef;
  short         side;
  short         offset;
};



// BSP node structure.

// Indicate a leaf.
//const int NF_SUBSECTOR = 0x8000;
#define NF_SUBSECTOR    0x8000

struct mapnode_t
{
  // Partition line from (x,y) to x+dx,y+dy)
  short         x;
  short         y;
  short         dx;
  short         dy;

  // Bounding box for each child,
  // clip against view frustum.
  short         bbox[2][4];

  // If NF_SUBSECTOR its a subsector,
  // else it's a node of another subtree.
  unsigned short        children[2];
};




// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
struct mapthing_t
{
  short x, y, z;
  short angle;
  short type;
  short flags;
  class Actor *mobj;
};

// mapthing_t flags
enum {
  // at which skill is it present?
  MTF_EASY   = 1,
  MTF_NORMAL = 2,
  MTF_HARD   = 4,
  // Deaf monsters/do not react to sound.
  MTF_AMBUSH = 8,
  // only present in multiplayer
  MTF_MULTIPLAYER = 16,
  // extra BOOM flags
  MTF_NOT_IN_DM   = 32, 
  MTF_NOT_IN_COOP = 64
};


extern char *Color_Names[MAXSKINCOLORS];


#endif                  // __DOOMDATA__
