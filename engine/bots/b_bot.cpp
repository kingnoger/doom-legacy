// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004 by DooM Legacy Team.
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
// $Log$
// Revision 1.4  2005/04/17 17:59:00  smite-meister
// netcode
//
// Revision 1.3  2004/11/09 20:38:51  smite-meister
// added packing to I/O structs
//
// Revision 1.2  2004/10/27 17:37:08  smite-meister
// netcode update
//
// Revision 1.1  2004/10/17 01:57:05  smite-meister
// bots!
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Client-side bots

#include "doomdef.h"
#include "command.h"

#include "g_game.h"
#include "g_map.h"
#include "g_mapinfo.h"

#include "acbot.h"
#include "b_path.h"

#include "m_random.h"

// to be read from an XML config-file...
static char *botnames[] =
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
  /* TODO addbot command
  if (!game.server)
    {
      CONS_Printf("Only the server can add a bot\n");
      return;
    }

  // TODO syntax: "addbot [bottype] [team] [parameters]..."
  // default: ACBot

  int n = COM_Argc();
  int i = P_Random() % NUMBOTNAMES;
  PlayerInfo *p = new ACBot(botnames[i], game.skill);

  Map *m = com_player ? com_player->mp : NULL;

  p->requestmap = m->info->mapnumber;

  if (!m->botnodes)
    m->botnodes = new BotNodes(m);

  if (!game.AddPlayer(p)) 
    {
      CONS_Printf("Cannot add any more players.\n");
      delete p;
      return; 
    }
  else
    Consoleplayer.push_back(p); // FIXME TEST
  */
}
