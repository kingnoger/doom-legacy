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
// DESCRIPTION:
//    Pawn class. A Pawn is an Actor that has either AI or human control.
//    Ville Bergholm
//
//-----------------------------------------------------------------------------


#ifndef g_pawn_h
#define g_pawn_h 1

#include <vector>

#include "g_actor.h" // Actor class

#include "d_items.h"
#include "p_pspr.h" // 1st person weapon sprites

//  max Z move up or down without jumping
//  above this, a heigth difference is considered as a 'dropoff'
#define MAXSTEPMOVE (24*FRACUNIT)

class LArchive;
class PlayerInfo;
class Map;
struct line_t;
struct sector_t;
struct event_t;

struct inventory_t
{
  byte type;
  byte count;

  inventory_t()
  {
    type = count = 0;
  }

  inventory_t(byte t, byte c)
  {
    type = t;
    count = c;
  }
};

//
// Player internal flags, for cheats and debug. Must fit into an int.
//
typedef enum
{
  // No clipping, walk through barriers.
  CF_NOCLIP           = 1,
  // No damage, no health loss.
  CF_GODMODE          = 2,
  // Not really a cheat, just a debug aid.
  CF_NOMOMENTUM       = 4,

  //added:28-02-98: new cheats
  CF_FLYAROUND        = 8,

  //added:28-02-98: NOT REALLY A CHEAT
  // Allow player avatar to walk in-air
  //  if trying to get over a small wall (hack for playability)
  //CF_JUMPOVER         = 16

} cheat_t;

class Pawn : public Actor
{
private:


public:
  int maxhealth;

  // Who did damage (NULL for floors/ceilings).
  Actor *attacker;

public:
  Pawn(fixed_t x, fixed_t y, fixed_t z, mobjtype_t t);

  virtual void Think();
  virtual thinkertype_e Type() {return tt_pawn;}; // "name-tag" function
  virtual bool Morph();
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  bool GiveBody(int num);
};


class PlayerPawn : public Pawn
{
private:
  const char *message; // message to player

public:
  PlayerInfo *player; // controlling player

  int skin;
  int color; // may be defined by team membership. See PlayerInfo class.

  int morphTics;   // player is in a morphed state if > 0

  // inventory
  int invTics; // when >0 show inventory in hud
  vector<inventory_t> inventory;
  //int inventorySlotNum; // number of full inventory slots (inventory.size())
  int invSlot;   // active inventory slot is inventory[invSlot]
  int st_curpos; // position of inventory scroll (move to hud module?)

  // Power ups. invinc and invis are tic counters.
  int powers[NUMPOWERS];

  // Bit flags, for cheats and debug.
  int cheats;

  // True if button was down last tic.
  bool attackdown;
  bool usedown;
  bool jumpdown;   //added:19-03-98:dont jump like a monkey!
  int refire;    // Refired shots are less accurate.


  bool backpack;

  weapontype_t pendingweapon;   // Is wp_nochange if not changing.
  weapontype_t readyweapon;
  bool         weaponowned[NUMWEAPONS];

  int ammo[NUMAMMO];
  int maxammo[NUMAMMO];

  byte armortype;   // Armor type is 0-2.
  int  armorpoints;

  byte cards; // bit field see declration of card_t

  weaponinfo_t    *weaponinfo; // can be changed when use level2 weapons (heretic)

  // Overlay view sprites (gun, etc).
  pspdef_t psprites[NUMPSPRITES];

  int specialsector; // current sector special (lava/slime/water...)

  // So gun flashes light up areas.
  int  extralight;
  // Current PLAYPAL, ???
  //  can be set to REDCOLORMAP for pain, etc.
  int fixedcolormap;

  // heretic crap, remove...
  int flyheight; // z for smoothing the z motion
  int chickenPeck;
  int flamecount;
  Actor *rain1;   // active rain maker 1
  Actor *rain2;   // active rain maker 2

public:
  virtual thinkertype_e Type() {return tt_ppawn;}; // "name-tag" function

  // in g_pawn.cpp
  PlayerPawn(fixed_t x, fixed_t y, fixed_t z, mobjtype_t t);

  virtual int  Serialize(LArchive & a);

  virtual void XYMovement();
  virtual void ZMovement();
  virtual void XYFriction(fixed_t oldx, fixed_t oldy, bool oldfriction);

  virtual void Think();
  void DeathThink();
  virtual bool Touch(Actor *a); // PPawn touches another Actor

  virtual bool Morph();
  void MorphThink();
  bool UndoMorph();

  void FinishLevel();

  void UseArtifact(artitype_t arti);
  bool GivePower(int /*powertype_t*/ power);

  void CalcHeight(bool onground); // update bobbing view height
  void MovePsprites();

  //#define P_SpawnPlayerMissile(s,t) P_SPMAngle(s,t,s->angle)
  inline DActor *SpawnPlayerMissile(mobjtype_t type)
  {
    return SPMAngle(type, angle);
  }

  DActor *SPMAngle(mobjtype_t type, angle_t ang);

  bool CanUnlockGenDoor(line_t *line);
  void ProcessSpecialSector(sector_t *sector, bool instantdamage);
  void PlayerOnSpecial3DFloor();
  void PlayerInSpecialSector();

  // in g_game.cpp
  bool InventoryResponder(bool primary, event_t *ev);

  // in p_user.cpp
  virtual bool Teleport(fixed_t nx, fixed_t ny, angle_t nangle);
  void Move();
  virtual void Thrust(angle_t angle, fixed_t move);

  // in p_map.cpp
  void UseLines();

  // in p_inter.cpp
  void SetMessage(const char *msg, bool ultmsg = true);
  bool GiveAmmo(ammotype_t at, int count);
  bool GiveWeapon(weapontype_t wt, bool dropped);
  bool GiveArmor(int at);
  bool GiveCard(card_t ct);
  void TouchSpecialThing(DActor *special);
  virtual void Die(Actor *inflictor, Actor *source);

  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);
  bool GiveArtifact(artitype_t arti, Actor *from);

  // in p_pspr.cpp
  void UseFavoriteWeapon();
  void SetupPsprites();
  void SetPsprite(int position, statenum_t stnum);
  void DropWeapon();
  void FireWeapon();
  bool CheckAmmo();
  void BringUpWeapon();

  // in p_hpspr.cpp
  void ActivateBeak();

  // in p_heretic.cpp
  void HerePlayerInSpecialSector();
};



#endif
