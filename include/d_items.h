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
// Revision 1.4  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.3  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.2  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Items: key cards, artifacts, weapon, ammunition.
//
//-----------------------------------------------------------------------------


#ifndef d_items_h
#define d_items_h 1

#include "doomdef.h"
#include "doomtype.h"
#include "info.h"

#ifdef __GNUG__
#pragma interface
#endif


// heretic stuff
#define AMMO_GWND_WIMPY 10
#define AMMO_GWND_HEFTY 50
#define AMMO_CBOW_WIMPY 5
#define AMMO_CBOW_HEFTY 20
#define AMMO_BLSR_WIMPY 10
#define AMMO_BLSR_HEFTY 25
#define AMMO_SKRD_WIMPY 20
#define AMMO_SKRD_HEFTY 100
#define AMMO_PHRD_WIMPY 1
#define AMMO_PHRD_HEFTY 10
#define AMMO_MACE_WIMPY 20
#define AMMO_MACE_HEFTY 100

#define USE_GWND_AMMO_1 1
#define USE_GWND_AMMO_2 1
#define USE_CBOW_AMMO_1 1
#define USE_CBOW_AMMO_2 1
#define USE_BLSR_AMMO_1 1
#define USE_BLSR_AMMO_2 5
#define USE_SKRD_AMMO_1 1
#define USE_SKRD_AMMO_2 5
#define USE_PHRD_AMMO_1 1
#define USE_PHRD_AMMO_2 1
#define USE_MACE_AMMO_1 1
#define USE_MACE_AMMO_2 5

//
// Key cards.
//
typedef enum
{
  it_bluecard   =    1,
  it_yellowcard =    2,
  it_redcard    =    4,
  it_blueskull  =    8,
  it_yellowskull= 0x10,
  it_redskull   = 0x20,
  it_allkeys    = 0x3f,
  NUMCARDS      = 6
} card_t;

typedef enum
{
  arti_none,
  arti_invulnerability,
  arti_invisibility,
  arti_health,
  arti_superhealth,
  arti_tomeofpower,
  arti_torch,
  arti_firebomb,
  arti_egg,
  arti_fly,
  arti_teleport,
  NUMARTIFACTS
} artitype_t;

// playerpawn inventory
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

#define NUMINVENTORYSLOTS  14
#define MAXARTECONT        16 

// Power up artifacts.
typedef enum
{
  pw_invulnerability,
  pw_strength,
  pw_invisibility,
  pw_ironfeet,
  pw_allmap,
  pw_infrared,

  // heretic
  pw_weaponlevel2,
  pw_flight,

  NUMPOWERS

} powertype_t;

//
// Power up durations,
//  how many seconds till expiration,
//  assuming TICRATE is 35 ticks/second.
//
typedef enum
{
  INVULNTICS  = (30*TICRATE),
  INVISTICS   = (60*TICRATE),
  INFRATICS   = (120*TICRATE),
  IRONTICS    = (60*TICRATE)
} powerduration_t;


// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum
{
  wp_doom,
  wp_fist = wp_doom,
  wp_pistol,
  wp_shotgun,
  wp_chaingun,
  wp_missile,
  wp_plasma,
  wp_bfg,
  wp_chainsaw,
  wp_supershotgun,

  // heretic stuff
  wp_heretic,
  wp_staff = wp_heretic, //=wp_fist,
  wp_goldwand,
  wp_crossbow,
  wp_blaster,
  wp_skullrod,
  wp_phoenixrod,
  wp_mace,
  wp_gauntlets,
  wp_beak,

  NUMWEAPONS,

  // No pending weapon change.
  wp_nochange,
  wp_barrel // barrel explosion
} weapontype_t;


// Ammunition types defined.
typedef enum
{
  am_doom,
  am_clip = am_doom, // Pistol / chaingun ammo.
  am_shell,   // Shotgun / double barreled shotgun.
  am_cell,    // Plasma rifle, BFG.
  am_misl,    // Missile launcher.

  // heretic stuff
  am_heretic,
  am_goldwand = am_heretic, // = am_clip,
  am_crossbow,
  am_blaster,
  am_skullrod,
  am_phoenixrod,
  am_mace,

  NUMAMMO,
  am_noammo = NUMAMMO   // Unlimited for chainsaw / fist.

} ammotype_t;


// "volatile" weapon info (tome of power etc...): sprite frames, ammunition use.
struct weaponinfo_t
{
  ammotype_t ammo;
  int        ammopershoot;
  statenum_t upstate;
  statenum_t downstate;
  statenum_t readystate;
  statenum_t atkstate;
  statenum_t holdatkstate;
  statenum_t flashstate;
};

// "static" weapon info
struct weapondata_t
{
  int group;   // in which group it resides
  int getammo; // how much ammo does a discarded weapon contain?
};

extern weapondata_t weapondata[NUMWEAPONS];
extern weapontype_t wgroups[8][4];

//extern weaponinfo_t doomweaponinfo[NUMWEAPONS];
extern weaponinfo_t wpnlev1info[NUMWEAPONS];
extern weaponinfo_t wpnlev2info[NUMWEAPONS];

// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern int maxammo1[NUMAMMO];
extern int maxammo2[NUMAMMO];

extern int clipammo[NUMAMMO];

#endif
