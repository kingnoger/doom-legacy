// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.23  2003/12/09 01:02:00  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.22  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.21  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.20  2003/11/30 00:09:43  smite-meister
// bugfixes
//
// Revision 1.19  2003/11/27 11:28:25  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.18  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.17  2003/11/12 11:07:19  smite-meister
// Serialization done. Map progression.
//
// Revision 1.16  2003/06/10 22:39:55  smite-meister
// Bugfixes
//
// Revision 1.15  2003/05/11 21:23:50  smite-meister
// Hexen fixes
//
// Revision 1.14  2003/05/05 00:24:48  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.13  2003/04/24 20:30:02  hurdler
// Remove lots of compiling warnings
//
// Revision 1.12  2003/04/04 00:01:54  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.11  2003/03/08 16:07:02  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.10  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.9  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.8  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.7  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.6  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.5  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.4  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.3  2002/12/16 22:11:12  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:11:39  smite-meister
// Blindness and missile clipping bugs fixed
//
//
// DESCRIPTION:
//    Part of GameInfo class implementation. Methods related to game state changes.
//-----------------------------------------------------------------------------

#include "g_game.h"
#include "g_player.h"
#include "g_team.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_level.h"
#include "g_pawn.h"


#include "g_input.h" // gamekeydown!
#include "m_misc.h" // FIL_* file functions
#include "dstrings.h"
#include "m_random.h"
#include "m_menu.h"
#include "am_map.h"

#include "s_sound.h"
#include "sounds.h"

#include "f_finale.h"
#include "d_netcmd.h"
#include "d_clisrv.h"
#include "hu_stuff.h" // HUD
#include "p_camera.h" // camera
#include "wi_stuff.h"
#include "console.h"

#include "w_wad.h"
#include "z_zone.h"


// Here's for the german edition.
// IF NO WOLF3D LEVELS, NO SECRET EXIT!

void S_Read_SNDINFO(int lump);


language_t   language = la_english;            // Language.

GameInfo game;

// constructor
TeamInfo::TeamInfo()
{
  name = "Romero's Roughnecks";
  color = 0;
  score = 0;
}


// constructor
GameInfo::GameInfo()
{
  demoversion = VERSION;
  mode = gm_none;
  mission = gmi_doom2;
  state = GS_NULL;
  wipestate = GS_DEMOSCREEN;
  action = ga_nothing;
  skill = sk_medium;
  maxplayers = 32;
  maxteams = 4;
  //teams.resize(maxteams); // idiot!
  currentcluster = NULL;
  currentmap = NULL;
};

//destructor
GameInfo::~GameInfo()
{
  ClearPlayers();
};


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

  extern consvar_t cv_teamplay;
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


// Tries to add a player into the game. The new player gets the number pnum
// if it is free, otherwise she gets the next free number.
// An already constructed PI can be given in "in", if necessary.
// Returns NULL if a new player cannot be added.

PlayerInfo *GameInfo::AddPlayer(int pnum, PlayerInfo *p)
{
  // a negative pnum just uses the first free slot
  int n = Players.size();

  if (n >= maxplayers)
    return NULL;  // no room in game

  // TODO what if maxplayers has recently been set to a lower-than-n value?
  // when are the extra players kicked? cv_maxplayers action func?

  if (pnum > maxplayers)
    pnum = -1;  // pnum too high

  // check if pnum is free
  if (pnum > 0 && Players.count(pnum))
    pnum = -1; // pnum already taken

  // find first free player number, if necessary
  if (pnum < 0)
    {
      for (int j = 1; j <= maxplayers; j++)
	if (Players.count(j) == 0)
	  {
	    pnum = j;
	    break;
	  }
    }

  // pnum is valid and free!
  // if a valid PI is given, use it. Otherwise, create a new PI.
  if (p == NULL)
    p = new PlayerInfo();

  p->number = pnum;
  Players[pnum] = p;

  return p;
}


// Removes a player from game.
// This and ClearPlayers are the ONLY ways a player should be removed.
bool GameInfo::RemovePlayer(int num)
{
  player_iter_t i = Players.find(num);

  if (i == Players.end())
    return false; // not found

  PlayerInfo *p = (*i).second;

  // remove avatar of player
  if (p->pawn)
    {
      p->pawn->player = NULL;
      p->pawn->Remove();
    }

  if (p == displayplayer)
    hud.ST_Stop();

  // make sure that global PI pointers are still OK
  if (consoleplayer == p) consoleplayer = NULL;
  if (consoleplayer2 == p) consoleplayer2 = NULL;

  if (displayplayer == p) displayplayer = NULL;
  if (displayplayer2 == p) displayplayer2 = NULL;

  delete p;
  // NOTE! because PI's are deleted, even local PI's must be dynamically
  // allocated, using a copy constructor: new PlayerInfo(localplayer).
  Players.erase(i);
  return true;
}

// Removes all players from a game.
void GameInfo::ClearPlayers()
{
  player_iter_t i;
  PlayerInfo *p;

  for (i = Players.begin(); i != Players.end(); i++)
    {
      p = (*i).second;
      // remove avatar of player
      if (p->pawn)
	{
	  p->pawn->player = NULL;
	  p->pawn->Remove();
	}
      delete p;
    }

  Players.clear();
  consoleplayer = consoleplayer2 = NULL;
  displayplayer = displayplayer2 = NULL;
  hud.ST_Stop();
}

void F_Ticker();
void D_PageTicker();

// ticks the game forward in time
void GameInfo::Ticker()
{
  extern bool dedicated;
  MapInfo *m;
  PlayerInfo *p;
  player_iter_t t;

  // TODO fix the intermissions and finales. when should they appear in general?
  // perhaps they should be made client-only stuff, the server waits until the clients are ready to continue.

  // check fraglimit cvar TODO how does this work? when are the scores (or teamscores) zeroed?
  if (cv_fraglimit.value && CheckScoreLimit())
    {
      // go on to the next map
      //currentmap->ExitMap(NULL, 0);
      //currentcluster->Finish(currentmap->nextlevel, 0);
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
	      p->ExitLevel(0, 0, false);
	      p->mp->RemovePlayer(p);
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
	      if (m == NULL)
		m = *currentcluster->maps.begin();

	      if (currentcluster->number != m->cluster)
		{
		  // cluster change, everyone follows p!
		  currentcluster->Finish(p->requestmap, p->entrypoint);
		  currentcluster = FindCluster(m->cluster);

		  //action = ga_intermission;
		  break; // this is important!
		}

	      CONS_Printf("activating %d...", m->mapnumber);

	      // normal individual mapchange
	      if (!m->Activate())
		I_Error("Darn!\n");

	      p->Reset(!currentcluster->keepstuff, true);
	      m->me->AddPlayer(p);
	    }
	}
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

  //CONS_Printf("======== GI::Ticker, tic %d, st %d\n", gametic, state);


  // read/write demo
  if (!dedicated)
    for (t = Players.begin(); t != Players.end(); t++)
      {
	ticcmd_t *cmd = &((*t).second)->cmd;
	int j = (*t).first; // playernum
	  
	if (demoplayback)
	  ReadDemoTiccmd(cmd, j);
	else
	  {
	    // FIXME here the netcode is bypassed until it's fixed. See also G_BuildTiccmd()!
	    //memcpy(cmd, &netcmds[buf][players[i]->number-1], sizeof(ticcmd_t));
	  }

	if (demorecording)
	  WriteDemoTiccmd(cmd, j);
      }

  // do main actions
  switch (state)
    {
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

    case GS_DEMOSCREEN:
      D_PageTicker();
      break;
      
    case GS_WAITINGPLAYERS:
    case GS_DEDICATEDSERVER:
    case GS_NULL:
    default:
      // do nothing
      break;
    }
}

// starts a new local game
bool GameInfo::DeferredNewGame(skill_t sk, bool splitscreen)
{
  CONS_Printf("Deferred:Starting a new game\n");
  if (clustermap.empty())
    return false;

  CONS_Printf("Deferred: really\n");

  // read these lumps _after_ MAPINFO but not separately for each map
  Read_SNDINFO(fc.FindNumForName("SNDINFO"));
  //S_Read_SNDSEQ(fc.FindNumForName("SNDSEQ"));

  Downgrade(VERSION);
  paused = false;
  nomonsters = false;

  if (sk > sk_nightmare)
    sk = sk_nightmare;

  skill = sk;

  if (demoplayback)
    COM_BufAddText ("stopdemo\n");

  // this leaves the actual game if needed
  SV_StartSinglePlayerServer();

  // delete old players
  ClearPlayers();

  // add local players
  consoleplayer = AddPlayer(-1, new PlayerInfo(localplayer));

  if (splitscreen)
    consoleplayer2 = AddPlayer(-1, new PlayerInfo(localplayer2));
  
  COM_BufAddText (va("splitscreen %d;deathmatch 0;fastmonsters 0;"
		     "respawnmonsters 0;timelimit 0;fraglimit 0\n",
		     splitscreen));

  COM_BufAddText("restartgame\n");
  return true;
}


void P_InitSwitchList();
void P_ACSInitNewGame();

// starts or restarts the game.
bool GameInfo::StartGame()
{
  CONS_Printf("Starting a new game, any time now\n");

  if (clustermap.empty() || mapinfo.empty())
    return false;

  CONS_Printf("Starting a new game, %d clusters\n", clustermap.size());

  cluster_iter_t t = clustermap.begin();
  currentcluster = (*t).second; 
  //currentmap = *currentcluster->maps.begin();

  automap.Close();  // TODO client-only stuff...

  //added:27-02-98: disable selected features for compatibility with
  //                older demos, plus reset new features as default
  if (!Downgrade(demoversion))
    {
      CONS_Printf("Cannot Downgrade engine.\n");
      CL_Reset();
      StartIntro();
      return false;
    }

  if (paused)
    {
      paused = false;
      S.ResumeMusic();
    }

  M_ClearRandom();

  if (server && skill == sk_nightmare)
    {
      CV_SetValue(&cv_respawnmonsters,1);
      CV_SetValue(&cv_fastmonsters,1);
    }

  // this should be CL_Reset or something...
  //playerdeadview = false;

  // FIXME make map restart option work, do not FreeTags and Setup,
  // instead just Reset the maps

  if (wipestate == GS_LEVEL)
    wipestate = GS_WIPE;  // force a wipe

  state = GS_LEVEL;

  player_iter_t i;
  for (i = Players.begin(); i != Players.end(); i++)
    (*i).second->Reset(true, true);

  // set switch texture names/numbers (TODO bad design, fix...)
  P_InitSwitchList();

  P_ACSInitNewGame(); // clear the ACS world vars etc.

  //Fab:19-07-98:start cd music for this level (note: can be remapped)
  /*
    FIXME cd music
  if (game.mode==commercial)
    I_PlayCD (map, true);                // Doom2, 32 maps
  else
    I_PlayCD ((episode-1)*9+map, true);  // Doom1, 9maps per episode
  */

  //AM_LevelInit()
  //BOT_InitLevelBots ();

  // TODO client stuff!!!
  displayplayer = consoleplayer;          // view the guy you are playing
  if (cv_splitscreen.value)
    displayplayer2 = consoleplayer2;
  else
    displayplayer2 = NULL;

  action = ga_nothing;

#ifdef PARANOIA
  Z_CheckHeap(-2);
#endif

  // clear cmd building stuff
  memset(gamekeydown, 0, sizeof(gamekeydown));

  // clear hud messages remains (usually from game startup)
  CON_ClearHUD();
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
	  CL_Reset();
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
      if (nextcluster == NULL)
	CL_Reset();
    }

  action = ga_nextlevel;
}

void GameInfo::EndFinale()
{
  if (state == GS_FINALE)
    action = ga_nextlevel;
}

// was G_DoWorldDone
// load next level

void GameInfo::NextLevel()
{
  action = ga_nothing;
  state = GS_LEVEL;
}
