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
// Revision 1.4  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.3  2004/07/25 20:18:47  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.2  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.1.1.1  2002/11/16 14:18:20  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Console interface

#ifndef console_h
#define console_h 1

#include "doomtype.h"


/// \brief Console
///
/// Console serves as an user interface to the command buffer.
/// It gathers input, keeps track of command history and of course
/// draws the messages on screen.
/// There is only one global instance in use, called "con".

class Console
{
public:
  bool refresh; ///< explicitly refresh screen after each CONS_Printf (game is not yet in display loop)
  bool recalc;  ///< set true when screen size has changed. on next tick, console will conform

protected:
  bool graphic; ///< console can be drawn
  bool active;  ///< console is active (accepting input)

  int con_tick; ///< console ticker for anim of blinking prompt cursor


  // console input
#define  CON_MAXPROMPTCHARS 256
  char inputlines[32][CON_MAXPROMPTCHARS]; // hold last 32 prompt lines for history
  int  input_cx;       // position in current input line
  int  input_cy;      // current input line number
  int  input_hist;      // line number of history input line to restore


  // console output
  char *con_buffer; ///< wrapping buffer that stores formatted console output
  char *con_line;   ///< pointer to current output line

  int con_cols;     ///< console text buffer width in columns
  int con_lines;    ///< console text buffer height in rows

  int con_cx;       ///< cursor position in current line (column number)
  int con_cy;       ///< cursor line number in con_buffer, wraps around using modulo
  int con_scrollup; ///< how many rows of text to scroll up (pgup/pgdn)


  // hud lines, used when console is closed
#define  CON_MAXHUDLINES 5
  int con_hudlines;                   ///< number of console heads up message lines
  int con_lineowner[CON_MAXHUDLINES]; ///< In splitscreen, which player gets this line of text
  int con_hudtime[CON_MAXHUDLINES];   ///< remaining time of display for hud msg lines

  // graphics
  class Texture *con_backpic; ///< console background picture, loaded static
  Texture *con_lborder, *con_rborder; ///< console borders in translucent mode

  int con_destheight; ///< destination height in pixels
  int con_height;     ///< current console height in pixels


protected:
  /// called after vidmode has changed
  void RecalcSize();

  void DrawConsole();
  void DrawHudlines();

  void Print(char *msg);
  void Linefeed(int player);

public:
  Console();
  ~Console();

  /// client init
  void Init();

  /// change console state (on/off)
  void Toggle(bool forceoff = false);

  /// clear the output buffer
  void Clear();

  /// clears HUD messages
  void ClearHUD();

  /// event responder
  bool Responder(struct event_t *ev);

  /// animation, message timers
  void Ticker();

  /// draws the console
  void Drawer();

  /// wrapper
  friend void CONS_Printf(char *fmt, ...);
};


void CONS_Error(char *msg); // print out error msg, wait for keypress


extern Console con;


extern int con_keymap;

// character input
extern char *shiftxform; // SHIFT-keycode to ASCII mapping
char KeyTranslation(unsigned char ch); // keycode to ASCII mapping


// top clip value for view render: do not draw part of view hidden by console
extern int  con_clipviewtop;
extern int  con_clearlines;  // lines of top of screen to refresh
extern bool con_hudupdate;   // hud messages have changed, need refresh



// globally used colormaps
extern byte *whitemap;
extern byte *greenmap;
extern byte *graymap;


#endif
