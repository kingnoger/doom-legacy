// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2005 by DooM Legacy Team.
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
/// \brief Chasecam

#ifndef p_camera_h
#define p_camera_h

#include "m_fixed.h"
#include "g_actor.h"

/// \brief Chasecam, FS cameras.
class Camera : public Actor
{
  DECLARE_CLASS(Camera);
public:
  int  fixedcolormap;
  bool chase;

  Camera(struct mapthing_t *mt);

  void ClearCamera();
  void ResetCamera(Actor *p);

  virtual void Think();
};


#endif
