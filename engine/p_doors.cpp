// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Door animation code (opening/closing)


#include "doomdef.h"

#include "p_spec.h"
#include "r_defs.h"

#include "g_game.h"
#include "g_pawn.h"
#include "g_map.h"

#include "sounds.h"


//=========================================================================
//                           VERTICAL DOORS
//=========================================================================

IMPLEMENT_CLASS(vdoor_t, sectoreffect_t);
vdoor_t::vdoor_t() {}

// constructor
vdoor_t::vdoor_t(Map *m, byte t, sector_t *s, fixed_t sp, int delay)
  : sectoreffect_t(m, s)
{
  type = t;
  speed = sp;
  topwait = delay;
  boomlighttag = 0;
  s->ceilingdata = this;

  if (type & Delayed)
    {
      direction = 2;
      return;
    }

  switch (type & TMASK)
    {
    case Close:
    case CwO:
      direction = -1;
      topheight = s->ceilingheight;
      MakeSound(false);
      break;

    case Open:
    case OwC:
      direction = 1;
      topheight = s->FindLowestCeilingSurrounding() - 4;
      if (topheight != s->ceilingheight)
	MakeSound(true);
      break;

    default:
      break;
    }
}


void vdoor_t::Think()
{
  int res;

  switch (direction)
    {
    case 0:
      // WAITING
      if (!--topcount)
        {
	  switch (type & TMASK)
            {
	    case OwC:
	      direction = -1; // time to go back down
	      MakeSound(false);
	      break;

	    case CwO:
	      direction = 1;
	      MakeSound(true);
	      break;

	    default:
	      break;
            }
        }
      break;

    case 2:
      //  INITIAL WAIT
      if (!--topcount)
        {
	  switch (type & TMASK)
	    {
	    case Close:
	    case CwO:
	      direction = -1;
	      topheight = sector->ceilingheight;
	      MakeSound(false);
	      break;

	    case Open:
	    case OwC:
	      direction = 1;
	      topheight = sector->FindLowestCeilingSurrounding() - 4;
	      if (topheight != sector->ceilingheight)
		MakeSound(true);
	      break;

	    default:
	      break;
            }
        }
      break;

    case -1:
      // DOWN
      res = mp->T_MovePlane(sector, -speed, sector->floorheight, false, 1);
      if (res == res_pastdest)
        {
	  mp->SN_StopSequence(&sector->soundorg);
	  switch (type & TMASK)
            {
	    case OwC:
	    case Close:
	      sector->ceilingdata = NULL;
	      mp->TagFinished(sector->tag);
	      mp->RemoveThinker(this);  // unlink and free
	      break;

	    case CwO:
	      direction = 0;
	      topcount = topwait;
	      break;

	    default:
	      break;
            }

	  //SoM: 3/6/2000: Code to turn lighting off in tagged sectors.
	  if (boomsupport && boomlighttag)
	    mp->EV_TurnTagLightsOff(boomlighttag);
        }
      else if (res == res_crushed)
        {
	  if ((type & TMASK) != Close)
            {
	      direction = 1;
	      MakeSound(true);
	    }
        }
      break;

    case 1:
      // UP
      res = mp->T_MovePlane(sector, speed, topheight, false, 1);

      if (res == res_pastdest)
        {
	  mp->SN_StopSequence(&sector->soundorg);
	  switch(type & TMASK)
            {
	    case OwC:
	      direction = 0; // wait at top
	      topcount = topwait;
	      break;

	    case Open:
	    case CwO:
	      sector->ceilingdata = NULL;
	      mp->TagFinished(sector->tag);
	      mp->RemoveThinker(this);  // unlink and free
	      // if (game.mode == gm_heretic) S.Stop3DSound(&sector->soundorg);
	      break;

	    default:
	      break;
            }

	  //SoM: 3/6/2000: turn lighting on in tagged sectors of manual doors
	  if (boomsupport && boomlighttag)
	    mp->EV_LightTurnOn(boomlighttag, 0);
        }
      break;
    }
}


void vdoor_t::MakeSound(bool open) const
{
  int seq = sector->seqType;

  if ((game.mode == gm_hexen || game.mode == gm_heretic) && seq >= 0)
    {
      // TODO this is a nasty kludge which disables door sequences in Doom.
      mp->SN_StartSequence(&sector->soundorg, SEQ_DOOR + seq);
      return;
    }

  int s;
  if (open)
    {
      if (type & Blazing)
	s = sfx_bdopn;
      else
	s = sfx_doropn;
    }
  else
    {
      if (type & Blazing)
	s = sfx_bdcls;
      else
	s = sfx_dorcls;
    }

  S_StartSound(&sector->soundorg, s);
}


// operate a door
int Map::EV_DoDoor(int tag, line_t *line, Actor *mo, byte type, fixed_t speed, int delay)
{
  sector_t*  sec;
  int secnum = -1;
  int rtn = 0;

  if (speed >= 8)
    type |= vdoor_t::Blazing;

  if (tag)
    {
      while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
	{
	  sec = &sectors[secnum];
	  if (sec->Active(sector_t::ceiling_special)) //SoM: 3/6/2000
	    continue;

	  // new door thinker
	  rtn++;
	  new vdoor_t(this, type, sec, speed, delay);
	}
      return rtn;
    }
  else
    {
      // tag == 0, door is on the other side of the linedef
      bool good = mo->flags & MF_NOTMONSTER;

      // if the wrong side of door is pushed, give oof sound
      if (!line->sideptr[1] && good)
	{
	  S_StartSound(mo, sfx_usefail);
	  return 0;
	}

      // if the sector has an active thinker, use it
      sec = line->sideptr[1]->sector;

      if (sec->ceilingdata) //SoM: 3/6/2000
	{
	  // FIXME dangerous and wrong, since it could be a ceiling_t for example, started using a script!
	  vdoor_t *door = (vdoor_t *)sec->ceilingdata; //SoM: 3/6/2000
	  if ((door->type & vdoor_t::TMASK) == vdoor_t::OwC)
	    {
	      if (door->direction == -1)
		door->direction = 1;    // go back up
	      else if (GET_SPAC(line->flags) != SPAC_PUSH) // so that you can get them open
		{
		  if (!good)
		    return 0;            // JDC: bad guys never close doors

		  door->direction = -1;   // start going down immediately
		}
	      return 1;
	    }
	}      
      // new door thinker
      new vdoor_t(this, type, sec, speed, delay);

      return 1;
    }
}



// Generic function to open a door (used by FraggleScript)
void Map::EV_OpenDoor(int sectag, int speed, int wait_time)
{
  int type;
  int secnum = -1;

  if (speed < 1) speed = 1;
  
  // find out door type first

  if (wait_time)               // door closes afterward
    type = vdoor_t::OwC;
  else
    type = vdoor_t::Open;

  if (speed >= 4)
    type |= vdoor_t::Blazing;

  // open door in all the sectors with the specified tag

  while ((secnum = FindSectorFromTag(sectag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      // if the ceiling already moving, don't start the door action
      if (sec->Active(sector_t::ceiling_special))
        continue;

      // new door thinker
      new vdoor_t(this, type, sec, VDOORSPEED*speed, wait_time);
    }
}


// Used by FraggleScript
void Map::EV_CloseDoor(int sectag, int speed)
{
  int secnum = -1;

  if (speed < 1) speed = 1;
  
  // find out door type first
  int type = vdoor_t::Close;
  if (speed >= 4)
    type |= vdoor_t::Blazing;
  
  // open door in all the sectors with the specified tag

  while ((secnum = FindSectorFromTag(sectag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      // if the ceiling already moving, don't start the door action
      if (sec->Active(sector_t::ceiling_special)) //jff 2/22/98
        continue;

      // new door thinker
      new vdoor_t(this, type, sec, VDOORSPEED*speed, 0);
    }  
}


// Spawn a door that closes after 30 seconds
void Map::SpawnDoorCloseIn30(sector_t* sec)
{
  vdoor_t *door = new vdoor_t(this, vdoor_t::Close | vdoor_t::Delayed, sec, VDOORSPEED, VDOORWAIT);
  door->topcount = 30 * 35;
  sec->special = 0;
}

// Spawn a door that opens after 5 minutes
void Map::SpawnDoorRaiseIn5Mins(sector_t *sec)
{
  vdoor_t *door = new vdoor_t(this, vdoor_t::Open | vdoor_t::Delayed, sec, VDOORSPEED, VDOORWAIT);
  door->topcount = 5 * 60 * 35;
  sec->special = 0;
}



// ==========================================================================
//  SLIDE DOORS, ABANDONED TO THE MISTS OF TIME!!!
// ==========================================================================
