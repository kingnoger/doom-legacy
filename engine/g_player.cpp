// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
//
// $Log$
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


PlayerInfo::PlayerInfo(const string & n = "")
{
  // TODO: finish initialization...
  name = n;
  number = 0;
  team = 0;
  playerstate = PST_WAITFORMAP;
  frags.resize(game.maxplayers);
  pawntype = MT_CHAINGUY; //MT_PLAYER;
};


// was P_CheckFragLimit
// WARNING : check cv_fraglimit>0 before call this function !
void PlayerInfo::CheckFragLimit()
{
  // FIXME only checks score, doesn't count it. score must therefore be
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
