// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.6  2004/09/23 23:21:19  smite-meister
// HUD updated
//
// Revision 1.5  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.4  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.3  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.2  2002/12/03 10:23:46  smite-meister
// Video system overhaul
//
// Revision 1.1.1.1  2002/11/16 14:18:27  hurdler
// Initial C++ version of Doom Legacy
//-----------------------------------------------------------------------------

/// \file
/// \brief HUD widgets

#ifndef st_lib_h
#define st_lib_h 1


/// \brief ABC for HUD widgets
///
/// HUD widget classes currently include
/// number, percent, multicon, binicon, slider and inventory widgets.
class HudWidget
{
protected:
  int x, y; ///< screen coordinates
  //int type;    ///< overlay, shadowed, translucent...

  virtual void Draw() = 0; ///< pure virtual drawing routine

public:
  HudWidget(int x, int y);

  /// force: update even if no change
  virtual void Update(bool force)  { Draw(); };
};



/// \brief Number widget
///
/// Screen coordinates mark the upper right-hand corner of the number (right-justified).
/// Note! 1994 is a magic number representing a non-number value!
class HudNumber : public HudWidget
{
private:
  int digits;  ///< max # of digits in the number
  class Texture **nums; ///< graphics for numbers 0-9, nums[10] is the minus sign

protected:
  const int *n; ///< pointer to current value
  int     oldn; ///< last drawn value

  virtual void Draw();

public:
  HudNumber(int x, int y, int digits, const int *number, Texture **tex);

  virtual void Update(bool force);  
  void Set(const int *nnum) { n = nnum; }
};



/// \brief Percentile widget
class HudPercent : public HudNumber
{
private:
  Texture *pcent; ///< percent sign graphic

public:
  HudPercent(int x, int y, const int *number, Texture **tex, Texture *percent);
  virtual void Update(bool force);
};



/// \brief Multiple Icons widget
class HudMultIcon : public HudWidget
{
private:
  int     oldinum; ///< previous icon number
  const int *inum; ///< pointer to current icon number, -1 is a magic value meaning no icon is drawn
  Texture **icons; ///< array of icons

  virtual void Draw();

public:
  HudMultIcon(int x, int y, const int *inum, Texture **tex);
  virtual void Update(bool force);
};



/// \brief Binary Icon widget
class HudBinIcon : public HudWidget
{
private:
  // center-justified location of icon
  bool     oldstatus;  ///< last icon value
  const bool *status;  ///< pointer to current icon status
  Texture  *icons[2];  ///< The icons. If p[0] == NULL, draw background instead

  virtual void Draw();

public:
  HudBinIcon(int x, int y, const bool *status, Texture *p0, Texture *p1);
  virtual void Update(bool force);
};



/// \brief Horizontal "slow" slider widget (Heretic health bar etc.)
class HudSlider : public HudWidget
{
private:
  int minval, maxval; ///< min and max values
  int   oldval, cval; ///< old and current values
  const int     *val; ///< pointer to current value
  Texture      **tex; ///< Slider graphics. See the Draw() implementation.

  virtual void Draw();

public:
  HudSlider(int x, int y, const int *v, int mi, int ma, Texture **t);
  virtual void Update(bool force);
};



/// \brief Inventory widget
///
/// A set of 7 multicons plus a little extra.
class HudInventory : public HudWidget
{
private:
  const bool    *open; ///< show expanded inventory or just one slot?
  const int  *itemuse; ///< counter for the item use animation
  const struct inventory_t *slots;
  const int *selected; ///< selected slot number 0-6
  Texture **nums;  ///< small numbers 0-9
  Texture **items; ///< item graphics
  Texture **tex;   ///< inventory graphics

  virtual void Draw();
  void DrawNumber(int x, int y, int val);

public:
  HudInventory(int x, int y, const bool *op, const int *iu, const inventory_t *vals,
	       const int *sel, Texture **nn, Texture **it, Texture **t);
  virtual void Update(bool force);
};

#endif
