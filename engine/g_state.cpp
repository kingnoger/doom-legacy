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
#include "g_map.h"
#include "g_level.h"
#include "g_pawn.h"
#include "p_info.h"

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


bool         nomusic;    
bool         nosound;
language_t   language = la_english;            // Language.

GameInfo game;


void GameInfo::UpdateScore(PlayerInfo *killer, PlayerInfo *victim)
{
  killer->frags[victim->number]++;
  
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

  // check fraglimit cvar
  if (cv_fraglimit.value)
    killer->CheckFragLimit();
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
  int n = players.size();
  int m = teams.size();
  int ret;
  fragsort_t *ft;
  int **teamfrags;

  if (cv_teamplay.value)
    {
      ft = new fragsort_t[m];
      teamfrags = (int **)(new int[m][m]);

      for (i=0; i<m; i++)
	{
	  ft[i].num   = i; // team 0 are the unteamed
	  ft[i].color = teams[i]->color;
	  ft[i].name  = teams[i]->name.c_str();
	}

      // calculate teamfrags
      int team1, team2;
      for (i=0; i<n; i++)
	{
	  team1 = players[i]->team;

	  for (j=0; j<n; j++)
	    {
	      team2 = players[j]->team;

	      teamfrags[team1][team2] +=
		players[i]->frags[players[j]->number - 1];
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

    } else { // not teamgame
      ft = new fragsort_t[n]; 

      for (i=0; i<n; i++)
	{
	  ft[i].num = players[i]->number;
	  ft[i].color = players[i]->pawn->color;
	  ft[i].name  = players[i]->name.c_str();
	}

      // type is a magic number telling which fragtable we want
      switch (type)
	{
	case 0: // just normal frags
	  for (i=0; i<n; i++)
	    ft[i].count = players[i]->score;
	  break;

	case 1: // buchholtz
	  for (i=0; i<n; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<n; j++)
		if (i != j)
		  {
		    int k = players[j]->number - 1;
		    ft[i].count += players[i]->frags[k]*(players[j]->score + players[j]->frags[k]);
		    // FIXME is this formula correct?
		  }
	    }
	  break;

	case 2: // individual
	  for (i=0; i<n; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<n; j++)
		if (i != j)
		  {
		    int k = players[i]->number - 1;
		    int l = players[j]->number - 1;
		    if(players[i]->frags[l] > players[j]->frags[k])
		      ft[i].count += 3;
		    else if(players[i]->frags[l] == players[j]->frags[k])
		      ft[i].count += 1;
		  }
	    }
	  break;

	case 3: // deaths
	  for (i=0; i<n; i++)
	    {
	      ft[i].count = 0;
	      for (j=0; j<n; j++)
		ft[i].count += players[j]->frags[players[i]->number - 1];
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


// Tries to add a player into the game. The new player gets the number pnum+1
// if it is free, otherwise she gets the next free number.
// An already constructed PI can be given in "in", if necessary.
// Returns NULL if a new player cannot be added.

PlayerInfo *GameInfo::AddPlayer(int pnum, PlayerInfo *in)
{
  // a negative pnum just uses the first free slot
  int n = players.size();

  // no room in game
  if (n >= maxplayers)
    return NULL;

  // too high a pnum
  if (pnum >= maxplayers)
    pnum = -1;

  //vector<bool> present(maxplayers, false);
  /*
    // not needed anymore because now "present" vector is available at GameInfo
  for (i=0; i<n; i++)
    {
      // what if maxplayers has recently been set to a lower-than-n value?
      // when are the extra players kicked?
      present[players[i]->number-1] = true;
    }
  */

  if (pnum >= (int)present.size())
    present.resize(maxplayers);
  else if (pnum >= 0 && present[pnum])
  // pnum already taken
    pnum = -1;

  int m = present.size();
  int i;
  // find first free player number, if necessary
  if (pnum < 0)
    {
      for (i=0; i<m; i++)
	if (present[i] == NULL)
	  {
	    pnum = i;
	    break;
	  }
      // make room for one more player
      if (i == m)
	{
	  present.push_back(NULL);
	  pnum = i;
	}
    }

  // player number is not too high, has not been taken and there's room in the game!

  PlayerInfo *p;

  // if a valid PI is given, use it. Otherwise, create a new PI.
  if (in == NULL)
    p = new PlayerInfo();
  else
    p = in;

  p->number = pnum+1;
  players.push_back(p);
  present[pnum] = p;

  // FIXME the frags vectors of other players must be lengthened

  return p;
}


// Removes a player from game.
// This and ClearPlayers are the ONLY ways a player should be removed.
void GameInfo::RemovePlayer(vector<PlayerInfo *>::iterator it)
{
  // it is an iterator to _players_ vector ! NOT a player number!
  int j, n = players.size();

  if (it >= players.end())
    return;

  PlayerInfo *p = *it;

  for (j = 0 ; j<n ; j++)
    {
      // the frags vector is not compacted (shortened) now, only between levels 
      players[j]->frags[p->number - 1] = 0;
    }
	    
  // remove avatar of player
  if (p->pawn)
    {
      p->pawn->player = NULL;
      p->pawn->Remove();
    }

  if (p == displayplayer)
    hud.ST_Stop();

  // FIXME make sure that global PI pointers are still OK
  if (consoleplayer == p) consoleplayer = NULL;
  if (consoleplayer2 == p) consoleplayer2 = NULL;

  if (displayplayer == p) displayplayer = NULL;
  if (displayplayer2 == p) displayplayer2 = NULL;

  present[p->number - 1] = NULL;
  delete p;
  // NOTE! because PI's are deleted, even local PI's must be dynamically
  // allocated, using a copy constructor: new PlayerInfo(localplayer).
  players.erase(it);
}

// Removes all players from a game.
void GameInfo::ClearPlayers()
{
  int i, n = players.size();
  PlayerInfo *p;

  for (i=0; i<n; i++)
    {
      p = players[i];
      // remove avatar of player
      if (p->pawn)
	{
	  p->pawn->player = NULL;
	  p->pawn->Remove();
	}
      delete p;
    }
  present.clear();
  players.clear();
  consoleplayer = consoleplayer2 = NULL;
  displayplayer = displayplayer2 = NULL;
}

void F_Ticker();
void D_PageTicker();
//
// was G_Ticker
// Make ticcmd_ts for the players.
//
void GameInfo::Ticker()
{
  //extern ticcmd_t netcmds[32][32]; TODO
  extern bool dedicated;

  // level is physically exited -> ExitLevel(int exit)
  // ExitLevel(int exit): action is set to ga_completed
  // 
  // LevelCompleted(): end level, start intermission (set state), reset action to ga_nothing
  // intermission ends -> EndIntermission()
  // EndIntermission(): check winning/finale (set state, init finale), else set ga_worlddone
  //
  // WorldDone(): action to ga_nothing, load next level, set state to GS_LEVEL

  // do things to change the game state
  while (action != ga_nothing)
    switch (action)
      {
      case ga_completed :
	LevelCompleted();
	break;
      case ga_worlddone :
	WorldDone();
	break;
      case ga_nothing   :
	break;
      default : I_Error("game.action = %d\n", action);
      }

  CONS_Printf("======== GI::Ticker, tic %d, st %d\n", gametic, state);

  // assign players to maps if needed
  vector<PlayerInfo *>::iterator it;
  if (state == GS_LEVEL)
    {
      for (it = players.begin(); it < players.end(); it++)
	{
	  switch ((*it)->playerstate)
	    {
	    case PST_WAITFORMAP:
	      /*
		if (!multiplayer && !cv_deathmatch.value)
		{
		// FIXME, not right. Map should call StartLevel
		// reload the level from scratch
		StartLevel(true, true);
		}
		else
	      */ 
	      {
		int m = 0;
		// just assign the player to a map
		maps[m]->AddPlayer(*it);
	      }
	      break;
	    case PST_REMOVE:
	      // the player is removed from the game
	      RemovePlayer(it);
	      // FIXME should we do it-- here?
	      break;
	    default:
	      break;
	    }
	}
    }


  //int buf = gametic % BACKUPTICS; TODO
  ticcmd_t *cmd;

  int i, n = players.size();
  // read/write demo

  for (i=0 ; i<n ; i++)
    {
      if (!dedicated)
        {
	  cmd = &players[i]->cmd;
	  
	  if (demoplayback)
	    ReadDemoTiccmd(cmd, i);
	  else
	    {
	      // FIXME here the netcode is bypassed until it's fixed. See also G_BuildTiccmd()!
	      //memcpy(cmd, &netcmds[buf][players[i]->number-1], sizeof(ticcmd_t));
	    }

	  if (demorecording)
	    WriteDemoTiccmd(cmd, i);
	}
    }

  // do main actions
  switch (state)
    {
    case GS_LEVEL:
      if (!paused)
	{
	  n = maps.size();
	  for (i=0; i<n; i++)
	    maps[i]->Ticker(); // tick the maps
	}
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
bool GameInfo::DeferredNewGame(skill_t sk, LevelNode *node, bool splitscreen)
{
  // if (n == NULL) do_something....
  // FIXME first we should check if we have all the WAD resources n requires
  // if (not enough resources) return false;
  firstlevel = node;

  Downgrade(VERSION);
  paused = false;
  nomonsters = false;
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

// starts or restarts the game. firstlevel must be set.
bool GameInfo::StartGame()
{
  if (firstlevel == NULL)
    return false;

  NewLevel(skill, firstlevel, true);
  return true;
}

// was G_InitNew()
// Sets up and starts a new level inside a game.
//
// This is the map command interpretation (result of Command_Map_f())
void GameInfo::NewLevel(skill_t sk, LevelNode *n, bool resetplayer)
{
  currentlevel = n;

  automap.Close();

  // delete old level
  int i;
  for (i = maps.size()-1; i>=0; i--)
    delete maps[i];
  maps.clear();

  //added:27-02-98: disable selected features for compatibility with
  //                older demos, plus reset new features as default
  if (!Downgrade(demoversion))
    {
      CONS_Printf("Cannot Downgrade engine.\n");
      CL_Reset();
      StartIntro();
      return;
    }

  if (paused)
    {
      paused = false;
      S.ResumeMusic();
    }

  if (sk > sk_nightmare)
    sk = sk_nightmare;

  M_ClearRandom();

  if (server && sk == sk_nightmare)
    {
      CV_SetValue(&cv_respawnmonsters,1);
      CV_SetValue(&cv_fastmonsters,1);
    }

  if (fc.FindNumForName(n->maplump.c_str()) == -1)
    {
      // FIXME! this entire block
      //has the name got a dot (.) in it?
      if (!FIL_CheckExtension(n->maplump.c_str()))
	// append .wad to the name
	;

      // try to load the file
      if (true)
	CONS_Printf("\2Map '%s' not found\n"
		    "(use .wad extension for external maps)\n", n->maplump.c_str());
      Command_ExitGame_f();
      return;
    }

  skill = sk;

  // this should be CL_Reset or something...
  //playerdeadview = false;

  Map *m = new Map(n->maplump);
  m->level = n;
  maps.push_back(m); // just one map for now

  StartLevel(false, resetplayer);
}


//
// StartLevel : (re)starts the 'currentlevel' stored in 'maps' vector
// was G_DoLoadLevel()
// if re is true, do not reload the entire maps, just restart them.
void GameInfo::StartLevel(bool re, bool resetplayer)
{
  // FIXME make restart option work, do not FreeTags and Setup,
  // instead just Reset the maps
  int i;

  //levelstarttic = gametic;        // for time calculation

  if (wipestate == GS_LEVEL)
    wipestate = GS_WIPE;             // force a wipe

  state = GS_LEVEL;

  int n = players.size();
  for (i=0 ; i<n ; i++)
    players[i]->Reset(resetplayer, cv_deathmatch.value ? false : true);

  Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1); // destroys pawns if they are not Detached

  // set switch texture names/numbers (bad design..)
  P_InitSwitchList();

  // setup all maps in the level
  LevelNode *l = currentlevel;
  l->kills = l->items = l->secrets = 0;
  n = maps.size();
  for (i = 0; i<n; i++)
    {
      if (!(maps[i]->Setup(gametic)))
	{
	  // fail so reset game stuff
	  Command_ExitGame_f();
	  return;
	}
      l->kills += maps[i]->kills;
      l->items += maps[i]->items;
      l->secrets += maps[i]->secrets;
    }

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
}


// was G_ExitLevel
//
// for now, exit can be 0 (normal exit) or 1 (secret exit)
void GameInfo::ExitLevel(int ex)
{
  if (state == GS_LEVEL) // FIXME! is this necessary?
    {
      action = ga_completed;
      currentlevel->exitused = (*(currentlevel->exit.find(ex))).second;
    }
}

// Here's for the german edition.
/*
void G_SecretExitLevel (void)
{
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ((game.mode == commercial)
      && (W_CheckNumForName("map31")<0))
        secretexit = false;
    else
        secretexit = true;
    game.action = ga_completed;
}
*/


// was G_DoCompleted
//
// start intermission
void GameInfo::LevelCompleted()
{
  int i, n;

  // action is ga_completed
  action = ga_nothing;

  hud.ST_Stop();

  // save or forfeit pawns
  // player has a pawn if he is alive or dead, but not if he is in respawnqueue
  n = players.size();
  for (i=0 ; i<n ; i++)
    if (players[i]->playerstate == PST_LIVE) 
      {     
	//if (players[i]->pawn)
	players[i]->pawn->FinishLevel(); // take away cards and stuff, save the pawn
      }
    else
      players[i]->pawn = NULL; // let the pawn be destroyed with the map

  // TODO here we have a problem with hubs: if a player is dead when the hub is exited,
  // his corpse is never placed in bodyqueue and thus never flushed out...

  // compact the frags vectors, reassign player numbers? Maybe not.

  // detach chasecam
  if (camera.chase)
    camera.ClearCamera();

  automap.Close();

  currentlevel->done = true;
  currentlevel->time = maps[0]->maptic / TICRATE;

  // dm nextmap wraparound?

  state = GS_INTERMISSION;

  wi.Start(currentlevel, firstlevel);
}


//
// was G_NextLevel (WorldDone)
//
// called when intermission ends
// init next level or go to the final scene
void GameInfo::EndIntermission()
{
  // action is ga_nothing
  const LevelNode *next = currentlevel->exitused;

  // check need for finale
  if (cv_deathmatch.value == 0)
    {
      int c = currentlevel->cluster;
      clusterdef_t *cd = P_FindCluster(c);

      // check winning
      if (next == NULL)
	{
	  // disconnect from network
	  CL_Reset();
	  state = GS_FINALE;
	  F_StartFinale(cd, false, true); // final piece of story is exittext
	  return;
	}

      int n = next->cluster;
      // check "mid-game finale" (story) (cluster change)
      if (n != c)
	{
	  clusterdef_t *ncd = P_FindCluster(n);
	  if (ncd && !(ncd->entertext.empty()))
	    {
	      state = GS_FINALE;
	      F_StartFinale(ncd, true, false);
	      return;
	    }
	  else if (cd && !(cd->exittext.empty()))
	    {
	      state = GS_FINALE;
	      F_StartFinale(cd, false, false);
	      return;
	    }
	}
    } else {
      // no finales in deathmatch
      // FIXME end game here, show final frags
      if (next == NULL)
	CL_Reset();
    }

  action = ga_worlddone;
}

void GameInfo::EndFinale()
{
  if (state == GS_FINALE) action = ga_worlddone;
}

// was G_DoWorldDone
// load next level

void GameInfo::WorldDone()
{
  action = ga_nothing;
  // maybe a state change here? Right now state is GS_INTERMISSION or GS_FINALE ??

  LevelNode *next = currentlevel->exitused;

  /*
  if (demoversion < 129)
    {
      // FIXME! doesn't work! restarts the same level!
      StartLevel(true, false);
    }
    else */
  if (server && !demoplayback)
    // not in demo because demo have the mapcommand on it
    {
      bool reset = false;
      // resetplayer in deathmatch for more equality
      if (cv_deathmatch.value)
	reset = true;

      NewLevel(skill, next, reset);
      //COM_BufAddText (va("map \"%s\" -noresetplayers\n", currentlevel->mapname.c_str()));
      //COM_BufAddText (va("map \"%s\"\n", currentlevel->mapname.c_str())); 
    }
}
