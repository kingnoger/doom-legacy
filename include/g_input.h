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
// Revision 1.4  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.3  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.2  2003/12/09 01:02:01  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//-----------------------------------------------------------------------------

/// \file
/// Handles mouse/keyboard/joystick inputs,
/// maps inputs to game controls (forward,use,open...)

#ifndef g_input_h
#define g_input_h 1

#include "keys.h"

enum gamecontrols_e
{
  gc_null = 0,        //a key/button mapped to gc_null has no effect
  gc_forward,
  gc_backward,
  gc_strafe,
  gc_straferight,
  gc_strafeleft,
  gc_speed,
  gc_turnleft,
  gc_turnright,
  gc_fire,
  gc_use,
  gc_lookup,
  gc_lookdown,
  gc_centerview,
  gc_mouseaiming,     // mouse aiming is momentary (toggleable in the menu)
  gc_weapon1,
  gc_weapon2,
  gc_weapon3,
  gc_weapon4,
  gc_weapon5,
  gc_weapon6,
  gc_weapon7,
  gc_weapon8,
  gc_talkkey,
  gc_scores,
  gc_jump,
  gc_console,
  gc_nextweapon,
  gc_prevweapon,
  gc_bestweapon,
  gc_invnext,
  gc_invprev,
  gc_invuse,
  gc_flydown,     // flyup is jump !
  num_gamecontrols
};

// current state of the keys : true if pushed
extern  byte    gamekeydown[NUMINPUTS];

// two key codes (or virtual key) per game control
extern  int     gamecontrol[num_gamecontrols][2];
extern  int     gamecontrol2[num_gamecontrols][2];    // secondary splitscreen player

// peace to my little coder fingers!
// check a gamecontrol being active or not

// remaps the input event to a game control.
void  G_MapEventsToControls(struct event_t *ev);

// returns the name of a key
char* G_KeynumToString(int keynum);
int   G_KeyStringtoNum(char *keystr);

// detach any keys associated to the given game control
void  G_ClearControlKeys(int (*setupcontrols)[2], int control);
void  Command_Setcontrol_f();
void  Command_Setcontrol2_f();
void  G_Controldefault();
void  G_CheckDoubleUsage(int keynum);


#endif
