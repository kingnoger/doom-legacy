// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.18  2005/04/01 18:03:08  smite-meister
// fix
//
// Revision 1.17  2005/04/01 14:47:46  smite-meister
// dehacked works
//
// Revision 1.14  2004/12/19 23:43:20  smite-meister
// more BEX support
//
// Revision 1.13  2004/12/08 16:49:05  segabor
// Missing devparm reference added
//
// Revision 1.12  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.10  2004/03/28 15:16:15  smite-meister
// Texture cache.
//
// Revision 1.9  2004/01/10 16:03:00  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.8  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.6  2003/03/23 14:24:14  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.5  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.4  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
// Revision 1.3  2002/12/23 23:20:57  smite-meister
// WAD2+WAD3 support added!
//
// Revision 1.1.1.1  2002/11/16 14:18:38  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.9  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.6  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief DeHackEd and BEX support

#include <stdarg.h>
#include <ctype.h>

#include "dehacked.h"
#include "parser.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "dstrings.h"
#include "d_items.h"
#include "info.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"

#include "a_functions.h" // action function prototypes


dehacked_t DEH; // one global instance

static char   **savesprnames;
static actionf_p1 *d_actions;
static actionf_p2 *w_actions;


//========================================================================
//     BEX mnemonics
//========================================================================

// [CODEPTR] mnemonics
struct dactor_mnemonic_t
{
  char       *name;
  actionf_p1  ptr;
};

struct weapon_mnemonic_t
{
  char       *name;
  actionf_p2  ptr;
};

weapon_mnemonic_t BEX_WeaponMnemonics[] = 
{
#define WEAPON(x) {#x, A_ ## x},
#define DACTOR(x)
#include "a_functions.h"
  {"NULL", NULL},
  {NULL, NULL}
};

dactor_mnemonic_t BEX_DActorMnemonics[] = 
{
#define WEAPON(x)
#define DACTOR(x) {#x, A_ ## x},
#include "a_functions.h"
  {"NULL", NULL},
  {NULL, NULL}
};

// THING bit flag mnemonics
struct flag_mnemonic_t
{
  char *name;
  int original;
  int flag;
};

flag_mnemonic_t BEX_FlagMnemonics[32] =
{
  {"SPECIAL",         0x0001, MF_SPECIAL},   // Call TouchSpecialThing when touched.
  {"SOLID",           0x0002, MF_SOLID},     // Blocks
  {"SHOOTABLE",       0x0004, MF_SHOOTABLE}, // Can be hit
  {"NOSECTOR",        0x0008, MF_NOSECTOR},  // Don't link to sector (invisible but touchable)
  {"NOBLOCKMAP",      0x0010, MF_NOBLOCKMAP},// Don't link to blockmap (inert but visible)
  {"AMBUSH",          0x0020, MF_AMBUSH},    // Not to be activated by sound, deaf monster.
  {"JUSTHIT",         0x0040, 0}, // almost useless
  {"JUSTATTACKED",    0x0080, 0}, // even more useless
  {"SPAWNCEILING",    0x0100, MF_SPAWNCEILING}, // Spawned hanging from the ceiling
  {"NOGRAVITY",       0x0200, MF_NOGRAVITY}, // Does not feel gravity
  {"DROPOFF",         0x0400, MF_DROPOFF},   // Can jump/drop from high places
  {"PICKUP",          0x0800, MF_PICKUP},    // Can/will pick up items. (players) // useless?
  {"NOCLIP",          0x1000, MF_NOCLIPLINE | MF_NOCLIPTHING}, // Does not clip against lines or Actors.
  {"SLIDE",           0x2000, 0}, // completely unused
  {"FLOAT",           0x4000, MF_FLOAT},     // Active floater, can move freely in air (cacodemons etc.)
  {"TELEPORT",        0x8000, 0}, // completely unused
  {"MISSILE",     0x00010000, MF_MISSILE},   // Missile. Don't hit same species, explode on block.
  {"DROPPED",     0x00020000, MF_DROPPED},   // Dropped by a monster
  {"SHADOW",      0x00040000, MF_SHADOW},    // Partial invisibility (spectre). Makes targeting harder.
  {"NOBLOOD",     0x00080000, MF_NOBLOOD},   // Does not bleed when shot (furniture)
  {"CORPSE",      0x00100000, MF_CORPSE},    // Acts like a corpse, falls down stairs etc.
  {"INFLOAT",     0x00200000, 0}, // useless?
  {"COUNTKILL",   0x00400000, MF_COUNTKILL}, // On kill, count towards intermission kill total.
  {"COUNTITEM",   0x00800000, MF_COUNTITEM}, // On pickup, count towards intermission item total.
  {"SKULLFLY",    0x01000000, 0}, // useless?
  {"NOTDMATCH",   0x02000000, MF_NOTDMATCH}, // Not spawned in DM (keycards etc.)
  {"TRANSLATION", 0x04000000, 0},
  {"UNUSED1",     0x08000000, 0},
  {"UNUSED2",     0x10000000, 0},
  {"UNUSED3",     0x20000000, 0},
  {"UNUSED4",     0x40000000, 0},
  {"TRANSLUCENT", 0x80000000, 0}
};


flag_mnemonic_t BEX_Flag2Mnemonics[32] =
{
  {"LOGRAV",           0x0001, MF2_LOGRAV},       // Experiences only 1/8 gravity
  {"WINDTHRUST",       0x0002, MF2_WINDTHRUST},   // Is affected by wind
  {"FLOORBOUNCE",      0x0004, MF2_FLOORBOUNCE},  // Bounces off the floor
  {"THRUGHOST",        0x0008, MF2_THRUGHOST},    // Will pass through ghosts (missile)
  {"FLY",              0x0010, 0}, // eflags, useless
  {"FOOTCLIP",         0x0020, MF2_FOOTCLIP},     // Feet may be be clipped
  {"SPAWNFLOAT",       0x0040, 0}, // into flags! TODO
  {"NOTELEPORT",       0x0080, MF2_NOTELEPORT},   // Does not teleport
  {"RIP",              0x0100, MF2_RIP},          // Rips through solid targets (missile)
  {"PUSHABLE",         0x0200, MF2_PUSHABLE},     // Can be pushed by other moving actors
  {"SLIDE",            0x0400, MF2_SLIDE},        // Slides against walls
  {"ONMOBJ",           0x0800, 0}, // eflags, useless
  {"PASSMOBJ",         0x1000, MF2_PASSMOBJ},     // Can move over/under other Actors 
  {"CANNOTPUSH",       0x2000, MF2_CANNOTPUSH},   // Cannot push other pushable actors
  {"FEETARECLIPPED",   0x4000, 0}, // made a variable, useless
  {"BOSS",             0x8000, MF2_BOSS},         // Is a major boss, not as easy to kill
  {"FIREDAMAGE",   0x00010000, MF2_FIREDAMAGE},   // Does fire damage
  {"NODMGTHRUST",  0x00020000, MF2_NODMGTHRUST},  // Does not thrust target when damaging        
  {"TELESTOMP",    0x00040000, MF2_TELESTOMP},    // Can telefrag another Actor
  {"FLOATBOB",     0x00080000, MF2_FLOATBOB},     // Bobs up and down in the air (item)
  {"DONTDRAW",     0x00100000, MF2_DONTDRAW},     // Invisible (does not generate a vissprite)
  // Hexen additions
  {"IMPACT",       0x00200000, MF2_IMPACT},       // Can activate SPAC_IMPACT
  {"PUSHWALL",     0x00400000, MF2_PUSHWALL},     // Can activate SPAC_PUSH
  {"MCROSS",       0x00800000, MF2_MCROSS},       // Can activate SPAC_MCROSS
  {"PCROSS",       0x01000000, MF2_PCROSS},       // Can activate SPAC_PCROSS
  {"CANTLEAVEFLOORPIC", 0x02000000, MF2_CANTLEAVEFLOORPIC},    // Stays within a certain floor texture
  {"NONSHOOTABLE", 0x04000000, MF2_NONSHOOTABLE}, // Transparent to MF_MISSILEs
  {"INVULNERABLE", 0x08000000, MF2_INVULNERABLE}, // Does not take damage
  {"DORMANT",      0x10000000, MF2_DORMANT},      // Cannot be damaged, is not noticed by seekers
  {"ICEDAMAGE",    0x20000000, MF2_ICEDAMAGE},    // Does ice damage
  {"SEEKERMISSILE", 0x40000000, MF2_SEEKERMISSILE}, // Is a seeker (for reflection)
  {"REFLECTIVE",   0x80000000, MF2_REFLECTIVE},   // Reflects missiles
};


struct string_mnemonic_t
{
  char *name;
  int   num;
};

#define BEX_STR(x) {#x, TXT_ ## x},
string_mnemonic_t BEX_StringMnemonics[] =
{
  BEX_STR(GOTARMOR)
  BEX_STR(GOTMEGA)
  BEX_STR(GOTHTHBONUS)
  BEX_STR(GOTARMBONUS)
  BEX_STR(GOTSTIM)
  BEX_STR(GOTMEDINEED)
  BEX_STR(GOTMEDIKIT)
  BEX_STR(GOTSUPER)
  BEX_STR(GOTINVUL)
  BEX_STR(GOTBERSERK)
  BEX_STR(GOTINVIS)
  BEX_STR(GOTSUIT)
  BEX_STR(GOTMAP)
  BEX_STR(GOTVISOR)
  BEX_STR(GOTMSPHERE)
  BEX_STR(GOTCLIP)
  BEX_STR(GOTCLIPBOX)
  BEX_STR(GOTROCKET)
  BEX_STR(GOTROCKBOX)
  BEX_STR(GOTCELL)
  BEX_STR(GOTCELLBOX)
  BEX_STR(GOTSHELLS)
  BEX_STR(GOTSHELLBOX)
  BEX_STR(GOTBACKPACK)
  BEX_STR(GOTBFG9000)
  BEX_STR(GOTCHAINGUN)
  BEX_STR(GOTCHAINSAW)
  BEX_STR(GOTLAUNCHER)
  BEX_STR(GOTPLASMA)
  BEX_STR(GOTSHOTGUN)
  BEX_STR(GOTSHOTGUN2)

  BEX_STR(PD_BLUEO)
  BEX_STR(PD_YELLOWO)
  BEX_STR(PD_REDO)
  BEX_STR(PD_BLUEK)
  BEX_STR(PD_YELLOWK)
  BEX_STR(PD_REDK)

  BEX_STR(CC_ZOMBIE)
  BEX_STR(CC_SHOTGUN)
  BEX_STR(CC_HEAVY)
  BEX_STR(CC_IMP)
  BEX_STR(CC_DEMON)
  BEX_STR(CC_LOST)
  BEX_STR(CC_CACO)
  BEX_STR(CC_HELL)
  BEX_STR(CC_BARON)
  BEX_STR(CC_ARACH)
  BEX_STR(CC_PAIN)
  BEX_STR(CC_REVEN)
  BEX_STR(CC_MANCU)
  BEX_STR(CC_ARCH)
  BEX_STR(CC_SPIDER)
  BEX_STR(CC_CYBER)
  BEX_STR(CC_HERO)

  // Boom messages.
  BEX_STR(PD_BLUEC)
  BEX_STR(PD_YELLOWC)
  BEX_STR(PD_REDC)
  BEX_STR(PD_BLUES)
  BEX_STR(PD_YELLOWS)
  BEX_STR(PD_REDS)
  BEX_STR(PD_ANY)
  BEX_STR(PD_ALL3)
  BEX_STR(PD_ALL6)

  {NULL, -1}
};
#undef BEX_STR



//========================================================================
//  The DeHackEd class
//========================================================================

enum
{
  NUM_DOOM_STATES = 967, // all states in Doom (with null)
  NUM_LEGACY_STATES = 10,
  NUM_HERETIC_STATES = 1205, // with null
  NUM_HEXEN_STATES = 2846, // with null

  NUM_DOOM_THINGS = 137,
  NUM_LEGACY_THINGS = 12,
  NUM_HERETIC_THINGS = 160,
  NUM_HEXEN_THINGS = 398,

  // Offsets for both things and states. These must remain in order (D < H < X)
  OFS_HERETIC  = 10000,
  OFS_HEXEN    = 20000,
};


// reads an index like H232 and converts it into a number (for internal use only)
static int ReadTableIndex(const char *p)
{
  if (isdigit(p[0]))
    return atoi(p);
  else switch (p[0])
    {
    case 'H':
      return OFS_HERETIC + atoi(p+1);

    case 'X':
      return OFS_HEXEN + atoi(p+1);

    default:
      return atoi(p+1);
    }
}


// Our mobj tables have no gaps!
static int ThingMap(int num)
{
  if (num < 0)
    goto err;

  if (num < OFS_HERETIC)
    {
      if (num < NUM_DOOM_THINGS)
	return MT_DOOM + num;
    }
  else if (num < OFS_HEXEN)
    {
      if (num - OFS_HERETIC < NUM_HERETIC_THINGS)
	return MT_HERETIC + num - OFS_HERETIC;
    }
  else if (num - OFS_HEXEN < NUM_HEXEN_THINGS)
    return MT_HEXEN + num - OFS_HEXEN;

 err:
  DEH.error("Thing %d doesn't exist\n", num+1);
  return MT_DEFAULT_THING;
}


// Our state tables have lots of gaps due to the mobj/weaponstate separation:(
// State number remapping: weapon states get negated numbers.
static int StateMap(int num)
{
  /// Describes one gap in our state table.
  struct stategap_t
  {
    statenum_t       gap_start;    // gap begins after this state
    weaponstatenum_t first_wstate; // the first weaponstate constituting the gap
  };

  int n;
  stategap_t *gap;

  if (num < 0)
    goto err;
  else if (num < OFS_HERETIC)
    {
      if (num < NUM_DOOM_STATES)
	{
	  stategap_t Doom_gaps[] =
	  {
	    {S_NULL, S_LIGHTDONE},
	    {S_DOOM_END, S_HLIGHTDONE},
	    {S_DOOM_END, S_HLIGHTDONE} // the last row must be duplicated...
	  };
	  // Doom:
	  // 0 null state
	  // 1-89 doom weapons
	  // 90-966 doom things
	  // (967-976 legacy additions) (cannot be accessed using DeHackEd)

	  n = sizeof(Doom_gaps)/sizeof(stategap_t) - 1;
	  gap = Doom_gaps;
	}
      else
	goto err;
    }
 else if (num < OFS_HEXEN)
    {
      if (num - OFS_HERETIC < NUM_HERETIC_STATES)
	{
	  num += S_HERETIC - OFS_HERETIC;

	  // Heretic: 9 separate groups of weapon states
 	  stategap_t Heretic_gaps[] =
	  {
	    {S_HTFOG13, S_HLIGHTDONE},
	    {S_STAFFPUFF2_6, S_BEAKREADY},
	    {S_WGNT, S_GAUNTLETREADY},
	    {S_BLSR, S_BLASTERREADY},
	    {S_WMCE, S_MACEREADY},
	    {S_WSKL, S_HORNRODREADY},
	    {S_RAINAIRXPLR4_3, S_GOLDWANDREADY},
	    {S_WPHX, S_PHOENIXREADY},
	    {S_WBOW, S_CRBOW1},
	    {S_HERETIC_END, S_XLIGHTDONE},
	    {S_HERETIC_END, S_XLIGHTDONE},
	  };

	  n = sizeof(Heretic_gaps)/sizeof(stategap_t) - 1;
	  gap = Heretic_gaps;
	}
      else
	goto err;
    }
  else if (num - OFS_HEXEN < NUM_HEXEN_STATES)
    {
      num += S_HEXEN - OFS_HEXEN;

      // Hexen: 13 separate groups of weapon states 
      stategap_t Hexen_gaps[] =
      {
	{S_TELESMOKE26, S_XLIGHTDONE},
	{S_AXE, S_FAXEREADY},
	{S_HAMM, S_FHAMMERREADY},
	{S_HAMMERPUFF5, S_FSWORDREADY},
	{S_FSWORD_FLAME10, S_CMACEREADY},
	{S_CSTAFF, S_CSTAFFREADY},
	{S_CFLAME8, S_CFLAMEREADY1},
	{S_CFLAME_MISSILE_X, S_CHOLYREADY},
	{S_HOLY_MISSILE_P5, S_MWANDREADY},
	{S_MW_LIGHTNING8, S_MLIGHTNINGREADY},
	{S_LIGHTNING_ZAP_X8, S_MSTAFFREADY},
	{S_MSTAFF3, S_SNOUTREADY},
	{S_COS3, S_CONEREADY},
	{S_HEXEN_END, NUMWEAPONSTATES},
	{S_HEXEN_END, NUMWEAPONSTATES},
      };

      n = sizeof(Hexen_gaps)/sizeof(stategap_t) - 1;
      gap = Hexen_gaps;
    }
  else
    goto err;

  for (int i=0; i<n; i++)
    {
      int len = gap[i+1].first_wstate - gap[i].first_wstate; // gap lenght
      if (num <= gap[i].gap_start)
	return num;
      else if (num <= gap[i].gap_start + len)
	return -(gap[i].first_wstate + num - gap[i].gap_start - 1);
      num -= len;
    }
  
  I_Error("DEH: should never arrive here!\n");

 err:
  DEH.error("Frame %d doesn't exist!\n", num);
  return S_DEFAULT_STATE;
}



dehacked_t::dehacked_t()
{
  loaded = false;
  num_errors = 0;

  idfa_armor = 200;
  idfa_armorfactor = 0.5;
  idkfa_armor = 200;
  idkfa_armorfactor = 0.5;
  god_health = 100;

  // TODO these are not yet used...
  max_health = 200;
  max_soul_health = 200;
}


void dehacked_t::error(char *first, ...)
{
  va_list argptr;

  char buf[1000];

  va_start(argptr, first);
  vsprintf(buf, first, argptr);
  va_end(argptr);

  CONS_Printf("DEH: %s", buf);
  num_errors++;
}


// A small hack for retrieving values following a '=' sign.
// Does not change the parser state.
int dehacked_t::FindValue()
{
  char *temp = p.Pointer(); // save the current location

  // find the first occurrence of str
  char *res = strstr(temp, "=");
  if (res)
    p.SetPointer(++res); // pass the = sign
  else
    {
      error("Missing equality sign!\n");
      return 0;
    }

  int value;
  if (!p.MustGetInt(&value))
    {
      error("No value found\n");
      value = 0;
    }

  p.SetPointer(temp); // and go back
  return value;
}


// Bigger HACK for retrieving special state numbers and mapping them.
// Does not change the parser state.
int dehacked_t::FindState()
{
  char *temp = p.Pointer(); // save the current location

  // find the first occurrence of "="
  char *res = strstr(temp, "=");
  if (res)
    p.SetPointer(++res); // pass the = sign
  else
    {
      error("Missing equality sign!\n");
      return 0;
    }

  res = p.GetToken(" \t");

  int value = 0;
  if (!res)
    {
      error("No value found\n");
    }
  else
    value = ReadTableIndex(res);

  p.SetPointer(temp); // restore parser

  return StateMap(value);
}


int dehacked_t::ReadFlags(flag_mnemonic_t *mnemonics)
{
  int i, value = 0;
  char *word = p.GetToken("=+| \t");

  // we allow bitwise-ORed combinations of BEX mnemonics and numeric values
  while (word)
    {
      if (isdigit(word[0]))
	{
	  // old-style numeric entry, just do the conversion
	  int temp = atoi(word);
	  for (i=0; i<32; i++)
	    if (temp & mnemonics[i].original)
	      value |= mnemonics[i].flag;

	  word = p.GetToken("+| \t"); // next token
	  continue;
	}

      // must be a mnemonic
      for (i=0; i<32; i++)
	if (!strcasecmp(word, mnemonics[i].name))
	  {
	    value |= mnemonics[i].flag;
	    if (mnemonics[i].flag == 0)
	      error("NOTE: Mnemonic %s has no in-game effect.\n", mnemonics[i].name);
	    break;
	  }

      if (i >= 32)
	{
	  error("Unknown bit mnemonic '%s'\n", word);
	  break;
	}
		
      word = p.GetToken("+| \t"); // next token
    }

  return value;
}




// Utility for setting codepointers / action functions.
// Accepts both numeric and BEX mnemonic references.
// The 'to' state must already be mapped.
static void SetAction(int to, const char *mnemonic)
{
  if (isdigit(mnemonic[1]))
    {
      // this must also handle strings like H111
      int from = StateMap(ReadTableIndex(mnemonic));
      if (to > 0)
	if (from > 0)
	  states[to].action = d_actions[from];
	else
	  DEH.error("Tried to use a weapon codepointer in a thing frame!\n");
      else
	if (from < 0)
	  weaponstates[-to].action = w_actions[-from];
	else
	  DEH.error("Tried to use a thing codepointer in a weapon frame!\n");

      return;
    }

  if (to > 0)
    {
      dactor_mnemonic_t *m;
      for (m = BEX_DActorMnemonics; m->name && strcasecmp(mnemonic, m->name); m++);
      if (!m->name)
	DEH.error("[CODEPTR]: Unknown mnemonic '%s'\n", mnemonic);
      states[to].action = m->ptr;
    }
  else
    {
      weapon_mnemonic_t *w;
      for (w = BEX_WeaponMnemonics; w->name && strcasecmp(mnemonic, w->name); w++);
      if (!w->name)
	DEH.error("[CODEPTR]: Unknown mnemonic '%s'\n", mnemonic);
      weaponstates[-to].action = w->ptr;
    }
}



//========================================================================
// Load a dehacked file format 6. I (BP) don't know other format
//========================================================================

/*
  Thing sample:
Thing 1 (Player)
ID # = 3232              -1,             // doomednum
Initial frame = 32       S_PLAY,         // spawnstate
Hit points = 3232        100,            // spawnhealth
First moving frame = 32  S_PLAY_RUN1,    // seestate
Alert sound = 32         sfx_None,       // seesound
Reaction time = 3232     0,              // reactiontime
Attack sound = 32        sfx_None,       // attacksound
Injury frame = 32        S_PLAY_PAIN,    // painstate
Pain chance = 3232       255,            // painchance
Pain sound = 32          sfx_plpain,     // painsound
Close attack frame = 32  S_NULL,         // meleestate
Far attack frame = 32    S_PLAY_ATK1,    // missilestate
Death frame = 32         S_PLAY_DIE1,    // deathstate
Exploding frame = 32     S_PLAY_XDIE1,   // xdeathstate
Death sound = 32         sfx_pldeth,     // deathsound
Speed = 3232             0,              // speed
Width = 211812352        16*FRACUNIT,    // radius
Height = 211812352       56*FRACUNIT,    // height
Mass = 3232              100,            // mass
Missile damage = 3232    0,              // damage
Action sound = 32        sfx_None,       // activesound
Bits = 3232              MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH,
Respawn frame = 32       S_NULL          // raisestate
*/

void dehacked_t::Read_Thing(const char *str)
{
  int t = ThingMap(ReadTableIndex(str) - 1);

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" \t"); // get first word

      // special handling for mnemonics
      if (!strcasecmp(word, "Bits"))
	{
	  mobjinfo[t].flags = ReadFlags(BEX_FlagMnemonics); continue;
	}
      else if (!strcasecmp(word,"Bits2"))
	{
	  mobjinfo[t].flags2 = ReadFlags(BEX_Flag2Mnemonics); continue;
	}

      int value = FindValue();

      // set the value in appropriate field
      if (!strcasecmp(word, "ID"))            mobjinfo[t].doomednum    = value;
      else if (!strcasecmp(word,"Hit"))       mobjinfo[t].spawnhealth  = value;
      else if (!strcasecmp(word,"Alert"))     mobjinfo[t].seesound     = value;
      else if (!strcasecmp(word,"Reaction"))  mobjinfo[t].reactiontime = value;
      else if (!strcasecmp(word,"Attack"))    mobjinfo[t].attacksound  = value;
      else if (!strcasecmp(word,"Pain"))
	{
	  word = p.GetToken(" \t");
	  if (!strcasecmp(word,"chance"))     mobjinfo[t].painchance = value;
	  else if (!strcasecmp(word,"sound")) mobjinfo[t].painsound  = value;
	}
      else if (!strcasecmp(word,"Death"))
	{
	  word = p.GetToken(" \t");
	  if (!strcasecmp(word,"frame"))
	    {
	      value = FindState();
	      if (value < 0)
		{
		  error("Thing %s : Weapon states must not be used with Things!\n", str);
		  continue;
		}
	      mobjinfo[t].deathstate  = statenum_t(value);
	    }
	  else if (!strcasecmp(word,"sound")) mobjinfo[t].deathsound  = value;
	}
      else if (!strcasecmp(word,"Speed"))     mobjinfo[t].speed       = float(value)/FRACUNIT;
      else if (!strcasecmp(word,"Width"))     mobjinfo[t].radius      = value;
      else if (!strcasecmp(word,"Height"))    mobjinfo[t].height      = value;
      else if (!strcasecmp(word,"Mass"))      mobjinfo[t].mass        = value;
      else if (!strcasecmp(word,"Missile"))   mobjinfo[t].damage      = value;
      else if (!strcasecmp(word,"Action"))    mobjinfo[t].activesound = value;
      else
	{
	  value = FindState();
	  if (value < 0)
	    {
	      error("Thing %s : Weapon states must not be used with Things!\n", str);
	      continue;
	    }

	  if (!strcasecmp(word,"Initial"))        mobjinfo[t].spawnstate   = statenum_t(value);
	  else if (!strcasecmp(word,"First"))     mobjinfo[t].seestate     = statenum_t(value);
	  else if (!strcasecmp(word,"Injury"))    mobjinfo[t].painstate    = statenum_t(value);
	  else if (!strcasecmp(word,"Close"))     mobjinfo[t].meleestate   = statenum_t(value);
	  else if (!strcasecmp(word,"Far"))       mobjinfo[t].missilestate = statenum_t(value);
	  else if (!strcasecmp(word,"Exploding")) mobjinfo[t].xdeathstate  = statenum_t(value);
	  else if (!strcasecmp(word,"Respawn"))   mobjinfo[t].raisestate   = statenum_t(value);
	  else error("Thing %s : Unknown field '%s'\n", str, word);
	}
    }
}


/*
  Frame sample:
Sprite number = 10
Sprite subnumber = 32968
Duration = 200
Next frame = 200
Codep = 111 // Legacy addition
*/

void dehacked_t::Read_Frame(const char *str)
{
  int s = StateMap(ReadTableIndex(str));

  if (s == 0)
    {
      DEH.error("You must not modify frame 0.\n");
      return;
    }

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();

      // set the value in appropriate field
      char *word = p.GetToken(" \t");

      if (s > 0)
	{
	  state_t *state = &states[s];
	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" \t");
	      if (!strcasecmp(word,"number"))         state->sprite = spritenum_t(value);
	      else if (!strcasecmp(word,"subnumber")) state->frame  = value;
	    }
	  else if (!strcasecmp(word,"Duration"))      state->tics      = value;
	  else if (!strcasecmp(word,"Next"))
	    {
	      value = FindState();
	      if (value < 0)
		{
		  error("Frame %s: A mobj frame cannot be followed by a weapon frame!\n", str);
		  continue;
		}

	      state->nextstate = statenum_t(value);
	    }
	  else if (!strcasecmp(word,"Codep"))
	    {
	      word = p.GetToken(" \t=");
	      SetAction(s, word);
	    }
	  else error("Frame %s : Unknown field '%s'\n", str, word);
	}
      else
	{
	  weaponstate_t *wstate = &weaponstates[-s];
	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" \t");
	      if (!strcasecmp(word,"number"))         wstate->sprite = spritenum_t(value);
	      else if (!strcasecmp(word,"subnumber")) wstate->frame  = value;
	    }
	  else if (!strcasecmp(word,"Duration"))      wstate->tics      = value;
	  else if (!strcasecmp(word,"Next"))
	    {
	      value = FindState();
	      if (value > 0)
		{
		  error("Weaponframe %s: A weapon frame cannot be followed by a mobj frame!\n", str);
		  continue;
		}

	      wstate->nextstate = weaponstatenum_t(value);
	    }
	  else if (!strcasecmp(word,"Codep"))
	    {
	      word = p.GetToken(" \t=");
	      SetAction(s, word);
	    }
	  else error("Weaponframe %s : Unknown field '%s'\n", str, word);
	}
    }
}


// deprecated
void dehacked_t::Read_Sound(int num)
{
  error("Sound command currently unsupported\n");
  return;

  if (num >= NUMSFX || num < 0)
    {
      error("Sound %d doesn't exist\n");
      return;
    }
  
  while (p.NewLine(false))
    {
      // TODO dehacked sound commands
      /*

      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" ");

	if (!strcasecmp(word,"Offset"))
	  {
	    value -= 150360;
	    if (value<=64)
	      value/=8;
	    else if (value<=260)
	      value=(value+4)/8;
	    else value=(value+8)/8;

	    if (value>=-1 && value < NUMSFX-1)
	      strcpy(S_sfx[num].lumpname, savesfxnames[value+1]);
	    else
	      error("Sound %d : offset out of bound\n",num);
	  }
	else if (!strcasecmp(word,"Zero/One"))
	  S_sfx[num].multiplicity = value;
	else if (!strcasecmp(word,"Value"))
	  S_sfx[num].priority   =value;
	else
	  error("Sound %d : unknown word '%s'\n",num,word);
      */
    }
}


// this part of dehacked really sucks, but it still partly supported
void dehacked_t::Read_Text(int len1, int len2)
{
  char s[2001];
  int i;

  // dehacked text
  // It is hard to change all the text in Doom.
  // Here we implement only the vital, easy things.
  // Yes, "text" can change some tables like music, sound and sprite names
  if (len1+len2 > 2000)
    {
      error("Text too long\n");
      return;
    }
  
  if (p.ReadChars(s, len1 + len2) != len1 + len2)
    {
      error("Text reading failed\n");
      return;
    }

  // sound table (not supported anymore)
  /*
    for (i=0;i<NUMSFX;i++)
      if (!strncmp(savesfxname[i],s,len1))
      {
        strncpy(S_sfx[i].lumpname,&(s[len1]),len2);
        S_sfx[i].lumpname[len2]='\0';
        return;
      }
  */

  // sprite table TODO we should enforce length 4 for the names...
  for (i=0; i<NUMSPRITES; i++)
    if (!strncmp(savesprnames[i], s, len1))
      {
        strncpy(sprnames[i], &s[len1], len2);
        sprnames[i][len2] = '\0';
        return;
      }


  // music table
  for (i=1; i<NUMMUSIC; i++)
    if (MusicNames[i] && !strncmp(MusicNames[i], s, len1))
      {
        strncpy(MusicNames[i], &(s[len1]), len2);
        MusicNames[i][len2]='\0';
        return;
      }

  // text table
  for (i=0; i<NUMTEXT; i++)
    {
      int temp = strlen(text[i]);
      if (temp == len1 && !strncmp(text[i], s, len1))
	{
	  // FIXME odd. If I remove this comment, DEH crashes with a segfault.
	  // can't you write to static tables??
	  //if (temp < len2)  // increase size of the text
	    {
	      text[i] = (char *)malloc(len2 + 1);
	      if (!text[i])
		I_Error("Read_Text : Out of memory");
	    }

	  strncpy(text[i], s + len1, len2);
	  text[i][len2] = '\0';
	  return;
	}
    }

  s[len1] = '\0';
  error("Text not changed :%s\n", s);
}


/*
  Weapon sample:
Ammo type = 2
TODO ammopershoot
Deselect frame = 11
Select frame = 12
Bobbing frame = 13
Shooting frame = 17
Firing frame = 10
*/
void dehacked_t::Read_Weapon(int num)
{
  if (num >= NUMWEAPONS || num < 0)
    {
      error("Weapon %d doesn't exist\n", num);
      return;
    }

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" \t");

      if (!strcasecmp(word,"Ammo"))
	wpnlev1info[num].ammo = ammotype_t(value);
      else
	{
	  value = -FindState();
	  if (value < 0)
	    {
	      error("Weapon %d : Thing states must not be used with weapons!\n", num);
	      continue;
	    }

	  if (!strcasecmp(word,"Deselect"))      wpnlev1info[num].upstate    = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Select"))   wpnlev1info[num].downstate  = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Bobbing"))  wpnlev1info[num].readystate = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Shooting"))
	    wpnlev1info[num].atkstate = wpnlev1info[num].holdatkstate        = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Firing"))   wpnlev1info[num].flashstate = weaponstatenum_t(value);
	  else error("Weapon %d : unknown word '%s'\n", num, word);
	}
    }
}


/*
  Ammo sample:
Max ammo = 400
Per ammo = 40
*/
void dehacked_t::Read_Ammo(int num)
{
  if (num >= NUMAMMO || num < 0)
    {
      error("Ammo %d doesn't exist\n", num);
      return;
    }

  // support only Doom ammo with this command
  const mobjtype_t clips[4][4] =
  {
    {MT_CLIP, MT_SHELL, MT_CELL, MT_ROCKETAMMO},
    {MT_AMMOBOX, MT_SHELLBOX, MT_CELLPACK, MT_ROCKETBOX},
    {MT_CHAINGUN, MT_SHOTGUN, MT_PLASMA, MT_ROCKETLAUNCH},
    {MT_NONE, MT_SUPERSHOTGUN, MT_BFG, MT_NONE}
  };

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" \t");

      if (!strcasecmp(word,"Max"))
	maxammo1[num] = value;
      else if (!strcasecmp(word,"Per"))
	{
	  mobjinfo[clips[0][num]].spawnhealth = value;
	  mobjinfo[clips[1][num]].spawnhealth = 5*value;
	}
      else if (!strcasecmp(word,"Perweapon"))
	{
	  mobjinfo[clips[2][num]].spawnhealth = value;
	  if (clips[3][num] > MT_NONE)
	    mobjinfo[clips[3][num]].spawnhealth = value;
	}
      else
	error("Ammo %d : unknown word '%s'\n", num, word);
    }
}


// miscellaneous one-liners
void dehacked_t::Read_Misc()
{
  extern int MaxArmor[];
  const double ac = 1.0/6;

  char *word1, *word2;
  int value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      value = FindValue();
      word1 = p.GetToken(" \t");
      word2 = p.GetToken(" \t");

      if (!strcasecmp(word1,"Initial"))
	{
	  if (!strcasecmp(word2,"Health"))       mobjinfo[MT_PLAYER].spawnhealth = value;
	  else if (!strcasecmp(word2,"Bullets")) pawndata[0].bammo = value;
	}
      else if (!strcasecmp(word1,"Max"))
	{
	  if (!strcasecmp(word2,"Health"))          max_health = value;
	  else if (!strcasecmp(word2,"Armor"))      MaxArmor[0] = value;
	  else if (!strcasecmp(word2,"Soulsphere")) max_soul_health = value;
	}
      else if (!strcasecmp(word1,"Green"))
	{
	  mobjinfo[MT_GREENARMOR].spawnhealth = value*100;
	  mobjinfo[MT_GREENARMOR].speed = ac*(value+1);
	}
      else if (!strcasecmp(word1,"Blue"))
	{
	  mobjinfo[MT_BLUEARMOR].spawnhealth = value*100;
	  mobjinfo[MT_BLUEARMOR].speed = ac*(value+1);
	}
      else if (!strcasecmp(word1,"Soulsphere")) mobjinfo[MT_SOULSPHERE].spawnhealth = value;
      else if (!strcasecmp(word1,"Megasphere")) mobjinfo[MT_MEGA].spawnhealth = value;
      else if (!strcasecmp(word1,"God"))        god_health = value;
      else if (!strcasecmp(word1,"IDFA"))
	{
	  word2 = p.GetToken(" \t");
	  if (!strcasecmp(word2,"="))          idfa_armor = value;
	  else if (!strcasecmp(word2,"Class")) idfa_armorfactor = ac*(value+1);
	}
      else if (!strcasecmp(word1,"IDKFA"))
	{
	  word2 = p.GetToken(" \t");
	  if (!strcasecmp(word2,"="))          idkfa_armor = value;
	  else if (!strcasecmp(word2,"Class")) idkfa_armorfactor = ac*(value+1);
	}
      else if (!strcasecmp(word1,"BFG")) wpnlev1info[wp_bfg].ammopershoot = value;
      else if (!strcasecmp(word1,"Monsters"))      {} // TODO
      else error("Misc : unknown command '%s'\n", word1);
    }
}

extern byte cheat_mus_seq[];
extern byte cheat_choppers_seq[];
extern byte cheat_god_seq[];
extern byte cheat_ammo_seq[];
extern byte cheat_ammonokey_seq[];
extern byte cheat_noclip_seq[];
extern byte cheat_commercial_noclip_seq[];
extern byte cheat_powerup_seq[7][10];
extern byte cheat_clev_seq[];
extern byte cheat_mypos_seq[];
extern byte cheat_amap_seq[];

static void change_cheat_code(byte *old, byte *n)
{
  for ( ; *n && *n != 0xff; old++, n++)
    if (*old == 1 || *old == 0xff) // no more place in the cheat
      {
	DEH.error("Cheat too long\n");
	return;
      }
    else
      *old = *n;

  // newcheatseq < oldcheat
  n = old;
  // search special cheat with 100
  for ( ; *n != 0xff; n++)
    if (*n == 1)
      {
	*old++ = 1;
	*old++ = 0;
	*old++ = 0;
	break;
      }
  *old = 0xff;

  return;
}


// deprecated
void dehacked_t::Read_Cheat()
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      // FIXME how does this work?
      p.GetToken("=");
      byte *value = (byte *)p.GetToken(" \n"); // skip the space
      p.GetToken(" \n");         // finish the string
      char *word = p.GetToken(" \t");

      if (!strcasecmp(word,"Change"))        change_cheat_code(cheat_mus_seq,value);
      else if (!strcasecmp(word,"Chainsaw")) change_cheat_code(cheat_choppers_seq,value);
      else if (!strcasecmp(word,"God"))      change_cheat_code(cheat_god_seq,value);
      else if (!strcasecmp(word,"Ammo"))
	{
	  word = p.GetToken(" \t");
	  if (word && !strcasecmp(word,"&")) change_cheat_code(cheat_ammo_seq,value);
	  else                           change_cheat_code(cheat_ammonokey_seq,value);
	}
      else if (!strcasecmp(word,"No"))
	{
	  word = p.GetToken(" \t");
	  if (word)
	    word = p.GetToken(" \t");

	  if (word && !strcasecmp(word,"1")) change_cheat_code(cheat_noclip_seq,value);
	  else                           change_cheat_code(cheat_commercial_noclip_seq,value);
	}
      /* // FIXME! this could be replaced with a possibility to create new cheat codes (easy)
      else if (!strcasecmp(word,"Invincibility")) change_cheat_code(cheat_powerup_seq[0],value);
      else if (!strcasecmp(word,"Berserk"))       change_cheat_code(cheat_powerup_seq[1],value);
      else if (!strcasecmp(word,"Invisibility"))  change_cheat_code(cheat_powerup_seq[2],value);
      else if (!strcasecmp(word,"Radiation"))     change_cheat_code(cheat_powerup_seq[3],value);
      else if (!strcasecmp(word,"Auto-map"))      change_cheat_code(cheat_powerup_seq[4],value);
      else if (!strcasecmp(word,"Lite-Amp"))      change_cheat_code(cheat_powerup_seq[5],value);
      else if (!strcasecmp(word,"BEHOLD"))        change_cheat_code(cheat_powerup_seq[6],value);
      */
      else if (!strcasecmp(word,"Level"))         change_cheat_code(cheat_clev_seq,value);
      else if (!strcasecmp(word,"Player"))        change_cheat_code(cheat_mypos_seq,value);
      else if (!strcasecmp(word,"Map"))           change_cheat_code(cheat_amap_seq,value);
      else error("Cheat : unknown word '%s'\n",word);
    }
}


// Parses the BEX [CODEPTR] section
void dehacked_t::Read_CODEPTR()
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" \t");
      if (strcasecmp(word, "Frame"))
	{
	  error("[CODEPTR]: Unknown command '%s'\n", word);
	  continue;
	}

      // the "to" state number
      int s = StateMap(ReadTableIndex(p.GetToken(" \t=")));

      if (s == 0)
	{
	  DEH.error("You must not modify frame 0.\n");
	  continue;
	}

      SetAction(s, p.GetToken(" \t="));
    }
}


// Parses the BEX [STRINGS] section
void dehacked_t::Read_STRINGS()
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" \t");
      string_mnemonic_t *m;
      for (m = BEX_StringMnemonics; m->name && strcasecmp(word, m->name); m++);
      if (!m->name)
	{	  
	  error("[STRINGS]: Unknown mnemonic '%s'\n", word);
	  continue;
	}

      // FIXME backslash-continued lines...
      char *newtext = p.GetToken("=");
      text[m->num] = Z_StrDup(newtext);
    }
}



// dehacked command parser
enum DEH_cmd_t
{
  DEH_Thing,
  DEH_Frame,
  DEH_Pointer,
  DEH_Sound,
  DEH_Sprite,
  DEH_Text,
  DEH_Weapon,
  DEH_Ammo,
  DEH_Misc,
  DEH_Cheat,
  DEH_Doom,
  DEH_Patch,
  DEH_CODEPTR,
  DEH_PARS,
  DEH_STRINGS,
  DEH_NUM
};

static const char *DEH_cmds[DEH_NUM + 1] =
{
  "thing", "frame", "pointer", "sound", "sprite", "text", "weapon", "ammo", "misc", "cheat", "doom", "patch",
  "[CODEPTR]", "[PARS]", "[STRINGS]", NULL
};


// Parse a DeHackEd lump
// (there is special trick for converting .deh files into WAD lumps)
bool dehacked_t::LoadDehackedLump(const char *buf, int len)
{
  if (!p.Open(buf, len))
    return false;

  p.DeleteChars('\r'); // annoying cr's.

  num_errors = 0;

  // original values
  d_actions = (actionf_p1 *)Z_Malloc(NUMSTATES * sizeof(actionf_p1), PU_STATIC, NULL);
  w_actions = (actionf_p2 *)Z_Malloc(NUMWEAPONSTATES * sizeof(actionf_p2), PU_STATIC, NULL);
  savesprnames = (char **)Z_Malloc(NUMSPRITES * sizeof(char *), PU_STATIC, NULL);

  int i;

  // save original values
  for (i=0; i<NUMSTATES; i++)
    d_actions[i] = states[i].action;
  for (i=0; i<NUMWEAPONSTATES; i++)
    w_actions[i] = weaponstates[i].action;

  for (i=0; i<NUMSPRITES; i++)
    savesprnames[i] = sprnames[i];

  p.RemoveComments('#', true);
  while (p.NewLine())
    {
      char *word1, *word2;

      if ((word1 = p.GetToken(" \t")))
	{
	  word2 = p.GetToken(" \t");
	  if (word2)
	    i = atoi(word2);
	  else
	    {
	      i = 0;
	      //error("Warning: missing argument for '%s'\n", word1);
	    }

	  switch (P_MatchString(word1, DEH_cmds))
	    {
	    case DEH_Thing:
	      Read_Thing(word2);
	      break;

	    case DEH_Frame:
	      Read_Frame(word2);
	      break;

	    case DEH_Pointer:
	      /*
		Syntax:
		pointer uuu (frame xxx)
		codep frame = yyy
	      */
	      p.GetToken(" \t"); // get rid of "(frame"
	      if ((word1 = p.GetToken(")")))
		{
		  int s = atoi(word1);
		  if (p.NewLine())
		    {
		      p.GetToken(" \t");
		      p.GetToken(" \t");
		      word2 = p.GetToken(" \t=");
		      SetAction(s, word2);
		    }
		}
	      else
		error("Pointer %d : (Frame xxx) missing\n", i);
	      break;

	    case DEH_Sound:
	      Read_Sound(i);
	      break;

	    case DEH_Sprite:
	      if (i < NUMSPRITES && i >= 0)
		{
		  if (p.NewLine())
		    {
		      int k;
		      k = (FindValue() - 151328) / 8;
		      if (k >= 0 && k < NUMSPRITES)
			sprnames[i] = savesprnames[k];
		      else
			error("Sprite %i : offset out of bound\n", i);
		    }
		}
	      else
		error("Sprite %d doesn't exist\n", i);
	      break;

	    case DEH_Text:
	      if ((word1 = p.GetToken(" \t")))
		{
		  int j = atoi(word1);
		  Read_Text(i, j);
		}
	      else
		error("Text : missing second number\n");
	      break;

	    case DEH_Weapon:
	      Read_Weapon(i);
	      break;

	    case DEH_Ammo:
	      Read_Ammo(i);
	      break;

	    case DEH_Misc:
	      Read_Misc();
	      break;

	    case DEH_Cheat:
	      Read_Cheat();
	      break;

	    case DEH_Doom:
	      i = FindValue();
	      if (i != 19)
		error("Warning : Patch from a different Doom version (%d), only version 1.9 is supported\n", i);
	      break;

	    case DEH_Patch:
	      if (word2 && !strcasecmp(word2, "format"))
		{
		  if (FindValue() != 6)
		    error("Warning : Patch format not supported");
		}
	      break;

	      // BEX stuff
	    case DEH_CODEPTR:
	      Read_CODEPTR();
	      break;
	    case DEH_PARS:
	      // TODO support PARS?
	      error("BEX [PARS] block currently unsupported.\n");
	      break;
	    case DEH_STRINGS:
	      Read_STRINGS();
	      break;
	    default:
	      error("Unknown command : %s\n", word1);
	    }
	}
      else
        error("No word in this line:\n%s\n", p.GetToken("\0"));
    }

  if (num_errors > 0)
    {
      CONS_Printf("DEH: %d warning(s).\n", num_errors);
      if (devparm)
	getchar();
    }

  loaded = true;
  p.Clear();

  return true;
}
