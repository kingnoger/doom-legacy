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
// Revision 1.6  2004/09/23 23:21:19  smite-meister
// HUD updated
//
// Revision 1.5  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.4  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.3  2004/01/06 14:37:45  smite-meister
// six bugfixes, cleanup
//
// Revision 1.2  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
// Revision 1.1.1.1  2002/11/16 14:18:20  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief AutoMap class

#ifndef am_map_h
#define am_map_h 1

#include "doomtype.h"
#include "m_fixed.h"


struct fpoint_t
{
  int x, y;
};

struct fline_t
{
  fpoint_t a, b;
};

struct mpoint_t
{
  fixed_t x,y;
};


/// \brief The in-game overhead map.
///
/// There is only one global instance in use, called "automap".
class AutoMap
{
private:
  int  amclock; // tic counter

  bool bigstate;  // toggle between user view and large view (full map view)
  bool followplayer; // specifies whether to follow the player around
  bool grid;

  const class Map *mp;     // currently seen Map
  const class PlayerPawn *mpawn; // the pawn represented by an arrow

  class Texture *marknums[10]; // numbers used for marking by the automap

  static const int AM_NUMMARKPOINTS = 10;
  mpoint_t markpoints[AM_NUMMARKPOINTS];   // where the points are
  int markpointnum;                    // next point to be assigned
  byte *mapback; // pointer to the raw data for the automap background.

public:
  bool active;
  bool translucent; // draw overlaid on 3D view?
  bool am_recalc;   // true if screen size changes
  int  am_cheating; // AutoMap cheats

  AutoMap();

  // Called by main loop.
  void Ticker();
  void doFollowPlayer();

  // eats events
  bool Responder(struct event_t *ev);
  void addMark();
  void clearMarks();
  void restoreScaleAndLoc();
  void changeWindowLoc();

  // called after a vidmode change
  void Resize();

  // opens the map, sets mpawn and mp.
  void Open(const PlayerPawn *p);
  void loadPics();
  void InitVariables();

  // called after a map change
  void Reset(const PlayerPawn *p);

  void Close();
  void unloadPics();

  // drawing
  void Drawer();
  void clearFB(int color);
  void drawGrid(int color);
  void drawWalls();
  void drawPlayers();
  void drawThings(int colors, int colorrange);
  void drawMarks();
  void updateLightLev();
};

extern AutoMap automap;

#endif
