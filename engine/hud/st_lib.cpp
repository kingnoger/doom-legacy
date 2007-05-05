// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Implementation of Hud Widgets

#include "doomdef.h"
#include "st_lib.h"

#include "hud.h"
#include "r_data.h"

#include "v_video.h"
#include "i_video.h"    //rendermode
#include "g_pawn.h"
#include "g_game.h"

#define BG 1
extern int fgbuffer; // contains both the buffer and the drawing flags (updated elsewhere)



HudWidget::HudWidget(int nx, int ny)
{
  x = nx;
  y = ny;
}


//===================================================================================

HudNumber::HudNumber(int nx, int ny, int dig, const int *number, Material **tex)
  : HudWidget(nx, ny)
{
  digits = dig;
  oldn = 0;
  n = number;
  nums = tex;
}


void HudNumber::Update(bool force)
{
  if (oldn != *n || force)
    Draw();
}


void HudNumber::Draw()
{
  int lnum = oldn = *n; // the number to be drawn
  int w = nums[0]->worldwidth;
  int h = nums[0]->worldheight;
  int dx = x - digits * w; // drawing x coord (right-aligned field!)

  // clear the area (right aligned field)
  // dont clear background in overlay
  if (!hud.overlay_on && rendermode == render_soft)
    V_CopyRect(dx, y, BG, w*digits, h, dx, y, fgbuffer);

  // if non-number, do not draw it
  if (lnum == 1994)
    return;

  bool neg = (lnum < 0);

  if (neg)
    {
      // INumber: if (val < -9) V_DrawScaledPatch(x+1, y+1, fgbuffer, W_CachePatchName("LAME", PU_CACHE));
      if (digits == 2 && lnum < -9)
	lnum = 9;
      else if (digits == 3 && lnum < -99)
	lnum = 99;
      else
	lnum = -lnum;
    }

  dx = x;

  // in the special case of 0, you draw 0
  if (lnum == 0)
    {
      nums[0]->Draw(dx - w, y, fgbuffer);
      return;
    }

  int dig = digits; // local copy
  // draw the new number
  while (lnum != 0 && dig--)
    {
      dx -= w;
      nums[lnum % 10]->Draw(dx, y, fgbuffer);
      lnum /= 10;
    }

  // draw a minus sign if necessary
  if (neg)
    nums[10]->Draw(dx - 8, y, fgbuffer);
}



//===================================================================================

HudPercent::HudPercent(int nx, int ny, const int *number, Material **tex, Material *percent)
  : HudNumber(nx, ny, 3, number, tex)
{
  pcent = percent;
}


void HudPercent::Update(bool force)
{
  // draw percent sign
  if (force)
    pcent->Draw(x, y, fgbuffer);
  // draw number
  if (oldn != *n || force)
    Draw();
}



//===================================================================================

HudMultIcon::HudMultIcon(int nx, int ny, const int *inumber, Material **tex)
  : HudWidget(nx, ny)
{
  oldinum = -1;
  inum = inumber;
  icons = tex;
}


void HudMultIcon::Update(bool force)
{
  if ((oldinum != *inum || force) && (*inum != -1))
    Draw();
}


void HudMultIcon::Draw()
{
  if ((oldinum != -1) && !hud.overlay_on && rendermode == render_soft && icons[oldinum])
    {
      // sw mode: background is not always fully redrawn
      int w, h;
      int dx, dy;
      dx = x - icons[oldinum]->leftoffs;
      dy = y - icons[oldinum]->topoffs;
      w = icons[oldinum]->worldwidth;
      h = icons[oldinum]->worldheight;

      V_CopyRect(dx, dy, BG, w, h, dx, dy, fgbuffer);
    }
  int i = *inum;
  if (i >= 0 && icons[i])
    icons[i]->Draw(x, y, fgbuffer);
  // FIXME! *inum might go beyond allowed limits!
  oldinum = i;
}



//===================================================================================

HudBinIcon::HudBinIcon(int nx, int ny, const bool *st, Material *p0, Material *p1)
  : HudWidget(nx, ny)
{
  oldstatus = false;
  status = st;
  icons[0] = p0;
  icons[1] = p1;
}


void HudBinIcon::Update(bool force)
{
  if (oldstatus != *status || force)
    Draw();
}


void HudBinIcon::Draw()
{
  oldstatus = *status;

  if (oldstatus == true)
    icons[1]->Draw(x, y, fgbuffer);
  else if (icons[0] != NULL)
    icons[0]->Draw(x, y, fgbuffer);
  else if (!hud.overlay_on && rendermode == render_soft)
    {
      int w, h;
      int dx, dy;
      dx = x - icons[1]->leftoffs;
      dy = y - icons[1]->topoffs;
      w = icons[1]->worldwidth;
      h = icons[1]->worldheight;

      V_CopyRect(dx, dy, BG, w, h, dx, dy, fgbuffer);
    }
}



//===================================================================================

HudSlider::HudSlider(int nx, int ny, const int *v, int mi, int ma, Material **t)
  : HudWidget(nx, ny)
{
  val = v;
  minval = mi;
  maxval = ma;
  oldval = cval = 0;
  tex = t;
}


void HudSlider::Update(bool force)
{
  // more like tick()!
  int i = *val; // marker targets here, moves slowly

  if (i < minval) i = minval;
  if (i > maxval) i = maxval;

  int delta;

  if (i > cval)
    {
      delta = (i - cval)>>2;
      if (delta < 1) delta = 1;
      else if (delta > 8) delta = 8;
      cval += delta;
    }
  else if (i < cval)
    {
      delta = (cval - i)>>2;
      if (delta < 1) delta = 1;
      else if (delta > 8) delta = 8;
      cval -= delta;
    }

  if (oldval != cval || force)
    Draw();
}

/*
TODO
static void ShadeLine(int x, int y, int height, int shade)
{
  byte *dest;
  byte *shades;
    
  shades = colormaps+9*256+shade*2*256;
  dest = vid.screens[0]+y*vid.width+x;
  while(height--)
    {
      *(dest) = *(shades+*dest);
      dest += vid.width;
    }
}
*/

/*
static void ShadeChain(int x, int y)
{
  int i;

  if (rendermode != render_soft)
    return;
    
  for(i = 0; i < 16*hud.st_scalex; i++)
    {
      ShadeLine((x+277)*hud.st_scalex+i, y*hud.st_scaley, 10*hud.st_scaley, i/4);
      ShadeLine((x+19)*hud.st_scalex+i, y*hud.st_scaley, 10*hud.st_scaley, 7-(i/4));
    }
}
*/

void HudSlider::Draw()
{
  oldval = cval;
  // Heretic: x = st_x, y = st_y+32
  // textures: chainback, chain, marker, ltface, rtface

  int pos = ((cval - minval)*256)/(maxval - minval);

  int by = 0;
  //int by = (cpos == CPawn->health) ? 0 : ChainWiggle;

  tex[0]->Draw(x, y, fgbuffer);
  tex[1]->Draw(x+2 + (pos % 17), y+1+by, fgbuffer);
  tex[2]->Draw(x+17+pos, y+1+by, fgbuffer);
  tex[3]->Draw(x, y, fgbuffer);
  tex[4]->Draw(x+276, y, fgbuffer);

  //ShadeChain(x, y);
}


void HexenHudSlider::Draw()
{
  oldval = cval;
  // Hexen: x = st_x, y = st_y+58
  // textures: NULL, chain, marker, ltface, rtface

  int pos = ((cval - minval)*220)/(maxval - minval);

  tex[1]->Draw(x+28 + (pos % 9), y, fgbuffer);
  tex[2]->Draw(x+7+pos, y, fgbuffer);
  tex[3]->Draw(x, y, fgbuffer);
  tex[4]->Draw(x+277, y, fgbuffer);
}


//===================================================================================

HudInventory::HudInventory(int nx, int ny, const bool *op, const int *iu, const inventory_t *vals,
			   const int *sel, Material **nn, Material **it, Material **t)
  : HudWidget(nx, ny)
{
  open = op;
  itemuse = iu;
  slots = vals;
  selected = sel;
  nums = nn;
  items = it;
  tex = t;
}


// draws up to 2 digits
void HudInventory::DrawNumber(int x, int y, int val)
{
  if (val == 1)
    return;

  int w = nums[0]->worldwidth; // was 4
    
  if (val > 9)
    nums[val/10]->Draw(x, y, fgbuffer);
  nums[val%10]->Draw(x+w, y, fgbuffer);
}



void HudInventory::Draw()
{
  int i;
  // selected (selectbox) refers to the same logical slot as invSlot
  // (it is the selected slot in the visible part of inventory (0-6))

  // heretic: x = st_x + 34, y = st_y + 1
  // two guiding bools: *open and overlay
  // textures: inv_background, artibox (also items[0]), selectbox,
  // 4 inv_gems, blacksq, 5 artiflash frames

  int sel = *selected;

  if (*open == true)
    {
      // open inventory
      // background (7 slots) (not for overlay!)
      tex[0]->Draw(x, y+1, fgbuffer);

      // draw items
      for (i = 0; i < 7; i++)
	{
	  // draw artibox
	  //tex[1]->Draw(x+16+i*31, y+1, fgbuffer);

	  if (slots[i].type != arti_none)
	    {
	      items[slots[i].type]->Draw(x+16+i*31, y+1, fgbuffer);
	      DrawNumber(x+35+i*31, y+23, slots[i].count);
	    }
	}

      // draw select box
      tex[2]->Draw(x+16 + sel*31, y+30, fgbuffer);

      // blinking arrowheads (using a hack slot. this is so embarassing.)
      if (slots[7].type)
	(!(game.tic&4) ? tex[3] : tex[4])->Draw(x+4, y, fgbuffer);

      if (slots[7].count)
	(!(game.tic&4) ? tex[5] : tex[6])->Draw(x+235, y, fgbuffer);
    }
  else
    {
      // closed inventory
      if (*itemuse > 0)
	{
	  // black square
	  tex[7]->Draw(x+146, y+2, fgbuffer);
	  // item use animation
	  tex[8 + (*itemuse) - 1]->Draw(x+148, y+2, fgbuffer);
	}
      else
	{
	  // just a single item in a box
	  //tex[1]->Draw(x+100, y, fgbuffer);
	  //tex[7]->Draw(x+146, y+2, fgbuffer);
	  if (slots[sel].type != arti_none)
	    {
	      items[slots[sel].type]->Draw(x+145, y+1, fgbuffer);
	      DrawNumber(x+145+22, y+23, slots[sel].count);
	    }
	}
    }
}


// Slightly different offsets from Heretic inventory
void HexenHudInventory::Draw()
{
  int i;
  // hexen: x = st_x + 38, y = st_y + 25

  int sel = *selected;

  if (*open == true)
    {
      // open inventory
      // background (7 slots) (not for overlay!)
      tex[0]->Draw(x, y+2, fgbuffer);

      // draw items
      for (i = 0; i < 7; i++)
	{
	  // draw artibox
	  //tex[1]->Draw(x+12+i*31, y, fgbuffer);

	  if (slots[i].type != arti_none)
	    {
	      items[slots[i].type]->Draw(x+12+i*31, y+3, fgbuffer);
	      DrawNumber(x+30+i*31, y+25, slots[i].count);
	    }
	}

      // draw select box
      tex[2]->Draw(x+12 + sel*31, y+3, fgbuffer);

      // blinking arrowheads (using a hack slot. this is so embarassing.)
      if (slots[7].type)
	(!(game.tic&4) ? tex[3] : tex[4])->Draw(x+4, y+3, fgbuffer);

      if (slots[7].count)
	(!(game.tic&4) ? tex[5] : tex[6])->Draw(x+231, y+3, fgbuffer);
    }
  else
    {
      // closed inventory
      if (*itemuse > 0)
	{
	  // black square
	  tex[7]->Draw(x+106, y, fgbuffer);
	  // item use animation
	  tex[8 + (*itemuse) - 1]->Draw(x+110, y+4, fgbuffer);
	}
      else
	{
	  // just a single item in a box
	  //tex[1]->Draw(x+100, y, fgbuffer);
	  //tex[7]->Draw(x+106, y, fgbuffer);
	  if (slots[sel].type != arti_none)
	    {
	      items[slots[sel].type]->Draw(x+105, y+3, fgbuffer);
	      DrawNumber(x+124, y+24, slots[sel].count);
	    }
	}
    }
}



void HudInventory::Update(bool force)
{
  Draw();
}
