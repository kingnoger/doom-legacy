// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Polyobjects.

#ifndef r_poly_h
#define r_poly_h 1

#include "r_defs.h"

/// \brief Physical Polyobject definition.
///
/// Does not include the Thinker logic, which resides in polyobject_t.
struct polyobj_t
{
  int              numsegs;     ///< how many segs it consists of
  struct seg_t   **segs;        ///< pointers to the segs
  mappoint_t       spawnspot;   ///< spawn spot coords, also sound source
  struct vertex_t *originalPts; ///< base for the rotations, origin is the anchor spot
  angle_t  angle;
  int      tag;          ///< PO number
  int      bbox[4];      ///< bounding box in blockmap coordinates
  int      validcount;
  bool     crush;        ///< should the polyobj crush mobjs?
  unsigned seqType;      ///< sound sequence
  //fixed_t size; // polyobj size (area of POLY_AREAUNIT == size of FRACUNIT)
  class polyobject_t *specialdata; ///< pointer to a Thinker, if the poly is moving
};

/// \brief Polyblockmap list element.
///
/// One polyobj can be linked to many adjacent blockmap cells.
struct polyblock_t
{
  polyobj_t *polyobj;
  polyblock_t *prev;
  polyblock_t *next;
};

#endif
