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
// Revision 1.2  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.1.1.1  2002/11/16 14:17:49  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION: 
//      holds the weapon info for now...
//
//-----------------------------------------------------------------------------



// We are referring to sprite numbers.
#include "info.h"
#include "d_items.h"

int weapongroup[NUMWEAPONS] =
{
  0,1,2,3,4,5,6,0,2, // Doom
  0,1,2,3,5,4,6,0,7  // Heretic
};


weapontype_t wgroups[8][4] =
{
  {wp_fist,     wp_staff,        wp_chainsaw, wp_gauntlets},
  {wp_pistol,   wp_goldwand,     wp_nochange, wp_nochange},
  {wp_shotgun,  wp_supershotgun, wp_crossbow, wp_nochange},
  {wp_chaingun, wp_blaster,      wp_nochange, wp_nochange},
  {wp_missile,  wp_phoenixrod,   wp_nochange, wp_nochange},
  {wp_plasma,   wp_skullrod,     wp_nochange, wp_nochange},
  {wp_bfg,      wp_mace,         wp_nochange, wp_nochange},
  {wp_beak,     wp_nochange,     wp_nochange, wp_nochange}
};


int maxammo1[NUMAMMO] =
{
  200, 50, 300, 50, // Doom
  100, 50, 200, 200, 20, 150  // Heretic
};

int maxammo2[NUMAMMO] =
{
  400, 100, 600, 100, // Doom
  200, 100, 400, 400, 40, 300 // Heretic
};

// a weapon is found with two clip loads,
// a big item has five clip loads
int clipammo[NUMAMMO] =
{
  10, 4, 20, 1,       // Doom
  5, 2, 6, 10, 1, 10  // Heretic // used in deathmatch 1 & 3 mul by 5 (P_GiveWeapon)
};

// ammo get with the weapon
int GetWeaponAmmo[NUMWEAPONS] =
{
  0,  // fist
  20, // pistol
  8,  // shotgun
  20, // chaingun
  2,  // missile    
  40, // plasma     
  40, // bfg        
  0,  // chainsaw   
  8,  // supershotgun

  // Heretic
  0,  // staff
  25, // gold wand
  10, // crossbow
  30, // blaster
  50, // skull rod
  2,  // phoenix rod
  50, // mace
  0,  // gauntlets
  0   // beak
};

//
// PSPRITE ACTIONS for weapons.
// This struct controls the weapon animations.
//


weaponinfo_t wpnlev1info[NUMWEAPONS] =
{
  // Doom weapons
  {am_noammo, 0, S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_PUNCH1, S_NULL},             // fist
  {am_clip,   1, S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOL1, S_PISTOLFLASH}, // pistol
  {am_shell,  1, S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUN1, S_SGUNFLASH1},            // shotgun
  {am_clip,   1, S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAIN1, S_CHAINFLASH1},      // chaingun
  {am_misl,   1, S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILE1, S_MISSILEFLASH1}, // missile launcher
  {am_cell,   1, S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMA1, S_PLASMAFLASH1},       // plasma rifle
  {am_cell,  40, S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFG1, S_BFGFLASH1}, // bfg 9000
  {am_noammo, 0, S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_SAW1, S_NULL},      // chainsaw
  {am_shell,  2, S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUN1, S_DSGUNFLASH1}, // super shotgun

  // Heretic weapons
  {am_noammo, 0, S_STAFFUP, S_STAFFDOWN, S_STAFFREADY, S_STAFFATK1_1, S_STAFFATK1_1, S_NULL}, // Staff
  {am_goldwand, USE_GWND_AMMO_1, S_GOLDWANDUP, S_GOLDWANDDOWN, S_GOLDWANDREADY, S_GOLDWANDATK1_1, S_GOLDWANDATK1_1, S_NULL}, // Gold wand
  {am_crossbow, USE_CBOW_AMMO_1, S_CRBOWUP, S_CRBOWDOWN, S_CRBOW1, S_CRBOWATK1_1, S_CRBOWATK1_1, S_NULL}, // Crossbow
  {am_blaster, USE_BLSR_AMMO_1, S_BLASTERUP, S_BLASTERDOWN, S_BLASTERREADY, S_BLASTERATK1_1, S_BLASTERATK1_3, S_NULL}, // Blaster
  {am_skullrod, USE_SKRD_AMMO_1, S_HORNRODUP, S_HORNRODDOWN, S_HORNRODREADY, S_HORNRODATK1_1, S_HORNRODATK1_1, S_NULL}, // Skull rod
  {am_phoenixrod, USE_PHRD_AMMO_1, S_PHOENIXUP, S_PHOENIXDOWN, S_PHOENIXREADY, S_PHOENIXATK1_1, S_PHOENIXATK1_1, S_NULL}, // Phoenix rod
  {am_mace, USE_MACE_AMMO_1, S_MACEUP, S_MACEDOWN, S_MACEREADY, S_MACEATK1_1, S_MACEATK1_2, S_NULL}, // Mace
  {am_noammo, 0, S_GAUNTLETUP, S_GAUNTLETDOWN, S_GAUNTLETREADY, S_GAUNTLETATK1_1, S_GAUNTLETATK1_3, S_NULL}, // Gauntlets
  {am_noammo, 0, S_BEAKUP, S_BEAKDOWN, S_BEAKREADY, S_BEAKATK1_1, S_BEAKATK1_1, S_NULL} // Beak
};


// right now Doom weapons are exactly the same when tome of power is active, but who knows...

weaponinfo_t wpnlev2info[NUMWEAPONS] =
{
  // Doom weapons
  {am_noammo, 0, S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_PUNCH1, S_NULL},             // fist
  {am_clip,   1, S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOL1, S_PISTOLFLASH}, // pistol
  {am_shell,  1, S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUN1, S_SGUNFLASH1},            // shotgun
  {am_clip,   1, S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAIN1, S_CHAINFLASH1},      // chaingun
  {am_misl,   1, S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILE1, S_MISSILEFLASH1}, // missile launcher
  {am_cell,   1, S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMA1, S_PLASMAFLASH1},       // plasma rifle
  {am_cell,  40, S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFG1, S_BFGFLASH1}, // bfg 9000
  {am_noammo, 0, S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_SAW1, S_NULL},      // chainsaw
  {am_shell,  2, S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUN1, S_DSGUNFLASH1}, // super shotgun

  // Heretic weapons
  {am_noammo, 0, S_STAFFUP2, S_STAFFDOWN2, S_STAFFREADY2_1, S_STAFFATK2_1, S_STAFFATK2_1, S_NULL}, // Staff
  {am_goldwand, USE_GWND_AMMO_2, S_GOLDWANDUP, S_GOLDWANDDOWN, S_GOLDWANDREADY, S_GOLDWANDATK2_1, S_GOLDWANDATK2_1, S_NULL}, // Gold wand
  {am_crossbow, USE_CBOW_AMMO_2, S_CRBOWUP, S_CRBOWDOWN, S_CRBOW1, S_CRBOWATK2_1, S_CRBOWATK2_1, S_NULL}, // Crossbow
  {am_blaster, USE_BLSR_AMMO_2, S_BLASTERUP, S_BLASTERDOWN, S_BLASTERREADY, S_BLASTERATK2_1, S_BLASTERATK2_3, S_NULL}, // Blaster
  {am_skullrod, USE_SKRD_AMMO_2, S_HORNRODUP, S_HORNRODDOWN, S_HORNRODREADY, S_HORNRODATK2_1, S_HORNRODATK2_1, S_NULL}, // Skull rod
  {am_phoenixrod, USE_PHRD_AMMO_2, S_PHOENIXUP, S_PHOENIXDOWN, S_PHOENIXREADY, S_PHOENIXATK2_1, S_PHOENIXATK2_2, S_NULL}, // Phoenix rod
  {am_mace, USE_MACE_AMMO_2, S_MACEUP, S_MACEDOWN, S_MACEREADY, S_MACEATK2_1, S_MACEATK2_1, S_NULL}, // Mace
  {am_noammo, 0, S_GAUNTLETUP2, S_GAUNTLETDOWN2, S_GAUNTLETREADY2_1, S_GAUNTLETATK2_1, S_GAUNTLETATK2_3, S_NULL}, // Gauntlets
  {am_noammo, 0, S_BEAKUP, S_BEAKDOWN, S_BEAKREADY, S_BEAKATK2_1, S_BEAKATK2_1, S_NULL} // Beak
};
