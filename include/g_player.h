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
// $Log$
// Revision 1.10  2003/11/30 00:09:47  smite-meister
// bugfixes
//
// Revision 1.9  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.8  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.7  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.6  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.5  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.4  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.3  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.2  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
//
//
// DESCRIPTION:
//    PlayerInfo class definition. It describes one human player.
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


//
// Player states.
//
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


class PlayerInfo
{
  friend class GameInfo;
public:
  int    number;   // The player number.
  int    team;     // index into game.teams vector
  string name;

  // Can be changed during the game, takes effect at next respawn.
  int ptype; // what kind of pawn are we playing?
  int color; // skin color to be copied to each pawn
  int skin;  // skin to be copied to each pawn

  bool spectator;
  playerstate_t playerstate;
  ticcmd_t  cmd;

  int requestmap; // the map which we wish to enter
  int entrypoint; // which spawning point to use

  // Determine POV,
  //  including viewpoint bobbing during movement.
  fixed_t  viewz;   // Focal origin above r.z
  fixed_t  viewheight;  // Base height above floor for viewz.
  fixed_t  deltaviewheight;  // Bob/squat speed.
  fixed_t  bob;  // bounded/scaled total momentum.

  // Frags, kills of other players. (type was USHORT)
  map<int, int> Frags; // player number -> frags
  int score; // game-type dependent scoring based on frags, updated in real time

  int kills, items, secrets, time;

  // Hint messages.
  const char *message;

  // added by Boris : preferred weapons order stuff
  char  favoriteweapon[NUMWEAPONS];
  bool  originalweaponswitch;

  bool autoaim; // using autoaim?

  class Map        *mp;   // the map with which the player is currently associated
  class PlayerPawn *pawn; // the thing that is being controlled by this player (marine, imp, whatever)

public:

  PlayerInfo(const string & n = "");

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
  // resets the player (when starting a new level, for example)
  void ExitLevel(int nextmap, int ep, bool force = false);
  void Reset(bool resetpawn, bool resetfrags);

  void SetMessage(const char *msg, bool ultmsg = true);

  // in g_game.cpp
  bool InventoryResponder(int (*gc)[2], struct event_t *ev);
};


// Player taking events, and displaying.
extern  PlayerInfo *consoleplayer;
extern  PlayerInfo *consoleplayer2;
extern  PlayerInfo *displayplayer;
extern  PlayerInfo *displayplayer2; // for splitscreen

// model PI's for both local players
extern PlayerInfo localplayer;
extern PlayerInfo localplayer2;

#endif
