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
// Revision 1.7  2003/04/14 08:58:23  smite-meister
// Hexen maps load.
//
// Revision 1.6  2003/04/08 09:46:05  smite-meister
// Bugfixes
//
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
  { "PlayerFighterNormalDeath", "\0", 2, 256},
  { "PlayerFighterCrazyDeath", "\0", 2, 256},
  { "PlayerFighterExtreme1Death", "\0", 2, 256},
  { "PlayerFighterExtreme2Death", "\0", 2, 256},
  { "PlayerFighterExtreme3Death", "\0", 2, 256},

  { "PlayerFighterBurnDeath", "\0", 2, 256},
  { "PlayerClericNormalDeath", "\0", 2, 256},
  { "PlayerClericCrazyDeath", "\0", 2, 256},
  { "PlayerClericExtreme1Death", "\0", 2, 256},
  { "PlayerClericExtreme2Death", "\0", 2, 256},
  { "PlayerClericExtreme3Death", "\0", 2, 256},
  { "PlayerClericBurnDeath", "\0", 2, 256},
  { "PlayerMageNormalDeath", "\0", 2, 256},
  { "PlayerMageCrazyDeath", "\0", 2, 256},
  { "PlayerMageExtreme1Death", "\0", 2, 256},

  { "PlayerMageExtreme2Death", "\0", 2, 256},
  { "PlayerMageExtreme3Death", "\0", 2, 256},
  { "PlayerMageBurnDeath", "\0", 2, 256},
  { "PlayerFighterPain", "\0", 2, 256},
  { "PlayerClericPain", "\0", 2, 256},
  { "PlayerMagePain", "\0", 2, 256},
  { "PlayerFighterGrunt", "\0", 2, 256},
  { "PlayerClericGrunt", "\0", 2, 256},
  { "PlayerMageGrunt", "\0", 2, 256},
  { "PlayerLand", "\0", 2, 32},

  { "PlayerPoisonCough", "\0", 2, 256},
  { "PlayerFighterFallingScream", "\0", 2, 256},
  { "PlayerClericFallingScream", "\0", 2, 256},
  { "PlayerMageFallingScream", "\0", 2, 256},
  { "PlayerFallingSplat", "\0", 2, 256},
  { "PlayerFighterFailedUse", "\0", 1, 256},
  { "PlayerClericFailedUse", "\0", 1, 256},
  { "PlayerMageFailedUse", "\0", 1, 256},
  { "PlatformStart", "\0", 2, 36},
  { "PlatformStartMetal", "\0", 2, 36},

  { "PlatformStop", "\0", 2, 40},
  { "StoneMove", "\0", 2, 32},
  { "MetalMove", "\0", 2, 32},
  { "DoorOpen", "\0", 2, 36},
  { "DoorLocked", "\0", 2, 36},
  { "DoorOpenMetal", "\0", 2, 36},
  { "DoorCloseMetal", "\0", 2, 36},
  { "DoorCloseLight", "\0", 2, 36},
  { "DoorCloseHeavy", "\0", 2, 36},
  { "DoorCreak", "\0", 2, 36},

  { "PickupWeapon", "\0", 2, 36},
  { "PickupArtifact", "\0", 2, 36},
  { "PickupKey", "\0", 2, 36},
  { "PickupItem", "\0", 2, 36},
  { "PickupPiece", "\0", 2, 36},
  { "WeaponBuild", "\0", 2, 36},
  { "UseArtifact", "\0", 2, 36},
  { "BlastRadius", "\0", 2, 36},
  { "Teleport", "\0", 2, 256},
  { "ThunderCrash", "\0", 2, 30},

  { "FighterPunchMiss", "\0", 2, 80},
  { "FighterPunchHitThing", "\0", 2, 80},
  { "FighterPunchHitWall", "\0", 2, 80},
  { "FighterGrunt", "\0", 2, 80},
  { "FighterAxeHitThing", "\0", 2, 80},
  { "FighterHammerMiss", "\0", 2, 80},
  { "FighterHammerHitThing", "\0", 2, 80},
  { "FighterHammerHitWall", "\0", 2, 80},
  { "FighterHammerContinuous", "\0", 2, 32},
  { "FighterHammerExplode", "\0", 2, 80},

  { "FighterSwordFire", "\0", 2, 80},
  { "FighterSwordExplode", "\0", 2, 80},
  { "ClericCStaffFire", "\0", 2, 80},
  { "ClericCStaffExplode", "\0", 2, 40},
  { "ClericCStaffHitThing", "\0", 2, 80},
  { "ClericFlameFire", "\0", 2, 80},
  { "ClericFlameExplode", "\0", 2, 80},
  { "ClericFlameCircle", "\0", 2, 80},
  { "MageWandFire", "\0", 2, 80},
  { "MageLightningFire", "\0", 2, 80},

  { "MageLightningZap", "\0", 2, 32},
  { "MageLightningContinuous", "\0", 2, 32},
  { "MageLightningReady", "\0", 2, 30},
  { "MageShardsFire","\0", 2, 80},
  { "MageShardsExplode","\0", 2, 36},
  { "MageStaffFire","\0", 2, 80},
  { "MageStaffExplode","\0", 2, 40},
  { "Switch1", "\0", 2, 32},
  { "Switch2", "\0", 2, 32},
  { "SerpentSight", "\0", 2, 32},

  { "SerpentActive", "\0", 2, 32},
  { "SerpentPain", "\0", 2, 32},
  { "SerpentAttack", "\0", 2, 32},
  { "SerpentMeleeHit", "\0", 2, 32},
  { "SerpentDeath", "\0", 2, 40},
  { "SerpentBirth", "\0", 2, 32},
  { "SerpentFXContinuous", "\0", 2, 32},
  { "SerpentFXHit", "\0", 2, 32},
  { "PotteryExplode", "\0", 2, 32},
  { "Drip", "\0", 2, 32},

  { "CentaurSight", "\0", 2, 32},
  { "CentaurActive", "\0", 2, 32},
  { "CentaurPain", "\0", 2, 32},
  { "CentaurAttack", "\0", 2, 32},
  { "CentaurDeath", "\0", 2, 40},
  { "CentaurLeaderAttack", "\0", 2, 32},
  { "CentaurMissileExplode", "\0", 2, 32},
  { "Wind", "\0", 2, 1},
  { "BishopSight", "\0", 2, 32},
  { "BishopActive", "\0", 2, 32},

  { "BishopPain", "\0", 2, 32},
  { "BishopAttack", "\0", 2, 32},
  { "BishopDeath", "\0", 2, 40},
  { "BishopMissileExplode", "\0", 2, 32},
  { "BishopBlur", "\0", 2, 32},
  { "DemonSight", "\0", 2, 32},
  { "DemonActive", "\0", 2, 32},
  { "DemonPain", "\0", 2, 32},
  { "DemonAttack", "\0", 2, 32},
  { "DemonMissileFire", "\0", 2, 32},

  { "DemonMissileExplode", "\0", 2, 32},
  { "DemonDeath", "\0", 2, 40},
  { "WraithSight", "\0", 2, 32},
  { "WraithActive", "\0", 2, 32},
  { "WraithPain", "\0", 2, 32},
  { "WraithAttack", "\0", 2, 32},
  { "WraithMissileFire", "\0", 2, 32},
  { "WraithMissileExplode", "\0", 2, 32},
  { "WraithDeath", "\0", 2, 40},
  { "PigActive1", "\0", 2, 32},

  { "PigActive2", "\0", 2, 32},
  { "PigPain", "\0", 2, 32},
  { "PigAttack", "\0", 2, 32},
  { "PigDeath", "\0", 2, 40},
  { "MaulatorSight", "\0", 2, 32},
  { "MaulatorActive", "\0", 2, 32},
  { "MaulatorPain", "\0", 2, 32},
  { "MaulatorHamSwing", "\0", 2, 32},
  { "MaulatorHamHit", "\0", 2, 32},
  { "MaulatorMissileHit", "\0", 2, 32},

  { "MaulatorDeath", "\0", 2, 40},
  { "FreezeDeath", "\0", 2, 40},
  { "FreezeShatter", "\0", 2, 40},
  { "EttinSight", "\0", 2, 32},
  { "EttinActive", "\0", 2, 32},
  { "EttinPain", "\0", 2, 32},
  { "EttinAttack", "\0", 2, 32},
  { "EttinDeath", "\0", 2, 40},
  { "FireDemonSpawn", "\0", 2, 32},
  { "FireDemonActive", "\0", 2, 32},

  { "FireDemonPain", "\0", 2, 32},
  { "FireDemonAttack", "\0", 2, 32},
  { "FireDemonMissileHit", "\0", 2, 32},
  { "FireDemonDeath", "\0", 2, 40},
  { "IceGuySight", "\0", 2, 32},
  { "IceGuyActive", "\0", 2, 32},
  { "IceGuyAttack", "\0", 2, 32},
  { "IceGuyMissileExplode", "\0", 2, 32},
  { "SorcererSight", "\0", 2, 256},
  { "SorcererActive", "\0", 2, 256},

  { "SorcererPain", "\0", 2, 256},
  { "SorcererSpellCast", "\0", 2, 256},
  { "SorcererBallWoosh", "\0", 4, 256},
  { "SorcererDeathScream", "\0", 2, 256},
  { "SorcererBishopSpawn", "\0", 2, 80},
  { "SorcererBallPop", "\0", 2, 80},
  { "SorcererBallBounce", "\0", 3, 80},
  { "SorcererBallExplode", "\0", 3, 80},
  { "SorcererBigBallExplode", "\0", 3, 80},
  { "SorcererHeadScream", "\0", 2, 256},

  { "DragonSight", "\0", 2, 64},
  { "DragonActive", "\0", 2, 64},
  { "DragonWingflap", "\0", 2, 64},
  { "DragonAttack", "\0", 2, 64},
  { "DragonPain", "\0", 2, 64},
  { "DragonDeath", "\0", 2, 64},
  { "DragonFireballExplode", "\0", 2, 32},
  { "KoraxSight", "\0", 2, 256},
  { "KoraxActive", "\0", 2, 256},
  { "KoraxPain", "\0", 2, 256},

  { "KoraxAttack", "\0", 2, 256},
  { "KoraxCommand", "\0", 2, 256},
  { "KoraxDeath", "\0", 2, 256},
  { "KoraxStep", "\0", 2, 128},
  { "ThrustSpikeRaise", "\0", 2, 32},
  { "ThrustSpikeLower", "\0", 2, 32},
  { "GlassShatter", "\0", 2, 32},
  { "FlechetteBounce", "\0", 2, 32},
  { "FlechetteExplode", "\0", 2, 32},
  { "LavaMove", "\0", 2, 36},

  { "WaterMove", "\0", 2, 36},
  { "IceStartMove", "\0", 2, 36},
  { "EarthStartMove", "\0", 2, 36},
  { "WaterSplash", "\0", 2, 32},
  { "LavaSizzle", "\0", 2, 32},
  { "SludgeGloop", "\0", 2, 32},
  { "HolySymbolFire", "\0", 2, 64},
  { "SpiritActive", "\0", 2, 32},
  { "SpiritAttack", "\0", 2, 32},
  { "SpiritDie", "\0", 2, 32},

  { "ValveTurn", "\0", 2, 36},
  { "RopePull", "\0", 2, 36},
  { "FlyBuzz", "\0", 2, 20},
  { "Ignite", "\0", 2, 32},
  { "PuzzleSuccess", "\0", 2, 256},
  { "PuzzleFailFighter", "\0", 2, 256},
  { "PuzzleFailCleric", "\0", 2, 256},
  { "PuzzleFailMage", "\0", 2, 256},
  { "Earthquake", "\0", 2, 32},
  { "BellRing", "\0", 2, 32},

  { "TreeBreak", "\0", 2, 32},
  { "TreeExplode", "\0", 2, 32},
  { "SuitofArmorBreak", "\0", 2, 32},
  { "PoisonShroomPain", "\0", 2, 20},
  { "PoisonShroomDeath", "\0", 2, 32},
  { "Ambient1", "\0", 1, 1},
  { "Ambient2", "\0", 1, 1},
  { "Ambient3", "\0", 1, 1},
  { "Ambient4", "\0", 1, 1},
  { "Ambient5", "\0", 1, 1},

  { "Ambient6", "\0", 1, 1},
  { "Ambient7", "\0", 1, 1},
  { "Ambient8", "\0", 1, 1},
  { "Ambient9", "\0", 1, 1},
  { "Ambient10", "\0", 1, 1},
  { "Ambient11", "\0", 1, 1},
  { "Ambient12", "\0", 1, 1},
  { "Ambient13", "\0", 1, 1},
  { "Ambient14", "\0", 1, 1},
  { "Ambient15", "\0", 1, 1},

  { "StartupTick", "\0", 2, 32},
  { "SwitchOtherLevel", "\0", 2, 32},
  { "Respawn", "\0", 2, 32},
  { "KoraxVoiceGreetings", "\0", 1, 512}, // originally Korax speeches had multiplicity 2
  { "KoraxVoiceReady", "\0", 1, 512},
  { "KoraxVoiceBlood", "\0", 1, 512},
  { "KoraxVoiceGame", "\0", 1, 512},
  { "KoraxVoiceBoard", "\0", 1, 512},
  { "KoraxVoiceWorship", "\0", 1, 512},
  { "KoraxVoiceMaybe", "\0", 1, 512},

  { "KoraxVoiceStrong", "\0", 1, 512},
  { "KoraxVoiceFace", "\0", 1, 512},
  { "BatScream", "\0", 2, 32},
  { "Chat", "\0", 2, 512},
  { "MenuMove", "\0", 2, 32},
  { "ClockTick", "\0", 2, 32},
  { "Fireball", "\0", 2, 32},
  { "PuppyBeat", "\0", 2, 30},
  { "MysticIncant", "\0", 4, 32} // 498
};



// parses the Hexen SNDINFO lump
void S_Read_SNDINFO(int lump)
{
  int i, j, n;

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
		if (tag[0] == '$')
		  {
		    if (!strcmp(tag, "$MAP"))
		      {
			i = sscanf(s, "%*40s %d %16s", &j, lname);
			if (i == 2  && j >= 1 && j <= 99)
			  ;// FIXME store map music
		      }
		    // $ARCHIVEPATH ignored
		  }
		else
		  {
		    // must be a tagname => lumpname mapping
		    for (i = sfx_Hexen; i < NUMSFX; i++)
		      if (!strcmp(S_sfx[i].tagname, tag))
			{
			  n = sscanf(s, "%*40s%16s", lname);
			  if (n == 1)
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
    if (!S_sfx[i].lumpname[0])
      {
	CONS_Printf("SNDINFO: Missing tag %s\n", S_sfx[i].tagname);
	strcpy(S_sfx[i].lumpname, "default");
      }
}


int S_GetSoundID(const char *tag)
{
  int i;

  for (i = sfx_Hexen; i < NUMSFX; i++)
    if (!strcmp(S_sfx[i].tagname, tag))
      return i;

  return 0;
}
