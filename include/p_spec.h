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
// Revision 1.5  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.4  2003/03/08 16:07:15  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.3  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.2  2002/12/16 22:05:04  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:26  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.17  2002/09/20 22:41:34  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.15  2002/09/06 17:18:36  vberghol
// added most of the changes up to RC2
//
// Revision 1.14  2002/09/05 14:12:17  vberghol
// network code partly bypassed
//
// Revision 1.12  2002/08/08 18:36:26  vberghol
// p_spec.cpp fixed
//
// Revision 1.11  2002/08/08 12:01:32  vberghol
// pian engine on valmis!
//
// Revision 1.10  2002/08/06 13:14:29  vberghol
// ...
//
// Revision 1.9  2002/07/26 19:23:06  vberghol
// a little something
//
// Revision 1.8  2002/07/23 19:21:46  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.7  2002/07/18 19:16:41  vberghol
// renamed a few files
//
// Revision 1.6  2002/07/13 17:56:58  vberghol
// *** empty log message ***
//
// Revision 1.5  2002/07/12 19:21:40  vberghol
// hop
//
// Revision 1.4  2002/07/10 19:57:03  vberghol
// g_pawn.cpp tehty
//
// Revision 1.3  2002/07/01 21:00:53  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:27  vberghol
// Version 133 Experimental!
//
// Revision 1.12  2001/03/21 18:24:38  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.11  2001/02/24 13:35:20  bpereira
// no message
//
// Revision 1.10  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.9  2000/11/11 13:59:46  bpereira
// no message
//
// Revision 1.8  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.7  2000/10/21 08:43:30  bpereira
// no message
//
// Revision 1.6  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.5  2000/04/07 18:48:56  hurdler
// AnyKey doesn't seem to compile under Linux, now renamed to AnyKey_
//
// Revision 1.4  2000/04/06 20:54:28  hurdler
// Mostly remove warnings under windows
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
//      Implements special effects:
//      Texture animation, height or lighting changes
//       according to adjacent sectors, respective
//       utility functions, etc.
//
//-----------------------------------------------------------------------------


#ifndef p_spec_h
#define p_spec_h 1

#include <list>

#include "r_defs.h"
#include "g_think.h" // Thinker class

using namespace std;

class Actor;
class DActor;
class PlayerPawn;

extern int boomsupport;

//      Define values for map objects
#define MO_TELEPORTMAN          14

// at game start
void    P_InitPicAnims ();
// at map load (sectors)
void    P_SetupLevelFlatAnims ();


// geom. info, independent of Map
sector_t *getNextSector(line_t *line, sector_t *sec);

fixed_t P_FindLowestFloorSurrounding(sector_t *sec);
fixed_t P_FindHighestFloorSurrounding(sector_t *sec);

fixed_t P_FindNextLowestFloor(sector_t *sec, int currentheight);
fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight);

fixed_t P_FindLowestCeilingSurrounding(sector_t *sec);
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec);

fixed_t P_FindNextLowestCeiling(sector_t *sec, int currentheight);
fixed_t P_FindNextHighestCeiling(sector_t *sec, int currentheight);

int P_FindMinSurroundingLight(sector_t *sector, int     max);

int P_CheckTag(line_t *line);



//======================================
//   Polyobjects
//======================================


class polyevent_t : public Thinker
{
  friend class Map;
  
  int polyobj;
  int speed;
  unsigned int dist;
  int angle;
  fixed_t xs, ys;

public:
  polyevent_t();
  virtual void Think();
};


typedef enum
{
  PODOOR_NONE,
  PODOOR_SLIDE,
  PODOOR_SWING,
} podoortype_e;

class polydoor_t : public Thinker
{
  friend class Map;

  int polyobj;
  int speed;
  int dist;
  int totalDist;
  int direction;
  fixed_t xs, ys;
  int tics;
  int waitTics;
  podoortype_e type;
  bool close;

public:
  polydoor_t();
  virtual void Think();
};

enum
{
  PO_ANCHOR_TYPE = 3000,
  PO_SPAWN_TYPE,
  PO_SPAWNCRUSH_TYPE
};

#define PO_LINE_START 1 // polyobj line start special
#define PO_LINE_EXPLICIT 5


//
// P_LIGHTS
//

class fireflicker_t : public Thinker
{
  friend class Map;
  sector_t *sector;
  int       count;
  int       maxlight;
  int       minlight;
public:
  //  _t(plattype_e ty, sector_t *s, line_t *l);
  virtual void Think();
};


class lightflash_t : public Thinker
{
  friend class Map;
  sector_t *sector;
  int         count;
  int         maxlight;
  int         minlight;
  int         maxtime;
  int         mintime;
public:
  //  _t(plattype_e ty, sector_t *s, line_t *l);
  virtual void Think();
};


class strobe_t : public Thinker
{
  friend class Map;
  sector_t *sector;
  int         count;
  int         minlight;
  int         maxlight;
  int         darktime;
  int         brighttime;
public:
  //  _t(plattype_e ty, sector_t *s, line_t *l);
  virtual void Think();
};


class glow_t : public Thinker
{
  friend class Map;
  sector_t *sector;
  int         minlight;
  int         maxlight;
  int         direction;
public:
  //  _t(plattype_e ty, sector_t *s, line_t *l);
  virtual void Think();
};

//SoM: thinker struct for fading lights. ToDo: Add effects for light
// transition
class lightlevel_t : public Thinker
{
  friend class Map;
  sector_t *sector;
  int destlevel;
  int speed;
public:
  //  _t(plattype_e ty, sector_t *s, line_t *l);
  virtual void Think();
};

#define GLOWSPEED               8
#define STROBEBRIGHT            5
#define FASTDARK                15
#define SLOWDARK                35


//
// P_SWITCH
//
#pragma pack(1) //Hurdler: 04/04/2000: I think pragma is more portable
struct switchlist_t
{
  char        name1[9];
  char        name2[9];
  short       episode;
};
#pragma pack()

void P_InitSwitchList();

typedef enum
{
  button_none,
  button_top,
  button_middle,
  button_bottom

} button_e;


class button_t : public Thinker
{
  friend class Map;
  line_t *line;
  mappoint_t *soundorg;
  int     texture;
  int     timer;
  button_e where;

public:
  static int buttonsound;

  button_t(line_t *l, button_e w, int tex, int time);
  virtual void Think();
};

// 1 second, in ticks.
#define BUTTONTIME      35



// SoM: 3/4/2000: Misc Boom stuff for thinkers that can share sectors, and some other stuff

typedef enum
{
  floor_special,
  ceiling_special,
  lighting_special,
} special_e;


//SoM: 3/6/2000
int P_SectorActive(special_e t, sector_t *s);


typedef enum
{
  trigChangeOnly,
  numChangeOnly,
} change_e;


//
// P_PLATS
//
typedef enum
{
  up,
  down,
  waiting,
  in_stasis

} plat_e;


typedef enum
{
  perpetualRaise,
  downWaitUpStay,
  raiseAndChange,
  raiseToNearestAndChange,
  blazeDWUS,
  //SoM:3/4/2000: Added boom stuffs
  genLift,      //General stuff
  genPerpetual, 
  toggleUpDn,   //Instant toggle of stuff.

} plattype_e;


class plat_t : public Thinker
{
  friend class Map;
private:
  plattype_e  type;
  sector_t *sector;
  fixed_t     speed;
  fixed_t     low;
  fixed_t     high;
  int         wait;
  int         count;
  plat_e      status;
  plat_e      oldstatus;
  bool        crush;
  int         tag;

  list<plat_t *>::iterator li;

public:
  plat_t(plattype_e ty, sector_t *s, line_t *l);
  virtual void Think();
};


#define PLATWAIT                3
#define PLATSPEED               (FRACUNIT/NEWTICRATERATIO)


//
// P_DOORS
//
typedef enum
{
  normalDoor,
  close30ThenOpen,
  doorclose,
  dooropen,
  raiseIn5Mins,
  blazeRaise,
  blazeOpen,
  blazeClose,

  //SoM: 3/4/2000: General door types...
  genRaise,
  genBlazeRaise,
  genOpen,
  genBlazeOpen,
  genClose,
  genBlazeClose,
  genCdO,
  genBlazeCdO,
} vldoor_e;



class vldoor_t : public Thinker
{
  friend class Map;
private:
  vldoor_e    type;
  sector_t *sector;
  fixed_t     topheight;
  fixed_t     speed;
  int   direction; // 1 = up, 0 = waiting at top, -1 = down
  int   topwait;   // tics to wait at the top

  // (keep in case a door going down is reset)
  // when it reaches 0, start going down
  int   topcountdown;

  line_t *line;   // the line that triggered the door.

public:
  static int doorclosesound;

  vldoor_t(vldoor_e ty, sector_t *sec, fixed_t sp, int delay, line_t *li);
  virtual void Think();
};


#define VDOORSPEED    (FRACUNIT*2/NEWTICRATERATIO)
#define VDOORWAIT     150


#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum
{
  sd_opening,
  sd_waiting,
  sd_closing

} sd_e;



typedef enum
{
  sdt_openOnly,
  sdt_closeOnly,
  sdt_openAndClose

} sdt_e;


class slidedoor_t : public Thinker
{
  friend class Map;
private:
  sdt_e       type;
  line_t *line;
  int         frame;
  int         whichDoorIndex;
  int         timer;
  sector_t *frontsector;
  sector_t *backsector;
  sd_e         status;
public:
  slidedoor_t();
  virtual void Think();
};


typedef struct
{
  char        frontFrame1[9];
  char        frontFrame2[9];
  char        frontFrame3[9];
  char        frontFrame4[9];
  char        backFrame1[9];
  char        backFrame2[9];
  char        backFrame3[9];
  char        backFrame4[9];

} slidename_t;



typedef struct
{
  int             frontFrames[4];
  int             backFrames[4];

} slideframe_t;



// how many frames of animation
#define SNUMFRAMES              4

#define SDOORWAIT               (35*3)
#define SWAITTICS               4

// how many diff. types of anims
#define MAXSLIDEDOORS   5

void P_InitSlidingDoorFrames();
void EV_SlidingDoor(line_t *line, Actor *thing);
#endif



//
// P_CEILNG
//
typedef enum
{
  lowerToFloor,
  raiseToHighest,
  //SoM:3/4/2000: Extra boom stuffs that tricked me...
  lowerToLowest,
  lowerToMaxFloor,

  lowerAndCrush,
  crushAndRaise,
  fastCrushAndRaise,
  silentCrushAndRaise,
  instantRaise,

  //SoM:3/4/2000
  //jff 02/04/98 add types for generalized ceiling mover
  genCeiling,
  genCeilingChg,
  genCeilingChg0,
  genCeilingChgT,

  //jff 02/05/98 add types for generalized ceiling mover
  genCrusher,
  genSilentCrusher,

} ceiling_e;


class ceiling_t : public Thinker
{
  friend class Map;
private:
  ceiling_e  type;
  sector_t  *sector;
  fixed_t    oldspeed; //SoM: 3/6/2000
  bool       crush;

  //SoM: 3/6/2000: Support ceiling changers
  int newspecial;
  int oldspecial;
  short texture;

  int         tag;   // ID
  int         olddirection;

  list<ceiling_t *>::iterator li;

public:
  int        direction;   // 1 = up, 0 = waiting, -1 = down
  fixed_t    speed;
  fixed_t    bottomheight;
  fixed_t    topheight;

public:
  static int ceilmovesound;

  ceiling_t(ceiling_e ty, sector_t *s, int t);
  virtual void Think();
};


#define CEILSPEED               (FRACUNIT/NEWTICRATERATIO)
#define CEILWAIT                150


//
// P_FLOOR
//
typedef enum
{
  // lower floor to highest surrounding floor
  lowerFloor,

    // lower floor to lowest surrounding floor
  lowerFloorToLowest,

  // lower floor to highest surrounding floor VERY FAST
  turboLower,

  // raise floor to lowest surrounding CEILING
  raiseFloor,

  // raise floor to next highest surrounding floor
  raiseFloorToNearest,

  // lower floor to lowest surrounding floor
  lowerFloorToNearest,

  // lower floor 24
  lowerFloor24,

  // lower floor 32
  lowerFloor32Turbo,

  // raise floor to shortest height texture around it
  raiseToTexture,

  // lower floor to lowest surrounding floor
  //  and change floorpic
  lowerAndChange,

  raiseFloor24,

  //raise floor 32
  raiseFloor32Turbo,

  raiseFloor24AndChange,
  raiseFloorCrush,

  // raise to next highest floor, turbo-speed
  raiseFloorTurbo,
  donutRaise,
  raiseFloor512,
  instantLower,

  //SoM: 3/4/2000 Boom copy YEAH YEAH
  genFloor,
  genFloorChg,
  genFloorChg0,
  genFloorChgT,

    //new types for stair builders
  buildStair,
  genBuildStair,

} floor_e;

//SoM:3/4/2000: Anothe boom code copy.
typedef enum
{
  elevateUp,
  elevateDown,
  elevateCurrent,
} elevator_e;


typedef enum
{
  build8,     // slowly build by 8
  turbo16     // quickly build by 16
} stair_e;


class floormove_t : public Thinker
{
  friend class Map;
private:
  floor_e     type;
  bool     crush;
  sector_t *sector;
  int         newspecial;
  int         oldspecial; //SoM: 3/6/2000
  short       texture;
public:
  int         direction;
  fixed_t     floordestheight;
  fixed_t     speed;

public:
  floormove_t(floor_e ty, sector_t *s, line_t *line, int secnum);
  virtual void Think();
};


class elevator_t : public Thinker
{
  friend class Map;
private:
  elevator_e type;
  sector_t *sector;
  int direction;
  fixed_t floordestheight;
  fixed_t ceilingdestheight;
  fixed_t speed;

public:
  elevator_t(elevator_e ty, sector_t *s, line_t *l);
  virtual void Think();
};


#define ELEVATORSPEED (FRACUNIT*4/NEWTICRATERATIO) //SoM: 3/6/2000
#define FLOORSPEED    (FRACUNIT/NEWTICRATERATIO)

typedef enum
{
  ok,
  crushed,
  pastdest
} result_e;


//
// BOOM generalized linedefs
// 

/* SoM: 3/4/2000: This is a large section of copied code. Sorry if this offends people, but
   I really don't want to read, understand and rewrite all the changes to the source and entire
   team made! Anyway, this is for the generalized linedef types. */

//jff 3/14/98 add bits and shifts for generalized sector types

#define DAMAGE_MASK     0x60
#define DAMAGE_SHIFT    5
#define SECRET_MASK     0x80
#define SECRET_SHIFT    7
#define FRICTION_MASK   0x100
#define FRICTION_SHIFT  8
#define PUSH_MASK       0x200
#define PUSH_SHIFT      9

//jff 02/04/98 Define masks, shifts, for fields in 
// generalized linedef types

#define GenFloorBase          0x6000
#define GenCeilingBase        0x4000
#define GenDoorBase           0x3c00
#define GenLockedBase         0x3800
#define GenLiftBase           0x3400
#define GenStairsBase         0x3000
#define GenCrusherBase        0x2F80

#define TriggerType           0x0007
#define TriggerTypeShift      0

// define masks and shifts for the floor type fields

#define FloorCrush            0x1000
#define FloorChange           0x0c00
#define FloorTarget           0x0380
#define FloorDirection        0x0040
#define FloorModel            0x0020
#define FloorSpeed            0x0018

#define FloorCrushShift           12
#define FloorChangeShift          10
#define FloorTargetShift           7
#define FloorDirectionShift        6
#define FloorModelShift            5
#define FloorSpeedShift            3
                               
// define masks and shifts for the ceiling type fields

#define CeilingCrush          0x1000
#define CeilingChange         0x0c00
#define CeilingTarget         0x0380
#define CeilingDirection      0x0040
#define CeilingModel          0x0020
#define CeilingSpeed          0x0018

#define CeilingCrushShift         12
#define CeilingChangeShift        10
#define CeilingTargetShift         7
#define CeilingDirectionShift      6
#define CeilingModelShift          5
#define CeilingSpeedShift          3

// define masks and shifts for the lift type fields

#define LiftTarget            0x0300
#define LiftDelay             0x00c0
#define LiftMonster           0x0020
#define LiftSpeed             0x0018

#define LiftTargetShift            8
#define LiftDelayShift             6
#define LiftMonsterShift           5
#define LiftSpeedShift             3

// define masks and shifts for the stairs type fields

#define StairIgnore           0x0200
#define StairDirection        0x0100
#define StairStep             0x00c0
#define StairMonster          0x0020
#define StairSpeed            0x0018

#define StairIgnoreShift           9
#define StairDirectionShift        8
#define StairStepShift             6
#define StairMonsterShift          5
#define StairSpeedShift            3

// define masks and shifts for the crusher type fields

#define CrusherSilent         0x0040
#define CrusherMonster        0x0020
#define CrusherSpeed          0x0018

#define CrusherSilentShift         6
#define CrusherMonsterShift        5
#define CrusherSpeedShift          3

// define masks and shifts for the door type fields

#define DoorDelay             0x0300
#define DoorMonster           0x0080
#define DoorKind              0x0060
#define DoorSpeed             0x0018

#define DoorDelayShift             8
#define DoorMonsterShift           7
#define DoorKindShift              5
#define DoorSpeedShift             3

// define masks and shifts for the locked door type fields

#define LockedNKeys           0x0200
#define LockedKey             0x01c0
#define LockedKind            0x0020
#define LockedSpeed           0x0018

#define LockedNKeysShift           9
#define LockedKeyShift             6
#define LockedKindShift            5
#define LockedSpeedShift           3


// define names for the TriggerType field of the general linedefs

typedef enum
{
  WalkOnce,
  WalkMany,
  SwitchOnce,
  SwitchMany,
  GunOnce,
  GunMany,
  PushOnce,
  PushMany,
} triggertype_e;

// define names for the Speed field of the general linedefs

typedef enum
{
  SpeedSlow,
  SpeedNormal,
  SpeedFast,
  SpeedTurbo,
} motionspeed_e;

// define names for the Target field of the general floor

typedef enum
{
  FtoHnF,
  FtoLnF,
  FtoNnF,
  FtoLnC,
  FtoC,
  FbyST,
  Fby24,
  Fby32,
} floortarget_e;

// define names for the Changer Type field of the general floor

typedef enum
{
  FNoChg,
  FChgZero,
  FChgTxt,
  FChgTyp,
} floorchange_e;

// define names for the Change Model field of the general floor

typedef enum
{
  FTriggerModel,
  FNumericModel,
} floormodel_t;

// define names for the Target field of the general ceiling

typedef enum
{
  CtoHnC,
  CtoLnC,
  CtoNnC,
  CtoHnF,
  CtoF,
  CbyST,
  Cby24,
  Cby32,
} ceilingtarget_e;

// define names for the Changer Type field of the general ceiling

typedef enum
{
  CNoChg,
  CChgZero,
  CChgTxt,
  CChgTyp,
} ceilingchange_e;

// define names for the Change Model field of the general ceiling

typedef enum
{
  CTriggerModel,
  CNumericModel,
} ceilingmodel_t;

// define names for the Target field of the general lift

typedef enum
{
  F2LnF,
  F2NnF,
  F2LnC,
  LnF2HnF,
} lifttarget_e;

// define names for the door Kind field of the general ceiling

typedef enum
{
  OdCDoor,
  ODoor,
  CdODoor,
  CDoor,
} doorkind_e;

// define names for the locked door Kind field of the general ceiling

typedef enum
{
  AnyKey_,
  RCard,
  BCard,
  YCard,
  RSkull,
  BSkull,
  YSkull,
  AllKeys,
} keykind_e;

/* SoM: End generalized linedef code */

typedef enum {
  sc_side,
  sc_floor,
  sc_ceiling,
  sc_carry,
  sc_carry_ceiling,
} scroll_e;

// generalized scroller code
class scroll_t : public Thinker
{
  friend class Map;
private:
  scroll_e type;
  fixed_t dx, dy;      // (dx,dy) scroll speeds
  int affectee;        // Number of affected sidedef, sector, tag, or whatever
  int control;         // Control sector (-1 if none) used to control scrolling
  fixed_t last_height; // Last known height of control sector
  fixed_t vdx, vdy;    // Accumulated velocity if accelerative
  int accel;           // Whether it's accelerative
public:
  scroll_t(scroll_e t, fixed_t ndx, fixed_t ndy,
	   int ctrl, int aff, int acc, const sector_t *csec);
  virtual void Think();
};



//SoM: 3/8/2000: added new model of friction for ice/sludge effects

class friction_t : public Thinker
{
  friend class Map;
private:
  float friction;        // friction value (E800 = normal)
  float movefactor;      // inertia factor when adding to momentum
  int   affectee;        // Number of affected sector
public:
  friction_t(float fri, float mf, int aff);
  virtual void Think();
};

extern const float normal_friction;

//SoM: 3/8/2000: Model for Pushers for push/pull effects

typedef enum {
  p_push,
  p_pull,
  p_wind,
  p_current,
  p_upcurrent,
  p_downcurrent
} pusher_e;

class pusher_t : public Thinker
{
  friend class Map;
private:
  pusher_e type;
  DActor *source;     // Point source if point pusher
  int x_mag, y_mag;   // X, Y Strength
  int magnitude;      // Vector strength for point pusher
  int radius;         // Effective radius for point pusher
  int x, y;           // X, Y of point source if point pusher
  int affectee;       // Number of affected sector

public:
  pusher_t(pusher_e t, int x_m, int y_m, DActor *src, int aff);
  virtual void Think();

  friend bool PIT_PushThing(Actor *thing);
};

//SoM: 3/9/2000: Prototype functions for pushers
//bool  PIT_PushThing(Actor *thing);

// heretic stuff
void P_InitLava();
void P_AmbientSound();
void P_AddAmbientSfx(int sequence);
void P_InitAmbientSound();

#endif
