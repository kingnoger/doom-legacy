// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003 by DooM Legacy Team.
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
// Revision 1.1  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
//
//
// DESCRIPTION:
//   Mapthing-related event functions
//
//-----------------------------------------------------------------------------


#include "g_game.h"
#include "g_actor.h"
#include "g_map.h"

#include "s_sound.h"
#include "sounds.h"
#include "tables.h"

extern mobjtype_t TranslateThingType[];


//==========================================================================
//
// was ActivateThing
//
//==========================================================================

bool DActor::Activate()
{
  if (flags & MF_COUNTKILL)
    { // Monster
      if (flags2 & MF2_DORMANT)
	{
	  flags2 &= ~MF2_DORMANT;
	  tics = 1;
	  return true;
	}
      return false;
    }

  // TODO this could probably be replaced with
  // SetState(attackstate/spawnstate) and corresponding StartSound()
  // i.e. store this functionality in the mobjinfo struct...
  switch (type)
    {
    case MT_ZTWINEDTORCH:
    case MT_ZTWINEDTORCH_UNLIT:
      SetState(S_ZTWINEDTORCH_1);
      S_StartSound(this, SFX_IGNITE);
      break;
    case MT_ZWALLTORCH:
    case MT_ZWALLTORCH_UNLIT:
      SetState(S_ZWALLTORCH1);
      S_StartSound(this, SFX_IGNITE);
      break;
    case MT_ZGEMPEDESTAL:
      SetState(S_ZGEMPEDESTAL2);
      break;
    case MT_ZWINGEDSTATUENOSKULL:
      SetState(S_ZWINGEDSTATUENOSKULL2);
      break;
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
    case MT_ZFIREBULL:
    case MT_ZFIREBULL_UNLIT:
      SetState(S_ZFIREBULL_BIRTH);
      S_StartSound(this, SFX_IGNITE);
      break;
    case MT_ZBELL:
      if (health > 0)
	Damage(NULL, NULL, 10); // 'ring' the bell
      break;
    case MT_ZCAULDRON:
    case MT_ZCAULDRON_UNLIT:
      SetState(S_ZCAULDRON1);
      S_StartSound(this, SFX_IGNITE);
      break;
    case MT_FLAME_SMALL:
      S_StartSound(this, SFX_IGNITE);
      SetState(S_FLAME_SMALL1);
      break;
    case MT_FLAME_LARGE:
      S_StartSound(this, SFX_IGNITE);
      SetState(S_FLAME_LARGE1);
      break;
    case MT_BAT_SPAWNER:
      SetState(S_SPAWNBATS1);
      break;
    default:
      return false;
      break;
    }
  return true;
}

//==========================================================================
//
// was DeactivateThing
//
//==========================================================================

bool DActor::Deactivate()
{
  if (flags & MF_COUNTKILL)
    { // Monster
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
    case MT_ZTWINEDTORCH:
    case MT_ZTWINEDTORCH_UNLIT:
      SetState(S_ZTWINEDTORCH_UNLIT);
      break;
    case MT_ZWALLTORCH:
    case MT_ZWALLTORCH_UNLIT:
      SetState(S_ZWALLTORCH_U);
      break;
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
    case MT_ZFIREBULL:
    case MT_ZFIREBULL_UNLIT:
      SetState(S_ZFIREBULL_DEATH);
      break;
    case MT_ZCAULDRON:
    case MT_ZCAULDRON_UNLIT:
      SetState(S_ZCAULDRON_U);
      break;
    case MT_FLAME_SMALL:
      SetState(S_FLAME_SDORM1);
      break;
    case MT_FLAME_LARGE:
      SetState(S_FLAME_LDORM1);
      break;
    case MT_BAT_SPAWNER:
      SetState(S_SPAWNBATS_OFF);
      break;
    default:
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
  if (game.nomonsters && (mobjinfo[moType].flags & MF_COUNTKILL))   
    return false; // Don't spawn monsters if -nomonsters
    
  angle_t angle = int(args[2] << 24);
  int fineAngle = angle >> ANGLETOFINESHIFT;
  fixed_t speed = int(args[3] << 13);
  fixed_t vspeed = int(args[4] << 13);
  while ((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      DActor *newMobj = SpawnDActor(mobj->x, mobj->y, mobj->z, moType);
      if (newMobj->info->seesound)
	S_StartSound(newMobj, newMobj->info->seesound);

      newMobj->owner = mobj; // Originator
      newMobj->angle = angle;
      newMobj->px = FixedMul(speed, finecosine[fineAngle]);
      newMobj->py = FixedMul(speed, finesine[fineAngle]);
      newMobj->pz = vspeed;
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
  fixed_t z;

  bool success = false;
  int searcher = -1;
  int tid = args[0];
  mobjtype_t moType = TranslateThingType[args[1]];
  if (game.nomonsters && (mobjinfo[moType].flags & MF_COUNTKILL))
    return false; // Don't spawn monsters if -nomonsters

  angle_t angle = int(args[2] << 24);
  while ((mobj = FindFromTIDmap(tid, &searcher)) != NULL)
    {
      if (mobjinfo[moType].flags2 & MF2_FLOATBOB)
	z = mobj->z - mobj->floorz;
      else
	z = mobj->z;

      DActor *newMobj = SpawnDActor(mobj->x, mobj->y, z, moType);
      if (!newMobj->TestLocation())
	newMobj->Remove(); // Didn't fit
      else
	{
	  newMobj->angle = angle;
	  if (fog)
	    {	      
	      DActor *fogMobj = SpawnDActor(mobj->x, mobj->y, mobj->z+TELEFOGHEIGHT, MT_TFOG);
	      S_StartSound(fogMobj, SFX_TELEPORT);
	    }

	  newMobj->flags |= MF_DROPPED; // Don't respawn
	  if (newMobj->flags2 & MF2_FLOATBOB)
	    newMobj->special1 = newMobj->z - newMobj->floorz;

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
      if (mobj->Type() == Thinker::tt_dactor)
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
      if (mobj->Type() == Thinker::tt_dactor)
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

