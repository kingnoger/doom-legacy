// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004 by DooM Legacy Team.
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
/// \brief Re-implementation of some original Doom "features"

#include "doomdata.h"
#include "p_hacks.h"
#include "g_player.h"
#include "g_map.h"
#include "r_sprite.h"


VoodooDoll::VoodooDoll(const PlayerPawn &p)
  : PlayerPawn(p)
{
  flags  = MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP | MF_NOTMONSTER;
  flags2 = MF2_WINDTHRUST | MF2_PUSHABLE | MF2_SLIDE | MF2_PASSMOBJ | MF2_TELESTOMP;
  pres->color = 2;
}


void VoodooDoll::Spawn(PlayerInfo *p, mapthing_t *mthing)
{
  VoodooDoll *d = new VoodooDoll(*p->pawn); // a copy of the real pawn

  d->x = mthing->x << FRACBITS;
  d->y = mthing->y << FRACBITS;
  d->z = ONFLOORZ;

  // set player, team?
  d->victim = p->pawn;

  d->sprev = d->snext = NULL;

  Map *m = p->mp;
  m->SpawnActor(d);


  d->eflags |= MFE_ONGROUND;
  d->z = d->floorz;

  //d->angle = ANG45 * (mthing->angle/45);

  d->spawnpoint = mthing;
  mthing->mobj = d;

  // set the timer
  mthing->type = short((m->maptic + 20) & 0xFFFF);
}


bool VoodooDoll::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  return victim->Damage(inflictor, source, damage, dtype);
}


// TODO: pickup, linedef activation, zombie effect
