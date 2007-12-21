// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Map environmental effects.

#include "g_map.h"
#include "g_mapinfo.h"
#include "p_effects.h"
#include "p_spec.h"

#include "m_random.h"
#include "r_data.h"
#include "sounds.h"


//========================================================
//  Hexen lightning effect
//========================================================

MapEffect::MapEffect(Map *m)
{
  mp = m;

  if (!m->info->lightning)
    return;

  sector_t *s = m->sectors;
  sectorflash_t f = {NULL, 0};
  for (int i = 0; i < m->numsectors; i++)
    {
      if (s[i].SkyCeiling()
	  || s[i].special == SS_IndoorLightning1
	  || s[i].special == SS_IndoorLightning2)
	{
	  f.sec = &s[i];
	  flash_sectors.push_back(f);
	}
    }

  if (flash_sectors.empty())
    {
      m->info->lightning = false; // no way to see the flashes
      return;
    }

  flash_duration = 0;
  flash_delay = ((P_Random() & 15) + 5) * TICRATE; // first flash
}



bool MapEffect::Force()
{
  flash_delay = 0;
  return true;
}



void MapEffect::LightningFlash()
{
  flash_duration--;
  if (flash_duration > 0)
    {
      // flash is fading
      int n = flash_sectors.size();
      for (int i = 0; i < n; i++)
	{
	  sector_t *s = flash_sectors[i].sec;
	  if (flash_sectors[i].orig_light < s->lightlevel - 4)
	    s->lightlevel -= 4;
	}
    }					
  else if (flash_duration == 0)
    {
      // flash is over, return pre-flash light levels
      int n = flash_sectors.size();
      for (int i = 0; i < n; i++)
	flash_sectors[i].sec->lightlevel = flash_sectors[i].orig_light;
      
      mp->skytexture = materials.Get(mp->info->sky1.c_str(), TEX_wall); // set default sky
    }

  flash_delay--;
  if (flash_delay > 0)
    return;

  // new flash
  flash_duration = (P_Random() & 7) + 8;
  short flash = 200 + (P_Random() & 31);

  int n = flash_sectors.size();
  for (int i = 0; i < n; i++)
    {
      sector_t *s = flash_sectors[i].sec;
      flash_sectors[i].orig_light = s->lightlevel; // record current lightlevel

      // add more light
      if (s->special == SS_IndoorLightning1)
	s->lightlevel += 64;
      else if (s->special == SS_IndoorLightning2)
	s->lightlevel += 32;
      else
	s->lightlevel = flash;

      // clamp between original light level and flash level
      s->lightlevel = max(min(s->lightlevel, flash), flash_sectors[i].orig_light);
    }

  mp->skytexture = materials.Get(mp->info->sky2.c_str(), TEX_wall); // set alternate sky
  S_StartAmbSound(NULL, SFX_THUNDER_CRASH);

  // how soon will we get the next flash?
  if (P_Random() < 50)
    flash_delay = (P_Random() & 15) + 16; // rapid second flash
  else
    {
      if (P_Random() < 128 && !(mp->maptic & 32))
	flash_delay = ((P_Random() & 7) + 2) * TICRATE;
      else
	flash_delay = ((P_Random() & 15) + 5) * TICRATE;
    }
}
