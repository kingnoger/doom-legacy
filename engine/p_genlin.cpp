// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
//
// DESCRIPTION:
//  Generalized linedef type handlers
//  Floors, Ceilings, Doors, Locked Doors, Lifts, Stairs, Crushers
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "g_game.h"
#include "g_map.h"
#include "r_data.h"
#include "p_spec.h" // geometry-related thinker classes
#include "m_random.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"

/*
  SoM: 3/9/2000: Copied this entire file from Boom sources to Legacy sources.
  This file contains all routiens for Generalized linedef types.
*/

//
// was EV_DoGenFloor()
//
// Handle generalized floor types
//
// Passed the line activating the generalized floor function
// Returns true if a thinker is created
//
int Map::EV_DoGenFloor(line_t *line)
{
  int                   secnum;
  int                   rtn;
  bool               manual;
  sector_t*             sec;
  floor_t*          floor;
  unsigned              value = (unsigned)line->special - GenFloorBase;

  // parse the bit fields in the line's special type

  int Crsh = (value & FloorCrush) >> FloorCrushShift;
  int ChgT = (value & FloorChange) >> FloorChangeShift;
  int Targ = (value & FloorTarget) >> FloorTargetShift;
  int Dirn = (value & FloorDirection) >> FloorDirectionShift;
  int ChgM = (value & FloorModel) >> FloorModelShift;
  int Sped = (value & FloorSpeed) >> FloorSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_floor;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_floor:                
      // Do not start another function if floor already moving
      if (P_SectorActive(floor_special,sec))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}

      fixed_t speed;
      // set the speed of motion
      switch (Sped)
	{
	case SpeedNormal:
	  speed = FLOORSPEED*2;
	  break;
	case SpeedFast:
	  speed = FLOORSPEED*4;
	  break;
	case SpeedTurbo:
	  speed = FLOORSPEED*8;
	  break;
	case SpeedSlow:
	default:
	  speed = FLOORSPEED;
	  break;
	}

      // new floor thinker
      rtn++;
      floor = new floor_t(floor_t::RelHeight, sec, speed, Crsh ? 10 : 0, 0);
      AddThinker(floor);

      floor->direction = Dirn? 1 : -1;
      floor->texture = sec->floorpic;
      floor->newspecial = sec->special;

      // set the destination height
      switch(Targ)
	{
	case FtoHnF:
	  floor->destheight = P_FindHighestFloorSurrounding(sec);
	  break;
	case FtoLnF:
	  floor->destheight = P_FindLowestFloorSurrounding(sec);
	  break;
	case FtoNnF:
	  floor->destheight = Dirn?
	    P_FindNextHighestFloor(sec,sec->floorheight) :
	      P_FindNextLowestFloor(sec,sec->floorheight);
	  break;
	case FtoLnC:
	  floor->destheight = P_FindLowestCeilingSurrounding(sec);
	  break;
	case FtoC:
	  floor->destheight = sec->ceilingheight;
	  break;
	case FbyST:
	  floor->destheight = (floor->sector->floorheight>>FRACBITS) +
	    floor->direction * (FindShortestLowerAround(sec)>>FRACBITS);
	  if (floor->destheight>32000)
	    floor->destheight=32000;
	  if (floor->destheight<-32000)
	    floor->destheight=-32000;
	  floor->destheight<<=FRACBITS;
	  break;
	case Fby24:
	  floor->destheight = floor->sector->floorheight +
	    floor->direction * 24*FRACUNIT;
	  break;
	case Fby32:
	  floor->destheight = floor->sector->floorheight +
	    floor->direction * 32*FRACUNIT;
	  break;
	default:
	  break;
	}

      // set texture/type change properties
      if (ChgT)   // if a texture change is indicated
	{
	  if (ChgM) // if a numeric model change
	    {
	      sector_t *sec2;

	      sec2 = (Targ==FtoLnC || Targ==FtoC)?
		FindModelCeilingSector(floor->destheight,sec) :
		FindModelFloorSector(floor->destheight,sec);
	      if (sec2)
		{
		  floor->texture = sec2->floorpic;
		  switch(ChgT)
		    {
		    case FChgZero:  // zero type
		      floor->newspecial = 0;
		      floor->type |= floor_t::SetTxTy;
		      break;
		    case FChgTyp:   // copy type
		      floor->newspecial = sec2->special;
		      floor->type |= floor_t::SetTxTy;
		      break;
		    case FChgTxt:   // leave type be
		      floor->type |= floor_t::SetTexture;
		      break;
		    default:
		      break;
		    }
		}
	    }
	  else     // else if a trigger model change
	    {
	      floor->texture = line->frontsector->floorpic;
	      switch (ChgT)
		{
		case FChgZero:    // zero type
		  floor->newspecial = 0;
		  floor->type |= floor_t::SetTxTy;
		  break;
		case FChgTyp:     // copy type
		  floor->newspecial = line->frontsector->special;
		  floor->type |= floor_t::SetTxTy;
		  break;
		case FChgTxt:     // leave type be
		  floor->type |= floor_t::SetTexture;
		default:
		  break;
		}
	    }
	}
      if (manual) return rtn;
    }
  return rtn;
}


//
// was EV_DoGenCeiling()
//
// Handle generalized ceiling types
//
// Passed the linedef activating the ceiling function
// Returns true if a thinker created
//
int Map::EV_DoGenCeiling(line_t *line)
{
  int                   secnum;
  int                   rtn;
  bool               manual;
  fixed_t               targheight;
  sector_t*             sec;
  ceiling_t*            ceiling;
  unsigned              value = (unsigned)line->special - GenCeilingBase;

  // parse the bit fields in the line's special type

  int Crsh = (value & CeilingCrush) >> CeilingCrushShift;
  int ChgT = (value & CeilingChange) >> CeilingChangeShift;
  int Targ = (value & CeilingTarget) >> CeilingTargetShift;
  int Dirn = (value & CeilingDirection) >> CeilingDirectionShift;
  int ChgM = (value & CeilingModel) >> CeilingModelShift;
  int Sped = (value & CeilingSpeed) >> CeilingSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_ceiling;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_ceiling:                
      // Do not start another function if ceiling already moving
      if (P_SectorActive(ceiling_special,sec))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}

      fixed_t speed;
      // set speed of motion
      switch (Sped)
	{
	case SpeedNormal:
	  speed = CEILSPEED*2;
	  break;
	case SpeedFast:
	  speed = CEILSPEED*4;
	  break;
	case SpeedTurbo:
	  speed = CEILSPEED*8;
	  break;
	case SpeedSlow:
	default:
	  speed = CEILSPEED;
	  break;
	}

      // new ceiling thinker
      rtn++;
      ceiling = new ceiling_t(ceiling_t::RelHeight, sec, speed, speed, Crsh ? 10 : 0, 0);
      AddThinker(ceiling);

      ceiling->direction = Dirn? 1 : -1;
      ceiling->texture = sec->ceilingpic;
      ceiling->newspecial = sec->special;

      // set destination target height
      targheight = sec->ceilingheight;
      switch(Targ)
	{
	case CtoHnC:
	  targheight = P_FindHighestCeilingSurrounding(sec);
	  break;
	case CtoLnC:
	  targheight = P_FindLowestCeilingSurrounding(sec);
	  break;
	case CtoNnC:
	  targheight = Dirn?
	    P_FindNextHighestCeiling(sec,sec->ceilingheight) :
	      P_FindNextLowestCeiling(sec,sec->ceilingheight);
	  break;
	case CtoHnF:
	  targheight = P_FindHighestFloorSurrounding(sec);
	  break;
	case CtoF:
	  targheight = sec->floorheight;
	  break;
	case CbyST:
	  targheight = (ceiling->sector->ceilingheight>>FRACBITS) +
	    ceiling->direction * (FindShortestUpperAround(sec)>>FRACBITS);
	  if (targheight>32000)
	    targheight=32000;
	  if (targheight<-32000)
	    targheight=-32000;
	  targheight<<=FRACBITS;
	  break;
	case Cby24:
	  targheight = ceiling->sector->ceilingheight +
	    ceiling->direction * 24*FRACUNIT;
	  break;
	case Cby32:
	  targheight = ceiling->sector->ceilingheight +
	    ceiling->direction * 32*FRACUNIT;
	  break;
	default:
	  break;
	}
      //that doesn't compile under windows
      //Dirn? ceiling->topheight : ceiling->bottomheight = targheight;
      if(Dirn)
	ceiling->topheight = targheight;
      else
	ceiling->bottomheight = targheight;

      // set texture/type change properties
      if (ChgT)     // if a texture change is indicated
	{
	  if (ChgM)   // if a numeric model change
	    {
	      sector_t *sec2;

	      sec2 = (Targ==CtoHnF || Targ==CtoF)?         
		FindModelFloorSector(targheight, sec) :
		FindModelCeilingSector(targheight, sec);
	      if (sec2)
		{
		  ceiling->texture = sec2->ceilingpic;
		  switch (ChgT)
		    {
		    case CChgZero:  // type is zeroed
		      ceiling->newspecial = 0;
		      ceiling->type |= ceiling_t::SetTxTy;
		      break;
		    case CChgTyp:   // type is copied
		      ceiling->newspecial = sec2->special;
		      ceiling->type |= ceiling_t::SetTxTy;
		      break;
		    case CChgTxt:   // type is left alone
		      ceiling->type |= ceiling_t::SetTexture;
		      break;
		    default:
		      break;
		    }
		}
	    }
	  else        // else if a trigger model change
	    {
	      ceiling->texture = line->frontsector->ceilingpic;
	      switch (ChgT)
		{
		case CChgZero:    // type is zeroed
		  ceiling->newspecial = 0;
		  ceiling->type |= ceiling_t::SetTxTy;
		  break;
		case CChgTyp:     // type is copied
		  ceiling->newspecial = line->frontsector->special;
		  ceiling->type |= ceiling_t::SetTxTy;
		  break;
		case CChgTxt:     // type is left alone
		  ceiling->type |= ceiling_t::SetTexture;
		  break;
		default:
		  break;
		}
	    }
	}
      AddActiveCeiling(ceiling); // add this ceiling to the active list
      if (manual) return rtn;
    }
  return rtn;
}

//
// was EV_DoGenLift()
//
// Handle generalized lift types
//
// Passed the linedef activating the lift
// Returns true if a thinker is created
//
int Map::EV_DoGenLift(line_t *line)
{
  plat_t*         plat;
  int             secnum;
  int             rtn;
  bool         manual;
  sector_t*       sec;
  unsigned        value = (unsigned)line->special - GenLiftBase;

  // parse the bit fields in the line's special type

  int Targ = (value & LiftTarget) >> LiftTargetShift;
  int Dely = (value & LiftDelay) >> LiftDelayShift;
  int Sped = (value & LiftSpeed) >> LiftSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  secnum = -1;
  rtn = 0;

  // Activate all <type> plats that are in_stasis

  if (Targ==LnF2HnF)
    ActivateInStasisPlat(line->tag);
        
  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_lift;
    }

  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_lift:
      // Do not start another function if floor already moving
      if (P_SectorActive(floor_special,sec))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
      
      int type;
      // setup the target destination height
      switch(Targ)
	{
	case F2LnF:
	  type = plat_t::LnF;
	  break;
	case F2NnF:
	  type = plat_t::NLnF;
	  break;
	case F2LnC:
	  type = plat_t::LnC;
	  break;
	case LnF2HnF:
	default:
	  type = plat_t::LHF;
	  break;
	}

      fixed_t speed;
      // setup the speed of motion
      switch(Sped)
	{
	case SpeedNormal:
	  speed = PLATSPEED * 4;
	  break;
	case SpeedFast:
	  speed = PLATSPEED * 8;
	  break;
	case SpeedTurbo:
	  speed = PLATSPEED * 16;
	  break;
	case SpeedSlow:
	default:
	  speed = PLATSPEED * 2;
	  break;
	}

      int wait;
      // setup the delay time before the floor returns
      switch(Dely)
	{
	case 0:
	  wait = 1*35;
	  break;
	case 1:
	  wait = PLATWAIT*35;
	  break;
	case 2:
	  wait = 5*35;
	  break;
	case 3:
	default:
	  wait = 10*35;
	  break;
	}

      // Setup the plat thinker
      rtn++;
      plat = new plat_t(type, sec, line->tag, speed, wait, 0);
      AddThinker(plat);
      AddActivePlat(plat); // add this plat to the list of active plats

      if (manual)
	return rtn;
    }
  return rtn;
}

//
// EV_DoGenStairs()
//
// Handle generalized stair building
//
// Passed the linedef activating the stairs
// Returns true if a thinker is created
//
int Map::EV_DoGenStairs(line_t *line)
{
  int       secnum;
  int       i;
  sector_t *sec;
    
  unsigned value = (unsigned)line->special - GenStairsBase;

  // parse the bit fields in the line's special type

  int Igno = (value & StairIgnore) >> StairIgnoreShift;
  int Dirn = (value & StairDirection) >> StairDirectionShift;
  int Step = (value & StairStep) >> StairStepShift;
  int Sped = (value & StairSpeed) >> StairSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  int rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  bool manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_stair;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_stair:          
      //Do not start another function if floor already moving
      //Add special lockout condition to wait for entire
      //staircase to build before retriggering
      if (P_SectorActive(floor_special,sec) || sec->stairlock)
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
      
      fixed_t speed;
      // setup speed of stair building
      switch(Sped)
	{
	default:
	case SpeedSlow:
	  speed = FLOORSPEED/4;
	  break;
	case SpeedNormal:
	  speed = FLOORSPEED/2;
	  break;
	case SpeedFast:
	  speed = FLOORSPEED*2;
	  break;
	case SpeedTurbo:
	  speed = FLOORSPEED*4;
	  break;
	}

      fixed_t stairsize;
      // setup stepsize for stairs
      switch(Step)
	{
	default:
	case 0:
	  stairsize = 4*FRACUNIT;
	  break;
	case 1:
	  stairsize = 8*FRACUNIT;
	  break;
	case 2:
	  stairsize = 16*FRACUNIT;
	  break;
	case 3:
	  stairsize = 24*FRACUNIT;
	  break;
	}

      fixed_t height = sec->floorheight + (Dirn ? 1 : -1) * stairsize;

      // new floor thinker
      rtn++;
      floor_t *floor = new floor_t(floor_t::AbsHeight, sec, speed, 0, height);
      AddThinker(floor);

      int texture = sec->floorpic;

      sec->stairlock = -2;
      sec->nextsec = -1;
      sec->prevsec = -1;

      int osecnum = secnum;
      // Find next sector to raise
      // 1.     Find 2-sided line with same sector side[0]
      // 2.     Other side is the next sector to raise
      bool ok;
      do
	{
	  ok = false;
	  for (i = 0;i < sec->linecount;i++)
	    {
	      if ( !((sec->lines[i])->backsector) )
		continue;
                                  
	      sector_t *tsec = (sec->lines[i])->frontsector;
	      int newsecnum = tsec-sectors;
          
	      if (secnum != newsecnum)
		continue;

	      tsec = (sec->lines[i])->backsector;
	      newsecnum = tsec - sectors;

	      if (!Igno && tsec->floorpic != texture)
		continue;

	      if (P_SectorActive(floor_special,tsec) || tsec->stairlock)
		continue;

	      height += floor->direction * stairsize;

	      // link the stair chain in both directions
	      // lock the stair sector until building complete
	      sec->nextsec = newsecnum; // link step to next
	      tsec->prevsec = secnum;   // link next back
	      tsec->nextsec = -1;       // set next forward link as end
	      tsec->stairlock = -2;     // lock the step

	      sec = tsec;
	      secnum = newsecnum;

	      floor = new floor_t(floor_t::AbsHeight, sec, speed, 0, height);
	      AddThinker(floor);

	      //floor->direction = Dirn? 1 : -1;

	      ok = true;
	      break;
	    }
	} while(ok);
      if (manual)
        return rtn;
      secnum = osecnum;
    }
  // retriggerable generalized stairs build up or down alternately
  if (rtn)
    line->special ^= StairDirection; // alternate dir on succ activations
  return rtn;
}

//
// EV_DoGenCrusher()
//
// Handle generalized crusher types
//
// Passed the linedef activating the crusher
// Returns true if a thinker created
//
int Map::EV_DoGenCrusher(line_t *line)
{
  int                   secnum;
  int                   rtn;
  bool               manual;
  sector_t*             sec;
  ceiling_t*            ceiling;
  unsigned              value = (unsigned)line->special - GenCrusherBase;

  // parse the bit fields in the line's special type

  int Slnt = (value & CrusherSilent) >> CrusherSilentShift;
  int Sped = (value & CrusherSpeed) >> CrusherSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  rtn = ActivateInStasisCeiling(line->tag);

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_crusher;
    }

  secnum = -1;
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

    manual_crusher:                
      // Do not start another function if ceiling already moving
      if (P_SectorActive(ceiling_special,sec))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}

      int speed;
      // setup ceiling motion speed
      switch (Sped)
	{
	case SpeedNormal:
	  speed = CEILSPEED*2;
	  break;
	case SpeedFast:
	  speed = CEILSPEED*4;
	  break;
	case SpeedTurbo:
	  speed = CEILSPEED*8;
	  break;
	default:
	case SpeedSlow:
	  speed = CEILSPEED;
	  break;
	}

      // new ceiling thinker
      rtn = 1;
      ceiling = new ceiling_t(ceiling_t::Crusher, sec, speed, speed, 10, 8*FRACUNIT);
      AddThinker(ceiling);

      ceiling->texture = sec->ceilingpic;
      ceiling->newspecial = sec->special;
      if (Slnt)
	ceiling->type |= ceiling_t::Silent;

      AddActiveCeiling(ceiling);  // add to list of active ceilings
      if (manual) return rtn;
    }
  return rtn;
}

//
// was EV_DoGenLockedDoor()
//
// Handle generalized locked door types
//
// Passed the linedef activating the generalized locked door
// Returns true if a thinker created
//
int Map::EV_DoGenLockedDoor(line_t *line)
{
  int   secnum,rtn;
  sector_t* sec;
  vdoor_t* door;
  bool manual;
  unsigned  value = (unsigned)line->special - GenLockedBase;

  // parse the bit fields in the line's special type

  int Kind = (value & LockedKind) >> LockedKindShift;
  int Sped = (value & LockedSpeed) >> LockedSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_locked;
    }

  secnum = -1;
  rtn = 0;
  
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
    manual_locked:
      // Do not start another function if ceiling already moving
      if (P_SectorActive(ceiling_special,sec))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
  
      // new door thinker
      rtn = 1;
      door = new vdoor_t(vdoor_t::OwC, sec, 0, VDOORWAIT, line);
      AddThinker(door);

      door->topheight = P_FindLowestCeilingSurrounding(sec);
      door->topheight -= 4*FRACUNIT;
      door->direction = 1;

      door->type = Kind? vdoor_t::Open : vdoor_t::OwC;
      // setup speed of door motion
      switch(Sped)
	{
	default:
	case SpeedSlow:
	  door->speed = VDOORSPEED;
	  break;
	case SpeedNormal:
	  door->speed = VDOORSPEED*2;
	  break;
	case SpeedFast:
	  door->type |= vdoor_t::Blazing;
	  door->speed = VDOORSPEED*4;
	  break;
	case SpeedTurbo:
	  door->type |= vdoor_t::Blazing;
	  door->speed = VDOORSPEED*8;

	  break;
	}

      // killough 4/15/98: fix generalized door opening sounds
      // (previously they always had the blazing door close sound)

      S_StartSound(&door->sector->soundorg,   // killough 4/15/98
		   door->speed >= VDOORSPEED*4 ? sfx_bdopn : sfx_doropn);

      if (manual)
	return rtn;
    }
  return rtn;
}

//
// EV_DoGenDoor()
//
// Handle generalized door types
//
// Passed the linedef activating the generalized door
// Returns true if a thinker created
//
int Map::EV_DoGenDoor(line_t *line)
{
  int   secnum,rtn;
  sector_t* sec;
  bool   manual;
  vdoor_t* door;
  unsigned  value = (unsigned)line->special - GenDoorBase;

  // parse the bit fields in the line's special type

  int Dely = (value & DoorDelay) >> DoorDelayShift;
  int Kind = (value & DoorKind) >> DoorKindShift;
  int Sped = (value & DoorSpeed) >> DoorSpeedShift;
  int Trig = (value & TriggerType) >> TriggerTypeShift;

  rtn = 0;

  // check if a manual trigger, if so do just the sector on the backside
  manual = false;
  if (Trig==PushOnce || Trig==PushMany)
    {
      if (!(sec = line->backsector))
	return rtn;
      secnum = sec-sectors;
      manual = true;
      goto manual_door;
    }


  secnum = -1;
  rtn = 0;
  
  // if not manual do all sectors tagged the same as the line
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
    manual_door:
      // Do not start another function if ceiling already moving
      if (P_SectorActive(ceiling_special,sec))
	{
	  if (!manual)
	    continue;
	  else
	    return rtn;
	}
  
      // new door thinker
      rtn = 1;
      door = new vdoor_t(vdoor_t::Delayed, sec, 0, VDOORWAIT, line);
      AddThinker(door);

      // setup delay for door remaining open/closed
      switch(Dely)
	{
	default:
	case 0:
	  door->topwait = 35;
	  break;
	case 1:
	  door->topwait = VDOORWAIT;
	  break;
	case 2:
	  door->topwait = 2*VDOORWAIT;
	  break;
	case 3:
	  door->topwait = 7*VDOORWAIT;
	  break;
	}

      // setup speed of door motion
      switch(Sped)
	{
	default:
	case SpeedSlow:
	  door->speed = VDOORSPEED;
	  break;
	case SpeedNormal:
	  door->speed = VDOORSPEED*2;
	  break;
	case SpeedFast:
	  door->speed = VDOORSPEED*4;
	  break;
	case SpeedTurbo:
	  door->speed = VDOORSPEED*8;
	  break;
	}

      // set kind of door, whether it opens then close, opens, closes etc.
      // assign target heights accordingly
      switch(Kind)
	{
	case OdCDoor:
	  door->direction = 1;
	  door->topheight = P_FindLowestCeilingSurrounding(sec);
	  door->topheight -= 4*FRACUNIT;
	  if (door->topheight != sec->ceilingheight)
	    S_StartSound(&door->sector->soundorg,sfx_bdopn);
	  door->type = vdoor_t::OwC;
	  break;
	case ODoor:
	  door->direction = 1;
	  door->topheight = P_FindLowestCeilingSurrounding(sec);
	  door->topheight -= 4*FRACUNIT;
	  if (door->topheight != sec->ceilingheight)
	    S_StartSound(&door->sector->soundorg,sfx_bdopn);
	  door->type = vdoor_t::Open;
	  break;
	case CdODoor:
	  door->topheight = sec->ceilingheight;
	  door->direction = -1;
	  S_StartSound(&door->sector->soundorg,sfx_dorcls);
	  door->type = vdoor_t::CwO;
	  break;
	case CDoor:
	  door->topheight = P_FindLowestCeilingSurrounding(sec);
	  door->topheight -= 4*FRACUNIT;
	  door->direction = -1;
	  S_StartSound(&door->sector->soundorg,sfx_dorcls);
	  door->type = vdoor_t::Close;
	  break;
	default:
	  break;
	}

      if (Sped >= SpeedFast)
	door->type |= vdoor_t::Blazing;

      if (manual)
	return rtn;
    }
  return rtn;
}
