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
// Revision 1.36  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.35  2004/10/14 19:35:30  smite-meister
// automap, bbox_t
//
// Revision 1.34  2004/09/06 19:58:03  smite-meister
// Doom linedefs done!
//
//
// Revision 1.29  2004/08/15 18:08:28  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.28  2004/08/12 18:30:24  smite-meister
// cleaned startup
//
// Revision 1.27  2004/07/05 16:53:26  smite-meister
// Netcode replaced
//
// Revision 1.26  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.23  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.22  2003/12/23 18:06:06  smite-meister
// Hexen stairbuilders. Moving geometry done!
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
// Revision 1.10  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.9  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.8  2003/04/14 08:58:27  smite-meister
// Hexen maps load.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Utilities, LineDef and Sector specials
///
/// Map geometry utility functions
/// Line tag hashing
/// Linedef specials
/// Sector specials
/// Scrollers, friction, pushers


#include "doomdef.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_spec.h"
#include "p_maputl.h"
#include "r_data.h"

#include "m_bbox.h" // bounding boxes
#include "m_random.h"
#include "tables.h"
#include "z_zone.h"


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

  secnum = sec-sectors;
  int linecount = sec->linecount;
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

  secnum = sec-sectors;
  int linecount = sec->linecount;
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




//====================================================================
//                       Sector specials
//====================================================================


/// We try to be as tolerant as possible here.
/// Internally we use modified "generalized Boom sector types".
/// The high byte is used for flags.
int Map::SpawnSectorSpecial(int sp, sector_t *sec)
{
  enum
  {
    DOOM_Light_Flicker    = 1,
    DOOM_Light_BlinkFast  = 2,
    DOOM_Light_BlinkSlow  = 3,
    DOOM_Light_StrobeHurt = 4,
    DOOM_Damage_Hellslime = 5,

    DOOM_Unused1,
    DOOM_Damage_Nukage     = 7,
    DOOM_Light_Glow        = 8,
    DOOM_Secret            = 9,
    DOOM_SpawnDoorClose30s = 10,

    DOOM_Damage_EndLevel   = 11,
    DOOM_Light_SyncFast    = 12,
    DOOM_Light_SyncSlow    = 13,
    DOOM_SpawnDoorOpen5min = 14,
    DOOM_Unused2,

    DOOM_DamageSuperHellslime = 16,
    DOOM_Light_Fireflicker    = 17,

    BOOM_LIGHTMASK    = 0x001F, // bits 0-4
    BOOM_DAMAGEMASK   = 0x0060, // bits 5-6
    BOOM_Secret       = 0x0080, // bit 7

    HERETIC_Lava_FlowEast = 4,
    HERETIC_Lava_Wimpy    = 5,
    HERETIC_Damage_Sludge = 7,
    HERETIC_Friction_Low  = 15,
    HERETIC_Lava_Hefty    = 16,

    HEXEN_Light_Phased = 1,
    HEXEN_Light_SequenceStart = 2,
  };


  if (sp == 0)
    {
      sec->special = 0;
      return 0;
    }

  CONS_Printf("sec %d: %d  => ", sec-sectors, sp);

  if (sp == DOOM_Secret)
    {
      secrets++;
      sp &= ~SS_SPECIALMASK; // zero low byte
      sp |= SS_secret;
      sec->special = sp;
      return sp;
    }

  const char HScrollDirs[4][2] = {{1,0}, {0,1}, {0,-1}, {-1,0}};
  const char HScrollSpeeds[5] = { 5, 10, 25, 30, 35 };
  const float d = 0.707;
  const float XScrollDirs[8][2] = {{0,1}, {1,0}, {0,-1}, {-1,0}, {-d,d}, {d,d}, {d,-d}, {-d,-d}};

  int temp;
  if (hexen_format)
    {
      // Boom damage (and secret) bits cannot be interpreted 'cos they are used otherwise

      temp = sp & SS_SPECIALMASK; // low byte

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
	case HEXEN_Light_Phased:
	  // Hardcoded base, use sector->lightlevel as the index
	  new phasedlight_t(this, sec, 80, -1);
	  break;

	case HEXEN_Light_SequenceStart:
	  SpawnPhasedLightSequence(sec, 1);
	  break;

	case SS_LightSequence_1:
	case SS_LightSequence_2:
	  // Phased light sequencing. Leave them be, they fit into the unused lightmask area.
	case SS_Stairs_Special1:
	case SS_Stairs_Special2:
	  // Same with stair sequences.
	case SS_IndoorLightning1:
	case SS_IndoorLightning2:
	case SS_Sky2:
	  // and these.
	  sec->special = sp;
	  return sp;

	default:
	  break;
	}

      sp &= ~SS_SPECIALMASK; // zero low byte
      sec->special = sp;
      return sp;
    }

  // Hexen done, Doom/Boom/Heretic to go

  if (sp & BOOM_Secret)
    {
      secrets++;
      sp |= SS_secret;
    }

  if (game.mode == gm_heretic)
    {
      temp = sp & 0x3F; // six lowest bits
      sp &= ~SS_SPECIALMASK; // zero low byte

      if (temp < 20)
	switch (temp)
	  {
	  case HERETIC_Damage_Sludge:
	    sec->damage = 4;
	    sec->damagetype = dt_corrosive;
	    sp |= SS_damage_32;
	    break;

	  case HERETIC_Lava_FlowEast:
	    AddThinker(new scroll_t(scroll_t::sc_carry_floor, 2048*28, 0, NULL, sec - sectors, false));
	    // fallthru
	  case HERETIC_Lava_Wimpy:
	    sec->damage = 5;
	    sec->damagetype = dt_heat;
	    sp |= SS_damage_16;
	    break;

	  case HERETIC_Lava_Hefty:
	    sec->damage = 8;
	    sec->damagetype = dt_heat;
	    sp |= SS_damage_16;
	    break;

	  case HERETIC_Friction_Low:
	    sec->friction = 0.97266f;
	    sec->movefactor = 0.25f;
	    sp |= SS_friction;
	    break;

	  default:
	    // all the rest of the specials are identical to Doom
	    // so they are handled using the Doom code later on
	    sp |= temp;
	  }
      else if (temp <= 39)
	{
	  // Heretic scrollers
	  temp -= 20; // zero base

	  fixed_t dx = HScrollDirs[temp/5][0]*HScrollSpeeds[temp%5]*2048;
	  fixed_t dy = HScrollDirs[temp/5][1]*HScrollSpeeds[temp%5]*2048;

	  // texture scrolls, actors are pushed
	  AddThinker(new scroll_t(scroll_t::sc_floor | scroll_t::sc_carry_floor,
				  dx, dy, NULL, sec - sectors, false));
	}
      else if (temp <= 51)
	{
	  // Heretic winds
	  temp -= 40; // zero base

	  fixed_t dx = HScrollDirs[temp/3][0]*HScrollSpeeds[temp%3]*2048;
	  fixed_t dy = HScrollDirs[temp/3][1]*HScrollSpeeds[temp%3]*2048;
	  
	  AddThinker(new scroll_t(scroll_t::sc_carry_floor | scroll_t::sc_wind,
				  dx, dy, NULL, sec - sectors, false));
	}
    }
  else
    {
      const char BoomDamage[4] = {0, 5, 10, 20};
      // in Heretic (and Hexen), Boom damage bits cannot be used because of winds and scrollers
      // Boom damage flags
      temp = (sp & BOOM_DAMAGEMASK) >> 5;
      if (temp)
	{
	  sp |= SS_damage_32;
	  sec->damage = BoomDamage[temp];
	  sec->damagetype = dt_radiation; // could as well choose randomly?      
	}
    }


  // Doom / Boom / some Heretic types

  temp = sp & BOOM_LIGHTMASK;
  sp &= ~SS_SPECIALMASK; // zero low byte

  int i, dam = 0;
  lightfx_t *lfx = NULL;

  const short ff_tics = 4;
  const short glowspeed = 8;

  switch (temp)
    {
    case DOOM_Damage_Nukage:  // nukage/slime
      dam = 5;
      break;

    case DOOM_Damage_Hellslime:
      dam = 10;
      break;

    case DOOM_Light_StrobeHurt:
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, false); // fallthru
    case DOOM_DamageSuperHellslime:
      dam = 20;
      break;

    case DOOM_Damage_EndLevel: // level end hurt need special handling
      dam = 20;
      sp |= 11; // FIXME
      break;

    case DOOM_SpawnDoorClose30s: // after 30 s, close door
      SpawnDoorCloseIn30(sec);
      break;

    case DOOM_SpawnDoorOpen5min: // after 5 min, open door
      SpawnDoorRaiseIn5Mins(sec);
      break;

    case DOOM_Light_Flicker:
      i = P_FindMinSurroundingLight(sec, sec->lightlevel);
      lfx = new lightfx_t(this, sec, lightfx_t::Flicker, sec->lightlevel, i, 64, 7);
      lfx->count = (P_Random() & lfx->maxtime) + 1;
      break;

    case DOOM_Light_BlinkFast:
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, false);
      break;

    case DOOM_Light_BlinkSlow:
      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, false);
      break;

    case DOOM_Light_Glow:
      i = P_FindMinSurroundingLight(sec, sec->lightlevel);
      lfx = new lightfx_t(this, sec, lightfx_t::Glow, sec->lightlevel, i, -glowspeed);
      break;

    case DOOM_Light_SyncFast:
      SpawnStrobeLight(sec, STROBEBRIGHT, FASTDARK, true);
      break;

    case DOOM_Light_SyncSlow:
      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, true);
      break;

    case DOOM_Light_Fireflicker:
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
  CONS_Printf("%d\n", sp);

  sec->special = sp;
  return sp;
}






//====================================================================
//         Clearable line specials (handled during Map setup)
//====================================================================


//SoM: 3/23/2000: Adds a sectors floor and ceiling to a sector's ffloor list
static void P_AddFFloor(sector_t* sec, ffloor_t* ffloor)
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



/// After the map has been loaded, scan for linedefs
/// that spawn thinkers or confer properties
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

  InitTagLists(); // Create xref tables for tags

  // subtypes
  const int LEGACY_BOOM_SCROLLERS = 0;
  const int LEGACY_BOOM_FRICTION  = 1;
  const int LEGACY_BOOM_PUSHERS   = 2;
  const int LEGACY_BOOM_RENDERER  = 3;
  const int LEGACY_BOOM_EXOTIC    = 4;
  const int LEGACY_FF        = 10;
  const int LEGACY_RENDERER  = 11;
  const int LEGACY_MISC      = 13;

  //  Init line EFFECTs
  for (i = 0; i < numlines; i++)
    {
      line_t *l = &lines[i];
      line_t *l2;
      int special = l->special;

      int s, subtype;

      // Legacy extensions are mapped to Hexen linedef namespace so they are reachable from Hexen as well!
      // only check for clearable stuff here
      if (special == LEGACY_EXT && (subtype = l->args[0]) < 128)
	{
	  int tag = l->tag; // Doom format: use the tag if we have one
	  if (!tag)
	    tag = l->args[3] + 256 * l->args[4]; // Hexen format: get the tag from args[3-4]

	  int sec = sides[*l->sidenum].sector - sectors;
	  int kind = l->args[1];


	  if (subtype == LEGACY_BOOM_SCROLLERS)
	    {
	      SpawnScroller(l, tag, kind, l->args[2]);
	    }
	  else if (subtype == LEGACY_BOOM_FRICTION)
	    {
	      SpawnFriction(l, tag);
	    }
	  else if (subtype == LEGACY_BOOM_PUSHERS)
	    {
	      SpawnPusher(l, tag, kind);
	    }
	  else if (subtype == LEGACY_BOOM_RENDERER)
	    {
	      switch (kind)
		{
		  // Boom: 213 floor lighting independently (e.g. lava)
		case 0:
		  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
		    sectors[s].floorlightsec = sec;
		  break;

		  // Boom: 261 ceiling lighting independently
		case 1:
		  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
		    sectors[s].ceilinglightsec = sec;
		  break;

		default:
		  goto error;
		}
	    }
	  else if (subtype == LEGACY_BOOM_EXOTIC)
	    {
	      // types which store data in the texture name fields
	      switch (kind)
		{
		  // Boom: 242 fake floor and ceiling
		case 1:
		  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
		    {
		      sectors[s].heightsec = sec;
		    }
		  break;

		  // Boom: 260 transparent middle texture
		case 2:
		  {
		    int temp = sides[*l->sidenum].special; // transmap number stored here
		    if (temp == -1)
		      temp = 0; // default, TRANMAP

		    if (!tag)
		      l->transmap = temp;
		    else for (s = -1; (l2 = FindLineFromTag(tag, &s)); )
		      l2->transmap = temp; // make tagged lines translucent too
		  }
		  break;

		  // Legacy: swimmable water with Boom 242-style colormaps
		case 3:
		  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
		    {
		      sectors[s].heightsec = sec;
		      sectors[s].altheightsec = 1;
		    }
		  break;

		  // Legacy: easy colormap/fog effect
		case 4:
		  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
		    {
		      sectors[s].midmap = l->frontsector->midmap;
		      sectors[s].altheightsec = 2;
		    }
		  break;

		default:
		  goto error;
		}
	    }
	  else if (subtype == LEGACY_FF)  // fake floors
	    {
	      int ff_flags = FF_EXISTS;

	      // make it simple for now
	      switch (kind + 281)
		{
		case 281: // Legacy: 3D floor
		  ff_flags |= FF_SOLID|FF_RENDERALL|FF_CUTLEVEL;
		  break;

		case 289: // Legacy: 3D floor without shadow
		  ff_flags |= FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_CUTLEVEL;
		  break;

		case 300: // Legacy: translucent 3D floor
		  ff_flags |= FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA;
		  break;

		case 301: // Legacy: translucent swimmable water
		  ff_flags |= FF_RENDERALL|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES |
		    FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
		  break;

		case 304: // Legacy: Opaque water
		  ff_flags |= FF_RENDERALL|FF_SWIMMABLE|FF_BOTHPLANES |
		    FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
		  break;

		case 302: // Legacy: 3D fog
		  // SoM: Because it's fog, check for an extra colormap and set the fog flag...
		  if (sectors[sec].extra_colormap)
		    sectors[sec].extra_colormap->fog = 1;
		  ff_flags |= FF_RENDERALL|FF_FOG|FF_BOTHPLANES|FF_INVERTPLANES |
		    FF_ALLSIDES|FF_INVERTSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
		  break;

		case 303: // Legacy: Light effect
		  ff_flags |= FF_CUTSPRITES;
		  break;

		case 305: // Legacy: Double light effect
		  ff_flags |= FF_CUTSPRITES|FF_DOUBLESHADOW;
		  break;

		default:
		  goto error;
		}

	      if (ff_flags)
		for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0; )
		  AddFakeFloor(&sectors[s], &sectors[sec], lines+i, ff_flags);
	    }
	  else if (subtype == LEGACY_RENDERER)
	    {
	      if (kind >= 100)
		l->transmap = kind - 100; // transmap number
	      else if (kind == 0) // 283 (legacy fog sheet)
		continue;  // FIXME fog sheet requires keeping (renderer!, r_segs):
	    }
	  else if (subtype == LEGACY_MISC)
	    {
	      switch (kind)
		{
		  // Instant lower for floor SSNTails 06-13-2002
		case 0:
		  EV_DoFloor(tag, l, floor_t::LnF, MAXINT/2, 0, 0);
		  break;
	  
		  // Instant raise for ceilings SSNTails 06-13-2002
		case 1:
		  EV_DoCeiling(tag, l, ceiling_t::HnC, MAXINT/2, 0, 0);
		  break;

		default:
		  goto error;
		}
	    }

	  l->special = 0;
	  continue;

	error:
	  I_Error("Unknown Legacy linedef subtype %d in line %d.\n", kind, i);
	}


      // finally check ungrouped specials 
      bool clear = true;

      switch (special)
        {
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
	  // TODO is this used? if not, replace it with a thing...
	  if (special >= 1000 && special < 1032)
            {
	      for (s = -1; (s = FindSectorFromTag(l->tag, s)) >= 0;)
		sectors[s].teamstartsec = special - 999; // only 999 so we know when it is set (it's != 0)
	      break;
            }
	  else
	    clear = false;
        }


      if (clear)
	l->special = 0;
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

  if (type == sc_side)  //Scroll wall texture
    {
      side_t *side = mp->sides + affectee;
      side->textureoffset += tdx;
      side->rowoffset += tdy;
      return;
    }

  sector_t *sec = mp->sectors + affectee;

  if (type & sc_floor)  //Scroll floor texture
    {
      sec->floor_xoffs += tdx;
      sec->floor_yoffs += tdy;
    }
  
  if (type & sc_ceiling)  //Scroll ceiling texture
    {
      sec->ceiling_xoffs += tdx;
      sec->ceiling_yoffs += tdy;
    }


  // Factor to scale scrolling effect into mobj-carrying properties = 3/32.
  // (This is so scrolling floors and objects on them can move at same speed.)
  const fixed_t CARRYFACTOR = fixed_t(FRACUNIT * 0.09375);

  if (type & sc_carry_floor)
    {
      tdx = -FixedMul(tdx, CARRYFACTOR); // it seems texture offsets go the other way?
      tdy = FixedMul(tdy, CARRYFACTOR);

      fixed_t height = sec->floorheight;
      fixed_t waterheight = sec->heightsec != -1 &&
        mp->sectors[sec->heightsec].floorheight > height ?
        mp->sectors[sec->heightsec].floorheight : MININT;

      for (msecnode_t *node = sec->touching_thinglist; node; node = node->m_snext)
	{
	  Actor *thing = node->m_thing;

	  if (type & sc_wind && !(thing->flags2 & MF2_WINDTHRUST))
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

scroll_t::scroll_t(short t, fixed_t dx, fixed_t dy, sector_t *csec, int aff, bool acc)
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


// Initialize the scrollers
/*
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

	default:
	  clear = false;
        }

      if (clear)
	l->special = 0;
    }
}
*/


/// spawns Boom wall, floor and ceiling scrollers
void Map::SpawnScroller(line_t *l, int tag, int type, int control)
{
  int s = l->sidenum[0];
  line_t *l2;
  fixed_t dx = l->dx >> SCROLL_SHIFT;  // direction and speed of scrolling
  fixed_t dy = l->dy >> SCROLL_SHIFT;

  // possible control sector
  sector_t *csec = (control & scroll_t::sc_displacement) ? sides[s].sector : NULL;
  bool accel = control & scroll_t::sc_accelerative;

  if (type == scroll_t::sc_side)
    {
      if (control & scroll_t::sc_offsets)
	{
	  AddThinker(new scroll_t(scroll_t::sc_side, -sides[s].textureoffset,
				  sides[s].rowoffset, NULL, s, accel));
	}
      else
	for (s = -1; (l2 = FindLineFromTag(tag, &s)) != NULL; )
	  if (l2 != l)
	    AddThinker(Add_WallScroller(dx, dy, l2, csec, accel));
      return;
    }

  // okay, the rest of the types are stackable (flags)
  for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0;)
    AddThinker(new scroll_t(type, -dx, dy, csec, s, accel));
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
void Map::SpawnFriction(line_t *l, int tag)
{
  extern float normal_friction;

  // line length controls magnitude
  float length = P_AproxDistance(l->dx,l->dy) >> FRACBITS;

  // l = 200 gives 1, l = 100 gives the original, l = 0 gives 0.8125

  // friction value to be applied during movement
  //friction = (0x1EB8*length)/0x80 + 0xD000;
  float friction = length * 9.375e-4 + 0.8125;

  if (friction > 1.0)
    friction = 1.0;
  if (friction < 0.0)
    friction = 0.0;

  // object max speed is proportional to movefactor/(1-friction)
  // higher friction value actually means 'less friction'.
  // the movefactors are a bit different from BOOM (better;)

  float movefactor; // applied to each player move to simulate inertia

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

  for (int s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )
    {
      //AddThinker(new friction_t(friction,movefactor,s));
      sectors[s].friction   = friction;
      sectors[s].movefactor = movefactor;
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

  if (type == p_point)
    {
      int xl,xh,yl,yh,bx,by;
      // Seek out all pushable things within the force radius of this
      // point pusher. Crosses sectors, so use blockmap.

      tmpusher = this; // MT_PUSH/MT_PULL point source
      tmb.Set(x, y, radius);

      xl = (tmb[BOXLEFT] - mp->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
      xh = (tmb[BOXRIGHT] - mp->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
      yl = (tmb[BOXBOTTOM] - mp->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
      yh = (tmb[BOXTOP] - mp->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
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
  sector_t *sec = sectors + s;
  Actor *thing = sec->thinglist;

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
void Map::SpawnPusher(line_t *l, int tag, int type)
{
  int s;

  switch (type)
    {
    case pusher_t::p_wind:
      for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )	  
	AddThinker(new pusher_t(pusher_t::p_wind, l->dx, l->dy, NULL, s));
      break;

    case pusher_t::p_current:
      for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )
	AddThinker(new pusher_t(pusher_t::p_current, l->dx, l->dy, NULL, s));
      break;

    case pusher_t::p_point:
      for (s = -1; (s = FindSectorFromTag(tag, s)) >= 0 ; )
	{
	  DActor* thing = GetPushThing(s);
	  if (thing) // No MT_P* means no effect
	    AddThinker(new pusher_t(pusher_t::p_point, l->dx, l->dy, thing, s));
	}
      break;
    }
}
