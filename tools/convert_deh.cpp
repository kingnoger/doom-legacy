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
// Revision 1.4  2005/06/23 17:25:40  smite-meister
// map conversion command added
//
// Revision 1.3  2005/06/22 20:44:31  smite-meister
// alpha3 bugfixes
//
// Revision 1.2  2005/04/01 18:03:07  smite-meister
// fix
//
// Revision 1.1  2005/04/01 14:47:46  smite-meister
// dehacked works
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief DeHackEd distiller and converter from Legacy 1.43 to Legacy 2.0.

#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "dehacked.h"
#include "parser.h"

#include "g_actor.h"
#include "info.h"
#include "dstrings.h"


//===========================================
// Partial Parser class implementation
//===========================================

Parser::Parser()
{
  length = 0;
  ms = me = s = e = NULL;
}


Parser::~Parser()
{
  if (ms)
    free(ms);
}



// prepares a buffer for parsing
int Parser::Open(const char *buf, int len)
{
  if (len <= 0)
    return 0;

  length = len;

  ms = (char *)malloc(length + 1);
  memcpy(ms, buf, length);
  ms[length] = '\0'; // to make searching easy

  me = ms + length; // past-the-end pointer
  s = e = ms;

  return length;
}


// Removes all chars 'c' from the buffer and compactifies it.
void Parser::DeleteChars(char c)
{
  char *q = ms;
  for (char *p = ms; p < me; p++)
    if (*p != c)
      *q++ = *p;

  // and then fix the new limits
  me = q;
  length = me - ms;
  ms[length] = '\0'; // to make searching easy
}


// Clears the parser
void Parser::Clear()
{
  if (ms)
    free(ms);

  length = 0;
  ms = me = s = e = NULL;
}


// Replace all comments after 's' with whitespace.
// anything between the symbol // or ; and the next newline is a comment.
// TODO right now there is no way to escape these symbols!
void Parser::RemoveComments(char c, bool linestart)
{
  if (linestart)
    {
      // only interpret it as a comment if it is in the beginning of a line
      // (for DeHackEd and the stupid ID # thing!)
      for (char *p = s; p < me; p++)
	{
	  if (p[0] == '\n' && p[1] == c)
	    {
	      for (p++ ; p < me && *p != '\n'; p++)
		*p = ' ';
	      p--;
	    }
	}   
      return;
    }

  if (c == '/')
    for (char *p = s; p+1 < me; p++)
      {
	if (p[0] == '/' && p[1] == '/')
	  for ( ; p < me && *p != '\n'; p++)
	    *p = ' ';
      }
  else
    for (char *p = s; p < me; p++)
      {
	if (p[0] == c)
	  for ( ; p < me && *p != '\n'; p++)
	    *p = ' ';
      }    
}


// Reads at most n chars starting from the next line. Updates e.
// Returns the number of chars actually read.
int Parser::ReadChars(char *to, int n)
{
  int i;
  for (i = 0; e < me && i < n; e++, i++)
    to[i] = *e;

  to[i] = '\0';
  return i; 
}


// NOTE you must use this before using the line-oriented Parser methods.
// Seeks the next row ending with a newline.
// Returns false if the lump ends.
bool Parser::NewLine(bool pass_ws)
{
  // end passes any whitespace
  if (pass_ws)
    while (e < me && isspace(*e))
      e++;

  s = e; // this is where the next line starts

  // seek the next newline
  while (e < me && (*e != '\n'))
    e++;

  if (e < me)
    {
      *e = '\0';  // mark the line end
      e++; // past-the-end
      return true;
    }

  return false; // lump ends
}


// passes any contiguous whitespace
void Parser::PassWS()
{
  while (s < me && isspace(*s))
    s++;
}



// Tokenizer. Advances s.
char *Parser::GetToken(const char *delim)
{
  // Damnation! If strtok_r() only was part of ISO C!
  //return strtok_r(s, delim, &s);

  // pass initial delimiters
  int n = strlen(delim);
  for (; s < me && *s; s++)
    {
      int i;
      for (i=0; i<n; i++)
	if (*s == delim[i])
	  break;
      if (i >= n)
	break;
    }

  if (s == me || *s == '\0')
    return NULL;

  char *temp = strtok(s, delim);
  s += strlen(temp) + 1; // because strtok marks the token end with a NUL
  if (s >= e)
    s = e-1; // do not leave the line
  return temp;
}


// Gets one char, ignoring starting whitespace.
bool Parser::GetChar(char *to)
{
  PassWS();

  if (*s)
    {
      *to = *s;
      s++;
      return true;
    }

  return false;
}


// Get an integer, advance the 's' pointer
int Parser::GetInt()
{
  char *tail = NULL;
  int val = strtol(s, &tail, 0);
  if (tail == s)
    {
      printf("Expected an integer, got '%s'.\n", s);
      return 0;
    }

  s = tail;
  return val;
}


bool Parser::MustGetInt(int *to)
{
  char *tail = NULL;
  int val = strtol(s, &tail, 0);
  if (tail == s)
    return false;

  s = tail;
  *to = val;
  return true;
}



// Tries to match a string 'p' to a NULL-terminated array of strings.
// Returns the index of the first matching string, or -1 if there is no match.
int P_MatchString(const char *p, const char *strings[])
{
  for (int i=0; strings[i]; i++)
    if (!strcasecmp(p, strings[i]))
      return i;

  return -1;
}



//===========================================
//   The DeHackEd part
//===========================================


dehacked_t DEH; // one global instance

FILE *out = NULL;

enum
{
  NUM_DOOM_STATES = 967, // all states in Doom (with null)
  NUM_LEGACY_STATES = 9,
  s_gap1 = 563, // hblood * 3
  s_gap2 = 570, // s_play * 47
  NUM_HERETIC_STATES = 1205, // with null

  NUM_DOOM_THINGS = 137,
  NUM_LEGACY_THINGS = 11,
  t_gap1 = 55, // tfog, teleportman * 2
  t_gap2 = 94, // hblood * 1
  t_gap3 = 96, // hplayer * 1
  NUM_HERETIC_THINGS = 160,

  NUM_DOOM_WSTATES = 87, // weapon states in Doom
};


bool ExpandThingNum(int num)
{
  int t = num-1; // begin at 0

  // t is zero-based
  if (t < NUM_DOOM_THINGS)
    fprintf(out, "Thing %d", t+1);
  else if ((t -= NUM_DOOM_THINGS) < NUM_LEGACY_THINGS)
    fprintf(out, "Thing L%d", t+1);
  else if ((t -= NUM_LEGACY_THINGS) < t_gap1)
    fprintf(out, "Thing H%d", t+1);
  else if ((t += 2) < t_gap2)
    fprintf(out, "Thing H%d", t+1);
  else if ((t += 1) < t_gap3)
    fprintf(out, "Thing H%d", t+1);
  else if ((t += 1) < NUM_HERETIC_THINGS)
    fprintf(out, "Thing H%d", t+1);
  else
    {
      DEH.error("Thing %d doesn't exist!\n", num);
      fprintf(out, "Thing -1");
      return false;
    }

  return true;
}


const char *ExpandStateNum(int s)
{
  static char text[100];

  // s is zero-based
  if (s < NUM_DOOM_STATES)
    sprintf(text, "%d", s);
  else if ((s -= NUM_DOOM_STATES) < NUM_LEGACY_STATES)
    sprintf(text, "L%d", s);
  else if ((s -= NUM_LEGACY_STATES - 1) < s_gap1) // HNULL
    sprintf(text, "H%d", s);
  else if ((s += 3) < s_gap2) // HBLOOD
    sprintf(text, "H%d", s);
  else if ((s += 47) < NUM_HERETIC_STATES)// HPLAY
    sprintf(text, "H%d", s);
  else
    {
      DEH.error("State %d doesn't exist!\n", s);
      return "-1";
    }

  return text;
}

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


flag_mnemonic_t BEX_Flag2Mnemonics[32] =
{
  {"LOGRAV",           0x0001, MF2_LOGRAV},       // Experiences only 1/8 gravity
  {"WINDTHRUST",       0x0002, MF2_WINDTHRUST},   // Is affected by wind
  {"FLOORBOUNCE",      0x0004, MF2_FLOORBOUNCE},  // Bounces off the floor
  {"THRUGHOST",        0x0008, MF2_THRUGHOST},    // Will pass through ghosts (missile)
  {"FLY",              0x0010, 0}, // eflags, useless
  {"FOOTCLIP",         0x0020, MF2_FOOTCLIP},     // Feet may be be clipped
  {"SPAWNFLOAT",       0x0040, 0x08000000}, // FIXME TEMP
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

  printf("DEH: %s", buf);
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



int dehacked_t::ReadFlags(flag_mnemonic_t *mnemonics)
{
  int i, value = 0;
  char *word = p.GetToken("=+| \t");

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

  if (value == 0)
    fputs("0", out);
  else for (i=0; i<32; i++)
    if (value & mnemonics[i].flag)
      fprintf(out, "%s ", mnemonics[i].name);

  fputs("\n", out);
  return value;
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
  int num = atoi(str);

  ExpandThingNum(num);
  // preserve the comment
  fprintf(out, " %s\n", p.Pointer());

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" "); // get first word

      // special handling for mnemonics
      if (!strcasecmp(word, "Bits"))
	{
	  fprintf(out, "Bits = ");
	  ReadFlags(BEX_FlagMnemonics);
	  continue;
	}
      else if (!strcasecmp(word,"Bits2"))
	{
	  fprintf(out, "Bits2 = ");
	  ReadFlags(BEX_Flag2Mnemonics);
	  continue;
	}

      int value = FindValue();

      // set the value in appropriate field
      if (!strcasecmp(word, "ID"))            fprintf(out, "ID # = %d\n", value);
      else if (!strcasecmp(word,"Hit"))       fprintf(out, "Hit points = %d\n", value);
      else if (!strcasecmp(word,"Alert"))     fprintf(out, "Alert sound = %d\n", value);
      else if (!strcasecmp(word,"Reaction"))  fprintf(out, "Reaction time = %d\n", value);
      else if (!strcasecmp(word,"Attack"))    fprintf(out, "Attack sound = %d\n", value);
      else if (!strcasecmp(word,"Pain"))
	{
	  word = p.GetToken(" ");
	  if (!strcasecmp(word,"chance"))     fprintf(out, "Pain chance = %d\n", value);
	  else if (!strcasecmp(word,"sound")) fprintf(out, "Pain sound = %d\n", value);
	}
      else if (!strcasecmp(word,"Death"))
	{
	  word = p.GetToken(" ");
	  if (!strcasecmp(word,"frame"))
	    {
	      fprintf(out, "Death frame = %s\n", ExpandStateNum(value));
	    }
	  else if (!strcasecmp(word,"sound")) fprintf(out, "Death sound = %d\n", value);
	}
      else if (!strcasecmp(word,"Speed"))     fprintf(out, "Speed = %d\n", value);
      else if (!strcasecmp(word,"Width"))     fprintf(out, "Width = %d\n", value);
      else if (!strcasecmp(word,"Height"))    fprintf(out, "Height = %d\n", value);
      else if (!strcasecmp(word,"Mass"))      fprintf(out, "Mass = %d\n", value);
      else if (!strcasecmp(word,"Missile"))   fprintf(out, "Missile damage = %d\n", value);
      else if (!strcasecmp(word,"Action"))    fprintf(out, "Action sound = %d\n", value);
      else
	{
	  const char *s = ExpandStateNum(value);

	  if (!strcasecmp(word,"Initial"))        fprintf(out, "Initial frame = %s\n", s);
	  else if (!strcasecmp(word,"First"))     fprintf(out, "First moving frame = %s\n", s);
	  else if (!strcasecmp(word,"Injury"))    fprintf(out, "Injury frame = %s\n", s);
	  else if (!strcasecmp(word,"Close"))     fprintf(out, "Close attack frame = %s\n", s);
	  else if (!strcasecmp(word,"Far"))       fprintf(out, "Far attack frame = %s\n", s);
	  else if (!strcasecmp(word,"Exploding")) fprintf(out, "Exploding frame = %s\n", s);
	  else if (!strcasecmp(word,"Respawn"))   fprintf(out, "Respawn frame = %s\n", s);
	  else error("Thing %d : Unknown field '%s'\n", num, word);
	}
    }

  fputs("\n", out);
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
  int num = atoi(str);

  fprintf(out, "Frame %s %s\n", ExpandStateNum(num), p.Pointer());

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();

      // set the value in appropriate field
      char *word = p.GetToken(" ");

	  if (!strcasecmp(word,"Sprite"))
	    {
	      word = p.GetToken(" ");
	      if (!strcasecmp(word,"number"))         fprintf(out, "Sprite number = %d\n", value);
	      else if (!strcasecmp(word,"subnumber")) fprintf(out, "Sprite subnumber = %d\n", value);
	    }
	  else if (!strcasecmp(word,"Duration"))      fprintf(out, "Duration = %d\n", value);
	  else if (!strcasecmp(word,"Next"))
	    {
	      fprintf(out, "Next frame = %s\n", ExpandStateNum(value));
	    }
	  else if (!strcasecmp(word,"Codep"))
	    {
	      fprintf(out, "Codep = %s\n", ExpandStateNum(value));
	    }
	  else error("Frame %d : Unknown field '%s'\n", num, word);
    }

  fputs("\n", out);
}


// deprecated
void dehacked_t::Read_Sound(int num)
{
  error("Sound command currently unsupported.\n");
  return;
}


// this part of dehacked really sucks, but it still partly supported
void dehacked_t::Read_Text(int len1, int len2)
{
  char s[2001];

  // FIXME dehacked text
  // it is hard to change all the text in doom
  // here i implement only vital things
  // yes, "text" can change some tables like music, sound and sprite names
  if (len1+len2 > 2000)
    {
      error("Text too long\n");
      return;
    }

  if (p.ReadChars(s, len1 + len2) != len1 + len2)
    {
      error("Read failed\n");
      return;
    }

  fprintf(out, "Text %d %d\n%s\n\n", len1, len2, s);
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
  fprintf(out, "Weapon %d %s\n", num, p.Pointer());

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" ");

      if (!strcasecmp(word,"Ammo"))
	fprintf(out, "Ammo type = %d\n", value);
      else
	{
	  const char *p = ExpandStateNum(value);

	  if (!strcasecmp(word,"Deselect"))      fprintf(out, "Deselect frame = %s\n", p);
	  else if (!strcasecmp(word,"Select"))   fprintf(out, "Select frame = %s\n", p);
	  else if (!strcasecmp(word,"Bobbing"))  fprintf(out, "Bobbing frame = %s\n", p);
	  else if (!strcasecmp(word,"Shooting")) fprintf(out, "Shooting frame = %s\n", p);
	  else if (!strcasecmp(word,"Firing"))   fprintf(out, "Firing frame = %s\n", p);
	  else error("Weapon %d : unknown word '%s'\n", num, word);
	}
    }
  fputs("\n", out);
}


/*
  Ammo sample:
Max ammo = 400
Per ammo = 40
*/
void dehacked_t::Read_Ammo(int num)
{
  fprintf(out, "Ammo %d %s\n", num, p.Pointer());

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = FindValue();
      char *word = p.GetToken(" ");

      if (!strcasecmp(word,"Max"))
	fprintf(out, "Max ammo = %d\n", value);
      else if (!strcasecmp(word,"Per"))
	fprintf(out, "Per ammo = %d\n", value);
      /*
      else if (!strcasecmp(word,"Perweapon"))
	{
	  mobjinfo[clips[2][num]].spawnhealth = value;
	  if (clips[3][num] > MT_NONE)
	    mobjinfo[clips[3][num]].spawnhealth = value;
	}
      */
      else
	error("Ammo %d : unknown word '%s'\n", num, word);
    }
  fputs("\n", out);
}


// miscellaneous one-liners
void dehacked_t::Read_Misc()
{
  error("Misc command currently unsupported.\n");
  return;

  /*
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
      word1 = p.GetToken(" ");
      word2 = p.GetToken(" ");

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
	  word2 = p.GetToken(" ");
	  if (!strcasecmp(word2,"="))          idfa_armor = value;
	  else if (!strcasecmp(word2,"Class")) idfa_armorfactor = ac*(value+1);
	}
      else if (!strcasecmp(word1,"IDKFA"))
	{
	  word2 = p.GetToken(" ");
	  if (!strcasecmp(word2,"="))          idkfa_armor = value;
	  else if (!strcasecmp(word2,"Class")) idkfa_armorfactor = ac*(value+1);
	}
      else if (!strcasecmp(word1,"BFG")) wpnlev1info[wp_bfg].ammopershoot = value;
      else if (!strcasecmp(word1,"Monsters"))      {} // TODO
      else error("Misc : unknown command '%s'\n", word1);
    }
  */
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

/*
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
}
*/


// deprecated
void dehacked_t::Read_Cheat()
{
  error("Cheat command currently unsupported.\n");
  return;

  /*
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
       // FIXME! this could be replaced with a possibility to create new cheat codes (easy)
      else if (!strcasecmp(word,"Invincibility")) change_cheat_code(cheat_powerup_seq[0],value);
      else if (!strcasecmp(word,"Berserk"))       change_cheat_code(cheat_powerup_seq[1],value);
      else if (!strcasecmp(word,"Invisibility"))  change_cheat_code(cheat_powerup_seq[2],value);
      else if (!strcasecmp(word,"Radiation"))     change_cheat_code(cheat_powerup_seq[3],value);
      else if (!strcasecmp(word,"Auto-map"))      change_cheat_code(cheat_powerup_seq[4],value);
      else if (!strcasecmp(word,"Lite-Amp"))      change_cheat_code(cheat_powerup_seq[5],value);
      else if (!strcasecmp(word,"BEHOLD"))        change_cheat_code(cheat_powerup_seq[6],value);

      else if (!strcasecmp(word,"Level"))         change_cheat_code(cheat_clev_seq,value);
      else if (!strcasecmp(word,"Player"))        change_cheat_code(cheat_mypos_seq,value);
      else if (!strcasecmp(word,"Map"))           change_cheat_code(cheat_amap_seq,value);
      else error("Cheat : unknown word '%s'\n",word);
    }
  */
}


// Parses the BEX [CODEPTR] section
void dehacked_t::Read_CODEPTR()
{
  fprintf(out, "[CODEPTR]\n");

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
     
      fprintf(out, "Frame %s = %s\n", ExpandStateNum(s), word); 
    }

  fputs("\n", out);
}


// Parses the BEX [STRINGS] section
void dehacked_t::Read_STRINGS()
{
  fprintf(out, "[STRINGS]\n");

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
      char *newtext = p.GetToken("=");
      fprintf(out, "%s =%s\n", word, newtext);
    }

  fputs("\n", out);
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

  int i;

  p.RemoveComments('#', true);
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
	      p.GetToken(" "); // get rid of "(frame"
	      if ((word1 = p.GetToken(")")))
		{
		  int s = atoi(word1);
		  fprintf(out, "Pointer %d (frame %d)\n", i, s);
		  if (p.NewLine())
		    {
		      p.GetToken(" ");
		      p.GetToken(" ");
		      word2 = p.GetToken(" =");
		      fprintf(out, "Codep frame = %s\n\n", word2);
		    }
		}
	      else
		error("Pointer %d : (Frame xxx) missing\n", i);
	      break;

	    case DEH_Sound:
	      Read_Sound(i);
	      break;

	    case DEH_Sprite:
	      fprintf(out, "Sprite %d\n", i);
	      error("Sprite command currently unsupported.\n");
	      if (p.NewLine())
		{
		  // FIXME
		  //int k = (FindValue() - 151328) / 8;
		}
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
	      i = FindValue();
	      if (i != 19)
		error("Warning : Patch from a different Doom version (%d), only version 1.9 is supported\n", i);
	      fprintf(out, "Doom version = %d\n", i);
	      break;

	    case DEH_Patch:
	      if (word2 && !strcasecmp(word2, "format"))
		{
		  if ((i = FindValue()) != 6)
		    error("Warning : Patch format not supported");
		  fprintf(out, "Patch format = %d\n\n", i);
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
      printf("DEH: %d warning(s).\n", num_errors);
    }

  loaded = true;
  p.Clear();

  return true;
}


int FIL_ReadFile(const char *name, byte **buffer)
{
  struct stat fileinfo;

  int handle = open(name, O_RDONLY | O_BINARY, 0666);

  if (handle == -1)
    return 0;

  if (fstat(handle, &fileinfo) == -1)
    return 0;

  int length = fileinfo.st_size;
  byte *buf = (byte *)malloc(length+1);

  int count = read(handle, buf, length);
  close(handle);

  if (count < length)
    return 0;

  //Fab:26-04-98:append 0 byte for script text files
  buf[length] = 0;

  *buffer = buf;
  return length;
}


int main(int argc, char *argv[])
{
  if (argc != 3)
    {
      printf("This program converts DeHackEd files written for Doom Legacy versions 1.43 or earlier\n"
	     "to a format that is compatible with Doom Legacy 2.0.\n"
	     "Only those DeHackEd files need to be converted that use Heretic resources.\n"
	     "Usage: convert_deh old.deh new.deh\n");
      return -1;
    }

  int len = 0;
  byte *infile = NULL;
  if ((len = FIL_ReadFile(argv[1], &infile)) == 0)
    {
      printf("Could not open the input file '%s'.\n", argv[1]);
      return -1;
    }

  out = fopen(argv[2], "wb");
  if (!out)
    {
      printf("Could not open the output file '%s'.\n", argv[2]);
      return -1;
    }

  DEH.LoadDehackedLump((char *)infile, len);
  free(infile);
  fclose(out);

  return 0;
}
