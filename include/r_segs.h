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
// Revision 1.2  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.1.1.1  2002/11/16 14:18:26  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.5  2002/09/05 14:12:18  vberghol
// network code partly bypassed
//
// Revision 1.4  2002/08/19 18:06:43  vberghol
// renderer somewhat fixed
//
// Revision 1.3  2002/07/01 21:00:55  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:28  vberghol
// Version 133 Experimental!
//
// Revision 1.5  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.4  2000/11/03 02:37:36  stroggonmeth
// Fix a few warnings when compiling.
//
// Revision 1.3  2000/11/02 19:49:36  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Software render, drawing LineSegs from BSP.

#ifndef r_segs_h
#define r_segs_h 1


#ifndef MAXFFLOORS
# define MAXFFLOORS 40
#endif

/// Silhouette, needed for clipping Segs (mainly) and sprites representing things.
enum
{
  SIL_NONE   = 0,
  SIL_BOTTOM = 1,
  SIL_TOP    = 2,
  SIL_BOTH   = 3
};

struct drawseg_t
{
  seg_t*              curline;
  int                 x1;
  int                 x2;

  fixed_t             scale1;
  fixed_t             scale2;
  fixed_t             scalestep;

  // 0=none, 1=bottom, 2=top, 3=both
  int                 silhouette;

  // do not clip sprites above this
  fixed_t             bsilheight;

  // do not clip sprites below this
  fixed_t             tsilheight;

  // Pointers to lists for sprite clipping,
  //  all three adjusted so [x1] is first value.
  short*              sprtopclip;
  short*              sprbottomclip;
  short*              maskedtexturecol;

  struct visplane_t*  ffloorplanes[MAXFFLOORS];
  int                 numffloorplanes;
  ffloor_t*    thicksides[MAXFFLOORS];
  short*              thicksidecol;
  int                 numthicksides;
  fixed_t             frontscale[MAXVIDWIDTH];
};



extern lighttable_t**   walllights;

//void R_RenderMaskedSegRange( drawseg_t*    ds, int  x1, int x2);


//void R_RenderThickSideRange(drawseg_t* ds, int x1, int x2, ffloor_t*  ffloor);

//void R_StoreWallRange( int   start,int   stop );
#endif
