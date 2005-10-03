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
// Revision 1.20  2005/10/03 17:12:25  smite-meister
// zdoom fix
//
// Revision 1.19  2005/09/12 18:33:43  smite-meister
// fixed_t, vec_t
//
// Revision 1.18  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.17  2005/06/05 19:32:25  smite-meister
// unsigned map structures
//
// Revision 1.16  2004/11/04 21:12:53  smite-meister
// save/load fixed
//
// Revision 1.14  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.12  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.10  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.8  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.7  2003/04/14 08:58:27  smite-meister
// Hexen maps load.
//
// Revision 1.6  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.5  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Teleportation.

#include "doomdef.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "p_spec.h"
#include "m_random.h"
#include "p_maputl.h"

#include "r_defs.h"
#include "sounds.h"
#include "tables.h"

bool Actor::Teleport(fixed_t nx, fixed_t ny, angle_t nangle, bool silent)
{
  // silent means "Boom-style"
  fixed_t  oldx, oldy, oldz;
  fixed_t  aboveFloor;

  oldx = pos.x;
  oldy = pos.y;
  oldz = pos.z;
  aboveFloor = pos.z - floorz;

  if (!TeleportMove(nx, ny))
    return false;

  // CHANGED now all things retain their flying height like this:
  pos.z = floorz + aboveFloor;
  if (Top() > ceilingz)
    pos.z = ceilingz-height;

  if (!silent)
    {
      fixed_t fogDelta = 0;
      if (game.mode == gm_heretic && !(flags & MF_MISSILE))
	fogDelta = TELEFOGHEIGHT;

      // spawn teleport fog at source and destination
      DActor *fog = mp->SpawnDActor(oldx, oldy, oldz+fogDelta, MT_TFOG);
      S_StartSound(fog, sfx_teleport);

      unsigned an = nangle >> ANGLETOFINESHIFT;
      fog = mp->SpawnDActor(nx+20*finecosine[an], ny+20*finesine[an], pos.z + fogDelta, MT_TFOG);
      S_StartSound (fog, sfx_teleport);

      if ((flags2 & MF2_FOOTCLIP) && (subsector->sector->floortype >= FLOOR_LIQUID))
	floorclip = FOOTCLIPSIZE;
      else
	floorclip = 0;

      if (flags & MF_MISSILE)
	{
	  fixed_t speed = P_AproxDistance(vel.x, vel.y);
	  vel.x = speed * finecosine[an];
	  vel.y = speed * finesine[an];
	}
      else
	vel.Set(0,0,0);
    }
  else
    {
      angle_t ang = nangle - yaw;
      // Sine, cosine of angle adjustment
      fixed_t s = finesine[ang>>ANGLETOFINESHIFT];
      fixed_t c = finecosine[ang>>ANGLETOFINESHIFT];

      fixed_t tpx = vel.x;
      fixed_t tpy = vel.y;
      // Rotate thing's momentum to come out of exit just like it entered
      vel.x = (tpx * c) - (tpy * s);
      vel.y = (tpy * c) + (tpx * s);
    }

  yaw = nangle;

  return true;
}



//=========================================================================
//                            TELEPORTATION
//=========================================================================

bool Map::EV_Teleport(int tag, line_t *line, Actor *thing, int type, int flags)
{
  if (!line)
    return false;

  // don't teleport missiles
  // TODO give all Doom missiles the MF2_NOTELEPORT flag....simpler
  if (thing->flags2 & MF2_NOTELEPORT)
    return false;

  Actor *m;
  int i;

  bool noplayer = flags & TP_noplayer;
  bool silent   = flags & TP_silent;
  bool reldir   = flags & TP_reldir;
  bool reverse  = flags & TP_flip;

  if (noplayer && thing->IsOf(PlayerPawn::_type))
    return false;

  if (type == TP_toTID) // go to thing with correct TID (Hexen system)
    {
      i = TIDmap.count(tag);
      if (i > 0)
	{
	  i = (P_Random() % i) - 1;
	  m = FindFromTIDmap(tag, &i);
	  if (!m)
	    I_Error("Can't find teleport mapspot\n");

	  return thing->Teleport(m->pos.x, m->pos.y, m->yaw, silent); // does the angle change work correctly?
	}
    }
  else if (type == TP_toThingInSector) // go to teleport thing in tagged sector
    {
      for (i = -1; (i = FindSectorFromTag(tag, i)) >= 0;)
	for (m = sectors[i].thinglist; m != NULL; m = m->snext)
	  {
	    if (!m->IsOf(DActor::_type))
	      continue;
	    DActor *dm = (DActor *)m;
	    // not a teleportman
	    if (dm->type != MT_TELEPORTMAN)
	      continue;

	    if (silent || reldir)
	      {
		// Get the angle between the exit thing and source linedef.
		// Rotate 90 degrees, so that walking perpendicularly across
		// teleporter linedef causes thing to exit in the direction
		// indicated by the exit thing.
		angle_t ang = m->yaw + thing->yaw - R_PointToAngle2(0, 0, line->dx, line->dy) - ANG90;
		return thing->Teleport(m->pos.x, m->pos.y, ang, true);
	      }
	    else
	      return thing->Teleport(m->pos.x, m->pos.y, m->yaw, false);
	  }
    }
  else // if (type == TP_toLine)
    return EV_SilentLineTeleport(tag, line, thing, reverse);

  return false;
}



// Silent linedef-based TELEPORTATION, by Lee Killough
// Primarily for rooms-over-rooms etc.
// This is the complete player-preserving kind of teleporter.
// It has advantages over the teleporter with thing exits.

bool Map::EV_SilentLineTeleport(int tag, line_t *line, Actor *thing, bool reverse)
{
  if (!line)
    return false;

  if (thing->flags2 & MF2_NOTELEPORT)
    return false;

  line_t *l;
  for (int i = -1; (l = FindLineFromTag(tag, &i)) != NULL;)
    if (l != line && l->backsector)
      {
        // Get the thing's position along the source linedef
        fixed_t pos = abs(line->dx) > abs(line->dy) ?
          (thing->pos.x - line->v1->x) / line->dx :
	  (thing->pos.y - line->v1->y) / line->dy ;

        // Get the angle between the two linedefs, for rotating
        // orientation and momentum. Rotate 180 degrees, and flip
        // the position across the exit linedef, if reversed.
        angle_t angle = (reverse ? pos = 1-pos, 0 : ANG180) +
          R_PointToAngle2(0, 0, l->dx, l->dy) -
          R_PointToAngle2(0, 0, line->dx, line->dy);

        // Interpolate position across the exit linedef
        fixed_t x = l->v2->x - (pos * l->dx);
        fixed_t y = l->v2->y - (pos * l->dy);

        // Maximum distance thing can be moved away from interpolated
        // exit, to ensure that it is on the correct side of exit linedef

	// maximum fixed_t units to move object to avoid hiccups
#define FUDGEFACTOR 10
        int fudge = FUDGEFACTOR;

        // Whether walking towards first side of exit linedef steps down
        int stepdown = l->frontsector->floorheight < l->backsector->floorheight;

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
            y -= l->dx < 0 != side ? -fixed_epsilon : fixed_epsilon;
          else
            x += l->dy < 0 != side ? -fixed_epsilon : fixed_epsilon;

        // Height of thing above ground
        fixed_t z = thing->Feet() - thing->floorz;

	if (!thing->Teleport(x, y, thing->yaw + angle, true))
	  return false;

        // Adjust z position to be same height above ground as before.
        // Ground level at the exit is measured as the higher of the
        // two floor heights at the exit linedef.
        thing->pos.z = z + l->sideptr[stepdown]->sector->floorheight;

        return true;
      }
  return false;
}
