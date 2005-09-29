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
// Revision 1.20  2005/09/29 15:35:27  smite-meister
// JDS texture standard
//
// Revision 1.19  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.18  2005/07/31 14:50:25  smite-meister
// thing spawning fix
//
// Revision 1.17  2005/06/05 19:32:27  smite-meister
// unsigned map structures
//
// Revision 1.15  2005/01/04 18:32:44  smite-meister
// better colormap handling
//
// Revision 1.14  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.13  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.12  2004/10/14 19:35:50  smite-meister
// automap, bbox_t
//
// Revision 1.9  2004/08/18 14:35:20  smite-meister
// PNG support!
//
// Revision 1.8  2004/07/25 20:18:47  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.7  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.6  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.5  2003/04/14 08:58:31  smite-meister
// Hexen maps load.
//
// Revision 1.4  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.3  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.2  2003/03/08 16:07:16  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.1.1.1  2002/11/16 14:18:27  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.30  2001/08/12 17:57:15  hurdler
// Beter support of sector coloured lighting in hw mode
//
// Revision 1.29  2001/08/11 15:18:02  hurdler
// Add sector colormap in hw mode (first attempt)
//
// Revision 1.28  2001/08/09 21:35:17  hurdler
// Add translucent 3D water in hw mode
//
// Revision 1.27  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.26  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.25  2001/05/30 04:00:52  stroggonmeth
// Fixed crashing bugs in software with 3D floors.
//
// Revision 1.24  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.23  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.21  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.20  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.19  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.17  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.16  2000/11/21 21:13:17  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.15  2000/11/09 17:56:20  stroggonmeth
// Hopefully fixed a few bugs and did a few optimizations.
//
// Revision 1.14  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.13  2000/10/07 20:36:13  crashrl
// Added deathmatch team-start-sectors via sector/line-tag and linedef-type 1000-1031
//
// Revision 1.6  2000/04/12 16:01:59  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.4  2000/04/06 20:47:08  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Runtime structs for map data.

#ifndef r_defs_h
#define r_defs_h 1

// Some more or less basic data types we depend on.
#include "m_fixed.h"
#include "m_bbox.h"


/// \brief Your plain vanilla vertex
struct vertex_t
{
  fixed_t  x, y;
};


// Each sector has a mappoint_t in its center
//  for sound origin purposes.
// I suppose this does not handle sound from
//  moving objects (doppler), because
//  position is prolly just buffered, not
//  updated.
// FIXME change to vect<fixed_t> ?
struct mappoint_t
{
  fixed_t x, y, z;
};


/// "Fake floor" types
enum ffloortype_e
{
  FF_EXISTS            = 0x1,    ///< MAKE SURE IT'S VALID
  FF_SOLID             = 0x2,    ///< Does it clip things?
  FF_RENDERSIDES       = 0x4,    ///< Render the sides?
  FF_RENDERPLANES      = 0x8,    ///< Render the floor/ceiling?
  FF_RENDERALL         = 0xC,    ///< Render everything?
  FF_SWIMMABLE         = 0x10,   ///< Can we swim?
  FF_NOSHADE           = 0x20,   ///< Does it mess with the lighting?
  FF_CUTSOLIDS         = 0x40,   ///< Does it cut out hidden solid pixles?
  FF_CUTEXTRA          = 0x80,   ///< Does it cut out hidden translucent pixles?
  FF_CUTLEVEL          = 0xC0,   ///< Does it cut out all hidden pixles?
  FF_CUTSPRITES        = 0x100,  ///< Final Step in 3D water
  FF_BOTHPLANES        = 0x200,  ///< Render both planes all the time?
  FF_EXTRA             = 0x400,  ///< Does it get cut by FF_CUTEXTRAS?
  FF_TRANSLUCENT       = 0x800,  ///< See through!
  FF_FOG               = 0x1000, ///< Fog "brush"?
  FF_INVERTPLANES      = 0x2000, ///< Reverse the plane visibility rules?
  FF_ALLSIDES          = 0x4000, ///< Render inside and outside sides?
  FF_INVERTSIDES       = 0x8000, ///< Only render inside sides?
  FF_DOUBLESHADOW      = 0x10000,///< Make two lightlist entries to reset light?
};


/// \brief Fake floor, better known as 3D floor:)
///
/// Store fake planes in a resizable array insted of just by
/// heightsec. Allows for multiple fake planes.
struct ffloor_t
{
  fixed_t          *topheight;
  short            *toppic;
  short            *toplightlevel;
  fixed_t          *topxoffs;
  fixed_t          *topyoffs;

  fixed_t          *bottomheight;
  short            *bottompic;
  short            *bottomlightlevel;
  fixed_t          *bottomxoffs;
  fixed_t          *bottomyoffs;

  fixed_t          delta;

  int              secnum;
  ffloortype_e     flags;
  struct line_t*   master;

  struct sector_t* target;

  ffloor_t* next;
  ffloor_t* prev;

  int              lastlight;
  int              alpha;
};




/// Sector floor properties
enum floortype_t
{
  FLOOR_SOLID,
  FLOOR_ICE,
  FLOOR_LIQUID,
  FLOOR_WATER,
  FLOOR_LAVA,
  FLOOR_SLUDGE
};


/// \brief Runtime map sector
struct sector_t
{
  fixed_t  floorheight, ceilingheight;
  short    floorpic, ceilingpic;
  short    lightlevel;
  short    special, tag;
  int      nexttag, firsttag; //SoM: 3/6/2000: by killough: improves searches for tags.

  short     soundtraversed; // 0 = untraversed, 1,2 = sndlines -1
  class Actor *soundtarget; // thing that made a sound (or null)

  short      seqType;   // sector sound sequence
  mappoint_t soundorg;  // origin for any sounds played by the sector

  short  floortype;   // see floortype_t

  int    blockbox[4]; // mapblock bounding box for height changes

  int     validcount; // if == validcount, already checked
  Actor  *thinglist;  // list of mobjs in sector

  //SoM: 3/6/2000: Start boom extra stuff
  // Thinker for reversable actions
  class Thinker *floordata; // make thinkers on
  Thinker *ceilingdata;  // floors, ceilings, lighting,
  Thinker *lightingdata; // independent of one another

  // lockout machinery for stairbuilding
  int stairlock;   // -2 on first locked -1 after thinker done 0 normally
  int prevsec;     // -1 or number of sector for previous step
  int nextsec;     // -1 or number of next step sector

  /// floor and ceiling texture offsets
  fixed_t   floor_xoffs,   floor_yoffs;
  fixed_t ceiling_xoffs, ceiling_yoffs;

  int heightsec;      ///< control sector number, or -1 if none
  int heightsec_type; ///< what type of control sector is it?

  enum controlsector_t
  {
    CS_boom = 0,
    CS_water,
    CS_colormap // FIXME probably unneeded
  };

  int floorlightsec, ceilinglightsec;
  int teamstartsec;

  // TODO this could be replaced with a pointer to a sectorinfo struct
  // (to save space, since most sectors have default values for these...)
  short damage, damagetype; // TEST given according to damage bits in 'special'
  float gravity;  // TEST
  float friction, movefactor;  // friction belongs here, not in Actor

  class fadetable_t *bottommap, *midmap, *topmap; // dynamic colormaps

  // list of mobjs that are at least partially in the sector
  // thinglist is a subset of touching_thinglist
  struct msecnode_t *touching_thinglist;               // phares 3/14/98
  //SoM: 3/6/2000: end stuff...

  int       linecount;
  line_t**  lines;  // [linecount] size

  //SoM: 2/23/2000: Improved fake floor hack
  ffloor_t                  *ffloors;
  int                       *attached;
  int                        numattached;
  struct lightlist_t        *lightlist;
  int                        numlights;
  bool                       moved;

  int                        validsort; //if == validsort allready been sorted
  bool                       added;

  // SoM: 4/3/2000: per-sector colormaps!
  fadetable_t *extra_colormap;

  // ----- for special tricks with HW renderer -----
  bool                       pseudoSector;
  bool                       virtualFloor;
  fixed_t                    virtualFloorheight;
  bool                       virtualCeiling;
  fixed_t                    virtualCeilingheight;
  struct linechain_t        *sectorLines;
  sector_t                 **stackList;
  double                     lineoutLength;
  // ----- end special tricks -----

public:
  fixed_t FindLowestFloorSurrounding();
  fixed_t FindHighestFloorSurrounding();
  fixed_t FindLowestCeilingSurrounding();
  fixed_t FindHighestCeilingSurrounding();

  fixed_t FindNextLowestFloor(fixed_t currentheight);
  fixed_t FindNextHighestFloor(fixed_t currentheight);
  fixed_t FindNextLowestCeiling(fixed_t currentheight);
  fixed_t FindNextHighestCeiling(fixed_t currentheight);

  int FindMinSurroundingLight(int max);
  enum special_e
  {
    floor_special,
    ceiling_special,
    lighting_special,
  };
  bool Active(special_e t);

  void SetFloorType(const char *floorpic);
};



/// \brief SideDef
struct side_t
{
  /// add this to the calculated texture column
  fixed_t textureoffset;

  /// add this to the calculated texture top
  fixed_t rowoffset;

  /// Texture indices. We do not maintain names here.
  short toptexture;
  short bottomtexture;
  short midtexture;

  /// Sector the SideDef is facing.
  sector_t *sector;

  /// LineDef the SideDef belongs to.
  line_t   *line;
};



/// Move clipping aid for LineDefs.
enum slopetype_t
{
  ST_HORIZONTAL,
  ST_VERTICAL,
  ST_POSITIVE,
  ST_NEGATIVE
};


/// LineDef flags
enum line_flags_e
{
  /// Solid, is an obstacle.
  ML_BLOCKING = 0x0001,

  /// Blocks monsters only.
  ML_BLOCKMONSTERS = 0x0002,

  /// Backside will not be present at all if not twosided.
  ML_TWOSIDED = 0x0004,

  // If a texture is pegged, the texture will have
  // the end exposed to air held constant at the
  // top or bottom of the texture (stairs or pulled
  // down things) and will move with a height change
  // of one of the neighbor sectors.
  // Unpegged textures allways have the first row of
  // the texture at the top pixel of the line for both
  // top and bottom textures (use next to windows).

  /// upper texture unpegged
  ML_DONTPEGTOP = 0x0008,

  /// lower texture unpegged
  ML_DONTPEGBOTTOM = 0x0010,

  /// In AutoMap: don't map as two sided: IT'S A SECRET!
  ML_SECRET = 0x0020,

  /// Sound rendering: don't let sound cross two of these.
  ML_SOUNDBLOCK = 0x0040,

  /// Don't draw on the automap at all.
  ML_DONTDRAW = 0x0080,

  /// Set if already seen, thus drawn in automap.
  ML_MAPPED = 0x0100,

  /// Boom: "player use" can extend through the line
  ML_PASSUSE = 0x0200,
  /// Hexen: special is repeatable
  ML_REPEAT_SPECIAL = 0x0200,

  /// Anything can trigger the line.
  ML_ALLTRIGGER = 0x0400,

  /// Hexen: Special activation, 3 bit field
  ML_SPAC_MASK	= 0x1c00,
  ML_SPAC_SHIFT	= 10,

  // Hexen: Special activation types (3 bits)
  SPAC_CROSS  =	0,	///< when player crosses line
  SPAC_USE    = 1,	///< when player uses line
  SPAC_MCROSS =	2,	///< when monster crosses line
  SPAC_IMPACT =	3,	///< when projectile hits line
  SPAC_PUSH   =	4,	///< when player/monster pushes line
  SPAC_PCROSS =	5,	///< when projectile crosses line
  // new types
  SPAC_PASSUSE = 6,     ///< Converted Boom ML_PASSUSE

  /// internal: converted Alltrigger
  ML_MONSTERS_CAN_ACTIVATE = 0x2000,

  /// internal: Boom generalized linedef
  ML_BOOM_GENERALIZED = 0x4000,
};

#define GET_SPAC(flags) (((flags) & ML_SPAC_MASK) >> ML_SPAC_SHIFT)


/// \brief LineDef
struct line_t
{
  /// Vertices, from v1 to v2.
  vertex_t *v1, *v2;

  /// Precalculated v2 - v1 for side checking.
  fixed_t   dx, dy;

  short flags;   ///< bit flags
  short special; ///< linedef type or special action
  short tag;
  int firsttag, nexttag;  ///< hash system, improves searches for tags.

  /// hexen args
  byte args[5];

  /// To aid move clipping.
  byte slopetype;

  /// Visual appearance: SideDefs. sideptr[1] will be NULL if one-sided.
  side_t *sideptr[2];

  /// Neat. Another bounding box, for the extent of the LineDef.
  bbox_t bbox;

  /// Front and back sector.
  // NOTE: redundant? Can be retrieved from SideDefs.
  sector_t *frontsector;
  sector_t *backsector;

  /// if == global validcount, already checked
  int validcount;

  /// Thinker for complex actions
  Thinker *thinker;

  /// wallsplat_t list
  struct wallsplat_t *splats;

  int transmap;   ///< translucency filter, -1 == none
  int ecolormap;  ///< SoM: Used for 282 linedefs
};



/// \brief SubSector
///
/// Each sector is divided into one or more convex subsectors during BSP nodebuilding.
/// Basically, this is a list of LineSegs, indicating the visible walls that define
/// (all or some) sides of a convex BSP leaf.
struct subsector_t
{
  sector_t *sector;    ///< to which sector does this subsector belong?
  Uint16    numlines;  ///< number of linesegs surrounding this subsector
  Uint16    firstline; ///< index into the segs array

  int       validcount;  // Hurdler: added for optimized mlook in hw mode

  struct floorsplat_t *splats; ///< floorsplat list
  struct polyobj_t    *poly;
};


/// \brief Sector list node, showing all sectors an object appears in.
///
/// There are two threads that flow through these nodes. The first thread
/// starts at touching_thinglist in a sector_t and flows through the m_snext
/// links to find all mobjs that are entirely or partially in the sector.
/// The second thread starts at touching_sectorlist in an Actor and flows
/// through the m_tnext links to find all sectors a thing touches. This is
/// useful when applying friction or push effects to sectors. These effects
/// can be done as thinkers that act upon all objects touching their sectors.
/// As an mobj moves through the world, these nodes are created and
/// destroyed, with the links changed appropriately.
///
/// For the links, NULL means top or end of list.
struct msecnode_t
{
  sector_t    *m_sector; ///< a sector containing this object
  Actor       *m_thing;  ///< this object
  msecnode_t  *m_tprev;  ///< prev msecnode_t for this thing
  msecnode_t  *m_tnext;  ///< next msecnode_t for this thing
  msecnode_t  *m_sprev;  ///< prev msecnode_t for this sector
  msecnode_t  *m_snext;  ///< next msecnode_t for this sector
  bool visited; ///< used in search algorithms
};



/// \brief LineSeg
struct seg_t
{
  vertex_t *v1, *v2;
  int       side;
  fixed_t   offset;
  angle_t   angle;
  side_t   *sidedef;
  line_t   *linedef;

  // Sector references.
  // Could be retrieved from linedef, too.
  // backsector is NULL for one sided lines
  sector_t *frontsector;
  sector_t *backsector;

  // lenght of the seg : used by the hardware renderer
  //float       length;

  //Hurdler: 04/12/2000: added for static lightmap
  //lightmap_t  *lightmaps;

  // SoM: Why slow things down by calculating lightlists for every thick side.
  int         numlights;
  struct r_lightlist_t *rlights;
};



/// \brief BSP node
struct node_t
{
  /// Partition line.
  fixed_t  x, y;
  fixed_t  dx, dy;

  /// Bounding box for each child.
  bbox_t bbox[2];

#define NF_SUBSECTOR    0x8000 ///< Indicates a BSP leaf == subsector.
  Uint16 children[2];
};




/// \brief Runtime mapthing
struct mapthing_t
{
  short tid;      ///< Thing ID (from Hexen)
  short x, y, z;  ///< coordinates
  short angle;    ///< orientation
  short type;     ///< DoomEd number
  short flags;
  byte special;   ///< thing action
  byte args[5];   ///< arguments for the thing action

  class Actor *mobj;
};


/// mapthing_t flags
enum mapthing_flags_e
{
  // original Doom flags
  MTF_EASY   = 0x0001,      ///< at which skill is it present?
  MTF_NORMAL = 0x0002,
  MTF_HARD   = 0x0004,
  MTF_AMBUSH = 0x0008,      ///< deaf monster/does not react to sound
  MTF_MULTIPLAYER = 0x0010, ///< only appears in multiplayer

  // BOOM extras
  MTF_NOT_IN_DM   = 0x0020, ///< does not appear in deathmatch
  MTF_NOT_IN_COOP = 0x0040, ///< does not appear in coop

  // Hexen extras
  MTF_DORMANT  = 0x0010,   ///< dormant until activated     
  MTF_FIGHTER  = 0x0020,   ///< appears for fighter
  MTF_CLERIC   = 0x0040,   ///< appears for cleric
  MTF_MAGE     = 0x0080,   ///< appears for mage
  MTF_GSINGLE  = 0x0100,   ///< appears in single player games
  MTF_GCOOP    = 0x0200,   ///< appears in coop games
  MTF_GDEATHMATCH = 0x0400 ///< appears in dm games
};


#endif
