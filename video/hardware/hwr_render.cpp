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
// Revision 1.1  2004/05/01 23:29:19  hurdler
// add dummy new renderer
//
//
//
// DESCRIPTION:
//      new hardware renderer, using the standard HardWareRender driver DLL for Doom Legacy
//
//-----------------------------------------------------------------------------

#include "command.h"
#include "hardware/hwr_render.h"

// to be removed once the new renderer is working
#include "hardware/hw_light.h"
#include "hardware/hw_main.h"

consvar_t cv_grnewrenderer = {"gr_newrenderer", "Off", CV_SAVE, CV_OnOff };

void HWRend::RenderPlayerView(int viewnumber, PlayerInfo *player)
{
  R.HWR_RenderPlayerView(viewnumber, player);
}

void HWRend::Setup(int bspnum)
{
  // BP: reset light between levels (we draw preview frame lights on current frame)
  HWR_ResetLights();
  // Correct missing sidedefs & deep water trick
  R.HWR_CorrectSWTricks();
  //CONS_Printf("\n xxx seg(%d) v1 = %d, line(%d) v1 = %d\n", 1578, segs[1578].v1 - vertexes, segs[1578].linedef - lines, segs[1578].linedef->v1 - vertexes);
  R.HWR_CreatePlanePolygons(bspnum-1); // FIXME BUG this messes up the polyobjs
  //CONS_Printf(" xxx seg(%d) v1 = %d, line(%d) v1 = %d\n", 1578, segs[1578].v1 - vertexes, segs[1578].linedef - lines, segs[1578].linedef->v1 - vertexes);
  HWR_PrepLevelCache();
  R.HWR_CreateStaticLightmaps(bspnum-1);
}
