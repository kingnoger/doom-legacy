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
// Revision 1.1  2002/11/16 14:18:27  hurdler
// Initial revision
//
// Revision 1.11  2002/09/08 14:38:09  vberghol
// Now it works! Sorta.
//
// Revision 1.10  2002/09/05 14:12:18  vberghol
// network code partly bypassed
//
// Revision 1.9  2002/08/24 11:57:28  vberghol
// d_main.cpp is better
//
// Revision 1.8  2002/08/23 19:37:37  jpakkane
// Fix Linux compilation error with virtual functions.
//
// Revision 1.7  2002/08/21 16:58:36  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.6  2002/08/19 18:06:44  vberghol
// renderer somewhat fixed
//
// Revision 1.5  2002/08/02 20:14:53  vberghol
// p_enemy.cpp done!
//
// Revision 1.4  2002/07/01 21:00:56  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:59  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.3  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      The status bar widget code.
//
//-----------------------------------------------------------------------------

#ifndef st_lib_h
#define st_lib_h 1

struct patch_t;
class PlayerPawn; // temporary!!!FIXME

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
  //friend class HUD;
protected:
  //  HUD *h;
  int x, y; // screen coordinates
  //int data; // internal data, methods may use it to communicate. A bit dirty.
  const bool *on; // pointer to boolean stating whether to update widget

  virtual void Draw() = 0; // pure virtual drawing routine

public:
  //int data; // user data
  // constructor
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
  patch_t **n; // array of patches for numbers 0-9, n[10] is minus sign
  int type; // overlay, shadowed, translucent...
protected:
  const int *num; // pointer to current value
  int oldnum;     // last number value

  void Draw();

public:

  HudNumber(int x, int y, const bool *on, int nwidth, const int *nnum, patch_t **pl)
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


// Percentile widget ("child" of number widget)
class HudPercent : public HudNumber
{
private:
  patch_t *p; // percent sign graphic

public:
  HudPercent(int x, int y, const bool *on, const int *nnum, patch_t** pl, patch_t* percent)
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
  // center-justified location of icons
  // *inum == -1 is a magic value meaning no icon is drawn
  int oldinum; // last icon number
  const int *inum;   // pointer to current icon number
  patch_t **p; // list of icons

  void Draw();

public:
  //void STlib_initMultIcon(st_multicon_t* mi, int x, int y, patch_t** il, int* inum, bool* on);
  HudMultIcon(int x, int y, const bool *on, const int *ninum, patch_t **np) : HudWidget(x, y, on)
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
  bool oldval; // last icon value
  const bool *val;  // pointer to current icon status
  patch_t *p[2]; // icon. If p[0] == NULL, draw background instead

  void Draw();

public:
  //void STlib_initBinIcon(st_binicon_t* b, int x, int y, patch_t* i, bool* val, bool* on);
  HudBinIcon(int x, int y, const bool *on, const bool *nval,
	     patch_t *p0, patch_t *p1) : HudWidget(x, y, on)
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
  patch_t **p; // patches array. see Draw() implementation.

  void Draw();

public:

  HudSlider(int x, int y, const bool *on, const int *v, int mi, int ma, patch_t **np)
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

// inventory

class HudInventory : public HudWidget
{
private:
  const bool *open; // show expanded inventory or just one slot?
  int  *itemuse; // counter for the flash...
  bool overlay;  // overlaid or normal?
  int nslots;    // visible slots
  patch_t **n;     // small numbers 0-9
  patch_t **items; // item pictures
  patch_t **p;     // array of patches
  const PlayerPawn *pawn; // temp trick

  void Draw();
  void DrawNumber(int x, int y, int val);

public:
  HudInventory(int x, int y, const bool *on, const bool *op, int *iu, bool over, int ns,
	       patch_t **nn, patch_t **ni, patch_t **np, const PlayerPawn *npa)
    : HudWidget(x, y, on)
  {
    open = op;
    itemuse = iu;
    overlay = over;
    nslots = ns;
    n = nn;
    items = ni;
    p = np;
    pawn = npa;
  }
  void Update(bool force);
};

#endif
