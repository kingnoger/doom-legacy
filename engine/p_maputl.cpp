// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief Map geometry utility functions, blockmap iterators, traces and intercepts.
///
/// BLOCKMAP iterator functions and some central PIT_* functions to use for iteration.
/// Intercepts and traces.
/// Functions for manipulating msecnode_t threads.

#include <vector>

#include "doomdef.h"
#include "g_actor.h"
#include "g_map.h"

#include "m_bbox.h"
#include "r_poly.h"
#include "r_sky.h"
#include "p_maputl.h"

#include "tables.h"
#include "z_zone.h"

//==========================================================================
//  Simple map geometry utility functions
//==========================================================================

/*!
  \defgroup g_geoutils Simple geometry utility functions

  Distances, point vs. line problems, line intercepts, bounding boxes etc.
  Most of this is 2D stuff.
*/


/// \brief Estimation of 2D vector length (not exact)
/// \ingroup g_geoutils
/*!
  Sort of octagonal norm.
*/
fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
  dx = abs(dx);
  dy = abs(dy);
  if (dx < dy)
    return dx+dy-(dx>>1);
  return dx+dy-(dy>>1);
}


/// \brief On which side of a 2D line the point is?
/// \ingroup g_geoutils
/*!
  \return side number, 0 or 1
*/
int P_PointOnLineSide(fixed_t x, fixed_t y, const line_t *line)
{
  if (!line->dx)
    {
      if (x <= line->v1->x)
	return line->dy > 0;

      return line->dy < 0;
    }
  if (!line->dy)
    {
      if (y <= line->v1->y)
	return line->dx < 0;

      return line->dx > 0;
    }

  fixed_t dx = x - line->v1->x;
  fixed_t dy = y - line->v1->y;

#if 1
  fixed_t left = (line->dy >> 16) * dx;  // shift so that it always fits in 32 bits
  fixed_t right = dy * (line->dx >> 16);
#else
  // TEST: more accurate PointOnLineSide
  Sint64 left = line->dy.value() * dx.value();
  Sint64 right = dy.value() * line->dx.value();
#endif

  if (right < left)
    return 0;               // front side
  return 1;                   // back side
}


/// \brief On which side of a 2D divline the point is?
/// \ingroup g_geoutils
/*!
  \return side number, 0 or 1
*/
int P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line)
{
  if (!line->dx)
    {
      if (x <= line->x)
	return line->dy > 0;

      return line->dy < 0;
    }
  if (!line->dy)
    {
      if (y <= line->y)
	return line->dx < 0;

      return line->dx > 0;
    }

  fixed_t dx = x - line->x;
  fixed_t dy = y - line->y;

  // try to quickly decide by looking at sign bits
  if ((line->dy.value() ^ line->dx.value() ^ dx.value() ^ dy.value())&0x80000000)
    {
      if ((line->dy.value() ^ dx.value()) & 0x80000000)
	return 1;           // (left is negative)
      return 0;
    }

#if 1
  fixed_t left = (line->dy >> 8) * (dx >> 8); // shift so result always fits in 32 bits
  fixed_t right = (dy >> 8) * (line->dx >> 8);
#else
  // TEST: more accurate PointOnDivlineSide
  Sint64 left = line->dy.value() * dx.value();
  Sint64 right = dy.value() * line->dx.value();
#endif

  if (right < left)
    return 0;               // front side
  return 1;                   // back side
}


void divline_t::MakeDivline(const line_t *li)
{
  x = li->v1->x;
  y = li->v1->y;
  dx = li->dx;
  dy = li->dy;
}


/// \brief On which side of a 2D line the bounding box is?
/// \ingroup g_geoutils
/*!
  Considers the line to be infinite in length.
  \return side number, 0 or 1, or -1 if box crosses the line
*/
int bbox_t::BoxOnLineSide(const line_t *ld)
{
  int         p1;
  int         p2;

  switch (ld->slopetype)
    {
    case ST_HORIZONTAL:
      p1 = box[BOXTOP] > ld->v1->y;
      p2 = box[BOXBOTTOM] > ld->v1->y;
      if (ld->dx < 0)
        {
	  p1 ^= 1;
	  p2 ^= 1;
        }
      break;

    case ST_VERTICAL:
      p1 = box[BOXRIGHT] < ld->v1->x;
      p2 = box[BOXLEFT] < ld->v1->x;
      if (ld->dy < 0)
        {
	  p1 ^= 1;
	  p2 ^= 1;
        }
      break;

    case ST_POSITIVE:
      p1 = P_PointOnLineSide (box[BOXLEFT], box[BOXTOP], ld);
      p2 = P_PointOnLineSide (box[BOXRIGHT], box[BOXBOTTOM], ld);
      break;

    case ST_NEGATIVE:
      p1 = P_PointOnLineSide (box[BOXRIGHT], box[BOXTOP], ld);
      p2 = P_PointOnLineSide (box[BOXLEFT], box[BOXBOTTOM], ld);
      break;
    default :
      I_Error("P_BoxOnLineSide: unknow slopetype %d\n",ld->slopetype);
      return -1;
    }

  if (p1 == p2)
    return p1;
  return -1;
}




/// \brief Finds the intercept point of two 2D line segments
/// \ingroup g_geoutils
/*!
  This is only called by the addthings and addlines traversers.
  \return fractional intercept point along the first divline
*/
fixed_t P_InterceptVector(divline_t *v2, divline_t *v1)
{
#if 0
  // FIXME TEST, new version, more accurate
  Sint64 den = v1->dy.value() * v2->dx.value() - v1->dx.value() * v2->dy.value();
  den >>= fixed_t::FBITS;
  if (den == 0)
    return 0; // parallel lines

  Sint64 num = (v2->y.value() - v1->y.value()) * v1->dx.value();
  num += -(v2->x.value() - v1->x.value()) * v1->dy.value();

  fixed_t res;
  res.setvalue(num / den);
  return res;

#elif 1
  fixed_t den = (v1->dy >> 8) * v2->dx - (v1->dx >> 8) * v2->dy;

  if (den == 0)
    return 0;
  //  I_Error ("P_InterceptVector: parallel");

  fixed_t num = ((v1->x - v2->x) >> 8) * v1->dy + ((v2->y - v1->y) >> 8) * v1->dx;

  return num / den;
#else   // UNUSED, float debug.
  float       frac,num,den;
  float       v1x,v1y,v1dx,v1dy;
  float       v2x,v2y,v2dx,v2dy;

  v1x = (float)v1->x/FRACUNIT;
  v1y = (float)v1->y/FRACUNIT;
  v1dx = (float)v1->dx/FRACUNIT;
  v1dy = (float)v1->dy/FRACUNIT;
  v2x = (float)v2->x/FRACUNIT;
  v2y = (float)v2->y/FRACUNIT;
  v2dx = (float)v2->dx/FRACUNIT;
  v2dy = (float)v2->dy/FRACUNIT;

  den = v1dy*v2dx - v1dx*v2dy;

  if (den == 0)
    return 0;       // parallel

  num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
  frac = num / den;

  return frac*FRACUNIT;
#endif
}



//==========================================================================
//  Line openings and vertical ranges
//==========================================================================


sector_t::zcheck_t sector_t::CheckZ(fixed_t z)
{
  if (z < floorheight)
    return (floorpic == skyflatnum) ? z_Sky : z_Wall;

  if (z > ceilingheight)
    return (ceilingpic == skyflatnum) ? z_Sky : z_Wall;

  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      if (z > *rover->bottomheight && z < *rover->topheight)
	return z_FFloor;
    }

  return z_Open;
}



static list<range_t> Line_openings; // sorted from low to high


list<range_t> *sector_t::FindLineOpeningsInRange(const range_t& in)
{
  Line_openings.clear();

  // start chopping the range up with sector planes

  range_t r = in;

  if (floorheight > r.low)
    r.low = floorheight;

  if (ceilingheight < r.high)
    r.high = ceilingheight;

  if (r.high <= r.low)
    return &Line_openings; // no opening

  Line_openings.push_back(r);

  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      fixed_t h = *rover->topheight;
      fixed_t l = *rover->bottomheight;

      list<range_t>::iterator a, b, o, t;

      // find range of openings [a,b) which this ffloor changes, starting from the lowest
      for (a = Line_openings.begin(); l >= a->high && a != Line_openings.end(); a++)
	; // a is the first affected one

      for (b = a; h > b->low && b != Line_openings.end(); b++)
	; // b is past-the-end

      for (o = a; o != b; )
	{
	  if (l <= o->low)
	    {
	      // cut from below
	      if (h >= o->high)
		{
		  // entire opening is closed
		  t = o;
		  o++ ;
		  Line_openings.erase(t);
		  continue; // skip o++ at end of block
		}
	      else
		o->low = h; // raise bottom
	    }
	  else if (h >= o->high)
	    {
	      // cut from above
	      o->high = l; // lower top
	    }
	  else
	    {
	      // floor splits the opening into two: [o->low, l] and [h, o->high]
	      r.low = o->low;
	      r.high = l;
	      Line_openings.insert(o, r); // inserted just before o
	      o->low = h;
	    }

	  o++;
	}
    }

  return &Line_openings;
}



range_t sector_t::FindZRange(fixed_t z)
{
  range_t r;

  // NOTE z is assumed to be always between absolute floor and ceiling
  r.low  = floorheight;
  r.high = ceilingheight;

  // see if fake floors make the range narrower
  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      fixed_t h = *rover->topheight;
      if (h <= z && h > r.low)
	r.low = h;

      h = *rover->bottomheight;
      if (h >= z && h < r.high)
	r.high = h;
    }

  return r;
}



range_t sector_t::FindZRange(const Actor *a)
{
  range_t r;

  // start with real ceil and floor
  r.high = ceilingheight;
  r.low  = floorheight;

  if (!ffloors)
    return r;

  // Check fake floors

  fixed_t thingbot = a->Feet();
  fixed_t thingtop = a->Top();

  for (ffloor_t *rover = ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      fixed_t ffcenter = (*rover->topheight + *rover->bottomheight)/2;
      fixed_t delta1 = abs(thingbot - ffcenter);
      fixed_t delta2 = abs(thingtop - ffcenter);

      // NOTE: this logic works only when max climbing height is less than Actor.height/2
	    
      if (delta1 > delta2)
	{
	  // ffloor acts as a ceiling
	  if (*rover->bottomheight < r.high)
	    r.high = *rover->bottomheight;
	}
      else
	{
	  // ffloor acts as a floor
	  if (*rover->topheight > r.low)
	    r.low = *rover->topheight;
	}
    }

  return r;
}




void line_opening_t::SubtractFromOpening(const Actor *a, sector_t *s)
{
  range_t r = s->FindZRange(a);

  // now see if the opening is changed
  if (r.high < top)
    {
      top = r.high;
      top_sky = (s->ceilingpic == skyflatnum && r.high == s->ceilingheight);
    }

  // TEST originally lowfloor is the lowest floor encountered, now it is the second-highest.
  // This makes monsters a little more bold in stairs etc.
  if (r.low > bottom)
    {
      lowfloor = bottom;
      bottom = r.low;
      bottom_sky = (s->floorpic == skyflatnum && r.low == s->floorheight);
      bottompic = s->floorpic;
    }
  else if (r.low > lowfloor)
    lowfloor = r.low;

  /*
    // original lowfloor logic
  if (r.low > bottom)
    {
      bottom = r.low;
      bottom_sky = (s->floorpic == skyflatnum && r.low == s->floorheight);
      bottompic = s->floorpic;
    }
  else if (r.low < lowfloor)
    lowfloor = r.low;
  */
}


static line_opening_t Opening;

/// Sets Opening to the window through a two sided line.
line_opening_t *line_opening_t::Get(line_t *line, Actor *thing)
{
  Opening.Reset();

  if (line->sideptr[1] == NULL)
    {
      // single sided line
      Opening.bottom = Opening.top = 0;
      return &Opening;
    }

  Opening.SubtractFromOpening(thing, line->frontsector);
  Opening.SubtractFromOpening(thing, line->backsector);

  return &Opening;
}



//==========================================================================
//   Blockmap iterators
//==========================================================================

/// \brief Iterate a blockmap cell for line_t'
/// \ingroup g_iterators
/*!
  For each line in the given mapblock, call the passed \ref g_pit PIT function.
  If the function returns false, exit with false without checking anything else.
 
  The validcount flags are used to avoid checking lines that are marked in multiple mapblocks,
  so increment validcount before the first call to BlockLinesIterator, then make one or more calls to it.
*/
bool Map::BlockLinesIterator(int x, int y, line_iterator_t func)
{
  int i;

  if (x<0 || y<0 || x>=bmapwidth || y>=bmapheight)
    return true;

  // first iterate through polyblockmap, then normal blockmap
  int cell = y*bmapwidth + x;

  polyblock_t *polyLink = PolyBlockMap[cell];

  while (polyLink)
    {
      if (polyLink->polyobj)
	{
	  if (polyLink->polyobj->validcount != validcount)
	    {
	      polyLink->polyobj->validcount = validcount;
	      seg_t **tempSeg = polyLink->polyobj->segs;
	      for (i=0; i < polyLink->polyobj->numsegs; i++, tempSeg++)
		{
		  if ((*tempSeg)->linedef->validcount == validcount)
		    continue;

		  (*tempSeg)->linedef->validcount = validcount;
		  if (!func((*tempSeg)->linedef))
		    return false;
		}
	    }
	}
      polyLink = polyLink->next;
    }

  // iterate through the blocklist
  for (Uint16 *p = bmap.index[cell]; *p != MAPBLOCK_END; p++) // index skips the initial zero marker
    {
      line_t *ld = &lines[*p];

      if (ld->validcount == validcount)
	continue;   // line has already been checked

      ld->validcount = validcount;

      if (!func(ld))
	return false;
    }
  return true;        // everything was checked
}


/// \brief Iterate a blockmap cell for Actor's
/// \ingroup g_iterators
/*!
  For each Actor in the given mapblock, call the passed \ref g_pit PIT function.
  If the function returns false, exit with false without checking anything else.
*/
bool Map::BlockThingsIterator(int x, int y, thing_iterator_t func)
{
  Actor *mobj;

  if (x<0 || y<0 || x>=bmapwidth || y>=bmapheight)
    return true;

  //added:15-02-98: check interaction (ligne de tir, ...)
  //                avec les objets dans le blocmap
  for (mobj = blocklinks[y*bmapwidth+x]; mobj; mobj = mobj->bnext)
    {
      if (!func(mobj))
	return false;
    }
  return true;
}



/// \brief Searches though the surrounding mapblocks for Actors.
/// \ingroup g_iterators
/*!
  \param[in] distance is in MAPBLOCKUNITS
*/
Actor *Map::RoughBlockSearch(Actor *center, Actor *master, int distance, int flags)
{
  // TODO this is pretty ugly. One that searches a circular area would be better...
  int count;
  Actor *target;

  int startX = (center->pos.x - bmaporgx).floor() >> MAPBLOCKBITS;
  int startY = (center->pos.y - bmaporgy).floor() >> MAPBLOCKBITS;
	
  if (startX >= 0 && startX < bmapwidth && startY >= 0 && startY < bmapheight)
    {
      if ((target = RoughBlockCheck(center, master, startY*bmapwidth+startX, flags)))
	{ // found a target right away
	  return target;
	}
    }

  for (count = 1; count <= distance; count++)
    {
      int blockX = startX-count;
      int blockY = startY-count;

      if (blockY < 0)
	blockY = 0;
      else if (blockY >= bmapheight)
	blockY = bmapheight-1;

      if (blockX < 0)
	blockX = 0;
      else if (blockX >= bmapwidth)
	blockX = bmapwidth-1;

      int blockIndex = blockY*bmapwidth+blockX;
      int firstStop = startX+count;
      if (firstStop < 0)
	continue;

      if (firstStop >= bmapwidth)
	firstStop = bmapwidth-1;

      int secondStop = startY+count;
      if (secondStop < 0)
	continue;

      if (secondStop >= bmapheight)
	secondStop = bmapheight-1;

      int thirdStop = secondStop*bmapwidth+blockX;
      secondStop = secondStop*bmapwidth+firstStop;
      firstStop += blockY*bmapwidth;
      int finalStop = blockIndex;		

      // Trace the first block section (along the top)
      for ( ; blockIndex <= firstStop; blockIndex++)
	if ((target = RoughBlockCheck(center, master, blockIndex, flags)))
	  return target;

      // Trace the second block section (right edge)
      for (blockIndex--; blockIndex <= secondStop; blockIndex += bmapwidth)
	if ((target = RoughBlockCheck(center, master, blockIndex, flags)))
	  return target;

      // Trace the third block section (bottom edge)
      for (blockIndex -= bmapwidth; blockIndex >= thirdStop; blockIndex--)
	if ((target = RoughBlockCheck(center, master, blockIndex, flags)))
	  return target;

      // Trace the final block section (left edge)
      for (blockIndex++; blockIndex > finalStop; blockIndex -= bmapwidth)
	if ((target = RoughBlockCheck(center, master, blockIndex, flags)))
	  return target;
    }
  return NULL;	
}


/// \brief Searches a single blockmap cell for Actors.
/// \ingroup g_iterators
/*!
  TODO rewrite using blockmap iterators (-> PIT_...)
*/
Actor *Map::RoughBlockCheck(Actor *center, Actor *master, int index, int flags)
{
  enum { friendly = 1, bloodsc = 2 }; // you could add more

  Actor *link;
  for (link = blocklinks[index]; link; link = link->bnext)
    if ((link->flags & MF_SHOOTABLE) &&
	(link->flags & MF_VALIDTARGET) && // meaning "monster or player"
	!(link->flags2 & MF2_DORMANT))
      {
	if (link == master)
	  continue; // don't target master

	if ((flags & friendly) &&
	    (link->owner == master)) // or his little helpers TODO teammates
	  continue;

	if (flags & bloodsc)
	  {
	    if (CheckSight(center, link))
	      {
		angle_t angle = R_PointToAngle2(master->pos.x, master->pos.y, link->pos.x, link->pos.y) - master->yaw;
		angle >>= 24;
		if (angle>226 || angle<30)
		  return link;
	      }
	  }
	else
	  {
	    if (CheckSight(center, link))
	      return link;
	  }
      }

  return NULL;
}



//==========================================================================
//  Thinker iteration
//==========================================================================

/// \brief Iterate the Thinker list
/*!
  Iterates through all the Thinkers in the Map, calling 'func' for each.
  If the function returns false, exit with false without checking anything else.
*/
bool Map::IterateThinkers(thinker_iterator_t func)
{
  Thinker *t, *n;
  for (t = thinkercap.next; t != &thinkercap; t = n)
    {
      n = t->next; // if t is removed while it thinks, its 'next' pointer will no longer be valid.
      if (!func(t))
	return false;
    }
  return true;
}


/// \brief Iterate the Actors in the Thinker list
/*!
  Iterates through all the Actors in the Map, calling 'func' for each.
  If the function returns false, exit with false without checking anything else.
*/
bool Map::IterateActors(thing_iterator_t func)
{
  Thinker *t, *n;
  for (t = thinkercap.next; t != &thinkercap; t = n)
    {
      n = t->next; // if t is removed while it thinks, its 'next' pointer will no longer be valid.
      if (t->IsOf(Actor::_type))
	if (!func(reinterpret_cast<Actor*>(t)))
	  return false;
    }
  return true;
}


//==========================================================================
//  Trace/intercept routines
//==========================================================================

trace_t trace;        ///< changed by Map::PathTraverse ONLY
static bool earlyout;

/// \brief Find lines intercepted by the trace.
/// \ingroup g_pit
/// \ingroup g_trace
/*!
  Checks if the line_t intercepts the given trace. If so, adds it to the intercepts list.
  A line is crossed if its endpoints are on opposite sides of the trace.
  Iteration is stopped if earlyout is true and a solid line is hit.
*/
static bool PIT_AddLineIntercepts(line_t *ld)
{
  int  s1, s2;

  // avoid precision problems with two routines
  if (trace.delta.x > 16 || trace.delta.y > 16
      || trace.delta.x < -16 || trace.delta.y < -16)
    {
      //Hurdler: crash here with phobia when you shoot on the door next the stone bridge
      //stack overflow???
      s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace.dl);
      s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace.dl);
    }
  else
    {
      s1 = P_PointOnLineSide (trace.start.x, trace.start.y, ld);
      s2 = P_PointOnLineSide (trace.start.x+trace.delta.x, trace.start.y+trace.delta.y, ld);
    }

  if (s1 == s2)
    return true;    // line isn't crossed

  // hit the line
  divline_t  dl;
  dl.MakeDivline(ld);
  fixed_t frac = P_InterceptVector(&trace.dl, &dl);

  if (frac < 0)
    return true;    // behind source

  // try to early out the check
  if (earlyout && frac < 1 && !ld->backsector)
    {
      return false;   // stop checking
    }

  intercept_t in;
  in.frac = frac.Float();
  in.isaline = true;
  in.line = ld;
  trace.intercepts.push_back(in);

  return true;        // continue
}


/// \brief Find Actors intercepted by the trace.
/// \ingroup g_pit
/// \ingroup g_trace
/*!
  Checks if the Actor intercepts the given trace. If so, adds it to the intercepts list.
*/
static bool PIT_AddThingIntercepts(Actor *thing)
{
  fixed_t  x1, y1, x2, y2;
  int      s1, s2;

  bool tracepositive = (trace.delta.x.value() ^ trace.delta.y.value()) > 0;

  // check a corner to corner crossection for hit
  if (tracepositive)
    {
      x1 = thing->pos.x - thing->radius;
      y1 = thing->pos.y + thing->radius;

      x2 = thing->pos.x + thing->radius;
      y2 = thing->pos.y - thing->radius;
    }
  else
    {
      x1 = thing->pos.x - thing->radius;
      y1 = thing->pos.y - thing->radius;

      x2 = thing->pos.x + thing->radius;
      y2 = thing->pos.y + thing->radius;
    }

  s1 = P_PointOnDivlineSide (x1, y1, &trace.dl);
  s2 = P_PointOnDivlineSide (x2, y2, &trace.dl);

  if (s1 == s2)
    return true;            // line isn't crossed

  divline_t dl;
  dl.x = x1;
  dl.y = y1;
  dl.dx = x2-x1;
  dl.dy = y2-y1;

  fixed_t frac = P_InterceptVector(&trace.dl, &dl);

  if (frac < 0)
    return true;            // behind source

  intercept_t in;
  in.frac = frac.Float();
  in.isaline = false;
  in.thing = thing;
  trace.intercepts.push_back(in);

  return true;                // keep going
}

/// \brief Traverses the accumulated intercepts in order of closeness up to maxfrac.
/// \ingroup g_trace
/*!
  Calls the traverser function on all intercept_t's in the
  intercepts vector, in the nearness-of-intercept order.
  \return true if the traverser function returns true for all lines
*/
bool trace_t::TraverseIntercepts(traverser_t func, float maxfrac)
{
  int count = intercepts.size();
  int i = count;

  // TODO introsort
  intercept_t *in = NULL;
  while (i--)
    {
      float dist = fixed_t::FMAX;

      for (int j = 0; j < count; j++)
	{
	  intercept_t *scan = &intercepts[j];
	  if (scan->frac < dist)
	    {
	      dist = scan->frac;
	      in = scan;
	    }
	}

      if (dist > maxfrac)
	return true;        // checked everything in range

#if 0  // UNUSED
      {
        // don't check these yet, there may be others inserted
        in = scan = intercepts;
        for (scan = intercepts ; scan<intercept_p ; scan++)
	  if (scan->frac > maxfrac)
	    *in++ = *scan;
        intercept_p = in;
        return false;
      }
#endif

      // call the traverser function on the closest intercept_t
      if (!func(in))
	return false; // don't bother going farther

      in->frac = fixed_t::FMAX; // make sure this intercept is not chosen again
    }

  return true; // everything was traversed
}



void trace_t::Init(Map *m, const vec_t<fixed_t>& v1, const vec_t<fixed_t>& v2)
{
  mp = m;
  start = v1;
  delta = v2-v1;

  vec_t<float> temp;
  temp.x = delta.x.Float();
  temp.y = delta.y.Float();
  temp.z = delta.z.Float();

  length = temp.Norm();
  sin_pitch = temp.z / length;

  intercepts.clear();

  dl.x = start.x; dl.y = start.y;
  dl.dx = delta.x; dl.dy = delta.y;

  lastz = start.z;
  frac = 0;
}



/// \brief Traces a line through the blockmap.
/// \ingroup g_trace
/*!
  Traces a line from v1 to v2 by stepping through the blockmap
  adding line/thing intercepts and then calling the traverser function for each intercept.
  \return true if the traverser function returns true for all lines
*/
bool Map::PathTraverse(const vec_t<fixed_t>& v1, const vec_t<fixed_t>& v2, int flags, traverser_t trav)
{
  // small HACK: make local copies so we can change them
  vec_t<fixed_t> p1(v1);
  vec_t<fixed_t> p2(v2);

  earlyout = flags & PT_EARLYOUT;

  validcount++;

#define MAPBLOCKSIZE (MAPBLOCKUNITS * fixed_t::UNIT)

  if (((p1.x-bmaporgx).value() & (MAPBLOCKSIZE-1)) == 0)
    p1.x += 1; // don't side exactly on a line

  if (((p1.y-bmaporgy).value() & (MAPBLOCKSIZE-1)) == 0)
    p1.y += 1; // don't side exactly on a line

  // set up the trace struct
  trace.Init(this, p1, p2);

  p1.x -= bmaporgx;
  p1.y -= bmaporgy;
  int xt1 = p1.x.floor() >> MAPBLOCKBITS;
  int yt1 = p1.y.floor() >> MAPBLOCKBITS;

  p2.x -= bmaporgx;
  p2.y -= bmaporgy;
  int xt2 = p2.x.floor() >> MAPBLOCKBITS;
  int yt2 = p2.y.floor() >> MAPBLOCKBITS;

  fixed_t     xstep, ystep;
  fixed_t     partial;
  int         mapxstep, mapystep;

  if (xt2 > xt1)
    {
      mapxstep = 1;
      partial = 1 - (p1.x >> MAPBLOCKBITS).frac();
      ystep = (p2.y-p1.y) / abs(p2.x-p1.x);
    }
  else if (xt2 < xt1)
    {
      mapxstep = -1;
      partial = (p1.x >> MAPBLOCKBITS).frac();
      ystep = (p2.y-p1.y) / abs(p2.x-p1.x);
    }
  else
    {
      mapxstep = 0;
      partial = 1;
      ystep = 256;
    }

  fixed_t yintercept = (p1.y >> MAPBLOCKBITS) + (partial * ystep);

  if (yt2 > yt1)
    {
      mapystep = 1;
      partial = 1 - (p1.y >> MAPBLOCKBITS).frac();
      xstep = (p2.x-p1.x) / abs(p2.y-p1.y);
    }
  else if (yt2 < yt1)
    {
      mapystep = -1;
      partial = (p1.y >> MAPBLOCKBITS).frac();
      xstep = (p2.x-p1.x) / abs(p2.y-p1.y);
    }
  else
    {
      mapystep = 0;
      partial = 1;
      xstep = 256;
    }

  fixed_t xintercept = (p1.x >> MAPBLOCKBITS) + (partial * xstep);

  // Step through map blocks.
  // Count is present to prevent a round off error
  // from skipping the break.
  int mapx = xt1;
  int mapy = yt1;

  for (int count = 0 ; count < 64 ; count++)
    {
      if (flags & PT_ADDLINES)
        {
	  if (!BlockLinesIterator (mapx, mapy, PIT_AddLineIntercepts))
	    return false;   // early out
        }

      if (flags & PT_ADDTHINGS)
        {
	  if (!BlockThingsIterator (mapx, mapy, PIT_AddThingIntercepts))
	    return false;   // early out
        }

      if (mapx == xt2 && mapy == yt2)
	break;

      if (yintercept.floor() == mapy)
        {
	  yintercept += ystep;
	  mapx += mapxstep;
        }
      else if (xintercept.floor() == mapx)
        {
	  xintercept += xstep;
	  mapy += mapystep;
        }

    }
  // go through the sorted list
  return trace.TraverseIntercepts(trav, 1);
}





//==========================================================================
//  Functions for manipulating msecnode_t threads
//==========================================================================

msecnode_t *msecnode_t::headsecnode = NULL; // freelist for secnodes

void msecnode_t::InitSecnodes()
{
  headsecnode = NULL;
}

// Retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.
msecnode_t *msecnode_t::GetNode()
{
  msecnode_t *node;

  if (headsecnode)
    {
      node = headsecnode;
      headsecnode = headsecnode->m_snext;
    }
  else
    node = (msecnode_t*)Z_Malloc(sizeof(*node), PU_LEVEL, NULL);

  return node;
}

// Returns a node to the freelist.
void msecnode_t::Free()
{
  m_snext = headsecnode;
  headsecnode = this;

  // TEST
  /*
  m_thing = NULL;
  m_sector = NULL;
  m_sprev = NULL;
  m_tprev = m_tnext = NULL;
  */
}

// Deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.
msecnode_t *msecnode_t::Delete()
{
  // Unlink from the Thing thread. The Thing thread begins at
  // sector_list (unavailable here) and not from m_thing->touching_sectorlist.

  if (m_tprev)
    m_tprev->m_tnext = m_tnext;
  if (m_tnext)
    m_tnext->m_tprev = m_tprev;

  // Unlink from the sector thread. This thread begins at
  // m_sector->touching_thinglist.

  if (m_sprev)
    m_sprev->m_snext = m_snext;
  else
    m_sector->touching_thinglist = m_snext;

  if (m_snext)
    m_snext->m_sprev = m_sprev;

  /*
  for (msecnode_t *p = m_sector->touching_thinglist; p; p = p->m_snext)
    if (!p->m_thing || !p->m_sector)
      I_Error("error during msecnode delete");
  */

  // Return this node to the freelist
  Free();
  return m_tnext; // unharmed by Free
}



// Searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.
msecnode_t *msecnode_t::AddToSectorlist(sector_t *s, Actor *thing, msecnode_t *seclist)
{
  msecnode_t *node = seclist;
  while (node)
    {
      if (node->m_sector == s)   // Already have a node for this sector?
	{
	  node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
	  return seclist;
	}
      node = node->m_tnext;
    }

  // Couldn't find an existing node for this sector. Add one at the head of the list.
  node = GetNode();

  //mark new nodes unvisited.
  node->visited = false;

  node->m_sector = s;       // sector
  node->m_thing  = thing;   // Actor
  node->m_tprev  = NULL;    // prev node on Thing thread
  node->m_tnext  = seclist;  // next node on Thing thread
  if (seclist)
    seclist->m_tprev = node; // set back link on Thing

  // Add new node at head of sector thread starting at s->touching_thinglist

  node->m_sprev  = NULL;    // prev node on sector thread
  node->m_snext  = s->touching_thinglist; // next node on sector thread
  if (s->touching_thinglist)
    node->m_snext->m_sprev = node;
  s->touching_thinglist = node;

  return node;
}



msecnode_t *msecnode_t::CleanSectorlist(msecnode_t *seclist)
{
  msecnode_t *node = seclist;
  while (node)
    {
      if (node->m_thing == NULL)
	{
	  if (node == seclist)
	    seclist = node->m_tnext;
	  node = node->Delete();
	}
      else
	node = node->m_tnext;
    }

  return seclist;
}
