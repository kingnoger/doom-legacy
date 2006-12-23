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
/// \brief Texture blitting, blitting rectangles between buffers. Font system.

#include <ctype.h>

#include "doomdef.h"
#include "console.h"
#include "command.h"

#include "r_data.h"
#include "r_draw.h"

#include "v_video.h"
#include "screen.h"

#include "i_video.h"
#include "z_zone.h"

#ifndef NO_OPENGL
# include "hardware/hwr_render.h"
#endif

#include "hardware/oglrenderer.hpp"

using namespace std;

byte *current_colormap; // for applying colormaps to Drawn Textures


//=================================================================
//                    2D Drawing of Textures
//=================================================================

void PatchTexture::Draw(float x, float y, int scrn)
{
  int flags = scrn & V_FLAGMASK;
  scrn &= V_SCREENMASK;

  if (rendermode == render_opengl)
    {
      if (oglrenderer && oglrenderer->ReadyToDraw())
	// Console tries to use some patches before graphics are
	// initialized. If this is the case, then create the missing
	// texture.
	oglrenderer->Draw2DGraphic_Doom(x, y, this, flags);
      return;
    }

  byte *dest_tl = vid.screens[scrn]; // destination top left

  // location scaling
  if (flags & V_SLOC)
    {
      x *= vid.dupx;
      y *= vid.dupy;
      dest_tl += vid.scaledofs;
    }

  // size scaling
  fixed_t colfrac = xscale;
  fixed_t rowfrac = yscale;

  // clipping to LFB
  int x2, y2; // past-the-end coords for lower right corner

  if (flags & V_SSIZE)
    {
      x -= leftoffs * vid.dupx;
      y -= topoffs * vid.dupy;
      x2 = min(int(x + worldwidth * vid.dupx), vid.width);
      y2 = int(y + worldheight * vid.dupy); // TODO we do not clip y, just check it...
      colfrac /= vid.dupx;
      rowfrac /= vid.dupy;
    }
  else
    {
      x -= leftoffs;
      y -= topoffs;
      x2 = min(int(x + worldwidth), vid.width);
      y2 = int(y + worldheight);
    }

  // clipped left side
  int x1 = max(int(x), 0);
  int y1 = int(y);

#ifdef RANGECHECK
  if (y < 0 || y2 > vid.height)
    {
      fprintf(stderr, "Patch at (%f,%f) exceeds LFB.\n", x,y);
      // No I_Error abort - what is up with TNT.WAD?
      return;
    }
#endif

  dest_tl += y1*vid.width + x1; // top left corner
  byte *dest_tr = dest_tl + x2 - x1; // top right, past-the-end

  // starting location in texture space
  fixed_t col;
  if (flags & V_FLIPX)
    {
      col = (x2 - x - 1)*colfrac; // a little back from right edge
      colfrac = -colfrac; // reverse stride
    }
  else
    {
      col = (x1 - x)*colfrac;
    }

  patch_t *p = GeneratePatch();

  for ( ; dest_tl < dest_tr; col += colfrac, dest_tl++)
    {
      post_t *post = (post_t *)(patch_data + p->columnofs[col.floor()]);

      // step through the posts in a column
      while (post->topdelta != 0xff)
	{
	  // step through the posts in a column
	  byte *dest = dest_tl + (post->topdelta/rowfrac).floor()*vid.width;
	  byte *source = post->data;
	  int count = (post->length/rowfrac).floor();

	  fixed_t row = 0;
	  while (count--)
	    {
	      byte pixel = source[row.floor()];

	      // the compiler is supposed to optimize the ifs out of the loop
	      if (flags & V_MAP)
		pixel = current_colormap[pixel];

	      if (flags & V_TL)
		pixel = transtables[0][(pixel << 8) + *dest];

	      *dest = pixel;
	      dest += vid.width;
	      row += rowfrac;
	    }
	  post = (post_t *)&post->data[post->length + 1]; // next post
	}
    }
}



void LumpTexture::Draw(float x, float y, int scrn)
{
  int flags = scrn & V_FLAGMASK;
  scrn &= V_SCREENMASK;

  if (rendermode == render_opengl)
    {
      if (oglrenderer && oglrenderer->ReadyToDraw()) 
	oglrenderer->Draw2DGraphic_Doom(x, y, this, flags);
      return;
    }

  byte *dest_tl = vid.screens[scrn];

  // location scaling
  if (flags & V_SLOC)
    {
      x *= vid.dupx;
      y *= vid.dupy;
      dest_tl += vid.scaledofs;
    }

  // size scaling
  fixed_t colfrac = xscale;
  fixed_t rowfrac = yscale;

  // clipping to LFB
  int x2, y2; // clipped past-the-end coordinates of the lower right corner in LFB

  if (flags & V_SSIZE)
    {
      x -= leftoffs * vid.dupx;
      y -= topoffs * vid.dupy;
      x2 = min(int(x + worldwidth * vid.dupx), vid.width);
      y2 = min(int(y + worldheight * vid.dupy), vid.height);
      colfrac /= vid.dupx;
      rowfrac /= vid.dupy;
    }
  else
    {
      x -= leftoffs;
      y -= topoffs;
      x2 = min(int(x + worldwidth), vid.width);
      y2 = min(int(y + worldheight), vid.height);
    }

  // clipped upper left corner
  int x1 = max(int(x), 0);
  int y1 = max(int(y), 0);

  dest_tl += y1*vid.width + x1; // top left
  byte *dest_tr = dest_tl + x2 - x1; // top right, past-the-end

  // starting location in texture space
  fixed_t startrow = (y1 - y)*rowfrac;
  fixed_t col;
  if (flags & V_FLIPX)
    {
      col = (x2 - x - 1)*colfrac; // a little back from right edge
      colfrac = -colfrac; // reverse stride
    }
  else
    {
      col = (x1 - x)*colfrac;
    }

  int zzz = (y2-y1)*vid.width; // LFB offset from top to bottom, past-the-end
  byte *base = Generate(); // in col-major order!

  for ( ; dest_tl < dest_tr; col += colfrac, dest_tl++)
    {
      // LFB limits for the column
      byte *dest_end = dest_tl + zzz; // past-the-end
      byte *source = base + height * col.floor();

      fixed_t row = startrow;
      for (byte *dest = dest_tl; dest < dest_end; row += rowfrac, dest += vid.width)
	{
	  byte pixel = source[row.floor()];

	  // the compiler is supposed to optimize the ifs out of the loop
	  if (flags & V_MAP)
	    pixel = current_colormap[pixel];

	  if (flags & V_TL)
	    pixel = transtables[0][(pixel << 8) + *dest];

	  *dest = pixel;
	}
    }
}



// Fills a box of pixels using a flat texture as a pattern,
// scaled to screen size.
void LumpTexture::DrawFill(int x, int y, int w, int h)
{
  if(rendermode == render_opengl){
    if(oglrenderer && oglrenderer->ReadyToDraw()) 
      oglrenderer->Draw2DGraphicFill_Doom(x, y, w, h, this);
    return;
  }

  byte *flat = Generate(); // in col-major order
  byte *base_dest = vid.screens[0] + y*vid.dupy*vid.width + x*vid.dupx + vid.scaledofs;

  w *= vid.dupx;
  h *= vid.dupy;

  fixed_t dx = fixed_t(1) / vid.dupx;
  fixed_t dy = fixed_t(1) / vid.dupy;

  fixed_t xfrac = 0;
  for (int u=0; u<w; u++, xfrac += dx, base_dest++)
    {
      fixed_t yfrac = 0;
      byte *src = flat + (xfrac.floor() % width) * height;
      byte *dest = base_dest;
      for (int v=0; v<h; v++, yfrac += dy)
        {
          *dest = src[yfrac.floor() % height];
	  dest += vid.width;
        }
    }
}



//=================================================================
//                         Blitting
//=================================================================

/// Copies a rectangular area from one screen buffer to another
void V_CopyRect(int srcx, int srcy, int srcscrn,
                int width, int height,
                int destx, int desty, int destscrn)
{
  // WARNING don't mix
  if ((srcscrn & V_SLOC) || (destscrn & V_SLOC))
    {
      srcx*=vid.dupx;
      srcy*=vid.dupy;
      width*=vid.dupx;
      height*=vid.dupy;
      destx*=vid.dupx;
      desty*=vid.dupy;
    }
  srcscrn &= 0xffff;
  destscrn &= 0xffff;

#ifdef RANGECHECK
  if (srcx<0
      ||srcx+width >vid.width
      || srcy<0
      || srcy+height>vid.height
      ||destx<0||destx+width >vid.width
      || desty<0
      || desty+height>vid.height
      || (unsigned)srcscrn>4
      || (unsigned)destscrn>4)
    I_Error ("Bad V_CopyRect %d %d %d %d %d %d %d %d", srcx, srcy,
             srcscrn, width, height, destx, desty, destscrn);
#endif

  byte *src = vid.screens[srcscrn]+vid.width*srcy+srcx;
  byte *dest = vid.screens[destscrn]+vid.width*desty+destx;

  for (; height>0 ; height--)
    {
      memcpy (dest, src, width);
      src += vid.width;
      dest += vid.width;
    }
}


// Copy a rectangular area from one bitmap to another (8bpp)
void VID_BlitLinearScreen(byte *src, byte *dest, int width, int height,
                          int srcrowbytes, int destrowbytes)
{
  if (srcrowbytes == destrowbytes)
    memcpy(dest, src, srcrowbytes * height);
  else
    {
      while (height--)
        {
          memcpy(dest, src, width);

          dest += destrowbytes;
          src += srcrowbytes;
        }
    }
}





//======================================================================
//                     Misc. drawing stuff
//======================================================================


//  Fills a box of pixels with a single color, NOTE: scaled to screen size
void V_DrawFill(int x, int y, int w, int h, int c)
{
#ifndef NO_OPENGL
  if (rendermode != render_soft)
    {
      HWR.DrawFill(x, y, w, h, c);
      return;
    }
#endif

  byte *dest = vid.screens[0] + y*vid.dupy*vid.width + x*vid.dupx + vid.scaledofs;

  w *= vid.dupx;
  h *= vid.dupy;

  for (int v=0 ; v<h ; v++, dest += vid.width)
    for (int u=0 ; u<w ; u++)
      dest[u] = c;
}




//
//  Fade all the screen buffer, so that the menu is more readable,
//  especially now that we use the small hufont in the menus...
//
void V_DrawFadeScreen()
{
#ifndef NO_OPENGL
  if (rendermode != render_soft)
    {
      HWR.FadeScreenMenuBack(0x01010160, 0);  //faB: hack, 0 means full height :o
      return;
    }
#endif

  int w = vid.width>>2;

  byte  p1, p2, p3, p4;
  byte *fadetable = R_GetFadetable(0)->colormap + 16*256;

  for (int y=0 ; y<vid.height ; y++)
    {
      int *buf = (int *)(vid.screens[0] + vid.width*y);
      for (int x=0 ; x<w ; x++)
        {
          int quad = buf[x];
          p1 = fadetable[quad&255];
          p2 = fadetable[(quad>>8)&255];
          p3 = fadetable[(quad>>16)&255];
          p4 = fadetable[quad>>24];
          buf[x] = (p4<<24) | (p3<<16) | (p2<<8) | p1;
        }
    }

#ifdef _16bitcrapneverfinished
    else
    {
        w = vid.width;
        for (y=0 ; y<vid.height ; y++)
        {
            wput = (short*) (vid.screens[0] + vid.width*y);
            for (x=0 ; x<w ; x++)
            {
                *wput = (*wput>>1) & 0x3def;
                wput++;
            }
        }
    }
#endif
}


/// Simple translucence with one color, coords are true LFB coords
void V_DrawFadeConsBack(int x1, int y1, int x2, int y2)
{
#ifndef NO_OPENGL
  if (rendermode!=render_soft)
    {
      HWR.FadeScreenMenuBack(0x00500000, y2);
      return;
    }
#endif

  if (vid.BytesPerPixel == 1)
    {
      for (int y=y1; y<y2; y++)
        {
	  byte *buf = vid.screens[0] + vid.width*y;
          for (int x=x1 ; x<x2 ; x++)
            buf[x] = greenmap[buf[x]];
        }
    }
  else
    {
      int w = x2-x1;
      for (int y=y1 ; y<y2 ; y++)
        {
	  short *wput = (short*)(vid.screens[0] + vid.width*y) + x1;
          for (int x=0 ; x<w ; x++)
	    {
	      *wput = ((*wput&0x7bde) + (15<<5)) >>1;
	      wput++;
	    }
        }
    }
}


//========================================================================
//                           Font system
//========================================================================

#define HU_FONTSTART    '!'     // the first font character
#define HU_FONTEND      '_'     // the last font character
#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1) // default font size

// Doom:
// STCFN033-95 + 121 : small red font

// Heretic:
// FONTA01-59 : medium silver font
// FONTB01-58 : large green font, some symbols empty

// Hexen:
// FONTA01-59 : medium silver font
// FONTAY01-59 : like FONTA but yellow
// FONTB01-58 : large red font, some symbols empty

font_t *hud_font;
font_t *big_font; // TODO used width-1 instead of width...


font_t::font_t(int startlump, int endlump, char firstchar)
{
  if (startlump < 0 || endlump < 0)
    I_Error("Incomplete font!\n");

  int truesize = endlump - startlump + 1; // we have this many lumps
  char lastchar = firstchar + truesize - 1;

  // the font range must include '!' and '_'. We will duplicate letters if need be.
  start = min(firstchar, '!');
  end = max(lastchar, '_');
  int size = end - start + 1;

  font = (Texture **)Z_Malloc(size*sizeof(Texture*), PU_STATIC, NULL);

  for (int i = start; i <= end; i++)
    if (i < firstchar || i > lastchar)
      // replace the missing letters with the first char
      font[i - start] = tc.GetPtrNum(startlump);
    else
      font[i - start] = tc.GetPtrNum(i - firstchar + startlump);

  // use the character '0' as a "prototype" for the font
  if (start <= '0' && '0' <= end)
    {
      height = font['0' - start]->worldheight;
      width = font['0' - start]->worldwidth;
    }
  else
    {
      height = font[0]->worldheight;
      width = font[0]->worldwidth;
    }
}


// Writes a single character (draw WHITE if bit 7 set)
void font_t::DrawCharacter(float x, float y, char c, int flags)
{
  if (c & 0x80)
    {
      // special "white" property used by console
      flags |= V_MAP;
      current_colormap = whitemap;
      c &= 0x7F;
    }

  c = toupper(c);
  if (c < start || c > end)
    return;

  Texture *t = font[c - start];
  t->Draw(x, y, flags);
}



//  Draw a string using the font
//  NOTE: the text is centered for screens larger than the base width
void font_t::DrawString(float x, float y, const char *str, int flags)
{
  if (flags & V_WHITEMAP)
    {
      current_colormap = whitemap;
      flags |= V_MAP;
    }

  float dupx = 1;
  float dupy = 1;

  if (rendermode == render_opengl)
    {
      // TODO damn scaledofs
      if (flags & V_SSIZE)
	{
	  dupx = vid.fdupx;
	  dupy = vid.fdupy;
	}

      if (flags & V_SLOC)
	{
	  x *= vid.fdupx;
	  y *= vid.fdupy;
	  flags &= ~V_SLOC; // not passed on to Texture::Draw
	}
    }
  else
    {
      if (flags & V_SSIZE)
	{
	  dupx = vid.dupx;
	  dupy = vid.dupy;
	}

      if (flags & V_SLOC)
	{
	  x *= vid.dupx;
	  y *= vid.dupy;
	  flags &= ~V_SLOC; // not passed on to Texture::Draw
	}

      // unravel scaledofs
      x += vid.scaledofs % vid.width;
      y += vid.scaledofs / vid.width;
    }

  // cursor coordinates
  float cx = x;
  float cy = y;
  float rowheight = (height + 1) * dupy;

  while (1)
    {
      int c = *str++;
      if (!c)
        break;

      if (c == '\n')
        {
          cx = x;
	  cy += rowheight;
          continue;
        }

      c = toupper(c);
      if (c < start || c > end)
        {
          cx += 4*dupx;
          continue;
        }

      Texture *t = font[c - start];
      t->Draw(cx, cy, flags);

      cx += t->worldwidth * dupx;
    }
}


// returns the width of the string (unscaled)
float font_t::StringWidth(const char *str)
{
  float w = 0;

  for (int i = 0; str[i]; i++)
    {
      int c = toupper(str[i]);
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->worldwidth;
    }

  return w;
}


// returns the width of the next n chars of str
float font_t::StringWidth(const char *str, int n)
{
  float w = 0;

  for (int i = 0; i<n && str[i]; i++)
    {
      int c = toupper(str[i]);
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->worldwidth;
    }

  return w;
}


float font_t::StringHeight(const char *str)
{
  return height;
}
