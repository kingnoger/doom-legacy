// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.2  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.1.1.1  2002/11/16 14:18:21  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.9  2002/09/08 14:38:08  vberghol
// Now it works! Sorta.
//
// Revision 1.8  2002/08/27 11:51:47  vberghol
// Menu rewritten
//
// Revision 1.7  2002/08/26 07:38:36  vberghol
// menu fixed again
//
// Revision 1.6  2002/08/25 18:22:00  vberghol
// little fixes
//
// Revision 1.5  2002/08/16 20:49:26  vberghol
// engine ALMOST done!
//
// Revision 1.4  2002/07/01 21:00:42  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:56  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.3  2001/03/03 06:17:33  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Input event system

#ifndef d_event_h
#define d_event_h 1

// Input event types.
enum evtype_t
{
  ev_keydown,
  ev_keyup,
  ev_mouse,
  ev_joystick,
  ev_mouse2
};

// Event structure.
struct event_t
{
  evtype_t    type;
  int         data1;          // keys / mouse/joystick buttons
  int         data2;          // mouse/joystick x move
  int         data3;          // mouse/joystick y move
};


//
// GLOBAL VARIABLES
//
#define MAXEVENTS               64

extern  event_t         events[MAXEVENTS];
extern  int             eventhead;
extern  int             eventtail;


// current modifier key status
extern bool shiftdown;
extern bool altdown;


// finale
bool F_Responder (event_t* ev);

// cheat
bool cht_Responder (event_t* ev);

#endif
