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
// Revision 1.19  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.18  2004/01/06 14:37:45  smite-meister
// six bugfixes, cleanup
//
// Revision 1.17  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.16  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.15  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.14  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.13  2003/05/11 21:23:52  smite-meister
// Hexen fixes
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
//
//
// DESCRIPTION:
//    Pawn class. A Pawn is an Actor that has either AI or human control.
//
//-----------------------------------------------------------------------------


#ifndef g_pawn_h
#define g_pawn_h 1

#include <vector>

#include "g_actor.h" // Actor class
#include "d_items.h"
#include "p_pspr.h" // 1st person weapon sprites

using namespace std;

#define JUMPSPEED (6*FRACUNIT/NEWTICRATERATIO)


//
// Player internal flags, for cheats and debug. Must fit into an int.
//
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
  mobjtype_t mt;
  weapontype_t bweapon; // beginning weapon (besides fist/staff)
  int bammo;            // ammo for bweapon
  mobjtype_t nproj;     // natural projectile, if any
  // TODO nproj should be replaced with a function pointer.
  // Somebody should then write these shooting functions...
};


class Pawn : public Actor
{
  DECLARE_CLASS(Pawn);
private:


public:
  int color; // stupid extra color value for automap
  //int skin;

  int maxhealth;
  float speed; // walking speed (units/tic), runspeed = 2*speed
  const struct pawn_info_t *pinfo;

  int attackphase; // for the more complex weapons
  // Who did damage (NULL for floors/ceilings).
  Actor *attacker;

public:
  Pawn(fixed_t x, fixed_t y, fixed_t z, int type);

  virtual void Detach();
  virtual void Think();
  virtual void CheckPointers();

  virtual thinkertype_e Type() {return tt_pawn;}; // "name-tag" function
  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  bool GiveBody(int num);
};


class PlayerPawn : public Pawn
{
  DECLARE_CLASS(PlayerPawn);
public:
  // Overlay view sprites (gun, etc).
  pspdef_t psprites[NUMPSPRITES];

  class PlayerInfo *player; // controlling player

  byte pclass; // player class, a Hexen kludge

  int morphTics;   // player is in a morphed state if > 0

  // Inventory
  int invTics; // when >0 show inventory in hud
  vector<inventory_t> inventory;
  int invSlot;   // active inventory slot is inventory[invSlot]

  // Tic counters for power ups.
  int powers[NUMPOWERS];

  // Bit flags, for cheats and debug.
  int cheats;

  // True if button was down last tic. (into eflags?)
  bool attackdown;
  bool usedown;
  bool jumpdown;   // dont jump like a monkey!

  int  refire;     // Refired shots are less accurate.

  int  keycards; // bit field, see the definition of keycard_t
  bool backpack;

  weapontype_t pendingweapon;   // Is wp_nochange if not changing.
  weapontype_t readyweapon;
  bool         weaponowned[NUMWEAPONS];

  const weaponinfo_t *weaponinfo; // can be changed when use level2 weapons (heretic)

  int ammo[NUMAMMO];
  const int *maxammo;

  float toughness; // natural armor
  float armorfactor[NUMARMOR];
  int   armorpoints[NUMARMOR];

  int specialsector; // current sector special (lava/slime/water...)

  // So gun flashes light up areas.
  int  extralight;
  // Current PLAYPAL, ???
  //  can be set to REDCOLORMAP for pain, etc.
  int fixedcolormap;

  int fly_zspeed; //  for smoothing the z motion while flying

public:
  virtual thinkertype_e Type() {return tt_ppawn;}; // "name-tag" function

  // in g_pawn.cpp
  PlayerPawn(fixed_t x, fixed_t y, fixed_t z, int type);

  virtual void XYMovement();
  virtual void ZMovement();
  virtual void XYFriction(fixed_t oldx, fixed_t oldy, bool oldfriction);

  virtual void Think();
  void DeathThink();

  void MorphThink();
  bool UndoMorph();

  void Reset();

  void UseArtifact(artitype_t arti);
  bool GivePower(int /*powertype_t*/ power);

  void MovePsprites();

  inline DActor *SpawnPlayerMissile(mobjtype_t type) { return SPMAngle(type, angle); }
  DActor *SPMAngle(mobjtype_t type, angle_t ang);

  bool CanUnlockGenDoor(struct line_t *line);
  void ProcessSpecialSector(struct sector_t *sector, bool instantdamage);
  void PlayerOnSpecial3DFloor();
  void PlayerInSpecialSector();

  // in p_user.cpp
  virtual bool Teleport(fixed_t nx, fixed_t ny, angle_t nangle);
  void Move();
  virtual void Thrust(angle_t angle, fixed_t move);

  // in p_map.cpp
  void UseLines();
  bool UsePuzzleItem(int type);

  // in p_inter.cpp
  bool GiveAmmo(ammotype_t at, int count);
  bool GiveWeapon(weapontype_t wt, bool dropped);
  bool GiveArmor(armortype_t type, float factor, int points);
  bool GiveKey(keycard_t k);
  bool GiveArtifact(artitype_t arti, DActor *from);
  void TouchSpecialThing(DActor *special);
  virtual bool Touch(Actor *a); // PPawn touches another Actor
  virtual void Die(Actor *inflictor, Actor *source);
  virtual void Killed(PlayerPawn *victim, Actor *inflictor);
  virtual bool Morph(mobjtype_t form);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  // in p_pspr.cpp
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
