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
// Revision 1.1  2002/11/16 14:18:27  hurdler
// Initial revision
//
// Revision 1.8  2002/09/05 14:12:17  vberghol
// network code partly bypassed
//
// Revision 1.7  2002/08/21 16:58:35  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.6  2002/08/19 18:06:42  vberghol
// renderer somewhat fixed
//
// Revision 1.5  2002/08/08 12:01:32  vberghol
// pian engine on valmis!
//
// Revision 1.4  2002/07/01 21:00:54  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:58  vberghol
// HUD alkaa olla kunnossa
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
//
// DESCRIPTION:
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef r_main_h
#define r_main_h 1

#include "r_defs.h"

class PlayerInfo;

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

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS             16
#define LIGHTSEGSHIFT            4

#define MAXLIGHTSCALE           48
#define LIGHTSCALESHIFT         12
#define MAXLIGHTZ              128
#define LIGHTZSHIFT             20

extern lighttable_t*    scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t*    scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t*    zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int              extralight;
extern lighttable_t*    fixedcolormap;

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS            32


// Blocky/low detail mode.
//B remove this?
//  0 = high, 1 = low
extern  int             detailshift;


//
// Utility functions.
int R_PointOnSide(fixed_t x, fixed_t y, node_t* node);

int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t* line);

//angle_t R_PointToAngle(fixed_t x, fixed_t y);

angle_t R_PointToAngle2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);

//fixed_t R_PointToDist(fixed_t x, fixed_t y);

//SoM: 3/27/2000
fixed_t R_PointToDist2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1);

//fixed_t R_ScaleFromGlobalAngle (angle_t visangle);

//subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
//subsector_t *R_IsPointInSubsector (fixed_t x, fixed_t y);

void R_AddPointToBox(int x, int y, fixed_t* box);



//
// REFRESH - the actual rendering functions.
//

extern consvar_t cv_psprites;
extern consvar_t cv_perspcorr;
extern consvar_t cv_tiltview;
extern consvar_t cv_splitscreen;
extern consvar_t cv_viewsize;
extern consvar_t cv_detaillevel;
extern consvar_t cv_scalestatusbar;




// Called by startup code.
void R_Init (void);


// just sets setsizeneeded true
extern bool setsizeneeded;
void   R_SetViewSize (void);

// do it (sometimes explicitly called)
void   R_ExecuteSetViewSize (void);

//void R_SetupFrame (PlayerInfo *player);
// Called by G_Drawer.
//void R_RenderPlayerView (PlayerInfo *player);

// add commands related to engine, at game startup
void   R_RegisterEngineStuff (void);
#endif
