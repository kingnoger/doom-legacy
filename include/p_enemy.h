// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.4  2003/11/27 11:28:26  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.3  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
//
//
// DESCRIPTION:
//   Common function prototypes for monster AI
//
//-----------------------------------------------------------------------------


#ifndef p_enemy_h
#define p_enemy_h 1

#include "info.h" // mobjtype_t

#define HITDICE(a) ((1+(P_Random() & 7))*(a))

#define USERANGE        (64*FRACUNIT)
#define MELEERANGE      (64*FRACUNIT)
#define MISSILERANGE    (32*64*FRACUNIT)

#define FLOATSPEED      (FRACUNIT*4)

typedef enum
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

} dirtype_t;

void   P_NoiseAlert(class Actor *target, Actor *emitter);
void   P_BulletSlope(class PlayerPawn *p);

void   A_FaceTarget(class DActor *actor);
void   A_Chase(DActor *actor);
void   A_Fall(DActor *actor);
void   A_Look(DActor *actor);
void   A_Pain(DActor *actor);

#endif
