// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
// Revision 1.7  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.6  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.5  2003/12/03 10:49:50  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.4  2003/11/27 11:28:26  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.3  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Common function prototypes for monster AI

#ifndef p_enemy_h
#define p_enemy_h 1

#include "m_fixed.h"


#define HITDICE(a) ((1+(P_Random() & 7))*(a))

const fixed_t MELEERANGE   = 64;
const fixed_t AIMRANGE     = 16*64;
const fixed_t MISSILERANGE = 32*64;

const float   FLOATSPEED = 4;

enum dirtype_t
{
  DI_EAST,
  DI_NORTHEAST,
  DI_NORTH,
  DI_NORTHWEST,
  DI_WEST,
  DI_SOUTHWEST,
  DI_SOUTH,
  DI_SOUTHEAST,
  DI_NODIR,
  NUMDIRS
};

void    P_NoiseAlert(class Actor *target, Actor *emitter);
fixed_t P_BulletSlope(class PlayerPawn *p);

void   A_FaceTarget(class DActor *actor);
void   A_Chase(DActor *actor);
void   A_Fall(DActor *actor);
void   A_Look(DActor *actor);
void   A_Pain(DActor *actor);

#endif
