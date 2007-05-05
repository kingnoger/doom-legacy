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

#ifndef v_video_h
#define v_video_h 1

#include <vector>
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
  char  start, end;     ///< first and last ASCII characters included in the font
  std::vector<class Material*> font; ///< one Material per symbol

public:
  float height, width;  ///< world dimensions of the character '0' in the font

public:
  font_t(int startlump, int endlump, char firstchar = '!');

  /// Write a single character (draw WHITE if bit 7 set)
  void DrawCharacter(float x, float y, char c, int flags);
  /// Write a string using the font.
  void DrawString(float x, float y, const char *str, int flags);
  /// Returns the width of the string in unscaled pixels
  float StringWidth(const char *str);
  float StringWidth(const char *str, int n);
  float StringHeight(const char *str);
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
