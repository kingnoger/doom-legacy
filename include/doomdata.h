// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.14  2006/02/08 19:09:27  jussip
// Added beginnings of a new OpenGL renderer.
//
// Revision 1.13  2006/01/04 23:15:08  jussip
// Read and convert GL nodes if they exist.
//
// Revision 1.12  2005/06/05 19:32:26  smite-meister
// unsigned map structures
//
// Revision 1.11  2004/11/28 18:02:23  smite-meister
// RPCs finally work!
//
// Revision 1.10  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.8  2004/08/14 16:28:38  smite-meister
// removed libtnl.a
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief External data structures for Doom/Hexen maps.
///
/// The following data structures define the persistent format
/// used in the lumps of the WAD files.
/// The data is transferred into (different) runtime structs during
/// map setup. The corresponding runtime structs are defined in r_defs.h.

#ifndef doomdata_h
#define doomdata_h 1

// The most basic types we use, portability.
#include "doomtype.h"

const Uint16 NULL_INDEX = 0xFFFF; // or -1. Used for sidenums and blocklist linedefnums ONLY.

#define GL2_HEADER "gNd2"
#define GL5_HEADER "gNd5"

#define VERT_IS_GL_V2 (1 << 15)
#define VERT_IS_GL_V5 (1 << 31)
#define VERT_IS_GL (1 << 31)

#define CHILD_IS_SUBSEC_V2 (1 << 15)
#define CHILD_IS_SUBSEC_V5 (1 << 31)
#define CHILD_IS_SUBSEC (1 << 31)

// TODO tags to unsigned? lightlevels too?


/// Lump order in a map WAD: each map needs a couple of lumps
/// to provide a complete scene geometry description.
enum map_lump_e
{
  LUMP_LABEL,    ///< A separator, name, ExMx or MAPxx
  LUMP_THINGS,   ///< Monsters, items..
  LUMP_LINEDEFS, ///< LineDefs, from editing
  LUMP_SIDEDEFS, ///< SideDefs, from editing
  LUMP_VERTEXES, ///< Vertices, edited and BSP splits generated
  LUMP_SEGS,     ///< LineSegs, from LineDefs split by BSP
  LUMP_SSECTORS, ///< SubSectors, list of LineSegs
  LUMP_NODES,    ///< BSP nodes
  LUMP_SECTORS,  ///< Sectors, from editing
  LUMP_REJECT,   ///< LUT, sector-sector visibility        
  LUMP_BLOCKMAP, ///< LUT, motion clipping, walls/grid element
  LUMP_BEHAVIOR  ///< Hexen ACS script lump
};

/// Lump order for GL nodes.
enum map_gllump_e
{
  LUMP_GL_LABEL,    ///< Separator, like GL_MAP01
  LUMP_GL_VERTEXES, ///< Extra vertices
  LUMP_GL_SEGS,     ///< Segs suitable for GL rendering
  LUMP_GL_SSECT,    ///< Truely convex GL subsectors
  LUMP_GL_NODES,    ///< GL BSP nodes
  LUMP_GL_PVS,      ///< Subsector to subsector visibility info.
};


/// \brief A single Vertex.
struct mapvertex_t
{
  Sint16  x, y;
} __attribute__((packed));



/// \brief SideDef, defining the visual appearance of a wall, by setting textures and offsets.
struct mapsidedef_t
{
  Sint16        textureoffset; ///< x texture offset
  Sint16        rowoffset;     ///< y texture offset
  char          toptexture[8];
  char          bottomtexture[8];
  char          midtexture[8];
  Uint16        sector;        ///< front sector, towards viewer.
} __attribute__((packed));



/// \brief LineDef, a border between sectors. Used as input to the BSP builder.
struct doom_maplinedef_t
{
  Uint16 v1, v2;     ///< numbers of start and end vertices
  Uint16 flags;
  Uint16 special;    ///< line action
  Sint16 tag;
  Uint16 sidenum[2]; ///< numbers of sidedefs. sidenum[1] will be 0xFFFF if one-sided.
} __attribute__((packed));



/// \brief The Hexen LineDef type.
struct hex_maplinedef_t
{
  Uint16 v1, v2;     ///< numbers of start and end vertices
  Uint16 flags;
  byte   special;    ///< line action
  byte   args[5];    ///< arguments for the action
  Uint16 sidenum[2]; ///< numbers of sidedefs. sidenum[1] will be 0xFFFF if one-sided.
} __attribute__((packed));



/// \brief Sector definition, from editing.
struct mapsector_t
{
  Sint16        floorheight;   ///< floor z coordinate
  Sint16        ceilingheight; ///< ceiling z coordinate
  char          floorpic[8];   ///< floor texture name
  char          ceilingpic[8]; ///< ceiling texture name
  Sint16        lightlevel;
  Uint16        special;       ///< sector action
  Sint16        tag;
} __attribute__((packed));



/// \brief SubSector, as generated by BSP.
struct mapsubsector_t
{
  Uint16  numsegs;  ///< number of segs surrounding this subsector
  Uint16  firstseg; ///< index of first seg, the rest are stored sequentially.
} __attribute__((packed));



/// \brief LineSeg, generated by splitting LineDefs
/// using partition lines selected by BSP builder.
struct mapseg_t
{
  Uint16  v1, v2;  ///< numbers of start and end vertices
  Sint16  angle;   ///< orientation
  Uint16  linedef; ///< number of linedef to which it belongs
  Uint16  side;    ///< 0 or 1 depending on which side of the linedef this seg belongs
  Sint16  offset;  ///< position of the start vertex of the seg along the linedef
} __attribute__((packed));



/// \brief BSP node
struct mapnode_t
{
  /// Partition line from (x,y) to x+dx,y+dy)
  Sint16  x, y;
  Sint16  dx, dy;

  /// Bounding box for each child, clip against view frustum.
  Sint16  bbox[2][4];

  /// If NF_SUBSECTOR its a subsector, else it's a node of another subtree.
  Uint16 children[2];
} __attribute__((packed));



/// \brief Thing definition. Position, orientation and type,
/// plus skill/visibility flags and attributes.
struct doom_mapthing_t
{
  Sint16 x, y;   ///< coordinates
  Sint16 angle;  ///< orientation
  Uint16 type;   ///< DoomEd number
  Uint16 flags;
} __attribute__((packed));



/// \brief Hexen Thing
struct hex_mapthing_t
{
  Sint16 tid;    ///< thing ID, for grouping things
  Sint16 x, y;   ///< coordinates
  Sint16 height; ///< height above sector floor
  Sint16 angle;  ///< orientation
  Uint16 type;   ///< DoomEd number
  Uint16 flags;
  byte special;  ///< thing action
  byte args[5];  ///< arguments for the action
} __attribute__((packed));



/// \brief Blockmap header
///
/// The header is followed by a width*height array of Uint16 offsets to the blocklists.
/// Each blocklist is situated at blockmaplump + sizeof(Uint16)*offset and consists of a list of
/// Uint16 linedef numbers terminated by 0xFFFF.
struct blockmapheader_t
{
  Sint16 origin_x, origin_y; ///< coordinates of the bottom-left corner of the blockmap
  Uint16 width, height;      ///< blockmap size in x and y directions (measured in blocks)
} __attribute__((packed));



//===========================================================
//   Not directly map-related data
//===========================================================

/// \brief A DoomTexture patch constituent.
///
/// Each texture is composed of one or more patches,
/// with patches being lumps stored in the WAD.
/// The lumps are referenced by number, and patched
/// into the rectangular texture space using origin
/// and possibly other attributes.
struct mappatch_t
{
  Sint16  originx, originy;  ///< origin of the patch inside the texture
  Uint16  patch;     ///< number of the patch to use
  Uint16  stepdir;   ///< always 1
  Uint16  colormap;  ///< always 0
} __attribute__((packed));



/// \brief A DoomTexture definition in a TEXTUREx lump.
///
/// A Doom wall texture is a list of patches
/// which are to be combined in a predefined order.
struct maptexture_t
{
  char         name[8];         ///< texture name
  Uint16       flags;           ///< extension, used to be zero
  byte         xscale, yscale;  ///< extension, used to be zero
  Uint16       width, height;   ///< texture dimensions
  Uint32       columndirectory; ///< unused, always zero (used to be void**)
  Uint16       patchcount;      ///< number of patches constituting the texture
  mappatch_t   patches[1];      ///< array of the patches
} __attribute__((packed));



/// \brief Header for Doom native sound format.
///
/// First a 8-byte header composed of 4 unsigned (16-bit) short integers,
/// then the data (8-bit 11 kHz mono sound).
/// Max. # of samples = 65535 = about 6 seconds of sound.
struct doomsfx_t
{
  Uint16 magic;   ///< always 3
  Uint16 rate;    ///< always 11025
  Uint16 samples; ///< number of 1-byte samples
  Uint16 zero;    ///< always 0
  byte   data[0]; ///< actual data begins here
} __attribute__((packed));



/// \brief Template for the Boom ANIMATED lump.
///
/// Used for defining texture and flat animation sequences.
struct ANIMATED_t
{
  char   istexture;    ///< 1 means texture, 0 means flat, -1 is a terminator
  char   endname[9];   ///< last texture of the animation, NUL-terminated
  char   startname[9]; ///< first texture of the animation, NUL-terminated
  Uint32 speed;        ///< duration of one frame in tics
} __attribute__((packed));



/// \brief Template for the Boom SWITCHES lump.
struct SWITCHES_t
{
  char   name1[9]; ///< texture name for the ON position
  char   name2[9]; ///< texture name for the OFF position
  Uint16 episode; 
} __attribute__((packed));

/// \brief GL v2 Vertex
///
/// Fortunately v2 and v5 vertices are identical except for the header.

struct mapglvertex_t
{
  Sint32 x; ///< X-coordinate in 16.16 fixed point
  Sint32 y; ///< Y-coordinate in 16.16 fixed point
} __attribute__((packed));

/// \brief GL v2 seg
///
/// A single wall segment.

struct mapgl2seg_t
{
  Uint16 start_vertex; ///< From this vertex...
  Uint16 end_vertex;   ///< ... to this one.
  Uint16 linedef;      ///< Along this linedef.
  Uint16 side;         ///< 0 means right side, 1 means left side.
  Uint16 partner_seg;  ///< Which GL seg is on the other side of the linedef.
} __attribute__((packed));

/// \brief GL v5 seg
///
/// A single wall segment.

struct mapgl5seg_t
{
  Uint32 start_vertex; ///< From this vertex...
  Uint32 end_vertex;   ///< ... to this one.
  Uint16 linedef;      ///< Along this linedef.
  Uint16 side;         ///< 0 means right side, 1 means left side.
  Uint32 partner_seg;  ///< Which GL seg is on the other side of the linedef.
} __attribute__((packed));

/// \brief GL v2 subsector.
///
/// A single convex area ready for GL rendering.

struct mapgl2subsector_t
{
  Uint16 count;       ///< Read this many segs.
  Uint16 first_seg;   ///< Starting with this
} __attribute__((packed));

/// \brief GL v5 subsector.
///
/// A single convex area ready for GL rendering.

struct mapgl5subsector_t
{
  Uint32 count;       ///< Read this many segs.
  Uint32 first_seg;   ///< Starting with this
} __attribute__((packed));

/// \brief GL v2 Node
///
/// A single node in the BSP tree

struct mapgl2node_t
{
  Sint16 x;           
  Sint16 y;           
  Sint16 dx;          
  Sint16 dy;          
  Sint16 bbox[2][4];  ///< Bounding boxes for children
  Uint16 children[2]; ///< Offsets to children.
} __attribute__((packed));

/// \brief GL v5 Node
///
/// A single node in the BSP tree

struct mapgl5node_t
{
  Sint16 x;           
  Sint16 y;           
  Sint16 dx;
  Sint16 dy;          
  Sint16 bbox[2][4];  ///< Bounding boxes for children
  Uint32 children[2]; ///< Offsets to children.
} __attribute__((packed));


#endif
