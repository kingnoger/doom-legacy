// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.5  2003/06/10 22:39:59  smite-meister
// Bugfixes
//
// Revision 1.4  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   GameInfo class definition.
// 
//-----------------------------------------------------------------------------


#ifndef g_game_h
#define g_game_h 1

#include <vector>
#include <map>
#include <string>

#include "doomdef.h"
#include "doomdata.h"

using namespace std;


// languages
enum language_t
{
  la_english,
  la_french,
  la_german,
  la_unknown
};


// skill levels
enum skill_t
{
  sk_baby,
  sk_easy,
  sk_medium,
  sk_hard,
  sk_nightmare
};

// Game mode handling - identify IWAD version,
//  handle IWAD dependent animations etc.
// change mode to a bitfield? We might be playing doom AND heretic (crossover game)?
enum gamemode_t
{
  gm_none,
  gm_doom1s,  // DOOM 1 shareware, E1, M9
  gm_doom1,   // DOOM 1 registered, E3, M27
  gm_doom2,   // DOOM 2 retail (commercial), E1 M34
  gm_udoom,   // DOOM 1 retail (Ultimate DOOM), E4, M36
  gm_heretic,
  gm_hexen
};


// Mission packs - might be useful for TC stuff? NOT! This is a RELIC! Do NOT use!
enum gamemission_t
{
  gmi_none,
  gmi_doom2,  // DOOM 2, default
  gmi_tnt,    // TNT Evilution mission pack
  gmi_plut    // Plutonia Experiment pack
};

// the current state of the game
enum gamestate_t
{
  GS_WIPE = -1,     // game never assumes this state, used to force a wipe
  GS_NULL = 0,                // at begin
  GS_LEVEL,                   // we are playing
  GS_INTERMISSION,            // gazing at the intermission screen
  GS_FINALE,                  // game final animation
  GS_DEMOSCREEN,              // looking at a demo
  //legacy
  GS_DEDICATEDSERVER,         // added 27-4-98 : new state for dedicated server
  GS_WAITINGPLAYERS           // added 3-9-98 : waiting player in net game
};


/*
  GameInfo:  Info about the game, common to all players
  Should this be different for clients and servers? Probably not.

  Born: on a server when a new game is started, copied to joining clients?
  Dies: when server ends the game
*/


class GameInfo
{
  friend class Intermission;
  friend class PlayerInfo;
  friend class NetCode; // kludge until netcode is rewritten
private:
  // gameaction: delayed game state changes
  enum gameaction_t
  {
    ga_nothing,
    ga_levelcompleted,
    ga_nextlevel,
    //HeXen
    /*
    ga_initnew,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_leavemap,
    ga_singlereborn
    */
  };

  gameaction_t  action; // delayed state changes
public:
  // demoversion is the 'dynamic' version number, this should be == game VERSION.
  // When playing back demos, 'demoversion' receives the version number of the
  // demo. At each change to the game play, demoversion is compared to
  // the game version, if it's older, the changes are not done, and the older
  // code is used for compatibility.

  unsigned demoversion;

  gamemode_t    mode;       // which game are we playing?
  gamemission_t mission;    // for specialized "mission pack" iwads. Deprecated.
  gamestate_t   state, wipestate;
  skill_t       skill;      // skill level

  bool netgame;     // only true in a netgame (nonlocal players possible)
  bool multiplayer; // Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
  bool modified;    // an external modification-dll is in use
  bool nomonsters;  // checkparm of -nomonsters
  bool paused;      // Game Pause?

  bool inventory;   // playerpawns have an inventory

protected:
  int maxplayers; // max # of players allowed
  int maxteams;   // max # of teams

  typedef map<int, PlayerInfo *>::iterator player_iter_t;
  map<int, PlayerInfo *> Players; // mapping from player number to Playerinfo*

  vector<class TeamInfo *> teams;

  map<int, class LevelNode *> levelgraph;
  LevelNode *currentlevel; // in which LevelNode are we in the game

  vector<class Map *> maps; // active map (level) info
  // several maps can be active at once!
  // same server consvars and game state/action for everyone though.
  // when one map is Exit():ed, all are exited.

  // vector<Map *> stasismaps;  // for Hexen-style hubs

public:

  GameInfo();
  ~GameInfo();

  // in g_game.cpp
  void StartIntro();
  void Drawer();
  bool Responder(struct event_t *ev);
  int  Serialize(class LArchive &a);
  void LoadGame(int slot);
  void SaveGame(int slot, char* description);

  bool Downgrade(int version);

  void Ticker(); // ticks the game forward in time

  // ----- player-related stuff -----
  PlayerInfo *AddPlayer(int pnum, PlayerInfo *in = NULL); // tries to add a player to the game
  bool RemovePlayer(int number);  // erases player from game
  void ClearPlayers();            // erases all players
  PlayerInfo *FindPlayer(int number); // returns player 'number' if he is in the game, otherwise NULL

  void UpdateScore(PlayerInfo *killer, PlayerInfo *victim);
  int  GetFrags(struct fragsort_t **fs, int type);
  bool CheckScoreLimit();

  // ----- level-related stuff -----
  // in g_level.cpp
  int Create_MAPINFO_levelgraph(int lump);
  int Create_Classic_levelgraph(int episode);

  // in g_state.cpp
  bool DeferredNewGame(skill_t sk, bool splitscreen);
  bool StartGame();
  void SetupLevel(LevelNode *n, skill_t skill, bool resetplayers);
  void StartLevel(bool restart, bool resetplayers);
  void ExitLevel(int exit, unsigned entrypoint);
  void EndLevel();
  void EndIntermission();
  void EndFinale();
  void NextLevel();

  // ----- demos -----
  void BeginRecording();
  void PlayDemo(char *defdemoname);
  void ReadDemoTiccmd(struct ticcmd_t* cmd, int playernum);
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

void G_LoadGame(int slot);
void G_SaveGame(int slot, char* description);

void G_ReadDemoTiccmd(ticcmd_t* cmd,int playernum);
void G_WriteDemoTiccmd(ticcmd_t* cmd,int playernum);


void G_DeferedPlayDemo(char* demo);
void G_DoneLevelLoad();

// Only called by startup code.
void G_RecordDemo(char* name);

void G_TimeDemo(char* name);

#endif
