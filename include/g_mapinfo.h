// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// $Id$
//
// Portions Copyright(C) 2000 Simon Howard
// Copyright (C) 2002-2003 by Doom Legacy Team
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
// Revision 1.4  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.3  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.2  2003/11/30 00:09:47  smite-meister
// bugfixes
//
// Revision 1.1  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.9  2003/06/10 22:40:01  smite-meister
// Bugfixes
//
// Revision 1.8  2003/05/11 21:23:52  smite-meister
// Hexen fixes
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
//
//--------------------------------------------------------------------------
#ifndef g_mapinfo_h
#define g_mapinfo_h 1

#include <string>
#include <vector>
#include <map>

using namespace std;

// OK. Doom map format is (undestandably) primitive and has no place for
// general information about maps. There are at least two ways to fix this:

// 1)  MAPINFO lump (from Hexen and ZDoom)
//     There can be one MAPINFO lump per wad. It is a text lump that consists of
//     two kinds of blocks: MAP blocks and CLUSTERDEF blocks.

// 2)  MapInfo lumps (Legacy, others?)
//     There is one MapInfo lump per _map_. It is the same lump as the
//     mapname "separator", i.e. MAP16, E6M2, MYOWNMAP... 
//     MapInfo only contains info about the one map to which it belongs.
//     It is a text lump that consists of blocks, such as [script], ...

//     Internally, all the data for each map is combined into one class, MapInfo

enum mapstate_e
{
  MAP_UNLOADED = 0, // not loaded, "me" should be NULL
  MAP_RUNNING,      // currently running, "me" is valid
  MAP_INSTASIS,     // presently halted, but "me" is still valid
  MAP_FINISHED,     // (at least some) players have finished the map, but it is still running, "me" is valid
  MAP_SAVED         // "me" should be NULL, the map is saved on disk
};


class MapInfo
{
  friend class Map;

public:
  mapstate_e   state;
  Map         *me;    // the actual running Map instance corresponding to this MapInfo

  string lumpname;   // map lump name ("MAP04")
  string nicename;   // map long nice name ("The Nuclear Plant")
  string savename;   // name of the file the map is currently saved in

  int    cluster;
  int    mapnumber;  // real map number, used with Teleport_NewMap

  unsigned entrypoint;  // the requested entry point in this map
  int    exitused, exitloc; // exit used when the level was last exited

  //map<int, MapInfo *> exit;  // possible remapping of exit numbers
  // for now it's simple: exit 0 is the "normal exit", exit 100 is the "secret exit"
  // exit 255 means that the game ends here.

  string version;   // map version string
  string author;    // map creator
  string hint;      // optional hint for the map

  // string previewpic; // preview picture lumpname

  int   scripts; // how many FS scripts in the map?
  int   partime; // in seconds
  float gravity; // temporary, could be part of ZoneInfo or sector_t?


  // thing number mappings (experimental)
  int doom_offs[2];
  int heretic_offs[2];
  int hexen_offs[2];

  // less essential stuff (can be replaced with scripting or other more efficient means)
  string musiclump;

  int    warptrans;      // a Hexen quirk, another map numbering for deathmatch
  int    warpnext;       // this one uses warptrans numbers
  //char  nextmaplump[17];
  //char  secretmaplump[17];
  int    nextlevel;
  int    secretlevel;
  bool   doublesky;
  bool   lightning;
  // ordering matters here!
  string sky1;
  float  sky1sp;
  string sky2;
  float  sky2sp;

  int    cdtrack;
  string fadetablelump;

  int BossDeathKey;  // bit flags to see which bosses end the map when killed.

public:
  MapInfo();
  ~MapInfo();

  void Ticker(bool hub);
  bool Activate(class PlayerInfo *p);
  int  EvictPlayers(int n, int ep);
  void Close();
  bool HubSave();
  bool HubLoad();

  char *Read(int lumpnum);

  int Serialize(class LArchive &a);
  int Unserialize(LArchive &a);
};


#endif
