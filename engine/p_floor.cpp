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
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:17:56  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.11  2002/09/20 22:41:29  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.9  2002/09/06 17:18:32  vberghol
// added most of the changes up to RC2
//
// Revision 1.8  2002/08/21 16:58:32  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.7  2002/08/17 21:21:46  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.6  2002/08/06 13:14:21  vberghol
// ...
//
// Revision 1.5  2002/07/23 19:21:40  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.4  2002/07/18 19:16:37  vberghol
// renamed a few files
//
// Revision 1.3  2002/07/01 21:00:16  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:12  vberghol
// Version 133 Experimental!
//
// Revision 1.12  2001/03/30 17:12:50  bpereira
// no message
//
// Revision 1.11  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.10  2000/11/02 17:50:07  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.9  2000/10/21 08:43:30  bpereira
// no message
//
// Revision 1.8  2000/09/28 20:57:16  bpereira
// no message
//
// Revision 1.7  2000/07/01 09:23:49  bpereira
// no message
//
// Revision 1.6  2000/05/23 15:22:34  stroggonmeth
// Not much. A graphic bug fixed.
//
// Revision 1.5  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.4  2000/04/08 17:29:24  stroggonmeth
// no message
//
// Revision 1.3  2000/04/04 00:32:46  stroggonmeth
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
result_e Map::T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest,
		     bool crush, int floorOrCeiling, int direction)
{
  bool       flag;
  fixed_t       lastpos;     
  fixed_t       destheight; //jff 02/04/98 used to keep floors/ceilings
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
	      return pastdest;
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
		  return crushed;
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
	      return pastdest;
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
			return crushed;
		    }
		  sector->floorheight = lastpos;
		  CheckSector(sector,crush);
		  return crushed;
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
	      return pastdest;
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
		    return crushed;
		  sector->ceilingheight = lastpos;
		  CheckSector(sector,crush);
		  return crushed;
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
	      return pastdest;
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
		  return crushed;
		}
	    }
          break;
	}
      break;
    }
  return ok;
}

// was T_MoveFloor
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void floormove_t::Think()
{
  result_e res = result_e(0);

  res = mp->T_MovePlane(sector, speed, floordestheight, crush, 0, direction);

  if (!(mp->maptic % (8*NEWTICRATERATIO)))
    S_StartSound(&sector->soundorg, ceiling_t::ceilmovesound);

  if (res == pastdest)
    {
      //sector->specialdata = NULL;
      if (direction == 1)
        {
	  switch(type)
            {
	    case donutRaise:
	      sector->special = newspecial;
	      sector->floorpic = texture;
	      break;
	    case genFloorChgT: //SoM: 3/6/2000: Add support for General types
	    case genFloorChg0:
	      sector->special = newspecial;
	      //SoM: 3/6/2000: this records the old special of the sector
	      sector->oldspecial = oldspecial;
	      // Don't break.
	    case genFloorChg:
	      sector->floorpic = texture;
	      break;
	    default:
	      break;
            }
        }
      else if (direction == -1)
        {
	  switch(type)
            {
	    case lowerAndChange:
	      sector->special = newspecial;
	      // SoM: 3/6/2000: Store old special type
	      sector->oldspecial = oldspecial;
	      sector->floorpic = texture;
	      break;
	    case genFloorChgT:
	    case genFloorChg0:
	      sector->special = newspecial;
	      sector->oldspecial = oldspecial;
	      // Don't break
	    case genFloorChg:
	      sector->floorpic = texture;
	      break;
	    default:
	      break;
            }
        }

      sector->floordata = NULL; // Clear up the thinker so others can use it
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

      if ((type == buildStair && game.mode == gm_heretic) || game.mode != gm_heretic)
	S_StartSound(&sector->soundorg, sfx_pstop);
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
  result_e res = result_e(0);

  if (direction<0)      // moving down
    {
      //jff 4/7/98 reverse order of ceiling/floor
      res = mp->T_MovePlane(sector, speed, ceilingdestheight, 0, 1, direction); // move floor
      if (res==ok || res==pastdest) // jff 4/7/98 don't move ceil if blocked
	mp->T_MovePlane(sector, speed, floordestheight, 0, 0, direction);// move ceiling
    }
  else // up
    {
      //jff 4/7/98 reverse order of ceiling/floor
      res = mp->T_MovePlane(sector,speed,floordestheight,0,0,direction); // move ceiling
      // jff 4/7/98 don't move floor if blocked
      if (res==ok || res==pastdest) 
	mp->T_MovePlane(sector, speed, ceilingdestheight, 0, 1, direction); // move floor
    }

  // make floor move sound
  if (!(mp->maptic % (8*NEWTICRATERATIO)))
    S_StartSound(&sector->soundorg, sfx_stnmov);
    
  if (res == pastdest)            // if destination height acheived
    {
      sector->floordata = NULL;     //jff 2/22/98
      sector->ceilingdata = NULL;   //jff 2/22/98
      mp->RemoveThinker(this);  // unlink and free

      // make floor stop sound
      S_StartSound(&sector->soundorg, sfx_pstop);
    }
}

// constructor

floormove_t::floormove_t(floor_e ty, sector_t *sec, line_t *line, int secnum)
{
  type = ty;
  sector = sec;
  crush = false;
  sec->floordata = this;

  switch (ty)
    {
    case lowerFloor:
      direction = -1;
      speed = FLOORSPEED;
      floordestheight = P_FindHighestFloorSurrounding(sec);
      break;

      //jff 02/03/30 support lowering floor by 24 absolute
    case lowerFloor24:
      direction = -1;
      speed = FLOORSPEED;
      floordestheight = sector->floorheight + 24 * FRACUNIT;
      break;

      //jff 02/03/30 support lowering floor by 32 absolute (fast)
    case lowerFloor32Turbo:
      direction = -1;
      speed = FLOORSPEED*4;
      floordestheight = sector->floorheight + 32 * FRACUNIT;
      break;

    case lowerFloorToLowest:
      direction = -1;
      speed = FLOORSPEED;
      floordestheight = P_FindLowestFloorSurrounding(sec);
      break;

      //jff 02/03/30 support lowering floor to next lowest floor
    case lowerFloorToNearest:
      direction = -1;
      speed = FLOORSPEED;
      floordestheight =
	P_FindNextLowestFloor(sec,sector->floorheight);
      break;

    case turboLower:
      direction = -1;
      speed = FLOORSPEED * 4;
      floordestheight = P_FindHighestFloorSurrounding(sec);
      if (floordestheight != sec->floorheight || game.mode == gm_heretic )
	floordestheight += 8*FRACUNIT;
      break;

    case raiseFloorCrush:
      crush = true;
    case raiseFloor:
      direction = 1;
      speed = FLOORSPEED;
      floordestheight = P_FindLowestCeilingSurrounding(sec);
      if (floordestheight > sec->ceilingheight)
	floordestheight = sec->ceilingheight;
      floordestheight -= (8*FRACUNIT)* (ty == raiseFloorCrush);
      break;

    case raiseFloorTurbo:
      direction = 1;
      speed = FLOORSPEED*4;
      floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
      break;

    case raiseFloorToNearest:
      direction = 1;
      speed = FLOORSPEED;
      floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
      break;

    case raiseFloor24:
      direction = 1;
      speed = FLOORSPEED;
      floordestheight = sector->floorheight + 24 * FRACUNIT;
      break;

      // SoM: 3/6/2000: support straight raise by 32 (fast)
    case raiseFloor32Turbo:
      direction = 1;
      speed = FLOORSPEED*4;
      floordestheight = sector->floorheight + 32 * FRACUNIT;
      break;

    case raiseFloor512:
      direction = 1;
      speed = FLOORSPEED;
      floordestheight = sector->floorheight + 512 * FRACUNIT;
      break;

    case raiseFloor24AndChange:
      direction = 1;
      speed = FLOORSPEED;
      floordestheight = sector->floorheight + 24 * FRACUNIT;
      sec->floorpic = line->frontsector->floorpic;
      sec->special = line->frontsector->special;
      sec->oldspecial = line->frontsector->oldspecial;
      break;

    case raiseToTexture:
      {
	int     minsize = MAXINT;

	if (boomsupport)
	  minsize = 32000<<FRACBITS; //SoM: 3/6/2000: ???
	direction = 1;
	speed = FLOORSPEED;
	for (int i = 0; i < sec->linecount; i++)
	  {
	    if (mp->twoSided(secnum, i))
	      {
		side_t *side = mp->getSide(secnum,i,0);
		// jff 8/14/98 don't scan texture 0, its not real
		if (side->bottomtexture > 0 ||
		    (!boomsupport && !side->bottomtexture))
		  if (textureheight[side->bottomtexture] < minsize)
		    minsize = textureheight[side->bottomtexture];
		side = mp->getSide(secnum,i,1);
		// jff 8/14/98 don't scan texture 0, its not real
		if (side->bottomtexture > 0 ||
		    (!boomsupport && !side->bottomtexture))
		  if (textureheight[side->bottomtexture] < minsize)
		    minsize = textureheight[side->bottomtexture];
	      }
	  }
	if (!boomsupport)
	  floordestheight = sector->floorheight + minsize;
	else
	  {
	    floordestheight =
	      (sector->floorheight>>FRACBITS) + (minsize>>FRACBITS);
	    if (floordestheight>32000)
	      floordestheight = 32000;        //jff 3/13/98 do not
	    floordestheight<<=FRACBITS;       // allow height overflow
	  }
	break;
      }
      //SoM: 3/6/2000: Boom changed allot of stuff I guess, and this was one of 'em 
    case lowerAndChange:
      direction = -1;
      speed = FLOORSPEED;
      floordestheight = P_FindLowestFloorSurrounding(sec);
      texture = sec->floorpic;

      // jff 1/24/98 make sure newspecial gets initialized
      // in case no surrounding sector is at floordestheight
      // --> should not affect compatibility <--
      newspecial = sec->special; 
      //jff 3/14/98 transfer both old and new special
      oldspecial = sec->oldspecial;
    
      //jff 5/23/98 use model subroutine to unify fixes and handling
      // BP: heretic have change something here
      sec = mp->FindModelFloorSector(floordestheight, sec);
      if (sec)
	{
	  texture = sec->floorpic;
	  newspecial = sec->special;
	  //jff 3/14/98 transfer both old and new special
	  oldspecial = sec->oldspecial;
	}
      break;

      // Instant Lower SSNTails 06-13-2002
    case instantLower:
      direction = -1;
      speed = MAXINT/2; // Go too fast and you'll cause problems...
      floordestheight = P_FindLowestFloorSurrounding(sec);
      break;

    case buildStair:
      direction = 1;
      break;
    case donutRaise:
      direction = 1;
      speed = FLOORSPEED / 2;
      newspecial = 0;
      break;
    default:
      break;

    }
}

// was EV_DoFloor
// HANDLE FLOOR TYPES
//
int Map::EV_DoFloor(line_t *line, floor_e floortype)
{
  int           secnum = -1;
  int           rtn = 0;
  sector_t     *sec;
  floormove_t  *floor;

  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
        
      // SoM: 3/6/2000: Boom has multiple thinkers per sector.
      // Don't start a second thinker on the same floor
      if (P_SectorActive(floor_special,sec)) //jff 2/23/98
	continue;

      // new floor thinker
      rtn = 1;
      floor = new floormove_t(floortype, sec, line, secnum);
      AddThinker(floor);
    }
  return rtn;
}


// SoM: 3/6/2000: Function for chaning just the floor texture and type.
//
// was EV_DoChange()
//
// Handle pure change types. These change floor texture and sector type
// by trigger or numeric model without moving the floor.
//
// The linedef causing the change and the type of change is passed
// Returns true if any sector changes
//
//
int Map::EV_DoChange(line_t *line, change_e changetype)
{
  int                   secnum;
  int                   rtn;
  sector_t*             sec;
  sector_t*             secm;

  secnum = -1;
  rtn = 0;
  // change all sectors with the same tag as the linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
              
      rtn = 1;

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
int Map::EV_BuildStairs(line_t *line, stair_e type)
{
  int                   secnum;
  int                   osecnum;
  int                   height;
  int                   i;
  int                   newsecnum;
  int                   texture;
  int                   ok;
  int                   rtn;
    
  sector_t*             sec;
  sector_t*             tsec;

  floormove_t*  floor;
    
  fixed_t               stairsize;
  fixed_t               speed;

  secnum = -1;
  rtn = 0;

  // start a stair at each sector tagged the same as the linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
              
      // don't start a stair if the first step's floor is already moving
      if (P_SectorActive(floor_special,sec))
	continue;
      
      // create new floor thinker for first step
      rtn = 1;
      floor = new floormove_t(buildStair, sec, line, secnum);
      AddThinker(floor);

      // set up the speed and stepsize according to the stairs type
      switch(type)
	{
	case build8:
	  speed = FLOORSPEED/4;
	  stairsize = 8*FRACUNIT;
	  if (boomsupport)
	    floor->crush = false; //jff 2/27/98 fix uninitialized crush field
	  break;
	case turbo16:
	  speed = FLOORSPEED*4;
	  stairsize = 16*FRACUNIT;
	  if (boomsupport)
	    floor->crush = true;  //jff 2/27/98 fix uninitialized crush field
	  break;
	  // used by heretic
	default:
	  speed = FLOORSPEED;
	  stairsize = type;
	  if (boomsupport)
	    floor->crush = true;  //jff 2/27/98 fix uninitialized crush field
	  break;
	}

      floor->speed = speed;
      height = sec->floorheight + stairsize;
      floor->floordestheight = height;
              
      texture = sec->floorpic;
      osecnum = secnum;           //jff 3/4/98 preserve loop index
      
      // Find next sector to raise
      //   1. Find 2-sided line with same sector side[0] (lowest numbered)
      //   2. Other side is the next sector to raise
      //   3. Unless already moving, or different texture, then stop building
      do
	{
	  ok = 0;
	  for (i = 0;i < sec->linecount;i++)
	    {
	      if ( !((sec->lines[i])->flags & ML_TWOSIDED) )
		continue;
                                  
	      tsec = (sec->lines[i])->frontsector;
	      newsecnum = tsec-sectors;
          
	      if (secnum != newsecnum)
		continue;

	      tsec = (sec->lines[i])->backsector;
	      if (!tsec) continue;     //jff 5/7/98 if no backside, continue
	      newsecnum = tsec - sectors;

	      // if sector's floor is different texture, look for another
	      if (tsec->floorpic != texture)
		continue;

	      if (!boomsupport) // jff 6/19/98 prevent double stepsize
		height += stairsize; // jff 6/28/98 change demo compatibility

	      // if sector's floor already moving, look for another
	      if (P_SectorActive(floor_special,tsec)) //jff 2/22/98
		continue;
                                  
	      if (boomsupport) // jff 6/19/98 increase height AFTER continue
		height += stairsize; // jff 6/28/98 change demo compatibility

	      sec = tsec;
	      secnum = newsecnum;

	      // create and initialize a thinker for the next step
	      floor = new floormove_t(buildStair, sec, line, secnum);
	      AddThinker(floor);

	      floor->speed = speed;
	      floor->floordestheight = height;
	      //jff 2/27/98 fix uninitialized crush field
	      if (boomsupport)
		floor->crush = type==build8? false : true;
	      ok = 1;
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
  sector_t* s1;
  sector_t* s2;
  sector_t* s3;
  int       secnum;
  int       rtn;
  int       i;
  floormove_t* floor;

  secnum = -1;
  rtn = 0;
  // do function on all sectors with same tag as linedef
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      s1 = &sectors[secnum];                // s1 is pillar's sector
              
      // do not start the donut if the pillar is already moving
      if (P_SectorActive(floor_special,s1)) //jff 2/22/98
	continue;
                      
      s2 = getNextSector(s1->lines[0],s1);  // s2 is pool's sector
      if (!s2) continue;                    // note lowest numbered line around
      // pillar must be two-sided 

      // do not start the donut if the pool is already moving
      if (boomsupport && P_SectorActive(floor_special,s2)) 
	continue;                           //jff 5/7/98
                      
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

	  rtn = 1; //jff 1/26/98 no donut action - no switch change on return

	  s3 = s2->lines[i]->backsector;      // s3 is model sector for changes
        
	  //  Spawn rising slime
	  floor = new floormove_t(donutRaise, s2, line, secnum);
	  AddThinker(floor);

	  floor->texture = s3->floorpic;
	  floor->floordestheight = s3->floorheight;
        
	  //  Spawn lowering donut-hole pillar
	  floor = new floormove_t(lowerFloor, s1, line, secnum);
	  AddThinker(floor);

	  // replace default lowerFloor data
	  floor->speed = FLOORSPEED / 2;
	  floor->floordestheight = s3->floorheight;
	  break;
	}
    }
  return rtn;
}

// constructor

elevator_t::elevator_t(elevator_e ty, sector_t *sec, line_t *line)
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
int Map::EV_DoElevator(line_t* line, elevator_e elevtype)
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
      elevator = new elevator_t(elevtype, sec, line);
      AddThinker(elevator);
    }
  return rtn;
}
