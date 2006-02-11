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

  int time, partime; ///< the time it took to complete cluster, partime (in s)

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
