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
// Revision 1.2  2004/10/27 17:37:08  smite-meister
// netcode update
//
// Revision 1.1  2004/10/17 01:57:05  smite-meister
// bots!
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Bot players

#ifndef b_bot_h
#define b_bot_h 1

#include "m_fixed.h"
#include "g_player.h"


/// \brief ABC for AI-controlled playerpawns (bots!)
class BotPlayer : public PlayerInfo
{
public:
  BotPlayer(const string &n) : PlayerInfo(n) {};

  virtual void GetInput(int lpnum, int elapsed) = 0;
  virtual void SetMessage(const char *msg, int priority = 0, int type = M_CONSOLE) {}
};


void Command_AddBot();

#endif
