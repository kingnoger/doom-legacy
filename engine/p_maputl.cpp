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

// Returns 0 or 1
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

  fixed_t dx = (x - line->v1->x);
  fixed_t dy = (y - line->v1->y);

  fixed_t left = FixedMul (line->dy>>FRACBITS , dx);
  fixed_t right = FixedMul (dy , line->dx>>FRACBITS);

  if (right < left)
    return 0;               // front side
  return 1;                   // back side
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


// Returns 0 or 1.
int P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line)
{
  fixed_t     dx;
  fixed_t     dy;
  fixed_t     left;
  fixed_t     right;

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

  dx = (x - line->x);
  dy = (y - line->y);

  // try to quickly decide by looking at sign bits
  if ((line->dy ^ line->dx ^ dx ^ dy)&0x80000000)
    {
      if ((line->dy ^ dx) & 0x80000000)
	return 1;           // (left is negative)
      return 0;
    }

  left = FixedMul (line->dy>>8, dx>>8);
  right = FixedMul (dy>>8 , line->dx>>8);

  if (right < left)
    return 0;               // front side
  return 1;                   // back side
}


void P_MakeDivline(line_t *li, divline_t *dl)
{
  dl->x = li->v1->x;
  dl->y = li->v1->y;
  dl->dx = li->dx;
  dl->dy = li->dy;
}



//
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
fixed_t P_InterceptVector(divline_t *v2, divline_t *v1)
{
#if 1
  fixed_t     frac;
  fixed_t     num;
  fixed_t     den;

  den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

  if (den == 0)
    return 0;
  //  I_Error ("P_InterceptVector: parallel");

  num = FixedMul ((v1->x - v2->x)>>8 ,v1->dy)
    + FixedMul ((v2->y - v1->y)>>8, v1->dx);

  frac = FixedDiv (num , den);

  return frac;
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



// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated

fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t lowfloor;

void P_LineOpening(line_t *linedef)
{
  extern Actor *tmthing;

  if (linedef->sidenum[1] == -1)
    {
      // single sided line
      openrange = 0;
      return;
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
    opentop = front->ceilingheight;
  else
    opentop = back->ceilingheight;

  if (front->floorheight > back->floorheight)
    {
      openbottom = front->floorheight;
      lowfloor = back->floorheight;
    }
  else
    {
      openbottom = back->floorheight;
      lowfloor = front->floorheight;
    }

  if (tmthing && (front->ffloors || back->ffloors))
    {
      //SoM: 3/27/2000: Check for fake floors in the sector.

      fixed_t thingbot = tmthing->z;
      fixed_t thingtop = thingbot + tmthing->height;

      fixed_t lowestceiling = opentop;
      fixed_t highestfloor = openbottom;
      fixed_t highest_lowfloor = lowfloor;
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

      if (lowestceiling < opentop)
	opentop = lowestceiling;

      if (highestfloor > openbottom)
	openbottom = highestfloor;

      if (highest_lowfloor > lowfloor)
	lowfloor = highest_lowfloor;
    }

  openrange = opentop - openbottom;
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
  int offset = y*bmapwidth + x;

  polyblock_t *polyLink = PolyBlockMap[offset];

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

  offset = blockmap[offset];

  //Hurdler: FIXME: this a temporary "fix" for the bug with phobia...
  //                ... but it's not correct!!!!! 
  if (offset < 0)
    {
      static int first = 1;
      if (first)
	{
	  CONS_Printf("Warning: this map has reached a limit of the doom engine.\n");
	  first = 0;
	}
      return true;
    }

  for (short  *p = &blockmaplump[offset] ; *p != -1 ; p++)
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
  int                 s1;
  int                 s2;
  fixed_t             frac;
  divline_t           dl;

  // avoid precision problems with two routines
  if (trace.dx > FRACUNIT*16
       || trace.dy > FRACUNIT*16
       || trace.dx < -FRACUNIT*16
       || trace.dy < -FRACUNIT*16)
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
  P_MakeDivline (ld, &dl);
  frac = P_InterceptVector (&trace, &dl);

  if (frac < 0)
    return true;    // behind source

  // try to early out the check
  if (earlyout
      && frac < FRACUNIT
      && !ld->backsector)
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

  bool tracepositive = (trace.dx ^ trace.dy) > 0;

  // check a corner to corner crossection for hit
  if (tracepositive)
    {
      x1 = thing->x - thing->radius;
      y1 = thing->y + thing->radius;

      x2 = thing->x + thing->radius;
      y2 = thing->y - thing->radius;
    }
  else
    {
      x1 = thing->x - thing->radius;
      y1 = thing->y - thing->radius;

      x2 = thing->x + thing->radius;
      y2 = thing->y + thing->radius;
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
      fixed_t dist = MAXINT;

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

      in->frac = MAXINT; // make sure this intercept is not chosen again
    }

  return true; // everything was traversed
}


// Traces a line from x1,y1 to x2,y2 by stepping through the blockmap,
// adding line/thing intercepts and then calling the traverser function for each intercept.
// Returns true if the traverser function returns true for all lines.
bool Map::PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int flags, traverser_t trav)
{
  fixed_t     xstep;
  fixed_t     ystep;

  fixed_t     partial;
  int         mapxstep;
  int         mapystep;

  earlyout = flags & PT_EARLYOUT;

  validcount++;
  intercepts.clear();

  if (((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
    x1 += FRACUNIT; // don't side exactly on a line

  if (((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
    y1 += FRACUNIT; // don't side exactly on a line

  trace.x = x1;
  trace.y = y1;
  trace.dx = x2 - x1;
  trace.dy = y2 - y1;

  x1 -= bmaporgx;
  y1 -= bmaporgy;
  fixed_t xt1 = x1>>MAPBLOCKSHIFT;
  fixed_t yt1 = y1>>MAPBLOCKSHIFT;

  x2 -= bmaporgx;
  y2 -= bmaporgy;
  fixed_t xt2 = x2>>MAPBLOCKSHIFT;
  fixed_t yt2 = y2>>MAPBLOCKSHIFT;

  if (xt2 > xt1)
    {
      mapxstep = 1;
      partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
      ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
  else if (xt2 < xt1)
    {
      mapxstep = -1;
      partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
      ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
  else
    {
      mapxstep = 0;
      partial = FRACUNIT;
      ystep = 256*FRACUNIT;
    }

  fixed_t yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);


  if (yt2 > yt1)
    {
      mapystep = 1;
      partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
      xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
  else if (yt2 < yt1)
    {
      mapystep = -1;
      partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
      xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
  else
    {
      mapystep = 0;
      partial = FRACUNIT;
      xstep = 256*FRACUNIT;
    }
  fixed_t xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);

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
	  if (!BlockLinesIterator (mapx, mapy,PIT_AddLineIntercepts))
	    return false;   // early out
        }

      if (flags & PT_ADDTHINGS)
        {
	  if (!BlockThingsIterator (mapx, mapy,PIT_AddThingIntercepts))
	    return false;   // early out
        }

      if (mapx == xt2 && mapy == yt2)
	break;

      if ((yintercept >> FRACBITS) == mapy)
        {
	  yintercept += ystep;
	  mapx += mapxstep;
        }
      else if ((xintercept >> FRACBITS) == mapx)
        {
	  xintercept += xstep;
	  mapy += mapystep;
        }

    }
  // go through the sorted list
  return P_TraverseIntercepts (trav, FRACUNIT);
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

  int startX = (center->x - bmaporgx)>>MAPBLOCKSHIFT;
  int startY = (center->y - bmaporgy)>>MAPBLOCKSHIFT;
	
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
		angle_t angle = R_PointToAngle2(master->x, master->y, link->x, link->y) - master->angle;
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
  return(node);
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
	  return(nextnode);
	}
      node = node->m_tnext;
    }

  // Couldn't find an existing node for this sector. Add one at the head
  // of the list.

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
  return(node);
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
      return(tn);
    }
  return NULL;
}


// Delete an entire sector list
void P_DelSeclist(msecnode_t *node)
{
  while (node)
    node = P_DelSecnode(node);
}
