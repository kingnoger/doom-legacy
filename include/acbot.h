// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
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
/// \brief ACBot class

#ifndef acbot_h
#define acbot_h 1

#include <list>
#include "m_fixed.h"
#include "b_bot.h"


/// \brief ACBot by tonyd, changes by rellik and smite-meister.
class ACBot : public BotPlayer
{
protected:
  int skill; ///< skill level of the bot

  byte num_weapons; // used to check if got a new weapon

  bool straferight;

  // timers
  int avoidtimer;
  int blockedcount; // if blocked by something like a barrel, will reverse and try to get around it
  int strafetimer;
  int weaponchangetimer;

  class Actor *lastTarget; // last moving target, either enemy or teammate
  fixed_t lastTargetX, lastTargetY; // where the last target was last seen

  list<struct SearchNode_t *> path; //path to the best item on the map
  SearchNode_t *destination;   //the closest node to where wants to go 

public:
  ACBot(const string &name, int skill);

  void BuildTiccmd();
  void ClearPath();

  void AvoidMissile(const Actor *missile);
  void ChangeWeapon();
  void TurnTowardsPoint(fixed_t x, fixed_t y);
  void AimWeapon();

  void LookForThings();
  bool LookForSpecialLine(fixed_t *x, fixed_t *y);

  bool QuickReachable(Actor *a);
  bool QuickReachable(fixed_t x, fixed_t y);
};

#endif
