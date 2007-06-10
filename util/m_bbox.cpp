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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Bounding boxes

#include "doomtype.h"
#include "m_bbox.h"
#include "p_maputl.h"

void bbox_t::Clear()
{
  box[BOXTOP] = box[BOXRIGHT] = fixed_t::FMIN;
  box[BOXBOTTOM] = box[BOXLEFT] = fixed_t::FMAX;
}


void bbox_t::Add(fixed_t x, fixed_t y)
{
  if (x<box[BOXLEFT  ])   box[BOXLEFT  ] = x;
  if (x>box[BOXRIGHT ])   box[BOXRIGHT ] = x;

  if (y<box[BOXBOTTOM])   box[BOXBOTTOM] = y;
  if (y>box[BOXTOP   ])   box[BOXTOP   ] = y;
}


void bbox_t::Move(fixed_t x, fixed_t y)
{
  box[BOXLEFT] += x;
  box[BOXRIGHT] += x;

  box[BOXTOP] += y;
  box[BOXBOTTOM] += y;
}



bool bbox_t::PointInBox(fixed_t x, fixed_t y)
{
  if (x < box[BOXLEFT]   || x > box[BOXRIGHT] ||
      y < box[BOXBOTTOM] || y > box[BOXTOP])
    return false;

  return true;
}

bool bbox_t::CircleTouchBox(fixed_t x, fixed_t y, fixed_t radius)
{
  if (box[BOXLEFT  ]-radius > x ||
      box[BOXRIGHT ]+radius < x ||
      box[BOXBOTTOM]-radius > y ||
      box[BOXTOP   ]+radius < y)
    return false;

  return true;
}

bool bbox_t::BoxTouchBox(const bbox_t &other)
{
  if (box[BOXRIGHT] <= other[BOXLEFT]
      || box[BOXLEFT] >= other[BOXRIGHT]
      || box[BOXTOP] <= other[BOXBOTTOM]
      || box[BOXBOTTOM] >= other[BOXTOP])
    return false;

  return true;
}

// Returns true if line drawn between given points intercepts any edge
// of the box.

bool bbox_t::LineCrossesEdge(const fixed_t x1, const fixed_t y1,
			      const fixed_t x2, const fixed_t y2) const {
  if(P_LinesegsCross(x1, y1, x2, y2, 
		     box[BOXLEFT], box[BOXTOP], box[BOXLEFT], box[BOXBOTTOM]))
    return true;

  if(P_LinesegsCross(x1, y1, x2, y2, 
		     box[BOXLEFT], box[BOXBOTTOM], box[BOXRIGHT], box[BOXBOTTOM]))
    return true;

  if(P_LinesegsCross(x1, y1, x2, y2, 
		     box[BOXRIGHT], box[BOXBOTTOM], box[BOXRIGHT], box[BOXTOP]))
    return true;

  if(P_LinesegsCross(x1, y1, x2, y2, 
		     box[BOXRIGHT], box[BOXTOP], box[BOXLEFT], box[BOXTOP]))
    return true;

  return false;
}

// set temp location and boundingbox
void bbox_t::Set(fixed_t x, fixed_t y, fixed_t r)
{
  box[BOXTOP]    = y + r;
  box[BOXBOTTOM] = y - r;
  box[BOXRIGHT]  = x + r;
  box[BOXLEFT]   = x - r;
}

