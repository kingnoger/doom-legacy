// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Runtime structs for map data.

#ifndef r_defs_h
#define r_defs_h 1

// Some more or less basic data types we depend on.
#include "m_fixed.h"
#include "m_bbox.h"

/*!
  \defgroup g_mapgeometry Runtime Map geometry

  Runtime structures for Map geometry and related things.
*/

/// \brief Your plain vanilla vertex
/// \ingroup \g_mapgeometry
struct vertex_t
{
  fixed_t  x, y;

  inline bool operator==(const vertex_t& v) const { return (x == v.x) && (y == v.y); }
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
/// \ingroup \g_mapgeometry
enum ffloortype_e
{
  FF_EXISTS            = 0x0001,  ///< MAKE SURE IT'S VALID
  FF_SOLID             = 0x0002,  ///< Does it clip things?
  FF_RENDERSIDES       = 0x0004,  ///< Render the sides?
  FF_RENDERPLANES      = 0x0008,  ///< Render the floor/ceiling?
  FF_RENDERALL = FF_RENDERSIDES | FF_RENDERPLANES,  ///< Render everything?
  FF_SWIMMABLE         = 0x0010,  ///< Can we swim?
  FF_NOSHADE           = 0x0020,  ///< Does it mess with the lighting?
  FF_CUTSOLIDS         = 0x0040,  ///< Does it cut out hidden solid pixles?
  FF_CUTEXTRA          = 0x0080,  ///< Does it cut out hidden translucent pixles?
  FF_CUTLEVEL = FF_CUTSOLIDS | FF_CUTEXTRA,  ///< Does it cut out all hidden pixles?
  FF_CUTSPRITES        = 0x0100,  ///< Final Step in 3D water
  FF_BOTHPLANES        = 0x0200,  ///< Render both planes all the time?
  FF_EXTRA             = 0x0400,  ///< Does it get cut by FF_CUTEXTRAS?
  FF_TRANSLUCENT       = 0x0800,  ///< See through!
  FF_FOG               = 0x1000,  ///< Fog "brush"?
  FF_INVERTPLANES      = 0x2000,  ///< Reverse the plane visibility rules?
  FF_ALLSIDES          = 0x4000,  ///< Render inside and outside sides?
  FF_INVERTSIDES       = 0x8000,  ///< Only render inside sides?
  FF_DOUBLESHADOW     = 0x10000,  ///< Make two lightlist entries to reset light?
};


/// \brief Fake floor, better known as 3D floor:)
/// \ingroup \g_mapgeometry
///
/// Store fake planes in a resizable array instead of just by
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
/// \ingroup \g_mapgeometry
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
/// \ingroup \g_mapgeometry
struct sector_t
{
  fixed_t  floorheight, ceilingheight;
  short    floorpic, ceilingpic;
  short    lightlevel;
  short    special, tag;
  int      nexttag, firsttag; //SoM: 3/6/2000: by killough: improves searches for tags.

  short     soundtraversed; ///< Noise alert: # of MF_SOUNDBLOCK lines noise has crossed
  class Actor *soundtarget; ///< Noise alert: thing that made a sound

  short      seqType;   ///< sector sound sequence
  mappoint_t soundorg;  ///< origin for any sounds played by the sector

  short  floortype;     ///< see floortype_t

  int    blockbox[4];   ///< mapblock bounding box for height changes TODO obsolete?

  int     validcount;   ///< if == global validcount, already checked
  Actor  *thinglist;    ///< list of Actors in sector

  //SoM: 3/6/2000: Start boom extra stuff
  // Thinker for reversable actions
  class Thinker *floordata; ///< floor effect
  Thinker *ceilingdata;     ///< ceiling effect
  Thinker *lightingdata;    ///< lighting effect

  // lockout machinery for stairbuilding
  int stairlock;   ///< -2 on first locked -1 after thinker done 0 normally
  int prevsec;     ///< -1 or number of sector for previous step
  int nextsec;     ///< -1 or number of next step sector

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
  int   damage; // TEST given according to damage bits in 'special'
  float gravity;  // TEST
  float friction, movefactor;  ///< sector floor friction properties

  class fadetable_t *bottommap, *midmap, *topmap; ///< dynamic colormaps

  /// list of Actors that are at least partially in the sector (superset of thinglist)
  struct msecnode_t *touching_thinglist;

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
/// \ingroup \g_mapgeometry
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



/// \brief Move clipping aid for LineDefs.
/// \ingroup \g_mapgeometry
enum slopetype_t
{
  ST_HORIZONTAL,
  ST_VERTICAL,
  ST_POSITIVE,
  ST_NEGATIVE
};


/// \brief LineDef flags
/// \ingroup \g_mapgeometry
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
/// \ingroup \g_mapgeometry
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



/// \brief SubSector, BSP leaf
/// \ingroup \g_mapgeometry
///
/// Each sector is divided into one or more convex subsectors during BSP nodebuilding.
/// Basically, this is a list of LineSegs, indicating the visible walls that define
/// (all or some) sides of a convex BSP leaf.
struct subsector_t
{
  Uint32 num_segs;  ///< number of linesegs surrounding this subsector
  Uint32 first_seg; ///< index into the segs array
  sector_t *sector; ///< to which sector does this subsector belong?

  //int       validcount;  // Hurdler: added for optimized mlook in hw mode

  struct floorsplat_t *splats; ///< floorsplat list
  struct polyobj_t    *poly;
};


/// \brief Sector list node, showing all sectors an Actor appears in.
/// \ingroup \g_mapgeometry
///
/// There are two threads that flow through these nodes. The first thread
/// starts at touching_thinglist in a sector_t and flows through the m_snext
/// links to find all mobjs that are entirely or partially in the sector.
/// The second thread starts at touching_sectorlist in an Actor and flows
/// through the m_tnext links to find all sectors a thing touches. This is
/// useful when applying friction or push effects to sectors. These effects
/// can be done as thinkers that act upon all objects touching their sectors.
/// As an Actor moves through the world, these nodes are created and
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

private:
  /// Freelist for unused nodes
  static msecnode_t *headsecnode;

  /// Allocate a new node (or get one from the freelist)
  static msecnode_t *GetNode();
  /// Return a node to freelist
  void Free();
  /// Unlink from both threads and Free the node
  msecnode_t *Delete();

public:
  static void InitSecnodes();

  /// Adds a sector/Actor node to a sector list
  static msecnode_t *AddToSectorlist(sector_t *s, Actor *thing, msecnode_t *seclist);

  /// Delete an entire sector list
  static void DeleteSectorlist(msecnode_t *seclist)
  {
    while (seclist)
      seclist = seclist->Delete();
  }

  /// Removes unused nodes from a sector list
  static msecnode_t *CleanSectorlist(msecnode_t *seclist);
};



/// \brief LineSeg
/// \ingroup \g_mapgeometry
struct seg_t
{
  vertex_t *v1, *v2;     ///< start and end vertices
  line_t   *linedef;     ///< corresponding linedef
  short     side;        ///< 0 means right side, 1 means left side.
  seg_t    *partner_seg; ///< the other side

  side_t   *sidedef;
  fixed_t   offset;
  angle_t   angle;
  float     length; ///< seg length

  /// Sector references. Backsector is NULL for one sided lines
  // Could be retrieved from linedef, too.
  sector_t *frontsector;
  sector_t *backsector;

  // SoM: Why slow things down by calculating lightlists for every thick side.
  int         numlights;
  struct r_lightlist_t *rlights;
};



/// \brief BSP node
/// \ingroup \g_mapgeometry
struct node_t
{
  /// Partition line.
  fixed_t  x, y;
  fixed_t  dx, dy;

  /// Bounding box for each child.
  bbox_t bbox[2];

  Uint32 children[2]; ///< indices of children nodes
#define NF_SUBSECTOR (1 << 31) ///< Indicates a BSP leaf => child == subsector.
};




/// \brief Runtime mapthing
/// \ingroup \g_mapgeometry
struct mapthing_t
{
  short tid;      ///< Thing ID (from Hexen)
  short x, y, z;  ///< coordinates
  short angle;    ///< orientation
  short type;     ///< DoomEd number
  short flags;    ///< see mapthing_flags_e
  byte special;   ///< thing action
  byte args[5];   ///< arguments for the thing action

  class Actor *mobj;
};


/// \brief mapthing_t flags
/// \ingroup \g_mapgeometry
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




// GL node definitions. Currently we use some v5 GL nodes directly.
// This is not very optimal, but it will do for the moment.
// glvertexes and vertexes are both the same.


/// \brief Data needed to render level geometry with GL renderer.
/// \ingroup \g_mapgeometry
///
/// These are all unpacked, endianness-swapped and converted to v5 GL
/// nodes.
struct gllevel_t
{
  vertex_t *vertexes;
  int      numvertexes;

  line_t *lines;
  int    numlines;

  side_t *sides;
  int    numsides;

  sector_t *sectors;
  int      numsectors;

  vertex_t *glvertexes;
  int      numglvertexes;

  seg_t  *glsegs;
  int     numglsegs;

  subsector_t *glsubsectors;
  int          numglsubsectors;

  node_t  *glnodes;
  int      numglnodes;

  polyobj_t *polyobjs;
  int       numpolyobjs;

  byte *glvis;
};

#endif
