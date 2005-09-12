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
// Revision 1.17  2005/09/12 18:33:45  smite-meister
// fixed_t, vec_t
//
// Revision 1.16  2005/04/22 19:44:50  smite-meister
// bugs fixed
//
// Revision 1.15  2005/01/25 18:29:18  smite-meister
// preparing for alpha
//
// Revision 1.14  2004/12/31 16:19:41  smite-meister
// alpha fixes
//
// Revision 1.13  2004/10/27 17:37:11  smite-meister
// netcode update
//
// Revision 1.12  2004/10/14 19:35:52  smite-meister
// automap, bbox_t
//
// Revision 1.11  2004/09/23 23:21:20  smite-meister
// HUD updated
//
// Revision 1.9  2004/08/15 18:08:30  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.8  2004/07/25 20:16:43  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.7  2004/03/28 15:16:15  smite-meister
// Texture cache.
//
// Revision 1.6  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.5  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/03 10:07:13  smite-meister
// Video unit overhaul begins
//
// Revision 1.13  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.12  2001/04/01 17:35:06  bpereira
// no message
//
// Revision 1.11  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.10  2001/02/24 13:35:21  bpereira
// no message
//
// Revision 1.9  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.8  2000/11/09 17:56:20  stroggonmeth
// Hopefully fixed a few bugs and did a few optimizations.
//
// Revision 1.7  2000/11/03 03:48:54  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.6  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/07/01 09:23:49  bpereira
// no message
//
// Revision 1.4  2000/04/07 18:47:09  hurdler
// There is still a problem with the asm code and boom colormap
// At least, with this little modif, it compiles on my Linux box
//
// Revision 1.3  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
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

// ==========================================================================
//                     COMMON DATA FOR 8bpp AND 16bpp
// ==========================================================================

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

#ifdef HORIZONTALDRAW
//Fab 17-06-98: horizontal column drawer optimisation
byte*           yhlookup[MAXVIDWIDTH];
int             hcolumnofs[MAXVIDHEIGHT];
#endif

// =========================================================================
//                      COLUMN DRAWING CODE STUFF
// =========================================================================

lighttable_t           *dc_fadetable; ///< the entire fadetable in the map (for fuzz drawing)
lighttable_t           *dc_colormap;
int                     dc_x;         // viewport x coordinate of the column
int                     dc_yl, dc_yh; // low and high y limits of the column in viewport coords

//Hurdler: 04/06/2000: asm code still use it
//#ifdef OLDWATER
int                     dc_yw;          //added:24-02-98: WATER!
lighttable_t           *dc_wcolormap;   //added:24-02-98:WATER!
//#endif

fixed_t                 dc_iscale;
fixed_t                 dc_texturemid;

byte*                   dc_source;

// -----------------------
// translucency stuff here
// -----------------------

// R_DrawTransColumn uses this
byte                   *dc_transmap; // one of the translucency tables


// ----------------------
// translation stuff here
// ----------------------

byte  translationtables[MAXSKINCOLORS][256];
// R_DrawTranslatedColumn uses this
byte*                   dc_translation;

r_lightlist_t *dc_lightlist = NULL;
int                     dc_numlights = 0;
int                     dc_maxlights;

int     dc_texheight;

// =========================================================================
//                      SPAN DRAWING CODE STUFF
// =========================================================================

int                     ds_y;
int                     ds_x1;
int                     ds_x2;

lighttable_t*           ds_colormap;

Sint32                 ds_xfrac;
Sint32                 ds_yfrac;
Sint32                 ds_xstep;
Sint32                 ds_ystep;

byte*                   ds_source;      // start of a 64*64 tile image
byte*                   ds_transmap;    // one of the translucency tables


// ==========================================================================
//                        OLD DOOM FUZZY EFFECT
// ==========================================================================

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
  // for each color AND each playerclass: TRANTBL[0-8]

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


#ifdef HORIZONTALDRAW
  //Fab 17-06-98
  // create similar lookup tables for horizontal column draw optimisation

  // (the first column is the bottom line)
  for (i=0; i<width; i++)
    yhlookup[i] = vid.screens[2] + ((width-i-1) * bytesperpixel * height);

  for (i=0; i<height; i++)
    hcolumnofs[i] = i * bytesperpixel;
#endif
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
      window_background = tc.GetPtr("GRNROCK");
      break;
    case gm_heretic:
      if (fc.FindNumForName("e2m1") == -1)
        window_background = tc.GetPtr("FLOOR04");
      else
        window_background = tc.GetPtr("FLAT513");
      break;
    case gm_hexen:
      window_background = tc.GetPtr("F_022");
      break;
    default:
      // DOOM border patch.
      window_background = tc.GetPtr("FLOOR7_2");
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
