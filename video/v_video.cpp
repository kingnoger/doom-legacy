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
// Revision 1.9  2004/08/12 18:30:34  smite-meister
// cleaned startup
//
// Revision 1.8  2004/08/02 20:49:58  jussip
// Minor compilation fix.
//
// Revision 1.7  2004/07/25 20:16:43  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.6  2004/04/01 09:16:16  smite-meister
// Texture system bugfixes
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
// Revision 1.29  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.28  2001/07/28 16:18:37  bpereira
// no message
//
// Revision 1.27  2001/05/16 21:21:14  bpereira
// no message
//
// Revision 1.26  2001/04/28 14:33:41  metzgermeister
// *** empty log message ***
//
// Revision 1.25  2001/04/17 22:30:40  hurdler
// fix some (strange!) problems
//
// Revision 1.24  2001/04/09 20:20:46  metzgermeister
// fixed crash bug
//
// Revision 1.23  2001/04/01 17:35:07  bpereira
// no message
//
// Revision 1.22  2001/03/30 17:12:51  bpereira
// no message
//
// Revision 1.21  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.20  2001/02/28 17:50:55  bpereira
// no message
//
// Revision 1.19  2001/02/24 13:35:21  bpereira
// no message
//
// Revision 1.18  2001/02/19 17:40:34  hurdler
// Fix a bug with "chat on" in hw mode
//
// Revision 1.17  2001/02/10 13:05:45  hurdler
// no message
//
// Revision 1.16  2001/01/31 17:14:08  hurdler
// Add cv_scalestatusbar in hardware mode
//
// Revision 1.15  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.14  2000/11/06 20:52:16  bpereira
// no message
//
// Revision 1.13  2000/11/04 16:23:44  bpereira
// no message
//
// Revision 1.12  2000/11/02 19:49:37  bpereira
// no message
//
// Revision 1.11  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.10  2000/08/31 14:30:56  bpereira
// no message
//
// Revision 1.9  2000/04/27 17:43:19  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.8  2000/04/24 20:24:38  bpereira
// no message
//
// Revision 1.7  2000/04/24 15:10:57  hurdler
// Support colormap for text
//
// Revision 1.6  2000/04/22 21:12:15  hurdler
// I like it better like that
//
// Revision 1.5  2000/04/06 20:47:08  hurdler
// add Boris' changes for coronas in doom3.wad
//
// Revision 1.4  2000/03/29 20:10:50  hurdler
// your fix didn't work under windows, find another solution
//
// Revision 1.3  2000/03/12 23:16:41  linuxcub
// Fixed definition of VID_BlitLinearScreen (Well, it now compiles under RH61)
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Functions to draw patches (by post) directly to screen.
//      Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------

#include <ctype.h>

#include "doomdef.h"

#include "r_data.h"
#include "r_state.h"

#include "v_video.h"
#include "hu_stuff.h"
#include "r_draw.h"
#include "console.h"
#include "command.h"
#include "screen.h"

#include "i_video.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hwr_render.h"
#endif


byte *current_colormap; // for applying colormaps to Drawn Textures


//
// V_CopyRect
//
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


#ifdef DEBUG
  CONS_Printf("V_CopyRect: vidwidth %d screen[%d]=%x to screen[%d]=%x\n",
              vid.width,srcscrn,vid.screens[srcscrn],destscrn,vid.screens[destscrn]);
  CONS_Printf("..........: srcx %d srcy %d width %d height %d destx %d desty %d\n",
              srcx,srcy,width,height,destx,desty);
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



#if !defined(USEASM) || defined(WIN32)
// --------------------------------------------------------------------------
// Copy a rectangular area from one bitmap to another (8bpp)
// srcPitch, destPitch : width of source and destination bitmaps
// --------------------------------------------------------------------------
void VID_BlitLinearScreen(byte* srcptr, byte* destptr,
                          int width, int height,
                          int srcrowbytes, int destrowbytes)
{
  if (srcrowbytes==destrowbytes)
    memcpy(destptr, srcptr, srcrowbytes * height);
  else
    {
      while (height--)
        {
          memcpy(destptr, srcptr, width);

          destptr += destrowbytes;
          srcptr += srcrowbytes;
        }
    }
}
#endif



//======================================================================

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
  fixed_t rowfrac, colfrac;
  fixed_t col = 0;
  if (flags & V_SSIZE)
    {
      colfrac = FixedDiv(FRACUNIT, vid.dupx<<FRACBITS);
      rowfrac = FixedDiv(FRACUNIT, vid.dupy<<FRACBITS);
      destend = desttop + width*vid.dupx;

      if (flags & V_FLIPX)
        {
          colfrac = -colfrac;
          col = (width << FRACBITS) + colfrac;
        }
    }
  else
    {
      colfrac = rowfrac = 1;
      destend = desttop + width;

      if (flags & V_FLIPX)
        {
          colfrac = -1;
          col = width - 1;
        }
    }

  patch_t *p = (patch_t *)Generate();

  if (flags & V_SSIZE)
    for ( ; desttop < destend; col += colfrac, desttop++)
      {
        post_t *post = (post_t *)(data + p->columnofs[col >> FRACBITS]);

        // step through the posts in a column
        while (post->topdelta != 0xff)
          {
            byte *source = post->data;
            byte *dest   = desttop + post->topdelta*vid.dupy*vid.width;
            int count  = post->length*vid.dupy;

            int row = 0;
            while (count--)
              {
                byte pixel = source[row >> FRACBITS];

                // the compiler is supposed to optimize the ifs out of the loop
                if (flags & V_MAP)
                  pixel = current_colormap[pixel];

                if (flags & V_TL)
                  pixel = transtables[(pixel << 8) + *dest];

                *dest = pixel;
                dest += vid.width;
                row += rowfrac;
              }
            post = (post_t *)&post->data[post->length + 1]; // next post
          }
      }
  else // unscaled, perhaps a bit faster?
    for ( ; desttop < destend; col += colfrac, desttop++)
      {
        post_t *post = (post_t *)(data + p->columnofs[col]);

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
                  pixel = transtables[(pixel << 8) + *dest];

                *dest = pixel;
                dest += vid.width;
              }
            post = (post_t *)&post->data[post->length + 1];
          }
      }
}


//======================================================================

//
// V_DrawRawScreen
// V_DrawScalePic: CURRENTLY USED FOR StatusBarOverlay, scale pic but not starting coords
// V_BlitScalePic
// Always scaled in software...
//

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

  if (mode != PALETTE)
    {
      CONS_Printf("pic mode %d not supported in Software\n", mode);
      return;
    }

  byte *dest = vid.screens[scrn] + max(0, y*vid.width) + max(0, x);

  // y clipping to the screen
  //  if (y + height*vid.dupy >= vid.height)
  //  mheight = (vid.height - y) / vid.dupy - 1;
  // TODO WARNING no x clipping (not needed for the moment)

  byte *base = Generate();

  // TODO crap
  for (int i=0; i<height; i++)
    {
      for (int dupy = vid.dupy; dupy; dupy--)
        {
          byte *src = base;
          for (int j=0; j<width; j++)
            {
              for (int dupx = vid.dupx; dupx; dupx--)
                *dest++ = *src;
              src++;
            }
          dest += vid.width - vid.dupx*width;
        }
      base += width;
    }
}


//======================================================================

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
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



//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
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




//
//  Fills a box of pixels with a single color, NOTE: scaled to screen size
//
//added:05-02-98:
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
//  Fills a box of pixels using a flat texture as a pattern,
//  scaled to screen size.
//
void V_DrawFlatFill(int x, int y, int w, int h, Texture *t)
{
#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR.DrawFill(x, y, w, h, t);
      return;
    }
#endif

  byte *flat = t->Generate();
  byte *dest = vid.screens[0] + y*vid.dupy*vid.width + x*vid.dupx + vid.scaledofs;

  w *= vid.dupx;
  h *= vid.dupy;

  int height = t->height;
  int width = t->width;

  fixed_t dx = FixedDiv(FRACUNIT, vid.dupx<<FRACBITS);
  fixed_t dy = FixedDiv(FRACUNIT, vid.dupy<<FRACBITS);

  fixed_t yfrac = 0;
  for (int v=0; v<h; v++, dest += vid.width)
    {
      fixed_t xfrac = 0;
      byte *src = flat + (((yfrac >> FRACBITS) % height) * width);
      for (int u=0; u<w; u++)
        {
          dest[u] = src[(xfrac >> FRACBITS) % width];
          xfrac += dx;
        }
      yfrac += dy;
    }
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
  byte *fadetable = (byte *)colormaps + 16*256;

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


// Simple translucence with one color, coords are resolution dependent
//
//added:20-03-98: console test
void V_DrawFadeConsBack(int x1, int y1, int x2, int y2)
{
  int         x,y,w;
  int         *buf;
  unsigned    quad;
  byte        p1, p2, p3, p4;
  short*      wput;

#ifdef HWRENDER
  if (rendermode!=render_soft)
    {
      HWR.FadeScreenMenuBack(0x00500000, y2);
      return;
    }
#endif

  if (vid.BytesPerPixel == 1)
    {
      x1 >>=2;
      x2 >>=2;
      for (y=y1 ; y<y2 ; y++)
        {
          buf = (int *)(vid.screens[0] + vid.width*y);
          for (x=x1 ; x<x2 ; x++)
            {
              quad = buf[x];
              p1 = greenmap[quad&255];
              p2 = greenmap[(quad>>8)&255];
              p3 = greenmap[(quad>>16)&255];
              p4 = greenmap[quad>>24];
              buf[x] = (p4<<24) | (p3<<16) | (p2<<8) | p1;
            }
        }
    }
  else
    {
      w = x2-x1;
      for (y=y1 ; y<y2 ; y++)
        {
          wput = (short*)(vid.screens[0] + vid.width*y) + x1;
          for (x=0 ; x<w ; x++) {
            *wput = ((*wput&0x7bde) + (15<<5)) >>1;
	    wput++;
	  }
        }
    }
}


//
// Writes a single character (draw WHITE if bit 7 set)
//
void V_DrawCharacter(int x, int y, int c)
{
  bool white = c & 0x80;
  int flags = c & V_FLAGMASK;
  c &= 0x7F;

  c = toupper(c) - HU_FONTSTART;
  if (c < 0 || c >= HU_FONTSIZE)
    return;

  Texture *t = hud.font[c];
  if (x + t->width > vid.width)
    return;

  if (white)
    {
      flags |= V_MAP;
      current_colormap = whitemap;
    }

  t->Draw(x, y, flags);
}



//
//  Write a string using the hu_font
//  NOTE: the text is centered for screens larger than the base width
//
void V_DrawString(int x, int y, int option, const char *str)
{
  if (option & V_WHITEMAP)
    {
      current_colormap = whitemap;
      option |= V_MAP;
    }

  int dupx, dupy, scrwidth;

  // TODO
  //if (option & V_SSIZE) {
      // scale later
      dupx = dupy = 1;
      scrwidth = BASEVIDWIDTH;
      /*
    }
  else
    {
      dupx = vid.dupx;
      dupy = vid.dupy;
      scrwidth = vid.width;
    }
      */

  // cursor coordinates
  int cx = x;
  int cy = y;

  while (1)
    {
      int c = *str++;
      if (!c)
        break;

      if (c == '\n')
        {
          cx = x;
          cy += 12*dupy;
          continue;
        }

      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        {
          cx += 4*dupx;
          continue;
        }

      Texture *t = hud.font[c];

      int w = t->width * dupx;
      if (cx + w > scrwidth)
        break;

      t->Draw(cx, cy, option | V_SCALE);

      cx += w;
    }
}


//
// Find string width from hu_font chars
//
int V_StringWidth(const char *str)
{
  int w = 0;

  for (int i = 0; str[i]; i++)
    {
      int c = toupper(str[i]) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        w += 4;
      else
        w += hud.font[c]->width;
    }

  return w;
}

//
// Find string height from hu_font chars
//
int V_StringHeight(const char *str)
{
  return hud.font[0]->height;
}


//---------------------------------------------------------------------------

int FontBBaseLump;

// Draw text using font B.
void V_DrawTextB(const char *text, int x, int y)
{
  char c;

  while((c = *text++) != 0)
    {
      if (c < 33)
        x += 8;
      else
        {
          Texture *t = tc.GetPtrNum(FontBBaseLump + toupper(c) - 33);
          t->Draw(x, y, V_SCALE);
          x += t->width - 1;
        }
    }
}


void V_DrawTextBGray(char *text, int x, int y)
{
  char c;
  current_colormap = graymap;

  while((c = *text++) != 0)
    {
      if (c < 33)
        x += 8;
      else
        {
          Texture *t = tc.GetPtrNum(FontBBaseLump + toupper(c) - 33);
          t->Draw(x, y, V_SCALE | V_MAP);
          x += t->width - 1;
        }
    }
}



// Returns the pixel width of a string using font B.
int V_TextBWidth(const char *text)
{
  char c;
  int width = 0;
  while((c = *text++) != 0)
    {
      if (c < 33)
        width += 5;
      else
        {
          Texture *t = tc.GetPtrNum(FontBBaseLump + toupper(c) - 33);
          width += t->width - 1;
        }
    }
  return width;
}

int V_TextBHeight(const char *text)
{
  return 16;
}


//
//
//
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
