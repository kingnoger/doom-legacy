// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Global map utility functions.

#ifndef p_maputl_h
#define p_maputl_h 1

#include <vector>
#include "vect.h"
#include "m_fixed.h"
#include "tables.h"

using namespace std;

#define USERANGE 64

extern int validcount;



/// \brief Vertical range.
struct range_t
{
  fixed_t low;
  fixed_t high;
  //fixed_t lowfloor;
};



/// \brief Opening between several sectors, from Actor point of view.
struct line_opening_t
{
  fixed_t top;
  fixed_t bottom;
  fixed_t lowfloor;  ///< one floor down from bottom

  bool sky; ///< top limit is by a sky plane

public:
  inline fixed_t Range() { return top - bottom; }

  /// Returns the opening for an Actor across a line.
  static line_opening_t *Get(struct line_t *line, class Actor *thing);
};



/// \brief Encapsulates the XY-plane geometry of a linedef for line traces. 
/// \ingroup g_geoutils
struct divline_t 
{
  fixed_t   x, y; ///< starting point (v1)
  fixed_t dx, dy; ///< v2-v1

  /// copies the relevant parts of a linedef
  void MakeDivline(const struct line_t *li);
};



/// \brief Defines a 3D line trace.
/// \ingroup g_trace
struct trace_t
{
  vec_t<fixed_t> start; ///< starting point
  vec_t<fixed_t> delta; ///< == end-start
  float sin_pitch;      ///< sine of the pitch angle
  float length;         ///< == |delta|

  divline_t dl; ///< copy of the XY part

  /// Work var, last confirmed z-coord. NOTE make sure that only one func at a time changes this!
  fixed_t lastz;

private:
  inline void Init() { dl.x = start.x; dl.y = start.y; dl.dx = delta.x; dl.dy = delta.y; lastz = start.z; }

public:
  /// Initializes the trace.
  void Make(const vec_t<fixed_t>& v1, const vec_t<fixed_t>& v2);

  /// Returns a point along the trace, frac is in [0,1].
  inline vec_t<fixed_t> Point(float frac) { return start + delta * frac; }

  /// Checks if trace hits a Z-plane while traversing sector s up to fraction frac. lastz must be correct.
  bool HitZPlane(struct sector_t *s, float& frac, range_t& r);
};

extern trace_t trace;



/// \brief Describes a single intercept of a trace line, either an Actor or a line_t
/// \ingroup g_trace
struct intercept_t
{
  class Map    *m; ///< Ugly but necessary, since line_t's don't carry a Map *. Actors do.
  fixed_t    frac; ///< Fractional position of the intercept along the 2D trace line.
  bool    isaline;
  union
  {
    class  Actor  *thing;
    struct line_t *line;
  };
};



/// \brief Flags for Map::PathTraverse
/// \ingroup g_trace
enum
{
  PT_ADDLINES  = 0x1,
  PT_ADDTHINGS = 0x2,
  PT_EARLYOUT  = 0x4
};




/// \brief Actor::CheckPosition() results
/// \ingroup g_collision
struct position_check_t
{
  fixed_t  floorz;   ///< highest floor
  fixed_t  ceilingz; ///< lowest ceiling
  fixed_t  dropoffz; ///< lowest floor
  int      floorpic; ///< floorpic of highest floor
  //fixed_t  sectorfloorz, sectorceilingz; ///< not counting fake floors

  // thing or line that blocked the position (NULL if not blocked)
  // block_thing is set only if an actual collision takes place, and iteration is stopped (z and flags included)
  class  Actor  *block_thing;
  struct line_t *block_line;

  vector<line_t*> spechit; ///< line crossings (impacts and pushes are done at once)

  /// Keep track of the line that lowers the ceiling, so missiles don't explode against sky hack walls.
  bool sky;
};

extern position_check_t PosCheck;

/// variables used by movement functions to communicate
extern bool floatok;

extern class bbox_t tmb;



fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
int     P_PointOnLineSide(fixed_t x, fixed_t y, const line_t *line);
int     P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line);
fixed_t P_InterceptVector(divline_t* v2, divline_t* v1);

inline angle_t R_PointToAngle2(const vec_t<fixed_t>& a, const vec_t<fixed_t>& b)
{
  return R_PointToAngle2(a.x, a.y, b.x, b.y);
}

inline fixed_t P_XYdist(const vec_t<fixed_t>& a, const vec_t<fixed_t>& b)
{
  return P_AproxDistance(a.x - b.x, a.y - b.y);
}


#endif
