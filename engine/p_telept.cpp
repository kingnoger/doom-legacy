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
// Revision 1.4  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/16 22:12:00  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:04  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.7  2002/09/20 22:41:33  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.6  2002/08/17 21:21:53  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.5  2002/08/08 12:01:29  vberghol
// pian engine on valmis!
//
// Revision 1.4  2002/07/23 19:21:44  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:22  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:16  vberghol
// Version 133 Experimental!
//
// Revision 1.8  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.7  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.6  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.5  2000/11/04 16:23:43  bpereira
// no message
//
// Revision 1.4  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Teleportation.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_map.h"

#include "p_maputl.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_main.h" //SoM: 3/16/2000

// was P_Teleport

bool Actor::Teleport(fixed_t nx, fixed_t ny, angle_t nangle, bool silent)
{
  // silent means "Boom-style"
  fixed_t  oldx, oldy, oldz;
  fixed_t  aboveFloor;

  oldx = x;
  oldy = y;
  oldz = z;
  aboveFloor = z - floorz;

  if (!TeleportMove(nx, ny))
    return false;

  /* 
  z = floorz;  //fixme: not needed?
  if (player)
    {
      // heretic code
      player_t *player = player;
      if(player->powers[pw_flight] && aboveFloor)
        {
	  z = floorz+aboveFloor;
	  if (z + height > ceilingz)
	    z = ceilingz-height;
        }
      player->viewz = z+player->viewheight;
    }
  else if (flags & MF_MISSILE) // heretic stuff
    {
      z = floorz+aboveFloor;
      if(z+height > ceilingz)
	z = ceilingz-height;
    }
  */
  //FIXME why can't all things retain their flying height like this:
  z = floorz + aboveFloor;
  if (z + height > ceilingz)
    z = ceilingz-height;

  if (!silent)
    {
      fixed_t fogDelta = 0;
      if (game.mode == gm_heretic && !(flags & MF_MISSILE))
	fogDelta = TELEFOGHEIGHT;

      // spawn teleport fog at source and destination
      DActor *fog = mp->SpawnDActor(oldx, oldy, oldz+fogDelta, MT_TFOG);
      S_StartSound(fog, sfx_telept);

      unsigned an = nangle >> ANGLETOFINESHIFT;
      fog = mp->SpawnDActor(nx+20*finecosine[an], ny+20*finesine[an], z + fogDelta, MT_TFOG);
      S_StartSound (fog, sfx_telept);

      if ((flags2 & MF2_FOOTCLIP) && (subsector->sector->floortype != FLOOR_SOLID) && (game.mode == gm_heretic))
	{
	  flags2 |= MF2_FEETARECLIPPED;
	}
      else if (flags2 & MF2_FEETARECLIPPED)
	{
	  flags2 &= ~MF2_FEETARECLIPPED;
	}

      if (flags & MF_MISSILE)
	{
	  fixed_t speed = P_AproxDistance(px, py);
	  px = FixedMul(speed, finecosine[an]);
	  py = FixedMul(speed, finesine[an]);
	}
      else
	px = py = pz = 0;
    }
  else
    {
      angle_t ang = nangle - angle;
      // Sine, cosine of angle adjustment
      fixed_t s = finesine[ang>>ANGLETOFINESHIFT];
      fixed_t c = finecosine[ang>>ANGLETOFINESHIFT];

      fixed_t tpx = px;
      fixed_t tpy = py;
      // Rotate thing's momentum to come out of exit just like it entered
      px = FixedMul(tpx, c) - FixedMul(tpy, s);
      py = FixedMul(tpy, c) + FixedMul(tpx, s);
    }

  angle = nangle;

  return true;
}

// =========================================================================
//                            TELEPORTATION
// =========================================================================

// was EV_Teleport

bool Map::EV_Teleport(line_t *line, int side, Actor *thing)
{
  int         i;
  Actor    *m;

  // don't teleport missiles
  // TODO give all non-heretic missiles the MF2_NOTELEPORT flag....simpler
  if (((thing->flags & MF_MISSILE) && game.mode != gm_heretic) 
      || (thing->flags2 & MF2_NOTELEPORT))
    return false;

  // Don't teleport if hit back of line,
  //  so you can get out of teleporter.
  if (side == 1)
    return false;


  int tag = line->tag;
  for (i = 0; i < numsectors; i++)
    {
      if (sectors[i].tag == tag )
        {
	  // FIXME why search the entire Thinker list, why not just the sector's thinglist?
	  for (m = sectors[i].thinglist; m != NULL; m = m->snext)
	    {
	      if (m->Type() != Thinker::tt_dactor)
		continue;
	      DActor *dm = (DActor *)m;
	      // not a teleportman
	      if (dm->type != MT_TELEPORTMAN)
		continue;

	      /*
		 sector_t *sec = m->subsector->sector;
	      // wrong sector
	      if (sec-sectors != i )
		continue;
	      */

	      return thing->Teleport(m->x, m->y, m->angle);
            }	  
        }
    }
  return false;
}




//
//  New boom teleporting functions.
//  

// was EV_SilentTeleport

bool Map::EV_SilentTeleport(line_t *line, int side, Actor *thing)
{
  int       i;
  Actor    *m;

  // don't teleport missiles
  // Don't teleport if hit back of line,
  // so you can get out of teleporter.

  if (side || thing->flags & MF_MISSILE)
    return false;

  for (i = -1; (i = FindSectorFromLineTag(line, i)) >= 0;)
    /*   
    for (th = thinkercap.next; th != &thinkercap; th = th->next)
      if (th->function.acp1 == (actionf_p1) P_MobjThinker &&
          (m = (Actor *) th)->type == MT_TELEPORTMAN  &&
          m->subsector->sector-sectors == i)
    */

    for (m = sectors[i].thinglist; m != NULL; m = m->snext)
        {
	  if (m->Type() != Thinker::tt_dactor)
	    continue;
	  DActor *dm = (DActor *)m;
	  // not a teleportman
	  if (dm->type != MT_TELEPORTMAN)
	    continue;

          // Get the angle between the exit thing and source linedef.
          // Rotate 90 degrees, so that walking perpendicularly across
          // teleporter linedef causes thing to exit in the direction
          // indicated by the exit thing.
          angle_t ang = m->angle + thing->angle
	    - R_PointToAngle2(0, 0, line->dx, line->dy) - ANG90;
	  // FIXME is the angle ever normalized to [0, 360] ?

	  return thing->Teleport(m->x, m->y, ang, true);
        }
  return false;
}

//
// Silent linedef-based TELEPORTATION, by Lee Killough
// Primarily for rooms-over-rooms etc.
// This is the complete player-preserving kind of teleporter.
// It has advantages over the teleporter with thing exits.
//

// maximum fixed_t units to move object to avoid hiccups
#define FUDGEFACTOR 10

int Map::EV_SilentLineTeleport(line_t *line, int side, Actor *thing, bool reverse)
{
  int i;
  line_t *l;

  if (side || thing->flags & MF_MISSILE)
    return 0;

  for (i = -1; (i = FindLineFromLineTag(line, i)) >= 0;)
    if ((l=lines+i) != line && l->backsector)
      {
        // Get the thing's position along the source linedef
        fixed_t pos = abs(line->dx) > abs(line->dy) ?
          FixedDiv(thing->x - line->v1->x, line->dx) :
          FixedDiv(thing->y - line->v1->y, line->dy) ;

        // Get the angle between the two linedefs, for rotating
        // orientation and momentum. Rotate 180 degrees, and flip
        // the position across the exit linedef, if reversed.
        angle_t angle = (reverse ? pos = FRACUNIT-pos, 0 : ANG180) +
          R_PointToAngle2(0, 0, l->dx, l->dy) -
          R_PointToAngle2(0, 0, line->dx, line->dy);

        // Interpolate position across the exit linedef
        fixed_t x = l->v2->x - FixedMul(pos, l->dx);
        fixed_t y = l->v2->y - FixedMul(pos, l->dy);

        // Maximum distance thing can be moved away from interpolated
        // exit, to ensure that it is on the correct side of exit linedef
        int fudge = FUDGEFACTOR;

        // Whether walking towards first side of exit linedef steps down
        int stepdown =
          l->frontsector->floorheight < l->backsector->floorheight;

        // Side to exit the linedef on positionally.
        //
        // Notes:
        //
        // This flag concerns exit position, not momentum. Due to
        // roundoff error, the thing can land on either the left or
        // the right side of the exit linedef, and steps must be
        // taken to make sure it does not end up on the wrong side.
        //
        // Exit momentum is always towards side 1 in a reversed
        // teleporter, and always towards side 0 otherwise.
        //
        // Exiting positionally on side 1 is always safe, as far
        // as avoiding oscillations and stuck-in-wall problems,
        // but may not be optimum for non-reversed teleporters.
        //
        // Exiting on side 0 can cause oscillations if momentum
        // is towards side 1, as it is with reversed teleporters.
        //
        // Exiting on side 1 slightly improves player viewing
        // when going down a step on a non-reversed teleporter.

        //int side = reverse || (player && stepdown);
	// FIXME why treat players differently?
	int side = reverse || stepdown;

        // Make sure we are on correct side of exit linedef.
        while (P_PointOnLineSide(x, y, l) != side && --fudge>=0)
          if (abs(l->dx) > abs(l->dy))
            y -= l->dx < 0 != side ? -1 : 1;
          else
            x += l->dy < 0 != side ? -1 : 1;

        // Height of thing above ground
        fixed_t z = thing->z - thing->floorz;

	if (!thing->Teleport(x, y, thing->angle + angle, true))
	  return false;

        // Adjust z position to be same height above ground as before.
        // Ground level at the exit is measured as the higher of the
        // two floor heights at the exit linedef.
        thing->z = z + sides[l->sidenum[stepdown]].sector->floorheight;

        return true;
      }
  return 0;
}

