// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Part of GameInfo class implementation.
/// Methods related to game state changes.


#include "g_game.h"
#include "g_player.h"
#include "g_team.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_level.h"
#include "g_pawn.h"

#include "command.h"
#include "console.h"
#include "cvars.h"

#include "g_input.h" // gamekeydown!
#include "dstrings.h"
#include "m_random.h"
#include "m_menu.h"
#include "am_map.h"

#include "sounds.h"
#include "s_sound.h"

#include "f_finale.h"
#include "wi_stuff.h"
#include "hud.h"

#include "w_wad.h"
#include "z_zone.h"


void F_Ticker();


// WARNING : check cv_fraglimit>0 before call this function !
bool GameInfo::CheckScoreLimit()
{
  // only checks score, doesn't count it. score must therefore be
  // updated in real time
  if (cv_teamplay.value)
    {
      int i, n = teams.size();
      for (i=0; i<n; i++)
	if (teams[i]->score >= cv_fraglimit.value)
	  return true;
    }
  else
    {
      player_iter_t i;
      for (i = Players.begin(); i != Players.end(); i++)
	if ((*i).second->score >= cv_fraglimit.value)
	  return true;
    }
  return false;
}



int GameInfo::GetFrags(fragsort_t **fragtab, int type)
{
  // PlayerInfo class holds a raw frags table for each player.
  // It also holds a cumulative, game type sensitive frag field named score
  // if the game type is changed in the middle of a game, score is NOT zeroed,
  // just the scoring system is different from there on.

  // scoring is done in PlayerPawn::Kill

  int i, j;
  int n = Players.size();

  int ret;
  fragsort_t *ft;

  player_iter_t u, w;

  if (cv_teamplay.value)
    {
      int m = teams.size();
      ft = new fragsort_t[m];
	  
	  //FIXME: gcc-4 did not like this kind of dynamic allocation so I changed it to static. Later shall be fixed.
#if __GNUC__ >= 4
	  const int cm = m;
      // int **teamfrags = new int[cm][cm];
	  int teamfrags[cm][cm];
#else
      int **teamfrags = (int **)(new int[m][m]);
#endif
      for (i=0; i<m; i++)
	{
	  ft[i].num   = i; // team 0 are the unteamed
	  ft[i].color = teams[i]->color;
	  ft[i].name  = teams[i]->name.c_str();
	}

      // calculate teamfrags
      for (u = Players.begin(); u != Players.end(); u++)
	{
	  int team1 = (*u).second->team;

	  for (w = Players.begin(); w != Players.end(); w++)
	    {
	      int team2 = (*w).second->team;

	      teamfrags[team1][team2] +=
		(*u).second->Frags[(*w).first];
	    }
	}

      // type is a magic number telling which fragtable we want
      switch (type)
	{
	case 0: // just normal frags
	  for (i=0; i<m; i++)
	    ft[i].count = teams[i]->score;
	  break;

	case 1: // buchholtz
	  for (i=0; i<m; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<m; j++)
		if (i != j)
		  ft[i].count += teamfrags[i][j] * teams[j]->score;
	    }
	  break;

	case 2: // individual
	  for (i=0; i<m; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<m; j++)
		if (i != j)
		  {
		    if(teamfrags[i][j] > teamfrags[j][i])
		      ft[i].count += 3;
		    else if(teamfrags[i][j] == teamfrags[j][i])
		      ft[i].count += 1;
		  }
	    }
	  break;

	case 3: // deaths
	  for (i=0; i<m; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<m; j++)
		ft[i].count += teamfrags[j][i];
	    }
	  break;
      
	default:
	  break;
	}
#if __GNUC__ < 4
      delete [] teamfrags;
#endif
      ret = m;
    }
  else
    {
      // not teamgame
      ft = new fragsort_t[n]; 
      for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	{
	  ft[i].num = (*u).second->number;
	  ft[i].color = (*u).second->pawn->color;
	  ft[i].name  = (*u).second->name.c_str();
	}

      // type is a magic number telling which fragtable we want
      switch (type)
	{
	case 0: // just normal frags
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    ft[i].count = (*u).second->score;
	  break;

	case 1: // buchholtz
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    {
	      ft[i].count = 0;
	      for (w = Players.begin(); w != Players.end(); w++)
		if (u != w)
		  {
		    int k = (*w).first;
		    ft[i].count += (*u).second->Frags[k]*((*w).second->score + (*w).second->Frags[k]);
		    // FIXME is this formula correct?
		  }
	    }
	  break;

	case 2: // individual
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    {
	      ft[i].count = 0;
	      for (w = Players.begin(); w != Players.end(); w++)
		if (u != w)
		  {
		    int k = (*u).first;
		    int l = (*w).first;
		    if ((*u).second->Frags[l] > (*w).second->Frags[k])
		      ft[i].count += 3;
		    else if ((*u).second->Frags[l] == (*w).second->Frags[k])
		      ft[i].count += 1;
		  }
	    }
	  break;

	case 3: // deaths
	  for (i = 0, u = Players.begin(); u != Players.end(); i++, u++)
	    {
	      ft[i].count = 0;
	      for (w = Players.begin(); w != Players.end(); w++)
		ft[i].count += (*w).second->Frags[(*u).first];
	    }
	  break;
      
	default:
	  break;
	}
      ret = n;
    }

  *fragtab = ft; 
  return ret;
}


// ticks the game forward in time
void GameInfo::Ticker()
{
  tic++;

  // TODO read/write demo ticcmd's here

  // do main actions
  switch (state)
    {
    case GS_INTRO:
      if (--pagetic <= 0)
	AdvanceIntro();
      break;

    case GS_LEVEL:
      if (!paused && currentcluster)
	currentcluster->Ticker();

      if (!dedicated)
	{
	  hud.Ticker();
	  automap.Ticker();
	}
      break;

    case GS_INTERMISSION:
      wi.Ticker();
      break;

    case GS_FINALE:
      F_Ticker();
      break;

    case GS_NULL:
    default:
      // do nothing
      break;
    }


  MapInfo *m;
  PlayerInfo *p;

  if (state == GS_LEVEL)
    {
      // manage players
      for (player_iter_t t = Players.begin(); t != Players.end(); )
	{
	  p = t->second;
	  t++; // because "old t" may be invalidated

	  if (p->playerstate == PST_REMOVE)
	    {
	      // the player is removed from the game (invalidates "old t")
	      if (!p->mp)
		RemovePlayer(p->number); // first the maps throw out the removed player, then the game proper.

	      // TODO purge the removed players from the frag maps of other players?
	      continue;
	    }
	  else if (server &&
		   p->playerstate == PST_NEEDMAP)
	    {
	      // assign the player to a map
	      CONS_Printf("Map request..");

	      if (p->requestmap == 0)
		m = entrypoint->minfo;
		//m = currentcluster->maps[0]; // first map in cluster
	      else
		m = FindMapInfo(p->requestmap);

	      if (!m)
		{
		  // game ends
		  currentcluster->Finish(p->requestmap, p->entrypoint);
		  StartFinale(NULL);
		  break;
		}
	      else if (!m->found)
		m = currentcluster->maps[0];

	      // cluster change?
	      if (currentcluster->number != m->cluster)
		{
		  // TODO minor thing: if several players exit maps on the same tick,
		  // and someone besides the first one causes a cluster change, some
		  // maps could be loaded in vain...

		  // cluster change!
		  currentcluster->Finish(p->requestmap, p->entrypoint);

		  MapCluster *next = FindCluster(m->cluster);
		  StartFinale(next);
		  currentcluster = next;

		  break; // this is important! no need to check the other players.
		}

	      p->Reset(!currentcluster->keepstuff, true);

	      // normal individual mapchange
	      if (!m->Activate(p))
		I_Error("Darn!\n");
	    }
	}
    }
}



// called as a result of the rpc
void GameInfo::StartIntermission()
{
  state = GS_INTERMISSION;
}


//
void GameInfo::EndIntermission()
{
  state = GS_LEVEL;

  for (int i = 0; i < NUM_LOCALPLAYERS; i++)
    {
      PlayerInfo *p = LocalPlayers[i].info;
      if (p && p->playerstate == PST_INTERMISSION)
	if (server)
	  p->playerstate = PST_NEEDMAP;
	else
	  p->c2sIntermissionDone();
    }
}


void GameInfo::StartFinale(MapCluster *next)
{
  // check need for finale
  if (cv_deathmatch.value == 0)
    {
      // check winning
      if (!next)
	{
	  state = GS_FINALE;
	  F_StartFinale(currentcluster, false, true); // final piece of story is exittext
	  return;
	}

      int c = currentcluster->number;
      int n = next->number;
      // check "mid-game finale" (story) (requires cluster change)
      if (n != c)
	{
	  if (!next->entertext.empty())
	    {
	      state = GS_FINALE;
	      F_StartFinale(next, true, false);
	      return;
	    }
	  else if (!(currentcluster->exittext.empty()))
	    {
	      state = GS_FINALE;
	      F_StartFinale(currentcluster, false, false);
	      return;
	    }
	}
    }
  else
    {
      // no finales in deathmatch
      // FIXME end game here, show final frags
      //if (nextcluster == NULL)
	//CL_Reset();
    }
}


void GameInfo::EndFinale()
{
  state = GS_LEVEL;
}
