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
// Revision 1.12  2003/12/23 18:06:06  smite-meister
// Hexen stairbuilders. Moving geometry done!
//
// Revision 1.11  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.10  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.9  2003/11/30 00:09:43  smite-meister
// bugfixes
//
// Revision 1.8  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.7  2003/11/12 11:07:20  smite-meister
// Serialization done. Map progression.
//
// Revision 1.6  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.5  2003/05/30 13:34:45  smite-meister
// Cleanup, HUD improved, serialization
//
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
//   Floor movement: floors, stairbuilders, donuts. Elevators, pillars.
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


// Move a plane (floor or ceiling) and check for crushing
int Map::T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, int crush, int floorOrCeiling)
{
  fixed_t  lastpos;     
  fixed_t  destheight; //jff 02/04/98 used to keep floors/ceilings from moving thru each other

  switch (floorOrCeiling)
    {
    case 0: // Moving a floor
      lastpos = sector->floorheight;
      if (speed < 0)
	{
	  // floor going down
          //SoM: 3/20/2000: Make splash when platform floor hits water
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->floorheight - speed) < sectors[sector->heightsec].floorheight
		 && sector->floorheight > sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }

          if (sector->floorheight + speed < dest)
	    {
	      sector->floorheight = dest;
	      if (CheckSector(sector, crush) && sector->numattached)                   
		{
		  sector->floorheight = lastpos;
		  CheckSector(sector, crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      sector->floorheight += speed;
	      if (CheckSector(sector, crush) && sector->numattached)
		{
		  sector->floorheight = lastpos;
		  CheckSector(sector, crush);
		  return res_crushed;
		}
	    }
	}
      else
	{
	  // floor going up
          //SoM: 3/20/2000: Make splash when platform floor hits water
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->floorheight + speed) > sectors[sector->heightsec].floorheight
		 && sector->floorheight < sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }
          // keep floor from moving thru ceilings
          destheight = (!boomsupport || dest < sector->ceilingheight)?
	    dest : sector->ceilingheight;

          if (sector->floorheight + speed > destheight)
	    {
	      sector->floorheight = destheight;
	      if (CheckSector(sector, crush))
		{
		  sector->floorheight = lastpos;
		  CheckSector(sector, crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      // crushing is possible
	      sector->floorheight += speed;
	      if (CheckSector(sector, crush))
		{
		  if (!boomsupport)
		    {
		      if (crush == true)
			return res_crushed;
		    }
		  sector->floorheight = lastpos;
		  CheckSector(sector, crush);
		  return res_crushed;
		}
	    }
	}
      break;
                                                                        
    case 1: // moving a ceiling
      lastpos = sector->ceilingheight;
      if (speed < 0)
	{
	  // ceiling going down
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

	  lastpos = sector->ceilingheight;
          if (sector->ceilingheight + speed < destheight)
	    {
	      sector->ceilingheight = destheight;
	      if (CheckSector(sector, crush))
		{
		  sector->ceilingheight = lastpos;
		  CheckSector(sector, crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      // crushing is possible
	      sector->ceilingheight += speed;
	      if (CheckSector(sector, crush))
		{
		  if (crush == true)
		    return res_crushed;
		  sector->ceilingheight = lastpos;
		  CheckSector(sector, crush);
		  return res_crushed;
		}
	    }
	}
      else
	{
          // ceiling going up
          if(boomsupport && sector->heightsec != -1 && sector->altheightsec == 1)
	    {
	      if((sector->ceilingheight + speed) > sectors[sector->heightsec].floorheight
		 && sector->ceilingheight < sectors[sector->heightsec].floorheight)
		S_StartSound(&sector->soundorg, sfx_gloop);
	    }
          if (sector->ceilingheight + speed > dest)
	    {
	      sector->ceilingheight = dest;
	      if (CheckSector(sector, crush) && sector->numattached)
		{
		  sector->ceilingheight = lastpos;
		  CheckSector(sector, crush);
		}
	      return res_pastdest;
	    }
          else
	    {
	      sector->ceilingheight += speed;
	      if (CheckSector(sector, crush) && sector->numattached)
		{
		  sector->ceilingheight = lastpos;
		  CheckSector(sector, crush);
		  return res_crushed;
		}
	    }
	}
      break;

    }
  return res_ok;
}


//==========================================================================
//  floor_t: Moving floors
//==========================================================================

IMPLEMENT_CLASS(floor_t, "Floor");
floor_t::floor_t() {}

// constructor
floor_t::floor_t(int ty, sector_t *sec, fixed_t sp, int cru, fixed_t height)
  : sectoreffect_t(sec)
{
  type = ty;
  crush = cru;
  speed = sp; // default: up
  sec->floordata = this;

  switch (ty & TMASK)
    {
    case RelHeight:
      destheight = sec->floorheight + height;
      if (height < 0)
	speed = -speed;
      break;

    case AbsHeight:
      destheight = height;
      if (sec->floorheight > height)
	speed = -speed;
      break;

    case Ceiling:
      destheight = sec->ceilingheight + height;
      break;

    case LnF:
      destheight = P_FindLowestFloorSurrounding(sec) + height;
      speed = -speed;
      break;

    case UpNnF:
      destheight = P_FindNextHighestFloor(sec, sec->floorheight) + height;
      break;

    case DownNnF:
      destheight = P_FindNextLowestFloor(sec, sec->floorheight) + height;
      speed = -speed;
      break;

    case HnF:
      destheight = P_FindHighestFloorSurrounding(sec) + height;
      speed = -speed; // TODO up/down?
      break;

    case LnC:
      destheight = P_FindLowestCeilingSurrounding(sec);
      if (destheight > sec->ceilingheight)
	destheight = sec->ceilingheight;
      destheight += height;
      // TODO up/down?
      break;

    case SLT:
      destheight = sector->floorheight + mp->FindShortestLowerAround(sec) + height;
      if (boomsupport && destheight > (32000 << FRACBITS))
	destheight = 32000 << FRACBITS; //jff 3/13/98 do not allow height overflow
      break;

    default:
      I_Error("Unknown floor target %d!\n", ty);
      break;
    }

  //SN_StartSequence((mobj_t *)&sector->soundorg, SEQ_PLATFORM+floor->sector->seqType);
}


void floor_t::Think()
{
  int res = mp->T_MovePlane(sector, speed, destheight, crush, 0);

  if (!(mp->maptic % (8*NEWTICRATERATIO)))
    S_StartSound(&sector->soundorg, ceiling_t::ceilmovesound);

  if (res == res_pastdest)
    {
      if (type & SetTexture)
	sector->floorpic = texture;

      if (type & SetSpecial)
	sector->special = newspecial;

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


//==========================================================================
// Basic moving floors
//==========================================================================

int Map::EV_DoFloor(int tag, line_t *line, int type, fixed_t speed, int crush, fixed_t height)
{
  int  secnum = -1;
  int  rtn = 0;

  if (!tag)
    return false;

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
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
	      //jff 5/23/98 use model subroutine to unify fixes and handling
	      // BP: heretic have change something here
	      sec = FindModelFloorSector(floor->destheight, sec);
	      if (sec)
		{
		  floor->texture = sec->floorpic;
		  floor->newspecial = sec->special;
		}
	    }
	  else
	    {
	      // "trigger model"
	      floor->texture = line->frontsector->floorpic;
	      floor->newspecial = line->frontsector->special;
	    }
	}
    }
  return rtn;
}


// Handle pure change types. These change floor texture and sector type
// by trigger or numeric model without moving the floor (no Thinker needed)
//
// The linedef causing the change and the type of change is passed
// Returns true if any sector changes

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
	  break;
	case numChangeOnly:
	  secm = FindModelFloorSector(sec->floorheight, sec);
	  if (secm) // if no model, no change
	    {
	      sec->floorpic = secm->floorpic;
	      sec->special = secm->special;
	    }
	  break;
	default:
	  break;
	}
    }
  return rtn;
}

//==========================================================================
// Stairbuilders, Doom and Hexen styles
//==========================================================================

int Map::EV_BuildStairs(int tag, int type, fixed_t speed, fixed_t stepsize, int crush)
{
  // TODO Hexen compatibility (a new stairbuilding method...)
  int                   i;

  int secnum = -1;
  int rtn = 0;

  // start a stair at each sector tagged the same as the linedef
  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
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



//==========================================================================
//  stair_t: Stairbuilder steps
//==========================================================================

IMPLEMENT_CLASS(stair_t, "Stairbuilder");
stair_t::stair_t() {}

static fixed_t StartHeight, StepDelta; // small hacks...

stair_t::stair_t(int ty, sector_t *sec, fixed_t h, fixed_t sp, int rcount, int sdelay)
  : sectoreffect_t(sec)
{
  sec->floordata = this;

  state = Moving;
  resetcount = rcount;
  destheight = h;
  originalheight = sec->floorheight;

  wait = 0;
  stepdelay = 0;
  delayheight = 0;
  stepdelta = 0;

  switch (ty)
    {
    case Normal:
      speed = sp;
      if (sdelay)
	{
	  stepdelay = sdelay;
	  delayheight = sec->floorheight + StepDelta;
	  stepdelta = StepDelta;
	}
      break;

    case Sync:
      speed = FixedMul(sp, FixedDiv(h - StartHeight, StepDelta));
      break;

    default:
      break;
    }

  //SN_StartSequence((mobj_t *)&sec->soundorg, SEQ_PLATFORM + sec->seqType);
}


// TODO this could be used to do the Boom stairs as well (and to remove stairlocks etc. from sector_t)
void stair_t::Think()
{
  if (resetcount)
    if (!--resetcount)
      {
	destheight = originalheight;
	speed = -speed;
	state = Moving;
	stepdelay = 0; // no more delays on the way back
	//SN_StartSequence((mobj_t *)&sec->soundorg, SEQ_PLATFORM + sec->seqType);
      }

  int res;

  switch (state)
    {
    case Waiting:
      if (!--wait)
	state = Moving;
      return;

    case Moving:
      res = mp->T_MovePlane(sector, speed, destheight, 0, 0);

      if (res == res_pastdest)
	{
	  //SN_StopSequence((mobj_t *)&sector->soundorg);
	  if (resetcount)
	    {
	      state = Done;
	      return; // now just wait for the reset
	    }

	  sector->floordata = NULL;

	  mp->TagFinished(sector->tag);
	  mp->RemoveThinker(this);
	  return;
	}

      if (stepdelay)
	if ((speed >= 0 && sector->floorheight >= delayheight) || (speed < 0 && sector->floorheight <= delayheight))
	  {
	    state = Waiting;
	    wait = stepdelay;
	    delayheight += stepdelta;
	  }
      break;

    case Done:
      break;
    }
}

// helper struct for spawning stairbuilders
struct step_t
{
  sector_t *sector;
  char      phase; // 0 or 1
  fixed_t   height;
};

int Map::EV_BuildHexenStairs(int tag, int type, fixed_t speed, fixed_t stepdelta, int resetdelay, int stepdelay)
{
  deque<step_t> stairqueue;
  step_t s;

  int ret = 0;
  int Texture = 0;

  // type STAIRS_PHASED is not implemented (nor was it in original Hexen)
  StepDelta = stepdelta;

  int secnum = -1;
  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];

      // TODO wrong, if there are several tagged sectors with different heights/textures... was like this in Hexen.
      Texture = sec->floorpic;
      StartHeight = sec->floorheight;

      if (sec->floordata)
	continue;

      s.sector = sec;
      s.phase = 0;
      s.height = sec->floorheight;
      stairqueue.push_back(s);

      sec->special = 0;
    }

  extern int validcount;
  validcount++;

  while (!stairqueue.empty())
    {
      s = stairqueue.front();
      stairqueue.pop_front();
      sector_t *sec = s.sector;
      char phase = s.phase;

      s.height += stepdelta;

      stair_t *stair = new stair_t(type, sec, s.height, speed, resetdelay, stepdelay);
      AddThinker(stair);
      ret++;

      // Find the next step (alternating phase!)
      for (int i = 0; i < sec->linecount; i++)
	{
	  if (!(sec->lines[i]->flags & ML_TWOSIDED))
	    continue;

	  sector_t *tsec = sec->lines[i]->frontsector;
	  if ((tsec->special == phase + SS_Stairs_Special1) && !tsec->floordata
	      && tsec->floorpic == Texture && tsec->validcount != validcount)
	    {
	      s.sector = tsec;
	      s.phase = phase^1;
	      // s.height is OK
	      stairqueue.push_back(s);

	      tsec->validcount = validcount;
	      //tsec->special = 0;
	    }
	  tsec = sec->lines[i]->backsector;
	  if ((tsec->special == phase + SS_Stairs_Special1) && !tsec->floordata
	      && tsec->floorpic == Texture && tsec->validcount != validcount)
	    {
	      s.sector = tsec;
	      s.phase = phase^1;
	      // s.height is OK
	      stairqueue.push_back(s);

	      tsec->validcount = validcount;
	      //tsec->special = 0;
	    }
	}
    }

  return ret;
}


// ==========================================================================
// Handle donut function: lower pillar, raise surrounding pool, both to height,
// texture and type of the sector surrounding the pool.
//
// Passed the linedef that triggered the donut
// Returns whether a thinker was created

int Map::EV_DoDonut(int tag)
{
  int       i;
  int secnum = -1;
  int rtn = 0;
  // do function on all sectors with same tag as linedef
  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
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


//=========================================================================
// Boom Elevators, Hexen Pillars
//=========================================================================

IMPLEMENT_CLASS(elevator_t, "Elevator");
elevator_t::elevator_t() {}

// constructor
elevator_t::elevator_t(int ty, sector_t *sec, fixed_t sp, fixed_t height_f, fixed_t height_c, int cr)
  : sectoreffect_t(sec)
{
  type = ty;
  crush = cr;
  sec->floordata = this;
  sec->ceilingdata = this;

  // set up the fields according to the type of elevator action
  switch (ty)
    {
      // (only OpenPillar uses height_c)
      // relative displacement
    case RelHeight:
      if (height_f < 0)
	sp = -sp;
      floordest = sec->floorheight + height_f;
      ceilingdest = sec->ceilingheight + height_f;
      break;

      // elevator down to next floor
    case Down:
      sp = -sp;
      floordest = P_FindNextLowestFloor(sec, sec->floorheight) + height_f;
      ceilingdest = floordest + sec->ceilingheight - sec->floorheight;
      break;

      // elevator up to next floor
    case Up:
      floordest = P_FindNextHighestFloor(sec, sec->floorheight) + height_f;
      ceilingdest = floordest + sec->ceilingheight - sec->floorheight;
      break;

      // elevator to floor height of activating switch's front sector
    case Current:
      floordest = height_f;
      ceilingdest = floordest + sec->ceilingheight - sec->floorheight;
      if (floordest < sec->floorheight)
	sp = -sp;
      break;

    case ClosePillar:
      if (!height_f)
	floordest = sec->floorheight + (sec->ceilingheight - sec->floorheight) / 2;
      else
	floordest = sec->floorheight + height_f;
      ceilingdest = floordest;

      if (floordest - sec->floorheight >= sec->ceilingheight - ceilingdest)
	{
	  floorspeed = sp;
	  ceilingspeed = -FixedMul(sec->ceilingheight - ceilingdest, FixedDiv(sp, floordest - sec->floorheight));
	}
      else
	{
	  floorspeed = FixedMul(floordest - sec->floorheight, FixedDiv(sp, sec->ceilingheight - ceilingdest));
	  ceilingspeed = -sp;
	}

      // SN_StartSequence((mobj_t *)&sec->soundorg, SEQ_PLATFORM+sec->seqType); // TODO
      return;

    case OpenPillar:
      if (!height_f)
	floordest = P_FindLowestFloorSurrounding(sec);
      else
	floordest = sec->floorheight - height_f;

      if (!height_c)
	ceilingdest = P_FindHighestCeilingSurrounding(sec);
      else
	ceilingdest = sec->ceilingheight + height_c;

      if (sec->floorheight - floordest >= ceilingdest - sec->ceilingheight)
	{
	  floorspeed = -sp;
	  ceilingspeed = FixedMul(sec->ceilingheight - ceilingdest, FixedDiv(sp, floordest - sec->floorheight));
	}
      else
	{
	  floorspeed = -FixedMul(floordest - sec->floorheight, FixedDiv(sp, sec->ceilingheight - ceilingdest));
	  ceilingspeed = sp;
	}

      //SN_StartSequence((mobj_t *)&sector->soundorg, SEQ_PLATFORM+sec->seqType);
      return;

    default:
      I_Error("Unknown elevator type %d!\n", ty);
      break;
    }

  floorspeed = ceilingspeed = sp;
}


// Move an elevator to it's destination (up or down)
//
// SoM: 3/6/2000: The function moves the plane differently based on direction, so if it's 
// traveling really fast, the floor and ceiling won't hit each other and stop the lift.
void elevator_t::Think()
{
  int res1, res2;
  res1 = res2 = res_crushed;

  if (ceilingspeed < 0)
    {
      // down/closing: first move floor, then ceiling
      res1 = mp->T_MovePlane(sector, floorspeed, floordest, crush, 0);
      if (res1 == res_ok || res1 == res_pastdest)
	res2 = mp->T_MovePlane(sector, ceilingspeed, ceilingdest, crush, 1);
    }
  else
    {
      // up/opening: first move ceiling, then floor
      res1 = mp->T_MovePlane(sector, ceilingspeed, ceilingdest, crush, 1);
      if (res1 == res_ok || res1 == res_pastdest)
	res2 = mp->T_MovePlane(sector, floorspeed, floordest, crush, 0);
    }

  if (res1 == res_pastdest && res2 == res_pastdest) // if destination height acheived
    {
      sector->floordata = NULL;
      sector->ceilingdata = NULL;
      mp->RemoveThinker(this);

      mp->TagFinished(sector->tag);

      // make floor stop sound
      S_StartSound(&sector->soundorg, sfx_pstop);
      //SN_StopSequence((mobj_t *)&sector->soundorg); // TODO
      return;
    }

  // make floor move sound
  //if (!(mp->maptic % (8*NEWTICRATERATIO)))  S_StartSound(&sector->soundorg, sfx_stnmov);
}


int Map::EV_DoElevator(int tag, int type, fixed_t speed, fixed_t height_f, fixed_t height_c, int crush)
{
  int secnum = -1;
  int rtn = 0;

  // act on all sectors with the correct tag
  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
              
      // If either floor or ceiling is already activated, skip it
      if (sec->floordata || sec->ceilingdata)
	continue;

      if (type == elevator_t::ClosePillar)
	if (sec->floorheight == sec->ceilingheight)
	  continue; // pillar is already closed

      if (type == elevator_t::OpenPillar)
	if (sec->floorheight != sec->ceilingheight)
	  continue;  // pillar isn't closed

      // create and initialize new elevator thinker
      elevator_t *elevator = new elevator_t(type, sec, speed, height_f, height_c, crush);
      AddThinker(elevator);
      rtn++;
    }

  return rtn;
}



//==========================================================================
//  FloorWaggle (Hexen)
//==========================================================================

IMPLEMENT_CLASS(floorwaggle_t, "Floorwaggle");
floorwaggle_t::floorwaggle_t() {}

floorwaggle_t::floorwaggle_t(sector_t *sec, fixed_t a, angle_t f, angle_t ph, int w)
  : sectoreffect_t(sec)
{
  sec->floordata = this;

  baseheight = sector->floorheight;
  phase = ph;
  freq = f;
  amp = 0;
  maxamp = a;
  ampdelta = a / (35 + ((3*35) * (a >> 13))/255);
  wait = w ? w : -1;
  state = Expand;
}


void floorwaggle_t::Think()
{
  switch (state)
    {
    case Expand:
      if ((amp += ampdelta) >= maxamp)
	{
	  amp = maxamp;
	  state = Stable;
	}
      break;

    case Stable:
      if (wait != -1)
	{
	  if (!--wait)
	    state = Reduce;
	}
      break;

    case Reduce:
      if ((amp -= ampdelta) <= 0)
	{
	  sector->floorheight = baseheight;
	  mp->CheckSector(sector, true);
	  sector->floordata = NULL;
	  mp->TagFinished(sector->tag);
	  mp->RemoveThinker(this);
	  return;
	}
      break;
    }
  // floatbob == 8*sin(x), 2*pi divided in 64 units
  phase += freq;
  sector->floorheight = baseheight + FixedMul(finesine[phase >> ANGLETOFINESHIFT], amp);
  mp->CheckSector(sector, true);
}

  
int Map::EV_DoFloorWaggle(int tag, fixed_t amp, angle_t freq, angle_t offset, int wait)
{
  int ret = 0;
  int secnum = -1;

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      if (sec->floordata)
	continue;

      floorwaggle_t *waggle = new floorwaggle_t(sec, amp, freq, offset, wait);
      AddThinker(waggle);
      ret++;
    }

  return ret;
}
