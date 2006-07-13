// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
/// \brief Server/client console commands

#include "command.h"
#include "cvars.h"
#include "console.h"
#include "parser.h"

#include "n_interface.h"

#include "g_game.h"
#include "g_level.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "w_wad.h"
#include "hud.h"
#include "i_system.h"



//========================================================================
//    Informational commands
//========================================================================

//  Returns program version.
void Command_Version_f()
{
  CONS_Printf(LEGACY_VERSION_BANNER" "__TIME__" "__DATE__"\n",
	      LEGACY_VERSION/100, LEGACY_VERSION%100, LEGACY_SUBVERSION,
	      LEGACY_VERSIONSTRING);
}


// prints info about current game and server
void Command_GameInfo_f()  // TODO
{
  CONS_Printf("Server: %d\n", game.server);
  CONS_Printf("Netgame: %d\n", game.netgame);
  CONS_Printf("Multiplayer: %d\n", game.multiplayer);
  CONS_Printf("Network state: %d\n", game.net->netstate);
}


// prints info about a map
void Command_MapInfo_f()
{
  MapInfo *m;
  if (COM_Argc() > 1)
    {
      m = game.FindMapInfo(strupr(COM_Argv(1)));
      if (!m)
	{
	  CONS_Printf("No such map.\n");
	  return;
	}
    }
  else
    {
      if (!com_player || !com_player->mp)
	return;
      m = com_player->mp->info;
    }

  CONS_Printf("Map %d: %s (%s)\n", m->mapnumber, m->nicename.c_str(), m->lumpname.c_str());
  if (!m->version.empty())
    CONS_Printf("Version: %s\n", m->version.c_str());
  CONS_Printf("Par: %d s\n", m->partime);
  if (!m->author.empty())
    CONS_Printf("Author: %s\n", m->author.c_str());
  if (!m->description.empty())
    CONS_Printf("%s\n", m->description.c_str());
}


// prints player roster
void Command_Players_f()
{
  CONS_Printf("Num             Name Score Ping\n");
  for (GameInfo::player_iter_t t = game.Players.begin(); t != game.Players.end(); t++)
    {
      PlayerInfo *p = t->second;
      // TODO highlight serverplayers
      CONS_Printf("%3d %16s  %4d %4d\n", p->number, p->name.c_str(), 1, 20);
    }
}


// prints scoreboard
void Command_Frags_f() // TODO
{
  if (COM_Argc() > 2)
    {
      CONS_Printf("Usage: frags [team]\n");
      return;
    }
  bool team = (COM_Argc() == 2);

  //int n = GI::GetFrags(&table, team);
  /*
    for (int i=0; i<n; i++)
      {
	  CONS_Printf("%-16s", game.FindPlayer(i)->name.c_str());
	  for(j=0;j<NETMAXPLAYERS;j++)
	    if(game.FindPlayer(j))
	      CONS_Printf(" %3d",game.FindPlayer(i)->Frags[j]);
	  CONS_Printf("\n");
    }
	  CONS_Printf("%-8s",game.teams[i]->name.c_str());
	  for(j=0;j<11;j++)
	    if(teamingame(j))
	      CONS_Printf(" %3d",fragtbl[i][j]);
	  CONS_Printf("\n");
        }
    */
}



//========================================================================
//    Utilities
//========================================================================

bool ConvertMapToHexen(int lumpnum);

/// Converts the relevant parts of a Doom/Heretic map to Hexen format,
/// writes the lumps on disk.
void Command_ConvertMap_f()
{
  if (COM_Argc() < 2)
    {
      CONS_Printf("Usage: convertmap <maplumpname>\n");
      return;
    }

  const char *temp = strupr(COM_Argv(1));
  int lump = fc.FindNumForName(temp);
  if (lump == -1)
    {
      CONS_Printf("Map %s cannot be found.\n", temp);
      return;
    }

  ConvertMapToHexen(lump);
}




//========================================================================
//    Chat commands
//========================================================================

// helper function
static void PasteMsg(char *buf, int i)
{
  int j = COM_Argc();

  strcpy(buf, COM_Argv(i++));

  for ( ; i<j; i++)
    {
      strcat(buf, " ");
      strcat(buf, COM_Argv(i));
    }
}


void Command_Say_f()
{
  char buf[255];

  if (COM_Argc() < 2)
    {
      CONS_Printf ("say <message> : send a message\n");
      return;
    }

  if (!com_player)
    return;

  PasteMsg(buf, 1);
  game.net->SendChat(com_player->number, 0, buf);
}


void Command_Sayto_f()
{
  char buf[255];

  if (COM_Argc() < 3)
    {
      CONS_Printf ("sayto <playername|playernum> <message> : send a message to a player\n");
      return;
    }

  if (!com_player)
    return;

  PlayerInfo *p = game.FindPlayer(COM_Argv(1));
  if (!p)
    return;

  PasteMsg(buf, 2);
  game.net->SendChat(com_player->number, p->number, buf);
}


void Command_Sayteam_f()
{
  char buf[255];

  if (COM_Argc() < 2)
    {
      CONS_Printf ("sayteam <message> : send a message to your team\n");
      return;
    }

  if (!com_player)
    return;

  PasteMsg(buf, 1);
  game.net->SendChat(com_player->number, -com_player->team, buf);
}






//========================================================================
//    Basic game commands
//========================================================================

// temporarily pauses the game, or in netgame, requests pause from server
void Command_Pause_f()
{
  if (!game.server && !cv_allowpause.value)
    {
      CONS_Printf("Server allows no pauses.\n");
      return;
    }

  bool on;

  if (COM_Argc() > 1)
    on = atoi(COM_Argv(1));
  else
    on = !game.paused;

  game.net->Pause(0, on);
}



// quit the game immediately
void Command_Quit_f()
{
  I_Quit();
}


// connect to a remote server
void Command_Connect_f()
{
  if (COM_Argc() < 2)
    {
      CONS_Printf("connect <serveraddress> : connect to a server\n"
		  "connect ANY : connect to the first LAN server found\n");
      return;
    }

  if (game.Playing())
    {
      CONS_Printf("End the current game first.\n");
      return;
    }

  CONS_Printf("connecting...\n");

  if (!strcasecmp(COM_Argv(1), "any"))
    game.net->CL_StartPinging(true);
  else
    game.net->CL_Connect(Address(COM_Argv(1)));
}



// shuts down the current game
void Command_Reset_f()
{
  game.SV_Reset();

  if (!game.dedicated)
    game.StartIntro();
}



//========================================================================
//    Game management (server only)
//========================================================================



// load a game
void Command_Load_f() // TODO
{
  if (COM_Argc() != 2)
    {
      CONS_Printf("load <slot>: load a saved game\n");
      return;
    }

  if (!game.server)
    {
      CONS_Printf("Only the server can load a game.\n");
      return;
    }

  game.LoadGame(atoi(COM_Argv(1)));
}


// save the game
void Command_Save_f()
{
  if (COM_Argc() != 3)
    {
      CONS_Printf("save <slot> <desciption>: save game\n");
      return;
    }

  if (!game.server)
    {
      CONS_Printf("Only server can save a game\n");
      return;
    }

  int slot = atoi(COM_Argv(1));
  char *desc = COM_Argv(2);

  game.SaveGame(slot, desc);
}






//  play a demo, add .lmp for external demos
//  eg: playdemo demo1 plays the internal game demo
void Command_Playdemo_f ()
{
  char    name[256];

  if (COM_Argc() != 2)
    {
      CONS_Printf ("playdemo <demoname> : playback a demo\n");
      return;
    }

  // disconnect from server here ?
  if(game.netgame)
    {
      CONS_Printf("\nYou can't play a demo while in net game\n");
      return;
    }

  // open the demo file
  strcpy (name, COM_Argv(1));
  // dont add .lmp so internal game demos can be played
  //FIL_DefaultExtension (name, ".lmp");

  CONS_Printf ("Playing back demo '%s'.\n", name);

  game.PlayDemo(name);
}

//  stop current demo
//
void Command_Stopdemo_f ()
{
  game.CheckDemoStatus ();
  CONS_Printf ("Stopped demo.\n");
}






//  Add a pwad at run-time
//  Search for sounds, maps, musics, etc..
void Command_Addfile_f() // TODO
{
  // FIXME rewrite the "late adding of wadfiles to the resource list"-system
  if (COM_Argc() != 2)
    {
      CONS_Printf("addfile <wadfile.wad> : load wad file\n");
      return;
    }
  /*
      // here check if file exist !!!
      if( !findfile(MAPNAME,NULL,false) )
        {
	  CONS_Printf("\2File %s' not found\n",MAPNAME);
	  return;
        }
  */

  //P_AddWadFile(COM_Argv(1), NULL);
}



/// Initialize a new game using a MAPINFO lump
void Command_NewGame_f()
{
  if (COM_Argc() < 3 || COM_Argc() > 5)
    {
      CONS_Printf("Usage: newgame <MAPINFO lump> (local | server) [episode] [skill]\n");
      return;
    }

  if (game.Playing())
    {
      CONS_Printf("First end the current game.\n");
      return;
    }

  int sk = sk_medium;
  int epi = 1;
  if (COM_Argc() >= 4)
    {
      epi = atoi(COM_Argv(3));

      if (COM_Argc() >= 5)
	{
	  sk = atoi(COM_Argv(4));
	  sk = (sk > sk_nightmare) ? sk_nightmare : ((sk < 0) ? 0 : sk);
	}
    }

  int lump = fc.FindNumForName(COM_Argv(1));
  if (lump < 0)
    {
      CONS_Printf("MAPINFO lump '%s' not found.\n", COM_Argv(1));
      return;
    }

  if (!game.SV_SpawnServer(lump))
    return;

  if (!strcasecmp(COM_Argv(2), "server"))
    game.SV_SetServerState(true);

  if (!game.dedicated)
    {
      // add local players
      int n = 1 + cv_splitscreen.value;
      for (int i=0; i<n; i++)
	LocalPlayers[i].info = game.AddPlayer(new PlayerInfo(&LocalPlayers[i]));

      // TODO add bots

      for (int i=0; i < n; i++)
	ViewPlayers.push_back(LocalPlayers[i].info);

      hud.ST_Start(LocalPlayers[0].info);
    }

  game.StartGame(skill_t(sk), epi);
}


// starts or restarts the game
void Command_StartGame_f()
{
  if (!game.server)
    {
      CONS_Printf("Only the server can restart the game.\n");
      return;
    }

  if (!game.StartGame(game.skill, 1))
    CONS_Printf("You must first set the levelgraph!\n");
}



/// Warp to a new map.
/// Called either from map <mapname> console command, or idclev cheat.
void Command_Map_f()
{
  if (COM_Argc() < 2 || COM_Argc() > 3)
    {
      CONS_Printf("Usage: map <number|name> [<entrypoint>]: warp players to a map.\n");
      return;
    }

  if (!game.server)
    {
      CONS_Printf("Only the server can change the map.\n");
      return;
    }

  if (!game.currentcluster)
    {
      CONS_Printf("No game running.\n");
      return;
    }

  MapInfo *m = game.FindMapInfo(strupr(COM_Argv(1)));
  if (!m)
    {
      CONS_Printf("No such map.\n");
      return;
    }

  if (!m->found)
    {
      CONS_Printf("Map %s cannot be found.\n", m->lumpname.c_str());
      return;
    }    

  int ep = atoi(COM_Argv(2));

  CONS_Printf("Warping to %s (%s)...\n", m->nicename.c_str(), m->lumpname.c_str());
  game.currentcluster->Finish(m->mapnumber, ep);
}



void Command_RestartLevel_f()
{
  if (!game.server)
    {
      CONS_Printf("Only the server can restart the level.\n");
      return;
    }


  if (game.state == GameInfo::GS_LEVEL)
    ; // game.StartLevel(true, true); FIXME
  else
    CONS_Printf("You should be in a level to restart it!\n");
}



// throws out a remote client
void Command_Kick_f()
{
  if (COM_Argc() != 2)
    {
      CONS_Printf("kick <playername> | <playernum> : kick a player from the game.\n");
      return;
    }

  if (game.server)
    {
      PlayerInfo *p = game.FindPlayer(COM_Argv(1));
      if (!p)
	CONS_Printf("there is no player with that name/number\n");
      else
	p->playerstate = PST_REMOVE;
    }
  else
    CONS_Printf("You are not the server.\n");
}



// helper function for Command_Kill_f
void Kill_pawn(Actor *v, Actor *k)
{
  if (v && v->health > 0)
    {
      v->flags |= MF_SHOOTABLE;
      v->flags2 &= ~(MF2_NONSHOOTABLE | MF2_INVULNERABLE);
      v->Damage(k, k, 10000, dt_always);
    }
}


// Kills just about anything
void Command_Kill_f()
{
  if (COM_Argc() < 2)
    {
      CONS_Printf ("Usage: kill me | <playernum> | monsters\n");
      // TODO extend usage: kill team
      return;
    }

  if (!game.server)
    {
      // client players can only commit suicide
      if (COM_Argc() > 2 || strcmp(COM_Argv(1), "me"))
	CONS_Printf("Only the server can kill others thru the console!\n");
      else if (com_player)
	game.net->RequestSuicide(com_player->number);

      return;
    }

  PlayerInfo *p;
  for (int i=1; i<COM_Argc(); i++)
    {
      char *s = COM_Argv(i);
      Actor *m = NULL;

      if (!strcasecmp(s, "me") && com_player)
	{
	  m = com_player->pawn; // suicide
	  Kill_pawn(m, m);
	}
      else if (!strcasecmp(s, "monsters") && com_player && com_player->mp)
	{
	  // monsters
	  CONS_Printf("%d monsters killed.\n", com_player->mp->Massacre());
	}
      else if ((p = game.FindPlayer(s)))
	{
	  m = p->pawn; // another player by number or name
	  Kill_pawn(m, NULL); // server does the killing
	}
      else
	{
	  CONS_Printf("Player %s is not in the game.\n", s);
	}
    }
}
