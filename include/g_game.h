// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.1  2002/11/16 14:18:23  hurdler
// Initial revision
//
// Revision 1.17  2002/08/31 11:40:19  vberghol
// menu and map loading bugfixes
//
// Revision 1.16  2002/08/30 11:45:40  vberghol
// players system modified
//
// Revision 1.15  2002/08/27 11:51:47  vberghol
// Menu rewritten
//
// Revision 1.14  2002/08/24 12:39:36  vberghol
// lousy fix
//
// Revision 1.13  2002/08/20 17:01:46  vberghol
// Now it compiles. Will it link? Or work?
//
// Revision 1.12  2002/08/20 13:57:00  vberghol
// sdfgsd
//
// Revision 1.11  2002/08/17 16:02:05  vberghol
// final compile for engine!
//
// Revision 1.10  2002/08/16 20:49:27  vberghol
// engine ALMOST done!
//
// Revision 1.9  2002/08/11 17:16:51  vberghol
// ...
//
// Revision 1.8  2002/08/06 13:14:27  vberghol
// ...
//
// Revision 1.7  2002/07/18 19:16:39  vberghol
// renamed a few files
//
// Revision 1.6  2002/07/13 17:56:57  vberghol
// *** empty log message ***
//
// Revision 1.5  2002/07/04 18:02:27  vberghol
// Pientä fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.4  2002/07/01 21:00:46  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:57  vberghol
// HUD alkaa olla kunnossa
//
//
// DESCRIPTION:
//   GameInfo class definition.
// 
//-----------------------------------------------------------------------------


#ifndef g_game_h
#define g_game_h 1

#include <vector>
#include <string>

#include "doomdef.h"
#include "d_event.h"
#include "d_ticcmd.h"

using namespace std;

class LevelNode;
class Map;
class PlayerInfo;
class TeamInfo;

struct fragsort_t;

// languages
typedef enum {
  english,
  french,
  german,
  unknown
} language_t;


// skill levels
typedef enum {
  sk_baby,
  sk_easy,
  sk_medium,
  sk_hard,
  sk_nightmare
} skill_t;

// Game mode handling - identify IWAD version,
//  handle IWAD dependent animations etc.
// change mode to a bitfield. We might be playing doom AND heretic (crossover game)?
typedef enum {
  shareware,    // DOOM 1 shareware, E1, M9
  registered,   // DOOM 1 registered, E3, M27
  commercial,   // DOOM 2 retail, E1 M34
  retail,       // DOOM 1 retail (Ultimate DOOM), E4, M36
  heretic,
  hexen,
  indetermined  // Well, no IWAD found.
} gamemode_t;


// Mission packs - might be useful for TC stuff? NOT!
typedef enum {
  gmi_none,
  gmi_doom2,     // DOOM 2, default
  gmi_tnt,  // TNT Evilution mission pack
  gmi_plut  // Plutonia Experiment pack
} gamemission_t;

// the current state of the game
typedef enum {
  GS_WIPE = -1,     // game never assumes this state, used to force a wipe
  GS_NULL = 0,                // at begin
  GS_LEVEL,                   // we are playing
  GS_INTERMISSION,            // gazing at the intermission screen
  GS_FINALE,                  // game final animation
  GS_DEMOSCREEN,              // looking at a demo
  //legacy
  GS_DEDICATEDSERVER,         // added 27-4-98 : new state for dedicated server
  GS_WAITINGPLAYERS           // added 3-9-98 : waiting player in net game
} gamestate_t;

// gameaction??
typedef enum {
  ga_nothing,
  ga_completed,
  ga_worlddone,
    //HeXen
/*
    ga_initnew,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_leavemap,
    ga_singlereborn
*/
} gameaction_t;

// boolean flags describing game properties (common to all players)
typedef enum {
  gf_multiplayer =    1,  // Only true if >1 player (or one remote player). 
  //netgame => multiplayer but not (multiplayer=>netgame)
  gf_netgame     =    2,  // Netgame? only true in a netgame
  gf_modified    =    4,  // added homemade stuff (PWADs, deh files?)
  gf_paused      =    8,  // only server can pause a multiplayer game
  gf_nomonsters  =   16  // no monsters
} gameflags_t;



/*
  GameInfo:  Info about one game, common to all players
  Should this be different for clients and servers? Probably no.

  Born: on a server when a new game is started, copied to joining clients?
  Dies: when server ends the game
*/


class GameInfo
{
private:
  gameaction_t  action; // delayed state changes
public:
  // demoversion is the 'dynamic' version number, this should be == game VERSION.
  // When playing back demos, 'demoversion' receives the version number of the
  // demo. At each change to the game play, demoversion is compared to
  // the game version, if it's older, the changes are not done, and the older
  // code is used for compatibility.

  byte demoversion;

  gamemode_t    mode;       // which game are we playing?
  gamemission_t mission;    // for specialized "mission pack" iwads. Deprecated.
  gamestate_t   state, wipestate;
  skill_t       skill;      // skill level

  //int flags;      // bit flags: multiplayer, nomonsters, allowcheats... for now just bools
  bool netgame;     // only true in a netgame (nonlocal players possible)
  bool multiplayer; // Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
  bool modified;    // an external modification-dll is in use
  bool nomonsters;  // checkparm of -nomonsters
  bool paused;      // Game Pause?

  bool inventory;   // true with heretic and hexen
  bool raven;       // true with heretic and hexen

  int maxplayers; // max # of players allowed
  int maxteams;   // max # of teams

  // two structures for storing players (each player must be in both)
  vector<PlayerInfo *> players; // no NULLs here, players not necessarily in order
  vector<PlayerInfo *> present; // NULL == not present, PI's in playernumber order
  // present[i]->number == i+1

  vector<TeamInfo *> teams;

  LevelNode *firstlevel;   // first LevelNode
  LevelNode *currentlevel; // in which levelNode are we in the game
  // one levelnode can include several maps to be opened a once

  vector<Map *> maps; // active map (level) info
  // several maps can be active at once!
  // same server consvars and game state/action for everyone though.
  // when one map is Exit():ed, all are exited.

  // vector<Map *> stasismaps;  // for Hexen-style hubs

public:

  // constructor
  GameInfo()
  {
    demoversion = VERSION;
    mode = indetermined;
    mission = gmi_doom2;
    state = GS_NULL;
    wipestate = GS_DEMOSCREEN;
    action = ga_nothing;
    skill = sk_medium;
    maxplayers = 32;
    maxteams = 4;
    present.resize(maxplayers);
    teams.resize(maxteams);
  };

  //destructor
  ~GameInfo()
  {
    ClearPlayers();
    maps.clear();
  };

  // in g_game.cpp
  void StartIntro();
  void Drawer();
  bool Responder(event_t *ev);
  void LoadGame(int slot);
  void SaveGame(int slot, char* description);


  bool Downgrade(int version);

  // in g_state.cpp
  bool DeferredNewGame(skill_t sk, LevelNode *n, bool splitscreen);
  bool StartGame();

  void Ticker(); // ticks the game forward in time

  // ----- player-related stuff -----
  // tries to add a player to the game
  PlayerInfo *AddPlayer(int pnum, PlayerInfo *in = NULL);
  // erases player pointed to by it from players vector
  void RemovePlayer(vector<PlayerInfo *>::iterator it);
  // erases all players
  void ClearPlayers();

  bool SameTeam(int a, int b);
  void UpdateScore(PlayerInfo *killer, PlayerInfo *victim);
  int  GetFrags(fragsort_t **fs, int type);

  // ----- level-related stuff -----
  void NewLevel(skill_t skill, LevelNode *n, bool resetplayer);
  void StartLevel(bool restart, bool resetplayer);
  void ExitLevel(int exit);
  void LevelCompleted();
  void EndIntermission();
  void EndFinale();
  void WorldDone();

  // ----- demos -----
  void BeginRecording();
  void PlayDemo(char *defdemoname);
  void ReadDemoTiccmd(ticcmd_t* cmd, int playernum);
  void WriteDemoTiccmd(ticcmd_t* cmd, int playernum);
  void StopDemo();
  bool CheckDemoStatus();
};

extern GameInfo game;

// =========================
// Status flags for refresh.
// =========================
// VB: miscellaneous stuff, doesn't really belong here

extern  bool         nodrawers;
extern  bool         noblit;

extern  bool   devparm;                // development mode (-devparm)

// Hardware flags

extern bool  nomusic; //defined in d_main.c
extern bool  nosound;
extern language_t language;

//unsigned int  hwflags;    // nomusic, nosound, hwrender...


// ======================================
// DEMO playback/recording related stuff.
// ======================================


// demoplaying back and demo recording
extern  bool demoplayback;
extern  bool demorecording;
extern  bool   timingdemo;       

// Quit after playing a demo from cmdline.
extern  bool         singledemo;

extern  tic_t           gametic;



struct consvar_t;

// used in game menu
extern consvar_t  cv_crosshair;
extern consvar_t  cv_autorun;
extern consvar_t  cv_invertmouse;
extern consvar_t  cv_alwaysfreelook;
extern consvar_t  cv_mousemove;
extern consvar_t  cv_showmessages;

extern consvar_t cv_crosshair2;
extern consvar_t cv_autorun2;
extern consvar_t cv_invertmouse2;
extern consvar_t cv_alwaysfreelook2;
extern consvar_t cv_mousemove2;
extern consvar_t cv_showmessages2;

extern consvar_t cv_fastmonsters;
//extern consvar_t  cv_crosshairscale;
extern consvar_t cv_joystickfreelook;

void  Command_Turbo_f();

short G_ClipAimingPitch(int* aiming);

extern angle_t localangle,localangle2;
extern int     localaiming,localaiming2; // should be a angle_t but signed

void G_BuildTiccmd(ticcmd_t* cmd, bool primary, int realtics);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame(int slot);

// Called by M_Responder.
void G_SaveGame(int slot, char* description);

// in g_state.cpp

LevelNode *G_CreateClassicMapList(int episode);

void G_ReadDemoTiccmd(ticcmd_t* cmd,int playernum);
void G_WriteDemoTiccmd(ticcmd_t* cmd,int playernum);


void G_DeferedPlayDemo(char* demo);
void G_DoneLevelLoad();

// Only called by startup code.
void G_RecordDemo(char* name);

void G_TimeDemo(char* name);

#endif
