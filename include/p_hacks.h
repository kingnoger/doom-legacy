// Emacs style mode select   -*- C++ -*-
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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Re-implementation of some original Doom "features"

#ifndef p_hacks_h
#define p_hacks_h 1

#include "g_pawn.h"

/// \brief Voodoo dolls
///
/// Originally a bug in the player spawning code. When there are several player_x
/// playerstarts in a map, the PlayerPawn is spawned to the first (FIXME last?) one and the rest
/// spawn Voodoo dolls attached to the real pawn.
/// Whatever happens to the dolls, happens to the pawn.
 
class VoodooDoll : public PlayerPawn
{
  PlayerPawn *victim;

public:
  VoodooDoll(const PlayerPawn &p);
  virtual ~VoodooDoll();

  virtual void CheckPointers();
  virtual void XYFriction(fixed_t oldx, fixed_t oldy, bool oldfriction);
  virtual void Think();


  virtual bool Touch(Actor *a);
  virtual bool Damage(Actor *inflictor, Actor *source, int damage, int dtype = dt_normal);

  static void Spawn(PlayerInfo *p, struct mapthing_t *mthing);
};


#endif
