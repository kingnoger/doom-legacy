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
// Revision 1.1  2002/11/16 14:17:54  hurdler
// Initial revision
//
// Revision 1.16  2002/09/20 22:41:28  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.14  2002/09/06 17:18:32  vberghol
// added most of the changes up to RC2
//
// Revision 1.13  2002/09/05 14:12:14  vberghol
// network code partly bypassed
//
// Revision 1.11  2002/08/21 16:58:31  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.10  2002/08/17 21:21:44  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.9  2002/08/06 13:14:20  vberghol
// ...
//
// Revision 1.8  2002/07/26 19:23:03  vberghol
// a little something
//
// Revision 1.7  2002/07/23 19:21:39  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.6  2002/07/18 19:16:37  vberghol
// renamed a few files
//
// Revision 1.5  2002/07/13 17:55:53  vberghol
// jäi kartan liikkuviin osiin... p_doors.cpp
//
// Revision 1.4  2002/07/12 19:21:37  vberghol
// hop
//
// Revision 1.3  2002/07/01 21:00:15  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:12  vberghol
// Version 133 Experimental!
//
// Revision 1.8  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.7  2000/10/21 08:43:30  bpereira
// no message
//
// Revision 1.6  2000/09/28 20:57:16  bpereira
// no message
//
// Revision 1.5  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
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
//      Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "g_map.h" // Map class
#include "p_spec.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"
#include "g_game.h" // FIXME! temp

// ==========================================================================
//                              CEILINGS
// ==========================================================================

int ceiling_t::ceilmovesound = 0;

// constructor
ceiling_t::ceiling_t(ceiling_e ty, sector_t *s, int t)
{
  type = ty;
  sector = s;
  tag = t;
  crush = false;
  s->ceilingdata = this;
  
  switch (type)
    {
    case fastCrushAndRaise:
      crush = true;
      topheight = s->ceilingheight;
      bottomheight = s->floorheight + (8*FRACUNIT);
      direction = -1;
      speed = CEILSPEED * 2;
      break;

    case silentCrushAndRaise:
    case crushAndRaise:
      crush = true;
      topheight = s->ceilingheight;
    case lowerAndCrush:
    case lowerToFloor:
      bottomheight = s->floorheight;
      if (type != lowerToFloor)
	bottomheight += 8*FRACUNIT;
      direction = -1;
      speed = CEILSPEED;
      break;

    case raiseToHighest:
      topheight = P_FindHighestCeilingSurrounding(s);
      direction = 1;
      speed = CEILSPEED;
      break;

      //SoM: 3/6/2000: Added Boom types
    case lowerToLowest:
      bottomheight = P_FindLowestCeilingSurrounding(s);
      direction = -1;
      speed = CEILSPEED;
      break;

    case lowerToMaxFloor:
      bottomheight = P_FindHighestFloorSurrounding(s);
      direction = -1;
      speed = CEILSPEED;
      break;

      // Instant-raise SSNTails 06-13-2002
    case instantRaise:
      topheight = P_FindHighestCeilingSurrounding(s);
      direction = 1;
      speed = MAXINT/2; // Go too fast and you'll cause problems...
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
  result_e    res;
  //Map *mp = game.maps[0]; // FIXME! must get right Map* from somewhere!

  switch(direction)
    {
    case 0:
      // IN STASIS
      break;
    case 1:
      // UP
      res = mp->T_MovePlane(sector, speed, topheight, false, 1, direction);

      if (!(mp->maptic % (8*NEWTICRATERATIO)))
        {
	  switch(type)
            {
	    case silentCrushAndRaise:
	    case genSilentCrusher:
	      break;
	    default:
	      S_StartSound(&sector->soundorg, ceilmovesound);
	      // ?
	      break;
            }
        }

      if (res == pastdest)
        {
	  switch(type)
            {
	    case raiseToHighest:
	    case genCeiling: //SoM: 3/6/2000
	      mp->RemoveActiveCeiling(this);
	      break;

              // SoM: 3/6/2000: movers with texture change, change the texture then get removed
	    case genCeilingChgT:
	    case genCeilingChg0:
	      sector->special = newspecial;
	      sector->oldspecial = oldspecial;
	    case genCeilingChg:
	      sector->ceilingpic = texture;
	      mp->RemoveActiveCeiling(this);
	      break;

	    case silentCrushAndRaise:
	      S_StartSound(&sector->soundorg, sfx_pstop);
	    case fastCrushAndRaise:
	    case genCrusher: // SoM: 3/6/2000
	    case genSilentCrusher:
	    case crushAndRaise:
	      direction = -1;
	      break;

	    default:
	      break;
            }
        }
      break;

    case -1:
      // DOWN
      res = mp->T_MovePlane(sector, speed, bottomheight, crush,1,direction);

      if (!(mp->maptic % (8*NEWTICRATERATIO)))
        {
	  switch(type)
            {
	    case silentCrushAndRaise:
	    case genSilentCrusher:
	      break;
	    default:
	      S_StartSound(&sector->soundorg, ceilmovesound);
            }
        }

      if (res == pastdest)
        {
	  switch(type) //SoM: 3/6/2000: Use boom code
            {
	    case genSilentCrusher:
	    case genCrusher:
	      if (oldspeed<CEILSPEED*3)
		speed = oldspeed;
	      direction = 1;
	      break;
    
              // make platform stop at bottom of all crusher strokes
              // except generalized ones, reset speed, start back up
	    case silentCrushAndRaise:
	      S_StartSound(&sector->soundorg, sfx_pstop);
	    case crushAndRaise: 
	      speed = CEILSPEED;
	    case fastCrushAndRaise:
	      direction = 1;
	      break;
              
              // in the case of ceiling mover/changer, change the texture
              // then remove the active ceiling
	    case genCeilingChgT:
	    case genCeilingChg0:
	      sector->special = newspecial;
	      //jff add to fix bug in special transfers from changes
	      sector->oldspecial = oldspecial;
	    case genCeilingChg:
	      sector->ceilingpic = texture;
	      mp->RemoveActiveCeiling(this);
	      break;
    
              // all other case, just remove the active ceiling
	    case lowerAndCrush:
	    case lowerToFloor:
	    case lowerToLowest:
	    case lowerToMaxFloor:
	    case genCeiling:
	      mp->RemoveActiveCeiling(this);
	      break;
    
	    default:
	      break;
            }
        }
      else // ( res != pastdest )
        {
	  if (res == crushed)
            {
	      switch(type)
                {
                  //SoM: 2/6/2000
                  // slow down slow crushers on obstacle
		case genCrusher:  
		case genSilentCrusher:
		  if (oldspeed < CEILSPEED*3)
		    speed = CEILSPEED / 8;
		  break;

		case silentCrushAndRaise:
		case crushAndRaise:
		case lowerAndCrush:
		  speed = CEILSPEED / 8;
		  break;

		default:
		  break;
                }
            }
        }
      break;
    }
}


//
// was EV_DoCeiling
// Move a ceiling up/down and all around!
//
int Map::EV_DoCeiling(line_t *line, ceiling_e type)
{
  int         secnum;
  int         rtn;
  sector_t   *sec;
  ceiling_t  *ceiling;

  secnum = -1;
  rtn = 0;

  //  Reactivate in-stasis ceilings...for certain types.
  // This restarts a crusher after it has been stopped
  switch(type)
    {
    case fastCrushAndRaise:
    case silentCrushAndRaise:
    case crushAndRaise:
      rtn = ActivateInStasisCeiling(line); //SoM: Return true if the crusher is activated
    default:
      break;
    }

  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];

      if (P_SectorActive(ceiling_special,sec))  //SoM: 3/6/2000
	continue;

      // new ceiling thinker
      rtn = 1;
      ceiling = new ceiling_t(type, sec, sec->tag);

      AddThinker(ceiling);
      AddActiveCeiling(ceiling);
    }
  return rtn;
}


// was P_AddActiveCeiling
// Add an active ceiling
//
//SoM: 3/6/2000: Take advantage of the new Boom method for active ceilings.

void Map::AddActiveCeiling(ceiling_t *ceiling)
{
  activeceilings.push_front(ceiling);
  ceiling->li = activeceilings.begin();
  // list iterators are only invalidated when erased
  /*
  ceilinglist_t *list = (ceilinglist_t *)malloc(sizeof *list);
  list->ceiling = ceiling;
  ceiling->list = list;
  if ((list->next = activeceilings))
    list->next->prev = &list->next;
  list->prev = &activeceilings;
  activeceilings = list;
  */
}



// was P_RemoveActiveCeiling
// Remove a ceiling's thinker
//
// SoM: 3/6/2000 :Use improved Boom code.
void Map::RemoveActiveCeiling(ceiling_t *ceiling)
{
  activeceilings.erase(ceiling->li); // remove the pointer from list
  ceiling->sector->ceilingdata = NULL;
  RemoveThinker(ceiling);

  /*
  ceilinglist_t *list = ceiling->list;
  ceiling->sector->ceilingdata = NULL;  //jff 2/22/98
  RemoveThinker(ceiling);
  if ((*list->prev = list->next))
    list->next->prev = list->prev;
  free(list);
  */
}



//
// Restart a ceiling that's in-stasis
//
//SoM: 3/6/2000: Use improved boom code
int Map::ActivateInStasisCeiling(line_t *line)
{
  int rtn = 0;
  list<ceiling_t *>::iterator i;
  for (i = activeceilings.begin(); i != activeceilings.end(); i++)
    {
      ceiling_t *ceil = *i;
      if (ceil->tag == line->tag && ceil->direction == 0)
	{
	  ceil->direction = ceil->olddirection;
	  rtn = 1;
	}
    }
  return rtn;

  /*
  ceilinglist_t *cl;
  for (cl=activeceilings; cl; cl=cl->next)
  {
    ceiling_t *ceiling = cl->ceiling;
    if (ceiling->tag == line->tag && ceiling->direction == 0)
    {
      ceiling->direction = ceiling->olddirection;
      //ceiling->thinker.function.acp1 = (actionf_p1) T_MoveCeiling;
      rtn=1;
    }
  }
  return rtn;
  */
}



//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
//SoM: 3/6/2000: use improved Boom code
int Map::EV_CeilingCrushStop(line_t *line)
{
  int rtn = 0;
  list<ceiling_t *>::iterator i;
  for (i = activeceilings.begin(); i != activeceilings.end(); i++)
    {
      ceiling_t *ceil = *i;
      if (ceil->direction != 0 && ceil->tag == line->tag)
	{
	  ceil->olddirection = ceil->direction;
	  ceil->direction = 0;
	  //ceiling->thinker.function.acv = (actionf_v)NULL; // FIXME put in stasis
	  rtn = 1;
	}
    }
  return rtn;

  /*
  ceilinglist_t *cl;
  for (cl=activeceilings; cl; cl=cl->next)
  {
    ceiling_t *ceiling = cl->ceiling;
    if (ceiling->direction != 0 && ceiling->tag == line->tag)
    {
      ceiling->olddirection = ceiling->direction;
      ceiling->direction = 0;
      //ceiling->thinker.function.acv = (actionf_v)NULL; // FIXME put in stasis
      rtn = 1;
    }
  }
  return rtn;
  */
}



// SoM: 3/6/2000: Extra, boom only function.
//
// P_RemoveAllActiveCeilings()
//
// Removes all ceilings from the active ceiling list
//
// Passed nothing, returns nothing
//
void Map::RemoveAllActiveCeilings()
{
  activeceilings.clear();
  /*
  while (activeceilings)
  {  
    ceilinglist_t *next = activeceilings->next;
    free(activeceilings);
    activeceilings = next;
  }
  */
}
