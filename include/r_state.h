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
// Revision 1.9  2002/09/05 14:12:18  vberghol
// network code partly bypassed
//
// Revision 1.8  2002/08/19 18:06:43  vberghol
// renderer somewhat fixed
//
// Revision 1.7  2002/08/06 13:14:29  vberghol
// ...
//
// Revision 1.6  2002/08/02 20:14:52  vberghol
// p_enemy.cpp done!
//
// Revision 1.5  2002/07/13 17:56:59  vberghol
// *** empty log message ***
//
// Revision 1.4  2002/07/04 18:02:27  vberghol
// Pientä fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.3  2002/07/01 21:00:55  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:29  vberghol
// Version 133 Experimental!
//
// Revision 1.11  2001/03/21 18:24:56  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.10  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.9  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.8  2000/04/18 17:39:40  stroggonmeth
// Bug fixes and performance tuning.
//
// Revision 1.7  2000/04/08 17:29:25  stroggonmeth
// no message
//
// Revision 1.6  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.5  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.4  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.3  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#ifndef r_state_h
#define r_state_h 1

// Need data structure definitions.

#include "r_sprite.h"
#include "r_defs.h"
#include "tables.h"


#ifdef __GNUG__
#pragma interface
#endif

class PlayerPawn;

//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern fixed_t*         textureheight;

// needed for pre rendering (fracs)
extern fixed_t*         spritewidth;
extern fixed_t*         spriteoffset;
extern fixed_t*         spritetopoffset;
extern fixed_t*         spriteheight;

extern lighttable_t*    colormaps;

//SoM: 3/30/2000: Boom colormaps.
//SoM: 4/7/2000: Had to put a limit on colormaps :(
#define                 MAXCOLORMAPS 30


extern int                 num_extra_colormaps;
extern extracolormap_t     extra_colormaps[MAXCOLORMAPS];

extern int              viewwidth;
extern int              scaledviewwidth;
extern int              viewheight;

extern int              firstflat;
extern int              firstwaterflat; //added:18-02-98:WATER!

// for global animation
extern int*             flattranslation;
extern int*             texturetranslation;


// Sprite....
extern int              firstspritelump;
extern int              lastspritelump;
extern int              numspritelumps;



//
// Lookup tables for map data.
//
extern int              numsprites;
extern spritedef_t*     sprites;

/*
nowadays these are private Map class members
extern int              numvertexes;
extern vertex_t*        vertexes;

extern int              numsegs;
extern seg_t*           segs;

extern int              numsectors;
extern sector_t*        sectors;

extern int              numsubsectors;
extern subsector_t*     subsectors;

extern int              numnodes;
extern node_t*          nodes;

extern int              numlines;
extern line_t*          lines;

extern int              numsides;
extern side_t*          sides;
*/

//
// POV data.
//
//extern fixed_t          viewx;
//extern fixed_t          viewy;
//extern fixed_t          viewz;

// SoM: Portals require that certain functions use a different x and y pos
// than the actual view pos...
extern fixed_t          bspx;
extern fixed_t          bspy;

//extern angle_t          viewangle;
//extern angle_t          aimingangle;
extern angle_t          bspangle;
//extern PlayerPawn *viewplayer;

extern consvar_t        cv_allowmlook;

// ?
extern angle_t          clipangle;

extern int              viewangletox[FINEANGLES/2];
extern angle_t          xtoviewangle[MAXVIDWIDTH+1];
//extern fixed_t                finetangent[FINEANGLES/2];

extern fixed_t          rw_distance;
extern angle_t          rw_normalangle;



// angle to line origin
extern int              rw_angle1;

// Segs count?
extern int              sscount;

#endif
