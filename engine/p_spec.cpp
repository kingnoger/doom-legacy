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
// Revision 1.26  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.24  2004/01/02 14:25:01  smite-meister
// cleanup
//
// Revision 1.23  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.22  2003/12/23 18:06:06  smite-meister
// Hexen stairbuilders. Moving geometry done!
//
// Revision 1.21  2003/12/21 12:29:09  smite-meister
// bugfixes
//
// Revision 1.20  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.19  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.18  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.17  2003/11/30 00:09:47  smite-meister
// bugfixes
//
// Revision 1.16  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.15  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.14  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.13  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.12  2003/05/30 13:34:46  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.10  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.9  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.8  2003/04/14 08:58:27  smite-meister
// Hexen maps load.
//
// Revision 1.7  2003/04/08 09:46:06  smite-meister
// Bugfixes
//
// Revision 1.6  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.5  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.4  2003/03/08 16:07:09  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/16 22:11:57  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:09  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Map geometry utility functions
//   Line tag hashing
//   Linedef specials
//   Sector specials
//   Scrollers, friction, pushers
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"

#include "g_game.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "d_netcmd.h"

#include "p_spec.h"
#include "p_maputl.h"

#include "m_bbox.h" // bounding boxes
#include "m_random.h"

#include "r_data.h"
#include "r_main.h"
#include "r_sky.h"

#include "sounds.h"
#include "t_script.h"
#include "p_acs.h"

#include "hardware/hw3sound.h"

#include "w_wad.h"
#include "z_zone.h"
#include "tables.h"


//SoM: Enable Boom features?
int boomsupport = 1;
int variable_friction = 1;
int allow_pushers = 1;


//========================================================
//  Sector effects: base class for most moving geometry
//========================================================

IMPLEMENT_CLASS(sectoreffect_t, Thinker);
sectoreffect_t::sectoreffect_t() {}

sectoreffect_t::sectoreffect_t(Map *m, sector_t *s)
{
  m->AddThinker(this); // when a sectoreffect is constructed, it is immediately added to the Thinker list.
  sector = s;
}


//========================================================
// UTILITIES
//========================================================

// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.

side_t *Map::getSide(int sec, int line, int side)
{
  return &sides[ (sectors[sec].lines[line])->sidenum[side] ];
}


// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.

sector_t *Map::getSector(int sec, int line, int side)
{
  return sides[ (sectors[sec].lines[line])->sidenum[side] ].sector;
}


// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.

int Map::twoSided(int sec, int line)
{
  return boomsupport ?
    ((sectors[sec].lines[line])->sidenum[1] != -1)
    :
    ((sectors[sec].lines[line])->flags & ML_TWOSIDED);
}


// Return sector_t * of sector next to current.
// NULL if not two-sided line

sector_t *getNextSector(line_t *line, sector_t *sec)
{
  if (!boomsupport)
    {
      if (!(line->flags & ML_TWOSIDED))
	return NULL;
    }

  if (line->frontsector == sec)
    {
      if (!boomsupport || line->backsector!=sec)
	return line->backsector;
      else
	return NULL;
    }
  return line->frontsector;
}


// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS

fixed_t P_FindLowestFloorSurrounding(sector_t* sec)
{
  int                 i;
  line_t*             check;
  sector_t*           other;
  fixed_t             floor = sec->floorheight;

  for (i=0 ;i < sec->linecount ; i++)
    {
      check = sec->lines[i];
      other = getNextSector(check,sec);

      if (!other)
	continue;

      if (other->floorheight < floor)
	floor = other->floorheight;
    }
  return floor;
}


// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS

fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
  int                 i;
  line_t*             check;
  sector_t*           other;
  fixed_t             floor = -500*FRACUNIT;
  int                 foundsector = 0;


  for (i=0 ;i < sec->linecount ; i++)
    {
      check = sec->lines[i];
      other = getNextSector(check,sec);

      if (!other)
	continue;

      if (other->floorheight > floor || !foundsector)
	floor = other->floorheight;

      if(!foundsector)
	foundsector = 1;
    }
  return floor;
}


// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
// Rewritten by Lee Killough to avoid fixed array and to be faster

fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
	other->floorheight > currentheight)
      {
	int height = other->floorheight;
	while (++i < sec->linecount)
	  if ((other = getNextSector(sec->lines[i],sec)) &&
	      other->floorheight < height &&
	      other->floorheight > currentheight)
	    height = other->floorheight;
	return height;
      }
  return currentheight;
}


////////////////////////////////////////////////////
// SoM: Start new Boom functions
////////////////////////////////////////////////////

// Passed a sector and a floor height, returns the fixed point value
// of the largest floor height in a surrounding sector smaller than
// the floor height passed. If no such height exists the floorheight
// passed is returned.

fixed_t P_FindNextLowestFloor(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
	other->floorheight < currentheight)
      {
	int height = other->floorheight;
	while (++i < sec->linecount)
	  if ((other = getNextSector(sec->lines[i],sec)) &&
	      other->floorheight > height &&
	      other->floorheight < currentheight)
	    height = other->floorheight;
	return height;
      }
  return currentheight;
}


// Passed a sector and a ceiling height, returns the fixed point value
// of the largest ceiling height in a surrounding sector smaller than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.

fixed_t P_FindNextLowestCeiling(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
        other->ceilingheight < currentheight)
      {
	int height = other->ceilingheight;
	while (++i < sec->linecount)
	  if ((other = getNextSector(sec->lines[i],sec)) &&
	      other->ceilingheight > height &&
	      other->ceilingheight < currentheight)
	    height = other->ceilingheight;
	return height;
      }
  return currentheight;
}



// Passed a sector and a ceiling height, returns the fixed point value
// of the smallest ceiling height in a surrounding sector larger than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.

fixed_t P_FindNextHighestCeiling(sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  for (i=0 ;i < sec->linecount ; i++)
    if ((other = getNextSector(sec->lines[i],sec)) &&
	other->ceilingheight > currentheight)
      {
	int height = other->ceilingheight;
	while (++i < sec->linecount)
	  if ((other = getNextSector(sec->lines[i],sec)) &&
	      other->ceilingheight < height &&
	      other->ceilingheight > currentheight)
	    height = other->ceilingheight;
	return height;
      }
  return currentheight;
}

////////////////////////////
// End New Boom functions
////////////////////////////



//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindLowestCeilingSurrounding(sector_t* sec)
{
  int                 i;
  line_t*             check;
  sector_t*           other;
  fixed_t             height = MAXINT;
  int                 foundsector = 0;

  if (boomsupport) height = 32000*FRACUNIT; //SoM: 3/7/2000: Remove ovf
                                              
  for (i=0 ;i < sec->linecount ; i++)
    {
      check = sec->lines[i];
      other = getNextSector(check,sec);

      if (!other)
	continue;

      if (other->ceilingheight < height || !foundsector)
	height = other->ceilingheight;

      if(!foundsector)
	foundsector = 1;
    }
  return height;
}


//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindHighestCeilingSurrounding(sector_t* sec)
{
  int         i;
  line_t*     check;
  sector_t*   other;
  fixed_t     height = 0;
  int         foundsector = 0;

  for (i=0 ;i < sec->linecount ; i++)
    {
      check = sec->lines[i];
      other = getNextSector(check,sec);

      if (!other)
	continue;

      if (other->ceilingheight > height || !foundsector)
	height = other->ceilingheight;

      if(!foundsector)
	foundsector = 1;
    }
  return height;
}


// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
// TODO in this and FindShortestUpperAround: replace all indices with pointers
// (in line_t, replace sidenum with side_t *)
fixed_t Map::FindShortestLowerAround(sector_t *sec)
{
  int minsize = MAXINT;
  int secnum = sec - sectors;

  if (boomsupport)
    minsize = 32000;

  for (int i = 0; i < sec->linecount; i++)
    {
      if (twoSided(secnum, i))
	{
	  side_t *side = getSide(secnum,i,0);
	  if (side->bottomtexture > 0)
	    if (tc[side->bottomtexture]->height < minsize)
	      minsize = tc[side->bottomtexture]->height;
	  side = getSide(secnum,i,1);
	  if (side->bottomtexture > 0)
	    if (tc[side->bottomtexture]->height < minsize)
	      minsize = tc[side->bottomtexture]->height;
	}
    }
  return minsize << FRACBITS;
}



// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.

fixed_t Map::FindShortestUpperAround(sector_t *sec)
{
  int minsize = MAXINT;
  int secnum = sec - sectors;

  if (boomsupport)
    minsize = 32000;

  for (int i = 0; i < sec->linecount; i++)
    {
      if (twoSided(secnum, i))
	{
	  side_t *side = getSide(secnum,i,0);
	  if (side->toptexture > 0)
	    if (tc[side->toptexture]->height < minsize)
	      minsize = tc[side->toptexture]->height;
	  side = getSide(secnum,i,1);
	  if (side->toptexture > 0)
	    if (tc[side->toptexture]->height < minsize)
	      minsize = tc[side->toptexture]->height;
	}
    }
  return minsize << FRACBITS;
}




// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL

sector_t *Map::FindModelFloorSector(fixed_t floordestheight, sector_t *sec)
{
  int i, secnum;
  //sector_t *sec=NULL;
  int linecount;

  secnum = sec-sectors;
  //sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < (!boomsupport && sec->linecount<linecount?
                   sec->linecount : linecount); i++)
    {
      if ( twoSided(secnum, i) )
	{
	  if (getSide(secnum,i,0)->sector-sectors == secnum)
	    sec = getSector(secnum,i,1);
	  else
	    sec = getSector(secnum,i,0);

	  if (sec->floorheight == floordestheight)
	    return sec;
	}
    }
  return NULL;
}



// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL

sector_t *Map::FindModelCeilingSector(fixed_t ceildestheight, sector_t *sec)
{
  int i, secnum;
  //sector_t *sec=NULL;
  int linecount;

  secnum = sec-sectors;
  //sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < (!boomsupport && sec->linecount<linecount?
                   sec->linecount : linecount); i++)
    {
      if ( twoSided(secnum, i) )
	{
	  if (getSide(secnum,i,0)->sector-sectors == secnum)
	    sec = getSector(secnum,i,1);
	  else
	    sec = getSector(secnum,i,0);

	  if (sec->ceilingheight == ceildestheight)
	    return sec;
	}
    }
  return NULL;
}



// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//SoM: 3/7/2000: Killough wrote this to improve the process.
int Map::FindSectorFromLineTag(line_t *line, int start)
{
  start = (start >= 0) ? sectors[start].nexttag :
    sectors[(unsigned) line->tag % (unsigned) numsectors].firsttag;
  while (start >= 0 && sectors[start].tag != line->tag)
    start = sectors[start].nexttag;
  return start;
}



// Used by FraggleScript
int Map::FindSectorFromTag(int tag, int start)
{
  start = start >= 0 ? sectors[start].nexttag :
    sectors[(unsigned) tag % (unsigned) numsectors].firsttag;
  while (start >= 0 && sectors[start].tag != tag)
    start = sectors[start].nexttag;
  return start;
}


//SoM: 3/7/2000: More boom specific stuff...
// killough 4/16/98: Same thing, only for linedefs
/*
int Map::FindLineFromLineTag(const line_t *line, int start)
{
  start = start >= 0 ? lines[start].nexttag :
    lines[(unsigned) line->tag % (unsigned) numlines].firsttag;
  while (start >= 0 && lines[start].tag != line->tag)
    start = lines[start].nexttag;
  return start;
}
*/
line_t *Map::FindLineFromTag(int tag, int *start)
{
  int index = (*start >= 0) ? lines[*start].nexttag :
    lines[(unsigned) tag % (unsigned) numlines].firsttag;

  for ( ; index >= 0; index = lines[index].nexttag)
    if (lines[index].tag == tag)
      {
	*start = index;
	return &lines[index];
      }

  // not found
  *start = index;
  return NULL;
}


//SoM: 3/7/2000: Oh joy!
// Hash the sector tags across the sectors and linedefs.
void Map::InitTagLists()
{
  register int i;

  for (i=numsectors; --i>=0; )
    sectors[i].firsttag = -1;
  for (i=numsectors; --i>=0; )
    {
      int j = (unsigned) sectors[i].tag % (unsigned) numsectors;
      sectors[i].nexttag = sectors[j].firsttag;
      sectors[j].firsttag = i;
    }

  for (i=numlines; --i>=0; )
    lines[i].firsttag = -1;
  for (i=numlines; --i>=0; )
    {
      int j = (unsigned) lines[i].tag % (unsigned) numlines;
      lines[i].nexttag = lines[j].firsttag;
      lines[j].firsttag = i;
    }
}




//
// Find minimum light from an adjacent sector
//
int P_FindMinSurroundingLight(sector_t* sector, int max)
{
  int         i;
  int         min;
  line_t*     line;
  sector_t*   check;

  min = max;
  for (i=0 ; i < sector->linecount ; i++)
    {
      line = sector->lines[i];
      check = getNextSector(line,sector);

      if (!check)
	continue;

      if (check->lightlevel < min)
	min = check->lightlevel;
    }
  return min;
}


// Passed a linedef special class (floor, ceiling, lighting) and a sector
// returns whether the sector is already busy with a linedef special of the
// same class. If old demo compatibility true, all linedef special classes
// are the same.

bool P_SectorActive(special_e t, sector_t *sec)
{
  if (!boomsupport)
    return sec->floordata || sec->ceilingdata || sec->lightingdata;
  else switch (t)
    {
    case floor_special:
      return sec->floordata;

    case ceiling_special:
      return sec->ceilingdata;

    case lighting_special:
      return sec->lightingdata;
    }

  return true;
}




//============================================================================
//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//
//============================================================================

// New Hexen linedef system.

bool Map::ActivateLine(line_t *line, Actor *thing, int side, int atype)
{
  // Things that should NOT trigger specials...
  if (thing->flags & MF_NOTRIGGER)
    return false;

  unsigned spec = unsigned(line->special);
  bool p = (thing->Type() == &PlayerPawn::_type);
  // flying blood or water does not activate anything
  bool forceuse = (line->flags & ML_MONSTERS_CAN_ACTIVATE) && !(thing->flags & MF_NOSPLASH);

  // Boom generalized types first
  if (boomsupport && spec >= GenCrusherBase)
    {
      // consistency check
      switch (spec & 6) // bits 1 and 2 of the trigger field ("once")
	{
	case WalkOnce:
	  if (!(atype == SPAC_CROSS || atype == SPAC_MCROSS || atype == SPAC_PCROSS))
	    return false;
	  break;

	case SwitchOnce:
	  if (!(atype == SPAC_USE || atype == SPAC_PASSUSE))
	    return false;
	  break;

	case GunOnce:
	  if (atype != SPAC_IMPACT)
	    return false;
	  break;

	case PushOnce: // like opening a door, means using it
	  if (!(atype == SPAC_USE || atype == SPAC_PASSUSE)) // yeah. SPAC_USE, not SPAC_PUSH.
	    return false;
	  break;

	default:
	  I_Error("Boom generalized type logic breakdown!\n");
	}

      // pointer to line function is NULL by default, set non-null if
      // line special is walkover generalized linedef type
      int (Map::*linefunc)(line_t *line) = NULL;
  
      // check each range of generalized linedefs
      if (spec >= GenFloorBase)
	{
	  if (!p && ((spec & FloorChange) || !(spec & FloorModel)) && !forceuse)
	    return false;   // FloorModel is "Allow Monsters" if FloorChange is 0
	  linefunc = &Map::EV_DoGenFloor;
	}
      else if (spec >= GenCeilingBase)
	{
	  if (!p && ((spec & CeilingChange) || !(spec & CeilingModel)) && !forceuse)
	    return false;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
	  linefunc = &Map::EV_DoGenCeiling;
	}
      else if (spec >= GenDoorBase)
	{
	  if (!p && ((!(spec & DoorMonster) && !forceuse) || (line->flags & ML_SECRET)))
	    // monsters disallowed from this door
	    // they can't open secret doors either
	    return false;
	  linefunc = &Map::EV_DoGenDoor;
	}
      else if (spec >= GenLockedBase)
	{
	  if (!p || !((PlayerPawn *)thing)->CanUnlockGenDoor(line))
	    return false;  // monsters disallowed from unlocking doors, players need key
	  linefunc = &Map::EV_DoGenLockedDoor;
	}
      else if (spec >= GenLiftBase)
	{
	  if (!p && !(spec & LiftMonster) && !forceuse)
	    return false; // monsters disallowed
	  linefunc = &Map::EV_DoGenLift;
	}
      else if (spec >= GenStairsBase)
	{
	  if (!p && !(spec & StairMonster) && !forceuse)
	    return false; // monsters disallowed
	  linefunc = &Map::EV_DoGenStairs;
	}
      else // if (spec >= GenCrusherBase)
	{
	  if (!p && !(spec & CrusherMonster) && !forceuse)
	    return false; // monsters disallowed
	  linefunc = &Map::EV_DoGenCrusher;
	}

      if (!linefunc)
	return false; // should not happen
  
      if (!line->tag)
	if ((spec & 6) != 6) // "door" types can be used without a tag
	  return false;

      switch ((spec & TriggerType) >> TriggerTypeShift)
	{
	case WalkOnce:
	case PushOnce:
	  if ((this->*linefunc)(line))
	    line->special = 0;  // clear special
	  return true;

	case WalkMany:
	case PushMany:
	  (this->*linefunc)(line);
	  return true;

	case SwitchOnce:
	case GunOnce:
	  if ((this->*linefunc)(line))
	    ChangeSwitchTexture(line, 0);
	  return true;

	case SwitchMany:
	case GunMany:
	  if ((this->*linefunc)(line))
	    ChangeSwitchTexture(line, 1);
	  return true;

	default:
	  return false;
	}
    }
  // Boom generalized types done

  int act = GET_SPAC(line->flags);
  if (act != atype)
    return false;

  // monsters can activate the MCROSS activation type, but never open secret doors
  if (!p && !(thing->flags & MF_MISSILE) && !forceuse)
    if (act != SPAC_MCROSS || line->flags & ML_SECRET)
      return false;

  bool repeat = line->flags & ML_REPEAT_SPECIAL;
  bool success = ExecuteLineSpecial(spec, line->args, line, side, thing);

  if (!repeat && success)
    line->special = 0;    // clear the special on non-retriggerable lines

  if ((act == SPAC_USE || act == SPAC_PASSUSE || act == SPAC_IMPACT) && success)
    ChangeSwitchTexture(line, repeat); // FIXME the function

  return true;
}


bool P_CheckKeys(Actor *mo, int lock);

// 
// Hexen linedefs
//
#define SPEED(a)  (((a)*FRACUNIT)/8)
#define HEIGHT(a) ((a)*FRACUNIT)
#define TICS(a)   (((a)*TICRATE)/35)
#define OCTICS(a) (((a)*TICRATE)/8)
#define ANGLE(a)  angle_t((a) << 24) // angle_t is 32-bit int, Hexen angle is a 8-bit byte

bool Map::ExecuteLineSpecial(unsigned special, byte *args, line_t *line, int side, Actor *mo)
{
  bool success = false;
  int lock;

  // callers: ACS, thing death, thing pickup, line activation

  // line->tag == line->args[0] and args[0] are not always same (scripts!)  FIXME which one should we use?
  // we always have args, but line may be NULL

  int tag = args[0];
  if (line && tag == 255)
    {
      // special case to handle converted Doom/Heretic linedefs
      tag = line->tag;
    }
  
  CONS_Printf("ExeSpecial (%d), tag %d (%d)\n", special, tag, args[0]);
  switch (special)
    {
    case 0: // NOP
      break;
    case 1: // Poly Start Line
      break;
    case 2: // Poly Rotate Left
      success = EV_RotatePoly(args, 1, false);
      break;
    case 3: // Poly Rotate Right
      success = EV_RotatePoly(args, -1, false);
      break;
    case 4: // Poly Move
      success = EV_MovePoly(args, false, false);
      break;
    case 5: // Poly Explicit Line:  Only used in initialization
      break;
    case 6: // Poly Move Times 8
      success = EV_MovePoly(args, true, false);
      break;
    case 7: // Poly Door Swing
      success = EV_OpenPolyDoor(args, polydoor_t::pd_swing);
      break;
    case 8: // Poly Door Slide
      success = EV_OpenPolyDoor(args, polydoor_t::pd_slide);
      break;
    case 10: // Door Close
      success = EV_DoDoor(tag, line, mo, vdoor_t::Close, SPEED(args[1]), TICS(args[2]));
      break;
    case 11: // Door Open
      success = EV_DoDoor(tag, line, mo, vdoor_t::Open, SPEED(args[1]), TICS(args[2]));
      break;
    case 12: // Door Raise
      success = EV_DoDoor(tag, line, mo, vdoor_t::OwC, SPEED(args[1]), TICS(args[2]));
      break;
    case 13: // Door Locked_Raise
      if (P_CheckKeys(mo, args[3]))
	success = EV_DoDoor(tag, line, mo, vdoor_t::OwC, SPEED(args[1]), TICS(args[2]));
      break;
    case 20: // Floor Lower by Value
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(args[1]), 0, -HEIGHT(args[2]));
      break;
    case 21: // Floor Lower to Lowest
      success = EV_DoFloor(tag, line, floor_t::LnF, SPEED(args[1]), 0, 0);
      break;
    case 22: // Floor Lower to Nearest
      success = EV_DoFloor(tag, line, floor_t::DownNnF, SPEED(args[1]), 0, 0);
      break;
    case 23: // Floor Raise by Value
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(args[1]), 0, HEIGHT(args[2]));
      break;
    case 24: // Floor Raise to Highest
      success = EV_DoFloor(tag, line, floor_t::HnF, SPEED(args[1]), 0, 0);
      break;
    case 25: // Floor Raise to Nearest
      success = EV_DoFloor(tag, line, floor_t::UpNnF, SPEED(args[1]), 0, 0);
      break;
    case 26: // Stairs Build Down Normal
      success = EV_BuildHexenStairs(tag, stair_t::Normal, -SPEED(args[1]), -HEIGHT(args[2]), args[4], args[3]);
      break;
    case 27: // Build Stairs Up Normal
      success = EV_BuildHexenStairs(tag, stair_t::Normal, SPEED(args[1]), HEIGHT(args[2]), args[4], args[3]);
      break;
    case 28: // Floor Raise and Crush
      success = EV_DoFloor(tag, line, floor_t::Ceiling, SPEED(args[1]), args[2], -HEIGHT(8));
      break;
    case 29: // Build Pillar (no crushing)
      success = EV_DoElevator(tag, elevator_t::ClosePillar, SPEED(args[1]), HEIGHT(args[2]), 0, 0);
      break;
    case 30: // Open Pillar
      success = EV_DoElevator(tag, elevator_t::OpenPillar, SPEED(args[1]), HEIGHT(args[2]), HEIGHT(args[3]), 0);
      break;
    case 31: // Stairs Build Down Sync
      success = EV_BuildHexenStairs(tag, stair_t::Sync, -SPEED(args[1]), -HEIGHT(args[2]), args[3], 0);
      break;
    case 32: // Build Stairs Up Sync
      success = EV_BuildHexenStairs(tag, stair_t::Sync, SPEED(args[1]), HEIGHT(args[2]), args[3], 0);
      break;
    case 35: // Raise Floor by Value Times 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(args[1]), 0, 8*HEIGHT(args[2]));
      break;
    case 36: // Lower Floor by Value Times 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(args[1]), 0, -8*HEIGHT(args[2]));
      break;
    case 40: // Ceiling Lower by Value
      success = EV_DoCeiling(tag, ceiling_t::RelHeight, SPEED(args[1]), 0, 0, -HEIGHT(args[2]));
      break;
    case 41: // Ceiling Raise by Value
      success = EV_DoCeiling(tag, ceiling_t::RelHeight, SPEED(args[1]), 0, 0, HEIGHT(args[2]));
      break;
    case 42: // Ceiling Crush and Raise
      success = EV_DoCeiling(tag, ceiling_t::Crusher, SPEED(args[1]), SPEED(args[1])/2, args[2], HEIGHT(8));
      break;
    case 43: // Ceiling Lower and Crush
      success = EV_DoCeiling(tag, ceiling_t::Floor, SPEED(args[1]), 0, args[2], HEIGHT(8));
      break;
    case 44: // Ceiling Crush Stop
      success = EV_StopCeiling(tag);
      break;
    case 45: // Ceiling Crush Raise and Stay
      success = EV_DoCeiling(tag, ceiling_t::CrushOnce, SPEED(args[1]), SPEED(args[1])/2, args[2], HEIGHT(8));
      break;
      /*
      case 46: // Floor Crush Stop TODO activefloors list or something
      success = EV_FloorCrushStop(line, args);
      break;
      */
    case 60: // Plat Perpetual Raise
      success = EV_DoPlat(tag, line, plat_t::LHF, SPEED(args[1]), TICS(args[2]), 0);
      break;
    case 61: // Plat Stop
      EV_StopPlat(tag);
      break;
    case 62: // Plat Down-Wait-Up-Stay
      success = EV_DoPlat(tag, line, plat_t::LnF, SPEED(args[1]), TICS(args[2]), 0);
      break;
    case 63: // Plat Down-by-Value*8-Wait-Up-Stay
      success = EV_DoPlat(tag, line, plat_t::RelHeight, SPEED(args[1]), TICS(args[2]), -8*HEIGHT(args[3]));
      break;
    case 64: // Plat Up-Wait-Down-Stay
      success = EV_DoPlat(tag, line, plat_t::NHnF, SPEED(args[1]), TICS(args[2]), 0);
      break;
    case 65: // Plat Up-by-Value*8-Wait-Down-Stay
      success = EV_DoPlat(tag, line, plat_t::RelHeight, SPEED(args[1]), TICS(args[2]), 8*HEIGHT(args[3]));
      break;
    case 66: // Floor Lower Instant * 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(16000), 0, -8*HEIGHT(args[2]));
      break;
    case 67: // Floor Raise Instant * 8
      success = EV_DoFloor(tag, line, floor_t::RelHeight, SPEED(16000), 0, 8*HEIGHT(args[2]));
      break;
    case 68: // Floor Move to Value * 8
      success = EV_DoFloor(tag, line, floor_t::AbsHeight, SPEED(args[1]), 0,
			   (args[3] ? -1 : 1) * 8 * HEIGHT(args[2]));
      break;
    case 69: // Ceiling Move to Value * 8
      success = EV_DoCeiling(tag, ceiling_t::AbsHeight, SPEED(args[1]), SPEED(args[1]), 0,
			     (args[3] ? -1 : 1) * 8 * HEIGHT(args[2]));
      break;
    case 70: // Teleport
      if (!side)
	// Only teleport when crossing the front side of a line
	success = EV_Teleport(tag, line, mo, false);
      break;
    case 71: // Teleport, no fog (silent)
      if (!side)
	// Only teleport when crossing the front side of a line
	success = EV_Teleport(tag, line, mo, true);
      break;
    case 72: // Thrust Mobj
      if(!side) // Only thrust on side 0
	{
	  mo->Thrust(ANGLE(args[0]), args[1]<<FRACBITS);
	  success = 1;
	}
      break;
    case 73: // Damage Mobj
      if (args[0])
	mo->Damage(NULL, NULL, args[0]);
      else
	// If arg1 is zero, then guarantee a kill
	mo->Damage(NULL, NULL, 10000, dt_always);
      success = true;
      break;
    case 74: // Teleport_NewMap
      if (!side)
	{ // Only teleport when crossing the front side of a line
	  if (mo->health > 0 && !(mo->flags & MF_CORPSE))
	    {
	      // Activator must be alive
	      ExitMap(mo, args[0], args[1]);
	      success = true;
	    }
	}
      break;
    case 75: // Teleport_EndGame
      if (!side)
	{ // Only teleport when crossing the front side of a line
	  if (mo->health > 0 && !(mo->flags & MF_CORPSE))
	    {
	      if (cv_deathmatch.value)
		ExitMap(mo, 1, 0);// Winning in deathmatch just goes back to map 1
	      else
		ExitMap(mo, -1, 0);

	      success = true;
	    }
	}
      break;
    case 80: // ACS_Execute
      if (args[1] && args[1] != info->mapnumber)
	success = P_AddToACSStore(args[1], args[0], &args[2]);
      else
	success = StartACS(args[0], &args[2], mo, line, side);
      break;
    case 81: // ACS_Suspend
      success = SuspendACS(args[0]);
      break;
    case 82: // ACS_Terminate
      success = TerminateACS(args[0]);
      break;
    case 83: // ACS_LockedExecute
      lock = args[4];
      if (P_CheckKeys(mo, lock))
	{
	  args[4] = 0;
	  if (args[1] && args[1] != info->mapnumber)
	    success = P_AddToACSStore(args[1], args[0], &args[2]);
	  else
	    success = StartACS(args[0], &args[2], mo, line, side);
	  args[4] = lock;
	}
      break;

    case 85: // TEST new linedeftype FS_Execute
#ifdef FRAGGLESCRIPT
      success = T_RunScript(tag, mo);
#endif
      break;

    case 90: // Poly Rotate Left Override
      success = EV_RotatePoly(args, 1, true);
      break;
    case 91: // Poly Rotate Right Override
      success = EV_RotatePoly(args, -1, true);
      break;
    case 92: // Poly Move Override
      success = EV_MovePoly(args, false, true);
      break;
    case 93: // Poly Move Times 8 Override
      success = EV_MovePoly(args, true, true);
      break;
    case 94: // Build Pillar Crush
      success = EV_DoElevator(tag, elevator_t::ClosePillar, SPEED(args[1]), HEIGHT(args[2]), 0, args[3]);
      break;
    case 95: // Lower Floor and Ceiling
      success = EV_DoElevator(tag, elevator_t::RelHeight, SPEED(args[1]), -HEIGHT(args[2]));
      break;
    case 96: // Raise Floor and Ceiling
      success = EV_DoElevator(tag, elevator_t::RelHeight, SPEED(args[1]), HEIGHT(args[2]));
      break;
    case 109: // Force Lightning
      success = true;
      ForceLightning();
      break;
    case 110: // Light Raise by Value
      success = EV_SpawnLight(tag, lightfx_t::RelChange, args[1]);
      break;
    case 111: // Light Lower by Value
      success = EV_SpawnLight(tag, lightfx_t::RelChange, -args[1]);
      break;
    case 112: // Light Change to Value
      success = EV_SpawnLight(tag, lightfx_t::AbsChange, args[1]);
      break;
    case 113: // Light Fade
      success = EV_SpawnLight(tag, lightfx_t::Fade, args[1], 0, args[2]);
      break;
    case 114: // Light Glow
      success = EV_SpawnLight(tag, lightfx_t::Glow, args[1], args[2], args[3]);
      break;
    case 115: // Light Flicker
      success = EV_SpawnLight(tag, lightfx_t::Flicker, args[1], args[2], 32, 8);
      break;
    case 116: // Light Strobe
      success = EV_SpawnLight(tag, lightfx_t::Strobe, args[1], args[2], args[3], args[4]);
      break;
    case 120: // Quake Tremor
      success = EV_LocalQuake(args);
      break;
      /*
    case 129: // UsePuzzleItem. Not needed, see P_UseArtifact()
      success = EV_LineSearchForPuzzleItem(line, args, mo);
      break;
      */
    case 130: // Thing_Activate
      success = EV_ThingActivate(args[0]);
      break;
    case 131: // Thing_Deactivate
      success = EV_ThingDeactivate(args[0]);
      break;
    case 132: // Thing_Remove
      success = EV_ThingRemove(args[0]);
      break;
    case 133: // Thing_Destroy
      success = EV_ThingDestroy(args[0]);
      break;
    case 134: // Thing_Projectile
      success = EV_ThingProjectile(args, 0);
      break;
    case 135: // Thing_Spawn
      success = EV_ThingSpawn(args, 1);
      break;
    case 136: // Thing_ProjectileGravity
      success = EV_ThingProjectile(args, 1);
      break;
    case 137: // Thing_SpawnNoFog
      success = EV_ThingSpawn(args, 0);
      break;
    case 138: // Floor_Waggle
      success = EV_DoFloorWaggle(tag, args[1] << 13, angle_t(args[2] << 20), angle_t(args[3] << 26), args[4]*35);
      break;
    case 140: // Sector_SoundChange
      success = EV_SectorSoundChange(args[0], args[1]);
      break;

      // Line specials only processed during level initialization
      // 100: Scroll_Texture_Left
      // 101: Scroll_Texture_Right
      // 102: Scroll_Texture_Up
      // 103: Scroll_Texture_Down
      // 121: Line_SetIdentification

    case 200: // TODO ZDoom Generic_Floor
    case 201: // TODO ZDoom Generic_Ceiling
      //success = EV_DoCeiling(tag, type, SPEED(args[1]), SPEED(args[1]), crush, HEIGHT(args[2]));
      break;
      // TODO other ZDoom Generic types

    case 245: // ZDoom Elevator_RaiseToNearest
      success = EV_DoElevator(tag, elevator_t::Up, SPEED(args[1]), 0);
      break;
    case 246: // ZDoom Elevator_MoveToFloor
      if (line)
	success = EV_DoElevator(tag, elevator_t::Current, SPEED(args[1]), line->frontsector->floorheight);
      else
	success = false;
      break;
    case 247: // ZDoom Elevator_LowerToNearest
      success = EV_DoElevator(tag, elevator_t::Down, SPEED(args[1]), 0);
      break;

      // Inert Line specials
    default:
      CONS_Printf("Unhandled line special %d\n", special);
      break;
    }

  return success;
}


int Map::EV_SectorSoundChange(int tag, int seq)
{
  if (!tag)
    return false;

  int secNum = -1;
  int rtn = 0;
  while ((secNum = FindSectorFromTag(tag, secNum)) >= 0)
    {
      sectors[secNum].seqType = seq;
      rtn++;
    }
  return rtn;
}


//
// Animate textures etc.
//
void Map::UpdateSpecials()
{
  //  LEVEL TIMER
  if (cv_timelimit.value && maptic > unsigned(cv_timelimit.value))
    ExitMap(NULL, 0);

  LightningFlash();
}



//SoM: 3/23/2000: Adds a sectors floor and ceiling to a sector's ffloor list
void P_AddFFloor(sector_t* sec, ffloor_t* ffloor);

void Map::AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags)
{
  //Add the floor
  ffloor_t *ffloor = (ffloor_t*)Z_Malloc(sizeof(ffloor_t), PU_LEVEL, NULL);
  ffloor->secnum = sec2 - sectors;
  ffloor->target = sec;
  ffloor->bottomheight     = &sec2->floorheight;
  ffloor->bottompic        = &sec2->floorpic;
  ffloor->bottomlightlevel = &sec2->lightlevel;
  ffloor->bottomxoffs      = &sec2->floor_xoffs;
  ffloor->bottomyoffs      = &sec2->floor_yoffs;

  //Add the ceiling
  ffloor->topheight     = &sec2->ceilingheight;
  ffloor->toppic        = &sec2->ceilingpic;
  ffloor->toplightlevel = &sec2->lightlevel;
  ffloor->topxoffs      = &sec2->ceiling_xoffs;
  ffloor->topyoffs      = &sec2->ceiling_yoffs;

  ffloor->flags = ffloortype_e(flags);
  ffloor->master = master;

  if(flags & FF_TRANSLUCENT)
    {
      if(sides[master->sidenum[0]].toptexture > 0)
	ffloor->alpha = sides[master->sidenum[0]].toptexture;
      else
	ffloor->alpha = 0x70;
    }

  if(sec2->numattached == 0)
    {
      sec2->attached = (int *)malloc(sizeof(int));
      sec2->attached[0] = sec - sectors;
      sec2->numattached = 1;
    }
  else
    {
      sec2->attached = (int *)realloc(sec2->attached, sizeof(int) * (sec2->numattached + 1));
      sec2->attached[sec2->numattached] = sec - sectors;
      sec2->numattached ++;
    }

  P_AddFFloor(sec, ffloor);
}



void P_AddFFloor(sector_t* sec, ffloor_t* ffloor)
{
  ffloor_t* rover;

  if(!sec->ffloors)
    {
      sec->ffloors = ffloor;
      ffloor->next = 0;
      ffloor->prev = 0;
      return;
    }

  for(rover = sec->ffloors; rover->next; rover = rover->next);

  rover->next = ffloor;
  ffloor->prev = rover;
  ffloor->next = 0;
}




int Map::SpawnSectorSpecial(int sp, sector_t *sec)
{
  // internally we use modified "generalized Boom sector types"
  if (sp == 0)
    {
      sec->special = 0;
      return 0;
    }

  if (sp == 9)
    {
      secrets++;
      sec->special = SS_secret;
      return SS_secret;
    }

  const char HScrollDirs[4][2] = {{1,0}, {0,1}, {0,-1}, {-1,0}};
  const char HScrollSpeeds[5] = { 5, 10, 25, 30, 35 };
  const float d = 0.707;
  const float XScrollDirs[8][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}, {-d,d}, {d,d}, {d,-d}, {-d,-d}};

  int temp;
  CONS_Printf("SS: %d =>", sp);
  if (hexen_format)
    {
      // Boom damage (and secret) bits cannot be interpreted 'cos they are used otherwise

      temp = sp & 0xFF; // eight lowest bits

      if (temp >= 40 && temp <= 51)
	{
	  // Hexen winds (just like Heretic?)
	  temp -= 40; // zero base

	  fixed_t dx = HScrollDirs[temp/3][0]*HScrollSpeeds[temp%3]*2048;
	  fixed_t dy = HScrollDirs[temp/3][1]*HScrollSpeeds[temp%3]*2048;
	  
	  AddThinker(new scroll_t(scroll_t::sc_wind, dx, dy, NULL, sec - sectors, false));
	}
      else if (temp >= 201 && temp <= 224)
	{
	  // Hexen scrollers
	  temp -= 201; // zero base

	  fixed_t dx = int(XScrollDirs[temp/3][0]*HScrollSpeeds[temp%3]*2048);
	  fixed_t dy = int(XScrollDirs[temp/3][1]*HScrollSpeeds[temp%3]*2048);
	  
	  AddThinker(new scroll_t(scroll_t::sc_carry_floor, dx, dy, NULL, sec - sectors, false));
	}
      else switch (temp)
	{
	case 1: // Phased light
	  // Hardcoded base, use sector->lightlevel as the index
	  new phasedlight_t(this, sec, 80, -1);
	  break;
	case 2: // Phased light sequence start
	  SpawnPhasedLightSequence(sec, 1);
	  break;

	case 3:
	case 4:
	  // Phased light sequencing. Leave them be, they fit into the unused lightmask area.
	case 26: // Stairs_Special1
	case 27: // Stairs_Special2
	  // Same with stair sequences.
	  sec->special = sp;
	  CONS_Printf("%d\n", sp);
	  return sp;

	case 198: // Lightning Special
	case 199: // Lightning Flash special
	case 200: // Sky2
	  // Used in (R_plane):R_Drawplanes
	  break;
	default:
	  break;
	}

      sp &= ~0xFF; // zero eight lowest bits
      sec->special = sp;
      CONS_Printf("%d\n", sp);
      return sp;
    }

  if (sp & SS_secret)
    secrets++;

  if (game.mode == gm_heretic)
    {
      temp = sp & 0x3F; // six lowest bits
      sp &= ~0x3F; // zero it

      if (temp < 20)
	switch (temp)
	  {
	  case 7:  // sludge
	    sec->damage = 4;
	    sec->damagetype = dt_corrosive;
	    sp |= SS_damage_32;
	    break;

	  case 4:  // lava_scroll_east
	    AddThinker(new scroll_t(scroll_t::sc_carry_floor, 2048*28, 0, NULL, sec - sectors, false));
	    // fallthru
	  case 5:  // lava_wimpy
	    sec->damage = 5;
	    sec->damagetype = dt_heat;
	    sp |= SS_damage_16;
	    break;

	  case 16: // lava_hefty
	    sec->damage = 8;
	    sec->damagetype = dt_heat;
	    sp |= SS_damage_16;
	    break;

	  case 15: // low friction
	    sec->friction = 0.97266f;
	    sec->movefactor = 0.25f;
	    sp |= SS_friction;
	    break;

	  default:
	    sp |= temp; // put it back, handle later
	  }
      else if (temp <= 39)
	{
	  // Heretic scrollers
	  temp -= 20; // zero base

	  fixed_t dx = HScrollDirs[temp/5][0]*HScrollSpeeds[temp%5]*2048;
	  fixed_t dy = HScrollDirs[temp/5][1]*HScrollSpeeds[temp%5]*2048;

	  // texture does not scroll, Actors do
	  AddThinker(new scroll_t(scroll_t::sc_push, dx, dy, NULL, sec - sectors, false));
	}
      else if (temp <= 51)
	{
	  // Heretic winds
	  temp -= 40; // zero base

	  fixed_t dx = HScrollDirs[temp/3][0]*HScrollSpeeds[temp%3]*2048;
	  fixed_t dy = HScrollDirs[temp/3][1]*HScrollSpeeds[temp%3]*2048;
	  
	  AddThinker(new scroll_t(scroll_t::sc_wind, dx, dy, NULL, sec - sectors, false));
	}
    }
  else
    {
      const char BoomDamage[4] = {0, 5, 10, 20};
      // in Heretic and Hexen, Boom damage bits cannot be used because of winds and scrollers
      // Boom damage flags
      temp = (sp & SS_DAMAGEMASK) >> 5;
      sp &= ~SS_DAMAGEMASK; // zero it
      if (temp)
	{
	  sp |= SS_damage_32;
	  sec->damage = BoomDamage[temp];
	  sec->damagetype = dt_radiation; // could as well choose randomly?      
	}
    }


  // Doom / Boom

  temp = sp & SS_LIGHTMASK;
  sp &= ~SS_LIGHTMASK; // zeroed "light" bits

  int i, dam = 0;
  lightfx_t *lfx = NULL;

  const short ff_tics = 4;
  const short glowspeed = 8;

  switch (temp)
    {
    case 7:  // nukage/slime
      dam = 5;
      break;

    case 5:  // hellslime
      dam = 10;
      break;

    case 4:  // strobe hurt
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, false); // fallthru
    case 16: // super hellslime
      dam = 20;
      break;

    case 11: // level end hurt need special handling
      dam = 20;
      sp |= 11;
      break;

    case 10: // after 30 s, close door
      SpawnDoorCloseIn30(sec);
      break;

    case 14: // after 5 min, open door
      SpawnDoorRaiseIn5Mins(sec);
      break;

    case 1: // FLICKERING LIGHTS
      i = P_FindMinSurroundingLight(sec, sec->lightlevel);
      lfx = new lightfx_t(this, sec, lightfx_t::Flicker, sec->lightlevel, i, 64, 7);
      lfx->count = (P_Random() & lfx->maxtime) + 1;
      break;

    case 2: // STROBE FAST
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, false);
      break;

    case 3: // STROBE SLOW
      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, false);
      break;

    case 8: // GLOWING LIGHT
      i = P_FindMinSurroundingLight(sec, sec->lightlevel);
      lfx = new lightfx_t(this, sec, lightfx_t::Glow, sec->lightlevel, i, -glowspeed);
      break;

    case 12: // SYNC STROBE SLOW
      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, true);
      break;

    case 13: // SYNC STROBE FAST
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, true);
      break;

    case 17:
      i = P_FindMinSurroundingLight(sec, sec->lightlevel) + 16;
      lfx = new lightfx_t(this, sec, lightfx_t::FireFlicker, sec->lightlevel, i, ff_tics);
      lfx->count = ff_tics;
      break;

    default:
      break;
    }

  if (dam)
    {
      sp |= SS_damage_32;
      sec->damage = dam;
      sec->damagetype = dt_radiation; // could as well choose randomly?
    }

  /*
    // TODO support phased lighting with specials 21-24 ? (like ZDoom)
    else if (temp < 40)
    {
    temp -= 20;
    }
  */

  sec->special = sp;
  return sp;
}

//
// After the map has been loaded, scan for linedefs
//  that spawn thinkers or confer properties
//
void Map::SpawnLineSpecials()
{
  int i;

  RemoveAllActiveCeilings();
  RemoveAllActivePlats();

  // First set the Hexen line tags.
  for (i = 0; i < numlines; i++)
    if (lines[i].special == 121)
      {
	// Hexen: Line_SetIdentification
	lines[i].tag = lines[i].args[0];
	lines[i].special = 0;
      }

  InitTagLists();   //Create xref tables for tags

  // Then spawn the stuff that use the line/sector tags.
  SpawnScrollers(); //Add generalized scrollers
  SpawnFriction();  //New friction model using linedefs
  SpawnPushers();   //New pusher model using linedefs

  //  Init line EFFECTs
  for (i = 0; i < numlines; i++)
    {
      switch (lines[i].special)
        {
          int s, sec;
          // support for drawn heights coming from different sector
	case 242:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    sectors[s].heightsec = sec;
	  break;

          //SoM: 3/20/2000: support for drawn heights coming from different sector
	case 280:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
            {
              sectors[s].heightsec = sec;
              sectors[s].altheightsec = 1;
            }
	  break;

          //SoM: 4/4/2000: HACK! Copy colormaps. Just plain colormaps.
	case 282:
	  for(s = -1; (s = FindSectorFromLineTag(lines + i, s)) >= 0;)
            {
              sectors[s].midmap = lines[i].frontsector->midmap;
              sectors[s].altheightsec = 2;
            }
	  break;

	case 281:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL);
	  break;

	case 289:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_CUTLEVEL);
	  break;

          // TL block
	case 300:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA);
	  break;

          // TL water
	case 301:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_RENDERALL|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES);
	  break;

          // Fog
	case 302:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  // SoM: Because it's fog, check for an extra colormap and set
	  // the fog flag...
	  if(sectors[sec].extra_colormap)
	    sectors[sec].extra_colormap->fog = 1;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_RENDERALL|FF_FOG|FF_BOTHPLANES|FF_INVERTPLANES|FF_ALLSIDES|FF_INVERTSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES);
	  break;

          // Light effect
	case 303:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_CUTSPRITES);
	  break;

          // Opaque water
	case 304:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_RENDERALL|FF_SWIMMABLE|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES);
	  break;

          // Double light effect
	case 305:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    AddFakeFloor(&sectors[s], &sectors[sec], lines+i, FF_EXISTS|FF_CUTSPRITES|FF_DOUBLESHADOW);
	  break;

          // floor lighting independently (e.g. lava)
	case 213:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    sectors[s].floorlightsec = sec;
	  break;

          // ceiling lighting independently
	case 261:
	  sec = sides[*lines[i].sidenum].sector-sectors;
	  for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
	    sectors[s].ceilinglightsec = sec;
	  break;

	  // Instant lower for floor SSNTails 06-13-2002
	case 290:
	  EV_DoFloor(lines[i].tag, &lines[i], floor_t::LnF, MAXINT/2, 0, 0);
	  break;
	  
	  // Instant raise for ceilings SSNTails 06-13-2002
	case 291:
	  EV_DoCeiling(lines[i].tag, ceiling_t::HnC, MAXINT/2, MAXINT/2, 0, 0);
	  break;

	default:
	  // TODO is this used? if not, replace it with a thing...
	  if (lines[i].special>=1000 && lines[i].special<1032)
            {
	      for (s = -1; (s = FindSectorFromLineTag(lines+i,s)) >= 0;)
                {
                  sectors[s].teamstartsec = lines[i].special-999; // only 999 so we know when it is set (it's != 0)
                }
	      break;
            }
        }
    }
}



//========================================================
//  Hexen lightning effect
//========================================================

void Map::InitLightning()
{
  Flashcount = 0;

  if (!info->lightning)
    return;

  int count = 0;
  for (int i = 0; i < numsectors; i++)
    {
      if(sectors[i].ceilingpic == skyflatnum
	 || sectors[i].special == SS_light_IndoorLightning1
	 || sectors[i].special == SS_light_IndoorLightning2)
	count++;
    }

  if (!count)
    {
      info->lightning = false; // no way to see the flashes
      return;
    }

  LightningLightLevels = (int *)Z_Malloc(count * sizeof(int), PU_LEVEL, NULL);
  NextLightningFlash = ((P_Random() & 15) + 5) * 35; // don't flash at level start
}



void Map::ForceLightning()
{
  NextLightningFlash = 0;
}



void Map::LightningFlash()
{
  int i;
  sector_t *s;
  int *lite;

  if (!info->lightning)
    return;

  NextLightningFlash--;

  if (Flashcount--)
    {
      if (Flashcount)
	{
	  // still flashing
	  lite = LightningLightLevels;
	  s = sectors;
	  for (i = 0; i < numsectors; i++, s++)
	    {
	      if (s->ceilingpic == skyflatnum 
		  || s->special == SS_light_IndoorLightning1
		  || s->special == SS_light_IndoorLightning2)
		{
		  if (*lite < s->lightlevel - 4)
		    s->lightlevel -= 4;

		  lite++;
		}
	    }
	}					
      else
	{
	  // flash ends
	  lite = LightningLightLevels;
	  s = sectors;
	  for (i = 0; i < numsectors; i++, s++)
	    {
	      if(s->ceilingpic == skyflatnum
		 || s->special == SS_light_IndoorLightning1
		 || s->special == SS_light_IndoorLightning2)
		{
		  s->lightlevel = *lite;
		  lite++;
		}
	    }
	  skytexture = tc.GetPtr(info->sky1.c_str()); // set alternate sky
	}
      return;
    }


  if (NextLightningFlash > 0)
    return;

  // new flash
  Flashcount = (P_Random() & 7) + 8;
  int flashlight = 200 + (P_Random() & 31);
  s = sectors;
  lite = LightningLightLevels;

  for (i = 0; i < numsectors; i++, s++)
    {
      if (s->ceilingpic == skyflatnum
	  || s->special == SS_light_IndoorLightning1
	  || s->special == SS_light_IndoorLightning2)
	{
	  *lite = s->lightlevel;
	  if (s->special == SS_light_IndoorLightning1)
	    { 
	      s->lightlevel += 64;
	      if (s->lightlevel > flashlight)
		s->lightlevel = flashlight;
	    }
	  else if (s->special == SS_light_IndoorLightning2)
	    {
	      s->lightlevel += 32;
	      if (s->lightlevel > flashlight)
		s->lightlevel = flashlight;
	    }
	  else
	    s->lightlevel = flashlight;

	  if (s->lightlevel < *lite)
	    s->lightlevel = *lite;

	  lite++;
	}
    }

  skytexture = tc.GetPtr(info->sky2.c_str()); // set alternate sky
  S_StartAmbSound(SFX_THUNDER_CRASH);

  // Calculate the next lighting flash
  if (P_Random() < 50)
    NextLightningFlash = (P_Random()&15) + 16; // Immediate Quick flash
  else
    {
      if (P_Random() < 128 && !(maptic & 32))
	NextLightningFlash = ((P_Random()&7) + 2) * 35;
      else
	NextLightningFlash = ((P_Random()&15) + 5) * 35;
    }
}



//==========================================================================
//  Scrollers
//==========================================================================

IMPLEMENT_CLASS(scroll_t, Thinker);
scroll_t::scroll_t() {}

//
// This function, with the help of r_plane.c and r_bsp.c, supports generalized
// scrolling floors and walls, with optional mobj-carrying properties, e.g.
// conveyor belts, rivers, etc. A linedef with a special type affects all
// tagged sectors the same way, by creating scrolling and/or object-carrying
// properties. Multiple linedefs may be used on the same sector and are
// cumulative, although the special case of scrolling a floor and carrying
// things on it, requires only one linedef. The linedef's direction determines
// the scrolling direction, and the linedef's length determines the scrolling
// speed. This was designed so that an edge around the sector could be used to
// control the direction of the sector's scrolling, which is usually what is
// desired.
//

void scroll_t::Think()
{
  fixed_t tdx = vx, tdy = vy;

  if (control)
    {   // compute scroll amounts based on a sector's height changes
      fixed_t height = control->floorheight + control->ceilingheight;
      fixed_t delta = height - last_height;
      last_height = height;
      tdx = FixedMul(tdx, delta);
      tdy = FixedMul(tdy, delta);
    }

  if (accel)
    {
      vdx = tdx += vdx;
      vdy = tdy += vdy;
    }

  if (!(tdx | tdy))                   // no-op if both (x,y) offsets 0
    return;

  switch (type)
    {
      side_t *side;
      sector_t *sec;
      fixed_t height, waterheight;
      msecnode_t *node;
      Actor *thing;

    case sc_side:                   //Scroll wall texture
      side = mp->sides + affectee;
      side->textureoffset += tdx;
      side->rowoffset += tdy;
      break;

    case sc_floor:                  //Scroll floor texture
      sec = mp->sectors + affectee;
      sec->floor_xoffs += tdx;
      sec->floor_yoffs += tdy;
      break;

    case sc_ceiling:               //Scroll ceiling texture
      sec = mp->sectors + affectee;
      sec->ceiling_xoffs += tdx;
      sec->ceiling_yoffs += tdy;
      break;

    case sc_carry_floor:
    case sc_push:
    case sc_wind:

      sec = mp->sectors + affectee;
      height = sec->floorheight;
      waterheight = sec->heightsec != -1 &&
        mp->sectors[sec->heightsec].floorheight > height ?
        mp->sectors[sec->heightsec].floorheight : MININT;

      for (node = sec->touching_thinglist; node; node = node->m_snext)
	{
	  thing = node->m_thing;

	  if (type == sc_wind && !(thing->flags2 & MF2_WINDTHRUST))
	    continue;

	  if (!(thing->flags & MF_NOCLIPLINE) &&
            (!(thing->flags & MF_NOGRAVITY || thing->z > height) ||
             thing->z < waterheight))
          {
            // Move objects only if on floor or underwater,
            // non-floating, and clipped.
            thing->px += tdx;
            thing->py += tdy;
          }
	}
      break;

    case sc_carry_ceiling:       // to be added later
      break;
    }
}

// Add a generalized scroller to the thinker list.
//
// type: the enumerated type of scrolling: floor, ceiling, floor carrier,
//   wall, floor carrier & scroller
//
// (dx,dy): the direction and speed of the scrolling or its acceleration
//
// control: the sector whose heights control this scroller's effect
//   remotely, or -1 if no control sector
//
// affectee: the index of the affected object (sector or sidedef)
//
// accel: non-zero if this is an accelerative effect

scroll_t::scroll_t(scroll_e t, fixed_t dx, fixed_t dy, sector_t *csec, int aff, bool acc)
{
  type = t;
  vx = dx;
  vy = dy;
  accel = acc;
  vdx = vdy = 0;
  control = csec;
  affectee = aff;

  if (control)
    last_height = control->floorheight + control->ceilingheight;
}

// Adds wall scroller. Scroll amount is rotated with respect to wall's
// linedef first, so that scrolling towards the wall in a perpendicular
// direction is translated into vertical motion, while scrolling along
// the wall in a parallel direction is translated into horizontal motion.

static scroll_t *Add_WallScroller(fixed_t dx, fixed_t dy, const line_t *l,
				  sector_t *control, int accel)
{
  fixed_t x = abs(l->dx), y = abs(l->dy), d;
  if (y > x)
    d = x, x = y, y = d;
  d = FixedDiv(x, finesine[(tantoangle[FixedDiv(y,x) >> DBITS] + ANG90)
                          >> ANGLETOFINESHIFT]);
  x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
  y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
  return new scroll_t(scroll_t::sc_side, x, y, control, *l->sidenum, accel);
}

// Amount (dx,dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))


// Initialize the scrollers
void Map::SpawnScrollers()
{
  int i, s;
  line_t *l = lines, *l2;

  for (i=0;i<numlines;i++,l++)
    {
      fixed_t dx = l->dx >> SCROLL_SHIFT;  // direction and speed of scrolling
      fixed_t dy = l->dy >> SCROLL_SHIFT;
      //int control = -1;
      sector_t *control = NULL;
      bool accel = false;   // no control sector or acceleration
      int special = l->special;

      // Types 245-249 are same as 250-254 except that the
      // first side's sector's heights cause scrolling when they change, and
      // this linedef controls the direction and speed of the scrolling. The
      // most complicated linedef since donuts, but powerful :)

      if (special >= 245 && special <= 249)         // displacement scrollers
        {
          special += 250-245;
          control = sides[*l->sidenum].sector;
        }
      else if (special >= 214 && special <= 218)       // accelerative scrollers
	{
	  accel = true;
	  special += 250-214;
	  control = sides[*l->sidenum].sector;
	}

      bool clear = true; // should the special be cleared?

      switch (special)
        {
        case 250:   // scroll effect ceiling
          for (s=-1; (s = FindSectorFromLineTag(l,s)) >= 0;)
	    AddThinker(new scroll_t(scroll_t::sc_ceiling, -dx, dy, control, s, accel));
          break;

        case 251:   // scroll effect floor
        case 253:   // scroll and carry objects on floor
          for (s=-1; (s = FindSectorFromLineTag(l,s)) >= 0;)
            AddThinker(new scroll_t(scroll_t::sc_floor, -dx, dy, control, s, accel));
          if (special != 253)
            break;

        case 252: // carry objects on floor
          dx = FixedMul(dx,CARRYFACTOR);
          dy = FixedMul(dy,CARRYFACTOR);
          for (s=-1; (s = FindSectorFromLineTag(l,s)) >= 0;)
	    AddThinker(new scroll_t(scroll_t::sc_carry_floor, dx, dy, control, s, accel));
          break;

          // scroll wall according to linedef
          // (same direction and speed as scrolling floors)
        case 254:
          for (s=-1; (l2 = FindLineFromTag(l->tag, &s)) != NULL; )
            if (s != i)
              AddThinker(Add_WallScroller(dx, dy, l2, control, accel));
          break;

        case 255:
          AddThinker(new scroll_t(scroll_t::sc_side, -sides[s].textureoffset,
                       sides[s].rowoffset, NULL, l->sidenum[0], accel));
          break;

	  // Hexen
	case 100: // Scroll_Texture_Left
          AddThinker(new scroll_t(scroll_t::sc_side, -l->args[0] << 10, 0, NULL, l->sidenum[0], false));
	  break;
	case 101: // Scroll_Texture_Right
          AddThinker(new scroll_t(scroll_t::sc_side, l->args[0] << 10, 0, NULL, l->sidenum[0], false));
	  break;
	case 102: // Scroll_Texture_Up
          AddThinker(new scroll_t(scroll_t::sc_side, 0, l->args[0] << 10, NULL, l->sidenum[0], false));
	  break;
	case 103: // Scroll_Texture_Down
          AddThinker(new scroll_t(scroll_t::sc_side, 0, -l->args[0] << 10, NULL, l->sidenum[0], false));
	  break;

	default:
	  clear = false;
        }

      if (clear)
	l->special = 0;
    }
}



//==========================================================================
//  Friction
//==========================================================================



//============================================
//SoM: 3/8/2000: added new model of friction for ice/sludge effects
// FIXME Do we even need a friction thinker?
/*
class friction_t : public Thinker
{
  friend class Map;
  DECLARE_CLASS(friction_t)
private:
  float friction;        // friction value (E800 = normal)
  float movefactor;      // inertia factor when adding to momentum
  int   affectee;        // Number of affected sector
public:
  friction_t(float fri, float mf, int aff);
  
  virtual void Think();
};


IMPLEMENT_CLASS(friction_t, Thinker);
friction_t::friction_t() {}

// constructor
// Adds friction thinker.
friction_t::friction_t(float fri, float mf, int aff)
{
  friction = fri;
  movefactor = mf;
  affectee = aff;
}

int friction_t::Marshal(LArchive & a)
{
  return 0;
}

//Function to apply friction to all the things in a sector.
void friction_t::Think()
{
  if (!boomsupport || !variable_friction)
    return;

  sector_t *sec = mp->sectors + affectee;

  // Be sure the special sector type is still turned on. If so, proceed.
  // Else, bail out; the sector type has been changed on us.

  if (!(sec->special & SS_friction))
    return;

  // Assign the friction value to players on the floor, non-floating,
  // and clipped. Normally the object's friction value is kept at
  // ORIG_FRICTION and this thinker changes it for icy or muddy floors.

  // In Phase II, you can apply friction to Things other than players.

  // When the object is straddling sectors with the same
  // floorheight that have different frictions, use the lowest
  // friction value (muddy has precedence over icy).

  Actor      *thing;
  msecnode_t *node = sec->touching_thinglist; // things touching this sector
  while (node)
    {
      thing = node->m_thing;
      // FIXME friction ought to apply to monsters as well
      if (thing->Type() == Thinker::tt_ppawn &&
	  !(thing->flags & (MF_NOGRAVITY | MF_NOCLIPLINE)) &&
	  thing->z <= sec->floorheight)
	{
	  if ((thing->friction == normal_friction) ||
	      (friction < thing->friction))
	    {
	      thing->friction   = friction;
	      thing->movefactor = movefactor;
	    }
	}
      node = node->m_snext;
    }
}
*/

//Spawn all friction.
void Map::SpawnFriction()
{
  int i;
  line_t *l = lines;
  register int s;
  float length;     // line length controls magnitude
  float friction;   // friction value to be applied during movement
  float movefactor; // applied to each player move to simulate inertia

  extern float normal_friction;

  for (i = 0 ; i < numlines ; i++,l++)
    if (l->special == 223)
      {
	length = P_AproxDistance(l->dx,l->dy) >> FRACBITS;

	//friction = (0x1EB8*length)/0x80 + 0xD000;
	// l = 200 gives 1, l = 100 gives the original, l = 0 gives 0.8125
	friction = length * 9.375e-4 + 0.8125;

	if (friction > 1.0)
	  friction = 1.0;
	if (friction < 0.0)
	  friction = 0.0;

	// object max speed is proportional to movefactor/(1-friction)
	// higher friction value actually means 'less friction'.
	// the movefactors are a bit different from BOOM (better;)

	if (friction > normal_friction)
	  // ice, max speed is unchanged.
	  //movefactor = ((0x10092 - friction)*(0x70))/0x158;
	  //movefactor = (65682 - friction) * 112/344;
	  movefactor = (1.0 - friction) * 10.667;
	else
	  // mud
	  //movefactor = ((friction - 0xDB34)*(0xA))/0x80;
	  //movefactor = (friction - 56116) * 10/128;
	  movefactor = (friction - 0.8125) * 10.667;

	// minimum movefactor (1/64)
	if (movefactor < 1.0/64)
	  movefactor = 1.0/64;

	for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )
	  {
	    //AddThinker(new friction_t(friction,movefactor,s));
	    sectors[s].friction   = friction;
	    sectors[s].movefactor = movefactor;
	  }

	l->special = 0;
      }
}


//==========================================================================
//  Pushers
//==========================================================================

IMPLEMENT_CLASS(pusher_t, Thinker);
pusher_t::pusher_t() {}

#define PUSH_FACTOR 7

// constructor
pusher_t::pusher_t(pusher_e t, int x_m, int y_m, DActor *src, int aff)
{
  source = src;
  type = t;
  x_mag = x_m >> FRACBITS;
  y_mag = y_m >> FRACBITS;
  magnitude = P_AproxDistance(x_mag, y_mag);
  if (source) // point source exist?
    {
      radius = (magnitude) << (FRACBITS+1); // where force goes to zero
      x = src->x;
      y = src->y;
    }
  affectee = aff;
}


// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.

static pusher_t *tmpusher; // pusher structure for blockmap searches

bool PIT_PushThing(Actor* thing)
{
  if (thing->IsOf(PlayerPawn::_type) && !(thing->flags & (MF_NOGRAVITY | MF_NOCLIPLINE)))
    {
      angle_t pushangle;
      int dist;
      int speed;
      int sx,sy;

      sx = tmpusher->x;
      sy = tmpusher->y;
      dist = P_AproxDistance(thing->x - sx,thing->y - sy);
      speed = (tmpusher->magnitude -
	       ((dist>>FRACBITS)>>1))<<(FRACBITS-PUSH_FACTOR-1);

      // If speed <= 0, you're outside the effective radius. You also have
      // to be able to see the push/pull source point.

      if ((speed > 0) && (thing->mp->CheckSight(thing, tmpusher->source)))
	{
	  pushangle = R_PointToAngle2(thing->x,thing->y,sx,sy);
	  if (tmpusher->source->type == MT_PUSH)
	    pushangle += ANG180;    // away
	  pushangle >>= ANGLETOFINESHIFT;
	  thing->px += FixedMul(speed,finecosine[pushangle]);
	  thing->py += FixedMul(speed,finesine[pushangle]);
	}
    }
  return true;
}

// looks for all objects that are inside the radius of
// the effect.

void pusher_t::Think()
{
  if (!allow_pushers)
    return;

  sector_t *sec = mp->sectors + affectee;

  // Be sure the special sector type is still turned on. If so, proceed.
  // Else, bail out; the sector type has been changed on us.

  if (!(sec->special & SS_wind))
    return;

  // For constant pushers (wind/current) there are 3 situations:
  //
  // 1) Affected Thing is above the floor.
  //
  //    Apply the full force if wind, no force if current.
  //
  // 2) Affected Thing is on the ground.
  //
  //    Apply half force if wind, full force if current.
  //
  // 3) Affected Thing is below the ground (underwater effect).
  //
  //    Apply no force if wind, full force if current.
  //
  // Apply the effect to clipped players only for now.
  //
  // In Phase II, you can apply these effects to Things other than players.

  if (type == p_push)
    {
      int xl,xh,yl,yh,bx,by;
      // Seek out all pushable things within the force radius of this
      // point pusher. Crosses sectors, so use blockmap.

      tmpusher = this; // MT_PUSH/MT_PULL point source
      tmbbox[BOXTOP]    = y + radius;
      tmbbox[BOXBOTTOM] = y - radius;
      tmbbox[BOXRIGHT]  = x + radius;
      tmbbox[BOXLEFT]   = x - radius;

      xl = (tmbbox[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
      xh = (tmbbox[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
      yl = (tmbbox[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
      yh = (tmbbox[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
      for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	  mp->BlockThingsIterator(bx,by,PIT_PushThing);
      return;
    }

  // constant pushers p_wind and p_current
  int ht = 0;

  if (sec->heightsec != -1) // special water sector?
    ht = mp->sectors[sec->heightsec].floorheight;

  Actor   *thing;
  int xspeed, yspeed;
  msecnode_t *node = sec->touching_thinglist; // things touching this sector

  for ( ; node ; node = node->m_snext)
    {
      thing = node->m_thing;
      if (thing->flags & (MF_NOGRAVITY | MF_NOCLIPLINE))
	continue;

      // not a player? FIXME wind should also affect monsters....
      if (!thing->IsOf(PlayerPawn::_type))
	continue;

      if (type == p_wind)
	{
	  if (sec->heightsec == -1) // NOT special water sector
	    if (thing->z > thing->floorz) // above ground
	      {
		xspeed = x_mag; // full force
		yspeed = y_mag;
	      }
	    else // on ground
	      {
		xspeed = (x_mag)>>1; // half force
		yspeed = (y_mag)>>1;
	      }
	  else // special water sector
	    {
	      if (thing->z > ht) // above ground
		{
		  xspeed = x_mag; // full force
		  yspeed = y_mag;
		}
	      else if (thing->z + thing->height < ht) // underwater
		xspeed = yspeed = 0; // no force
	      else // wading in water
		{
		  xspeed = (x_mag)>>1; // half force
		  yspeed = (y_mag)>>1;
		}
	    }
	}
      else // p_current
	{
	  if (sec->heightsec == -1) // NOT special water sector
	    if (thing->z > sec->floorheight) // above ground
	      xspeed = yspeed = 0; // no force
	    else // on ground
	      {
		xspeed = x_mag; // full force
		yspeed = y_mag;
	      }
	  else // special water sector
	    if (thing->z > ht) // above ground
	      xspeed = yspeed = 0; // no force
	    else // underwater
	      {
		xspeed = x_mag; // full force
		yspeed = y_mag;
	      }
	}
      thing->px += xspeed<<(FRACBITS-PUSH_FACTOR);
      thing->py += yspeed<<(FRACBITS-PUSH_FACTOR);
    }
}

// Get pusher object.
DActor *Map::GetPushThing(int s)
{
  Actor* thing;
  sector_t* sec;

  sec = sectors + s;
  thing = sec->thinglist;
  while (thing)
    {
      if (thing->IsOf(DActor::_type))
	{
	  DActor *dp = (DActor *)thing;
	  switch (dp->type)
	    {
	    case MT_PUSH:
	    case MT_PULL:
	      return dp;
	    default:
	      break;
	    }
	}
      thing = thing->snext;
    }
  return NULL;
}

// Spawn pushers.
void Map::SpawnPushers()
{
  int i;
  line_t *l = lines;
  register int s;

  for (i = 0; i < numlines; i++, l++)
    {
      bool clear = true;
      switch (l->special)
	{
	case 224: // wind
	  for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )	  
	    AddThinker(new pusher_t(pusher_t::p_wind, l->dx, l->dy, NULL, s));
	  break;
	case 225: // current
	  for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )
	    AddThinker(new pusher_t(pusher_t::p_current, l->dx, l->dy, NULL, s));
	  break;
	case 226: // push/pull
	  for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )
	    {
	      // TODO don't spawn push mapthings at all?
	      DActor* thing = GetPushThing(s);
	      if (thing) // No MT_P* means no effect
		AddThinker(new pusher_t(pusher_t::p_push, l->dx, l->dy, thing, s));
	    }
	  break;

	default:
	  clear = false;
	}

      if (clear)
	l->special = 0;
    }
}
