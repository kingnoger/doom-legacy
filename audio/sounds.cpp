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
// Revision 1.1  2002/11/16 14:17:48  hurdler
// Initial revision
//
// Revision 1.6  2002/09/20 22:41:26  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.5  2002/08/21 16:58:28  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.4  2002/08/19 18:06:38  vberghol
// renderer somewhat fixed
//
// Revision 1.3  2002/07/01 20:59:48  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:09  vberghol
// Version 133 Experimental!
//
// Revision 1.10  2001/03/30 17:12:51  bpereira
// no message
//
// Revision 1.9  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.8  2001/02/24 13:35:21  bpereira
// no message
//
// Revision 1.7  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.6  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.5  2000/11/03 11:48:40  hurdler
// Fix compiling problem under win32 with 3D-Floors and FragglScript (to verify!)
//
// Revision 1.4  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.3  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      music/sound tables, and related sound routines
//
// Note: the tables were originally created by a sound utility at Id,
//       kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------


#include "doomtype.h"
#include "i_sound.h"
#include "sounds.h"
#include "r_defs.h"
#include "r_things.h"
#include "r_sprite.h" // skins
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
//         singularity(U)           pitch       (U) for UNUSED
//   name           |     priority(U) | volume  
//     |            |       |  link   |   |  
  { "none"       , false,   0, NULL, -1, -1},

  { "dspistol"   , false,  64, NULL, -1, -1},
  { "dsshotgn"   , false,  64, NULL, -1, -1},
  { "dssgcock"   , false,  64, NULL, -1, -1},
  { "dsdshtgn"   , false,  64, NULL, -1, -1},
  { "dsdbopn\0"  , false,  64, NULL, -1, -1},
  { "dsdbcls\0"  , false,  64, NULL, -1, -1},
  { "dsdbload"   , false,  64, NULL, -1, -1},
  { "dsplasma"   , false,  64, NULL, -1, -1},
  { "dsbfg\0\0\0", false,  64, NULL, -1, -1},
  { "dssawup\0"  , false,  64, NULL, -1, -1},
  { "dssawidl"   , false, 118, NULL, -1, -1},
  { "dssawful"   , false,  64, NULL, -1, -1},
  { "dssawhit"   , false,  64, NULL, -1, -1},
  { "dsrlaunc"   , false,  64, NULL, -1, -1},
  { "dsrxplod"   , false,  70, NULL, -1, -1},
  { "dsfirsht"   , false,  70, NULL, -1, -1},
  { "dsfirxpl"   , false,  70, NULL, -1, -1},
  { "dspstart"   , false, 100, NULL, -1, -1},
  { "dspstop\0"  , false, 100, NULL, -1, -1},
  { "dsdoropn"   , false, 100, NULL, -1, -1},
  { "dsdorcls"   , false, 100, NULL, -1, -1},
  { "dsstnmov"   , false, 119, NULL, -1, -1},
  { "dsswtchn"   , false,  78, NULL, -1, -1},
  { "dsswtchx"   , false,  78, NULL, -1, -1},
  { "dsplpain"   , false,  96, NULL, -1, -1},   //SKSPLPAIN},
  { "dsdmpain"   , false,  96, NULL, -1, -1},
  { "dspopain"   , false,  96, NULL, -1, -1},
  { "dsvipain"   , false,  96, NULL, -1, -1},
  { "dsmnpain"   , false,  96, NULL, -1, -1},
  { "dspepain"   , false,  96, NULL, -1, -1},
  { "dsslop\0\0" , false,  78, NULL, -1, -1},    //SKSSLOP},
  { "dsitemup"   ,  true,  78, NULL, -1, -1},
  { "dswpnup"    ,  true,  78, NULL, -1, -1},
  { "dsoof\0\0\0", false,  96, NULL, -1, -1},    //SKSOOF},
  { "dstelept"   , false,  32, NULL, -1, -1},
  { "dsposit1"   ,  true,  98, NULL, -1, -1},
  { "dsposit2"   ,  true,  98, NULL, -1, -1},
  { "dsposit3"   ,  true,  98, NULL, -1, -1},
  { "dsbgsit1"   ,  true,  98, NULL, -1, -1},
  { "dsbgsit2"   ,  true,  98, NULL, -1, -1},
  { "dssgtsit"   ,  true,  98, NULL, -1, -1},
  { "dscacsit"   ,  true,  98, NULL, -1, -1},
  { "dsbrssit"   ,  true,  94, NULL, -1, -1},
  { "dscybsit"   ,  true,  92, NULL, -1, -1},
  { "dsspisit"   ,  true,  90, NULL, -1, -1},
  { "dsbspsit"   ,  true,  90, NULL, -1, -1},
  { "dskntsit"   ,  true,  90, NULL, -1, -1},
  { "dsvilsit"   ,  true,  90, NULL, -1, -1},
  { "dsmansit"   ,  true,  90, NULL, -1, -1},
  { "dspesit\0"  ,  true,  90, NULL, -1, -1},
  { "dssklatk"   , false,  70, NULL, -1, -1},
  { "dssgtatk"   , false,  70, NULL, -1, -1},
  { "dsskepch"   , false,  70, NULL, -1, -1},
  { "dsvilatk"   , false,  70, NULL, -1, -1},
  { "dsclaw\0\0" , false,  70, NULL, -1, -1},
  { "dsskeswg"   , false,  70, NULL, -1, -1},
  { "dspldeth"   , false,  32, NULL, -1, -1}, //SKSPLDETH},
  { "dspdiehi"   , false,  32, NULL, -1, -1}, //SKSPDIEHI},
  { "dspodth1"   , false,  70, NULL, -1, -1},
  { "dspodth2"   , false,  70, NULL, -1, -1},
  { "dspodth3"   , false,  70, NULL, -1, -1},
  { "dsbgdth1"   , false,  70, NULL, -1, -1},
  { "dsbgdth2"   , false,  70, NULL, -1, -1},
  { "dssgtdth"   , false,  70, NULL, -1, -1},
  { "dscacdth"   , false,  70, NULL, -1, -1},
  { "dsskldth"   , false,  70, NULL, -1, -1},
  { "dsbrsdth"   , false,  32, NULL, -1, -1},
  { "dscybdth"   , false,  32, NULL, -1, -1},
  { "dsspidth"   , false,  32, NULL, -1, -1},
  { "dsbspdth"   , false,  32, NULL, -1, -1},
  { "dsvildth"   , false,  32, NULL, -1, -1},
  { "dskntdth"   , false,  32, NULL, -1, -1},
  { "dspedth\0"  , false,  32, NULL, -1, -1},
  { "dsskedth"   , false,  32, NULL, -1, -1},
  { "dsposact"   ,  true, 120, NULL, -1, -1},
  { "dsbgact\0"  ,  true, 120, NULL, -1, -1},
  { "dsdmact\0"  ,  true, 120, NULL, -1, -1},
  { "dsbspact"   ,  true, 100, NULL, -1, -1},
  { "dsbspwlk"   ,  true, 100, NULL, -1, -1},
  { "dsvilact"   ,  true, 100, NULL, -1, -1},
  { "dsnoway\0"  , false,  78, NULL, -1, -1}, //SKSNOWAY},
  { "dsbarexp"   , false,  60, NULL, -1, -1},
  { "dspunch\0"  , false,  64, NULL, -1, -1}, //SKSPUNCH},
  { "dshoof\0\0" , false,  70, NULL, -1, -1},
  { "dsmetal\0"  , false,  70, NULL, -1, -1},
  { "dschgun\0"  , false,  64, &S_sfx[sfx_pistol], 150, 0},
  { "dstink\0\0" , false,  60, NULL, -1, -1},
  { "dsbdopn\0"  , false, 100, NULL, -1, -1},
  { "dsbdcls\0"  , false, 100, NULL, -1, -1},
  { "dsitmbk\0"  , false, 100, NULL, -1, -1},
  { "dsflame\0"  , false,  32, NULL, -1, -1},
  { "dsflamst"   , false,  32, NULL, -1, -1},
  { "dsgetpow"   , false,  60, NULL, -1, -1},
  { "dsbospit"   , false,  70, NULL, -1, -1},
  { "dsboscub"   , false,  70, NULL, -1, -1},
  { "dsbossit"   , false,  70, NULL, -1, -1},
  { "dsbospn\0"  , false,  70, NULL, -1, -1},
  { "dsbosdth"   , false,  70, NULL, -1, -1},
  { "dsmanatk"   , false,  70, NULL, -1, -1},
  { "dsmandth"   , false,  70, NULL, -1, -1},
  { "dssssit\0"  , false,  70, NULL, -1, -1},
  { "dsssdth\0"  , false,  70, NULL, -1, -1},
  { "dskeenpn"   , false,  70, NULL, -1, -1},
  { "dskeendt"   , false,  70, NULL, -1, -1},
  { "dsskeact"   , false,  70, NULL, -1, -1},
  { "dsskesit"   , false,  70, NULL, -1, -1},
  { "dsskeatk"   , false,  70, NULL, -1, -1},
  { "dsradio\0"  , false,  60, NULL, -1, -1}, //SKSRADIO},

  // legacy.wad extra sounds
  //added:22-02-98: sound when the player avatar jumps in air 'hmpf!'
  { "dsjump\0\0" , false,  60, NULL, -1, -1}, //SKSJUMP},
  { "dsouch\0\0" , false,  64, NULL, -1, -1}, //SKSOUCH},
  //added:09-08-98:test water sounds
  { "dsgloop\0"  , false,  60, NULL, -1, -1},
  { "dssplash"   , false,  64, NULL, -1, -1},
  { "dsfloush"   , false,  64, NULL, -1, -1},

  // heretic.wad sounds
  { "gldhit",  false, 32, NULL, -1, -1},
  { "gntful",  false, 32, NULL, -1, -1},
  { "gnthit",  false, 32, NULL, -1, -1},
  { "gntpow",  false, 32, NULL, -1, -1},
  { "gntact",  false, 32, NULL, -1, -1},
  { "gntuse",  false, 32, NULL, -1, -1},
  { "phosht",  false, 32, NULL, -1, -1},
  { "phohit",  false, 32, NULL, -1, -1},
  { "-phopow", false, 32, &S_sfx[sfx_hedat1], -1, -1},
  { "lobsht",  false, 20, NULL, -1, -1},
  { "lobhit",  false, 20, NULL, -1, -1},
  { "lobpow",  false, 20, NULL, -1, -1},
  { "hrnsht",  false, 32, NULL, -1, -1},
  { "hrnhit",  false, 32, NULL, -1, -1},
  { "hrnpow",  false, 32, NULL, -1, -1},
  { "ramphit", false, 32, NULL, -1, -1},
  { "ramrain", false, 10, NULL, -1, -1},
  { "bowsht",  false, 32, NULL, -1, -1},
  { "stfhit",  false, 32, NULL, -1, -1},
  { "stfpow",  false, 32, NULL, -1, -1},
  { "stfcrk",  false, 32, NULL, -1, -1},
  { "impsit",  false, 32, NULL, -1, -1},
  { "impat1",  false, 32, NULL, -1, -1},
  { "impat2",  false, 32, NULL, -1, -1},
  { "impdth",  false, 80, NULL, -1, -1},
  { "-impact", false, 20, &S_sfx[sfx_impsit], -1, -1},
  { "imppai",  false, 32, NULL, -1, -1},
  { "mumsit",  false, 32, NULL, -1, -1},
  { "mumat1",  false, 32, NULL, -1, -1},
  { "mumat2",  false, 32, NULL, -1, -1},
  { "mumdth",  false, 80, NULL, -1, -1},
  { "-mumact", false, 20, &S_sfx[sfx_mumsit], -1, -1},
  { "mumpai",  false, 32, NULL, -1, -1},
  { "mumhed",  false, 32, NULL, -1, -1},
  { "bstsit",  false, 32, NULL, -1, -1},
  { "bstatk",  false, 32, NULL, -1, -1},
  { "bstdth",  false, 80, NULL, -1, -1},
  { "bstact",  false, 20, NULL, -1, -1},
  { "bstpai",  false, 32, NULL, -1, -1},
  { "clksit",  false, 32, NULL, -1, -1},
  { "clkatk",  false, 32, NULL, -1, -1},
  { "clkdth",  false, 80, NULL, -1, -1},
  { "clkact",  false, 20, NULL, -1, -1},
  { "clkpai",  false, 32, NULL, -1, -1},
  { "snksit",  false, 32, NULL, -1, -1},
  { "snkatk",  false, 32, NULL, -1, -1},
  { "snkdth",  false, 80, NULL, -1, -1},
  { "snkact",  false, 20, NULL, -1, -1},
  { "snkpai",  false, 32, NULL, -1, -1},
  { "kgtsit",  false, 32, NULL, -1, -1},
  { "kgtatk",  false, 32, NULL, -1, -1},
  { "kgtat2",  false, 32, NULL, -1, -1},
  { "kgtdth",  false, 80, NULL, -1, -1},
  { "-kgtact", false, 20, &S_sfx[sfx_kgtsit], -1, -1},
  { "kgtpai",  false, 32, NULL, -1, -1},
  { "wizsit",  false, 32, NULL, -1, -1},
  { "wizatk",  false, 32, NULL, -1, -1},
  { "wizdth",  false, 80, NULL, -1, -1},
  { "wizact",  false, 20, NULL, -1, -1},
  { "wizpai",  false, 32, NULL, -1, -1},
  { "minsit",  false, 32, NULL, -1, -1},
  { "minat1",  false, 32, NULL, -1, -1},
  { "minat2",  false, 32, NULL, -1, -1},
  { "minat3",  false, 32, NULL, -1, -1},
  { "mindth",  false, 80, NULL, -1, -1},
  { "minact",  false, 20, NULL, -1, -1},
  { "minpai",  false, 32, NULL, -1, -1},
  { "hedsit",  false, 32, NULL, -1, -1},
  { "hedat1",  false, 32, NULL, -1, -1},
  { "hedat2",  false, 32, NULL, -1, -1},
  { "hedat3",  false, 32, NULL, -1, -1},
  { "heddth",  false, 80, NULL, -1, -1},
  { "hedact",  false, 20, NULL, -1, -1},
  { "hedpai",  false, 32, NULL, -1, -1},
  { "sorzap",  false, 32, NULL, -1, -1},
  { "sorrise", false, 32, NULL, -1, -1},
  { "sorsit",  false, 200,NULL, -1, -1},
  { "soratk",  false, 32, NULL, -1, -1},
  { "soract",  false, 200,NULL, -1, -1},
  { "sorpai",  false, 200,NULL, -1, -1},
  { "sordsph", false, 200,NULL, -1, -1},
  { "sordexp", false, 200,NULL, -1, -1},
  { "sordbon", false, 200,NULL, -1, -1},
  { "-sbtsit", false, 32, &S_sfx[sfx_bstsit], -1, -1},
  { "-sbtatk", false, 32, &S_sfx[sfx_bstatk], -1, -1},
  { "sbtdth",  false, 80, NULL, -1, -1},
  { "sbtact",  false, 20, NULL, -1, -1},
  { "sbtpai",  false, 32, NULL, -1, -1},
  { "plroof",  false, 32, NULL, -1, -1},
  { "plrpai",  false, 32, NULL, -1, -1},
  { "plrdth",  false, 80, NULL, -1, -1},
  { "gibdth",  false, 100,NULL, -1, -1},
  { "plrwdth", false, 80, NULL, -1, -1},
  { "plrcdth", false, 100,NULL, -1, -1},
  { "itemup",  false, 32, NULL, -1, -1},
  { "wpnup",   false, 32, NULL, -1, -1},
  { "telept",  false, 50, NULL, -1, -1},
  { "doropn",  false, 40, NULL, -1, -1},
  { "dorcls",  false, 40, NULL, -1, -1},
  { "dormov",  false, 40, NULL, -1, -1},
  { "artiup",  false, 32, NULL, -1, -1},
  { "switch",  false, 40, NULL, -1, -1},
  { "pstart",  false, 40, NULL, -1, -1},
  { "pstop",   false, 40, NULL, -1, -1},
  { "stnmov",  false, 40, NULL, -1, -1},
  { "chicpai", false, 32, NULL, -1, -1},
  { "chicatk", false, 32, NULL, -1, -1},
  { "chicdth", false, 40, NULL, -1, -1},
  { "chicact", false, 32, NULL, -1, -1},
  { "chicpk1", false, 32, NULL, -1, -1},
  { "chicpk2", false, 32, NULL, -1, -1},
  { "chicpk3", false, 32, NULL, -1, -1},
  { "keyup"  , false, 50, NULL, -1, -1},
  { "ripslop", false, 16, NULL, -1, -1},
  { "newpod" , false, 16, NULL, -1, -1},
  { "podexp" , false, 40, NULL, -1, -1},
  { "bounce" , false, 16, NULL, -1, -1},
  { "-volsht", false, 16, &S_sfx[sfx_bstatk], -1, -1},
  { "-volhit", false, 16, &S_sfx[sfx_lobhit], -1, -1},
  { "burn"   , false, 10, NULL, -1, -1},
  { "splash" , false, 10, NULL, -1, -1},
  { "gloop"  , false, 10, NULL, -1, -1},
  { "respawn", false, 10, NULL, -1, -1},
  { "blssht" , false, 32, NULL, -1, -1},
  { "blshit" , false, 32, NULL, -1, -1},
  { "chat"   , false, 100,NULL, -1, -1},
  { "artiuse", false, 32, NULL, -1, -1},
  { "gfrag"  , false, 100,NULL, -1, -1},
  { "waterfl", false, 16, NULL, -1, -1},

  // Monophonic sounds

  { "wind"   , false, 16, NULL, -1, -1},
  { "amb1"   , false,  1, NULL, -1, -1},
  { "amb2"   , false,  1, NULL, -1, -1},
  { "amb3"   , false,  1, NULL, -1, -1},
  { "amb4"   , false,  1, NULL, -1, -1},
  { "amb5"   , false,  1, NULL, -1, -1},
  { "amb6"   , false,  1, NULL, -1, -1},
  { "amb7"   , false,  1, NULL, -1, -1},
  { "amb8"   , false,  1, NULL, -1, -1},
  { "amb9"   , false,  1, NULL, -1, -1},
  { "amb10"  , false,  1, NULL, -1, -1},
  { "amb11"  , false,  1, NULL, -1, -1}
  // skin sounds free slots to add sounds at run time (Boris HACK!!!)
  // initialized to NULL
};


// Prepare free sfx slots to add sfx at run time
void S_InitRuntimeSounds()
{
    int  i;

    for (i=sfx_freeslot0; i<=sfx_lastfreeslot; i++)
        S_sfx[i].name = NULL;
}

// Add a new sound fx into a free sfx slot.
//
int S_AddSoundFx(const char *name,int singularity)
{
  int i;

  for(i=sfx_freeslot0;i<NUMSFX;i++)
    {
      if(!S_sfx[i].name)
        {
            S_sfx[i].name=(char *)Z_Malloc(7,PU_STATIC,NULL);
            strncpy(S_sfx[i].name,name,6);
            S_sfx[i].name[6]='\0';
            S_sfx[i].singularity=singularity;
            S_sfx[i].priority=60;
            S_sfx[i].link=0;
            S_sfx[i].pitch=-1;
            S_sfx[i].volume=-1;
            S_sfx[i].lumpnum=-1;
            S_sfx[i].refcount=-1;

            // if precache load it here ! todo !
            S_sfx[i].data=NULL;
            S_sfx[i].length=0;
            return i;
        }
    }
    CONS_Printf("\2No more free sound slots\n");
    return 0;
}


void S_RemoveSoundFx (int id)
{
    if (id>=sfx_freeslot0 &&
        id<=sfx_lastfreeslot &&
        S_sfx[id].name)
    {
        Z_Free(S_sfx[id].name);
        S_sfx[id].name=NULL;
        S_sfx[id].lumpnum=-1;
        I_FreeSfx(&S_sfx[id]);
    }
}

