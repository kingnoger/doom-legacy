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
/// \brief Software renderer: span/column drawer functions, all related global variables
///
/// All drawing to the view buffer is accomplished in this file.
/// The other refresh files only know about coordinates,
/// not the architecture of the frame buffer.
/// The frame buffer is a linear one, and we need only the base address.
/// NOTE: Actual drawing routines found in r_draw8.cpp and r_draw16.cpp

#include "doomdef.h"
#include "command.h"
#include "g_game.h"

#include "r_local.h"
#include "r_state.h"
#include "hud.h"
#include "i_video.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"


#ifdef HWRENDER
#include "hardware/hwr_render.h"
#endif

//==========================================================================
//                     COMMON DATA FOR 8bpp AND 16bpp
//==========================================================================

byte*           viewimage;
int             viewwidth;
int             scaledviewwidth;
int             viewheight;
int             viewwindowx;
int             viewwindowy;

// pointer to the start of each line of the screen,
// these tables convert viewport coords into LFB offsets
byte**          ylookup;
byte*           ylookup1[MAXVIDHEIGHT]; // for view1 (splitscreen)
byte*           ylookup2[MAXVIDHEIGHT]; // for view2 (splitscreen)

                 // x byte offset for columns inside the viewwindow
                // so the first column starts at (SCRWIDTH-VIEWWIDTH)/2
int             columnofs[MAXVIDWIDTH];


byte  translationtables[MAXSKINCOLORS][256];

//=========================================================================
//                      COLUMN DRAWING VARIABLES
//=========================================================================

/*!
  \defgroup g_sw_columndrawer Software renderer: column drawing
  \ingroup g_sw

  The column drawing routines of the software renderer handle the drawing of wall textures and sprites to the framebuffer.
  They use a number of global variables starting with "dc_" for parameter passing.

  The texturemapping for the i:th pixel in the column is given by
  ylookup[dc_yl+i][columnofs[dc_x]] = dc_colormap[dc_source[(dc_texturemid + (dc_yl+i-centery)*dc_iscale) % dc_texheight]];

  *R_DrawColumn_8: basic
  *R_DrawFuzzColumn_8: messes with dc_yl, dc_yh, uses dest[fuzzoffset[fuzzpos]] as source, maps it with lighttable 6
  *R_DrawTranslucentColumn_8: adds a dc_transmap[source][dest] mapping before the final dc_colormap
  R_DrawShadeColumn_8: chooses lightlevel colormap depending on source pixel, applies it on dest. What if source pixel >= 34?
  R_DrawTranslatedColumn_8: adds a dc_translation colormap before the final dc_colormap
  *R_DrawFogColumn_8: applies dc_colormap to dest, no source
  *R_DrawColumnShadowed_8: for fake floors shadowing walls

  @{*/
int            dc_x;          ///< viewport x coordinate of the column
int            dc_yl, dc_yh;  ///< low and high y limits of the column in viewport coords

byte*          dc_source;     ///< unmasked column data for the source texture 
int            dc_texheight;  ///< height of repeating source texture, zero for nonrepeating ones
fixed_t        dc_iscale;     ///< inverse scaling factor (viewport_coord * dc_iscale = texture_coord)
fixed_t        dc_texturemid; ///< texture y coordinate corresponding to the center of the viewport

lighttable_t  *dc_colormap;    ///< lighttable to use
byte          *dc_transmap;    ///< translucency table to use
byte          *dc_translation; ///< translation colormap to use

/// These are for 3D floors that cast shadows on walls.
r_lightlist_t *dc_lightlist = NULL;
int            dc_numlights = 0;
int            dc_maxlights;
//@}

//=========================================================================
//                      SPAN DRAWING CODE STUFF
//=========================================================================

/*!
  \defgroup g_sw_spandrawer Software renderer: span drawing
  \ingroup g_sw

  The span drawing routines of the software renderer handle the drawing of floor textures to the framebuffer.
  They use a number of global variables starting with "ds_" for parameter passing.

  *R_DrawSpan_8: basic
  *R_DrawTranslucentSpan_8: adds a ds_transmap[source][dest] mapping before the final ds_colormap
  *R_DrawFogSpan_8: applies ds_colormap to dest, no source
  @{*/
int       ds_y;         ///< viewport y coordinate for the span
int       ds_x1, ds_x2; ///< start and end viewport x coords for the span

byte     *ds_source;          ///< 2^n * 2^m -sized raw texture data
int       ds_xbits, ds_ybits; ///< n, m
fixed_t   ds_xfrac, ds_yfrac; ///< starting texture offsets
fixed_t   ds_xstep, ds_ystep; ///< texture scaling

lighttable_t *ds_colormap; ///< lighttable to use
byte         *ds_transmap; ///< translucency table to use
//@}

//==========================================================================
//                        OLD DOOM FUZZY EFFECT
//==========================================================================

//
// Spectre/Invisibility.
//
#define FUZZOFF       (1)

int fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

int fuzzpos = 0;     // move through the fuzz table


//  fuzzoffsets are dependend of vid width, for optimising purpose
//  this is called by SCR_Recalc() whenever the screen size changes
//
void R_RecalcFuzzOffsets()
{
    int i;
    for (i=0;i<FUZZTABLE;i++)
    {
        fuzzoffset[i] = (fuzzoffset[i] < 0) ? -vid.width : vid.width;
    }
}


// =========================================================================
//                   TRANSLATION COLORMAP CODE
// =========================================================================

//  Creates the translation tables to map the green color ramp to
//  another ramp (gray, brown, red, ...)
//
//  This is precalculated for drawing the player sprites in the player's
//  chosen color
//
void R_InitTranslationTables()
{
  int i, j;

  // player color translation (now static)
  //translationtables = (byte *)Z_MallocAlign(256 * MAXSKINCOLORS, PU_STATIC, 0, 8);

  // The first translation table is basically an identity map
  // (excepting the translucent palette index). It is taken from COLORMAP.
  byte *temp = R_GetFadetable(0)->colormap;
  for (i=0; i<256; i++)
    translationtables[0][i] = temp[i];

  // TODO Hexen has different translation colormaps
  // for each color AND each playerclass???: TRANTBL[0-8]
  /*
  for (i = 0; i < 3*(MAXPLAYERS-1); i++)
    {
      int lump = fc.GetNumForName("TRANTBL0") + i;
      fc.ReadLumpHeader(lump, translationtables[i], 256);
    }
  */

  // translate just the 16 green colors
  for (i=0 ; i<256 ; i++)
    {
      if ((game.mode < gm_heretic && i >= 112 && i <= 127) ||
	  (game.mode >= gm_heretic && i >=  225 && i <=  240))
        {
	  if (game.mode >= gm_heretic)
            {
	      translationtables[1][i] =   0+(i-225); // dark gray
	      translationtables[2][i] =  67+(i-225); // brown
	      translationtables[3][i] = 145+(i-225); // red
	      translationtables[4][i] =   9+(i-225); // light gray
	      translationtables[5][i] =  74+(i-225); // light brown
	      translationtables[6][i] = 150+(i-225); // light red
	      translationtables[7][i] = 192+(i-225); // light blue
	      translationtables[8][i] = 185+(i-225); // dark blue
	      translationtables[9][i] = 114+(i-225); // yellow
	      translationtables[10][i] = 95+(i-225); // beige
            }
	  else
            {
	      int index = i - 112;

	      // map green ramp to gray, brown, red
	      translationtables[1][i] = 0x60 + index;
	      translationtables[2][i] = 0x40 + index;
	      translationtables[3][i] = 0x20 + index;
	      translationtables[4][i] = 0x58 + index; // light gray
	      translationtables[5][i] = 0x38 + index; // light brown
	      translationtables[6][i] = 0xb0 + index; // light red
	      translationtables[7][i] = 0xc0 + index; // light blue

	      if (index < 9)
		translationtables[8][i] = 0xc7 + index;   // dark blue
	      else
		translationtables[8][i] = 0xf0-9 + index;

	      if (index < 8)
		translationtables[9][i] = 0xe0 + index;   // yellow
	      else
		translationtables[9][i] = 0xa0-8 + index;

	      translationtables[10][i] = 0x80 + index;     // beige
            }
        }
      else
        {
	  // Keep all other colors as is.
	  for (j=1; j < MAXSKINCOLORS; j++)
	    translationtables[j][i] = i;
        }
    }
}



// ==========================================================================
//               COMMON DRAWER FOR 8 AND 16 BIT COLOR MODES
// ==========================================================================

// in a perfect world, all routines would be compatible for either mode,
// and optimised enough
//
// in reality, the few routines that can work for either mode, are
// put here


// R_InitViewBuffer
// Creates lookup tables for getting the framebuffer address
//  of a pixel to draw.
//
void R_InitViewBuffer(int width, int height)
{
  int i;
  int bytesperpixel = vid.BytesPerPixel;

  if (bytesperpixel<1 || bytesperpixel>4)
    I_Error ("R_InitViewBuffer : wrong bytesperpixel value %d\n", bytesperpixel);

  // Handle resize,
  //  e.g. smaller view windows
  //  with border and/or status bar.
  viewwindowx = (vid.width-width) >> 1;

  // Column offset for those columns of the view window, but
  // relative to the entire screen
  for (i=0 ; i<width ; i++)
    columnofs[i] = (viewwindowx + i) * bytesperpixel;

  // Same with base row offset.
  if (width == vid.width)
    viewwindowy = 0;
  else
    viewwindowy = (vid.height-hud.stbarheight-height) >> 1;

  // Precalculate all row offsets.
  for (i=0 ; i<height ; i++)
    {
      ylookup1[i] = vid.buffer + (i+viewwindowy)*vid.width*bytesperpixel;
      ylookup2[i] = vid.buffer + (i+(vid.height>>1))*vid.width*bytesperpixel; // for splitscreen
    }
}


//
//  Window border and background textures
//
Texture *window_border[8];
Texture *window_background; // used to fill the space around the viewport

void R_InitViewBorder()
{
  const char *Doom_borders[] = {"BRDR_T", "BRDR_B", "BRDR_L", "BRDR_R", "BRDR_TL", "BRDR_TR", "BRDR_BL", "BRDR_BR"};
  const char *Raven_borders[] = {"BORDT", "BORDB", "BORDL", "BORDR", "BORDTL", "BORDTR", "BORDBL", "BORDBR"};
  const char **bname;
  if (game.mode < gm_heretic)
    bname = Doom_borders;
  else
    bname = Raven_borders;

  for (int i=0; i<8; i++)
    window_border[i] = tc.GetPtr(bname[i]);

  // choose and cache the default bg texture
  switch (game.mode)
    {
    case gm_doom2:
      // DOOM II border patch, original was GRNROCK
      window_background = tc.GetPtr("GRNROCK", TEX_floor);
      break;
    case gm_heretic:
      if (fc.FindNumForName("e2m1") == -1)
        window_background = tc.GetPtr("FLOOR04", TEX_floor);
      else
        window_background = tc.GetPtr("FLAT513", TEX_floor);
      break;
    case gm_hexen:
      window_background = tc.GetPtr("F_022", TEX_floor);
      break;
    default:
      // DOOM border patch.
      window_background = tc.GetPtr("FLOOR7_2", TEX_floor);
    }
}


//
// Fills the back screen with a pattern for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen()
{
  int  x, y;
  int  step, boff;

  // HW draws everything directly
  if (rendermode != render_soft)
    return;

  //added:08-01-98:draw pattern around the status bar too (when hires),
  //                so return only when in full-screen without status bar.
  if ((scaledviewwidth == vid.width)&&(viewheight==vid.height))
    return;

  Texture *t = window_background;

  for (y=0; y<vid.height; y += t->height)
    for (x=0; x<vid.width; x += t->width)
      t->Draw(x, y, 1);

  //added:08-01-98:dont draw the borders when viewwidth is full vid.width.
  if (scaledviewwidth == vid.width)
    return;

  if (game.mode >= gm_heretic)
    {
      step = 16;
      boff = 4; // borderoffset
    }
  else
    {
      step = 8;
      boff = 8;
    }

  for (x=0 ; x<scaledviewwidth ; x+=step)
    window_border[BRDR_T]->Draw(viewwindowx+x, viewwindowy-boff, 1);

  for (x=0 ; x<scaledviewwidth ; x+=step)
    window_border[BRDR_B]->Draw(viewwindowx+x, viewwindowy+viewheight, 1);

  for (y=0 ; y<viewheight ; y+=step)
    window_border[BRDR_L]->Draw(viewwindowx-boff, viewwindowy+y, 1);

  for (y=0 ; y<viewheight ; y+=step)
    window_border[BRDR_R]->Draw(viewwindowx+scaledviewwidth, viewwindowy+y, 1);

  // Draw beveled corners.
  window_border[BRDR_TL]->Draw(viewwindowx-boff, viewwindowy-boff, 1);
  window_border[BRDR_TR]->Draw(viewwindowx+scaledviewwidth, viewwindowy-boff, 1);
  window_border[BRDR_BL]->Draw(viewwindowx-boff, viewwindowy+viewheight, 1);
  window_border[BRDR_BR]->Draw(viewwindowx+scaledviewwidth, viewwindowy+viewheight, 1);
}


//
// Blit part of back buffer to visible screen
//
void R_VideoErase(unsigned ofs, int count)
{
  // LFB copy.
  // This might not be a good idea if memcpy
  //  is not optiomal, e.g. byte by byte on
  //  a 32bit CPU, as GNU GCC/Linux libc did
  //  at one point.
  memcpy(vid.screens[0]+ofs, vid.screens[1]+ofs, count);
}


//
// Blits the view border from backbuffer to visible screen.
//
void R_DrawViewBorder()
{
#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR.DrawViewBorder();
      return;
    }
#endif

  if (scaledviewwidth == vid.width)
    return;

  R_VideoErase(0, (vid.height - hud.stbarheight)*vid.width);
  /*
  int top  = (vid.height -hud.stbarheight -viewheight) >> 1;
  int side = (vid.width - scaledviewwidth) >> 1;

  // copy top and one line of left side
  R_VideoErase(0, top*vid.width + side);

  // copy one line of right side and bottom
  int ofs = (viewheight + top)*vid.width - side;
  R_VideoErase(ofs, top*vid.width + side);

  // copy sides using wraparound
  ofs = top*vid.width + vid.width-side;
  side <<= 1;

  //added:05-02-98:simpler using our new VID_Blit routine
  VID_BlitLinearScreen(vid.screens[1]+ofs, vid.screens[0]+ofs,
		       side, viewheight-1, vid.width, vid.width);
  */
}
