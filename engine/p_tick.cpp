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
// Revision 1.1  2002/11/16 14:18:08  hurdler
// Initial revision
//
// Revision 1.15  2002/09/08 14:38:06  vberghol
// Now it works! Sorta.
//
// Revision 1.14  2002/09/05 14:12:15  vberghol
// network code partly bypassed
//
// Revision 1.12  2002/08/17 21:21:53  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.11  2002/08/16 20:49:26  vberghol
// engine ALMOST done!
//
// Revision 1.10  2002/08/11 17:16:50  vberghol
// ...
//
// Revision 1.9  2002/08/06 13:14:25  vberghol
// ...
//
// Revision 1.8  2002/07/23 19:21:44  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.7  2002/07/18 19:16:38  vberghol
// renamed a few files
//
// Revision 1.6  2002/07/12 19:21:39  vberghol
// hop
//
// Revision 1.5  2002/07/10 19:57:01  vberghol
// g_pawn.cpp tehty
//
// Revision 1.4  2002/07/01 21:00:22  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:54  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.6  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.5  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/10/21 08:43:31  bpereira
// no message
//
// Revision 1.3  2000/10/08 13:30:01  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
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
  // FIXME: NOP.
  //thinker->function.acv = (actionf_v)(-1);
  thinker->mp = NULL;
}


//
// P_AllocateThinker
// Allocates memory and adds a new thinker at the end of the list.
//

//void P_AllocateThinker (thinker_t *thinker) { }


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

  CONS_Printf("Map::Ticker: nplayers = %d\n", n);
  if (!respawnqueue.empty())
    i = RespawnPlayers();

  CONS_Printf("=> %d respawns\n", i);

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
