// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
//
// $Log$
// Revision 1.12  2003/05/11 21:23:50  smite-meister
// Hexen fixes
//
// Revision 1.11  2003/04/04 00:01:54  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.10  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.9  2003/03/15 20:07:14  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.8  2003/03/08 16:07:00  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.7  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.6  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.5  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.4  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
//
//
// DESCRIPTION:
//   PlayerInfo class implementation
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "g_player.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "d_netcmd.h" // consvars

// global data

// PI's for both local players. Used by menu to setup their properties.
// These are only "models" for local players, the actual PI's
// that are added to a game are copy constructed from these.
PlayerInfo localplayer("Batman");
PlayerInfo localplayer2("Robin");

PlayerInfo *consoleplayer = NULL;   // player taking events
PlayerInfo *consoleplayer2 = NULL;   // secondary player taking events
PlayerInfo *displayplayer = NULL;   // view being displayed
PlayerInfo *displayplayer2 = NULL;  // secondary view (splitscreen)


// lists of mobjtypes that can be played by humans!
pawn_info_t pawndata[] = 
{
  {MT_PLAYER,   wp_pistol,  50, MT_NONE}, // 0
  {MT_POSSESSED, wp_pistol,  20, MT_NONE},
  {MT_SHOTGUY,  wp_shotgun,  8, MT_NONE},
  {MT_TROOP,    wp_nochange, 0, MT_TROOPSHOT},
  {MT_SERGEANT, wp_nochange, 0, MT_NONE},
  {MT_SHADOWS,  wp_nochange, 0, MT_NONE},
  {MT_SKULL,    wp_nochange, 0, MT_NONE},
  {MT_HEAD,     wp_nochange, 0, MT_HEADSHOT},
  {MT_BRUISER,  wp_nochange, 0, MT_BRUISERSHOT},
  {MT_SPIDER,   wp_chaingun, 100, MT_NONE},
  {MT_CYBORG,   wp_missile,  20,  MT_NONE}, //10

  {MT_WOLFSS,   wp_chaingun, 50, MT_NONE},
  {MT_CHAINGUY, wp_chaingun, 50, MT_NONE},
  {MT_KNIGHT,   wp_nochange, 0,  MT_BRUISERSHOT},
  {MT_BABY,     wp_plasma,  50,  MT_ARACHPLAZ},
  {MT_PAIN,     wp_nochange, 0,  MT_SKULL},
  {MT_UNDEAD,   wp_nochange, 0,  MT_TRACER},
  {MT_FATSO,    wp_nochange, 0,  MT_FATSHOT},
  {MT_VILE,     wp_nochange, 0,  MT_FIRE}, // 18

  {MT_HPLAYER,  wp_goldwand, 50, MT_NONE},
  {MT_CHICKEN,  wp_beak,      0, MT_NONE},
  {MT_MUMMY,    wp_nochange, 0, MT_NONE},
  {MT_MUMMYLEADER, wp_nochange, 0, MT_MUMMYFX1},
  {MT_MUMMYGHOST,  wp_nochange, 0, MT_NONE},
  {MT_MUMMYLEADERGHOST, wp_nochange, 0, MT_MUMMYFX1},
  {MT_BEAST,    wp_nochange, 0, MT_BEASTBALL},
  {MT_SNAKE,    wp_nochange, 0, MT_SNAKEPRO_A},
  {MT_HHEAD,    wp_nochange, 0, MT_HEADFX1},
  {MT_CLINK,    wp_nochange, 0, MT_NONE},
  {MT_WIZARD,   wp_nochange, 0, MT_WIZFX1},
  {MT_IMP,      wp_nochange, 0, MT_NONE},
  {MT_IMPLEADER,wp_nochange, 0, MT_IMPBALL},
  {MT_HKNIGHT,  wp_nochange, 0, MT_KNIGHTAXE},
  {MT_KNIGHTGHOST, wp_nochange, 0, MT_REDAXE},
  {MT_SORCERER1, wp_nochange, 0, MT_SRCRFX1},
  {MT_SORCERER2, wp_nochange, 0, MT_SOR2FX1},
  {MT_MINOTAUR,  wp_nochange, 0, MT_MNTRFX1}, // 36

  {MT_PLAYER_FIGHTER, wp_fpunch, 0, MT_NONE},
  {MT_PLAYER_CLERIC, wp_cmace, 0, MT_NONE},
  {MT_PLAYER_MAGE, wp_mwand, 0, MT_NONE}
};


PlayerInfo::PlayerInfo(const string & n)
{
  // TODO: finish initialization...
  name = n;
  number = 0;
  team = 0;
  pawntype = 0;
  pclass = 0;
  color = 0;

  message = NULL;

  playerstate = PST_WAITFORMAP;
};



//--------------------------------------------------------------------------
// was P_SetMessage


bool ultimatemsg;

void PlayerInfo::SetMessage(const char *msg, bool ultmsg)
{
  if ((ultimatemsg || !cv_showmessages.value) && !ultmsg)
    return;
    
  message = msg;
  //player->messageTics = MESSAGETICS;
  //BorderTopRefresh = true;
  if (ultmsg)
    ultimatemsg = true;
}


// Reset players between levels
void PlayerInfo::Reset(bool resetpawn, bool resetfrags)
{
  // if player didn't get out alive, reset his pawn anyway
  if (playerstate != PST_LIVE)
    resetpawn = true;

  playerstate = PST_WAITFORMAP;
  kills = items = secrets = 0;

  if (resetpawn)
    {
      // TODO: actually, only PST_LIVE players _can_ have pawns at this point...
      if (pawn)
	{
	  delete pawn;
	  pawn = NULL;
	}
    }

#ifdef CLIENTPREDICTION2
      spirit = NULL;
#endif

  // Initial height of PointOfView
  // will be set by player think.
  viewz = 1;

  if (resetfrags)
    {
      score = 0;
      Frags.clear();
    }

  return;
}
