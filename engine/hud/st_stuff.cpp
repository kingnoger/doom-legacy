// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.36  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.35  2005/07/20 20:27:22  smite-meister
// adv. texture cache
//
// Revision 1.33  2005/04/17 18:36:34  smite-meister
// netcode
//
// Revision 1.29  2004/12/05 14:46:33  smite-meister
// keybar
//
// Revision 1.28  2004/12/02 17:22:35  smite-meister
// HUD fixed
//
// Revision 1.27  2004/11/13 22:38:43  smite-meister
// intermission works
//
// Revision 1.26  2004/11/04 21:12:53  smite-meister
// save/load fixed
//
// Revision 1.24  2004/10/27 17:37:08  smite-meister
// netcode update
//
// Revision 1.23  2004/10/14 19:35:46  smite-meister
// automap, bbox_t
//
// Revision 1.22  2004/09/23 23:21:18  smite-meister
// HUD updated
//
// Revision 1.20  2004/09/03 16:28:50  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.19  2004/07/25 20:19:21  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.18  2004/07/05 16:53:27  smite-meister
// Netcode replaced
//
// Revision 1.17  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.15  2003/12/09 01:02:01  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.14  2003/11/23 19:07:41  smite-meister
// New startup order
//
// Revision 1.13  2003/05/30 13:34:47  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.12  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.11  2003/04/14 08:58:28  smite-meister
// Hexen maps load.
//
// Revision 1.10  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.9  2003/03/15 20:07:19  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.8  2003/03/08 16:07:11  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.7  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.6  2003/01/12 12:56:41  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.5  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.4  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.3  2002/12/16 22:12:18  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:20:08  smite-meister
// HUD rationalized
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Status bar code.
///
/// Numbers, icons, face/direction indicator animation.
/// Does palette flashes as well (red pain/berserk, bright pickup...)

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "am_map.h"

#include "d_event.h"
#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"

#include "screen.h"
#include "r_main.h"
#include "r_data.h"

#include "m_random.h"

#include "hud.h"
#include "st_lib.h"
#include "i_video.h"
#include "v_video.h"
#include "tables.h"

#include "w_wad.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hwr_states.h"
#endif

#define FG 0
#define BG 1

// buffer for drawing status bar
int fgbuffer = FG;


// Size of statusbar.
#define ST_HEIGHT_DOOM    32
#define ST_HEIGHT_HERETIC 42
#define ST_HEIGHT_HEXEN   39
#define ST_WIDTH         320


// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS            1
#define NUMREDPALS              8
#define STARTBONUSPALS          9
#define NUMBONUSPALS            4
// Radiation suit, green shift.
#define RADIATIONPAL            13
// new Hexen palettes
#define STARTPOISONPALS 13
#define NUMPOISONPALS   8
#define STARTICEPAL     21
#define STARTHOLYPAL    22
#define STARTSCOURGEPAL 25



// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY              96

// For Responder
#define ST_TOGGLECHAT           KEY_ENTER


// Number of status faces.
#define ST_NUMPAINFACES         5
#define ST_NUMSTRAIGHTFACES     3
#define ST_NUMTURNFACES         2
#define ST_NUMSPECIALFACES      3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES        2

#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_TURNOFFSET           (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET           (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET       (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET        (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE              (ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE             (ST_GODFACE+1)

#define ST_EVILGRINCOUNT        (2*TICRATE)
#define ST_STRAIGHTFACECOUNT    (TICRATE/2)
#define ST_TURNCOUNT            (1*TICRATE)
#define ST_OUCHCOUNT            (1*TICRATE)
#define ST_RAMPAGEDELAY         (2*TICRATE)

#define ST_MUCHPAIN             20


//=========================================
// HUD widget control variables
//=========================================

// palette flashes
static int  st_berzerk;
static bool st_radiation;

// inside the HUD class:
// statusbar_on, mainbar_on, invopen

static const bool st_true = true;

static bool st_notdeathmatch;
static bool st_godmode;

static int  st_pawncolor;
static int  st_health;
static int  st_maxhealth;
static int  st_oldhealth; // to get appopriately pained face

static int  st_armor;
static int  st_readywp;
static int  st_atype;
static int  st_readywp_ammo;

static int  st_ammo[NUMAMMO];
static int  st_maxammo[NUMAMMO];

static int  st_faceindex = 0; // current marine face

// owned keys
static int st_keyboxes[NUMKEYS];

// number of frags so far in deathmatch
static int  st_fragscount;

// Heretic spinning icons
static int  st_flight = -1;
static int  st_book = -1;
static int  st_speed = -1;
static int  st_defense = -1;
static int  st_minotaur = -1;

static inventory_t st_invslots[7+1]; // visible inventory slots (+ one hack slot)
int st_curpos = 0; // active inv. slot (0-6)

// used for evil grin
static bool st_oldweaponsowned[NUMWEAPONS];

static bool st_mana1icon, st_mana2icon;
static int st_mana1, st_mana2;

#define BLINKTHRESHOLD  (4*32)



//=========================================
//  Legacy status bar overlay
//=========================================

static Texture *sbohealth;
static Texture *sbofrags;
static Texture *sboarmor;
static Texture *PatchAmmoPic[NUMAMMO + 1];


//=========================================
// Doom status bar graphics
//=========================================

// doom.wad number sets:
// "AMMNUM0", small, thin, gray, no minus
// "WINUM0", large, red, minus is base-2, '%' is base-1

// "STTNUM0", large, red, minus is base-1, '%' is base+10
static Texture *PatchBNum[11]; // 0-9, big numbers, STTMINUS
static Texture *tallpercent; // big % sign

// "STYSNUM0", small, yellow, no minus
static Texture *PatchSNum[11];

// "STGNUM0", small, dark gray, no minus
static Texture *PatchArms[6][2]; // weapon ownership patches

static Texture *PatchArmsBack; // arms background
static Texture *PatchFaces[ST_NUMFACES]; // marine face patches
static Texture *PatchFaceBack; // face background

static Texture *PatchKeys[NUMKEYS]; // 3 key-cards, 3 skulls
static Texture *PatchSTATBAR;
static Texture *PatchKEYBAR;

// ammo type pics (Texture's)
static const char DHAmmoPics[NUMAMMO + 1][10] =
{
  {"SBOAMMO1"}, //{"CLIPA0"},  // 0, bullets
  {"SBOAMMO2"}, //{"SHELA0"},  // shells
  {"SBOAMMO3"}, //{"CELLA0"},  // plasma
  {"SBOAMMO4"}, //{"ROCKA0"},  // rockets
  {"INAMGLD"}, // gold wand
  {"INAMBOW"}, // crossbow
  {"INAMBST"}, // blaster
  {"INAMRAM"}, // skullrod
  {"INAMPNX"}, // phoenix rod
  {"INAMLOB"}, // mace
  {"MANABRT1"}, // mana 1
  {"MANABRT2"}, // mana 2
  {"BLACKSQ"}  // no ammopic
};


//=========================================
// Heretic status bar graphics
//=========================================

static Texture *PatchGod[2];
static Texture *PatchBARBACK;
static Texture *PatchLTFCTOP;
static Texture *PatchRTFCTOP;
static Texture *PatchARMCLEAR;

static Texture *Patch_InvBar[13];
static Texture *PatchARTI[NUMARTIFACTS];
static Texture *Patch_ChainSlider[5];

// Heretic numbers:
// SMALLIN0, small yellow number
static Texture *PatchINum[11];  // IN0, big yellow number
// FONTB16, big green numbers (of a font)
// FONTA16, medium silver numbers (of a font)

// Hexen:
// IN0 : medium yellow numbers with minus (NEGNUM)
// SMALLIN0: little yellow numbers
// INRED0 : medium red numbers, no minus
// FONTA01-59 : medium silver font
// FONTAY01-59 : like FONTA but yellow
// FONTB01-59 : large brown font

int playpalette;

int SpinBookLump; // frames 0-15 each
int SpinFlyLump;
int SpinSpeedLump;
int SpinDefenseLump;
int SpinMinotaurLump;

Texture *PatchFlight[16];
Texture *PatchBook[16];
Texture *PatchSpeed[16];
Texture *PatchDefense[16];
Texture *PatchMinotaur[16];

// Hexen extras
static Texture *PatchH2BAR;
static Texture *PatchH2TOP;
static Texture *PatchMana1[2];
static Texture *PatchMana2[2];
static Texture *PatchKILLS;

static const char ArtiPatchName[][10] =
{
  {"ARTIBOX"},    // none
  {"ARTIINVU"},   // invulnerability
  {"ARTIINVS"},   // invisibility
  {"ARTIPTN2"},   // health
  {"ARTISPHL"},   // superhealth
  {"ARTIPWBK"},   // tomeofpower
  {"ARTITRCH"},   // torch
  {"ARTIFBMB"},   // firebomb
  {"ARTIEGGC"},   // egg
  {"ARTISOAR"},   // fly
  {"ARTIATLP"},   // teleport

  {"ARTIHRAD"},   // healing radius
  {"ARTISUMN"},   // summon maulotaur
  {"ARTIPORK"},   // porkelator
  {"ARTIBLST"},   // blast radius
  {"ARTIPSBG"},   // poison bag
  {"ARTITELO"},   // teleport other
  {"ARTISPED"},   // speed
  {"ARTIBMAN"},   // boost mana
  {"ARTIBRAC"},   // boost armor

  {"ARTISKLL"},   // arti_puzzskull
  {"ARTIBGEM"},   // arti_puzzgembig
  {"ARTIGEMR"},   // arti_puzzgemred
  {"ARTIGEMG"},   // arti_puzzgemgreen1
  {"ARTIGMG2"},   // arti_puzzgemgreen2
  {"ARTIGEMB"},   // arti_puzzgemblue1
  {"ARTIGMB2"},   // arti_puzzgemblue2
  {"ARTIBOK1"},   // arti_puzzbook1
  {"ARTIBOK2"},   // arti_puzzbook2
  {"ARTISKL2"},   // arti_puzzskull2
  {"ARTIFWEP"},   // arti_puzzfweapon
  {"ARTICWEP"},   // arti_puzzcweapon
  {"ARTIMWEP"},   // arti_puzzmweapon
  {"ARTIGEAR"},   // arti_puzzgear1
  {"ARTIGER2"},   // arti_puzzgear2
  {"ARTIGER3"},   // arti_puzzgear3
  {"ARTIGER4"},   // arti_puzzgear4

  {"WFR1A0"},     // weapon pieces
  {"WFR2A0"},
  {"WFR3A0"},
  {"WCH1A0"},
  {"WCH2A0"},
  {"WCH3A0"},
  {"WMS1A0"},
  {"WMS2A0"},
  {"WMS3A0"}
};



static void ST_SetClassData(int num, int cls)
{
  if (game.mode == gm_hexen)
    {
      int base;
      /*
        PatchWEAPONSLOT = tc.CachePtrNum(fc.GetNumForName("WPSLOT0") + cls);
        PatchWEAPONFULL = tc.CachePtrNum(fc.GetNumForName("WPFULL0") + cls);
        PatchPIECE1     = tc.CachePtrNum(fc.GetNumForName("WPIECEF1") + cls);
        PatchPIECE2     = tc.CachePtrNum(fc.GetNumForName("WPIECEF2") + cls);
        PatchPIECE3     = tc.CachePtrNum(fc.GetNumForName("WPIECEF3") + cls);
      */

      Patch_ChainSlider[1] = tc.GetPtrNum(fc.GetNumForName("CHAIN") + cls);

      base = fc.GetNumForName("LIFEGEM");
      if (!game.multiplayer)
        // single player game uses red life gem
        Patch_ChainSlider[2] = tc.GetPtrNum(base + 4*cls + 1);
      else
        Patch_ChainSlider[2] = tc.GetPtrNum(base + 4*cls + num % 4);
    }
  else
    {
      // heretic
      if (!game.multiplayer)
        // single player game uses red life gem
        Patch_ChainSlider[2] = tc.GetPtr("LIFEGEM2");
      else
        Patch_ChainSlider[2] = tc.GetPtrNum(fc.GetNumForName("LIFEGEM0") + num % 4);
    }
}


void ST_LoadHexenData()
{
  int i;
  int startLump;

  PatchH2BAR = tc.GetPtr("H2BAR");
  PatchH2TOP = tc.GetPtr("H2TOP");

  PatchSTATBAR = tc.GetPtr("STATBAR");
  PatchKEYBAR = tc.GetPtr("KEYBAR");

  //PatchARTICLEAR = tc.GetPtr("ARTICLS");
  PatchARMCLEAR = tc.GetPtr("ARMCLS");
  //PatchMANACLEAR = tc.GetPtr("MANACLS");
  /*
    PatchMANAVIAL1 = tc.GetPtr("MANAVL1");
    PatchMANAVIAL2 = tc.GetPtr("MANAVL2");
    PatchMANAVIALDIM1 = tc.GetPtr("MANAVL1D");
    PatchMANAVIALDIM2 = tc.GetPtr("MANAVL2D");
  */
  PatchMana1[0] = tc.GetPtr("MANADIM1");
  PatchMana1[1] = tc.GetPtr("MANABRT1");
  PatchMana2[0] = tc.GetPtr("MANADIM2");
  PatchMana2[1] = tc.GetPtr("MANABRT2");

  sbohealth = tc.GetPtr("SBOHEALT"); //"PTN2A0"
  sbofrags  = tc.GetPtr("SBOFRAGS"); //"ARTISKLL"
  sboarmor  = tc.GetPtr("SBOARMOR"); //"ARM1A0"

  Patch_InvBar[0] = tc.GetPtr("INVBAR");
  Patch_InvBar[1] = tc.GetPtr("ARTIBOX");
  Patch_InvBar[2] = tc.GetPtr("SELECTBO");
  Patch_InvBar[3] = tc.GetPtr("INVGEML1");
  Patch_InvBar[4] = tc.GetPtr("INVGEML2");
  Patch_InvBar[5] = tc.GetPtr("INVGEMR1");
  Patch_InvBar[6] = tc.GetPtr("INVGEMR2");
  Patch_InvBar[7] = tc.GetPtr("BLACKSQ");

  // artifact inventory pics
  for (i=0; i <= NUMARTIFACTS; i++)
    PatchARTI[i] = tc.GetPtr(ArtiPatchName[i]);

  // artifact use flash
  startLump = fc.GetNumForName("USEARTIA");
  for (i=0; i<5; i++) Patch_InvBar[i+8] = tc.GetPtrNum(startLump + i);

  // ammo pics
  for (i=0; i < NUMAMMO; i++)
    PatchAmmoPic[i] = tc.GetPtr(DHAmmoPics[i]);

  // keys
  startLump = fc.GetNumForName("KEYSLOT1");
  for (i=0; i<11; i++)
    PatchKeys[i] = tc.GetPtrNum(startLump+i);

  // numbers
  startLump = fc.GetNumForName("IN0");
  for (i = 0; i < 10; i++)
    PatchINum[i] = tc.GetPtrNum(startLump+i);
  PatchINum[10] = tc.GetPtr("NEGNUM");

  // BNum
  startLump = fc.GetNumForName("FONTB16");
  for (i = 0; i < 10; i++)
    PatchBNum[i] = tc.GetPtrNum(startLump+i);
  PatchBNum[10] = tc.GetPtrNum(startLump-3); //("FONTB13")

  //SNum
  startLump = fc.GetNumForName("SMALLIN0");
  for (i = 0; i < 10; i++)
    PatchSNum[i] = tc.GetPtrNum(startLump+i);
  PatchSNum[10] = PatchSNum[0]; // no minus available

  playpalette = fc.GetNumForName("PLAYPAL");

  SpinFlyLump = fc.GetNumForName("SPFLY0");
  SpinSpeedLump = fc.GetNumForName("SPBOOT0");
  SpinDefenseLump = fc.GetNumForName("SPSHLD0");
  SpinMinotaurLump = fc.GetNumForName("SPMINO0");

  for (i=0; i<16; i++)
    {
      PatchFlight[i] = tc.GetPtrNum(SpinFlyLump + i);
      PatchSpeed[i] = tc.GetPtrNum(SpinSpeedLump + i);
      PatchDefense[i] = tc.GetPtrNum(SpinDefenseLump + i);
      PatchMinotaur[i] = tc.GetPtrNum(SpinMinotaurLump + i);
    }

  PatchKILLS = tc.GetPtr("KILLS");

  // health chain slider
  Patch_ChainSlider[0] = NULL;
  Patch_ChainSlider[3] = tc.GetPtr("LFEDGE");
  Patch_ChainSlider[4] = tc.GetPtr("RTEDGE");
  ST_SetClassData(0, 0);
}

void ST_LoadHereticData()
{
  int i;
  int startLump;

  // gargoyle eyes
  PatchGod[0] = tc.GetPtr("GOD1");
  PatchGod[1] = tc.GetPtr("GOD2");

  PatchBARBACK = tc.GetPtr("BARBACK");

  if (cv_deathmatch.value)
    PatchSTATBAR = tc.GetPtr("STATBAR");
  else
    PatchSTATBAR = tc.GetPtr("LIFEBAR");

  PatchLTFCTOP = tc.GetPtr("LTFCTOP");
  PatchRTFCTOP = tc.GetPtr("RTFCTOP");
  PatchARMCLEAR  = tc.GetPtr("ARMCLEAR");

  // inventory bar pics
  Patch_InvBar[0] = tc.GetPtr("INVBAR");
  Patch_InvBar[1] = tc.GetPtr("ARTIBOX");
  Patch_InvBar[2] = tc.GetPtr("SELECTBO");
  Patch_InvBar[3] = tc.GetPtr("INVGEML1");
  Patch_InvBar[4] = tc.GetPtr("INVGEML2");
  Patch_InvBar[5] = tc.GetPtr("INVGEMR1");
  Patch_InvBar[6] = tc.GetPtr("INVGEMR2");
  Patch_InvBar[7] = tc.GetPtr("BLACKSQ"); // useful?

  // artifact use flash
  startLump = fc.GetNumForName("USEARTIA");
  for (i=0; i<5; i++) Patch_InvBar[i+8] = tc.GetPtrNum(startLump + i);

  // artifact inventory pics
  for (i=0; i <= NUMARTIFACTS; i++)
    PatchARTI[i] = tc.GetPtr(ArtiPatchName[i]);

  // ammo pics
  //  for (i=0; i < am_heretic; i++)
  //  PatchAmmoPic[i] = NULL;
  //for (i=am_heretic; i <= NUMAMMO; i++)
  for (i = 0; i < NUMAMMO; i++)
    PatchAmmoPic[i] = tc.GetPtr(DHAmmoPics[i]);

  sbohealth = tc.GetPtr("SBOHEALT"); //"PTN2A0"
  sbofrags  = tc.GetPtr("SBOFRAGS"); //"FACEB1"
  sboarmor  = tc.GetPtr("SBOARMOR"); //"SHLDA0"

  // keys
  PatchKeys[11] = PatchKeys[14] = tc.GetPtr("BKEYICON");
  PatchKeys[12] = PatchKeys[15] = tc.GetPtr("YKEYICON");
  PatchKeys[13] = PatchKeys[16] = tc.GetPtr("GKEYICON");

  // health chain slider
  Patch_ChainSlider[0] = tc.GetPtr("CHAINBAC");
  Patch_ChainSlider[1] = tc.GetPtr("CHAIN");
  Patch_ChainSlider[3] = tc.GetPtr("LTFACE");
  Patch_ChainSlider[4] = tc.GetPtr("RTFACE");
  ST_SetClassData(0, 0);

  // INum
  startLump = fc.GetNumForName("IN0");
  for (i = 0; i < 10; i++)
    PatchINum[i] = tc.GetPtrNum(startLump+i);
  PatchINum[10] = tc.GetPtr("NEGNUM");
  // and "LAME"...

  // BNum
  startLump = fc.GetNumForName("FONTB16");
  for (i = 0; i < 10; i++)
    PatchBNum[i] = tc.GetPtrNum(startLump+i);
  PatchBNum[10] = tc.GetPtrNum(startLump-3); //("FONTB13")

  //SNum
  startLump = fc.GetNumForName("SMALLIN0");
  for (i = 0; i < 10; i++)
    PatchSNum[i] = tc.GetPtrNum(startLump+i);
  PatchSNum[10] = PatchSNum[0]; // no minus available

  playpalette = fc.GetNumForName("PLAYPAL");
  SpinBookLump = fc.GetNumForName("SPINBK0");
  SpinFlyLump = fc.GetNumForName("SPFLY0");

  for (i=0; i<16; i++)
    {
      PatchFlight[i] = tc.GetPtrNum(SpinFlyLump + i);
      PatchBook[i] = tc.GetPtrNum(SpinBookLump + i);
    }
}



// made separate so that skins code can reload custom face graphics
void ST_loadFaceGraphics (char *facestr)
{
  int   i,j;
  int   facenum;
  char  namelump[9];
  char* namebuf;

  //hack: make sure base face name is no more than 3 chars
  if (strlen(facestr)>3)
    facestr[3]='\0';
  strcpy (namelump, facestr);  // copy base name
  namebuf = namelump;
  while (*namebuf>' ') namebuf++;

  // face states
  facenum = 0;
  for (i=0;i<ST_NUMPAINFACES;i++)
    {
      for (j=0;j<ST_NUMSTRAIGHTFACES;j++)
        {
          sprintf(namebuf, "ST%d%d", i, j);
          PatchFaces[facenum++] = tc.GetPtr(namelump);
        }
      sprintf(namebuf, "TR%d0", i);        // turn right
      PatchFaces[facenum++] = tc.GetPtr(namelump);
      sprintf(namebuf, "TL%d0", i);        // turn left
      PatchFaces[facenum++] = tc.GetPtr(namelump);
      sprintf(namebuf, "OUCH%d", i);       // ouch!
      PatchFaces[facenum++] = tc.GetPtr(namelump);
      sprintf(namebuf, "EVL%d", i);        // evil grin ;)
      PatchFaces[facenum++] = tc.GetPtr(namelump);
      sprintf(namebuf, "KILL%d", i);       // pissed off
      PatchFaces[facenum++] = tc.GetPtr(namelump);
    }
  strcpy (namebuf, "GOD0");
  PatchFaces[facenum++] = tc.GetPtr(namelump);
  strcpy (namebuf, "DEAD0");
  PatchFaces[facenum++] = tc.GetPtr(namelump);

  // face backgrounds for different player colors
  //added:08-02-98: uses only STFB0, which is remapped to the right
  //                colors using the player translation tables, so if
  //                you add new player colors, it is automatically
  //                used for the statusbar.
  strcpy (namebuf, "B0");
  i = fc.FindNumForName(namelump);
  if (i != -1)
    PatchFaceBack = tc.GetPtrNum(i);
  else
    PatchFaceBack = tc.GetPtr("STFB0");

}


void ST_LoadDoomData()
{
  int  i;
  char namebuf[9];

  // Load the numbers, tall and short
  for (i=0; i<10; i++)
    {
      sprintf(namebuf, "STTNUM%d", i);
      PatchBNum[i] = tc.GetPtr(namebuf);

      sprintf(namebuf, "STYSNUM%d", i);
      PatchSNum[i] = tc.GetPtr(namebuf);
    }

  PatchBNum[10] = tc.GetPtr("STTMINUS");
  PatchSNum[10] = PatchSNum[0]; // no minus available

  // percent signs.
  tallpercent = tc.GetPtr("STTPRCNT");

  // key cards
  for (i=0; i<6; i++)
    {
      sprintf(namebuf, "STKEYS%d", i);
      PatchKeys[i+11] = tc.GetPtr(namebuf);
    }

  // arms background box
  PatchArmsBack = tc.GetPtr("STARMS");

  // arms ownership widgets
  for (i=0;i<6;i++)
    {
      sprintf(namebuf, "STGNUM%d", i+2);

      // gray #
      PatchArms[i][0] = tc.GetPtr(namebuf);

      // yellow #
      PatchArms[i][1] = PatchSNum[i+2];
    }

  // status bar background bits
  PatchSTATBAR = tc.GetPtr("STBAR");

  // the original Doom uses 'STF' as base name for all face graphics
  ST_loadFaceGraphics("STF");

  // ammo pics
  for (i = 0; i < NUMAMMO; i++)
    PatchAmmoPic[i] = tc.GetPtr(DHAmmoPics[i]);

  sbohealth = tc.GetPtr("SBOHEALT"); //"STIMA0"
  sbofrags  = tc.GetPtr("SBOFRAGS"); //"M_SKULL1"
  sboarmor  = tc.GetPtr("SBOARMOR"); //"ARM1A0"
}



// made separate so that skins code can reload custom face graphics
void ST_unloadFaceGraphics()
{
  for (int i=0;i<ST_NUMFACES;i++)
    PatchFaces[i]->Release();

  // face background
  PatchFaceBack->Release();
}



void ST_unloadData()
{
  int i;

  // unload the numbers, tall and short
  for (i=0;i<10;i++)
    {
      PatchBNum[i]->Release();
      PatchSNum[i]->Release();
    }
  // unload tall percent
  tallpercent->Release();

  // unload arms background
  PatchArmsBack->Release();

  // unload gray #'s
  for (i=0;i<6;i++)
    PatchArms[i][0]->Release();

  // unload the key cards
  for (i=0;i<NUMKEYS;i++)
    if (PatchKeys[i])
      PatchKeys[i]->Release();

  PatchSTATBAR->Release();

  ST_unloadFaceGraphics();
}




// refresh the status bar background
void HUD::ST_RefreshBackground()
{
  int flags = (fgbuffer & V_FLAGMASK) | BG;

  if (game.mode == gm_hexen)
    {
      PatchH2BAR->Draw(st_x, st_y-27, flags); // x=0, y = 134
      if (!automap.active)
	PatchSTATBAR->Draw(st_x+38, st_y+1, flags); // x=38, y=162
      else
	PatchKEYBAR->Draw(st_x+38, st_y+1, flags);
    }
  else if (game.mode == gm_heretic)
    {
      PatchBARBACK->Draw(st_x, st_y, flags);
      PatchSTATBAR->Draw(st_x+34, st_y+2, flags);
      //V_DrawScaledPatch(st_x+34, st_y+2, flags, PatchINVBAR);
      // background:
      //main
      PatchARMCLEAR->Draw(st_x+57, st_y+13, flags);
      //V_DrawScaledPatch(st_x+108, st_y+3, flags, PatchBLACKSQ);
      PatchARMCLEAR->Draw(st_x+224, st_y+13, flags);
    }
  else
    {
      // software mode copies patch to BG buffer,
      // hardware modes directly draw the statusbar to the screen
      PatchSTATBAR->Draw(st_x, st_y, flags);

      // draw the faceback for the statusbarplayer
      current_colormap = translationtables[st_pawncolor];
      PatchFaceBack->Draw(st_x+143, st_y, flags | V_MAP);
    }

  // copy the statusbar buffer to the screen
  if (rendermode == render_soft)
    V_CopyRect(0, vid.height-stbarheight, BG, vid.width, stbarheight, 0, vid.height-stbarheight, FG);
}


static int ST_calcPainOffset()
{
  static int  lastcalc;
  static int  oldhealth = -1;

  int health = (st_health > st_maxhealth) ? st_maxhealth : st_health;
  if (health < 0)
    health = 0;

  if (health != oldhealth)
    {
      lastcalc = ST_FACESTRIDE * (((st_maxhealth - health) * ST_NUMPAINFACES) / (st_maxhealth+1));
      oldhealth = health;
    }
  return lastcalc;
}


// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void HUD::ST_updateFaceWidget(const PlayerPawn *st_pawn)
{
  int         i;
  angle_t     badguyangle;
  angle_t     diffang;
  static int  lastattackdown = -1;
  static int  priority = 0;

  // count until face changes
  static int  st_facecount = 0;

  if (priority < 10)
    {
      // dead
      if (!st_pawn->health)
        {
          priority = 9;
          st_faceindex = ST_DEADFACE;
          st_facecount = 1;
        }
    }

  if (priority < 9)
    {
      if (bonuscount)
        {
          // picking up bonus
	  bool doevilgrin = false;

          for (i=0;i<NUMWEAPONS;i++)
	    if (st_oldweaponsowned[i] != st_pawn->weaponowned[i])
	      {
		doevilgrin = true;
		break;
	      }

          if (doevilgrin)
            {
              // evil grin if just picked up weapon
              priority = 8;
              st_facecount = ST_EVILGRINCOUNT;
              st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
            }
        }
    }

  if (priority < 8)
    {
      if (damagecount && st_pawn->attacker && st_pawn->attacker != st_pawn)
        {
          // being attacked
          priority = 7;

          if (st_pawn->health - st_oldhealth > ST_MUCHPAIN)
            {
              st_facecount = ST_TURNCOUNT;
              st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
          else
            {
              badguyangle = R_PointToAngle2(st_pawn->pos.x, st_pawn->pos.y,
                                            st_pawn->attacker->pos.x,
                                            st_pawn->attacker->pos.y);

              if (badguyangle > st_pawn->yaw)
                {
                  // whether right or left
                  diffang = badguyangle - st_pawn->yaw;
                  i = diffang > ANG180;
                }
              else
                {
                  // whether left or right
                  diffang = st_pawn->yaw - badguyangle;
                  i = diffang <= ANG180;
                } // confusing, aint it?


              st_facecount = ST_TURNCOUNT;
              st_faceindex = ST_calcPainOffset();

              if (diffang < ANG45)
                {
                  // head-on
                  st_faceindex += ST_RAMPAGEOFFSET;
                }
              else if (i)
                {
                  // turn face right
                  st_faceindex += ST_TURNOFFSET;
                }
              else
                {
                  // turn face left
                  st_faceindex += ST_TURNOFFSET+1;
                }
            }
        }
    }

  if (priority < 7)
    {
      // getting hurt because of your own damn stupidity
      if (damagecount)
        {
          if (st_pawn->health - st_oldhealth > ST_MUCHPAIN)
            {
              priority = 7;
              st_facecount = ST_TURNCOUNT;
              st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
          else
            {
              priority = 6;
              st_facecount = ST_TURNCOUNT;
              st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
            }
        }
    }

  if (priority < 6)
    {
      // rapid firing
      if (st_pawn->attackdown)
        {
          if (lastattackdown==-1)
            lastattackdown = ST_RAMPAGEDELAY;
          else if (!--lastattackdown)
            {
              priority = 5;
              st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
              st_facecount = 1;
              lastattackdown = 1;
            }
        }
      else
        lastattackdown = -1;
    }

  if (priority < 5)
    {
      // invulnerability
      if ((st_pawn->cheats & CF_GODMODE)
          || st_pawn->powers[pw_invulnerability])
        {
          priority = 4;

          st_faceindex = ST_GODFACE;
          st_facecount = 1;
        }
    }

  // look left or look right if the facecount has timed out
  if (!st_facecount)
    {
      st_faceindex = ST_calcPainOffset() + (M_Random() % 3);
      st_facecount = ST_STRAIGHTFACECOUNT;
      priority = 0;
    }

  st_facecount--;
}


void HUD::UpdateWidgets()
{
  // if no target, don't update
  if (!st_active || !st_player)
    return;

  PlayerPawn *st_pawn = st_player->pawn;

  if (!st_pawn || st_pawn->eflags & MFE_REMOVE) // TEST
    {
      // it will be deleted soon
      return;
    }

  if (st_pawn->powers[pw_strength])
    st_berzerk = 12 - (st_pawn->powers[pw_strength]>>6);  // slowly fade the berzerk out??? FIXME it's on/off!
  else
    st_berzerk = 0;

  st_radiation = (st_pawn->powers[pw_ironfeet] > BLINKTHRESHOLD || st_pawn->powers[pw_ironfeet] & 8);

  const int largeammo = 1994; // means "n/a"
  int i;

  statusbar_on = (cv_viewsize.value < 11) || automap.active;

  // status bar overlay at viewsize 11
  overlay_on = (cv_viewsize.value == 11);
  if (overlay_on)
    fgbuffer |= V_TL;
  else
    fgbuffer &= ~V_TL;

  // when pawn is detached from player, no more update
  if (st_pawn->player)
    st_fragscount = st_pawn->player->score;

  st_notdeathmatch = !cv_deathmatch.value;

  st_godmode = (st_pawn->cheats & CF_GODMODE);

  st_pawncolor = st_pawn->color;
  st_health = st_pawn->health;

  if (game.mode == gm_heretic || game.mode == gm_hexen)
    {
      if (st_player->invTics)
        invopen = true;
      else
        invopen = false;

      mainbar_on = statusbar_on && !invopen && !automap.active;

      // inventory
      if (itemuse > 0)
        itemuse--;

      int n = st_pawn->inventory.size();

      // kind of a HACK
      if (st_player->invSlot >= n)
	st_player->invSlot = 0;

      if (st_player->invSlot < st_player->invPos)
	st_player->invPos = st_player->invSlot;

      st_curpos = st_player->invPos;
      int left = st_player->invSlot - st_curpos; // how many slots are there left of the first visible slot?
      for (i=0; i<7; i++)
        if (i+left < n && st_pawn->inventory[left+i].type != arti_none)
          st_invslots[i] = st_pawn->inventory[left+i];
        else
          st_invslots[i] = inventory_t(arti_none, 0);

      st_invslots[7].type = (left > 0) ? 1 : 0; // hack
      st_invslots[7].count = (n - left > 7) ? 1 : 0;

      int frame = (game.tic/3) & 15;
      // flight icon
      if (st_pawn->powers[pw_flight] > BLINKTHRESHOLD || (st_pawn->powers[pw_flight] & 16))
        st_flight = frame;
          // TODO stop the spinning when not in air?
          // if (st_pawn->flags2 & MF2_FLY)
      else
        st_flight = -1;

      // book icon
      if ((st_pawn->powers[pw_weaponlevel2] > BLINKTHRESHOLD || (st_pawn->powers[pw_weaponlevel2] & 16)) && !st_pawn->morphTics)
        st_book = frame;
      else
        st_book = -1;

      // speed icon
      if (st_pawn->powers[pw_speed] > BLINKTHRESHOLD || (st_pawn->powers[pw_speed] & 16))
        st_speed = frame;
      else
        st_speed = -1;

      // defense icon
      if (st_pawn->powers[pw_invulnerability] > BLINKTHRESHOLD || (st_pawn->powers[pw_invulnerability] & 16))
        st_defense = frame;
      else
        st_defense = -1;

      // minotaur icon
      if (st_pawn->powers[pw_minotaur] > BLINKTHRESHOLD || (st_pawn->powers[pw_minotaur] & 16))
        st_minotaur = frame;
      else
        st_minotaur = -1;

      st_mana1 = st_pawn->ammo[am_mana1];
      st_mana2 = st_pawn->ammo[am_mana2];
      st_mana1icon = (st_mana1 > 0);
      st_mana2icon = (st_mana2 > 0);
    }
  else
    {
      // doom
      for (i=0; i<NUMAMMO; i++)
        {
          st_ammo[i] = st_pawn->ammo[i];
          st_maxammo[i] = st_pawn->maxammo[i];
        }

      // refresh everything if this is him coming back to life
      ST_updateFaceWidget(st_pawn); // updates st_oldweaponsowned
      for (i=0; i<NUMWEAPONS; i++)
	st_oldweaponsowned[i] = st_pawn->weaponowned[i];

      st_oldhealth = st_health;
    }

  st_armor = int(100 * st_pawn->toughness);
  for (i=0; i<NUMARMOR; i++)
    st_armor += st_pawn->armorpoints[i];

  if (game.mode == gm_hexen)
    st_armor /= 5; // "AC"

  st_readywp = st_pawn->readyweapon;

  st_atype = st_pawn->weaponinfo[st_pawn->readyweapon].ammo;
  if (st_atype == am_noammo)
    st_readywp_ammo = largeammo;
  else if (st_atype == am_manaboth)
    st_readywp_ammo = min(st_pawn->ammo[am_mana1], st_pawn->ammo[am_mana2]);
  else
    st_readywp_ammo = st_pawn->ammo[st_atype];

  // update keycard widgets
  for (i=0; i<NUMKEYS; i++)
    st_keyboxes[i] = (st_pawn->keycards & (1 << i)) ? i : -1;
}

// sets the new palette based upon current values of damagecount
// and bonuscount
void HUD::PaletteFlash()
{
  int palette;
  int dcount = damagecount;

  if (st_berzerk > dcount)
    dcount = st_berzerk;

  if (poisoncount)
    {
      palette = (poisoncount + 7) >> 3;
      if (palette >= NUMPOISONPALS)
        palette = NUMPOISONPALS-1;

      palette += STARTPOISONPALS;
    }
  else if (dcount)
    {
      palette = (dcount + 7) >> 3;

      if (palette >= NUMREDPALS)
        palette = NUMREDPALS-1;

      palette += STARTREDPALS;
    }
  else if (bonuscount)
    {
      palette = (bonuscount+7)>>3;

      if (palette >= NUMBONUSPALS)
        palette = NUMBONUSPALS-1;

      palette += STARTBONUSPALS;
    }
  else if (st_radiation)
    palette = RADIATIONPAL; // not relevant in heretic
  /*
  else if (sbpawn->flags2 & MF2_ICEDAMAGE)
    { // TODO Frozen player
      palette = STARTICEPAL;
    }
  */
  else
    palette = 0;

  if (palette != st_palette)
    {
      st_palette = palette;

#ifdef HWRENDER
      if (rendermode != render_soft)
        {
          switch (palette)
            {
            case 0x00: State::SetGlobalColor(0.0f, 0.0f, 0.0f, 0.0f, State::NONE); break;  // pas de changement
            case 0x01: State::SetGlobalColor(0.59f, 0.21f, 0.21f, 1.0f); break; // red
            case 0x02: State::SetGlobalColor(0.59f, 0.21f, 0.21f, 1.0f); break; // red
            case 0x03: State::SetGlobalColor(0.65f, 0.18f, 0.18f, 1.0f); break; // red
            case 0x04: State::SetGlobalColor(0.71f, 0.15f, 0.15f, 1.0f); break; // red
            case 0x05: State::SetGlobalColor(0.78f, 0.12f, 0.12f, 1.0f); break; // red
            case 0x06: State::SetGlobalColor(0.84f, 0.09f, 0.09f, 1.0f); break; // red
            case 0x07: State::SetGlobalColor(0.90f, 0.06f, 0.06f, 1.0f); break; // red
            case 0x08: State::SetGlobalColor(0.97f, 0.03f, 0.03f, 1.0f); break; // red
            case 0x09: State::SetGlobalColor(0.37f, 0.37f, 1.00f, 1.0f); break; // blue
            case 0x0a: State::SetGlobalColor(0.56f, 0.62f, 0.44f, 1.0f); break; // light green
            case 0x0b: State::SetGlobalColor(0.59f, 0.69f, 0.40f, 1.0f); break; // light green
            case 0x0c: State::SetGlobalColor(0.62f, 0.75f, 0.37f, 1.0f); break; // light green
            case 0x0d: State::SetGlobalColor(0.37f, 1.00f, 0.37f, 1.0f); break; // green
            case 0x0e: State::SetGlobalColor(0.37f, 0.37f, 1.00f, 1.0f); break; // blue
            case 0x0f: State::SetGlobalColor(0.37f, 0.37f, 1.00f, 1.0f); break; // blue
            }
        }
      else
#endif
        {
	  vid.SetPalette(palette);
        }
    }
}



void HUD::ST_Recalc()
{
  switch (game.mode)
    {
    case gm_hexen:
      stbarheight = ST_HEIGHT_HEXEN;
      break;
    case gm_heretic:
      stbarheight = ST_HEIGHT_HERETIC;
      break;
    default:
      stbarheight = ST_HEIGHT_DOOM;
      break;
    }

  if (cv_scalestatusbar.value || cv_viewsize.value > 10)
    {
      fgbuffer = FG | V_SCALE; // scale patch by default

#ifdef HWRENDER
      if (rendermode != render_soft)
        {
          st_x = 0;
          st_y = BASEVIDHEIGHT - stbarheight;

	  stbarheight = int(stbarheight * vid.fdupy); // real height
        }
      else
#endif
        {
          st_x = (vid.width - ST_WIDTH * vid.dupx) / (2 * vid.dupx);
          st_y = (vid.height - stbarheight * vid.dupy) / vid.dupy;

	  stbarheight *= vid.dupy; // real height
        }
    }
  else
    {
      fgbuffer = FG;
      st_x = (vid.width - ST_WIDTH) >> 1;
      st_y = vid.height - stbarheight;
    }

  // TODO not good. When should the widgets be created?
  // and renew the widgets
  ST_CreateWidgets();
}


void HUD::CreateHexenWidgets()
{
  HudWidget *h;

  // spinning icons
  h = new HudMultIcon(st_x+20, 19, &st_flight, PatchFlight);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+60, 19, &st_speed, PatchSpeed);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+260, 19, &st_defense, PatchDefense);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+300, 19, &st_minotaur, PatchMinotaur);
  statusbar.push_back(h);

  // gargoyle wings
  h = new HudBinIcon(st_x, st_y-27, &st_true, NULL, PatchH2TOP);
  statusbar.push_back(h);

  // health slider
  h = new HexenHudSlider(st_x, st_y+32, &st_health, 0, 100, Patch_ChainSlider);
  statusbar.push_back(h);

  // mainbar (closed inventory shown)
  // frags / health
  if (cv_deathmatch.value)
    //V_DrawPatch(38, 162, PatchKILLS);
    h = new HudNumber(st_x+50, st_y+16, 3, &st_fragscount, PatchINum);
  else
    // TODO: use red numbers if health is low
    h = new HudNumber(st_x+65, st_y+16, 3, &st_health, PatchINum);
  mainbar.push_back(h);

  // mana
  h = new HudNumber(st_x+92, st_y+21, 3, &st_mana1, PatchSNum);
  mainbar.push_back(h);
  h = new HudBinIcon(st_x+77, st_y+4, &st_mana1icon, PatchMana1[0], PatchMana1[1]);
  mainbar.push_back(h);

  h = new HudNumber(st_x+124, st_y+21, 3, &st_mana2, PatchSNum);
  mainbar.push_back(h);
  h = new HudBinIcon(st_x+110, st_y+4, &st_mana2icon, PatchMana2[0], PatchMana2[1]);
  mainbar.push_back(h);
  // TODO mana vials (new widget type?)

  // armor
  h = new HudNumber(st_x+275, st_y+16, 3, &st_armor, PatchINum);
  mainbar.push_back(h);

  // inventory system
  h = new HexenHudInventory(st_x+38, st_y-1, &invopen, &itemuse, st_invslots, &st_curpos,
			    PatchSNum, PatchARTI, Patch_InvBar);
  statusbar.push_back(h);

  // TODO Weapon Pieces
  int i;

  // Keybar (in map screen, keys and armor pieces)
  for (i=0; i<5; i++)
    {
      h = new HudMultIcon(st_x+46+i*20, st_y+3, &st_keyboxes[i], PatchKeys);
      keybar.push_back(h);
    }

  for (i=1; i<NUMARMOR; i++)
    {
      /*
      // TODO fading icons: 0:nothing, <= ArmInc[cl][i]>>2:fuzz, <= ArmInc[cl][i]>>1:altfuzz, otherwise normal
      h = new HudFadeIcon(st_x+119+i*31, st_y+3, &st_armorboxes[i], "ARMSLOT1"+i-1);
      keybar.push_back(h);
      */
    }
}


void HUD::CreateHereticWidgets()
{
  // statusbar_on, mainbar_on, invopen

  int i;
  HudWidget *h;

  h = new HudMultIcon(st_x+20, 20, &st_flight, PatchFlight);
  statusbar.push_back(h);

  h = new HudMultIcon(st_x+300, 20, &st_book, PatchBook);
  statusbar.push_back(h);

  // godmode indicators
  h = new HudBinIcon(st_x+16, st_y+9, &st_godmode, NULL, PatchGod[0]);
  statusbar.push_back(h);
  h = new HudBinIcon(st_x+287, st_y+9, &st_godmode, NULL, PatchGod[1]);
  statusbar.push_back(h);

  // gargoyle horns
  h = new HudBinIcon(st_x, st_y-10, &st_true, NULL, PatchLTFCTOP);
  statusbar.push_back(h);
  h = new HudBinIcon(st_x+290, st_y-10, &st_true, NULL, PatchRTFCTOP);
  statusbar.push_back(h);

  // health slider
  h = new HudSlider(st_x, st_y+32, &st_health, 0, 100, Patch_ChainSlider);
  statusbar.push_back(h);

  // inventory system
  h = new HudInventory(st_x+34, st_y+1, &invopen, &itemuse, st_invslots, &st_curpos,
                       PatchSNum, PatchARTI, Patch_InvBar);
  statusbar.push_back(h);

  // mainbar (closed inventory shown)
  // frags / health
  if (cv_deathmatch.value)
    h = new HudNumber(st_x+61+27, st_y+12, 3, &st_fragscount, PatchINum);
  else
    h = new HudNumber(st_x+61+27, st_y+12, 3, &st_health, PatchINum);
  mainbar.push_back(h);

  // Keys
  const int ST_KEYY[3] = {22, 6, 14};
  for (i=0; i<6; i++)
    {
      h = new HudMultIcon(st_x+153, st_y+ST_KEYY[i%3], &st_keyboxes[i+11], PatchKeys);
      mainbar.push_back(h);
    }

  // readyweapon ammo
  h = new HudNumber(st_x + 109 + 27, st_y + 4, 3, &st_readywp_ammo, PatchINum);
  mainbar.push_back(h);

  // ammo type icon
  h = new HudMultIcon(st_x + 111, st_y + 14, &st_atype, PatchAmmoPic);
  mainbar.push_back(h);

  // armor
  h = new HudNumber(st_x+228+27, st_y+12, 3, &st_armor, PatchINum);
  mainbar.push_back(h);
}

/*
  DOOM widgets:
  readyweapon ammo
  frags
  ammo * 4
  maxammo * 4
  health
  armor
  armsbg
  weapons owned * 6
  keycards * 3 (6!)
  face
 */

void HUD::CreateDoomWidgets()
{
  int i;
  HudWidget *h;

  // ready weapon ammo
  h = new HudNumber(st_x+44, st_y+3, 3, &st_readywp_ammo, PatchBNum);
  statusbar.push_back(h);

  // ammo count and maxammo (all four kinds)
  const int ST_AMMOY[4] = {5, 11, 23, 17};
  for (i=0; i<4; i++)
    {
      h = new HudNumber(st_x+288, st_y + ST_AMMOY[i], 3, &st_ammo[i], PatchSNum);
      statusbar.push_back(h);

      h = new HudNumber(st_x+314, st_y + ST_AMMOY[i], 3, &st_maxammo[i], PatchSNum);
      statusbar.push_back(h);
    }

  // health percentage
  h = new HudPercent(st_x+90, st_y+3, &st_health, PatchBNum, tallpercent);
  statusbar.push_back(h);

  // armor percentage - should be colored later
  h = new HudPercent(st_x+221, st_y+3, &st_armor, PatchBNum, tallpercent);
  statusbar.push_back(h);

  // Weapons

  if (cv_deathmatch.value)
    {
      // frags
      h = new HudNumber(st_x+138, st_y+3, 2, &st_fragscount, PatchBNum);
      statusbar.push_back(h);
    }
  else
    {
      // weapons owned
      for (i=0; i<6; i++)
	{
	  // these are shown
	  const int wsel[] = {wp_pistol, wp_shotgun, wp_chaingun, wp_missile, wp_plasma, wp_bfg};
	  h = new HudBinIcon(st_x+111+(i%3)*12, st_y+4+(i/3)*10, &st_oldweaponsowned[wsel[i]],
			     PatchArms[i][0], PatchArms[i][1]);
	  statusbar.push_back(h);
	}
    }

  // arms background
  h = new HudBinIcon(st_x+104, st_y, &st_notdeathmatch, NULL, PatchArmsBack);
  statusbar.push_back(h);

  // face
  h = new HudMultIcon(st_x+143, st_y, &st_faceindex, PatchFaces);
  statusbar.push_back(h);

  // Key icon positions.
  const int ST_KEYY[3] = {3, 13, 23};
  for (i=0; i<6; i++)
    {
      h = new HudMultIcon(st_x+239+(i/3)*10, st_y+ST_KEYY[i%3], &st_keyboxes[i+11], PatchKeys);
      statusbar.push_back(h);
    }
}


void HUD::ST_CreateWidgets()
{
  CreateOverlayWidgets();

  for (int i = statusbar.size()-1; i>=0; i--)
    delete statusbar[i];
  statusbar.clear();

  for (int i = mainbar.size()-1; i>=0; i--)
    delete mainbar[i];
  mainbar.clear();

  for (int i = keybar.size()-1; i>=0; i--)
    delete keybar[i];
  keybar.clear();

  switch (game.mode)
    {
    case gm_hexen:
      CreateHexenWidgets();
      break;
    case gm_heretic:
      CreateHereticWidgets();
      break;
    default:
      CreateDoomWidgets();
      break;
    }

  st_refresh = true;
}


void HUD::ST_Drawer(bool refresh)
{
  int i;

  if (!st_active)
    return;

  //st_refresh = st_refresh || refresh;
  st_refresh = true;

  // Do red-/gold-shifts from damage/items
  PaletteFlash();

  // is either statusbar or overlay on?
  if (!(statusbar_on || overlay_on))
    return;

  // and draw them
  if (statusbar_on)
    {
      // after ST_Start(), screen refresh needed, or vid mode change
      if (st_refresh)
        {
          // draw status bar background to off-screen buff
          ST_RefreshBackground();
        }

      for (i = statusbar.size()-1; i>=0; i--)
	statusbar[i]->Update(st_refresh);

      if (mainbar_on)
	for (i = mainbar.size()-1; i>=0; i--)
	  mainbar[i]->Update(st_refresh);
      else if (automap.active)
	for (i = keybar.size()-1; i>=0; i--)
	  keybar[i]->Update(st_refresh);

      st_refresh = false;
    }
  else
    {
      if (!drawscore || cv_splitscreen.value)
	for (i = overlay.size()-1; i>=0; i--)
	  overlay[i]->Update(true);
    }
}



// stops the status bar, "detaches" it from a playerpawn

void HUD::ST_Stop()
{
  if (!st_active)
    return;

  st_player = NULL;
  vid.SetPalette(0);

  st_active = false;
}


// sets up status bar to follow pawn p
// 'link' the statusbar display to a player, which could be
// another player than consoleplayer, for example, when you
// change the view in a multiplayer demo with F12.

void HUD::ST_Start(PlayerInfo *p)
{
  int i;

  if (st_active)
    ST_Stop();

  st_player = p;

  st_palette = -1;

  if (game.mode == gm_heretic || game.mode == gm_hexen)
    {
      //ST_SetClassData(pawndata[p->ptype].pclass, p->number); TODO FIXME
    }
  else
    {
      // Doom
      // face initialization
      st_faceindex = 0;
      st_oldhealth = -1;
      st_maxhealth = 100; //p->maxhealth / 2;

      for (i=0;i<NUMWEAPONS;i++)
        st_oldweaponsowned[i] = false; //sbpawn->weaponowned[i]; FIXME too
    }

  st_active = true;
}



//=========================================================================
//                         STATUS BAR OVERLAY
//=========================================================================


// recreates the overlay widget set based on the consvar
void HUD::CreateOverlayWidgets()
{
  const char *cmds = cv_stbaroverlay.str;
  HudWidget *h;

  for (int i = overlay.size()-1; i>=0; i--)
    delete overlay[i];
  overlay.clear();

  for (char c = *cmds++; c; c = *cmds++)
    {
      if (c >= 'A' && c <= 'Z')
        c = c + 'a' - 'A';

      switch (c)
        {
        case 'i': // inventory
          h = new HudInventory(st_x+34, st_y+9, &invopen, &itemuse, st_invslots, &st_curpos,
                               PatchSNum, PatchARTI, Patch_InvBar);
          overlay.push_back(h);
          break;

        case 'h': // draw health
          h = new HudNumber(st_x+70, st_y+14, 3, &st_health, PatchBNum);
          overlay.push_back(h);
          h = new HudBinIcon(st_x+72, st_y+14, &st_true, NULL, sbohealth);
          overlay.push_back(h);
          break;

        case 'a': // draw ammo
          h = new HudNumber(st_x+170, st_y+14, 3, &st_readywp_ammo, PatchBNum);
          overlay.push_back(h);
          h = new HudMultIcon(st_x+172, st_y+14, &st_atype, PatchAmmoPic);
          overlay.push_back(h);
          break;

	case 'm': // draw armor
	  h = new HudNumber(st_x+270, st_y+14, 3, &st_armor, PatchBNum);
	  overlay.push_back(h);
	  h = new HudBinIcon(st_x+272, st_y+14, &st_true, NULL, sboarmor);
	  overlay.push_back(h);
	  break;

        case 'k': // draw keys
          for (int i=0; i<6; i++)
            {
              h = new HudMultIcon(st_x+308-(i/3)*10, st_y+22-(i%3)*10, &st_keyboxes[i+11], PatchKeys);
              overlay.push_back(h);
            }
          for (int i=0; i<11; i++)
            {
              h = new HudMultIcon(st_x+4+(i/6)*296, 25+(i%6)*30, &st_keyboxes[i], PatchKeys);
              overlay.push_back(h);
            }
          break;

        case 'f': // draw frags
          h = new HudNumber(st_x+300, 2, 3, &st_fragscount, PatchBNum);
          overlay.push_back(h);
          h = new HudBinIcon(st_x+302, 2, &st_true, NULL, sbofrags);
          overlay.push_back(h);
          break;


        default:
          break;
           /*
             //TODO
        case 'e': // number of monster killed
          if ((!cv_deathmatch.value) && (!cv_splitscreen.value))
            {
              char buf[16];
              sprintf(buf, "%d/%d", 0, 0); // FIXME! should be sbpawn.kills, map.kills
              V_DrawString(SCX(318-V_StringWidth(buf)), SCY(1), V_NOSCALESTART, buf);

            }
          break;

        case 's': // number of secrets found
          if ((!cv_deathmatch.value) && (!cv_splitscreen.value))
            {
              char buf[16];
              sprintf(buf, "%d/%d", 0, 0); // FIXME! should be sbpawn., map.secrets
              V_DrawString(SCX(318-V_StringWidth(buf)), SCY(11), V_NOSCALESTART, buf);
            }
          break;
          case 'r': // current frame rate
          {
          char buf[8];
          int framerate = 35;
          sprintf(buf, "%d FPS", framerate);
          V_DrawString(SCX(2), SCY(4), V_NOSCALESTART, buf);
          }
          break;
          */
        }
    }
}
