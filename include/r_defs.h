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
// Revision 1.32  2001/08/19 20:41:04  hurdler
// small changes
//
// Revision 1.31  2001/08/13 22:53:40  stroggonmeth
// Small commit
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
// Revision 1.22  2001/03/30 17:12:51  bpereira
// no message
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
// Revision 1.18  2001/02/28 17:50:55  bpereira
// no message
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
// Revision 1.12  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.11  2000/07/01 09:23:49  bpereira
// no message
//
// Revision 1.10  2000/04/18 17:39:39  stroggonmeth
// Bug fixes and performance tuning.
//
// Revision 1.9  2000/04/18 12:55:39  hurdler
// join with Boris' code
//
// Revision 1.7  2000/04/15 22:12:58  stroggonmeth
// Minor bug fixes
//
// Revision 1.6  2000/04/12 16:01:59  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.5  2000/04/11 19:07:25  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.4  2000/04/06 20:47:08  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
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
//      Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------


#ifndef r_defs_h
#define r_defs_h 1


// Some more or less basic data types
// we depend on.
#include "m_fixed.h"
#include "screen.h"     //added:26-01-98:MAXVIDWIDTH, MAXVIDHEIGHT

class Thinker;


// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
enum
{
  SIL_NONE   = 0,
  SIL_BOTTOM = 1,
  SIL_TOP    = 2,
  SIL_BOTH   = 3
};

//faB: was upped to 512, but still people come with levels that break the
//     limits, so had to do an ugly re-alloc to get rid of the overflow.
//#define MAXDRAWSEGS             256        // see r_segs.c for more

// SoM: Moved this here...
// This could be wider for >8 bit display.
// Indeed, true color support is posibble
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef byte    lighttable_t;


// SoM: ExtraColormap type. Use for extra_colormaps from now on.
struct extracolormap_t
{
  unsigned short  maskcolor;
  unsigned short  fadecolor;
  double          maskamt;
  unsigned short  fadestart, fadeend;
  int             fog;

  //Hurdler: rgba is used in hw mode for coloured sector lighting
  int             rgba; // similar to maskcolor in sw mode

  lighttable_t*   colormap;
};

//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//  like some DOOM-alikes ("wt", "WebView") did.
//
struct vertex_t
{
  fixed_t  x, y;
};


// Forward of LineDefs, for Sectors.
struct line_t;
struct sector_t;

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

//SoM: 3/23/2000: Store fake planes in a resizalbe array insted of just by
//heightsec. Allows for multiple fake planes.
typedef enum
{
  FF_EXISTS            = 0x1,    //MAKE SURE IT'S VALID
  FF_SOLID             = 0x2,    //Does it clip things?
  FF_RENDERSIDES       = 0x4,    //Render the sides?
  FF_RENDERPLANES      = 0x8,    //Render the floor/ceiling?
  FF_RENDERALL         = 0xC,    //Render everything?
  FF_SWIMMABLE         = 0x10,   //Can we swim?
  FF_NOSHADE           = 0x20,   //Does it mess with the lighting?
  FF_CUTSOLIDS         = 0x40,   //Does it cut out hidden solid pixles?
  FF_CUTEXTRA          = 0x80,   //Does it cut out hidden translucent pixles?
  FF_CUTLEVEL          = 0xC0,   //Does it cut out all hidden pixles?
  FF_CUTSPRITES        = 0x100,  //Final Step in 3D water
  FF_BOTHPLANES        = 0x200,  //Render both planes all the time?
  FF_EXTRA             = 0x400,  //Does it get cut by FF_CUTEXTRAS?
  FF_TRANSLUCENT       = 0x800,  //See through!
  FF_FOG               = 0x1000, //Fog "brush"?
  FF_INVERTPLANES      = 0x2000, //Reverse the plane visibility rules?
  FF_ALLSIDES          = 0x4000, //Render inside and outside sides?
  FF_INVERTSIDES       = 0x8000, //Only render inside sides?
  FF_DOUBLESHADOW      = 0x10000,//Make two lightlist entries to reset light?
} ffloortype_e;


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
  line_t*   master;

  sector_t* target;

  ffloor_t* next;
  ffloor_t* prev;

  int              lastlight;
  int              alpha;
};


// SoM: This struct holds information for shadows casted by 3D floors.
// This information is contained inside the sector_t and is used as the base
// information for casted shadows.
struct lightlist_t
{
  fixed_t                 height;
  short                   *lightlevel;
  extracolormap_t*        extra_colormap;
  ffloor_t*               caster;
};


// SoM: This struct is used for rendering walls with shadows casted on them...
struct r_lightlist_t
{
  fixed_t                 height;
  fixed_t                 heightstep;
  fixed_t                 botheight;
  fixed_t                 botheightstep;
  short                   lightlevel;
  extracolormap_t*        extra_colormap;
  lighttable_t*           rcolormap;
  int                     flags;
};


typedef enum {
  FLOOR_SOLID,
  FLOOR_ICE,
  FLOOR_LIQUID,
  FLOOR_WATER,
  FLOOR_LAVA,
  FLOOR_SLUDGE
} floortype_t;

// ----- for special tricks with HW renderer -----

//
// For creating a chain with the lines around a sector
//
struct  linechain_t
{
  line_t        *line;
  linechain_t   *next;
};
// ----- end special tricks -----


//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
class Actor;
struct msecnode_t;

struct sector_t
{
  fixed_t     floorheight;
  fixed_t     ceilingheight;
  short       floorpic;
  short       ceilingpic;
  short       lightlevel;
  short       special;
  short       oldspecial;      //SoM: 3/6/2000: Remember if a sector was secret (for automap)
  short       tag;
  int nexttag,firsttag;        //SoM: 3/6/2000: by killough: improves searches for tags.

  // 0 = untraversed, 1,2 = sndlines -1
  short       soundtraversed;
  short       floortype;  // see floortype_t beffor 

  // thing that made a sound (or null)
  Actor*     soundtarget;

  // mapblock bounding box for height changes
  int         blockbox[4];

  // origin for any sounds played by the sector
  mappoint_t soundorg;

  // if == validcount, already checked
  int         validcount;

  // list of mobjs in sector
  Actor*     thinglist;

  //SoM: 3/6/2000: Start boom extra stuff
  // thinker_t for reversable actions
  void *floordata;    // make thinkers on
  void *ceilingdata;  // floors, ceilings, lighting,
  void *lightingdata; // independent of one another
  
  // lockout machinery for stairbuilding
  int stairlock;   // -2 on first locked -1 after thinker done 0 normally
  int prevsec;     // -1 or number of sector for previous step
  int nextsec;     // -1 or number of next step sector
  
  // floor and ceiling texture offsets
  fixed_t   floor_xoffs,   floor_yoffs;
  fixed_t ceiling_xoffs, ceiling_yoffs;

  int heightsec;    // other sector, or -1 if no other sector
  int altheightsec; // Use old boom model? 1 for no 0 for yes.
  
  int floorlightsec, ceilinglightsec;
  int teamstartsec;

  // TEST
  short damage, damagetype; // given according to damage bits in 'special'

  float gravity;

  // friction belongs here, not in Actor
  float friction, movefactor;

  int bottommap, midmap, topmap; // dynamic colormaps
  
  // list of mobjs that are at least partially in the sector
  // thinglist is a subset of touching_thinglist
  msecnode_t *touching_thinglist;               // phares 3/14/98  
  //SoM: 3/6/2000: end stuff...

  int                 linecount;
  line_t**     lines;  // [linecount] size

  //SoM: 2/23/2000: Improved fake floor hack
  ffloor_t*                  ffloors;
  int                        *attached;
  int                        numattached;
  lightlist_t*               lightlist;
  int                        numlights;
  bool                    moved;

  int                        validsort; //if == validsort allready been sorted
  bool                    added;

  // SoM: 4/3/2000: per-sector colormaps!
  extracolormap_t*           extra_colormap;

  // ----- for special tricks with HW renderer -----
  bool                    pseudoSector;
  bool                    virtualFloor;
  fixed_t                    virtualFloorheight;
  bool                    virtualCeiling;
  fixed_t                    virtualCeilingheight;
  linechain_t               *sectorLines;
  sector_t           **stackList;
  double                     lineoutLength;
  // ----- end special tricks -----
};



//
// The SideDef.
//

struct side_t
{
  // add this to the calculated texture column
  fixed_t     textureoffset;

  // add this to the calculated texture top
  fixed_t     rowoffset;

  // Texture indices.
  // We do not maintain names here.
  short       toptexture;
  short       bottomtexture;
  short       midtexture;

  // Sector the SideDef is facing.
  sector_t*   sector;

  //SoM: 3/6/2000: This is the special of the linedef this side belongs to.
  int special;
};



//
// Move clipping aid for LineDefs.
//
typedef enum
{
  ST_HORIZONTAL,
  ST_VERTICAL,
  ST_POSITIVE,
  ST_NEGATIVE
} slopetype_t;



struct line_t
{
  // Vertices, from v1 to v2.
  vertex_t*   v1;
  vertex_t*   v2;

  // Precalculated v2 - v1 for side checking.
  fixed_t     dx;
  fixed_t     dy;

  short       flags;
  short       special;
  short       tag;
  // hexen args
  byte args[5];

  // Visual appearance: SideDefs.
  //  sidenum[1] will be -1 if one sided
  short       sidenum[2];

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
  fixed_t     bbox[4];

  // To aid move clipping.
  slopetype_t slopetype;

  // Front and back sector.
  // Note: redundant? Can be retrieved from SideDefs.
  sector_t*   frontsector;
  sector_t*   backsector;

  // if == validcount, already checked
  int         validcount;

  // Thinker for complex actions
  Thinker  *thinker;

  // wallsplat_t list
  void*       splats;
    
  //SoM: 3/6/2000
  int tranlump;          // translucency filter, -1 == none 
  // (Will have to fix to use with Legacy's Translucency?)
  int firsttag,nexttag;  // improves searches for tags.

  int ecolormap;         // SoM: Used for 282 linedefs
};




//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs,
//  indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
struct subsector_t
{
  sector_t*   sector;
  short       numlines;
  short       firstline;
  // floorsplat_t list
  void*       splats;
  //Hurdler: added for optimized mlook in hw mode
  int         validcount;

  struct polyobj_t *poly;
};


// SoM: 3/6/200
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an Actor and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

struct msecnode_t
{
  sector_t    *m_sector; // a sector containing this object
  Actor       *m_thing;  // this object
  msecnode_t  *m_tprev;  // prev msecnode_t for this thing
  msecnode_t  *m_tnext;  // next msecnode_t for this thing
  msecnode_t  *m_sprev;  // prev msecnode_t for this sector
  msecnode_t  *m_snext;  // next msecnode_t for this sector
  bool visited; // killough 4/4/98, 4/7/98: used in search algorithms
};


//Hurdler: 04/12/2000: for now, only used in hardware mode
//                     maybe later for software as well?
//                     that's why it's moved here
struct light_t
{
  USHORT  type;           // light,... (cfr #define in hwr_light.c)

  float   light_xoffset;
  float   light_yoffset;  // y offset to adjust corona's height

  ULONG   corona_color;   // color of the light for static lighting
  float   corona_radius;  // radius of the coronas

  ULONG   dynamic_color;  // color of the light for dynamic lighting
  float   dynamic_radius; // radius of the light ball
  float   dynamic_sqrradius; // radius^2 of the light ball
};

struct lightmap_t
{
  float       s[2], t[2];
  light_t    *light;
  lightmap_t *next;
};

//
// The LineSeg.
//
struct seg_t
{
  vertex_t*   v1;
  vertex_t*   v2;

  fixed_t     offset;

  angle_t     angle;

  side_t*     sidedef;
  line_t*     linedef;

  // Sector references.
  // Could be retrieved from linedef, too.
  // backsector is NULL for one sided lines
  sector_t*   frontsector;
  sector_t*   backsector;

  // lenght of the seg : used by the hardware renderer
  float       length;

  //Hurdler: 04/12/2000: added for static lightmap
  lightmap_t  *lightmaps;

  // SoM: Why slow things down by calculating lightlists for every
  // thick side.
  int               numlights;
  r_lightlist_t*    rlights;
};



//
// BSP node.
//
struct node_t
{
  // Partition line.
  fixed_t     x;
  fixed_t     y;
  fixed_t     dx;
  fixed_t     dy;

  // Bounding box for each child.
  fixed_t     bbox[2][4];

  // If NF_SUBSECTOR its a subsector.
  unsigned short children[2];
};




//
// OTHER TYPES
//




#ifndef MAXFFLOORS
#define MAXFFLOORS    40
#endif

//
// ?
//
struct visplane_t;

struct drawseg_t
{
  seg_t*              curline;
  int                 x1;
  int                 x2;

  fixed_t             scale1;
  fixed_t             scale2;
  fixed_t             scalestep;

  // 0=none, 1=bottom, 2=top, 3=both
  int                 silhouette;
  
  // do not clip sprites above this
  fixed_t             bsilheight;

  // do not clip sprites below this
  fixed_t             tsilheight;

  // Pointers to lists for sprite clipping,
  //  all three adjusted so [x1] is first value.
  short*              sprtopclip;
  short*              sprbottomclip;
  short*              maskedtexturecol;

  visplane_t*  ffloorplanes[MAXFFLOORS];
  int                 numffloorplanes;
  ffloor_t*    thicksides[MAXFFLOORS];
  short*              thicksidecol;
  int                 numthicksides;
  fixed_t             frontscale[MAXVIDWIDTH];
};


typedef enum
{
  SC_NONE = 0,
  SC_TOP = 1,
  SC_BOTTOM = 2
} spritecut_e;

// A vissprite_t is a thing
//  that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
struct vissprite_t
{
  // Doubly linked list.
  vissprite_t* prev;
  vissprite_t* next;

  int                 x1;
  int                 x2;

  // for line side calculation
  fixed_t             gx;
  fixed_t             gy;

  // global bottom / top for silhouette clipping
  fixed_t             gz;
  fixed_t             gzt;

  // Physical bottom / top for sorting with 3D floors.
  fixed_t				pz;
  fixed_t				pzt;

  // horizontal position of x1
  fixed_t             startfrac;

  fixed_t             scale;

  // negative if flipped
  fixed_t             xiscale;

  fixed_t             texturemid;
  int                 patch;

  // for color translation and shadow draw,
  //  maxbright frames as well
  lighttable_t*       colormap;

  //Fab:29-04-98: for MF_SHADOW sprites, which translucency table to use
  byte*               transmap;

  // SoM: 3/6/2000: height sector for underwater/fake ceiling support
  int                 heightsec;

  //SoM: 4/3/2000: Global colormaps!
  extracolormap_t*    extra_colormap;
  fixed_t             xscale;

  //SoM: Precalculated top and bottom screen coords for the sprite.
  fixed_t             thingheight; //The actual height of the thing (for 3D floors)
  sector_t*           sector; //The sector containing the thing.
  fixed_t             sz;
  fixed_t             szt;

  int                 cut;  //0 for none, bit 1 for top, bit 2 for bottom
};

//
// Frame flags:
// handles maximum brightness (torches, muzzle flare, light sources)
//

// faB: I noticed they didn't use the 32 bits of the frame field,
//      so now we use the upper 16 bits for new effects.

#define FF_FRAMEMASK    0x7fff  // only the frame number
#define FF_FULLBRIGHT   0x8000  // frame always appear full bright (fixedcolormap)

// faB:
//  MF_SHADOW is no more used to activate translucency (or the old fuzzy)
//  The frame field allows to set translucency per frame, instead of per sprite.
//  Now, (frame & FF_TRANSMASK) is the translucency table number, if 0
//  it is not translucent.

// Note:
//  MF_SHADOW still affects the targeting for monsters (they miss more)

#define FF_TRANSMASK   0x70000  // 0 = no trans(opaque), 1-7 = transl. table
#define FF_TRANSSHIFT       16

// faB: new 'alpha' shade effect, for smoke..

#define FF_SMOKESHADE  0x80000  // sprite is an alpha channel

// translucency tables

// TODO: add another asm routine which use the fg and bg indexes in the
//       inverse order so the 20-80 becomes 80-20 translucency, no need
//       for other tables (thus 1090,2080,5050,8020,9010, and fire special)

typedef enum
{
  tr_transmed=1,   //sprite 50 backg 50  most shots
  tr_transmor=2,   //       20       80  puffs
  tr_transhi =3,   //       10       90  blur effect
  tr_transfir=4,   // 50 50 but brighter for fireballs, shots..
  tr_transfx1=5    // 50 50 brighter some colors, else opaque for torches
} transnum_t;


#endif
