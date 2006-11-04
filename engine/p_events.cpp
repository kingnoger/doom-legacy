// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
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
/// \brief Linedef events

#include "command.h"
#include "cvars.h"

#include "g_map.h"
#include "g_mapinfo.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_spec.h"
#include "p_acs.h"
#include "r_data.h"
#include "r_sky.h" // TODO remove
#include "sounds.h"

#include "m_random.h"
#include "z_zone.h"


bool P_CheckKeys(Actor *mo, int lock);
static int ZDoom_GenFloor(int target, int flags);
static int ZDoom_GenCeiling(int target, int flags);
static int ZDoom_GenLift(int target);



void Map::UpdateSpecials()
{
  //  LEVEL TIMER
  if (cv_timelimit.value && maptic > unsigned(cv_timelimit.value))
    ExitMap(NULL, 0);

  LightningFlash();
}


//========================================================
//  Hexen lightning effect
//========================================================

void Map::InitLightning()
{
  Flashcount = 0;

  if (!info->lightning)
    return;

  int count = 0;
  for (int i = 0; i < numsectors; i++)
    {
      if(sectors[i].ceilingpic == skyflatnum
	 || sectors[i].special == SS_IndoorLightning1
	 || sectors[i].special == SS_IndoorLightning2)
	count++;
    }

  if (!count)
    {
      info->lightning = false; // no way to see the flashes
      return;
    }

  LightningLightLevels = (int *)Z_Malloc(count * sizeof(int), PU_LEVEL, NULL);
  NextLightningFlash = ((P_Random() & 15) + 5) * 35; // don't flash at level start
}



void Map::ForceLightning()
{
  NextLightningFlash = 0;
}



void Map::LightningFlash()
{
  int i;
  sector_t *s;
  int *lite;

  if (!info->lightning)
    return;

  NextLightningFlash--;

  if (Flashcount-- > 0)
    {
      if (Flashcount > 0)
	{
	  // still flashing
	  lite = LightningLightLevels;
	  s = sectors;
	  for (i = 0; i < numsectors; i++, s++)
	    {
	      if (s->ceilingpic == skyflatnum 
		  || s->special == SS_IndoorLightning1
		  || s->special == SS_IndoorLightning2)
		{
		  if (*lite < s->lightlevel - 4)
		    s->lightlevel -= 4;

		  lite++;
		}
	    }
	}					
      else
	{
	  // flash ends
	  lite = LightningLightLevels;
	  s = sectors;
	  for (i = 0; i < numsectors; i++, s++)
	    {
	      if (s->ceilingpic == skyflatnum
		  || s->special == SS_IndoorLightning1
		  || s->special == SS_IndoorLightning2)
		{
		  s->lightlevel = *lite;
		  lite++;
		}
	    }
	  skytexture = tc.GetPtr(info->sky1.c_str(), TEX_wall); // set default sky
	}
      return;
    }


  if (NextLightningFlash > 0)
    return;

  // new flash
  Flashcount = (P_Random() & 7) + 8;
  int flashlight = 200 + (P_Random() & 31);
  s = sectors;
  lite = LightningLightLevels;

  for (i = 0; i < numsectors; i++, s++)
    {
      if (s->ceilingpic == skyflatnum
	  || s->special == SS_IndoorLightning1
	  || s->special == SS_IndoorLightning2)
	{
	  *lite = s->lightlevel;
	  if (s->special == SS_IndoorLightning1)
	    { 
	      s->lightlevel += 64;
	      if (s->lightlevel > flashlight)
		s->lightlevel = flashlight;
	    }
	  else if (s->special == SS_IndoorLightning2)
	    {
	      s->lightlevel += 32;
	      if (s->lightlevel > flashlight)
		s->lightlevel = flashlight;
	    }
	  else
	    s->lightlevel = flashlight;

	  if (s->lightlevel < *lite)
	    s->lightlevel = *lite;

	  lite++;
	}
    }

  skytexture = tc.GetPtr(info->sky2.c_str(), TEX_wall); // set alternate sky
  S_StartAmbSound(NULL, SFX_THUNDER_CRASH);

  // Calculate the next lighting flash
  if (P_Random() < 50)
    NextLightningFlash = (P_Random()&15) + 16; // Immediate Quick flash
  else
    {
      if (P_Random() < 128 && !(maptic & 32))
	NextLightningFlash = ((P_Random()&7) + 2) * 35;
      else
	NextLightningFlash = ((P_Random()&15) + 5) * 35;
    }
}




//============================================================================
//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//
//============================================================================

// New Hexen-derived linedef system.

bool Map::ActivateLine(line_t *line, Actor *thing, int side, int atype)
{
  // The triggerer's ability to cause an atype activation has been checked by the caller.
  // Next we check if the line responds to it.

  unsigned spec = unsigned(line->special);
  PlayerPawn *p = thing->IsOf(PlayerPawn::_type) ? reinterpret_cast<PlayerPawn*>(thing) : NULL;

  // Boom generalized types first
  if (boomsupport && spec >= GenCrusherBase)
    {
      // consistency check
      switch (spec & 6) // bits 1 and 2 of the trigger field ("once")
	{
	case WalkOnce:
	  if (!(atype == SPAC_CROSS || atype == SPAC_MCROSS || atype == SPAC_PCROSS))
	    return false;
	  break;

	case SwitchOnce:
	  if (!(atype == SPAC_USE || atype == SPAC_PASSUSE))
	    return false;
	  break;

	case GunOnce:
	  if (atype != SPAC_IMPACT)
	    return false;
	  break;

	case PushOnce: // like opening a door, means using it
	  if (!(atype == SPAC_USE || atype == SPAC_PASSUSE)) // yeah. SPAC_USE, not SPAC_PUSH.
	    return false;
	  break;

	default:
	  I_Error("Boom generalized type logic breakdown!\n");
	}

      // pointer to line function is NULL by default, set non-null if
      // line special is walkover generalized linedef type
      int (Map::*linefunc)(line_t *line) = NULL;
  
      // check each range of generalized linedefs
      if (spec >= GenFloorBase)
	{
	  if (!p && ((spec & FloorChange) || !(spec & FloorModel)))
	    return false;   // FloorModel is "Allow Monsters" if FloorChange is 0
	  linefunc = &Map::EV_DoGenFloor;
	}
      else if (spec >= GenCeilingBase)
	{
	  if (!p && ((spec & CeilingChange) || !(spec & CeilingModel)))
	    return false;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
	  linefunc = &Map::EV_DoGenCeiling;
	}
      else if (spec >= GenDoorBase)
	{
	  if (!p && (!(spec & DoorMonster) || (line->flags & ML_SECRET)))
	    // monsters disallowed from this door
	    // they can't open secret doors either
	    return false;
	  linefunc = &Map::EV_DoGenDoor;
	}
      else if (spec >= GenLockedBase)
	{
	  if (!p || !p->CanUnlockGenDoor(line))
	    return false;  // monsters disallowed from unlocking doors, players need key
	  linefunc = &Map::EV_DoGenLockedDoor;
	}
      else if (spec >= GenLiftBase)
	{
	  if (!p && !(spec & LiftMonster))
	    return false; // monsters disallowed
	  linefunc = &Map::EV_DoGenLift;
	}
      else if (spec >= GenStairsBase)
	{
	  if (!p && !(spec & StairMonster))
	    return false; // monsters disallowed
	  linefunc = &Map::EV_DoGenStairs;
	}
      else // if (spec >= GenCrusherBase)
	{
	  if (!p && !(spec & CrusherMonster))
	    return false; // monsters disallowed
	  linefunc = &Map::EV_DoGenCrusher;
	}

      if (!linefunc)
	return false; // should not happen
  
      if (!line->tag)
	if ((spec & 6) != 6) // "door" types can be used without a tag
	  return false;

      switch ((spec & TriggerType) >> TriggerTypeShift)
	{
	case WalkOnce:
	case PushOnce:
	  if ((this->*linefunc)(line))
	    line->special = 0;  // clear special
	  return true;

	case WalkMany:
	case PushMany:
	  (this->*linefunc)(line);
	  return true;

	case SwitchOnce:
	case GunOnce:
	  if ((this->*linefunc)(line))
	    ChangeSwitchTexture(line, 0);
	  return true;

	case SwitchMany:
	case GunMany:
	  if ((this->*linefunc)(line))
	    ChangeSwitchTexture(line, 1);
	  return true;

	default:
	  return false;
	}
    }
  // Boom generalized types done

  int act = GET_SPAC(line->flags);
  if (act != atype)
    return false;

  if (!p)
    {
      // ML_MONSTERS_CAN_ACTIVATE enables monsters to activate SPAC_CROSS and SPAC_USE,
      // for other activation types we have corresponding Actor flags.
      // HOWEVER: monsters never open secret doors

      bool monster_ok = (!(act <= SPAC_USE || act == SPAC_PASSUSE) || (line->flags & ML_MONSTERS_CAN_ACTIVATE))
	&& !(line->flags & ML_SECRET);

      if (!monster_ok)
	return false;
    }

  bool repeat = line->flags & ML_REPEAT_SPECIAL;
  bool success = ExecuteLineSpecial(spec, line->args, line, side, thing);

  if (!repeat && success)
    line->special = 0;    // clear the special on non-retriggerable lines

  if ((act == SPAC_USE || act == SPAC_PASSUSE || act == SPAC_IMPACT) && success)
    ChangeSwitchTexture(line, repeat); // check if texture needs to be changed

  return true;
}



// 
// Hexen linedefs
//
#define SPEED(a)  (fixed_t(a)/8)
#define HEIGHT(a) (fixed_t(a))
#define TICS(a)   (((a)*TICRATE)/35)
#define OCTICS(a) (((a)*TICRATE)/8)
#define ANGLE(a)  angle_t((a) << 24) // angle_t is 32-bit int, Hexen angle is a 8-bit byte

bool Map::ExecuteLineSpecial(unsigned special, byte *args, line_t *line, int side, Actor *mo)
{
  bool success = false;
  int lock;

  // callers: ACS, thing death, thing pickup, line activation

  // line->tag == line->args[0] and args[0] are not always same (scripts!)  FIXME which one should we use?
  // we always have args, but line may be NULL

  int tag = args[0];
  if (line && tag == 255)
    {
      // special case to handle converted Doom/Heretic linedefs
      tag = line->tag;
    }
  
  CONS_Printf("ExeSpecial (%d), tag %d (%d,%d,%d,%d,%d)\n", special, tag,
	      args[0], args[1], args[2], args[3], args[4]);
  switch (special)
    {
    case 0: // NOP
      break;
    case 1: // Poly Start Line
      break;
    case 2: // Poly Rotate Left
      success = EV_RotatePoly(args, 1, false);
      break;
    case 3: // Poly Rotate Right
      success = EV_RotatePoly(args, -1, false);
      break;
    case 4: // Poly Move
      success = EV_MovePoly(args, false, false);
      break;
    case 5: // Poly Explicit Line:  Only used in initialization
      break;
    case 6: // Poly Move Times 8
      success = EV_MovePoly(args, true, false);
      break;
    case 7: // Poly Door Swing
      success = EV_OpenPolyDoor(args, polyobject_t::pd_swing);
      break;
    case 8: // Poly Door Slide
      success = EV_OpenPolyDoor(args, polyobject_t::pd_slide);
      break;
    case 10: // Door Close
      success = EV_DoDoor(tag, line, mo, vdoor_t::Close, SPEED(args[1]), TICS(args[2]));
      break;
    case 11: // Door Open
      success = EV_DoDoor(tag, line, mo, vdoor_t::Open, SPEED(args[1]), TICS(args[2]));
      break;
    case 12: // Door Raise
      success = EV_DoDoor(tag, line, mo, vdoor_t::OwC, SPEED(args[1]), TICS(args[2]));
      break;
    case 13: // Door Locked_Raise
      if (P_CheckKeys(mo, args[3]))
	success = EV_DoDoor(tag, line, mo, vdoor_t::OwC, SPEED(args[1]), TICS(args[2]));
      break;
    case 20: // Floor Lower by Value
      success = EV_DoFloor(tag, line, floor_t::RelHeight, -SPEED(args[1]), 0, -HEIGHT(args[2]));
      break;
    case 21: // Floor Lower to Lowest
      success = EV_DoFloor(tag, line, floor_t::LnF, -SPEED(args[1]), 0, 0);
      break;
    case 22: // Floor Lower to Nearest
      success = EV_DoFloor(tag, line, floor_t::DownNnF, -SPEED(args[1]), 0, 0);
      break;
    case 23: // Floor Raise by Value
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(args[1]), 0, HEIGHT(args[2]));
      break;
    case 24: // Floor Raise to Highest
      success = EV_DoFloor(tag, line, floor_t::HnF, SPEED(args[1]), 0, 0);
      break;
    case 25: // Floor Raise to Nearest
      success = EV_DoFloor(tag, line, floor_t::UpNnF, SPEED(args[1]), 0, 0);
      break;
    case 26: // Stairs Build Down Normal
      success = EV_BuildHexenStairs(tag, stair_t::Normal, -SPEED(args[1]), -HEIGHT(args[2]), args[4], args[3]);
      break;
    case 27: // Build Stairs Up Normal
      success = EV_BuildHexenStairs(tag, stair_t::Normal, SPEED(args[1]), HEIGHT(args[2]), args[4], args[3]);
      break;
    case 28: // Floor Raise and Crush
      success = EV_DoFloor(tag, line, floor_t::Ceiling, SPEED(args[1]), args[2], -HEIGHT(8));
      break;
    case 29: // Build Pillar (no crushing)
      success = EV_DoElevator(tag, elevator_t::ClosePillar, SPEED(args[1]), HEIGHT(args[2]), 0, 0);
      break;
    case 30: // Open Pillar
      success = EV_DoElevator(tag, elevator_t::OpenPillar, SPEED(args[1]), HEIGHT(args[2]), HEIGHT(args[3]), 0);
      break;
    case 31: // Stairs Build Down Sync
      success = EV_BuildHexenStairs(tag, stair_t::Sync, -SPEED(args[1]), -HEIGHT(args[2]), args[3], 0);
      break;
    case 32: // Build Stairs Up Sync
      success = EV_BuildHexenStairs(tag, stair_t::Sync, SPEED(args[1]), HEIGHT(args[2]), args[3], 0);
      break;
    case 35: // Raise Floor by Value Times 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(args[1]), 0, 8*HEIGHT(args[2]));
      break;
    case 36: // Lower Floor by Value Times 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, -SPEED(args[1]), 0, -8*HEIGHT(args[2]));
      break;
    case 40: // Ceiling Lower by Value
      success = EV_DoCeiling(tag, line, ceiling_t::RelHeight, -SPEED(args[1]), 0, -HEIGHT(args[2]));
      break;
    case 41: // Ceiling Raise by Value
      success = EV_DoCeiling(tag, line, ceiling_t::RelHeight, SPEED(args[1]), 0, HEIGHT(args[2]));
      break;
    case 42: // Ceiling Crush and Raise
      success = EV_DoCrusher(tag, args[4] ? ceiling_t::Silent : 0, SPEED(args[1]),
			     args[3] ? SPEED(args[3]) : SPEED(args[1])/2, args[2], HEIGHT(8));
      break;
    case 43: // Ceiling Lower and Crush
      success = EV_DoCeiling(tag, line, ceiling_t::Floor, -SPEED(args[1]), args[2], HEIGHT(8));
      break;
    case 44: // Ceiling Crush Stop
      success = EV_StopCeiling(tag);
      break;
    case 45: // Ceiling Crush Raise and Stay
      success = EV_DoCrusher(tag, crusher_t::CrushOnce, SPEED(args[1]), SPEED(args[1])/2, args[2], HEIGHT(8));
      break;
      /*
      case 46: // Floor Crush Stop TODO activefloors list or something
      success = EV_FloorCrushStop(line, args);
      break;
      */
    case 60: // Plat Perpetual Raise
      success = EV_DoPlat(tag, line, plat_t::LHF, SPEED(args[1]), TICS(args[2]), 0);
      break;
    case 61: // Plat Stop
      EV_StopPlat(tag);
      break;
    case 62: // Plat Down-Wait-Up-Stay
      success = EV_DoPlat(tag, line, plat_t::LnF, SPEED(args[1]), TICS(args[2]), 0);
      break;
    case 63: // Plat Down-by-Value*8-Wait-Up-Stay
      success = EV_DoPlat(tag, line, plat_t::RelHeight, SPEED(args[1]), TICS(args[2]), -8*HEIGHT(args[3]));
      break;
    case 64: // Plat Up-Wait-Down-Stay
      success = EV_DoPlat(tag, line, plat_t::NHnF, SPEED(args[1]), TICS(args[2]), 0);
      break;
    case 65: // Plat Up-by-Value*8-Wait-Down-Stay
      success = EV_DoPlat(tag, line, plat_t::RelHeight, SPEED(args[1]), TICS(args[2]), 8*HEIGHT(args[3]));
      break;
    case 66: // Floor Lower Instant * 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, -SPEED(16000), 0, -8*HEIGHT(args[2]));
      break;
    case 67: // Floor Raise Instant * 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(16000), 0, 8*HEIGHT(args[2]));
      break;
    case 68: // Floor Move to Value * 8
      success = EV_DoFloor(tag, line, floor_t::AbsHeight, SPEED(args[1]), 0,
			   (args[3] ? -1 : 1) * 8 * HEIGHT(args[2]));
      break;
    case 69: // Ceiling Move to Value * 8
      success = EV_DoCeiling(tag, line, ceiling_t::AbsHeight, SPEED(args[1]), 0,
			     (args[3] ? -1 : 1) * 8 * HEIGHT(args[2]));
      break;
    case 70: // Teleport
      if (!side)
	// Only teleport when crossing the front side of a line
	success = EV_Teleport(tag, line, mo, args[1], args[2]);
      break;
    case 71: // Teleport, no fog (silent)
      if (!side)
	// Only teleport when crossing the front side of a line
	success = EV_Teleport(tag, line, mo, TP_toTID, TP_silent);
      break;
    case 72: // Thrust Mobj
      if(!side) // Only thrust on side 0
	{
	  mo->Thrust(ANGLE(args[0]), fixed_t(args[1]));
	  success = 1;
	}
      break;
    case 73: // Damage Mobj
      if (args[0])
	mo->Damage(NULL, NULL, args[0]);
      else
	// If arg1 is zero, then guarantee a kill
	mo->Damage(NULL, NULL, 10000, dt_always);
      success = true;
      break;
    case 74: // Teleport_NewMap
      if (!side)
	{ // Only teleport when crossing the front side of a line
	  if (mo->health > 0 && !(mo->flags & MF_CORPSE))
	    {
	      // Activator must be alive
	      ExitMap(mo, args[0], args[1]);
	      success = true;
	    }
	}
      break;
    case 75: // Teleport_EndGame
      if (!side)
	{ // Only teleport when crossing the front side of a line
	  if (mo->health > 0 && !(mo->flags & MF_CORPSE))
	    {
	      if (cv_deathmatch.value)
		ExitMap(mo, 1, 0);// Winning in deathmatch just goes back to map 1
	      else
		ExitMap(mo, -1, 0);

	      success = true;
	    }
	}
      break;
    case 80: // ACS_Execute
      if (args[1] && args[1] != info->mapnumber)
	success = P_AddToACSStore(args[1], args[0], &args[2]);
      else
	success = StartACS(args[0], &args[2], mo, line, side);
      break;
    case 81: // ACS_Suspend
      success = SuspendACS(args[0]);
      break;
    case 82: // ACS_Terminate
      success = TerminateACS(args[0]);
      break;
    case 83: // ACS_LockedExecute
      lock = args[4];
      if (P_CheckKeys(mo, lock))
	{
	  args[4] = 0;
	  if (args[1] && args[1] != info->mapnumber)
	    success = P_AddToACSStore(args[1], args[0], &args[2]);
	  else
	    success = StartACS(args[0], &args[2], mo, line, side);
	  args[4] = lock;
	}
      break;

    case 90: // Poly Rotate Left Override
      success = EV_RotatePoly(args, 1, true);
      break;
    case 91: // Poly Rotate Right Override
      success = EV_RotatePoly(args, -1, true);
      break;
    case 92: // Poly Move Override
      success = EV_MovePoly(args, false, true);
      break;
    case 93: // Poly Move Times 8 Override
      success = EV_MovePoly(args, true, true);
      break;
    case 94: // Build Pillar Crush
      success = EV_DoElevator(tag, elevator_t::ClosePillar, SPEED(args[1]), HEIGHT(args[2]), 0, args[3]);
      break;
    case 95: // Lower Floor and Ceiling
      success = EV_DoElevator(tag, elevator_t::RelHeight, SPEED(args[1]), -HEIGHT(args[2]));
      break;
    case 96: // Raise Floor and Ceiling
      success = EV_DoElevator(tag, elevator_t::RelHeight, SPEED(args[1]), HEIGHT(args[2]));
      break;
    case 109: // Force Lightning
      success = true;
      ForceLightning();
      break;
    case 110: // Light Raise by Value
      success = EV_SpawnLight(tag, lightfx_t::RelChange, args[1]);
      break;
    case 111: // Light Lower by Value
      success = EV_SpawnLight(tag, lightfx_t::RelChange, -args[1]);
      break;
    case 112: // Light Change to Value
      success = EV_SpawnLight(tag, lightfx_t::AbsChange, args[1]);
      break;
    case 113: // Light Fade
      success = EV_SpawnLight(tag, lightfx_t::Fade, args[1], 0, args[2]);
      break;
    case 114: // Light Glow
      success = EV_SpawnLight(tag, lightfx_t::Glow, args[1], args[2], args[3]);
      break;
    case 115: // Light Flicker
      success = EV_SpawnLight(tag, lightfx_t::Flicker, args[1], args[2], 32, 8);
      break;
    case 116: // Light Strobe
      success = EV_SpawnLight(tag, lightfx_t::Strobe, args[1], args[2], args[3], args[4]);
      break;
    case 120: // Quake Tremor
      success = EV_LocalQuake(args);
      break;
    case 129: // UsePuzzleItem. Not really needed, see P_UseArtifact()
      success = EV_LineSearchForPuzzleItem(line, args, mo);
      break;
    case 130: // Thing_Activate
      success = EV_ThingActivate(args[0]);
      break;
    case 131: // Thing_Deactivate
      success = EV_ThingDeactivate(args[0]);
      break;
    case 132: // Thing_Remove
      success = EV_ThingRemove(args[0]);
      break;
    case 133: // Thing_Destroy
      success = EV_ThingDestroy(args[0]);
      break;
    case 134: // Thing_Projectile
      success = EV_ThingProjectile(args, 0);
      break;
    case 135: // Thing_Spawn
      success = EV_ThingSpawn(args, 1);
      break;
    case 136: // Thing_ProjectileGravity
      success = EV_ThingProjectile(args, 1);
      break;
    case 137: // Thing_SpawnNoFog
      success = EV_ThingSpawn(args, 0);
      break;
    case 138: // Floor_Waggle
      success = EV_DoFloorWaggle(tag, HEIGHT(args[1]) >> 3, angle_t(args[2] << 20), ANGLE(args[3]) << 2, args[4]*35);
      break;
    case 140: // Sector_SoundChange
      success = EV_SectorSoundChange(args[0], args[1]);
      break;

      // Line specials only processed during level initialization
      // 100: Scroll_Texture_Left
      // 101: Scroll_Texture_Right
      // 102: Scroll_Texture_Up
      // 103: Scroll_Texture_Down
      // 121: Line_SetIdentification

      // TODO other ZDoom Generic types?

    case 200: // ZDoom Generic_Floor
      {
	bool neg_height = args[4] & 0x20;
	success = EV_DoFloor(tag, line, ZDoom_GenFloor(args[3], args[4]),
			     args[4] & 0x08 ? SPEED(args[1]) : -SPEED(args[1]),
			     (args[4] & 0x10) ? 20 : 0,
			     neg_height ? -HEIGHT(args[2]) : HEIGHT(args[2]));
      }
      break;
    case 201: // ZDoom Generic_Ceiling
      {
	bool neg_height = args[4] & 0x20;
	success = EV_DoCeiling(tag, line, ZDoom_GenCeiling(args[3], args[4]),
			       args[4] & 0x08 ? SPEED(args[1]) : -SPEED(args[1]),
			       (args[4] & 0x10) ? 20 : 0,
			       neg_height ? -HEIGHT(args[2]) : HEIGHT(args[2]));
      }
      break;
    case 202: // ZDoom Generic_Door
      if (P_CheckKeys(mo, args[4])) // TODO key order different from ZDoom
	success = EV_DoDoor(tag, line, mo, args[2], SPEED(args[1]), OCTICS(args[3]));
      break;
    case 203: // ZDoom Generic_Lift
      success = EV_DoPlat(tag, line, ZDoom_GenLift(args[3]), SPEED(args[1]), TICS(args[2]), HEIGHT(args[4]));
      break;
    case 217: // ZDoom Stairs_BuildUpDoom (TODO only partial implementation)
      success = EV_BuildStairs(tag, 0, SPEED(args[1]), HEIGHT(args[2]), 0);
      break;
    case 215: // ZDoom Teleport_Line
      if (!side) // Only teleport when crossing the front side of a line
	success = EV_Teleport(tag, line, mo, TP_toLine, (args[2] & 0x1) ? TP_flip : 0);
      break;
    case 232: // ZDoom Light_StrobeDoom
      success = EV_StartLightStrobing(tag, args[1], args[2]);
      break;
    case 233: // ZDoom Light_MinNeighbor
      success = EV_TurnTagLightsOff(tag);
      break;
    case 234: // ZDoom Light_MaxNeighbor
      success = EV_LightTurnOn(tag, 0);
      break;
    case 245: // ZDoom Elevator_RaiseToNearest
      success = EV_DoElevator(tag, elevator_t::Up, SPEED(args[1]), 0);
      break;
    case 246: // ZDoom Elevator_MoveToFloor
      if (line)
	success = EV_DoElevator(tag, elevator_t::Current, SPEED(args[1]), line->frontsector->floorheight);
      else
	success = false;
      break;
    case 247: // ZDoom Elevator_LowerToNearest
      success = EV_DoElevator(tag, elevator_t::Down, SPEED(args[1]), 0);
      break;
    case 250: // ZDoom Floor_Donut
      success = EV_DoDonut(tag, SPEED(args[1]), SPEED(args[2]));
      break;

    case LEGACY_EXT: // Legacy extensions to Hexen linedef namespace (all under this one type)

      // different tag handling
      if (line && line->tag) 
	tag = line->tag; // Doom style
      else
	tag = args[3] + (args[4] << 8); // Hexen style

      switch (args[0])
	{
	case LEGACY_FS:
	  if (side == 0 || args[1] == 0) // 1-sided?
	    success = FS_RunScript(tag, mo);
	  break;

	default:
	  CONS_Printf("Unknown Legacy linedef extension %d.\n", args[0]);
	}
      break;

      // Inert Line specials
    default:
      CONS_Printf("Unhandled line special %d\n", special);
      break;
    }

  return success;
}





int Map::EV_SectorSoundChange(int tag, int seq)
{
  if (!tag)
    return false;

  int secNum = -1;
  int rtn = 0;
  while ((secNum = FindSectorFromTag(tag, secNum)) >= 0)
    {
      sectors[secNum].seqType = seq;
      rtn++;
    }
  return rtn;
}





/// interprets ZDoom linedeftype 200 fields
static int ZDoom_GenFloor(int target, int flags)
{
  int type = 0;

  switch (target)
    {
    case 0:
      type = floor_t::RelHeight;
      break;
    case 1:
      type = floor_t::HnF;
      break;
    case 2:
      type = floor_t::LnF;
      break;
    case 3:
      if (flags & 0x08)
	type = floor_t::UpNnF;
      else
	type = floor_t::DownNnF;
      break;
    case 4:
      type = floor_t::LnC;
      break;
    case 5:
      type = floor_t::Ceiling;
      break;
    case 6:
      if (flags & 0x08)
	type = floor_t::UpSLT;
      else
	type = floor_t::DownSLT;
      break;
    }

  // fallthrus intended
  switch (flags & 0x03)
    {
    case 1:
      type |= floor_t::ZeroSpecial;
    case 3:
      type |= floor_t::SetSpecial;
    case 2:
      type |= floor_t::SetTexture;
    }

  if (flags & 0x04)
    type |= floor_t::NumericModel;

  return type;
}


/// interprets ZDoom linedeftype 201 fields
static int ZDoom_GenCeiling(int target, int flags)
{
  int type = 0;

  switch (target)
    {
    case 0:
      type = ceiling_t::RelHeight;
      break;
    case 1:
      type = ceiling_t::HnC;
      break;
    case 2:
      type = ceiling_t::LnC;
      break;
    case 3:
      if (flags & 0x08)
	type = ceiling_t::UpNnC;
      else
	type = ceiling_t::DownNnC;
      break;
    case 4:
      type = ceiling_t::HnF;
      break;
    case 5:
      type = ceiling_t::Floor;
      break;
    case 6:
      if (flags & 0x08)
	type = ceiling_t::UpSUT;
      else
	type = ceiling_t::DownSUT;
      break;
    }

  // fallthrus intended
  switch (flags & 0x03)
    {
    case 1:
      type |= ceiling_t::ZeroSpecial;
    case 3:
      type |= ceiling_t::SetSpecial;
    case 2:
      type |= ceiling_t::SetTexture;
    }

  if (flags & 0x04)
    type |= ceiling_t::NumericModel;

  return type;
}


/// interprets ZDoom linedeftype 203 fields
static int ZDoom_GenLift(int target)
{
  int type = 0;

  switch (target & 0x0F)
    {
    case 0:
      type = plat_t::RelHeight;
      break;
    case 1:
      type = plat_t::LnF;
      break;
    case 2:
      //type = plat_t::NLnF;
      type = plat_t::NHnF;
      break;
    case 3:
      type = plat_t::LnC;
      break;
    case 4:
      type = plat_t::LHF;
      break;
    case 5: // extension
      type = plat_t::CeilingToggle;
      break;
    }

  // Extension: High bits used as flags!
  if (target & 0x10)
    type |= plat_t::SetTexture;

  if (target & 0x20)
    type |= plat_t::ZeroSpecial;

  return type;
}
