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
// Revision 1.1  2002/11/16 14:18:25  hurdler
// Initial revision
//
// Revision 1.9  2002/08/19 18:06:42  vberghol
// renderer somewhat fixed
//
// Revision 1.8  2002/08/11 17:16:52  vberghol
// ...
//
// Revision 1.7  2002/08/08 12:01:32  vberghol
// pian engine on valmis!
//
// Revision 1.6  2002/08/06 13:14:29  vberghol
// ...
//
// Revision 1.5  2002/07/18 19:16:41  vberghol
// renamed a few files
//
// Revision 1.4  2002/07/10 19:57:03  vberghol
// g_pawn.cpp tehty
//
// Revision 1.3  2002/07/01 21:00:52  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:27  vberghol
// Version 133 Experimental!
//
// Revision 1.3  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//  Sprite animation.
//
//-----------------------------------------------------------------------------


#ifndef p_pspr_h
#define p_pspr_h 1

// Basic data types.
// Needs fixed point, and BAM angles.
#include "m_fixed.h"
#include "tables.h"
#include "info.h"


#ifdef __GNUG__
#pragma interface
#endif



//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
typedef enum
{
  ps_weapon,
  ps_flash,
  NUMPSPRITES

} psprnum_t;

struct pspdef_t
{
  state_t  *state;  // a NULL state means not active
  int       tics;
  fixed_t   sx;
  fixed_t   sy;
};

#endif
