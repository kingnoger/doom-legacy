// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.7  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.6  2004/12/31 16:19:40  smite-meister
// alpha fixes
//
// Revision 1.5  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.4  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.3  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.2  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.1.1.1  2002/11/16 14:18:27  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.6  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.5  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Software renderer state variables. Lights.


#ifndef r_main_h
#define r_main_h 1

#include "r_defs.h"

extern  int     viewangleoffset;

//
// POV related.
//
extern fixed_t          viewcos;
extern fixed_t          viewsin;

extern int              viewwidth;
extern int              viewheight;
extern int              viewwindowx;
extern int              viewwindowy;



extern int              centerx;
extern int              centery;

extern int      centerypsp;

extern fixed_t          centerxfrac;
extern fixed_t          centeryfrac;
extern fixed_t          projection;
extern fixed_t          projectiony;    //added:02-02-98:aspect ratio test...

extern int              validcount;

extern int              linecount;
extern int              loopcount;

extern int      framecount;



//==========================================
//               Lights
//==========================================


// This could be wider for >8 bit display.
// Indeed, true color support is posibble
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef byte lighttable_t;



//Hurdler: 04/12/2000: for now, only used in hardware mode
//                     maybe later for software as well?
//                     that's why it's moved here
struct light_t
{
  Uint16  type;           // light,... (cfr #define in hwr_light.c)

  float   light_xoffset;
  float   light_yoffset;  // y offset to adjust corona's height

  Uint32  corona_color;   // color of the light for static lighting
  float   corona_radius;  // radius of the coronas

  Uint32  dynamic_color;  // color of the light for dynamic lighting
  float   dynamic_radius; // radius of the light ball
  float   dynamic_sqrradius; // radius^2 of the light ball
};

struct lightmap_t
{
  float       s[2], t[2];
  light_t    *light;
  lightmap_t *next;
};


/// \brief Information for shadows casted by 3D floors.
///
/// This information is contained inside the sector_t and is used as the base
/// information for casted shadows.
struct lightlist_t
{
  fixed_t           height;
  short            *lightlevel;
  fadetable_t      *extra_colormap;
  struct ffloor_t  *caster;
};


/// Used for rendering walls with shadows cast on them.
struct r_lightlist_t
{
  fixed_t           height;
  fixed_t           heightstep;
  fixed_t           botheight;
  fixed_t           botheightstep;
  short             lightlevel;
  fadetable_t      *extra_colormap;
  lighttable_t     *rcolormap;
  int               flags;
};




//SoM: 3/30/2000: Boom colormaps.
//SoM: 4/7/2000: Had to put a limit on colormaps :(
#define                 MAXCOLORMAPS 30


// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS             16
#define LIGHTSEGSHIFT            4

#define MAXLIGHTSCALE           48
#define LIGHTSCALESHIFT         4 // was 12
#define MAXLIGHTZ              128
#define LIGHTZSHIFT              4 // without any fixed_t stuff

/// Currently used fadetable
extern lighttable_t *base_colormap;

/// Precalculated fadetable offsets
extern int  scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern int  scalelightfixed[MAXLIGHTSCALE];
extern int  zlight[LIGHTLEVELS][MAXLIGHTZ];
extern int  fixedcolormap;

extern int              extralight;


// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS            32




// Blocky/low detail mode. remove this?
// 0 = high, 1 = low
extern int detailshift;



// Utility functions.

int R_PointOnSide(fixed_t x, fixed_t y, node_t* node);
int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t* line);



// REFRESH - the actual rendering functions.

// initializes the client part of the renderer
void R_Init();

// just sets setsizeneeded true
extern bool setsizeneeded;
void   R_SetViewSize();

// do it (sometimes explicitly called)
void   R_ExecuteSetViewSize();

// add commands related to engine, at game startup
void   R_RegisterEngineStuff();

#endif
