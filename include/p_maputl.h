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
// Revision 1.1  2002/11/16 14:18:25  hurdler
// Initial revision
//
// Revision 1.11  2002/08/14 17:07:20  vberghol
// p_map.cpp done... 3 to go
//
// Revision 1.10  2002/08/13 19:47:46  vberghol
// p_inter.cpp done
//
// Revision 1.9  2002/08/11 17:16:52  vberghol
// ...
//
// Revision 1.8  2002/08/08 12:01:32  vberghol
// pian engine on valmis!
//
// Revision 1.7  2002/08/06 13:14:28  vberghol
// ...
//
// Revision 1.6  2002/08/02 20:14:52  vberghol
// p_enemy.cpp done!
//
// Revision 1.5  2002/07/26 19:23:06  vberghol
// a little something
//
// Revision 1.4  2002/07/23 19:21:46  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:52  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:27  vberghol
// Version 133 Experimental!
//
// Revision 1.4  2000/11/02 19:49:36  bpereira
// no message
//
// Revision 1.3  2000/04/08 17:29:25  stroggonmeth
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
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
bool P_CheckSight (Actor* t1, Actor* t2);


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

//void P_UnsetThingPosition(Actor* thing);
//void P_SetThingPosition(Actor* thing);


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

//bool P_BlockLinesIterator (int x, int y, bool(*func)(line_t*) );
//bool P_BlockThingsIterator (int x, int y, bool(*func)(Actor*) );

#define PT_ADDLINES     1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT     4

extern divline_t        trace;

extern fixed_t      tmbbox[4];     //p_map.c

#endif // __P_MAPUTL__
