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
//-----------------------------------------------------------------------------

/// \file
/// \brief Bounding boxes

#ifndef m_bbox_h
#define m_bbox_h 1

#include "m_fixed.h"


/// Bounding box coordinate order.
enum bbox_e
{
  BOXTOP,
  BOXBOTTOM,
  BOXLEFT,
  BOXRIGHT
};


/// \brief A rectangular axis-aligned bounding box
///
/// NOTE! Must remain a Plain Old Datatype (no vtable!)
struct bbox_t
{
  fixed_t box[4];

  void Clear();
  void Add(fixed_t x, fixed_t y);
  void Set(fixed_t x, fixed_t y, fixed_t r);
  void Move(fixed_t x, fixed_t y);

  inline bbox_t operator=(const bbox_t &other)
  {
    for (int i=0; i<4; i++)
      box[i] = other.box[i];
    return *this;
  };
  inline fixed_t operator[](bbox_e side) const { return box[side]; };

  bool PointInBox(fixed_t x, fixed_t y);
  bool CircleTouchBox(fixed_t x, fixed_t y, fixed_t radius);
  bool BoxTouchBox(const bbox_t &other);
  bool LineCrossesEdge(const fixed_t x1, const fixed_t y1,
		       const fixed_t x2, const fixed_t y2) const;
  int  BoxOnLineSide(const struct line_t *ld);
};

#endif
