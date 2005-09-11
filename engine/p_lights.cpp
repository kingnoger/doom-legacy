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
// Revision 1.15  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.14  2005/05/26 17:22:50  smite-meister
// windows alpha fix
//
// Revision 1.13  2005/03/04 16:23:07  smite-meister
// mp3, sector_t
//
// Revision 1.12  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.11  2004/09/03 16:28:50  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.10  2004/04/25 16:26:49  smite-meister
// Doxygen
//
// Revision 1.9  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.8  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.6  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.4  2003/11/12 11:07:22  smite-meister
// Serialization done. Map progression.
//
// Revision 1.3  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.2  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.1.1.1  2002/11/16 14:17:59  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Sector based lighting effects.

#include "doomdef.h"

#include "g_map.h"

#include "p_spec.h"
#include "m_random.h"


//=========================================================================
//                          Sector light effects
//=========================================================================

IMPLEMENT_CLASS(lightfx_t, sectoreffect_t);
lightfx_t::lightfx_t() {}

lightfx_t::lightfx_t(Map *m, sector_t *s, lightfx_e t, short maxl, short minl, short maxt, short mint)
  : sectoreffect_t(m, s)
{
  type = t;
  count = 0;
  maxlight = maxl;
  minlight = minl;
  maxtime = maxt;
  if (mint < 0)
    {
      // use 10.6 fixed point (we assume that rate is already given in this format)
      currentlight = s->lightlevel << 6;
      maxlight <<= 6;
      minlight <<= 6;
    }
  else
    mintime = mint;

  s->lightingdata = this;
}


void lightfx_t::Think()
{
  int i;

  if (count > 0)
    {
      count--;
      return;
    }

  switch (type)
    {
    case Fade:
      {
	// formerly "lightlevel", the only effect that ends by itself
	bool finish = false;
	currentlight += rate;

	if (rate >= 0)
	  {
	    if (currentlight >= maxlight)
	      finish = true;
	  }
	else
	  {
	    if (currentlight <= maxlight)
	      finish = true;
	  }

	if (finish)
	  {
	    sector->lightlevel = maxlight >> 6;  // set to dest lightlevel
	    sector->lightingdata = NULL;    // clear lightingdata
	    mp->RemoveThinker(this);     // remove thinker       
	  }
	else
	  sector->lightlevel = currentlight >> 6;
      }
      break;

    case Glow:
      currentlight += rate;
      if (rate > 0)
	{
	  if (currentlight > maxlight)
	    {
	      //currentlight = 2*maxlight - currentlight;
	      // this reflection, although basically correct, causes unstable oscillations if rate is too high!
	      currentlight = maxlight;
	      rate = -rate; // reverse direction
	    }
	}
      else
	{
	  if (currentlight < minlight)
	    {
	      //currentlight = 2*minlight - currentlight;
	      currentlight = minlight;
	      rate = -rate; // reverse direction
	    }
	}
      sector->lightlevel = currentlight >> 6;
      break;

    case Strobe:
      if (sector->lightlevel == maxlight)
	{
	  sector->lightlevel = minlight;
	  count = mintime;
	}
      else
	{
	  sector->lightlevel = maxlight;
	  count = maxtime;
	}
      break;

    case Flicker:
      // formerly "lightflash"
      if (sector->lightlevel == maxlight)
	{
	  sector->lightlevel = minlight;
	  count = (P_Random() % mintime) + 1;
	}
      else
	{
	  sector->lightlevel = maxlight;
	  count = (P_Random() % maxtime) + 1;
	}
      break;

    case FireFlicker:
      i = (P_Random()&3) * 16;
      if (maxlight - i < minlight)
	sector->lightlevel = minlight;
      else
	sector->lightlevel = maxlight - i;

      count = maxtime;
      break;

    default:
      break;
    }
}



void Map::SpawnStrobeLight(sector_t *sec, short brighttime, short darktime, bool inSync)
{
  short minlight = sec->FindMinSurroundingLight(sec->lightlevel);
  short maxlight = sec->lightlevel;
  if (minlight == maxlight)
    minlight = 0;

  lightfx_t *lfx = new lightfx_t(this, sec, lightfx_t::Strobe, maxlight, minlight, brighttime, darktime);

  if (!inSync)
    lfx->count = (P_Random() & 7) + 1;
  else
    lfx->count = 1;
}


//
// Start strobing lights (usually from a trigger)
//
int Map::EV_StartLightStrobing(int tag, short brighttime, short darktime)
{
  int rtn = 0;

  for (int i = -1; (i = FindSectorFromTag(tag,i)) >= 0; )
    {
      rtn++;
      sector_t *sec = &sectors[i];
      if (sec->Active(sector_t::lighting_special))
	continue;

      SpawnStrobeLight(sec, brighttime, darktime, false);
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

      new lightfx_t(this, sec, lightfx_t::Fade, destvalue, 0, speed << 6, -1);
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

  if (!tag)
    return false;

  for (int i = -1; (i = FindSectorFromTag(tag, i)) >= 0; )
    {
      rtn++;
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
	  speed = fixed_t(maxl - sec->lightlevel) / fixed_t(maxt);
	  new lightfx_t(this, sec, lightfx_t::Fade, maxl, 0, speed.value() >> 10, -1); // use a custom 10.6 fixed point
	  break;

	case lightfx_t::Glow:
	  speed = fixed_t(maxl - minl) / fixed_t(maxt);
	  new lightfx_t(this, sec, lightfx_t::Glow, maxl, minl, speed.value() >> 10, -1); // use a custom 10.6 fixed point
	  break;

	case lightfx_t::Flicker:
	  sec->lightlevel = maxl;
	  lfx = new lightfx_t(this, sec, lightfx_t::Flicker, maxl, minl, maxt, mint);
	  lfx->count = (P_Random() & 64) + 1;
	  break;

	case lightfx_t::Strobe:
	  sec->lightlevel = maxl;
	  lfx = new lightfx_t(this, sec, lightfx_t::Strobe, maxl, minl, maxt, mint);
	  lfx->count = maxt;
	  break;

	default:
	  rtn = false;
	  break;
	}
    }
  return rtn;
}



//==========================================================================
//                         Phased lights (Hexen)
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

IMPLEMENT_CLASS(phasedlight_t, sectoreffect_t);
phasedlight_t::phasedlight_t() {}

void phasedlight_t::Think()
{
  index = (index + 1) & 63;
  sector->lightlevel = base + PhaseTable[index];
}


phasedlight_t::phasedlight_t(Map *m, sector_t *s, int b, int ind)
  : sectoreffect_t(m, s)
{
  s->lightingdata = this;

  if (ind == -1)
    index = s->lightlevel & 63; // by-hand method
  else
    index = ind & 63;  // automatic method

  base = b & 255;
  s->lightlevel = base + PhaseTable[index];
}


// Spawns a sequence of phased lights following the sector types 3 and 4
void Map::SpawnPhasedLightSequence(sector_t *sector, int indexStep)
{
  int i;

  sector_t *sec = sector;
  sector_t *temp, *next;

  deque<sector_t*> run;

  int phase = 0; // look for LightSequence_1 first

  // TODO for now this is limited to one non-branching sequence
  // that does not touch other sequences

  // first count the sequence lenght to determine indexDelta
  while (sec && !sec->lightingdata)
    {
      run.push_back(sec);
      sec->special = 0; // make sure that the search doesn't back up.

      next = NULL;
      for (i = 0; i < sec->linecount; i++)
	{
	  temp = getNextSector(sec->lines[i], sec);
	  if (!temp)
	    continue;

	  if (temp->special == phase + SS_LightSequence_1)
	    {
	      phase = phase^1; // phase alternates
	      next = temp;
	      break; // early out
	    }
	}
      sec = next;
    }

  fixed_t index = 0;
  fixed_t indexDelta = fixed_t(64) / fixed_t(indexStep * int(run.size()));
  int base = sector->lightlevel;

  while (!run.empty())
    {
      sec = run.front();
      run.pop_front();

      if (sec->lightlevel)
	base = sec->lightlevel;
      new phasedlight_t(this, sec, base, index.floor());
      index += indexDelta;
    }
}
