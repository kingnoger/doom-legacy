// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.13  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.12  2001/06/30 15:06:01  bpereira
// fixed wronf next level name in intermission
//
// Revision 1.11  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.10  2001/02/10 12:27:13  bpereira
// no message
//
// Revision 1.9  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.8  2000/11/04 16:23:42  bpereira
// no message
//
// Revision 1.7  2000/11/03 13:15:13  hurdler
// Some debug comments, please verify this and change what is needed!
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
  {"SLIDE",           0x2000, 0}, // slides along walls, TODO could be useful
  {"FLOAT",           0x4000, MF_FLOAT},     // Active floater, can move freely in air (cacodemons etc.)
  {"TELEPORT",        0x8000, 0}, // completely unused
  {"MISSILE",     0x00010000, MF_MISSILE},   // Missile. Don't hit same species, explode on block.
  {"DROPPED",     0x00020000, MF_DROPPED},   // Dropped by a monster
  {"SHADOW",      0x00040000, MF_SHADOW},    // Partial invisibility (spectre). Makes targeting harder.
  {"NOBLOOD",     0x00080000, MF_NOBLOOD},   // Does not bleed when shot (furniture)
  {"CORPSE",      0x00100000, MF_CORPSE},    // Acts like a corpse, falls down stairs etc.
  {"INFLOAT",     0x00200000, 0}, // almost useless
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

// FIXME NOTE these mnemonics are NOT final!
flag_mnemonic_t BEX_Flag2Mnemonics[32] =
{
  // physical properties
  {"LOGRAV",       0x0001, MF2_LOGRAV},    // Experiences only 1/8 gravity
  {"WINDTHRUST",   0x0002, MF2_WINDTHRUST},    // Is affected by wind
  {"FLOORBOUNCE",  0x0004, MF2_FLOORBOUNCE},    // Bounces off the floor
  {"SLIDE",        0x0008, MF2_SLIDE},    // Slides against walls
  {"PUSHABLE",     0x0010, MF2_PUSHABLE},    // Can be pushed by other moving actors
  {"CANNOTPUSH",   0x0020, MF2_CANNOTPUSH},    // Cannot push other pushable actors
  // game mechanics
  {"FLOATBOB",     0x0040, MF2_FLOATBOB},    // Bobs up and down in the air (item)
  {"THRUGHOST",    0x0080, MF2_THRUGHOST},    // Will pass through ghosts (missile)
  {"RIP",          0x0100, MF2_RIP},    // Rips through solid targets (missile)
  {"PASSMOBJ",     0x0200, MF2_PASSMOBJ},    // Can move over/under other Actors 
  {"NOTELEPORT",   0x0400, MF2_NOTELEPORT},    // Does not teleport
  {"NONSHOOTABLE", 0x0800, MF2_NONSHOOTABLE},    // Transparent to MF_MISSILEs
  {"INVULNERABLE", 0x1000, MF2_INVULNERABLE},    // Does not take damage
  {"DORMANT",      0x2000, MF2_DORMANT},    // Cannot be damaged, is not noticed by seekers
  {"CANTLEAVEFLOORPIC", 0x4000, MF2_CANTLEAVEFLOORPIC},    // Stays within a certain floor texture
  {"BOSS",         0x8000, MF2_BOSS},    // Is a major boss, not as easy to kill
  {"SEEKER",       0x00010000, MF2_SEEKERMISSILE},    // Is a seeker (for reflection)
  {"REFLECTIVE",   0x00020000, MF2_REFLECTIVE},    // Reflects missiles
  // rendering
  {"FOOTCLIP",     0x00040000, MF2_FOOTCLIP},    // Feet may be be clipped
  {"DONTDRAW",     0x00080000, MF2_DONTDRAW},    // Invisible (does not generate a vissprite)
  // giving hurt
  {"FIREDAMAGE",   0x00100000, MF2_FIREDAMAGE},    // Does fire damage
  {"ICEDAMAGE",    0x00200000, MF2_ICEDAMAGE},    // Does ice damage
  {"NODMGTHRUST",  0x00400000, MF2_NODMGTHRUST},    // Does not thrust target when damaging        
  {"TELESTOMP",    0x00800000, MF2_TELESTOMP},    // Can telefrag another Actor
  {"UNUSED1",      0x01000000, 0}, // unused
  {"UNUSED1",      0x02000000, 0}, // unused
  {"UNUSED1",      0x04000000, 0}, // unused
  {"UNUSED1",      0x08000000, 0}, // unused
  // activation
  {"IMPACT",       0x10000000, MF2_IMPACT},    // Can activate SPAC_IMPACT
  {"PUSHWALL",     0x20000000, MF2_PUSHWALL},    // Can activate SPAC_PUSH
  {"MCROSS",       0x40000000, MF2_MCROSS},    // Can activate SPAC_MCROSS
  {"PCROSS",       0x80000000, MF2_PCROSS},    // Can activate SPAC_PCROSS
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

dehacked_t::dehacked_t()
{
  loaded = false;
  num_errors = 0;

  // FIXME most of these are not really used
  idfa_armor = 200;
  idfa_armor_class = 2;
  idkfa_armor = 200;
  idkfa_armor_class = 2;
  god_health = 100;

  initial_health = 100;
  initial_bullets = 50;
  max_health = 200;
  maxsoul = 200;

  green_armor_class = 1;
  blue_armor_class = 2;
  soul_health = 200;
  mega_health = 200;
}


void dehacked_t::error(char *first, ...)
{
  va_list argptr;

  char buf[1000];

  va_start(argptr, first);
  vsprintf(buf, first, argptr);
  va_end(argptr);

  CONS_Printf("%s\n", buf);
  num_errors++;
}


// a small hack for retrieving values following a '=' sign
int dehacked_t::FindValue()
{
  int value;
  char *temp = p.Pointer(); // save the current location
  p.GoToNext("=");
  if (!p.MustGetInt(&value))
    {
      error("No value found\n");
      value = 0;
    }
  p.SetPointer(temp); // and go back
  return value;
}


int dehacked_t::ReadFlags(flag_mnemonic_t *mnemonics)
{
  int i, value = 0;
  char *word = p.GetToken("= \t\r");

  if (word && isdigit(word[0]))
    {
      // old-style numeric entry, just do the conversion
      int temp = atoi(word);
      for (i=0; i<32; i++)
	if (temp & mnemonics[i].original)
	  value |= mnemonics[i].flag;

      return value;
    }

  // must be a BEX mnemonic, in which case no numeric entries are allowed
  while (word)
    {
      if (isdigit(word[0]))
	{
	  error("You may not combine BEX bit mnemonics with numbers.\n");
	  return value;
	}

      for (i=0; i<32; i++)
	if (!strcasecmp(word, mnemonics[i].name))
	  {
	    value |= mnemonics[i].flag;
	    CONS_Printf("flag value 0x%08lX, %s\n", mnemonics[i].flag, word);
	    break;
	  }

      if (i >= 32)
	{
	  error("Unknown bit mnemonic '%s'\n", word);
	  break;
	}
		
      word = p.GetToken("+| \t\r"); // next token
    }

  return value;
}


// state number remapping: weapon states get negated numbers
static int StateMap(int num)
{
  // 0 null state
  // 1-89 doom weapons
  // 90-967 doom things
  // 968-975 legacy additions (smoke and splash)

  if (num < 0 || num > 975) // FIXME upper limit
    {
      DEH.error("Frame %d doesn't exist!\n", num);
      return S_TNT1; // FIXME temp
    }

  if (num == 0)
    {
      DEH.error("You must not modify frame 0.\n");
      return S_TNT1; // FIXME temp
    }

  if (num <= 89)
    return -num;
  else
    return num - 89;
}


// Utility for setting codepointers / action functions.
// Accepts both numeric and BEX mnemonic references.
static void SetAction(int to, const char *mnemonic)
{
  to = StateMap(to);

  if (mnemonic[0] >= '0' && mnemonic[0] <= '9' )
    {
      int from = StateMap(strtol(mnemonic, NULL, 0));
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

void dehacked_t::Read_Thing(int num)
{
  num--; // begin at 0 not 1;
  if (num >= NUMMOBJTYPES || num < 0)
    {
      error("Thing %d doesn't exist\n", num);
      return;
    }

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" "); // get first word

      // special handling for mnemonics
      if (!strcasecmp(word, "Bits"))
	{
	  mobjinfo[num].flags = ReadFlags(BEX_FlagMnemonics); continue;
	}
      else if (!strcasecmp(word,"Bits2"))
	{
	  mobjinfo[num].flags2 = ReadFlags(BEX_Flag2Mnemonics); continue;
	}

      int value = FindValue();

      // set the value in appropriate field
      if (!strcasecmp(word, "ID"))            mobjinfo[num].doomednum    = value;
      else if (!strcasecmp(word,"Hit"))       mobjinfo[num].spawnhealth  = value;
      else if (!strcasecmp(word,"Alert"))     mobjinfo[num].seesound     = value;
      else if (!strcasecmp(word,"Reaction"))  mobjinfo[num].reactiontime = value;
      else if (!strcasecmp(word,"Attack"))    mobjinfo[num].attacksound  = value;
      else if (!strcasecmp(word,"Pain"))
	{
	  word = p.GetToken(" ");
	  if (!strcasecmp(word,"chance"))     mobjinfo[num].painchance = value;
	  else if (!strcasecmp(word,"sound")) mobjinfo[num].painsound  = value;
	}
      else if (!strcasecmp(word,"Death"))
	{
	  word = p.GetToken(" ");
	  if (!strcasecmp(word,"frame"))
	    {
	      value = StateMap(value);
	      if (value <= 0)
		{
		  error("Thing %d : Weapon states must not be used with Things!\n", num);
		  continue;
		}
	      mobjinfo[num].deathstate  = statenum_t(value);
	    }
	  else if (!strcasecmp(word,"sound")) mobjinfo[num].deathsound  = value;
	}
      else if (!strcasecmp(word,"Speed"))     mobjinfo[num].speed       = value;
      else if (!strcasecmp(word,"Width"))     mobjinfo[num].radius      = value;
      else if (!strcasecmp(word,"Height"))    mobjinfo[num].height      = value;
      else if (!strcasecmp(word,"Mass"))      mobjinfo[num].mass        = value;
      else if (!strcasecmp(word,"Missile"))   mobjinfo[num].damage      = value;
      else if (!strcasecmp(word,"Action"))    mobjinfo[num].activesound = value;
      else
	{
	  value = StateMap(value);
	  if (value <= 0)
	    {
	      error("Thing %d : Weapon states must not be used with Things!\n", num);
	      continue;
	    }

	  if (!strcasecmp(word,"Initial"))        mobjinfo[num].spawnstate   = statenum_t(value);
	  else if (!strcasecmp(word,"First"))     mobjinfo[num].seestate     = statenum_t(value);
	  else if (!strcasecmp(word,"Injury"))    mobjinfo[num].painstate    = statenum_t(value);
	  else if (!strcasecmp(word,"Close"))     mobjinfo[num].meleestate   = statenum_t(value);
	  else if (!strcasecmp(word,"Far"))       mobjinfo[num].missilestate = statenum_t(value);
	  else if (!strcasecmp(word,"Exploding")) mobjinfo[num].xdeathstate  = statenum_t(value);
	  else if (!strcasecmp(word,"Respawn"))   mobjinfo[num].raisestate   = statenum_t(value);
	  else error("Thing %d : Unknown field '%s'\n", num, word);
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

void dehacked_t::Read_Frame(int num)
{
  int s = StateMap(num);
  if (!s)
    return;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();

      // set the value in appropriate field
      char *word = p.GetToken(" ");

      if (s > 0)
	{
	  state_t *state = &states[s];
	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" ");
	      if (!strcasecmp(word,"number"))         state->sprite = spritenum_t(value);
	      else if (!strcasecmp(word,"subnumber")) state->frame  = value;
	    }
	  else if (!strcasecmp(word,"Duration"))      state->tics      = value;
	  else if (!strcasecmp(word,"Next"))          state->nextstate = statenum_t(value);
	  else if (!strcasecmp(word,"Codep"))
	    {
	      word = p.GetToken(" =");
	      SetAction(s, word);
	    }
	  else error("Frame %d : Unknown field '%s'\n", num, word);
	}
      else
	{
	  weaponstate_t *wstate = &weaponstates[-s];
	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" ");
	      if (!strcasecmp(word,"number"))         wstate->sprite = spritenum_t(value);
	      else if (!strcasecmp(word,"subnumber")) wstate->frame  = value;
	    }
	  else if (!strcasecmp(word,"Duration"))      wstate->tics      = value;
	  else if (!strcasecmp(word,"Next"))          wstate->nextstate = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Codep"))
	    {
	      word = p.GetToken(" =");
	      SetAction(s, word);
	    }
	  else error("Weaponframe %d : Unknown field '%s'\n", num, word);
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

  // FIXME dehacked text
  // it is hard to change all the text in doom
  // here i implement only vital things
  // yes, "text" can change some tables like music, sound and sprite names
  if (len1+len2 > 2000)
    {
      error("Text too long\n");
      return;
    }
  
  if (p.GetStringN(s, len1 + len2) != len1 + len2)
    {
      error("Read failed\n");
      return;
    }

  // sound table
  /*
    for (i=0;i<NUMSFX;i++)
      if (!strncmp(savesfxname[i],s,len1))
      {
        strncpy(S_sfx[i].lumpname,&(s[len1]),len2);
        S_sfx[i].lumpname[len2]='\0';
        return;
      }

  // sprite table
  for (i=0; i<NUMSPRITES; i++)
    if (!strncmp(savesprnames[i],s,len1))
      {
        strncpy(sprnames[i],&(s[len1]),len2);
        sprnames[i][len2]='\0';
        return;
      }
  */

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
	  if (temp < len2)  // increase size of the text
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

  // special text : text changed in Legacy but with dehacked support
  // I don't think this is necessary...
  /*
  for (i=SPECIALDEHACKED; i<NUMTEXT; i++)
    {
      int temp = strlen(text[i]);

      if (len1 > temp && strstr(s, text[i]))
       {
	 // remove space for center the text
	 char *t = &s[len1+len2-1];

           while(t[0]==' ') { t[0]='\0'; t--; }
           // skip the space
           while(s[len1]==' ') len1++;

           // remove version string identifier
           t=strstr(&(s[len1]),"v%i.%i");
           if (!t) {
              t=strstr(&(s[len1]),"%i.%i");
              if (!t) {
                 t=strstr(&(s[len1]),"%i");
                 if (!t) {
                      t=s+len1+strlen(&(s[len1]));
                 }
              }
           }
           t[0]='\0';
           len2=strlen(&s[len1]);

           if (strlen(text[i])<(unsigned)len2)         // incresse size of the text
           {
              text[i]=(char *)malloc(len2+1);
              if (text[i]==NULL)
                  I_Error("Read_Text : No More free Mem");
           }

           strncpy(text[i],&(s[len1]),len2);
           text[i][len2]='\0';
           return;
       }
    }
  */

  s[len1] = '\0';
  error("Text not changed :%s\n", s);
}


/*
  Weapon sample:
Ammo type = 2
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
      char *word = p.GetToken(" ");

      if (!strcasecmp(word,"Ammo"))
	wpnlev1info[num].ammo = ammotype_t(value);
      else
	{
	  value = -StateMap(value);
	  if (value <= 0)
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
      char *word = p.GetToken(" ");

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

  char *word1, *word2;
  int value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      value = FindValue();
      word1 = p.GetToken(" ");
      word2 = p.GetToken(" ");

      if (!strcasecmp(word1,"Initial"))
	{
	  if (!strcasecmp(word2,"Health"))          initial_health = value;
	  else if (!strcasecmp(word2,"Bullets"))    initial_bullets = value;
	}
      else if (!strcasecmp(word1,"Max"))
	{
	  if (!strcasecmp(word2,"Health"))          max_health = value;
	  else if (!strcasecmp(word2,"Armor"))      MaxArmor[0] = value;
	  else if (!strcasecmp(word2,"Soulsphere")) maxsoul = value;
	}
      else if (!strcasecmp(word1,"Green"))         green_armor_class = value;
      else if (!strcasecmp(word1,"Blue"))          blue_armor_class = value;
      else if (!strcasecmp(word1,"Soulsphere"))    soul_health = value;
      else if (!strcasecmp(word1,"Megasphere"))    mega_health = value;
      else if (!strcasecmp(word1,"God"))           god_health = value;
      else if (!strcasecmp(word1,"IDFA"))
	{
	  word2 = p.GetToken(" ");
	  if (!strcasecmp(word2,"="))               idfa_armor = value;
	  else if (!strcasecmp(word2,"Class"))      idfa_armor_class = value;
	}
      else if (!strcasecmp(word1,"IDKFA"))
	{
	  word2 = p.GetToken(" ");
	  if (!strcasecmp(word2,"="))               idkfa_armor = value;
	  else if (!strcasecmp(word2,"Class"))      idkfa_armor_class = value;
	}
      else if (!strcasecmp(word1,"BFG"))            wpnlev1info[wp_bfg].ammopershoot = value;
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
      char *word = p.GetToken(" ");

      if (!strcasecmp(word,"Change"))        change_cheat_code(cheat_mus_seq,value);
      else if (!strcasecmp(word,"Chainsaw")) change_cheat_code(cheat_choppers_seq,value);
      else if (!strcasecmp(word,"God"))      change_cheat_code(cheat_god_seq,value);
      else if (!strcasecmp(word,"Ammo"))
	{
	  word = p.GetToken(" ");
	  if (word && !strcasecmp(word,"&")) change_cheat_code(cheat_ammo_seq,value);
	  else                           change_cheat_code(cheat_ammonokey_seq,value);
	}
      else if (!strcasecmp(word,"No"))
	{
	  word = p.GetToken(" ");
	  if (word)
	    word = p.GetToken(" ");

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

      char *word = p.GetToken(" ");
      if (strcasecmp(word, "Frame"))
	{
	  error("[CODEPTR]: Unknown command '%s'\n", word);
	  continue;
	}

      int s = p.GetInt();
      word = p.GetToken(" =");
      SetAction(s, word);
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

      char *word = p.GetToken(" ");
      string_mnemonic_t *m;
      for (m = BEX_StringMnemonics; m->name && strcasecmp(word, m->name); m++);
      if (!m->name)
	{	  
	  error("[STRINGS]: Unknown mnemonic '%s'\n", word);
	  continue;
	}

      // FIXME backslash-continued lines...
      p.GetToken("=");
      text[m->num] = Z_StrDup(p.Pointer());
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

  p.RemoveComments('#');
  while (p.NewLine())
    {
      char *word1, *word2;

      if ((word1 = p.GetToken(" ")))
	{
	  word2 = p.GetToken(" ");
	  if (word2)
	    i = atoi(word2);
	  else
	    {
	      i = 0;
	      error("Warning: missing argument for '%s'\n", word1);
	    }

	  switch (P_MatchString(word1, DEH_cmds))
	    {
	    case DEH_Thing:
	      Read_Thing(i);
	      break;

	    case DEH_Frame:
	      Read_Frame(i);
	      break;

	    case DEH_Pointer:
	      /*
		Syntax:
		pointer uuu (frame xxx)
		codep frame = yyy
	      */
	      p.GetToken(" "); // get rid of "(frame"
	      if ((word1 = p.GetToken(")")))
		{
		  int s = atoi(word1);
		  if (p.NewLine())
		    {
		      p.GetToken(" ");
		      p.GetToken(" ");
		      word2 = p.GetToken(" =");
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
	      if ((word1 = p.GetToken(" ")))
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
	      p.NewLine();
	      i = FindValue();
	      if (i != 19)
		error("Warning : Patch from a different Doom version (%d), only version 1.9 is supported\n", i);
	      break;

	    case DEH_Patch:
	      word1 = p.GetToken(" ");
	      if (word1 && !strcasecmp(word1, "format"))
		{
		  p.NewLine();
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
      CONS_Printf("%d warning(s) in the dehacked file\n", num_errors);
      if (devparm)
	getchar();
    }

  loaded = true;
  p.Clear();

  return true;
}
