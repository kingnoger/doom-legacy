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
// Revision 1.29  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.28  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.27  2004/10/14 19:35:30  smite-meister
// automap, bbox_t
//
// Revision 1.26  2004/10/11 11:23:46  smite-meister
// map utils
//
// Revision 1.25  2004/09/13 20:43:30  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.22  2004/04/01 09:16:16  smite-meister
// Texture system bugfixes
//
// Revision 1.19  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.16  2003/11/27 11:28:25  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.15  2003/11/12 11:07:22  smite-meister
// Serialization done. Map progression.
//
// Revision 1.13  2003/05/30 13:34:45  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.10  2003/04/24 20:30:12  hurdler
// Remove lots of compiling warnings
//
// Revision 1.8  2003/03/15 20:07:16  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.7  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.6  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.5  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/16 22:11:46  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:11:39  smite-meister
// Blindness and missile clipping bugs fixed
//
// Revision 1.1.1.1  2002/11/16 14:18:01  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Movement, collision handling. Shooting and aiming.

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

Actor    *tmthing;
static int tmflags; // useless? same as tmthing->flags?
fixed_t tmx, tmy;

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
bool     floatok;

fixed_t  tmfloorz;
fixed_t  tmceilingz;
fixed_t  tmdropoffz;
int      tmfloorpic;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
vector<line_t *> spechit;

Actor *tmfloorthing;   // the thing corresponding to tmfloorz
                                // or NULL if tmfloorz is from a sector

//added:28-02-98: used at P_ThingHeightClip() for moving sectors
fixed_t  tmsectorfloorz;
fixed_t  tmsectorceilingz;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t *ceilingline;

// set by PIT_CheckLine() for any line that stopped the PIT_CheckLine()
// that is, for any line which is 'solid'
line_t *blockingline;

Actor *BlockingMobj; //thing that blocked position (NULL if not blocked, or blocked by a line)

static msecnode_t *sector_list = NULL;
msecnode_t *P_AddSecnode(sector_t *s, Actor *thing, msecnode_t *nextnode);
msecnode_t *P_DelSecnode(msecnode_t *node);

bbox_t tmb; // a bounding box


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
  xl = (tmb[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tmb[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tmb[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tmb[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

  for (bx=xl; bx<=xh; bx++)
    for (by=yl; by<=yh; by++)
      if (!BlockLinesIterator(bx, by, func))
	return false;
 
  return true;
}


//===========================================
//    Teleportation
//===========================================

static bool PIT_StompThing(Actor *thing)
{
  // don't clip against self
  if (thing == tmthing)
    return true;

  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;

  if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
    return true; // didn't hit it

  // monsters don't stomp things except on boss level
  // FIXME boss level? does this mean a brainshooter? last condition was gamemap != 30
  //if (game.mode != gm_heretic && tmthing->Type() != Thinker::tt_ppawn && thing->mp->braintargets.size())
  //  return false;

  // Not allowed to stomp things
  if (!(tmthing->flags2 & MF2_TELESTOMP))
    return false;

  thing->Damage(tmthing, tmthing, 10000, dt_telefrag | dt_always);

  return true;
}


bool Actor::TeleportMove(fixed_t nx, fixed_t ny)
{
  int  xl, xh, yl, yh, bx, by;

  // kill anything occupying the position
  tmthing = this;
  tmflags = flags;

  tmb.Set(nx, ny, radius);

  subsector_t *newsubsec = mp->R_PointInSubsector(nx,ny);
  ceilingline = NULL;

  // FIXME do a checkposition first

  // The base floor/ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.
  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;
  tmfloorpic = newsubsec->sector->floorpic;

  validcount++;
  spechit.clear();

  // stomp on any things contacted
  xl = (tmb[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmb[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmb[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmb[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockThingsIterator(bx,by,PIT_StompThing))
	return false;

  // the move is ok,
  // so link the thing into its new position
  UnsetPosition();
  x = nx;
  y = ny;
  SetPosition();

  return true;
}


//===========================================
//  Spikes
//===========================================

static bool PIT_ThrustStompThing(Actor *thing)
{
  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;
  if (abs(thing->x - tmthing->x) >= blockdist || 
      abs(thing->y - tmthing->y) >= blockdist ||
      (thing->z > tmthing->z + tmthing->height))
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

  tmb.Set(actor->x, actor->y, actor->radius);

  xl = (tmb[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmb[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmb[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmb[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

  // stomp on any things contacted
  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      mp->BlockThingsIterator(bx,by,PIT_ThrustStompThing);
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
	  blockx = (x - mp->bmaporgx)>>MAPBLOCKSHIFT;
	  blocky = (y - mp->bmaporgy)>>MAPBLOCKSHIFT;

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
  subsector_t *ss = mp->R_PointInSubsector(x,y);
  subsector = ss;

  if (!(flags & MF_NOSECTOR))
    {
      // invisible things don't go into the sector links
      sector_t *sec = ss->sector;
#ifdef PARANOIA
      if (sprev != NULL || snext != NULL)
	I_Error("Actor::SetPosition: thing at (%d, %d) is already linked", x, y);
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

      mp->CreateSecNodeList(this,x,y);
    }

  int blockx, blocky;
  Actor **link;

  // link into blockmap
  if (! (flags & MF_NOBLOCKMAP))
    {
      // inert things don't need to be in blockmap
      blockx = (x - mp->bmaporgx)>>MAPBLOCKSHIFT;
      blocky = (y - mp->bmaporgy)>>MAPBLOCKSHIFT;

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

// Iterator function for Actor->Actor collision checks. Global variable 'tmthing'
// is the active object whose collisions are checked. 'thing' is any other object
// to which it may collide.
static bool PIT_CheckThing(Actor *thing)
{
  // don't clip against self
  if (thing == tmthing)
    return true;

  if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
    return true;

  if (thing->flags & MF_NOCLIPTHING)
    return true;

  fixed_t blockdist = thing->radius + tmthing->radius;

  if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
    {
      // didn't hit it
      return true;
    }
  BlockingMobj = thing;

  // heretic stuffs
  if ((tmthing->flags2 & MF2_PASSMOBJ) && !(thing->flags & MF_SPECIAL))
    { // check if a mobj passed over/under another object
      /*
      if ((tmthing->type == MT_IMP || tmthing->type == MT_WIZARD)
	 && (thing->type == MT_IMP || thing->type == MT_WIZARD))
        { // don't let imps/wizards fly over other imps/wizards
	// FIXME why not? it's much easier this way.
	  return false;
        }
      */
      if (tmthing->z >= thing->z + thing->height)
        {
	  // over
	  return true;
        }
      else if (tmthing->z + tmthing->height < thing->z)
        {
	  // under thing
	  return true;
        }
    }

  return !(tmthing->Touch(thing));
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


// Iterator for Actor->Line collision checking
// Adjusts tmfloorz and tmceilingz as lines are contacted
static bool PIT_CheckLine(line_t *ld)
{
  if (!tmb.BoxTouchBox(ld->bbox))
    return true;

  if (tmb.BoxOnLineSide(ld) != -1)
    return true;

  // A line has been hit

  // The moving thing's destination position will cross
  // the given line.
  // If this should not be allowed, return false.
  // If the line is special, keep track of it
  // to process later if the move is proven ok.
  // NOTE: specials are NOT sorted by order,
  // so two special lines that are only 8 pixels apart
  // could be crossed in either order.

  // 10-12-99 BP: moved this line to out of the if so upper and 
  //              lower texture can be hit by a splat
  blockingline = ld;

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
	  push = false;
	}
    }

  if (stopped)
    {
      // take damage from impact
      if (tmthing->eflags & MFE_BLASTED)
	tmthing->Damage(NULL, NULL, tmthing->mass >> 5);

      if (push)
	CheckForPushSpecial(ld, 0, tmthing);
      return false;
    }

  // set openrange, opentop, openbottom
  P_LineOpening(ld);

  // adjust floor / ceiling heights
  if (opentop < tmceilingz)
    {
      tmsectorceilingz = tmceilingz = opentop;
      ceilingline = ld;
    }

  if (openbottom > tmfloorz)
    tmsectorfloorz = tmfloorz = openbottom;

  if (lowfloor < tmdropoffz)
    tmdropoffz = lowfloor;

  // if contacted a special line, add it to the list
  if (ld->special)
    spechit.push_back(ld);

  return true;
}




//
// Checks if a shootable linedef should be triggered
//
void Actor::CheckMissileImpact()
{
  int n = spechit.size();
  if (!(flags & MF_MISSILE) || !n || !owner)
    return;

  // monsters don't shoot triggers
  if (!(owner->flags & MF_NOTMONSTER))
    return;

  for (int i = n - 1; i >= 0; i--)
    mp->ActivateLine(spechit[i], owner, 0, SPAC_IMPACT);
}



// Attempt to move to a new position,
// crossing special lines in the way.
bool Actor::TryMove(fixed_t nx, fixed_t ny, bool allowdropoff)
{
  //  max Z move up or down without jumping
  //  above this, a heigth difference is considered as a 'dropoff'
  const fixed_t MAXSTEPMOVE = 24 * FRACUNIT;

  fixed_t oldx, oldy;
  int     side;
  int     oldside;

  floatok = false;

  if (!CheckPosition(nx, ny))
    {
      CheckMissileImpact();
      return false;       // solid wall or thing
    }

  if (!(flags & MF_NOCLIPLINE))
    {
      fixed_t maxstep = MAXSTEPMOVE;
      if (tmceilingz - tmfloorz < height)
	{
	  CheckMissileImpact();
	  return false; // doesn't fit in z direction
	}

      floatok = true;

      if (tmceilingz < z + height && !(eflags & MFE_FLY))
	{
	  CheckMissileImpact();
	  return false; // must lower itself to fit
	}

      if (eflags & MFE_FLY)
	{
	  if (z + height > tmceilingz)
	    {
	      pz = -8*FRACUNIT;
	      return false;
	    }
	  else if (z < tmfloorz && tmfloorz-tmdropoffz > 24*FRACUNIT)
	    {
	      pz = 8*FRACUNIT;
	      return false;
	    }
	}

      // jump out of water
      if ((eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)) == (MFE_UNDERWATER|MFE_TOUCHWATER))
	maxstep = 37*FRACUNIT;

      if (tmfloorz - z > maxstep)
	// The Minotaur floor fire (MT_MNTRFX2) can step up any amount
	// FIXME && type != MT_MNTRFX2
	{
	  CheckMissileImpact();
	  return false;       // too big a step up
	}

      if ((flags & MF_MISSILE) && tmfloorz > z)
	CheckMissileImpact();

      if (!boomsupport || !allowdropoff)
	if (!(flags & (MF_DROPOFF | MF_FLOAT)) && !tmfloorthing
	    && tmfloorz - tmdropoffz > MAXSTEPMOVE
	    && !(eflags & MFE_BLASTED))
	  return false;       // don't go over a dropoff (unless blasted)

      if (flags2 & MF2_CANTLEAVEFLOORPIC
	  && (tmfloorpic != subsector->sector->floorpic || tmfloorz != z))
	// must stay within a sector of a certain floor type
	return false;
    }

  // the move is ok, so link the thing into its new position
  UnsetPosition();

  oldx = x;
  oldy = y;
  x = nx;
  y = ny;

  //added:28-02-98:
  if (tmfloorthing)
    eflags &= ~MFE_ONGROUND;  //not on real floor
  else
    eflags |= MFE_ONGROUND;

  SetPosition();

  // Heretic fake water...
  if ((flags2 & MF2_FOOTCLIP) && (subsector->sector->floortype >= FLOOR_LIQUID))
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

	  side = P_PointOnLineSide (x, y, ld);
	  oldside = P_PointOnLineSide (oldx, oldy, ld);
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


// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
bool P_ThingHeightClip(Actor *thing)
{
  bool onfloor = (thing->z <= thing->floorz);

  thing->CheckPosition(thing->x, thing->y);

  // what about stranding a monster partially off an edge?

  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;

  if (!tmfloorthing && onfloor && !(thing->flags & MF_NOGRAVITY))
    {
      // walking monsters rise and fall with the floor
      thing->z = thing->floorz;
    }
  else
    {
      // don't adjust a floating monster unless forced to
      //added:18-04-98:test onfloor
      if (!onfloor)                    //was tmsectorceilingz
	if (thing->z+thing->height > tmceilingz)
	  thing->z = thing->ceilingz - thing->height;

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
      && thing->z >= thing->floorz)
    return false;

  return true;
}



//==========================================================================
// Sliding moves
// Allows the player to slide along any angled walls.
//==========================================================================

fixed_t         bestslidefrac;
fixed_t         secondslidefrac;

line_t *bestslideline;
line_t *secondslideline;

Actor *slidemo;

fixed_t         tmxmove;
fixed_t         tmymove;


// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
void P_HitSlideLine(line_t *ld)
{
  int                 side;

  angle_t             lineangle;
  angle_t             moveangle;
  angle_t             deltaangle;

  fixed_t             movelen;
  fixed_t             newlen;


  if (ld->slopetype == ST_HORIZONTAL)
    {
      tmymove = 0;
      return;
    }

  if (ld->slopetype == ST_VERTICAL)
    {
      tmxmove = 0;
      return;
    }

  side = P_PointOnLineSide (slidemo->x, slidemo->y, ld);

  lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);

  if (side == 1)
    lineangle += ANG180;

  moveangle = R_PointToAngle2 (0,0, tmxmove, tmymove);
  deltaangle = moveangle-lineangle;

  if (deltaangle > ANG180)
    deltaangle += ANG180;
  //  I_Error ("SlideLine: ang>ANG180");

  lineangle >>= ANGLETOFINESHIFT;
  deltaangle >>= ANGLETOFINESHIFT;

  movelen = P_AproxDistance (tmxmove, tmymove);
  newlen = FixedMul (movelen, finecosine[deltaangle]);

  tmxmove = FixedMul (newlen, finecosine[lineangle]);
  tmymove = FixedMul (newlen, finesine[lineangle]);
}


/// tries sliding along the intercept
static bool PTR_SlideTraverse(intercept_t *in)
{
#ifdef PARANOIA
  if (!in->isaline)
    I_Error ("PTR_SlideTraverse: not a line?");
#endif

  line_t *li = in->line;

  if (!(li->flags & ML_TWOSIDED))
    {
      if (P_PointOnLineSide(slidemo->x, slidemo->y, li))
        {
	  // don't hit the back side
	  return true;
        }
      goto isblocking;
    }

  // set openrange, opentop, openbottom
  P_LineOpening (li);

  if (openrange < slidemo->height)
    goto isblocking;                // doesn't fit

  if (opentop - slidemo->z < slidemo->height)
    goto isblocking;                // mobj is too high

  if (openbottom - slidemo->z > 24*FRACUNIT)
    goto isblocking;                // too big a step up

  // this line doesn't block movement
  return true;

  // the line does block movement,
  // see if it is closer than best so far
 isblocking:

  if (in->frac < bestslidefrac)
    {
      secondslidefrac = bestslidefrac;
      secondslideline = bestslideline;
      bestslidefrac = in->frac;
      bestslideline = li;
    }

  return false;       // stop
}



// The px / py move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
void Map::SlideMove(Actor *mo)
{
  fixed_t             leadx;
  fixed_t             leady;
  fixed_t             trailx;
  fixed_t             traily;
  fixed_t             newx;
  fixed_t             newy;
  int                 hitcount;

  slidemo = mo;
  hitcount = 0;

 retry:
  if (++hitcount == 3)
    goto stairstep;         // don't loop forever


  // trace along the three leading corners
  if (mo->px > 0)
    {
      leadx = mo->x + mo->radius;
      trailx = mo->x - mo->radius;
    }
  else
    {
      leadx = mo->x - mo->radius;
      trailx = mo->x + mo->radius;
    }

  if (mo->py > 0)
    {
      leady = mo->y + mo->radius;
      traily = mo->y - mo->radius;
    }
  else
    {
      leady = mo->y - mo->radius;
      traily = mo->y + mo->radius;
    }

  bestslidefrac = FRACUNIT+1;

  PathTraverse (leadx, leady, leadx+mo->px, leady+mo->py,
		   PT_ADDLINES, PTR_SlideTraverse);
  PathTraverse (trailx, leady, trailx+mo->px, leady+mo->py,
		   PT_ADDLINES, PTR_SlideTraverse);
  PathTraverse (leadx, traily, leadx+mo->px, traily+mo->py,
		   PT_ADDLINES, PTR_SlideTraverse);

  // move up to the wall
  if (bestslidefrac == FRACUNIT+1)
    {
      // the move most have hit the middle, so stairstep
    stairstep:
      if (!mo->TryMove(mo->x, mo->y + mo->py, true)) //SoM: 4/10/2000
	mo->TryMove (mo->x + mo->px, mo->y, true);  //Allow things to
      return;                                             //drop off.
    }

  // fudge a bit to make sure it doesn't hit
  bestslidefrac -= 0x800;
  if (bestslidefrac > 0)
    {
      newx = FixedMul (mo->px, bestslidefrac);
      newy = FixedMul (mo->py, bestslidefrac);

      if (!mo->TryMove(mo->x+newx, mo->y+newy, true))
	goto stairstep;
    }

  // Now continue along the wall.
  // First calculate remainder.
  bestslidefrac = FRACUNIT-(bestslidefrac+0x800);

  if (bestslidefrac > FRACUNIT)
    bestslidefrac = FRACUNIT;

  if (bestslidefrac <= 0)
    return;

  tmxmove = FixedMul (mo->px, bestslidefrac);
  tmymove = FixedMul (mo->py, bestslidefrac);

  P_HitSlideLine (bestslideline);     // clip the moves

  mo->px = tmxmove;
  mo->py = tmymove;

  if (!mo->TryMove(mo->x+tmxmove, mo->y+tmymove, true))
    {
      goto retry;
    }
}


//==========================================================================
//   Autoaim, shooting with instahit weapons
//==========================================================================

Actor *linetarget;     // who got hit (or NULL)
Actor *shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t         shootz;
fixed_t         lastz; //SoM: The last z height of the bullet when it crossed a line

int             la_damage;
int             la_dtype;
fixed_t         attackrange;

fixed_t         aimslope;


mobjtype_t PuffType = MT_PUFF;
Actor *PuffSpawned;

void Map::SpawnPuff(fixed_t x, fixed_t y, fixed_t z)
{
  z += P_SignedRandom()<<10;

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
      puff->pz = FRACUNIT;
      break;
    case MT_HAMMERPUFF:
    case MT_GAUNTLETPUFF1:
    case MT_GAUNTLETPUFF2:
      puff->pz = int(.8*FRACUNIT);
      break;
    default:
      break;
    }
  PuffSpawned = puff;
}

// Sets linetarget and aimslope when a target is aimed at.
// Returns true if the thing is not shootable, else continue through..
static bool PTR_AimTraverse(intercept_t *in)
{
  line_t *li;
  Actor *th;
  fixed_t             slope;
  fixed_t             thingtopslope;
  fixed_t             thingbottomslope;
  fixed_t             dist;
  int                 dir;

  extern fixed_t bottomslope, topslope;

  if (in->isaline)
    {
      li = in->line;

      if (!(li->flags & ML_TWOSIDED))
	return false;               // stop

      // Crosses a two sided line.
      // A two sided line will restrict
      // the possible target ranges.
      tmthing = NULL;
      P_LineOpening (li);

      if (openbottom >= opentop)
	return false;               // stop

      dist = FixedMul (attackrange, in->frac);

      if (li->frontsector->floorheight != li->backsector->floorheight)
        {
	  slope = FixedDiv (openbottom - shootz , dist);
	  if (slope > bottomslope)
	    bottomslope = slope;
        }

      if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
	  slope = FixedDiv (opentop - shootz , dist);
	  if (slope < topslope)
	    topslope = slope;
        }

      if (topslope <= bottomslope)
	return false;               // stop

      if (li->frontsector->ffloors || li->backsector->ffloors)
        {
          int  frontflag;

          dir = aimslope > 0 ? 1 : aimslope < 0 ? -1 : 0;

          frontflag = P_PointOnLineSide(shootthing->x, shootthing->y, li);

          //SoM: Check 3D FLOORS!
          if (li->frontsector->ffloors)
	    {
	      ffloor_t *rover = li->frontsector->ffloors;
	      fixed_t    highslope, lowslope;

	      for(; rover; rover = rover->next)
		{
		  if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

		  highslope = FixedDiv (*rover->topheight - shootz, dist);
		  lowslope = FixedDiv (*rover->bottomheight - shootz, dist);
		  if ((aimslope >= lowslope && aimslope <= highslope))
		    return false;

		  if (lastz > *rover->topheight && dir == -1 && aimslope < highslope)
		    frontflag |= 0x2;

		  if (lastz < *rover->bottomheight && dir == 1 && aimslope > lowslope)
		    frontflag |= 0x2;
		}
	    }

          if (li->backsector->ffloors)
	    {
	      ffloor_t *rover = li->backsector->ffloors;
	      fixed_t    highslope, lowslope;

	      for(; rover; rover = rover->next)
		{
		  if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

		  highslope = FixedDiv (*rover->topheight - shootz, dist);
		  lowslope = FixedDiv (*rover->bottomheight - shootz, dist);
		  if ((aimslope >= lowslope && aimslope <= highslope))
		    return false;

		  if (lastz > *rover->topheight && dir == -1 && aimslope < highslope)
		    frontflag |= 0x4;

		  if (lastz < *rover->bottomheight && dir == 1 && aimslope > lowslope)
		    frontflag |= 0x4;
		}
	    }
          if ((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            return false;
        }

      lastz = FixedMul (aimslope, dist) + shootz;

      return true;                    // shot continues
    }

  // shoot a thing
  th = in->thing;
  if (th == shootthing)
    return true;                    // can't shoot self

  // TODO pods should not be targeted it seems. Add a new flag?
  // || (th->type == MT_POD))
  if ((!(th->flags & MF_SHOOTABLE)) || (th->flags & MF_CORPSE))
    return true; // corpse or something

  // check angles to see if the thing can be aimed at
  dist = FixedMul (attackrange, in->frac);
  thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

  //added:15-02-98: bottomslope is negative!
  if (thingtopslope < bottomslope)
    return true;                    // shot over the thing

  thingbottomslope = FixedDiv (th->z - shootz, dist);

  if (thingbottomslope > topslope)
    return true;                    // shot under the thing

  // this thing can be hit!
  if (thingtopslope > topslope)
    thingtopslope = topslope;

  if (thingbottomslope < bottomslope)
    thingbottomslope = bottomslope;

  //added:15-02-98: find the slope just in the middle(y) of the thing!
  aimslope = (thingtopslope+thingbottomslope)/2;
  linetarget = th;

  return false;                       // don't go any farther
}


// Firing instant-hit weapons
//added:18-02-98: added clipping the shots on the floor and ceiling.
static bool PTR_ShootTraverse(intercept_t *in)
{
  fixed_t             x;
  fixed_t             y;
  fixed_t             z;
  fixed_t             frac;

  line_t *li;
  sector_t *sector=NULL;

  fixed_t             slope;
  fixed_t             dist;
  fixed_t             thingtopslope;
  fixed_t             thingbottomslope;

  fixed_t             floorz = 0;  //SoM: Bullets should hit fake floors!
  fixed_t             ceilingz = 0;

  //added:18-02-98:
  fixed_t        distz;    //dist between hit z on wall       and gun z
  fixed_t        clipz;    //dist between hit z on floor/ceil and gun z
  bool        hitplane;    //true if we clipped z on floor/ceil plane
  bool        diffheights; //check for sky hacks with different ceil heights

  int            sectorside;
  int            dir;

  if (aimslope > 0)
    dir = 1;
  else if (aimslope < 0)
    dir = -1;
  else
    dir = 0;

  // we need the right Map * from somewhere.
  Map *m = in->m;

  if (in->isaline)
    {
      //shut up compiler, otherwise it's only used when TWOSIDED
      diffheights = false;

      li = in->line;

      if (li->special)
	m->ActivateLine(li, shootthing, 0, SPAC_IMPACT);
      // m->ShootSpecialLine(shootthing, li); // your documentation no longer confuses me, old version

      if (!(li->flags & ML_TWOSIDED))
	goto hitline;

      // crosses a two sided line
      //added:16-02-98: Fab comments : sets opentop, openbottom, openrange
      //                lowfloor is the height of the lowest floor
      //                         (be it front or back)
      tmthing = NULL;
      P_LineOpening (li);

      dist = FixedMul (attackrange, in->frac);

      // hit lower texture ?
      if (li->frontsector->floorheight != li->backsector->floorheight)
        {
	  //added:18-02-98: comments :
	  // find the slope aiming on the border between the two floors
	  slope = FixedDiv (openbottom - shootz , dist);
	  if (slope > aimslope)
	    goto hitline;
        }

      // hit upper texture ?
      if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
	  //added:18-02-98: remember : diff ceil heights
	  diffheights = true;

	  slope = FixedDiv (opentop - shootz , dist);
	  if (slope < aimslope)
	    goto hitline;
        }

      if (li->frontsector->ffloors || li->backsector->ffloors)
        {
          int  frontflag;

          frontflag = P_PointOnLineSide(shootthing->x, shootthing->y, li);

          //SoM: Check 3D FLOORS!
          if (li->frontsector->ffloors)
	    {
	      ffloor_t *rover = li->frontsector->ffloors;
	      fixed_t    highslope, lowslope;

	      for(; rover; rover = rover->next)
		{
		  if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

		  highslope = FixedDiv (*rover->topheight - shootz, dist);
		  lowslope = FixedDiv (*rover->bottomheight - shootz, dist);
		  if ((aimslope >= lowslope && aimslope <= highslope))
		    goto hitline;

		  if (lastz > *rover->topheight && dir == -1 && aimslope < highslope)
		    frontflag |= 0x2;

		  if (lastz < *rover->bottomheight && dir == 1 && aimslope > lowslope)
		    frontflag |= 0x2;
		}
	    }

          if (li->backsector->ffloors)
	    {
	      ffloor_t *rover = li->backsector->ffloors;
	      fixed_t    highslope, lowslope;

	      for(; rover; rover = rover->next)
		{
		  if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS)) continue;

		  highslope = FixedDiv (*rover->topheight - shootz, dist);
		  lowslope = FixedDiv (*rover->bottomheight - shootz, dist);
		  if ((aimslope >= lowslope && aimslope <= highslope))
		    goto hitline;

		  if (lastz > *rover->topheight && dir == -1 && aimslope < highslope)
		    frontflag |= 0x4;

		  if (lastz < *rover->bottomheight && dir == 1 && aimslope > lowslope)
		    frontflag |= 0x4;
		}
	    }
          if ((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            goto hitline;
        }
      lastz = FixedMul (aimslope, dist) + shootz;

      // shot continues
      return true;


      // hit line
    hitline:

      // position a bit closer
      frac = in->frac - FixedDiv (4*FRACUNIT,attackrange);
      dist = FixedMul (frac, attackrange);    //dist to hit on line

      distz = FixedMul (aimslope, dist);      //z add between gun z and hit z
      z = shootz + distz;                     // hit z on wall

      //added:17-02-98: clip shots on floor and ceiling
      //                use a simple triangle stuff a/b = c/d ...
      // BP:13-3-99: fix the side usage
      hitplane = false;
      sectorside=P_PointOnLineSide(shootthing->x,shootthing->y,li);
      if (li->sidenum[sectorside] != -1) // can happen in nocliping mode
        {
	  sector = m->sides[li->sidenum[sectorside]].sector;
	  floorz = sector->floorheight;
	  ceilingz = sector->ceilingheight;
	  if (sector->ffloors)
            {
              ffloor_t *rover;
              for(rover = sector->ffloors; rover; rover = rover->next)
		{
		  if (!(rover->flags & FF_SOLID)) continue;

		  if (dir == 1 && *rover->bottomheight < ceilingz && *rover->bottomheight > lastz)
		    ceilingz = *rover->bottomheight;
		  if (dir == -1 && *rover->topheight > floorz && *rover->topheight < lastz)
		    floorz = *rover->topheight;
		}
            }

	  if ((z > ceilingz) && distz)
            {
	      clipz = ceilingz - shootz;
	      frac = FixedDiv(FixedMul(frac,clipz), distz);
	      hitplane = true;
            }
	  else
	    if ((z < floorz) && distz)
	      {
		clipz = shootz - floorz;
		frac = -FixedDiv(FixedMul(frac,clipz), distz);
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
      //SPLAT TEST ----------------------------------------------------------
#ifdef WALLSPLATS
      if (!hitplane)
        {
	  divline_t   divl;
	  fixed_t     frac;

	  P_MakeDivline (li, &divl);
	  frac = P_InterceptVector (&divl, &trace);
	  R.R_AddWallSplat (li, sectorside, "A_DMG1", z, frac, SPLATDRAWMODE_SHADE);
        }
#endif
      // --------------------------------------------------------- SPLAT TEST


      x = trace.x + FixedMul (trace.dx, frac);
      y = trace.y + FixedMul (trace.dy, frac);

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
          if (dir == 1 && z + (16 << FRACBITS) > ceilingz)
            z = ceilingz - (16 << FRACBITS);
          if (dir == -1 && z < floorz)
            z = floorz;
        }
      // Spawn bullet puffs.
      m->SpawnPuff (x,y,z);

      // don't go any farther
      return false;
    }

  // shoot a thing
  Actor *th = in->thing;
  if (th == shootthing)
    return true;            // can't shoot self

  if (!(th->flags & MF_SHOOTABLE))
    return true;            // corpse or something

  // check for physical attacks on a ghost
  // FIXME this is stupid. We should use some sort of damage type system.
  // (if damagetype != dt_ethereal ...)
  /*
  if (game.mode == gm_heretic && (th->flags & MF_SHADOW))
    if (shootthing->Type() == Thinker::tt_ppawn)
      if (((PlayerPawn *)shootthing)->player->readyweapon == wp_staff)
	return true;
  */

  // check angles to see if the thing can be aimed at
  dist = FixedMul (attackrange, in->frac);
  thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

  if (thingtopslope < aimslope)
    return true;            // shot over the thing

  thingbottomslope = FixedDiv (th->z - shootz, dist);

  if (thingbottomslope > aimslope)
    return true;            // shot under the thing

  // SoM: SO THIS IS THE PROBLEM!!!
  // heh.
  // A bullet would travel through a 3D floor until it hit a LINEDEF! Thus
  // it appears that the bullet hits the 3D floor but it actually just hits
  // the line behind it. Thus allowing a bullet to hit things under a 3D
  // floor and still be clipped a 3D floor.
  if (th->subsector->sector->ffloors)
    {
      sector_t *sector = th->subsector->sector;
      ffloor_t *rover;

      for(rover = sector->ffloors; rover; rover = rover->next)
	{
	  if (!(rover->flags & FF_SOLID))
	    continue;

	  if (dir == -1 && *rover->topheight < lastz && *rover->topheight > th->z + th->height)
	    return true;
	  if (dir == 1 && *rover->bottomheight > lastz && *rover->bottomheight < th->z)
	    return true;
	}
    }

  // hit thing
  // position a bit closer
  frac = in->frac - FixedDiv (10*FRACUNIT,attackrange);

  x = trace.x + FixedMul (trace.dx, frac);
  y = trace.y + FixedMul (trace.dy, frac);
  z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

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

  if (in->thing->flags & MF_NOBLOOD)
    m->SpawnPuff (x,y,z);
  else
    {
      if (game.mode == gm_heretic || game.mode == gm_hexen)
	m->SpawnPuff(x, y, z);

      if (hitplane)
	{
	  m->SpawnBloodSplats (x,y,z, la_damage, trace.dx, trace.dy);
	  return false;
	}
    }

  return false;
}


// Actor tries to find a target
fixed_t Actor::AimLineAttack(angle_t ang, fixed_t distance)
{
  fixed_t     x2;
  fixed_t     y2;
  extern fixed_t bottomslope, topslope;

  ang >>= ANGLETOFINESHIFT;
  shootthing = this;

  // FIXME since monsters shouldn't change their "aiming" angle,
  // why not use the same routine for them also?
  // if ((Type() == Thinker::tt_ppawn) && game.demoversion>=128)
    {
      fixed_t cosineaiming = finecosine[aiming>>ANGLETOFINESHIFT];
      int aim = ((int)aiming)>>ANGLETOFINESHIFT;
      x2 = x + FixedMul(FixedMul(distance,finecosine[ang]),cosineaiming);
      y2 = y + FixedMul(FixedMul(distance,finesine[ang]),cosineaiming); 

      topslope    =  100*FRACUNIT/160+finetangent[(2048+aim) & FINEMASK];
      bottomslope = -100*FRACUNIT/160+finetangent[(2048+aim) & FINEMASK];
    }
    /*
  else
    {
      x2 = x + (distance>>FRACBITS)*finecosine[ang];
      y2 = y + (distance>>FRACBITS)*finesine[ang];

      //added:15-02-98: Fab comments...
      // Doom's base engine says that at a distance of 160,
      // the 2d graphics on the plane x,y correspond 1/1 with plane units
      topslope = 100*FRACUNIT/160;
      bottomslope = -100*FRACUNIT/160;
    }
    */
  shootz = lastz = z + (height>>1) + 8*FRACUNIT;

  // can't shoot outside view angles


  attackrange = distance;
  linetarget = NULL;

  //added:15-02-98: comments
  // traverse all linedefs and mobjs from the blockmap containing t1,
  // to the blockmap containing the dest. point.
  // Call the function for each mobj/line on the way,
  // starting with the mobj/linedef at the shortest distance...
  mp->PathTraverse (x, y, x2, y2,
		  PT_ADDLINES|PT_ADDTHINGS,
		  PTR_AimTraverse);

  //added:15-02-98: linetarget is only for mobjs, not for linedefs
  if (linetarget)
    return aimslope;

  return 0;
}


// If damage == 0, it is just a test trace
// that will leave linetarget set.
//
// Shoots an insta-hit projectile from Actor to the direction determined by (yaw, pitch)
// 'distance' is the max distance for the projectile
// 'damage' is obvious, dtype is the damage type.
// global variable 'linetarget' will point to the Actor who got hit.

void Actor::LineAttack(angle_t yaw, fixed_t distance, fixed_t pitch, int damage, int dtype)
{
  fixed_t     x2;
  fixed_t     y2;

  yaw >>= ANGLETOFINESHIFT;
  shootthing = this;
  la_damage = damage;
  la_dtype = dtype;
  linetarget = NULL;

  // FIXME see AimLineAttack, same here
  // player autoaimed attack, 
  /*
  if (!player)
    {   
      x2 = x + (distance>>FRACBITS)*finecosine[yaw]; 
      y2 = y + (distance>>FRACBITS)*finesine[yaw];   
    }
  else
  */
    {
      fixed_t cosangle = finecosine[aiming>>ANGLETOFINESHIFT];

      x2 = x + FixedMul(FixedMul(distance,finecosine[yaw]),cosangle);
      y2 = y + FixedMul(FixedMul(distance,finesine[yaw]),cosangle); 
    }

  shootz = lastz = z + (height>>1) + 8*FRACUNIT - floorclip;

  attackrange = distance;
  aimslope = pitch;

  tmthing = shootthing;

  mp->PathTraverse(x, y, x2, y2,
		   PT_ADDLINES|PT_ADDTHINGS,
		   PTR_ShootTraverse);
}


//==========================================================================
//  Using linedefs
//==========================================================================

static PlayerPawn *usething;

static bool PTR_UseTraverse(intercept_t *in)
{
  tmthing = NULL; // FIXME why? affects P_LineOpening
  line_t *line = in->line;
  CONS_Printf("Line: s = %d, tag = %d\n", line->special, line->tag);
  if (!line->special)
    {
      P_LineOpening(line);
      // TEST: use through a hole only if you can reach it
      if (openrange <= 0 || opentop < usething->z || openbottom > usething->z + usething->height)
        {
	  S_StartSound(usething, sfx_usefail);

	  // can't use through a wall
	  return false;
        }

      // not a special line, but keep checking
      return true ;
    }

  // can't use backsides of lines
  int side = P_PointOnLineSide(usething->x, usething->y, line);
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

  int ang = angle >> ANGLETOFINESHIFT;

  x1 = x;
  y1 = y;
  x2 = x1 + (USERANGE>>FRACBITS)*finecosine[ang];
  y2 = y1 + (USERANGE>>FRACBITS)*finesine[ang];

  mp->PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse);
}


//==========================================================================
//  Puzzle item usage
//==========================================================================

static PlayerPawn *PuzzleItemUser;
static int  PuzzleItemType;
static bool PuzzleActivated;

static bool PTR_PuzzleItemTraverse(intercept_t *in)
{
  const int USE_PUZZLE_ITEM_SPECIAL = 129;

  if (in->isaline)
    { // Check line
      line_t *line = in->line;
      if (line->special != USE_PUZZLE_ITEM_SPECIAL)
	{
	  P_LineOpening(line);
	  if (openrange <= 0)
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

      if (P_PointOnLineSide(PuzzleItemUser->x, PuzzleItemUser->y, line) == 1)
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

  int angle = angle >> ANGLETOFINESHIFT;
  x1 = x;
  y1 = y;
  x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
  y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];
  mp->PathTraverse(x1, y1, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_PuzzleItemTraverse);

  return PuzzleActivated;
}


//==========================================================================
// RADIUS ATTACK
//==========================================================================

Actor *bombowner;
Actor *bomb;
int    bombdamage;
int    bombdistance;
int    bombdtype;
bool   bomb_damage_owner;

// "bombowner" is the creature
// that caused the explosion at "bomb".
static bool PIT_RadiusAttack(Actor *thing)
{
  fixed_t  dx, dy, dz;
  fixed_t  dist;

  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  if (!bomb_damage_owner && thing == bombowner)
    return true;

  // Bosses take no damage from concussion.
  // if (thing->flags2 & MF2_BOSS) return true;

  dx = abs(thing->x - bomb->x);
  dy = abs(thing->y - bomb->y);

  dist = dx>dy ? dx : dy;
  dist -= thing->radius;

  //added:22-02-98: now checks also z dist for rockets exploding
  //                above yer head...
  dz = abs(thing->z+(thing->height>>1) - bomb->z);
  int d = (dist > dz ? dist : dz) >> FRACBITS;

  if (d < 0)
    d = 0;

  if (d >= bombdistance)
    return true;    // out of range

  // geometry blocks the blast?
  if (thing->floorz > bomb->z && bomb->ceilingz < thing->z)
    return true;
  if (thing->ceilingz < bomb->z && bomb->floorz > thing->z)
    return true;

  if (thing->mp->CheckSight(thing, bomb))
    {
      int damage = (bombdamage * (bombdistance - d)/bombdistance) + 1;

      // Hexen: if(thing->player) damage >>= 2;

      int apx = 0, apy = 0;
      if (dist)
        {
	  apx = (thing->x - bomb->x)/d;
	  apy = (thing->y - bomb->y)/d;
        }
      // must be in direct path
      if (thing->Damage(bomb, bombowner, damage, bombdtype) && !(thing->flags & MF_NOBLOOD))
	thing->mp->SpawnBloodSplats(thing->x,thing->y,thing->z, damage, apx, apy);
    }

  return true;
}


// Culprit is the creature that caused the explosion.
void Actor::RadiusAttack(Actor *culprit, int damage, int distance, int dtype, bool downer)
{
  int nx, ny;

  int         xl;
  int         xh;
  int         yl;
  int         yh;

  if (distance < 0)
    bombdistance = damage;
  else
    bombdistance = distance;

  fixed_t dist = (bombdistance + MAXRADIUS) << FRACBITS;
  yh = (y + dist - mp->bmaporgy)>>MAPBLOCKSHIFT;
  yl = (y - dist - mp->bmaporgy)>>MAPBLOCKSHIFT;
  xh = (x + dist - mp->bmaporgx)>>MAPBLOCKSHIFT;
  xl = (x - dist - mp->bmaporgx)>>MAPBLOCKSHIFT;

  bomb = this;
  bombowner = culprit;
  bombdamage = damage;
  bombdtype = dtype;
  bomb_damage_owner = downer;

  for (ny=yl ; ny<=yh ; ny++)
    for (nx=xl ; nx<=xh ; nx++)
      mp->BlockThingsIterator (nx, ny, PIT_RadiusAttack);
}



//==========================================================================
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//==========================================================================
int         crushdamage;
bool        nofit;
sector_t   *sectorchecked;

// deals crush damage
static bool PIT_ChangeSector(Actor *thing)
{
  if (P_ThingHeightClip (thing))
    {
      // keep checking
      return true;
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

  if (crushdamage && !(thing->mp->maptic % (4*NEWTICRATERATIO)))
    {
      thing->Damage(NULL, NULL, crushdamage, dt_crushing); // was 10

      if ((!(thing->mp->maptic % (16*NEWTICRATERATIO)) && !(thing->flags&MF_NOBLOOD)))
        {
	  // spray blood in a random direction
	  DActor *mo = thing->mp->SpawnDActor(thing->x, thing->y,
	    thing->z + thing->height/2, MT_BLOOD);
            
	  mo->px  = P_SignedRandom()<<12;
	  mo->py  = P_SignedRandom()<<12;
        }
    }

  // keep checking (crush other things)
  return true;
}


// changes sector height, crushes things
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


//SoM: 3/15/2000: New function. Much faster.
bool Map::CheckSector(sector_t *sector, int crunch)
{
  msecnode_t      *n;

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

  if (sector->numattached)
    {
      int            i;
      sector_t *sec;
      for(i = 0; i < sector->numattached; i ++)
	{
	  sec = &sectors[sector->attached[i]];
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

// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.
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

  sector_list = P_AddSecnode(ld->frontsector,tmthing,sector_list);

  // Don't assume all lines are 2-sided, since some Things
  // like MT_TFOG are allowed regardless of whether their radius takes
  // them beyond an impassable linedef.

  // Use sidedefs instead of 2s flag to determine two-sidedness.

  if (ld->backsector)
    sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

  return true;
}


// alters/creates the sector_list that shows what sectors
// the object resides in.
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
  tmflags = thing->flags;

  tmb.Set(x, y, tmthing->radius);

  validcount++; // used to make sure we only process a line once

  xl = (tmb[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tmb[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tmb[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tmb[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      BlockLinesIterator(bx,by,PIT_GetSectors);

  // Add the sector of the (x,y) point to sector_list.

  sector_list = P_AddSecnode(thing->subsector->sector,thing,sector_list);

  // Now delete any nodes that won't be used. These are the ones where
  // m_thing is still NULL.

  node = sector_list;
  while (node)
    {
      if (node->m_thing == NULL)
	{
	  if (node == sector_list)
	    sector_list = node->m_tnext;
	  node = P_DelSecnode(node);
	}
      else
	node = node->m_tnext;
    }

  thing->touching_sectorlist = sector_list;
  sector_list = NULL;
}



//=============================================================================
//  Z movement
//=============================================================================

Actor *onmobj; //generic global onmobj...used for landing on pods/players

static bool PIT_CheckOnmobjZ(Actor *thing)
{
  if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))    
    return true; // Can't hit thing
    
  fixed_t blockdist = thing->radius + tmthing->radius;
  if (abs(thing->x-tmx) >= blockdist || abs(thing->y-tmy) >= blockdist)    
    return true; // Didn't hit thing

  if (thing == tmthing)
    return true; // Don't clip against self

  if (tmthing->z > thing->z + thing->height)
    return true; // over
  else if (tmthing->z + tmthing->height < thing->z)    
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
  fixed_t nz = z + pz;
  if ((flags & MF_FLOAT) && target)
    {       // float down towards target if too close
      if (!(eflags & (MFE_SKULLFLY | MFE_INFLOAT)))
	{
	  int dist = P_AproxDistance(x - target->x, y - target->y);
	  int delta = target->z + (height>>1) - nz;
	  if (delta < 0 && dist < -(delta*3))
	    nz -= FLOATSPEED;
	  else if (delta > 0 && dist < (delta*3))
	    nz += FLOATSPEED;
	}
    }
  if ((eflags & MFE_FLY) && !(nz <= floorz) && mp->maptic & 2)
    {
      nz += finesine[(FINEANGLES/20*mp->maptic>>2) & FINEMASK];
    }

  // FIXME it would be better if we didn't need to actually change the Actor
  // this is a fake movement after all...
  int npz = pz;
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
	  z = nz;
	  pz = npz;
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
  z = nz;
  pz = npz;
}


// Checks if the new Z position is legal
Actor *Actor::CheckOnmobj()
{
  int          xl,xh,yl,yh,bx,by;
  subsector_t *newsubsec;
    
  tmthing = this;
  tmflags = flags;
  fixed_t oldz = z;
  fixed_t oldpz = pz;
  FakeZMovement();
  
  tmb.Set(x, y, tmthing->radius);

  newsubsec = mp->R_PointInSubsector (x, y);
  ceilingline = NULL;
    
  //
  // the base floor / ceiling is from the subsector that contains the
  // point.  Any contacted lines the step closer together will adjust them
  //
  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;
  tmfloorpic = newsubsec->sector->floorpic; 
   
  validcount++;
  spechit.clear();
    
  if (tmflags & MF_NOCLIPTHING)
    return NULL;
    
  //
  // check things first, possibly picking things up
  // the bounding box is extended by MAXRADIUS because Actors are grouped
  // into mapblocks based on their origin point, and can overlap into adjacent
  // blocks by up to MAXRADIUS units
  //
  xl = (tmb[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmb[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmb[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmb[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
    
  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockThingsIterator(bx,by,PIT_CheckOnmobjZ))
	{
	  z = oldz;
	  pz = oldpz;
	  return onmobj;
	}

  z = oldz;
  pz = oldpz;
  return NULL;
}


// =========================================================================
//                         MOVEMENT CLIPPING
// =========================================================================

//
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//  a Actor (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the Actor->x,y)
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
bool Actor::CheckPosition(fixed_t nx, fixed_t ny)
{
  tmthing = this;
  tmflags = flags;

  tmb.Set(nx, ny, radius);

  subsector_t *newsubsec = mp->R_PointInSubsector(nx,ny);
  ceilingline = blockingline = NULL;

  // The base floor / ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.
  tmfloorz = tmsectorfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = tmsectorceilingz = newsubsec->sector->ceilingheight;
  tmfloorpic = newsubsec->sector->floorpic;

  //SoM: 3/23/2000: Check list of fake floors and see if
  //tmfloorz/tmceilingz need to be altered.
  if (newsubsec->sector->ffloors)
    {
      ffloor_t *rover;
      int        thingtop = z + height;

      for(rover = newsubsec->sector->ffloors; rover; rover = rover->next)
	{
	  if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
	    continue;
	  fixed_t delta1 = z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
	  fixed_t delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
	  if (*rover->topheight > tmfloorz && abs(delta1) < abs(delta2))
	    tmfloorz = tmdropoffz = *rover->topheight;
	  if (*rover->bottomheight < tmceilingz && abs(delta1) >= abs(delta2))
	    tmceilingz = *rover->bottomheight;
	}
    }

  // tmfloorthing is set when tmfloorz comes from a thing's top
  tmfloorthing = NULL; // FIXME for this to work, the lines should be checked first, then things...

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
      xl = (tmb[BOXLEFT] - bmox - MAXRADIUS)>>MAPBLOCKSHIFT;
      xh = (tmb[BOXRIGHT] - bmox + MAXRADIUS)>>MAPBLOCKSHIFT;
      yl = (tmb[BOXBOTTOM] - bmoy - MAXRADIUS)>>MAPBLOCKSHIFT;
      yh = (tmb[BOXTOP] - bmoy + MAXRADIUS)>>MAPBLOCKSHIFT;

      BlockingMobj = NULL;        
      for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	  if (!mp->BlockThingsIterator(bx,by,PIT_CheckThing))
	    return false;
    }

  if (!(flags & MF_NOCLIPLINE))
    {
      // check lines
      BlockingMobj = NULL;

      xl = (tmb[BOXLEFT] - bmox)>>MAPBLOCKSHIFT;
      xh = (tmb[BOXRIGHT] - bmox)>>MAPBLOCKSHIFT;
      yl = (tmb[BOXBOTTOM] - bmoy)>>MAPBLOCKSHIFT;
      yh = (tmb[BOXTOP] - bmoy)>>MAPBLOCKSHIFT;
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
  int temp = flags;

  flags &= ~MF_PICKUP;
  if (CheckPosition(x, y))
    { // XY is ok, now check Z
      flags = temp;
      if ((z < floorz) || (z+height > ceilingz))
	return false; // bad z
      return true;
    }
  flags = temp;
  return false;
}
