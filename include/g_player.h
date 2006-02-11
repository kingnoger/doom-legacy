// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief PlayerInfo class definition.

#ifndef g_player_h
#define g_player_h 1

#include <map>
#include <vector>
#include <deque>
#include <string>
#include "tnl/tnlNetObject.h"
#include "doomtype.h"
#include "d_ticcmd.h"
#include "d_items.h"

using namespace std;
using namespace TNL;


/// Player states.
enum playerstate_t
{
  PST_NEEDMAP,      ///< waiting to be assigned to a map respawn queue (by GameInfo)
  PST_RESPAWN,      ///< waiting in a respawn queue (of a certain Map)
  PST_ALIVE,        ///< playing, camping or spectating
  PST_DEAD,         ///< dead on the ground, view follows killer
  PST_INTERMISSION, ///< viewing an intermission
  PST_REMOVE        ///< waiting to be removed from the game
};


/// \brief Network player options and preferences.
///
/// Things about your avatar that can be changed locally,
/// and which you can setup _before_ entering a game.
/// The server may override any of these when you join a game.
/// Also, you may be given new choices (team etc.) after joining.
class PlayerOptions
{
public:
  /// requested playername
  string name;
  
  // Pawn preferences. Can be changed during the game, take effect at next respawn.
  int ptype;  ///< what kind of pawn are we playing?
  int pclass; ///< pawn class (from Hexen)
  int color; ///< skin color to be copied to each pawn
  int skin;  ///< skin to be copied to each pawn

  // Weapon preferences
  bool autoaim;                 ///< using autoaim?
  bool originalweaponswitch;    ///<
  unsigned char weaponpref[NUMWEAPONS];  ///< 

  // Message preferences
  int  messagefilter; ///< minimum message priority the player wants to receive

  // TODO chasecam prefs

public:
  PlayerOptions(const string &n = "");

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);

  void Write(BitStream *stream);
  void Read(BitStream *stream);
};


/// \brief Player options and preferences.
///
/// This class adds the purely local options which are never sent to server.
class LocalPlayerInfo : public PlayerOptions
{
public:
  int  controlkeyset;
  bool autorun;
  int  crosshair; // TODO problem

  int  pnumber;  ///< player number the server sent during joining
  class PlayerInfo *info; ///< Pointer to the corresponding (ghosted) PlayerInfo object.
  class BotAI *ai;        ///< possible bot AI controlling the player

public:
  LocalPlayerInfo(const string &n = "bot", int keyset = 0);
  void GetInput(int elapsed);
};


/// \brief Describes a single player, either human or AI.
///
/// Created when a player joins the game, deleted when he leaves.
/// Ghosted over network.
class PlayerInfo : public NetObject
{
  friend class GameInfo;
  typedef NetObject Parent;
  TNL_DECLARE_CLASS(PlayerInfo);

  virtual bool onGhostAdd(class GhostConnection *c);
  virtual void onGhostRemove();
  virtual U32  packUpdate(GhostConnection *c, U32 updateMask, class BitStream *s);
  virtual void unpackUpdate(GhostConnection *c, BitStream *s);

public:
  string name;     ///< name of the player
  int    number;   ///< player number
  int    team;     ///< player's team

  class LConnection *connection; ///< network connection
  unsigned client_hash;          ///< hash of the client network address

  playerstate_t playerstate;
  bool spectator;     ///< ghost spectator in a map?
  bool map_completed; ///< TEST has finished the map, but continues playing
  bool leaving_map;   ///< TEST

  int requestmap;  ///< the map which we wish to enter
  int entrypoint;  ///< which spawning point to use

  class Map        *mp;   ///< the map with which the player is currently associated
  class PlayerPawn *pawn; ///< the thing that is being controlled by this player (marine, imp, whatever)
  class Actor      *pov;  ///< the POV of the player. usually same as pawn, but can also be a chasecam etc...

  PlayerPawn *hubsavepawn; ///< copy of the pawn made when entering a map within a hub
  //============ Score ============

  map<int, int> Frags; ///< mapping from player number to how many times you have fragged him
  int score;           ///< game-type dependent scoring based on frags, updated in real time
  int kills, items, secrets, time; ///< accomplishments in the current Map

  //============ Messages ============

  enum messagetype_t
  {
    M_CONSOLE = 0, ///< print message on console (echoed briefly on HUD)
    M_HUD          ///< glue message to HUD for a number of seconds
  };

  struct message_t
  {
    int priority;
    int type;
    string msg;
  };

  deque<message_t> messages; ///< local message queue

  //============ Feedback ============

  // POV height and bobbing during movement.
  fixed_t  viewz;           ///< absolute viewpoint z coordinate
  fixed_t  viewheight;      ///< distance from feet to eyes
  fixed_t  deltaviewheight; ///< bob/squat speed.

  // HUD flashes
  int palette;
  int damagecount;
  int bonuscount;
  //int poisoncount;
  bool itemuse;

  //============ Options ============

  PlayerOptions options;

  //============ Control ============

  ticcmd_t    cmd;  ///< current state of the player's controls
  int invTics;  ///< When >0, show inventory in HUD
  int invSlot;  ///< Active inventory slot is pawn->inventory[invSlot]
  int invPos;   ///< Position of the active slot on HUD, always 0-6

public:
  PlayerInfo(const LocalPlayerInfo *p = NULL);

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);

  void SavePawn();
  void LoadPawn();

  bool InventoryResponder(short (*gc)[2], struct event_t *ev);

  void ExitLevel(int nextmap, int ep);
  bool CheckIntermission(const Map *m);
  void Reset(bool resetpawn, bool resetfrags);  // resets the player (when starting a new level, for example)

  virtual void SetMessage(const char *msg, int priority = 0, int type = M_CONSOLE);

  /// Client: Calculate the walking / running viewpoint bobbing and weapon swing
  void CalcViewHeight();
};


// TEMP
enum
{
  NUM_LOCALHUMANS = 2,
  NUM_LOCALBOTS = 10,
  NUM_LOCALPLAYERS = NUM_LOCALHUMANS + NUM_LOCALBOTS
};

/// Local human players, then local bot players
extern LocalPlayerInfo LocalPlayers[NUM_LOCALPLAYERS];

/// Observed players (multiple viewports...)
extern vector<PlayerInfo *> ViewPlayers;

bool G_RemoveLocalPlayer(PlayerInfo *p);

#endif
