// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2005 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Client-side bots

#include "doomdef.h"
#include "command.h"

#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_mapinfo.h"

#include "acbot.h"
#include "b_path.h"

#include "m_random.h"

// to be read from an XML config-file...
static const char *botnames[] =
{
  "Frag-God",
  "Thresh",
  "Reptile",
  "Archer",
  "Freak",
  "TF-Master",
  "Carmack",
  "Romero",
  "Quaker",
  "FragMaster",
  "Punisher",
  "Xoleras",
  "Hurdlerbot",
  "Meisterbot",
  "Borisbot",
  "Tailsbot",
  "TonyD-bot",
  "Azathoth",
  "yo momma",
  "crusher",
  "aimbot",
  "crash",
  "akira",
  "meiko",
  "undead",
  "death",
  "unit",
  "fodder",
  "2-vile",
  "nitemare",
  "nos482",
  "billy"
};

const int NUMBOTNAMES = sizeof(botnames)/sizeof(char *);

unsigned num_bots = 0; // TEMP

BotAI::BotAI()
{
  subject = NULL;
  pawn = NULL;
  mp = NULL;
  cmd = NULL;
};


/*
  find the dlls in the bots directory
  open them, store the bot types into a vector (if the dll version is ok)
  addbot <type> x y z... : find <type> in the vector, re-load the dll if necessary,
    exchange interfaces, create the bot: dll->interf->CreateBot(...).
    The dll can get the parameters x,y,z... directly from the command buffer.
 */



// add bots to game
void Command_AddBot_f()
{
  // TODO addbot command

  // TODO syntax: "addbot [bottype] [team] [parameters]..."
  // default: ACBot
  int n, i;

  for (i=0; i<NUM_LOCALBOTS; i++)
    if (LocalPlayers[NUM_LOCALHUMANS + i].ai == NULL)
      break;

  if (i == NUM_LOCALBOTS)
    {
      CONS_Printf("Only %d bots per client.\n", NUM_LOCALBOTS);
      return;
    }

  LocalPlayerInfo *p = &LocalPlayers[NUM_LOCALHUMANS + i];

  //n = COM_Argc(); TODO read params

  p->name = botnames[P_Random() % NUMBOTNAMES];
  p->ai = new ACBot(game.skill);

  Map *m = com_player ? com_player->mp : NULL;
  if (!m)
    return;
  //p->requestmap = m->info->mapnumber;

  if (!m->botnodes)
    m->botnodes = new BotNodes(m);

  if (game.server)
    {
      p->info = game.AddPlayer(new PlayerInfo(p));
      if (!p->info) 
	{
	  CONS_Printf("Cannot add any more players.\n");
	  return; 
	}
    }
  // TODO otherwise ask server to add a new player...
}
