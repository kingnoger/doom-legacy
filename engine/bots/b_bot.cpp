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
// Revision 1.1  2004/10/17 01:57:05  smite-meister
// bots!
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Client-side bots

#include "doomdef.h"
#include "command.h"

#include "g_game.h"

#include "acbot.h"

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



// add bots to game
void Command_AddBot_f()
{
  extern bool server;

  if (!server)
    {
      CONS_Printf("Only the server can add a bot\n");
      return;
    }

  // TODO syntax: "addbot [bottype] [team] [parameters]..."
  // default: ACBot
  int i = P_Random() % NUMBOTNAMES;
  PlayerInfo *p = new ACBot(botnames[i], game.skill);
  
  if (!game.AddPlayer(p)) 
    {
      CONS_Printf("Cannot add any more players.\n");
      delete p;
      return; 
    }
  else
    consoleplayer2 = p; // FIXME TEST
}
