// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
/// \brief Movement, collision handling. Shooting and aiming.


/*!
  \defgroup g_physics Physics engine

  The physics engine of Doom Legacy.
 */

/*!
  \defgroup g_collision Collision detection
  \ingroup g_physics

  Stuff used for collision detection and
  interactions between multiple Actors, and Actors and line_t's.
 */

/*!
  \defgroup g_iterators Map geometry iterator functions
  \ingroup g_physics

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
  \ingroup g_physics

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
#include "g_decorate.h"

#include "g_damage.h"
#include "command.h"
#include "p_maputl.h"
#include "m_bbox.h"
#include "m_random.h"

#include "p_enemy.h"

#include "r_sky.h"
#include "r_splats.h"

#include "sounds.h"
#include "tables.h"
#include "z_zone.h"

extern int boomsupport;


//===========================================
//  Radius iteration
//===========================================

static Actor *tmthing;

bbox_t  tmb; // bounding box, used by line PIT_* functions

// iterates the radius around (x,y) using func
bool Map::BlockIterateLinesRadius(fixed_t x, fixed_t y, fixed_t radius, line_iterator_t func)
{
  validcount++; // used by BlockLinesIterator to make sure we only process a line once
  tmb.Set(x, y, radius);

  int xl, xh, yl, yh, bx, by;

  // check lines within box
  xl = bmap.BlockX(tmb[BOXLEFT]);
  xh = bmap.BlockX(tmb[BOXRIGHT]);
  yl = bmap.BlockY(tmb[BOXBOTTOM]);
  yh = bmap.BlockY(tmb[BOXTOP]);

  for (bx=xl; bx<=xh; bx++)
    for (by=yl; by<=yh; by++)
      if (!BlockLinesIterator(bx, by, func))
	return false;

  return true;
}


static fixed_t tmx, tmy; // used by thing PIT_* functions

bool Map::BlockIterateThingsRadius(fixed_t x, fixed_t y, fixed_t radius, thing_iterator_t func)
{
  int xl,xh,yl,yh,bx,by;

  tmx = x;
  tmy = y;

  xl = bmap.BlockX(x - radius);
  xh = bmap.BlockX(x + radius);
  yl = bmap.BlockY(y - radius);
  yh = bmap.BlockY(y + radius);

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!BlockThingsIterator(bx, by, func))
	return false;

  return true;
}



//===========================================
//  Movement
//===========================================

// If floatok is true, XY move would be ok if we were at the correct Z
bool    floatok;

static bool interact; // touch or just see if it fits? shoot or just trace?

// checkposition data (stuff in the XY footprint of the Actor)
position_check_t PosCheck;


const float   SHOOTFRAC   = 36.0/56; ///< fraction of Actor height where shots start
extern const fixed_t MAXSTEP      = 24; ///< Max Z move up or down without jumping. Above this, a height difference is considered a 'dropoff'.
const fixed_t MAXWATERSTEP = 37; ///< Same, but in water.



// Attempt to move to a new position,
// crossing special lines in the way.
// TODO verify onmobj logic
bool Actor::TryMove(fixed_t nx, fixed_t ny, bool allowdropoff)
{
  floatok = false;

  // TODO the problems with the collision check code are
  // No traces are used, rather blockmapcells are iterated one by one by PIT_CheckLine, and we stop
  // when the first blocking thing/line is found =>
  // 1) there might be others we actually hit first.
  // If the iteration is stopped by a line we have immediate collision damage and impact activation.
  // z fit is only checked later, possible z-blocking lines are pushed to spechit.
  // Here, when we notice that move is not OK due to failed CheckPosition or z-blocking,
  // 2) we AGAIN give collision damage and do impact activation for entire spechit.
  // If z fit ok, all crossings in spechit are processed.

  if (!CheckPosition(nx, ny, true))
    {
      // blocked by an unpassable line or an Actor
      if (PosCheck.block_thing)
	{
	  // try to climb on top of it
	  if (PosCheck.block_thing->Top() - Feet() <= MAXSTEP &&
	      //PosCheck.block_thing->subsector->sector->ceilingheight - PosCheck.block_thing->Top() >= height &&
	      PosCheck.op.top - PosCheck.block_thing->Top() >= height)
	    PosCheck.floor_thing = PosCheck.block_thing;
	  else
	    return false;
	}
      else
	{
	  // must be a blocking line
	  CheckLineImpact();
	  return false;
	}
    }

  // handle z-lineclip and spechit
  if (!(flags & MF_NOCLIPLINE))
    {
      // do we fit in the z direction?
      if (PosCheck.op.Range() < height)
	{
	  CheckLineImpact();
	  return false;
	}

      floatok = true; // if we float to the correct height first, we will fit

      // When flying, we have a slight z-directional autopilot for convenience
      if (eflags & MFE_FLY)
	{
	  if (Top() > PosCheck.op.top)
	    {
	      vel.z = -8;
	      return false;
	    }
	  else if (Feet() < PosCheck.op.bottom && PosCheck.op.Drop() > MAXSTEP)
	    {
	      vel.z = 8;
	      return false;
	    }
	}

      // do we hit the upper texture?
      if (Top() > PosCheck.op.top &&
	  !(flags2 & MF2_CEILINGHUGGER)) // ceilinghuggers step down any amount
	{
	  CheckLineImpact();
	  PosCheck.skyimpact = PosCheck.op.top_sky;
	  return false; // must lower itself to fit
	}

      // do we hit the lower texture without being able to climb the step?
      if (PosCheck.op.bottom > Feet() &&
	  !(flags2 & MF2_FLOORHUGGER)) // floorhuggers step up any amount
	{
	  // easier to move in water / climb out of water
	  fixed_t maxstep = (eflags & MFE_UNDERWATER) ? MAXWATERSTEP : MAXSTEP;

	  if (flags & MF_MISSILE || // missiles do not step up
	      PosCheck.op.bottom - Feet() > maxstep)
	    {
	      CheckLineImpact();
	      PosCheck.skyimpact = PosCheck.op.bottom_sky;
	      return false;       // too big a step up
	    }
	}

      // are we afraid of the dropoff?
      if (!allowdropoff && !(flags & MF_DROPOFF) && !(eflags & MFE_BLASTED))
	if (PosCheck.op.Drop() > MAXSTEP && !PosCheck.floor_thing)
	  return false; // don't go over a dropoff (unless blasted)

      // are we unable to leave the floor texture? (water monsters)
      if (flags2 & MF2_CANTLEAVEFLOORPIC
	  && (PosCheck.op.bottompic != subsector->sector->floorpic || PosCheck.op.bottom != Feet()))
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
  if (PosCheck.floor_thing)
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
  if (!(flags & MF_NOCLIPLINE))
    {
      while (PosCheck.spechit.size())
        {
	  // see if the line was crossed
	  line_t *ld = PosCheck.spechit.back();
	  PosCheck.spechit.pop_back();

	  int side = P_PointOnLineSide(pos.x, pos.y, ld);
	  int oldside = P_PointOnLineSide(oldx, oldy, ld);
	  if (side != oldside && ld->special)
            {
	      if (flags2 & MF2_MCROSS &&
		  mp->ActivateLine(ld, this, oldside, SPAC_MCROSS))
		continue;

	      if (flags2 & MF2_PCROSS &&
		  mp->ActivateLine(ld, this, oldside, SPAC_PCROSS))
		continue;
		  
	      if (flags & (MF_PLAYER | MF_MONSTER))
		mp->ActivateLine(ld, this, oldside, SPAC_CROSS);
            }
        }
    }

  return true;
}



//=====================================================================
//              Actor position checking and setting
//=====================================================================

/// \brief Unlinks an Actor from all blockmap and sector lists
/*!
  On each position change, BLOCKMAP and other lookups maintaining lists of things inside
  these structures need to be updated.
  This function should _only_ be called in an unset/set sequence, or when
  an Actor is completely removed from a Map.
*/
void Actor::UnsetPosition(bool clear_touching_sectorlist)
{
  // Free touching_sectorlist.
  // Must be done before Actor is deleted or removed, otherwise optional.
  // In SetPosition(), we'll keep any nodes that represent
  // sectors the Thing still touches. We'll add new ones then, and
  // delete any nodes for sectors the Thing has vacated. Then we'll
  // put it back into touching_sectorlist. It's done this way to
  // avoid a lot of deleting/creating for nodes, when most of the
  // time you just get back what you deleted anyway.
  if (clear_touching_sectorlist && touching_sectorlist)
    {
      msecnode_t::DeleteSectorlist(touching_sectorlist);
      touching_sectorlist = NULL;
    }

  if (!(flags & MF_NOSECTOR))
    {
      // inert things don't need to be in blockmap?
      // unlink from sector
      if (snext)
	snext->sprev = sprev;

      if (sprev)
	sprev->snext = snext;
      else
	subsector->sector->thinglist = snext;
#ifdef PARANOIA
      sprev = snext = NULL;
#endif
    }

  if (!(flags & MF_NOBLOCKMAP))
    {
      // inert things don't need to be in blockmap
      // unlink from blockmap
      if (bnext)
	bnext->bprev = bprev;

      if (bprev)
	bprev->bnext = bnext;
      else
        {
	  int blockx = mp->bmap.BlockX(pos.x);
	  int blocky = mp->bmap.BlockY(pos.y);

	  if (blockx>=0 && blockx < mp->bmap.width && blocky>=0 && blocky < mp->bmap.height)
	    mp->blocklinks[blocky * mp->bmap.width + blockx] = bnext;
        }

      bprev = bnext = NULL;
    }
}



/// \brief Links an Actor into blockmap and a sector lists
/*!
  Tries to link the Actor to its current position.
  Does NOT check whether it actually fits there.
*/
void Actor::SetPosition()
{
  // NOTE that PosCheck.op.bottom and PosCheck.op.top must be set (using CheckPosition() or something)
  floorz = PosCheck.op.bottom;
  ceilingz = PosCheck.op.top;

  // link into subsector
  subsector_t *ss = mp->R_PointInSubsector(pos.x, pos.y);
  subsector = ss;

  if (!(flags & MF_NOSECTOR))
    {
      // some things don't go into the sector links
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
      // Collect the sectors the object will live in by looking at
      // the existing touching_sectorlist and adding new nodes and deleting
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
      blockx = mp->bmap.BlockX(pos.x);
      blocky = mp->bmap.BlockY(pos.y);

      if (blockx>=0
	  && blockx < mp->bmap.width
	  && blocky>=0
	  && blocky < mp->bmap.height)
        {
	  link = &mp->blocklinks[blocky * mp->bmap.width + blockx];
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
  int n = PosCheck.spechit.size();
  if (!n || (flags & MF_NOCLIPLINE))
    return;

  // monsters don't shoot triggers (wrong) TODO
  if (owner && !(owner->flags & MF_PLAYER))
    return;

  if (eflags & MFE_BLASTED)
    {
      Damage(NULL, NULL, int(mass/32.0f));
    }

  for (int i = n - 1; i >= 0; i--)
    {
      line_t *ld = PosCheck.spechit[i];
      int side = P_PointOnLineSide(pos.x, pos.y, ld);
      CheckForPushSpecial(ld, side, this);
    }
}




/// \brief Checks if an Actor is physically collided by another.
/// \ingroup g_collision
/// \ingroup g_pit
/*!
  Iterator function for Actor->Actor collision checks. tmthing collides, thing gets collided.
  Sets PosCheck.block_thing, calls Actor::Touch.
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
    PosCheck.block_thing = thing;

  return !collision;
}




/// \brief Checks if a line_t is hit by an Actor.
/// \ingroup g_collision
/// \ingroup g_pit
/*!
  Iterator for Actor->Line collision checking.
  Adjusts PosCheck.op.bottom and PosCheck.op.top as lines are contacted.
  Sets PosCheck.block_line, pushes lines, adds them to spechit vector.
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
  // If the line is special, keep track of it to process later if the move is proven ok.
  // NOTE: specials are NOT sorted by order, so two special lines that are only 8 pixels apart
  // could be crossed in either order.

  PosCheck.block_line = ld;

  bool stopped = false;
  bool push = true;

  if (!ld->backsector) // one-sided line
    {
      stopped = true;
    }
  else if (!(tmthing->flags & MF_MISSILE))
    {
      // missile and Camera can cross uncrossable lines with a backsector

      if (ld->flags & ML_BLOCKING)
	stopped = true; // block everything
      else if ((ld->flags & ML_BLOCKMONSTERS) && (tmthing->flags & MF_MONSTER))
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
	    CheckForPushSpecial(ld, 0, tmthing); // TODO correct side?
	}

      return false;
    }

  // We need just one check per sector, but a sector can be bordered by
  // several line_t's and hence can appear here more than once.

  sector_t *s = ld->frontsector;
  if (s->validcount != validcount)
    {
      s->validcount = validcount;
      PosCheck.op.SubtractFromOpening(tmthing, s);
    }

  s = ld->backsector;
  if (s->validcount != validcount)
    {
      s->validcount = validcount;
      PosCheck.op.SubtractFromOpening(tmthing, s);
    }

  /*
    // No early out, checked at Actor::TryMove
  if (PosCheck.op.Range() < tmthing->height)
    return false; // collision? push? TODO
  */

  // crossing the line is possible during this move
  // if contacted a special line, add it to the list
  if (ld->special && interact)
    PosCheck.spechit.push_back(ld);

  return true;
}


//=========================================================================
//                         MOVEMENT CLIPPING
//=========================================================================


/// \brief Actor collision checking
/// \ingroup g_collision
/*!
  Does full Actor->Actor collision checking, XY Actor-line_t collisions.
  Actor does not need to be valid during check. Its z-coordinate will be used in
  finding openings between sectors.
  Crossed special lines are stored into spechit.
  \param[in] act Should we generate collisions or just check fit?
  \return false if XY position is utterly impossible
 */
bool Actor::CheckPosition(fixed_t nx, fixed_t ny, bool act)
{
  interact = act; // should we touch or just look?
  tmthing = this;

  // The base floor / ceiling is from the subsector that contains the point.
  // Any contacted lines will adjust them closer together.
  subsector_t *ss = mp->R_PointInSubsector(nx,ny);

  PosCheck.op.Reset();
  PosCheck.op.SubtractFromOpening(this, ss->sector);
  PosCheck.op.lowfloor = PosCheck.op.bottom; // necessary if no lines are encountered...

  PosCheck.spechit.clear();
  PosCheck.block_line = NULL;
  PosCheck.block_thing = NULL;
  PosCheck.floor_thing = NULL;
  PosCheck.skyimpact = false;

  // check lines
  if (!(flags & MF_NOCLIPLINE) && !mp->BlockIterateLinesRadius(nx, ny, radius, PIT_CheckLine))
    return false;

  // Check things, possibly picking things up.
  // The bounding box is extended by MAXRADIUS
  // because Actors are grouped into mapblocks
  // based on their origin point, and can overlap
  // into adjacent blocks by up to MAXRADIUS units.
  if (!(flags & MF_NOCLIPTHING) && !mp->BlockIterateThingsRadius(nx, ny, radius + MAXRADIUS, PIT_CheckThing))
    return false;

  return true;
}


// Returns true if the mobj is not blocked by anything at its current
// location, otherwise returns false.
bool Actor::TestLocation()
{
  return (CheckPosition(pos.x, pos.y, false) &&
	  (Feet() >= PosCheck.op.bottom) &&
	  (Top() <= PosCheck.op.top));
}



bool Actor::TestLocation(fixed_t nx, fixed_t ny)
{
  return (CheckPosition(nx, ny, false) &&
	  (Feet() >= PosCheck.op.bottom) &&
	  (Top() <= PosCheck.op.top));
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
  if (flags & MF_NOCLIPTHING)
    return NULL;

  tmthing = this;
  fixed_t oldz = pos.z;
  fixed_t oldpz = vel.z;
  FakeZMovement();
  // float vz+float_ai+fly change z, vz and z are clipped by geometry
  // then check if new z position makes us hit things
  
  // the bounding box is extended by MAXRADIUS because Actors are grouped
  // into mapblocks based on their origin point, and can overlap into adjacent
  // blocks by up to MAXRADIUS units
  if (!mp->BlockIterateThingsRadius(pos.x, pos.y, radius + MAXRADIUS, PIT_CheckOnmobjZ))
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

static float   bestslidefrac;
static line_t *bestslideline;
static Actor  *slidemo;
static vec_t<fixed_t> tmmove;


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
      line_opening_t *open = line_opening_t::Get(li, slidemo);

      if (!(open->Range() < slidemo->height ||
	    open->top < slidemo->Top() || 
	    open->bottom > slidemo->Feet() + MAXSTEP))
	return true; // this line doesn't block movement
    }

  // the line does block movement,
  // see if it is closer than best so far

  if (in->frac < bestslidefrac)
    {
      bestslidefrac = in->frac;
      bestslideline = li;
    }

  return false;       // stop
}



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



// The px / py move is bad, so try to slide along a wall.
// Find the first line hit, move flush to it, and slide along it
//
// This is a kludgy mess.
void Actor::SlideMove(fixed_t nx, fixed_t ny)
{
  vec_t<fixed_t> delta(nx - pos.x, ny - pos.y, 0);
  vec_t<fixed_t> corner;

  fixed_t leadx, leady;
  fixed_t trailx, traily;

  slidemo = this;
  int hitcount = 0;

  const float fudge = 1.0/32;   // TODO find a better way

 retry:
  if (++hitcount == 3)
    goto stairstep;         // don't loop forever


  // trace along the three leading corners
  if (delta.x > 0)
    {
      leadx = pos.x + radius;
      trailx = pos.x - radius;
    }
  else
    {
      leadx = pos.x - radius;
      trailx = pos.x + radius;
    }

  if (delta.y > 0)
    {
      leady = pos.y + radius;
      traily = pos.y - radius;
    }
  else
    {
      leady = pos.y - radius;
      traily = pos.y + radius;
    }

  bestslidefrac = 2;

  // find bestslideline and -frac
  corner.x = leadx; corner.y = leady;
  mp->PathTraverse(corner, corner + delta, PT_ADDLINES, PTR_SlideTraverse);

  corner.x = trailx; corner.y = leady;
  mp->PathTraverse(corner, corner + delta, PT_ADDLINES, PTR_SlideTraverse);

  corner.x = leadx; corner.y = traily;
  mp->PathTraverse(corner, corner + delta, PT_ADDLINES, PTR_SlideTraverse);

  // move up to the wall
  if (bestslidefrac == 2)
    {
      // the move must have hit the middle, so stairstep
    stairstep:
      if (!TryMove(pos.x, pos.y + delta.y, true)) //SoM: 4/10/2000
	TryMove (pos.x + delta.x, pos.y, true);  //Allow things to drop off.
      return;
    }

  // fudge a bit to make sure it doesn't hit
  bestslidefrac -= fudge;
  if (bestslidefrac > 0)
    {
      fixed_t newx = bestslidefrac * delta.x;
      fixed_t newy = bestslidefrac * delta.y;

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

  tmmove.x = bestslidefrac * delta.x;
  tmmove.y = bestslidefrac * delta.y;

  P_HitSlideLine(bestslideline);     // clip the moves

  delta.x = tmmove.x;
  delta.y = tmmove.y;

  if (!TryMove(pos.x+delta.x, pos.y+delta.y, true))
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
      line_opening_t *open = line_opening_t::Get(li, slidemo);

      // will it fit through?
      if (open->top >= slidemo->Top() &&
	  open->bottom <= slidemo->Feet())
	return true; // this line doesn't block movement
    }

  // the line does block movement, see if it is closer than best so far
  if (in->frac < bestslidefrac)
    {
      bestslidefrac = in->frac;
      bestslideline = li;
    }

  return false; // stop
}


///
void Actor::BounceWall(fixed_t nx, fixed_t ny)
{
  vec_t<fixed_t> delta(nx - pos.x, ny - pos.y, 0);
  vec_t<fixed_t> corner;

  fixed_t leadx, leady;

  slidemo = this;

  // trace from leading corner
  if (delta.x > 0)
    leadx = pos.x+radius;
  else
    leadx = pos.x-radius;

  if (delta.y > 0)
    leady = pos.y+radius;
  else
    leady = pos.y-radius;

  bestslidefrac = 2;
  bestslideline = NULL;

  corner.x = leadx; corner.y = leady;
  mp->PathTraverse(corner, corner + delta, PT_ADDLINES, PTR_BounceTraverse);

  if (!bestslideline)
    return;

  int side = P_PointOnLineSide(pos.x, pos.y, bestslideline);
  angle_t lineangle = R_PointToAngle2(0, 0, bestslideline->dx, bestslideline->dy);
  if (side == 1)
    lineangle += ANG180;

  angle_t moveangle = R_PointToAngle2(0, 0, delta.x, delta.y);
  angle_t newangle = 2*lineangle - moveangle;

  newangle >>= ANGLETOFINESHIFT;

  fixed_t movelen = 0.75 * P_AproxDistance(delta.x, delta.y); // lose energy in the bounce

  if (movelen < 1)
    movelen = 2;

  delta.x = movelen * finecosine[newangle];
  delta.y = movelen * finesine[newangle];
}






//==========================================================================
//   Line traces, autoaim, shooting with instahit weapons
//==========================================================================

/// i/o variables
static Actor  *shootthing;   ///< Instigator of the trace.

static bool    hitsky;       ///< Did we hit a sky plane or wall?
static Actor  *target_actor; ///< Actor that got hit (or NULL)
line_t *target_line;  ///< line_t that got hit (or NULL)

static float bottomsine, topsine; // vertical aiming range


mobjtype_t PuffType = MT_PUFF; ///< for Actor::LineAttack

void Map::SpawnPuff(const vec_t<fixed_t>& r, mobjtype_t pufftype)
{
  vec_t<fixed_t> p = r;
  p.z += P_SignedFRandom(6);

  DActor *puff = SpawnDActor(p, pufftype);

  if (target_actor && puff->info->seesound)
    S_StartSound(puff, puff->info->seesound); // Hit thing sound
  else if (puff->info->attacksound)
    S_StartSound(puff, puff->info->attacksound);


  switch (pufftype)
    {
    case MT_PUFF:
      puff->tics -= P_Random()&3;
      if (puff->tics < 1)
	puff->tics = 1;
        
      // don't make punches spark on the wall
      if (trace.length == MELEERANGE)
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
}


/// \brief Aiming up and down for missile attacks.
/// \ingroup g_ptr
/*!
  Seeks targets along the trace between bottomsine and topsine.
  Sets target_actor when a target is found.
  Returns true if the thing is not shootable, else continue through..
*/
static bool PTR_AimTraverse(intercept_t *in)
{
  float dist = trace.length * in->frac; // 3D distance

  if (in->isaline)
    {
      line_t *li = in->line;

      if (!(li->flags & ML_TWOSIDED))
	return false;               // stop

      // Crosses a two sided line.
      // A two sided line will restrict the possible target ranges.
      line_opening_t *open = line_opening_t::Get(li, shootthing); // TODO wrong, but...

      if (open->Range() <= 0)
	return false;               // stop

      float temp;

      if (li->frontsector->floorheight != li->backsector->floorheight)
        {
	  temp = (open->bottom - trace.start.z).Float() / dist;
	  if (temp > bottomsine)
	    bottomsine = temp;
        }

      if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
	  temp = (open->top - trace.start.z).Float() / dist;
	  if (temp < topsine)
	    topsine = temp;
        }

      if (topsine <= bottomsine)
	return false;               // stop


      // TODO trace.sin_pitch is used here, but it makes no sense! we're aiming up and down! it's not well-defined yet!
      if (li->frontsector->ffloors || li->backsector->ffloors)
        {
          int frontflag = P_PointOnLineSide(trace.start.x, trace.start.y, li);

          //SoM: Check 3D FLOORS!
	  for (ffloor_t *rover = li->frontsector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
		continue;

	      float highsine = (*rover->topheight - trace.start.z).Float() / dist;
	      float lowsine = (*rover->bottomheight - trace.start.z).Float() / dist;
	      if ((trace.sin_pitch >= lowsine && trace.sin_pitch <= highsine))
		return false; // hit the side of the floor

	      if (trace.lastz > *rover->topheight && trace.sin_pitch < 0 && trace.sin_pitch < highsine)
		frontflag |= 0x2;

	      if (trace.lastz < *rover->bottomheight && trace.sin_pitch > 0 && trace.sin_pitch > lowsine)
		frontflag |= 0x2;
	    }

	  for (ffloor_t *rover = li->backsector->ffloors; rover; rover = rover->next)
	    {
	      if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
		continue;

	      float highsine = (*rover->topheight - trace.start.z).Float() / dist;
	      float lowsine = (*rover->bottomheight - trace.start.z).Float() / dist;
	      if ((trace.sin_pitch >= lowsine && trace.sin_pitch <= highsine))
		return false; // hit the side of the floor

	      if (trace.lastz > *rover->topheight && trace.sin_pitch < 0 && trace.sin_pitch < highsine)
		frontflag |= 0x4;

	      if (trace.lastz < *rover->bottomheight && trace.sin_pitch > 0 && trace.sin_pitch > lowsine)
		frontflag |= 0x4;
	    }

          if ((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            return false;
        }

      trace.lastz = trace.start.z + fixed_t(trace.sin_pitch * dist); // TODO trace.sin_pitch is not well-defined

      return true; // shot continues
    }

  // shoot a thing
  Actor *th = in->thing;
  if (th == shootthing)
    return true; // can't shoot self

  if (!(th->flags & MF_VALIDTARGET) || !(th->flags & MF_SHOOTABLE) || (th->flags & MF_CORPSE))
    return true; // corpse or something
  // NOTE since top- and bottomsines are not adjusted, we may try to shoot through the thing!

  // check angles to see if the thing can be aimed at
  float thingtopsine = (th->Top() - trace.start.z).Float() / dist;

  if (thingtopsine < bottomsine)
    return true;                    // shot over the thing

  float thingbottomsine = (th->Feet() - trace.start.z).Float() / dist;

  if (thingbottomsine > topsine)
    return true;                    // shot under the thing

  // this thing can be hit! take aim!
  if (thingtopsine > topsine)
    thingtopsine = topsine;

  if (thingbottomsine < bottomsine)
    thingbottomsine = bottomsine;

  //added:15-02-98: find the sine just in the middle(y) of the thing!
  trace.sin_pitch = (thingtopsine + thingbottomsine)/2;
  target_actor = th;

  return false; // don't seek any farther
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
Actor *Actor::AimLineAttack(angle_t ang, float distance, float& sinpitch)
{
  shootthing = this;

  target_actor = NULL;

  float aimsine = Sin(pitch).Float(); // initial try, TODO bad heuristics for ffloors...

  //added:15-02-98: Fab comments...
  // Doom's base engine says that at a distance of 160,
  // the 2d graphics on the plane x,y correspond 1/1 with plane units
  const float ds = 10.0f/16; // tan of half the vertical fov

  topsine    = aimsine + ds; // TODO not correct... should add the angles and do some clipping
  bottomsine = aimsine - ds;
  // can't shoot outside view angles

  // start point
  vec_t<fixed_t> s(pos);
  s.z = Feet() +SHOOTFRAC*height -floorclip;

  // Since monsters shouldn't change their "pitch" angle, why not use the same routine for them also?
  fixed_t temp = distance * Cos(pitch); // XY length
  vec_t<fixed_t> delta(temp * Cos(ang), temp * Sin(ang), distance * aimsine);

  mp->PathTraverse(s, s+delta, PT_ADDLINES | PT_ADDTHINGS, PTR_AimTraverse);

  // found a target?
  sinpitch = trace.sin_pitch; // unchanged if no target was found
  return target_actor;
}



/// helper function for traces, updates lastz
/// \param[in] s sector to check
/// \return true if we hit a Z-plane (and thus modified frac)
bool trace_t::HitZPlane(sector_t *s)
{
  range_t r = s->FindZRange(lastz);

  float dz = sin_pitch * length * frac;
  lastz = start.z + dz; // z at intercept

  if (lastz > r.high) // hit ceiling
    {
      frac = (frac * (r.high - start.z).Float()) / dz;
      lastz = r.high;
      hitsky = (s->SkyCeiling() && r.high == s->ceilingheight);
      return true;
    }
  else if (lastz < r.low) // hit floor
    {
      frac = (frac * (r.low - start.z).Float()) / dz;
      lastz = r.low;
      hitsky = (s->SkyFloor() && r.low == s->floorheight);
      return true;
    }

  return false;
}


/// \brief Checks if a line trace is stopped by an intercept.
/// \ingroup g_ptr
/*!
  Sets target_actor or target_line if an actor or wall is hit.
  \return true if the trace continues after this intercept
*/
static bool PTR_LineTrace(intercept_t *in)
{
  // we need the right Map * from somewhere.
  Map *m = trace.mp;

  // NOTE: The Map::PathTraverse tracing system works strictly in the XY plane using blockmap.
  // Hence a Z-plane (floor, ceiling, fake floor) may actually intercept the trace
  // BEFORE it reaches its next designated intercept_t.

  trace.frac = in->frac; // actual intercept fraction (trace may hit horizontal planes too)

  if (in->isaline)
    {
      line_t *li = in->line;

      // crosses a two sided line, find front sector
      int side = P_PointOnLineSide(trace.start.x, trace.start.y, li);

      if (!li->sideptr[side]) // one-sided line, we must have noclipped to the wrong side
	return false; // stop here, hit nothing

      sector_t *front = li->sideptr[side]->sector;

      if (!trace.HitZPlane(front))
	{
	  // we did hit the line
	  if (li->special && interact)
	    m->ActivateLine(li, shootthing, 0, SPAC_IMPACT); // your documentation no longer confuses me, old version

	  if (li->flags & ML_TWOSIDED)
	    {
	      // HitZPlane updated trace.lastz
	      // Let's see what happens with backsector geometry:
	      switch (li->sideptr[!side]->sector->CheckZ(trace.lastz))
		{
		case sector_t::z_Open: // went thru, shot continues
		  return true; 

		case sector_t::z_Sky: // hit a skywall (no puffs or scorchmarks!)
		  hitsky = true;
		  break;

		default:
		  break;
		  // hit a solid wall
		}
	    }

	  target_line = li; // we impacted a wall
	}
    }
  else
    {
      // intercepted by a thing
      Actor *th = in->thing;
      if (th == shootthing)
	return true; // can't shoot self

      if (!(th->flags & MF_SHOOTABLE))
	return true; // nonshootable

      // an Actor can be in several sectors at once, so we need the exact impact point and its sector
      vec_t<fixed_t> impact_point(trace.Point(trace.frac));

      if (impact_point.z > th->Top())
	return true; // over

      if (impact_point.z < th->Feet())
	return true; // under

      sector_t *sec = m->R_PointInSubsector(impact_point.x, impact_point.y)->sector;
      
      if (trace.HitZPlane(sec))
	; // Z-plane shielded Actor, trace hit the plane
      else
	target_actor = th; // we hit a thing
    }

  // don't go any farther
  return false;
}



/// \brief Shoots an insta-hit projectile from Actor to the direction determined by (yaw, pitch)
/// \ingroup g_trace
/*!
  The function performs a trace starting from (roughly) the center of the actor.
  Contacted target will be damaged.
  \param ang yaw angle for the attack
  \param distance max range for the projectile (including z direction!)
  \param sine sin(pitch) for the attack
  \param damage damage amount, if negative, cause no interactions
  \param dtype damage type
  \return pointer to the target or NULL if none was hit
*/
Actor *Actor::LineAttack(angle_t ang, float distance, float sine, int damage, int dtype)
{
  // do the trace
  LineTrace(ang, distance, sine, damage >= 0);

  if (hitsky)
    return NULL;

  if (damage < 0)
    return target_actor;

  vec_t<fixed_t> ipoint;

  // do the damage, puffs and splats
  
  if (target_line)
    {
      // hit wall, make a wall splat
      divline_t divl;
      divl.MakeDivline(target_line);
      fixed_t fr = P_InterceptVector(&divl, &trace.dl);
      int side = P_PointOnLineSide(trace.start.x, trace.start.y, target_line);
      mp->R_AddWallSplat(target_line, side, "A_DMG1", trace.lastz, fr, SPLATDRAWMODE_SHADE);

      // position a bit closer
      ipoint = trace.Point(trace.frac - 4.0 / trace.length);
    }
  else if (target_actor)
    {
      // traces should continue through ghosts
      // check for physical attacks on a ghost FIXME this is stupid. We should use some sort of damage type system.
      // (if damagetype != dt_ethereal ...)
      /*
	if (game.mode == gm_heretic && (th->flags & MF_SHADOW))
	if (shootthing->Type() == Thinker::tt_ppawn)
	if (shootthing->readyweapon == wp_staff)
	return true;
      */

      // hit thing, do damage
      target_actor->Damage(this, this, damage, dtype);

      // position a bit closer
      ipoint = trace.Point(trace.frac - 10.0 / trace.length);

      if (!(target_actor->flags & MF_NOBLOOD ||
	    target_actor->flags2 & MF2_INVULNERABLE))
	{

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

	  mp->SpawnBloodSplats(ipoint, damage, trace.delta.x, trace.delta.y);
	}

      if (game.mode < gm_heretic)
	return target_actor;

      if (PuffType == MT_BLASTERPUFF1)   
	PuffType = MT_BLASTERPUFF2;  // Make blaster big puff
    }
  else
    // hit Z-plane or nothing
    // position a bit closer
    ipoint = trace.Point(trace.frac - 4.0 / trace.length);

  if (PuffType != MT_NONE)
    mp->SpawnPuff(ipoint, PuffType);

  return target_actor;

  /* TODO missed cleric flame attack:
     case MT_FLAMEPUFF:
     mp->SpawnPuff(trace.Point(1), MT_FLAMEPUFF);
  */
}


/// \brief Sends a trace from Actor to the direction determined by (yaw, pitch)
/// \ingroup g_trace
/*!
  The function performs a trace starting from (roughly) the center of the actor.
  \param ang yaw angle for the attack
  \param distance max range for the projectile (including z direction!)
  \param sine sin(pitch) for the attack
  \param inter does the trace cause interactions in the Map?
  \return pointer to the target Actor or NULL if something else (line, plane, nothing) was hit
*/
Actor *Actor::LineTrace(angle_t ang, float distance, float sine, bool inter)
{
  shootthing = this;
  interact = inter;

  target_line  = NULL;
  target_actor = NULL;
  hitsky = false;

  // start point
  vec_t<fixed_t> s(pos);
  s.z = Feet() +SHOOTFRAC*height -floorclip;

  float temp = sqrt(1 - sine*sine) * distance; // XY length

  // end point
  vec_t<fixed_t> delta(temp * Cos(ang), temp * Sin(ang), fixed_t(sine*distance));

  mp->PathTraverse(s, s+delta, PT_ADDLINES | PT_ADDTHINGS, PTR_LineTrace);

  return target_actor;
}



//==========================================================================
//   Blood spawning
//==========================================================================

static Actor   *bloodthing;
static fixed_t  blood_x, blood_y;

/// \brief Spray blood splats on walls.
/// \ingroup g_ptr
/*!
  Adds a wall splat on the first solid wall encountered.
*/
static bool PTR_BloodTraverse(intercept_t *in)
{
  if (in->isaline)
    {
      line_t *li = in->line;
      fixed_t z = bloodthing->pos.z + P_SignedFRandom(3);
      if (li->flags & ML_TWOSIDED)
	{
	  line_opening_t *open = line_opening_t::Get(li, bloodthing);

	  // hit lower or upper texture?
	  if ((li->frontsector->floorheight == li->backsector->floorheight || open->bottom <= z) &&
	      (li->frontsector->ceilingheight == li->backsector->ceilingheight || open->top >= z))
	    return true; // nope
	}

      divline_t divl;
      divl.MakeDivline(li);

      fixed_t frac = P_InterceptVector(&divl, &trace.dl);
      if (game.mode >= gm_heretic)
	trace.mp->R_AddWallSplat(li, P_PointOnLineSide(blood_x,blood_y,li),"BLODC0", z, frac, SPLATDRAWMODE_TRANS);
      else
	trace.mp->R_AddWallSplat(li, P_PointOnLineSide(blood_x,blood_y,li),"BLUDC0", z, frac, SPLATDRAWMODE_TRANS);
      return false;
    }

  //continue
  return true;
}


// First calls SpawnBlood for the usual blood sprites, then spawns blood splats around on walls.
void Map::SpawnBloodSplats(const vec_t<fixed_t>& r, int damage, fixed_t px, fixed_t py)
{
  // spawn the usual falling blood sprites at location
  bloodthing = SpawnBlood(r, damage);

  angle_t angle;
  angle_t anglemul = 1;

  // traverse all linedefs and mobjs from the blockmap containing t1,
  // to the blockmap containing the dest. point.
  // Call the function for each mobj/line on the way,
  // starting with the mobj/linedef at the shortest distance...

  if (!px && !py)
    {   
      // from inside
      angle = 0;
      anglemul = 2; 
    }
  else
    {
      // get direction of damage
      angle = R_PointToAngle2(0, 0, px, py);
    }

  int distance = damage * 6;
  int numsplats = damage/3 + 1;

  if (numsplats > 20)
    numsplats = 20;  // BFG is funy without this check

  //CONS_Printf ("spawning %d bloodsplats at distance of %d\n", numsplats, distance);
  //CONS_Printf ("damage %d\n", damage);
  blood_x = r.x;
  blood_y = r.y;

  for (int i=0; i<numsplats; i++)
    {
      // find random angle between 0-180deg centered on damage angle
      angle_t anglesplat = angle + (P_Random() - 128) * (ANG90 / 128 * anglemul);

      vec_t<fixed_t> delta(distance * Cos(anglesplat), distance * Sin(anglesplat), 0);

      PathTraverse(r, r+delta, PT_ADDLINES, PTR_BloodTraverse);
    }

#ifdef FLOORSPLATS
  // add a test floor splat
  R_AddFloorSplat(bloodthing->subsector, "STEP2", r.x, r.y, bloodthing->floorz, SPLATDRAWMODE_SHADE);
#endif
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
  CONS_Printf("Line %d: s = %d, tag = %d, flags = %x\n", line-usething->mp->lines, line->special, line->tag, line->flags);
  if (!line->special)
    {
      line_opening_t *open = line_opening_t::Get(line, usething);
      // TEST: use through a hole only if you can reach it
      if (open->Range() <= 0 || open->top < usething->Feet() || open->bottom > usething->Top())
        {
	  S_StartSound(usething, sfx_usefail);

	  // can't use through a wall
	  return false;
        }

      // not a special line, but keep checking
      return true;
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


/// Looks for special lines in front of the player to activate.
void PlayerPawn::UseLines()
{
  usething = this;

  vec_t<fixed_t> delta(USERANGE * Cos(yaw), USERANGE * Sin(yaw), 0);

  mp->PathTraverse(pos, pos+delta, PT_ADDLINES, PTR_UseTraverse);
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
	  line_opening_t *open = line_opening_t::Get(line, PuzzleItemUser);
	  if (open->Range() <= 0)
	    {
	      S_StartSound(PuzzleItemUser, PuzzleItemUser->info->puzzfailsound);
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
  PuzzleItemUser = this;
  PuzzleItemType = type;
  PuzzleActivated = false;

  vec_t<fixed_t> delta(USERANGE * Cos(yaw), USERANGE * Sin(yaw), 0);

  mp->PathTraverse(pos, pos+delta, PT_ADDLINES | PT_ADDTHINGS, PTR_PuzzleItemTraverse);
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
      if (thing->Damage(Bomb.b, Bomb.owner, damage, Bomb.dtype) &&
	  !(thing->flags & MF_NOBLOOD) &&
	  (Bomb.dtype & dt_TYPEMASK) < dt_LIVING)
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

  Bomb.b = this;
  Bomb.owner = culprit;
  Bomb.damage = damage;
  Bomb.dtype = dtype;
  Bomb.damage_owner = downer;

  mp->BlockIterateThingsRadius(pos.x, pos.y, Bomb.radius + MAXRADIUS, PIT_RadiusAttack);
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
  // TODO check this
  bool onfloor = (thing->pos.z <= thing->floorz);

  thing->CheckPosition(thing->pos.x, thing->pos.y, true);

  // what about stranding a monster partially off an edge?

  thing->floorz = PosCheck.op.bottom;
  thing->ceilingz = PosCheck.op.top;

  if (onfloor && !(thing->flags & MF_NOGRAVITY))
    {
      // walking monsters rise and fall with the floor TODO allow 9 units altitude (Hexen)?
      thing->pos.z = thing->floorz;
    }
  else
    {
      // don't adjust a floating monster unless forced to
      //added:18-04-98:test onfloor
      if (!onfloor)                    //was tmsectorceilingz
	if (thing->Top() > PosCheck.op.top)
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
      BlockThingsIterator(x, y, PIT_ChangeSector);

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

  BlockIterateLinesRadius(x, y, tmthing->radius, PIT_GetSectors);

  // Add the sector of the (x,y) point to sector_list.

  sector_list = msecnode_t::AddToSectorlist(thing->subsector->sector, thing, sector_list);

  // Now delete any nodes that won't be used. These are the ones where
  // m_thing is still NULL.

  thing->touching_sectorlist = msecnode_t::CleanSectorlist(sector_list);
  sector_list = NULL; // just to be sure

  /*
  for (msecnode_t *p = thing->touching_sectorlist; p; p = p->m_tnext)
    if (!p->m_thing || !p->m_sector)
      I_Error("xxsdfsd");
  */
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

  if (!(thing->flags & MF_SOLID))
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;

  if (abs(thing->pos.x - tmx) >= blockdist || abs(thing->pos.y - tmy) >= blockdist)
    return true; // didn't hit it

  // Monsters don't stomp things except unless they come from a bossbrain shooter.
  // Not allowed to stomp things?
  if (!(tmthing->flags2 & MF2_TELESTOMP))
    return false;

  if (!(thing->flags & MF_SHOOTABLE))
    return false; // cannot be stomped

  thing->Damage(tmthing, tmthing, 10000, dt_telefrag | dt_always);

  return true;
}


bool Actor::TeleportMove(fixed_t nx, fixed_t ny)
{
  // kill anything occupying the position
  tmthing = this;
  sector_t *newsec = mp->R_PointInSubsector(nx, ny)->sector;

  // TODO do a linecheck first to see if we fit
  // this is a simplification which only works fully within a sector with no fakefloors
  PosCheck.op.bottom = newsec->floorheight;
  PosCheck.op.top = newsec->ceilingheight;

  // stomp on any things contacted
  if (!mp->BlockIterateThingsRadius(nx, ny, radius + MAXRADIUS, PIT_StompThing))
    return false;

  // the move is ok, so link the thing into its new position
  UnsetPosition();
  pos.x = nx;
  pos.y = ny;
  SetPosition();

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
  tmthing = actor;
  // stomp on any things contacted
  actor->mp->BlockIterateThingsRadius(actor->pos.x, actor->pos.y, actor->radius+MAXRADIUS, PIT_ThrustStompThing);
}
