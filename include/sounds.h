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
// Revision 1.2  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Partly created by the sound utility written by Dave Taylor.
//   Original Doom/Heretic sound and music
//
//-----------------------------------------------------------------------------

#ifndef sounds_h
#define sounds_h 1

#include "doomtype.h"

// struct for Doom native sound format:
// first a 8-byte header composed of 4 unsigned (16-bit) short integers (LE/BE ?),
// then the data (8-bit 11 KHz mono sound)
// max # of samples = 65535 = about 6 seconds of sound
struct doomsfx_t
{
  unsigned short magic; // always 3
  unsigned short rate;  // always 11025
  unsigned short samples; // number of 1-byte samples
  unsigned short zero; // always 0
  byte data[0]; // actual data begins here
};

//
// SoundFX struct.
//

struct sfxinfo_t
{
  char*      name;        // up to 8-character name 
  bool       singularity; // Sfx singularity (only one at a time)
  int        priority;    // Sfx priority
  sfxinfo_t* link;        // referenced sound if a link
  int        pitch;       // pitch if a link
  int        volume;      // volume if a link

  void*      data;    // sound data
  int        length;  // data length in bytes

  // sound that can be remapped for a skin, indexes skins[].skinsounds
  // 0 up to (NUMSKINSOUNDS-1), -1 = not skin specifc
  //int         skinsound;

  // this is checked every second to see if sound
  // can be thrown out (if 0, then decrement, if -1,
  // then throw out, if > 0, then it is in use)
  int         refcount;
  int         lumpnum;   // lump number of sfx
};



// the complete set of Doom/Heretic sound effects
extern sfxinfo_t S_sfx[];

// the complete set of music
extern char* MusicNames[];
//#define NUMMUSICFREESLOTS  64

//
// Identifiers for all music in game.
//
// note! Some musics are really reused in Doom and Heretic.

typedef enum
{
  mus_None = 0,

  // Doom
  mus_e1m1,
  mus_e1m2,
  mus_e1m3,
  mus_e1m4,
  mus_e1m5,
  mus_e1m6,
  mus_e1m7,
  mus_e1m8,
  mus_e1m9,

  mus_e2m1,
  mus_e2m2,
  mus_e2m3,
  mus_e2m4,
  mus_e2m5,
  mus_e2m6,
  mus_e2m7,
  mus_e2m8,
  mus_e2m9,

  mus_e3m1,
  mus_e3m2,
  mus_e3m3,
  mus_e3m4,
  mus_e3m5,
  mus_e3m6,
  mus_e3m7,
  mus_e3m8,
  mus_e3m9,
  // Ultimate Doom, Episode 4
  mus_e4m1, // mus_e3m4 // American
  mus_e4m2, // mus_e3m2 // Romero
  mus_e4m3, // mus_e3m3 // Shawn
  mus_e4m4, // mus_e1m5 // American
  mus_e4m5, // mus_e2m7 // Tim
  mus_e4m6, // mus_e2m4 // Romero
  mus_e4m7, // mus_e2m6 // J.Anderson CHIRON.WAD
  mus_e4m8, // mus_e2m5 // Shawn
  mus_e4m9, // mus_e1m9 // Tim

  mus_inter,
  mus_intro,
  mus_bunny,
  mus_victor,
  mus_introa,

  // Doom II
  mus_runnin, // map01
  mus_stalks,
  mus_countd,
  mus_betwee,
  mus_doom,
  mus_the_da,
  mus_shawn,
  mus_ddtblu,
  mus_in_cit,
  mus_dead,
  mus_stlks2,
  mus_theda2,
  mus_doom2,
  mus_ddtbl2,
  mus_runni2,
  mus_dead2,
  mus_stlks3,
  mus_romero,
  mus_shawn2,
  mus_messag,
  mus_count2,
  mus_ddtbl3,
  mus_ampie,
  mus_theda3,
  mus_adrian,
  mus_messg2,
  mus_romer2,
  mus_tense,
  mus_shawn3,
  mus_openin,
  mus_evil,
  mus_ultima, //map32
  mus_read_m,
  mus_dm2ttl,
  mus_dm2int,

  // Heretic
  mus_he1m1,
  mus_he1m2,
  mus_he1m3,
  mus_he1m4,
  mus_he1m5,
  mus_he1m6,
  mus_he1m7,
  mus_he1m8,
  mus_he1m9,
            
  mus_he2m1,
  mus_he2m2,
  mus_he2m3,
  mus_he2m4,
  mus_he2m5,
  mus_he2m6,
  mus_he2m7,
  mus_he2m8,
  mus_he2m9,

  mus_he3m1,
  mus_he3m2,
  mus_he3m3,
  mus_he3m4,
  mus_he3m5,
  mus_he3m6,
  mus_he3m7,
  mus_he3m8,
  mus_he3m9,

  mus_he4m1,
  mus_he4m2,
  mus_he4m3,
  mus_he4m4,
  mus_he4m5,
  mus_he4m6,
  mus_he4m7,
  mus_he4m8,
  mus_he4m9,

  mus_he5m1,
  mus_he5m2,
  mus_he5m3,
  mus_he5m4,
  mus_he5m5,
  mus_he5m6,
  mus_he5m7,
  mus_he5m8,
  mus_he5m9,

  mus_he6m1,
  mus_he6m2,
  mus_he6m3,
            
  mus_htitl,
  mus_hintr,
  mus_hcptd,
            
  /*            
  mus_firstfreeslot,
  // 64 free slots here
  mus_lastfreeslot = mus_firstfreeslot + NUMMUSICFREESLOTS - 1,
  */
  NUMMUSIC
} musicenum_t;


//
// Identifiers for all sfx in game.
//

typedef enum
{
  sfx_None,   // 0 
  sfx_pistol,
  sfx_shotgn,
  sfx_sgcock,
  sfx_dshtgn,
  sfx_dbopn,
  sfx_dbcls,
  sfx_dbload,
  sfx_plasma,
  sfx_bfg,
  sfx_sawup,  // 10
  sfx_sawidl,
  sfx_sawful,
  sfx_sawhit,
  sfx_rlaunc,
  sfx_rxplod,
  sfx_firsht,
  sfx_firxpl,
  sfx_pstart,
  sfx_pstop,
  sfx_doropn, // 20
  sfx_dorcls,
  sfx_stnmov,
  sfx_swtchn,
  sfx_swtchx,
  sfx_plpain,
  sfx_dmpain,
  sfx_popain,
  sfx_vipain,
  sfx_mnpain,
  sfx_pepain, // 30
  sfx_slop,
  sfx_itemup,
  sfx_wpnup,
  sfx_oof,
  sfx_telept,
  sfx_posit1,
  sfx_posit2,
  sfx_posit3,
  sfx_bgsit1,
  sfx_bgsit2, // 40
  sfx_sgtsit,
  sfx_cacsit,
  sfx_brssit,
  sfx_cybsit,
  sfx_spisit,
  sfx_bspsit,
  sfx_kntsit,
  sfx_vilsit,
  sfx_mansit,
  sfx_pesit, // 50
  sfx_sklatk,
  sfx_sgtatk,
  sfx_skepch,
  sfx_vilatk,
  sfx_claw,
  sfx_skeswg,
  sfx_pldeth,
  sfx_pdiehi,
  sfx_podth1,
  sfx_podth2, // 60
  sfx_podth3,
  sfx_bgdth1,
  sfx_bgdth2,
  sfx_sgtdth,
  sfx_cacdth,
  sfx_skldth,
  sfx_brsdth,
  sfx_cybdth,
  sfx_spidth,
  sfx_bspdth, // 70
  sfx_vildth,
  sfx_kntdth,
  sfx_pedth,
  sfx_skedth,
  sfx_posact,
  sfx_bgact,
  sfx_dmact,
  sfx_bspact,
  sfx_bspwlk,
  sfx_vilact, // 80
  sfx_noway,
  sfx_barexp,
  sfx_punch,
  sfx_hoof,
  sfx_metal,
  sfx_chgun,
  sfx_tink,
  sfx_bdopn,
  sfx_bdcls,
  sfx_itmbk, // 90
  sfx_flame,
  sfx_flamst,
  sfx_getpow,
  sfx_bospit,
  sfx_boscub,
  sfx_bossit,
  sfx_bospn,
  sfx_bosdth,
  sfx_manatk,
  sfx_mandth, // 100
  sfx_sssit,
  sfx_ssdth,
  sfx_keenpn,
  sfx_keendt,
  sfx_skeact,
  sfx_skesit,
  sfx_skeatk,
  sfx_radio,

  // legacy.wad
  //added:22-02-98: player avatar jumps
  sfx_jump,
  //added:22-02-98: player hits something hard and says 'ouch!'
  sfx_ouch, // 110
  //test water
  sfx_gloop,
  sfx_splash,
  sfx_floush,

  // heretic.wad
  sfx_gldhit, // 114
  sfx_gntful,
  sfx_gnthit,
  sfx_gntpow,
  sfx_gntact, //
  sfx_gntuse,
  sfx_phosht,
  sfx_phohit, // 120
  sfx_phopow,
  sfx_lobsht,
  sfx_lobhit,
  sfx_lobpow,
  sfx_hrnsht,
  sfx_hrnhit,
  sfx_hrnpow,
  sfx_ramphit,
  sfx_ramrain,
  sfx_bowsht, // 130
  sfx_stfhit,
  sfx_stfpow,
  sfx_stfcrk,
  sfx_impsit,
  sfx_impat1,
  sfx_impat2,
  sfx_impdth,
  sfx_impact,
  sfx_imppai,
  sfx_mumsit, // 140
  sfx_mumat1,
  sfx_mumat2,
  sfx_mumdth,
  sfx_mumact,
  sfx_mumpai,
  sfx_mumhed,
  sfx_bstsit,
  sfx_bstatk,
  sfx_bstdth,
  sfx_bstact, // 150
  sfx_bstpai,
  sfx_clksit,
  sfx_clkatk,
  sfx_clkdth,
  sfx_clkact,
  sfx_clkpai,
  sfx_snksit,
  sfx_snkatk,
  sfx_snkdth,
  sfx_snkact, // 160
  sfx_snkpai,
  sfx_kgtsit,
  sfx_kgtatk,
  sfx_kgtat2,
  sfx_kgtdth,
  sfx_kgtact,
  sfx_kgtpai,
  sfx_wizsit,
  sfx_wizatk,
  sfx_wizdth, // 170
  sfx_wizact,
  sfx_wizpai,
  sfx_minsit,
  sfx_minat1,
  sfx_minat2,
  sfx_minat3,
  sfx_mindth,
  sfx_minact,
  sfx_minpai,
  sfx_hedsit, // 180
  sfx_hedat1,
  sfx_hedat2,
  sfx_hedat3,
  sfx_heddth,
  sfx_hedact,
  sfx_hedpai,
  sfx_sorzap,
  sfx_sorrise,
  sfx_sorsit,
  sfx_soratk, // 190
  sfx_soract,
  sfx_sorpai,
  sfx_sordsph,
  sfx_sordexp,
  sfx_sordbon,
  sfx_sbtsit,
  sfx_sbtatk,
  sfx_sbtdth,
  sfx_sbtact,
  sfx_sbtpai, // 200
  sfx_plroof, //
  sfx_plrpai,
  sfx_plrdth,             // Normal
  sfx_gibdth,             // Extreme
  sfx_plrwdth,    // Wimpy
  sfx_plrcdth,    // Crazy
  sfx_hitemup,
  sfx_hwpnup,
  sfx_htelept, //
  sfx_hdoropn,
  sfx_hdorcls,
  sfx_dormov, // 210
  sfx_artiup,
  sfx_switch, //
  sfx_hpstart,
  sfx_hpstop,
  sfx_hstnmov,
  sfx_chicpai,
  sfx_chicatk,
  sfx_chicdth,
  sfx_chicact,
  sfx_chicpk1,
  sfx_chicpk2, // 220
  sfx_chicpk3,
  sfx_keyup,
  sfx_ripslop,
  sfx_newpod,
  sfx_podexp,
  sfx_bounce,
  sfx_volsht,
  sfx_volhit,
  sfx_burn,
  sfx_hsplash, // 230
  sfx_hgloop,
  sfx_respawn, //
  sfx_blssht,
  sfx_blshit,
  sfx_chat, //
  sfx_artiuse,
  sfx_gfrag,
  sfx_waterfl, // 236

  // Monophonic sounds

  sfx_wind,
  sfx_amb1,
  sfx_amb2,
  sfx_amb3, // 240
  sfx_amb4,
  sfx_amb5,
  sfx_amb6,
  sfx_amb7,
  sfx_amb8,
  sfx_amb9,
  sfx_amb10,
  sfx_amb11,

  // free slots for S_AddSoundFx() at run-time --------------------
  sfx_freeslot0,
  //
  // ... 60 free sounds here ...
  //
  //sfx_lastfreeslot = (sfx_freeslot0+NUMSFXFREESLOTS-1),
  sfx_lastfreeslot = (sfx_freeslot0+320-1), // temp...
  // end of freeslots ---------------------------------------------

  NUMSFX
} sfxenum_t;


void   S_InitRuntimeSounds();
int    S_AddSoundFx(const char *name,int singularity);
void   S_RemoveSoundFx(int id);

#endif
