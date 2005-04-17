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
// $Log$
// Revision 1.3  2005/04/17 17:59:00  smite-meister
// netcode
//
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


/// \brief ABC for PlayerInfo AI (a client-side bot)
class BotAI
{
protected:
  class  PlayerInfo *subject; ///< the player the AI is controlling
  class  PlayerPawn *pawn;    ///< shorthand for subject->pawn
  class  Map        *mp;      ///< shorthand for subject->mp
  struct ticcmd_t   *cmd;     ///< shorthand for *subject->cmd

public:
  BotAI();

  virtual void BuildInput(PlayerInfo *p, int elapsed_tics) = 0;
};


extern unsigned num_bots;

void Command_AddBot();

#endif
