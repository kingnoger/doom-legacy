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
// Revision 1.13  2004/12/08 16:49:05  segabor
// Missing devparm reference added
//
// Revision 1.12  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.11  2004/09/24 11:34:00  smite-meister
// fix
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
// Revision 1.7  2003/04/08 09:46:07  smite-meister
// Bugfixes
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
// Revision 1.2  2002/12/16 22:19:37  smite-meister
// HUD fix
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
// Revision 1.5  2000/08/31 14:30:55  bpereira
// no message
//
// Revision 1.4  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.3  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief DeHackEd and BEX support

#include <stdarg.h>

#include "dehacked.h"
#include "parser.h"

#include "g_game.h"

#include "d_items.h"
#include "sounds.h"
#include "info.h"
#include "dstrings.h"

#include "w_wad.h"
#include "z_zone.h"

extern bool devparm;			//in d_main.cpp

dehacked_t DEH; // one global instance

static char   **savesprnames;
static actionf_p1 *d_actions;
static actionf_p2 *w_actions;


struct d_mnemonic_t
{
  char       *name;
  actionf_p1  ptr;
};

struct w_mnemonic_t
{
  char       *name;
  actionf_p2  ptr;
};

#include "a_functions.h" // prototypes

w_mnemonic_t BEX_Weapon_mnemonics[] = 
{
#define WEAPON(x) {#x, A_ ## x},
#define DACTOR(x)
#include "a_functions.h"
  {"NULL", NULL},
  {NULL, NULL}
};

d_mnemonic_t BEX_DActor_mnemonics[] = 
{
#define WEAPON(x)
#define DACTOR(x) {#x, A_ ## x},
#include "a_functions.h"
  {"NULL", NULL},
  {NULL, NULL}
};



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

  if (devparm)
    {
      char buf[1000];

      va_start(argptr, first);
      vsprintf(buf, first, argptr);
      va_end(argptr);

      CONS_Printf("%s\n",buf);
    }

  num_errors++;
}


// a small hack for retrieving values following a '=' sign
static int SearchValue(Parser &p)
{
  int value;
  char *temp = p.Pointer(); // save the current location
  p.GoToNext("=");
  if (!p.MustGetInt(&value))
    {
      DEH.error("No value found\n");
      value = 0;
    }
  p.SetPointer(temp); // and go back
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
      return 0;
    }

  if (num == 0)
    {
      DEH.error("You must not modify frame 0.\n");
      return 0;
    }

  if (num <= 89)
    return -num;
  else
    return num - 89;
}


// ========================================================================
// Load a dehacked file format 6. I (BP) don't know other format
// ========================================================================

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

static void Read_Thing(Parser &p, int num)
{
  num--; // begin at 0 not 1;
  if (num >= NUMMOBJTYPES || num < 0)
    {
      DEH.error("Thing %d doesn't exist\n", num);
      return;
    }

  char *word;
  int value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      value = SearchValue(p);

      // set the value in appropriate field
      word = p.GetToken(" ");
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
		  DEH.error("Thing %d : Weapon states must not be used with Things!\n", num);
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
      else if (!strcasecmp(word,"Bits"))      mobjinfo[num].flags       = value;
      else if (!strcasecmp(word,"Bits2"))     mobjinfo[num].flags2      = value;
      else
	{
	  value = StateMap(value);
	  if (value <= 0)
	    {
	      DEH.error("Thing %d : Weapon states must not be used with Things!\n", num);
	      continue;
	    }

	  if (!strcasecmp(word,"Initial"))        mobjinfo[num].spawnstate   = statenum_t(value);
	  else if (!strcasecmp(word,"First"))     mobjinfo[num].seestate     = statenum_t(value);
	  else if (!strcasecmp(word,"Injury"))    mobjinfo[num].painstate    = statenum_t(value);
	  else if (!strcasecmp(word,"Close"))     mobjinfo[num].meleestate   = statenum_t(value);
	  else if (!strcasecmp(word,"Far"))       mobjinfo[num].missilestate = statenum_t(value);
	  else if (!strcasecmp(word,"Exploding")) mobjinfo[num].xdeathstate  = statenum_t(value);
	  else if (!strcasecmp(word,"Respawn"))   mobjinfo[num].raisestate   = statenum_t(value);
	  else DEH.error("Thing %d : Unknown field '%s'\n", num, word);
	}
    }
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
      d_mnemonic_t *m;
      for (m = BEX_DActor_mnemonics; m->name && strcasecmp(mnemonic, m->name); m++);
      if (!m->name)
	DEH.error("[CODEPTR]: Unknown mnemonic '%s'\n", mnemonic);
      states[to].action = m->ptr;
    }
  else
    {
      w_mnemonic_t *w;
      for (w = BEX_Weapon_mnemonics; w->name && strcasecmp(mnemonic, w->name); w++);
      if (!w->name)
	DEH.error("[CODEPTR]: Unknown mnemonic '%s'\n", mnemonic);
      weaponstates[-to].action = w->ptr;
    }
}


// parses the [CODEPTR] item
static void Read_CODEPTR(Parser &p)
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      char *word = p.GetToken(" ");
      if (strcasecmp(word, "Frame"))
	{
	  DEH.error("[CODEPTR]: Unknown command '%s'\n", word);
	  continue;
	}

      int s = p.GetInt();
      word = p.GetToken(" =");
      SetAction(s, word);
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

static void Read_Frame(Parser &p, int num)
{
  int s = StateMap(num);
  if (!s)
    return;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = SearchValue(p);

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
	  else DEH.error("Frame %d : Unknown field '%s'\n", num, word);
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
	  else DEH.error("Weaponframe %d : Unknown field '%s'\n", num, word);
	}
    }
}


static void Read_Sound(Parser &p, int num)
{
  char *word;
  int value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      value = SearchValue(p);
      word = p.GetToken(" ");
      // TODO dehacked sound commands
      /*
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
	      DEH.error("Sound %d : offset out of bound\n",num);
	  }
	else if (!strcasecmp(word,"Zero/One"))
	  S_sfx[num].multiplicity = value;
	else if (!strcasecmp(word,"Value"))
	  S_sfx[num].priority   =value;
	else
	  DEH.error("Sound %d : unknown word '%s'\n",num,word);
      */
    }
}


// this part of dehacked really sucks, but it still partly supported
static void Read_Text(Parser &p, int len1, int len2)
{
  char s[2001];
  int i;

  // FIXME dehacked text
  // it is hard to change all the text in doom
  // here i implement only vital things
  // yes, "text" can change some tables like music, sound and sprite names
  if (len1+len2 > 2000)
    {
      DEH.error("Text too long\n");
      return;
    }
  
  if (p.GetStringN(s, len1 + len2) != len1 + len2)
    {
      DEH.error("Read failed\n");
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
      if (!strncmp(text[i], s, len1) && temp == len1)
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
  DEH.error("Text not changed :%s\n", s);
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
static void Read_Weapon(Parser &p, int num)
{
  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      int value = SearchValue(p);
      char *word = p.GetToken(" ");

      if (!strcasecmp(word,"Ammo"))
	wpnlev1info[num].ammo = ammotype_t(value);
      else
	{
	  value = -StateMap(value);
	  if (value <= 0)
	    {
	      DEH.error("Weapon %d : Thing states must not be used with weapons!\n", num);
	      continue;
	    }

	  if (!strcasecmp(word,"Deselect"))      wpnlev1info[num].upstate    = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Select"))   wpnlev1info[num].downstate  = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Bobbing"))  wpnlev1info[num].readystate = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Shooting"))
	    wpnlev1info[num].atkstate = wpnlev1info[num].holdatkstate        = weaponstatenum_t(value);
	  else if (!strcasecmp(word,"Firing"))   wpnlev1info[num].flashstate = weaponstatenum_t(value);
	  else DEH.error("Weapon %d : unknown word '%s'\n", num, word);
	}
    }
}

/*
  Ammo sample:
Max ammo = 400
Per ammo = 40
*/
static void Read_Ammo(Parser &p, int num)
{
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

      int value = SearchValue(p);
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
	  if (clips[3][num] > 0)
	    mobjinfo[clips[3][num]].spawnhealth = value;
	}
      else
	DEH.error("Ammo %d : unknown word '%s'\n", num, word);
    }
}


// miscellaneous one-liners
void dehacked_t::Read_Misc(Parser &p)
{
  extern int MaxArmor[];

  char *word1, *word2;
  int value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      value = SearchValue(p);
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

static void Read_Cheat(Parser &p)
{
  char *word;
  byte *value;

  while (p.NewLine(false))
    {
      p.PassWS();
      if (!p.LineLen())
	break; // a whitespace-only line ends the record

      // FIXME how does this work?
      p.GetToken("=");
      value = (byte *)p.GetToken(" \n"); // skip the space
      p.GetToken(" \n");         // finish the string
      word = p.GetToken(" ");

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
      else DEH.error("Cheat : unknown word '%s'\n",word);
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
  Parser p;
  int i;

  if (!p.Open(buf, len))
    return false;

  num_errors = 0;

  // original values
  d_actions = (actionf_p1 *)Z_Malloc(NUMSTATES * sizeof(actionf_p1), PU_STATIC, NULL);
  w_actions = (actionf_p2 *)Z_Malloc(NUMWEAPONSTATES * sizeof(actionf_p2), PU_STATIC, NULL);
  savesprnames = (char **)Z_Malloc(NUMSPRITES * sizeof(char *), PU_STATIC, NULL);

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
	      Read_Thing(p, i);
	      break;

	    case DEH_Frame:
	      Read_Frame(p, i);
	      break;

	    case DEH_Pointer:
	      /*
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
	      if (i < NUMSFX && i >= 0)
		Read_Sound(p, i);
	      else
		error("Sound %d doesn't exist\n");
	      break;

	    case DEH_Sprite:
	      if (i < NUMSPRITES && i >= 0)
		{
		  if (p.NewLine())
		    {
		      int k;
		      k = (SearchValue(p) - 151328) / 8;
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
		  Read_Text(p, i, j);
		}
	      else
		error("Text : missing second number\n");
	      break;

	    case DEH_Weapon:
	      if (i < NUMWEAPONS && i >= 0)
		Read_Weapon(p, i);
	      else
		error("Weapon %d doesn't exist\n", i);
	      break;

	    case DEH_Ammo:
	      if (i < NUMAMMO && i >= 0)
		Read_Ammo(p, i);
	      else
		error("Ammo %d doesn't exist\n", i);
	      break;

	    case DEH_Misc:
	      Read_Misc(p);
	      break;

	    case DEH_Cheat:
	      Read_Cheat(p);
	      break;

	    case DEH_Doom:
	      p.NewLine();
	      i = SearchValue(p);
	      if (i != 19)
		error("Warning : Patch from a different Doom version (%d), only version 1.9 is supported\n", i);
	      break;

	    case DEH_Patch:
	      word1 = p.GetToken(" ");
	      if (word1 && !strcasecmp(word1, "format"))
		{
		  p.NewLine();
		  if (SearchValue(p) != 6)
		    error("Warning : Patch format not supported");
		}
	      break;

	      // BEX stuff
	    case DEH_CODEPTR:
	      Read_CODEPTR(p);
	      break;
	    case DEH_PARS:
	      // TODO support PARS
	      break;
	    case DEH_STRINGS:
	      // TODO support STRINGS
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
  return true;
}
