// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2004 by Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log$
// Revision 1.9  2004/10/14 19:35:50  smite-meister
// automap, bbox_t
//
// Revision 1.5  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.1  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.7  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.6  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.5  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.4  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
//--------------------------------------------------------------------------

/// \file
/// \brief MapInfo class definition

#ifndef g_mapinfo_h
#define g_mapinfo_h 1

#include <string>
#include <vector>
#include <map>

using namespace std;

/// \brief Stores all kinds of metadata for an associated Map.
///
/// OK. Doom map format is (undestandably) primitive and has no place for
/// general information about maps. There are at least two ways to fix this:
///
/// 1)  MAPINFO lump (from Hexen and ZDoom)
///     There can be one MAPINFO lump per wad. It is a text lump that consists of
///     two kinds of blocks: MAP blocks and CLUSTERDEF blocks.
///
/// 2)  LevelInfo lumps (Legacy, others?)
///     There is one LevelInfo lump per Map. It is the same lump as the
///     mapname "separator", i.e. MAP16, E6M2, MYOWNMAP... 
///     LevelInfo only contains info about the one map to which it belongs.
///     It is a text lump that consists of blocks, such as [script], ...
///
/// Internally, all the data for each map is combined into one class, MapInfo.
/// It contains all the information regarding a Map that doesn't directly affect
/// gameplay but can be of interest to human players, such as the Map
/// name, author, recommendation for # of players, hints, music that is
/// played during the map, partime etc.

class MapInfo
{
  friend class Map;

public:
  /// current state of the corresponding Map
  enum mapstate_e
    {
      MAP_UNLOADED = 0, ///< not loaded, "me" should be NULL
      MAP_RUNNING,      ///< currently running, "me" is valid
      MAP_INSTASIS,     ///< presently halted, but "me" is still valid
      MAP_RESET,        ///< (single player) player has died, the map should be reset
      MAP_FINISHED,     ///< (at least some) players have finished the map, but it is still running, "me" is valid
      MAP_SAVED         ///< "me" should be NULL, the Map is saved on disk
    };

  mapstate_e   state;
  Map         *me;   ///< the actual running Map instance corresponding to this MapInfo

  string lumpname;   ///< map lump name ("MAP04")
  string nicename;   ///< map long nice name ("The Nuclear Plant")
  string savename;   ///< name of the file the map is currently saved in

  int    cluster;    ///< in which cluster does this map belong?
  int    mapnumber;  ///< real map number, used with Teleport_NewMap

  string version;   ///< map version string
  string author;    ///< map creator
  string hint;      ///< optional hint for the map

  // string previewpic; // preview picture lumpname

  int   scripts; ///< how many FS scripts in the map?
  int   partime; ///< in seconds
  float gravity; ///< temporary, could be part of ZoneInfo or sector_t?


  // thing number mappings (experimental)
  int doom_offs[2];
  int heretic_offs[2];
  int hexen_offs[2];

  // less essential stuff (can be replaced with scripting or other more efficient means)
  string musiclump;

  int    warptrans;    ///< a Hexen quirk, another map numbering for deathmatch
  int    warpnext;     ///< this one uses warptrans numbers
  int    nextlevel;    ///< next map number (normal Doom exit)
  int    secretlevel;  ///< next map number (Doom secret exit)

  bool   doublesky;
  bool   lightning;
  string sky1;
  float  sky1sp;
  string sky2;
  float  sky2sp;

  int    cdtrack;
  string fadetablelump;

  int BossDeathKey;  ///< bit flags to see which bosses end the map when killed.

public:
  MapInfo();
  ~MapInfo();

  void Ticker(bool hub);
  bool Activate(class PlayerInfo *p);
  int  EvictPlayers(int next, int ep = 0, bool force = false);
  void Close(int next, int ep = 0, bool force = false);
  bool HubSave();
  bool HubLoad();

  char *Read(int lumpnum);

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
};


#endif
