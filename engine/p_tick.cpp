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
// Revision 1.3  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.2  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.1.1.1  2002/11/16 14:18:08  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//  Part of Map class implementation.
//      Thinkers, Ticker().
//
//-----------------------------------------------------------------------------


#include "g_map.h"
#include "g_game.h"
#include "g_player.h" // invTics
#include "g_pawn.h" // invTics
#include "z_zone.h"
#include "t_script.h"

// (old info)
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//


//
// was P_InitThinkers
//
void Map::InitThinkers()
{
  thinkercap.prev = thinkercap.next = &thinkercap;
}


//
// was P_AddThinker
// Adds a new thinker at the end of the list.
//
void Map::AddThinker(Thinker *thinker)
{
  thinker->mp = this;
  thinkercap.prev->next = thinker;
  thinker->next = &thinkercap;
  thinker->prev = thinkercap.prev;
  thinkercap.prev = thinker;
}

// removes a thinker from the Map without deleting it
void Map::DetachThinker(Thinker *thinker)
{
  thinker->mp = NULL;
  thinker->next->prev = thinker->prev;
  thinker->prev->next = thinker->next;
  thinker->prev = thinker->next = NULL;
  Z_ChangeTag(thinker, PU_STATIC);
}

//
// was P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
void Map::RemoveThinker(Thinker *thinker)
{
  //thinker->function.acv = (actionf_v)(-1);
  thinker->mp = NULL;
}


//
// was P_RunThinkers
//
void Map::RunThinkers()
{
  Thinker *t;

  t = thinkercap.next;
  while (t != &thinkercap)
    {
      if (t->mp == NULL)
        {
	  // time to remove it
	  t->next->prev = t->prev;
	  t->prev->next = t->next;
	  Thinker *removeit = t;
	  t = t->next;
	  delete removeit;
        }
      else
        {
	  t->Think();
	  t = t->next;
        }
    }
}


//
// was P_Ticker
//
void Map::Ticker()
{
  int i=0, n = players.size();

  if (!respawnqueue.empty())
    i = RespawnPlayers();

  // players[i]->Think();
  // now playerpawns are ticked with other Thinkers at RunThinkers.
  // the Thinking order may have changed...

  RunThinkers();
  UpdateSpecials();
  RespawnSpecials();

  AmbientSound();

  // for par times etc.
  maptic++;

#ifdef FRAGGLESCRIPT
  // SoM: Update FraggleScript...
  T_DelayedScripts();
#endif
}
