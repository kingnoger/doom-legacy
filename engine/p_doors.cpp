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
// Revision 1.9  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.8  2003/05/30 13:34:45  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.7  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.6  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.5  2003/04/14 08:58:26  smite-meister
// Hexen maps load.
//
// Revision 1.4  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.3  2003/03/15 20:07:15  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:17:54  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION: 
//      Door animation code (opening/closing)
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "doomdata.h"

#include "p_spec.h"

#include "g_game.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_player.h"

#include "dstrings.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"


#include "hardware/hw3sound.h"



// =========================================================================
//                            VERTICAL DOORS
// =========================================================================

int vdoor_t::s_open = 0;
int vdoor_t::s_bopen = 0;
int vdoor_t::s_close = 0;
int vdoor_t::s_bclose = 0;


// constructor
vdoor_t::vdoor_t(byte t, sector_t *s, fixed_t sp, int delay, line_t *li)
{
  type = t;
  sector = s;
  speed = sp;
  topwait = delay;
  line = li;
  s->ceilingdata = this;

  if (type & Delayed)
    {
      direction = 2;
      return;
    }

  switch (type & TMASK)
    {
    case Close:
      direction = -1;
      topheight = P_FindLowestCeilingSurrounding(s) - 4*FRACUNIT;
      MakeSound(false);
      break;

    case CwO:
      direction = -1;
      topheight = s->ceilingheight; // why different?
      MakeSound(false);
      break;

    case Open:
    case OwC:
      direction = 1;
      topheight = P_FindLowestCeilingSurrounding(s) - 4*FRACUNIT;
      if (topheight != s->ceilingheight)
	MakeSound(true);
      break;

    default:
      break;
    }
}


int vdoor_t::Serialize(LArchive & a)
{
  return 0;
}

//
// was T_VerticalDoor
//
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
	      direction = -1;
	      topheight = P_FindLowestCeilingSurrounding(sector) - 4*FRACUNIT;
	      MakeSound(false);
	      break;

	    case CwO:
	      direction = -1;
	      topheight = sector->ceilingheight; // why different?
	      MakeSound(false);
	      break;

	    case Open:
	    case OwC:
	      direction = 1;
	      topheight = P_FindLowestCeilingSurrounding(sector) - 4*FRACUNIT;
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
      res = mp->T_MovePlane(sector, speed, sector->floorheight, false, 1, -1);
      if (res == res_pastdest)
        {
	  switch (type & TMASK)
            {
	    case OwC:
	    case Close:
	      sector->ceilingdata = NULL;  // SoM: 3/6/2000
	      mp->TagFinished(sector->tag);
	      mp->RemoveThinker(this);  // unlink and free
	      if (boomsupport) //SoM: Removes the double closing sound of doors.
		MakeSound(false);
	      break;

	    case CwO:
	      direction = 0;
	      topcount = topwait;
	      break;

	    default:
	      break;
            }
	  //SoM: 3/6/2000: Code to turn lighting off in tagged sectors.
	  if (boomsupport && line && line->tag)
            {
              if (line->special > GenLockedBase && (line->special & 6) == 6)
                mp->EV_TurnTagLightsOff(line->tag);
              else switch (line->special)
                {
		case 10: case 12: case 13: // doors that close (they also should be manual P* doors)
		  // FIXME this does not work. If you give a door with a nonzero tag,
		  // it is interpreted as a Hexenlike door...
		  mp->EV_TurnTagLightsOff(line->tag);
		default:
                  break;
                }
            }
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
      res = mp->T_MovePlane(sector, speed, topheight, false, 1, 1);

      if (res == res_pastdest)
        {
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
	  if (boomsupport && line && line->tag)
            {
              if (line->special > GenLockedBase && (line->special&6)==6)     //jff 3/9/98 all manual doors
                mp->EV_LightTurnOn(line->tag, 0);
              else
                switch (line->special)
                {
		case 10: case 12: case 13: // doors that close (they also should be manual P* doors)
		  mp->EV_LightTurnOn(line->tag, 0);
		default:
		  break;
                }
            }
        }
      break;
    }
}


void vdoor_t::MakeSound(bool open) const
{
  // TODO sound sequences (makes more sense in Heretic too...)
  if (open)
    {
      if (type & Blazing)
	S_StartSound(&sector->soundorg, s_bopen);
      else
	S_StartSound(&sector->soundorg, s_open);
    }
  else
    {
      if (type & Blazing)
	S_StartSound(&sector->soundorg, s_bclose);
      else
	S_StartSound(&sector->soundorg, s_close);
    }
}


//
// was EV_DoLockedDoor
// Move a locked door up/down
//
// SoM: Removed the player checks at every different color door (checking to make sure 'p' is 
// not NULL) because you only need to do that once.
/*
int Map::EV_DoLockedDoor(line_t *line, PlayerPawn *p, byte type, byte lock, fixed_t speed, int delay)
{
  if (!p)
    return 0;

  if (!P_CheckKeys(p, lock))
    return 0;

  switch(line->special)
    {
      case 99:  // Blue Lock
      case 133:
        if (!(p->cards & it_bluecard) && !(p->cards & it_blueskull))
        {
            p->player->SetMessage(PD_BLUEO);
            S_StartScreamSound(p, sfx_oof); //SoM: 3/6/200: killough's idea
            return 0;
        }
        break;

      case 134: // Red Lock
      case 135:
        if (!(p->cards & it_redcard) && !(p->cards & it_redskull))
        {
            p->player->SetMessage(PD_REDO);
            S_StartScreamSound(p, sfx_oof); //SoM: 3/6/200: killough's idea
            return 0;
        }
        break;

      case 136: // Yellow Lock
      case 137:
        if (!(p->cards & it_yellowcard) &&
            !(p->cards & it_yellowskull))
        {
            p->player->SetMessage(PD_YELLOWO);
            S_StartScreamSound(p, sfx_oof); //SoM: 3/6/200: killough's idea
            return 0;
        }
        break;
    }

  return EV_DoDoor(line,type,speed, delay);
}
*/

//  was EV_DoDoor()

int Map::EV_DoDoor(line_t *line, Actor *mo, byte type, fixed_t speed, int delay)
{
  sector_t*  sec;
  vdoor_t*   door;

  int secnum = -1;
  int rtn = 0;

  if (line->tag)
    {
      while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
	{
	  sec = &sectors[secnum];
	  if (P_SectorActive(ceiling_special,sec)) //SoM: 3/6/2000
	    continue;

	  // new door thinker
	  rtn++;
	  door = new vdoor_t(type, sec, speed, delay, line);
	  AddThinker(door);
	}
      return rtn;
    }
  else
    {
      // tag == 0, door is on the other side of the linedef
      PlayerPawn *p = mo ? ((mo->Type() == Thinker::tt_ppawn) ? (PlayerPawn *)mo : NULL) : NULL;

      vdoor_t*   door;

      //SoM: 3/6/2000
      // if the wrong side of door is pushed, give oof sound
      if (line->sidenum[1] == -1)
	{
	  S_StartScreamSound(p, sfx_oof);    // killough 3/20/98
	  return 0;
	}

      // if the sector has an active thinker, use it
      sec = sides[line->sidenum[1]].sector;

      if (sec->ceilingdata) //SoM: 3/6/2000
	{
	  door = (vdoor_t *)sec->ceilingdata; //SoM: 3/6/2000
	  if (door->type & vdoor_t::TMASK == vdoor_t::OwC)
	    {
	      if (door->direction == -1)
		door->direction = 1;    // go back up
	      else if (GET_SPAC(line->flags) != SPAC_PUSH) // so that you can get them open
		{
		  if (!p)
		    return 0;            // JDC: bad guys never close doors

		  door->direction = -1;   // start going down immediately
		}
	      return 1;
	    }
	}      
      // new door thinker
      door = new vdoor_t(type, sec, speed, delay, line);
      AddThinker(door);

      return 1;
    }
}


// was EV_OpenDoor
// Generic function to open a door (used by FraggleScript)

void Map::EV_OpenDoor(int sectag, int speed, int wait_time)
{
  byte door_type;
  int secnum = -1;
  vdoor_t *door;

  if (speed < 1) speed = 1;
  
  // find out door type first

  if (wait_time)               // door closes afterward
    {
      if(speed >= 4)              // Blazing ?
        door_type = vdoor_t::OwC | vdoor_t::Blazing;
      else
        door_type = vdoor_t::OwC;
    }
  else
    {
      if(speed >= 4)              // Blazing ?
        door_type = vdoor_t::Open | vdoor_t::Blazing;
      else
        door_type = vdoor_t::Open;
    }

  // open door in all the sectors with the specified tag

  while ((secnum = FindSectorFromTag(sectag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      // if the ceiling already moving, don't start the door action
      if (P_SectorActive(ceiling_special,sec))
        continue;

      // new door thinker
      door = new vdoor_t(door_type, sec, VDOORSPEED*speed, wait_time, NULL);
      AddThinker(door);
    }
}


// was EV_CloseDoor
//
// Used by FraggleScript
void Map::EV_CloseDoor(int sectag, int speed)
{
  byte door_type;
  int secnum = -1;
  vdoor_t *door;

  if(speed < 1) speed = 1;
  
  // find out door type first

  if(speed >= 4)              // Blazing ?
    door_type = vdoor_t::Close | vdoor_t::Blazing;
  else
    door_type = vdoor_t::Close;
  
  // open door in all the sectors with the specified tag

  while ((secnum = FindSectorFromTag(sectag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      // if the ceiling already moving, don't start the door action
      if (P_SectorActive(ceiling_special,sec)) //jff 2/22/98
        continue;

      // new door thinker
      door = new vdoor_t(door_type, sec, VDOORSPEED*speed, 0, NULL);
      AddThinker(door);
    }  
}



//
// was EV_VerticalDoor : open a door manually, no tag value
//
//SoM: 3/6/2000: Needs int return for boom compatability. Also removed "side" and used boom
//methods insted.
/*
int Map::EV_VerticalDoor(line_t* line, Actor *m)
{
  PlayerPawn *p = (m->Type() == Thinker::tt_ppawn) ? (PlayerPawn *)m : NULL;
  int         secnum;
  sector_t*   sec;
  vdoor_t*   door;

  //  Check for locks

  switch(line->special)
    {
    case 26: // Blue Lock
    case 32:
      if ( !p )
	return 0;
      if (!(p->cards & it_bluecard) && !(p->cards & it_blueskull))
        {
	  p->player->SetMessage(PD_BLUEK);
	  S_StartScreamSound(p, sfx_oof); //SoM: 3/6/2000: Killough's idea
	  return 0;
        }
      break;

    case 27: // Yellow Lock
    case 34:
      if ( !p )
	return 0;
      
      if (!(p->cards & it_yellowcard) &&
	  !(p->cards & it_yellowskull))
        {
	  p->player->SetMessage(PD_YELLOWK);
	  S_StartScreamSound(p, sfx_oof); //SoM: 3/6/2000: Killough's idea
	  return 0;
        }
      break;

    case 28: // Red Lock
    case 33:
      if ( !p )
	return 0;

      if (!(p->cards & it_redcard) && !(p->cards & it_redskull))
        {
	  p->player->SetMessage(PD_REDK);
	  S_StartScreamSound(p, sfx_oof); //SoM: 3/6/2000: Killough's idea
	  return 0;
        }
      break;
    }
  //SoM: 3/6/2000
  // if the wrong side of door is pushed, give oof sound
  if (line->sidenum[1]==-1)                     // killough
    {
      S_StartScreamSound(p, sfx_oof);    // killough 3/20/98
      return 0;
    }

  // if the sector has an active thinker, use it
  sec = sides[line->sidenum[1]].sector;
  secnum = sec-sectors;

  if (sec->ceilingdata) //SoM: 3/6/2000
    {
      door = (vdoor_t *)sec->ceilingdata; //SoM: 3/6/2000
      switch(line->special)
        {
	case  1: // ONLY FOR "RAISE" DOORS, NOT "OPEN"s
	case  26:
	case  27:
	case  28:
	case  117:
	  if (door->direction == -1)
	    door->direction = 1;    // go back up
	  else
            {
	      if (!p)
		return 0;            // JDC: bad guys never close doors

	      door->direction = -1;   // start going down immediately
            }
	  return 1;
        }
    }

  // for proper sound
  switch(line->special)
    {
    case 117: // BLAZING DOOR RAISE
    case 118: // BLAZING DOOR OPEN
      S_StartSound(&sec->soundorg,sfx_bdopn);
      break;

    case 1:   // NORMAL DOOR SOUND
    case 31:
      S_StartSound(&sec->soundorg,sfx_doropn);
      break;

    default:  // LOCKED DOOR SOUND
      S_StartSound(&sec->soundorg,sfx_doropn);
      break;
    }

  int speed = VDOORSPEED;
  byte type = vdoor_t::OwC;

  switch(line->special)
    {
    case 1:
    case 26:
    case 27:
    case 28:
      type = vdoor_t::OwC;
      break;

    case 31:
    case 32:
    case 33:
    case 34:
      type = vdoor_t::Open;
      line->special = 0;
      break;

    case 117: // blazing door raise
      type = vdoor_t::OwC | vdoor_t::blazing;
      speed = VDOORSPEED*4;
      break;
    case 118: // blazing door open
      type = vdoor_t::Open | vdoor_t::blazing;
      line->special = 0;
      speed = VDOORSPEED*4;
      break;
    }
  // new door thinker
  // FIXME, you get some extra sounds here. the entire door/switch system should be reorganized.
  // you should just set the type right and let constructor take care of the rest.
  door = new vdoor_t(type, sec, speed, VDOORWAIT, line);
  AddThinker(door);

  return 1;
}
*/

// was P_SpawnDoorCloseIn30
// Spawn a door that closes after 30 seconds
//
void Map::SpawnDoorCloseIn30(sector_t* sec)
{
  vdoor_t *door = new vdoor_t(vdoor_t::Close | vdoor_t::Delayed, sec, VDOORSPEED, VDOORWAIT, NULL);
  door->topcount = 30 * 35;
  AddThinker(door);

  sec->special = 0;
}

// P_SpawnDoorRaiseIn5Mins
// Spawn a door that opens after 5 minutes
//
void Map::SpawnDoorRaiseIn5Mins(sector_t *sec)
{
  vdoor_t *door = new vdoor_t(vdoor_t::Open | vdoor_t::Delayed, sec, VDOORSPEED, VDOORWAIT, NULL);
  door->topcount = 5 * 60 * 35;
  AddThinker(door);

  sec->special = 0;
}



// ==========================================================================
//  SLIDE DOORS, ABANDONED TO THE MISTS OF TIME!!!
// ==========================================================================
