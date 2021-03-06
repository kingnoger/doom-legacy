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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Globally defined strings.

#ifndef dstrings_h
#define dstrings_h 1


// QuitDOOM messages
#define NUM_QUITMESSAGES   15
extern const char* endmsg[];


// languages
enum language_t
{
  la_english,
  la_french,
  la_unknown
};

// All important printed strings.
// Language selection (message strings).
extern const char *text[];

enum text_enum
{
  TXT_DOOM2TITLE,
  TXT_DOOMUTITLE,
  TXT_DOOMTITLE,
  TXT_DOOM1TITLE,

  TXT_D_DEVSTR,
  TXT_PRESSKEY,
  TXT_PRESSYN,
  TXT_LOADNET,
  TXT_QLOADNET,
  TXT_QSAVESPOT,
  TXT_SAVEDEAD,
  TXT_QSPROMPT,
  TXT_QLPROMPT,
  TXT_NEWGAME,
  TXT_NIGHTMARE,
  TXT_SWSTRING,
  TXT_MSGOFF,
  TXT_MSGON,
  TXT_NETEND,
  TXT_ENDGAME,
  TXT_QUIT_OS_Y,

  TXT_DETAILHI,
  TXT_DETAILLO,
  TXT_GAMMALVL0,
  TXT_GAMMALVL1,
  TXT_GAMMALVL2,
  TXT_GAMMALVL3,
  TXT_GAMMALVL4,
  TXT_EMPTYSTRING,
  TXT_GGSAVED,

  TXT_GOTARMOR,
  TXT_GOTMEGA,
  TXT_GOTHTHBONUS,
  TXT_GOTARMBONUS,
  TXT_GOTSTIM,
  TXT_GOTMEDINEED,
  TXT_GOTMEDIKIT,
  TXT_GOTSUPER,
  TXT_GOTINVUL,
  TXT_GOTBERSERK,
  TXT_GOTINVIS,
  TXT_GOTSUIT,
  TXT_GOTMAP,
  TXT_GOTVISOR,
  TXT_GOTMSPHERE,

  TXT_GOTCLIP,
  TXT_GOTCLIPBOX,
  TXT_GOTROCKET,
  TXT_GOTROCKBOX,
  TXT_GOTCELL,
  TXT_GOTCELLBOX,
  TXT_GOTSHELLS,
  TXT_GOTSHELLBOX,
  TXT_GOTBACKPACK,

  TXT_GOTBFG9000,
  TXT_GOTCHAINGUN,
  TXT_GOTCHAINSAW,
  TXT_GOTLAUNCHER,
  TXT_GOTPLASMA,
  TXT_GOTSHOTGUN,
  TXT_GOTSHOTGUN2,
  TXT_PD_BLUEO,
  TXT_PD_YELLOWO,
  TXT_PD_REDO,
  TXT_PD_BLUEK,
  TXT_PD_YELLOWK,
  TXT_PD_REDK,

  TXT_HUSTR_CHATMACRO1,
  TXT_HUSTR_CHATMACRO2,
  TXT_HUSTR_CHATMACRO3,
  TXT_HUSTR_CHATMACRO4,
  TXT_HUSTR_CHATMACRO5,
  TXT_HUSTR_CHATMACRO6,
  TXT_HUSTR_CHATMACRO7,
  TXT_HUSTR_CHATMACRO8,
  TXT_HUSTR_CHATMACRO9,
  TXT_HUSTR_CHATMACRO0,
  TXT_HUSTR_TALKTOSELF1,
  TXT_HUSTR_TALKTOSELF2,
  TXT_HUSTR_TALKTOSELF3,
  TXT_HUSTR_TALKTOSELF4,
  TXT_HUSTR_TALKTOSELF5,
  TXT_HUSTR_MSGU,
  TXT_HUSTR_MESSAGESENT,

  TXT_AMSTR_FOLLOWON,
  TXT_AMSTR_FOLLOWOFF,
  TXT_AMSTR_GRIDON,
  TXT_AMSTR_GRIDOFF,
  TXT_AMSTR_MARKEDSPOT,
  TXT_AMSTR_MARKSCLEARED,

  TXT_STSTR_MUS,
  TXT_STSTR_NOMUS,
  TXT_STSTR_DQDON,
  TXT_STSTR_DQDOFF,
  TXT_STSTR_KFAADDED,
  TXT_STSTR_FAADDED,
  TXT_STSTR_NCON,
  TXT_STSTR_NCOFF,
  TXT_STSTR_BEHOLD,
  TXT_STSTR_BEHOLDX,
  TXT_STSTR_CHOPPERS,
  TXT_STSTR_CLEV,

  TXT_CC_ZOMBIE,
  TXT_CC_SHOTGUN,
  TXT_CC_HEAVY,
  TXT_CC_IMP,
  TXT_CC_DEMON,
  TXT_CC_LOST,
  TXT_CC_CACO,
  TXT_CC_HELL,
  TXT_CC_BARON,
  TXT_CC_ARACH,
  TXT_CC_PAIN,
  TXT_CC_REVEN,
  TXT_CC_MANCU,
  TXT_CC_ARCH,
  TXT_CC_SPIDER,
  TXT_CC_CYBER,
  TXT_CC_HERO,

  TXT_QUITMSG,
  TXT_QUITMSG1,
  TXT_QUITMSG2,
  TXT_QUITMSG3,
  TXT_QUITMSG4,
  TXT_QUITMSG5,
  TXT_QUITMSG6,
  TXT_QUITMSG7,

  TXT_QUIT2MSG,
  TXT_QUIT2MSG1,
  TXT_QUIT2MSG2,
  TXT_QUIT2MSG3,
  TXT_QUIT2MSG4,
  TXT_QUIT2MSG5,
  TXT_QUIT2MSG6,

  // Boom messages.
  TXT_PD_BLUEC,
  TXT_PD_YELLOWC,
  TXT_PD_REDC,
  TXT_PD_BLUES,
  TXT_PD_YELLOWS,
  TXT_PD_REDS,
  TXT_PD_ANY,
  TXT_PD_ALL3,
  TXT_PD_ALL6,

  // combined artifacts in d_items.h order
  TXT_ARTIINVULNERABILITY,
  TXT_ARTIINVISIBILITY,
  TXT_ARTIHEALTH,
  TXT_ARTISUPERHEALTH,
  TXT_ARTITOMEOFPOWER,
  TXT_ARTITORCH,
  TXT_ARTIFIREBOMB,
  TXT_ARTIEGG,
  TXT_ARTIFLY,
  TXT_ARTITELEPORT,

  TXT_ARTIHEALINGRADIUS,
  TXT_ARTISUMMON,
  TXT_ARTIPORK,
  TXT_ARTIBLASTRADIUS,
  TXT_ARTIPOISONBAG,
  TXT_ARTITELEPORTOTHER,
  TXT_ARTISPEED,
  TXT_ARTIBOOSTMANA,
  TXT_ARTIBOOSTARMOR,
  TXT_XARTIINVULNERABILITY,

  // puzzle artifacts
  TXT_ARTIPUZZSKULL,
  TXT_ARTIPUZZGEMBIG,
  TXT_ARTIPUZZGEMRED,
  TXT_ARTIPUZZGEMGREEN1,
  TXT_ARTIPUZZGEMGREEN2,
  TXT_ARTIPUZZGEMBLUE1,
  TXT_ARTIPUZZGEMBLUE2,
  TXT_ARTIPUZZBOOK1,
  TXT_ARTIPUZZBOOK2,
  TXT_ARTIPUZZSKULL2,
  TXT_ARTIPUZZFWEAPON,
  TXT_ARTIPUZZCWEAPON,
  TXT_ARTIPUZZMWEAPON,
  TXT_ARTIPUZZGEAR1,
  TXT_ARTIPUZZGEAR2,
  TXT_ARTIPUZZGEAR3,
  TXT_ARTIPUZZGEAR4,
  TXT_USEPUZZLEFAILED,

  TXT_AMMOGOLDWAND1,
  TXT_AMMOGOLDWAND2,
  TXT_AMMOMACE1,
  TXT_AMMOMACE2,
  TXT_AMMOCROSSBOW1,
  TXT_AMMOCROSSBOW2,
  TXT_AMMOBLASTER1,
  TXT_AMMOBLASTER2,
  TXT_AMMOSKULLROD1,
  TXT_AMMOSKULLROD2,
  TXT_AMMOPHOENIXROD1,
  TXT_AMMOPHOENIXROD2,

  TXT_WPNMACE,
  TXT_WPNCROSSBOW,
  TXT_WPNBLASTER,
  TXT_WPNSKULLROD,
  TXT_WPNPHOENIXROD,
  TXT_WPNGAUNTLETS,

  TXT_ITEMHEALTH,
  TXT_ITEMBAGOFHOLDING,
  TXT_ITEMSHIELD1,
  TXT_ITEMSHIELD2,
  TXT_ITEMSUPERMAP,

  TXT_CHEATGODON,
  TXT_CHEATGODOFF,
  TXT_CHEATNOCLIPON,
  TXT_CHEATNOCLIPOFF,
  TXT_CHEATWEAPONS,
  TXT_CHEATFLIGHTON,
  TXT_CHEATFLIGHTOFF,
  TXT_CHEATPOWERON,
  TXT_CHEATPOWEROFF,
  TXT_CHEATHEALTH,
  TXT_CHEATKEYS,
  TXT_CHEATSOUNDON,
  TXT_CHEATSOUNDOFF,
  TXT_CHEATTICKERON,
  TXT_CHEATTICKEROFF,
  TXT_CHEATARTIFACTS1,
  TXT_CHEATARTIFACTS2,
  TXT_CHEATARTIFACTS3,
  TXT_CHEATARTIFACTSFAIL,
  TXT_CHEATWARP,
  TXT_CHEATSCREENSHOT,
  TXT_CHEATCHICKENON,
  TXT_CHEATCHICKENOFF,
  TXT_CHEATMASSACRE,
  TXT_CHEATIDDQD,
  TXT_CHEATIDKFA,

  TXT_DEATHMSG_SUICIDE,
  TXT_DEATHMSG_TELEFRAG,
  TXT_DEATHMSG_FIST,
  TXT_DEATHMSG_GUN,
  TXT_DEATHMSG_SHOTGUN,
  TXT_DEATHMSG_MACHGUN,
  TXT_DEATHMSG_ROCKET,
  TXT_DEATHMSG_GIBROCKET,
  TXT_DEATHMSG_PLASMA,
  TXT_DEATHMSG_BFGBALL,
  TXT_DEATHMSG_CHAINSAW,
  TXT_DEATHMSG_SUPSHOTGUN,
  TXT_DEATHMSG_PLAYUNKNOW,
  TXT_DEATHMSG_HELLSLIME,
  TXT_DEATHMSG_NUKE,
  TXT_DEATHMSG_SUPHELLSLIME,
  TXT_DEATHMSG_SPECUNKNOW,
  TXT_DEATHMSG_BARRELFRAG,
  TXT_DEATHMSG_BARREL,
  TXT_DEATHMSG_POSSESSED,
  TXT_DEATHMSG_SHOTGUY,
  TXT_DEATHMSG_VILE,
  TXT_DEATHMSG_FATSO,
  TXT_DEATHMSG_CHAINGUY,
  TXT_DEATHMSG_TROOP,
  TXT_DEATHMSG_SERGEANT,
  TXT_DEATHMSG_SHADOWS,
  TXT_DEATHMSG_HEAD,
  TXT_DEATHMSG_BRUISER,
  TXT_DEATHMSG_UNDEAD,
  TXT_DEATHMSG_KNIGHT,
  TXT_DEATHMSG_SKULL,
  TXT_DEATHMSG_SPIDER,
  TXT_DEATHMSG_BABY,
  TXT_DEATHMSG_CYBORG,
  TXT_DEATHMSG_PAIN,
  TXT_DEATHMSG_WOLFSS,
  TXT_DEATHMSG_DEAD,

  // Hexen
  TXT_MANA_1,
  TXT_MANA_2,
  TXT_MANA_BOTH,

  TXT_KEY_STEEL,
  TXT_KEY_CAVE,
  TXT_KEY_AXE,
  TXT_KEY_FIRE,
  TXT_KEY_EMERALD,
  TXT_KEY_DUNGEON,
  TXT_KEY_SILVER,
  TXT_KEY_RUSTED,
  TXT_KEY_HORN,
  TXT_KEY_SWAMP,
  TXT_KEY_CASTLE,
  TXT_GOTBLUECARD,
  TXT_GOTYELWCARD,
  TXT_GOTREDCARD,
  TXT_GOTBLUESKUL,
  TXT_GOTYELWSKUL,
  TXT_GOTREDSKULL,

  // Items
  TXT_ARMOR1,
  TXT_ARMOR2,
  TXT_ARMOR3,
  TXT_ARMOR4,

  // Weapons
  TXT_WEAPON_F2,
  TXT_WEAPON_F3,
  TXT_WEAPON_F4,
  TXT_WEAPON_C2,
  TXT_WEAPON_C3,
  TXT_WEAPON_C4,
  TXT_WEAPON_M2,
  TXT_WEAPON_M3,
  TXT_WEAPON_M4,
  TXT_QUIETUS_PIECE,
  TXT_WRAITHVERGE_PIECE,
  TXT_BLOODSCOURGE_PIECE,

  NUMTEXT
};


//
// D_Main.C
//
#define D_DEVSTR          text[TXT_D_DEVSTR]

//
//      M_Menu.C
//
#define PRESSKEY          text[TXT_PRESSKEY]
#define PRESSYN           text[TXT_PRESSYN]
#define LOADNET           text[TXT_LOADNET]
#define QLOADNET          text[TXT_QLOADNET]
#define QSAVESPOT         text[TXT_QSAVESPOT]
#define SAVEDEAD          text[TXT_SAVEDEAD]
#define QSPROMPT          text[TXT_QSPROMPT]
#define QLPROMPT          text[TXT_QLPROMPT]
#define NEWGAME           text[TXT_NEWGAME]
#define NIGHTMARE         text[TXT_NIGHTMARE]
#define SWSTRING          text[TXT_SWSTRING]
#define MSGOFF            text[TXT_MSGOFF]
#define MSGON             text[TXT_MSGON]
#define NETEND            text[TXT_NETEND]
#define ENDGAME           text[TXT_ENDGAME]
#define DOSY              text[TXT_QUIT_OS_Y]
#define DETAILHI          text[TXT_DETAILHI]
#define DETAILLO          text[TXT_DETAILLO]
#define GAMMALVL0         text[TXT_GAMMALVL0]
#define GAMMALVL1         text[TXT_GAMMALVL1]
#define GAMMALVL2         text[TXT_GAMMALVL2]
#define GAMMALVL3         text[TXT_GAMMALVL3]
#define GAMMALVL4         text[TXT_GAMMALVL4]
#define EMPTYSTRING       text[TXT_EMPTYSTRING]

//
//      P_inter.C
//
#define GOTHTHBONUS       text[TXT_GOTHTHBONUS]
#define GOTARMBONUS       text[TXT_GOTARMBONUS]
#define GOTSTIM           text[TXT_GOTSTIM]
#define GOTMEDINEED       text[TXT_GOTMEDINEED]
#define GOTMEDIKIT        text[TXT_GOTMEDIKIT]
#define GOTSUPER          text[TXT_GOTSUPER]
#define GOTBLUECARD       text[TXT_GOTBLUECARD]
#define GOTYELWCARD       text[TXT_GOTYELWCARD]
#define GOTREDCARD        text[TXT_GOTREDCARD]
#define GOTBLUESKUL       text[TXT_GOTBLUESKUL]
#define GOTYELWSKUL       text[TXT_GOTYELWSKUL]
#define GOTREDSKULL       text[TXT_GOTREDSKULL]
#define GOTINVUL          text[TXT_GOTINVUL]
#define GOTBERSERK        text[TXT_GOTBERSERK]
#define GOTINVIS          text[TXT_GOTINVIS]
#define GOTSUIT           text[TXT_GOTSUIT]
#define GOTMAP            text[TXT_GOTMAP]
#define GOTVISOR          text[TXT_GOTVISOR]
#define GOTMSPHERE        text[TXT_GOTMSPHERE]
#define GOTCLIP           text[TXT_GOTCLIP]
#define GOTCLIPBOX        text[TXT_GOTCLIPBOX]
#define GOTROCKET         text[TXT_GOTROCKET]
#define GOTROCKBOX        text[TXT_GOTROCKBOX]
#define GOTCELL           text[TXT_GOTCELL]
#define GOTCELLBOX        text[TXT_GOTCELLBOX]
#define GOTSHELLS         text[TXT_GOTSHELLS]
#define GOTSHELLBOX       text[TXT_GOTSHELLBOX]
#define GOTBACKPACK       text[TXT_GOTBACKPACK]
#define GOTBFG9000        text[TXT_GOTBFG9000]
#define GOTCHAINGUN       text[TXT_GOTCHAINGUN]
#define GOTCHAINSAW       text[TXT_GOTCHAINSAW]
#define GOTLAUNCHER       text[TXT_GOTLAUNCHER]
#define GOTPLASMA         text[TXT_GOTPLASMA]
#define GOTSHOTGUN        text[TXT_GOTSHOTGUN]
#define GOTSHOTGUN2       text[TXT_GOTSHOTGUN2]

//
// P_Doors.C
//
#define PD_BLUEO          text[TXT_PD_BLUEO]
#define PD_REDO           text[TXT_PD_REDO]
#define PD_YELLOWO        text[TXT_PD_YELLOWO]
#define PD_BLUEK          text[TXT_PD_BLUEK]
#define PD_REDK           text[TXT_PD_REDK]
#define PD_YELLOWK        text[TXT_PD_YELLOWK]

//SoM: 3/9/2000: Add new key messages.
#define PD_BLUEC          text[TXT_PD_BLUEC]
#define PD_REDC           text[TXT_PD_REDC]
#define PD_YELLOWC        text[TXT_PD_YELLOWC]
#define PD_BLUES          text[TXT_PD_BLUES]
#define PD_REDS           text[TXT_PD_REDS]
#define PD_YELLOWS        text[TXT_PD_YELLOWS]
#define PD_ANY            text[TXT_PD_ANY]
#define PD_ALL3           text[TXT_PD_ALL3]
#define PD_ALL6           text[TXT_PD_ALL6]


//
//      HU_stuff.C
//
#define HUSTR_MSGU        text[TXT_HUSTR_MSGU]
#define HUSTR_CHATMACRO1  text[TXT_HUSTR_CHATMACRO1]
#define HUSTR_CHATMACRO2  text[TXT_HUSTR_CHATMACRO2]
#define HUSTR_CHATMACRO3  text[TXT_HUSTR_CHATMACRO3]
#define HUSTR_CHATMACRO4  text[TXT_HUSTR_CHATMACRO4]
#define HUSTR_CHATMACRO5  text[TXT_HUSTR_CHATMACRO5]
#define HUSTR_CHATMACRO6  text[TXT_HUSTR_CHATMACRO6]
#define HUSTR_CHATMACRO7  text[TXT_HUSTR_CHATMACRO7]
#define HUSTR_CHATMACRO8  text[TXT_HUSTR_CHATMACRO8]
#define HUSTR_CHATMACRO9  text[TXT_HUSTR_CHATMACRO9]
#define HUSTR_CHATMACRO0  text[TXT_HUSTR_CHATMACRO0]
#define HUSTR_TALKTOSELF1 text[TXT_HUSTR_TALKTOSELF1]
#define HUSTR_TALKTOSELF2 text[TXT_HUSTR_TALKTOSELF2]
#define HUSTR_TALKTOSELF3 text[TXT_HUSTR_TALKTOSELF3]
#define HUSTR_TALKTOSELF4 text[TXT_HUSTR_TALKTOSELF4]
#define HUSTR_TALKTOSELF5 text[TXT_HUSTR_TALKTOSELF5]
#define HUSTR_MESSAGESENT text[TXT_HUSTR_MESSAGESENT]

// The following should NOT be changed unless it seems
// just AWFULLY necessary

#define HUSTR_KEYGREEN  'g'
#define HUSTR_KEYINDIGO 'i'
#define HUSTR_KEYBROWN  'b'
#define HUSTR_KEYRED    'r'

//
//      AM_map.C
//

#define AMSTR_FOLLOWON     text[TXT_AMSTR_FOLLOWON]
#define AMSTR_FOLLOWOFF    text[TXT_AMSTR_FOLLOWOFF]
#define AMSTR_GRIDON       text[TXT_AMSTR_GRIDON]
#define AMSTR_GRIDOFF      text[TXT_AMSTR_GRIDOFF]
#define AMSTR_MARKEDSPOT   text[TXT_AMSTR_MARKEDSPOT]
#define AMSTR_MARKSCLEARED text[TXT_AMSTR_MARKSCLEARED]

//
//      ST_stuff.C
//

#define STSTR_MUS          text[TXT_STSTR_MUS]
#define STSTR_NOMUS        text[TXT_STSTR_NOMUS]
#define STSTR_DQDON        text[TXT_STSTR_DQDON]
#define STSTR_DQDOFF       text[TXT_STSTR_DQDOFF]
#define STSTR_KFAADDED     text[TXT_STSTR_KFAADDED]
#define STSTR_FAADDED      text[TXT_STSTR_FAADDED]
#define STSTR_NCON         text[TXT_STSTR_NCON]
#define STSTR_NCOFF        text[TXT_STSTR_NCOFF]
#define STSTR_BEHOLD       text[TXT_STSTR_BEHOLD]
#define STSTR_BEHOLDX      text[TXT_STSTR_BEHOLDX]
#define STSTR_CHOPPERS     text[TXT_STSTR_CHOPPERS]
#define STSTR_CLEV         text[TXT_STSTR_CLEV]


// heretic
#define GOT_AMMOGOLDWAND1        text[TXT_AMMOGOLDWAND1]
#define GOT_AMMOGOLDWAND2        text[TXT_AMMOGOLDWAND2]
#define GOT_AMMOMACE1            text[TXT_AMMOMACE1]
#define GOT_AMMOMACE2            text[TXT_AMMOMACE2]
#define GOT_AMMOCROSSBOW1        text[TXT_AMMOCROSSBOW1]
#define GOT_AMMOCROSSBOW2        text[TXT_AMMOCROSSBOW2]
#define GOT_AMMOBLASTER1         text[TXT_AMMOBLASTER1]
#define GOT_AMMOBLASTER2         text[TXT_AMMOBLASTER2]
#define GOT_AMMOSKULLROD1        text[TXT_AMMOSKULLROD1]
#define GOT_AMMOSKULLROD2        text[TXT_AMMOSKULLROD2]
#define GOT_AMMOPHOENIXROD1      text[TXT_AMMOPHOENIXROD1]
#define GOT_AMMOPHOENIXROD2      text[TXT_AMMOPHOENIXROD2]

#define GOT_WPNMACE              text[TXT_WPNMACE]
#define GOT_WPNCROSSBOW          text[TXT_WPNCROSSBOW]
#define GOT_WPNBLASTER           text[TXT_WPNBLASTER]
#define GOT_WPNSKULLROD          text[TXT_WPNSKULLROD]
#define GOT_WPNPHOENIXROD        text[TXT_WPNPHOENIXROD]
#define GOT_WPNGAUNTLETS         text[TXT_WPNGAUNTLETS]

#define CHEAT_GODON           text[TXT_CHEATGODON]
#define CHEAT_GODOFF          text[TXT_CHEATGODOFF]
#define CHEAT_NOCLIPON        text[TXT_CHEATNOCLIPON]
#define CHEAT_NOCLIPOFF       text[TXT_CHEATNOCLIPOFF]
#define CHEAT_WEAPONS         text[TXT_CHEATWEAPONS]
#define CHEAT_FLIGHTON        text[TXT_CHEATFLIGHTON]
#define CHEAT_FLIGHTOFF       text[TXT_CHEATFLIGHTOFF]
#define CHEAT_POWERON         text[TXT_CHEATPOWERON]
#define CHEAT_POWEROFF        text[TXT_CHEATPOWEROFF]
#define CHEAT_HEALTH          text[TXT_CHEATHEALTH]
#define CHEAT_KEYS            text[TXT_CHEATKEYS]
#define CHEAT_SOUNDON         text[TXT_CHEATSOUNDON]
#define CHEAT_SOUNDOFF        text[TXT_CHEATSOUNDOFF]
#define CHEAT_TICKERON        text[TXT_CHEATTICKERON]
#define CHEAT_TICKEROFF       text[TXT_CHEATTICKEROFF]
#define CHEAT_ARTIFACTS1      text[TXT_CHEATARTIFACTS1]
#define CHEAT_ARTIFACTS2      text[TXT_CHEATARTIFACTS2]
#define CHEAT_ARTIFACTS3      text[TXT_CHEATARTIFACTS3]
#define CHEAT_ARTIFACTSFAIL   text[TXT_CHEATARTIFACTSFAIL]
#define CHEAT_WARP            text[TXT_CHEATWARP]
#define CHEAT_SCREENSHOT      text[TXT_CHEATSCREENSHOT]
#define CHEAT_CHICKENON       text[TXT_CHEATCHICKENON]
#define CHEAT_CHICKENOFF      text[TXT_CHEATCHICKENOFF]
#define CHEAT_MASSACRE        text[TXT_CHEATMASSACRE]
#define CHEAT_IDDQD           text[TXT_CHEATIDDQD]
#define CHEAT_IDKFA           text[TXT_CHEATIDKFA]


#endif
