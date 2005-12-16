// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.22  2005/12/16 18:18:21  smite-meister
// Deus Vult BLOCKMAP fix
//
// Revision 1.21  2005/09/29 15:15:19  smite-meister
// aiming fix
//
// Revision 1.20  2005/09/13 14:23:12  smite-meister
// fixed_t fix
//
// Revision 1.19  2005/09/12 18:33:42  smite-meister
// fixed_t, vec_t
//
// Revision 1.18  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.17  2005/07/11 16:58:40  smite-meister
// msecnode_t bug fixed
//
// Revision 1.16  2005/06/05 19:32:25  smite-meister
// unsigned map structures
//
// Revision 1.15  2004/11/18 20:30:11  smite-meister
// tnt, plutonia
//
// Revision 1.14  2004/11/13 22:38:43  smite-meister
// intermission works
//
// Revision 1.13  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.12  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.11  2004/10/14 19:35:30  smite-meister
// automap, bbox_t
//
// Revision 1.10  2004/10/11 11:23:46  smite-meister
// map utils
//
// Revision 1.7  2003/11/12 11:07:22  smite-meister
// Serialization done. Map progression.
//
// Revision 1.5  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.4  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.3  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.2  2002/12/16 22:11:53  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:01  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Map geometry utility functions, blockmap iterators, traces and intercepts.
///
/// Geometric utility functions.
/// BLOCKMAP iterator functions and some central PIT_* functions to use for iteration.
/// Intercepts and traces.
/// Functions for manipulating msecnode_t threads.

#include <vector>

#include "doomdef.h"
#include "g_actor.h"
#include "g_map.h"

#include "m_bbox.h"
#include "r_poly.h"
#include "p_maputl.h"

#include "tables.h"
#include "z_zone.h"


//==========================================================================
//  Simple map geometry utility functions
//==========================================================================

// Gives an estimation of distance (not exact)
// Sort of octagonal norm.
fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
  dx = abs(dx);
  dy = abs(dy);
  if (dx < dy)
    return dx+dy-(dx>>1);
  return dx+dy-(dy>>1);
}

/// On which side of a line the point is? Returns 0 or 1.
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


// Returns 0 or 1.
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


/// copies the relevant parts of a linedef
void divline_t::MakeDivline(const line_t *li)
{
  x = li->v1->x;
  y = li->v1->y;
  dx = li->dx;
  dy = li->dy;
}



// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
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




//
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
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



// Sets Open to the window through a two sided line.
// OPTIMIZE: keep this precalculated

static line_opening_t Opening;

line_opening_t *P_LineOpening(line_t *linedef)
{
  extern Actor *tmthing;

  if (linedef->sideptr[1] == NULL)
    {
      // single sided line
      Opening.range = 0;
      return &Opening;
    }

  sector_t *front = linedef->frontsector;
  sector_t *back  = linedef->backsector;
#ifdef PARANOIA
  if(!front)
    I_Error("lindef without front");
  if(!back)
    I_Error("lindef without back");
#endif

  if (front->ceilingheight < back->ceilingheight)
    Opening.top = front->ceilingheight;
  else
    Opening.top = back->ceilingheight;

  if (front->floorheight > back->floorheight)
    {
      Opening.bottom = front->floorheight;
      Opening.lowfloor = back->floorheight;
    }
  else
    {
      Opening.bottom = back->floorheight;
      Opening.lowfloor = front->floorheight;
    }

  if (tmthing && (front->ffloors || back->ffloors))
    {
      //SoM: 3/27/2000: Check for fake floors in the sector.

      fixed_t thingbot = tmthing->Feet();
      fixed_t thingtop = thingbot + tmthing->height;

      fixed_t lowestceiling = Opening.top;
      fixed_t highestfloor = Opening.bottom;
      fixed_t highest_lowfloor = Opening.lowfloor;
      fixed_t delta1, delta2;

      // Check for frontsector's fake floors
      if (front->ffloors)
	for (ffloor_t *rover = front->ffloors; rover; rover = rover->next)
	  {
	    if (!(rover->flags & FF_SOLID))
	      continue;

	    delta1 = abs(thingbot - ((*rover->bottomheight + *rover->topheight) / 2));
	    delta2 = abs(thingtop - ((*rover->bottomheight + *rover->topheight) / 2));
	    
	    if (delta1 >= delta2)
	      {
		if (*rover->bottomheight < lowestceiling)
		  lowestceiling = *rover->bottomheight;
	      }
	    else
	      {
		if (*rover->topheight > highestfloor)
		  highestfloor = *rover->topheight;
		else if (*rover->topheight > highest_lowfloor)
		  highest_lowfloor = *rover->topheight;
	      }
	  }

      // Check for backsectors fake floors
      if (back->ffloors)
	for (ffloor_t *rover = back->ffloors; rover; rover = rover->next)
	  {
	    if (!(rover->flags & FF_SOLID))
	      continue;

	    delta1 = abs(thingbot - ((*rover->bottomheight + *rover->topheight) / 2));
	    delta2 = abs(thingtop - ((*rover->bottomheight + *rover->topheight) / 2));
	    
	    if (delta1 >= delta2)
	      {
		if (*rover->bottomheight < lowestceiling)
		  lowestceiling = *rover->bottomheight;
	      }
	    else
	      {
		if (*rover->topheight > highestfloor)
		  highestfloor = *rover->topheight;
		else if (*rover->topheight > highest_lowfloor)
		  highest_lowfloor = *rover->topheight;
	      }
	  }

      if (lowestceiling < Opening.top)
	Opening.top = lowestceiling;

      if (highestfloor > Opening.bottom)
	Opening.bottom = highestfloor;

      if (highest_lowfloor > Opening.lowfloor)
	Opening.lowfloor = highest_lowfloor;
    }

  Opening.range = Opening.top - Opening.bottom;

  return &Opening;
}



//==========================================================================
//   Blockmap iterators
//==========================================================================

// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to BlockLinesIterator, then make one or more calls
// to it.
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


// Same as previous, but iterates through things
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


//==========================================================================
//  Thinker iteration
//==========================================================================

// Iterates through all the Thinkers in the Map, calling 'func' for each.
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


//==========================================================================
//  Trace/intercept routines
//==========================================================================

static vector<intercept_t> intercepts;
divline_t   trace;
static bool earlyout;

// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.

static Map *tempMap;

static bool PIT_AddLineIntercepts(line_t *ld)
{
  int  s1, s2;

  // avoid precision problems with two routines
  if (trace.dx > 16 || trace.dy > 16
      || trace.dx < -16 || trace.dy < -16)
    {
      //Hurdler: crash here with phobia when you shoot on the door next the stone bridge
      //stack overflow???
      s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
      s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
    }
  else
    {
      s1 = P_PointOnLineSide (trace.x, trace.y, ld);
      s2 = P_PointOnLineSide (trace.x+trace.dx, trace.y+trace.dy, ld);
    }

  if (s1 == s2)
    return true;    // line isn't crossed

  // hit the line
  divline_t  dl;
  dl.MakeDivline(ld);
  fixed_t frac = P_InterceptVector(&trace, &dl);

  if (frac < 0)
    return true;    // behind source

  // try to early out the check
  if (earlyout && frac < 1 && !ld->backsector)
    {
      return false;   // stop checking
    }

  intercept_t in;
  in.m = tempMap;
  in.frac = frac;
  in.isaline = true;
  in.line = ld;
  intercepts.push_back(in);

  return true;        // continue
}



// iteration function to see if an Actor intercepts a given line (trace)
static bool PIT_AddThingIntercepts(Actor *thing)
{
  fixed_t  x1, y1, x2, y2;
  int      s1, s2;

  bool tracepositive = (trace.dx.value() ^ trace.dy.value()) > 0;

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

  s1 = P_PointOnDivlineSide (x1, y1, &trace);
  s2 = P_PointOnDivlineSide (x2, y2, &trace);

  if (s1 == s2)
    return true;            // line isn't crossed

  divline_t dl;
  dl.x = x1;
  dl.y = y1;
  dl.dx = x2-x1;
  dl.dy = y2-y1;

  fixed_t frac = P_InterceptVector (&trace, &dl);

  if (frac < 0)
    return true;            // behind source

  intercept_t in;
  in.m = tempMap;
  in.frac = frac;
  in.isaline = false;
  in.thing = thing;
  intercepts.push_back(in);

  return true;                // keep going
}


// Calls the traverser function on all intercept_t's in the
// intercepts vector, in the nearness-of-intercept order.
// Returns true if the traverser function returns true for all lines.
static bool P_TraverseIntercepts(traverser_t func, fixed_t maxfrac)
{
  int count = intercepts.size();
  int i = count;

  intercept_t *in = NULL;
  while (i--)
    {
      fixed_t dist = fixed_t::FMAX;

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


// Traces a line from x1,y1 to x2,y2 by stepping through the blockmap,
// adding line/thing intercepts and then calling the traverser function for each intercept.
// Returns true if the traverser function returns true for all lines.
bool Map::PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int flags, traverser_t trav)
{
  earlyout = flags & PT_EARLYOUT;

  validcount++;
  intercepts.clear();

#define MAPBLOCKSIZE (MAPBLOCKUNITS * fixed_t::UNIT)

  if (((x1-bmaporgx).value() & (MAPBLOCKSIZE-1)) == 0)
    x1 += 1; // don't side exactly on a line

  if (((y1-bmaporgy).value() & (MAPBLOCKSIZE-1)) == 0)
    y1 += 1; // don't side exactly on a line

  trace.x = x1;
  trace.y = y1;
  trace.dx = x2 - x1;
  trace.dy = y2 - y1;

  x1 -= bmaporgx;
  y1 -= bmaporgy;
  int xt1 = x1.floor() >> MAPBLOCKBITS;
  int yt1 = y1.floor() >> MAPBLOCKBITS;

  x2 -= bmaporgx;
  y2 -= bmaporgy;
  int xt2 = x2.floor() >> MAPBLOCKBITS;
  int yt2 = y2.floor() >> MAPBLOCKBITS;

  fixed_t     xstep, ystep;
  fixed_t     partial;
  int         mapxstep, mapystep;

  if (xt2 > xt1)
    {
      mapxstep = 1;
      partial = 1 - (x1 >> MAPBLOCKBITS).frac();
      ystep = (y2-y1) / abs(x2-x1);
    }
  else if (xt2 < xt1)
    {
      mapxstep = -1;
      partial = (x1 >> MAPBLOCKBITS).frac();
      ystep = (y2-y1) / abs(x2-x1);
    }
  else
    {
      mapxstep = 0;
      partial = 1;
      ystep = 256;
    }

  fixed_t yintercept = (y1 >> MAPBLOCKBITS) + (partial * ystep);

  if (yt2 > yt1)
    {
      mapystep = 1;
      partial = 1 - (y1 >> MAPBLOCKBITS).frac();
      xstep = (x2-x1) / abs(y2-y1);
    }
  else if (yt2 < yt1)
    {
      mapystep = -1;
      partial = (y1 >> MAPBLOCKBITS).frac();
      xstep = (x2-x1) / abs(y2-y1);
    }
  else
    {
      mapystep = 0;
      partial = 1;
      xstep = 256;
    }

  fixed_t xintercept = (x1 >> MAPBLOCKBITS) + (partial * xstep);

  // Step through map blocks.
  // Count is present to prevent a round off error
  // from skipping the break.
  int mapx = xt1;
  int mapy = yt1;

  // argh. FIXME. I couldn't think of anything else.
  // this is for PIT_AddLineIntercepts so it knows the right Map.
  tempMap = this;

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
  return P_TraverseIntercepts(trav, 1);
}




//===========================================================================
// Searches though the surrounding mapblocks for Actors.
//		distance is in MAPBLOCKUNITS
//===========================================================================

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


// TODO rewrite using blockmap iterators (-> PIT_...)
Actor *Map::RoughBlockCheck(Actor *center, Actor *master, int index, int flags)
{
  enum { friendly = 1, bloodsc = 2 }; // you could add more

  Actor *link;
  for (link = blocklinks[index]; link; link = link->bnext)
    if ((link->flags & MF_SHOOTABLE) &&
	((link->flags & MF_COUNTKILL) || (link->flags & MF_NOTMONSTER)) && // meaning "monster or player"
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
//  Functions for manipulating msecnode_t threads
//==========================================================================

static msecnode_t *headsecnode = NULL; // freelist for secnodes

void P_Initsecnode()
{
  headsecnode = NULL;
}

// Retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.
msecnode_t *P_GetSecnode()
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
void P_PutSecnode(msecnode_t *node)
{
  node->m_snext = headsecnode;
  headsecnode = node;
}

// Searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.
msecnode_t *P_AddSecnode(sector_t *s, Actor *thing, msecnode_t *nextnode)
{
  msecnode_t *node = nextnode;
  while (node)
    {
      if (node->m_sector == s)   // Already have a node for this sector?
	{
	  node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
	  return nextnode;
	}
      node = node->m_tnext;
    }

  // Couldn't find an existing node for this sector. Add one at the head of the list.
  node = P_GetSecnode();

  //mark new nodes unvisited.
  node->visited = false;

  node->m_sector = s;       // sector
  node->m_thing  = thing;     // mobj
  node->m_tprev  = NULL;    // prev node on Thing thread
  node->m_tnext  = nextnode;  // next node on Thing thread
  if (nextnode)
    nextnode->m_tprev = node; // set back link on Thing

  // Add new node at head of sector thread starting at s->touching_thinglist

  node->m_sprev  = NULL;    // prev node on sector thread
  node->m_snext  = s->touching_thinglist; // next node on sector thread
  if (s->touching_thinglist)
    node->m_snext->m_sprev = node;
  s->touching_thinglist = node;

  return node;
}


// Deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.
msecnode_t *P_DelSecnode(msecnode_t *node)
{
  msecnode_t *tp;  // prev node on thing thread
  msecnode_t *tn;  // next node on thing thread
  msecnode_t *sp;  // prev node on sector thread
  msecnode_t *sn;  // next node on sector thread

  if (node)
    {
      // Unlink from the Thing thread. The Thing thread begins at
      // sector_list and not from Actor->touching_sectorlist.

      tp = node->m_tprev;
      tn = node->m_tnext;
      if (tp)
	tp->m_tnext = tn;
      if (tn)
	tn->m_tprev = tp;

      // Unlink from the sector thread. This thread begins at
      // sector_t->touching_thinglist.

      sp = node->m_sprev;
      sn = node->m_snext;
      if (sp)
	sp->m_snext = sn;
      else
	node->m_sector->touching_thinglist = sn;

      if (sn)
	sn->m_sprev = sp;

      // Return this node to the freelist

      P_PutSecnode(node);
      return tn;
    }

  return NULL;
}


// Delete an entire sector list
void P_DelSeclist(msecnode_t *node)
{
  while (node)
    node = P_DelSecnode(node);
}
