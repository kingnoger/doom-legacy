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
// Revision 1.3  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.2  2004/07/25 20:18:47  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.4  2002/08/19 18:06:42  vberghol
// renderer somewhat fixed
//
// Revision 1.3  2002/07/01 21:00:53  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:28  vberghol
// Version 133 Experimental!
//
// Revision 1.8  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.7  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.6  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.5  2000/08/31 14:30:56  bpereira
// no message
//
// Revision 1.4  2000/08/10 14:58:07  ydario
// OS/2 port
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
/// \brief Software renderer, BSP traversal and handling.

#ifndef r_bsp_h
#define r_bsp_h 1

#include "r_defs.h"

extern seg_t*           curline;
extern side_t*          sidedef;
extern line_t*          linedef;
extern sector_t*        frontsector;
extern sector_t*        backsector;

extern bool          skymap;

// faB: drawsegs are now allocated on the fly ... see r_segs.c
// extern drawseg_t*       drawsegs;
//SoM: 3/26/2000: Use boom code.
extern struct drawseg_t *drawsegs;
extern unsigned         maxdrawsegs;
extern drawseg_t*       ds_p;
extern drawseg_t*       firstnewseg;


typedef void (*drawfunc_t) (int start, int stop);


// BSP?
void R_ClearClipSegs();
void R_SetupClipSegs();
void R_ClearDrawSegs();


int  R_GetPlaneLight(sector_t* sector, fixed_t  planeheight, bool underside);

#endif
