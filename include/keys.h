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
// Revision 1.4  2004/09/09 22:04:39  jussip
// New joy code a bit more finished. Button binding works.
//
// Revision 1.3  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.2  2003/12/09 01:02:01  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.1.1.1  2002/11/16 14:18:24  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Key codes
//
//-----------------------------------------------------------------------------


#ifndef keys_h
#define keys_h 1

//
// DOOM keyboard definition.
//

// This is the key event codes as posted by the keyboard handler,
// ascii codes are 0->127,
// scancodes are 0x80 + 0->127

enum key_input_e
{
  KEY_NULL = 0,       // null key, triggers nothing

  KEY_BACKSPACE  = 8,
  KEY_TAB        = 9,
  KEY_ENTER      = 13,
  KEY_PAUSE      = 19,
  KEY_ESCAPE     = 27,
  KEY_SPACE      = 32,

  KEY_MINUS      = 45,
  // numbers
  KEY_EQUALS     = 61,
  KEY_CONSOLE    = 96,
  // small letters
  KEY_DELETE     = 127, // ascii ends here

  // the rest are in arbitrary, almost-scancode-like order
  KEY_SHIFT     = (0x80+54),
  KEY_CTRL      = (0x80+29),
  KEY_ALT       = (0x80+56),

  KEY_NUMLOCK    = (0x80+69),
  KEY_CAPSLOCK   = (0x80+58),
  KEY_SCROLLLOCK = (0x80+70),

  //  scancodes 71-83 (non-extended)
  KEY_KEYPAD7   = (0x80+71),
  KEY_KEYPAD8   = (0x80+72),
  KEY_KEYPAD9   = (0x80+73),
  KEY_MINUSPAD  = (0x80+74),
  KEY_KEYPAD4   = (0x80+75),
  KEY_KEYPAD5   = (0x80+76),
  KEY_KEYPAD6   = (0x80+77),
  KEY_PLUSPAD   = (0x80+78),
  KEY_KEYPAD1   = (0x80+79),
  KEY_KEYPAD2   = (0x80+80),
  KEY_KEYPAD3   = (0x80+81),
  KEY_KEYPAD0   = (0x80+82),
  //KEY_KPADDEL   = (0x80+83),

  //  windows95 keys...
  KEY_LEFTWIN   = (0x80+91),
  KEY_RIGHTWIN  = (0x80+92),
  KEY_MENU      = (0x80+93),

  //  scancodes 71-83 EXTENDED are remapped
  //  to these by the keyboard handler (just add 30)
  KEY_KPADSLASH  = (0x80+100),      //extended scancode 53 '/' remapped
  KEY_HOME       = (0x80+101),
  KEY_UPARROW    = (0x80+102),
  KEY_PGUP       = (0x80+103),
  KEY_LEFTARROW  = (0x80+105),
  KEY_RIGHTARROW = (0x80+107),
  KEY_END        = (0x80+109),
  KEY_DOWNARROW  = (0x80+110),
  KEY_PGDN       = (0x80+111),
  KEY_INS        = (0x80+112),
  KEY_KPADDEL    = (0x80+113),

  KEY_F1        = (0x80+0x3b),
  KEY_F2        = (0x80+0x3c),
  KEY_F3        = (0x80+0x3d),
  KEY_F4        = (0x80+0x3e),
  KEY_F5        = (0x80+0x3f),
  KEY_F6        = (0x80+0x40),
  KEY_F7        = (0x80+0x41),
  KEY_F8        = (0x80+0x42),
  KEY_F9        = (0x80+0x43),
  KEY_F10       = (0x80+0x44),
  KEY_F11       = (0x80+0x57),
  KEY_F12       = (0x80+0x58),

  KEY_NUMKB     = 256, // all real keyboard codes are under this value

  // mouse and joystick buttons are handled as 'virtual' keys
  MOUSEBUTTONS =  8,
  MAXJOYSTICKS = 4,   // "Only" 4 joysticks per machine.
  JOYBUTTONS   = 16,  // Max number of buttons for a joystick.
  JOYHATBUTTONS = 4,  // Four hat directions.

  KEY_MOUSE1          = KEY_NUMKB, // mouse buttons, including the wheel
  KEY_MOUSEWHEELUP    = KEY_MOUSE1 + 3, // usually
  KEY_MOUSEWHEELDOWN,
  KEY_DBLMOUSE1       = KEY_MOUSE1     + MOUSEBUTTONS, // double clicks

  KEY_2MOUSE1         = KEY_DBLMOUSE1  + MOUSEBUTTONS, // second mouse buttons
  KEY_2MOUSEWHEELUP   = KEY_2MOUSE1 + 3,
  KEY_2MOUSEWHEELDOWN,
  KEY_DBL2MOUSE1      = KEY_2MOUSE1    + MOUSEBUTTONS,

  KEY_JOYSTICKSTART   = KEY_DBL2MOUSE1 + MOUSEBUTTONS, // joystick buttons
  KEY_JOY0BUT0,
  KEY_JOY0BUT1,
  KEY_JOY0BUT2,
  KEY_JOY0BUT3,
  KEY_JOY0BUT4,
  KEY_JOY0BUT5,
  KEY_JOY0BUT6,
  KEY_JOY0BUT7,
  KEY_JOY0BUT8,
  KEY_JOY0BUT9,
  KEY_JOY0BUT10,
  KEY_JOY0BUT11,
  KEY_JOY0BUT12,
  KEY_JOY0BUT13,
  KEY_JOY0BUT14,
  KEY_JOY0BUT15,

  KEY_JOY1BUT0,
  KEY_JOY1BUT1,
  KEY_JOY1BUT2,
  KEY_JOY1BUT3,
  KEY_JOY1BUT4,
  KEY_JOY1BUT5,
  KEY_JOY1BUT6,
  KEY_JOY1BUT7,
  KEY_JOY1BUT8,
  KEY_JOY1BUT9,
  KEY_JOY1BUT10,
  KEY_JOY1BUT11,
  KEY_JOY1BUT12,
  KEY_JOY1BUT13,
  KEY_JOY1BUT14,
  KEY_JOY1BUT15,

  KEY_JOY2BUT0,
  KEY_JOY2BUT1,
  KEY_JOY2BUT2,
  KEY_JOY2BUT3,
  KEY_JOY2BUT4,
  KEY_JOY2BUT5,
  KEY_JOY2BUT6,
  KEY_JOY2BUT7,
  KEY_JOY2BUT8,
  KEY_JOY2BUT9,
  KEY_JOY2BUT10,
  KEY_JOY2BUT11,
  KEY_JOY2BUT12,
  KEY_JOY2BUT13,
  KEY_JOY2BUT14,
  KEY_JOY2BUT15,

  KEY_JOY3BUT0,
  KEY_JOY3BUT1,
  KEY_JOY3BUT2,
  KEY_JOY3BUT3,
  KEY_JOY3BUT4,
  KEY_JOY3BUT5,
  KEY_JOY3BUT6,
  KEY_JOY3BUT7,
  KEY_JOY3BUT8,
  KEY_JOY3BUT9,
  KEY_JOY3BUT10,
  KEY_JOY3BUT11,
  KEY_JOY3BUT12,
  KEY_JOY3BUT13,
  KEY_JOY3BUT14,
  KEY_JOY3BUT15,

  KEY_JOYSTICKEND,
  /*
  KEY_DBLJOY1       = KEY_JOY1 + JOYBUTTONS,
  KEY_DBLJOY14      = KEY_DBLJOY1 + JOYBUTTONS - 1,
  */

  // number of total 'button' inputs, includes keyboard keys, plus virtual
  // keys (mousebuttons and joybuttons become keys)
  NUMINPUTS
};


#endif
