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
// Revision 1.8  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.7  2003/11/12 11:07:19  smite-meister
// Serialization done. Map progression.
//
// Revision 1.6  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.5  2003/05/30 13:34:44  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.4  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.3  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.2  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.1.1.1  2002/11/16 14:17:54  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:  
//      Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "g_game.h"
#include "g_map.h" // Map class
#include "p_spec.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"


// ==========================================================================
//                              CEILINGS
// ==========================================================================

IMPLEMENT_CLASS(ceiling_t, "Ceiling");
ceiling_t::ceiling_t() {}

int ceiling_t::ceilmovesound = 0;

// constructor
ceiling_t::ceiling_t(int ty, sector_t *sec, fixed_t usp, fixed_t dsp, int cru, fixed_t height)
{
  // normal ceilings and crushers could be two different classes, but...
  type = ty;
  sector = sec;
  tag = sec->tag;
  upspeed = usp;
  oldspeed = downspeed = dsp;
  crush = cru;
  sec->ceilingdata = this;

  // target?
  switch (type & TMASK)
    {
    case RelHeight:
      topheight = bottomheight = sec->ceilingheight + height;
      direction = (height > 0) ? 1 : -1;
      break;

    case AbsHeight:
      if (sec->ceilingheight <= height)
	{
	  direction = 1;
	  topheight = height;	  
	}
      else
	{
	  direction = -1;
	  bottomheight = height;
	}
      break;

    case Floor:
      bottomheight = sec->floorheight + height;
      direction = -1;
      break;

    case HnC:
      topheight = P_FindHighestCeilingSurrounding(sec);
      direction = 1;
      break;

    case LnC:
      bottomheight = P_FindLowestCeilingSurrounding(sec);
      direction = -1;
      break;

    case HnF:
      bottomheight = P_FindHighestFloorSurrounding(sec);
      direction = -1;
      break;

    case Crusher:
    case CrushOnce:
      topheight = sec->ceilingheight;
      bottomheight = sec->floorheight + height;
      direction = -1;
      break;

    default:
      break;
    }
}


//
// was T_MoveCeiling
//
void ceiling_t::Think()
{
  int res;

  switch (direction)
    {
    case 0:
      // IN STASIS
      break;
    case 1:
      // UP
      res = mp->T_MovePlane(sector, upspeed, topheight, false, 1, direction);

      if (!(mp->maptic % (8*NEWTICRATERATIO)))
        {
	  // TODO replace with sound sequences
	  if (!(type & Silent))
	    S_StartSound(&sector->soundorg, ceilmovesound);
        }

      if (res == res_pastdest)
        {
	  // movers with texture/special change: change the texture then get removed
	  if (type & SetSpecial)
	    sector->special = newspecial;

	  if (type & SetTexture)
	    sector->ceilingpic = texture;

	  switch (type & TMASK)
            {
	    case Crusher:
	      if (type & Silent)
		S_StartSound(&sector->soundorg, sfx_pstop);
	      direction = -1;
	      break;
	      
	    default:
	      //SN_StopSequence((mobj_t *)&ceiling->sector->soundorg);
	      mp->RemoveActiveCeiling(this);
	      break;
            }
        }
      break;

    case -1:
      // DOWN
      res = mp->T_MovePlane(sector, downspeed, bottomheight, crush,1,direction);

      if (!(mp->maptic % (8*NEWTICRATERATIO)))
        {
	  if (!(type & Silent))
	    S_StartSound(&sector->soundorg, ceilmovesound);
        }

      if (res == res_pastdest)
        {
	  if (type & SetSpecial)
	    sector->special = newspecial;

	  if (type & SetTexture)
	    sector->ceilingpic = texture;

	  switch (type & TMASK)
	    {
	    case Crusher:
	    case CrushOnce:
	      if (type & Silent)
		S_StartSound(&sector->soundorg, sfx_pstop);

	      downspeed = oldspeed; // reset downspeed
	      direction = 1;
	      break;

              // all other cases, just remove the active ceiling
	    default:
	      //SN_StopSequence((mobj_t *)&ceiling->sector->soundorg);
	      mp->RemoveActiveCeiling(this);
	      break;
            }
        }
      else if (res == res_crushed)
	{
	  // slow down slow crushers on obstacle (unless they are really fast)
	  switch (type & TMASK)
	    {
	    case Crusher:
	    case CrushOnce:
	      if (game.mode != gm_hexen && oldspeed < CEILSPEED*3)
		downspeed = CEILSPEED / 8;
	      
	    default:
	      break;
	    }
	}
      break;
    }
}


//
// was EV_DoCeiling
// Move a ceiling up/down and all around!
//
int Map::EV_DoCeiling(int tag, int type, fixed_t uspeed, fixed_t dspeed, int crush, fixed_t height)
{
  sector_t   *sec;

  int secnum = -1;
  int rtn = 0;

  if (!tag)
    return false;

  //  Reactivate in-stasis ceilings...for certain types.
  // This restarts a crusher after it has been stopped
  switch (type & ceiling_t::TMASK)
    {
    case ceiling_t::Crusher:
    case ceiling_t::CrushOnce:
      rtn += ActivateInStasisCeiling(tag); //SoM: Return true if the crusher is activated
    default:
      break;
    }

  while ((secnum = FindSectorFromTag(tag, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (P_SectorActive(ceiling_special,sec))  //SoM: 3/6/2000
	continue;

      // new ceiling thinker
      rtn++;
      ceiling_t *ceiling = new ceiling_t(type, sec, uspeed, dspeed, crush, height);

      AddThinker(ceiling);
      AddActiveCeiling(ceiling);
    }
  return rtn;
}



// was P_AddActiveCeiling
// Add an active ceiling
//
void Map::AddActiveCeiling(ceiling_t *ceiling)
{
  activeceilings.push_front(ceiling);
  ceiling->li = activeceilings.begin();
  // list iterators are only invalidated when erased
}



// was P_RemoveActiveCeiling
// Remove a ceiling's thinker
//
void Map::RemoveActiveCeiling(ceiling_t *ceiling)
{
  activeceilings.erase(ceiling->li); // remove the pointer from list
  ceiling->sector->ceilingdata = NULL;
  RemoveThinker(ceiling);
  TagFinished(ceiling->sector->tag);
}


//
// Restart a ceiling that's in-stasis
//
int Map::ActivateInStasisCeiling(int tag)
{
  int rtn = 0;
  list<ceiling_t *>::iterator i;
  for (i = activeceilings.begin(); i != activeceilings.end(); i++)
    {
      ceiling_t *ceil = *i;
      if (ceil->tag == tag && ceil->direction == 0)
	{
	  ceil->direction = ceil->olddirection;
	  ceil->sector->ceilingdata = ceil; // what if it is already in use?
	  rtn++;
	}
    }
  return rtn;
}



//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
int Map::EV_StopCeiling(int tag)
{
  int rtn = 0;
  list<ceiling_t *>::iterator i;
  for (i = activeceilings.begin(); i != activeceilings.end(); i++)
    {
      ceiling_t *ceil = *i;
      if (ceil->direction != 0 && ceil->tag == tag)
	{
	  //SN_StopSequence((mobj_t*)&ceil->sector->soundorg);
	  ceil->olddirection = ceil->direction;
	  ceil->direction = 0;
	  ceil->sector->ceilingdata = NULL;
	  TagFinished(ceil->sector->tag);
	  rtn++;
	}
    }
  return rtn;
}



//
// P_RemoveAllActiveCeilings()
// Removes all ceilings from the active ceiling list
// 
void Map::RemoveAllActiveCeilings()
{
  activeceilings.clear();
}
