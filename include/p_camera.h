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
// Revision 1.2  2002/12/16 22:04:59  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:29  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.4  2002/08/19 18:06:41  vberghol
// renderer somewhat fixed
//
// Revision 1.3  2002/08/13 19:47:45  vberghol
// p_inter.cpp done
//
// Revision 1.2  2002/08/11 17:16:51  vberghol
// ...
//
// Revision 1.1  2002/07/23 19:21:45  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.7  2002/07/18 19:16:41  vberghol
// renamed a few files
//
// Revision 1.6  2002/07/10 19:57:03  vberghol
// g_pawn.cpp tehty
//
// Revision 1.5  2002/07/08 20:46:35  vberghol
// More files compile!
//
// Revision 1.4  2002/07/04 18:02:27  vberghol
// Pientä fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.3  2002/07/01 21:00:52  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:26  vberghol
// Version 133 Experimental!
//
//
//
// DESCRIPTION:
//    All kinds of cameras, chasecam.
//
//-----------------------------------------------------------------------------


#ifndef p_camera_h
#define p_camera_h

#include "m_fixed.h"
#include "g_actor.h"

struct consvar_t;

class Camera : public Actor
{
private:

  //SoM: Things used by FS cameras.
  fixed_t  viewheight;
  angle_t  startangle;

public:
  int  fixedcolormap;
  bool chase;

  void ClearCamera();
  void ResetCamera(Actor *p);
  void MoveChaseCamera(Actor *p);
};

extern Camera camera;

extern consvar_t cv_cam_dist;  
extern consvar_t cv_cam_height;
extern consvar_t cv_cam_speed;

// default viewheight is changeable at console
extern consvar_t cv_viewheight;

#endif
