// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.4  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.3  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.2  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.1.1.1  2002/11/16 14:18:26  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      low level span/column drawer functions.
//
//-----------------------------------------------------------------------------


#ifndef r_draw_h
#define r_draw_h 1

#include "doomdef.h"
#include "r_defs.h"

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------

#define FUZZTABLE     50

extern int fuzzoffset[FUZZTABLE];
extern int fuzzpos;

extern byte*            ylookup[MAXVIDHEIGHT];
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

extern lighttable_t*    dc_colormap;
extern lighttable_t*    dc_wcolormap;   //added:24-02-98:WATER!
extern int              dc_x;
extern int              dc_yl;
extern int              dc_yh;
extern int              dc_yw;          //added:24-02-98:WATER!
extern fixed_t          dc_iscale;
extern fixed_t          dc_texturemid;

extern byte*            dc_source;      // first pixel in a column

// translucency stuff here
extern byte*            transtables;    // translucency tables, should be (*transtables)[5][256][256]
extern byte*            dc_transmap;

// translation stuff here

extern byte*            translationtables;
extern byte*            dc_translation;

extern r_lightlist_t   *dc_lightlist;
extern int                        dc_numlights;
extern int                        dc_maxlights;

//Fix TUTIFRUTI
extern int      dc_texheight;


// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

extern int              ds_y;
extern int              ds_x1;
extern int              ds_x2;

extern lighttable_t*    ds_colormap;

extern fixed_t          ds_xfrac;
extern fixed_t          ds_yfrac;
extern fixed_t          ds_xstep;
extern fixed_t          ds_ystep;

extern byte*            ds_source;      // start of a 64*64 tile image
extern byte*            ds_transmap;


// viewborder patches lump numbers
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

extern class Texture *viewbordertex[8];

// ------------------------------------------------
// r_draw.c COMMON ROUTINES FOR BOTH 8bpp and 16bpp
// ------------------------------------------------

//added:26-01-98: called by SCR_Recalc() when video mode changes
void    R_RecalcFuzzOffsets();
// Initialize color translation tables, for player rendering etc.
void    R_InitTranslationTables();

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


// =========================================================================
#endif
