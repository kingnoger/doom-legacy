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
// Revision 1.4  2005/09/12 18:33:45  smite-meister
// fixed_t, vec_t
//
// Revision 1.3  2004/10/14 19:35:51  smite-meister
// automap, bbox_t
//
// Revision 1.2  2004/10/11 11:14:51  smite-meister
// map utils
//
// Revision 1.1.1.1  2002/11/16 14:18:38  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Bounding boxes

#include "doomtype.h"
#include "m_bbox.h"


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


// set temp location and boundingbox
void bbox_t::Set(fixed_t x, fixed_t y, fixed_t r)
{
  // HACK
  extern fixed_t tmx, tmy;
  tmx = x;
  tmy = y;

  box[BOXTOP]    = y + r;
  box[BOXBOTTOM] = y - r;
  box[BOXRIGHT]  = x + r;
  box[BOXLEFT]   = x - r;
}
