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
// Revision 1.1  2002/11/16 14:17:59  hurdler
// Initial revision
//
// Revision 1.6  2002/08/17 21:21:49  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.5  2002/08/06 13:14:23  vberghol
// ...
//
// Revision 1.4  2002/07/23 19:21:42  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:19  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:14  vberghol
// Version 133 Experimental!
//
// Revision 1.5  2000/11/02 17:50:07  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
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
//      Handle Sector base lighting effects.
//      Muzzle flash?
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "p_spec.h"
#include "r_state.h"
#include "g_map.h"
#include "z_zone.h"
#include "m_random.h"


// =========================================================================
//                           FIRELIGHT FLICKER
// =========================================================================

//
// was T_FireFlicker
//
void fireflicker_t::Think()
{
  int amount;

  if (--count)
    return;

  amount = (P_Random()&3)*16;

  if (sector->lightlevel - amount < minlight)
    sector->lightlevel = minlight;
  else
    sector->lightlevel = maxlight - amount;

  count = 4;
}



//
// was P_SpawnFireFlicker
//
void Map::SpawnFireFlicker(sector_t *sector)
{
  fireflicker_t *flick;

  // Note that we are resetting sector attributes.
  // Nothing special about it during gameplay.
  sector->special &= ~31; //SoM: Clear non-generalized sector type

  flick = new fireflicker_t();
  AddThinker(flick);

  flick->sector = sector;
  flick->maxlight = sector->lightlevel;
  flick->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel)+16;
  flick->count = 4;
}



//
// BROKEN LIGHT FLASHING
//


//
// was T_LightFlash
// Do flashing lights.
//
void lightflash_t::Think()
{
  if (--count)
    return;

  if (sector->lightlevel == maxlight)
    {
       sector->lightlevel = minlight;
      count = (P_Random()&mintime)+1;
    }
  else
    {
       sector->lightlevel = maxlight;
      count = (P_Random()&maxtime)+1;
    }
}


//
// was P_SpawnLightFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void Map::SpawnLightFlash(sector_t *sector)
{
  lightflash_t*       flash;

  // nothing special about it during gameplay
  sector->special &= ~31; //SoM: 3/7/2000: Clear non-generalized type

  flash = new lightflash_t();
  AddThinker(flash);

  flash->sector = sector;
  flash->maxlight = sector->lightlevel;

  flash->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
  flash->maxtime = 64;
  flash->mintime = 7;
  flash->count = (P_Random()&flash->maxtime)+1;
}



//
// STROBE LIGHT FLASHING
//


//
// was T_StrobeFlash
//
void strobe_t::Think()
{
  if (--count)
    return;

  if (sector->lightlevel == minlight)
    {
      sector->lightlevel = maxlight;
      count = brighttime;
    }
  else
    {
      sector->lightlevel = minlight;
      count =darktime;
    }
}

//
// was P_SpawnStrobeFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void Map::SpawnStrobeFlash(sector_t *sector, int fastOrSlow, int inSync)
{
  strobe_t*   flash;

  flash = new strobe_t();
  AddThinker (flash);

  flash->sector = sector;
  flash->darktime = fastOrSlow;
  flash->brighttime = STROBEBRIGHT;
  flash->maxlight = sector->lightlevel;
  flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

  if (flash->minlight == flash->maxlight)
    flash->minlight = 0;
  // nothing special about it during gameplay
  sector->special &= ~31; //SoM: 3/7/2000: Clear non-generalized sector type

  if (!inSync)
    flash->count = (P_Random()&7)+1;
  else
    flash->count = 1;
}


// was EV_StartLightStrobing
// Start strobing lights (usually from a trigger)
//
int Map::EV_StartLightStrobing(line_t *line)
{
  int         secnum;
  sector_t*   sec;

  secnum = -1;
  while ((secnum = FindSectorFromLineTag(line,secnum)) >= 0)
    {
      sec = &sectors[secnum];
      if (P_SectorActive(lighting_special,sec)) //SoM: 3/7/2000: New way to check thinker
	continue;

      SpawnStrobeFlash (sec,SLOWDARK, 0);
    }
  return 1;
}



// was EV_TurnTagLightsOff
// TURN LINE'S TAG LIGHTS OFF
//
int Map::EV_TurnTagLightsOff(line_t* line)
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
      if (sector->tag == line->tag)
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


// was EV_LightTurnOn
// TURN LINE'S TAG LIGHTS ON
//
int Map::EV_LightTurnOn(line_t *line, int bright)
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
      if (sector->tag == line->tag)
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


// was T_Glow
// Spawn glowing light
//

void glow_t::Think()
{
    switch(direction)
    {
      case -1:
        // DOWN
        sector->lightlevel -= GLOWSPEED;
        if (sector->lightlevel <= minlight)
        {
            sector->lightlevel += GLOWSPEED;
            direction = 1;
        }
        break;

      case 1:
        // UP
        sector->lightlevel += GLOWSPEED;
        if (sector->lightlevel >= maxlight)
        {
            sector->lightlevel -= GLOWSPEED;
            direction = -1;
        }
        break;
    }
}

// was P_SpawnGlowingLight
void Map::SpawnGlowingLight(sector_t *sector)
{
  glow_t*     g;

  g = new glow_t();

  AddThinker(g);

  g->sector = sector;
  g->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
  g->maxlight = sector->lightlevel;
  g->direction = -1;

  sector->special &= ~31; //SoM: 3/7/2000: Reset only non-generic types.
}



// was P_FadeLight()
//
// Fade all the lights in sectors with a particular tag to a new value
//
void Map::FadeLight(int tag, int destvalue, int speed)
{
  int i;
  lightlevel_t *ll;

  // search all sectors for ones with tag
  for (i = -1; (i = FindSectorFromTag(tag,i)) >= 0;)
    {
      sector_t *sector = &sectors[i];
      sector->lightingdata = sector;    // just set it to something

      ll = new lightlevel_t();
      AddThinker(ll);       // add thinker

      ll->sector = sector;
      ll->destlevel = destvalue;
      ll->speed = speed;
  }
}



// was T_LightFade()
//
// Just fade the light level in a sector to a new level
//

void lightlevel_t::Think()
{
  if(sector->lightlevel < destlevel)
  {
      // increase the lightlevel
    if(sector->lightlevel + speed >= destlevel)
    {
          // stop changing light level
       sector->lightlevel = destlevel;    // set to dest lightlevel

       sector->lightingdata = NULL;          // clear lightingdata
       mp->RemoveThinker(this);    // remove thinker       
    }
    else
    {
        sector->lightlevel += speed; // move lightlevel
    }
  }
  else
  {
        // decrease lightlevel
    if(sector->lightlevel - speed <= destlevel)
    {
          // stop changing light level
       sector->lightlevel = destlevel;    // set to dest lightlevel

       sector->lightingdata = NULL;          // clear lightingdata
       mp->RemoveThinker(this);            // remove thinker       
    }
    else
    {
        sector->lightlevel -= speed;      // move lightlevel
    }
  }
}

