// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief Main program, simply calls D_DoomMain and the main game loop D_DoomLoop.

// in m_argv.h
extern  int     myargc;
extern  char**  myargv;

void D_DoomLoop();
bool D_DoomMain();

int main(int argc, char **argv)
{ 
  myargc = argc; 
  myargv = argv; 
 
  if (D_DoomMain())
    D_DoomLoop();

  return 0;
} 
