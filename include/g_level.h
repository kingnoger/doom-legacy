// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
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
// Revision 1.7  2003/07/02 17:52:46  smite-meister
// VDir fix
//
// Revision 1.6  2003/06/10 22:39:59  smite-meister
// Bugfixes
//
// Revision 1.5  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.4  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
//
// DESCRIPTION:
//   LevelNode class.
//-----------------------------------------------------------------------------

// These are used to build a graph describing connections
// between maps, e.g. which map follows which. A levelnode
// also holds all the necessary information for the intermission/finale
// following the level.

#ifndef g_level_h
#define g_level_h 1

#include <map>
#include <string>

using namespace std;

// (implemented as sparse graphs). These generalize map ordering.
// idea: each map has n "exit methods". These could be
// normal exits, secret exits, portals etc. Each exit number
// corresponds to a destination in the 'exit' map.
// note! several exits in the Map can have the same exit number,
// exit numbers can be remapped to point to new destinations.

// for now it's simple: exit[0] is the normal exit, exit[100] secret exit
// exits are pointers to new levelnodes. NULL means episode ends here.

class LevelNode
{
  friend class GameInfo;
  friend class Map;

public:
  unsigned  number;     // unique level number
  unsigned  cluster;    // level cluster (for finales, hubs etc. See p_info.h)
  string    levelname;  // nice long name

  unsigned entrypoint;  // the requested entry point in this level (Hexen)  
  map<int, LevelNode *> exit;  // remapping of exit numbers
  LevelNode *exitused;  // exit used when the map was last exited

  bool done;            // has it been finished yet?

  vector<class MapInfo_t *> contents; // the maps which make up this level

public:
  int kills, items, secrets;  // level totals
  int time, partime; // the time it took to complete level, partime (in s)

  // old Doom relics, deprecated
  // Scripting is a better solution for new levels.
  int BossDeathKey; // What will boss deaths accomplish?

  // intermission data for the level.
  int episode; // Episode. Only used to decide which intermission to show.
  string interpic; // intermission background picture lumpname
  //intermission music lumpname?

public:
  LevelNode();
  LevelNode(class MapInfo_t *info);
};


#endif
