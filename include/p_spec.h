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
// Revision 1.24  2004/08/29 20:48:49  smite-meister
// bugfixes. wow.
//
// Revision 1.23  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.22  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.20  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.19  2003/12/23 18:06:06  smite-meister
// Hexen stairbuilders. Moving geometry done!
//
// Revision 1.18  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.17  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.16  2003/11/30 00:09:48  smite-meister
// bugfixes
//
// Revision 1.15  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.14  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.13  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.12  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.11  2003/05/30 13:34:49  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.10  2003/05/05 00:24:50  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.9  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.8  2003/04/14 08:58:30  smite-meister
// Hexen maps load.
//
// Revision 1.7  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.6  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Dynamic geometry elements and related utilities defined.
///
/// Texture animation, height or lighting changes
/// according to adjacent sectors, respective
/// utility functions, etc.


#ifndef p_spec_h
#define p_spec_h 1

#include <list>

#include "r_defs.h"
#include "g_think.h" // Thinker class

using namespace std;


extern int boomsupport;


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


/// sector special effects
enum sectorspecial_t
{
  SS_none = 0,

  // light effects (1-31) (bits 0-4)
  SS_LIGHTMASK    = 0x001F, // 0-31

  SS_light_flicker   = 1,
  SS_light_blinkfast = 2,
  SS_light_blinkslow = 3,
  SS_light_glow      = 8,
  SS_light_syncfast  = 12,
  SS_light_syncslow  = 13,
  SS_light_fireflicker = 17,

  // damage frequency (bits 5-6)
  SS_DAMAGEMASK   = 0x0060, // 32+64

  SS_damage_16 = 0x0020, // every 16 tics
  SS_damage_32 = 0x0040,
  SS_damage_XX = 0x0060,

  // other Boom stuff
  SS_secret   = 0x0080,
  SS_friction = 0x0100,
  SS_wind     = 0x0200,

  // bits 10-15 are free

  // Hexen: TODO a real mess
  SS_Light_Phased = 1,
  SS_LightSequence_Start = 2,
  SS_LightSequence_1 = 3,
  SS_LightSequence_2 = 4,
  SS_Stairs_Special1 = 26,
  SS_Stairs_Special2 = 27,
  SS_light_IndoorLightning1 = 198,
  SS_light_IndoorLightning2 = 199,
  SS_Sky2 = 200
};



// the result of a plane movement
enum planeresult_e
{
  res_ok = 0,
  res_crushed,
  res_pastdest
};

enum special_e
{
  floor_special,
  ceiling_special,
  lighting_special,
};

bool P_SectorActive(special_e t, sector_t *s);

enum change_e
{
  trigChangeOnly,
  numChangeOnly,
};


//======================================
///  Timed buttons
//======================================

class button_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(button_t)
public:
  enum button_e
  {
    none = 0,
    top,
    middle,
    bottom
  };

private:
  line_t *line;
  mappoint_t *soundorg;
  int     texture;
  int     timer;
  byte where; // button_e

public:
  button_t(line_t *l, button_e w, int tex, int time);
  
  virtual void Think();
};

void P_InitSwitchList();

// 1 second, in ticks.
#define BUTTONTIME      35


//======================================
//   Polyobjects
//======================================

/// Polyobject Rotator
class polyobject_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(polyobject_t)
protected:  
  int polyobj;
  int speed;
  int dist;

public:
  polyobject_t(int num);
  polyobject_t(int num, byte *args, int dir);
  
  virtual void Think();
  virtual int  PushForce();
};

/// Polyobject Mover
class polymove_t : public polyobject_t
{
  friend class Map;
  DECLARE_CLASS(polymove_t)
protected:
  int angle;
  fixed_t xs, ys;

public:
  polymove_t(int num, byte *args, bool timesEight, bool mirror);
  
  virtual void Think();
  virtual int  PushForce();
};


/// Polyobject Door
class polydoor_t : public polyobject_t
{
  friend class Map;
  DECLARE_CLASS(polydoor_t)
public:
  enum podoor_e
  {
    pd_none,
    pd_slide,
    pd_swing,
  }; 

protected:
  int totalDist;
  int direction;
  fixed_t xs, ys;
  int tics;
  int waitTics;
  byte type; // podoor_e
  bool close;

public:
  polydoor_t(int num, int type, byte *args, bool mirror);
  
  virtual void Think();
  virtual int  PushForce();
};

enum
{
  // linedef specials
  PO_LINE_START = 1,
  PO_LINE_EXPLICIT = 5,
  // mapthing specials
  PO_ANCHOR_TYPE = 3000,
  PO_SPAWN_TYPE,
  PO_SPAWNCRUSH_TYPE
};

//========================================================
//  Sector effects
//========================================================

/// Base class for most moving geometry
class sectoreffect_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(sectoreffect_t)

protected:
  sector_t *sector;
  sectoreffect_t(Map *m, sector_t *s);
};

//======================================
///   Sector light effects
//======================================

class lightfx_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(lightfx_t)

protected:
  short     type;
  short     count;
  short     maxlight, minlight;
  union
  {
    short maxtime;
    short rate; // 10.6 fixed point for smoother light changes
  };
  union
  {
    short mintime;
    short currentlight; // 10.6 fixed point for smoother light changes
  };

public:
  enum lightfx_e
  {
    AbsChange,   // one-time absolute change
    RelChange,   // one-time relative change
    Fade,        // linear ramp to maxlight, rate is speed
    Glow,        // sawtooth wave between min and max, rate is speed
    Strobe,      // square wave, constant min and max times
    Flicker,     // square wave, random min and max times
    FireFlicker  // square wave with more randomness
  };

  lightfx_t(Map *m, sector_t *sec, lightfx_e type, short maxlight, short minlight = 0, short maxtime = 0, short mintime = 0);
  
  virtual void Think();
};

// strobe light timings
#define STROBEBRIGHT  5
#define FASTDARK     15
#define SLOWDARK     35


/// Sequential Hexen light effect
class phasedlight_t: public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(phasedlight_t)

protected:
  short base;
  short index;

public:
  phasedlight_t(Map *m, sector_t *sec, int base, int index);
  
  virtual void Think();
};


//======================================
///  Platforms/Lifts (complex moving floors)
//======================================

class plat_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(plat_t)
public:
  enum plat_e
  {
    // targets
    RelHeight = 0,
    AbsHeight,
    LnF,
    NLnF,
    NHnF,
    LnC,
    LHF,
    CeilingToggle,
    TMASK = 0xF,
    // flags
    Returning = 0x10, // coming back, will stop
    Perpetual = 0x20, // will not stop until stopped
    SetTexture = 0x40,
  };

  enum status_e
  {
    up,
    down,
    waiting,
    in_stasis
  };

private:
  int       type;
  fixed_t   speed;
  fixed_t   low, high;
  int       wait, count;
  byte      status, oldstatus; // status_e
  int       tag;

  list<plat_t *>::iterator li;

public:
  plat_t(Map *m, int type, sector_t *sec, int tag, fixed_t speed, int wait, fixed_t height);
  
  virtual void Think();
};


#define PLATWAIT                3
#define PLATSPEED               (FRACUNIT/NEWTICRATERATIO)


//======================================
///  Doors (complex moving ceilings)
//======================================

class vdoor_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(vdoor_t)
public:
  enum vdoor_e
  {
    // 2 bits, type of door
    OwC,
    Open,
    CwO,
    Close,
    TMASK   = 0x3,
    Blazing = 0x4,
    Delayed = 0x8 // initial delay (topcount)
  };

private:
  byte      type;
  char      direction; // 1 = up, 0 = waiting at top, -1 = down, 2 = initial delay
  fixed_t   topheight;
  fixed_t   speed;
  int       topwait;   // tics to wait at the top
  int       topcount;   // when it reaches 0, start going down

  short     boomlighttag; // for Boom push-door light effect

public:
  vdoor_t(Map *m, byte type, sector_t *sec, fixed_t speed, int delay);
  
  virtual void Think();
  void MakeSound(bool open) const;
};


#define VDOORSPEED    (FRACUNIT*2/NEWTICRATERATIO)
#define VDOORWAIT     150


// Sliding doors removed


//======================================
///  Moving ceilings (also crushers)
//======================================

class ceiling_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(ceiling_t)
public:

  enum ceiling_e
  {
    // target
    RelHeight = 0,
    AbsHeight,
    Floor,
    HnC,
    //NnC,
    LnC,
    HnF,
    CrushOnce,
    Crusher,
    TMASK  = 0xF,
    // flags
    Silent = 0x10,
    SetTexture = 0x20,
    SetSpecial = 0x40,
    SetTxTy    = 0x20 + 0x40
  };

private:
  int       type;
  int       crush;

  // ceiling changers
  int       newspecial;
  short     texture;

  fixed_t   upspeed, downspeed;
  fixed_t   oldspeed;

  int       tag;
  char      olddirection;
  list<ceiling_t *>::iterator li;

public:
  char      direction;   // 1 = up, 0 = waiting, -1 = down
  fixed_t   bottomheight, topheight;

  ceiling_t(Map *m, int ty, sector_t *sec, fixed_t usp, fixed_t dsp, int cru, fixed_t height);
  
  virtual void Think();
};


#define CEILSPEED               (FRACUNIT/NEWTICRATERATIO)
#define CEILWAIT                150


//======================================
///  Moving floors (also stairs, donuts)
//======================================

class floor_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(floor_t)
public:
  enum floor_e
  {
    // target
    RelHeight = 0,
    AbsHeight,
    Ceiling,
    LnF,
    UpNnF,
    DownNnF,
    HnF,
    LnC,
    SLT,
    TMASK = 0xF,
    // flags
    SetTexture = 0x10,
    SetSpecial = 0x20,
    SetTxTy    = 0x10 + 0x20,
    NumericModel = 0x40 // otherwise assume "trigger model"
  };

private:
  int       type;
  int       crush;

  // floor changers
  int       newspecial;
  short     texture;

  fixed_t   speed; // sign denotes direction

public:
  fixed_t   destheight;

public:
  floor_t(Map *m, int type, sector_t *sec, fixed_t speed, int crush, fixed_t height);
  
  virtual void Think();
};

enum stair_e
{
  STAIRS_NORMAL,
  STAIRS_SYNC,
  STAIRS_PHASED
};


//======================================
///  Stairbuilders
//======================================

class stair_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(stair_t)
public:
  enum stair_e
  {
    // types
    Normal = 0,
    Sync,
    // internal states
    Moving = 0,
    Waiting,
    Done
  };

private:
  char state;
  int resetcount; // ticks until reset, if 0, no reset
  int wait, stepdelay;

  fixed_t speed;
  fixed_t destheight, originalheight;
  fixed_t delayheight, stepdelta;

public:
  stair_t(Map *m, int ty, sector_t *sec, fixed_t h, fixed_t sp, int rcount, int sdelay);
  
  virtual void Think();
};


//==================================================
///  Moving floor and ceiling (elevators, pillars)
//==================================================

class elevator_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(elevator_t)
public:
  enum elevator_e
  {
    Up = 0,
    Down,
    Current,
    RelHeight,
    OpenPillar,
    ClosePillar
  };

private:
  int     type;
  int     crush;
  fixed_t floordest, ceilingdest;
  fixed_t floorspeed, ceilingspeed;

public:
  elevator_t(Map *m, int type, sector_t *s, fixed_t speed, fixed_t height_f, fixed_t height_c, int crush);
  
  virtual void Think();
};


#define ELEVATORSPEED (FRACUNIT*4/NEWTICRATERATIO) //SoM: 3/6/2000
#define FLOORSPEED    (FRACUNIT/NEWTICRATERATIO)

//======================================
///  Sine wave floorshake (Hexen)
//======================================

class floorwaggle_t : public sectoreffect_t
{
  friend class Map;
  DECLARE_CLASS(floorwaggle_t)
public:
  enum floorwaggle_e
  {
    Expand = 0,
    Stable,
    Reduce
  };

private:
  char    state;
  angle_t phase, freq;
  fixed_t baseheight, amp, maxamp, ampdelta;
  int     wait;

public:
  floorwaggle_t(Map *m, sector_t *sec, fixed_t a, angle_t f, angle_t ph, int w);
  
  virtual void Think();
};


//======================================
// BOOM generalized linedefs
//======================================

/* SoM: 3/4/2000: This is a large section of copied code. Sorry if this offends people, but
   I really don't want to read, understand and rewrite all the changes to the source and entire
   team made! Anyway, this is for the generalized linedef types. */

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



//============================================
/// Generalized scroller

class scroll_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(scroll_t)
public:
  enum scroll_e
  {
    // what is being scrolled?
    sc_floor         = 0x01,
    sc_ceiling       = 0x02,
    sc_carry_floor   = 0x04,
    sc_carry_ceiling = 0x08, ///< not implemented

    sc_side          = 0x10, ///< not stackable with the other types

    sc_wind = 0x20,  ///< only affects things with MF2_WINDTHRUST flag

    // how is the scrolling controlled?
    sc_constant     = 0x0,
    sc_displacement = 0x1,
    sc_accelerative = 0x2,
    sc_offsets      = 0x4,
  };

private:
  short     type;
  fixed_t   vx, vy;      ///< scroll speed
  int       affectee;    ///< Number of affected sidedef, sector, tag, or whatever
  sector_t *control;     ///< Control sector (NULL if none) used to control scrolling
  fixed_t   last_height; ///< Last known height of control sector
  bool      accel;       ///< Whether it's accelerative
  fixed_t   vdx, vdy;    ///< Accumulated velocity if accelerative

public:

  scroll_t(short type, fixed_t dx, fixed_t dy, sector_t *csec, int aff, bool acc);
  
  virtual void Think();
};


//============================================
/// Boom push/pull effect

class pusher_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(pusher_t)
public:
  enum pusher_e
  {
    p_wind        = 0,
    p_current     = 1,
    p_point       = 2,
    p_upcurrent,
    p_downcurrent
  };

private:
  byte type;
  class DActor *source;  ///< Point source if point pusher
  int x_mag, y_mag;   ///< X, Y Strength
  int magnitude;      ///< Vector strength for point pusher
  int radius;         ///< Effective radius for point pusher
  int x, y;           ///< X, Y of point source if point pusher
  int affectee;       ///< Number of affected sector

public:
  pusher_t(pusher_e t, int x_m, int y_m, DActor *src, int aff);
  
  virtual void Think();

  friend bool PIT_PushThing(class Actor *thing);
};


#endif
