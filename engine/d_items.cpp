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
// Revision 1.7  2003/05/30 13:34:41  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.6  2003/04/04 00:01:52  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.5  2003/03/15 20:07:13  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.4  2003/03/08 16:06:58  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.3  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
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

// "static" weapon data
weapondata_t weapondata[NUMWEAPONS+1] =
{
  // Doom
  {0, wp_chainsaw, 0}, {0, wp_staff, 0}, {1, wp_goldwand, 20}, {2, wp_supershotgun, 8}, {2, wp_crossbow, 8},
  {3, wp_blaster, 20}, {4, wp_phoenixrod, 2}, {5, wp_skullrod, 40}, {6, wp_mace, 40},
  // Heretic
  {0, wp_gauntlets, 0}, {0, wp_fpunch, 0}, {1, wp_timons_axe, 25}, {2, wp_hammer_of_retribution, 10},
  {3, wp_quietus, 30}, {4, wp_missile, 2}, {5, wp_plasma, 50}, {6, wp_bfg, 50}, {7, wp_snout, 0},
  // Hexen
  {0, wp_cmace, 0}, {0, wp_mwand, 0}, {0, wp_fist, 0}, {1, wp_serpent_staff, 25}, {1, wp_cone_of_shards, 25},
  {1, wp_pistol, 25}, {2, wp_firestorm, 25}, {2, wp_arc_of_death, 25}, {2, wp_shotgun, 25},
  {3, wp_wraithverge, 25}, {3, wp_bloodscourge, 25}, {3, wp_chaingun, 25}, {7, wp_beak, 0},
  // wp_nochange
  {0, wp_fist, 0}
};

int maxammo1[NUMAMMO] =
{
  200, 50, 300, 50, // Doom
  100, 50, 200, 200, 20, 150,  // Heretic
  200, 200 // Hexen
};

int maxammo2[NUMAMMO] =
{
  400, 100, 600, 100, // Doom
  200, 100, 400, 400, 40, 300, // Heretic
  200, 200 // Hexen
};

// a weapon is found with two clip loads,
// a big item has five clip loads
int clipammo[NUMAMMO] =
{
  10, 4, 20, 1,       // Doom
  5, 2, 6, 10, 1, 10,  // Heretic // used in deathmatch 1 & 3 mul by 5 (P_GiveWeapon)
  15, 15 // Hexen
};


//
// PSPRITE ACTIONS for weapons.
// This struct controls the weapon animations.
//


weaponinfo_t wpnlev1info[NUMWEAPONS] =
{
  // Doom weapons
  {am_noammo, 0, S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_PUNCH1, S_WNULL},             // fist
  {am_noammo, 0, S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_SAW1, S_WNULL},      // chainsaw
  {am_clip,   1, S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOL1, S_PISTOLFLASH}, // pistol
  {am_shell,  1, S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUN1, S_SGUNFLASH1},            // shotgun
  {am_shell,  2, S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUN1, S_DSGUNFLASH1}, // super shotgun
  {am_clip,   1, S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAIN1, S_CHAINFLASH1},      // chaingun
  {am_misl,   1, S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILE1, S_MISSILEFLASH1}, // missile launcher
  {am_cell,   1, S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMA1, S_PLASMAFLASH1},       // plasma rifle
  {am_cell,  40, S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFG1, S_BFGFLASH1}, // bfg 9000

  // Heretic weapons
  {am_noammo, 0, S_STAFFUP, S_STAFFDOWN, S_STAFFREADY, S_STAFFATK1_1, S_STAFFATK1_1, S_WNULL}, // Staff
  {am_noammo, 0, S_GAUNTLETUP, S_GAUNTLETDOWN, S_GAUNTLETREADY, S_GAUNTLETATK1_1, S_GAUNTLETATK1_3, S_WNULL}, // Gauntlets
  {am_goldwand, USE_GWND_AMMO_1, S_GOLDWANDUP, S_GOLDWANDDOWN, S_GOLDWANDREADY, S_GOLDWANDATK1_1, S_GOLDWANDATK1_1, S_WNULL}, // Gold wand
  {am_crossbow, USE_CBOW_AMMO_1, S_CRBOWUP, S_CRBOWDOWN, S_CRBOW1, S_CRBOWATK1_1, S_CRBOWATK1_1, S_WNULL}, // Crossbow
  {am_blaster, USE_BLSR_AMMO_1, S_BLASTERUP, S_BLASTERDOWN, S_BLASTERREADY, S_BLASTERATK1_1, S_BLASTERATK1_3, S_WNULL}, // Blaster
  {am_phoenixrod, USE_PHRD_AMMO_1, S_PHOENIXUP, S_PHOENIXDOWN, S_PHOENIXREADY, S_PHOENIXATK1_1, S_PHOENIXATK1_1, S_WNULL}, // Phoenix rod
  {am_skullrod, USE_SKRD_AMMO_1, S_HORNRODUP, S_HORNRODDOWN, S_HORNRODREADY, S_HORNRODATK1_1, S_HORNRODATK1_1, S_WNULL}, // Skull rod
  {am_mace, USE_MACE_AMMO_1, S_MACEUP, S_MACEDOWN, S_MACEREADY, S_MACEATK1_1, S_MACEATK1_2, S_WNULL}, // Mace
  {am_noammo, 0, S_BEAKUP, S_BEAKDOWN, S_BEAKREADY, S_BEAKATK1_1, S_BEAKATK1_1, S_WNULL}, // Beak

  // Hexen weapons
  {am_noammo, 0, S_XPUNCHUP, S_XPUNCHDOWN, S_XPUNCHREADY, S_PUNCHATK1_1, S_PUNCHATK1_1, S_WNULL}, // Fighter - Punch
  {am_noammo, 0, S_CMACEUP, S_CMACEDOWN, S_CMACEREADY, S_CMACEATK_1, S_CMACEATK_1, S_WNULL},   // Cleric - Mace
  {am_noammo, 0, S_MWANDUP, S_MWANDDOWN, S_MWANDREADY, S_MWANDATK_1, S_MWANDATK_1, S_WNULL },  // Mage - Wand
  {am_noammo, 2, S_FAXEUP, S_FAXEDOWN, S_FAXEREADY, S_FAXEATK_1, S_FAXEATK_1, S_WNULL},        // Fighter - Axe
  {am_mana1,  1, S_CSTAFFUP, S_CSTAFFDOWN, S_CSTAFFREADY, S_CSTAFFATK_1, S_CSTAFFATK_1, S_WNULL}, // Cleric - Serpent Staff
  {am_mana1,  3, S_CONEUP, S_CONEDOWN, S_CONEREADY, S_CONEATK1_1, S_CONEATK1_3, S_WNULL},         // Mage - Cone of shards
  {am_noammo, 3, S_FHAMMERUP, S_FHAMMERDOWN, S_FHAMMERREADY, S_FHAMMERATK_1, S_FHAMMERATK_1, S_WNULL}, // Fighter - Hammer
  {am_mana2,  4, S_CFLAMEUP, S_CFLAMEDOWN, S_CFLAMEREADY1, S_CFLAMEATK_1, S_CFLAMEATK_1, S_WNULL}, // Cleric - Flame Strike
  {am_mana2,  5, S_MLIGHTNINGUP, S_MLIGHTNINGDOWN, S_MLIGHTNINGREADY, S_MLIGHTNINGATK_1, S_MLIGHTNINGATK_1, S_WNULL}, // Mage - Lightning
  {am_manaboth, 14, S_FSWORDUP, S_FSWORDDOWN, S_FSWORDREADY, S_FSWORDATK_1, S_FSWORDATK_1, S_WNULL}, // Fighter - Rune Sword
  {am_manaboth, 18, S_CHOLYUP, S_CHOLYDOWN, S_CHOLYREADY, S_CHOLYATK_1, S_CHOLYATK_1, S_WNULL},      // Cleric - Holy Symbol
  {am_manaboth, 15, S_MSTAFFUP, S_MSTAFFDOWN, S_MSTAFFREADY, S_MSTAFFATK_1, S_MSTAFFATK_1, S_WNULL}, // Mage - Staff
  {am_noammo, 0, S_SNOUTUP, S_SNOUTDOWN, S_SNOUTREADY, S_SNOUTATK1, S_SNOUTATK1, S_WNULL} // Pig - Snout
};


// right now Doom and Hexen weapons are exactly the same when tome of power is active, but who knows...

weaponinfo_t wpnlev2info[NUMWEAPONS] =
{
  // Doom weapons
  {am_noammo, 0, S_PUNCHUP, S_PUNCHDOWN, S_PUNCH, S_PUNCH1, S_PUNCH1, S_WNULL},             // fist
  {am_noammo, 0, S_SAWUP, S_SAWDOWN, S_SAW, S_SAW1, S_SAW1, S_WNULL},      // chainsaw
  {am_clip,   1, S_PISTOLUP, S_PISTOLDOWN, S_PISTOL, S_PISTOL1, S_PISTOL1, S_PISTOLFLASH}, // pistol
  {am_shell,  1, S_SGUNUP, S_SGUNDOWN, S_SGUN, S_SGUN1, S_SGUN1, S_SGUNFLASH1},            // shotgun
  {am_shell,  2, S_DSGUNUP, S_DSGUNDOWN, S_DSGUN, S_DSGUN1, S_DSGUN1, S_DSGUNFLASH1}, // super shotgun
  {am_clip,   1, S_CHAINUP, S_CHAINDOWN, S_CHAIN, S_CHAIN1, S_CHAIN1, S_CHAINFLASH1},      // chaingun
  {am_misl,   1, S_MISSILEUP, S_MISSILEDOWN, S_MISSILE, S_MISSILE1, S_MISSILE1, S_MISSILEFLASH1}, // missile launcher
  {am_cell,   1, S_PLASMAUP, S_PLASMADOWN, S_PLASMA, S_PLASMA1, S_PLASMA1, S_PLASMAFLASH1},       // plasma rifle
  {am_cell,  40, S_BFGUP, S_BFGDOWN, S_BFG, S_BFG1, S_BFG1, S_BFGFLASH1}, // bfg 9000

  // Heretic weapons
  {am_noammo, 0, S_STAFFUP2, S_STAFFDOWN2, S_STAFFREADY2_1, S_STAFFATK2_1, S_STAFFATK2_1, S_WNULL}, // Staff
  {am_noammo, 0, S_GAUNTLETUP2, S_GAUNTLETDOWN2, S_GAUNTLETREADY2_1, S_GAUNTLETATK2_1, S_GAUNTLETATK2_3, S_WNULL}, // Gauntlets
  {am_goldwand, USE_GWND_AMMO_2, S_GOLDWANDUP, S_GOLDWANDDOWN, S_GOLDWANDREADY, S_GOLDWANDATK2_1, S_GOLDWANDATK2_1, S_WNULL}, // Gold wand
  {am_crossbow, USE_CBOW_AMMO_2, S_CRBOWUP, S_CRBOWDOWN, S_CRBOW1, S_CRBOWATK2_1, S_CRBOWATK2_1, S_WNULL}, // Crossbow
  {am_blaster, USE_BLSR_AMMO_2, S_BLASTERUP, S_BLASTERDOWN, S_BLASTERREADY, S_BLASTERATK2_1, S_BLASTERATK2_3, S_WNULL}, // Blaster
  {am_phoenixrod, USE_PHRD_AMMO_2, S_PHOENIXUP, S_PHOENIXDOWN, S_PHOENIXREADY, S_PHOENIXATK2_1, S_PHOENIXATK2_2, S_WNULL}, // Phoenix rod
  {am_skullrod, USE_SKRD_AMMO_2, S_HORNRODUP, S_HORNRODDOWN, S_HORNRODREADY, S_HORNRODATK2_1, S_HORNRODATK2_1, S_WNULL}, // Skull rod
  {am_mace, USE_MACE_AMMO_2, S_MACEUP, S_MACEDOWN, S_MACEREADY, S_MACEATK2_1, S_MACEATK2_1, S_WNULL}, // Mace
  {am_noammo, 0, S_BEAKUP, S_BEAKDOWN, S_BEAKREADY, S_BEAKATK2_1, S_BEAKATK2_1, S_WNULL}, // Beak

  // Hexen weapons
  {am_noammo, 0, S_XPUNCHUP, S_XPUNCHDOWN, S_XPUNCHREADY, S_PUNCHATK1_1, S_PUNCHATK1_1, S_WNULL}, // Fighter - Punch
  {am_noammo, 0, S_CMACEUP, S_CMACEDOWN, S_CMACEREADY, S_CMACEATK_1, S_CMACEATK_1, S_WNULL},   // Cleric - Mace
  {am_noammo, 0, S_MWANDUP, S_MWANDDOWN, S_MWANDREADY, S_MWANDATK_1, S_MWANDATK_1, S_WNULL },  // Mage - Wand
  {am_noammo, 2, S_FAXEUP, S_FAXEDOWN, S_FAXEREADY, S_FAXEATK_1, S_FAXEATK_1, S_WNULL},        // Fighter - Axe
  {am_mana1,  1, S_CSTAFFUP, S_CSTAFFDOWN, S_CSTAFFREADY, S_CSTAFFATK_1, S_CSTAFFATK_1, S_WNULL}, // Cleric - Serpent Staff
  {am_mana1,  3, S_CONEUP, S_CONEDOWN, S_CONEREADY, S_CONEATK1_1, S_CONEATK1_3, S_WNULL},         // Mage - Cone of shards
  {am_noammo, 3, S_FHAMMERUP, S_FHAMMERDOWN, S_FHAMMERREADY, S_FHAMMERATK_1, S_FHAMMERATK_1, S_WNULL}, // Fighter - Hammer
  {am_mana2,  4, S_CFLAMEUP, S_CFLAMEDOWN, S_CFLAMEREADY1, S_CFLAMEATK_1, S_CFLAMEATK_1, S_WNULL}, // Cleric - Flame Strike
  {am_mana2,  5, S_MLIGHTNINGUP, S_MLIGHTNINGDOWN, S_MLIGHTNINGREADY, S_MLIGHTNINGATK_1, S_MLIGHTNINGATK_1, S_WNULL}, // Mage - Lightning
  {am_manaboth, 14, S_FSWORDUP, S_FSWORDDOWN, S_FSWORDREADY, S_FSWORDATK_1, S_FSWORDATK_1, S_WNULL}, // Fighter - Rune Sword
  {am_manaboth, 18, S_CHOLYUP, S_CHOLYDOWN, S_CHOLYREADY, S_CHOLYATK_1, S_CHOLYATK_1, S_WNULL},      // Cleric - Holy Symbol
  {am_manaboth, 15, S_MSTAFFUP, S_MSTAFFDOWN, S_MSTAFFREADY, S_MSTAFFATK_1, S_MSTAFFATK_1, S_WNULL}, // Mage - Staff
  {am_noammo, 0, S_SNOUTUP, S_SNOUTDOWN, S_SNOUTREADY, S_SNOUTATK1, S_SNOUTATK1, S_WNULL} // Pig - Snout
};


// inventory_t implementation

inventory_t::inventory_t()
{
  type = count = 0;
}

inventory_t::inventory_t(byte t, byte c)
{
  type = t;
  count = c;
}
