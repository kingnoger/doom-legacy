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
// Revision 1.5  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.4  2003/03/15 20:07:12  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.3  2003/02/16 16:54:49  smite-meister
// L2 sound cache done
//
// Revision 1.2  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.1.1.1  2002/11/16 14:17:48  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      music/sound tables, and related sound routines
//
// Note: the tables were originally created by a sound utility at Id,
//       kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------


#include "sounds.h"
#include "z_zone.h"
#include "w_wad.h"


// Doom/Heretic music names corresponding to musicenum_t
// NOTE: padded with \0 to stringlen=8, to allow dehacked patching

char* MusicNames[NUMMUSIC] =
{
    { NULL },
    // Doom
    { "D_E1M1\0\0" },
    { "D_E1M2\0\0" },
    { "D_E1M3\0\0" },
    { "D_E1M4\0\0" },
    { "D_E1M5\0\0" },
    { "D_E1M6\0\0" },
    { "D_E1M7\0\0" },
    { "D_E1M8\0\0" },
    { "D_E1M9\0\0" },

    { "D_E2M1\0\0" },
    { "D_E2M2\0\0" },
    { "D_E2M3\0\0" },
    { "D_E2M4\0\0" },
    { "D_E2M5\0\0" },
    { "D_E2M6\0\0" },
    { "D_E2M7\0\0" },
    { "D_E2M8\0\0" },
    { "D_E2M9\0\0" },

    { "D_E3M1\0\0" },
    { "D_E3M2\0\0" },
    { "D_E3M3\0\0" },
    { "D_E3M4\0\0" },
    { "D_E3M5\0\0" },
    { "D_E3M6\0\0" },
    { "D_E3M7\0\0" },
    { "D_E3M8\0\0" },
    { "D_E3M9\0\0" },
    // Ultimate Doom Ep. 4 re-used music
    { "D_E3M4\0\0" },
    { "D_E3M2\0\0" },
    { "D_E3M3\0\0" },
    { "D_E1M5\0\0" },
    { "D_E2M7\0\0" },
    { "D_E2M4\0\0" },
    { "D_E2M6\0\0" },
    { "D_E2M5\0\0" },
    { "D_E1M9\0\0" },

    { "D_INTER\0"  },
    { "D_INTRO\0"  },
    { "D_BUNNY\0"  },
    { "D_VICTOR"   },
    { "D_INTROA"   },
    // Doom II
    { "D_RUNNIN"   },
    { "D_STALKS"   },
    { "D_COUNTD"   },
    { "D_BETWEE"   },
    { "D_DOOM\0\0" },
    { "D_THE_DA"   },
    { "D_SHAWN\0"  },
    { "D_DDTBLU"   },
    { "D_IN_CIT"   },
    { "D_DEAD\0\0" },
    { "D_STLKS2"   },
    { "D_THEDA2"   },
    { "D_DOOM2\0"  },
    { "D_DDTBL2"   },
    { "D_RUNNI2"   },
    { "D_DEAD2\0"  },
    { "D_STLKS3"   },
    { "D_ROMERO"   },
    { "D_SHAWN2"   },
    { "D_MESSAG"   },
    { "D_COUNT2"   },
    { "D_DDTBL3"   },
    { "D_AMPIE\0"  },
    { "D_THEDA3"   },
    { "D_ADRIAN"   },
    { "D_MESSG2"   },
    { "D_ROMER2"   },
    { "D_TENSE\0"  },
    { "D_SHAWN3"   },
    { "D_OPENIN"   },
    { "D_EVIL\0\0" },
    { "D_ULTIMA"   },
    { "D_READ_M"   },
    { "D_DM2TTL"   },
    { "D_DM2INT"   },
    // Heretic
    { "MUS_E1M1" }, // 1-1
    { "MUS_E1M2" },
    { "MUS_E1M3" },
    { "MUS_E1M4" },
    { "MUS_E1M5" },
    { "MUS_E1M6" },
    { "MUS_E1M7" },
    { "MUS_E1M8" },
    { "MUS_E1M9" },

    { "MUS_E2M1" }, // 2-1
    { "MUS_E2M2" },
    { "MUS_E2M3" },
    { "MUS_E2M4" },
    { "MUS_E1M4" },
    { "MUS_E2M6" },
    { "MUS_E2M7" },
    { "MUS_E2M8" },
    { "MUS_E2M9" },

    { "MUS_E1M1" }, // 3-1
    { "MUS_E3M2" },
    { "MUS_E3M3" },
    { "MUS_E1M6" },
    { "MUS_E1M3" },
    { "MUS_E1M2" },
    { "MUS_E1M5" },
    { "MUS_E1M9" },
    { "MUS_E2M6" },

    { "MUS_E1M6" }, // 4-1
    { "MUS_E1M2" },
    { "MUS_E1M3" },
    { "MUS_E1M4" },
    { "MUS_E1M5" },
    { "MUS_E1M1" },
    { "MUS_E1M7" },
    { "MUS_E1M8" },
    { "MUS_E1M9" },

    { "MUS_E2M1" }, // 5-1
    { "MUS_E2M2" },
    { "MUS_E2M3" },
    { "MUS_E2M4" },
    { "MUS_E1M4" },
    { "MUS_E2M6" },
    { "MUS_E2M7" },
    { "MUS_E2M8" },
    { "MUS_E2M9" },

    { "MUS_E3M2" }, // 6-1
    { "MUS_E3M3" }, // 6-2
    { "MUS_E1M6" }, // 6-3

    { "MUS_TITL" },
    { "MUS_INTR" },
    { "MUS_CPTD" }
};


//
// Information about all the sfx
//

sfxinfo_t S_sfx[NUMSFX] =
{
  // S_sfx[0] needs to be a dummy for odd reasons.
//              multiplicity
//   tagname             |  priority(U)
//     |    lumpname     |    |
  { NULL, "none"       , 0,   0},
  { NULL, "dspistol"   , 0,  64},
  { NULL, "dsshotgn"   , 0,  64},
  { NULL, "dssgcock"   , 0,  64},
  { NULL, "dsdshtgn"   , 0,  64},
  { NULL, "dsdbopn\0"  , 0,  64},
  { NULL, "dsdbcls\0"  , 0,  64},
  { NULL, "dsdbload"   , 0,  64},
  { NULL, "dsplasma"   , 0,  64},
  { NULL, "dsbfg\0\0\0", 0,  64},

  { NULL, "dssawup\0"  , 0,  64},
  { NULL, "dssawidl"   , 0, 118},
  { NULL, "dssawful"   , 0,  64},
  { NULL, "dssawhit"   , 0,  64},
  { NULL, "dsrlaunc"   , 0,  64},
  { NULL, "dsrxplod"   , 0,  70},
  { NULL, "dsfirsht"   , 0,  70},
  { NULL, "dsfirxpl"   , 0,  70},
  { NULL, "dspstart"   , 0, 100},
  { NULL, "dspstop\0"  , 0, 100},

  { NULL, "dsdoropn"   , 0, 100},
  { NULL, "dsdorcls"   , 0, 100},
  { NULL, "dsstnmov"   , 0, 119},
  { NULL, "dsswtchn"   , 0,  78},
  { NULL, "dsswtchx"   , 0,  78},
  { NULL, "dsplpain"   , 0,  96},   //SKSPLPAIN},
  { NULL, "dsdmpain"   , 0,  96},
  { NULL, "dspopain"   , 0,  96},
  { NULL, "dsvipain"   , 0,  96},
  { NULL, "dsmnpain"   , 0,  96},

  { NULL, "dspepain"   , 0,  96},
  { NULL, "dsslop\0\0" , 0,  78},    //SKSSLOP},
  { NULL, "dsitemup"   ,  1,  78},
  { NULL, "dswpnup"    ,  1,  78},
  { NULL, "dsoof\0\0\0", 0,  96},    //SKSOOF},
  { NULL, "dstelept"   , 0,  32},
  { NULL, "dsposit1"   ,  1,  98},
  { NULL, "dsposit2"   ,  1,  98},
  { NULL, "dsposit3"   ,  1,  98},
  { NULL, "dsbgsit1"   ,  1,  98},

  { NULL, "dsbgsit2"   ,  1,  98},
  { NULL, "dssgtsit"   ,  1,  98},
  { NULL, "dscacsit"   ,  1,  98},
  { NULL, "dsbrssit"   ,  1,  94},
  { NULL, "dscybsit"   ,  1,  92},
  { NULL, "dsspisit"   ,  1,  90},
  { NULL, "dsbspsit"   ,  1,  90},
  { NULL, "dskntsit"   ,  1,  90},
  { NULL, "dsvilsit"   ,  1,  90},
  { NULL, "dsmansit"   ,  1,  90},

  { NULL, "dspesit\0"  ,  1,  90},
  { NULL, "dssklatk"   , 0,  70},
  { NULL, "dssgtatk"   , 0,  70},
  { NULL, "dsskepch"   , 0,  70},
  { NULL, "dsvilatk"   , 0,  70},
  { NULL, "dsclaw\0\0" , 0,  70},
  { NULL, "dsskeswg"   , 0,  70},
  { NULL, "dspldeth"   , 0,  32}, //SKSPLDETH},
  { NULL, "dspdiehi"   , 0,  32}, //SKSPDIEHI},
  { NULL, "dspodth1"   , 0,  70},

  { NULL, "dspodth2"   , 0,  70},
  { NULL, "dspodth3"   , 0,  70},
  { NULL, "dsbgdth1"   , 0,  70},
  { NULL, "dsbgdth2"   , 0,  70},
  { NULL, "dssgtdth"   , 0,  70},
  { NULL, "dscacdth"   , 0,  70},
  { NULL, "dsskldth"   , 0,  70},
  { NULL, "dsbrsdth"   , 0,  32},
  { NULL, "dscybdth"   , 0,  32},
  { NULL, "dsspidth"   , 0,  32},

  { NULL, "dsbspdth"   , 0,  32},
  { NULL, "dsvildth"   , 0,  32},
  { NULL, "dskntdth"   , 0,  32},
  { NULL, "dspedth\0"  , 0,  32},
  { NULL, "dsskedth"   , 0,  32},
  { NULL, "dsposact"   ,  1, 120},
  { NULL, "dsbgact\0"  ,  1, 120},
  { NULL, "dsdmact\0"  ,  1, 120},
  { NULL, "dsbspact"   ,  1, 100},
  { NULL, "dsbspwlk"   ,  1, 100},

  { NULL, "dsvilact"   ,  1, 100},
  { NULL, "dsnoway\0"  , 0,  78}, //SKSNOWAY},
  { NULL, "dsbarexp"   , 0,  60},
  { NULL, "dspunch\0"  , 0,  64}, //SKSPUNCH},
  { NULL, "dshoof\0\0" , 0,  70},
  { NULL, "dsmetal\0"  , 0,  70},
  { NULL, "dspistol"  , 0,  64}, // FIXME wrong pitch for chaingun... "dschgun\0", &S_sfx[sfx_pistol], 150, 0},
  { NULL, "dstink\0\0" , 0,  60},
  { NULL, "dsbdopn\0"  , 0, 100},
  { NULL, "dsbdcls\0"  , 0, 100},

  { NULL, "dsitmbk\0"  , 0, 100},
  { NULL, "dsflame\0"  , 0,  32},
  { NULL, "dsflamst"   , 0,  32},
  { NULL, "dsgetpow"   , 0,  60},
  { NULL, "dsbospit"   , 0,  70},
  { NULL, "dsboscub"   , 0,  70},
  { NULL, "dsbossit"   , 0,  70},
  { NULL, "dsbospn\0"  , 0,  70},
  { NULL, "dsbosdth"   , 0,  70},
  { NULL, "dsmanatk"   , 0,  70},

  { NULL, "dsmandth"   , 0,  70},
  { NULL, "dssssit\0"  , 0,  70},
  { NULL, "dsssdth\0"  , 0,  70},
  { NULL, "dskeenpn"   , 0,  70},
  { NULL, "dskeendt"   , 0,  70},
  { NULL, "dsskeact"   , 0,  70},
  { NULL, "dsskesit"   , 0,  70},
  { NULL, "dsskeatk"   , 0,  70},
  { NULL, "dsradio\0"  , 0,  60}, //SKSRADIO},

  // legacy.wad extra sounds
  // sound when the player avatar jumps in air 'hmpf!'
  { NULL, "dsjump\0\0" , 0,  60}, //SKSJUMP},

  { NULL, "dsouch\0\0" , 0,  64}, //SKSOUCH}, // 110
  // water sounds
  { NULL, "dsgloop\0"  , 0,  60},
  { NULL, "dssplash"   , 0,  64},
  { NULL, "dsfloush"   , 0,  64},

  // Heretic
  { NULL, "gldhit",  0, 32},
  { NULL, "gntful",  0, 32},
  { NULL, "gnthit",  0, 32},
  { NULL, "gntpow",  0, 32},
  { NULL, "gntact",  0, 32},
  { NULL, "gntuse",  0, 32},

  { NULL, "phosht",  0, 32},
  { NULL, "phohit",  0, 32},
  { NULL, "hedat1", 0, 32}, // "-phopow", &S_sfx[sfx_hedat1], -1, -1},
  { NULL, "lobsht",  0, 20},
  { NULL, "lobhit",  0, 20},
  { NULL, "lobpow",  0, 20},
  { NULL, "hrnsht",  0, 32},
  { NULL, "hrnhit",  0, 32},
  { NULL, "hrnpow",  0, 32},
  { NULL, "ramphit", 0, 32},

  { NULL, "ramrain", 0, 10},
  { NULL, "bowsht",  0, 32},
  { NULL, "stfhit",  0, 32},
  { NULL, "stfpow",  0, 32},
  { NULL, "stfcrk",  0, 32},
  { NULL, "impsit",  0, 32},
  { NULL, "impat1",  0, 32},
  { NULL, "impat2",  0, 32},
  { NULL, "impdth",  0, 80},
  { NULL, "impsit", 0, 20}, //"-impact", &S_sfx[sfx_impsit], -1, -1},

  { NULL, "imppai",  0, 32},
  { NULL, "mumsit",  0, 32},
  { NULL, "mumat1",  0, 32},
  { NULL, "mumat2",  0, 32},
  { NULL, "mumdth",  0, 80},
  { NULL, "mumsit", 0, 20}, //"-mumact" &S_sfx[sfx_mumsit], -1, -1},
  { NULL, "mumpai",  0, 32},
  { NULL, "mumhed",  0, 32},
  { NULL, "bstsit",  0, 32},
  { NULL, "bstatk",  0, 32},

  { NULL, "bstdth",  0, 80},
  { NULL, "bstact",  0, 20},
  { NULL, "bstpai",  0, 32},
  { NULL, "clksit",  0, 32},
  { NULL, "clkatk",  0, 32},
  { NULL, "clkdth",  0, 80},
  { NULL, "clkact",  0, 20},
  { NULL, "clkpai",  0, 32},
  { NULL, "snksit",  0, 32},
  { NULL, "snkatk",  0, 32},

  { NULL, "snkdth",  0, 80},
  { NULL, "snkact",  0, 20},
  { NULL, "snkpai",  0, 32},
  { NULL, "kgtsit",  0, 32},
  { NULL, "kgtatk",  0, 32},
  { NULL, "kgtat2",  0, 32},
  { NULL, "kgtdth",  0, 80},
  { NULL, "kgtsit", 0, 20}, // "-kgtact", &S_sfx[sfx_kgtsit], -1, -1},
  { NULL, "kgtpai",  0, 32},
  { NULL, "wizsit",  0, 32},

  { NULL, "wizatk",  0, 32},
  { NULL, "wizdth",  0, 80},
  { NULL, "wizact",  0, 20},
  { NULL, "wizpai",  0, 32},
  { NULL, "minsit",  0, 32},
  { NULL, "minat1",  0, 32},
  { NULL, "minat2",  0, 32},
  { NULL, "minat3",  0, 32},
  { NULL, "mindth",  0, 80},
  { NULL, "minact",  0, 20},

  { NULL, "minpai",  0, 32},
  { NULL, "hedsit",  0, 32},
  { NULL, "hedat1",  0, 32},
  { NULL, "hedat2",  0, 32},
  { NULL, "hedat3",  0, 32},
  { NULL, "heddth",  0, 80},
  { NULL, "hedact",  0, 20},
  { NULL, "hedpai",  0, 32},
  { NULL, "sorzap",  0, 32},
  { NULL, "sorrise", 0, 32},

  { NULL, "sorsit",  0, 200},
  { NULL, "soratk",  0, 32},
  { NULL, "soract",  0, 200},
  { NULL, "sorpai",  0, 200},
  { NULL, "sordsph", 0, 200},
  { NULL, "sordexp", 0, 200},
  { NULL, "sordbon", 0, 200},
  { NULL, "bstsit", 0, 32}, // "-sbtsit", &S_sfx[sfx_bstsit], -1, -1},
  { NULL, "bstatk", 0, 32}, // "-sbtatk", &S_sfx[sfx_bstatk], -1, -1},
  { NULL, "sbtdth",  0, 80},

  { NULL, "sbtact",  0, 20},
  { NULL, "sbtpai",  0, 32},
  { NULL, "plroof",  0, 32},
  { NULL, "plrpai",  0, 32},
  { NULL, "plrdth",  0, 80},
  { NULL, "gibdth",  0, 100},
  { NULL, "plrwdth", 0, 80},
  { NULL, "plrcdth", 0, 100},
  { NULL, "itemup",  0, 32},
  { NULL, "wpnup",   0, 32},

  { NULL, "telept",  0, 50},
  { NULL, "doropn",  0, 40},
  { NULL, "dorcls",  0, 40},
  { NULL, "dormov",  0, 40},
  { NULL, "artiup",  0, 32},
  { NULL, "switch",  0, 40},
  { NULL, "pstart",  0, 40},
  { NULL, "pstop",   0, 40},
  { NULL, "stnmov",  0, 40},
  { NULL, "chicpai", 0, 32},

  { NULL, "chicatk", 0, 32},
  { NULL, "chicdth", 0, 40},
  { NULL, "chicact", 0, 32},
  { NULL, "chicpk1", 0, 32},
  { NULL, "chicpk2", 0, 32},
  { NULL, "chicpk3", 0, 32},
  { NULL, "keyup"  , 0, 50},
  { NULL, "ripslop", 0, 16},
  { NULL, "newpod" , 0, 16},
  { NULL, "podexp" , 0, 40},

  { NULL, "bounce" , 0, 16},
  { NULL, "bstatk", 0, 16}, // "-volsht", &S_sfx[sfx_bstatk], -1, -1},
  { NULL, "lobhit", 0, 16}, // "-volhit", &S_sfx[sfx_lobhit], -1, -1},
  { NULL, "burn"   , 0, 10},
  { NULL, "splash" , 0, 10},
  { NULL, "gloop"  , 0, 10},
  { NULL, "respawn", 0, 10},
  { NULL, "blssht" , 0, 32},
  { NULL, "blshit" , 0, 32},
  { NULL, "chat"   , 0, 100},

  { NULL, "artiuse", 0, 32},
  { NULL, "gfrag"  , 0, 100},
  { NULL, "waterfl", 0, 16},

  // Monophonic sounds
  { NULL, "wind"   , 0, 16},
  { NULL, "amb1"   , 0,  1},
  { NULL, "amb2"   , 0,  1},
  { NULL, "amb3"   , 0,  1},
  { NULL, "amb4"   , 0,  1},
  { NULL, "amb5"   , 0,  1},
  { NULL, "amb6"   , 0,  1},

  { NULL, "amb7"   , 0,  1},
  { NULL, "amb8"   , 0,  1},
  { NULL, "amb9"   , 0,  1},
  { NULL, "amb10"  , 0,  1},
  { NULL, "amb11"  , 0,  1},


  // Hexen
  // tagname, lumpname, multiplicity, priority
  { "PlayerFighterNormalDeath", "", 2, 256},
  { "PlayerFighterCrazyDeath", "", 2, 256},
  { "PlayerFighterExtreme1Death", "", 2, 256},
  { "PlayerFighterExtreme2Death", "", 2, 256},
  { "PlayerFighterExtreme3Death", "", 2, 256},

  { "PlayerFighterBurnDeath", "", 2, 256},
  { "PlayerClericNormalDeath", "", 2, 256},
  { "PlayerClericCrazyDeath", "", 2, 256},
  { "PlayerClericExtreme1Death", "", 2, 256},
  { "PlayerClericExtreme2Death", "", 2, 256},
  { "PlayerClericExtreme3Death", "", 2, 256},
  { "PlayerClericBurnDeath", "", 2, 256},
  { "PlayerMageNormalDeath", "", 2, 256},
  { "PlayerMageCrazyDeath", "", 2, 256},
  { "PlayerMageExtreme1Death", "", 2, 256},

  { "PlayerMageExtreme2Death", "", 2, 256},
  { "PlayerMageExtreme3Death", "", 2, 256},
  { "PlayerMageBurnDeath", "", 2, 256},
  { "PlayerFighterPain", "", 2, 256},
  { "PlayerClericPain", "", 2, 256},
  { "PlayerMagePain", "", 2, 256},
  { "PlayerFighterGrunt", "", 2, 256},
  { "PlayerClericGrunt", "", 2, 256},
  { "PlayerMageGrunt", "", 2, 256},
  { "PlayerLand", "", 2, 32},

  { "PlayerPoisonCough", "", 2, 256},
  { "PlayerFighterFallingScream", "", 2, 256},
  { "PlayerClericFallingScream", "", 2, 256},
  { "PlayerMageFallingScream", "", 2, 256},
  { "PlayerFallingSplat", "", 2, 256},
  { "PlayerFighterFailedUse", "", 1, 256},
  { "PlayerClericFailedUse", "", 1, 256},
  { "PlayerMageFailedUse", "", 1, 256},
  { "PlatformStart", "", 2, 36},
  { "PlatformStartMetal", "", 2, 36},

  { "PlatformStop", "", 2, 40},
  { "StoneMove", "", 2, 32},
  { "MetalMove", "", 2, 32},
  { "DoorOpen", "", 2, 36},
  { "DoorLocked", "", 2, 36},
  { "DoorOpenMetal", "", 2, 36},
  { "DoorCloseMetal", "", 2, 36},
  { "DoorCloseLight", "", 2, 36},
  { "DoorCloseHeavy", "", 2, 36},
  { "DoorCreak", "", 2, 36},

  { "PickupWeapon", "", 2, 36},
  { "PickupArtifact", "", 2, 36},
  { "PickupKey", "", 2, 36},
  { "PickupItem", "", 2, 36},
  { "PickupPiece", "", 2, 36},
  { "WeaponBuild", "", 2, 36},
  { "UseArtifact", "", 2, 36},
  { "BlastRadius", "", 2, 36},
  { "Teleport", "", 2, 256},
  { "ThunderCrash", "", 2, 30},

  { "FighterPunchMiss", "", 2, 80},
  { "FighterPunchHitThing", "", 2, 80},
  { "FighterPunchHitWall", "", 2, 80},
  { "FighterGrunt", "", 2, 80},
  { "FighterAxeHitThing", "", 2, 80},
  { "FighterHammerMiss", "", 2, 80},
  { "FighterHammerHitThing", "", 2, 80},
  { "FighterHammerHitWall", "", 2, 80},
  { "FighterHammerContinuous", "", 2, 32},
  { "FighterHammerExplode", "", 2, 80},

  { "FighterSwordFire", "", 2, 80},
  { "FighterSwordExplode", "", 2, 80},
  { "ClericCStaffFire", "", 2, 80},
  { "ClericCStaffExplode", "", 2, 40},
  { "ClericCStaffHitThing", "", 2, 80},
  { "ClericFlameFire", "", 2, 80},
  { "ClericFlameExplode", "", 2, 80},
  { "ClericFlameCircle", "", 2, 80},
  { "MageWandFire", "", 2, 80},
  { "MageLightningFire", "", 2, 80},

  { "MageLightningZap", "", 2, 32},
  { "MageLightningContinuous", "", 2, 32},
  { "MageLightningReady", "", 2, 30},
  { "MageShardsFire","", 2, 80},
  { "MageShardsExplode","", 2, 36},
  { "MageStaffFire","", 2, 80},
  { "MageStaffExplode","", 2, 40},
  { "Switch1", "", 2, 32},
  { "Switch2", "", 2, 32},
  { "SerpentSight", "", 2, 32},

  { "SerpentActive", "", 2, 32},
  { "SerpentPain", "", 2, 32},
  { "SerpentAttack", "", 2, 32},
  { "SerpentMeleeHit", "", 2, 32},
  { "SerpentDeath", "", 2, 40},
  { "SerpentBirth", "", 2, 32},
  { "SerpentFXContinuous", "", 2, 32},
  { "SerpentFXHit", "", 2, 32},
  { "PotteryExplode", "", 2, 32},
  { "Drip", "", 2, 32},

  { "CentaurSight", "", 2, 32},
  { "CentaurActive", "", 2, 32},
  { "CentaurPain", "", 2, 32},
  { "CentaurAttack", "", 2, 32},
  { "CentaurDeath", "", 2, 40},
  { "CentaurLeaderAttack", "", 2, 32},
  { "CentaurMissileExplode", "", 2, 32},
  { "Wind", "", 2, 1},
  { "BishopSight", "", 2, 32},
  { "BishopActive", "", 2, 32},

  { "BishopPain", "", 2, 32},
  { "BishopAttack", "", 2, 32},
  { "BishopDeath", "", 2, 40},
  { "BishopMissileExplode", "", 2, 32},
  { "BishopBlur", "", 2, 32},
  { "DemonSight", "", 2, 32},
  { "DemonActive", "", 2, 32},
  { "DemonPain", "", 2, 32},
  { "DemonAttack", "", 2, 32},
  { "DemonMissileFire", "", 2, 32},

  { "DemonMissileExplode", "", 2, 32},
  { "DemonDeath", "", 2, 40},
  { "WraithSight", "", 2, 32},
  { "WraithActive", "", 2, 32},
  { "WraithPain", "", 2, 32},
  { "WraithAttack", "", 2, 32},
  { "WraithMissileFire", "", 2, 32},
  { "WraithMissileExplode", "", 2, 32},
  { "WraithDeath", "", 2, 40},
  { "PigActive1", "", 2, 32},

  { "PigActive2", "", 2, 32},
  { "PigPain", "", 2, 32},
  { "PigAttack", "", 2, 32},
  { "PigDeath", "", 2, 40},
  { "MaulatorSight", "", 2, 32},
  { "MaulatorActive", "", 2, 32},
  { "MaulatorPain", "", 2, 32},
  { "MaulatorHamSwing", "", 2, 32},
  { "MaulatorHamHit", "", 2, 32},
  { "MaulatorMissileHit", "", 2, 32},

  { "MaulatorDeath", "", 2, 40},
  { "FreezeDeath", "", 2, 40},
  { "FreezeShatter", "", 2, 40},
  { "EttinSight", "", 2, 32},
  { "EttinActive", "", 2, 32},
  { "EttinPain", "", 2, 32},
  { "EttinAttack", "", 2, 32},
  { "EttinDeath", "", 2, 40},
  { "FireDemonSpawn", "", 2, 32},
  { "FireDemonActive", "", 2, 32},

  { "FireDemonPain", "", 2, 32},
  { "FireDemonAttack", "", 2, 32},
  { "FireDemonMissileHit", "", 2, 32},
  { "FireDemonDeath", "", 2, 40},
  { "IceGuySight", "", 2, 32},
  { "IceGuyActive", "", 2, 32},
  { "IceGuyAttack", "", 2, 32},
  { "IceGuyMissileExplode", "", 2, 32},
  { "SorcererSight", "", 2, 256},
  { "SorcererActive", "", 2, 256},

  { "SorcererPain", "", 2, 256},
  { "SorcererSpellCast", "", 2, 256},
  { "SorcererBallWoosh", "", 4, 256},
  { "SorcererDeathScream", "", 2, 256},
  { "SorcererBishopSpawn", "", 2, 80},
  { "SorcererBallPop", "", 2, 80},
  { "SorcererBallBounce", "", 3, 80},
  { "SorcererBallExplode", "", 3, 80},
  { "SorcererBigBallExplode", "", 3, 80},
  { "SorcererHeadScream", "", 2, 256},

  { "DragonSight", "", 2, 64},
  { "DragonActive", "", 2, 64},
  { "DragonWingflap", "", 2, 64},
  { "DragonAttack", "", 2, 64},
  { "DragonPain", "", 2, 64},
  { "DragonDeath", "", 2, 64},
  { "DragonFireballExplode", "", 2, 32},
  { "KoraxSight", "", 2, 256},
  { "KoraxActive", "", 2, 256},
  { "KoraxPain", "", 2, 256},

  { "KoraxAttack", "", 2, 256},
  { "KoraxCommand", "", 2, 256},
  { "KoraxDeath", "", 2, 256},
  { "KoraxStep", "", 2, 128},
  { "ThrustSpikeRaise", "", 2, 32},
  { "ThrustSpikeLower", "", 2, 32},
  { "GlassShatter", "", 2, 32},
  { "FlechetteBounce", "", 2, 32},
  { "FlechetteExplode", "", 2, 32},
  { "LavaMove", "", 2, 36},

  { "WaterMove", "", 2, 36},
  { "IceStartMove", "", 2, 36},
  { "EarthStartMove", "", 2, 36},
  { "WaterSplash", "", 2, 32},
  { "LavaSizzle", "", 2, 32},
  { "SludgeGloop", "", 2, 32},
  { "HolySymbolFire", "", 2, 64},
  { "SpiritActive", "", 2, 32},
  { "SpiritAttack", "", 2, 32},
  { "SpiritDie", "", 2, 32},

  { "ValveTurn", "", 2, 36},
  { "RopePull", "", 2, 36},
  { "FlyBuzz", "", 2, 20},
  { "Ignite", "", 2, 32},
  { "PuzzleSuccess", "", 2, 256},
  { "PuzzleFailFighter", "", 2, 256},
  { "PuzzleFailCleric", "", 2, 256},
  { "PuzzleFailMage", "", 2, 256},
  { "Earthquake", "", 2, 32},
  { "BellRing", "", 2, 32},

  { "TreeBreak", "", 2, 32},
  { "TreeExplode", "", 2, 32},
  { "SuitofArmorBreak", "", 2, 32},
  { "PoisonShroomPain", "", 2, 20},
  { "PoisonShroomDeath", "", 2, 32},
  { "Ambient1", "", 1, 1},
  { "Ambient2", "", 1, 1},
  { "Ambient3", "", 1, 1},
  { "Ambient4", "", 1, 1},
  { "Ambient5", "", 1, 1},

  { "Ambient6", "", 1, 1},
  { "Ambient7", "", 1, 1},
  { "Ambient8", "", 1, 1},
  { "Ambient9", "", 1, 1},
  { "Ambient10", "", 1, 1},
  { "Ambient11", "", 1, 1},
  { "Ambient12", "", 1, 1},
  { "Ambient13", "", 1, 1},
  { "Ambient14", "", 1, 1},
  { "Ambient15", "", 1, 1},

  { "StartupTick", "", 2, 32},
  { "SwitchOtherLevel", "", 2, 32},
  { "Respawn", "", 2, 32},
  { "KoraxVoiceGreetings", "", 1, 512}, // originally Korax speeches had multiplicity 2
  { "KoraxVoiceReady", "", 1, 512},
  { "KoraxVoiceBlood", "", 1, 512},
  { "KoraxVoiceGame", "", 1, 512},
  { "KoraxVoiceBoard", "", 1, 512},
  { "KoraxVoiceWorship", "", 1, 512},
  { "KoraxVoiceMaybe", "", 1, 512},

  { "KoraxVoiceStrong", "", 1, 512},
  { "KoraxVoiceFace", "", 1, 512},
  { "BatScream", "", 2, 32},
  { "Chat", "", 2, 512},
  { "MenuMove", "", 2, 32},
  { "ClockTick", "", 2, 32},
  { "Fireball", "", 2, 32},
  { "PuppyBeat", "", 2, 30},
  { "MysticIncant", "", 4, 32} // 498
};



// parses the Hexen SNDINFO lump
void S_Read_SNDINFO(int lump)
{
  int i, j;

  int length = fc.LumpLength(lump);
  char *ms = (char *)fc.CacheLumpNum(lump, PU_STATIC);
  char *me = ms + length; // past-the-end pointer

  char *s, *p;
  s = p = ms;
  char tag[41], lname[17];

  while (p < me)
    {
      if (*p == '\n') // line ends
	{
	  if (p > s)
	    {
	      // parse the line from s to p
	      *p = '\0';  // mark the line end

	      i = sscanf(s, "%40s", tag); // pass whitespace, read first word
	      if (i != EOF && tag[0] != ';')
		// not a blank line or comment
		if (!strcmp(tag, "$MAP"))
		  {
		    // $ARCHIVEPATH ignored
		    i = sscanf(s, "%*40s %d %16s", &j, lname);
		    if (i == 2  && j >= 1 && j <= 99)
		      ;// FIXME store map music
		  }
		else
		  {
		    // must be a tagname => lumpname mapping
		    for(i = sfx_Hexen; i < NUMSFX; i++)
		      if (!strcmp(S_sfx[i].tagname, tag))
			{
			  i = sscanf(s, "%*40s %16s", lname);
			  if (i == 2)
			    {
			      if (lname[0] == '?')
				strcpy(S_sfx[i].lumpname, "default");
			      else
				strncpy(S_sfx[i].lumpname, lname, 8);
			    }
			  break;
			}
		  }
	    }
	  s = p + 1;  // pass the line
	}
      p++;      
    }

  Z_Free(ms);

  // unmapped tags get the default sound
  for (i = sfx_Hexen; i < NUMSFX; i++)
    if (!strcmp(S_sfx[i].lumpname, ""))
      strcpy(S_sfx[i].lumpname, "default");
}
