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
// Revision 1.3  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.2  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.1.1.1  2002/11/16 14:17:59  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Handle sector based lighting effects.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "p_spec.h"
#include "r_defs.h"
#include "g_map.h"
#include "m_random.h"


// =========================================================================
//                           Sector light effects
// =========================================================================


lightfx_t::lightfx_t(sector_t *s, lightfx_e t, short maxl, short minl, short maxt, short mint)
{
  sec = s;
  type = t;
  count = 0;
  maxlight = maxl;
  minlight = minl;
  maxtime = maxt;
  if (mint < 0)
    currentlight = s->lightlevel << 6; // 10.6 fixed point
  else
    mintime = mint;

  s->lightingdata = this;
}


int lightfx_t::Serialize(LArchive & a)
{
  return 0;
}


void lightfx_t::Think()
{
  int i;

  if (count-- > 0)
    return;

  switch (type)
    {
    case Fade:
      {
	// formerly "lightlevel", the only effect that ends by itself
	bool finish = false;
	currentlight += rate;
	sec->lightlevel = currentlight >> 6;

	if (rate >= 0)
	  {
	    if (sec->lightlevel >= maxlight)
	      finish = true;
	  }
	else
	  {
	    if (sec->lightlevel <= maxlight)
	      finish = true;
	  }

	if (finish)
	  {
	    sec->lightlevel = maxlight;  // set to dest lightlevel
	    sec->lightingdata = NULL;    // clear lightingdata
	    mp->RemoveThinker(this);     // remove thinker       
	  }
      }
      break;

    case Glow:
      currentlight += rate;
      sec->lightlevel = currentlight >> 6;
      if (rate > 0)
	{
	  if (sec->lightlevel > maxlight)
	    {
	      sec->lightlevel = 2*maxlight - sec->lightlevel;
	      currentlight = sec->lightlevel << 6;
	      rate = -rate; // reverse direction
	    }
	}
      else
	{
	  if (sec->lightlevel < minlight)
	    {
	      sec->lightlevel = 2*minlight - sec->lightlevel;
	      currentlight = sec->lightlevel << 6;
	      rate = -rate; // reverse direction
	    }
	}
      break;

    case Strobe:
      if (sec->lightlevel == maxlight)
	{
	  sec->lightlevel = minlight;
	  count = mintime;
	}
      else
	{
	  sec->lightlevel = maxlight;
	  count = maxtime;
	}
      break;

    case Flicker:
      // formerly "lightflash"
      if (sec->lightlevel == maxlight)
	{
	  sec->lightlevel = minlight;
	  count = (P_Random() % mintime) + 1;
	}
      else
	{
	  sec->lightlevel = maxlight;
	  count = (P_Random() % maxtime) + 1;
	}
      break;

    case FireFlicker:
      i = (P_Random()&3) * 16;
      if (maxlight - i < minlight)
	sec->lightlevel = minlight;
      else
	sec->lightlevel = maxlight - i;

      count = maxtime;
      break;

    default:
      break;
    }
}


//
// was P_SpawnStrobeFlash
//
void Map::SpawnStrobeLight(sector_t *sec, short brighttime, short darktime, bool inSync)
{
  short minlight = P_FindMinSurroundingLight(sec, sec->lightlevel);
  short maxlight = sec->lightlevel;
  if (minlight == maxlight)
    minlight = 0;

  lightfx_t *lfx = new lightfx_t(sec, lightfx_t::Strobe, maxlight, minlight, brighttime, darktime);

  AddThinker(lfx);

  if (!inSync)
    lfx->count = (P_Random() & 7) + 1;
  else
    lfx->count = 1;
}


//
// Start strobing lights (usually from a trigger)
//
int Map::EV_StartLightStrobing(int tag)
{
  int rtn = 0;

  for (int i = -1; (i = FindSectorFromTag(tag,i)) >= 0; )
    {
      rtn++;
      sector_t *sec = &sectors[i];
      if (P_SectorActive(lighting_special,sec)) //SoM: 3/7/2000: New way to check thinker
	continue;

      SpawnStrobeLight(sec, STROBEBRIGHT, SLOWDARK, false);
    }

  return rtn;
}



//
// TURN LINE'S TAG LIGHTS OFF
//
int Map::EV_TurnTagLightsOff(int tag)
{
  int                 i;
  int                 j;
  int                 min;
  sector_t*           sector;
  sector_t*           tsec;
  line_t*             templine;

  sector = sectors;

  for (j = 0;j < numsectors; j++, sector++)
    {
      if (sector->tag == tag)
        {
	  min = sector->lightlevel;
	  for (i = 0;i < sector->linecount; i++)
            {
	      templine = sector->lines[i];
	      tsec = getNextSector(templine,sector);
	      if (!tsec)
		continue;
	      if (tsec->lightlevel < min)
		min = tsec->lightlevel;
            }
	  sector->lightlevel = min;
        }
    }
  return 1;
}


//
// TURN LINE'S TAG LIGHTS ON
//
int Map::EV_LightTurnOn(int tag, int bright)
{
  int         i;
  int         j;
  sector_t*   sector;
  sector_t*   temp;
  line_t*     templine;

  sector = sectors;

  for (i=0;i<numsectors;i++, sector++)
    {
      int tbright = bright; //SoM: 3/7/2000: Search for maximum per sector
      if (sector->tag == tag)
        {
	  // bright = 0 means to search
	  // for highest light level
	  // surrounding sector
	  if (!bright)
            {
	      for (j = 0;j < sector->linecount; j++)
                {
		  templine = sector->lines[j];
		  temp = getNextSector(templine,sector);
		  if (!temp)
		    continue;

		  if (temp->lightlevel > tbright) //SoM: 3/7/2000
		    tbright = temp->lightlevel;
                }
            }
	  sector-> lightlevel = tbright;
	  if(!boomsupport)
	    bright = tbright;
        }
    }
  return 1;
}


//
// Fade all the lights in sectors with a particular tag to a new value
//
int Map::EV_FadeLight(int tag, int destvalue, int speed)
{
  int rtn = 0;
  // search all sectors for ones with tag
  for (int i = -1; (i = FindSectorFromTag(tag, i)) >= 0; )
    {
      rtn++;
      sector_t *sec = &sectors[i];

      lightfx_t *lfx = new lightfx_t(sec, lightfx_t::Fade, destvalue, 0, speed << 6, -1);
      AddThinker(lfx);
  }

  return rtn;
}


//
// Start a light effect in all sectors with a particular tag
//
int Map::EV_SpawnLight(int tag, int type, short maxl, short minl, short maxt, short mint)
{
  lightfx_t *lfx;
  int rtn = 0;
  fixed_t speed;

  for (int i = -1; (i = FindSectorFromTag(tag, i)) >= 0; )
    {
      rtn++;
      lfx = NULL;
      sector_t *sec = &sectors[i];

      switch (type)
	{
	case lightfx_t::RelChange:
	  sec->lightlevel += maxl;
	  if (sec->lightlevel > 255)
	    sec->lightlevel = 255;
	  else if (sec->lightlevel < 0)
	    sec->lightlevel = 0;
	  break;

	case lightfx_t::AbsChange:
	  sec->lightlevel = maxl;
	  if (sec->lightlevel < 0)
	    sec->lightlevel = 0;
	  else if(sec->lightlevel > 255)
	    sec->lightlevel = 255;
	  break;

	case lightfx_t::Fade:
	  speed = FixedDiv((maxl - sec->lightlevel) << FRACBITS, maxt << FRACBITS);
	  lfx = new lightfx_t(sec, lightfx_t::Fade, maxl, 0, speed >> 10, -1); // use a custom 10.6 fixed point
	  break;

	case lightfx_t::Glow:
	  speed = FixedDiv((maxl - minl) << FRACBITS, maxt << FRACBITS);
	  lfx = new lightfx_t(sec, lightfx_t::Glow, maxl, minl, speed >> 10, -1); // use a custom 10.6 fixed point
	  break;

	case lightfx_t::Flicker:
	  sec->lightlevel = maxl;
	  lfx = new lightfx_t(sec, lightfx_t::Flicker, maxl, minl, maxt, mint);
	  lfx->count = (P_Random() & 64) + 1;
	  break;

	case lightfx_t::Strobe:
	  sec->lightlevel = maxl;
	  lfx = new lightfx_t(sec, lightfx_t::Strobe, maxl, minl, maxt, mint);
	  lfx->count = maxt;
	  break;

	default:
	  rtn = false;
	  break;
	}

      if (lfx)
	AddThinker(lfx);
    }
  return rtn;
}



//==========================================================================
//
// Phased lights (Hexen)
//
//==========================================================================

static int PhaseTable[64] =
{
  128, 112, 96, 80, 64, 48, 32, 32,
  16, 16, 16, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 16, 16, 16,
  32, 32, 48, 64, 80, 96, 112, 128
};


int phasedlight_t::Serialize(LArchive & a)
{
  return 0;
}


void phasedlight_t::Think()
{
  index = (index + 1) & 63;
  sec->lightlevel = base + PhaseTable[index];
}


phasedlight_t::phasedlight_t(sector_t *s, int b, int ind)
{
  sec = s;
  s->lightingdata = this;

  if (ind == -1)
    index = s->lightlevel & 63; // by-hand method
  else
    index = ind & 63;  // automatic method

  base = b & 255;
  s->lightlevel = base + PhaseTable[index];
}


//
// was P_SpawnLightSequence
//
void Map::SpawnPhasedLightSequence(sector_t *sector, int indexStep)
{
  int i;

  sector_t *sec = sector;
  sector_t *nextSec;
  sector_t *tempSec;

  int count = 1;
  int seqSpecial = SS_LightSequence_1; // look for LightSequence_1 first
  do
    {
      nextSec = NULL;
      sec->special = SS_LightSequence_Start; // make sure that the search doesn't back up.
      for (i = 0; i < sec->linecount; i++)
	{
	  tempSec = getNextSector(sec->lines[i], sec);
	  if (!tempSec)
	    continue;

	  if (tempSec->special == seqSpecial)
	    {
	      if (seqSpecial == SS_LightSequence_1)
		seqSpecial = SS_LightSequence_2;
	      else
		seqSpecial = SS_LightSequence_1;

	      nextSec = tempSec;
	      count++;
	    }
	}
      sec = nextSec;
    } while(sec);
  
  sec = sector;
  count *= indexStep;
  fixed_t index = 0;
  fixed_t indexDelta = FixedDiv(64*FRACUNIT, count*FRACUNIT);
  int base = sector->lightlevel;
  do
    {
      nextSec = NULL;
      if (sec->lightlevel)
	base = sec->lightlevel;
      AddThinker(new phasedlight_t(sec, base, index >> FRACBITS));
      index += indexDelta;
      for (i = 0; i < sec->linecount; i++)
	{
	  tempSec = getNextSector(sec->lines[i], sec);
	  if (!tempSec)
	    continue;

	  if (tempSec->special == SS_LightSequence_Start)
	    nextSec = tempSec;
	}
      sec = nextSec;
    } while(sec);
}
