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
// Revision 1.2  2004/10/11 11:13:42  smite-meister
// map utils
//
// Revision 1.1.1.1  2002/11/16 14:18:24  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Bounding boxes

#ifndef m_bbox_h
#define m_bbox_h 1

#include "m_fixed.h"


/// Bounding box coordinate order.
enum
{
  BOXTOP,
  BOXBOTTOM,
  BOXLEFT,
  BOXRIGHT
};


/// \brief A rectangular axis-aligned bounding box
class bbox_t
{
public:
  fixed_t box[4];

  void Clear();
  void Add(fixed_t x, fixed_t y);
  void Set(fixed_t x, fixed_t y, fixed_t r);

  bool PointInBox(fixed_t x, fixed_t y);
  bool CircleTouchBox(fixed_t x, fixed_t y, fixed_t radius);
  int  BoxOnLineSide(struct line_t *ld);
};

#endif
