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
// Revision 1.7  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.6  2003/04/24 20:30:23  hurdler
// Remove lots of compiling warnings
//
// Revision 1.5  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.4  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.3  2002/12/16 22:12:13  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:20:08  smite-meister
// HUD rationalized
//
//
// DESCRIPTION:
//  Implementation of Hud Widgets
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "st_lib.h"

#include "hu_stuff.h"
#include "v_video.h"
#include "r_data.h"

#include "i_video.h"    //rendermode
#include "g_pawn.h"

extern int fgbuffer; // in fact a HUD property, but...


//---------------------------------------------------------------------------
// was ST_drawOverlayNum: Draw a number fully, scaled, over the view
// was DrINumber: Draws a three digit number, left aligned, w = 9
// was DrBNumber: Draws positive left-aligned 3-digit number at x+6-w/2  (w = 12)
// right-aligned field!
void HudNumber::Draw()
{
  int lnum = oldnum = *num;
  bool neg = (lnum < 0);

  if (neg)
    {
      // INumber: if (val < -9) V_DrawScaledPatch(x+1, y+1, fgbuffer, W_CachePatchName("LAME", PU_CACHE));
      if (width == 2 && lnum < -9)
	lnum = -9;
      else if (width == 3 && lnum < -99)
	lnum = -99;
      
      lnum = -lnum;
    }

  int w = n[0]->width;
  int h = n[0]->height;
  int dx = x - width * w; // drawing x coord
  // clear the area (right aligned field)

#ifdef DEBUG
  CONS_Printf("V_CopyRect1: %d %d %d %d %d %d %d %d val: %d\n",
	      dx, y, BG, w*width, h, dx, y, fgbuffer, lnum);
#endif

  // dont clear background in overlay
  if (!hud.overlayon && rendermode == render_soft)
    V_CopyRect(dx, y, BG, w*width, h, dx, y, fgbuffer);

  // if non-number, do not draw it
  if (lnum == 1994)
    return;

  dx = x;

  // V_DrawScaledPatch(\([^,]+\),\([^,]+\),\([^,]+\),\([^,;]+\));$
  // \4->Draw(\1,\2,\3 | V_SCALE);

  // in the special case of 0, you draw 0
  if (lnum == 0)
    {
      // overlay: V_DrawScaledPatch(x - (w*vid.dupx), y, FG|V_NOSCALESTART, n[0]);
      n[0]->Draw(dx - w, y, fgbuffer | V_SCALE);
      return;
    }

  int digits = width; // local copy
  // draw the new number
  while (lnum != 0 && digits--)
    {
      dx -= w; // overlay:  x -= (w * vid.dupx);
      n[lnum % 10]->Draw(dx, y, fgbuffer | V_SCALE);
      lnum /= 10;
    }

  // draw a minus sign if necessary
  if (neg)
    n[10]->Draw(dx - 8, y, fgbuffer | V_SCALE);
  // overlay:  sttminus->Draw(x - (8*vid.dupx), y, FG|V_NOSCALESTART);
}


//---------------------------------------------------------------------------

void HudPercent::Update(bool force)
{
  if (*on == true)
    {
      // draw percent sign
      if (force)
	p->Draw(x, y, fgbuffer | V_SCALE);
      // draw number
      if (oldnum != *num || force)
	Draw();
    }
}

//---------------------------------------------------------------------------

void HudMultIcon::Update(bool force)
{
  if ((*on == true) && ((oldinum != *inum) || force) && (*inum != -1))
    Draw();
}

void HudMultIcon::Draw()
{
  if ((oldinum != -1) && !hud.overlayon && rendermode == render_soft && p[oldinum])
    {
      int w, h;
      int dx, dy;
      //faB:current hardware mode always refresh the statusbar
      // clear
      dx = x - p[oldinum]->leftoffset;
      dy = y - p[oldinum]->topoffset;
      w = p[oldinum]->width;
      h = p[oldinum]->height;

#ifdef DEBUG
      CONS_Printf("V_CopyRect2: %d %d %d %d %d %d %d %d\n",
		  dx, dy, BG, w, h, dx, dy, fgbuffer);
#endif
      V_CopyRect(dx, dy, BG, w, h, dx, dy, fgbuffer);
    }
  int i = *inum;
  if (i >= 0 && p[i])
    p[i]->Draw(x, y, fgbuffer | V_SCALE);
  // FIXME! *inum might go beyond allowed limits!
  oldinum = i;
}

//---------------------------------------------------------------------------

void HudBinIcon::Update(bool force)
{
  if ((*on == true) && (oldval != *val || force))
    Draw();
}

void HudBinIcon::Draw()
{
  oldval = *val;

  if (*val == true)
    p[1]->Draw(x, y, fgbuffer | V_SCALE);
  else if (p[0] != NULL)
    p[0]->Draw(x, y, fgbuffer | V_SCALE);
  else if (!hud.overlayon && rendermode == render_soft)
    {
      int w, h;
      int dx, dy;
      //faB:current hardware mode always refresh the statusbar
      // just clear
      dx = x; // - SHORT(p[1]->leftoffset); FIXME hud offsets
      dy = y; // - SHORT(p[1]->topoffset);
      w = p[1]->width;
      h = p[1]->height;

#ifdef DEBUG
      CONS_Printf("V_CopyRect3: %d %d %d %d %d %d %d %d\n",
		  dx, dy, BG, w, h, dx, dy, fgbuffer);
#endif

      V_CopyRect(dx, dy, BG, w, h, dx, dy, fgbuffer);
    }
}

//---------------------------------------------------------------------------

void HudSlider::Update(bool force)
{
  if (*on == false) return;

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

  if (oldval != cval || force) Draw();
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
  int by = 0;

  // patches in p: chainback, chain, marker, ltface, rtface
  // FIXME! use actual patch sizes below...

  int pos = ((cval-minval)*256)/(maxval-minval);

  //int by = (cpos == CPawn->health) ? 0 : ChainWiggle;
  p[0]->Draw(x, y, fgbuffer | V_SCALE);
  p[1]->Draw(x+2 + (pos%17), y+1+by, fgbuffer | V_SCALE);
  p[2]->Draw(x+17+pos, y+1+by, fgbuffer | V_SCALE);
  p[3]->Draw(x, y, fgbuffer | V_SCALE);
  p[4]->Draw(x+276, y, fgbuffer | V_SCALE);

  //ShadeChain(x, y);
}


//---------------------------------------------------------------------------

// was DrSmallNumber
// draws up to 2 digits

void HudInventory::DrawNumber(int x, int y, int val)
{
  if (val == 1)
    return;

  int w = n[0]->width; // was 4
    
  if (val > 9)
    n[val/10]->Draw(x, y, fgbuffer | V_SCALE);
  n[val%10]->Draw(x+w, y, fgbuffer | V_SCALE);
}

// was DrawInventoryBar

   
void HudInventory::Draw()
{
  extern int gametic; // blink
  int i;
  // selected (selectbox) refers to the same logical slot as invSlot
  // (it is the selected slot in the visible part of inventory (0-6))

  // x = st_x + 34, y = st_y + 1
  // two guiding bools: *open and overlay
  // patch order in p: inv_background, artibox (also items[0]), selectbox,
  // 4 inv_gems, blacksq, 5 artiflash frames

  int sel = *selected;

  if (*open == true)
    {
      // open inventory
      // background (7 slots) (not for overlay!)
      if (!overlay)
	p[0]->Draw(x, y+1, fgbuffer | V_SCALE);

      // draw stuff
      for(i = 0; i < 7; i++)
	{
	  if (overlay)
	    p[1]->Draw(x+16+i*31, y+1, fgbuffer | V_SLOC | V_TL); 
	  //V_DrawScaledPatch(x+16+i*31, y+1, 0, W_CachePatchName("ARTIBOX", PU_CACHE));
	  if (slots[i].type != arti_none)
	    {
	      items[slots[i].type]->Draw(x+16+i*31, y+1, fgbuffer | V_SCALE);
	      DrawNumber(x+35+i*31, y+23, slots[i].count);
	    }
	}

      // select box
      p[2]->Draw(x+16 + sel*31, y+30, fgbuffer | V_SCALE);

      // blinking arrowheads (using a hack slot. this is so embarassing.)
      if (slots[7].type)
	(!(gametic&4) ? p[3] : p[4])->Draw(x+4, y, fgbuffer | V_SCALE);

      if (slots[7].count)
	(!(gametic&4) ? p[5] : p[6])->Draw(x+235, y, fgbuffer | V_SCALE);
    }
  else
    {
      // closed inv.
      if (*itemuse > 0)
	{
	  p[7]->Draw(x, y, fgbuffer | V_SCALE);
	  p[8 + (*itemuse) - 1]->Draw(x, y, fgbuffer | V_SCALE);
	}
      else
	{
	  if (overlay)
	    p[1]->Draw(x+100, y, fgbuffer | V_SLOC | V_TL);
	  // p[7]->Draw(st_x+180, st_y+3, fgbuffer | V_SCALE);
	  if (slots[sel].type != arti_none)
	    {
	      items[slots[sel].type]->Draw(x+145, y, fgbuffer | V_SCALE);
	      DrawNumber(x+145+19, y+22, slots[sel].count);
	    }
	}
    }
}

void HudInventory::Update(bool force)
{
  if (*on == true)
    Draw();
  // && (oldval != *val || force)
}
