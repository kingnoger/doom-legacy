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
// Revision 1.5  2004/07/14 16:13:13  smite-meister
// cleanup, commands
//
// Revision 1.4  2004/07/13 20:23:39  smite-meister
// Mod system basics
//
// Revision 1.3  2004/07/09 19:43:40  smite-meister
// Netcode fixes
//
// Revision 1.2  2004/07/07 17:27:20  smite-meister
// bugfixes
//
// Revision 1.1  2004/07/05 16:53:30  smite-meister
// Netcode replaced
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Server/client console commands

#include "command.h"
#include "cvars.h"
#include "console.h"

#include "n_interface.h"
#include "n_connection.h"

#include "g_game.h"
#include "g_level.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "i_system.h"



//========================================================================
//    Informational commands
//========================================================================

//  Returns program version.
void Command_Version_f()
{
  CONS_Printf("Doom Legacy version %d.%d %s ("__TIME__" "__DATE__")\n",
	      VERSION/100, VERSION%100, VERSIONSTRING);
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
      if (!consoleplayer || !consoleplayer->mp)
	return;
      m = consoleplayer->mp->info;
    }

  CONS_Printf("Map %d: %s (%s)\n", m->mapnumber, m->nicename.c_str(), m->lumpname.c_str());
  if (!m->version.empty())
    CONS_Printf("Version: %s\n", m->version.c_str());
  CONS_Printf("Par: %d s\n", m->partime);
  if (!m->author.empty())
    CONS_Printf("Author: %s\n", m->author.c_str());
  if (!m->hint.empty())
    CONS_Printf("%s\n", m->hint.c_str());
}


// prints player roster
void Command_Players_f()
{
  CONS_Printf("Num             Name Score Ping\n");
  for (GameInfo::player_iter_t t = game.Players.begin(); t != game.Players.end(); t++)
    {
      PlayerInfo *p = (*t).second;
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

  PasteMsg(buf, 1);
  game.net->SayCmd(0, 0, buf);
}


void Command_Sayto_f()
{
  char buf[255];

  if (COM_Argc() < 3)
    {
      CONS_Printf ("sayto <playername|playernum> <message> : send a message to a player\n");
      return;
    }

  PlayerInfo *p = game.FindPlayer(COM_Argv(1));
  if (!p)
    return;

  PasteMsg(buf, 2);
  game.net->SayCmd(0, p->number, buf);
}


void Command_Sayteam_f()
{
  char buf[255];

  if (COM_Argc() < 2)
    {
      CONS_Printf ("sayteam <message> : send a message to your team\n");
      return;
    }

  PasteMsg(buf, 1);
  game.net->SayCmd(0, -consoleplayer->team, buf);
}


void LNetInterface::SayCmd(int from, int to, const char *msg)
{
  if (!game.netgame)
    return;

  if (from == 0 && consoleplayer)
    from = consoleplayer->number;

  if (server_con)
    server_con->rpcSay(from, to, msg);
  else if (game.server)
    for (GameInfo::player_iter_t t = game.Players.begin(); t != game.Players.end(); t++)
      {
	PlayerInfo *p = (*t).second;

	if (to == 0 || p->number == to || p->team == -to)
	  {
	    if (p->connection)
	      p->connection->rpcSay(from, to, msg);

	    if (p->number == to)
	      break; // nobody else should get it
	  }
      }
}


// to: 0 means everyone, positive numbers are players, negative numbers are teams
TNL_IMPLEMENT_RPC(LConnection, rpcSay, (S8 from, S8 to, const char *msg), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
  if (isConnectionToServer())
    {
      // client
      CONS_Printf("\3%s: %s\n", game.Players[from]->name.c_str(), msg);
    }
  else
    {
      from = player[0]->number;

      CONS_Printf("message!\n");
      CONS_Printf("\3%s: %s\n", game.Players[from]->name.c_str(), msg);
      game.net->SayCmd(player[0]->number, to, msg);
    }
};








//========================================================================
//    Basic game commands
//========================================================================

// temporarily pauses the game, or in netgame, requests pause from server
void Command_Pause_f()
{
  bool pause;

  if (COM_Argc() > 1)
    pause = (atoi(COM_Argv(1)) != 0);
  else
    pause = !game.paused;

  if (game.server)
    {
      game.paused = pause;

      if (pause)
	CONS_Printf("Game paused.\n");
      else
	CONS_Printf("Game unpaused.\n");
    }

  //SendNetXCmd(XD_PAUSE,&buf,1); // TODO
}


/*
void Got_Pause(char **cp,int playernum)
{
  game.paused = READBYTE(*cp);
  if(!demoplayback)
    {
      const char *name = game.FindPlayer(playernum)->name.c_str();
      if(game.netgame)
        {
	  if( game.paused )
	    CONS_Printf("Game paused by %s\n", name);
	  else
	    CONS_Printf("Game unpaused by %s\n", name);
        }

      if (game.paused)
	{
	  if(!Menu::active || game.netgame)
	    S.PauseMusic();
        }
      else
	S.ResumeMusic();
    }
}
*/


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
		  "connect ANY : connect to the first lan server found\n");
      return;
    }

  if (game.netgame)
    {
      CONS_Printf("You cannot connect while in a netgame!\n"
		  "Leave this game first.\n");
      return;
    }

  CONS_Printf("connecting...\n");

  if (!stricmp(COM_Argv(1),"any"))
    game.net->CL_StartPinging(true);
  else
    game.net->CL_Connect(Address(COM_Argv(1)));
}


// drop out of netgame
void Command_Disconnect_f() // TODO
{
  game.server = true;
  game.netgame = false;
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
      CONS_Printf("Only server can load a game\n");
      return;
    }

  // spawn a server if needed
  //SV_SpawnServer();

  int slot = atoi(COM_Argv(1));

  game.LoadGame(slot);
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

  MapInfo *m = game.FindMapInfo(COM_Argv(1));
  if (!m)
    {
      CONS_Printf("No such map.\n");
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



void Command_ExitLevel_f()
{
  if (!game.server)
    {
      CONS_Printf("Only the server can exit the level\n");
      return;
    }

  if (game.state != GameInfo::GS_LEVEL)
    CONS_Printf("You should be playing a level to exit it !\n");

  // TODO exitlevel
}




void Command_RestartGame_f()
{
  if (!game.server)
    {
      CONS_Printf("Only the server can restart the game.\n");
      return;
    }

  if (!game.StartGame())
    CONS_Printf("You must first set the levelgraph!\n");
}



// shuts down the current game
void Command_Reset_f()
{
  game.SV_Reset();

  if (!dedicated)
    game.StartIntro();
}






// throws out a remote client
void Command_Kick_f()
{
  if (COM_Argc() != 2)
    {
      CONS_Printf("kick <playername> or <playernum> : kick a player\n");
      return;
    }

  if (game.server)
    {
      PlayerInfo *p = game.FindPlayer(COM_Argv(1));
      if (!p)
	CONS_Printf("there is no player with that name/number\n");
      // TODO kick him! inform others so they can remove him.
    }
  else
    CONS_Printf("You are not the server\n");
}



/*
void Got_KickCmd(char **p,int playernum)
{
  int pnum=READBYTE(*p);
  int msg =READBYTE(*p);
  
  CONS_Printf("\2%s ", game.FindPlayer(pnum)->name.c_str());
  
  switch(msg)
    {
    case KICK_MSG_GO_AWAY:
      CONS_Printf("has been kicked (Go away)\n");
      break;
    case KICK_MSG_CON_FAIL:
      CONS_Printf("has been kicked (Consistency failure)\n");
      break;
    case KICK_MSG_TIMEOUT:
      CONS_Printf("left the game (Connection timeout)\n");
      break;
    case KICK_MSG_PLAYER_QUIT:
      CONS_Printf("left the game\n");
      break;
    }
  if(pnum == consoleplayer->number - 1)
    {
      CL_Reset();
      game.StartIntro();
      M_StartMessage("You have been kicked by the server\n\nPress ESC\n",NULL,MM_NOTHING);
    }
  else
    CL_RemovePlayer(pnum);
}
*/





// helper function for Command_Kill_f
static void Kill_pawn(Actor *v, Actor *k)
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
      CONS_Printf ("Usage: kill [me] | [<playernum>] | [monsters]\n");
      // TODO extend usage: kill team
      return;
    }

  if (!game.server)
    {
      // client players can only commit suicide
      if (COM_Argc() > 2 || strcmp(COM_Argv(1), "me"))
	CONS_Printf("Only the server can kill others thru console!\n");
      else if (consoleplayer)
	Kill_pawn(consoleplayer->pawn, consoleplayer->pawn);
      return;
    }

  PlayerInfo *p;
  for (int i=1; i<COM_Argc(); i++)
    {
      char *s = COM_Argv(i);
      Actor *m = NULL;

      if (!strcmp(s, "me") && consoleplayer)
	m = consoleplayer->pawn; // suicide
      else if (!strcmp(s, "monsters") && consoleplayer)
	{
	  // monsters
	  CONS_Printf("%d monsters killed.\n", consoleplayer->mp->Massacre());
	  continue;
	}
      else if ((p = game.FindPlayer(s)))
	m = p->pawn; // another player by number or name
      else
	{
	  CONS_Printf("Player %s is not in the game.\n", s);
	  continue;
	}

      Kill_pawn(m, NULL); // server does the killing
    }
}
