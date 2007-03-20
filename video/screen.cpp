// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief Video subsystem. Handles multiple resolutions, 8bpp/16bpp(highcolor) modes,
/// palettes etc.

#include "doomdef.h"

#include "screen.h"
#include "v_video.h"

#include "g_game.h"
#include "console.h"
#include "command.h"
#include "am_map.h"
#include "hud.h"

#include "m_argv.h"
#include "i_video.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"

#ifndef NO_OPENGL
#include "hardware/hwr_render.h"
#endif

#include "w_wad.h"
#include "z_zone.h"


// ------------------
// global video state
// ------------------
Video vid;


// --------------------------------------------
// assembly or c drawer routines for software mode 8bpp/16bpp
// --------------------------------------------
void (*skycolfunc) ();       //new sky column drawer draw posts >128 high
void (*colfunc) ();          // standard column upto 128 high posts
void (*basecolfunc) ();
void (*fuzzcolfunc) ();      // standard fuzzy effect column drawer
void (*transcolfunc) ();     // translucent column drawer
void (*shadecolfunc) ();     // smokie test..
void (*spanfunc) ();         // span drawer, use a 64x64 tile
void (*basespanfunc) ();     // default span func for color mode


// =========================================================================
// console variables
// =========================================================================

void CV_Fuzzymode_OnChange();
void CV_Fullscreen_OnChange();
void CV_Usegamma_OnChange();

CV_PossibleValue_t scr_depth_cons_t[]={{8,"8 bits"}, {16,"16 bits"}, {24,"24 bits"}, {32,"32 bits"}, {0,NULL}};

//added:03-02-98: default screen mode, as loaded/saved in config
consvar_t   cv_scr_width  = {"scr_width",  "320", CV_SAVE, CV_Unsigned};
consvar_t   cv_scr_height = {"scr_height", "200", CV_SAVE, CV_Unsigned};
consvar_t   cv_scr_depth =  {"scr_depth",  "16 bits",   CV_SAVE, scr_depth_cons_t};
consvar_t   cv_fullscreen = {"fullscreen", "Yes", CV_SAVE | CV_CALL | CV_NOINIT, CV_YesNo, CV_Fullscreen_OnChange};

// Are invisible things translucent or fuzzy?
consvar_t   cv_fuzzymode = {"fuzzymode", "Off", CV_SAVE | CV_CALL, CV_OnOff, CV_Fuzzymode_OnChange};

// gamma correction
CV_PossibleValue_t gamma_cons_t[] = {{0,"MIN"},{4,"MAX"},{0,NULL}};
consvar_t  cv_usegamma = {"gamma","0",CV_SAVE|CV_CALL,gamma_cons_t,CV_Usegamma_OnChange};

// TODO calculate gammatable anew each time? more gamma levels?
// gammatable[i][j] = round(255.0*pow((j+1)/256.0, 1.0-i*0.125));
byte gammatable[5][256] =
{
    {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
     17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
     33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
     49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
     65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
     81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
     97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
     113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
     128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
     144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
     160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
     176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
     192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
     208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
     224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
     240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255},

    {2,4,5,7,8,10,11,12,14,15,16,18,19,20,21,23,24,25,26,27,29,30,31,
     32,33,34,36,37,38,39,40,41,42,44,45,46,47,48,49,50,51,52,54,55,
     56,57,58,59,60,61,62,63,64,65,66,67,69,70,71,72,73,74,75,76,77,
     78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,
     99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,
     115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,129,
     130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,
     146,147,148,148,149,150,151,152,153,154,155,156,157,158,159,160,
     161,162,163,163,164,165,166,167,168,169,170,171,172,173,174,175,
     175,176,177,178,179,180,181,182,183,184,185,186,186,187,188,189,
     190,191,192,193,194,195,196,196,197,198,199,200,201,202,203,204,
     205,205,206,207,208,209,210,211,212,213,214,214,215,216,217,218,
     219,220,221,222,222,223,224,225,226,227,228,229,230,230,231,232,
     233,234,235,236,237,237,238,239,240,241,242,243,244,245,245,246,
     247,248,249,250,251,252,252,253,254,255},

    {4,7,9,11,13,15,17,19,21,22,24,26,27,29,30,32,33,35,36,38,39,40,42,
     43,45,46,47,48,50,51,52,54,55,56,57,59,60,61,62,63,65,66,67,68,69,
     70,72,73,74,75,76,77,78,79,80,82,83,84,85,86,87,88,89,90,91,92,93,
     94,95,96,97,98,100,101,102,103,104,105,106,107,108,109,110,111,112,
     113,114,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
     129,130,131,132,133,133,134,135,136,137,138,139,140,141,142,143,144,
     144,145,146,147,148,149,150,151,152,153,153,154,155,156,157,158,159,
     160,160,161,162,163,164,165,166,166,167,168,169,170,171,172,172,173,
     174,175,176,177,178,178,179,180,181,182,183,183,184,185,186,187,188,
     188,189,190,191,192,193,193,194,195,196,197,197,198,199,200,201,201,
     202,203,204,205,206,206,207,208,209,210,210,211,212,213,213,214,215,
     216,217,217,218,219,220,221,221,222,223,224,224,225,226,227,228,228,
     229,230,231,231,232,233,234,235,235,236,237,238,238,239,240,241,241,
     242,243,244,244,245,246,247,247,248,249,250,251,251,252,253,254,254,
     255},

    {8,12,16,19,22,24,27,29,31,34,36,38,40,41,43,45,47,49,50,52,53,55,
     57,58,60,61,63,64,65,67,68,70,71,72,74,75,76,77,79,80,81,82,84,85,
     86,87,88,90,91,92,93,94,95,96,98,99,100,101,102,103,104,105,106,107,
     108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,
     125,126,127,128,129,130,131,132,133,134,135,135,136,137,138,139,140,
     141,142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,
     155,156,157,158,159,160,160,161,162,163,164,165,165,166,167,168,169,
     169,170,171,172,173,173,174,175,176,176,177,178,179,180,180,181,182,
     183,183,184,185,186,186,187,188,189,189,190,191,192,192,193,194,195,
     195,196,197,197,198,199,200,200,201,202,202,203,204,205,205,206,207,
     207,208,209,210,210,211,212,212,213,214,214,215,216,216,217,218,219,
     219,220,221,221,222,223,223,224,225,225,226,227,227,228,229,229,230,
     231,231,232,233,233,234,235,235,236,237,237,238,238,239,240,240,241,
     242,242,243,244,244,245,246,246,247,247,248,249,249,250,251,251,252,
     253,253,254,254,255},

    {16,23,28,32,36,39,42,45,48,50,53,55,57,60,62,64,66,68,69,71,73,75,76,
     78,80,81,83,84,86,87,89,90,92,93,94,96,97,98,100,101,102,103,105,106,
     107,108,109,110,112,113,114,115,116,117,118,119,120,121,122,123,124,
     125,126,128,128,129,130,131,132,133,134,135,136,137,138,139,140,141,
     142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,155,
     156,157,158,159,159,160,161,162,163,163,164,165,166,166,167,168,169,
     169,170,171,172,172,173,174,175,175,176,177,177,178,179,180,180,181,
     182,182,183,184,184,185,186,187,187,188,189,189,190,191,191,192,193,
     193,194,195,195,196,196,197,198,198,199,200,200,201,202,202,203,203,
     204,205,205,206,207,207,208,208,209,210,210,211,211,212,213,213,214,
     214,215,216,216,217,217,218,219,219,220,220,221,221,222,223,223,224,
     224,225,225,226,227,227,228,228,229,229,230,230,231,232,232,233,233,
     234,234,235,235,236,236,237,237,238,239,239,240,240,241,241,242,242,
     243,243,244,244,245,245,246,246,247,247,248,248,249,249,250,250,251,
     251,252,252,253,254,254,255,255}
};

// reload palette when gamma is changed
void CV_Usegamma_OnChange()
{
  // reload palette
  vid.LoadPalette("PLAYPAL");
  vid.SetPalette(0);
}

// change drawer function when fuzzymode is changed
void CV_Fuzzymode_OnChange()
{
  if (vid.BytesPerPixel == 1)
    fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_8 : R_DrawTranslucentColumn_8;
  else if (vid.BytesPerPixel > 1)
    fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_16 : R_DrawTranslucentColumn_16;
}

int I_GetVideoModeForSize(int w, int h);

// Change fullscreen on/off when cv_fullscreen is changed
void CV_Fullscreen_OnChange()
{
  vid.setmodeneeded = I_GetVideoModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
}

// =========================================================================

Video::Video()
{
  buffer = direct = NULL;
  palette = NULL;
  currentpalette = 0;
}


//added:27-01-98: tell asm code the new rowbytes value.
void ASMCALL ASM_PatchRowBytes(int rowbytes);

//  The video mode change is delayed until the start of the next refresh
//  by setting the setmodeneeded to a value >0
int  I_SetVideoMode(int modenum);


void Video::SetMode()
{
  if (game.dedicated)
    return;

  if (!setmodeneeded)
    return;   //should never happen

  I_SetVideoMode(--setmodeneeded);
  SetPalette(0);

  //  setup the right draw routines for either 8bpp or 16bpp
  if (BytesPerPixel == 1)
    {
      colfunc = basecolfunc = R_DrawColumn_8;
      skycolfunc = R_DrawColumn_8;

      fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_8 : R_DrawTranslucentColumn_8;
      transcolfunc = R_DrawTranslatedColumn_8;
      shadecolfunc = R_DrawShadeColumn_8;  //R_DrawColumn_8;
      spanfunc = basespanfunc = R_DrawSpan_8;
    }
  else if (BytesPerPixel > 1)
    {
      CONS_Printf("using highcolor mode\n");

      colfunc = basecolfunc = R_DrawColumn_16;
      skycolfunc = R_DrawColumn_16;

      fuzzcolfunc = (cv_fuzzymode.value) ? R_DrawFuzzColumn_16 : R_DrawTranslucentColumn_16;
      transcolfunc = R_DrawTranslatedColumn_16;
      shadecolfunc = NULL;      //detect error if used somewhere..
      spanfunc = basespanfunc = R_DrawSpan_16;
    }
  else
    I_Error("unknown bytes per pixel mode %d\n", BytesPerPixel);

  setmodeneeded = 0;

  Recalc();
}

void R_Init8to16();

// Starts and initializes the video subsystem
void Video::Startup()
{
  if (game.dedicated)
    return;

  I_StartupGraphics();

  modenum = 0; // not exactly true, but doesn't matter here.
  setmodeneeded = 0;

  LoadPalette("PLAYPAL");
  SetPalette(0);

  //fab highcolor maps
  if (BytesPerPixel == 2)
    {
      CONS_Printf("\nInitHighColor...");
      R_Init8to16();
    }

  // create palette conversion colormaps if necessary (palette must be set!)
  tc.InitPaletteConversion();

  buffer = NULL;

  Recalc();
}



// Called after the video mode has changed
void Video::Recalc()
{
  rowbytes = width * BytesPerPixel;

#ifdef USEASM
  // patch the asm code depending on vid buffer rowbytes
  ASM_PatchRowBytes(rowbytes);
#endif

  // scale 1,2,3 times in x and y the patches for the
  // menus and overlays... calculated once and for all
  // used by routines in v_video.c
  {
    fdupx = (float)width / BASEVIDWIDTH;
    fdupy = (float)height / BASEVIDHEIGHT;
    dupx = (int)fdupx;
    dupy = (int)fdupy;
    //baseratio = FixedDiv(height << FRACBITS, BASEVIDHEIGHT << FRACBITS); //Hurdler: not used anymore
  }

  // calculate centering offset for the scaled menu
  scaledofs = 0;
  centerofs = (((height%BASEVIDHEIGHT)/2) * width) +
    (width%BASEVIDWIDTH)/2;

  int i;

#ifndef NO_OPENGL
  if (rendermode != render_soft)
    {
      // hardware modes do not use screens[] pointers
      buffer = NULL;
      // be sure to cause a NULL read/write error so we detect it, in case of..
      for (i=0 ; i<NUMSCREENS ; i++)
        screens[i] = NULL;
    }
  else
#endif
    {
      // screens[0] points to the actual video surface, other screens to buffers we allocate ourselves.
      screens[0] = direct;

      if (buffer)
        free(buffer);

      int screensize = width * height * BytesPerPixel;
      buffer = static_cast<byte*>(calloc(screensize * (NUMSCREENS-1), 1));

      for (i=1 ; i<NUMSCREENS ; i++)
        screens[i] = buffer + (i-1)*screensize;
    }

  // fuzzoffsets for the 'spectre' effect,... this is a quick hack
  R_RecalcFuzzOffsets();

  // r_plane stuff : visplanes, openings, floorclip, ceilingclip, spanstart,
  //                 spanstop, yslope, distscale, cachedheight, cacheddistance,
  //                 cachedxstep, cachedystep
  //              -> allocated at the maximum vidsize, static.

  // r_main : xtoviewangle, allocated at the maximum size.
  // r_things : negonearray, screenheightarray allocated max. size.

  // scr_viewsize doesn't change, neither detailLevel, but the pixels
  // per screenblock is different now, since we've changed resolution.
  R_SetViewSize();

  con.recalc = true;

  hud.st_palette = -1;

  // update automap because some screensize-dependent values
  // have to be calculated
  automap.Resize();
}


// Check for screen cmd-line parms : to force a resolution.
//
// Set the video mode to set at the 1st display loop (setmodeneeded)
//

void SCR_CheckDefaultMode()
{
  // 0 means not set at the cmd-line
  int scr_forcex = 0;
  int scr_forcey = 0;

  int p = M_CheckParm("-width");
  if (p && p < myargc-1)
    scr_forcex = atoi(myargv[p+1]);

  p = M_CheckParm("-height");
  if (p && p < myargc-1)
    scr_forcey = atoi(myargv[p+1]);

  if (scr_forcex && scr_forcey)
    {
      CONS_Printf("Using resolution: %d x %d\n", scr_forcex, scr_forcey);
      // returns -1 if not found, thus will be 0 (no mode change) if not found
      vid.setmodeneeded = I_GetVideoModeForSize(scr_forcex, scr_forcey) + 1;
    }
  else
    {
      CONS_Printf("Default resolution: %d x %d (%d bpp)\n", cv_scr_width.value,
                  cv_scr_height.value, cv_scr_depth.value);
      // see note above
      vid.setmodeneeded = I_GetVideoModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
    }
}


//added:03-02-98: sets the modenum as the new default video mode to be saved
//                in the config file
void SCR_SetDefaultMode()
{
  // remember the default screen size
  cv_scr_width.Set(vid.width);
  cv_scr_height.Set(vid.height);
  cv_scr_depth.Set(vid.BytesPerPixel * 8);
  // CV_SetValue (&cv_fullscreen, !vid.u.windowed); metzgermeister: unnecessary?
}


//-------------------------------------------------

// Retrieve the ARGB value from a palette color index
//#define V_GetColor(color)  (vid.palette[color&0xFF])

// keep a copy of the palette so that we can get the RGB
// value for a color index at any time.
void Video::LoadPalette(const char *lumpname)
{
  int i = fc.GetNumForName(lumpname);
  int palsize = fc.LumpLength(i)/3;
  if (palette)
    Z_Free(palette);
  palette = static_cast<RGB_t*>(Z_Malloc(sizeof(RGB_t)*palsize, PU_STATIC, NULL));

  RGB_t *pal = static_cast<RGB_t*>(fc.CacheLumpNum(i, PU_DAVE));
  byte *usegamma = gammatable[cv_usegamma.value];
  for (i=0; i<palsize; i++)
    {
      palette[i].r = usegamma[pal[i].r];
      palette[i].g = usegamma[pal[i].g];
      palette[i].b = usegamma[pal[i].b];
      //        if ((i&0xff) == HWR_PATCHES_CHROMAKEY_COLORINDEX)
      //            palette[i].s.alpha = 0;
      //        else
      //palette[i].alpha = 0xff;
    }

  Z_Free(pal);
}


// Set the current palette to use for palettized graphics
// (that is, most if not all of Doom's original graphics)
void Video::SetPalette(int palettenum)
{
  if (!palette)
    LoadPalette("PLAYPAL");

  currentpalette = palettenum;

  // PLAYPAL lump contains 14 different 256 color RGB palettes (28 for Hexen)
  // VB: is software gamma correction used also with OpenGL palette?

#ifndef NO_OPENGL
  if (rendermode != render_soft)
  {
    HWR.SetPalette(&palette[palettenum*256]);
  }
  else
#endif
    I_SetPalette(&palette[palettenum*256]);
}



// equivalent to LoadPalette(pal); SetPalette(0);
void Video::SetPaletteLump(const char *pal)
{
  LoadPalette(pal);
  currentpalette = 0;
#ifndef NO_OPENGL
  if (rendermode != render_soft)
  {
    HWR.SetPalette(palette);
  }
  else
#endif
    I_SetPalette(palette);
}


// returns the current palette
RGB_t *Video::GetCurrentPalette()
{
  if (!palette)
    return NULL;

  return &palette[currentpalette << 8];
}
