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

XD:
 - client map change (restart?)
 - artifact use
 - kick
 - pause request / pause



Player ticcmd contains both guaranteed_ordered and unguaranteed elements.
Shooting and artifact use should be guaranteed...



  Relevant TNL classes:

  NetObject: Thinker should inherit this, these can be ghosted and have RPC's called on them.
  NetEvent: Used for RPC's and communication...
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
#include "n_connection.h"

#include "hu_stuff.h"
#include "m_menu.h"

#include "i_system.h"
#include "m_argv.h"
#include "r_data.h"

#include "z_zone.h"


using namespace TNL;


#define DEFAULT_PORT 5029;

extern bool dedicated;


//====================================
//        Server consvars
//====================================

consvar_t cv_playdemospeed  = {"playdemospeed","0",0,CV_Unsigned};

#define MS_PORT "28910"
consvar_t cv_masterserver   = {"masterserver", "doomlegacy.dhs.org:"MS_PORT, CV_SAVE, NULL };
consvar_t cv_netstat = {"netstat","0",0,CV_OnOff};

// server info
consvar_t cv_internetserver = {"internetserver", "No", CV_SAVE, CV_YesNo };
consvar_t cv_servername     = {"servername", "DooM Legacy server", CV_SAVE, NULL };
consvar_t cv_allownewplayer = {"sv_allownewplayers","1",0,CV_OnOff};
CV_PossibleValue_t maxplayers_cons_t[]={{1,"MIN"},{32,"MAX"},{0,NULL}};
consvar_t cv_maxplayers     = {"sv_maxplayers","32",CV_NETVAR,maxplayers_cons_t,NULL,32};

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

consvar_t cv_deathmatch = {"deathmatch","0",CV_NETVAR | CV_CALL, deathmatch_cons_t, Deathmatch_OnChange};
consvar_t cv_teamplay   = {"teamplay"  ,"0",CV_NETVAR | CV_CALL,teamplay_cons_t, TeamPlay_OnChange};
consvar_t cv_teamdamage = {"teamdamage","0",CV_NETVAR,CV_OnOff};
consvar_t cv_exitmode   = {"exitmode", "1", CV_NETVAR, exitmode_cons_t, NULL};
consvar_t cv_fraglimit  = {"fraglimit" ,"0",CV_NETVAR | CV_CALL | CV_NOINIT,fraglimit_cons_t, FragLimit_OnChange};
consvar_t cv_timelimit  = {"timelimit" ,"0",CV_NETVAR | CV_CALL | CV_NOINIT,CV_Unsigned, TimeLimit_OnChange};

consvar_t cv_allowjump       = {"allowjump","1",CV_NETVAR,CV_YesNo};
consvar_t cv_allowrocketjump = {"allowrocketjump","0",CV_NETVAR,CV_YesNo};
consvar_t cv_allowautoaim    = {"allowautoaim","1",CV_NETVAR,CV_YesNo};
consvar_t cv_allowmlook      = {"allowmlook","1",CV_NETVAR,CV_YesNo};

consvar_t cv_itemrespawn     ={"respawnitem"    , "0",CV_NETVAR,CV_OnOff};
consvar_t cv_itemrespawntime ={"respawnitemtime","30",CV_NETVAR,CV_Unsigned};
consvar_t cv_respawnmonsters = {"respawnmonsters","0",CV_NETVAR,CV_OnOff};
consvar_t cv_respawnmonsterstime = {"respawnmonsterstime","12",CV_NETVAR,CV_Unsigned};
consvar_t cv_fragsweaponfalling  = {"fragsweaponfalling","0", CV_SAVE | CV_NETVAR, CV_OnOff};

consvar_t cv_gravity = {"gravity","1", CV_NETVAR | CV_FLOAT | CV_SHOWMODIF};
consvar_t cv_fastmonsters = {"fastmonsters","0", CV_NETVAR | CV_CALL,CV_OnOff,FastMonster_OnChange};
consvar_t cv_solidcorpse  = {"solidcorpse","0", CV_NETVAR | CV_SAVE,CV_OnOff};


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
	CV_SetValue(&cv_itemrespawn, 1);
      else
	CV_SetValue(&cv_itemrespawn, 0);
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
  if (cv_fraglimit.value > 0)
    game.CheckScoreLimit();
}


void TimeLimit_OnChange()
{
  if (cv_timelimit.value)
    CONS_Printf("Levels will end after %d minute(s).\n", cv_timelimit.value);
  else
    CONS_Printf("Time limit disabled\n");
}









//=========================================================================

TNL_IMPLEMENT_NETCONNECTION(LConnection, NetClassGroupGame, true);

LConnection::LConnection()
{
  //setIsAdaptive();
  setFixedRateParameters(50, 50, 2000, 2000); // packet rates, sizes (send and receive)
}




void LConnection::writeConnectRequest(BitStream *stream)
{
   Parent::writeConnectRequest(stream);

   stream->write(VERSION);
   stream->writeString(VERSIONSTRING);
   /*
     TODO: send local player data
     stream->writeString(playerName.getString());
   */
}


bool LConnection::readConnectRequest(BitStream *stream, const char **errorString)
{
  if (!Parent::readConnectRequest(stream, errorString))
    return false;

  char temp[255];
  *errorString = temp; // in case we need it

  int version;
  stream->read(&version);
  stream->readString(temp);

  if (version != VERSION || strcmp(temp, VERSIONSTRING))
    {
      sprintf(temp, "Different Legacy versions cannot play a net game! (Server version %d.%d%s)",
	      VERSION/100, VERSION%100, VERSIONSTRING);
      return false;
    }

  if (game.Players.size() >= unsigned(cv_maxplayers.value))
    {
      sprintf(temp, "Maximum number of players reached (%d).", cv_maxplayers.value);
      return false;
    }

  if (!cv_allownewplayer.value)
    {
      sprintf(temp, "The server is not accepting new players at the moment.");
      return false;
    }


  // TODO read playerdata,

  return true; // server accepts
}


void LConnection::writeConnectAccept(BitStream *stream)
{
  // TODO check that name is unique, send corrected info back etc.

  //SV_SendServerConfig(node)
}


bool LConnection::readConnectAccept(BitStream *stream, const char **errorString)
{
  // TEST
  char temp[255];
  *errorString = temp; // in case we need it
  sprintf(temp, "up yours, server!\n");

  return true;
}



/// connection response functions

void LConnection::onConnectTimedOut()
{
  CONS_Printf("connect attempt timed out.\n");
  ConnectionTerminated(false);
}


void LConnection::onConnectionRejected(const char *reason)
{
  CONS_Printf("connection rejected.\n");
  ConnectionTerminated(false);
}


void LConnection::onConnectionEstablished(bool isInitiator)
{
  // initiator becomes the "client", the other one "server"
  Parent::onConnectionEstablished(isInitiator);

  // To see how this program performs with 50% packet loss,
  // Try uncommenting the next line :)
  //setSimulatedNetParams(0.5, 0);
   
  if (isInitiator)
    {
      setGhostFrom(false);
      setGhostTo(true);
      CONS_Printf("%s - connected to server.\n", getNetAddressString());
      LNetInterface *n = (LNetInterface *)getInterface();
      n->server_con = this;
      n->netstate = LNetInterface::CL_Connected;
    }
  else
    {
      // TODO: make scope object
      ((LNetInterface *)getInterface())->client_con.push_back(this);

      /*
	Player *player = new Player;
	myPlayer = player;
	myPlayer->setInterface(getInterface());
	myPlayer->addToGame(((TestNetInterface *) getInterface())->game);
	setScopeObject(myPlayer);
      */

      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
      CONS_Printf("%s - client connected.\n", getNetAddressString());
    }
}


void LConnection::onTimedOut()
{
  CONS_Printf("%s - connection to %s timed out.", getNetAddressString(), isConnectionToServer() ? "server" : "client" );
  ConnectionTerminated(true);
}


void LConnection::onConnectionError(const char *error)
{
  CONS_Printf("connection error '%s'\n", error);
  ConnectionTerminated(true);
}


void LConnection::onDisconnect(const char *reason)
{
  CONS_Printf("%s - %s disconnected.", getNetAddressString(), isConnectionToServer() ? "server" : "client" );
  ConnectionTerminated(true);
}


void LConnection::ConnectionTerminated(bool established)
{
  if (isConnectionToServer())
    {
      ((LNetInterface *)getInterface())->CL_Reset();
    }
  else
    {
      if (established)
	;// drop client, inform others
    }
}










/// Sets up network interface
// combination of D_CheckNetGame (called from doommain), I_InitTcpNetwork, D_ClientServerInit

void InitNetwork()
{
  CONS_Printf("Initializing network...");

  bool ipx = M_CheckParm("-ipx");

  TransportProtocol ptc = ipx ? IPXProtocol : IPProtocol;

  S32 port = DEFAULT_PORT;
  if (M_CheckParm ("-udpport"))
    port = atoi(M_GetNextParm());

  // set up the interface
  LNetInterface *n = new LNetInterface(Address(ptc, Address::Any, port));
  game.net = n;

  n->SetPingAddress(Address(ptc, Address::Broadcast, port));

  // misc. commandline parameters
  n->nodownload = M_CheckParm("-nodownload");


  // parse network game options,
  if (M_CheckParm("-server") || dedicated)
    {
      game.SV_SpawnServer();

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


  if (game.netgame)
    game.multiplayer = true;

  CONS_Printf("done.\n");
}






//
// TryRunTics
//
void GameInfo::TryRunTics(tic_t elapsed)
{
  extern  bool singletics;

  // local input
  ticcmd_t localcmds;
  ticcmd_t localcmds2;


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
    COM_BufExecute();

  I_GetEvent();
  D_ProcessEvents(); // read controls

  Menu::Ticker();
  CON_Ticker();

  // translate inputs (keyboard/mouse/joystick) into game controls
  localcmds.Build(true, elapsed);
  if (cv_splitscreen.value)
    localcmds2.Build(false, elapsed);

  net->Update();

  while (elapsed-- > 0)
    Ticker();
}




// is there a game running
bool GameInfo::Playing()
{
  return false;
  //return (game.server || (game.net->netstate == LNetInterface::CL_Connected));
}







void GameInfo::SV_ResetServer()
{
  // disconnect clients etc.
  net->SV_Reset();

  Clear_mapinfo_clusterdef();
  ClearPlayers();
  // clear teams?

  Z_FreeTags(PU_LEVEL, MAXINT);

  // reset gamestate?
  state = GS_NULL;
  netgame = false;

  CONS_Printf("Server reset\n");
}


bool GameInfo::SV_SpawnServer()
{
  if (!Playing())
    {
      CONS_Printf("Starting server...\n");
      SV_ResetServer();
      //if (cv_internetserver.value) RegisterServer(0, 0);

      net->SV_Open();

      server = true;
      netgame = true;
      multiplayer = true;

      state = GS_LEVEL;
      return true;
    }

  return false;
}



bool GameInfo::SV_SpawnLocalServer()
{
  if (!Playing())
    {
      CONS_Printf("Starting local server...\n");
      SV_ResetServer();

      server = true;
      netgame = false;
      multiplayer = cv_splitscreen.value;
      return true;
    }

  return false;
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

void Command_Version_f();
void Command_GameInfo_f();
void Command_MapInfo_f();
void Command_Players_f();
void Command_Frags_f();
void Command_TeamFrags_f();

void Command_Pause_f();
void Command_Quit_f ();
void Command_Connect_f();
void Command_Kick_f();
void Command_Kill_f();

void Command_Addfile_f();
void Command_Map_f();
void Command_RestartLevel_f();
void Command_ExitLevel_f();
void Command_RestartGame_f();
void Command_ExitGame_f();

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
void COM_T_DumpScript_f();
void COM_T_RunScript_f();
void COM_T_Running_f();


void T_Init();

// =========================================================================
//                           SERVER INIT
// =========================================================================

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

  // informational commands
  COM_AddCommand("version", Command_Version_f);
  COM_AddCommand("gameinfo", Command_GameInfo_f);
  COM_AddCommand("mapinfo", Command_MapInfo_f);
  COM_AddCommand("players", Command_Players_f);
  COM_AddCommand("frags", Command_Frags_f);
  COM_AddCommand("teamfrags", Command_TeamFrags_f);

  // basic commands for controlling the game
  COM_AddCommand("pause", Command_Pause_f);
  COM_AddCommand("quit",  Command_Quit_f);
  COM_AddCommand("connect", Command_Connect_f);
  COM_AddCommand("kick", Command_Kick_f);
  COM_AddCommand("kill", Command_Kill_f);

  COM_AddCommand("addfile", Command_Addfile_f);
  COM_AddCommand("map", Command_Map_f);
  COM_AddCommand("restartlevel", Command_RestartLevel_f);
  COM_AddCommand("exitlevel", Command_ExitLevel_f);
  COM_AddCommand("restartgame", Command_RestartGame_f);
  COM_AddCommand("exitgame", Command_ExitGame_f);

  // bots
  //COM_AddCommand("addbot", Command_AddBot_f);

  // saving, demos
  COM_AddCommand("save", Command_Save_f);
  COM_AddCommand("load", Command_Load_f);
  COM_AddCommand("playdemo", Command_Playdemo_f);
  COM_AddCommand("timedemo", Command_Timedemo_f);
  COM_AddCommand("stopdemo", Command_Stopdemo_f);

  /*
  COM_AddCommand("saveconfig", Command_SaveConfig_f);
  COM_AddCommand("loadconfig", Command_LoadConfig_f);
  COM_AddCommand("changeconfig", Command_ChangeConfig_f);
  */

  // chat commands
  COM_AddCommand("say"    , Command_Say_f);
  COM_AddCommand("sayto"  , Command_Sayto_f);
  COM_AddCommand("sayteam", Command_Sayteam_f);
  COM_AddCommand("chatmacro", Command_Chatmacro_f); // hu_stuff.c

  // cheat commands, I'm bored of deh patches renaming the idclev ! :-)
  COM_AddCommand("noclip", Command_CheatNoClip_f);
  COM_AddCommand("god", Command_CheatGod_f);
  COM_AddCommand("gimme", Command_CheatGimme_f);

  // miscellaneous subsystem commands
  COM_AddCommand("meminfo", Command_Meminfo_f);

  //Added by Hurdler for master server connection
  CV_RegisterVar(&cv_masterserver);
  COM_AddCommand("listserv", Command_Listserv_f);

  // register console variables
  CV_RegisterVar(&cv_internetserver);
  CV_RegisterVar(&cv_servername);
  CV_RegisterVar(&cv_allownewplayer);
  CV_RegisterVar(&cv_maxplayers);

  CV_RegisterVar(&cv_deathmatch);
  CV_RegisterVar(&cv_teamplay);
  CV_RegisterVar(&cv_teamdamage);
  CV_RegisterVar(&cv_exitmode);
  CV_RegisterVar(&cv_fraglimit);
  CV_RegisterVar(&cv_timelimit);

  CV_RegisterVar(&cv_allowjump);
  CV_RegisterVar(&cv_allowrocketjump);
  CV_RegisterVar(&cv_allowautoaim);
  CV_RegisterVar(&cv_allowmlook);

  CV_RegisterVar(&cv_itemrespawn);
  CV_RegisterVar(&cv_itemrespawntime);
  CV_RegisterVar(&cv_respawnmonsters);
  CV_RegisterVar(&cv_respawnmonsterstime);
  CV_RegisterVar(&cv_fragsweaponfalling);

  CV_RegisterVar(&cv_gravity);
  CV_RegisterVar(&cv_fastmonsters);
  CV_RegisterVar(&cv_solidcorpse);

  CV_RegisterVar(&cv_playdemospeed);
  CV_RegisterVar(&cv_netstat);

  // add chat macro consvars
  HU_HackChatmacros();

  // scripting
  COM_AddCommand("runacs", Command_RunACS_f);
#ifdef FRAGGLESCRIPT
  T_Init();
  COM_AddCommand("t_dumpscript", COM_T_DumpScript_f);
  COM_AddCommand("t_runscript",  COM_T_RunScript_f);
  COM_AddCommand("t_running",    COM_T_Running_f);
#endif


  R_InitData(); // must init texture cache now! Even server needs to know the widths etc of textures...

  InitNetwork();
}


