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
// Revision 1.6  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.5  2003/04/14 08:58:30  smite-meister
// Hexen maps load.
//
// Revision 1.4  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.3  2003/03/08 16:07:14  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Most external data structures are defined here.
//      Most of the data is loaded into different structures at run time.
//      Some internal structures shared by many modules are here.
//
//-----------------------------------------------------------------------------

#ifndef doomdata_h
#define doomdata_h 1

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
  ML_BLOCKMAP,          // LUT, motion clipping, walls/grid element
  ML_BEHAVIOR  // Hexen script lump
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
  short v1, v2;
  short flags;
  short special;
  short tag;
  // sidenum[1] will be -1 if one sided
  short sidenum[2];             
};

// extended Hexen maplinedef
struct hex_maplinedef_t
{
  short v1, v2;
  short flags;
  byte special;
  byte args[5];
  short sidenum[2]; // sidenum[1] will be -1 if one sided
};


//
// LineDef attributes.
//

enum {
  // Solid, is an obstacle.
  ML_BLOCKING = 0x0001,

  // Blocks monsters only.
  ML_BLOCKMONSTERS = 0x0002,

  // Backside will not be present at all
  //  if not two sided.
  ML_TWOSIDED = 0x0004,

  // If a texture is pegged, the texture will have
  // the end exposed to air held constant at the
  // top or bottom of the texture (stairs or pulled
  // down things) and will move with a height change
  // of one of the neighbor sectors.
  // Unpegged textures allways have the first row of
  // the texture at the top pixel of the line for both
  // top and bottom textures (use next to windows).

  // upper texture unpegged
  ML_DONTPEGTOP = 0x0008,

  // lower texture unpegged
  ML_DONTPEGBOTTOM = 0x0010,

  // In AutoMap: don't map as two sided: IT'S A SECRET!
  ML_SECRET = 0x0020,

  // Sound rendering: don't let sound cross two of these.
  ML_SOUNDBLOCK = 0x0040,

  // Don't draw on the automap at all.
  ML_DONTDRAW = 0x0080,

  // Set if already seen, thus drawn in automap.
  ML_MAPPED = 0x0100,

  //SoM: 3/29/2000: If flag is set, the player can use through it.
  ML_PASSUSE = 0x0200,
  ML_REPEAT_SPECIAL = 0x0200, // Hexen: special is repeatable

  //SoM: 4/1/2000: If flag is set, anything can trigger the line.
  ML_ALLTRIGGER = 0x0400,
  ML_SPAC_SHIFT	= 10,    // Hexen: Special activation, 3 bit field
  ML_SPAC_MASK	= 0x1c00,

  // Special activation types (3 bits)
  SPAC_CROSS  =	0,	// when player crosses line
  SPAC_USE    = 1,	// when player uses line
  SPAC_MCROSS =	2,	// when monster crosses line
  SPAC_IMPACT =	3,	// when projectile hits line
  SPAC_PUSH   =	4,	// when player/monster pushes line
  SPAC_PCROSS =	5,	// when projectile crosses line
  // new types
  SPAC_PASSUSE = 6,  // Converted Boom ML_PASSUSE

  ML_MONSTERS_CAN_ACTIVATE = 0x2000,
};

#define GET_SPAC(flags) (((flags) & ML_SPAC_MASK) >> ML_SPAC_SHIFT)



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
struct doom_mapthing_t
{
  short x, y;
  short angle;
  short type;
  short flags;
};

// extended Hexen mapthing
struct hex_mapthing_t
{
  short tid;
  short x, y;
  short height;
  short angle;
  short type;
  short flags;
  byte special;
  byte args[5];
};

// Legacy runtime mapthing
struct mapthing_t
{
  short tid;
  short x, y, z;
  short angle;
  short type;
  short flags;
  byte special;
  byte args[5];

  class Actor *mobj;
};

// mapthing_t flags
enum {
  // original Doom flags
  MTF_EASY   = 0x0001,      // at which skill is it present?
  MTF_NORMAL = 0x0002,
  MTF_HARD   = 0x0004,
  MTF_AMBUSH = 0x0008,      // Deaf monsters/do not react to sound.
  MTF_MULTIPLAYER = 0x0010, // only present in multiplayer

  // BOOM extras
  MTF_NOT_IN_DM   = 0x0020,  
  MTF_NOT_IN_COOP = 0x0040,

  // Hexen extras
  MTF_DORMANT  = 0x0010,  // dormant until activated     
  MTF_FIGHTER  = 0x0020,  // appears for fighter
  MTF_CLERIC   = 0x0040,  // appears for cleric
  MTF_MAGE     = 0x0080,  // appears for mage
  MTF_GSINGLE  = 0x0100,   // appears in single player games
  MTF_GCOOP    = 0x0200,   // appears in coop games
  MTF_GDEATHMATCH = 0x0400 // appears in dm games
};


extern char *Color_Names[MAXSKINCOLORS];


#endif
