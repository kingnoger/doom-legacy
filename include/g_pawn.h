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
// $Log$
// Revision 1.30  2005/06/29 14:26:39  smite-meister
// valgrind pays off
//
// Revision 1.29  2005/06/22 20:44:31  smite-meister
// alpha3 bugfixes
//
// Revision 1.28  2005/04/22 19:44:50  smite-meister
// bugs fixed
//
// Revision 1.27  2005/04/17 17:47:54  smite-meister
// netcode
//
// Revision 1.26  2004/12/31 16:19:40  smite-meister
// alpha fixes
//
// Revision 1.25  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.24  2004/11/13 22:38:59  smite-meister
// intermission works
//
// Revision 1.20  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.17  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.15  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.14  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.12  2003/05/05 00:24:50  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.11  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.10  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.9  2003/03/15 20:07:20  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.8  2003/03/08 16:07:15  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.7  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
// Revision 1.6  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.5  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Pawn and PlayerPawn classes.

#ifndef g_pawn_h
#define g_pawn_h 1

#include <vector>

#include "g_actor.h" // Actor class
#include "d_items.h"
#include "p_pspr.h" // 1st person weapon sprites

using namespace std;


/// Player internal flags, for cheats and debug. Must fit into an int.
enum cheat_t
{
  CF_NOCLIP      = 1,  // No clipping, walk through barriers.
  CF_GODMODE     = 2,  // No damage, no health loss.
  CF_NOMOMENTUM  = 4,  // Not really a cheat, just a debug aid.
  CF_FLYAROUND   = 8,  // Fly using jump key
};

enum pclass_t
{
  PCLASS_NONE = 0,
  PCLASS_FIGHTER,
  PCLASS_CLERIC,
  PCLASS_MAGE,
  PCLASS_PIG,
  NUMCLASSES
};


// TODO testing, this is a hack...
struct pawn_info_t
{
  mobjtype_t   mt;
  pclass_t     pclass;
  weapontype_t bweapon; // beginning weapon (besides fist/staff)
  int          bammo;   // ammo for bweapon
  mobjtype_t   nproj;   // natural projectile, if any
};

extern pawn_info_t pawndata[];


/// \brief An Actor that has active AI or human control.
class Pawn : public Actor
{
  DECLARE_CLASS(Pawn)
private:


public:
  int color; // stupid extra color value for automap

  int maxhealth;
  float speed; ///< walking speed (units/tic), runspeed = 2*speed
  const struct pawn_info_t *pinfo;

  int attackphase; ///< counter for the more complex weapons

  Actor *attacker; ///< who damaged the Pawn? (NULL for floors/ceilings).

public:
  Pawn(fixed_t x, fixed_t y, fixed_t z, int type);

  virtual void Detach();
  virtual void Think();
  virtual void CheckPointers();

  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  bool GiveBody(int num);
};


/// \brief A Pawn that represents a player avatar.
///
/// It takes orders from its player, which is represented
/// by a PlayerInfo instance.

class PlayerPawn : public Pawn
{
  DECLARE_CLASS(PlayerPawn);
public:
  /// Controlling player
  class PlayerInfo *player; 

  /// player class, a Hexen kludge
  byte pclass;

  /// True if the corresponding button was down last tic.
  bool attackdown;
  bool usedown;
  bool jumpdown;  // don't jump like a monkey!

  int refire;     ///< Refired shots are less accurate. to Pawn?
  int morphTics;  ///< Player is in a morphed state if >0
  int fly_zspeed; ///< For smoothing the z motion while flying

  /// First person sprites (weapon and muzzle flash)
  pspdef_t psprites[NUMPSPRITES];

  /// Inventory
  vector<inventory_t> inventory;

  int  keycards; ///< Bit field, see the definition of keycard_t

  weapontype_t readyweapon;   ///< Current weapon
  weapontype_t pendingweapon; ///< Weapon we are changing to or wp_nochange
  bool         weaponowned[NUMWEAPONS];

  const weaponinfo_t *weaponinfo; ///< Changed when using level2 weapons (Heretic)

  int ammo[NUMAMMO];
  int maxammo[NUMAMMO];

  float toughness; ///< Natural armor, depends on class.
  float armorfactor[NUMARMOR];
  int   armorpoints[NUMARMOR];

  /// Tic counters for power ups.
  int powers[NUMPOWERS];

  /// Bit flags, for cheats and debug.
  int cheats;


  /// Current sector special (lava/slime/water...)
  int specialsector;

  /// Gun flashes light up nearby areas.
  int extralight;

  /// Colormap to replace the lightlevel-based colormap in rendering.
  /// Used for invulnerability, IR goggles etc.
  int fixedcolormap;

public:
  // in g_pawn.cpp
  PlayerPawn(fixed_t x, fixed_t y, fixed_t z, int type, int pclass = PCLASS_NONE);
  ~PlayerPawn();

  virtual void Think();
  void DeathThink();
  void MorphThink();

  void Move();
  virtual void XYMovement();
  virtual void ZMovement();
  virtual void XYFriction(fixed_t oldx, fixed_t oldy, bool oldfriction);
  virtual bool Teleport(fixed_t nx, fixed_t ny, angle_t nangle, bool silent = false);

  void Reset();
  weapontype_t FindWeapon(int g);

  inline DActor *SpawnPlayerMissile(mobjtype_t type) { return SPMAngle(type, angle); }
  DActor *SPMAngle(mobjtype_t type, angle_t ang);

  bool CanUnlockGenDoor(struct line_t *line);
  void ProcessSpecialSector(struct sector_t *sector, bool instantdamage);
  void PlayerOnSpecial3DFloor();
  void PlayerInSpecialSector();

  bool GivePower(int power);
  bool GiveAmmo(ammotype_t at, int count);
  bool GiveWeapon(weapontype_t wt, int ammocontent, bool dropped = false);
  bool GiveArmor(armortype_t type, float factor, int points);
  bool GiveKey(keycard_t k);
  bool GiveArtifact(artitype_t arti, DActor *from);

  // in p_user.cpp
  virtual bool Morph(mobjtype_t form);
  bool UndoMorph();

  void UseArtifact(artitype_t arti);

  // in p_map.cpp
  void UseLines();
  bool UsePuzzleItem(int type);

  // in p_inter.cpp
  void TouchSpecialThing(DActor *special);
  virtual bool Touch(Actor *a);
  virtual void Die(Actor *inflictor, Actor *source);
  virtual void Killed(PlayerPawn *victim, Actor *inflictor);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  // in p_pspr.cpp
  void MovePsprites();
  void UseFavoriteWeapon();
  void SetupPsprites();
  void SetPsprite(int position, weaponstatenum_t stnum, bool call = true);
  void DropWeapon();
  void FireWeapon();
  bool CheckAmmo();
  void BringUpWeapon();

  // in p_hpspr.cpp
  void ActivateMorphWeapon();
  void PostMorphWeapon(weapontype_t weapon);
};



#endif
