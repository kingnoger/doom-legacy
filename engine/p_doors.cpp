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

int vldoor_t::doorclosesound = 0;


// constructor
vldoor_t::vldoor_t(vldoor_e ty, sector_t *s, fixed_t sp, int delay, line_t *li)
{
  type = ty;
  sector = s;
  speed = sp;
  topwait = delay;
  line = li;
  s->ceilingdata = this;

  switch(type)
    {
    case blazeClose:
      topheight = P_FindLowestCeilingSurrounding(s);
      topheight -= 4*FRACUNIT;
      direction = -1;
      S_StartSound(&s->soundorg,sfx_bdcls);
      break;

    case doorclose:
      topheight = P_FindLowestCeilingSurrounding(s);
      topheight -= 4*FRACUNIT;
      direction = -1;
      S_StartSound(&s->soundorg, doorclosesound);
      break;

    case close30ThenOpen:
      topheight = s->ceilingheight;
      direction = -1;
      S_StartSound(&s->soundorg, doorclosesound);
      break;

    case blazeRaise:
    case blazeOpen:
      direction = 1;
      topheight = P_FindLowestCeilingSurrounding(s);
      topheight -= 4*FRACUNIT;
      if (topheight != s->ceilingheight)
	S_StartSound(&s->soundorg,sfx_bdopn);
      break;

    case normalDoor:
    case dooropen:
      direction = 1;
      topheight = P_FindLowestCeilingSurrounding(s);
      topheight -= 4*FRACUNIT;
      if (topheight != s->ceilingheight)
	S_StartSound(&s->soundorg,sfx_doropn);
      break;

    default:
      break;
    }
}


//
// was T_VerticalDoor
//
void vldoor_t::Think()
{
  result_e    res;

  switch(direction)
    {
    case 0:
      // WAITING
      if (!--topcountdown)
        {
	  switch(type)
            {
	    case blazeRaise:
	    case genBlazeRaise: //SoM: 3/6/2000
	      direction = -1; // time to go back down
	      S_StartSound(&sector->soundorg,
			   sfx_bdcls);
	      break;

	    case normalDoor:
	    case genRaise:   //SoM: 3/6/2000
	      direction = -1; // time to go back down
	      S_StartSound(&sector->soundorg,
			   doorclosesound);
	      break;

	    case close30ThenOpen:
	    case genCdO:      //SoM: 3/6/2000
	      direction = 1;
	      S_StartSound(&sector->soundorg,
			   sfx_doropn);
	      break;

              //SoM: 3/6/2000
	    case genBlazeCdO:
	      direction = 1;  // time to go back up
	      S_StartSound(&sector->soundorg,sfx_bdopn);
	      break;

	    default:
	      break;
            }
        }
      break;

    case 2:
      //  INITIAL WAIT
      if (!--topcountdown)
        {
	  switch(type)
            {
	    case raiseIn5Mins:
	      direction = 1;
	      type = normalDoor;
	      S_StartSound(&sector->soundorg,
			   sfx_doropn);
	      break;

	    default:
	      break;
            }
        }
      break;

    case -1:
      // DOWN
      res = mp->T_MovePlane(sector, speed, sector->floorheight, false,1,direction);
      if (res == pastdest)
        {
	  switch(type)
            {
	    case blazeRaise:
	    case blazeClose:
	    case genBlazeRaise:
	    case genBlazeClose:
	      sector->ceilingdata = NULL;  // SoM: 3/6/2000
	      mp->RemoveThinker(this);  // unlink and free
	      if(boomsupport) //SoM: Removes the double closing sound of doors.
		S_StartSound(&sector->soundorg, sfx_bdcls);
	      break;

	    case normalDoor:
	    case doorclose:
	    case genRaise:
	    case genClose:
	      sector->ceilingdata = NULL; //SoM: 3/6/2000
	      mp->RemoveThinker(this);  // unlink and free
	      if (game.mode == gm_heretic)
		S_StartSound (& sector->soundorg, sfx_dorcls);

	      break;

	    case close30ThenOpen:
	      direction = 0;
	      topcountdown = 35*30;
	      break;

              //SoM: 3/6/2000: General door stuff
	    case genCdO:
	    case genBlazeCdO:
	      direction = 0;
	      topcountdown = topwait; 
	      break;

	    default:
	      break;
            }
	  //SoM: 3/6/2000: Code to turn lighting off in tagged sectors.
	  if (boomsupport && line && line->tag)
            {
              if (line->special > GenLockedBase &&
                  (line->special&6)==6)
                mp->EV_TurnTagLightsOff(line);
              else switch (line->special)
                {
		case 1: case 31:
		case 26:
		case 27: case 28:
		case 32: case 33:
		case 34: case 117:
		case 118:
		  mp->EV_TurnTagLightsOff(line);
		default:
                  break;
                }
            }
        }
      else if (res == crushed)
        {
	  switch(type)
            {
	    case genClose: //SoM: 3/6/2000
	    case genBlazeClose: //SoM: 3/6/2000
	    case blazeClose:
	    case doorclose:           // DO NOT GO BACK UP!
	      break;

	    default:
	      direction = 1;
	      S_StartSound(&sector->soundorg,
			   sfx_doropn);
	      break;
            }
        }
      break;

    case 1:
      // UP
      res = mp->T_MovePlane(sector, speed, topheight, false,1,direction);

        if (res == pastdest)
        {
            switch(type)
            {
              case blazeRaise:
              case normalDoor:
              case genRaise:     //SoM: 3/6/2000
              case genBlazeRaise://SoM: 3/6/2000
                direction = 0; // wait at top
                topcountdown = topwait;
                break;

              case close30ThenOpen:
              case blazeOpen:
              case dooropen:
              case genBlazeOpen:
              case genOpen:
              case genCdO:
              case genBlazeCdO:
                sector->ceilingdata = NULL;
                mp->RemoveThinker(this);  // unlink and free
                if( game.mode == gm_heretic )
                    S.Stop3DSound(&sector->soundorg);

                break;

              default:
                break;
            }
            //SoM: 3/6/2000: turn lighting on in tagged sectors of manual doors
            if (boomsupport && line && line->tag)
            {
              if (line->special > GenLockedBase &&
                  (line->special&6)==6)     //jff 3/9/98 all manual doors
                mp->EV_LightTurnOn(line,0);
              else
                switch (line->special)
                {
                  case 1: case 31:
                  case 26:
                  case 27: case 28:
                  case 32: case 33:
                  case 34: case 117:
                  case 118:
                    mp->EV_LightTurnOn(line,0);
                  default:
                    break;
                }
            }
        }
        break;
    }
}


//
// was EV_DoLockedDoor
// Move a locked door up/down
//
// SoM: Removed the player checks at every different color door (checking to make sure 'p' is 
// not NULL) because you only need to do that once.
int Map::EV_DoLockedDoor(line_t *line, vldoor_e type, PlayerPawn *p, fixed_t speed)
{
  // PlayerPawn  *p;

  //p = thing->player;

  if (!p) return 0;

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

    return EV_DoDoor(line,type,speed);
}


//  was EV_DoDoor()

int Map::EV_DoDoor(line_t *line, vldoor_e type, fixed_t speed)
{
  int         secnum,rtn;
  sector_t*   sec;
  vldoor_t*   door;

  secnum = -1;
  rtn = 0;

  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
      if (P_SectorActive(ceiling_special,sec)) //SoM: 3/6/2000
	continue;

      // new door thinker

      rtn = 1;
      door = new vldoor_t(type, sec, speed, VDOORWAIT, line);
      AddThinker(door);
    }
  return rtn;
}


// was EV_OpenDoor
// Generic function to open a door (used by FraggleScript)

void Map::EV_OpenDoor(int sectag, int speed, int wait_time)
{
  vldoor_e door_type;
  int secnum = -1;
  vldoor_t *door;

  if (speed < 1) speed = 1;
  
  // find out door type first

  if (wait_time)               // door closes afterward
    {
      if(speed >= 4)              // blazing ?
        door_type = blazeRaise;
      else
        door_type = normalDoor;
    }
  else
    {
      if(speed >= 4)              // blazing ?
        door_type = blazeOpen;
      else
        door_type = dooropen;
    }

  // open door in all the sectors with the specified tag

  while ((secnum = FindSectorFromTag(sectag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      // if the ceiling already moving, don't start the door action
      if (P_SectorActive(ceiling_special,sec))
        continue;

      // new door thinker
      door = new vldoor_t(door_type, sec, VDOORSPEED*speed, wait_time, NULL);
      AddThinker(door);
    }
}


// was EV_CloseDoor
//
// Used by FraggleScript
void Map::EV_CloseDoor(int sectag, int speed)
{
  vldoor_e door_type;
  int secnum = -1;
  vldoor_t *door;

  if(speed < 1) speed = 1;
  
  // find out door type first

  if(speed >= 4)              // blazing ?
    door_type = blazeClose;
  else
    door_type = doorclose;
  
  // open door in all the sectors with the specified tag

  while ((secnum = FindSectorFromTag(sectag, secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];
      // if the ceiling already moving, don't start the door action
      if (P_SectorActive(ceiling_special,sec)) //jff 2/22/98
        continue;

      // new door thinker
      door = new vldoor_t(door_type, sec, VDOORSPEED*speed, 0, NULL);
      AddThinker(door);
    }  
}



//
// was EV_VerticalDoor : open a door manually, no tag value
//
//SoM: 3/6/2000: Needs int return for boom compatability. Also removed "side" and used boom
//methods insted.
int Map::EV_VerticalDoor(line_t* line, Actor *m)
{
  PlayerPawn *p = (m->Type() == Thinker::tt_ppawn) ? (PlayerPawn *)m : NULL;
  int         secnum;
  sector_t*   sec;
  vldoor_t*   door;
//    int         side; //SoM: 3/6/2000

//    side = 0;   // only front sides can be used

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
      door = (vldoor_t *)sec->ceilingdata; //SoM: 3/6/2000
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
  vldoor_e type = normalDoor;

  switch(line->special)
    {
    case 1:
    case 26:
    case 27:
    case 28:
      type = normalDoor;
      break;

    case 31:
    case 32:
    case 33:
    case 34:
      type = dooropen;
      line->special = 0;
      break;

    case 117: // blazing door raise
      type = blazeRaise;
      speed = VDOORSPEED*4;
      break;
    case 118: // blazing door open
      type = blazeOpen;
      line->special = 0;
      speed = VDOORSPEED*4;
      break;
    }
  // new door thinker
  // FIXME, you get some extra sounds here. the entire door/switch system should be reorganized.
  // you should just set the type right and let constructor take care of the rest.
  door = new vldoor_t(type, sec, speed, VDOORWAIT, line);
  AddThinker(door);

  return 1;
}


// was P_SpawnDoorCloseIn30
// Spawn a door that closes after 30 seconds
//
void Map::SpawnDoorCloseIn30 (sector_t* sec)
{
  vldoor_t *door = new vldoor_t(normalDoor, sec, VDOORSPEED, VDOORWAIT, NULL);
  AddThinker(door);

  sec->special = 0;

  door->direction = 0;
  door->topcountdown = 30 * 35;
}

// P_SpawnDoorRaiseIn5Mins
// Spawn a door that opens after 5 minutes
//
void Map::SpawnDoorRaiseIn5Mins(sector_t *sec, int secnum)
{
  vldoor_t *door = new vldoor_t(raiseIn5Mins, sec, VDOORSPEED, VDOORWAIT, NULL);
  AddThinker(door);

  sec->special = 0;

  door->direction = 2;
  door->topcountdown = 5 * 60 * 35;
}



// ==========================================================================
//  SLIDE DOORS, ABANDONED TO THE MISTS OF TIME!!!
// ==========================================================================
