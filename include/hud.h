// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// $Log$
// Revision 1.2  2004/09/24 11:34:00  smite-meister
// fix
//
// Revision 1.1  2004/09/23 23:21:19  smite-meister
// HUD updated
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Heads up display. Everything that is rendered on the 3D view.
/// Includes status bar, crosshair, icons...

#ifndef hud_h
#define hud_h 1

#include <vector>
#include <list>
#include <string>

using namespace std;


/// \brief Struct for scoreboard listings
struct fragsort_t
{
  int  count;
  int  num;
  int  color;
  const char *name;
};

void HU_DrawRanking(const char *title, int x, int y, fragsort_t *fragtable,
		    int scorelines, bool large, int white);

// Size of statusbar.
#define ST_HEIGHT_DOOM    32
#define ST_HEIGHT_HERETIC 42
#define ST_HEIGHT_HEXEN   65
#define ST_WIDTH         320


/// \brief Heads Up Display
///
/// This class handles everything that is drawn over the 3D view during the game,
/// such as the status bar. There is only one global instance in use, called "hud".
class HUD
{
protected:
  // statusbar data
  bool st_active;
  int  st_x, st_y;

  // what should we draw?
  bool statusbar_on;
  bool mainbar_on;
  bool invopen;
  bool drawscore; ///< should we draw frags instead of statusbar overlay?

  vector<class HudWidget *> statusbar; ///< status bar
  vector<HudWidget *> mainbar; ///< the part of the status bar that is hidden by open inventory
  vector<HudWidget *> overlay; ///< HUD overlay

  class PlayerPawn *sbpawn; ///< whose status is shown?
  // or maybe even Pawn*? See what a demon feels when shot!

  void PaletteFlash();

  void UpdateWidgets();
  void ST_updateFaceWidget();

  void ST_RefreshBackground();

  void ST_CreateWidgets();
  void CreateDoomWidgets();
  void CreateHereticWidgets();
  void CreateHexenWidgets();

public:
  void CreateOverlayWidgets();

  int  stbarheight;
  bool overlay_on;  ///< draw overlay instead of statusbar?
  bool st_refresh;  ///< the statusbar needs to be redrawn

  int itemuse;  ///< counter for Artifact Flashes

  int st_palette;  ///< current palette

  /// causes for palette flashes
  int poisoncount;
  int damagecount;
  int bonuscount;

  bool    chat_on; ///< player is currently typing a chat msg
protected:
  string  chat_msg;
  void SendChat();

  list<class HudTip*> tips; ///< scriptable HUD messages
  void DrawTips();

  vector<class HudPic*> pics; ///< scriptable HUD pictures
  void DrawPics();

public:
  HUD();

  void Startup();  // register HUD commands and consvars
  void Init();     // cache HUD data
  bool Responder(struct event_t* ev);
  void Ticker();
  void Draw(bool redrawsbar);

  void ST_Drawer(bool refresh);
  void ST_Start(PlayerPawn *p);
  void ST_Stop();

  void ST_Recalc(); // recalculates the status bar coordinates
  // (after changing the resolution or scaling, for example)

  void HU_Erase();

  int  GetFSPic(int lumpnum, int xpos, int ypos);
  bool DeleteFSPic(int handle);
  bool ModifyFSPic(int handle, int lumpnum, int xpos, int ypos);
  bool DisplayFSPic(int handle, bool newval);
};


extern HUD hud;

#endif
