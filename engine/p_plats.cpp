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
// Revision 1.3  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:18:02  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.11  2002/09/20 22:41:32  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.9  2002/09/05 14:12:14  vberghol
// network code partly bypassed
//
// Revision 1.7  2002/08/17 21:21:50  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.6  2002/08/08 12:01:28  vberghol
// pian engine on valmis!
//
// Revision 1.5  2002/08/06 13:14:24  vberghol
// ...
//
// Revision 1.4  2002/07/23 19:21:42  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:20  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:15  vberghol
// Version 133 Experimental!
//
// Revision 1.5  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.4  2000/10/21 08:43:30  bpereira
// no message
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
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
//      Plats (i.e. elevator platforms) code, raising/lowering.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "p_spec.h" // class definitions
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"
#include "m_random.h"
#include "g_game.h"
#include "g_map.h"

// was T_PlatRaise
// Move a plat up and down
//
void plat_t::Think()
{
  result_e    res;

  switch (status)
    {
    case up:
      res = mp->T_MovePlane(sector, speed, high, crush, 0, 1);

      if (game.mode == gm_heretic && !(mp->maptic % (32*NEWTICRATERATIO)))
	S_StartSound ( & sector->soundorg, sfx_stnmov);

      if (type == raiseAndChange || type == raiseToNearestAndChange)
        {
	  if (!(mp->maptic % (8*NEWTICRATERATIO)))
	    S_StartSound(&sector->soundorg, sfx_stnmov);
        }

      if (res == crushed && (!crush))
        {
	  count = wait;
	  status = down;
	  S_StartSound(&sector->soundorg, sfx_pstart);
        }
      else
        {
	  if (res == pastdest)
            {
	      //SoM: 3/7/2000: Moved this little baby over.
	      // if not an instant toggle type, wait, make plat stop sound
	      if (type != toggleUpDn)
                {
                  count = wait;
                  status = waiting;
                  S_StartSound(&sector->soundorg, sfx_pstop);
                }
	      else // else go into stasis awaiting next toggle activation
                {
                  oldstatus = status;//jff 3/14/98 after action wait  
                  status = in_stasis;      //for reactivation of toggle
                }

	      switch (type)
                {
		case blazeDWUS:
		case downWaitUpStay:
		case raiseAndChange:
		case raiseToNearestAndChange:
		case genLift:
		  mp->RemoveActivePlat(this); //SoM: 3/7/2000: Much cleaner boom code.
		default:
		  break;
                }
            }
        }
      break;

    case      down:
      res = mp->T_MovePlane(sector,speed,low,false,0,-1);

      if (res == pastdest)
        {
	  //SoM: 3/7/2000: if not an instant toggle, start waiting, make plat stop sound
	  if (type!=toggleUpDn) 
            {                           
              count = wait;
              status = waiting;
              S_StartSound(&sector->soundorg,sfx_pstop);
            }
	  else //SoM: 3/7/2000: instant toggles go into stasis awaiting next activation
            {
              oldstatus = status;  
              status = in_stasis;      
            }

	  //jff 1/26/98 remove the plat if it bounced so it can be tried again
	  //only affects plats that raise and bounce
    
	  if (boomsupport)
            {
              switch(type)
		{
                case raiseAndChange:
                case raiseToNearestAndChange:
                  mp->RemoveActivePlat(this);
                default:
                  break;
		}
            }
        }
      else
	if (game.mode == gm_heretic && !(mp->maptic & 31))
	  S_StartSound ( & sector->soundorg, sfx_stnmov);

      break;

    case      waiting:
      if (!--count)
        {
	  if (sector->floorheight == low)
	    status = up;
	  else
	    status = down;
	  S_StartSound(&sector->soundorg,sfx_pstart);
        }
    case      in_stasis:
      break;
    }
}


// constructor
plat_t::plat_t(plattype_e ty, sector_t *sec, line_t *line)
{
  type = ty;
  sector = sec;
  crush = false;
  tag = line->tag;

  sec->floordata = this;

  //jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
  //going down forever -- default low to plat height when triggered
  low = sec->floorheight;

  switch (ty)
    {
    case raiseToNearestAndChange:
      speed = PLATSPEED/2;
      high = P_FindNextHighestFloor(sec,sec->floorheight);
      wait = 0;
      status = up;
      // NO MORE DAMAGE, IF APPLICABLE
      sec->special = 0;
      sec->oldspecial = 0;

      S_StartSound(&sec->soundorg,sfx_stnmov);
      break;

    case raiseAndChange:
      speed = PLATSPEED/2;
      wait = 0;
      status = up;

      S_StartSound(&sec->soundorg,sfx_stnmov);
      break;

    case downWaitUpStay:
      speed = PLATSPEED * 4;
      low = P_FindLowestFloorSurrounding(sec);

      if (low > sec->floorheight)
	low = sec->floorheight;

      high = sec->floorheight;
      wait = 35*PLATWAIT;
      status = down;
      S_StartSound(&sec->soundorg,sfx_pstart);
      break;

    case blazeDWUS:
      speed = PLATSPEED * 8;
      low = P_FindLowestFloorSurrounding(sec);

      if (low > sec->floorheight)
	low = sec->floorheight;

      high = sec->floorheight;
      wait = 35*PLATWAIT;
      status = down;
      S_StartSound(&sec->soundorg,sfx_pstart);
      break;

    case perpetualRaise:
      speed = PLATSPEED;
      low = P_FindLowestFloorSurrounding(sec);
      if (low > sec->floorheight)
	low = sec->floorheight;

      high = P_FindHighestFloorSurrounding(sec);
      if (high < sec->floorheight)
	high = sec->floorheight;

      wait = 35*PLATWAIT;
      status = plat_e(P_Random()&1);

      S_StartSound(&sec->soundorg,sfx_pstart);
      break;

    case toggleUpDn: //SoM: 3/7/2000: Instant toggle.
      speed = PLATSPEED;
      wait = 35*PLATWAIT;
      crush = true;

      // set up toggling between ceiling, floor inclusive
      low = sec->ceilingheight;
      high = sec->floorheight;
      status =  down;
      break;

    default:
      break;
    }
}


// was EV_DoPlat
// Do Platforms
//  "amount" is only used for SOME platforms.
//
int Map::EV_DoPlat(line_t *line, plattype_e type, int amount)
{
  plat_t*     plat;
  int         secnum = -1;
  int         rtn = 0;
  sector_t*   sec;

  //  Activate all <type> plats that are in_stasis
  switch(type)
    {
    case perpetualRaise:
      ActivateInStasisPlat(line->tag);
      break;

    case toggleUpDn:
      ActivateInStasisPlat(line->tag);
      rtn=1;
      break;

    default:
      break;
    }

  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (P_SectorActive(floor_special,sec)) //SoM: 3/7/2000: 
	continue;

      // Find lowest & highest floors around sector
      rtn = 1;
      plat = new plat_t(type, sec, line);
      AddThinker(plat);

      switch (type)
	{
	case raiseToNearestAndChange:
	  sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
	  break;
	case raiseAndChange:
	  plat->high = sec->floorheight + amount*FRACUNIT;
	  sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
	  break;
	default:
	  break;
	}
      AddActivePlat(plat);
    }
  return rtn;
}

// was P_ActivateInStasis
//SoM: 3/7/2000: Use boom limit removal
void Map::ActivateInStasisPlat(int tag)
{
  list<plat_t *>::iterator i;
  for (i = activeplats.begin(); i != activeplats.end(); i++)
    {
      plat_t *plat = *i;
      if (plat->tag == tag && plat->status == in_stasis) 
	{
	  if (plat->type == toggleUpDn)
	    plat->status = plat->oldstatus == up ? down : up;
	  else
	    plat->status = plat->oldstatus;
	}
    }

  /*
  platlist_t *pl;
  for (pl=activeplats; pl; pl=pl->next)
    {
      plat_t *plat = pl->plat;
      if (plat->tag == tag && plat->status == in_stasis) 
	{
	  if (plat->type==toggleUpDn)
	    plat->status = plat->oldstatus==up? down : up;
	  else
	    plat->status = plat->oldstatus;
	}
    }
  */
}

// was EV_StopPlat
//SoM: 3/7/2000: use Boom code insted.
int Map::EV_StopPlat(line_t* line)
{
  list<plat_t *>::iterator i;
  for (i = activeplats.begin(); i != activeplats.end(); i++)
    {
      plat_t *plat = *i;
      if (plat->status != in_stasis && plat->tag == line->tag)
	{
	  plat->oldstatus = plat->status;
	  plat->status = in_stasis;
	}
    }
  return 1;

  /*
  platlist_t *pl;
  for (pl=activeplats; pl; pl=pl->next)
    {
      plat_t *plat = pl->plat;
      if (plat->status != in_stasis && plat->tag == line->tag)
	{
	  plat->oldstatus = plat->status;
	  plat->status = in_stasis;
	}
    }
  return 1;
  */
}

// was P_AddActivePlat
//SoM: 3/7/2000: No more limits!
void Map::AddActivePlat(plat_t *plat)
{
  activeplats.push_front(plat);
  plat->li = activeplats.begin();
  // list iterators are not invalidated until erased

  /*
  platlist_t *list = (platlist_t *)malloc(sizeof *list);
  list->plat = plat;
  plat->list = list;
  if ((list->next = activeplats))
    list->next->prev = &list->next;
  list->prev = &activeplats;
  activeplats = list;
  */
}

// was P_RemoveActivePlat
//SoM: 3/7/2000: No more limits!
void Map::RemoveActivePlat(plat_t* plat)
{
  activeplats.erase(plat->li); // remove the pointer from list
  plat->sector->floordata = NULL;
  RemoveThinker(plat);

  /*
  platlist_t *list = plat->list;
  plat->sector->floordata = NULL; //jff 2/23/98 multiple thinkers
  RemoveThinker(plat);
  if ((*list->prev = list->next))
    list->next->prev = list->prev;
  free(list);
  */
}

// was P_RemoveAllActivePlats
//SoM: 3/7/2000: Removes all active plats.
void Map::RemoveAllActivePlats()
{
  activeplats.clear();
  /*
  while (activeplats)
    {
      platlist_t *next = activeplats->next;
      free(activeplats);
      activeplats = next;
    }
  */
}
