// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
/// \brief Heretic/Hexen specific extra game routines, gametype patching.

#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_map.h"
#include "g_decorate.h"

#include "p_enemy.h"
#include "p_maputl.h"
#include "p_setup.h"

#include "sounds.h"
#include "m_random.h"
#include "dstrings.h"

#include "tables.h"

#include "w_wad.h"
#include "z_zone.h"



void P_MinotaurSlam(Actor *source, Actor *target)
{
  angle_t angle = R_PointToAngle2(source->pos, target->pos);
  fixed_t thrust = 16 + P_FRandom(6);
  target->vel.x += thrust * Cos(angle);
  target->vel.y += thrust * Sin(angle);
  target->Damage(NULL, NULL, HITDICE(6)); // FIXME Hexen minotaur HITDICE(4)

  //if(target->player)
  target->reactiontime = 14 + (P_Random()&7);
  source->args[0] = 0; // Stop charging
}


bool P_TouchWhirlwind(Actor *target)
{
  target->yaw += P_SignedRandom()<<20;
  target->vel.x += P_SignedFRandom(6);
  target->vel.y += P_SignedFRandom(6);
  if (target->mp->maptic & 16 && !(target->flags2 & MF2_BOSS))
    {
      fixed_t randVal = P_Random();
      if (randVal > 160)
	randVal = 160;

      target->vel.z += randVal >> 6;
      if (target->vel.z > 12)
	target->vel.z = 12;
    }

  if (!(target->mp->maptic & 7))
    return target->Damage(NULL, NULL, 3);

  return false;
}


//----------------------------------------------------------------------------
// Returns 1 if 'source' needs to turn clockwise, or 0 if 'source' needs
// to turn counter clockwise.  'delta' is set to the amount 'source'
// needs to turn.
//----------------------------------------------------------------------------
int P_FaceMobj(Actor *source, Actor *target, angle_t *delta)
{
  angle_t diff;
  angle_t angle1 = source->yaw;
  angle_t angle2 = R_PointToAngle2(source->pos, target->pos);
  if (angle2 > angle1)
    {
      diff = angle2-angle1;
      if(diff > ANG180)
	{
	  *delta = ANGLE_MAX-diff;
	  return(0);
	}
      else
	{
	  *delta = diff;
	  return(1);
	}
    }
  else
    {
      diff = angle1-angle2;
      if(diff > ANG180)
	{
	  *delta = ANGLE_MAX-diff;
	  return(1);
	}
      else
	{
	  *delta = diff;
	  return(0);
	}
    }
}


// Returns true if target was tracked, false if not.
bool DActor::SeekerMissile(angle_t thresh, angle_t turnMax)
{
  Actor *t = target;

  if (t == NULL)
    return false;
   
  if (!(t->flags & MF_SHOOTABLE))
    { // Target died
      target = NULL;
      return false;
    }

  angle_t delta;
  int dir = P_FaceMobj(this, t, &delta);
  if (delta > thresh)
    {
      delta >>= 1;
      if (delta > turnMax)
	delta = turnMax;
    }
  if (dir)
    { // Turn clockwise
      yaw += delta;
    }
  else
    { // Turn counter clockwise
      yaw -= delta;
    }
  int ang = yaw >> ANGLETOFINESHIFT;
  vel.x = info->speed * finecosine[ang];
  vel.y = info->speed * finesine[ang];

  if (Top() < t->Feet() || t->Top() < Feet())
    { // Need to seek vertically
      int dist = (P_XYdist(t->pos, pos) / info->speed).floor();
      if (dist < 1)
	dist = 1;
      vel.z = (t->Center() - Center()) / dist;
    }
  return true;
}


// Returns NULL if the missile exploded immediately, otherwise returns
// a Actor pointer to the missile.
DActor *DActor::SpawnMissileAngle(mobjtype_t t, angle_t angle, fixed_t h, fixed_t vz)
{
  h += Feet() -floorclip;
    
  DActor *mo = mp->SpawnDActor(pos.x, pos.y, h, t);
  if (mo->info->seesound)
    S_StartSound(mo, mo->info->seesound);

  mo->owner = this; // Originator
  mo->yaw = angle;
  mo->vel.x = mo->info->speed * Cos(angle);
  mo->vel.y = mo->info->speed * Sin(angle);
  mo->vel.z = vz;
  return mo->CheckMissileSpawn() ? mo : NULL;
}



//----------------------------------------------------------------------------


void DoomPatchEngine()
{
  cv_jumpspeed.Set("6.0");
  cv_fallingdamage.Set("0");

  // hacks: teleport fog, blood, gibs
  mobjinfo[MT_TFOG].spawnstate = &states[S_TFOG];
  mobjinfo[MT_IFOG].spawnstate = &states[S_IFOG];
  sprnames[SPR_BLUD] = "BLUD";
  states[S_GIBS].sprite = SPR_POL5;

  // linedef conversions
  int lump = fc.GetNumForName("XDOOM");
  linedef_xtable = (xtable_t *)fc.CacheLumpNum(lump, PU_STATIC);
  linedef_xtable_size = fc.LumpLength(lump) / sizeof(xtable_t);
}


void HereticPatchEngine()
{
  cv_jumpspeed.Set("6.0");
  cv_fallingdamage.Set("0");

  // hacks
  mobjinfo[MT_TFOG].spawnstate = &states[S_HTFOG1];
  mobjinfo[MT_IFOG].spawnstate = &states[S_HTFOG1];
  sprnames[SPR_BLUD] = "BLOD";
  states[S_GIBS].sprite = SPR_BLOD;

  int lump = fc.GetNumForName("XHERETIC");
  linedef_xtable = (xtable_t *)fc.CacheLumpNum(lump, PU_STATIC);
  linedef_xtable_size = fc.LumpLength(lump) / sizeof(xtable_t);

  // Above, good. Below, bad.
  text[TXT_PD_REDK] = "YOU NEED A GREEN KEY TO OPEN THIS DOOR";

  text[TXT_GOTBLUECARD] = "BLUE KEY";
  text[TXT_GOTYELWCARD] = "YELLOW KEY";
  text[TXT_GOTREDCARD] = "GREEN KEY";
}

void HexenPatchEngine()
{
  cv_jumpspeed.Set("9.0");
  cv_fallingdamage.Set("23");

  // hacks
  mobjinfo[MT_TFOG].spawnstate = &states[S_XTFOG1];
  mobjinfo[MT_IFOG].spawnstate = &states[S_XTFOG1];
  sprnames[SPR_BLUD] = "BLOD";
  states[S_GIBS].sprite = SPR_GIBS;
}
