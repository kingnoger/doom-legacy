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
//
// $Log$
// Revision 1.1  2002/11/16 14:18:20  hurdler
// Initial revision
//
// Revision 1.7  2002/09/17 14:26:28  vberghol
// switch bug fixed
//
// Revision 1.6  2002/09/15 17:31:22  jpakkane
// Fix Linux compilation issue.
//
// Revision 1.5  2002/09/08 14:38:08  vberghol
// Now it works! Sorta.
//
// Revision 1.4  2002/08/06 13:14:27  vberghol
// ...
//
// Revision 1.3  2002/07/01 21:00:41  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:20  vberghol
// Version 133 Experimental!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   AutoMap class
//
//-----------------------------------------------------------------------------

#ifndef am_map_h
#define am_map_h 1

#include<unistd.h> // For definition of NULL.

#include "doomtype.h"
#include "m_fixed.h"

class Map;
class PlayerPawn;
struct mpoint_t;
struct patch_t;
struct event_t;

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

// TODO ! the AutoMap class is not yet complete,
// there is still some class data that is just defined static.
// No problem as long as you only have one AutoMap.

class AutoMap
{
private:
  int  amclock; // tic counter

  bool bigstate;  // toggle between user view and large view (full map view)
  bool followplayer; // specifies whether to follow the player around
  bool grid;

  const Map *mp;     // currently seen Map
  const PlayerPawn *mpawn; // the pawn represented by an arrow

  patch_t *marknums[10];            // numbers used for marking by the automap

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
  bool Responder(event_t* ev);
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

// Used by ST StatusBar stuff.
#define AM_MSGHEADER (('a'<<24)+('m'<<16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e'<<8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x'<<8))

#endif
