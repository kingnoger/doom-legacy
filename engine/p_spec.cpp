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
//      Implements special effects:
//      Texture animation, height or lighting changes
//       according to adjacent sectors, respective
//       utility functions, etc.
//      Line Tag handling. Line and Sector triggers.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "d_netcmd.h"

#include "p_setup.h"    //levelflats for flat animation
#include "p_maputl.h"

#include "m_bbox.h" // bounding boxes
#include "m_random.h"

#include "r_data.h"
#include "r_state.h"

#include "s_sound.h"
#include "dstrings.h" //SoM: 3/10/2000
#include "r_main.h"   //Two extra includes.
#include "t_script.h"

#include "hardware/hw3sound.h"

#include "w_wad.h"
#include "z_zone.h"


//SoM: Enable Boom features?
int boomsupport = 1;
int variable_friction = 1;
int allow_pushers = 1;



//SoM: 3/7/2000
void P_FindAnimatedFlat (int i);


//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated.
//
struct anim_t
{
  char        istexture; // bool doesn't work, this can also take the value -1 !
  int         picnum;
  int         basepic;
  int         numpics;
  int         speed;
};


//
//      source animation definition
//
#pragma pack(1) //Hurdler: 04/04/2000: I think pragma is more portable
struct animdef_t
{
  char        istexture;      // if false, it is a flat, -1 is a terminator
  char        endname[9];
  char        startname[9];
  int         speed;
};
#pragma pack()


//SoM: 3/7/2000: New sturcture without limits.
// FIXME each map must have its own!
static anim_t*   lastanim;
static anim_t*   anims;
static size_t    maxanims;

//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//
animdef_t harddefs[] =
{
  // DOOM II flat animations.
  {false,     "NUKAGE3",      "NUKAGE1",      8},
  {false,     "FWATER4",      "FWATER1",      8},
  {false,     "SWATER4",      "SWATER1",      8},
  {false,     "LAVA4",        "LAVA1",        8},
  {false,     "BLOOD3",       "BLOOD1",       8},

  {false,     "RROCK08",      "RROCK05",      8},
  {false,     "SLIME04",      "SLIME01",      8},
  {false,     "SLIME08",      "SLIME05",      8},
  {false,     "SLIME12",      "SLIME09",      8},

    // animated textures
  {true,      "BLODGR4",      "BLODGR1",      8},
  {true,      "SLADRIP3",     "SLADRIP1",     8},

  {true,      "BLODRIP4",     "BLODRIP1",     8},
  {true,      "FIREWALL",     "FIREWALA",     8},
  {true,      "GSTFONT3",     "GSTFONT1",     8},
  {true,      "FIRELAVA",     "FIRELAV3",     8},
  {true,      "FIREMAG3",     "FIREMAG1",     8},
  {true,      "FIREBLU2",     "FIREBLU1",     8},
  {true,      "ROCKRED3",     "ROCKRED1",     8},

  {true,      "BFALL4",       "BFALL1",       8},
  {true,      "SFALL4",       "SFALL1",       8},
  {true,      "WFALL4",       "WFALL1",       8},
  {true,      "DBRAIN4",      "DBRAIN1",      8},

  // heretic 
  {false,     "FLTWAWA3",     "FLTWAWA1",     8}, // Water
  {false,     "FLTSLUD3",     "FLTSLUD1",     8}, // Sludge
  {false,     "FLTTELE4",     "FLTTELE1",     6}, // Teleport
  {false,     "FLTFLWW3",     "FLTFLWW1",     9}, // River - West
  {false,     "FLTLAVA4",     "FLTLAVA1",     8}, // Lava
  {false,     "FLATHUH4",     "FLATHUH1",     8}, // Super Lava
  {true,      "LAVAFL3",      "LAVAFL1",      6}, // Texture: Lavaflow
  {true,      "WATRWAL3",     "WATRWAL1",     4}, // Texture: Waterfall

  {-1}
};

animdef_t nulldefs[] = {
  {-1}
};

//
//      Animating line specials
//

//
// Init animated textures
// - now called at level loading P_SetupLevel()
//

static animdef_t *animdefs;

//SoM: 3/7/2000: Use new boom method of reading lump from wad file.
void P_InitPicAnims ()
{
  //  Init animation
  int         i;



  if (fc.FindNumForName("ANIMATED") != -1)
    animdefs = (animdef_t *)fc.CacheLumpName("ANIMATED", PU_STATIC);
  if (game.mode == gm_hexen)
    animdefs = nulldefs; // TODO hexen ANIMDEFS lump...
  else
    animdefs = harddefs;

  // count anims
  maxanims = 0;
  for (i = 0; animdefs[i].istexture != -1; i++);
  maxanims = i;

  lastanim = anims = (anim_t *)malloc(sizeof(anim_t) * (maxanims + 1));

  for (i = 0; animdefs[i].istexture != -1; i++)
    {
      if (animdefs[i].istexture)
	{
	  // different episode ?
	  if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
	    continue;

	  lastanim->picnum = R_TextureNumForName(animdefs[i].endname);
	  lastanim->basepic = R_TextureNumForName(animdefs[i].startname);
	}
      else
	{
	  if ((fc.FindNumForName(animdefs[i].startname)) == -1)
	    continue;

	  lastanim->picnum = R_FlatNumForName(animdefs[i].endname);
	  lastanim->basepic = R_FlatNumForName(animdefs[i].startname);
	}

      lastanim->istexture = animdefs[i].istexture;
      lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

      if (lastanim->numpics < 2)
        I_Error ("P_InitPicAnims: bad cycle from %s to %s",
		 animdefs[i].startname, animdefs[i].endname);

      lastanim->speed = LONG(animdefs[i].speed) * NEWTICRATERATIO;
      lastanim++;
    }
  lastanim->istexture = -1;

  // FIXME animdefs is used even after this!
  if(animdefs != harddefs)
    Z_ChangeTag (animdefs,PU_CACHE);
}

//  Check for flats in levelflats, that are part
//  of a flat anim sequence, if so, set them up for animation
//
//SoM: 3/16/2000: Changed parameter from pointer to "anims" entry number
void P_FindAnimatedFlat(int animnum)
{
  int            i;
  int            startflatnum,endflatnum;
  levelflat_t*   foundflats = levelflats;

  startflatnum = anims[animnum].basepic;
  endflatnum   = anims[animnum].picnum;

  // note: high word of lumpnum is the wad number
  if ( (startflatnum>>16) != (endflatnum>>16) )
    I_Error ("AnimatedFlat start %s not in same wad as end %s\n",
	     animdefs[animnum].startname, animdefs[animnum].endname);

  //
  // now search through the levelflats if this anim flat sequence is used
  //
  for (i = 0; i<numlevelflats; i++, foundflats++)
    {
      // is that levelflat from the flat anim sequence ?
      if (foundflats->lumpnum >= startflatnum &&
	  foundflats->lumpnum <= endflatnum)
        {
	  foundflats->baselumpnum = startflatnum;
	  foundflats->animseq = foundflats->lumpnum - startflatnum;
	  foundflats->numpics = endflatnum - startflatnum + 1;
	  foundflats->speed = anims[animnum].speed;

	  if (devparm)
	    CONS_Printf("animflat: %#03d name:%.8s animseq:%d numpics:%d speed:%d\n",
			i, foundflats->name, foundflats->animseq,
			foundflats->numpics,foundflats->speed);
        }
    }
}


//
//  Called by P_LoadSectors
//
void P_SetupLevelFlatAnims()
{
  int    i;

  // the original game flat anim sequences
  for (i=0; anims[i].istexture != -1; i++)
    {
      if (!anims[i].istexture)
        {
	  P_FindAnimatedFlat(i);
        }
    }
}


//
// UTILITIES
//


//
// was getSide
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t *Map::getSide(int sec, int line, int side)
{
  return &sides[ (sectors[sec].lines[line])->sidenum[side] ];
}


//
// was getSector
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t *Map::getSector(int sec, int line, int side)
{
  return sides[ (sectors[sec].lines[line])->sidenum[side] ].sector;
}


//
// was twoSided
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
//SoM: 3/7/2000: Use the boom method
int Map::twoSided(int sec, int line)
{
  return boomsupport ?
    ((sectors[sec].lines[line])->sidenum[1] != -1)
    :
    ((sectors[sec].lines[line])->flags & ML_TWOSIDED);
}


//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
//SoM: 3/7/2000: Use boom method.
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




//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
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




//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
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



//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
// SoM: 3/7/2000: Use Lee Killough's version insted.
// Rewritten by Lee Killough to avoid fixed array and to be faster
//
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

// P_FindNextLowestFloor()
//
// Passed a sector and a floor height, returns the fixed point value
// of the largest floor height in a surrounding sector smaller than
// the floor height passed. If no such height exists the floorheight
// passed is returned.
//
//
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


//
// P_FindNextLowestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the largest ceiling height in a surrounding sector smaller than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
//
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


//
// P_FindNextHighestCeiling()
//
// Passed a sector and a ceiling height, returns the fixed point value
// of the smallest ceiling height in a surrounding sector larger than
// the ceiling height passed. If no such height exists the ceiling height
// passed is returned.
//
//
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



//
// was P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
//
fixed_t Map::FindShortestTextureAround(int secnum)
{
  int minsize = MAXINT;
  side_t*     side;
  int i;
  sector_t *sec = &sectors[secnum];

  if (boomsupport)
    minsize = 32000<<FRACBITS;

  for (i = 0; i < sec->linecount; i++)
    {
      if (twoSided(secnum, i))
	{
	  side = getSide(secnum,i,0);
	  if (side->bottomtexture > 0)
	    if (textureheight[side->bottomtexture] < minsize)
	      minsize = textureheight[side->bottomtexture];
	  side = getSide(secnum,i,1);
	  if (side->bottomtexture > 0)
	    if (textureheight[side->bottomtexture] < minsize)
	      minsize = textureheight[side->bottomtexture];
	}
    }
  return minsize;
}



//
// was P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
//
fixed_t Map::FindShortestUpperAround(int secnum)
{
  int minsize = MAXINT;
  side_t*     side;
  int i;
  sector_t *sec = &sectors[secnum];

  if (boomsupport)
    minsize = 32000<<FRACBITS;

  for (i = 0; i < sec->linecount; i++)
    {
      if (twoSided(secnum, i))
	{
	  side = getSide(secnum,i,0);
	  if (side->toptexture > 0)
	    if (textureheight[side->toptexture] < minsize)
	      minsize = textureheight[side->toptexture];
	  side = getSide(secnum,i,1);
	  if (side->toptexture > 0)
	    if (textureheight[side->toptexture] < minsize)
	      minsize = textureheight[side->toptexture];
	}
    }
  return minsize;
}




//SoM: 3/7/2000
//
// was P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
//sector_t *P_FindModelFloorSector(fixed_t floordestheight,int secnum)
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



//SoM: 3/7/2000: Last one...
//
// was P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
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



// was P_FindSectorFromLineTag
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//
//SoM: 3/7/2000: Killough wrote this to improve the process.
int Map::FindSectorFromLineTag(line_t *line, int start)
{
  start = (start >= 0) ? sectors[start].nexttag :
    sectors[(unsigned) line->tag % (unsigned) numsectors].firsttag;
  while (start >= 0 && sectors[start].tag != line->tag)
    start = sectors[start].nexttag;
  return start;
}



//
// was P_FindSectorFromTag
// Used by FraggleScript
int Map::FindSectorFromTag(int tag, int start)
{
  start = start >= 0 ? sectors[start].nexttag :
    sectors[(unsigned) tag % (unsigned) numsectors].firsttag;
  while (start >= 0 && sectors[start].tag != tag)
    start = sectors[start].nexttag;
  return start;
}

// was P_FindLineFromLineTag
//SoM: 3/7/2000: More boom specific stuff...
// killough 4/16/98: Same thing, only for linedefs

int Map::FindLineFromLineTag(const line_t *line, int start)
{
  start = start >= 0 ? lines[start].nexttag :
    lines[(unsigned) line->tag % (unsigned) numlines].firsttag;
  while (start >= 0 && lines[start].tag != line->tag)
    start = lines[start].nexttag;
  return start;
}


// was P_InitTagLists
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


//
// P_SectorActive()
//
// Passed a linedef special class (floor, ceiling, lighting) and a sector
// returns whether the sector is already busy with a linedef special of the
// same class. If old demo compatibility true, all linedef special classes
// are the same.
//
//
int P_SectorActive(special_e t, sector_t *sec)
{
  if (!boomsupport)
    return sec->floordata || sec->ceilingdata || sec->lightingdata;
  else
    switch (t)
      {
      case floor_special:
        return (int)sec->floordata;
      case ceiling_special:
        return (int)sec->ceilingdata;
      case lighting_special:
        return (int)sec->lightingdata;
      }
  return 1;
}


//SoM: 3/7/2000
//
// P_CheckTag()
//
// Passed a line, returns true if the tag is non-zero or the line special
// allows no tag without harm. If compatibility, all linedef specials are
// allowed to have zero tag.
//
// Note: Only line specials activated by walkover, pushing, or shooting are
//       checked by this routine.
//
//
int P_CheckTag(line_t *line)
{
  if (!boomsupport)
    return 1;

  if (line->tag)
    return 1;

  switch(line->special)
    {
    case 1:                 // Manual door specials
    case 26:
    case 27:
    case 28:
    case 31:
    case 32:
    case 33:
    case 34:
    case 117:
    case 118:

    case 139:               // Lighting specials
    case 170:
    case 79:
    case 35:
    case 138:
    case 171:
    case 81:
    case 13:
    case 192:
    case 169:
    case 80:
    case 12:
    case 194:
    case 173:
    case 157:
    case 104:
    case 193:
    case 172:
    case 156:
    case 17:

    case 195:               // Thing teleporters
    case 174:
    case 97:
    case 39:
    case 126:
    case 125:
    case 210:
    case 209:
    case 208:
    case 207:

    case 11:                // Exits
    case 52:
    case 197:
    case 51:
    case 124:
    case 198:

    case 48:                // Scrolling walls
    case 85:
      // FraggleScript types!
    case 272:   // WR
    case 273:
    case 274:   // W1
    case 275:
    case 276:   // SR
    case 277:   // S1
    case 278:   // GR
    case 279:   // G1
      return 1;   // zero tag allowed

    case 105: if( game.mode == gm_heretic )
      return 1;

    default:
      break;
    }
  return 0;       // zero tag not allowed
}



//SoM: 3/7/2000: Is/WasSecret.
//
// P_IsSecret()
//
// Passed a sector, returns if the sector secret type is still active, i.e.
// secret type is set and the secret has not yet been obtained.
//
bool P_IsSecret(sector_t *sec)
{
  return (sec->special==9 || (sec->special&SECRET_MASK));
}


//
// P_WasSecret()
//
// Passed a sector, returns if the sector secret type is was active, i.e.
// secret type was set and the secret has been obtained already.
//
bool P_WasSecret(sector_t *sec)
{
  return (sec->oldspecial==9 || (sec->oldspecial&SECRET_MASK));
}


//============================================================================
//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//
//============================================================================


// 
// P_ExecuteLineSpecial
// Hexen linedefs
//
bool Map::ExecuteLineSpecial(int special, byte *args, line_t *line, int side, Actor *mo)
{
  bool buttonSuccess = false;
  /* FIXME
  switch (special)
    {
    case 1: // Poly Start Line
      break;
    case 2: // Poly Rotate Left
      buttonSuccess = EV_RotatePoly(line, args, 1, false);
      break;
    case 3: // Poly Rotate Right
      buttonSuccess = EV_RotatePoly(line, args, -1, false);
      break;
    case 4: // Poly Move
      buttonSuccess = EV_MovePoly(line, args, false, false);
      break;
    case 5: // Poly Explicit Line:  Only used in initialization
      break;
    case 6: // Poly Move Times 8
      buttonSuccess = EV_MovePoly(line, args, true, false);
      break;
    case 7: // Poly Door Swing
      buttonSuccess = EV_OpenPolyDoor(line, args, PODOOR_SWING);
      break;
    case 8: // Poly Door Slide
      buttonSuccess = EV_OpenPolyDoor(line, args, PODOOR_SLIDE);
      break;
    case 10: // Door Close
      buttonSuccess = EV_DoDoor(line, args, DREV_CLOSE);
      break;
    case 11: // Door Open
      if(!args[0])
	{
	  buttonSuccess = EV_VerticalDoor(line, mo);
	}
      else
	{
	  buttonSuccess = EV_DoDoor(line, args, DREV_OPEN);
	}
      break;
    case 12: // Door Raise
      if(!args[0])
	{
	  buttonSuccess = EV_VerticalDoor(line, mo);
	}
      else
	{
	  buttonSuccess = EV_DoDoor(line, args, DREV_NORMAL);
	}
      break;
    case 13: // Door Locked_Raise
      if(CheckedLockedDoor(mo, args[3]))
	{
	  if(!args[0])
	    {
	      buttonSuccess = EV_VerticalDoor(line, mo);
	    }
	  else
	    {
	      buttonSuccess = EV_DoDoor(line, args, DREV_NORMAL);
	    }
	}
      break;
    case 20: // Floor Lower by Value
      buttonSuccess = EV_DoFloor(line, args, FLEV_LOWERFLOORBYVALUE);
      break;
    case 21: // Floor Lower to Lowest
      buttonSuccess = EV_DoFloor(line, args, FLEV_LOWERFLOORTOLOWEST);
      break;
    case 22: // Floor Lower to Nearest
      buttonSuccess = EV_DoFloor(line, args, FLEV_LOWERFLOOR);
      break;
    case 23: // Floor Raise by Value
      buttonSuccess = EV_DoFloor(line, args, FLEV_RAISEFLOORBYVALUE);
      break;
    case 24: // Floor Raise to Highest
      buttonSuccess = EV_DoFloor(line, args, FLEV_RAISEFLOOR);
      break;
    case 25: // Floor Raise to Nearest
      buttonSuccess = EV_DoFloor(line, args, FLEV_RAISEFLOORTONEAREST);
      break;
    case 26: // Stairs Build Down Normal
      buttonSuccess = EV_BuildStairs(line, args, -1, STAIRS_NORMAL);
      break;
    case 27: // Build Stairs Up Normal
      buttonSuccess = EV_BuildStairs(line, args, 1, STAIRS_NORMAL);
      break;
    case 28: // Floor Raise and Crush
      buttonSuccess = EV_DoFloor(line, args, FLEV_RAISEFLOORCRUSH);
      break;
    case 29: // Build Pillar (no crushing)
      buttonSuccess = EV_BuildPillar(line, args, false);
      break;
    case 30: // Open Pillar
      buttonSuccess = EV_OpenPillar(line, args);
      break;
    case 31: // Stairs Build Down Sync
      buttonSuccess = EV_BuildStairs(line, args, -1, STAIRS_SYNC);
      break;
    case 32: // Build Stairs Up Sync
      buttonSuccess = EV_BuildStairs(line, args, 1, STAIRS_SYNC);
      break;
    case 35: // Raise Floor by Value Times 8
      buttonSuccess = EV_DoFloor(line, args, FLEV_RAISEBYVALUETIMES8);
      break;
    case 36: // Lower Floor by Value Times 8
      buttonSuccess = EV_DoFloor(line, args, FLEV_LOWERBYVALUETIMES8);
      break;
    case 40: // Ceiling Lower by Value
      buttonSuccess = EV_DoCeiling(line, args, CLEV_LOWERBYVALUE);
      break;
    case 41: // Ceiling Raise by Value
      buttonSuccess = EV_DoCeiling(line, args, CLEV_RAISEBYVALUE);
      break;
    case 42: // Ceiling Crush and Raise
      buttonSuccess = EV_DoCeiling(line, args, CLEV_CRUSHANDRAISE);
      break;
    case 43: // Ceiling Lower and Crush
      buttonSuccess = EV_DoCeiling(line, args, CLEV_LOWERANDCRUSH);
      break;
    case 44: // Ceiling Crush Stop
      buttonSuccess = EV_CeilingCrushStop(line, args);
      break;
    case 45: // Ceiling Crush Raise and Stay
      buttonSuccess = EV_DoCeiling(line, args, CLEV_CRUSHRAISEANDSTAY);
      break;
    case 46: // Floor Crush Stop
      buttonSuccess = EV_FloorCrushStop(line, args);
      break;
    case 60: // Plat Perpetual Raise
      buttonSuccess = EV_DoPlat(line, args, PLAT_PERPETUALRAISE, 0);
      break;
    case 61: // Plat Stop
      EV_StopPlat(line, args);
      break;
    case 62: // Plat Down-Wait-Up-Stay
      buttonSuccess = EV_DoPlat(line, args, PLAT_DOWNWAITUPSTAY, 0);
      break;
    case 63: // Plat Down-by-Value*8-Wait-Up-Stay
      buttonSuccess = EV_DoPlat(line, args, PLAT_DOWNBYVALUEWAITUPSTAY,
				0);
      break;
    case 64: // Plat Up-Wait-Down-Stay
      buttonSuccess = EV_DoPlat(line, args, PLAT_UPWAITDOWNSTAY, 0);
      break;
    case 65: // Plat Up-by-Value*8-Wait-Down-Stay
      buttonSuccess = EV_DoPlat(line, args, PLAT_UPBYVALUEWAITDOWNSTAY, 0);
      break;
    case 66: // Floor Lower Instant * 8
      buttonSuccess = EV_DoFloor(line, args, FLEV_LOWERTIMES8INSTANT);
      break;
    case 67: // Floor Raise Instant * 8
      buttonSuccess = EV_DoFloor(line, args, FLEV_RAISETIMES8INSTANT);
      break;
    case 68: // Floor Move to Value * 8
      buttonSuccess = EV_DoFloor(line, args, FLEV_MOVETOVALUETIMES8);
      break;
    case 69: // Ceiling Move to Value * 8
      buttonSuccess = EV_DoCeiling(line, args, CLEV_MOVETOVALUETIMES8);
      break;
    case 70: // Teleport
      if(side == 0)
	{ // Only teleport when crossing the front side of a line
	  buttonSuccess = EV_Teleport(args[0], mo, true);
	}
      break;
    case 71: // Teleport, no fog
      if(side == 0)
	{ // Only teleport when crossing the front side of a line
	  buttonSuccess = EV_Teleport(args[0], mo, false);
	}
      break;
    case 72: // Thrust Mobj
      if(!side) // Only thrust on side 0
	{
	  mo->Thrust(args[0]*(ANGLE_90/64), args[1]<<FRACBITS);
	  buttonSuccess = 1;
	}
      break;
    case 73: // Damage Mobj
      if(args[0])
	{
	  mo->Damage(NULL, NULL, args[0]);
	}
      else
	{ // If arg1 is zero, then guarantee a kill
	  mo->Damage(NULL, NULL, 10000, dt_always);
	}
      buttonSuccess = 1;
      break;
    case 74: // Teleport_NewMap
      if(side == 0)
	{ // Only teleport when crossing the front side of a line
	  if(!(mo && mo->player && mo->player->playerstate
	       == PST_DEAD)) // Players must be alive to teleport
	    {
	      G_Completed(args[0], args[1]);
	      buttonSuccess = true;
	    }
	}
      break;
    case 75: // Teleport_EndGame
      if(side == 0)
	{ // Only teleport when crossing the front side of a line
	  if(!(mo && mo->player && mo->player->playerstate
	       == PST_DEAD)) // Players must be alive to teleport
	    {
	      buttonSuccess = true;
	      if(deathmatch)
		{ // Winning in deathmatch just goes back to map 1
		  G_Completed(1, 0);
		}
	      else
		{ // Passing -1, -1 to G_Completed() starts the Finale
		  G_Completed(-1, -1);
		}
	    }
	}
      break;
    case 80: // ACS_Execute
      buttonSuccess =
	StartACS(args[0], args[1], &args[2], mo, line, side);
      break;
    case 81: // ACS_Suspend
      buttonSuccess = SuspendACS(args[0], args[1]);
      break;
    case 82: // ACS_Terminate
      buttonSuccess = TerminateACS(args[0], args[1]);
      break;
    case 83: // ACS_LockedExecute
      if (mo->Type() == Thinker::tt_ppawn)
        {
	PlayerPawn *p = (PlayerPawn *p)mo;
	if (p->player)
	  {
	    int lock = args[4];
	    if (lock)
	      {
		if (!(p->cards & (1 << (lock-1))))
		  {
		    extern char *TextKeyMessages[11];
		    char LockedBuffer[80];
		    sprintf(LockedBuffer, "YOU NEED THE %s\n", TextKeyMessages[lock-1]);
		    p->SetMessage(LockedBuffer, true);
		    S_StartSound(p, SFX_DOOR_LOCKED);
		    return false;
		  }
	      }
	    args[4] = 0;
	    // StartACS(newArgs[0], newArgs[1], &newArgs[2], p, line, side);
	    // FIXME check the args[1] map number to see if it is this map
	    // if not, do not start script but store it
	    StartACS(args[0], &args[2], p, line, side);
	    args[4] = lock;
	  }
	}
      break;
    case 90: // Poly Rotate Left Override
      buttonSuccess = EV_RotatePoly(line, args, 1, true);
      break;
    case 91: // Poly Rotate Right Override
      buttonSuccess = EV_RotatePoly(line, args, -1, true);
      break;
    case 92: // Poly Move Override
      buttonSuccess = EV_MovePoly(line, args, false, true);
      break;
    case 93: // Poly Move Times 8 Override
      buttonSuccess = EV_MovePoly(line, args, true, true);
      break;
    case 94: // Build Pillar Crush 
      buttonSuccess = EV_BuildPillar(line, args, true);
      break;
    case 95: // Lower Floor and Ceiling
      buttonSuccess = EV_DoFloorAndCeiling(line, args, false);
      break;
    case 96: // Raise Floor and Ceiling
      buttonSuccess = EV_DoFloorAndCeiling(line, args, true);
      break;
    case 109: // Force Lightning
      buttonSuccess = true;
      P_ForceLightning();
      break;
    case 110: // Light Raise by Value
      buttonSuccess = EV_SpawnLight(line, args, LITE_RAISEBYVALUE);
      break; 
    case 111: // Light Lower by Value
      buttonSuccess = EV_SpawnLight(line, args, LITE_LOWERBYVALUE);
      break; 
    case 112: // Light Change to Value
      buttonSuccess = EV_SpawnLight(line, args, LITE_CHANGETOVALUE);
      break; 
    case 113: // Light Fade
      buttonSuccess = EV_SpawnLight(line, args, LITE_FADE);
      break; 
    case 114: // Light Glow
      buttonSuccess = EV_SpawnLight(line, args, LITE_GLOW);
      break; 
    case 115: // Light Flicker
      buttonSuccess = EV_SpawnLight(line, args, LITE_FLICKER);
      break; 
    case 116: // Light Strobe
      buttonSuccess = EV_SpawnLight(line, args, LITE_STROBE);
      break; 
    case 120: // Quake Tremor
      buttonSuccess = A_LocalQuake(args, mo);
      break;
    case 129: // UsePuzzleItem
      buttonSuccess = EV_LineSearchForPuzzleItem(line, args, mo);
      break;
    case 130: // Thing_Activate
      buttonSuccess = EV_ThingActivate(args[0]);
      break;
    case 131: // Thing_Deactivate
      buttonSuccess = EV_ThingDeactivate(args[0]);
      break;
    case 132: // Thing_Remove
      buttonSuccess = EV_ThingRemove(args[0]);
      break;
    case 133: // Thing_Destroy
      buttonSuccess = EV_ThingDestroy(args[0]);
      break;
    case 134: // Thing_Projectile
      buttonSuccess = EV_ThingProjectile(args, 0);
      break;
    case 135: // Thing_Spawn
      buttonSuccess = EV_ThingSpawn(args, 1);
      break;
    case 136: // Thing_ProjectileGravity
      buttonSuccess = EV_ThingProjectile(args, 1);
      break;
    case 137: // Thing_SpawnNoFog
      buttonSuccess = EV_ThingSpawn(args, 0);
      break;
    case 138: // Floor_Waggle
      buttonSuccess = EV_StartFloorWaggle(args[0], args[1],
					  args[2], args[3], args[4]);
      break;
    case 140: // Sector_SoundChange
      buttonSuccess = EV_SectorSoundChange(args);
      break;

      // Line specials only processed during level initialization
      // 100: Scroll_Texture_Left
      // 101: Scroll_Texture_Right
      // 102: Scroll_Texture_Up
      // 103: Scroll_Texture_Down
      // 121: Line_SetIdentification

      // Inert Line specials
    default:
      break;
    }
  */
  return buttonSuccess;
}


//
// was P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
// Unused.
void Map::CrossSpecialLine(int linenum, int side, Actor *thing)
{
  line_t *line = &lines[linenum];

  ActivateCrossedLine(line, side, thing);
}

// was P_ActivateCrossedLine
void Map::ActivateCrossedLine(line_t *line, int side, Actor *thing)
{
  int  ok;

  //SoM: 4/26/2000: ALLTRIGGER should allow monsters to use generalized types too!
  bool forceuse = (line->flags & ML_ALLTRIGGER) && !(thing->flags & MF_NOSPLASH);

  // is thing a PlayerPawn?
  bool p = (thing->Type() == Thinker::tt_ppawn);

  //  Triggers that other things can activate
  if (!p && game.mode != gm_heretic)
    {
      // Things that should NOT trigger specials...
      if (thing->flags & MF_NOTRIGGER)
	return;
      /*
      switch(thing->type)
        {
	case MT_ROCKET:
	case MT_PLASMA:
	case MT_BFG:
	case MT_TROOPSHOT:
	case MT_HEADSHOT:
	case MT_BRUISERSHOT:
	  return;
	  break;

	default: break;
        }
      */
    }

  //int res;

  //SoM: 3/7/2000: Check for generalized line types/
  if (boomsupport)
    {
      // pointer to line function is NULL by default, set non-null if
      // line special is walkover generalized linedef type
      int (Map::*linefunc)(line_t *line) = NULL;
  
      // check each range of generalized linedefs
      if ((unsigned)line->special >= GenFloorBase)
	{
	  if (!p)
	    if (((line->special & FloorChange) || !(line->special & FloorModel)) && !forceuse)
	      return;     // FloorModel is "Allow Monsters" if FloorChange is 0
	  if (!line->tag)
	    return;
	  linefunc = &Map::EV_DoGenFloor;
	}
      else if ((unsigned)line->special >= GenCeilingBase)
	{
	  if (!p)
	    if (((line->special & CeilingChange) || !(line->special & CeilingModel)) && !forceuse)
	      return;     // CeilingModel is "Allow Monsters" if CeilingChange is 0
	  if (!line->tag)
	    return;
	  linefunc = &Map::EV_DoGenCeiling;
	}
      else if ((unsigned)line->special >= GenDoorBase)
	{
	  if (!p)
	    {
	      if (!(line->special & DoorMonster) && !forceuse)
		return;                    // monsters disallowed from this door
	      if (line->flags & ML_SECRET) // they can't open secret doors either
		return;
	    }
	  if (!line->tag)
	    return;
	  linefunc = &Map::EV_DoGenDoor;
	}
      else if ((unsigned)line->special >= GenLockedBase)
	{
	  if (!p)
	    return;                     // monsters disallowed from unlocking doors
	  if (((line->special&TriggerType)==WalkOnce) || ((line->special&TriggerType)==WalkMany))
	    {
	      if (! ((PlayerPawn *)thing)->CanUnlockGenDoor(line))
		return;
	    }
	  else
	    return;
	  linefunc = &Map::EV_DoGenLockedDoor;
	}
      else if ((unsigned)line->special >= GenLiftBase)
	{
	  if (!p)
	    if (!(line->special & LiftMonster) && !forceuse)
	      return; // monsters disallowed
	  if (!line->tag)
	    return;
	  linefunc = &Map::EV_DoGenLift;
	}
      else if ((unsigned)line->special >= GenStairsBase)
	{
	  if (!p)
	    if (!(line->special & StairMonster) && !forceuse)
	      return; // monsters disallowed
	  if (!line->tag)
	    return;
	  linefunc = &Map::EV_DoGenStairs;
	}
      else if ((unsigned)line->special >= GenCrusherBase)
	{
	  if (!p)
	    if (!(line->special & StairMonster) && !forceuse)
	      return; // monsters disallowed
	  if (!line->tag)
	    return;
	  linefunc = &Map::EV_DoGenCrusher;
	}
  
      if (linefunc) // if it was a valid generalized type
        switch((line->special & TriggerType) >> TriggerTypeShift)
	  {
          case WalkOnce:
            if ((this->*linefunc)(line))
              line->special = 0;    // clear special if a walk once type
            return;
          case WalkMany:
            (this->*linefunc)(line);
            return;
          default:                  // if not a walk type, do nothing here
            return;
	  }
    }


  if (!p)
    {
      ok = 0;
      if ( game.mode == gm_heretic && (line->special == 4 || line->special==39 || line->special == 97) )
	ok = 1;
      else
        switch(line->special)
	  {
          case 39:      // TELEPORT TRIGGER
          case 97:      // TELEPORT RETRIGGER
          case 125:     // TELEPORT MONSTERONLY TRIGGER
          case 126:     // TELEPORT MONSTERONLY RETRIGGER
          case 4:       // RAISE DOOR
          case 10:      // PLAT DOWN-WAIT-UP-STAY TRIGGER
          case 88:      // PLAT DOWN-WAIT-UP-STAY RETRIGGER
            ok = 1;
            break; 
	    // SoM: 3/4/2000: Add boom compatibility for extra monster usable
	    // linedef types.
          case 208:     //SoM: Silent thing teleporters
          case 207:
          case 243:     //Silent line to line teleporter
          case 244:     //Same as above but trigger once.
          case 262:     //Same as 243 but reversed
          case 263:     //Same as 244 but reversed
          case 264:     //Monster only, silent, trigger once, reversed
          case 265:     //Same as 264 but repeatable
          case 266:     //Monster only, silent, trigger once
          case 267:     //Same as 266 bot repeatable
          case 268:     //Monster only, silent, trigger once, set pos to thing
          case 269:     //Monster only, silent, repeatable, set pos to thing
            if(boomsupport)
              ok = 1;
            break;
	  }
      //SoM: Anything can trigger this line!
      if(line->flags & ML_ALLTRIGGER)
	ok = 1;

      if (!ok)
	return;
    }

  if (!P_CheckTag(line) && boomsupport)
    return;

  // Note: could use some const's here.
  switch (line->special)
    {
      // TRIGGERS.
      // All from here to RETRIGGERS.
    case 2:
      // Open Door
      if(EV_DoDoor(line,dooropen,VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 3:
      // Close Door
      if(EV_DoDoor(line,doorclose,VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 4:
      // Raise Door
      if(EV_DoDoor(line,normalDoor,VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 5:
      // Raise Floor
      if(EV_DoFloor(line,raiseFloor) || !boomsupport)
	line->special = 0;
      break;

    case 6:
      // Fast Ceiling Crush & Raise
      if(EV_DoCeiling(line,fastCrushAndRaise) || !boomsupport)
	line->special = 0;
      break;

    case 8:
      // Build Stairs
      if(EV_BuildStairs(line, stair_e(game.mode == gm_heretic ? 8*FRACUNIT : build8)) || !boomsupport)
	line->special = 0;
      break;

    case 10:
      // PlatDownWaitUp
      if(EV_DoPlat(line,downWaitUpStay,0) || !boomsupport)
	line->special = 0;
      break;

    case 12:
      // Light Turn On - brightest near
      if(EV_LightTurnOn(line,0) || !boomsupport)
	line->special = 0;
      break;

    case 13:
      // Light Turn On 255
      if(EV_LightTurnOn(line,255) || !boomsupport)
	line->special = 0;
      break;

    case 16:
      // Close Door 30
      if(EV_DoDoor(line,close30ThenOpen,VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 17:
      // Start Light Strobing
      if(EV_StartLightStrobing(line) || !boomsupport)
	line->special = 0;
      break;

    case 19:
      // Lower Floor
      if(EV_DoFloor(line,lowerFloor) || !boomsupport)
	line->special = 0;
      break;

    case 22:
      // Raise floor to nearest height and change texture
      if(EV_DoPlat(line,raiseToNearestAndChange,0) || !boomsupport)
	line->special = 0;
      break;

    case 25:
      // Ceiling Crush and Raise
      if(EV_DoCeiling(line,crushAndRaise) || !boomsupport)
	line->special = 0;
      break;

    case 30:
      // Raise floor to shortest texture height
      //  on either side of lines.
      if(EV_DoFloor(line,raiseToTexture) || !boomsupport)
	line->special = 0;
      break;

    case 35:
      // Lights Very Dark
      if(EV_LightTurnOn(line,35) || !boomsupport)
	line->special = 0;
      break;

    case 36:
      // Lower Floor (TURBO)
      if(EV_DoFloor(line,turboLower) || !boomsupport)
	line->special = 0;
      break;

    case 37:
      // LowerAndChange
      if(EV_DoFloor(line,lowerAndChange) || !boomsupport)
	line->special = 0;
      break;

    case 38:
      // Lower Floor To Lowest
      if(EV_DoFloor( line, lowerFloorToLowest ) || !boomsupport)
	line->special = 0;
      break;

    case 39:
      // TELEPORT!
      if(EV_Teleport( line, side, thing ) || !boomsupport)
	line->special = 0;
      break;

    case 40:
      // RaiseCeilingLowerFloor
      if(EV_DoCeiling( line, raiseToHighest ) || EV_DoFloor( line, lowerFloorToLowest ) ||
	 !boomsupport)
	line->special = 0;
      break;

    case 44:
      // Ceiling Crush
      if(EV_DoCeiling( line, lowerAndCrush ) || !boomsupport)
	line->special = 0;
      break;

    case 52:
      // EXIT!
      if( cv_allowexitlevel.value )
        {
	  ExitMap(0);
	  line->special = 0;  // heretic have right
        }
      break;

    case 53:
      // Perpetual Platform Raise
      if(EV_DoPlat(line,perpetualRaise,0) || !boomsupport)
	line->special = 0;
      break;

    case 54:
      // Platform Stop
      if(EV_StopPlat(line) || !boomsupport)
	line->special = 0;
      break;

    case 56:
      // Raise Floor Crush
      if(EV_DoFloor(line,raiseFloorCrush) || !boomsupport)
	line->special = 0;
      break;

    case 57:
      // Ceiling Crush Stop
      if(EV_CeilingCrushStop(line) || !boomsupport)
	line->special = 0;
      break;

    case 58:
      // Raise Floor 24
      if(EV_DoFloor(line,raiseFloor24) || !boomsupport)
	line->special = 0;
      break;

    case 59:
      // Raise Floor 24 And Change
      if(EV_DoFloor(line,raiseFloor24AndChange) || !boomsupport)
	line->special = 0;
      break;

    case 104:
      // Turn lights off in sector(tag)
      if(EV_TurnTagLightsOff(line) || !boomsupport)
	line->special = 0;
      break;

    case 108:
      // Blazing Door Raise (faster than TURBO!)
      if(EV_DoDoor (line,blazeRaise,4*VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 109:
      // Blazing Door Open (faster than TURBO!)
      if(EV_DoDoor (line,blazeOpen,4*VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 100:
      if( game.mode == gm_heretic )
	EV_DoDoor (line, normalDoor, VDOORSPEED * 3);
      else
        {
          // Build Stairs Turbo 16
          if(EV_BuildStairs(line,turbo16) || !boomsupport)
            line->special = 0;
        }
      break;

    case 110:
      // Blazing Door Close (faster than TURBO!)
      if(EV_DoDoor (line,blazeClose,4*VDOORSPEED) || !boomsupport)
	line->special = 0;
      break;

    case 119:
      // Raise floor to nearest surr. floor
      if(EV_DoFloor(line,raiseFloorToNearest) || !boomsupport)
	line->special = 0;
      break;

    case 121:
      // Blazing PlatDownWaitUpStay
      if(EV_DoPlat(line,blazeDWUS,0) || !boomsupport)
	line->special = 0;
      break;

    case 124:
      // Secret EXIT
      if( cv_allowexitlevel.value )
	ExitMap(100);
      break;

    case 125:
      // TELEPORT MonsterONLY
      if (!p)
        {
	  if(EV_Teleport( line, side, thing ) || !boomsupport)
	    line->special = 0;
        }
      break;

    case 130:
      // Raise Floor Turbo
      if(EV_DoFloor(line,raiseFloorTurbo) || !boomsupport)
	line->special = 0;
      break;

    case 141:
      // Silent Ceiling Crush & Raise
      if(EV_DoCeiling(line,silentCrushAndRaise) || !boomsupport)
	line->special = 0;
      break;

      //SoM: FraggleScript
    case 273: //(1sided)
      if(side) break;

    case 272: //(2sided)
#ifdef FRAGGLESCRIPT
      t_trigger = thing;
      T_RunScript(line->tag);
#endif
      break;

      // once-only triggers
    case 275: //(1sided)
      if(side) break;

    case 274: //(2sided)
#ifdef FRAGGLESCRIPT
      t_trigger = thing;
      T_RunScript(line->tag);
#endif
      line->special = 0;        // clear trigger
      break;


      // RETRIGGERS.  All from here till end.
    case 72:
      // Ceiling Crush
      EV_DoCeiling( line, lowerAndCrush );
      break;

    case 73:
      // Ceiling Crush and Raise
      EV_DoCeiling(line,crushAndRaise);
      break;

    case 74:
      // Ceiling Crush Stop
      EV_CeilingCrushStop(line);
      break;

    case 75:
      // Close Door
      EV_DoDoor(line,doorclose,VDOORSPEED);
      break;

    case 76:
      // Close Door 30
      EV_DoDoor(line,close30ThenOpen,VDOORSPEED);
      break;

    case 77:
      // Fast Ceiling Crush & Raise
      EV_DoCeiling(line,fastCrushAndRaise);
      break;

    case 79:
      // Lights Very Dark
      EV_LightTurnOn(line,35);
      break;

    case 80:
      // Light Turn On - brightest near
      EV_LightTurnOn(line,0);
      break;

    case 81:
      // Light Turn On 255
      EV_LightTurnOn(line,255);
      break;

    case 82:
      // Lower Floor To Lowest
      EV_DoFloor( line, lowerFloorToLowest );
      break;

    case 83:
      // Lower Floor
      EV_DoFloor(line,lowerFloor);
      break;

    case 84:
      // LowerAndChange
      EV_DoFloor(line,lowerAndChange);
      break;

    case 86:
      // Open Door
      EV_DoDoor(line,dooropen,VDOORSPEED);
      break;

    case 87:
      // Perpetual Platform Raise
      EV_DoPlat(line,perpetualRaise,0);
      break;

    case 88:
      // PlatDownWaitUp
      EV_DoPlat(line,downWaitUpStay,0);
      break;

    case 89:
      // Platform Stop
      EV_StopPlat(line);
      break;

    case 90:
      // Raise Door
      EV_DoDoor(line,normalDoor,VDOORSPEED);
      break;

    case 91:
      // Raise Floor
      EV_DoFloor(line,raiseFloor);
      break;

    case 92:
      // Raise Floor 24
      EV_DoFloor(line,raiseFloor24);
      break;

    case 93:
      // Raise Floor 24 And Change
      EV_DoFloor(line,raiseFloor24AndChange);
      break;

    case 94:
      // Raise Floor Crush
      EV_DoFloor(line,raiseFloorCrush);
      break;

    case 95:
      // Raise floor to nearest height
      // and change texture.
      EV_DoPlat(line,raiseToNearestAndChange,0);
      break;

    case 96:
      // Raise floor to shortest texture height
      // on either side of lines.
      EV_DoFloor(line,raiseToTexture);
      break;

    case 97:
      // TELEPORT!
      EV_Teleport( line, side, thing );
      break;

    case 98:
      // Lower Floor (TURBO)
      EV_DoFloor(line,turboLower);
      break;

    case 105:
      if( game.mode == gm_heretic )
        {
	  if( cv_allowexitlevel.value )
            {
	      ExitMap(100);
	      line->special = 0;
            }
        }
      else
	// Blazing Door Raise (faster than TURBO!)
	EV_DoDoor (line,blazeRaise,4*VDOORSPEED);
      break;

    case 106:
      if( game.mode == gm_heretic )
        {
          if(EV_BuildStairs (line, stair_e(16 * FRACUNIT)) || !boomsupport)
	    line->special = 0;
        }
      else
	// Blazing Door Open (faster than TURBO!)
	EV_DoDoor (line,blazeOpen,4*VDOORSPEED);
      break;

    case 107:
      if( game.mode != gm_heretic ) // used for a switch !
	// Blazing Door Close (faster than TURBO!)
	EV_DoDoor (line,blazeClose,4*VDOORSPEED);
      break;

    case 120:
      // Blazing PlatDownWaitUpStay.
      EV_DoPlat(line,blazeDWUS,0);
      break;

    case 126:
      // TELEPORT MonsterONLY.
      if (!p)
	EV_Teleport( line, side, thing );
      break;

    case 128:
      // Raise To Nearest Floor
      EV_DoFloor(line,raiseFloorToNearest);
      break;

    case 129:
      // Raise Floor Turbo
      EV_DoFloor(line,raiseFloorTurbo);
      break;

      // SoM:3/4/2000: Extended Boom W* triggers.
    default:
      if(boomsupport) {
	switch(line->special) {
	  //SoM: 3/4/2000:Boom Walk once triggers.
	  //SoM: 3/4/2000:Yes this is "copied" code! I just cleaned it up. Did you think I was going to retype all this?!
	case 142:
	  // Raise Floor 512
	  if (EV_DoFloor(line,raiseFloor512))
	    line->special = 0;
	  break;
  
	case 143:
	  // Raise Floor 24 and change
	  if (EV_DoPlat(line,raiseAndChange,24))
	    line->special = 0;
	  break;

	case 144:
	  // Raise Floor 32 and change
	  if (EV_DoPlat(line,raiseAndChange,32))
	    line->special = 0;
	  break;

	case 145:
	  // Lower Ceiling to Floor
	  if (EV_DoCeiling( line, lowerToFloor ))
	    line->special = 0;
	  break;

	case 146:
	  // Lower Pillar, Raise Donut
	  if (EV_DoDonut(line))
	    line->special = 0;
	  break;

	case 199:
	  // Lower ceiling to lowest surrounding ceiling
	  if (EV_DoCeiling(line,lowerToLowest))
	    line->special = 0;
	  break;

	case 200:
	  // Lower ceiling to highest surrounding floor
	  if (EV_DoCeiling(line,lowerToMaxFloor))
	    line->special = 0;
	  break;

	case 207:
	  // W1 silent teleporter (normal kind)
	  if (EV_SilentTeleport(line, side, thing))
	    line->special = 0;
	  break;

	case 153: 
	  // Texture/Type Change Only (Trig)
	  if (EV_DoChange(line,trigChangeOnly))
	    line->special = 0;
	  break;
  
	case 239: 
	  // Texture/Type Change Only (Numeric)
	  if (EV_DoChange(line,numChangeOnly))
	    line->special = 0;
	  break;
 
	case 219:
	  // Lower floor to next lower neighbor
	  if (EV_DoFloor(line,lowerFloorToNearest))
	    line->special = 0;
	  break;

	case 227:
	  // Raise elevator next floor
	  if (EV_DoElevator(line,elevateUp))
	    line->special = 0;
	  break;

	case 231:
	  // Lower elevator next floor
	  if (EV_DoElevator(line,elevateDown))
	    line->special = 0;
	  break;

	case 235:
	  // Elevator to current floor
	  if (EV_DoElevator(line,elevateCurrent))
	    line->special = 0;
	  break;

	case 243: 
	  // W1 silent teleporter (linedef-linedef kind)
	  if (EV_SilentLineTeleport(line, side, thing, false))
	    line->special = 0;
	  break;

	case 262: 
	  if (EV_SilentLineTeleport(line, side, thing, true))
	    line->special = 0;
	  break;
 
	case 264: 
	  if (!p &&
              EV_SilentLineTeleport(line, side, thing, true))
	    line->special = 0;
	  break;

	case 266: 
	  if (!p &&
	      EV_SilentLineTeleport(line, side, thing, false))
	    line->special = 0;
	  break;

	case 268: 
	  if (!p && EV_SilentTeleport(line, side, thing))
	    line->special = 0;
	  break;

	  // Extended walk many retriggerable
 
	  //Boom added lots of linedefs to fill in the gaps in trigger types

	case 147:
	  // Raise Floor 512
	  EV_DoFloor(line,raiseFloor512);
	  break;

	case 148:
	  // Raise Floor 24 and Change
	  EV_DoPlat(line,raiseAndChange,24);
	  break;

	case 149:
	  // Raise Floor 32 and Change
	  EV_DoPlat(line,raiseAndChange,32);
	  break;

	case 150:
	  // Start slow silent crusher
	  EV_DoCeiling(line,silentCrushAndRaise);
	  break;

	case 151:
	  // RaiseCeilingLowerFloor
	  EV_DoCeiling( line, raiseToHighest );
	  EV_DoFloor( line, lowerFloorToLowest );
	  break;

	case 152:
	  // Lower Ceiling to Floor
	  EV_DoCeiling( line, lowerToFloor );
	  break;

	case 256:
	  // Build stairs, step 8
	  EV_BuildStairs(line,build8);
	  break;

	case 257:
	  // Build stairs, step 16
	  EV_BuildStairs(line,turbo16);
	  break;

	case 155:
	  // Lower Pillar, Raise Donut
	  EV_DoDonut(line);
	  break;

	case 156:
	  // Start lights strobing
	  EV_StartLightStrobing(line);
	  break;

	case 157:
	  // Lights to dimmest near
	  EV_TurnTagLightsOff(line);
	  break;

	case 201:
	  // Lower ceiling to lowest surrounding ceiling
	  EV_DoCeiling(line,lowerToLowest);
	  break;

	case 202:
	  // Lower ceiling to highest surrounding floor
	  EV_DoCeiling(line,lowerToMaxFloor);
	  break;

	case 208:
	  // WR silent teleporter (normal kind)
	  EV_SilentTeleport(line, side, thing);
	  break;

	case 212:
	  // Toggle floor between C and F instantly
	  EV_DoPlat(line,toggleUpDn,0);
	  break;

	case 154:
	  // Texture/Type Change Only (Trigger)
	  EV_DoChange(line,trigChangeOnly);
	  break;

	case 240: 
	  // Texture/Type Change Only (Numeric)
	  EV_DoChange(line,numChangeOnly);
	  break;

	case 220:
	  // Lower floor to next lower neighbor
	  EV_DoFloor(line,lowerFloorToNearest);
	  break;

	case 228:
	  // Raise elevator next floor
	  EV_DoElevator(line,elevateUp);
	  break;

	case 232:
	  // Lower elevator next floor
	  EV_DoElevator(line,elevateDown);
	  break;

	case 236:
	  // Elevator to current floor
	  EV_DoElevator(line,elevateCurrent);
	  break;

	case 244: 
	  // WR silent teleporter (linedef-linedef kind)
	  EV_SilentLineTeleport(line, side, thing, false);
	  break;

	case 263: 
	  //Silent line-line reversed
	  EV_SilentLineTeleport(line, side, thing, true);
	  break;

	case 265: 
	  //Monster-only silent line-line reversed
	  if (!p)
	    EV_SilentLineTeleport(line, side, thing, true);
	  break;

	case 267: 
	  //Monster-only silent line-line
	  if (!p)
	    EV_SilentLineTeleport(line, side, thing, false);
	  break;

	case 269: 
	  //Monster-only silent
	  if (!p)
	    EV_SilentTeleport(line, side, thing);
	  break;
	}
      }
    }
}



//
// was P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void Map::ShootSpecialLine(Actor *thing, line_t *line)
{
  int ok;

  // is thing a PlayerPawn?
  bool p = (thing->Type() == Thinker::tt_ppawn);

  //SoM: 3/7/2000: Another General type check
  if (boomsupport)
    {
      // pointer to line function is NULL by default, set non-null if
      // line special is gun triggered generalized linedef type
      int (Map::*linefunc)(line_t *line) = NULL;

      // check each range of generalized linedefs
      if ((unsigned)line->special >= GenFloorBase)
	{
	  if (!p)
	    if ((line->special & FloorChange) || !(line->special & FloorModel))
	      return;   // FloorModel is "Allow Monsters" if FloorChange is 0
	  if (!line->tag) //jff 2/27/98 all gun generalized types require tag
	    return;

	  linefunc = &Map::EV_DoGenFloor;
	}
      else if ((unsigned)line->special >= GenCeilingBase)
	{
	  if (!p)
	    if ((line->special & CeilingChange) || !(line->special & CeilingModel))
	      return;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
	  if (!line->tag) //jff 2/27/98 all gun generalized types require tag
	    return;
	  linefunc = &Map::EV_DoGenCeiling;
	}
      else if ((unsigned)line->special >= GenDoorBase)
	{
	  if (!p)
	    {
	      if (!(line->special & DoorMonster))
		return;   // monsters disallowed from this door
	      if (line->flags & ML_SECRET) // they can't open secret doors either
		return;
	    }
	  if (!line->tag) //jff 3/2/98 all gun generalized types require tag
	    return;
	  linefunc = &Map::EV_DoGenDoor;
	}
      else if ((unsigned)line->special >= GenLockedBase)
	{
	  if (!p)
	    return;   // monsters disallowed from unlocking doors
	  if (((line->special&TriggerType)==GunOnce) || ((line->special&TriggerType)==GunMany))
	    { //jff 4/1/98 check for being a gun type before reporting door type
	      if (!((PlayerPawn *)thing)->CanUnlockGenDoor(line))
		return;
	    }
	  else
	    return;
	  if (!line->tag) //jff 2/27/98 all gun generalized types require tag
	    return;

	  linefunc = &Map::EV_DoGenLockedDoor;
	}
      else if ((unsigned)line->special >= GenLiftBase)
	{
	  if (!p)
	    if (!(line->special & LiftMonster))
	      return; // monsters disallowed
	  linefunc = &Map::EV_DoGenLift;
	}
      else if ((unsigned)line->special >= GenStairsBase)
	{
	  if (!p)
	    if (!(line->special & StairMonster))
	      return; // monsters disallowed
	  if (!line->tag) //jff 2/27/98 all gun generalized types require tag
	    return;
	  linefunc = &Map::EV_DoGenStairs;
	}
      else if ((unsigned)line->special >= GenCrusherBase)
	{
	  if (!p)
	    if (!(line->special & StairMonster))
	      return; // monsters disallowed
	  if (!line->tag) //jff 2/27/98 all gun generalized types require tag
	    return;
	  linefunc = &Map::EV_DoGenCrusher;
	}

      if (linefunc)
        switch((line->special & TriggerType) >> TriggerTypeShift)
	  {
          case GunOnce:
            if ((this->*linefunc)(line))
              ChangeSwitchTexture(line,0);
            return;
          case GunMany:
            if ((this->*linefunc)(line))
              ChangeSwitchTexture(line,1);
            return;
          default:  // if not a gun type, do nothing here
            return;
	  }
    }


  //  Impacts that other things can activate.
  if (!p)
    {
      ok = 0;
      switch(line->special)
        {
	case 46:
	  // OPEN DOOR IMPACT
	  ok = 1;
	  break;
        }
      if (!ok)
	return;
    }

  if(!P_CheckTag(line))
    return;

  switch(line->special)
    {
    case 24:
      // RAISE FLOOR
      if(EV_DoFloor(line,raiseFloor) || !boomsupport)
	ChangeSwitchTexture(line,0);
      break;

    case 46:
      // OPEN DOOR
      if(EV_DoDoor(line,dooropen,VDOORSPEED) || !boomsupport)
	ChangeSwitchTexture(line,1);
      break;

    case 47:
      // RAISE FLOOR NEAR AND CHANGE
      if(EV_DoPlat(line,raiseToNearestAndChange,0) || !boomsupport)
	ChangeSwitchTexture(line,0);
      break;

      //SoM: FraggleScript
    case 278:
    case 279:
#ifdef FRAGGLESCRIPT
      t_trigger = thing;
      T_RunScript(line->tag);
#endif
      if(line->special == 279) line->special = 0;       // clear if G1
      break;

    default:
      if (boomsupport)
	switch (line->special)
          {
	  case 197:
	    // Exit to next level
	    if( cv_allowexitlevel.value )
              {
		ChangeSwitchTexture(line,0);
		ExitMap(0);
              }
	    break;

	  case 198:
	    // Exit to secret level
	    if( cv_allowexitlevel.value )
              {
		ChangeSwitchTexture(line,0);
		ExitMap(100);
              }
	    break;
	    //jff end addition of new gun linedefs
          }
      break;
    }
}



//
// was P_UpdateSpecials
// Animate planes, scroll walls, etc.
//

void Map::UpdateSpecials()
{
  anim_t*     anim;
  int         i;
  int         pic; //SoM: 3/8/2000

  levelflat_t *foundflats;        // for flat animation

  //  LEVEL TIMER
  if (cv_timelimit.value && (ULONG)cv_timelimit.value < maptic)
    ExitMap(0);

  //  ANIMATE TEXTURES
  for (anim = anims ; anim < lastanim ; anim++)
    {
      for (i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
	{
	  pic = anim->basepic + ( (maptic/anim->speed + i)%anim->numpics );
	  if (anim->istexture)
	    texturetranslation[i] = pic;
	}
    }


  //  ANIMATE FLATS
  //Fab:FIXME: do not check the non-animate flat.. link the animated ones?
  // note: its faster than the original anywaysince it animates only
  //    flats used in the level, and there's usually very few of them
  foundflats = levelflats;
  for (i = 0; i<numlevelflats; i++,foundflats++)
    {
      if (foundflats->speed) // it is an animated flat
	{
	  // update the levelflat lump number
	  foundflats->lumpnum = foundflats->baselumpnum +
	    ( (maptic/foundflats->speed + foundflats->animseq) % foundflats->numpics);
	}
    }


  // BUTTONS are now done in their respective Thinkers
}


//SoM: 3/8/2000: EV_DoDonut moved to p_floor.c

//SoM: 3/23/2000: Adds a sectors floor and ceiling to a sector's ffloor list
//void P_AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags);
void P_AddFFloor(sector_t* sec, ffloor_t* ffloor);

// was P_AddFakeFloor
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

//
// SPECIAL SPAWNING
//

//
// was P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//

// Parses command line parameters.
void Map::SpawnSpecials()
{
  int i;

  /*
  int episode = 1;
  if (W_CheckNumForName("texture2") >= 0)
    episode = 2;
  */

  //  Init special SECTORs.
  sector_t *sector = sectors;
  for (i=0 ; i<numsectors ; i++, sector++)
    {
      if (!sector->special)
	continue;

      // Boom secret flag
      if (sector->special & SECRET_MASK)
	secrets++;

      // Doom basic sector types (lowest 5 bits)
      switch (sector->special & 31)
        {
	case 1:
	  // FLICKERING LIGHTS
	  SpawnLightFlash (sector);
	  break;

	case 2:
	  // STROBE FAST
	  SpawnStrobeFlash(sector,FASTDARK,0);
	  break;

	case 3:
	  // STROBE SLOW
	  SpawnStrobeFlash(sector,SLOWDARK,0);
	  break;

	case 4:
	  if (game.raven)
	    break;
	  // STROBE FAST/DEATH SLIME
	  SpawnStrobeFlash(sector,FASTDARK,0);
	  sector->special |= 3<<DAMAGE_SHIFT; //SoM: 3/8/2000: put damage bits in
	  break;

	case 8:
	  // GLOWING LIGHT
	  SpawnGlowingLight(sector);
	  break;

	case 9:
	  // SECRET SECTOR
	  if (sector->special<32)
	    secrets++;
	  break;

	case 10:
	  // DOOR CLOSE IN 30 SECONDS
	  SpawnDoorCloseIn30 (sector);
	  break;

	case 12:
	  // SYNC STROBE SLOW
	  SpawnStrobeFlash (sector, SLOWDARK, 1);
	  break;

	case 13:
	  // SYNC STROBE FAST
	  SpawnStrobeFlash (sector, FASTDARK, 1);
	  break;

	case 14:
	  // DOOR RAISE IN 5 MINUTES
	  SpawnDoorRaiseIn5Mins (sector, i);
	  break;

	case 17:
	  SpawnFireFlicker(sector);
	  break;
        }
    }

  //SoM: 3/8/2000: Boom level init functions
  RemoveAllActiveCeilings();
  RemoveAllActivePlats();
  //for (i = 0;i < MAXBUTTONS;i++) memset(&buttonlist[i],0,sizeof(button_t));

  InitTagLists();   //Create xref tables for tags
  SpawnScrollers(); //Add generalized scrollers
  SpawnFriction();  //New friction model using linedefs
  SpawnPushers();   //New pusher model using linedefs

  //  Init line EFFECTs
  for (i = 0;i < numlines; i++)
    {
      switch(lines[i].special)
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
	  EV_DoFloor(&lines[i], instantLower);
	  break;
	  
	  // Instant raise for ceilings SSNTails 06-13-2002
	case 291:
	  EV_DoCeiling(&lines[i], instantRaise);
	  break;

	default:
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





/*
  SoM: 3/8/2000: General scrolling functions.
*/

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
// Process the active scrollers.
// was T_Scroll

void scroll_t::Think()
{
  fixed_t tdx = dx, tdy = dy;

  if (control != -1)
    {   // compute scroll amounts based on a sector's height changes
      fixed_t height = mp->sectors[control].floorheight +
        mp->sectors[control].ceilingheight;
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

    case sc_carry:

      sec = mp->sectors + affectee;
      height = sec->floorheight;
      waterheight = sec->heightsec != -1 &&
        mp->sectors[sec->heightsec].floorheight > height ?
        mp->sectors[sec->heightsec].floorheight : MININT;

      for (node = sec->touching_thinglist; node; node = node->m_snext)
        if (!((thing = node->m_thing)->flags & MF_NOCLIPLINE) &&
            (!(thing->flags & MF_NOGRAVITY || thing->z > height) ||
             thing->z < waterheight))
          {
            // Move objects only if on floor or underwater,
            // non-floating, and clipped.
            thing->px += tdx;
            thing->py += tdy;
          }
      break;

    case sc_carry_ceiling:       // to be added later
      break;
    }
}

//
// was Add_Scroller()
//
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
//

scroll_t::scroll_t(scroll_e t, fixed_t ndx, fixed_t ndy,
		   int ctrl, int aff, int acc, const sector_t *csec)
{
  type = t;
  dx = ndx;
  dy = ndy;
  accel = acc;
  vdx = vdy = 0;
  control = ctrl;
  affectee = aff;
  // FIXME we could just use csec instead of control...
  if ((control) != -1) last_height = csec->floorheight + csec->ceilingheight;
}

// Adds wall scroller. Scroll amount is rotated with respect to wall's
// linedef first, so that scrolling towards the wall in a perpendicular
// direction is translated into vertical motion, while scrolling along
// the wall in a parallel direction is translated into horizontal motion.

static scroll_t *Add_WallScroller(fixed_t dx, fixed_t dy, const line_t *l,
				  int control, int accel, const sector_t *csec)
{
  fixed_t x = abs(l->dx), y = abs(l->dy), d;
  if (y > x)
    d = x, x = y, y = d;
  d = FixedDiv(x, finesine[(tantoangle[FixedDiv(y,x) >> DBITS] + ANG90)
                          >> ANGLETOFINESHIFT]);
  x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
  y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
  return new scroll_t(sc_side, x, y, control, *l->sidenum, accel, csec);
}

// Amount (dx,dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))

// was P_SpawnScroller
// Initialize the scrollers
void Map::SpawnScrollers()
{
  int i;
  line_t *l = lines;

  for (i=0;i<numlines;i++,l++)
    {
      fixed_t dx = l->dx >> SCROLL_SHIFT;  // direction and speed of scrolling
      fixed_t dy = l->dy >> SCROLL_SHIFT;
      int control = -1, accel = 0;         // no control sector or acceleration
      int special = l->special;

      // Types 245-249 are same as 250-254 except that the
      // first side's sector's heights cause scrolling when they change, and
      // this linedef controls the direction and speed of the scrolling. The
      // most complicated linedef since donuts, but powerful :)

      if (special >= 245 && special <= 249)         // displacement scrollers
        {
          special += 250-245;
          control = sides[*l->sidenum].sector - sectors;
        }
      else if (special >= 214 && special <= 218)       // accelerative scrollers
	{
	  accel = 1;
	  special += 250-214;
	  control = sides[*l->sidenum].sector - sectors;
	}
      sector_t *csec = NULL;
      if (control != -1) csec = &sectors[control];

      switch (special)
        {
          register int s;

        case 250:   // scroll effect ceiling
          for (s=-1; (s = FindSectorFromLineTag(l,s)) >= 0;)
	    AddThinker(new scroll_t(sc_ceiling, -dx, dy, control, s, accel, csec));
          break;
        case 251:   // scroll effect floor
        case 253:   // scroll and carry objects on floor
          for (s=-1; (s = FindSectorFromLineTag(l,s)) >= 0;)
            AddThinker(new scroll_t(sc_floor, -dx, dy, control, s, accel, csec));
          if (special != 253)
            break;

        case 252: // carry objects on floor
          dx = FixedMul(dx,CARRYFACTOR);
          dy = FixedMul(dy,CARRYFACTOR);
          for (s=-1; (s = FindSectorFromLineTag(l,s)) >= 0;)
	    AddThinker(new scroll_t(sc_carry, dx, dy, control, s, accel, csec));
          break;

          // scroll wall according to linedef
          // (same direction and speed as scrolling floors)
        case 254:
          for (s=-1; (s = FindLineFromLineTag(l,s)) >= 0;)
            if (s != i)
              AddThinker(Add_WallScroller(dx, dy, lines+s, control, accel, csec));
          break;

        case 255:
          s = lines[i].sidenum[0];
          AddThinker(new scroll_t(sc_side, -sides[s].textureoffset,
                       sides[s].rowoffset, -1, s, accel, NULL));
          break;

        case 48:                  // scroll first side
          AddThinker(new scroll_t(sc_side,  FRACUNIT, 0, -1, lines[i].sidenum[0], accel, NULL));
          break;

        case 99: // heretic right scrolling
          if(game.mode != gm_heretic)
	    break; // doom use it as bluekeydoor
        case 85:                  // jff 1/30/98 2-way scroll
          AddThinker(new scroll_t(sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel, NULL));
          break;
	default:
	  break;
        }
    }
}



/*
  SoM: 3/8/2000: Friction functions start.
  Add_Friction,
  T_Friction,
  P_SpawnFriction
*/

// constructor
// Adds friction thinker.
friction_t::friction_t(float fri, float mf, int aff)
{
  friction = fri;
  movefactor = mf;
  affectee = aff;
}


// was T_Friction
//Function to apply friction to all the things in a sector.
void friction_t::Think()
{
  if (!boomsupport || !variable_friction)
    return;

  sector_t *sec = mp->sectors + affectee;

  // Be sure the special sector type is still turned on. If so, proceed.
  // Else, bail out; the sector type has been changed on us.

  if (!(sec->special & FRICTION_MASK))
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

// was P_SpawnFriction
//Spawn all friction.
void Map::SpawnFriction()
{
  int i;
  line_t *l = lines;
  register int s;
  float length;     // line length controls magnitude
  float friction;   // friction value to be applied during movement
  float movefactor; // applied to each player move to simulate inertia

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
	  AddThinker(new friction_t(friction,movefactor,s));
      }
}





/*
  SoM: 3/8/2000: Push/Pull/Wind/Current functions.
*/


#define PUSH_FACTOR 7

// was Add_Pusher
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
  if (thing->Type() == Thinker::tt_ppawn && !(thing->flags & (MF_NOGRAVITY | MF_NOCLIPLINE)))
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



// was T_Pusher 
// looks for all objects that are inside the radius of
// the effect.

void pusher_t::Think()
{
  if (!allow_pushers)
    return;

  sector_t *sec = mp->sectors + affectee;

  // Be sure the special sector type is still turned on. If so, proceed.
  // Else, bail out; the sector type has been changed on us.

  if (!(sec->special & PUSH_MASK))
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
      if (thing->Type() != Thinker::tt_ppawn)
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

// was P_GetPushThing
// Get pusher object.
DActor *Map::GetPushThing(int s)
{
  Actor* thing;
  sector_t* sec;

  sec = sectors + s;
  thing = sec->thinglist;
  while (thing)
    {
      if (thing->Type() == Thinker::tt_dactor)
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

// was P_SpawnPushers
// Spawn pushers.
void Map::SpawnPushers()
{
  int i;
  line_t *l = lines;
  register int s;

  for (i = 0 ; i < numlines ; i++,l++)
    switch (l->special)
      {
      case 224: // wind
	for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )	  
	  AddThinker(new pusher_t(p_wind, l->dx, l->dy, NULL, s));
	break;
      case 225: // current
	for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )
	  AddThinker(new pusher_t(p_current, l->dx, l->dy, NULL, s));
	break;
      case 226: // push/pull
	for (s = -1; (s = FindSectorFromLineTag(l,s)) >= 0 ; )
	  {
	    // TODO don't spawn push mapthings at all?
	    DActor* thing = GetPushThing(s);
	    if (thing) // No MT_P* means no effect
	      AddThinker(new pusher_t(p_push, l->dx, l->dy, thing, s));
	  }
	break;
      }
}
