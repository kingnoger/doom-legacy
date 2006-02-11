// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Game startup, main loop.

#ifndef d_main_h
#define d_main_h 1

/// the infinite loop of D_DoomLoop() called from main()
void D_DoomLoop();

// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
void D_DoomMain();

/// Called by IO functions when input is detected.
void D_PostEvent(const struct event_t* ev);
void D_ProcessEvents();

#endif
