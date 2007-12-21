// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
/// \brief Mapthing-related event functions

#include "command.h"
#include "cvars.h"
#include "g_actor.h"
#include "g_map.h"
#include "g_decorate.h"

#include "sounds.h"
#include "tables.h"

extern mobjtype_t TranslateThingType[];


bool DActor::Activate()
{
  if (flags & MF_MONSTER)
    {
      if (flags2 & MF2_DORMANT)
	{
	  flags2 &= ~MF2_DORMANT;
	  tics = 1;
	  return true;
	}
      return false;
    }

  // NOTE: for non-MF_MONSTERs, meleestate (attacksound) is the active state (sound), seestate (seesound) are the deactivated ones.
  // TODO spikes, bell

  switch (type)
    {
    case MT_THRUSTFLOOR_UP:
    case MT_THRUSTFLOOR_DOWN:
      if (args[0]==0)
	{
	  S_StartSound(this, SFX_THRUSTSPIKE_LOWER);
	  flags2 &= ~MF2_DONTDRAW;
	  if (args[1])
	    SetState(S_BTHRUSTRAISE1);
	  else
	    SetState(S_THRUSTRAISE1);
	}
      break;

    case MT_ZBELL:
      if (health > 0)
	Damage(NULL, NULL, 10); // 'ring' the bell
      break;

    default:
      if (info->attacksound)
	S_StartSound(this, info->attacksound);

      if (info->meleestate)
	SetState(info->meleestate);
      else
	return false;

      break;
    }
  return true;
}



bool DActor::Deactivate()
{
  if (flags & MF_MONSTER)
    {
      if(!(flags2 & MF2_DORMANT))
	{
	  flags2 |= MF2_DORMANT;
	  tics = -1;
	  return true;
	}
      return false;
    }

  switch (type)
    {
    case MT_THRUSTFLOOR_UP:
    case MT_THRUSTFLOOR_DOWN:
      if (args[0]==1)
	{
	  S_StartSound(this, SFX_THRUSTSPIKE_RAISE);
	  if (args[1])
	    SetState(S_BTHRUSTLOWER);
	  else
	    SetState(S_THRUSTLOWER);
	}
      break;

    default:
      //if (info->seesound)
      //  S_StartSound(this, info->seesound);

      if (info->seestate)
	SetState(info->seestate);
      else
	return false;

      break;
    }
  return true;
}



//==========================================================================
//
// EV_ThingProjectile
//
//==========================================================================

bool Map::EV_ThingProjectile(byte *args, bool gravity)
{
  Actor *mobj;

  bool success = false;
  int searcher = -1;
  int tid = args[0];
  mobjtype_t moType = TranslateThingType[args[1]];
  if (cv_nomonsters.value && (mobjinfo[moType].flags & MF_MONSTER))   
    return false;
    
  angle_t angle = int(args[2] << 24);
  int fineAngle = angle >> ANGLETOFINESHIFT;
  fixed_t speed = args[3]/8.0f;
  fixed_t vspeed = args[4]/8.0f;
  while ((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      DActor *newMobj = SpawnDActor(mobj->pos.x, mobj->pos.y, mobj->pos.z, moType);
      if (newMobj->info->seesound)
	S_StartSound(newMobj, newMobj->info->seesound);

      newMobj->owner = mobj; // Originator
      newMobj->yaw   = angle;
      newMobj->vel.x = speed * finecosine[fineAngle];
      newMobj->vel.y = speed * finesine[fineAngle];
      newMobj->vel.z = vspeed;
      newMobj->flags |= MF_DROPPED; // Don't respawn
      if (gravity)
	{
	  newMobj->flags &= ~MF_NOGRAVITY;
	  newMobj->flags2 |= MF2_LOGRAV;
	}

      if (newMobj->CheckMissileSpawn())
	success = true;
    }
  return success;
}

//==========================================================================
//
// EV_ThingSpawn
//
//==========================================================================

bool Map::EV_ThingSpawn(byte *args, bool fog)
{
  Actor *mobj;

  bool success = false;
  int searcher = -1;
  int tid = args[0];
  mobjtype_t moType = TranslateThingType[args[1]];
  if (cv_nomonsters.value && (mobjinfo[moType].flags & MF_MONSTER))
    return false;

  angle_t angle = int(args[2] << 24);
  while ((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      DActor *newMobj = SpawnDActor(mobj->pos.x, mobj->pos.y, mobj->pos.z, moType);

      if (!newMobj->TestLocation())
	newMobj->Remove(); // Didn't fit
      else
	{
	  newMobj->yaw = angle;
	  if (fog)
	    {	      
	      DActor *fogMobj = SpawnDActor(mobj->pos.x, mobj->pos.y, mobj->pos.z + TELEFOGHEIGHT, MT_TFOG);
	      S_StartSound(fogMobj, sfx_teleport);
	    }

	  newMobj->flags |= MF_DROPPED; // Don't respawn
	  if (newMobj->flags2 & MF2_FLOATBOB)
	    newMobj->special1 = (newMobj->pos.z - newMobj->floorz).floor();

	  success = true;
	}
    }
  return success;
}

//==========================================================================
//
// EV_ThingActivate
//
//==========================================================================

bool Map::EV_ThingActivate(int tid)
{
  Actor *mobj;
  bool success = false;
  int searcher = -1;
  while((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      if (mobj->IsOf(DActor::_type))
	if (((DActor *)mobj)->Activate())
	  success = true;
    }
  return success;
}

//==========================================================================
//
// EV_ThingDeactivate
//
//==========================================================================

bool Map::EV_ThingDeactivate(int tid)
{
  Actor *mobj;
  bool success = false;
  int searcher = -1;
  while((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      if (mobj->IsOf(DActor::_type))
	if (((DActor *)mobj)->Deactivate())
	  success = true;
    }
  return success;
}

//==========================================================================
//
// EV_ThingRemove
//
//==========================================================================

bool Map::EV_ThingRemove(int tid)
{
  Actor *mobj;
  bool success = false;
  int searcher = -1;
  while ((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      /*
	// unnecessary now
      if (mobj->type == MT_BRIDGE)
	{
	  A_BridgeRemove(mobj);
	  return true;
	}
      */
      mobj->Remove();
      success = true;
    }
  return success;
}

//==========================================================================
//
// EV_ThingDestroy
//
//==========================================================================

bool Map::EV_ThingDestroy(int tid)
{
  Actor *mobj;
  bool success = false;
  int searcher = -1;
  while((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      if (mobj->flags & MF_SHOOTABLE)
	{
	  mobj->Damage(NULL, NULL, 10000, dt_always);
	  success = true;
	}
    }
  return success;
}

//==========================================================================
//
// EV_ThingMove
//
// arg[0] = tid
// arg[1] = speed
// arg[2] = angle (255 = use mobj angle)
// arg[3] = distance (pixels>>2)
//
//==========================================================================

/*
bool EV_ThingMove(byte *args)
{
	return false;
}
*/

