// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2006 by DooM Legacy Team.
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
/// \brief Game types (dm, coop, ctf...)

#include "tnl/tnlBitStream.h"
#include "tnl/tnlGhostConnection.h"

#include "n_interface.h"
#include "n_connection.h"

#include "command.h"
#include "cvars.h"

#include "g_type.h"
#include "g_game.h"
#include "g_team.h"
#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"

#include "w_wad.h"


TNL_IMPLEMENT_CLASS(GameType);

GameType::GameType()
{
  gt_name = "Coop";
  gt_version = 0;
  e.game = &game;
}


void GameType::WriteServerInfo(BitStream &s)
{
  consvar_t::SaveNetVars(s);
  fc.WriteNetInfo(s); // file names, sizes and md5 sums
  s.writeString(mapinfo_name.c_str()); // HACK
  // TODO how long it has been running, how long to go,  gamestate, tick, serverplayer?
}

void GameType::ReadServerInfo(BitStream &s)
{
  consvar_t::LoadNetVars(s);

  //void ReadNetInfo(BitStream &s)
  S32 n;
  s.read(&n); // number of files

  bool dl;
  char name[128];
  S32 size;
  byte md5[16];

  for (int i=0; i<n; i++)
    {
      s.read(&dl);  // downloadable?
      s.readString(name);
      s.read(&size);
      s.read(16, md5);
    }

  s.readString(name); // HACK, mapinfo lump name
  mapinfo_name = name;
}


// scope query on server
void GameType::performScopeQuery(GhostConnection *c)
{
  //CONS_Printf("doing scope query\n");

  // first scope the players and their pawns
  for (GameInfo::player_iter_t t = e.game->Players.begin(); t != e.game->Players.end(); t++)
    {
      PlayerInfo *p = t->second;
      bool owner = (p->connection == c); // connection c owns this player

      // TODO set/clear masks so that HUD data is only sent to the client owning the pinfo etc.
      c->objectInScope(p); // player information is always in scope

      // pawns are usually in scope only to their owners
      if ((!cv_hiddenplayers.value || owner)
	  && p->pawn)
        c->objectInScope(p->pawn);
    }

  // TODO in mods, here you could scope bases, flags etc.


  // then actors in PVS, thinkers etc.

  for (GameInfo::player_iter_t t = e.game->Players.begin(); t != e.game->Players.end(); t++)
    {
      PlayerInfo *p = t->second;
      if (p->playerstate == PST_RESPAWN ||
	  p->playerstate == PST_INMAP ||
	  p->playerstate == PST_FINISHEDMAP)
	{
	  // in a Map, can see something
	  Map *m = p->mp;
	  // TODO for now, ignore PVS, scope all stuff in Map
	  // TODO use IterateThinkers?
	  for (Thinker *t = m->thinkercap.next; t != &m->thinkercap; t = t->next)
	    {
	      if (t->IsOf(Actor::_type))
		{
		  Actor *a = reinterpret_cast<Actor *>(t);
		  c->objectInScope(a);
		}
	    }
	}
    }
}



void GameType::Frag(PlayerInfo *killer, PlayerInfo *victim)
{
  killer->Frags[victim->number]++;
  
  // scoring rule
  if (killer->team == 0)
    if (killer != victim)
      killer->score++;
    else
      killer->score--;
  else if (killer->team != victim->team)
    {
      e.game->teams[killer->team]->score++;
      killer->score++;
    }
  else
    {
      e.game->teams[killer->team]->score--;
      killer->score--;
    }
}




/*
  // subclass Map, give Map class virtual methods: Setup...

cv_deathmatch functionality::

kept:
 dm mapobject spawning / dm starts?
 giving ammo
 giving weapon pieces
 heretic weapon PL2 ammo use (damn!)
 chaos and banishment device destinations
 some Hexen monster and weapon properties
 hud/stbar look

removed:
 messagesystem changes
 health item autouse (use baby skill if you want it!)

new consvars:
 leaveplacedweapons

gametype hooks:
 setting consvars when starting game
 spawnplayer (dm keys!)
 finale/intermission behavior (handles also teleport_exitmap)


cv_teamplay::

 teamstarts
 scoring, scoreboards
 intermission
 
*/
