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
int P_Read_ANIMATED(int lump);
int P_Read_ANIMDEFS(int lump);
void P_InitSwitchList();
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


void GameInfo::UpdateScore(PlayerInfo *killer, PlayerInfo *victim)
{
  killer->Frags[victim->number]++;
  
  // scoring rule
  if (killer->team == 0)
    if (killer != victim)
      killer->score++;
    else
      killer->score--;
  else if (killer->team != victim->team)
    {
      teams[killer->team]->score++;
      killer->score++;
    }
  else
    {
      teams[killer->team]->score--;
      killer->score--;
    }
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
  MapInfo *m;
  PlayerInfo *p;
  player_iter_t t;


  tic++;

  // TODO fix the intermissions and finales. when should they appear in general?
  // perhaps they should be made client-only stuff, the server waits until the clients are ready to continue.

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
      case ga_nextlevel:
	NextLevel();
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


  if (state == GS_LEVEL)
    {
      // manage players
      for (t = Players.begin(); t != Players.end(); )
	{
	  p = (*t).second;
	  t++; // because "old t" may be invalidated
	  if (p->playerstate == PST_REMOVE)
	    {
	      // the player is removed from the game (invalidates "old t")
	      p->ExitLevel(0, 0);
	      RemovePlayer(p->number);
	    }
	}

      for (t = Players.begin(); t != Players.end(); t++)
	{
	  p = (*t).second;
	  if (p->playerstate == PST_WAITFORMAP)
	    {
	      CONS_Printf("Map request..");
	      if (p->requestmap < 0)
		{
		  // TODO game ends!
		  //action = ga_intermission;
		  break;
		}
	      
	      // assign the player to a map
	      m = FindMapInfo(p->requestmap);
	      if (!m)
		m = *currentcluster->maps.begin();

	      if (currentcluster->number != m->cluster)
		{
		  // cluster change, _everyone_ follows p! (even if they already have destinations set!)
		  // TODO this is a bit convoluted, but should work.
		  for (player_iter_t s = Players.begin(); s != Players.end(); s++)
		    {
		      PlayerInfo *r = (*s).second;
		      r->requestmap = p->requestmap;
		      r->entrypoint = p->entrypoint;
		      r->Reset(true, true); // everything goes.
		    }
		  currentcluster->Finish();
		  currentcluster = FindCluster(m->cluster);

		  //action = ga_intermission;
		  //break; // this is important!
		}

	      p->Reset(!currentcluster->keepstuff, true);

	      // normal individual mapchange
	      if (!m->Activate(p))
		I_Error("Darn!\n");
	    }
	}
    }
}


/// starts a new local game (assumes that we have done a SV_Reset()!)
bool GameInfo::NewGame(skill_t sk)
{
  if (clustermap.empty())
    return false;

  CONS_Printf("Starting a game\n");

  if (!dedicated)
    {
      // add local players
      consoleplayer = AddPlayer(new PlayerInfo(localplayer));
      if (cv_splitscreen.value)
	consoleplayer2 = AddPlayer(new PlayerInfo(localplayer2));


      // read these lumps _after_ MAPINFO but not separately for each map
      extern bool nosound;
      if (!nosound)
	{
	  //S_ClearSounds();
	  int n = fc.Size();
	  for (int i = 1; i < n; i++)
	    {
	      // cumulative reading
	      S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", i));
	      S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", i));
	    }

	  if (cv_precachesound.value)
	    S_PrecacheSounds();
	}
    }

  // texture and flat animations
  if (P_Read_ANIMDEFS(fc.FindNumForName("ANIMDEFS")) < 0)
    P_Read_ANIMATED(fc.FindNumForName("ANIMATED"));

  // set switch texture names/numbers, read "SWITCHES" lump
  P_InitSwitchList();

  if (sk > sk_nightmare)
    sk = sk_nightmare;

  skill = sk;

  // set cvars
  if (skill == sk_nightmare)
    {
      CV_SetValue(&cv_respawnmonsters, 1);
      CV_SetValue(&cv_fastmonsters, 1);
    }
  else
    {
      CV_SetValue(&cv_respawnmonsters, 0);
      CV_SetValue(&cv_fastmonsters, 0);
    }

  CV_SetValue(&cv_deathmatch, 0);
  CV_SetValue(&cv_timelimit, 0);
  CV_SetValue(&cv_fraglimit, 0);

  paused = false;

  StartGame();

  return true;
}



/// starts or restarts the game.
bool GameInfo::StartGame()
{
  if (clustermap.empty() || mapinfo.empty())
    return false;

  cluster_iter_t t = clustermap.begin();
  currentcluster = (*t).second; 

  if (paused)
    {
      paused = false;
      S.ResumeMusic();
    }

  //Fab:19-07-98:start cd music for this level (note: can be remapped)
  /*
    FIXME cd music
  if (game.mode==commercial)
    I_PlayCD (map, true);                // Doom2, 32 maps
  else
    I_PlayCD ((episode-1)*9+map, true);  // Doom1, 9maps per episode
  */


  extern bool force_wipe;
  force_wipe = true;

  state = GS_LEVEL;
  action = ga_nothing;

  player_iter_t i;
  for (i = Players.begin(); i != Players.end(); i++)
    (*i).second->Reset(true, true);

  M_ClearRandom();
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

  hud.ST_Stop();

  // detach chasecam
  if (camera.chase)
    camera.ClearCamera();

  automap.Close();

  //state = GS_INTERMISSION;
  //wi.Start(currentcluster, nextcluster);
}


// called when intermission ends
// init next level or go to the final scene
void GameInfo::EndIntermission()
{
  // TODO purge the removed players from the frag maps

  // check need for finale
  if (cv_deathmatch.value == 0)
    {
      // check winning
      if (nextcluster == NULL)
	{
	  // disconnect from network
	  //CL_Reset();
	  state = GS_FINALE;
	  F_StartFinale(currentcluster, false, true); // final piece of story is exittext
	  return;
	}

      int c = currentcluster->number;
      int n = nextcluster->number;
      // check "mid-game finale" (story) (cluster change)
      if (n != c)
	{
	  if (!(nextcluster->entertext.empty()))
	    {
	      state = GS_FINALE;
	      F_StartFinale(nextcluster, true, false);
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

  action = ga_nextlevel;
}

void GameInfo::EndFinale()
{
  if (state == GS_FINALE)
    action = ga_nextlevel;
}


// load next level

void GameInfo::NextLevel()
{
  action = ga_nothing;
  state = GS_LEVEL;
}
