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
// Revision 1.1  2002/11/16 14:18:28  hurdler
// Initial revision
//
// Revision 1.6  2002/09/25 15:17:42  vberghol
// Intermission fixed?
//
// Revision 1.5  2002/08/21 16:58:36  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.4  2002/07/01 21:00:58  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:59  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.10  2001/05/16 21:21:15  bpereira
// no message
//
// Revision 1.9  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.8  2001/04/01 17:35:07  bpereira
// no message
//
// Revision 1.7  2001/02/24 13:35:21  bpereira
// no message
//
// Revision 1.6  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.5  2000/11/02 19:49:37  bpereira
// no message
//
// Revision 1.4  2000/08/31 14:30:56  bpereira
// no message
//
// Revision 1.3  2000/03/29 20:10:50  hurdler
// your fix didn't work under windows, find another solution
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Gamma correction LUT.
//      Functions to draw patches (by post) directly to screen.
//      Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#ifndef v_video_h
#define v_video_h 1

#include "doomdef.h"
#include "doomtype.h"

struct consvar_t;

//
// VIDEO
//

//added:18-02-98:centering offset for the scaled graphics,
//               this is normally temporarily changed by m_menu.c only.
//               The rest of the time it should be zero.
extern  int     scaledofs;

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

extern  byte*   screens[5];

extern  int     dirtybox[4];

extern  byte    gammatable[5][256];
extern  consvar_t cv_ticrate;
extern  consvar_t cv_usegamma;

// patch_t, the strange Doom graphics format.
// A patch holds one or more columns. A column is a vertical run of pixels.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//
//WARNING: this structure is cloned in GlidePatch_t
struct patch_t
{
  short               width;          // bounding box size
  short               height;
  short               leftoffset;     // pixels to the left of origin
  short               topoffset;      // pixels below the origin
  int                 columnofs[8];   // only [width] used
  // the [0] is &columnofs[width]
};

// posts are runs of non masked source pixels
struct post_t
{
  byte topdelta;       // -1 (0xff) is the last post in a column
  byte length;         // length data bytes follows
};

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;



// pic_t, another Doom graphics format.
// a pic is an unmasked block of pixels, stored in horizontal way

typedef enum {
  PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
  INTENSITY       = 1,  // 1 byte intensity
  INTENSITY_ALPHA = 2,  // 2 byte : alpha then intensity
  RGB24           = 3,  // 24 bit rgb
  RGBA32          = 4,  // 32 bit rgba
} pic_mode_t;

struct pic_t
{
  short  width;
  byte   zero;   // set to 0 allow autodetection of pic_t 
                   // mode instead of patch or raw
  byte   mode;   // see pic_mode_t above
  short  height;
  short  reserved1;  // set to 0
  byte   data[0];
};

// Allocates buffer screens, call before R_Init.
void V_Init();

// Set the current RGB palette lookup to use for palettized graphics
void V_SetPalette(int palettenum);

void V_SetPaletteLump(char *pal);

extern RGBA_t  *pLocalPalette;

// Retrieve the ARGB value from a palette color index
#define V_GetColor(color)  (pLocalPalette[color&0xFF])


void V_CopyRect(int srcx,  int srcy,  int srcscrn,
		int width, int height,
		int destx, int desty, int destscrn);

//added:03-02-98:like V_DrawPatch, + using a colormap.
void V_DrawMappedPatch(int x, int y, int scrn, patch_t *patch, byte *colormap);

//added:05-02-98:V_DrawPatch scaled 2,3,4 times size and position.

// flags hacked in scrn (not supported by all functions (see src))
enum {
  V_NOSCALESTART = 0x010000,   // dont scale x,y, start coords
  V_SCALESTART   = 0x020000,   // scale x,y, start coords
  V_SCALEPATCH   = 0x040000,   // scale patch
  V_NOSCALEPATCH = 0x080000,   // don't scale patch
  V_WHITEMAP     = 0x100000,   // draw white (for v_drawstring)
  V_FLIPPEDPATCH = 0x200000   // flipped in y 
};

// default params : scale patch and scale start
void V_DrawScaledPatch(int x, int y, int scrn, patch_t* patch); // scrn has flags in

//added:05-02-98:kiktest : this draws a patch using translucency
void V_DrawTransPatch(int x, int y, int scrn, patch_t* patch);

//added:16-02-98: like V_DrawScaledPatch, plus translucency
void V_DrawTranslucentPatch(int x, int y, int scrn, patch_t* patch);


void V_DrawPatch(int x, int y, int scrn, patch_t* patch);


// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src);

// Reads a linear block of pixels into the view buffer.
void V_GetBlock( int x, int y, int scrn, int width, int height, byte* dest);

// draw a pic_t, SCALED
void V_DrawScalePic( int x1, int y1, int scrn, int lumpnum /*pic_t* pic */);

void V_DrawRawScreen(int x, int y, int lumpnum, int width, int height);

void V_MarkRect(int x, int y, int width, int height);


//added:05-02-98: fill a box with a single color
void V_DrawFill(int x, int y, int w, int h, int c);
//added:06-02-98: fill a box with a flat as a pattern
void V_DrawFlatFill(int x, int y, int w, int h, int flatnum);

//added:10-02-98: fade down the screen buffer before drawing the menu over
void V_DrawFadeScreen(void);

//added:20-03-98: test console
void V_DrawFadeConsBack(int x1, int y1, int x2, int y2);

//added:20-03-98: draw a single character
void V_DrawCharacter(int x, int y, int c);

//added:05-02-98: draw a string using the hu_font
void V_DrawString(int x, int y, int option, const char *str);

// Find string width from hu_font chars
int V_StringWidth(const char* str);

// Find string height from hu_font chars
int V_StringHeight(const char* str);

// draw text with fontB (big font)
extern int FontBBaseLump;
void V_DrawTextB(const char *text, int x, int y);
void V_DrawTextBGray(char *text, int x, int y);
int V_TextBWidth(const char *text);
int V_TextBHeight(const char *text);

//added:12-02-98:
void V_DrawTiltView(byte *viewbuffer);

//added:05-04-98: test persp. correction !!
void V_DrawPerspView(byte *viewbuffer, int aiming);

void VID_BlitLinearScreen(byte *srcptr, byte *destptr, int width,
                           int height, int srcrowbytes, int destrowbytes);

#endif
