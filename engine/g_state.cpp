// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// $Log$
// Revision 1.35  2004/08/12 18:30:23  smite-meister
// cleaned startup
//
// Revision 1.34  2004/07/14 16:13:13  smite-meister
// cleanup, commands
//
// Revision 1.33  2004/07/13 20:23:36  smite-meister
// Mod system basics
//
// Revision 1.32  2004/07/11 14:32:00  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.31  2004/07/05 16:53:24  smite-meister
// Netcode replaced
//
// Revision 1.30  2004/04/25 16:26:49  smite-meister
// Doxygen
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
#include "hu_stuff.h" // HUD
#include "p_camera.h" // camera

#include "w_wad.h"
#include "z_zone.h"


void F_Ticker();
void P_ACSInitNewGame();



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
      int **teamfrags = (int **)(new int[m][m]);

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
      delete [] teamfrags;
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

  // check fraglimit cvar TODO how does this work? when are the scores (or teamscores) zeroed?
  if (cv_fraglimit.value && CheckScoreLimit())
    {
      // go on to the next map
      //currentmap->ExitMap(NULL, 0);
      //currentcluster->Finish(currentmap->nextlevel, 0);
    }


  // do things to change the game state
  while (action != ga_nothing)
    switch (action)
      {
      case ga_intermission:
	StartIntermission();
	break;
      case ga_nothing:
	break;
      default : I_Error("game.action = %d\n", action);
      }


  // TODO read/write demo ticcmd's here


  // do main actions
  switch (state)
    {
    case GS_INTRO:
      if (--pagetic <= 0)
	AdvanceIntro();
      break;

    case GS_LEVEL:
      if (!paused)
	currentcluster->Ticker();
      hud.Ticker();
      automap.Ticker();
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
	      p->ExitLevel(0, 0);
	      RemovePlayer(p->number);
	      // TODO purge the removed players from the frag maps of other players?
	      continue;
	    }
	  else if (p->playerstate == PST_NEEDMAP)
	    {
	      CONS_Printf("Map request..");

	      // assign the player to a map
	      if (p->requestmap == 0)
		m = currentcluster->maps[0]; // first map in cluster
	      else
		m = FindMapInfo(p->requestmap);

	      if (!m || currentcluster->number != m->cluster)
		{
		  // TODO minor thing: if several players exit maps on the same tick,
		  // and someone besides the first one causes a cluster change, some
		  // maps could be loaded in vain...

		  // cluster change!
		  currentcluster->Finish(p->requestmap, p->entrypoint);

		  if (m)
		    {
		      MapCluster *next = FindCluster(m->cluster);
		      StartFinale(next);
		      currentcluster = next;
		    }
		  else
		    {
		      // game ends here
		      StartFinale(NULL);
		    }

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




/// starts or restarts the game. assumes that we have set up the clustermap.
bool GameInfo::StartGame(skill_t sk, int cluster)
{
  if (clustermap.empty() || mapinfo.empty())
    return false;

  CONS_Printf("Starting a game\n");

  currentcluster = FindCluster(cluster);
  if (!currentcluster)
    currentcluster = clustermap.begin()->second;

  skill = sk;

  // set cvars
  if (skill == sk_nightmare)
    {
      cv_respawnmonsters.Set(1);
      cv_fastmonsters.Set(1);
    }
  else
    {
      cv_respawnmonsters.Set(0);
      cv_fastmonsters.Set(0);
    }

  cv_deathmatch.Set(0);
  cv_timelimit.Set(0);
  cv_fraglimit.Set(0);



  if (paused)
    {
      paused = false;
      S.ResumeMusic();
    }

  extern bool force_wipe;
  force_wipe = true;

  state = GS_LEVEL;
  action = ga_nothing;

  player_iter_t i;
  for (i = Players.begin(); i != Players.end(); i++)
    (*i).second->Reset(true, true);

  P_ACSInitNewGame(); // clear the ACS world vars etc.

  memset(gamekeydown, 0, sizeof(gamekeydown));  // clear cmd building stuff

  // view the guy you are playing
  displayplayer = consoleplayer;
  displayplayer2 = consoleplayer2;

  // clear hud messages remains (usually from game startup)
  con.ClearHUD();
  automap.Close();

#ifdef PARANOIA
  Z_CheckHeap(-2);
#endif

  return true;
}



// start intermission
void GameInfo::StartIntermission()
{
  action = ga_nothing;
  // TODO separate server, client stuff in intermission

  hud.ST_Stop();

  // detach chasecam
  if (camera.chase)
    camera.ClearCamera();

  automap.Close();

  //state = GS_INTERMISSION;
  //wi.Start(currentcluster, next);
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
  if (state == GS_FINALE)
    state = GS_LEVEL;

  action = ga_nothing;
}
