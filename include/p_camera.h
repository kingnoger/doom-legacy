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
// Revision 1.3  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.2  2002/12/16 22:04:59  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:29  hurdler
// Initial C++ version of Doom Legacy
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

  Camera();

  void ClearCamera();
  void ResetCamera(Actor *p);

  virtual void Think();
};

extern Camera camera;

extern consvar_t cv_cam_dist;  
extern consvar_t cv_cam_height;
extern consvar_t cv_cam_speed;

// default viewheight is changeable at console
extern consvar_t cv_viewheight;

#endif
