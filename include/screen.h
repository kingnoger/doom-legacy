// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.2  2002/12/03 10:23:46  smite-meister
// Video system overhaul
//
// Revision 1.6  2002/08/21 16:58:36  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.5  2002/08/06 13:14:29  vberghol
// ...
//
// Revision 1.4  2002/08/02 20:14:52  vberghol
// p_enemy.cpp done!
//
// Revision 1.3  2002/07/01 21:00:56  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:29  vberghol
// Version 133 Experimental!
//
// Revision 1.9  2001/05/16 21:21:14  bpereira
// no message
//
// Revision 1.8  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.7  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
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
// Revision 1.3  2000/04/22 20:27:35  metzgermeister
// support for immediate fullscreen switching
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//
// Video system class. Takes care of remembering and changing of video modes,
// scaling coefficients for original Doom bitmaps, software mode multiple buffering etc.
//-----------------------------------------------------------------------------


#ifndef screen_h
#define screen_h 1

// FIXME only for Win32 version, move to Win32 video interface!
//HWND        WndParent;       // handle of the application's window

#include "doomtype.h"

// FIXME 4 or 5 ?
#define NUMSCREENS    4

// we try to re-allocate a minimum of buffers for stability of the memory,
// so all the small-enough tables based on screen size, are allocated once
// and for all at the maximum size.

#define MAXVIDWIDTH     1024 //dont set this too high because actually
#define MAXVIDHEIGHT    768  // lots of tables are allocated with the MAX
                             // size.
#define BASEVIDWIDTH    320  //NEVER CHANGE THIS! this is the original
#define BASEVIDHEIGHT   200  // resolution of the graphics.

// global video state
// was struct viddef_t
class Video
{
public:
  int  modenum;       // current vidmode num indexes videomodes list
  int  width, height; // current resolution in pixels
  int  rowbytes;      // bytes per scanline of the current vidmode
  int  BytesPerPixel; // color depth: 1=256color, 2=highcolor
  int  BitsPerPixel;  // == BytesPerPixel * 8

  // software mode only
  byte  *buffer;     // invisible screens buffer
  byte  *screens[5]; // Each screen is [vid.width*vid.height];
  byte  *direct;     // linear frame buffer, or vga base mem.

  bool windowed; // not fullscreen?
  int  numpages; // ...

  int   dupx, dupy;       // scale 1,2,3 value for menus & overlays
  float fdupx, fdupy;     // same as dupx,dupy but exact value when aspect ratio isn't 320/200
  int   centerofs;       // centering for the scaled menu gfx
  int   baseratio;       // SoM: Used to get the correct value for lighting walls

  int   setmodeneeded; // video mode change needed if > 0 // (the mode number to set + 1)
  bool  recalc;        // TODO not really necessary, remove...

  RGBA_t *palette;  // local copy of the palette for V_GetColor()

private:
  // Recalc screen size dependent stuff
  void Recalc();

public:
  // Starts up video hardware, loads palette, registers consvars
  void Startup();

  // Change video mode, only at the start of a refresh.
  void SetMode();

  // palette handling.
  // gamma correction is applied as palette is loaded
  void LoadPalette(const char *lumpname);
  // Set the current RGB palette lookup to use for palettized graphics
  void SetPalette(int palettenum);
  void SetPaletteLump(const char *pal);
};

extern Video vid;

// Check parms once at startup
void SCR_CheckDefaultMode();
// Set the mode number which is saved in the config
void SCR_SetDefaultMode();


// internal additional info for vesa modes only
struct vesa_extra_t
{
  int         vesamode;         // vesa mode number plus LINEAR_MODE bit
  void       *plinearmem;      // linear address of start of frame buffer
};
// a video modes from the video modes list,
// note: video mode 0 is always standard VGA320x200.
struct vmode_t
{
  vmode_t      *pnext;
  char         *name;
  unsigned int  width;
  unsigned int  height;
  unsigned int  rowbytes;          //bytes per scanline
  unsigned int  bytesperpixel;     // 1 for 256c, 2 for highcolor
  int           windowed;          // if true this is a windowed mode
  int           numpages;
  vesa_extra_t *pextradata;       //vesa mode extra data
  int         (*setmode)(Video *lvid, struct vmode_s *pcurrentmode);
  int           misc;              //misc for display driver (r_glide.dll etc)
};

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


// ----------------
// screen variables
// ----------------

extern byte*    scr_borderpatch;   // patch used to fill the view borders

struct consvar_t;

extern consvar_t cv_scr_width;
extern consvar_t cv_scr_height;
extern consvar_t cv_scr_depth;
extern consvar_t cv_fullscreen;
extern consvar_t cv_fuzzymode;
extern consvar_t cv_usegamma;

// wait for page flipping to end or not
extern consvar_t cv_vidwait;


// from vid_vesa.c : user config video mode decided at VID_Init ();
extern int      vid_modenum;


#endif
