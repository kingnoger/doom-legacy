// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
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
// Revision 1.6  2004/10/27 17:37:09  smite-meister
// netcode update
//
// Revision 1.5  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.4  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.3  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.2  2002/12/16 22:04:59  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:29  hurdler
// Initial C++ version of Doom Legacy
//-----------------------------------------------------------------------------

/// \file
/// \brief Chasecam

#ifndef p_camera_h
#define p_camera_h

#include "m_fixed.h"
#include "g_actor.h"

/// \brief Chasecam, FS cameras.
class Camera : public Actor
{
public:

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


#endif
