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
extern struct drawseg_t *drawsegs;
extern unsigned         maxdrawsegs;
extern drawseg_t*       ds_p;
extern drawseg_t*       firstnewseg;


typedef void (*drawfunc_t)(int start, int stop);


void R_ClearClipSegs();
void R_SetupClipSegs();
void R_ClearDrawSegs();

int  R_GetPlaneLight(sector_t* sector, fixed_t  planeheight, bool underside);

#endif
