// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.9  2005/05/31 18:04:21  smite-meister
// screenslink crash fixed
//
// Revision 1.8  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.7  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.6  2004/06/27 10:56:38  hurdler
// minor things
//
// Revision 1.5  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.4  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.3  2002/12/16 22:05:08  smite-meister
// Actor / DActor separation done!
//
// Revision 1.2  2002/12/03 10:23:46  smite-meister
// Video system overhaul
//-----------------------------------------------------------------------------

/// \file
/// \brief Video subsystem

#ifndef screen_h
#define screen_h 1

#include "doomtype.h"


#define NUMSCREENS 2

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

// we try to re-allocate a minimum of buffers for stability of the memory,
// so all the small-enough tables based on screen size, are allocated once
// and for all at the maximum size.

#define MAXVIDWIDTH     1600 //dont set this too high because actually
#define MAXVIDHEIGHT    1200 // lots of tables are allocated with the MAX size.
#define BASEVIDWIDTH    320  //NEVER CHANGE THIS! this is the original
#define BASEVIDHEIGHT   200  // resolution of the graphics.


/// \brief Video subsystem.
///
/// Takes care of video modes,
/// scaling coefficients for original Doom bitmaps,
/// software mode multiple buffering etc.
/// There's only one global instance in use, called 'vid'.
class Video
{
public:
  int  modenum;       ///< current vidmode num indexes videomodes list
  int  width, height; ///< current resolution in pixels
  int  rowbytes;      ///< bytes per scanline of the current vidmode
  int  BytesPerPixel; ///< color depth: 1=256color, 2=highcolor
  int  BitsPerPixel;  ///< == BytesPerPixel * 8

  // software mode only
  byte  *buffer;     ///< invisible screens buffer
  byte  *screens[NUMSCREENS]; ///< Each screen is [vid.width*vid.height];
  byte  *direct;     ///< linear frame buffer, or vga base mem.

  bool windowed;     ///< not fullscreen?

  int   dupx, dupy;    ///< integer scaling value (1,2,3) for menus & overlays
  float fdupx, fdupy;  ///< same as dupx,dupy but exact value when aspect ratio isn't 320/200

  int   centerofs;  ///< centering for the scaled menu gfx
  int   scaledofs;  ///< centering offset for the scaled graphics,

  //int   baseratio;       // SoM: Used to get the correct value for lighting walls //Hurdler: not used anymore

  int   setmodeneeded; ///< video mode change needed if > 0 // (the mode number to set + 1)

  RGBA_t *palette;  ///< local copy of the current palette

private:
  /// Recalc screen size dependent stuff
  void Recalc();

public:
  /// constructor
  Video();

  /// Starts up video hardware, loads palette, registers consvars.
  void Startup();

  /// Change video mode, only at the start of a refresh.
  void SetMode();

  /// Loads a set of palettes, applies gamma correction.
  void LoadPalette(const char *lumpname);

  /// Choose the current RGB palette for palettized graphics.
  void SetPalette(int palettenum);

  /// Equivalent to LoadPalette(pal); SetPalette(0);
  void SetPaletteLump(const char *pal);
};

extern Video vid;



// Check parms once at startup
void SCR_CheckDefaultMode();
// Set the mode number which is saved in the config
void SCR_SetDefaultMode();



// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

extern void     (*skycolfunc)();
extern void     (*colfunc)();
#ifdef HORIZONTALDRAW
 extern void     (*hcolfunc)();    //Fab 17-06-98
#endif
extern void     (*basecolfunc)();
extern void     (*fuzzcolfunc)();
extern void     (*transcolfunc)();
extern void     (*shadecolfunc)();
extern void     (*spanfunc)();
extern void     (*basespanfunc)();

// quick fix for tall/short skies, depending on bytesperpixel
extern void (*skydrawerfunc[2])();

#endif
