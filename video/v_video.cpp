// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.28  2005/10/05 17:25:53  smite-meister
// texturecache fix
//
// Revision 1.27  2005/09/12 18:33:45  smite-meister
// fixed_t, vec_t
//
// Revision 1.26  2005/07/20 20:27:23  smite-meister
// adv. texture cache
//
// Revision 1.25  2005/06/28 18:11:42  smite-meister
// fixed menu scaling
//
// Revision 1.24  2005/04/19 18:28:43  smite-meister
// new RPCs
//
// Revision 1.23  2005/04/05 16:15:13  smite-meister
// valgrind fixes
//
// Revision 1.19  2004/11/28 18:02:25  smite-meister
// RPCs finally work!
//
// Revision 1.18  2004/11/13 22:39:00  smite-meister
// intermission works
//
// Revision 1.17  2004/11/09 20:38:53  smite-meister
// added packing to I/O structs
//
// Revision 1.15  2004/10/31 22:22:13  smite-meister
// Hasta la vista, pic_t!
//
// Revision 1.14  2004/10/27 17:37:11  smite-meister
// netcode update
//
// Revision 1.13  2004/10/14 19:35:52  smite-meister
// automap, bbox_t
//
// Revision 1.12  2004/09/23 23:21:20  smite-meister
// HUD updated
//
// Revision 1.11  2004/09/14 21:41:57  hurdler
// rename "data" to "pixels" (I think it's more appropriate and that's how SDL and OpenGL name such data after all)
//
// Revision 1.10  2004/08/13 18:25:11  smite-meister
// sw renderer fix
//
// Revision 1.7  2004/07/25 20:16:43  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.4  2004/01/10 16:03:00  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.3  2002/12/16 22:22:05  smite-meister
// Actor/DActor separation
//
// Revision 1.2  2002/12/03 10:07:13  smite-meister
// Video unit overhaul begins
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Texture blitting, blitting rectangles between buffers. Font system.

#include <ctype.h>

#include "doomdef.h"
#include "console.h"
#include "command.h"

#include "r_data.h"
#include "r_main.h"
#include "r_draw.h"

#include "v_video.h"
#include "screen.h"

#include "i_video.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hwr_render.h"
#endif

using namespace std;

byte *current_colormap; // for applying colormaps to Drawn Textures

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



//=================================================================
//                    2D Drawing of Textures
//=================================================================

void PatchTexture::Draw(int x, int y, int scrn = 0)
{
  int flags = scrn & V_FLAGMASK;
  scrn &= V_SCREENMASK;

#ifdef HWRENDER
  // draw an hardware converted patch
  if (rendermode != render_soft)
    {
      HWR_Draw(x, y, flags);
      return;
    }
#endif

  byte *desttop = vid.screens[scrn];

  // scaling
  if (flags & V_SLOC)
    {
      x *= vid.dupx;
      y *= vid.dupy;
      desttop += vid.scaledofs;
    }

  if (flags & V_SSIZE)
    {
      x -= leftoffset * vid.dupx;
      y -= topoffset * vid.dupy;
    }
  else
    {
      x -= leftoffset;
      y -= topoffset;
    }

#ifdef RANGECHECK
  if (x<0 || x + width > vid.width || y<0 || y + height > vid.height || scrn > 4)
    {
      fprintf(stderr, "Patch at %d,%d exceeds LFB\n", x,y);
      // No I_Error abort - what is up with TNT.WAD?
      fprintf(stderr, "V_DrawPatch: bad patch (ignored)\n");
      return;
    }
#endif

  desttop += y * vid.width + x;

  byte *destend;
  fixed_t rowfrac = 1, colfrac = 1;
  fixed_t col = 0;
  int icol, idelta;
  if (flags & V_SSIZE)
    {
      colfrac /= vid.dupx;
      rowfrac /= vid.dupy;
      destend = desttop + width*vid.dupx;

      if (flags & V_FLIPX)
        {
          colfrac = -colfrac;
          col = colfrac + width;
        }
    }
  else
    {
      icol = 0;
      idelta = 1;

      destend = desttop + width;

      if (flags & V_FLIPX)
        {
          idelta = -1;
          icol = width - 1;
        }
    }

  patch_t *p = GeneratePatch();

  if (flags & V_SSIZE)
    for ( ; desttop < destend; col += colfrac, desttop++)
      {
        post_t *post = (post_t *)(patch_data + p->columnofs[col.floor()]);

        // step through the posts in a column
        while (post->topdelta != 0xff)
          {
            byte *source = post->data;
            byte *dest   = desttop + post->topdelta*vid.dupy*vid.width;
            int count  = post->length*vid.dupy;

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
  else // unscaled, perhaps a bit faster?
    for ( ; desttop < destend; icol += idelta, desttop++)
      {
        post_t *post = (post_t *)(patch_data + p->columnofs[icol]);

        // step through the posts in a column
        while (post->topdelta != 0xff)
          {
            byte *source = post->data;
            byte *dest = desttop + post->topdelta*vid.width;
            int count = post->length;

            while (count--)
              {
                byte pixel = *source++;

                // the compiler is supposed to optimize the ifs out of the loop
                if (flags & V_MAP)
                  pixel = current_colormap[pixel];

                if (flags & V_TL)
                  pixel = transtables[0][(pixel << 8) + *dest];

                *dest = pixel;
                dest += vid.width;
              }
            post = (post_t *)&post->data[post->length + 1];
          }
      }
}



void LumpTexture::Draw(int x, int y, int scrn = 0)
{
  int flags = scrn & V_FLAGMASK;
  scrn &= V_SCREENMASK;

#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR_Draw(x, y, flags);
      return;
    }
#endif

  byte *dest_left = vid.screens[scrn];

  // location scaling
  if (flags & V_SLOC)
    {
      x *= vid.dupx;
      y *= vid.dupy;
      dest_left += vid.scaledofs;
    }

  // size scaling
  fixed_t rowfrac = xscale;
  fixed_t colfrac = yscale;

  // visible (LFB) width after scaling
  int vis_width  = int(width  / xscale.Float());
  int vis_height = int(height / yscale.Float());

  // clipping to LFB
  int x2, y2; // clipped past-the-end coordinates of the lower right corner in LFB

  if (flags & V_SSIZE)
    {
      x -= leftoffset * vid.dupx;
      y -= topoffset * vid.dupy;
      x2 = min(x + vis_width*vid.dupx, vid.width);
      y2 = min(y + vis_height*vid.dupy, vid.height);
      colfrac /= vid.dupx;
      rowfrac /= vid.dupy;
    }
  else
    {
      x -= leftoffset;
      y -= topoffset;
      x2 = min(x + vis_width, vid.width);
      y2 = min(y + vis_height, vid.height);
    }

  // clipped upper left corner
  int x1 = max(x, 0);
  int y1 = max(y, 0);

  // starting location in texture space
  fixed_t startcol = (x1 - x)*xscale;
  fixed_t startrow = (y1 - y)*yscale;

  if (flags & V_SSIZE)
    {
      startcol /= vid.dupx;
      startrow /= vid.dupy;
    }

  dest_left += y1*vid.width + x1; // top left
  byte *dest_end = dest_left + (y2-y1)*vid.width; // lower left, past-the-end

  byte *base = Generate();

  if (flags & V_SSIZE)
    for (fixed_t row = startrow; dest_left < dest_end; row += rowfrac, dest_left += vid.width)
      {
	byte *source = base + width*(row.floor());
	byte *dest_right = dest_left + x2 - x1; // past-the-end
	fixed_t col = startcol;
	for (byte *dest = dest_left; dest < dest_right; col += colfrac, dest++)
	  {
	    byte pixel = source[col.floor()];

	    // the compiler is supposed to optimize the ifs out of the loop
	    if (flags & V_MAP)
	      pixel = current_colormap[pixel];

	    if (flags & V_TL)
	      pixel = transtables[0][(pixel << 8) + *dest];

	    *dest = pixel;
	  }
      }
  else // unscaled, perhaps a bit faster?
    for (int row = x1-x; dest_left < dest_end; row++, dest_left += vid.width)
      {
	byte *dest_right = dest_left + x2 - x1;
	int col = y1-y;
	for (byte *dest = dest_left; dest < dest_right; col++, dest++)
	  {
	    byte pixel = base[col];

	    // the compiler is supposed to optimize the ifs out of the loop
	    if (flags & V_MAP)
	      pixel = current_colormap[pixel];

	    if (flags & V_TL)
	      pixel = transtables[0][(pixel << 8) + *dest];

	    *dest = pixel;
	  }
        base += width;
      }
}



// Fills a box of pixels using a flat texture as a pattern,
// scaled to screen size.
void LumpTexture::DrawFill(int x, int y, int w, int h)
{
#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR.DrawFill(x, y, w, h, this);
      return;
    }
#endif

  byte *flat = Generate();
  byte *dest = vid.screens[0] + y*vid.dupy*vid.width + x*vid.dupx + vid.scaledofs;

  w *= vid.dupx;
  h *= vid.dupy;

  fixed_t dx = fixed_t(1) / vid.dupx;
  fixed_t dy = fixed_t(1) / vid.dupy;

  fixed_t yfrac = 0;
  for (int v=0; v<h; v++, dest += vid.width)
    {
      fixed_t xfrac = 0;
      byte *src = flat + ((yfrac.floor() % height) * width);
      for (int u=0; u<w; u++)
        {
          dest[u] = src[xfrac.floor() % width];
          xfrac += dx;
        }
      yfrac += dy;
    }
}




//======================================================================
//                     Misc. drawing stuff
//======================================================================

// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src)
{
#ifdef RANGECHECK
  if (x<0 ||x+width >vid.width || y<0 || y+height>vid.height || scrn > 4)
    I_Error ("Bad V_DrawBlock");
#endif

  byte *dest = vid.screens[scrn] + y*vid.width + x;

  while (height--)
    {
      memcpy(dest, src, width);

      src += width;
      dest += vid.width;
    }
}



// Gets a linear block of pixels from the view buffer.
void V_GetBlock(int x, int y, int scrn, int width, int height, byte *dest)
{
  if (rendermode!=render_soft)
    I_Error ("V_GetBlock: called in non-software mode");

#ifdef RANGECHECK
  if (x<0 ||x+width >vid.width || y<0 || y+height>vid.height || scrn > 4)
    I_Error ("Bad V_GetBlock");
#endif

  byte *src = vid.screens[scrn] + y*vid.width+x;

  while (height--)
    {
      memcpy(dest, src, width);
      src += vid.width;
      dest += width;
    }
}




//  Fills a box of pixels with a single color, NOTE: scaled to screen size
void V_DrawFill(int x, int y, int w, int h, int c)
{
#ifdef HWRENDER
  if (rendermode!=render_soft)
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
#ifdef HWRENDER
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
#ifdef HWRENDER
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
      height = font['0' - start]->height;
      width = font['0' - start]->width;
    }
  else
    {
      height = font[0]->height;
      width = font[0]->width;
    }
}


// Writes a single character (draw WHITE if bit 7 set)
void font_t::DrawCharacter(int x, int y, char c, int flags)
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
  if (x + t->width > vid.width)
    return;

  t->Draw(x, y, flags);
}



//  Draw a string using the font
//  NOTE: the text is centered for screens larger than the base width
void font_t::DrawString(int x, int y, const char *str, int flags)
{
  if (flags & V_WHITEMAP)
    {
      current_colormap = whitemap;
      flags |= V_MAP;
    }

  int dupx, dupy;

  if (flags & V_SSIZE)
    {
      dupx = vid.dupx;
      dupy = vid.dupy;
    }
  else
    {
      dupx = dupy = 1;
    }

  if (flags & V_SLOC)
    {
      x *= vid.dupx;
      y *= vid.dupy;
      flags &= ~V_SLOC; // not passed on to Texture::Draw
    }
    
  // cursor coordinates
  int cx = x + vid.scaledofs % vid.width;
  int cy = y + vid.scaledofs / vid.width;
  int rowheight = (height + 1) * dupy;

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

      int w = t->width * dupx;
      if (cx + w > vid.width)
        break;

      t->Draw(cx, cy, flags);

      cx += w;
    }
}


// returns the width of the string
int font_t::StringWidth(const char *str)
{
  int w = 0;

  for (int i = 0; str[i]; i++)
    {
      int c = toupper(str[i]);
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->width;
    }

  return w;
}


// returns the width of the next n chars of str
int font_t::StringWidth(const char *str, int n)
{
  int w = 0;

  for (int i = 0; i<n && str[i]; i++)
    {
      int c = toupper(str[i]);
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->width;
    }

  return w;
}


int font_t::StringHeight(const char *str)
{
  return height;
}



//========================================================================






struct modelvertex_t
{
  int px;
  int py;
};

void R_DrawSpanNoWrap();   //tmap.S

//
//  Tilts the view like DukeNukem...
//
//added:12-02-98:
#ifdef TILTVIEW
#ifdef HWRENDER
// TODO: Hurdler: see why we can't do it in software mode (it seems this only works for now with the DOS version)
void V_DrawTiltView (byte *viewbuffer)  // don't touch direct video I'll find something..
{}
#else

static modelvertex_t vertex[4];

void V_DrawTiltView (byte *viewbuffer)
{
    fixed_t    leftxfrac;
    fixed_t    leftyfrac;
    fixed_t    xstep;
    fixed_t    ystep;

    int        y;

    vertex[0].px = 0;   // tl
    vertex[0].py = 53;
    vertex[1].px = 212; // tr
    vertex[1].py = 0;
    vertex[2].px = 264; // br
    vertex[2].py = 144;
    vertex[3].px = 53;  // bl
    vertex[3].py = 199;

    // resize coords to screen
    for (y=0;y<4;y++)
    {
        vertex[y].px = (vertex[y].px * vid.width) / BASEVIDWIDTH;
        vertex[y].py = (vertex[y].py * vid.height) / BASEVIDHEIGHT;
    }

    ds_colormap = fixedcolormap;
    ds_source = viewbuffer;

    // starting points top-left and top-right
    leftxfrac  = vertex[0].px <<FRACBITS;
    leftyfrac  = vertex[0].py <<FRACBITS;

    // steps
    xstep = ((vertex[3].px - vertex[0].px)<<FRACBITS) / vid.height;
    ystep = ((vertex[3].py - vertex[0].py)<<FRACBITS) / vid.height;

    ds_y  = (int) vid.direct;
    ds_x1 = 0;
    ds_x2 = vid.width-1;
    ds_xstep = ((vertex[1].px - vertex[0].px)<<FRACBITS) / vid.width;
    ds_ystep = ((vertex[1].py - vertex[0].py)<<FRACBITS) / vid.width;


//    I_Error("ds_y %d ds_x1 %d ds_x2 %d ds_xstep %x ds_ystep %x \n"
//            "ds_xfrac %x ds_yfrac %x ds_source %x\n", ds_y,
//                      ds_x1,ds_x2,ds_xstep,ds_ystep,leftxfrac,leftyfrac,
//                      ds_source);

    // render spans
    for (y=0; y<vid.height; y++)
    {
        // FAST ASM routine!
        ds_xfrac = leftxfrac;
        ds_yfrac = leftyfrac;
        R_DrawSpanNoWrap ();
        ds_y += vid.rowbytes;

        // move along the left and right edges of the polygon
        leftxfrac += xstep;
        leftyfrac += ystep;
    }

}
#endif
#endif

//
// Test 'scrunch perspective correction' tm (c) ect.
//
//added:05-04-98:

#ifdef HWRENDER
// TODO: Hurdler: see why we can't do it in software mode (it seems this only works for now with the DOS version)
void V_DrawPerspView (byte *viewbuffer, int aiming)
{}
#else

void V_DrawPerspView (byte *viewbuffer, int aiming)
{

     byte*      source;
     byte*      dest;
     int        y;
     int        x1,w;
     int        offs;

     fixed_t    topfrac,bottomfrac,scale,scalestep;
     fixed_t    xfrac,xfracstep;

    source = viewbuffer;

    //+16 to -16 fixed
    offs = ((aiming*20)<<16) / 100;

    topfrac    = ((vid.width-40)<<16) - (offs*2);
    bottomfrac = ((vid.width-40)<<16) + (offs*2);

    scalestep  = (bottomfrac-topfrac) / vid.height;
    scale      = topfrac;

    for (y=0; y<vid.height; y++)
    {
        x1 = ((vid.width<<16) - scale)>>17;
        dest = ((byte*) vid.direct) + (vid.rowbytes*y) + x1;

        xfrac = (20<<FRACBITS) + ((!x1)&0xFFFF);
        xfracstep = FixedDiv((vid.width<<FRACBITS)-(xfrac<<1),scale);
        w = scale>>16;
        while (w--)
        {
            *dest++ = source[xfrac>>FRACBITS];
            xfrac += xfracstep;
        }
        scale += scalestep;
        source += vid.width;
    }

}
#endif
