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
// Revision 1.14  2005/10/07 20:04:06  smite-meister
// sprite scaling
//
// Revision 1.11  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.8  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.7  2004/10/27 17:37:09  smite-meister
// netcode update
//
// Revision 1.5  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.4  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.2  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.1.1.1  2002/11/16 14:18:26  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Software renderer: low level span/column drawer functions.

#ifndef r_draw_h
#define r_draw_h 1

#include "doomdef.h"
#include "m_fixed.h"
#include "screen.h"  // MAXVIDWIDTH, MAXVIDHEIGHT


typedef byte lighttable_t;

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------

#define FUZZTABLE     50

extern int fuzzoffset[FUZZTABLE];
extern int fuzzpos;

extern byte**           ylookup;
extern byte*            ylookup1[MAXVIDHEIGHT];
extern byte*            ylookup2[MAXVIDHEIGHT];
extern int              columnofs[MAXVIDWIDTH];

#ifdef HORIZONTALDRAW
//Fab 17-06-98
extern byte*            yhlookup[MAXVIDWIDTH];
extern int              hcolumnofs[MAXVIDHEIGHT];
#endif

// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

extern int dc_x, dc_yl, dc_yh; ///< screen coordinates for the column
extern fixed_t  dc_iscale;     ///< screen coord * dc_iscale = texture coord
extern fixed_t  dc_texturemid; ///< this texture y coordinate corresponds to screen midpoint

extern byte    *dc_source;     ///< first pixel in a column
extern int      dc_texheight;  ///< height of repeating texture, zero for nonrepeating ones

extern lighttable_t    *dc_colormap;

// translucency stuff here

#define  MAXTRANSTABLES  20  // how many translucency tables may be used
extern byte  *transtables[MAXTRANSTABLES]; // translucency tables
extern byte  *dc_transmap;

// TODO: add another asm routine which use the fg and bg indexes in the
//       inverse order so the 20-80 becomes 80-20 translucency, no need
//       for other tables (thus 1090,2080,5050,8020,9010, and fire special)

/// Translucency tables
enum transnum_t
{
  tr_transmed = 1,    //sprite 50 backg 50  most shots
  tr_transmor = 2,    //       20       80  puffs
  tr_transhi  = 3,    //       10       90  blur effect
  tr_transfir = 4,    // 50 50 but brighter for fireballs, shots..
  tr_transfx1 = 5,    // 50 50 brighter some colors, else opaque for torches
  tr_size     = 0x10000,  // one transtable is 256*256 bytes in size
};



/// Color translation stuff
extern byte *dc_translation;

extern struct r_lightlist_t *dc_lightlist;
extern int              dc_numlights;
extern int              dc_maxlights;



// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

/// span y-coordinate and left/right x limits
extern int ds_y, ds_x1, ds_x2;

extern lighttable_t*    ds_colormap;

/// span texture offset and stepping
extern fixed_t ds_xfrac, ds_yfrac;
extern fixed_t ds_xstep, ds_ystep;

/// tex dimensions
extern int ds_xbits, ds_ybits;

extern byte*            ds_source;      // start of a 64*64 tile image
extern byte*            ds_transmap;


/// windowborder textures
enum windowborder_e
{
  BRDR_T = 0,
  BRDR_B,
  BRDR_L,
  BRDR_R,
  BRDR_TL,
  BRDR_TR,
  BRDR_BL,
  BRDR_BR
};

// textures for window borders and background
extern class Texture *window_border[8];
extern Texture *window_background;

// ------------------------------------------------
// r_draw.c COMMON ROUTINES FOR BOTH 8bpp and 16bpp
// ------------------------------------------------

//added:26-01-98: called by SCR_Recalc() when video mode changes
void    R_RecalcFuzzOffsets();

// Initialize color translation tables, for player rendering etc.
void    R_InitTranslationTables();

void    R_InitTranslucencyTables();

void    R_InitViewBuffer(int width, int height);

void    R_InitViewBorder();

void    R_VideoErase(unsigned ofs, int count);

// Rendering function.
void    R_FillBackScreen();

// If the view size is not full screen, draws a border around it.
void    R_DrawViewBorder();


// -----------------
// 8bpp DRAWING CODE
// -----------------

#ifdef HORIZONTALDRAW
//Fab 17-06-98
void    R_DrawHColumn_8();
#endif

void    ASMCALL R_DrawColumn_8();
void    ASMCALL R_DrawSkyColumn_8();
void    ASMCALL R_DrawShadeColumn_8();             //smokie test..
void    ASMCALL R_DrawFuzzColumn_8();
void    ASMCALL R_DrawTranslucentColumn_8();
void    ASMCALL R_DrawTranslatedColumn_8();
void    ASMCALL R_DrawSpan_8();

void    R_DrawTranslucentSpan_8();
void    R_DrawFogSpan_8();
void    R_DrawFogColumn_8(); //SoM: Test
void    R_DrawColumnShadowed_8();
void    R_DrawPortalColumn_8();

// ------------------
// 16bpp DRAWING CODE
// ------------------

void    ASMCALL R_DrawColumn_16();
void    ASMCALL R_DrawSkyColumn_16();
void    ASMCALL R_DrawFuzzColumn_16();
void    ASMCALL R_DrawTranslucentColumn_16();
void    ASMCALL R_DrawTranslatedColumn_16();
void    ASMCALL R_DrawSpan_16();


#endif
