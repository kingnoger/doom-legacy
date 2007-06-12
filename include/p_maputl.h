// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
#include "r_defs.h"
#include "tables.h"

using namespace std;

#define USERANGE 64

extern int validcount;



/// \brief Vertical range.
struct range_t
{
  fixed_t high;
  fixed_t low;
};



/// \brief Opening between several sectors, from Actor point of view.
struct line_opening_t
{
  fixed_t top;
  fixed_t bottom;
  fixed_t lowfloor;  ///< one floor down from bottom
  class Material *bottompic; ///< floorpic of bottom plane

  bool top_sky;      ///< top    limit is set by a sky plane
  bool bottom_sky;   ///< bottom limit is set by a sky plane

public:
  inline void Reset()
  {
    top = fixed_t::FMAX;
    bottom = lowfloor = fixed_t::FMIN;
    bottompic = NULL;
    top_sky = bottom_sky = false;
  }

  inline fixed_t Range() { return top - bottom; }
  inline fixed_t  Drop() { return bottom - lowfloor; }

  /// Shrinks the vertical opening for Actor a by Z-planes in sector s.
  void SubtractFromOpening(const class Actor *a, struct sector_t *s);

  /// Returns the opening for an Actor across a line.
  static line_opening_t *Get(struct line_t *line, Actor *thing);
};



/// \brief Describes a single intercept of a trace_t line, either an Actor or a line_t.
/// \ingroup g_trace
struct intercept_t
{
  float frac;    ///< Fractional position of the intercept along the 2D trace line.
  bool  isaline;
  union
  {
    class  Actor  *thing;
    struct line_t *line;
  };
};


/// \brief Defines a 3D line trace.
/// \ingroup g_trace
struct trace_t
{
  typedef bool (*traverser_t)(intercept_t *in);

  class Map     *mp;    ///< Necessary, since line_t's don't carry a Map *. Actors do.  
  vec_t<fixed_t> start; ///< starting point
  vec_t<fixed_t> delta; ///< == end-start
  float sin_pitch;      ///< sine of the pitch angle
  float length;         ///< == |delta|

  divline_t dl; ///< copy of the XY part

  vector<intercept_t> intercepts; ///< intercepting Actors and/or line_t's

  /// Work vars, NOTE make sure that only one func at a time changes them!
  fixed_t lastz; ///< last confirmed z-coord. 
  float   frac;  ///< last confirmed fraction of length

public:
  /// Initializes the trace.
  void Init(Map *m, const vec_t<fixed_t>& v1, const vec_t<fixed_t>& v2);

  /// Traverses the accumulated intercepts in order of closeness up to maxfrac.
  bool TraverseIntercepts(traverser_t func, float maxfrac);

  /// Returns a point along the trace, f is in [0,1].
  inline vec_t<fixed_t> Point(float f) { return start + delta * f; }

  /// Checks if trace hits a Z-plane while traversing sector s from height lastz up to fraction frac.
  bool HitZPlane(struct sector_t *s);
};

extern trace_t trace;


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
  line_opening_t op;

  Actor  *block_thing; ///< thing that blocked position (or NULL)
  line_t *block_line;  ///< line that blocked position (or NULL)
  Actor  *floor_thing; ///< thing we are climbing on

  vector<line_t*> spechit; ///< line crossings (impacts and pushes are done at once)

  bool skyimpact; ///< Did the actor collide with a sky wall?
};

extern position_check_t PosCheck;

/// variables used by movement functions to communicate
extern bool floatok;

extern class bbox_t tmb;


enum lineside_e
{
  LS_FRONT = 0,
  LS_BACK  = 1,
  LS_ON    = 2
};

int P_PointOnLineSide(const fixed_t x, const fixed_t y, const line_t *line);
lineside_e P_PointOnDivlineSide(const fixed_t x, const fixed_t y, const divline_t *line);

bool    P_LinesegsCross(const divline_t* v0, const divline_t* v1);
float P_InterceptVector(const divline_t* v0, const divline_t* v1);

#endif
