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
// $Log$
// Revision 1.16  2005/03/04 16:23:07  smite-meister
// mp3, sector_t
//
// Revision 1.15  2004/12/05 14:46:32  smite-meister
// keybar
//
// Revision 1.14  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.13  2004/09/06 19:58:03  smite-meister
// Doom linedefs done!
//
// Revision 1.12  2004/04/25 16:26:49  smite-meister
// Doxygen
//
// Revision 1.11  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.10  2003/12/13 23:51:03  smite-meister
// Hexen update
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Plats (i.e. elevator platforms) code, raising/lowering.

#include "doomdef.h"

#include "g_game.h"
#include "g_map.h"

#include "p_spec.h" // class definitions

#include "m_random.h"
#include "sounds.h"


IMPLEMENT_CLASS(plat_t, sectoreffect_t);
plat_t::plat_t() {}

// constructor
plat_t::plat_t(Map *m, int ty, sector_t *sec, fixed_t sp, int wt, fixed_t height)
  : sectoreffect_t(m, sec)
{
  type = ty;
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
      low = sec->FindLowestFloorSurrounding() + height;
      if (low > fl)
	low = fl;
      status = down;
      break;

    case NLnF:
      high = fl;
      low = sec->FindNextLowestFloor(fl) + height;
      status = down;
      break;

    case NHnF:
      low = fl;
      high = sec->FindNextHighestFloor(fl) + height;
      status = up;
      break;

    case LnC:
      high = fl;
      low = sec->FindLowestCeilingSurrounding() + height;
      if (low > high)
	low = high;
      status = down;
      break;

    case LHF:
      low = sec->FindLowestFloorSurrounding();
      if (low > fl)
	low = fl;
      high = sec->FindHighestFloorSurrounding() + height;
      if (high < fl)
	high = fl;
      status = status_e(P_Random() & 1); // Ugh. The bones have spoken.
      break;

    case CeilingToggle: // instant toggle (reversed targets!)
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
  if (type & InStasis)
    return; // in stasis

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
	  // blocked, try again
	  status = down;
	  type &= ~Returning;
	  //mp->SN_StopSequence(&sector->soundorg);
	  //mp->SN_StartSequence(&sector->soundorg, SEQ_PLAT + sector->seqType);
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
	      status = down;
	      type |= InStasis;
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
              status = up;
	      type |= InStasis;  
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

      plat = new plat_t(this, type, sec, speed, wait, height);

      if (type & plat_t::SetTexture)
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
      if (type & plat_t::ZeroSpecial)
	sec->special = 0;

      AddActivePlat(plat);
      return 1;
    }

  //  Activate all <type> plats that are in_stasis
  if (type & plat_t::Perpetual)
    rtn += ActivateInStasisPlat(tag);

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (sec->Active(sector_t::floor_special))
	continue;

      rtn++;
      plat_t *plat = new plat_t(this, type, sec, speed, wait, height);

      if (type & plat_t::SetTexture)
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
      if (type & plat_t::ZeroSpecial)
	sec->special = 0;

      AddActivePlat(plat);
    }
  return rtn;
}



int Map::ActivateInStasisPlat(int tag)
{
  int rtn = 0;
  list<plat_t *>::iterator i;
  for (i = activeplats.begin(); i != activeplats.end(); i++)
    {
      plat_t *plat = *i;
      if (plat->sector->tag == tag && plat->type & plat_t::InStasis)
	{
	  SN_StartSequence(&plat->sector->soundorg, SEQ_PLAT + plat->sector->seqType);
	  plat->type &= ~plat_t::InStasis;
	  rtn++;
	}
    }
  return rtn;
}


int Map::EV_StopPlat(int tag)
{
  int rtn = 0;
  list<plat_t *>::iterator i;
  for (i = activeplats.begin(); i != activeplats.end(); i++)
    {
      plat_t *plat = *i;
      if (!(plat->type & plat_t::InStasis) && plat->sector->tag == tag)
	{
	  SN_StopSequence(&plat->sector->soundorg);
	  TagFinished(plat->sector->tag);
	  plat->type |= plat_t::InStasis;
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
