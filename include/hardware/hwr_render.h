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
// Revision 1.2  2004/05/02 21:15:56  hurdler
// add dummy new renderer (bis)
//
// Revision 1.1  2004/05/01 23:29:19  hurdler
// add dummy new renderer
//
//-----------------------------------------------------------------------------

#ifndef hwr_render_h
#define hwr_render_h 1

#include "../r_render.h"

/**
  \brief Handles the new hardware rendering code.
  Note: This is early development and all that can completely change in the future.
        It's here only until the new renderer will be at least as good as the old one.
*/
class HWRend
{
public:
  /// Do all the rendering depending on the player state.
  /// That means player view and player sprites but not the console and the menu.
  void RenderPlayerView(int viewnumber, PlayerInfo *player);

  /// Prepare hardware related structures for the current map.
  void Setup(int bspnum);

  /// Intialize OpenGL (windows, states,...).
  void Startup(int width, int height, int bpp);
};

extern HWRend HWR;

#endif
