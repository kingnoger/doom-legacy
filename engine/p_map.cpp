// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.1  2002/11/16 14:18:01  hurdler
// Initial revision
//
// Revision 1.17  2002/09/20 22:41:31  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.16  2002/08/21 16:58:33  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.15  2002/08/20 13:56:58  vberghol
// sdfgsd
//
// Revision 1.14  2002/08/19 18:30:13  vberghol
// just netcode to go!
//
// Revision 1.13  2002/08/14 17:07:18  vberghol
// p_map.cpp done... 3 to go
//
// Revision 1.12  2002/08/13 19:47:42  vberghol
// p_inter.cpp done
//
// Revision 1.11  2002/08/11 17:16:49  vberghol
// ...
//
// Revision 1.10  2002/08/08 12:01:27  vberghol
// pian engine on valmis!
//
// Revision 1.9  2002/08/06 13:14:23  vberghol
// ...
//
// Revision 1.8  2002/07/26 19:23:05  vberghol
// a little something
//
// Revision 1.7  2002/07/23 19:21:42  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.6  2002/07/13 17:55:54  vberghol
// jäi kartan liikkuviin osiin... p_doors.cpp
//
// Revision 1.5  2002/07/12 19:21:38  vberghol
// hop
//
// Revision 1.4  2002/07/10 19:57:00  vberghol
// g_pawn.cpp tehty
//
// Revision 1.3  2002/07/01 21:00:19  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:14  vberghol
// Version 133 Experimental!
//
// Revision 1.26  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.25  2001/08/07 00:53:33  hurdler
// lil' change
//
// Revision 1.24  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.20  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.19  2001/04/01 17:35:06  bpereira
// no message
//
// Revision 1.18  2001/03/30 17:12:50  bpereira
// no message
//
// Revision 1.17  2001/03/19 18:52:01  hurdler
// lil fix
//
// Revision 1.16  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.15  2001/03/09 21:53:56  metzgermeister
// *** empty log message ***
//
// Revision 1.14  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.12  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.11  2000/10/21 08:43:30  bpereira
// no message
//
// Revision 1.5  2000/04/15 22:12:57  stroggonmeth
// Minor bug fixes
//
// Revision 1.4  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Movement, collision handling.
//      Shooting and aiming.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "command.h"
#include "p_maputl.h"
#include "m_bbox.h"
#include "m_random.h"

#include "p_enemy.h"

#include "r_render.h"
#include "r_main.h"
#include "r_sky.h"
#include "s_sound.h"
#include "sounds.h"

#include "r_splats.h"   //faB: testing

#include "z_zone.h" //SoM: 3/15/2000


fixed_t   tmbbox[4];
Actor    *tmthing;
int       tmflags;
fixed_t   tmx;
fixed_t   tmy;


// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
bool      floatok;

fixed_t   tmfloorz;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
line_t **spechit;                //SoM: 3/15/2000: Limit removal
int      numspechit;



fixed_t  tmceilingz;
fixed_t  tmdropoffz;

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


//SoM: 3/15/2000
msecnode_t *sector_list = NULL;

//SoM: 3/15/2000
static int pe_x; // Pain Elemental position for Lost Soul checks
static int pe_y; // Pain Elemental position for Lost Soul checks
static int ls_x; // Lost Soul position for Lost Soul checks
static int ls_y; // Lost Soul position for Lost Soul checks



//
// TELEPORT MOVE
//

//
// PIT_StompThing
//
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
  if (game.mode != heretic && tmthing->Type() != Thinker::tt_ppawn && thing->mp->braintargets.size())
    return false;

  // Not allowed to stomp things
  if (game.mode == heretic && !(tmthing->flags2 & MF2_TELESTOMP))
    return false;

  thing->Damage(tmthing, tmthing, 10000);

  return true;
}


// experiment

void P_SetBox(fixed_t x, fixed_t y, fixed_t r)
{
  tmx = x;
  tmy = y;

  tmbbox[BOXTOP]    = y + r;
  tmbbox[BOXBOTTOM] = y - r;
  tmbbox[BOXRIGHT]  = x + r;
  tmbbox[BOXLEFT]   = x - r;
}


//
// was P_TeleportMove
//
bool Actor::TeleportMove(fixed_t nx, fixed_t ny)
{
  int  xl, xh, yl, yh, bx, by;

  // kill anything occupying the position
  tmthing = this;
  tmflags = flags;

  P_SetBox(nx, ny, radius);

  subsector_t *newsubsec = mp->R_PointInSubsector(nx,ny);
  ceilingline = NULL;

  // The base floor/ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.
  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;

  validcount++;
  numspechit = 0;

  // stomp on any things contacted
  xl = (tmbbox[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockThingsIterator(bx,by,PIT_StompThing))
	return false;

  // the move is ok,
  // so link the thing into its new position
  UnsetPosition();
  //floorz = tmfloorz;
  //ceilingz = tmceilingz;
  x = nx;
  y = ny;
  SetPosition();

  return true;
}


// =========================================================================
//                       MOVEMENT ITERATOR FUNCTIONS
// =========================================================================



static void add_spechit(line_t *ld)
{
  static int spechit_max = 0;

  //SoM: 3/15/2000: Boom limit removal.
  if (numspechit >= spechit_max)
    {
      spechit_max = spechit_max ? spechit_max*2 : 16;
      spechit = (line_t **)realloc(spechit, sizeof(line_t *) * spechit_max);
    }
  
  //int linenum = ld - lines;
  spechit[numspechit] = ld;
  numspechit++;
}


//
// PIT_CheckThing
//
static bool PIT_CheckThing (Actor *thing)
{
  fixed_t             blockdist;
  bool             solid;
  int                 damage;

  //added:22-02-98:
  fixed_t             topz;
  fixed_t             tmtopz;

  //SoM: 3/15/2000: Moved to front.

  // don't clip against self

  if (thing == tmthing)
    return true;

  if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
    return true;

#ifdef CLIENTPREDICTION2
  // mobj and spirit of a same player cannot colide
  if (thing->player && (thing->player->spirit == tmthing || thing->player->mo == tmthing))
    return true;
#endif

  blockdist = thing->radius + tmthing->radius;

  if (abs(thing->x - tmx) >= blockdist ||
       abs(thing->y - tmy) >= blockdist)
    {
      // didn't hit it
      return true;
    }

  // heretic stuffs
  if (tmthing->flags2&MF2_PASSMOBJ)
    { // check if a mobj passed over/under another object
      if ((tmthing->type == MT_IMP || tmthing->type == MT_WIZARD)
	 && (thing->type == MT_IMP || thing->type == MT_WIZARD))
        { // don't let imps/wizards fly over other imps/wizards
	  return false;
        }
      if (tmthing->z >= thing->z+thing->height
	 && !(thing->flags&MF_SPECIAL))
        {
	  return true;
        }
      else if (tmthing->z+tmthing->height < thing->z
	      && !(thing->flags&MF_SPECIAL))
        { // under thing
	  return true;
        }
    }

  // check for skulls slamming into things
  if (tmflags & MF_SKULLFLY)
    {
      damage = ((P_Random()%8)+1)*tmthing->info->damage;

      thing->Damage(tmthing, tmthing, damage);

      tmthing->flags &= ~MF_SKULLFLY;
      tmthing->px = tmthing->py = tmthing->pz = 0;

      tmthing->SetState(game.mode == heretic ? tmthing->info->seestate
			: tmthing->info->spawnstate);

      return false;           // stop moving
    }


  // missiles can hit other things
  if (tmthing->flags & MF_MISSILE)
    {
      // Check for passing through a ghost (heretic)
      if ((thing->flags & MF_SHADOW) && (tmthing->flags2 & MF2_THRUGHOST))
	return true;

      // see if it went over / under
      if (tmthing->z > thing->z + thing->height)
	return true;                // overhead
      if (tmthing->z+tmthing->height < thing->z)
	return true;                // underneath

      if (tmthing->target && (
			      tmthing->target->type == thing->type ||
			      (tmthing->target->type == MT_KNIGHT  && thing->type == MT_BRUISER)||
			      (tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT)))
        {
	  // Don't hit same species as originator.
	  if (thing == tmthing->target)
	    return true;

	  if (thing->type != MT_PLAYER)
            {
	      // Explode, but do no damage.
	      // Let players missile other players.
	      return false;
            }
        }

      if (! (thing->flags & MF_SHOOTABLE))
        {
	  // didn't do any damage
	  return !(thing->flags & MF_SOLID);
        }

      // more heretic stuff
      if (tmthing->flags2 & MF2_RIP)
        {
	  damage = ((P_Random () & 3) + 2) * tmthing->info->damage;
	  S_StartSound (tmthing, sfx_ripslop);
	  if (thing->Damage(tmthing, tmthing->target, damage))
            {
	      if (!(thing->flags & MF_NOBLOOD))
                {   // Ok to spawn some blood
		  tmthing->mp->SpawnBlood(tmthing->x, tmthing->y, tmthing->z, damage);
		  //P_RipperBlood (tmthing);
                }
            }
	  if (thing->flags2 & MF2_PUSHABLE
	      && !(tmthing->flags2 & MF2_CANNOTPUSH))
            {             // Push thing
	      thing->px += tmthing->px >> 2;
	      thing->py += tmthing->py >> 2;
            }
	  numspechit = 0;
	  return  true;
        }

      // damage / explode
      damage = ((P_Random()%8)+1)*tmthing->info->damage;
      if (thing->Damage(tmthing, tmthing->target, damage) && 
	  (thing->flags & MF_NOBLOOD)==0 && game.demoversion>=129)
	tmthing->mp->SpawnBloodSplats(tmthing->x,tmthing->y,tmthing->z, damage, thing->px, thing->py);

      // don't traverse any more
      return false;
    }
  if (thing->flags2 & MF2_PUSHABLE && !(tmthing->flags2 & MF2_CANNOTPUSH))
    {                         // Push thing
      thing->px += tmthing->px >> 2;
      thing->py += tmthing->py >> 2;
    }

  // check for special pickup
  if (thing->flags & MF_SPECIAL)
    {
      solid = thing->flags & MF_SOLID;
      // FIXME! this is not good
      //if (tmflags & MF_PICKUP)
      if (tmthing->Type() == Thinker::tt_ppawn)
        {
	  // can remove thing
	  ((PlayerPawn *)tmthing)->TouchSpecialThing(thing);
        }
      return !solid;
    }
  // check again for special pickup
  if (game.demoversion>=132 && tmthing->flags & MF_SPECIAL)
    {
      solid = tmthing->flags & MF_SOLID;
      //if (thing->flags & MF_PICKUP)
      if (thing->Type() == Thinker::tt_ppawn)
        {
	  // can remove thing
	  ((PlayerPawn *)thing)->TouchSpecialThing(tmthing);
        }
      return !solid;
    }


  //added:24-02-98:compatibility with old demos, it used to return with...
  //added:27-02-98:for version 112+, nonsolid things pass through other things
  if (game.demoversion<112 || game.demoversion>=132 || !(tmthing->flags & MF_SOLID))
    return !(thing->flags & MF_SOLID);

  //added:22-02-98: added z checking at last
  //SoM: 3/10/2000: Treat noclip things as non-solid!
  if ((thing->flags & MF_SOLID) && (tmthing->flags & MF_SOLID) &&
      !(thing->flags & MF_NOCLIP) && !(tmthing->flags & MF_NOCLIP))
    {
      // pass under
      tmtopz = tmthing->z + tmthing->height;

      if (tmtopz < thing->z)
        {
	  if (thing->z < tmceilingz)
	    tmceilingz = thing->z;
	  return true;
        }

      topz = thing->z + thing->height + FRACUNIT;

      // block only when jumping not high enough,
      // (dont climb max. 24units while already in air)
      // if not in air, let P_TryMove() decide if its not too high
      // FIXME why test player here
      if (tmthing->Type() == Thinker::tt_ppawn &&
	  tmthing->z < topz &&
	  tmthing->z > tmthing->floorz)  // block while in air
	return false;


      if (topz > tmfloorz)
        {
	  tmfloorz = topz;
	  tmfloorthing = thing;       //thing we may stand on
        }

    }

  // not solid not blocked
  return true;
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
static bool PIT_CrossLine (line_t *ld)
{
  if (!(ld->flags & ML_TWOSIDED) ||
      (ld->flags & (ML_BLOCKING|ML_BLOCKMONSTERS)))
    if (!(tmbbox[BOXLEFT]   > ld->bbox[BOXRIGHT]  ||
          tmbbox[BOXRIGHT]  < ld->bbox[BOXLEFT]   ||
          tmbbox[BOXTOP]    < ld->bbox[BOXBOTTOM] ||
          tmbbox[BOXBOTTOM] > ld->bbox[BOXTOP]))
      if (P_PointOnLineSide(pe_x,pe_y,ld) != P_PointOnLineSide(ls_x,ls_y,ld))
        return false;  // line blocks trajectory
  return true; // line doesn't block trajectory
}



//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//
static bool PIT_CheckLine(line_t *ld)
{
  if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
      || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
      || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
      || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
    return true;

  if (P_BoxOnLineSide (tmbbox, ld) != -1)
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
  if (!ld->backsector)
    {
      if (game.demoversion>=132 && tmthing->flags & MF_MISSILE && ld->special)
        add_spechit(ld);

      return false;           // one sided line
    }

  // missil and Camera can cross uncrossable line
  if (!(tmthing->flags & MF_MISSILE) &&
      !(tmthing->type == MT_CHASECAM))
    {
      if (ld->flags & ML_BLOCKING)
	return false;       // explicitly blocking everything

      if (!(tmthing->flags & MF_NOTMONSTER) &&
	  ld->flags & ML_BLOCKMONSTERS)
	return false;       // block monsters only
    }

  // set openrange, opentop, openbottom
  P_LineOpening (ld);

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
    add_spechit(ld);

  return true;
}




//==========================================================================
//
// CheckMissileImpact
//
//==========================================================================

void Actor::CheckMissileImpact()
{
  int i;
    
  if (!(flags & MF_MISSILE) || game.demoversion<132 || !numspechit || !target)
    return;

  //if (!target->player)
  // monsters don't shoot triggers
  //  if (!(owner->flags & MF_NOTMONSTER))
  if (owner->Type() != Thinker::tt_ppawn)
    return;

  for(i = numspechit-1; i >= 0; i--)
    mp->ShootSpecialLine(target, spechit[i]);
}

//
// was P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
bool Actor::TryMove(fixed_t nx, fixed_t ny, bool allowdropoff)
{
  fixed_t oldx, oldy;
  int     side;
  int     oldside;

  floatok = false;

  if (!CheckPosition(nx, ny))
    {
      CheckMissileImpact();
      return false;           // solid wall or thing
    }
#ifdef CLIENTPREDICTION2
  if (!(flags & MF_NOCLIP) && !(eflags & MF_NOZCHECKING))
#else
    if (!(flags & MF_NOCLIP))
#endif
      {
	fixed_t maxstep = MAXSTEPMOVE;
	if (tmceilingz - tmfloorz < height)
	  {
	    CheckMissileImpact();
	    return false;       // doesn't fit
	  }

	floatok = true;

	if (!(flags & MF_TELEPORT)
	    && tmceilingz - z < height
	    && !(flags2 & MF2_FLY))
	  {
	    CheckMissileImpact();
	    return false;       // mobj must lower itself to fit
	  }
	if(flags2 & MF2_FLY)
	  {
	    if (z+height > tmceilingz)
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
        if ((eflags & (MF_UNDERWATER|MF_TOUCHWATER))==(MF_UNDERWATER|MF_TOUCHWATER))
	  maxstep=37*FRACUNIT;

        if (!(flags & MF_TELEPORT) 
             // The Minotaur floor fire (MT_MNTRFX2) can step up any amount
             && type != MT_MNTRFX2
             && (tmfloorz - z > maxstep))
	  {
            CheckMissileImpact();
            return false;       // too big a step up
	  }

        if ((flags&MF_MISSILE) && tmfloorz > z)
	  CheckMissileImpact();

        if (!boomsupport || !allowdropoff)
          if (!(flags&(MF_DROPOFF|MF_FLOAT))
               && !tmfloorthing
               && tmfloorz - tmdropoffz > MAXSTEPMOVE)
	    return false;       // don't stand over a dropoff
      }

  // the move is ok,
  // so link the thing into its new position
  UnsetPosition();

  //added:28-02-98: gameplay hack : walk over a small wall while jumping
  //                stop jumping it succeeded
  // BP: removed in 1.28 because we can move in air now
  /*
    FIXME or remove altogether
  if (game.demoversion >= 112 && game.demoversion < 128 && player && (player->cheats & CF_JUMPOVER))
    {
      if (tmfloorz > floorz + MAXSTEPMOVE)
	pz >>= 2;
    }
  */

  oldx = x;
  oldy = y;
  //floorz = tmfloorz;
  //ceilingz = tmceilingz;
  x = nx;
  y = ny;

  //added:28-02-98:
  if (tmfloorthing)
    eflags &= ~MF_ONGROUND;  //not on real floor
  else
    eflags |= MF_ONGROUND;

  SetPosition();

  if ((flags2 & MF2_FOOTCLIP)
      && (GetThingFloorType() != FLOOR_SOLID) && game.mode == heretic)
    flags2 |= MF2_FEETARECLIPPED;
  else if (flags2 & MF2_FEETARECLIPPED)
    flags2 &= ~MF2_FEETARECLIPPED;

  // if any special lines were hit, do the effect
  if (!(flags&(MF_TELEPORT|MF_NOCLIP)) &&
       (type != MT_CHASECAM) && (type != MT_SPIRIT))
    {
      while (numspechit--)
        {
	  // see if the line was crossed
	  line_t *ld = spechit[numspechit];
	  side = P_PointOnLineSide (x, y, ld);
	  oldside = P_PointOnLineSide (oldx, oldy, ld);
	  if (side != oldside)
            {
	      if (ld->special)
		//P_CrossSpecialLine(ld-lines, oldside, this);
		mp->ActivateCrossedLine(ld, oldside, this);
            }
        }
    }

  return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
bool P_ThingHeightClip (Actor *thing)
{
  bool             onfloor;

  onfloor = (thing->z <= thing->floorz);

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

      //thing->eflags &= ~MF_ONGROUND;
    }

  //debug : be sure it falls to the floor
  thing->eflags &= ~MF_ONGROUND;

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



//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//
fixed_t         bestslidefrac;
fixed_t         secondslidefrac;

line_t *bestslideline;
line_t *secondslideline;

Actor *slidemo;

fixed_t         tmxmove;
fixed_t         tmymove;



//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
void P_HitSlideLine (line_t *ld)
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


//
// PTR_SlideTraverse
//
static bool PTR_SlideTraverse (intercept_t *in)
{
  line_t *li;

#ifdef PARANOIA
  if (!in->isaline)
    I_Error ("PTR_SlideTraverse: not a line?");
#endif

  li = in->d.line;

  if (! (li->flags & ML_TWOSIDED))
    {
      if (P_PointOnLineSide (slidemo->x, slidemo->y, li))
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



//
// was P_SlideMove
// The px / py move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
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


//
// Attack functions
//
Actor *linetarget;     // who got hit (or NULL)
Actor *shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t         shootz;
fixed_t         lastz; //SoM: The last z height of the bullet when it crossed a line

int             la_damage;
fixed_t         attackrange;

fixed_t         aimslope;



// ---------------------------------------
// was P_SpawnPuff
// 
void Map::SpawnPuff(fixed_t x, fixed_t y, fixed_t z)
{
  Actor *puff;
  extern mobjtype_t PuffType;

  z += P_SignedRandom()<<10;

  if (game.mode == heretic)
    {
      puff = SpawnActor(x, y, z, PuffType);
      if(puff->info->attacksound)
        {
	  S_StartSound(puff, puff->info->attacksound);
        }
      switch(PuffType)
        {
	case MT_BEAKPUFF:
	case MT_STAFFPUFF:
	  puff->pz = FRACUNIT;
	  break;
	case MT_GAUNTLETPUFF1:
	case MT_GAUNTLETPUFF2:
	  puff->pz = .8*FRACUNIT;
	default:
	  break;
        }
    }
  else
    {        
      puff = SpawnActor(x,y,z, MT_PUFF);
      puff->pz = FRACUNIT;
      puff->tics -= P_Random()&3;
        
      if (puff->tics < 1)
	puff->tics = 1;
        
      // don't make punches spark on the wall
      if (attackrange == MELEERANGE)
	puff->SetState(S_PUFF3);
    }
}


//
// PTR_AimTraverse
// Sets linetarget and aimslope when a target is aimed at.
//
//added:15-02-98: comment
// Returns true if the thing is not shootable, else continue through..
//
static bool PTR_AimTraverse (intercept_t *in)
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
      li = in->d.line;

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
  th = in->d.thing;
  if (th == shootthing)
    return true;                    // can't shoot self

  if ((!(th->flags&MF_SHOOTABLE)) || (th->flags&MF_CORPSE) || (th->type == MT_POD))
    return true;                    // corpse or something

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


//
// PTR_ShootTraverse
//
//added:18-02-98: added clipping the shots on the floor and ceiling.
//
static bool PTR_ShootTraverse (intercept_t *in)
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

      li = in->d.line;

      if (li->special)
	m->ShootSpecialLine(shootthing, li);

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
      if (!hitplane && game.demoversion>=129)
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

	  //added:24-02-98: compatibility with older demos
	  if (game.demoversion<112)
            {
	      diffheights = true;
	      hitplane = false;
            }

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
  Actor *th = in->d.thing;
  if (th == shootthing)
    return true;            // can't shoot self

  if (!(th->flags & MF_SHOOTABLE))
    return true;            // corpse or something

  // check for physical attacks on a ghost
  // FIXME this is stupid. We should use some sort of damage type system.
  // (if damagetype != dt_ethereal ...)
  /*
  if (game.mode == heretic && (th->flags & MF_SHADOW))
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

  if (game.demoversion<125)
    {
      // Spawn bullet puffs or blood spots,
      // depending on target type.
      if (in->d.thing->flags & MF_NOBLOOD)
	m->SpawnPuff(x,y,z);
      else
	m->SpawnBlood(x,y,z, la_damage);
    }

  if (la_damage)
    hitplane = th->Damage(shootthing, shootthing, la_damage);
  else
    hitplane = false;

  if (game.demoversion>=125)
    {
      // Spawn bullet puffs or blood spots,
      // depending on target type.
      if (in->d.thing->flags & MF_NOBLOOD && game.mode != heretic)
	m->SpawnPuff (x,y,z);
      else
        {
	  if (game.mode == heretic)
            {
	      extern mobjtype_t PuffType;
	      if (PuffType == MT_BLASTERPUFF1)
		// Make blaster big puff
		S_StartSound(m->SpawnActor(x, y, z, MT_BLASTERPUFF2), sfx_blshit);
	      else
		m->SpawnPuff(x, y, z);
            }    
	  if (hitplane) {
	    m->SpawnBloodSplats (x,y,z, la_damage, trace.dx, trace.dy);
	    return false;
	  }
        }
    }

  // don't go any farther
  return false;

}


//
// was P_AimLineAttack
//
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


//
// was P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//
//added:16-02-98: Fab comments...
//                ang    est l'angle de tir sur le plan x,y (orientation)
//                distance est la port‚e maximale de la balle
//                slope    est la pente vers la destination (up/down)
//                damage   est les degats infliges par la balle
void Actor::LineAttack(angle_t ang, fixed_t distance, fixed_t slope, int damage)
{
  fixed_t     x2;
  fixed_t     y2;

  ang >>= ANGLETOFINESHIFT;
  shootthing = this;
  la_damage = damage;

  // FIXME see AimLineAttack, same here
  // player autoaimed attack, 
  /*
  if (game.demoversion<128 || !player)
    {   
      x2 = x + (distance>>FRACBITS)*finecosine[ang]; 
      y2 = y + (distance>>FRACBITS)*finesine[ang];   
    }
  else
  */
    {
      fixed_t cosangle = finecosine[aiming>>ANGLETOFINESHIFT];

      x2 = x + FixedMul(FixedMul(distance,finecosine[ang]),cosangle);
      y2 = y + FixedMul(FixedMul(distance,finesine[ang]),cosangle); 
    }

  shootz = lastz = z + (height>>1) + 8*FRACUNIT;
  if (flags2 & MF2_FEETARECLIPPED)
    shootz -= FOOTCLIPSIZE;

  attackrange = distance;
  aimslope = slope;

  tmthing = shootthing;

  mp->PathTraverse (x, y, x2, y2,
		  PT_ADDLINES|PT_ADDTHINGS,
		  PTR_ShootTraverse);
}

//
// USE LINES
//
PlayerPawn *usething;

static bool PTR_UseTraverse (intercept_t *in)
{
  int         side;

  tmthing = NULL;
  if (!in->d.line->special)
    {
      P_LineOpening (in->d.line);
      if (openrange <= 0)
        {
	  if (game.mode != heretic)
	    S_StartSound (usething, sfx_noway);

	  // can't use through a wall
	  return false;
        }
      // not a special line, but keep checking
      return true ;
    }

  side = 0;
  if (P_PointOnLineSide (usething->x, usething->y, in->d.line) == 1)
    side = 1;

  //  return false;           // don't use back side
  usething->mp->UseSpecialLine (usething, in->d.line, side);

  // can't use for than one special line in a row
  // SoM: USE MORE THAN ONE!
  if (boomsupport && (in->d.line->flags&ML_PASSUSE))
    return true;
  else
    return false;
}


//
// was P_UseLines
// Looks for special lines in front of the player to activate.
//
void PlayerPawn::UseLines()
{
  int ang;
  fixed_t     x1;
  fixed_t     y1;
  fixed_t     x2;
  fixed_t     y2;

  usething = this;

  ang = angle >> ANGLETOFINESHIFT;

  x1 = x;
  y1 = y;
  x2 = x1 + (USERANGE>>FRACBITS)*finecosine[ang];
  y2 = y1 + (USERANGE>>FRACBITS)*finesine[ang];

  mp->PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse);
}


//
// RADIUS ATTACK
//
Actor *bombowner;
Actor *bomb;
int    bombdamage;


//
// PIT_RadiusAttack
// "bombowner" is the creature
// that caused the explosion at "bomb".
//
static bool PIT_RadiusAttack (Actor *thing)
{
  fixed_t     dx;
  fixed_t     dy;
  fixed_t     dz;
  fixed_t     dist;

  if (!(thing->flags & MF_SHOOTABLE))
    return true;

  // Boss spider and cyborg
  // take no damage from concussion.
  if (thing->type == MT_CYBORG
      || thing->type == MT_SPIDER
      || thing->type == MT_MINOTAUR 
      || thing->type == MT_SORCERER1
      || thing->type == MT_SORCERER2)
    return true;

  dx = abs(thing->x - bomb->x);
  dy = abs(thing->y - bomb->y);

  dist = dx>dy ? dx : dy;
  dist -= thing->radius;

  //added:22-02-98: now checks also z dist for rockets exploding
  //                above yer head...
  if (game.demoversion>=112)
    {
      dz = abs(thing->z+(thing->height>>1) - bomb->z);
      dist = dist > dz ? dist : dz;
    }
  dist >>= FRACBITS;

  if (dist < 0)
    dist = 0;

  if (dist >= bombdamage)
    return true;    // out of range

  if (thing->floorz > bomb->z && bomb->ceilingz < thing->z)
    return true;

  if (thing->ceilingz < bomb->z && bomb->floorz > thing->z)
    return true;

  if (thing->mp->CheckSight(thing, bomb))
    {
      int  damage=bombdamage - dist;
      int  apx=0, apy=0;
      if (dist)
        {
	  apx = (thing->x - bomb->x)/dist;
	  apy = (thing->y - bomb->y)/dist;
        }
      // must be in direct path
      if (thing->Damage(bomb, bombowner, damage) && (thing->flags & MF_NOBLOOD)==0 && game.demoversion>=129)
	thing->mp->SpawnBloodSplats(thing->x,thing->y,thing->z, damage, apx, apy);
    }

  return true;
}


//
// was P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void Actor::RadiusAttack(Actor *culprit, int damage)
{
  int nx, ny;

  int         xl;
  int         xh;
  int         yl;
  int         yh;

  fixed_t     dist;

  dist = (damage+MAXRADIUS)<<FRACBITS;
  yh = (y + dist - mp->bmaporgy)>>MAPBLOCKSHIFT;
  yl = (y - dist - mp->bmaporgy)>>MAPBLOCKSHIFT;
  xh = (x + dist - mp->bmaporgx)>>MAPBLOCKSHIFT;
  xl = (x - dist - mp->bmaporgx)>>MAPBLOCKSHIFT;
  bomb = this;
  if (type == MT_POD && owner)
    bombowner = owner;
  else
    bombowner = culprit;
  bombdamage = damage;

  for (ny=yl ; ny<=yh ; ny++)
    for (nx=xl ; nx<=xh ; nx++)
      mp->BlockThingsIterator (nx, ny, PIT_RadiusAttack);
}



//
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
//
bool         crushchange;
bool         nofit;
sector_t        *sectorchecked;

//
// PIT_ChangeSector
//
static bool PIT_ChangeSector (Actor *thing)
{
  if (P_ThingHeightClip (thing))
    {
      // keep checking
      return true;
    }

  // crunch bodies to giblets
  if (thing->flags & MF_CORPSE)
    {
      if (!game.raven)
        {
	  thing->SetState(S_GIBS);
	  thing->flags &= ~MF_SOLID;
	  //added:22-02-98: lets have a neat 'crunch' sound!
	  S_StartSound (thing, sfx_slop);
        }
      thing->height = 0;
      thing->radius = 0;
      //thing->skin = 0;

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

  if (! (thing->flags & MF_SHOOTABLE))
    {
      // assume it is bloody gibs or something
      return true;
    }

  nofit = true;

  if (crushchange && !(thing->mp->maptic % (4*NEWTICRATERATIO)))
    {
      thing->Damage(NULL,NULL,10);

      if (game.demoversion<132 || (!(thing->mp->maptic % (16*NEWTICRATERATIO)) && 
			      !(thing->flags&MF_NOBLOOD)))
        {
	  // spray blood in a random direction
	  Actor *mo = thing->mp->SpawnActor(thing->x, thing->y,
	    thing->z + thing->height/2, MT_BLOOD);
            
	  mo->px  = P_SignedRandom()<<12;
	  mo->py  = P_SignedRandom()<<12;
        }
    }

  // keep checking (crush other things)
  return true;
}



//
// was P_ChangeSector
//
bool Map::ChangeSector(sector_t *sector, bool crunch)
{
  int         x;
  int         y;

  nofit = false;
  crushchange = crunch;
  sectorchecked = sector;

  // re-check heights for all things near the moving sector
  for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
    for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
      BlockThingsIterator (x, y, PIT_ChangeSector);


  return nofit;
}

// was P_CheckSector
//SoM: 3/15/2000: New function. Much faster.
bool Map::CheckSector(sector_t *sector, bool crunch)
{
  msecnode_t      *n;

  if (!boomsupport) // use the old routine for old demos though
    return ChangeSector(sector,crunch);

  nofit = false;
  crushchange = crunch;


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




/*
  SoM: 3/15/2000
  Lots of new Boom functions that work faster and add functionality.
*/

static msecnode_t *headsecnode = NULL;

void P_Initsecnode()
{
  headsecnode = NULL;
}

// P_GetSecnode() retrieves a node from the freelist. The calling routine
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
    node = (msecnode_t*)Z_Malloc (sizeof(*node), PU_LEVEL, NULL);
  return(node);
}

// P_PutSecnode() returns a node to the freelist.

void P_PutSecnode(msecnode_t *node)
{
  node->m_snext = headsecnode;
  headsecnode = node;
}

// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.

msecnode_t *P_AddSecnode(sector_t *s, Actor *thing, msecnode_t *nextnode)
{
  msecnode_t *node;

  node = nextnode;
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
  node->visited = 0;

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


// P_DelSecnode() deletes a sector node from the list of
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
  return(NULL);
}

// Delete an entire sector list

void P_DelSeclist(msecnode_t *node)

{
  while (node)
    node = P_DelSecnode(node);
}


// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.

bool PIT_GetSectors(line_t *ld)
{
  if (tmbbox[BOXRIGHT]  <= ld->bbox[BOXLEFT]   ||
      tmbbox[BOXLEFT]   >= ld->bbox[BOXRIGHT]  ||
      tmbbox[BOXTOP]    <= ld->bbox[BOXBOTTOM] ||
      tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
    return true;

  if (P_BoxOnLineSide(tmbbox, ld) != -1)
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


// was P_CreateSecNodeList
// alters/creates the sector_list that shows what sectors
// the object resides in.

void Map::CreateSecNodeList(Actor *thing, fixed_t x, fixed_t y)
{
  int xl;
  int xh;
  int yl;
  int yh;
  int bx;
  int by;
  msecnode_t *node;

  // First, clear out the existing m_thing fields. As each node is
  // added or verified as needed, m_thing will be set properly. When
  // finished, delete all nodes where m_thing is still NULL. These
  // represent the sectors the Thing has vacated.

  node = sector_list;
  while (node)
    {
      node->m_thing = NULL;
      node = node->m_tnext;
    }

  tmthing = thing;
  tmflags = thing->flags;

  P_SetBox(x, y, tmthing->radius);

  validcount++; // used to make sure we only process a line once

  xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

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
}

// heretic code

//---------------------------------------------------------------------------
//
// PIT_CheckOnmobjZ
//
//---------------------------------------------------------------------------
Actor *onmobj; //generic global onmobj...used for landing on pods/players

static bool PIT_CheckOnmobjZ(Actor *thing)
{
  fixed_t blockdist;
    
  if (!(thing->flags&(MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
    { // Can't hit thing
      return true;
    }
  blockdist = thing->radius+tmthing->radius;
  if (abs(thing->x-tmx) >= blockdist || abs(thing->y-tmy) >= blockdist)
    { // Didn't hit thing
      return true;
    }
  if (thing == tmthing)
    { // Don't clip against self
      return true;
    }
  if (tmthing->z > thing->z+thing->height)
    {
      return true;
    }
  else if (tmthing->z+tmthing->height < thing->z)
    { // under thing
      return true;
    }
  if (thing->flags&MF_SOLID)
    {
      onmobj = thing;
    }
  return(!(thing->flags&MF_SOLID));
}

//=============================================================================
//
// was P_FakeZMovement
//
//              Fake the zmovement so that we can check if a move is legal
//=============================================================================

void Actor::FakeZMovement()
{
  extern consvar_t cv_gravity;
  //
  // adjust height
  //
  // z += pz;
  fixed_t nz = z + pz;
  if ((flags & MF_FLOAT) && target)
    {       // float down towards target if too close
      if (!(flags&MF_SKULLFLY) && !(flags&MF_INFLOAT))
	{
	  int dist = P_AproxDistance(x - target->x, y - target->y);
	  int delta = target->z + (height>>1) - nz;
	  if (delta < 0 && dist < -(delta*3))
	    nz -= FLOATSPEED;
	  else if (delta > 0 && dist < (delta*3))
	    nz += FLOATSPEED;
	}
    }
  if ((flags2 & MF2_FLY) && !(nz <= floorz)
      && mp->maptic & 2)
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
      if (flags & MF_SKULLFLY)
	{ // The skull slammed into something
	  npz = -npz;
	}
      if (info->crashstate && (flags & MF_CORPSE))
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
      if (flags & MF_SKULLFLY)
	{       // the skull slammed into something
	  npz = -npz;
	}
    }
  z = nz;
  pz = npz;
}

//=============================================================================
// was P_CheckOnmobj
// Checks if the new Z position is legal

Actor *Actor::CheckOnmobj()
{
  int          xl,xh,yl,yh,bx,by;
  subsector_t *newsubsec;
  //Actor oldmo;
    
  tmthing = this;
  tmflags = flags;
  //oldmo = *thing; // save the old mobj before the fake zmovement
  fixed_t oldz = z;
  fixed_t oldpz = pz;
  tmthing->FakeZMovement();
  
  P_SetBox(x, y, tmthing->radius);
    
  newsubsec = mp->R_PointInSubsector (x, y);
  ceilingline = NULL;
    
  //
  // the base floor / ceiling is from the subsector that contains the
  // point.  Any contacted lines the step closer together will adjust them
  //
  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;
    
  validcount++;
  numspechit = 0;
    
  if (tmflags & MF_NOCLIP)
    return NULL;
    
  //
  // check things first, possibly picking things up
  // the bounding box is extended by MAXRADIUS because Actors are grouped
  // into mapblocks based on their origin point, and can overlap into adjacent
  // blocks by up to MAXRADIUS units
  //
  xl = (tmbbox[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
    
  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockThingsIterator(bx,by,PIT_CheckOnmobjZ))
	{
	  //*tmthing = oldmo;
	  z = oldz;
	  pz = oldpz;
	  return onmobj;
	}

  //*tmthing = oldmo;
  z = oldz;
  pz = oldpz;
  return NULL;
}


// =========================================================================
//                         MOVEMENT CLIPPING
// =========================================================================

//
// was P_CheckPosition
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

  P_SetBox(nx, ny, radius);

  subsector_t *newsubsec = mp->R_PointInSubsector(nx,ny);
  ceilingline = blockingline = NULL;

  // The base floor / ceiling is from the subsector
  // that contains the point.
  // Any contacted lines the step closer together
  // will adjust them.
  tmfloorz = tmsectorfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = tmsectorceilingz = newsubsec->sector->ceilingheight;

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
  tmfloorthing = NULL;

  validcount++;
  numspechit = 0;

  if (tmflags & MF_NOCLIP)
    return true;

  // Check things first, possibly picking things up.
  // The bounding box is extended by MAXRADIUS
  // because Actors are grouped into mapblocks
  // based on their origin point, and can overlap
  // into adjacent blocks by up to MAXRADIUS units.

  int xl, xh, yl, yh, bx, by;
  fixed_t bmox = mp->bmaporgx; 
  fixed_t bmoy = mp->bmaporgy; 

  // BP: added MF_NOCLIPTHING :used by camera to not be blocked by things
  if (!(flags & MF_NOCLIPTHING))
    {
      xl = (tmbbox[BOXLEFT] - bmox - MAXRADIUS)>>MAPBLOCKSHIFT;
      xh = (tmbbox[BOXRIGHT] - bmox + MAXRADIUS)>>MAPBLOCKSHIFT;
      yl = (tmbbox[BOXBOTTOM] - bmoy - MAXRADIUS)>>MAPBLOCKSHIFT;
      yh = (tmbbox[BOXTOP] - bmoy + MAXRADIUS)>>MAPBLOCKSHIFT;
        
      for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	  if (!mp->BlockThingsIterator(bx,by,PIT_CheckThing))
	    return false;
    }
  // check lines
  xl = (tmbbox[BOXLEFT] - bmox)>>MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmox)>>MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmoy)>>MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmoy)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
    for (by=yl ; by<=yh ; by++)
      if (!mp->BlockLinesIterator(bx,by,PIT_CheckLine))
	return false;

  return true;
}


//----------------------------------------------------------------------------
//
// was P_TestMobjLocation
//
// Returns true if the mobj is not blocked by anything at its current
// location, otherwise returns false.
//
//----------------------------------------------------------------------------

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
