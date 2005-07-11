// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.15  2005/07/11 16:58:40  smite-meister
// msecnode_t bug fixed
//
// Revision 1.14  2004/12/02 17:22:34  smite-meister
// HUD fixed
//
// Revision 1.10  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.9  2003/12/07 00:16:34  smite-meister
// hah.
//
// Revision 1.8  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.7  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.6  2003/06/01 18:56:30  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.5  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.4  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.3  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.2  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.1.1.1  2002/11/16 14:18:08  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Part of Map class implementation. Thinkers, Map::Ticker().

#include "g_map.h"
#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "z_zone.h"

#include "r_defs.h"


// Resets the Thinker list
void Map::InitThinkers()
{
  thinkercap.prev = thinkercap.next = &thinkercap;
}


// Adds a new Thinker at the end of the list.
void Map::AddThinker(Thinker *t)
{
  if (!t)
    return;
  t->mp = this;
  thinkercap.prev->next = t;
  t->next = &thinkercap;
  t->prev = thinkercap.prev;
  thinkercap.prev = t;
}


// Removes a Thinker from the Map without deleting it
void Map::DetachThinker(Thinker *t)
{
  t->mp = NULL; // TEST a PlayerPawn often exits the Map while still Thinking! We need this still!
  t->next->prev = t->prev;
  t->prev->next = t->next;
  t->prev = t->next = NULL;

  Z_ChangeTag(t, PU_STATIC);

  force_pointercheck = true;
}


// Moves a Thinker to the removal list
void Map::RemoveThinker(Thinker *t)
{
  t->next->prev = t->prev;
  t->prev->next = t->next;

  // Most Thinkers could be deleted right away (it is assumed that there will be
  // no dangling pointers, or that they have already been taken care of.)
  // This does not work for Actors. Hence...
  DeletionList.push_back(t);
  // TODO perhaps this is needed just for Actors, and other Thinkers can be deleted right away?
}


void Map::RunThinkers()
{
  Thinker *t, *next; 
  for (t = thinkercap.next; t != &thinkercap; t = next)
    {
      next = t->next; // if t is removed while it thinks, its next pointer will no longer be valid.
      //if (t->mp == NULL) I_Error("Thinker::mp == NULL! Cannot be!\n");
      t->Think();
    }

  int n = DeletionList.size();
  if (n == 0 && !force_pointercheck)
    return;

  for (t = thinkercap.next; t != &thinkercap; t = t->next)
    t->CheckPointers();

  // FIXME unfortunate HACK (the entire sound alert system is unrealistic!)
  for (int i=0 ; i<numsectors ; i++)
    if (sectors[i].soundtarget && (sectors[i].soundtarget->eflags & MFE_REMOVE))
      sectors[i].soundtarget = NULL;

  force_pointercheck = false;

  for (int i=0; i<n; i++)
    delete DeletionList[i];

  DeletionList.clear();
}


// Ticks the map forward in time
void Map::Ticker()
{
  //CONS_Printf("Tic begins..");
  int i = 0;

  if (!respawnqueue.empty())
    i = RespawnPlayers();

  //CONS_Printf("think..");
  RunThinkers();

  //CONS_Printf("specials..");
  UpdateSpecials();

  //CONS_Printf("respawnspecials..");
  RespawnSpecials();

  //CONS_Printf("sound sequences..");
  UpdateSoundSequences();

  //CONS_Printf("FS..");
  FS_DelayedScripts();

  HandlePlayers();

  // for par times etc.
  maptic++;

  //CONS_Printf("tick done\n");
}
