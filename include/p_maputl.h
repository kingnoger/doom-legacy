// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.2  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   global map utility functions
//
//-----------------------------------------------------------------------------


#ifndef p_maputl_h
#define p_maputl_h 1

#include "m_fixed.h"

struct line_t;
struct msecnode_t;
class Actor;
class Map;

// P_MAP

// variables used by movement functions to communicate
extern bool     floatok;
extern fixed_t  tmfloorz;
extern Actor*  linetarget;     // who got hit (or NULL)

void P_DelSeclist(msecnode_t *p);

//
// P_MAPUTL
//

struct divline_t 
{
  fixed_t     x;
  fixed_t     y;
  fixed_t     dx;
  fixed_t     dy;
};

struct intercept_t
{
  Map *m; // ugly but necessary, since line_t's don't carry a Map *. Actors do.
  fixed_t     frac;           // along trace line
  bool     isaline;
  union
  {
    Actor  *thing;
    line_t *line;
  } d;
};


extern int              max_intercepts;
extern intercept_t*     intercepts;
extern intercept_t*     intercept_p;

void P_CheckIntercepts();

//bool P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
//		    int flags, traverser_t trav);

fixed_t P_AproxDistance (fixed_t dx, fixed_t dy);
int     P_PointOnLineSide (fixed_t x, fixed_t y, line_t* line);
int     P_BoxOnLineSide (fixed_t* tmbox, line_t* ld);
int     P_PointOnDivlineSide (fixed_t x, fixed_t y, divline_t* line);
void    P_MakeDivline (line_t* li, divline_t* dl);
fixed_t P_InterceptVector (divline_t* v2, divline_t* v1);


extern fixed_t          opentop;
extern fixed_t          openbottom;
extern fixed_t          openrange;
extern fixed_t          lowfloor;

void    P_LineOpening (line_t* linedef);


#define PT_ADDLINES     1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT     4

extern divline_t    trace;
extern fixed_t      tmbbox[4];     //p_map.c

#endif
