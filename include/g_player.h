// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log$
// Revision 1.14  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.13  2004/04/25 16:26:51  smite-meister
// Doxygen
//
//-----------------------------------------------------------------------------


#ifndef g_player_h
#define g_player_h 1

#include <map>
#include <string>
#include "doomtype.h"
#include "d_ticcmd.h"
#include "d_items.h"

using namespace std;

/// \file
/// \brief PlayerInfo class definition.



/// Player states.
enum playerstate_t
{
  PST_WAITFORMAP, // waiting to be assigned to a map respawn queue (by GameInfo)
  PST_SPECTATOR,  // a ghost spectator in a map. when player presses a key, he is sent to a respawn queue.
  PST_RESPAWN,    // waiting in a respawn queue (of a certain Map)
  PST_ALIVE,      // Playing or camping.
  PST_DONE,       // has finished the map, but continues playing
  PST_DEAD,       // Dead on the ground, view follows killer.
  PST_REMOVE      // waiting to be removed from the game
};


/// \brief One client-side player, either human or AI.
///
/// Created when a player joins the game, deleted when he leaves.
class PlayerInfo
{
  friend class GameInfo;
public:
  int    number;   ///< The player number.
  int    team;     ///< index into game.teams vector
  string name;

  // Can be changed during the game, takes effect at next respawn.
  int ptype; ///< what kind of pawn are we playing?
  int color; ///< skin color to be copied to each pawn
  int skin;  ///< skin to be copied to each pawn

  class LConnection *conn; ///< network connection
  bool spectator;
  playerstate_t playerstate;
  ticcmd_t  cmd;

  int requestmap; ///< the map which we wish to enter
  int entrypoint; ///< which spawning point to use

  // Frags, kills of other players. (type was USHORT)
  map<int, int> Frags; ///< player number -> frags
  int score; ///< game-type dependent scoring based on frags, updated in real time

  int kills, items, secrets, time;

  /// Hint messages.
  const char *message;

  // weapon preferences
  char  weaponpref[NUMWEAPONS];
  bool  originalweaponswitch;

  bool autoaim; ///< using autoaim?

  class Map        *mp;   ///< the map with which the player is currently associated
  class PlayerPawn *pawn; ///< the thing that is being controlled by this player (marine, imp, whatever)
  class Actor      *pov;  ///< the POV of the player. usually same as pawn, but can also be a chasecam etc...

  // POV height and bobbing during movement.
  fixed_t  viewz;           ///< absolute viewpoint z coordinate
  fixed_t  viewheight;      ///< distance from feet to eyes
  fixed_t  deltaviewheight; ///< bob/squat speed.
  fixed_t  bob_amplitude;   ///< basically pawn speed squared, affects weapon movement

public:

  PlayerInfo(const string & n = "");

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
  void ExitLevel(int nextmap, int ep);
  void Reset(bool resetpawn, bool resetfrags);  // resets the player (when starting a new level, for example)

  void SetMessage(const char *msg, bool ultmsg = true);
  void CalcViewHeight(bool onground); // update bobbing view height

  // in g_game.cpp
  bool InventoryResponder(int (*gc)[2], struct event_t *ev);
};


/// Locally controlled players
extern  PlayerInfo *consoleplayer;
extern  PlayerInfo *consoleplayer2;
// Players whose view is rendered on screen
extern  PlayerInfo *displayplayer;
extern  PlayerInfo *displayplayer2; // for splitscreen

// model PI's for both local players
extern PlayerInfo localplayer;
extern PlayerInfo localplayer2;

#endif
