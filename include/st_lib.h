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
//
//
// DESCRIPTION:
//      The status bar widget code.
//
//-----------------------------------------------------------------------------

#ifndef st_lib_h
#define st_lib_h 1

//
// Background and foreground screen numbers
//
#define BG 1
#define FG 0


// widget classes
//
// number, percent, multicon, binicon, slider, inventory


class HudWidget
{
protected:
  int       x, y; // screen coordinates
  const bool *on; // pointer to boolean stating whether to update widget

  virtual void Draw() = 0; // pure virtual drawing routine

public:
  HudWidget(int nx, int ny, const bool *non)
  {
    x = nx;
    y = ny;
    on = non;
  };

  // force: update even if no change
  virtual void Update(bool force)
  {
    if (*on == true) Draw();
  };
};



// Number widget
class HudNumber : public HudWidget
{
private:
  // coordinates: upper right-hand corner of the number (right-justified)
  // note! 1994 is a magic number representing a non-number value!

  int  width;  // max # of digits in number
  class Texture **n; // array of patches for numbers 0-9, n[10] is minus sign
  int type;    // overlay, shadowed, translucent...
protected:
  const int *num; // pointer to current value
  int oldnum;     // last number value

  void Draw();

public:

  HudNumber(int x, int y, const bool *on, int nwidth, const int *nnum, Texture **pl)
    : HudWidget(x, y, on)
  {
    width = nwidth;
    oldnum = 0;
    num = nnum;
    n = pl;
  };
  
  void Set(const int *ns)
  {
    num = ns;
  }

  void Update(bool force)
  {
    if ((*on == true) && (oldnum != *num || force)) Draw();
  }
};


// Percentile widget
class HudPercent : public HudNumber
{
private:
  Texture *p; // percent sign graphic

public:
  HudPercent(int x, int y, const bool *on, const int *nnum, Texture** pl, Texture* percent)
    : HudNumber(x, y, on, 3, nnum, pl)
  {
    p = percent;
  };

  void Update(bool force);
};


// Multiple Icon widget
class HudMultIcon : public HudWidget
{
private:
  int     oldinum; // last icon number
  const int *inum; // pointer to current icon number, -1 is a magic value meaning no icon is drawn
  Texture     **p; // list of icons

  void Draw();

public:
  HudMultIcon(int x, int y, const bool *on, const int *ninum, Texture **np) : HudWidget(x, y, on)
  {
    oldinum = -1;
    inum = ninum;
    p = np;
  }

  void Update(bool force);
};


// Binary Icon widget
class HudBinIcon : public HudWidget
{
private:
  // center-justified location of icon
  bool     oldval;  // last icon value
  const bool *val;  // pointer to current icon status
  Texture   *p[2];  // icon. If p[0] == NULL, draw background instead

  void Draw();

public:
  HudBinIcon(int x, int y, const bool *on, const bool *nval,
	     Texture *p0, Texture *p1) : HudWidget(x, y, on)
  {
    oldval = false;
    val = nval;
    p[0] = p0;
    p[1] = p1;
  };

  void Update(bool force);
};


// Horizontal "slow" slider (like Heretic health bar)
class HudSlider : public HudWidget
{
private:
  int minval, maxval; // min and max values
  int oldval, cval; // old, current shown value
  const int* val;   // pointer to current value
  Texture **p; // patches array. see Draw() implementation.

  void Draw();

public:

  HudSlider(int x, int y, const bool *on, const int *v, int mi, int ma, Texture **np)
    : HudWidget(x, y, on)
  {
    val = v;
    minval = mi;
    maxval = ma;
    oldval = cval = 0;
    p = np;
  }

  void Update(bool force);
};


// inventory == a set of 7 multicons plus a little extra
class HudInventory : public HudWidget
{
private:
  const bool *open; // show expanded inventory or just one slot?
  const int  *itemuse; // counter for the item use anim
  const struct inventory_t *slots;
  const int  *selected; // selected slot number 0-6
  bool overlay;  // overlaid or normal?
  Texture **n;     // small numbers 0-9
  Texture **items; // item pictures
  Texture **p;     // array of patches

  void Draw();
  void DrawNumber(int x, int y, int val);

public:
  HudInventory(int x, int y, const bool *on, const bool *op, const int *iu, const inventory_t *vals,
	       const int *sel, bool over, Texture **nn, Texture **ni, Texture **np)
    : HudWidget(x, y, on)
  {
    open = op;
    itemuse = iu;
    slots = vals;
    selected = sel;
    overlay = over;
    n = nn;
    items = ni;
    p = np;
  }
  void Update(bool force);
};

#endif
