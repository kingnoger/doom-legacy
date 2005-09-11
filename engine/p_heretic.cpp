// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// Revision 1.29  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.28  2005/06/28 17:05:00  smite-meister
// item respawning cleaned up
//
// Revision 1.27  2005/06/08 17:29:38  smite-meister
// FS bugfixes
//
// Revision 1.26  2005/03/29 17:20:45  smite-meister
// state and mobjinfo tables fixed
//
// Revision 1.25  2005/03/16 21:16:06  smite-meister
// menu cleanup, bugfixes
//
// Revision 1.24  2004/12/02 17:22:34  smite-meister
// HUD fixed
//
// Revision 1.23  2004/11/13 22:38:42  smite-meister
// intermission works
//
// Revision 1.22  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.21  2004/08/12 18:30:23  smite-meister
// cleaned startup
//
// Revision 1.18  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.16  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.14  2003/11/12 11:07:21  smite-meister
// Serialization done. Map progression.
//
// Revision 1.12  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.11  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.10  2003/04/14 08:58:26  smite-meister
// Hexen maps load.
//
// Revision 1.9  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.8  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.7  2003/03/15 20:07:16  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.6  2003/03/08 16:07:07  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.5  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.4  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:11:35  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:17:58  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Heretic/Hexen specific extra game routines, gametype patching.

#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_map.h"

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
  angle >>= ANGLETOFINESHIFT;
  fixed_t thrust = 16 + P_FRandom(6);
  target->vel.x += thrust * finecosine[angle];
  target->vel.y += thrust * finesine[angle];
  target->Damage(NULL, NULL, HITDICE(6));

  //if(target->player)
  target->reactiontime = 14 + (P_Random()&7);
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
DActor *DActor::SpawnMissileAngle(mobjtype_t t, angle_t angle, fixed_t momz)
{
  fixed_t mz;

  switch (t)
    {
    case MT_MNTRFX1: // Minotaur swing attack missile
      mz = pos.z+40;
      break;
    case MT_MNTRFX2: // Minotaur floor fire missile
      mz = ONFLOORZ; // +floorclip; 
      break;
    case MT_SRCRFX1: // Sorcerer Demon fireball
      mz = pos.z+48;
      break;
    case MT_ICEGUY_FX2: // Secondary Projectiles of the Ice Guy
      mz = pos.z+3;
      break;
    case MT_MSTAFF_FX2:
      mz = pos.z+40;
      break;
    default:
      mz = pos.z+32;
      break;

    }

  mz -= floorclip;
    
  DActor *mo = mp->SpawnDActor(pos.x, pos.y, mz, t);
  if (mo->info->seesound)
    S_StartSound(mo, mo->info->seesound);

  mo->owner = this; // Originator
  mo->yaw = angle;
  angle >>= ANGLETOFINESHIFT;
  mo->vel.x = mo->info->speed * finecosine[angle];
  mo->vel.y = mo->info->speed * finesine[angle];
  mo->vel.z = momz;
  return mo->CheckMissileSpawn() ? mo : NULL;
}



//----------------------------------------------------------------------------

static DActor *LavaInflictor;

void P_InitLava()
{
  LavaInflictor = new DActor(MT_PHOENIXFX2);
  LavaInflictor->flags =  MF_NOBLOCKMAP | MF_NOGRAVITY;
  LavaInflictor->flags2 = MF2_FIREDAMAGE|MF2_NODMGTHRUST;
}

//----------------------------------------------------------------------------


void DoomPatchEngine()
{
  cv_jumpspeed.Set("6.0");

  // hacks: teleport fog, blood, gibs
  mobjinfo[MT_TFOG].spawnstate = S_TFOG;
  mobjinfo[MT_IFOG].spawnstate = S_IFOG;
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

  // hacks
  mobjinfo[MT_TFOG].spawnstate = S_HTFOG1;
  mobjinfo[MT_IFOG].spawnstate = S_HTFOG1;
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

  // hacks
  mobjinfo[MT_TFOG].spawnstate = S_XTFOG1;
  mobjinfo[MT_IFOG].spawnstate = S_XTFOG1;
  sprnames[SPR_BLUD] = "BLOD";
  states[S_GIBS].sprite = SPR_GIBS;
}
