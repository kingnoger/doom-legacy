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
// Revision 1.11  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.10  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.9  2003/11/30 00:09:45  smite-meister
// bugfixes
//
// Revision 1.8  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.7  2003/11/12 11:07:22  smite-meister
// Serialization done. Map progression.
//
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
#include "sounds.h"
#include "z_zone.h"
#include "m_random.h"
#include "g_game.h"
#include "g_map.h"


IMPLEMENT_CLASS(plat_t, "Platform");
plat_t::plat_t() {}

// constructor
plat_t::plat_t(Map *m, int ty, sector_t *sec, int t, fixed_t sp, int wt, fixed_t height)
  : sectoreffect_t(m, sec)
{
  type = ty;
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
      return;

    default:
      I_Error("Unknown plat_t target %d!\n", ty);
      break;
    }

  mp->SN_StartSequence(&sec->soundorg, SEQ_PLAT + sec->seqType);
}


// Move a plat up and down
void plat_t::Think()
{
  int  res;
  int  crush = 0;

  if ((type & TMASK) == CeilingToggle)
    crush = 10; // the only crushing "platform"

  switch (status)
    {
    case up:
      res = mp->T_MovePlane(sector, speed, high, crush, 0);

      if (res == res_crushed && !crush)
        {
	  count = wait;
	  status = down;
	  mp->SN_StartSequence(&sector->soundorg, SEQ_PLAT + sector->seqType);
        }
      else if (res == res_pastdest)
	{
	  if (type & Returning)
	    {
	      mp->SN_StopSequence(&sector->soundorg);
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
	      mp->SN_StopSequence(&sector->soundorg);
	      break;
	    }
        }
      break;

    case down:
      res = mp->T_MovePlane(sector,-speed,low,false,0);

      if (res == res_pastdest)
        {
	  if (type & Returning)
	    {
	      mp->SN_StopSequence(&sector->soundorg);
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
	      mp->SN_StopSequence(&sector->soundorg);
	      break;
	    }
        }
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

	  mp->SN_StartSequence(&sector->soundorg, SEQ_PLAT + sector->seqType);
        }

    case in_stasis:
    default:
      break;
    }
}



// moving platforms (lifts)
int Map::EV_DoPlat(int tag, line_t *line, int type, fixed_t speed, int wait, fixed_t height)
{
  int  secnum = -1;
  int  rtn = 0;
  sector_t *sec;
  plat_t *plat;

  if (!tag)
    {
      // tag == 0, plat is on the other side of the linedef
      if (line->sidenum[1] == -1)
	return 0;

      sec = sides[line->sidenum[1]].sector;

      if (sec->floordata)
	return 0;

      plat = new plat_t(this, type, sec, tag, speed, wait, height);

      if (type & plat_t::SetTexture)
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
      AddActivePlat(plat);
      return 1;
    }

  //  Activate all <type> plats that are in_stasis
  if (type & plat_t::Perpetual)
    {
      ActivateInStasisPlat(tag);
      rtn++;
    }

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (P_SectorActive(floor_special,sec)) //SoM: 3/7/2000: 
	continue;

      // Find lowest & highest floors around sector
      rtn++;
      plat_t *plat = new plat_t(this, type, sec, tag, speed, wait, height);

      if (type & plat_t::SetTexture)
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;

      AddActivePlat(plat);
    }
  return rtn;
}



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


void Map::AddActivePlat(plat_t *plat)
{
  activeplats.push_front(plat);
  plat->li = activeplats.begin();
  // list iterators are not invalidated until erased
}


void Map::RemoveActivePlat(plat_t* plat)
{
  activeplats.erase(plat->li); // remove the pointer from list
  plat->sector->floordata = NULL;
  TagFinished(plat->sector->tag);
  RemoveThinker(plat);
}


void Map::RemoveAllActivePlats()
{
  activeplats.clear();
}
