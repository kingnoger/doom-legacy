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
// Revision 1.9  2005/06/28 16:53:56  smite-meister
// item respawning cleaned up
//
// Revision 1.8  2004/10/31 22:30:53  smite-meister
// cleanup
//
// Revision 1.7  2004/03/28 15:16:12  smite-meister
// Texture cache.
//
// Revision 1.6  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.5  2003/05/11 21:23:49  smite-meister
// Hexen fixes
//
// Revision 1.4  2003/05/05 00:24:48  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.3  2003/04/19 17:38:46  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.2  2003/04/04 00:01:53  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.1.1.1  2002/11/16 14:17:50  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Globally defined strings in English.

#include "dstrings.h"

char *text[NUMTEXT] =
{
  "DOOM 2: Hell on Earth",
  "The Ultimate DOOM Startup",
  "DOOM Registered Startup",
  "DOOM Shareware Startup",

  "Development mode ON.\n",
  "press a key.",
  "press y or n.",
  "only the server can do a load net game!\n\npress a key.",
  "you can't quickload during a netgame!\n\npress a key.",
  "you haven't picked a quicksave slot yet!\n\npress a key.",
  "you can't save if you aren't playing!\n\npress a key.",
  "quicksave over your game named\n\n'%s'?\n\npress y or n.",
  "do you want to quickload the game named\n\n'%s'?\n\npress y or n.",
  "you can't start a new game\nwhile in a network game.\n\n",
  "are you sure? this skill level\nisn't even remotely fair.\n\npress y or n.",
  "this is the shareware version of doom.\n\nyou need to order the entire trilogy.\n\npress a key.",
  "Messages OFF",
  "Messages ON",
  "you can't end a netgame!\n\npress a key.",
  "are you sure you want to end the game?\n\npress y or n.",
  "%s\n\n(press y to quit to OS.)",

  "High detail",
  "Low detail",
  "Gamma correction OFF",
  "Gamma correction level 1",
  "Gamma correction level 2",
  "Gamma correction level 3",
  "Gamma correction level 4",
  "empty slot",
  "game saved.",

  "Picked up the armor.",
  "Picked up the MegaArmor!",
  "Picked up a health bonus.",
  "Picked up an armor bonus.",
  "Picked up a stimpack.",
  "Picked up a medikit that you REALLY need!",
  "Picked up a medikit.",
  "Supercharge!",
  "Invulnerability!",
  "Berserk!",
  "Partial Invisibility",
  "Radiation Shielding Suit",
  "Computer Area Map",
  "Light Amplification Visor",
  "MegaSphere!",

  "Picked up a clip.",
  "Picked up a box of bullets.",
  "Picked up a rocket.",
  "Picked up a box of rockets.",
  "Picked up an energy cell.",
  "Picked up an energy cell pack.",
  "Picked up 4 shotgun shells.",
  "Picked up a box of shotgun shells.",
  "Picked up a backpack full of ammo!",

  "You got the BFG9000!  Oh, yes.",
  "You got the chaingun!",
  "A chainsaw!  Find some meat!",
  "You got the rocket launcher!",
  "You got the plasma gun!",
  "You got the shotgun!",
  "You got the super shotgun!",

  "You need a blue key to activate this object",
  "You need a yellow key to activate this object",
  "You need a red key to activate this object",
  "You need a blue key to open this door",
  "You need a yellow key to open this door",
  "You need a red key to open this door",

  "I'm ready to kick butt!",
  "I'm OK.",
  "I'm not looking too good!",
  "Help!",
  "You suck!",
  "Next time, scumbag...",
  "Come here!",
  "I'll take care of it.",
  "Yes",
  "No",
  "You mumble to yourself",
  "Who's there?",
  "You scare yourself",
  "You start to rave",
  "You've lost it...",
  "[Message unsent]",
  "[Message Sent]",

  "Follow Mode ON",
  "Follow Mode OFF",
  "Grid ON",
  "Grid OFF",
  "Marked Spot",
  "All Marks Cleared",

  "Music Change",
  "IMPOSSIBLE SELECTION",
  "Degreelessness Mode On",
  "Degreelessness Mode Off",
  "Very Happy Ammo Added",
  "Ammo (no keys) Added",
  "No Clipping Mode ON",
  "No Clipping Mode OFF",
  "inVuln, Str, Inviso, Rad, Allmap, or Lite-amp",
  "Power-up Toggled",
  "... doesn't suck - GM",
  "Changing Level...",

  "ZOMBIEMAN",
  "SHOTGUN GUY",
  "HEAVY WEAPON DUDE",
  "IMP",
  "DEMON",
  "LOST SOUL",
  "CACODEMON",
  "HELL KNIGHT",
  "BARON OF HELL",
  "ARACHNOTRON",
  "PAIN ELEMENTAL",
  "REVENANT",
  "MANCUBUS",
  "ARCH-VILE",
  "THE SPIDER MASTERMIND",
  "THE CYBERDEMON",
  "OUR HERO",

  // DOOM1
  "are you sure you want to\nquit this great game?",
  "please don't leave, there's more\ndemons to toast!",
  "let's beat it -- this is turning\ninto a bloodbath!",
  "i wouldn't leave if i were you.\ndos is much worse.",
  "you're trying to say you like dos\nbetter than me, right?",
  "don't leave yet -- there's a\ndemon around that corner!",
  "ya know, next time you come in here\ni'm gonna toast ya.",
  "go ahead and leave. see if i care.",

  // QuitDOOM II messages
  "you want to quit?\nthen, thou hast lost an eighth!",
  "don't go now, there's a \ndimensional shambler waiting\nat the dos prompt!",
  "get outta here and go back\nto your boring programs.",
  "if i were your boss, i'd \n deathmatch ya in a minute!",
  "look, bud. you leave now\nand you forfeit your body count!",
  "just leave. when you come\nback, i'll be waiting with a bat.",
  "you're lucky i don't smack\nyou for thinking about leaving.",

  //SoM: 3/9/2000: Boom generic key messages:
  "You need a blue card to open this door",
  "You need a yellow card to open this door",
  "You need a red card to open this door",
  "You need a blue skull to open this door",
  "You need a yellow skull to open this door",
  "You need a red skull to open this door",
  "Any key will open this door",
  "You need all three keys to open this door",
  "You need all six keys to open this door",

  // heretic + hexen artifacts
  "RING OF INVINCIBILITY",
  "SHADOWSPHERE",
  "QUARTZ FLASK",
  "MYSTIC URN",
  "TOME OF POWER",
  "TORCH",
  "TIME BOMB OF THE ANCIENTS",
  "MORPH OVUM",
  "WINGS OF WRATH",
  "CHAOS DEVICE",

  "MYSTIC AMBIT INCANT",
  "DARK SERVANT",
  "PORKALATOR",
  "DISC OF REPULSION",
  "FLECHETTE",
  "BANISHMENT DEVICE",
  "BOOTS OF SPEED",
  "KRATER OF MIGHT",
  "DRAGONSKIN BRACERS",
  "ICON OF THE DEFENDER",

  // Puzzle artifacts
  "YORICK'S SKULL",
  "HEART OF D'SPARIL",
  "RUBY PLANET",
  "EMERALD PLANET",
  "EMERALD PLANET",
  "SAPPHIRE PLANET",
  "SAPPHIRE PLANET",
  "DAEMON CODEX",
  "LIBER OSCURA",
  "FLAME MASK",
  "GLAIVE SEAL",
  "HOLY RELIC",
  "SIGIL OF THE MAGUS",
  "CLOCK GEAR",
  "CLOCK GEAR",
  "CLOCK GEAR",
  "CLOCK GEAR",
  "YOU CANNOT USE THIS HERE",
                                       
  // heretic items
  "WAND CRYSTAL",                       
  "CRYSTAL GEODE",                      
  "MACE SPHERES",                       
  "PILE OF MACE SPHERES",               
  "ETHEREAL ARROWS",                    
  "QUIVER OF ETHEREAL ARROWS",          
  "CLAW ORB",                           
  "ENERGY ORB",                         
  "LESSER RUNES",                       
  "GREATER RUNES",                      
  "FLAME ORB",                          
  "INFERNO ORB",                        
                                       
  "FIREMACE",                           
  "ETHEREAL CROSSBOW",                 
  "DRAGON CLAW",                        
  "HELLSTAFF",                          
  "PHOENIX ROD",                        
  "GAUNTLETS OF THE NECROMANCER",       

  "CRYSTAL VIAL",
  "BAG OF HOLDING",
  "SILVER SHIELD",
  "ENCHANTED SHIELD",
  "MAP SCROLL",

  // cheats
  "GOD MODE ON",                        
  "GOD MODE OFF",                       
  "NO CLIPPING ON",                     
  "NO CLIPPING OFF",                    
  "ALL WEAPONS",                        
  "FLIGHT ON",                          
  "FLIGHT OFF",                         
  "POWER ON",                           
  "POWER OFF",                          
  "FULL HEALTH",                        
  "ALL KEYS",                           
  "SOUND DEBUG ON",                     
  "SOUND DEBUG OFF",
  "TICKER ON",                          
  "TICKER OFF",                         
  "CHOOSE AN ARTIFACT ( A - J )",       
  "HOW MANY ( 1 - 9 )",                 
  "YOU GOT IT",                         
  "BAD INPUT",                          
  "LEVEL WARP",                         
  "SCREENSHOT",                         
  "CHICKEN ON",                         
  "CHICKEN OFF",                        
  "MASSACRE",                           
  "TRYING TO CHEAT, EH?  NOW YOU DIE!",
  "CHEATER - YOU DON'T DESERVE WEAPONS",

  // death messages
  "%s suicides\n",
  "%s was telefraged by %s\n",
  "%s was beaten to a pulp by %s\n",
  "%s was gunned by %s\n",
  "%s was shot down by %s\n",
  "%s was machine-gunned by %s\n",
  "%s was catched up by %s's rocket\n",
  "%s was gibbed by %s's rocket\n",
  "%s eats %s's toaster\n",
  "%s enjoys %s's big fraggin' gun\n",
  "%s was divided up into little pieces by %s's chainsaw\n",
  "%s ate 2 loads of %s's buckshot\n",
  "%s was killed by %s\n",
  "%s dies in hellslime\n",
  "%s gulped a load of nukage\n",
  "%s dies in super hellslime/strobe hurt\n",
  "%s dies in special sector\n",
  "%s was barrel-fragged by %s\n",
  "%s dies from a barrel explosion\n",
  "%s was shot by a possessed\n",
  "%s was shot down by a shotguy\n",
  "%s was blasted by an Arch-vile\n",
  "%s was exploded by a Mancubus\n",
  "%s was punctured by a Chainguy\n",
  "%s was fried by an Imp\n",
  "%s was eviscerated by a Demon\n",
  "%s was mauled by a Shadow Demon\n",
  "%s was fried by a Caco-demon\n",
  "%s was slain by a Baron of Hell\n",
  "%s was smashed by a Revenant\n",
  "%s was slain by a Hell-Knight\n",
  "%s was killed by a Lost Soul\n",
  "%s was killed by a The Spider Mastermind\n",
  "%s was killed by a Arachnotron\n",
  "%s was crushed by the Cyber-demon\n",
  "%s was killed by a Pain Elemental\n",
  "%s was killed by a WolfSS\n",
  "%s died\n",

  // Hexen strings
  "BLUE MANA",
  "GREEN MANA",
  "COMBINED MANA",

  // Keys
  "STEEL KEY",
  "CAVE KEY",
  "AXE KEY",
  "FIRE KEY",
  "EMERALD KEY",
  "DUNGEON KEY",
  "SILVER KEY",
  "RUSTED KEY",
  "HORN KEY",
  "SWAMP KEY",
  "CASTLE KEY",
  "Picked up a blue keycard.",
  "Picked up a yellow keycard.",
  "Picked up a red keycard.",
  "Picked up a blue skull key.",
  "Picked up a yellow skull key.",
  "Picked up a red skull key.",

  // Items
  "MESH ARMOR",
  "FALCON SHIELD",
  "PLATINUM HELMET",
  "AMULET OF WARDING",

  // Weapons
  "TIMON'S AXE",
  "HAMMER OF RETRIBUTION",
  "QUIETUS ASSEMBLED",
  "SERPENT STAFF",
  "FIRESTORM",
  "WRAITHVERGE ASSEMBLED",
  "FROST SHARDS",
  "ARC OF DEATH",
  "BLOODSCOURGE ASSEMBLED",
  "SEGMENT OF QUIETUS",
  "SEGMENT OF WRAITHVERGE",
  "SEGMENT OF BLOODSCOURGE"
};
