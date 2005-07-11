// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.22  2005/07/11 16:58:41  smite-meister
// msecnode_t bug fixed
//
// Revision 1.21  2004/09/13 20:43:31  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.20  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.19  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.18  2004/01/06 14:37:45  smite-meister
// six bugfixes, cleanup
//
// Revision 1.17  2004/01/02 14:25:02  smite-meister
// cleanup
//
// Revision 1.16  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.15  2003/12/03 10:49:50  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.14  2003/11/27 11:28:26  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.13  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
// Revision 1.12  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.10  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.9  2003/04/14 08:58:30  smite-meister
// Hexen maps load.
//
// Revision 1.8  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.7  2003/03/15 20:07:20  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.6  2003/03/08 16:07:14  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.5  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.2  2002/12/16 22:04:48  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------


#ifndef g_actor_h
#define g_actor_h 1

#include "m_fixed.h"  // Basics.
#include "g_think.h"  // We need the Thinker stuff.
#include "g_damage.h" // and damage types

// States are tied to finite states are
//  tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"

#define ONFLOORZ        MININT
#define ONCEILINGZ      MAXINT

#define TELEFOGHEIGHT  (32*FRACUNIT)
#define FOOTCLIPSIZE   (10*FRACUNIT)

/// \file
/// \brief Actor class definition
///
/// Actors are used to tell the refresh where to draw an image,
/// tell the world simulation when objects are contacted,
/// and tell the sound driver how to position a sound.
///
/// The refresh uses the next and prev links to follow
/// lists of things in sectors as they are being drawn.
/// The sprite, frame, and angle elements determine which patch_t
/// is used to draw the sprite if it is visible.
/// The sprite and frame values are allmost allways set
/// from state_t structures.
/// The statescr.exe utility generates the states.h and states.c
/// files that contain the sprite/frame numbers from the
/// statescr.txt source file.
/// The xyz origin point represents a point at the bottom middle
/// of the sprite (between the feet of a biped).
/// This is the default origin position for patch_ts grabbed
/// with lumpy.exe.
/// A walking creature will have its z equal to the floor
/// it is standing on.
///
/// The sound code uses the x,y, and subsector fields
/// to do stereo positioning of any sound effited by the Actor.
///
/// The play simulation uses the blocklinks, x,y,z, radius, height
/// to determine when Actors are touching each other,
/// touching lines in the map, or hit by trace lines (gunshots,
/// lines of sight, etc).
/// The Actor->flags element has various bit flags
/// used by the simulation.
///
/// Every Actor is linked into a single sector
/// based on its origin coordinates.
/// The subsector_t is found with R_PointInSubsector(x,y),
/// and the sector_t can be found with subsector->sector.
/// The sector links are only used by the rendering code,
/// the play simulation does not care about them at all.
///
/// Any Actor that needs to be acted upon by something else
/// in the play world (block movement, be shot, etc) will also
/// need to be linked into the blockmap.
/// If the thing has the MF_NOBLOCK flag set, it will not use
/// the block links. It can still interact with other things,
/// but only as the instigator (missiles will run into other
/// things, but nothing can run into a missile).
/// Each block in the grid is 128*128 units, and knows about
/// every line_t that it contains a piece of, and every
/// interactable Actor that has its origin contained.
///
/// A valid Actor is a Actor that has the proper subsector_t
/// filled in for its xy coordinates and is linked into the
/// sector from which the subsector was made, or has the
/// MF_NOSECTOR flag set (the subsector_t needs to be valid
/// even if MF_NOSECTOR is set), and is linked into a blockmap
/// block or has the MF_NOBLOCKMAP flag set.
/// Links should only be modified by the P_[Un]SetThingPosition()
/// functions.
/// Do not change the MF_NO? flags while a thing is valid.
///
/// Any questions?


/// Actor flags. More or less permanent attributes of the Actor.
enum mobjflag_t
{
  // physical properties
  MF_NOSECTOR         = 0x0001, ///< Don't link to sector (invisible but touchable)
  MF_NOBLOCKMAP       = 0x0002, ///< Don't link to blockmap (inert but visible)
  MF_SOLID            = 0x0004, ///< Blocks
  MF_SHOOTABLE        = 0x0008, ///< Can be hit
  MF_NOCLIPLINE       = 0x0010, ///< Does not clip against lines (walls)
  MF_NOCLIPTHING      = 0x0020, ///< Not blocked by other Actors. (chasecam, for example)
  MF_NOGRAVITY        = 0x0040, ///< Does not feel gravity
  // game mechanics
  MF_NOTRIGGER        = 0x0080, ///< Can not trigger linedefs (mainly missiles, chasecam)
  MF_NOTMONSTER       = 0x0100, ///< *Not affected by ML_BLOCKMONSTERS lines (PlayerPawns etc.)
  MF_PICKUP           = 0x0200, ///< Can/will pick up items. (players)
  MF_FLOAT            = 0x0400, ///< Active floater, can move freely in air (cacodemons etc.)
  MF_DROPOFF          = 0x0800, ///< Can jump/drop from high places
  MF_AMBUSH           = 0x1000, ///< *Not to be activated by sound, deaf monster.
  MF_NOSPLASH         = 0x2000, ///< Does not cause a splash when hitting water
  // appearance
  MF_SHADOW           = 0x4000, ///< Partial invisibility (spectre). Makes targeting harder.
  MF_ALTSHADOW        = 0x8000, ///< Alternate fuzziness
  MF_NOBLOOD         = 0x10000, ///< Does not bleed when shot (furniture)
  MF_NOSCORCH     = MF_NOBLOOD, ///< TEST flag double use for MF_MISSILEs, which cannot be hit (and thus cannot bleed)
  // spawning
  MF_SPAWNCEILING = 0x00020000, ///< Spawned hanging from the ceiling
  MF_SPAWNFLOAT   = 0x00040000, ///< Spawned at random height
  MF_NOTDMATCH    = 0x00080000, ///< Not spawned in DM (keycards etc.)
  MF_NORESPAWN    = 0x00100000, ///< Will not respawn after being picked up. Pretty similar to MF_DROPPED?
  // classification
  MF_COUNTKILL    = 0x00200000, ///< On kill, count this enemy object towards intermission kill total. Happy gathering.
  MF_COUNTITEM    = 0x00400000, ///< On picking up, count this item object towards intermission item total.
  MF_SPECIAL      = 0x00800000, ///< Call TouchSpecialThing when touched. Mostly items you can pick up.
  MF_DROPPED      = 0x01000000, ///< *Dropped by a monster
  MF_MISSILE      = 0x02000000, ///< Player missiles as well as fireballs. Don't hit same species, explode on block.
  MF_CORPSE       = 0x04000000, ///< *Acts like a corpse, falls down stairs etc.
  // 5 bits free
};


/// More semi-permanent flags. Mostly came with Heretic.
enum mobjflag2_t
{
  // physical properties
  MF2_LOGRAV         =     0x0001,    ///< Experiences only 1/8 gravity
  MF2_WINDTHRUST     =     0x0002,    ///< Is affected by wind
  MF2_FLOORBOUNCE    =     0x0004,    ///< Bounces off the floor
  MF2_SLIDE          =     0x0008,    ///< Slides against walls
  MF2_PUSHABLE       =     0x0010,    ///< Can be pushed by other moving actors
  MF2_CANNOTPUSH     =     0x0020,    ///< Cannot push other pushable actors
  // game mechanics
  MF2_FLOATBOB       =     0x0040,    ///< Bobs up and down in the air (item)
  MF2_THRUGHOST      =     0x0080,    ///< Will pass through ghosts (missile)
  MF2_RIP            =     0x0100,    ///< Rips through solid targets (missile)
  MF2_PASSMOBJ       =     0x0200,    ///< Can move over/under other Actors 
  MF2_NOTELEPORT     =     0x0400,    ///< Does not teleport
  MF2_NONSHOOTABLE   =     0x0800,    ///< Transparent to MF_MISSILEs
  MF2_INVULNERABLE   =     0x1000,    ///< Does not take damage
  MF2_DORMANT	     =     0x2000,    ///< Cannot be damaged, is not noticed by seekers
  MF2_CANTLEAVEFLOORPIC  = 0x4000,    ///< Stays within a certain floor texture
  MF2_BOSS           =     0x8000,    ///< Is a major boss, not as easy to kill
  MF2_SEEKERMISSILE  = 0x00010000,    ///< Is a seeker (for reflection)
  MF2_REFLECTIVE     = 0x00020000,    ///< Reflects missiles
  // rendering
  MF2_FOOTCLIP       = 0x00040000,    ///< Feet may be be clipped
  MF2_DONTDRAW       = 0x00080000,    ///< Invisible (does not generate a vissprite)
  // giving hurt
  MF2_FIREDAMAGE     = 0x00100000,    ///< Does fire damage
  MF2_ICEDAMAGE	     = 0x00200000,    ///< Does ice damage
  MF2_NODMGTHRUST    = 0x00400000,    ///< Does not thrust target when damaging        
  MF2_TELESTOMP      = 0x00800000,    ///< Can telefrag another Actor
  // 4 bits free (more damage types?)
  // activation
  MF2_IMPACT	     = 0x10000000,    ///< Can activate SPAC_IMPACT
  MF2_PUSHWALL	     = 0x20000000,    ///< Can activate SPAC_PUSH
  MF2_MCROSS	     = 0x40000000,    ///< Can activate SPAC_MCROSS
  MF2_PCROSS	     = 0x80000000,    ///< Can activate SPAC_PCROSS
};


/// Extra flags. They describe the transient state of the Actor.
enum mobjeflag_t
{
  // location
  MFE_ONGROUND      = 0x0001,  ///< Stands on solid floor (not on another Actor or in air)
  MFE_ONMOBJ        = 0x0002,  ///< Stands on top of another Actor
  // (instant damage in lava/slime sectors to prevent jump cheat..)
  MFE_JUSTHITFLOOR  = 0x0004,  ///< Just hit the floor while falling, cleared on next frame

  MFE_TOUCHWATER    = 0x0010,  ///< Touches water
  MFE_UNDERWATER    = 0x0020,  ///< Waist below water surface (swimming is possible)
  // active physics mode
  MFE_SWIMMING      = 0x0040,  ///< Swimming physics used (different gravity)
  MFE_FLY           = 0x0080,  ///< Well, flying. No gravity, some bobbing.
  MFE_INFLOAT       = 0x0100,  ///< Floating move in progress, don't auto float to target's height.
  MFE_SKULLFLY      = 0x0200,  ///< A charging skull or minotaur.
  MFE_BLASTED       = 0x0400,  ///< Uncontrollably thrown by a blast wave

  // combat
  MFE_JUSTHIT       = 0x1000,  ///< Got hit, will try to attack right back.
  MFE_JUSTATTACKED  = 0x2000,  ///< Will take at least one step before attacking again.

  MFE_REMOVE    = 0x80000000   ///< Actor will be deleted after the tic
};



/// \brief Basis class for all Thinkers with a well-defined location.
class Actor : public Thinker
{
  DECLARE_CLASS(Actor)
public:
  class presentation_t *pres;  ///< graphic presentation

  Actor *sprev, *snext;  ///< sector links
  Actor *bprev, *bnext;  ///< blockmap links

  struct subsector_t *subsector; ///< location

  /// The closest interval over all contacted Sectors (or Things).
  fixed_t floorz, ceilingz;

  /// a linked list of sectors where this object appears
  struct msecnode_t* touching_sectorlist;

  /// For nightmare and itemrespawn respawn.
  struct mapthing_t *spawnpoint;

public:
  /// position
  fixed_t x, y, z;

  // was: angle_t angle, aiming, (nothing)
  // TODO will be angle_t roll, pitch, yaw; // Euler angles
  angle_t  angle;  ///< orientation left-right
  angle_t  aiming; ///< up-down, updated with cmd->aiming.

  /// velocity, used to update position
  fixed_t px, py, pz;

  /// For movement checking.
  fixed_t mass;
  fixed_t radius;
  fixed_t height;

  int  health;

  int  flags;
  int  flags2;
  int  eflags;

  // Hexen fields
  short	tid;     ///< thing identifier
  byte	special; ///< special type
  byte	args[5]; ///< special arguments

  Actor *owner;   ///< Owner of this Actor. For example, for missiles this is the shooter.
  Actor *target;  ///< Thing being chased/attacked (or NULL), also the target for missiles.

  int reactiontime; ///< time (in tics) before the thing can attack or move again

  fixed_t floorclip; ///< cut this amount from legs (deep water illusion) (Hexen)

  short team; ///< see g_team.h

public:
  // in g_actor.cpp
  Actor(fixed_t nx, fixed_t ny, fixed_t nz); ///< construct a new Actor
  virtual ~Actor();

  void Remove();  ///< delayed destruction
  virtual void Detach();  ///< detaches the Actor from the Map

  virtual void Think();
  virtual void CheckPointers();

  void CheckWater(); ///< set some eflags if sector contains water

  float GetMoveFactor();
  virtual void XYMovement();
  virtual void ZMovement();

  virtual void XYFriction(fixed_t oldx, fixed_t oldy);
  virtual void Thrust(angle_t angle, fixed_t move);

  int  HitFloor();

  // in p_inter.cpp
  virtual bool Touch(Actor *a); ///< Actor touches another Actor
  virtual void Die(Actor *inflictor, Actor *source);
  virtual void Killed(class PlayerPawn *victim, Actor *inflictor);
  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  // in p_telept.cpp
  virtual bool Teleport(fixed_t nx, fixed_t ny, angle_t nangle, bool silent = false);

  // in p_map.cpp
  bool TeleportMove(fixed_t nx, fixed_t ny);
  bool TryMove(fixed_t nx, fixed_t ny, bool allowdropoff);
  void CheckMissileImpact();
  bool TestLocation();
  bool CheckPosition(fixed_t nx, fixed_t ny);
  void FakeZMovement();
  Actor *CheckOnmobj();

  fixed_t AimLineAttack(angle_t ang, fixed_t distance);
  void    LineAttack(angle_t ang, fixed_t distance, fixed_t slope,
		     int damage, int dtype = dt_normal);
  void  RadiusAttack(Actor *culprit, int damage, int distance = -1,
		     int dtype = dt_normal, bool downer = true);
  // in p_maputl.cpp
  void SetPosition();
  void UnsetPosition();
};


//========================================================
/// \brief Doom Actor.
///
/// An Actor with the standard Doom/Heretic AI
/// (uses the A_* routines and the states table in info.cpp)
class DActor : public Actor
{
  DECLARE_CLASS(DActor);
public:
  mobjtype_t  type;       ///< what kind of thing is it?
  const mobjinfo_t *info; ///< basic properties    

  // state machine variables
  const state_t *state;   ///< current state
  int            tics;    ///< state tic counter

  // Movement direction, movement generation (zig-zagging).
  int  movedir;    ///< 0-7
  int  movecount;  ///< when 0, select a new dir

  int  threshold;  ///< If >0, the target will be chased no matter what (even if shot)
  int  lastlook;   ///< Player number last looked for.

  int  special1, special2; ///< type dependent general storage

public:
  /// create a nonfunctional special DActor (LavaInflictor etc...)
  DActor(mobjtype_t t);
  /// create a new DActor of mobjtype t
  DActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t);

  virtual void Think();

  // in p_inter.cpp
  virtual bool Touch(Actor *a);
  virtual void Die(Actor *inflictor, Actor *source);
  virtual void Killed(PlayerPawn *victim, Actor *inflictor);
  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  bool SetState(statenum_t ns, bool call = true);

  void NightmareRespawn();

  DActor *SpawnMissile(Actor *dest, mobjtype_t type);
  bool   CheckMissileSpawn();
  void   ExplodeMissile();
  void   FloorBounceMissile();

  DActor *SpawnMissileAngle(mobjtype_t type, angle_t angle, fixed_t vz = 0);

  // in p_enemy.cpp
  bool CheckMeleeRange();
  bool CheckMissileRange();
  bool LookForPlayers(bool allaround);
  bool LookForMonsters();
  void P_NewChaseDir();
  bool P_TryWalk();
  bool P_Move();
  
  // in p_hpspr.cpp
  void BlasterMissileThink();
  void XBlasterMissileThink();

  // in p_henemy.cpp
  void DSparilTeleport();
  bool UpdateMorph(int tics);

  // in p_heretic.cpp
  bool SeekerMissile(angle_t thresh, angle_t turnMax);

  // in p_things.cpp
  bool Activate();
  bool Deactivate();
};

#endif
