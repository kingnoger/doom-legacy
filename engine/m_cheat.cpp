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
//
// $Log$
// Revision 1.19  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.18  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.17  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.16  2004/01/02 14:25:01  smite-meister
// cleanup
//
// Revision 1.15  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.14  2003/12/14 00:20:02  smite-meister
// quick bugfix
//
// Revision 1.13  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.12  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.11  2003/05/30 13:34:44  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.10  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.9  2003/03/15 20:07:15  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.8  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.7  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.6  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.5  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.4  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.3  2002/12/16 22:11:21  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:11:39  smite-meister
// Blindness and missile clipping bugs fixed
//
//
// DESCRIPTION:
//   Cheat sequences.
//
//-----------------------------------------------------------------------------
 

#include "tables.h"
#include "dstrings.h"
#include "dehacked.h"

#include "command.h"

#include "m_cheat.h"
#include "g_game.h"
#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"

#include "d_event.h"

#include "am_map.h"
#include "i_sound.h" // for I_PlayCD()
#include "sounds.h"
#include "w_wad.h"


void cht_Init()
{
  // used to generate the cheat scrambling table
}


// console commands

void Command_CheatNoClip_f()
{
  if (!game.server || !consoleplayer)
    return;

  PlayerPawn *p = consoleplayer->pawn;
  if (p == NULL) return;

  p->cheats ^= CF_NOCLIP;

  if (p->cheats & CF_NOCLIP)
    CONS_Printf (STSTR_NCON);
  else
    CONS_Printf (STSTR_NCOFF);
}

void Command_CheatGod_f()
{
  if (!game.server || !consoleplayer)
    return;

  PlayerPawn *p = consoleplayer->pawn;
  if (p == NULL) return;

  p->cheats ^= CF_GODMODE;
  if (p->cheats & CF_GODMODE)
    {
      p->health = 100;
      CONS_Printf ("%s\n", STSTR_DQDON);
    }
  else
    CONS_Printf ("%s\n", STSTR_DQDOFF);
}

void Command_CheatGimme_f()
{
  char*     s;
  int       i,j;

  if (!game.server || !consoleplayer)
    return;

  if (COM_Argc()<2)
    {
      CONS_Printf ("gimme [health] [ammo] [armor] ...\n");
      return;
    }

  PlayerPawn* p = consoleplayer->pawn;
  if (p == NULL) return;

  for (i=1; i<COM_Argc(); i++) {
    s = COM_Argv(i);

    if (!strncmp(s,"health",6))
      {
	p->health = 100;
	CONS_Printf("got health\n");
      }
    else if (!strncmp(s,"ammo",4))
      {
	for (j=0;j<NUMAMMO;j++)
	  p->ammo[j] = p->maxammo[j];

	CONS_Printf("got ammo\n");
      }
    else if (!strncmp(s,"armor",5))
      {
	p->armorpoints[0] = 200;
	p->armorfactor[0] = 0.5;

	CONS_Printf("got armor\n");
      }
    else if (!strncmp(s,"keys",4))
      {
	p->keycards = it_allkeys;

	CONS_Printf("got keys\n");
      }
    else if (!strncmp(s,"weapons",7))
      {
	for (j=0;j<NUMWEAPONS;j++)
	  p->weaponowned[j] = true;

	for (j=0;j<NUMAMMO;j++)
	  p->ammo[j] = p->maxammo[j];

	CONS_Printf("got weapons\n");
      }
    else if (!strncmp(s,"chainsaw",8))
      //
      // WEAPONS
      //
      {
	p->weaponowned[wp_chainsaw] = true;

	CONS_Printf("got chainsaw\n");
      }
    else if (!strncmp(s,"shotgun",7))
      {
	p->weaponowned[wp_shotgun] = true;
	p->ammo[am_shell] = p->maxammo[am_shell];

	CONS_Printf("got shotgun\n");
      }
    else if (!strncmp(s,"supershotgun",12))
      {
	if (game.mode == gm_doom2) // only in Doom2
	  {
	    p->weaponowned[wp_supershotgun] = true;
	    p->ammo[am_shell] = p->maxammo[am_shell];

	    CONS_Printf("got super shotgun\n");
	  }
      }
    else if (!strncmp(s,"rocket",6))
      {
	p->weaponowned[wp_missile] = true;
	p->ammo[am_misl] = p->maxammo[am_misl];

	CONS_Printf("got rocket launcher\n");
      }
    else if (!strncmp(s,"plasma",6))
      {
	p->weaponowned[wp_plasma] = true;
	p->ammo[am_cell] = p->maxammo[am_cell];

	CONS_Printf("got plasma\n");
      }
    else if (!strncmp(s,"bfg",3))
      {
	p->weaponowned[wp_bfg] = true;
	p->ammo[am_cell] = p->maxammo[am_cell];

	CONS_Printf("got bfg\n");
      }
    else if (!strncmp(s,"chaingun",8))
      {
	p->weaponowned[wp_chaingun] = true;
	p->ammo[am_clip] = p->maxammo[am_clip];

	CONS_Printf("got chaingun\n");
      }
    else if (!strncmp(s,"berserk",7))
      //
      // SPECIAL ITEMS
      //
      {
	if (!p->powers[pw_strength])
	  p->GivePower(pw_strength);
	CONS_Printf("got berserk strength\n");
      }
    //22/08/99: added by Hurdler
    else if (!strncmp(s,"map",3))
      {
	automap.am_cheating = 1;
	CONS_Printf("got map\n");
      }
    //
    else if (!strncmp(s,"fullmap",7))
      {
	automap.am_cheating = 2;
	CONS_Printf("got map and things\n");
      }
    else
      CONS_Printf ("can't give '%s' : unknown\n", s);
  }
}




//==========================================================================
//  Doom cheats
//==========================================================================

byte   cheat_mus_seq[] =
{
  //0xb2, 0x26, 0xb6, 0xae, 0xea, 0, 0, 0xff // idmus__
  'i', 'd', 'm', 'u', 's', 0, 0, 0xff
};

byte   cheat_cd_seq[] =
{
  //0xb2, 0x26, 0xe2, 0x26, 0, 0, 0xff // idcd__
  'i', 'd', 'c', 'd', 0, 0, 0xff
};

byte   cheat_choppers_seq[] =
{
  //0xb2, 0x26, 0xe2, 0x32, 0xf6, 0x2a, 0x2a, 0xa6, 0x6a, 0xea, 0xff // idchoppers
  'i', 'd', 'c', 'h', 'o', 'p', 'p', 'e', 'r', 's', 0xff
};

byte   cheat_god_seq[] =
{
  //0xb2, 0x26, 0x26, 0xaa, 0x26, 0xff  // iddqd
  'i', 'd', 'd', 'q', 'd', 0xff
};


byte   cheat_ammo_seq[] =
{
  //0xb2, 0x26, 0xf2, 0x66, 0xa2, 0xff  // idkfa
  'i', 'd', 'k', 'f', 'a', 0xff
};

byte   cheat_ammonokey_seq[] =
{
  //0xb2, 0x26, 0x66, 0xa2, 0xff        // idfa
  'i', 'd', 'f', 'a', 0xff 
};


// Smashing Pumpkins Into Small Pieces Of Putrid Debris.
byte   cheat_noclip_seq[] =
{
  //0xb2, 0x26, 0xea, 0x2a, 0xb2,       // idspispopd
  //0xea, 0x2a, 0xf6, 0x2a, 0x26, 0xff
  'i', 'd', 's', 'p', 'i', 's', 'p', 'o', 'p', 'd', 0xff
};

byte   cheat_commercial_noclip_seq[] =
{
  //0xb2, 0x26, 0xe2, 0x36, 0xb2, 0x2a, 0xff    // idclip
  'i', 'd', 'c', 'l', 'i', 'p', 0xff
};

//added:28-02-98: new cheat to fly around levels using jump !!
byte   cheat_fly_around_seq[] =
{
  'i', 'd', 'f', 'l', 'y', 0xff // idfly
};

/*
byte   cheat_powerup_seq[7][10] =
{
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6e, 0xff },     // beholdv
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xea, 0xff },     // beholds
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xb2, 0xff },     // beholdi
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6a, 0xff },     // beholdr
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xa2, 0xff },     // beholda
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x36, 0xff },     // beholdl
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xff }            // behold
};
*/

// idbehold message
byte   cheat_powerup_seq1[] =
{
  //0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xff // idbehold
  'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 0xff
};

// actual cheat
byte   cheat_powerup_seq2[] =
{
  //0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0, 0xff // idbehold_
  'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 0, 0xff
};

byte   cheat_clev_seq[] =
{
  //0xb2, 0x26,  0xe2, 0x36, 0xa6, 0x6e, 0, 0, 0xff  // idclev__
  'i', 'd', 'c', 'l', 'e', 'v', 0, 0, 0xff
};

// my position cheat
byte   cheat_mypos_seq[] =
{
  //0xb2, 0x26, 0xb6, 0xba, 0x2a, 0xf6, 0xea, 0xff      // idmypos
  'i', 'd', 'm', 'y', 'p', 'o', 's', 0xff
};

byte cheat_amap_seq[] =
{
  //0xb2, 0x26, 0x26, 0x2e, 0xff // iddt
  'i', 'd', 'd', 't', 0xff
};


//==========================================================================
//  Heretic cheats
//==========================================================================


// Toggle god mode
static byte CheatGodSeq[] =
{
  'q', 'u', 'i', 'c', 'k', 'e', 'n', 0xff
};

// Toggle no clipping mode
static byte CheatNoClipSeq[] =
{
  'k', 'i', 't', 't', 'y', 0xff
};

// Get all weapons and ammo
static byte CheatWeaponsSeq[] =
{
  'r', 'a', 'm', 'b', 'o', 0xff
};

// Toggle tome of power
static byte CheatPowerSeq[] =
{
  's', 'h', 'a', 'z', 'a', 'm', 0xff, 0
};

// Get full health
static byte CheatHealthSeq[] =
{
  'p', 'o', 'n', 'c', 'e', 0xff
};

// Get all keys
static byte CheatKeysSeq[] =
{
  's', 'k', 'e', 'l', 0xff, 0
};

// Get an artifact 1st stage (ask for type)
static byte CheatArtifact1Seq[] =
{
  'g', 'i', 'm', 'm', 'e', 0xff
};

// Get an artifact 2nd stage (ask for count)
static byte CheatArtifact2Seq[] =
{
  'g', 'i', 'm', 'm', 'e', 0, 0xff, 0
};

// Get an artifact final stage
static byte CheatArtifact3Seq[] =
{
  'g', 'i', 'm', 'm', 'e', 0, 0, 0xff
};

// Warp to new level
static byte CheatWarpSeq[] =
{
  'e', 'n', 'g', 'a', 'g', 'e', 0, 0, 0xff, 0
};

// Save a screenshot
static byte CheatChickenSeq[] =
{
  'c', 'o', 'c', 'k', 'a', 'd', 'o', 'o', 'd', 'l', 'e', 'd', 'o', 'o', 0xff, 0
};

// Kill all monsters
static byte CheatMassacreSeq[] =
{
  'm', 'a', 's', 's', 'a', 'c', 'r', 'e', 0xff, 0
};

static byte CheatIDKFASeq[] =
{
  'i', 'd', 'k', 'f', 'a', 0xff, 0
};

static byte CheatIDDQDSeq[] =
{
  'i', 'd', 'd', 'q', 'd', 0xff, 0
};



//--------------------------------------------------------------------------
//
// CHEAT FUNCTIONS
//
//--------------------------------------------------------------------------

// not yet a console command, but a cheat
void CheatFlyFunc(PlayerPawn *p, const byte *arg)
{
  char *msg;

  p->cheats ^= CF_FLYAROUND;
  if (p->cheats & CF_FLYAROUND)
    msg = "FLY MODE ON : USE JUMP KEY";
  else
    msg = "FLY MODE OFF";

  p->player->SetMessage(msg, false);
}

void CheatCDFunc(PlayerPawn *p, const byte *arg)
{
  // 'idcd' for changing cd track quickly
  //NOTE: the cheat uses the REAL track numbers, not remapped ones

  p->player->SetMessage("Changing cd track...", false);
  I_PlayCD((arg[0]-'0')*10 + (arg[1]-'0'), true);
}

void CheatMusFunc(PlayerPawn *p, const byte *arg)
{
  // 'mus' cheat for changing music
  int  musnum;
  char *msg;

  msg = STSTR_MUS;

  if (game.mode == gm_doom2)
    {
      musnum = (arg[0]-'0')*10 + arg[1]-'0';

      if (musnum < 1 || musnum > 35)
	msg = STSTR_NOMUS;
      else
	S_StartMusic(musnum + mus_runnin - 1, true);
    }
  else
    {
      musnum = (arg[0]-'1')*9 + (arg[1]-'1');

      if (musnum < 0 || musnum > 31)
	msg = STSTR_NOMUS;
      else
	S_StartMusic(musnum + mus_e1m1, true);
    }
  p->player->SetMessage(msg, false);
}

void CheatMyPosFunc(PlayerPawn *p, const byte *arg)
{
  // 'mypos' for player position
  //extern int statusbarplayer; // FIXME! show statbarpl. coordinates, not consolepl.

  CONS_Printf(va("ang=%i;x,y=(%i,%i)\n", p->angle / ANGLE_1, p->x >> FRACBITS,
		 p->y >> FRACBITS));
}

static void CheatAMFunc(PlayerPawn *p, const byte *arg)
{
  automap.am_cheating = (automap.am_cheating+1) % 3;
}

static void CheatGodFunc(PlayerPawn *p, const byte *arg)
{
  char *msg;

  p->cheats ^= CF_GODMODE;

  if (game.mode == gm_heretic) {
    if (p->cheats & CF_GODMODE)
      {
	msg = CHEAT_GODON;
      }
    else
      {
	msg = CHEAT_GODOFF;
      }
  } else { // doom then
    if (p->cheats & CF_GODMODE)
      {
	p->health = DEH.god_health;
	msg = STSTR_DQDON;
      }
    else
      msg = STSTR_DQDOFF;
  }
  p->player->SetMessage(msg, false);
}

static void CheatChopFunc(PlayerPawn *p, const byte *arg)
{
  // 'choppers' invulnerability & chainsaw
  p->weaponowned[wp_chainsaw] = true;
  p->powers[pw_invulnerability] = true;

  p->player->SetMessage(STSTR_CHOPPERS, false);
}


static void CheatPowerup1Func(PlayerPawn *p, const byte *arg)
{
  // 'behold' power-up menu
  p->player->SetMessage(STSTR_BEHOLD, false);
}


static void CheatPowerup2Func(PlayerPawn *p, const byte *arg)
{
  // arg[0] = [vsiral]
  // 'behold?' power-up cheats
  int i;

  switch (arg[0]) {
  case 'v': i=0; break;
  case 's': i=1; break;
  case 'i': i=2; break;
  case 'r': i=3; break;
  case 'a': i=4; break;
  case 'l': i=5; break;
  default: return; // invalid letter
  }

  if (!p->powers[i])
    p->GivePower(i);
  else if (i != pw_strength)
    p->powers[i] = 1;
  else
    p->powers[i] = 0;

  p->player->SetMessage(STSTR_BEHOLDX, false);
}


static void CheatNoClipFunc(PlayerPawn *p, const byte *arg)
{
  char *msg;

  p->cheats ^= CF_NOCLIP;

  if (p->cheats & CF_NOCLIP)
    {
      if (game.mode == gm_heretic) 
	msg = CHEAT_NOCLIPON;
      else
	msg = STSTR_NCON;
    }
  else
    {
      if (game.mode == gm_heretic) 
	msg = CHEAT_NOCLIPOFF;
      else
	msg = STSTR_NCOFF;
    }

  p->player->SetMessage(msg, false);
}

static const bool shareware = true;

static void CheatWeaponsFunc(PlayerPawn *p, const byte *arg)
{
  char *msg;
  int i;

  p->armorpoints[0] = DEH.idfa_armor;
  p->armorfactor[0] = 0.5;

  if (game.mode == gm_heretic)
    {
      // give backpack
      if (!p->backpack)
	{
	  p->maxammo = maxammo2;
	  p->backpack = true;
	}

      for (i = wp_heretic; i <= wp_gauntlets; i++)
	p->weaponowned[i] = true;

      // FIXME shareware == true always (it is not a variable!)
      // (we do not have "heretic shareware" gametype)(yet?)
      // also elsewhere in this file
      if (shareware)
	{
	  p->weaponowned[wp_skullrod] = false;
	  p->weaponowned[wp_phoenixrod] = false;
	  p->weaponowned[wp_mace] = false;
	}

      msg = CHEAT_WEAPONS;
    }
  else
    {
      for (i=0; i < wp_heretic; i++)
	p->weaponowned[i] = true;

      if (game.mode != gm_doom2)
	p->weaponowned[wp_supershotgun] = false;

      msg = STSTR_FAADDED;
    }

  for (i = 0; i < NUMAMMO; i++)
    p->ammo[i] = p->maxammo[i];

  p->player->SetMessage(msg, false);
}

bool P_UseArtifact(PlayerPawn *p, artitype_t arti);

static void CheatPowerFunc(PlayerPawn *p, const byte *arg)
{
  if(p->powers[pw_weaponlevel2])
    {
      p->powers[pw_weaponlevel2] = 0;
      p->player->SetMessage(CHEAT_POWEROFF, false);
    }
  else
    {
      P_UseArtifact(p, arti_tomeofpower);
      p->player->SetMessage(CHEAT_POWERON, false);
    }
}

static void CheatHealthFunc(PlayerPawn *p, const byte *arg)
{
  p->health = p->maxhealth;
  p->player->SetMessage(CHEAT_HEALTH, false);
}

static void CheatKeysFunc(PlayerPawn *p, const byte *arg)
{
  p->keycards |= it_allkeys;
  p->player->SetMessage(CHEAT_KEYS, false);
}

static void CheatArtifact1Func(PlayerPawn *p, const byte *arg)
{
  p->player->SetMessage(CHEAT_ARTIFACTS1, false);
}

static void CheatArtifact2Func(PlayerPawn *p, const byte *arg)
{
  p->player->SetMessage(CHEAT_ARTIFACTS2, false);
}

static void CheatArtifact3Func(PlayerPawn *p, const byte *arg)
{
  int i;
  int j;
  artitype_t type;
  int count;

  type = artitype_t(arg[0]-'a'+1);
  count = arg[1]-'0';
  if (type == 26 && count == 0)
    { // All artifacts
      for(i = arti_none+1; i < NUMARTIFACTS; i++)
	{
	  if(shareware && (i == arti_superhealth
			   || i == arti_teleport))
	    {
	      continue;
	    }
	  for(j = 0; j < 16; j++)
	    {
	      p->GiveArtifact(artitype_t(i), NULL);
	    }
	}
      p->player->SetMessage(CHEAT_ARTIFACTS3, false);
    }
  else if(type > arti_none && type < NUMARTIFACTS
	  && count > 0 && count < 10)
    {
      if(shareware && (type == arti_superhealth || type == arti_teleport))
	{
	  p->player->SetMessage(CHEAT_ARTIFACTSFAIL, false);
	  return;
	}
      for(i = 0; i < count; i++)
	{
	  p->GiveArtifact(type, NULL);
	}
      p->player->SetMessage(CHEAT_ARTIFACTS3, false);
    }
  else
    { // Bad input
      p->player->SetMessage(CHEAT_ARTIFACTSFAIL, false);
    }
}

static void CheatWarpFunc(PlayerPawn *p, const byte *arg)
{
  int mapnum;
  char *msg;

  // "idclev" or "engage" change-level cheat
  char name[9];

  switch (game.mode)
    {
    case gm_doom2:
    case gm_hexen:
      mapnum = (arg[0] - '0')*10 + arg[1] - '0';
      if (mapnum < 1 || mapnum > 99)
	return;
      sprintf(name, "MAP%2d", mapnum);
      break;

    default:
      // doom1, heretic
      int episode = arg[0] - '0';
      mapnum = arg[1] - '0';
      if (episode < 1 || episode > 9 || mapnum < 1 || mapnum > 9)
	return;
      sprintf(name, "E%1dM%1d", episode, mapnum);
    }

  if (game.mode == gm_heretic)
    msg = CHEAT_WARP;
  else
    msg = STSTR_CLEV;

  p->player->SetMessage(msg, false);
  COM_BufAddText(va("map %s\n", name));
}

static void CheatChickenFunc(PlayerPawn *p, const byte *arg)
{
  if (p->morphTics)
    {
      if (p->UndoMorph())
	{
	  p->player->SetMessage(CHEAT_CHICKENOFF, false);
	}
    }
  else if (p->Morph(MT_CHICPLAYER))
    {
      p->player->SetMessage(CHEAT_CHICKENON, false);
    }
}

static void CheatMassacreFunc(PlayerPawn *p, const byte *arg)
{
  p->mp->Massacre();
  p->player->SetMessage(CHEAT_MASSACRE, false);
}

static void CheatIDKFAFunc(PlayerPawn *p, const byte *arg)
{
  int i;

  if (game.mode == gm_heretic)
    {
      // playing heretic, let's punish the player!
      if (p->morphTics)
	return;

      for(i = 0; i < NUMWEAPONS; i++)
	p->weaponowned[i] = false;
      p->weaponowned[wp_staff] = true;
      p->pendingweapon = wp_staff;

      p->player->SetMessage(CHEAT_IDKFA, true);
    }
  else
    {
      // doom, give stuff
      p->armorpoints[0] = DEH.idkfa_armor;
      p->armorfactor[0] = 0.5;

      for (i = wp_doom; i < wp_heretic; i++)
	p->weaponowned[i] = true;

      if (game.mode != gm_doom2)
	p->weaponowned[wp_supershotgun] = false;

      for (i = am_doom; i < am_heretic; i++)
	p->ammo[i] = p->maxammo[i];

      p->keycards = it_allkeys;

      p->player->SetMessage(STSTR_KFAADDED, false);
    }
}

static void CheatIDDQDFunc(PlayerPawn *p, const byte *arg)
{
  p->Damage(p, p, 10000, dt_always);
  p->player->SetMessage(CHEAT_IDDQD, true);
}



//==========================================================================
//  Cheat lists (must be ended with a TCheat(NULL, ...) terminator)
//==========================================================================

// a class for handling cheat sequences
class TCheat
{
  typedef void (* fp)(PlayerPawn *p, const byte *arg);
private:
  fp func;
  byte *seq;
  byte *pos;
  byte args[2];
  byte currarg;

public:

  TCheat(fp f, byte *s);
  bool AddKey(byte key, bool *eat);

  friend bool cht_Responder(event_t* ev);
};


// constructor
TCheat::TCheat(fp f, byte *s)
{
  func = f;
  seq = pos = s;
  args[0] = args[1] = 0;
  currarg = 0;
}

// returns true if sequence is completed
bool TCheat::AddKey(byte key, bool *eat)
{
  if(*pos == 0)
    {
      // read a parameter
      *eat = true;
      args[currarg++] = key;
      pos++;
    }
  //else if (cheat_xlate_table[key] == *pos)
  else if (key == *pos)
    {
      // correct key, go on
      pos++;
    }
  else
    {
      // wrong key, reset sequence
      pos = seq;
      currarg = 0;
    }

  if(*pos == 0xff)
    {
      // sequence complete!
      pos = seq;
      currarg = 0;
      return true;
    }

  return false;
}


// universal cheats which work in every game mode (begin with id...)
static TCheat Basic_Cheats[] =
{
  TCheat(CheatFlyFunc, cheat_fly_around_seq),
  TCheat(CheatCDFunc, cheat_cd_seq),
  TCheat(CheatMyPosFunc, cheat_mypos_seq),
  TCheat(NULL, NULL)
};

// original Doom cheats
static TCheat Doom_Cheats[] =
{
  TCheat(CheatAMFunc, cheat_amap_seq),
  TCheat(CheatMusFunc, cheat_mus_seq),
  TCheat(CheatGodFunc, cheat_god_seq),
  TCheat(CheatWeaponsFunc, cheat_ammonokey_seq),
  TCheat(CheatChopFunc, cheat_choppers_seq),
  TCheat(CheatIDKFAFunc, cheat_ammo_seq),
  TCheat(CheatNoClipFunc, cheat_noclip_seq),
  TCheat(CheatNoClipFunc, cheat_commercial_noclip_seq),
  TCheat(CheatPowerup1Func, cheat_powerup_seq1),
  TCheat(CheatPowerup2Func, cheat_powerup_seq2),
  TCheat(CheatWarpFunc, cheat_clev_seq),
  TCheat(NULL, NULL)
};

// original Heretic cheats
static TCheat Heretic_Cheats[] =
{
  TCheat(CheatGodFunc, CheatGodSeq),
  TCheat(CheatNoClipFunc, CheatNoClipSeq),
  TCheat(CheatWeaponsFunc, CheatWeaponsSeq),
  TCheat(CheatPowerFunc, CheatPowerSeq),
  TCheat(CheatHealthFunc, CheatHealthSeq),
  TCheat(CheatKeysFunc, CheatKeysSeq),
  TCheat(CheatArtifact1Func, CheatArtifact1Seq),
  TCheat(CheatArtifact2Func, CheatArtifact2Seq),
  TCheat(CheatArtifact3Func, CheatArtifact3Seq),
  TCheat(CheatWarpFunc, CheatWarpSeq),
  TCheat(CheatChickenFunc, CheatChickenSeq),
  TCheat(CheatMassacreFunc, CheatMassacreSeq),
  TCheat(CheatIDKFAFunc, CheatIDKFASeq),
  TCheat(CheatIDDQDFunc, CheatIDDQDSeq),
  TCheat(NULL, NULL) // Terminator
};


bool cht_Responder(event_t* ev)
{
  int i;
  bool eat = false;
  TCheat *cheats = Doom_Cheats;

  if (ev->type != ev_keydown)
    return false;

  if (game.netgame || game.skill == sk_nightmare || !consoleplayer)
    { // Can't cheat in a net-game, or in nightmare mode
      return false;
    }

  PlayerPawn *p = consoleplayer->pawn;

  if (p == NULL || p->health <= 0)
    { // Dead players can't cheat
      return false;
    }

  byte key = ev->data1;

  // what about splitscreen?

  // universal cheats first
  for (i = 0; Basic_Cheats[i].func != NULL; i++)
    {
      if (Basic_Cheats[i].AddKey(key, &eat))
	{
	  Basic_Cheats[i].func(p, Basic_Cheats[i].args);
	}
    }

  // use heretic cheats instead?
  if (game.mode == gm_heretic)
    cheats = Heretic_Cheats;
  else if (game.mode == gm_hexen)
    return eat; // TODO no Hexen cheats yet

  for (i = 0; cheats[i].func != NULL; i++)
    {
      if (cheats[i].AddKey(key, &eat))
	{
	  CONS_Printf("Cheating, %d\n", i);
	  cheats[i].func(p, cheats[i].args);
	  if (game.mode == gm_heretic)
	    S_StartAmbSound(sfx_dorcls);
	}
    }
  return eat;
}
