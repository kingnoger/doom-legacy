// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
// $Log$
// Revision 1.4  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.3  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:17:56  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "doomdata.h"

#include "p_spec.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"
#include "g_game.h"

#include "g_map.h"

// ==========================================================================
//                              FLOORS
// ==========================================================================

// was T_MovePlane
// Move a plane (floor or ceiling) and check for crushing
//
//SoM: I had to copy the entire function from Boom because it was causing errors.
int Map::T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest,
		     int crush, int floorOrCeiling, int direction)
{
  bool     flag;
  fixed_t  lastpos;     
  fixed_t  destheight; //jff 02/04/98 used to keep floors/ceilings
  // from moving thru each other

  switch (floorOrCeiling)
    {
    case 0:
      // Moving a floor
      switch(direction)
	{
        case -1:
          //SoM: 3/20/2000: Make splash when platform floor hits water
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->floorheight - speed) < sectors[sector->heightsec].floorheight
		 && sector->floorheight > sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }
          // Moving a floor down
          if (sector->floorheight - speed < dest)
	    {
	      lastpos = sector->floorheight;
	      sector->floorheight = dest;
	      flag = CheckSector(sector,crush);
	      if (flag == true && sector->numattached)                   
		{
		  sector->floorheight =lastpos;
		  CheckSector(sector,crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      lastpos = sector->floorheight;
	      sector->floorheight -= speed;
	      flag = CheckSector(sector,crush);
	      if(flag == true && sector->numattached)
		{
		  sector->floorheight = lastpos;
		  CheckSector(sector, crush);
		  return res_crushed;
		}
	    }
          break;
                                                
        case 1:
          // Moving a floor up
          // keep floor from moving thru ceilings
          //SoM: 3/20/2000: Make splash when platform floor hits water
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->floorheight + speed) > sectors[sector->heightsec].floorheight
		 && sector->floorheight < sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }
          destheight = (!boomsupport || dest<sector->ceilingheight)?
	    dest : sector->ceilingheight;
          if (sector->floorheight + speed > destheight)
	    {
	      lastpos = sector->floorheight;
	      sector->floorheight = destheight;
	      flag = CheckSector(sector,crush);
	      if (flag == true)
		{
		  sector->floorheight = lastpos;
		  CheckSector(sector,crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      // crushing is possible
	      lastpos = sector->floorheight;
	      sector->floorheight += speed;
	      flag = CheckSector(sector,crush);
	      if (flag == true)
		{
		  if (!boomsupport)
		    {
		      if (crush == true)
			return res_crushed;
		    }
		  sector->floorheight = lastpos;
		  CheckSector(sector,crush);
		  return res_crushed;
		}
	    }
          break;
	}
      break;
                                                                        
    case 1:
      // moving a ceiling
      switch(direction)
	{
        case -1:
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->ceilingheight - speed) < sectors[sector->heightsec].floorheight
		 && sector->ceilingheight > sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }
          // moving a ceiling down
          // keep ceiling from moving thru floors
          destheight = (!boomsupport || dest>sector->floorheight)?
	    dest : sector->floorheight;
          if (sector->ceilingheight - speed < destheight)
	    {
	      lastpos = sector->ceilingheight;
	      sector->ceilingheight = destheight;
	      flag = CheckSector(sector,crush);

	      if (flag == true)
		{
		  sector->ceilingheight = lastpos;
		  CheckSector(sector,crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      // crushing is possible
	      lastpos = sector->ceilingheight;
	      sector->ceilingheight -= speed;
	      flag = CheckSector(sector,crush);

	      if (flag == true)
		{
		  if (crush == true)
		    return res_crushed;
		  sector->ceilingheight = lastpos;
		  CheckSector(sector,crush);
		  return res_crushed;
		}
	    }
          break;
                                                
        case 1:
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->ceilingheight + speed) > sectors[sector->heightsec].floorheight
		 && sector->ceilingheight < sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }
          // moving a ceiling up
          if (sector->ceilingheight + speed > dest)
	    {
	      lastpos = sector->ceilingheight;
	      sector->ceilingheight = dest;
	      flag = CheckSector(sector,crush);
	      if (flag == true && sector->numattached)
		{
		  sector->ceilingheight = lastpos;
		  CheckSector(sector,crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      lastpos = sector->ceilingheight;
	      sector->ceilingheight += speed;
	      flag = CheckSector(sector,crush);
	      if (flag == true && sector->numattached)
		{
		  sector->ceilingheight = lastpos;
		  CheckSector(sector,crush);
		  return res_crushed;
		}
	    }
          break;
	}
      break;
    }
  return res_ok;
}


//=================================
// constructor
floor_t::floor_t(int ty, sector_t *sec, fixed_t sp, int cru, fixed_t height)
{
  type = ty;
  sector = sec;
  crush = cru;
  speed = sp;
  sec->floordata = this;

  switch (ty & TMASK)
    {
    case RelHeight:
      destheight = sec->floorheight + height;
      direction = (height > 0) ? 1 : -1;
      break;

    case AbsHeight:
      destheight = height;
      direction = (sec->floorheight <= height) ? 1 : -1;
      break;

    case Ceiling:
      destheight = sec->ceilingheight + height;
      direction = 1;
      break;

    case LnF:
      destheight = P_FindLowestFloorSurrounding(sec) + height;
      direction = -1;
      break;

    case UpNnF:
      destheight = P_FindNextHighestFloor(sec, sec->floorheight) + height;
      direction = 1;
      break;

    case DownNnF:
      destheight = P_FindNextLowestFloor(sec, sec->floorheight) + height;
      direction = -1;
      break;

    case HnF:
      destheight = P_FindHighestFloorSurrounding(sec) + height;
      direction = -1; // TODO up/down?
      break;

    case LnC:
      destheight = P_FindLowestCeilingSurrounding(sec);
      if (destheight > sec->ceilingheight)
	destheight = sec->ceilingheight;
      destheight += height;
      direction = 1; // TODO up/down?
      break;

    case SLT:
      destheight = sector->floorheight + mp->FindShortestLowerAround(sec) + height;
      if (boomsupport && destheight > (32000 << FRACBITS))
	destheight = 32000 << FRACBITS; //jff 3/13/98 do not allow height overflow
      direction = 1;
      break;

    default:
      I_Error("Unknown floor target %d!\n", ty);
      break;
    }

  //SN_StartSequence((mobj_t *)&sector->soundorg, SEQ_PLATFORM+floor->sector->seqType);
}


// was T_MoveFloor
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void floor_t::Think()
{
  int res = mp->T_MovePlane(sector, speed, destheight, crush, 0, direction);

  if (!(mp->maptic % (8*NEWTICRATERATIO)))
    S_StartSound(&sector->soundorg, ceiling_t::ceilmovesound);

  if (res == res_pastdest)
    {
      if (type & SetTexture)
	sector->floorpic = texture;

      if (type & SetSpecial)
	{
	  sector->special = newspecial;
	  sector->oldspecial = oldspecial;
	}

      // TODO replace with sound sequence
      //if ((type == buildStair && game.mode == gm_heretic) || game.mode != gm_heretic)
      S_StartSound(&sector->soundorg, sfx_pstop);
      //SN_StopSequence((mobj_t *)&floor->sector->soundorg);
      sector->floordata = NULL; // Clear up the thinker so others can use it
      mp->TagFinished(sector->tag);
      mp->RemoveThinker(this);  // unlink and free

      // SoM: This code locks out stair steps while generic, retriggerable generic stairs
      // are building.
      
      if (sector->stairlock==-2) // if this sector is stairlocked
        {
          sector_t *sec = sector;
          sec->stairlock=-1;              // thinker done, promote lock to -1

          while (sec->prevsec != -1 && mp->sectors[sec->prevsec].stairlock!=-2)
            sec = &mp->sectors[sec->prevsec]; // search for a non-done thinker
          if (sec->prevsec==-1)           // if all thinkers previous are done
	    {
	      sec = sector;          // search forward
	      while (sec->nextsec!=-1 && mp->sectors[sec->nextsec].stairlock!=-2) 
		sec = &mp->sectors[sec->nextsec];
	      if (sec->nextsec==-1)         // if all thinkers ahead are done too
		{
		  while (sec->prevsec!=-1)    // clear all locks
		    {
		      sec->stairlock = 0;
		      sec = &mp->sectors[sec->prevsec];
		    }
		  sec->stairlock = 0;
		}
	    }
        }
    }
}




// was EV_DoFloor
// HANDLE FLOOR TYPES
//
int Map::EV_DoFloor(line_t *line, int type, fixed_t speed, int crush, fixed_t height)
{
  int  secnum = -1;
  int  rtn = 0;

  while ((secnum = FindSectorFromLineTag(line, secnum)) >= 0)
    {
      sector_t  *sec = &sectors[secnum];
        
      // Don't start a second thinker on the same floor
      if (P_SectorActive(floor_special, sec)) //jff 2/23/98
	continue;

      // new floor thinker
      rtn++;
      floor_t *floor = new floor_t(type, sec, speed, crush, height);
      AddThinker(floor);

      if (type & floor_t::SetTxTy) // either one
	{
	  // This is a bit crude but works for Doom.
	  // Boom extended linedefs do this for themselves in p_genlin.cpp
	  if (type & floor_t::NumericModel)
	    {
	      floor->texture = sec->floorpic;
	      // jff 1/24/98 make sure newspecial gets initialized
	      // in case no surrounding sector is at destheight
	      // --> should not affect compatibility <--
	      floor->newspecial = sec->special;
	      floor->oldspecial = sec->oldspecial;
	      //jff 5/23/98 use model subroutine to unify fixes and handling
	      // BP: heretic have change something here
	      sec = FindModelFloorSector(floor->destheight, sec);
	      if (sec)
		{
		  floor->texture = sec->floorpic;
		  floor->newspecial = sec->special;
		  floor->oldspecial = sec->oldspecial;
		}
	    }
	  else
	    {
	      // "trigger model"
	      floor->texture = line->frontsector->floorpic;
	      floor->newspecial = line->frontsector->special;
	      floor->oldspecial = line->frontsector->oldspecial;
	    }
	}
    }
  return rtn;
}


// SoM: 3/6/2000: Function for chaning just the floor texture and type.
//
// Handle pure change types. These change floor texture and sector type
// by trigger or numeric model without moving the floor (no Thinker needed)
//
// The linedef causing the change and the type of change is passed
// Returns true if any sector changes
//
int Map::EV_DoChange(line_t *line, int changetype)
{
  int secnum = -1;
  int rtn = 0;

  // change all sectors with the same tag as the linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      sector_t *secm;
      rtn++;

      // handle trigger or numeric change type
      switch(changetype)
	{
	case trigChangeOnly:
	  sec->floorpic = line->frontsector->floorpic;
	  sec->special = line->frontsector->special;
	  sec->oldspecial = line->frontsector->oldspecial;
	  break;
	case numChangeOnly:
	  secm = FindModelFloorSector(sec->floorheight, sec);
	  if (secm) // if no model, no change
	    {
	      sec->floorpic = secm->floorpic;
	      sec->special = secm->special;
	      sec->oldspecial = secm->oldspecial;
	    }
	  break;
	default:
	  break;
	}
    }
  return rtn;
}


// was EV_BuildStairs
// BUILD A STAIRCASE!
//

// SoM: 3/6/2000: Use the Boom version of this function.
int Map::EV_BuildStairs(line_t *line, int type, fixed_t speed, fixed_t stepsize, int crush)
{
  // TODO Hexen compatibility (a new stairbuilding method...)
  int                   i;

  int secnum = -1;
  int rtn = 0;

  // start a stair at each sector tagged the same as the linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      
      // don't start a stair if the first step's floor is already moving
      if (P_SectorActive(floor_special,sec))
	continue;

      fixed_t height = sec->floorheight + stepsize;      
      // create new floor thinker for first step
      rtn++;
      floor_t *floor = new floor_t(floor_t::AbsHeight, sec, speed, crush, height);
      AddThinker(floor);

      int texture = sec->floorpic;
      int osecnum = secnum;           //jff 3/4/98 preserve loop index
      
      // Find next sector to raise
      //   1. Find 2-sided line with same sector side[0] (lowest numbered)
      //   2. Other side is the next sector to raise
      //   3. Unless already moving, or different texture, then stop building
      bool ok;
      do
	{
	  ok = false;
	  for (i = 0;i < sec->linecount;i++)
	    {
	      if ( !((sec->lines[i])->flags & ML_TWOSIDED) )
		continue;
                                  
	      sector_t *tsec = (sec->lines[i])->frontsector;
	      int newsecnum = tsec-sectors;
          
	      if (secnum != newsecnum)
		continue;

	      tsec = (sec->lines[i])->backsector;
	      if (!tsec) continue;     //jff 5/7/98 if no backside, continue
	      newsecnum = tsec - sectors;

	      // if sector's floor is different texture, look for another
	      if (tsec->floorpic != texture)
		continue;

	      // if sector's floor already moving, look for another
	      if (P_SectorActive(floor_special,tsec)) //jff 2/22/98
		continue;
                                  
	      height += stepsize;
	      sec = tsec;
	      secnum = newsecnum;

	      // create and initialize a thinker for the next step
	      floor = new floor_t(floor_t::AbsHeight, sec, speed, crush, height);
	      AddThinker(floor);

	      ok = true;
	      break;
	    }
	} while(ok);      // continue until no next step is found
      secnum = osecnum; //jff 3/4/98 restore loop index
    }
  return rtn;
}


//SoM: 3/6/2000: boom donut function
//
// was EV_DoDonut()
//
// Handle donut function: lower pillar, raise surrounding pool, both to height,
// texture and type of the sector surrounding the pool.
//
// Passed the linedef that triggered the donut
// Returns whether a thinker was created
//
int Map::EV_DoDonut(line_t *line)
{
  int       i;


  int secnum = -1;
  int rtn = 0;
  // do function on all sectors with same tag as linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sector_t *s1 = &sectors[secnum];  // s1 is pillar's sector
              
      // do not start the donut if the pillar is already moving
      if (P_SectorActive(floor_special, s1))
	continue;
                      
      sector_t *s2 = getNextSector(s1->lines[0], s1);  // s2 is pool's sector
      if (!s2)
	continue; // note lowest numbered line around
      // pillar must be two-sided 

      // do not start the donut if the pool is already moving
      if (boomsupport && P_SectorActive(floor_special, s2)) 
	continue;
                      
      // find a two sided line around the pool whose other side isn't the pillar
      for (i = 0;i < s2->linecount;i++)
	{
	  //jff 3/29/98 use true two-sidedness, not the flag
	  // killough 4/5/98: changed demo_compatibility to compatibility
	  if (!boomsupport)
	    {
	      if ((!s2->lines[i]->flags & ML_TWOSIDED) ||
		  (s2->lines[i]->backsector == s1))
		continue;
	    }
	  else if (!s2->lines[i]->backsector || s2->lines[i]->backsector == s1)
	    continue;

	  rtn++; //jff 1/26/98 no donut action - no switch change on return

	  sector_t *s3 = s2->lines[i]->backsector; // s3 is model sector for changes
        
	  //  Spawn rising slime
	  floor_t *floor = new floor_t(floor_t::AbsHeight | floor_t::SetTxTy, s2, FLOORSPEED/2, 0, s3->floorheight);
	  AddThinker(floor);
	  floor->texture = s3->floorpic;
	  floor->newspecial = 0;

	  //  Spawn lowering donut-hole pillar
	  floor = new floor_t(floor_t::AbsHeight, s1, FLOORSPEED/2, 0, s3->floorheight);
	  AddThinker(floor);
	  break;
	}
    }
  return rtn;
}


//==============================
// constructor
elevator_t::elevator_t(int ty, sector_t *sec, line_t *line)
{
  type = ty;
  sector = sec;
  speed = ELEVATORSPEED;
  sec->floordata = this;
  sec->ceilingdata = this;

  // set up the fields according to the type of elevator action
  switch (ty)
    {
      // elevator down to next floor
    case elevateDown:
      direction = -1;
      floordestheight =
	P_FindNextLowestFloor(sec,sec->floorheight);
      ceilingdestheight =
	floordestheight + sec->ceilingheight - sec->floorheight;
      break;

      // elevator up to next floor
    case elevateUp:
      direction = 1;
      floordestheight =
	P_FindNextHighestFloor(sec,sec->floorheight);
      ceilingdestheight =
	floordestheight + sec->ceilingheight - sec->floorheight;
      break;

      // elevator to floor height of activating switch's front sector
    case elevateCurrent:
      floordestheight = line->frontsector->floorheight;
      ceilingdestheight =
	floordestheight + sec->ceilingheight - sec->floorheight;
      direction =
	floordestheight > sec->floorheight ?  1 : -1;
      break;

    default:
      break;
    }
}


// SoM: 3/6/2000: Lots'o'copied code here.. Elevators.
//
// was T_MoveElevator()
//
// Move an elevator to it's destination (up or down)
// Called once per tick for each moving floor.
//
// Passed an elevator_t structure that contains all pertinent info about the
// move. See P_SPEC.H for fields.
// No return.
//
// SoM: 3/6/2000: The function moves the plane differently based on direction, so if it's 
// traveling really fast, the floor and ceiling won't hit each other and stop the lift.
void elevator_t::Think()
{
  int res;

  if (direction<0)      // moving down
    {
      //jff 4/7/98 reverse order of ceiling/floor
      res = mp->T_MovePlane(sector, speed, ceilingdestheight, 0, 1, direction); // move floor
      if (res == res_ok || res == res_pastdest) // jff 4/7/98 don't move ceil if blocked
	mp->T_MovePlane(sector, speed, floordestheight, 0, 0, direction);// move ceiling
    }
  else // up
    {
      //jff 4/7/98 reverse order of ceiling/floor
      res = mp->T_MovePlane(sector,speed,floordestheight,0,0,direction); // move ceiling
      // jff 4/7/98 don't move floor if blocked
      if (res == res_ok || res == res_pastdest) 
	mp->T_MovePlane(sector, speed, ceilingdestheight, 0, 1, direction); // move floor
    }

  // make floor move sound
  if (!(mp->maptic % (8*NEWTICRATERATIO)))
    S_StartSound(&sector->soundorg, sfx_stnmov);
    
  if (res == res_pastdest)            // if destination height acheived
    {
      sector->floordata = NULL;     //jff 2/22/98
      sector->ceilingdata = NULL;   //jff 2/22/98
      mp->RemoveThinker(this);  // unlink and free

      // make floor stop sound
      S_StartSound(&sector->soundorg, sfx_pstop);
    }
}


// SoM: Boom elevator support.
//
// was EV_DoElevator
//
// Handle elevator linedef types
//
// Passed the linedef that triggered the elevator and the elevator action
//
// jff 2/22/98 new type to move floor and ceiling in parallel
//
int Map::EV_DoElevator(line_t* line, int type)
{
  int                   secnum;
  int                   rtn;
  sector_t*             sec;
  elevator_t*           elevator;

  secnum = -1;
  rtn = 0;
  // act on all sectors with the same tag as the triggering linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
              
      // If either floor or ceiling is already activated, skip it
      if (sec->floordata || sec->ceilingdata) //jff 2/22/98
	continue;
      
      // create and initialize new elevator thinker
      rtn = 1;
      elevator = new elevator_t(type, sec, line);
      AddThinker(elevator);
    }
  return rtn;
}
