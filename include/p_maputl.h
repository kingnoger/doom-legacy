// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// $Log$
// Revision 1.10  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.9  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.8  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.7  2004/11/04 21:12:54  smite-meister
// save/load fixed
//
// Revision 1.6  2004/10/17 02:01:39  smite-meister
// bots!
//
// Revision 1.5  2004/10/14 19:35:50  smite-meister
// automap, bbox_t
//
// Revision 1.4  2004/10/11 11:13:42  smite-meister
// map utils
//
// Revision 1.3  2004/09/13 20:43:31  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.2  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Global map utility functions

#ifndef p_maputl_h
#define p_maputl_h 1

#include <vector>
#include "vect.h"
#include "m_fixed.h"
#include "tables.h"


#define USERANGE 64


extern int validcount;

// P_MAP
// variables used by movement functions to communicate
extern bool    floatok;
extern fixed_t tmfloorz, tmceilingz;
extern class Actor *linetarget;

extern std::vector<struct line_t *> spechit;

struct position_check_t
{
  Actor  *thing;
  line_t *line;
};

extern position_check_t Blocking;
extern line_t *ceilingline;


// P_MAPUTL

void P_DelSeclist(struct msecnode_t *p);

struct divline_t 
{
  fixed_t   x, y;
  fixed_t dx, dy;
};

struct intercept_t
{
  class Map    *m; // ugly but necessary, since line_t's don't carry a Map *. Actors do.
  fixed_t    frac; // along trace line
  bool    isaline;
  union
  {
    Actor  *thing;
    line_t *line;
  };
};



fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
int     P_PointOnLineSide(fixed_t x, fixed_t y, const line_t *line);
int     P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line);
void    P_MakeDivline(line_t* li, divline_t* dl);
fixed_t P_InterceptVector(divline_t* v2, divline_t* v1);


struct line_opening_t
{
  fixed_t top, bottom, range, lowfloor;
};

line_opening_t *P_LineOpening(line_t *linedef);


#define PT_ADDLINES     1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT     4

extern divline_t trace;
extern class bbox_t tmb;




inline angle_t R_PointToAngle2(const vec_t<fixed_t>& a, const vec_t<fixed_t>& b)
{
  return R_PointToAngle2(a.x, a.y, b.x, b.y);
}

inline fixed_t P_XYdist(const vec_t<fixed_t>& a, const vec_t<fixed_t>& b)
{
  return P_AproxDistance(a.x - b.x, a.y - b.y);
}

#endif
