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
// Revision 1.2  2002/12/03 10:07:13  smite-meister
// Video unit overhaul begins
//
// Revision 1.8  2002/09/25 15:17:44  vberghol
// Intermission fixed?
//
// Revision 1.7  2002/08/30 11:45:41  vberghol
// players system modified
//
// Revision 1.6  2002/08/21 16:58:40  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.5  2002/08/19 18:06:49  vberghol
// renderer somewhat fixed
//
// Revision 1.4  2002/07/01 21:01:13  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:02:00  vberghol
// HUD alkaa olla kunnossa
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

#include "doomdef.h"
#include "r_local.h"
#include "r_state.h"

#include "v_video.h"
#include "hu_stuff.h"
#include "r_draw.h"
#include "console.h"
#include "screen.h"

#include "i_video.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#include "hardware/hw_main.h"
#endif


//added:18-02-98: this is an offset added to the destination address,
//                for all SCALED graphics. When the menu is displayed,
//                it is TEMPORARILY set to vid.centerofs, the rest of
//                the time it should be zero.
//                The menu is scaled, a round multiple of the original
//                pixels to keep the graphics clean, then it is centered
//                a little, but excepeted the menu, scaled graphics don't
//                have to be centered. Set by m_menu.c, and SCR_Recalc()
int     scaledofs;


// V_MarkRect : this used to refresh only the parts of the screen
//              that were modified since the last screen update
//              it is useless today
//
/*
int dirtybox[4];
void V_MarkRect(int x, int y, int width, int height)
{
  M_AddToBox (dirtybox, x, y);
  M_AddToBox (dirtybox, x+width-1, y+height-1);
}
*/

//
// V_CopyRect
//
void V_CopyRect(int srcx, int srcy, int srcscrn,
		int width, int height,
		int destx, int desty, int destscrn)
{
    // WARNING don't mix
    if ((srcscrn & V_SCALESTART) || (destscrn & V_SCALESTART))
    {
        srcx*=vid.dupx;
        srcy*=vid.dupy;
        width*=vid.dupx;
        height*=vid.dupy;
        destx*=vid.dupx;
        desty*=vid.dupy;
    }
    srcscrn&=0xffff;
    destscrn&=0xffff;

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
    {
        I_Error ("Bad V_CopyRect %d %d %d %d %d %d %d %d", srcx, srcy, 
                  srcscrn, width, height, destx, desty, destscrn);
    }
#endif
    //V_MarkRect (destx, desty, width, height);

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
void VID_BlitLinearScreen (byte* srcptr, byte* destptr,
                           int width, int height,
                           int srcrowbytes, int destrowbytes)
{
    if (srcrowbytes==destrowbytes)
      memcpy (destptr, srcptr, srcrowbytes * height);
    else
    {
        while (height--)
        {
            memcpy (destptr, srcptr, width);

            destptr += destrowbytes;
            srcptr += srcrowbytes;
        }
    }
}
#endif


//
// V_DrawPatch
// Masks a column based masked pic to the screen. NO SCALING!!!
//
void V_DrawPatch(int x, int y, int scrn, patch_t *patch)
{
  // draw an hardware converted patch
#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR_DrawPatch ((GlidePatch_t*)patch, x, y, V_NOSCALESTART|V_NOSCALEPATCH);
      return;
    }
#endif

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);
#ifdef RANGECHECK
  if (x<0 || x+SHORT(patch->width) > vid.width || 
      y<0 || y+SHORT(patch->height) > vid.height
      || (unsigned)scrn>4)
    {
      fprintf(stderr, "Patch at %d,%d exceeds LFB\n", x,y);
      // No I_Error abort - what is up with TNT.WAD?
      fprintf(stderr, "V_DrawPatch: bad patch (ignored)\n");
      return;
    }
#endif

  //if (!scrn) V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));

  int col = 0;
  int         count;
  column_t*   column;
  byte*       dest;
  byte*       source;

  byte *desttop = vid.screens[scrn]+y*vid.width+x;

  int w = SHORT(patch->width);

  for (; col<w ; x++, col++, desttop++)
    {
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

      // step through the posts in a column
      while (column->topdelta != 0xff)
        {
	  source = (byte *)column + 3;
	  dest = desttop + column->topdelta*vid.width;
	  count = column->length;

	  while (count--)
            {
	      *dest = *source++;
	      dest += vid.width;
            }
	  column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}



//
// V_DrawScaledPatch
//   like V_DrawPatch, but scaled 2,3,4 times the original size and position
//   this is used for menu and title screens, with high resolutions
//
//added:05-02-98:
// default params : scale patch and scale start
void V_DrawScaledPatch(int x, int y, int scrn, patch_t *patch) // hacked flags in scrn...
{
    int         count;
    int         col;
    column_t*   column;
    byte*       desttop;
    byte*       dest;
    byte*       source;

    int         dupx,dupy;
    int         ofs;
    int         colfrac,rowfrac;
    byte*       destend;

    // draw an hardware converted patch
#ifdef HWRENDER
    if (rendermode != render_soft) 
    {
        HWR_DrawPatch ((GlidePatch_t*)patch, x, y, scrn);
        return;
    }
#endif

    if ((scrn & V_NOSCALEPATCH))
        dupx = dupy = 1;
    else
    {
        dupx = vid.dupx;
        dupy = vid.dupy;
    }
        
    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    colfrac  = FixedDiv (FRACUNIT, dupx<<FRACBITS);
    rowfrac  = FixedDiv (FRACUNIT, dupy<<FRACBITS);

    desttop = vid.screens[scrn&0xFF];
    if (scrn&V_NOSCALESTART)
        desttop += (y*vid.width) + x;
    else
        desttop += (y*dupy*vid.width) + (x*dupx) + scaledofs;
    destend = desttop + SHORT(patch->width) * dupx;

    if (scrn & V_FLIPPEDPATCH)
    {
        colfrac = -colfrac;
        col=(SHORT(patch->width)<<FRACBITS)+colfrac;
    }
    else
        col = 0;

    for (; desttop<destend ; col+=colfrac, desttop++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[col>>FRACBITS]));

        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest   = desttop + column->topdelta*dupy*vid.width;
            count  = column->length*dupy;

            ofs = 0;
            while (count--)
            {
                *dest = source[ofs>>FRACBITS];
                dest += vid.width;
                ofs += rowfrac;
            }

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}


//
//  V_DrawMappedPatch : like V_DrawScaledPatch, but with a colormap.
//
//
//added:05-02-98:
void V_DrawMappedPatch(int x, int y, int scrn, patch_t *patch, byte *colormap)
{
    int         count;
    int         col;
    column_t*   column;
    byte*       desttop;
    byte*       dest;
    byte*       source;
    int         w;

    int         dupx,dupy;
    int         ofs;
    int         colfrac,rowfrac;

    // draw an hardware converted patch
#ifdef HWRENDER
    if (rendermode != render_soft) 
    {
        HWR_DrawMappedPatch ((GlidePatch_t*)patch, x, y, scrn, colormap);
        return;
    }
#endif

    if ((scrn & V_NOSCALEPATCH))
        dupx = dupy = 1;
    else
    {
        dupx = vid.dupx;
        dupy = vid.dupy;
    }
    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    if (scrn & V_NOSCALESTART)
        desttop = vid.screens[scrn&0xffff] + (y*vid.width) + x;
    else
        desttop = vid.screens[scrn&0xffff] + (y*vid.dupy*vid.width) + (x*vid.dupx) + scaledofs;

    scrn &= 0xffff;

    //if (!scrn) V_MarkRect (x, y, SHORT(patch->width)*dupx, SHORT(patch->height)*dupy);

    col = 0;
    colfrac  = FixedDiv (FRACUNIT, dupx<<FRACBITS);
    rowfrac  = FixedDiv (FRACUNIT, dupy<<FRACBITS);

    w = SHORT(patch->width)<<FRACBITS;

    for (; col<w ; col+=colfrac, desttop++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[col>>FRACBITS]));

        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest   = desttop + column->topdelta*dupy*vid.width;
            count  = column->length*dupy;

            ofs = 0;
            while (count--)
            {
                *dest = *(colormap + source[ofs>>FRACBITS]);
                dest += vid.width;
                ofs += rowfrac;
            }

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }

}



//added:16-02-98: now used for crosshair
//
//  This draws a patch over a background with translucency...SCALED
//  SCALE THE STARTING COORDS!!
//
void V_DrawTranslucentPatch (int x, int y, int scrn, patch_t *patch) // hacked flag in scrn
{
    int         count;
    int         col;
    column_t*   column;
    byte*       desttop;
    byte*       dest;
    byte*       source;
    int         w;

    int         dupx,dupy;
    int         ofs;
    int         colfrac,rowfrac;

    // draw an hardware converted patch
#ifdef HWRENDER
    if (rendermode != render_soft) 
      {
        HWR_DrawPatch ((GlidePatch_t*)patch, x, y, scrn);
        return;
      }
#endif

    dupx = vid.dupx;
    dupy = vid.dupy;

    y -= SHORT(patch->topoffset)*dupy;
    x -= SHORT(patch->leftoffset)*dupx;

    // if (!(scrn&0xffff)) V_MarkRect (x, y, SHORT(patch->width)*dupx, SHORT(patch->height)*dupy);

    col = 0;
    colfrac  = FixedDiv (FRACUNIT, dupx<<FRACBITS);
    rowfrac  = FixedDiv (FRACUNIT, dupy<<FRACBITS);

    desttop = vid.screens[scrn&0xffff];
    if (scrn&V_NOSCALESTART)
        desttop += (y*vid.width) + x;        
    else
        desttop += (y*dupy*vid.width) + (x*dupx) + scaledofs;

    w = SHORT(patch->width)<<FRACBITS;

    for (; col<w ; col+=colfrac, desttop++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[col>>FRACBITS]));

        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest   = desttop + column->topdelta*dupy*vid.width;
            count  = column->length*dupy;

            ofs = 0;
            while (count--)
            {
                *dest = *(transtables + ((source[ofs>>FRACBITS]<<8)&0xFF00) + (*dest&0xFF));
                dest += vid.width;
                ofs += rowfrac;
            }

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}


//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src)
{
  byte*       dest;

#ifdef RANGECHECK
    if (x<0
        ||x+width >vid.width
        || y<0
        || y+height>vid.height
        || (unsigned)scrn>4)
    {
        I_Error ("Bad V_DrawBlock");
    }
#endif

    //V_MarkRect (x, y, width, height);

    dest = vid.screens[scrn] + y*vid.width + x;

    while (height--)
    {
        memcpy (dest, src, width);

        src += width;
        dest += vid.width;
    }
}



//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
void V_GetBlock (int x, int y, int scrn, int width, int height, byte *dest)
{
  byte *src;

  if (rendermode!=render_soft)
    I_Error ("V_GetBlock: called in non-software mode");


#ifdef RANGECHECK
    if (x<0
        ||x+width >vid.width
        || y<0
        || y+height>vid.height
        || (unsigned)scrn>4)
    {
        I_Error ("Bad V_GetBlock");
    }
#endif

    src = vid.screens[scrn] + y*vid.width+x;

    while (height--)
    {
        memcpy (dest, src, width);
        src += vid.width;
        dest += width;
    }
}

static void V_BlitScalePic(int x1, int y1, int scrn, pic_t *pic);
//  FIXME Draw a linear pic, scaled, TOTALLY CRAP CODE!!! OPTIMISE AND ASM!!
//  CURRENTLY USED FOR StatusBarOverlay, scale pic but not starting coords
//
void V_DrawScalePic (int x1, int y1, int scrn, int lumpnum)
{
#ifdef HWRENDER
    if (rendermode!=render_soft)
    {
        HWR_DrawPic(x1, y1, lumpnum);
        return;
    }
#endif

    V_BlitScalePic(x1, y1, scrn, (pic_t *)fc.CacheLumpNum(lumpnum,PU_CACHE));
}

static void V_BlitScalePic(int x1, int y1, int scrn, pic_t *pic)
{
    int         dupx,dupy;
    int         x,y;
    byte        *src, *dest;
    int         width,height;

    width = SHORT(pic->width);
    height= SHORT(pic->height);
    scrn&=0xffff;

    if (pic->mode != 0)
    {
        CONS_Printf("pic mode %d not supported in Software\n",pic->mode);
        return;
    }

    dest = vid.screens[scrn] + max(0,y1*vid.width) + max(0,x1);
    // y cliping to the screen
    if (y1+height*vid.dupy>=vid.width)
        height = (vid.width-y1)/vid.dupy-1;
    // WARNING no x clipping (not needed for the moment)

    for (y=max(0,-y1/vid.dupy) ; y<height ; y++)
    {
        for(dupy=vid.dupy;dupy;dupy--)        
        {
            src = pic->data + y*width;
            for (x=0 ; x<width ; x++)
            {
                for(dupx=vid.dupx;dupx;dupx--)
                    *dest++ = *src;
                src++;
            }
            dest += vid.width-vid.dupx*width;
        }
    }
}


void V_DrawRawScreen(int x1, int y1, int lumpnum, int width, int height)
{
#ifdef HWRENDER
    if (rendermode!=render_soft)
    {
        // save size somewhere and mark lump as a raw pic !
        GlidePatch_t *grpatch = fc.GetHWRNum(lumpnum);
        grpatch->width = width;
        grpatch->height = height;
        grpatch->mipmap.flags |= TF_RAWASPIC;
        HWR_DrawPic(x1, y1, lumpnum);
        return;
    }
#endif

    V_BlitScalePic(x1, y1, 0, fc.CacheRawAsPic(lumpnum, width, height, PU_CACHE));
}


//
//  Fills a box of pixels with a single color, NOTE: scaled to screen size
//
//added:05-02-98:
void V_DrawFill (int x, int y, int w, int h, int c)
{
    byte      *dest;
    int       u, v;
    int       dupx,dupy;

#ifdef HWRENDER
    if (rendermode!=render_soft)
    {
        HWR_DrawFill(x, y, w, h, c);
        return;
    }
#endif

    dupx = vid.dupx;
    dupy = vid.dupy;

    dest = vid.screens[0] + y*dupy*vid.width + x*dupx + scaledofs;

    w *= dupx;
    h *= dupy;

    for (v=0 ; v<h ; v++, dest += vid.width)
        for (u=0 ; u<w ; u++)
            dest[u] = c;
}



//
//  Fills a box of pixels using a flat texture as a pattern,
//  scaled to screen size.
//
//added:06-02-98:
void V_DrawFlatFill (int x, int y, int w, int h, int flatnum)
{
    byte      *dest;
    int       u, v;
    int       dupx,dupy;
    fixed_t   dx,dy,xfrac,yfrac;
    byte      *src;
    byte      *flat;

#ifdef HWRENDER
    if (rendermode != render_soft)
    {
        HWR_DrawFlatFill(x,y,w,h,flatnum);
        return;
    }
#endif

    flat = (byte *)fc.CacheLumpNum (flatnum, PU_CACHE);

    dupx = vid.dupx;
    dupy = vid.dupy;

    dest = vid.screens[0] + y*dupy*vid.width + x*dupx + scaledofs;

    w *= dupx;
    h *= dupy;

    dx = FixedDiv(FRACUNIT,dupx<<FRACBITS);
    dy = FixedDiv(FRACUNIT,dupy<<FRACBITS);

    yfrac = 0;
    for (v=0; v<h ; v++, dest += vid.width)
    {
        xfrac = 0;
        src = flat + (((yfrac>>FRACBITS)&63)<<6);
        for (u=0 ; u<w ; u++)
        {
            dest[u] = src[(xfrac>>FRACBITS)&63];
            xfrac += dx;
        }
        yfrac += dy;
    }
}



//
//  Fade all the screen buffer, so that the menu is more readable,
//  especially now that we use the small hufont in the menus...
//
void V_DrawFadeScreen ()
{
    int         x,y,w;
    int         *buf;
    unsigned    quad;
    byte        p1, p2, p3, p4;
    byte*       fadetable = (byte *) colormaps + 16*256;
    //short*    wput;

#ifdef HWRENDER // not win32 only 19990829 by Kin
    if (rendermode!=render_soft) {
        HWR_FadeScreenMenuBack (0x01010160, 0);  //faB: hack, 0 means full height :o
        return;
    }
#endif

    w = vid.width>>2;
    for (y=0 ; y<vid.height ; y++)
    {
        buf = (int *)(vid.screens[0] + vid.width*y);
        for (x=0 ; x<w ; x++)
        {
            quad = buf[x];
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
                *wput++ = (*wput>>1) & 0x3def;
            }
        }
    }
#endif
}


// Simple translucence with one color, coords are resolution dependent
//
//added:20-03-98: console test
void V_DrawFadeConsBack (int x1, int y1, int x2, int y2)
{
    int         x,y,w;
    int         *buf;
    unsigned    quad;
    byte        p1, p2, p3, p4;
    short*      wput;

#ifdef HWRENDER // not win32 only 19990829 by Kin
    if (rendermode!=render_soft) {
        HWR_FadeScreenMenuBack (0x00500000, y2);  
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
            for (x=0 ; x<w ; x++)
            {
                *wput++ = ((*wput&0x7bde) + (15<<5)) >>1;
            }
        }
    }
}


// Writes a single character (draw WHITE if bit 7 set)
//
//added:20-03-98:
void V_DrawCharacter (int x, int y, int c)
{
    int         w;
    int         flags;
    bool     white;

    white = c & 0x80;
    flags = c & 0xffff0000;
    c &= 0x7f;

    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c>= HU_FONTSIZE)
        return;

    w = (hu_font[c]->width);
    if (x+w > vid.width)
        return;

    if (white)
        // draw with colormap, WITHOUT scale
        V_DrawMappedPatch(x, y, 0|flags, hu_font[c], whitemap);
    else
        V_DrawScaledPatch(x, y, 0|flags, hu_font[c]);
}



//
//  Write a string using the hu_font
//  NOTE: the text is centered for screens larger than the base width
//
//added:05-02-98:
void V_DrawString (int x, int y, int option, const char *str)
{
    int         w;
    const char* ch;
    int         c;
    int         cx;
    int         cy;
    int         dupx,dupy,scrwidth = BASEVIDWIDTH;

    ch = str;
    cx = x;
    cy = y;
    if (option & V_NOSCALESTART)
    {
        dupx = vid.dupx;
        dupy = vid.dupy;
        scrwidth = vid.width;
    }
    else
        dupx = dupy = 1;

    while(1)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x;
            cy += 12*dupy;
            continue;
        }

        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c>= HU_FONTSIZE)
        {
            cx += 4*dupx;
            continue;
        }

        w = (hu_font[c]->width)*dupx;
        if (cx+w > scrwidth)
            break;
        if (option & V_WHITEMAP)
            V_DrawMappedPatch(cx, cy, option, hu_font[c], whitemap);
        else
            V_DrawScaledPatch(cx, cy, option, hu_font[c]);
        cx+=w;
    }
}

//
// Find string width from hu_font chars
//
int V_StringWidth(const char *str)
{
  int             i;
  int             w = 0;
  int             c;

  for (i = 0;i < (int)strlen(str);i++)
    {
      c = toupper(str[i]) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
	w += 4;
      else
	w +=  (hu_font[c]->width);
    }

  return w;
}

//
// Find string height from hu_font chars
//
int V_StringHeight (const char *str)
{
  return (hu_font[0]->height);
}


//---------------------------------------------------------------------------
//
// PROC MN_DrTextB
//
// Draw text using font B.
//
//---------------------------------------------------------------------------
int FontBBaseLump;

void V_DrawTextB(const char *text, int x, int y)
{
    char c;
    patch_t *p;
    
    while((c = *text++) != 0)
    {
        if (c < 33)
        {
            x += 8;
        }
        else
        {
            p = fc.CachePatchNum(FontBBaseLump+toupper(c)-33, PU_CACHE);
            V_DrawScaledPatch(x, y, 0, p);
            x += p->width-1;
        }
    }
}

void V_DrawTextBGray(char *text, int x, int y)
{
    char c;
    patch_t *p;
    
    while((c = *text++) != 0)
    {
        if (c < 33)
        {
            x += 8;
        }
        else
        {
            p = fc.CachePatchNum(FontBBaseLump+toupper(c)-33, PU_CACHE);
            V_DrawMappedPatch(x, y, 0, p, graymap);
            x += p->width-1;
        }
    }
}


//---------------------------------------------------------------------------
//
// FUNC MN_TextBWidth
//
// Returns the pixel width of a string using font B.
//
//---------------------------------------------------------------------------

int V_TextBWidth(const char *text)
{
    char c;
    int width;
    patch_t *p;
    
    width = 0;
    while((c = *text++) != 0)
    {
        if (c < 33)
        {
            width += 5;
        }
        else
        {
            p = fc.CachePatchNum(FontBBaseLump+toupper(c)-33, PU_CACHE);
            width += p->width-1;
        }
    }
    return(width);
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

#ifdef HWRENDER // not win32 only 19990829 by Kin
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
