// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
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
// Revision 1.13  2004/07/14 16:13:13  smite-meister
// cleanup, commands
//
// Revision 1.12  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.11  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.10  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.9  2003/11/30 00:09:47  smite-meister
// bugfixes
//
// Revision 1.8  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
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
//-----------------------------------------------------------------------------

// \file
// \brief MapCluster class.

#ifndef g_level_h
#define g_level_h 1

#include <map>
#include <string>

using namespace std;


/// \brief MapCluster class is used to group maps together into hubs or episodes.
///
/// All maps within a cluster share the same intermission.
/// When a cluster is entered/exited, a finale may
/// take place. Clusters are also used for hubs.
/// They roughly correspond to Hexen/ZDoom clusters and Doom/Heretic episodes.
/// Within a cluster, several Maps may run simultaneously.
class MapCluster
{
  friend class GameInfo;
  friend class Map;

public:
  int    number;      ///< unique levelcluster number
  string clustername; ///< nice long name for the cluster ("Knee-deep in the dead")

  bool   hub;       ///< if true, save the maps when they are exited.
  bool   keepstuff; ///< if true, keys and powers are never taken away inside this cluster

  vector<class MapInfo *> maps; ///< the maps which make up this level

  int kills, items, secrets;  ///< cluster totals
  int time, partime; ///< the time it took to complete cluster, partime (in s)

  // intermission data
  string interpic;  ///< intermission background picture lumpname
  string intermusic;

  // finale data
  string entertext;
  string exittext;
  string finalepic;
  string finalemusic;
  int    episode; ///< which finale to show?

  MapCluster();
  MapCluster(int n);

  /// ticks all the active MapInfo's in the cluster
  void Ticker();
  /// shuts the cluster down, kicks out all players
  void Finish(int nextmap, int entrypoint = 0); 

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
};


#endif
