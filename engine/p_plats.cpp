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
// Revision 1.6  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.5  2003/05/30 13:34:46  smite-meister
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
// Revision 1.1.1.1  2002/11/16 14:18:02  hurdler
// Initial C++ version of Doom Legacy
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


// constructor
plat_t::plat_t(int ty, sector_t *sec, int t, fixed_t sp, int wt, fixed_t height)
{
  type = ty;
  sector = sec;
  tag = t;
  speed = sp;
  wait = wt;

  sec->floordata = this;

  //jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
  //going down forever -- default low to plat height when triggered
  fixed_t fl = low = sec->floorheight;

  switch (ty & TMASK)
    {
    case RelHeight:
      if (height > 0)
	{
	  high = low + height;
	  status = up;
	}
      else
	{
	  high = low;
	  low += height;
	  status = down;
	}
      break;

    case AbsHeight:
      if (height > low)
	{
	  high = height;
	  status = up;	 
	}
      else
	{
	  high = low;
	  low = height;
	  status = down;
	}
      break;

    case LnF:
      high = fl;
      low = P_FindLowestFloorSurrounding(sec) + height;
      if (low > fl)
	low = fl;
      status = down;
      break;

    case NLnF:
      high = fl;
      low = P_FindNextLowestFloor(sec, fl) + height;
      status = down;
      break;

    case NHnF:
      low = fl;
      high = P_FindNextHighestFloor(sec, fl) + height;
      status = up;
      break;

    case LnC:
      high = fl;
      low = P_FindLowestCeilingSurrounding(sec) + height;
      if (low > high)
	low = high;
      status = down;
      break;

    case LHF:
      type |= Perpetual;
      low = P_FindLowestFloorSurrounding(sec);
      if (low > fl)
	low = fl;
      high = P_FindHighestFloorSurrounding(sec) + height;
      if (high < fl)
	high = fl;
      status = status_e(P_Random() & 1); // Ugh. The bones have spoken.
      break;

    case CeilingToggle: // instant toggle (reversed targets!)
      type |= Perpetual;
      low = sec->ceilingheight;
      high = sec->floorheight;
      status =  down;
      break;

    default:
      I_Error("Unknown plat_t target %d!\n", ty);
      break;
    }

  // TODO sound sequences
  S_StartSound(&sec->soundorg,sfx_stnmov);
  //S_StartSound(&sec->soundorg,sfx_pstart);
}


int plat_t::Serialize(LArchive & a)
{
  return 0;
}

// was T_PlatRaise
// Move a plat up and down
//
void plat_t::Think()
{
  int  res;
  int  crush = 0;

  if ((type & TMASK) == CeilingToggle)
    crush = 10; // the only crushing "platform"

  switch (status)
    {
    case up:
      res = mp->T_MovePlane(sector, speed, high, crush, 0, 1);

      // TODO sequences...
      /*
      if (game.mode == gm_heretic && !(mp->maptic % (32*NEWTICRATERATIO)))
	S_StartSound ( & sector->soundorg, sfx_stnmov);

      if (type == raiseAndChange || type == raiseToNearestAndChange)
        {
	  if (!(mp->maptic % (8*NEWTICRATERATIO)))
	    S_StartSound(&sector->soundorg, sfx_stnmov);
        }
      */

      if (res == res_crushed && !crush)
        {
	  count = wait;
	  status = down;
	  S_StartSound(&sector->soundorg, sfx_pstart);
        }
      else if (res == res_pastdest)
	{
	  if (type & Returning)
	    {
              S_StartSound(&sector->soundorg,sfx_pstop);
	      mp->RemoveActivePlat(this); // done
	    }
	  else switch (type & TMASK)
	    {
	    case CeilingToggle:
	      // go into stasis awaiting next toggle activation
	      oldstatus = status;
	      status = in_stasis;
	      break;

	    default:
	      // wait before going back down
	      count = wait;
	      status = waiting;
	      S_StartSound(&sector->soundorg, sfx_pstop);
	      break;
	    }
        }
      break;

    case down:
      res = mp->T_MovePlane(sector,speed,low,false,0,-1);

      if (res == res_pastdest)
        {
	  if (type & Returning)
	    {
              S_StartSound(&sector->soundorg,sfx_pstop);
	      mp->RemoveActivePlat(this); // done
	    }
	  else switch (type & TMASK)
	    {
	    case CeilingToggle:
	      // go into stasis awaiting next activation
              oldstatus = status;  
              status = in_stasis;      
	      break;

	    default:
	      // start waiting, make plat stop sound
              count = wait;
              status = waiting;
              S_StartSound(&sector->soundorg,sfx_pstop);
	      break;
	    }
        }
      else if (game.mode == gm_heretic && !(mp->maptic & 31))
	S_StartSound ( & sector->soundorg, sfx_stnmov);
      break;

    case waiting:
      if (!--count)
        {
	  if (sector->floorheight == low)
	    status = up;
	  else
	    status = down;

	  if (!(type & Perpetual))
	    type |= Returning;
	  S_StartSound(&sector->soundorg,sfx_pstart);
        }

    case in_stasis:
    default:
      break;
    }
}



//
// was EV_DoPlat
//
int Map::EV_DoPlat(line_t *line, int type, fixed_t speed, int wait, fixed_t height)
{
  int  secnum = -1;
  int  rtn = 0;

  //  Activate all <type> plats that are in_stasis
  if (type & plat_t::Perpetual)
    {
      ActivateInStasisPlat(line->tag);
      rtn++;
    }

  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sector_t *sec = &sectors[secnum];

      if (P_SectorActive(floor_special,sec)) //SoM: 3/7/2000: 
	continue;

      // Find lowest & highest floors around sector
      rtn++;
      plat_t *plat = new plat_t(type, sec, line->tag, speed, wait, height);
      AddThinker(plat);

      if (type & plat_t::SetTexture)
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;

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
      if (plat->tag == tag && plat->status == plat_t::in_stasis) 
	{
	  if (plat->type == plat_t::CeilingToggle)
	    plat->status = plat->oldstatus == plat_t::up ? plat_t::down : plat_t::up;
	  else
	    plat->status = plat->oldstatus;
	}
    }
}

// was EV_StopPlat
//SoM: 3/7/2000: use Boom code insted.
int Map::EV_StopPlat(int tag)
{
  int rtn = 0;
  list<plat_t *>::iterator i;
  for (i = activeplats.begin(); i != activeplats.end(); i++)
    {
      plat_t *plat = *i;
      if (plat->status != plat_t::in_stasis && plat->tag == tag)
	{
	  TagFinished(plat->sector->tag);
	  plat->oldstatus = plat->status;
	  plat->status = plat_t::in_stasis;
	  rtn++;
	}
    }
  return rtn;
}

// was P_AddActivePlat
void Map::AddActivePlat(plat_t *plat)
{
  activeplats.push_front(plat);
  plat->li = activeplats.begin();
  // list iterators are not invalidated until erased
}

// was P_RemoveActivePlat
void Map::RemoveActivePlat(plat_t* plat)
{
  activeplats.erase(plat->li); // remove the pointer from list
  plat->sector->floordata = NULL;
  TagFinished(plat->sector->tag);
  RemoveThinker(plat);
}

// was P_RemoveAllActivePlats
//SoM: 3/7/2000: Removes all active plats.
void Map::RemoveAllActivePlats()
{
  activeplats.clear();
}
