// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
//
// $Log$
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
mobjtype_t Doom_Pawns[] = {
  MT_PLAYER, MT_POSSESSED, MT_SHOTGUY, MT_TROOP, MT_SERGEANT, MT_SHADOWS,
  MT_SKULL, MT_HEAD, MT_BRUISER, MT_SPIDER, MT_CYBORG, mobjtype_t(-1)
};

mobjtype_t Doom2_Pawns[] = {
  MT_PLAYER, MT_POSSESSED, MT_SHOTGUY, MT_CHAINGUY, MT_TROOP, MT_SERGEANT, MT_SHADOWS,
  MT_SKULL, MT_HEAD, MT_KNIGHT, MT_BRUISER, MT_BABY, MT_PAIN, MT_UNDEAD, MT_FATSO,
  MT_VILE, MT_SPIDER, MT_CYBORG, MT_WOLFSS, mobjtype_t(-1)
};

mobjtype_t Heretic_Pawns[] = {
  MT_PLAYER, //FIXME to a real heretic
  MT_CHICKEN,
  MT_MUMMY, MT_MUMMYLEADER, MT_MUMMYGHOST, MT_MUMMYLEADERGHOST,
  MT_BEAST, MT_SNAKE, MT_HHEAD, MT_CLINK, MT_WIZARD,
  MT_IMP, MT_IMPLEADER, MT_HKNIGHT, MT_KNIGHTGHOST,
  MT_SORCERER1, MT_SORCERER2, MT_MINOTAUR, mobjtype_t(-1)
};

vector<mobjtype_t> allowed_pawns; // FIXME temporary solution

PlayerInfo::PlayerInfo(const string & n)
{
  // TODO: finish initialization...
  name = n;
  number = 0;
  team = 0;
  playerstate = PST_WAITFORMAP;
  frags.resize(game.maxplayers);
  pawntype = MT_PLAYER;
};


// was P_CheckFragLimit
// WARNING : check cv_fraglimit>0 before call this function !
void PlayerInfo::CheckFragLimit()
{
  // only checks score, doesn't count it. score must therefore be
  // updated in real time
  if (cv_teamplay.value)
    {
      int teamscore = game.teams[team]->score;

      if(cv_fraglimit.value <= teamscore)
	game.ExitLevel(0);
    }
  else
    {
      if (cv_fraglimit.value <= score)
	game.ExitLevel(0);
    }
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
      frags.clear();
      frags.resize(game.players.size(), 0);
    }

  return;
}
