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
// Revision 1.13  2006/02/08 19:09:27  jussip
// Added beginnings of a new OpenGL renderer.
//
// Revision 1.12  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.11  2005/06/30 18:16:58  smite-meister
// texture anims fixed
//
// Revision 1.10  2005/03/16 21:16:08  smite-meister
// menu cleanup, bugfixes
//
// Revision 1.9  2004/12/08 10:16:03  segabor
// "segabor: byte alignment fix"
//
// Revision 1.8  2004/10/31 22:24:53  smite-meister
// pic_t moves into history
//
// Revision 1.7  2004/10/27 17:37:09  smite-meister
// netcode update
//
// Revision 1.6  2004/09/23 23:21:19  smite-meister
// HUD updated
//
// Revision 1.5  2004/08/13 18:25:11  smite-meister
// sw renderer fix
//
// Revision 1.4  2004/08/12 18:30:30  smite-meister
// cleaned startup
//
// Revision 1.3  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.2  2002/12/03 10:23:46  smite-meister
// Video system overhaul
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Texture blitting, blitting rectangles between buffers. Font system.

#ifndef v_video_h
#define v_video_h 1

#include "doomdef.h"
#include "doomtype.h"

/// flags for drawing Textures
enum texture_draw_e
{
  V_SLOC     =  0x10000,   // scale starting location
  V_SSIZE    =  0x20000,   // scale size
  V_FLIPX    =  0x40000,   // mirror the patch in the vertical direction
  V_MAP      =  0x80000,   // use a colormap
  V_WHITEMAP = 0x100000,   // white colormap (for V_DrawString)
  V_TL       = 0x200000,   // translucency using transmaps

  V_SCALE = V_SLOC | V_SSIZE,

  V_FLAGMASK = 0xFFFF0000,
  V_SCREENMASK = 0xF
};


/// \brief Class for raster fonts
class font_t
{
protected:
  char  start, end;  ///< first and last ascii characters included in the font
  class Texture **font;

public:
  int height, width; ///< dimensions of the character '0' in the font

public:
  font_t(int startlump, int endlump, char firstchar = '!');

  void DrawCharacter(int x, int y, char c, int flags = 0);
  void DrawString(int x, int y, const char *str, int flags = V_SCALE);
  int  StringWidth(const char *str);
  int  StringWidth(const char *str, int n);
  int  StringHeight(const char *str);
};

/// color translation
extern byte    translationtables[MAXSKINCOLORS][256];
extern byte   *current_colormap; // for applying colormaps to Drawn Textures
extern font_t *hud_font;
extern font_t *big_font;


void VID_BlitLinearScreen(byte *srcptr, byte *destptr, int width,
			  int height, int srcrowbytes, int destrowbytes);

void V_CopyRect(int srcx,  int srcy,  int srcscrn,
		int width, int height,
		int destx, int desty, int destscrn);


// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src);
// Reads a linear block of pixels into the view buffer.
void V_GetBlock( int x, int y, int scrn, int width, int height, byte* dest);


//added:05-02-98: fill a box with a single color
void V_DrawFill(int x, int y, int w, int h, int c);

//added:10-02-98: fade down the screen buffer before drawing the menu over
void V_DrawFadeScreen();
//added:20-03-98: test console
void V_DrawFadeConsBack(int x1, int y1, int x2, int y2);


//added:12-02-98:
void V_DrawTiltView(byte *viewbuffer);

//added:05-04-98: test persp. correction !!
void V_DrawPerspView(byte *viewbuffer, int aiming);

#endif
