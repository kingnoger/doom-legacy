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
// Revision 1.19  2004/11/19 16:51:04  smite-meister
// cleanup
//
// Revision 1.18  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.14  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.11  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.9  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.8  2003/05/30 13:34:47  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.7  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.6  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.5  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.4  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.3  2003/03/08 16:07:09  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:18:04  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Switches, buttons. Two-state animation.

#include <vector>

#include "g_game.h"
#include "g_map.h"

#include "p_spec.h"
#include "m_swap.h"

#include "r_data.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"


IMPLEMENT_CLASS(button_t, Thinker);
button_t::button_t() {}


struct switchdef_t
{
  char name1[9], name2[9];
  short sound;
  short episode;
};

switchdef_t DoomSwitchList[] =
{
  // Doom shareware episode 1 switches
  {"SW1BRCOM","SW2BRCOM", sfx_switchon, 1},
  {"SW1BRN1", "SW2BRN1",  sfx_switchon, 1},
  {"SW1BRN2", "SW2BRN2",  sfx_switchon, 1},
  {"SW1BRNGN","SW2BRNGN", sfx_switchon, 1},
  {"SW1BROWN","SW2BROWN", sfx_switchon, 1},
  {"SW1COMM", "SW2COMM",  sfx_switchon, 1},
  {"SW1COMP", "SW2COMP",  sfx_switchon, 1},
  {"SW1DIRT", "SW2DIRT",  sfx_switchon, 1},
  {"SW1EXIT", "SW2EXIT",  sfx_switchon, 1},
  {"SW1GRAY", "SW2GRAY",  sfx_switchon, 1},
  {"SW1GRAY1","SW2GRAY1", sfx_switchon, 1},
  {"SW1METAL","SW2METAL", sfx_switchon, 1},
  {"SW1PIPE", "SW2PIPE",  sfx_switchon, 1},
  {"SW1SLAD", "SW2SLAD",  sfx_switchon, 1},
  {"SW1STARG","SW2STARG", sfx_switchon, 1},
  {"SW1STON1","SW2STON1", sfx_switchon, 1},
  {"SW1STON2","SW2STON2", sfx_switchon, 1},
  {"SW1STONE","SW2STONE", sfx_switchon, 1},
  {"SW1STRTN","SW2STRTN", sfx_switchon, 1},

  // Doom registered episodes 2&3 switches
  {"SW1BLUE", "SW2BLUE",  sfx_switchon, 2},
  {"SW1CMT",  "SW2CMT",   sfx_switchon, 2},
  {"SW1GARG", "SW2GARG",  sfx_switchon, 2},
  {"SW1GSTON","SW2GSTON", sfx_switchon, 2},
  {"SW1HOT",  "SW2HOT",   sfx_switchon, 2},
  {"SW1LION", "SW2LION",  sfx_switchon, 2},
  {"SW1SATYR","SW2SATYR", sfx_switchon, 2},
  {"SW1SKIN", "SW2SKIN",  sfx_switchon, 2},
  {"SW1VINE", "SW2VINE",  sfx_switchon, 2},
  {"SW1WOOD", "SW2WOOD",  sfx_switchon, 2},

    // Doom II switches
  {"SW1PANEL","SW2PANEL", sfx_switchon, 3},
  {"SW1ROCK", "SW2ROCK",  sfx_switchon, 3},
  {"SW1MET2", "SW2MET2",  sfx_switchon, 3},
  {"SW1WDMET","SW2WDMET", sfx_switchon, 3},
  {"SW1BRIK", "SW2BRIK",  sfx_switchon, 3},
  {"SW1MOD1", "SW2MOD1",  sfx_switchon, 3},
  {"SW1ZIM",  "SW2ZIM",   sfx_switchon, 3},
  {"SW1STON6","SW2STON6", sfx_switchon, 3},
  {"SW1TEK",  "SW2TEK",   sfx_switchon, 3},
  {"SW1MARB", "SW2MARB",  sfx_switchon, 3},
  {"SW1SKULL","SW2SKULL", sfx_switchon, 3},

  {"\0",      "\0",       sfx_switchon, 0}
};

switchdef_t HereticSwitchList[] =
{
  // heretic
  {"SW1OFF",  "SW1ON", sfx_switchon, 1},
  {"SW2OFF",  "SW2ON", sfx_switchon, 1},

  {"\0",      "\0",    sfx_switchon, 0}
};

// TODO Hexen switch sounds
switchdef_t HexenSwitchList[] =
{
  {"SW_1_UP",  "SW_1_DN", sfx_switchon, 1},
  {"SW_2_UP",  "SW_2_DN", sfx_switchon, 1},
  {"VALVE1",   "VALVE2",  SFX_VALVE_TURN, 1},
  {"SW51_OFF", "SW51_ON", sfx_switchoff, 1},
  {"SW52_OFF", "SW52_ON", sfx_switchoff, 1},
  {"SW53_UP",  "SW53_DN", SFX_ROPE_PULL, 1},
  {"PUZZLE5",  "PUZZLE9", sfx_switchon, 1},
  {"PUZZLE6",  "PUZZLE10", sfx_switchon, 1},
  {"PUZZLE7",  "PUZZLE11", sfx_switchon, 1},
  {"PUZZLE8",  "PUZZLE12", sfx_switchon, 1},
  {"\0",       "\0",       sfx_switchon, 0}
};


// template for the Boom SWITCHES lump
struct switches_t
{
  char   name1[9];
  char   name2[9];
  short  episode;
};


struct switchlist_t
{
  int tex;
  short sound;
};

// list of switch types, common to all maps
static vector<switchlist_t> switchlist;


// TODO should each map have its own swlist? same goes with animated flats/textures?
void P_InitSwitchList()
{
  int i, n, nameset;
  switchdef_t *sd = DoomSwitchList;

  switch (game.mode)
    {
    case gm_doom1:
    case gm_udoom:
      nameset = 2;
      break;
    case gm_doom2:
      nameset = 3;
      break;
    case gm_heretic:
      sd = HereticSwitchList;
      nameset = 1;
      break;
    case gm_hexen:
      sd = HexenSwitchList;
      nameset = 1;
      break;
    default:
      nameset = 1;
    }

  switchlist.clear();
  switchlist_t temp;

  // Is Boom SWITCHES lump present?
  if ((i = fc.FindNumForName("SWITCHES")) != -1)
    {
      // ss is not needed anymore after this function, therefore not PU_STATIC
      switches_t *ss = (switches_t *)fc.CacheLumpNum(i, PU_CACHE);

      // endian conversion only when loading from extra lump
      for (i=0; ss[i].episode != 0; i++)
	{
	  n = SHORT(ss[i].episode);
	  if (n > nameset)
	    continue;

	  temp.tex = tc.Get(ss[i].name1);
	  temp.sound = sfx_switchon; // default
	  switchlist.push_back(temp);

	  temp.tex = tc.Get(ss[i].name2);
	  temp.sound = sfx_switchon; // default
	  switchlist.push_back(temp);
	}

      return; // nothing else
    }

  for (i=0; sd[i].episode != 0; i++)
    {
      if (sd[i].episode > nameset)
	continue;

      temp.tex = tc.Get(sd[i].name1);
      temp.sound = sd[i].sound;
      switchlist.push_back(temp);

      temp.tex = tc.Get(sd[i].name2);
      temp.sound = sd[i].sound; // could have different on/off sounds
      switchlist.push_back(temp);
    }
}



// Start a button counting down till it turns off.
button_t::button_t(line_t *l, button_e w, int tex, int time)
{
  l->thinker = this;

  line = l;
  where = w;
  texture = tex;
  timer = time;
  soundorg = &line->frontsector->soundorg;
}


// timed switches
void button_t::Think()
{
  if (timer > 0)
    timer--;

  if (!timer)
    {
      switch (where)
	{
	case top:
	  mp->sides[line->sidenum[0]].toptexture = texture;
	  break;

	case middle:
	  mp->sides[line->sidenum[0]].midtexture = texture;
	  break;

	case bottom:
	  mp->sides[line->sidenum[0]].bottomtexture = texture;
	  break;
	default:
	  break;
	}
      S_StartSound(soundorg, sfx_switchon);
      line->thinker = NULL;
      mp->RemoveThinker(this);  // unlink and free
    }
}


// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
void Map::ChangeSwitchTexture(line_t *line, int useAgain)
{
  int     texTop, texMid, texBot;
  int     i, n = switchlist.size();
  button_t::button_e loc = button_t::none;

  if (!useAgain)
    line->special = 0;

  texTop = sides[line->sidenum[0]].toptexture;
  texMid = sides[line->sidenum[0]].midtexture;
  texBot = sides[line->sidenum[0]].bottomtexture;

  for (i = 0; i < n; i++)
    {
      if (switchlist[i].tex == texTop)
        {
	  sides[line->sidenum[0]].toptexture = switchlist[i^1].tex; // clever...
	  loc = button_t::top;
	  break;
        }
      else if (switchlist[i].tex == texMid)
	{
	  sides[line->sidenum[0]].midtexture = switchlist[i^1].tex;
	  loc = button_t::middle;
	  break;
	}
      else if (switchlist[i].tex == texBot)
	{
	  sides[line->sidenum[0]].bottomtexture = switchlist[i^1].tex;
	  loc = button_t::bottom;
	  break;
	}
    }

  if (loc != button_t::none)
    {
      // EXIT SWITCH?
      if (line->special == 74)
	S_StartSound(&line->frontsector->soundorg, sfx_switchoff);
      else
	S_StartSound(&line->frontsector->soundorg, switchlist[i].sound);

      // See if it is a button and not already pressed
      if (useAgain && line->thinker == NULL)
	{
	  button_t *but = new button_t(line, loc, switchlist[i].tex, BUTTONTIME);
	  AddThinker(but);
	}
    }
}
