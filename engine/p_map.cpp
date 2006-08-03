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
/// \brief Movement, collision handling. Shooting and aiming.

/*!
  \defgroup g_iterators Map geometry iterator functions

  Functions and methods used to iterate through Map geometry.
 */

/*!
  \defgroup g_pit Iterator functions for Actors and line_t's 
  \ingroup g_iterators

  The PIT functions: Unary functors for Actors and line_t's.
  Return false to stop iteration, true to continue.
 */

/*!
  \defgroup g_trace Trace functions
  \ingroup g_iterators

  A trace is a line segment drawn through the Map geometry
  from point A to point B. It may be intercepted by linedefs and Actors.
  These intercept events are recorded using intercept_t.
  Afterwards, the intercepts may be processed using a traverser function.

  The trace system has been modified. Now 'slope' always means tan(angle),
  for sin(angle) we use 'sine'.
 */

/*!
  \defgroup g_ptr Traverser functions for intercept_t's
  \ingroup g_trace

  The PTR functions: Unary functors for intercept_t
  Return false to stop traversal, true to continue.
 */


#include <math.h>
#include "doomdef.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "g_damage.h"
#include "command.h"
#include "p_maputl.h"
#include "m_bbox.h"
#include "m_random.h"

#include "p_enemy.h"

#include "r_render.h"
#include "r_sky.h"
#include "r_splats.h"

#include "sounds.h"
#include "tables.h"
#include "z_zone.h"

extern int boomsupport;

// TODO add z parameter to ALL movement/clipping functions (you can always use ONFLOORZ as a default)

static Actor    *tmthing; // cp,


// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
bool     floatok; // A::Trymove, DA:PMove

static bool interact; // touch or just see if it fits?

// checkposition data (z properties in the XY footprint of the Actor)
/*
static struct
{
  fixed_t  floorz;   ///< highest floor
  fixed_t  ceilingz; ///< lowest ceiling
  fixed_t  dropoffz; ///< lowest floor
  int      floorpic; ///< floorpic of highest floor
  //fixed_t  sectorfloorz, sectorceilingz;

} tm;
*/

fixed_t tmfloorz;   ///< highest floor
fixed_t tmceilingz; ///< lowest ceiling
fixed_t tmdropoffz; ///< lowest floor
int     tmfloorpic; ///< floorpic of highest floor
fixed_t tmsectorfloorz, tmsectorceilingz;


Actor *tmfloorthing; // the thing corresponding to tmfloorz or NULL if tmfloorz is from a sector FIXME


//=======================================================
//   Stuff set by PIT_CheckLine() and PIT_CheckThing
//=======================================================
vector<line_t *>  spechit; // cp
position_check_t Blocking; // thing or line that blocked the position (NULL if not blocked) cp
// Blocking.thing is set only if an actual collision takes place, and iteration is stopped (z and flags included)


// keep track of the line that lowers the ceiling, so missiles don't explode against sky hack walls
line_t *ceilingline; // cp



bbox_t tmb; // a bounding box; checkposition()
fixed_t tmx, tmy; // mostly used here, set together with tmb


const fixed_t MAXSTEPMOVE = 24;
const fixed_t MAXWATERSTEPMOVE = 37;

// Attempt to move to a new position,
// crossing special lines in the way.
// TODO verify onmobj logic
bool Actor::TryMove(fixed_t nx, fixed_t ny, bool allowdropoff)
{
  //  max Z move up or down without jumping
  //  above this, a heigth difference is considered a 'dropoff'
  floatok = false;

  // TODO the problems with the collision check code are
  // 1) no traces are used, rather blockmapcells are iterated one by one and we stop
  //    when the first blocking thing/line is found -> there might be others we actually hit first
  // 2) z fit is only checked later
  // 3) if no z fit, all accumulated impacts and pushes are processed. if z fit ok, all crossings are processed.
  if (!CheckPosition(nx, ny, true))
    {
      // blocked by an unpassable line or an Actor
      if (Blocking.thing)
	{
	  // TODO try to climb on top of it
	  /*
	  (
	  Blocking.thing->Top() - Feet() > MAXSTEPMOVE ||
	  Blocking.thing->subsector->sector->ceilingheight - Blocking.thing->Top() < height ||
	  tmceilingz - Blocking.thing->Top() < height)
	  */
	  return false;
	}

      // must be a blocking line
      CheckLineImpact();
      return false;
    }

  // handle z-lineclip and spechit
  if (!(flags & MF_NOCLIPLINE))
    {
      // do we fit in the z direction?
      if (tmceilingz - tmfloorz < height)
	{
	  CheckLineImpact();
	  return false;
	}

      floatok = true; // if we float to the correct height first, we will fit

      if (eflags & MFE_FLY)
	{
	  if (Top() > tmceilingz)
	    {
	      vel.z = -8;
	      return false;
	    }
	  else if (Feet() < tmfloorz && tmfloorz - tmdropoffz > MAXSTEPMOVE)
	    {
	      vel.z = 8;
	      return false;
	    }
	}

      // do we hit the upper texture?
      if (Top() > tmceilingz &&
	  !(flags2 & MF2_CEILINGHUGGER)) // ceilinghuggers step down any amount
	{
	  CheckLineImpact();
	  return false; // must lower itself to fit
	}

      // do we hit the lower texture without being able to climb the step?
      if (tmfloorz > Feet() &&
	  !(flags2 & MF2_FLOORHUGGER)) // floorhuggers step up any amount
	{
	  fixed_t maxstep = MAXSTEPMOVE;

	  // jump out of water
	  if ((eflags & MFE_UNDERWATER) && (eflags & MFE_TOUCHWATER))
	    maxstep = 37;

	  if (flags & MF_MISSILE || // missiles do not step up
	      tmfloorz - Feet() > maxstep)
	    {
	      CheckLineImpact();
	      return false;       // too big a step up
	    }
	}

      // are we afraid of the dropoff?
      if (!allowdropoff)
	if (tmfloorz - tmdropoffz > MAXSTEPMOVE && !tmfloorthing &&
	    !(flags & MF_DROPOFF) && !(eflags & MFE_BLASTED))
	  return false; // don't go over a dropoff (unless blasted)

      // are we unable to leave the floor texture? (water monsters)
      if (flags2 & MF2_CANTLEAVEFLOORPIC
	  && (tmfloorpic != subsector->sector->floorpic || tmfloorz != Feet()))
	return false;
    }

  // the move is ok, so link the thing into its new position
  UnsetPosition();

  fixed_t oldx, oldy;
  oldx = pos.x;
  oldy = pos.y;
  pos.x = nx;
  pos.y = ny;

  //added:28-02-98:
  if (tmfloorthing)
    eflags &= ~MFE_ONGROUND;  //not on real floor
  else
    eflags |= MFE_ONGROUND;

  SetPosition();

  // Heretic fake water...
  if ((flags2 & MF2_FOOTCLIP) &&
      (subsector->sector->floortype >= FLOOR_LIQUID) &&
      Feet() == subsector->sector->floorheight)
    floorclip = FOOTCLIPSIZE;
  else
    floorclip = 0;

  // if any special lines were hit, do the effect
  if (!(flags & (MF_NOCLIPLINE | MF_NOTRIGGER)))
    {
      while (spechit.size())
        {
	  // see if the line was crossed
	  line_t *ld = spechit.back();
	  spechit.pop_back();

	  int side = P_PointOnLineSide(pos.x, pos.y, ld);
	  int oldside = P_PointOnLineSide(oldx, oldy, ld);
	  if (side != oldside)
            {
	      if (ld->special)
		{
		  if (flags & MF_NOTMONSTER || ld->flags & ML_MONSTERS_CAN_ACTIVATE)
		    mp->ActivateLine(ld, this, oldside, SPAC_CROSS);
		  else if (flags2 & MF2_MCROSS)
		    mp->ActivateLine(ld, this, oldside, SPAC_MCROSS);
		  else if (flags2 & MF2_PCROSS)
		    mp->ActivateLine(ld, this, oldside, SPAC_PCROSS);
		}
            }
        }
    }

  return true;
}



//=====================================================================
//              Actor position checking and setting
//=====================================================================

// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.

void Actor::UnsetPosition()
{
  //extern msecnode_t *sector_list;
  int blockx, blocky;

  if (! (flags & MF_NOSECTOR))
    {
      // inert things don't need to be in blockmap?
      // unlink from subsector
      if (snext)
	snext->sprev = sprev;

      if (sprev)
	sprev->snext = snext;
      else
	subsector->sector->thinglist = snext;
#ifdef PARANOIA
      sprev = snext = NULL;
#endif
      //SoM: 4/7/2000
      //
      // Save the sector list pointed to by touching_sectorlist.
      // In P_SetThingPosition, we'll keep any nodes that represent
      // sectors the Thing still touches. We'll add new ones then, and
      // delete any nodes for sectors the Thing has vacated. Then we'll
      // put it back into touching_sectorlist. It's done this way to
      // avoid a lot of deleting/creating for nodes, when most of the
      // time you just get back what you deleted anyway.
      //
      // If this Thing is being removed entirely, then the calling
      // routine will clear out the nodes in sector_list.

      // smite-meister: This is because normally this function is used in a unset/set sequence.
      // the subsequent set requires that sector_list is preserved...
    }

  if (! (flags & MF_NOBLOCKMAP))
    {
      // inert things don't need to be in blockmap
      // unlink from block map
      if (bnext)
	bnext->bprev = bprev;

      if (bprev)
	bprev->bnext = bnext;
      else
        {
	  blockx = (pos.x - mp->bmaporgx).floor() >> MAPBLOCKBITS;
	  blocky = (pos.y - mp->bmaporgy).floor() >> MAPBLOCKBITS;

	  if (blockx>=0 && blockx < mp->bmapwidth && blocky>=0 && blocky < mp->bmapheight)
	    mp->blocklinks[blocky * mp->bmapwidth + blockx] = bnext;
        }

      bprev = bnext = NULL;
    }
}

/*
bool Map::SetBMlink(fixed_t x, fixed_t y, Actor *a)
{
  int blockx = (x - bmaporgx) >> MAPBLOCKSHIFT;
  int blocky = (y - bmaporgy) >> MAPBLOCKSHIFT;

  if (blockx >= 0 && blockx < bmapwidth && blocky >= 0 && blocky < bmapheight)
    blocklinks[blocky * bmapwidth + blockx] = a;
}
*/

//
// Links a thing into both a block and a subsector
// based on it's x y. Sets subsector properly.
// Does NOT check whether it actually fits there.
//
void Actor::SetPosition()
{
  // NOTE that tmfloorz and tmceilingz must be set (using CheckPosition() or something)
  floorz = tmfloorz;
  ceilingz = tmceilingz;

  // link into subsector
  subsector_t *ss = mp->R_PointInSubsector(pos.x, pos.y);
  subsector = ss;

  if (!(flags & MF_NOSECTOR))
    {
      // invisible things don't go into the sector links
      sector_t *sec = ss->sector;
#ifdef PARANOIA
      if (sprev != NULL || snext != NULL)
	I_Error("Actor::SetPosition: thing at (%d, %d) is already linked", pos.x.floor(), pos.y.floor());
#endif

      sprev = NULL;
      snext = sec->thinglist;

      if (sec->thinglist)
	sec->thinglist->sprev = this;

      sec->thinglist = this;

      //SoM: 4/6/2000
      //
      // If sector_list isn't NULL, it has a collection of sector
      // nodes that were just removed from this Thing.

      // Collect the sectors the object will live in by looking at
      // the existing sector_list and adding new nodes and deleting
      // obsolete ones.

        // When a node is deleted, its sector links (the links starting
        // at sector_t->touching_thinglist) are broken. When a node is
        // added, new sector links are created.

      mp->CreateSecNodeList(this, pos.x, pos.y);
    }

  int blockx, blocky;
  Actor **link;

  // link into blockmap
  if (! (flags & MF_NOBLOCKMAP))
    {
      // inert things don't need to be in blockmap
      blockx = (pos.x - mp->bmaporgx).floor() >> MAPBLOCKBITS;
      blocky = (pos.y - mp->bmaporgy).floor() >> MAPBLOCKBITS;

      if (blockx>=0
	  && blockx < mp->bmapwidth
	  && blocky>=0
	  && blocky < mp->bmapheight)
        {
	  link = &mp->blocklinks[blocky * mp->bmapwidth + blockx];
	  bprev = NULL;
	  bnext = *link;
	  if (*link)
	    (*link)->bprev = this;

	  *link = this;
        }
      else
        {
	  // thing is off the map
	  bnext = bprev = NULL;
        }
    }
}




//===========================================
//           MOVEMENT ITERATORS
//===========================================

static void CheckForPushSpecial(line_t *line, int side, Actor *thing)
{
  if (line->special)
    {
      if (thing->flags2 & MF2_PUSHWALL)
	thing->mp->ActivateLine(line, thing, side, SPAC_PUSH);
      else if (thing->flags2 & MF2_IMPACT)
	thing->mp->ActivateLine(line, thing, side, SPAC_IMPACT);
    }
}



//
// Checks if a colliding actor triggers shootable or pushable linedefs
//
void Actor::CheckLineImpact()
{
  int n = spechit.size();
  if (!n || (flags & MF_NOCLIPLINE))
    return;

  // monsters don't shoot triggers (wrong) TODO
  if (owner && !(owner->flags & MF_NOTMONSTER))
    return;

  if (eflags & MFE_BLASTED)
    {
      Damage(NULL, NULL, int(mass/32.0f));
    }

  for (int i = n - 1; i >= 0; i--)
    {
      line_t *ld = spechit[i];
      int side = P_PointOnLineSide(pos.x, pos.y, ld);
      CheckForPushSpecial(ld, side, this);
      //was mp->ActivateLine(spechit[i], owner, 0, SPAC_IMPACT);
    }
}




/// \brief Checks if an Actor is physically collided by another.
/// \ingroup g_pit
/*!
  Iterator function for Actor->Actor collision checks. tmthing collides, thing gets collided.
  Sets Blocking.thing, calls Actor::Touch.
*/
static bool PIT_CheckThing(Actor *thing)
{
  // don't clip against self
  if (thing == tmthing)
    return true;

  if (interact)
    {
      if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
	return true;
    }
  else
    {
      if (!(thing->flags & MF_SOLID))
	return true;
    }

  if (thing->flags & MF_NOCLIPTHING)
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;

  if (abs(thing->pos.x - tmx) >= blockdist || abs(thing->pos.y - tmy) >= blockdist)
    {
      // didn't hit it
      return true;
    }

  // semi-hack to handle Heretic and Hexen monsters which cannot pass over each other
  if (!(tmthing->flags2 & MF2_NOPASSMOBJ) || !(thing->flags2 & MF2_NOPASSMOBJ))
    {
      // check if an Actor passed over/under another
      if (tmthing->Feet() >= thing->Top())
        {
	  // over
	  return true;
        }
      else if (tmthing->Top() <= thing->Feet())
        {
	  // under thing
	  return true;
        }
    }

  bool collision = interact ? tmthing->Touch(thing) : true; // they overlap geometrically
  if (collision)
    Blocking.thing = thing;

  return !collision;
}

// SoM: 3/15/2000
// PIT_CrossLine
// Checks to see if a PE->LS trajectory line crosses a blocking
// line. Returns false if it does.
//
// tmbbox holds the bounding box of the trajectory. If that box
// does not touch the bounding box of the line in question,
// then the trajectory is not blocked. If the PE is on one side
// of the line and the LS is on the other side, then the
// trajectory is blocked.
//
// Currently this assumes an infinite line, which is not quite
// correct. A more correct solution would be to check for an
// intersection of the trajectory and the line, but that takes
// longer and probably really isn't worth the effort.
//
/*
TODO
static bool PIT_CrossLine (line_t *ld)
{
  if (!(ld->flags & ML_TWOSIDED) ||
      (ld->flags & (ML_BLOCKING|ML_BLOCKMONSTERS)))
    if (tmb.BoxTouchBoxbox(ld->bbox))
      if (P_PointOnLineSide(pe_x,pe_y,ld) != P_PointOnLineSide(ls_x,ls_y,ld))
        return false;  // line blocks trajectory
  return true; // line doesn't block trajectory
}
*/



/// \brief Checks if a line_t is hit by an Actor.
/// \ingroup g_pit
/*!
  Iterator for Actor->Line collision checking.
  Adjusts tmfloorz and tmceilingz as lines are contacted.
  Sets Blocking.line, pushes lines, adds them to spechit vector.
*/
static bool PIT_CheckLine(line_t *ld)
{
  if (!tmb.BoxTouchBox(ld->bbox))
    return true;

  if (tmb.BoxOnLineSide(ld) != -1)
    return true;

  // A line has been hit.

  // The moving thing's destination position will cross the given line.
  // If this should not be allowed, return false.
  // If the line is special, keep track of it
  // to process later if the move is proven ok.
  // NOTE: specials are NOT sorted by order,
  // so two special lines that are only 8 pixels apart
  // could be crossed in either order.

  // 10-12-99 BP: moved this line to out of the if so upper and 
  //              lower texture can be hit by a splat
  Blocking.line = ld;

  bool stopped = false;
  bool push = true;

  if (!ld->backsector) // one-sided line
    {
      if ((tmthing->flags & MF_MISSILE) && ld->special)
        spechit.push_back(ld);

      stopped = true;
    }
  else if (!(tmthing->flags & MF_MISSILE))
    {
      // missile and Camera can cross uncrossable lines with a backsector

      if (ld->flags & ML_BLOCKING)
	stopped = true; // block everything
      else if ((ld->flags & ML_BLOCKMONSTERS) && !(tmthing->flags & MF_NOTMONSTER))
	{
	  stopped = true; // block monsters only
	  push = false; // TODO why?
	}
    }

  if (stopped)
    {
      if (interact)
	{
	  // take damage from impact
	  if (tmthing->eflags & MFE_BLASTED)
	    tmthing->Damage(NULL, NULL, int(tmthing->mass / 32.0f));

	  if (push)
	    CheckForPushSpecial(ld, 0, tmthing);
	}

      return false;
    }

  line_opening_t *open = P_LineOpening(ld, tmthing);

  // adjust floor / ceiling heights
  if (open->top < tmceilingz)
    {
      tmsectorceilingz = tmceilingz = open->top;
      ceilingline = ld;
    }

  if (open->bottom > tmfloorz)
    {
      tmsectorfloorz = tmfloorz = open->bottom;
      // TODO tmfloorpic = ;
    }

  if (open->lowfloor < tmdropoffz)
    tmdropoffz = open->lowfloor;

  // if contacted a special line, add it to the list
  if (ld->special)
    spechit.push_back(ld);

  return true;
}


// =========================================================================
//                         MOVEMENT CLIPPING
// =========================================================================

//
// This is NOT purely informative, it i.a. generates collisions between things.
//
// in:
//  a Actor (can be valid or invalid)
//  a position to be checked (doesn't need to be related to the Actor->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  tmfloorz
//  tmceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//

//added:27-02-98:
//
// tmfloorz
//     the nearest floor or thing's top under tmthing
// tmceilingz
//     the nearest ceiling or thing's bottom over tmthing
//

/// \brief xxxx
/*!
  Does full Actor->Actor collision checking, XY line collisions.
  Crossed special lines are stored into spechit.
  \return false if XY position is utterly impossible
 */
bool Actor::CheckPosition(fixed_t nx, fixed_t ny, bool act)
{
  interact = act; // should we touch or just look?
  tmthing = this;

  tmb.Set(nx, ny, radius);

  subsector_t *ss = mp->R_PointInSubsector(nx,ny);

  // The base floor / ceiling is from the subsector that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.
  tmfloorz = tmsectorfloorz = tmdropoffz = ss->sector->floorheight;
  tmceilingz = tmsectorceilingz = ss->sector->ceilingheight;
  tmfloorpic = ss->sector->floorpic;

  // Check list of fake floors and see if tmfloorz/tmdropoffz/tmceilingz need to be altered.
  if (ss->sector->ffloors)
    {
      fixed_t thingtop = Top();

      for (ffloor_t *rover = ss->sector->ffloors; rover; rover = rover->next)
	{
	  if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
	    continue;
	  fixed_t ffcenter = (*rover->topheight + *rover->bottomheight)/2;
	  fixed_t delta1 = abs(Feet() - ffcenter);
	  fixed_t delta2 = abs(thingtop - ffcenter);
	  if (*rover->topheight > tmfloorz && delta1 < delta2)
	    tmfloorz = tmdropoffz = *rover->topheight;
	  if (*rover->bottomheight < tmceilingz && delta1 >= delta2)
	    tmceilingz = *rover->bottomheight;
	}
    }

  // tmfloorthing is set when tmfloorz comes from a thing's top
  tmfloorthing = NULL; // FIXME for this to work, the lines should be checked first, then things...
  ceilingline = Blocking.line = NULL;
  Blocking.thing = NULL;

  validcount++;
  spechit.clear();

  // Check things first, possibly picking things up.
  // The bounding box is extended by MAXRADIUS
  // because Actors are grouped into mapblocks
  // based on their origin point, and can overlap
  // into adjacent blocks by up to MAXRADIUS units.

  int xl, xh, yl, yh, bx, by;
  fixed_t bmox = mp->bmaporgx; 
  fixed_t bmoy = mp->bmaporgy; 

  if (!(flags & MF_NOCLIPTHING))
    {
      // check things
      xl = ((tmb[BOXLEFT] - bmox).floor() - MAXRADIUS) >> MAPBLOCKBITS;
      xh = ((tmb[BOXRIGHT] - bmox).floor() + MAXRADIUS) >> MAPBLOCKBITS;
      yl = ((tmb[BOXBOTTOM] - bmoy).floor() - MAXRADIUS) >> MAPBLOCKBITS;
      yh = ((tmb[BOXTOP] - bmoy).floor() + MAXRADIUS) >> MAPBLOCKBITS;

      for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	  if (!mp->BlockThingsIterator(bx,by,PIT_CheckThing))
	    return false;
    }

  if (!(flags & MF_NOCLIPLINE))
    {
      // check lines
      xl = (tmb[BOXLEFT] - bmox).floor() >> MAPBLOCKBITS;
      xh = (tmb[BOXRIGHT] - bmox).floor() >> MAPBLOCKBITS;
      yl = (tmb[BOXBOTTOM] - bmoy).floor() >> MAPBLOCKBITS;
      yh = (tmb[BOXTOP] - bmoy).floor() >> MAPBLOCKBITS;
      for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	  if (!mp->BlockLinesIterator(bx,by,PIT_CheckLine))
	    return false;
    }

  return true;
}


// Returns true if the mobj is not blocked by anything at its current
// location, otherwise returns false.
bool Actor::TestLocation()
{
  if (CheckPosition(pos.x, pos.y, false) &&
      (Feet() >= floorz) && (Top() <= ceilingz))
    {
      spechit.clear(); // TODO we could not fill it in the first place...
      return true;
    }

  spechit.clear();
  return false;
}



bool Actor::TestLocation(fixed_t nx, fixed_t ny)
{
  if (CheckPosition(nx, ny, false) &&
      (Feet() >= floorz) && (Top() <= ceilingz))
    {
      spechit.clear(); // TODO we could not fill it in the first place...
      return true;
    }

  spechit.clear();
  return false;
}



//=============================================================================
//  Z movement
//=============================================================================

Actor *onmobj; //generic global onmobj...used for landing on pods/players

/// \brief Clip z movement
/// \ingroup g_pit
/*!
  TODO
*/
static bool PIT_CheckOnmobjZ(Actor *thing)
{
  if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))    
    return true; // Can't hit thing
    
  fixed_t blockdist = thing->radius + tmthing->radius;
  if (abs(thing->pos.x-tmx) >= blockdist || abs(thing->pos.y-tmy) >= blockdist)    
    return true; // Didn't hit thing

  if (thing == tmthing)
    return true; // Don't clip against self

  if (tmthing->Feet() > thing->Top())
    return true; // over
  else if (tmthing->Top() < thing->Feet())
    return true; // under thing

  if (thing->flags & MF_SOLID)
    onmobj = thing;
  
  return !(thing->flags & MF_SOLID);
}



// Fake the zmovement so that we can check if a move is legal
void Actor::FakeZMovement()
{
  // TODO: get rid of this entire function. ZMovement should be enough.

  extern consvar_t cv_gravity;
  //
  // adjust height
  //
  // z += pz;
  fixed_t nz = pos.z + vel.z;
  if ((flags & MF_FLOAT) && target)
    {       // float down towards target if too close
      if (!(eflags & (MFE_SKULLFLY | MFE_INFLOAT)))
	{
	  fixed_t dist = P_AproxDistance(pos.x - target->pos.x, pos.y - target->pos.y);
	  fixed_t delta = target->pos.z + (height>>1) - nz;
	  if (delta < 0 && dist < -(3*delta))
	    nz -= FLOATSPEED;
	  else if (delta > 0 && dist < (3*delta))
	    nz += FLOATSPEED;
	}
    }
  if ((eflags & MFE_FLY) && !(nz <= floorz) && mp->maptic & 2)
    {
      nz += finesine[(FINEANGLES/20*mp->maptic>>2) & FINEMASK];
    }

  // FIXME it would be better if we didn't need to actually change the Actor
  // this is a fake movement after all...
  fixed_t npz = vel.z;
  //
  // clip movement
  //
  if (nz <= floorz)
    { // Hit the floor
      nz = floorz;
      if (npz < 0)
	{
	  npz = 0;
	}
      if (eflags & MFE_SKULLFLY)
	{ // The skull slammed into something
	  npz = -npz;
	}
      if (flags & MF_CORPSE) // &&info->crashstate 
	{
	  pos.z = nz;
	  vel.z = npz;
	  return;
	}
    }
  else if (flags2 & MF2_LOGRAV)
    {
      if (npz == 0)
	npz = -(cv_gravity.value>>3)*2;
      else
	npz -= cv_gravity.value>>3;
    }
  else if (!(flags & MF_NOGRAVITY))
    {
      if (npz == 0)
	npz = -cv_gravity.value*2;
      else
	npz -= cv_gravity.value;
    }

  if (nz + height > ceilingz)
    {       // hit the ceiling
      if (npz > 0)
	npz = 0;
      nz = ceilingz - height;
      if (eflags & MFE_SKULLFLY)
	{       // the skull slammed into something
	  npz = -npz;
	}
    }
  pos.z = nz;
  vel.z = npz;
}


// Checks if the new Z position is legal
Actor *Actor::CheckOnmobj()
{
  int          xl,xh,yl,yh,bx,by;
    
  tmthing = this;
  fixed_t oldz = pos.z;
  fixed_t oldpz = vel.z;
  FakeZMovement();
  
  tmb.Set(pos.x, pos.y, radius);

  subsector_t *ss = mp->R_PointInSubsector(pos.x, pos.y);
    
  //
  // the base floor / ceiling is from the subsector that contains the
  // point.  Any contacted lines the step closer together will adjust them
  //
  tmfloorz = tmdropoffz = ss->sector->floorheight;
  tmceilingz = ss->sector->ceilingheight;
  tmfloorpic = ss->sector->floorpic; 
   
  validcount++;
  spechit.clear();
    
  if (flags & MF_NOCLIPTHING)
    return NULL;
    
  //
  // check things first, possibly picking things up
  // the bounding box is extended by MAXRADIUS because Actors are grouped
  // into mapblocks based on their origin point, and can overlap into adjacent
  // blocks by up to MAXRADIUS units
  //
  xl = ((tmb[BOXLEFT] - mp->bmaporgx).floor() - MAXRADIUS) >> MAPBLOCKBITS;
  xh = ((tmb[BOXRIGHT] - mp->bmaporgx).floor() + MAXRADIUS) >> MAPBLOCKBITS;
  yl = ((tmb[BOXBOTTOM] - mp->bmaporgy).floor() - MAXRADIUS) >> MAPBLOCKBITS;
  yh = ((tmb[BOXTOP] - mp->bmaporgy).floor() + MAXRADIUS) >> MAPBLOCKBITS;
    
  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockThingsIterator(bx,by,PIT_CheckOnmobjZ))
	{
	  pos.z = oldz;
	  vel.z = oldpz;
	  return onmobj;
	}

  pos.z = oldz;
  vel.z = oldpz;
  return NULL;
}





//==========================================================================
// Sliding moves
// Allows the player to slide along any angled walls.
//==========================================================================

static fixed_t bestslidefrac, secondslidefrac;
static line_t *bestslideline, *secondslideline;
static Actor *slidemo;
static vec_t<fixed_t> tmmove;


// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
static void P_HitSlideLine(line_t *ld)
{
  if (ld->slopetype == ST_HORIZONTAL)
    {
      tmmove.y = 0;
      slidemo->vel.y = 0;
      return;
    }

  if (ld->slopetype == ST_VERTICAL)
    {
      tmmove.x = 0;
      slidemo->vel.x = 0;
      return;
    }

  vec_t<float> line_normal(ld->dy.Float(), -ld->dx.Float(), 0); // line rotated 90 degrees clockwise
  //vec_t<fixed_t> line_normal(ld->dy, -ld->dx, 0); // line rotated 90 degrees clockwise

  tmmove -= tmmove.Project(line_normal);
  slidemo->vel -= slidemo->vel.Project(line_normal);

  /*
  int side = P_PointOnLineSide(slidemo->pos.x, slidemo->pos.y, ld);

  angle_t lineangle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

  if (side == 1)
    lineangle += ANG180;

  angle_t moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);
  angle_t deltaangle = moveangle-lineangle;

  if (deltaangle > ANG180)
    deltaangle += ANG180;
  //  I_Error ("SlideLine: ang>ANG180");

  lineangle >>= ANGLETOFINESHIFT;
  deltaangle >>= ANGLETOFINESHIFT;

  fixed_t newlen = P_AproxDistance(tmxmove, tmymove) * finecosine[deltaangle];

  tmxmove = newlen * finecosine[lineangle];
  tmymove = newlen * finesine[lineangle];
  */
}


/// \brief Tries sliding along the intercept_t
/// \ingroup g_ptr
/*!
  When a move is blocked by an unpassable line, try sliding along it.
*/
static bool PTR_SlideTraverse(intercept_t *in)
{
  line_t *li = in->line;

  if (!(li->flags & ML_TWOSIDED))
    {
      if (P_PointOnLineSide(slidemo->pos.x, slidemo->pos.y, li))
        {
	  // don't hit the back side
	  return true;
        }
    }
  else if (!(li->flags & ML_BLOCKING))
    {
      line_opening_t *open = P_LineOpening(li, slidemo);

      if (!(open->range < slidemo->height ||
	    open->top < slidemo->Top() || 
	    open->bottom > slidemo->Feet() + MAXSTEPMOVE))
	return true; // this line doesn't block movement
    }

  // the line does block movement,
  // see if it is closer than best so far

  if (in->frac < bestslidefrac)
    {
      //secondslidefrac = bestslidefrac;
      //secondslideline = bestslideline;
      bestslidefrac = in->frac;
      bestslideline = li;
    }

  return false;       // stop
}



// The px / py move is bad, so try to slide along a wall.
// Find the first line hit, move flush to it, and slide along it
//
// This is a kludgy mess.
void Actor::SlideMove(fixed_t nx, fixed_t ny)
{
  fixed_t dx = nx - pos.x;
  fixed_t dy = ny - pos.y;

  fixed_t leadx, leady;
  fixed_t trailx, traily;

  slidemo = this;
  int hitcount = 0;

  fixed_t fudge;   // FIXME find a better way
  fudge.setvalue(0x800);

 retry:
  if (++hitcount == 3)
    goto stairstep;         // don't loop forever


  // trace along the three leading corners
  if (dx > 0)
    {
      leadx = pos.x + radius;
      trailx = pos.x - radius;
    }
  else
    {
      leadx = pos.x - radius;
      trailx = pos.x + radius;
    }

  if (dy > 0)
    {
      leady = pos.y + radius;
      traily = pos.y - radius;
    }
  else
    {
      leady = pos.y - radius;
      traily = pos.y + radius;
    }

  bestslidefrac = 1 + fixed_epsilon;

  // find bestslideline and -frac
  mp->PathTraverse(leadx, leady, leadx+dx, leady+dy, PT_ADDLINES, PTR_SlideTraverse);
  mp->PathTraverse(trailx, leady, trailx+dx, leady+dy, PT_ADDLINES, PTR_SlideTraverse);
  mp->PathTraverse(leadx, traily, leadx+dx, traily+dy, PT_ADDLINES, PTR_SlideTraverse);
  
  // move up to the wall
  if (bestslidefrac == 1 + fixed_epsilon)
    {
      // the move must have hit the middle, so stairstep
    stairstep:
      if (!TryMove(pos.x, pos.y + dy, true)) //SoM: 4/10/2000
	TryMove (pos.x + dx, pos.y, true);  //Allow things to drop off.
      return;
    }

  // fudge a bit to make sure it doesn't hit
  bestslidefrac -= fudge;
  if (bestslidefrac > 0)
    {
      fixed_t newx = dx * bestslidefrac;
      fixed_t newy = dy * bestslidefrac;

      if (!TryMove(pos.x+newx, pos.y+newy, true))
	goto stairstep;
    }

  // Now continue along the wall.
  // First calculate remainder.
  bestslidefrac = 1 - (bestslidefrac + fudge);

  if (bestslidefrac > 1)
    bestslidefrac = 1;

  if (bestslidefrac <= 0)
    return;

  tmmove.x = dx * bestslidefrac;
  tmmove.y = dy * bestslidefrac;

  P_HitSlideLine(bestslideline);     // clip the moves

  dx = tmmove.x;
  dy = tmmove.y;

  if (!TryMove(pos.x+dx, pos.y+dy, true))
    {
      goto retry;
    }
}



//============================================================================
// Bouncing from walls
//============================================================================


/// \brief Checks if an Actor will bounce from the intercept
/// \ingroup g_ptr
/*!
  Uses the slidemove static variables.
*/
static bool PTR_BounceTraverse(intercept_t *in)
{
  line_t *li = in->line;

  if (!(li->flags & ML_TWOSIDED))
    {
      if (P_PointOnLineSide(slidemo->pos.x, slidemo->pos.y, li))
	return true;            // don't hit the back side
    }
  else
    {
      line_opening_t *open = P_LineOpening(li, slidemo);

      // will it fit through? FIXME does this include fake floors?
      if (open->range >= slidemo->height &&
	  open->top >= slidemo->Top())
	return true; // this line doesn't block movement
    }

  // the line does block movement, see if it is closer than best so far
  if (in->frac < bestslidefrac)
    {
      //secondslidefrac = bestslidefrac;
      //secondslideline = bestslideline;
      bestslidefrac = in->frac;
      bestslideline = li;
    }

  return false; // stop
}


///
void Actor::BounceWall(fixed_t nx, fixed_t ny)
{
  fixed_t dx = nx - pos.x;
  fixed_t dy = ny - pos.y;

  fixed_t leadx, leady;

  slidemo = this;

  // trace from leading corner
  if (dx > 0)
    leadx = pos.x+radius;
  else
    leadx = pos.x-radius;

  if (dy > 0)
    leady = pos.y+radius;
  else
    leady = pos.y-radius;

  bestslidefrac = 1 + fixed_epsilon;
  bestslideline = NULL;

  mp->PathTraverse(leadx, leady, leadx+dx, leady+dy, PT_ADDLINES, PTR_BounceTraverse);

  if (!bestslideline)
    return;

  int side = P_PointOnLineSide(pos.x, pos.y, bestslideline);
  angle_t lineangle = R_PointToAngle2(0, 0, bestslideline->dx, bestslideline->dy);
  if (side == 1)
    lineangle += ANG180;

  angle_t moveangle = R_PointToAngle2(0, 0, dx, dy);
  angle_t newangle = 2*lineangle - moveangle;

  newangle >>= ANGLETOFINESHIFT;

  fixed_t movelen = 0.75 * P_AproxDistance(dx, dy); // lose energy in the bounce

  if (movelen < 1)
    movelen = 2;

  dx = movelen * finecosine[newangle];
  dy = movelen * finesine[newangle];
}






//==========================================================================
//   Autoaim, shooting with instahit weapons
//==========================================================================

static Actor *shootthing;

Actor *linetarget;     // who got hit (or NULL) // TODO make static
static fixed_t aimsine; // == Sin(pitch)
static fixed_t bottomsine, topsine; // vertical aiming range

static fixed_t shootz; // bullet starting z
static fixed_t lastz;  //SoM: The last z height of the bullet when it crossed a line

static int     la_damage, la_dtype;
static fixed_t attackrange; // 3D length of the attack trace



mobjtype_t PuffType = MT_PUFF;
Actor *PuffSpawned;


void Map::SpawnPuff(fixed_t x, fixed_t y, fixed_t z)
{
  z += P_SignedFRandom(6);

  if (!(game.mode == gm_heretic || game.mode == gm_hexen))
    PuffType = MT_PUFF;

  DActor *puff = SpawnDActor(x, y, z, PuffType);

  if (linetarget && puff->info->seesound)
    // Hit thing sound
    S_StartSound(puff, puff->info->seesound);
  else if (puff->info->attacksound)
    S_StartSound(puff, puff->info->attacksound);

  switch (PuffType)
    {
    case MT_PUFF:
      puff->tics -= P_Random()&3;
      if (puff->tics < 1)
	puff->tics = 1;
        
      // don't make punches spark on the wall
      if (attackrange == MELEERANGE)
	puff->SetState(S_PUFF3);
      // fallthru
    case MT_PUNCHPUFF:
    case MT_BEAKPUFF:
    case MT_STAFFPUFF:
      puff->vel.z = 1;
      break;
    case MT_HAMMERPUFF:
    case MT_GAUNTLETPUFF1:
    case MT_GAUNTLETPUFF2:
      puff->vel.z = 0.8f;
      break;
    default:
      break;
    }
  PuffSpawned = puff;
}


/// \brief Aiming up and down for missile attacks.
/// \ingroup g_ptr
/*!
  Seeks targets along the trace between bottomsine and topsine.
  Sets linetarget when a target is hit.
  Returns true if the thing is not shootable, else continue through..
*/
static bool PTR_AimTraverse(intercept_t *in)
{
  fixed_t dist; // 3D distance

  if (in->isaline)
    {
      line_t *li = in->line;

      if (!(li->flags & ML_TWOSIDED))
	return false;               // stop

      // Crosses a two sided line.
      // A two sided line will restrict the possible target ranges.
      line_opening_t *open = P_LineOpening(li); // TODO thing?

      if (open->range <= 0)
	return false;               // stop

      dist = attackrange * in->frac;

      fixed_t temp;

      if (li->frontsector->floorheight != li->backsector->floorheight)
        {
	  temp = (open->bottom - shootz) / dist;
	  if (temp > bottomsine)
	    bottomsine = temp;
        }

      if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
	  temp = (open->top - shootz) / dist;
	  if (temp < topsine)
	    topsine = temp;
        }

      if (topsine <= bottomsine)
	return false;               // stop


      // FIXME aimsine is used here, but it makes no sense! we're aiming up and down! it's not well-defined yet!
      if (li->frontsector->ffloors || li->backsector->ffloors)
        {
          int dir = (aimsine > 0) ? 1 : ((aimsine < 0) ? -1 : 0);

          int frontflag = P_PointOnLineSide(shootthing->pos.x, shootthing->pos.y, li);

          //SoM: Check 3D FLOORS!
	  for (ffloor_t *rover = li->frontsector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
		continue;

	      fixed_t highsine = (*rover->topheight - shootz) / dist;
	      fixed_t lowsine = (*rover->bottomheight - shootz) / dist;
	      if ((aimsine >= lowsine && aimsine <= highsine))
		return false; // hit the side of the floor

	      if (lastz > *rover->topheight && dir == -1 && aimsine < highsine)
		frontflag |= 0x2;

	      if (lastz < *rover->bottomheight && dir == 1 && aimsine > lowsine)
		frontflag |= 0x2;
	    }

	  for (ffloor_t *rover = li->backsector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
		continue;

	      fixed_t highsine = (*rover->topheight - shootz) / dist;
	      fixed_t lowsine = (*rover->bottomheight - shootz) / dist;
	      if ((aimsine >= lowsine && aimsine <= highsine))
		return false; // hit the side of the floor

	      if (lastz > *rover->topheight && dir == -1 && aimsine < highsine)
		frontflag |= 0x4;

	      if (lastz < *rover->bottomheight && dir == 1 && aimsine > lowsine)
		frontflag |= 0x4;
	    }

          if ((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            return false;
        }

      lastz = aimsine * dist + shootz; // FIXME aimsine is not well-defined

      return true; // shot continues
    }

  // shoot a thing
  Actor *th = in->thing;
  if (th == shootthing)
    return true; // can't shoot self

  // TODO pods should not be targeted it seems. Add a new flag?
  // || (th->type == MT_POD))
  if (!(th->flags & MF_SHOOTABLE) || (th->flags & MF_CORPSE))
    return true; // corpse or something
  // NOTE since top- and bottomsines are not adjusted, we may try to shoot through the thing!

  // check angles to see if the thing can be aimed at
  dist = attackrange * in->frac;
  fixed_t thingtopsine = (th->Top() - shootz) / dist;

  if (thingtopsine < bottomsine)
    return true;                    // shot over the thing

  fixed_t thingbottomsine = (th->Feet() - shootz) / dist;

  if (thingbottomsine > topsine)
    return true;                    // shot under the thing

  // this thing can be hit! take aim!
  if (thingtopsine > topsine)
    thingtopsine = topsine;

  if (thingbottomsine < bottomsine)
    thingbottomsine = bottomsine;

  //added:15-02-98: find the sine just in the middle(y) of the thing!
  aimsine = (thingtopsine + thingbottomsine)/2;
  linetarget = th;

  return false; // don't seek any farther
}


/*
struct line_opening2_t
{
  fixed_t top;
  fixed_t bottom;
  fixed_t lowfloor;
  opening_t *next; ///< next one (upwards)

public:
  void FreeChain();
}
#warning modded for new lineopenings
*/



/// \brief Firing instant-hit missile weapons.
/// \ingroup g_ptr
/*!
  aimsine is sin(pitch) for the shot. Sets linetarget if a target is hit.
  \return true if the shot continues after this intercept
*/
static bool PTR_ShootTraverse(intercept_t *in)
{
  //added:18-02-98: added clipping the shots on the floor and ceiling.
  fixed_t x, y, z;
  fixed_t sine; // temp variable

  fixed_t dist;  // 3D distance
  fixed_t frac;

  bool        hitplane;    //true if we clipped z on floor/ceil plane

  int dir;

  if (aimsine > 0)
    dir = 1;
  else if (aimsine < 0)
    dir = -1;
  else
    dir = 0;

  // we need the right Map * from somewhere.
  Map *m = in->m;

  if (in->isaline)
    {
      //shut up compiler, otherwise it's only used when TWOSIDED
      bool diffheights = false; //check for sky hacks with different ceil heights

      line_t *li = in->line;

      if (li->special)
	m->ActivateLine(li, shootthing, 0, SPAC_IMPACT); // your documentation no longer confuses me, old version

      if (!(li->flags & ML_TWOSIDED))
	goto hitline;

      {
      // crosses a two sided line
      line_opening_t *open = P_LineOpening(li, shootthing); // TODO correct thing?

      dist = attackrange * in->frac;

      // hit low?
      if (li->frontsector->floorheight != li->backsector->floorheight)
	{
	  //added:18-02-98: comments :
	  // find the sine aiming on the border between the two floors
	  sine = (open->bottom - shootz) / dist;
	  if (sine > aimsine)
	    goto hitline;
	}

      // hit high?
      if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
	{
	  //added:18-02-98: remember : diff ceil heights
	  diffheights = true;

	  sine = (open->top - shootz) / dist;
	  if (sine < aimsine)
	    goto hitline;
	}
      }

      if (li->frontsector->ffloors || li->backsector->ffloors)
        {
          int frontflag = P_PointOnLineSide(shootthing->pos.x, shootthing->pos.y, li);

          //SoM: Check 3D FLOORS!
	  for (ffloor_t *rover = li->frontsector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

	      fixed_t highsine = (*rover->topheight - shootz) / dist;
	      fixed_t lowsine = (*rover->bottomheight - shootz) / dist;
	      if ((aimsine >= lowsine && aimsine <= highsine))
		goto hitline;

	      if (lastz > *rover->topheight && dir == -1 && aimsine < highsine)
		frontflag |= 0x2;

	      if (lastz < *rover->bottomheight && dir == 1 && aimsine > lowsine)
		frontflag |= 0x2;
	    }

	  for (ffloor_t *rover = li->backsector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

	      fixed_t highsine = (*rover->topheight - shootz) / dist;
	      fixed_t lowsine = (*rover->bottomheight - shootz) / dist;
	      if ((aimsine >= lowsine && aimsine <= highsine))
		goto hitline;

	      if (lastz > *rover->topheight && dir == -1 && aimsine < highsine)
		frontflag |= 0x4;

	      if (lastz < *rover->bottomheight && dir == 1 && aimsine > lowsine)
		frontflag |= 0x4;
	    }

          if ((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            goto hitline;
        }
      lastz = aimsine * dist + shootz;

      // shot continues
      return true;


      // hit line
    hitline:
#warning TODO check this
      // position a bit closer
      frac = in->frac - 4 / attackrange;
      dist = frac * attackrange; // four mapunits back from actual intercept

      fixed_t distz = aimsine * dist; // z add between gun z and hit z
      z = shootz + distz; // hit z on wall

      //added:17-02-98: clip shots on floor and ceiling
      //                use a simple triangle stuff a/b = c/d ...
      // BP:13-3-99: fix the side usage
      hitplane = false;
      int sectorside = P_PointOnLineSide(shootthing->pos.x, shootthing->pos.y, li);
      sector_t *sector = NULL;
      fixed_t   floorz = 0;  //SoM: Bullets should hit fake floors!
      fixed_t   ceilingz = 0;

      if (li->sideptr[sectorside] != NULL) // can happen in nocliping mode
        {
	  sector = li->sideptr[sectorside]->sector;

	  floorz = sector->floorheight;
	  ceilingz = sector->ceilingheight;

	  for (ffloor_t *rover = sector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID))
		continue;

	      if (dir == 1 && *rover->bottomheight < ceilingz && *rover->bottomheight > lastz)
		ceilingz = *rover->bottomheight;
	      if (dir == -1 && *rover->topheight > floorz && *rover->topheight < lastz)
		floorz = *rover->topheight;
	    }

	  if ((z > ceilingz) && distz != 0)
            {
	      frac = (frac * (ceilingz - shootz)) / distz;
	      hitplane = true;
            }
	  else if ((z < floorz) && distz != 0)
	    {
	      frac = -(frac * (shootz - floorz)) / distz;
	      hitplane = true;
	    }

	  if (sector->ffloors)
            {
	      if (dir == 1 && z > ceilingz)
		z = ceilingz;
	      if (dir == -1 && z < floorz)
		z = floorz;
            }
        }

      // make a wall splat
      if (!hitplane)
        {
	  divline_t   divl;
	  divl.MakeDivline(li);
	  fixed_t frac = P_InterceptVector(&divl, &trace);
	  m->R_AddWallSplat(li, sectorside, "A_DMG1", z, frac, SPLATDRAWMODE_SHADE);
        }


      x = trace.x + trace.dx * frac;
      y = trace.y + trace.dy * frac;

      if (li->frontsector->ceilingpic == skyflatnum)
        {
	  // don't shoot the sky!
	  if (z > li->frontsector->ceilingheight)
	    return false;

	  // it's a sky hack wall
	  if  ((!hitplane &&      //added:18-02-98:not for shots on planes
		li->backsector &&
		diffheights &&    //added:18-02-98:skip only REAL sky hacks
		//   eg: they use different ceil heights.
		li->backsector->ceilingpic == skyflatnum))
	    return false;
        }

      if (sector && sector->ffloors)
        {
          if (dir == 1 && z + 16 > ceilingz) // FIXME magic number
            z = ceilingz - 16;
          if (dir == -1 && z < floorz)
            z = floorz;
        }
      // Spawn bullet puffs.
      m->SpawnPuff(x,y,z);

      // don't go any farther
      return false;
    }

  // shoot a thing
  Actor *th = in->thing;
  if (th == shootthing)
    return true;            // can't shoot self

  if (!(th->flags & MF_SHOOTABLE))
    return true; // nonshootable

  // check for physical attacks on a ghost FIXME this is stupid. We should use some sort of damage type system.
  // (if damagetype != dt_ethereal ...)
  /*
  if (game.mode == gm_heretic && (th->flags & MF_SHADOW))
    if (shootthing->Type() == Thinker::tt_ppawn)
      if (shootthing->readyweapon == wp_staff)
	return true;
  */

  // check angles to see if the thing can be aimed at
  dist = attackrange * in->frac;
  fixed_t thingtopsine = (th->Top() - shootz) / dist;

  if (thingtopsine < aimsine)
    return true;            // shot over the thing

  fixed_t thingbottomsine = (th->Feet() - shootz) / dist;

  if (thingbottomsine > aimsine)
    return true;            // shot under the thing

  // SoM: SO THIS IS THE PROBLEM!!!
  // heh.
  // A bullet would travel through a 3D floor until it hit a LINEDEF! Thus
  // it appears that the bullet hits the 3D floor but it actually just hits
  // the line behind it. Thus allowing a bullet to hit things under a 3D
  // floor and still be clipped a 3D floor.

  sector_t *sector = th->subsector->sector;
  for (ffloor_t *rover = sector->ffloors; rover; rover = rover->next)
    {
      if (!(rover->flags & FF_SOLID))
	continue;

      if (dir == -1 && *rover->topheight < lastz && *rover->topheight > th->Top())
	return true;
      if (dir == 1 && *rover->bottomheight > lastz && *rover->bottomheight < th->Feet())
	return true;
    }


  // hit thing
  // position a bit closer
  frac = in->frac - 10 / attackrange;

  x = trace.x + (trace.dx * frac);
  y = trace.y + (trace.dy * frac);
  z = shootz + aimsine * (frac * attackrange);

  if (la_damage)
    hitplane = th->Damage(shootthing, shootthing, la_damage, la_dtype);
  else
    hitplane = false;

  // who got hit?
  linetarget = th;

  // Spawn bullet puffs or blood spots,
  // depending on target type.

  if (PuffType == MT_BLASTERPUFF1)   
    PuffType = MT_BLASTERPUFF2;  // Make blaster big puff

  if (in->thing->flags & MF_NOBLOOD ||
      in->thing->flags2 & MF2_INVULNERABLE)
    m->SpawnPuff(x,y,z);
  else
    {
      if (game.mode >= gm_heretic)
	m->SpawnPuff(x, y, z);

      /* TODO heretic blood splatter
      if(PuffType == MT_AXEPUFF || PuffType == MT_AXEPUFF_GLOW)
	{
	  P_BloodSplatter2(x, y, z, in->d.thing);
	}
      if(P_Random() < 192)
	{
	  P_BloodSplatter(x, y, z, in->d.thing);
	}
      */

      if (hitplane)
	{
	  m->SpawnBloodSplats(vec_t<fixed_t>(x,y,z), la_damage, trace.dx, trace.dy);
	  return false;
	}
    }

  return false;
}


/// \brief Actor tries to find a target for an attack
/// \ingroup g_trace
/*!
  The function performs a trace starting from (roughly) the center of the Actor.
  It returns the nearest suitable target that can be hit by varying the pitch of the trace.
  \param[in] ang yaw angle for the attack
  \param[in] distance range of the attack (including z direction!)
  \param[out] sinpitch sine of the pitch angle towards target
  \return pointer to the target or NULL if none was found
*/
Actor *Actor::AimLineAttack(angle_t ang, fixed_t distance, fixed_t &sinpitch)
{
  shootthing = this;
  attackrange = distance;
  linetarget = NULL;

  // Since monsters shouldn't change their "pitch" angle,
  // why not use the same routine for them also?
  fixed_t temp = distance * Cos(pitch);
  fixed_t x2 = pos.x + temp * Cos(ang);
  fixed_t y2 = pos.y + temp * Sin(ang);

  aimsine = Sin(pitch); // initial try, TODO bad heuristics for ffloors...

  //added:15-02-98: Fab comments...
  // Doom's base engine says that at a distance of 160,
  // the 2d graphics on the plane x,y correspond 1/1 with plane units
  const fixed_t ds = 10.0f/16; // tan of half the vertical fov


  topsine    = aimsine + ds; // TODO not correct... should add the angles and do some clipping
  bottomsine = aimsine - ds;
  // can't shoot outside view angles

  shootz = lastz = Center() + 8 - floorclip; // FIXME magic number

  //added:15-02-98: comments
  // traverse all linedefs and mobjs from the blockmap containing t1,
  // to the blockmap containing the dest. point.
  // Call the function for each mobj/line on the way,
  // starting with the mobj/linedef at the shortest distance...
  mp->PathTraverse(pos.x, pos.y, x2, y2, PT_ADDLINES | PT_ADDTHINGS, PTR_AimTraverse);

  // found a target?
  sinpitch = aimsine; // unchanged if no target was found
  return linetarget;
}


/// \brief Shoots an insta-hit projectile from Actor to the direction determined by (yaw, pitch)
/// \ingroup g_trace
/*!
  The function performs a trace starting from (roughly) the center of the actor.
  If damage == 0, it is just a test trace that will leave linetarget set.
  Otherwise, a contacted target will be damaged.
  \param ang yaw angle for the attack
  \param distance max range for the projectile (including z direction!)
  \param sine sin(pitch) for the attack
  \param damage damage amount
  \param dtype damage type
  \return pointer to the target or NULL if none was hit
*/
Actor *Actor::LineAttack(angle_t ang, fixed_t distance, fixed_t sine, int damage, int dtype)
{
  shootthing = this;
  attackrange = distance;
  linetarget = NULL;
  la_damage = damage;
  la_dtype = dtype;

  fixed_t temp = sqrt((1 - sine*sine).Float()) * distance;
  //fixed_t temp = distance * Cos(ArcSin(sine));
  //fixed_t temp = distance * Cos(pitch);
  fixed_t x2 = pos.x + temp * Cos(ang);
  fixed_t y2 = pos.y + temp * Sin(ang);

  shootz = lastz = Center() + 8 - floorclip; // FIXME magic number
  aimsine = sine;

  mp->PathTraverse(pos.x, pos.y, x2, y2, PT_ADDLINES | PT_ADDTHINGS, PTR_ShootTraverse);

  return linetarget;

  /* TODO missed cleric flame attack:
     case MT_FLAMEPUFF:
     P_SpawnPuff(x2, y2, shootz+FixedMul(sine, distance));
  */
}


//==========================================================================
//  Using linedefs
//==========================================================================

static PlayerPawn *usething;

/// \brief Using special line_t's
/// \ingroup g_ptr
/*!
  Called when a player has pushed USE.
*/
static bool PTR_UseTraverse(intercept_t *in)
{
  line_t *line = in->line;
  CONS_Printf("Line: s = %d, tag = %d\n", line->special, line->tag);
  if (!line->special)
    {
      line_opening_t *open = P_LineOpening(line, usething);
      // TEST: use through a hole only if you can reach it
      if (open->range <= 0 || open->top < usething->Feet() || open->bottom > usething->Top())
        {
	  S_StartSound(usething, sfx_usefail);

	  // can't use through a wall
	  return false;
        }

      // not a special line, but keep checking
      return true ;
    }

  // can't use backsides of lines
  int side = P_PointOnLineSide(usething->pos.x, usething->pos.y, line);
  if (side != 0)
    return false;

  int act = GET_SPAC(line->flags);

  // can't use for than one special line in a row
  // SoM: USE MORE THAN ONE!
  if (boomsupport && act == SPAC_PASSUSE)
    {
      usething->mp->ActivateLine(line, usething, side, SPAC_PASSUSE);
      return true;
    }

  if (act == SPAC_USE || line->flags & ML_BOOM_GENERALIZED)
    usething->mp->ActivateLine(line, usething, side, SPAC_USE);

  return false;
}


// Looks for special lines in front of the player to activate.
void PlayerPawn::UseLines()
{
  fixed_t  x1, y1, x2, y2;

  usething = this;

  int ang = yaw >> ANGLETOFINESHIFT;

  x1 = pos.x;
  y1 = pos.y;
  x2 = x1 + USERANGE * finecosine[ang];
  y2 = y1 + USERANGE * finesine[ang];

  mp->PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse);
}


//==========================================================================
//  Puzzle item usage
//==========================================================================

static PlayerPawn *PuzzleItemUser;
static int  PuzzleItemType;
static bool PuzzleActivated;

/// \brief Using Hexen puzzle items.
/// \ingroup g_ptr
/*!
  Called when a player activates a puzzle item.
*/
static bool PTR_PuzzleItemTraverse(intercept_t *in)
{
  const int USE_PUZZLE_ITEM_SPECIAL = 129;

  if (in->isaline)
    { // Check line
      line_t *line = in->line;
      if (line->special != USE_PUZZLE_ITEM_SPECIAL)
	{
	  line_opening_t *open = P_LineOpening(line, PuzzleItemUser);
	  if (open->range <= 0)
	    {
	      int sound;
	      switch (PuzzleItemUser->pclass)
		{
		case PCLASS_FIGHTER:
		  sound = SFX_PUZZLE_FAIL_FIGHTER;
		  break;
		case PCLASS_CLERIC:
		  sound = SFX_PUZZLE_FAIL_CLERIC;
		  break;
		case PCLASS_MAGE:
		  sound = SFX_PUZZLE_FAIL_MAGE;
		  break;
		default:
		  sound = SFX_PUZZLE_FAIL_FIGHTER;
		  break;
		}
	      S_StartSound(PuzzleItemUser, sound);
	      return false; // can't use through a wall
	    }
	  return true; // Continue searching
	}

      if (P_PointOnLineSide(PuzzleItemUser->pos.x, PuzzleItemUser->pos.y, line) == 1)
	return false; // Don't use back sides

      if (PuzzleItemType != line->args[0])
	return false; // Item type doesn't match

      PuzzleItemUser->mp->StartACS(line->args[1], &line->args[2], PuzzleItemUser, line, 0);
      line->special = 0;
      PuzzleActivated = true;
      return false; // Stop searching
    }

  // Check thing
  Actor *p = in->thing;
  if (p->special != USE_PUZZLE_ITEM_SPECIAL)
    return true; // Wrong special

  if (PuzzleItemType != p->args[0])
    return true; // Item type doesn't match

  PuzzleItemUser->mp->StartACS(p->args[1], &p->args[2], PuzzleItemUser, NULL, 0);
  p->special = 0;
  PuzzleActivated = true;
  return false; // Stop searching
}

//
// Returns true if the puzzle item was used on a line or a thing.
//
bool PlayerPawn::UsePuzzleItem(int type)
{
  fixed_t x1, y1, x2, y2;

  PuzzleItemUser = this;
  PuzzleItemType = type;
  PuzzleActivated = false;

  int ang = yaw >> ANGLETOFINESHIFT;
  x1 = pos.x;
  y1 = pos.y;
  x2 = x1 + USERANGE * finecosine[ang];
  y2 = y1 + USERANGE * finesine[ang];
  mp->PathTraverse(x1, y1, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_PuzzleItemTraverse);

  return PuzzleActivated;
}


//==========================================================================
// RADIUS ATTACK
//==========================================================================

static struct
{
  Actor  *owner; // the creature that caused the explosion at b
  Actor  *b;

  int     damage;
  int     dtype;
  fixed_t radius;
  bool    damage_owner;
} Bomb;

/// \brief Checks if an Actor is damaged by a radius attack.
/// \ingroup g_pit
/*!
  Explosions etc. Uses the struct Bomb to hold data.
*/
static bool PIT_RadiusAttack(Actor *thing)
{
  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  if (!Bomb.damage_owner && thing == Bomb.owner)
    return true;

  // Bosses take no damage from concussion.
  // if (thing->flags2 & MF2_BOSS) return true;

  // TODO uses a weird L-infinity norm, dist = max(dx,dy,dz), L2-norm would be better
  fixed_t dx = abs(thing->pos.x - Bomb.b->pos.x);
  fixed_t dy = abs(thing->pos.y - Bomb.b->pos.y);

  fixed_t temp = dx>dy ? dx : dy;
  temp -= thing->radius;

  //added:22-02-98: now checks also z dist for rockets exploding
  //                above yer head...
  fixed_t dz = abs(thing->Center() - Bomb.b->pos.z);
  fixed_t dist = temp > dz ? temp : dz;

  if (dist < 0)
    dist = 0;

  if (dist >= Bomb.radius)
    return true;    // out of range

  // geometry blocks the blast?
  if (thing->floorz > Bomb.b->pos.z && Bomb.b->ceilingz < thing->pos.z)
    return true;
  if (thing->ceilingz < Bomb.b->pos.z && Bomb.b->floorz > thing->pos.z)
    return true;

  if (thing->mp->CheckSight(thing, Bomb.b))
    {
      int damage = (Bomb.damage * ((Bomb.radius - dist) / Bomb.radius)).floor() + 1;

      // TODO Hexen: if(thing->player) damage >>= 2;

      fixed_t apx = 0, apy = 0;
      if (temp != 0)
        {
	  apx = (thing->pos.x - Bomb.b->pos.x) / (dist+1);
	  apy = (thing->pos.y - Bomb.b->pos.y) / (dist+1);
        }
      // must be in direct path
      if (thing->Damage(Bomb.b, Bomb.owner, damage, Bomb.dtype) && !(thing->flags & MF_NOBLOOD))
	thing->mp->SpawnBloodSplats(thing->pos, damage, apx, apy);
    }

  return true;
}


/// \brief Effects a radius attack.
/*!
  culprit is the creature that caused the explosion.
  Iterates through blockmap for targets.
*/
void Actor::RadiusAttack(Actor *culprit, int damage, fixed_t rad, int dtype, bool downer)
{
  if (rad < 0)
    Bomb.radius = damage;
  else
    Bomb.radius = rad;

  fixed_t dist = Bomb.radius + MAXRADIUS;
  int yh = (pos.y + dist - mp->bmaporgy).floor() >> MAPBLOCKBITS;
  int yl = (pos.y - dist - mp->bmaporgy).floor() >> MAPBLOCKBITS;
  int xh = (pos.x + dist - mp->bmaporgx).floor() >> MAPBLOCKBITS;
  int xl = (pos.x - dist - mp->bmaporgx).floor() >> MAPBLOCKBITS;

  Bomb.b = this;
  Bomb.owner = culprit;
  Bomb.damage = damage;
  Bomb.dtype = dtype;
  Bomb.damage_owner = downer;

  for (int ny=yl ; ny<=yh ; ny++)
    for (int nx=xl ; nx<=xh ; nx++)
      mp->BlockThingsIterator (nx, ny, PIT_RadiusAttack);
}



//==========================================================================
// SECTOR HEIGHT CHANGING
//==========================================================================

// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
static bool P_ThingHeightClip(Actor *thing)
{
  bool onfloor = (thing->pos.z <= thing->floorz);

  thing->CheckPosition(thing->pos.x, thing->pos.y, true);

  // what about stranding a monster partially off an edge?

  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;

  if (!tmfloorthing && onfloor && !(thing->flags & MF_NOGRAVITY))
    {
      // walking monsters rise and fall with the floor TODO allow 9 units altitude (Hexen)?
      thing->pos.z = thing->floorz;
    }
  else
    {
      // don't adjust a floating monster unless forced to
      //added:18-04-98:test onfloor
      if (!onfloor)                    //was tmsectorceilingz
	if (thing->Top() > tmceilingz)
	  thing->pos.z = thing->ceilingz - thing->height;

      //thing->eflags &= ~MFE_ONGROUND;
    }

  //debug : be sure it falls to the floor
  thing->eflags &= ~MFE_ONGROUND;

  //added:28-02-98:
  // test sector bouding top & bottom, not things

  //if (tmsectorceilingz - tmsectorfloorz < thing->height)
  //    return false;

  if (thing->ceilingz - thing->floorz < thing->height
      // BP: i know that this code cause many trouble but this fix alos 
      // lot of problem, mainly this is implementation of the stepping 
      // for mobj (walk on solid corpse without jumping or fake 3d bridge)
      // problem is imp into imp at map01 and monster going at top of others
      && thing->pos.z >= thing->floorz)
    return false;

  return true;
}

int         crushdamage;
bool        nofit;
sector_t   *sectorchecked;


/// \brief Checks if an Actor is affected by a sector_t height change.
/// \ingroup g_pit
/*!
  Updates Actor height clipping data, deals crush damage, crunches items.
*/
static bool PIT_ChangeSector(Actor *thing)
{
  if (P_ThingHeightClip(thing))
    {
      // keep checking
      return true;
    }

  // crunch nonsolid corpses to giblets
  if ((thing->flags & MF_CORPSE) && (thing->health <= 0))
    {
      // crunch it even if not MF_SHOOTABLE
      thing->Die(NULL, NULL, dt_crushing);
      return true; // keep checking
    }

  // crunch dropped items
  if (thing->flags & MF_DROPPED)
    {
      thing->Remove();

      // keep checking
      return true;
    }

  if (!(thing->flags & MF_SHOOTABLE) || !(thing->flags & MF_SOLID))
    {
      // assume it is bloody gibs or something
      return true;
    }

  nofit = true;

  if (crushdamage && !(thing->mp->maptic % 4))
    {
      thing->Damage(NULL, NULL, crushdamage, dt_crushing); // was 10

      if (!(thing->mp->maptic % 16) && !(thing->flags & MF_NOBLOOD)
	  && !(thing->flags2 & MF2_INVULNERABLE))
        {
	  // spray blood in a random direction
	  DActor *mo = thing->mp->SpawnDActor(thing->pos.x, thing->pos.y, thing->Center(), MT_BLOOD);
            
	  mo->vel.x  = P_SignedFRandom(4);
	  mo->vel.y  = P_SignedFRandom(4);
        }
    }

  // keep checking (crush other things)
  return true;
}

/*!
  changes sector height, crushes things
*/
bool Map::ChangeSector(sector_t *sector, int crunch)
{
  nofit = false;
  crushdamage = crunch;
  sectorchecked = sector;

  // re-check heights for all things near the moving sector
  for (int x = sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
    for (int y = sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
      BlockThingsIterator (x, y, PIT_ChangeSector);

  return nofit;
}

/// \brief Handles sector_t height changes.
/// \ingroup g_iterators
/*!
  After modifying a sectors floor or ceiling height, call this routine to adjust the positions
  of all things that touch the sector (touching_thinglist).
   If anything doesn't fit anymore, true will be returned.
  If crunch is true, they will take damage as they are being crushed.
  If crunch is false, you should set the sector height back
  the way it was and call P_ChangeSector again to undo the changes.
*/
bool Map::CheckSector(sector_t *sector, int crunch)
{
  //SoM: 3/15/2000: New function. Much faster.
  if (!boomsupport) // use the old routine for old demos though
    return ChangeSector(sector,crunch);

  nofit = false;
  crushdamage = crunch;


  // killough 4/4/98: scan list front-to-back until empty or exhausted,
  // restarting from beginning after each thing is processed. Avoids
  // crashes, and is sure to examine all things in the sector, and only
  // the things which are in the sector, until a steady-state is reached.
  // Things can arbitrarily be inserted and removed and it won't mess up.
  //
  // killough 4/7/98: simplified to avoid using complicated counter

  msecnode_t *n;

  if (sector->numattached)
    {
      for (int i = 0; i < sector->numattached; i++)
	{
	  sector_t *sec = &sectors[sector->attached[i]];
	  for (n=sec->touching_thinglist; n; n=n->m_snext)
	    n->visited = false;

	  sec->moved = true;

	  do {
	    for (n=sec->touching_thinglist; n; n=n->m_snext)
	      if (!n->visited)
		{
		  n->visited  = true;
		  if (!(n->m_thing->flags & MF_NOBLOCKMAP))
		    PIT_ChangeSector(n->m_thing);
		  break;
		}
	  } while (n);
	}
    }
  // Mark all things invalid
  sector->moved = true;

  for (n=sector->touching_thinglist; n; n=n->m_snext)
    n->visited = false;
  
  do {
    for (n=sector->touching_thinglist; n; n=n->m_snext)  // go through list
      if (!n->visited)               // unprocessed thing found
	{
	  n->visited  = true;          // mark thing as processed
	  if (!(n->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
	    PIT_ChangeSector(n->m_thing);    // process it
	  break;                 // exit and start over
	}
  } while (n);  // repeat from scratch until all things left are marked valid
  
  return nofit;
}



//==========================================================================
//  Sector lists
//==========================================================================

static msecnode_t *sector_list = NULL;

/// \brief Adds sectors to the touching_sectorlist of an Actor.
/// \ingroup g_pit
/*!
  Locates all the sectors the object is in by looking at the lines that
  cross through it. You have already decided that the object is allowed
  at this location, so don't bother with checking impassable or
  blocking lines.
*/
static bool PIT_GetSectors(line_t *ld)
{
  if (!tmb.BoxTouchBox(ld->bbox))
    return true;

  if (tmb.BoxOnLineSide(ld) != -1)
    return true;

  // This line crosses through the object.

  // Collect the sector(s) from the line and add to the
  // sector_list you're examining. If the Thing ends up being
  // allowed to move to this position, then the sector_list
  // will be attached to the Thing's Actor at touching_sectorlist.

  sector_list = msecnode_t::AddToSectorlist(ld->frontsector, tmthing, sector_list);

  // Don't assume all lines are 2-sided, since some Things
  // like MT_TFOG are allowed regardless of whether their radius takes
  // them beyond an impassable linedef.

  // Use sidedefs instead of 2s flag to determine two-sidedness.

  if (ld->backsector)
    sector_list = msecnode_t::AddToSectorlist(ld->backsector, tmthing, sector_list);

  return true;
}

/// \brief Builds the touching_sectorlist of an Actor.
/// \ingroup g_iterators
/*!
  Alters/creates the sector_list that shows what sectors the object resides in.
*/
void Map::CreateSecNodeList(Actor *thing, fixed_t x, fixed_t y)
{
  int xl, xh, yl, yh, bx, by;

  // First, clear out the existing m_thing fields. As each node is
  // added or verified as needed, m_thing will be set properly. When
  // finished, delete all nodes where m_thing is still NULL. These
  // represent the sectors the Thing has vacated.

  msecnode_t *node = sector_list = thing->touching_sectorlist;
  
  while (node)
    {
      node->m_thing = NULL;
      node = node->m_tnext;
    }

  tmthing = thing;

  tmb.Set(x, y, tmthing->radius);

  validcount++; // used to make sure we only process a line once

  xl = (tmb[BOXLEFT] - bmaporgx).floor() >> MAPBLOCKBITS;
  xh = (tmb[BOXRIGHT] - bmaporgx).floor() >> MAPBLOCKBITS;
  yl = (tmb[BOXBOTTOM] - bmaporgy).floor() >> MAPBLOCKBITS;
  yh = (tmb[BOXTOP] - bmaporgy).floor() >> MAPBLOCKBITS;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      BlockLinesIterator(bx,by,PIT_GetSectors);

  // Add the sector of the (x,y) point to sector_list.

  sector_list = msecnode_t::AddToSectorlist(thing->subsector->sector, thing, sector_list);

  // Now delete any nodes that won't be used. These are the ones where
  // m_thing is still NULL.

  thing->touching_sectorlist = msecnode_t::CleanSectorlist(sector_list);
  sector_list = NULL; // just to be sure
}





//===========================================
//    Teleportation
//===========================================

/// \brief Checks if an Actor blocks a teleportation move (or is telefragged)
/// \ingroup g_pit
/*!
  Well.
*/
static bool PIT_StompThing(Actor *thing)
{
  // don't clip against self
  if (thing == tmthing)
    return true;

  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;

  if (abs(thing->pos.x - tmx) >= blockdist || abs(thing->pos.y - tmy) >= blockdist)
    return true; // didn't hit it

  // Monsters don't stomp things except unless they come from a bossbrain shooter.
  // Not allowed to stomp things?
  if (!(tmthing->flags2 & MF2_TELESTOMP))
    return false;

  thing->Damage(tmthing, tmthing, 10000, dt_telefrag | dt_always);

  return true;
}


bool Actor::TeleportMove(fixed_t nx, fixed_t ny)
{
  // kill anything occupying the position
  tmthing = this;
  tmb.Set(nx, ny, radius);

  sector_t *newsec = mp->R_PointInSubsector(nx, ny)->sector;

  // FIXME do a checkposition first

  // The base floor/ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.
  tmfloorz = tmdropoffz = newsec->floorheight;
  tmceilingz = newsec->ceilingheight;
  tmfloorpic = newsec->floorpic;

  validcount++;
  spechit.clear();

  int  xl, xh, yl, yh, bx, by;

  // stomp on any things contacted
  xl = ((tmb[BOXLEFT] - mp->bmaporgx).floor() - MAXRADIUS) >> MAPBLOCKBITS;
  xh = ((tmb[BOXRIGHT] - mp->bmaporgx).floor() + MAXRADIUS) >> MAPBLOCKBITS;
  yl = ((tmb[BOXBOTTOM] - mp->bmaporgy).floor() - MAXRADIUS) >> MAPBLOCKBITS;
  yh = ((tmb[BOXTOP] - mp->bmaporgy).floor() + MAXRADIUS) >> MAPBLOCKBITS;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockThingsIterator(bx,by,PIT_StompThing))
	return false;

  // the move is ok,
  // so link the thing into its new position
  UnsetPosition();
  pos.x = nx;
  pos.y = ny;
  SetPosition();

  return true;
}





//===========================================
//  Radius iteration
//===========================================

// iterates the radius around (x,y) using func
bool Map::RadiusLinesCheck(fixed_t x, fixed_t y, fixed_t radius, line_iterator_t func)
{
  int xl, xh, yl, yh, bx, by;

  tmb.Set(x, y, radius);

  validcount++;
 
  // check lines
  xl = (tmb[BOXLEFT] - bmaporgx).floor() >> MAPBLOCKBITS;
  xh = (tmb[BOXRIGHT] - bmaporgx).floor() >> MAPBLOCKBITS;
  yl = (tmb[BOXBOTTOM] - bmaporgy).floor() >> MAPBLOCKBITS;
  yh = (tmb[BOXTOP] - bmaporgy).floor() >> MAPBLOCKBITS;

  for (bx=xl; bx<=xh; bx++)
    for (by=yl; by<=yh; by++)
      if (!BlockLinesIterator(bx, by, func))
	return false;
 
  return true;
}



//===========================================
//  Spikes
//===========================================

/// \brief Checks if an Actor is hit by a Hexen spike
/// \ingroup g_pit
/*!
  Well.
*/
static bool PIT_ThrustStompThing(Actor *thing)
{
  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;
  if (abs(thing->pos.x - tmthing->pos.x) >= blockdist || 
      abs(thing->pos.y - tmthing->pos.y) >= blockdist ||
      thing->Feet() > tmthing->Top())
    return true;   // didn't hit it

  if (thing == tmthing)
    return true;   // don't clip against self

  thing->Damage(tmthing, tmthing, 10001, dt_crushing);
  tmthing->args[1] = 1;	// Mark thrust thing as bloody

  return true;
}


void P_ThrustSpike(Actor *actor)
{
  int xl,xh,yl,yh,bx,by;

  tmthing = actor;
  Map *mp = actor->mp;

  tmb.Set(actor->pos.x, actor->pos.y, actor->radius);

  xl = ((tmb[BOXLEFT] - mp->bmaporgx).floor() - MAXRADIUS) >> MAPBLOCKBITS;
  xh = ((tmb[BOXRIGHT] - mp->bmaporgx).floor() + MAXRADIUS) >> MAPBLOCKBITS;
  yl = ((tmb[BOXBOTTOM] - mp->bmaporgy).floor() - MAXRADIUS) >> MAPBLOCKBITS;
  yh = ((tmb[BOXTOP] - mp->bmaporgy).floor() + MAXRADIUS) >> MAPBLOCKBITS;

  // stomp on any things contacted
  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      mp->BlockThingsIterator(bx,by,PIT_ThrustStompThing);
}

