// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
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
// $Log$
// Revision 1.1  2003/05/11 21:27:26  smite-meister
// Team system
//
//
//
// DESCRIPTION:
//    Teams.
//
//-----------------------------------------------------------------------------

#ifndef g_team_h
#define g_team_h 1

#include <string>

using namespace std;

// Every single pawn in the game belongs to a team.
// Some teams are always fixed, others can be created and destroyed.
// Team 0 is always the "individual" team.
//   Fellow teammates are targeted and attacked if the Actor AI so demands.
//   In a DM game all players belong to team 0.
// Team -1 is the "Hellspawn" team, in which all Doom creatures belong by default.
// Team -2 is the "D'Sparil" team, in which all Heretic creatures belong by default.
// Team -3 is the "Korax" team, in which all Hexen creatures belong by default.

// This means that if you put Hellspawn and Heretic monsters in the same room, they will
// attack each other unless they are specifically assigned to same team.

// Team score is only kept for teams > 0

class TeamInfo
{
  friend class GameInfo;
public:
  string name;
  int color;
  int score;
  // team flag/symbol ?
};

#endif
