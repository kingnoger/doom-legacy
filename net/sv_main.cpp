// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004 by DooM Legacy Team.
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
// Revision 1.10  2004/09/13 20:43:31  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.9  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.8  2004/08/12 18:30:31  smite-meister
// cleaned startup
//
// Revision 1.7  2004/08/06 18:54:39  smite-meister
// netcode update
//
// Revision 1.6  2004/07/14 16:13:13  smite-meister
// cleanup, commands
//
// Revision 1.5  2004/07/13 20:23:39  smite-meister
// Mod system basics
//
// Revision 1.4  2004/07/11 14:32:01  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.3  2004/07/09 19:43:40  smite-meister
// Netcode fixes
//
// Revision 1.2  2004/07/05 16:53:30  smite-meister
// Netcode replaced
//
// Revision 1.1  2004/06/25 19:50:59  smite-meister
// Netcode
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Main server code
///
/// Uses OpenTNL


/*
  Masterserver
    -matchmaker between public servers and clients looking for them

  Server (may include local clients)
    -local, no network traffic whatsoever
    -private/LAN, does not announce its existence, just answers to connecting clients
    -public, announces itself to masterserver, keeps sending heartbeat

  Client
    -can look for a server in a specific IP (private), on LAN,
     or ask public server lists from masterserver


legacy -server [-dedicated] [-port XXX]
 startup
 create netint, passwords, set them up
 open console
 read/create levelgraph
 setup first map(s)
 open server, notify master
 before starting game: send heartbeat, don't tick maps
 1: when client joins, send info to all clients, create scope object
 when start condition fulfilled, (lock game), start ticking maps
 when server is closed: make one of the clients new server? / kick clients
 when client disconnects: send info to others, remove pawn/pov, drop relevant items on ground?


legacy -connect IP/LAN [-port XXX]
 startup
 create netint, passwords, set them up
 4: open console/menu, start pinging (several LAN servers? browser like with masterserver)
 2: get server info, connect
 find resources, download if necessary
 send server ping, # of local players, names, teams, colors...
 1: server validates these (all may change)
 setup map, partial spawn
 start ghosting, scopeobj can be either pov (rider, fixed) or the actual pawn (free spectator)
 game starts, spawn the pawn
 when connection is lost: try to renegotiate using the same IP/signatures...
 3: if unable to renegotiate, open menu (destroy old game data)


legacy [no autostart]
 setup, netint, console, menu
 3: multiplayer menu
  client: search LAN 4:, connect masterserver, connect ip
  server: start server, load game, stop server

 singleplayer menu: server with local players, not a netgame



Net tasks: (ghost, rpc, netevent, info packet, raw packet)
 - player info (ghost?, rpc) name color skin autoaim?
 - add/drop player?
 - server info (ghost?, rpc)
 - game object data (ghost)
 - player input (?)
 - hud messages
 - score
 - file transfer (?)


RPC's from server to client:
 - startsound/stopsound/sequence/music
 - HUD colormap and other effects?
 - pause
 - console/HUD message (unicast/multicast)
 - map change/load

client to server:
 - request pause
 - say

Player ticcmd contains both guaranteed_ordered and unguaranteed elements.
Shooting and artifact use should be guaranteed...



In future, do dll mods like this:
polymorph Map, PlayerInfo? (and Actor descendants, of course)
Have polymorphed class GameType which creates these into GameInfo containers

 */


#include <vector>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "d_main.h"

#include "g_game.h"
#include "g_player.h"
#include "n_interface.h"

#include "hu_stuff.h"
#include "m_menu.h"

#include "i_system.h"
#include "m_argv.h"
#include "r_data.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"


using namespace TNL;


#define DEFAULT_PORT 5029;

extern bool dedicated;


//========================================================================
//        Server consvars
//========================================================================

consvar_t cv_playdemospeed  = {"playdemospeed","0",0,CV_Unsigned};

#define MS_PORT "28910"
consvar_t cv_masterserver   = {"masterserver", "doomlegacy.dhs.org:"MS_PORT, CV_SAVE, NULL };
consvar_t cv_netstat = {"netstat","0",0,CV_OnOff};

// server info
consvar_t cv_internetserver  = {"publicserver", "No", CV_SAVE, CV_YesNo };
consvar_t cv_servername      = {"servername", "DooM Legacy server", CV_SAVE, NULL };
consvar_t cv_allownewplayers = {"allownewplayers","1",0,CV_OnOff};
CV_PossibleValue_t maxplayers_cons_t[]={{1,"MIN"},{100,"MAX"},{0,NULL}};
consvar_t cv_maxplayers     = {"maxplayers","32",CV_NETVAR,maxplayers_cons_t,NULL,32};

// game rules
CV_PossibleValue_t teamplay_cons_t[]={{0,"Off"},{1,"Color"},{2,"Skin"},{3,NULL}};
CV_PossibleValue_t deathmatch_cons_t[]={{0,"Coop"},{1,"1"},{2,"2"},{3,"3"},{0,NULL}};
CV_PossibleValue_t fraglimit_cons_t[]={{0,"MIN"},{1000,"MAX"},{0,NULL}};
CV_PossibleValue_t exitmode_cons_t[]={{0,"noexit"},{1,"first"},{2,"ind"},{3,"last"},{4,NULL}};

void TeamPlay_OnChange();
void Deathmatch_OnChange();
void FragLimit_OnChange();
void TimeLimit_OnChange();
void FastMonster_OnChange();

consvar_t cv_deathmatch = {"deathmatch","0",CV_NETVAR | CV_NOINIT | CV_CALL, deathmatch_cons_t, Deathmatch_OnChange};
consvar_t cv_teamplay   = {"teamplay"  ,"0",CV_NETVAR | CV_CALL,teamplay_cons_t, TeamPlay_OnChange};
consvar_t cv_teamdamage = {"teamdamage","0",CV_NETVAR,CV_OnOff};
consvar_t cv_hiddenplayers = {"hiddenplayers","0",CV_NETVAR,CV_YesNo};
consvar_t cv_exitmode   = {"exitmode", "1", CV_NETVAR, exitmode_cons_t, NULL};
consvar_t cv_fraglimit  = {"fraglimit" ,"0",CV_NETVAR | CV_CALL | CV_NOINIT,fraglimit_cons_t, FragLimit_OnChange};
consvar_t cv_timelimit  = {"timelimit" ,"0",CV_NETVAR | CV_CALL | CV_NOINIT,CV_Unsigned, TimeLimit_OnChange};

consvar_t cv_allowjump       = {"allowjump","1",CV_NETVAR,CV_YesNo};
consvar_t cv_allowrocketjump = {"allowrocketjump","0",CV_NETVAR,CV_YesNo};
consvar_t cv_allowautoaim    = {"allowautoaim","1",CV_NETVAR,CV_YesNo};
consvar_t cv_allowmlook      = {"allowfreelook","1",CV_NETVAR,CV_YesNo};

consvar_t cv_itemrespawn     ={"respawnitem"    , "0",CV_NETVAR,CV_OnOff};
consvar_t cv_itemrespawntime ={"respawnitemtime","30",CV_NETVAR,CV_Unsigned};
consvar_t cv_respawnmonsters = {"respawnmonsters","0",CV_NETVAR,CV_OnOff};
consvar_t cv_respawnmonsterstime = {"respawnmonsterstime","12",CV_NETVAR,CV_Unsigned};
consvar_t cv_fragsweaponfalling  = {"fragsweaponfalling","0", CV_SAVE | CV_NETVAR, CV_OnOff};

consvar_t cv_gravity = {"gravity","1", CV_NETVAR | CV_FLOAT | CV_ANNOUNCE};
consvar_t cv_nomonsters = {"nomonsters","0", CV_NETVAR, CV_OnOff};
consvar_t cv_fastmonsters = {"fastmonsters","0", CV_NETVAR | CV_CALL,CV_OnOff,FastMonster_OnChange};
consvar_t cv_solidcorpse  = {"solidcorpse","0", CV_NETVAR | CV_SAVE,CV_OnOff};
consvar_t cv_voodoodolls  = {"voodoodolls","1",CV_NETVAR,CV_OnOff};

void TeamPlay_OnChange()
{
  // Change the name of the teams
  /*
  game.teams.resize(game.maxteams);

  if (cv_teamplay.value == 1)
    {
      // color
      for (i=0; i<game.maxteams; i++)
	{
	  game.teams[i] = new TeamInfo;
	  game.teams[i]->name = Color_Names[i];
	  game.teams[i]->color = i;
        }
    }
  else if (cv_teamplay.value == 2)
    {
      // skins
      for(i=0; i<game.maxteams; i++)
        {
	  game.teams[i] = new TeamInfo;
	  game.teams[i]->name = skins[i].name;
	  game.teams[i]->color = i; // kinda irrelevant
        }
    }
  */
}


void Deathmatch_OnChange()
{
  if (game.server)
    {
      if (cv_deathmatch.value >= 2)
	cv_itemrespawn.Set(1);
      else
	cv_itemrespawn.Set(0);
    }
  // FIXME, deathmatch_onchange
  //if (cv_deathmatch.value == 1 || cv_deathmatch.value == 3) P_RespawnWeapons();

  // give all keys to the players
  if (cv_deathmatch.value)
    {
      /*
      int n = game.players.size();
      for(j=0;j<n;j++)
	if (game.FindPlayer(j))
	  game.FindPlayer(j)->pawn->cards = it_allkeys;
      */
    }
}


void FragLimit_OnChange()
{
}


void TimeLimit_OnChange()
{
  if (cv_timelimit.value)
    CONS_Printf("Levels will end after %d minute(s).\n", cv_timelimit.value);
  else
    CONS_Printf("Time limit disabled\n");
}







//========================================================================
//    Main server code
//========================================================================


void GameInfo::TryRunTics(tic_t elapsed)
{
  extern  bool singletics;

  // max time step
  if (elapsed > TICRATE/7)
    {
      if (server)
	elapsed = 1;
      else
	elapsed = TICRATE/7;
    }
  
  if (singletics)
    elapsed = 1;

  if (elapsed >= 1)
    COM_BufExecute(); // process command buffer

  I_GetEvent();


  if (!dedicated)
    {
      Menu::Ticker();
      con.Ticker();

      // translate inputs (keyboard/mouse/joystick) into game controls
      if (consoleplayer)
	consoleplayer->cmd.Build(true, elapsed);
      if (consoleplayer2)
	consoleplayer2->cmd.Build(false, elapsed);
    }

  D_ProcessEvents(); // read control events, feed them to responders

  // get packets? (maybe connection is lost...)

  // queue ticcmds to server_con equipped with delta_t's (elapsed)
  // try resending them a few times until we get confirmation of arrival from server

  // run client simulation/server simulation


  net->Update();

  while (elapsed-- > 0)
    Ticker();
}




/// is there a game running?
bool GameInfo::Playing()
{
  return (state > GS_INTRO);
  //return (net->netstate == LNetInterface::CL_Connected);
}





void P_ACSInitNewGame();

/// after this you are not Playing()
void GameInfo::SV_Reset()
{
  net->SV_Reset(); // disconnect clients etc.

  Clear_mapinfo_clustermap();
  ClearPlayers();
  // clear teams?
  P_ACSInitNewGame(); // TODO clear FS global/hub data

  Z_FreeTags(PU_LEVEL, MAXINT);

  // reset gamestate?
  state = GS_NULL;
  server = true;
  netgame = false;
  multiplayer = cv_splitscreen.value;

  CONS_Printf("-- Server reset --\n");
}


/// initializes and opens the server (but does not set up the game!)
bool GameInfo::SV_SpawnServer()
{
  if (Playing())
    {
      I_Error("tried to spawn server while playing\n");
      return false;
    }

  bool local = !netgame; // temp HACK
  SV_Reset();
  CONS_Printf("Starting server...\n");

  if (!dedicated)
    {
      // add local players
      consoleplayer = AddPlayer(new PlayerInfo(localplayer));
      if (cv_splitscreen.value)
	consoleplayer2 = AddPlayer(new PlayerInfo(localplayer2));
    }

  if (!local)
    {
      net->SV_Open();
      netgame = true;
      multiplayer = true;
    }

  return true;
}


int P_Read_ANIMATED(int lump);
int P_Read_ANIMDEFS(int lump);
void P_InitSwitchList();

/// reads some lumps affecting the game behavior
void GameInfo::ReadResourceLumps()
{
  // read these lumps _after_ MAPINFO but not separately for each map
  extern bool nosound;
  if (!nosound)
    {
      S_ClearSounds();
      int n = fc.Size();
      for (int i = 0; i < n; i++)
	{
	  // cumulative reading
	  S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", i));
	  S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", i));
	}

      if (cv_precachesound.value)
	S_PrecacheSounds();
    }

  // texture and flat animations
  if (P_Read_ANIMDEFS(fc.FindNumForName("ANIMDEFS")) < 0)
    P_Read_ANIMATED(fc.FindNumForName("ANIMATED"));

  // set switch texture names/numbers, read "SWITCHES" lump
  P_InitSwitchList();
}







//=========================================================================
//                           SERVER INIT
//=========================================================================


/// Sets up network interface
void InitNetwork()
{
  CONS_Printf("Initializing network...");

  bool ipx = M_CheckParm("-ipx");
  TransportProtocol ptc = ipx ? IPXProtocol : IPProtocol;

  S32 port = DEFAULT_PORT;
  if (M_CheckParm ("-port"))
    port = atoi(M_GetNextParm());

  // set up the interface
  LNetInterface *n = new LNetInterface(Address(ptc, Address::Any, port));
  game.net = n;

  n->ping_address = Address(ptc, Address::Broadcast, port);

  // misc. commandline parameters
  n->nodownload = M_CheckParm("-nodownload");


  // parse network game options,
  if (M_CheckParm("-server") || dedicated)
    {
      game.netgame = true;

      // give a number of clients to wait for before starting game?
      // if (M_IsNextParm())
      //   doomcom->numnodes = atoi(M_GetNextParm());
    }
  else if (M_CheckParm("-connect"))
    {
      char server_hostname[255];

      if (M_IsNextParm())
	strcpy(server_hostname, M_GetNextParm());
      else
	server_hostname[0] = '\0'; // LAN server, use broadcast to detect it

      // server address only in ip
      if (server_hostname[0] && !ipx)
        {
	  COM_BufAddText("connect \"");
	  COM_BufAddText(server_hostname);
	  COM_BufAddText("\"\n");
        }
      else
        {
	  // so we're on a LAN
	  COM_BufAddText("connect any\n");
        }
    }

  CONS_Printf("done.\n");
}


// ------
// protos
// ------

/*
void Got_NameAndcolor(char **cp,int playernum);
void Got_WeaponPref  (char **cp,int playernum);
void Got_Mapcmd      (char **cp,int playernum);
void Got_ExitLevelcmd(char **cp,int playernum);
void Got_Pause       (char **cp,int playernum);
void Got_UseArtefact (char **cp,int playernum);
void Got_KickCmd     (char **p, int playernum);
void Got_AddPlayer   (char **p, int playernum);
void Got_Saycmd      (char **p, int playernum);

void Got_LoadGamecmd (char **cp,int playernum);
void Got_SaveGamecmd (char **cp,int playernum);
*/

void Command_Listserv_f() {}

void Command_Clear_f();
void Command_Keymap_f();
void Command_Bind_f();

void Command_SaveConfig_f();
void Command_LoadConfig_f();
void Command_ChangeConfig_f();

void Command_Version_f();
void Command_GameInfo_f();
void Command_MapInfo_f();
void Command_Players_f();
void Command_Frags_f();
void Command_TeamFrags_f();

void Command_Pause_f();
void Command_Quit_f ();
void Command_Connect_f();
void Command_Reset_f();

void Command_Kick_f();
void Command_Kill_f();
void Command_Addfile_f();

void Command_NewGame_f();
void Command_StartGame_f();
void Command_Map_f();

void Command_Save_f();
void Command_Load_f();
void Command_Playdemo_f();
void Command_Timedemo_f();
void Command_Stopdemo_f();

void Command_Say_f();
void Command_Sayto_f();
void Command_Sayteam_f();
void Command_Chatmacro_f();

void Command_CheatNoClip_f();
void Command_CheatGod_f();
void Command_CheatGimme_f();

void Command_Meminfo_f();

void Command_RunACS_f();

void COM_FS_DumpScript_f();
void COM_FS_RunScript_f();
void COM_FS_Running_f();
void FS_Init();



/// This is needed by both servers and clients
void SV_Init()
{
  CONS_Printf("SV_Init\n");

  // register commands

  // Ideas for new commands from Quake2
  //COM_AddCommand("status", Command_Status_f);
  //COM_AddCommand("heartbeat", SV_Heartbeat_f);
  //COM_AddCommand("killserver", SV_KillServer_f);

  /* ideas from Quake
    "notarget"
    "fly"
    "reconnect"
    "tell"
    "spawn"
    "begin"
    "prespawn"
    "ping"
    "demos"
  */

  // console
  COM_AddCommand("cls", Command_Clear_f);
  COM_AddCommand("keymap", Command_Keymap_f);
  COM_AddCommand("bind", Command_Bind_f);

  // config file management  
  COM_AddCommand("saveconfig", Command_SaveConfig_f);
  COM_AddCommand("loadconfig", Command_LoadConfig_f);
  COM_AddCommand("changeconfig", Command_ChangeConfig_f);

  // informational commands
  COM_AddCommand("version", Command_Version_f);
  COM_AddCommand("meminfo", Command_Meminfo_f);
  COM_AddCommand("gameinfo", Command_GameInfo_f);
  COM_AddCommand("mapinfo", Command_MapInfo_f);
  COM_AddCommand("players", Command_Players_f);
  COM_AddCommand("frags", Command_Frags_f);

  // chat commands
  COM_AddCommand("say"    , Command_Say_f);
  COM_AddCommand("sayto"  , Command_Sayto_f);
  COM_AddCommand("sayteam", Command_Sayteam_f);
  COM_AddCommand("chatmacro", Command_Chatmacro_f); // hu_stuff.c

  // basic commands for controlling the game
  COM_AddCommand("pause", Command_Pause_f);
  COM_AddCommand("quit",  Command_Quit_f);
  COM_AddCommand("connect", Command_Connect_f);
  COM_AddCommand("reset", Command_Reset_f);

  // game management (server only)
  COM_AddCommand("save", Command_Save_f);
  COM_AddCommand("load", Command_Load_f);
  COM_AddCommand("playdemo", Command_Playdemo_f);
  COM_AddCommand("stopdemo", Command_Stopdemo_f);
  COM_AddCommand("addfile", Command_Addfile_f);
  COM_AddCommand("kick", Command_Kick_f);
  COM_AddCommand("kill", Command_Kill_f);

  COM_AddCommand("newgame", Command_NewGame_f);
  COM_AddCommand("startgame", Command_StartGame_f);
  COM_AddCommand("map", Command_Map_f);

  COM_AddCommand("runacs", Command_RunACS_f);
#ifdef FRAGGLESCRIPT
  FS_Init();
  COM_AddCommand("fs_dumpscript", COM_FS_DumpScript_f);
  COM_AddCommand("fs_runscript",  COM_FS_RunScript_f);
  COM_AddCommand("fs_running",    COM_FS_Running_f);
#endif

  // bots
  //COM_AddCommand("addbot", Command_AddBot_f);

  // cheat commands, I'm bored of deh patches renaming the idclev ! :-)
  COM_AddCommand("noclip", Command_CheatNoClip_f);
  COM_AddCommand("god", Command_CheatGod_f);
  COM_AddCommand("gimme", Command_CheatGimme_f);

  //Added by Hurdler for master server connection
  cv_masterserver.Reg();
  COM_AddCommand("listserv", Command_Listserv_f);

  // register console variables
  cv_internetserver.Reg();
  cv_servername.Reg();
  cv_allownewplayers.Reg();
  cv_maxplayers.Reg();

  cv_deathmatch.Reg();
  cv_teamplay.Reg();
  cv_teamdamage.Reg();
  cv_hiddenplayers.Reg();
  cv_exitmode.Reg();
  cv_fraglimit.Reg();
  cv_timelimit.Reg();

  cv_allowjump.Reg();
  cv_allowrocketjump.Reg();
  cv_allowautoaim.Reg();
  cv_allowmlook.Reg();

  cv_itemrespawn.Reg();
  cv_itemrespawntime.Reg();
  cv_respawnmonsters.Reg();
  cv_respawnmonsterstime.Reg();
  cv_fragsweaponfalling.Reg();

  cv_gravity.Reg();
  cv_nomonsters.Reg();
  cv_fastmonsters.Reg();
  cv_solidcorpse.Reg();
  cv_voodoodolls.Reg();

  cv_playdemospeed.Reg();
  cv_netstat.Reg();

  // add chat macro consvars
  HU_HackChatmacros();


  R_ServerInit(); // must init texture cache now! Even server needs to know the texture heights etc.

  InitNetwork();
}
