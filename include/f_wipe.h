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
// Revision 1.1  2002/11/16 14:18:23  hurdler
// Initial revision
//
// Revision 1.3  2002/07/01 21:00:45  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:23  vberghol
// Version 133 Experimental!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Mission start screen wipe/melt, special effects.
//      
//-----------------------------------------------------------------------------


#ifndef f_wipe_h
#define f_wipe_h 1

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------

enum
{
  // simple gradual pixel change for 8-bit only
  wipe_ColorXForm,

  // weird screen melt
  wipe_Melt,

  wipe_NUMWIPES
};


int wipe_StartScreen
( int           x,
  int           y,
  int           width,
  int           height );


int wipe_EndScreen
( int           x,
  int           y,
  int           width,
  int           height );


int wipe_ScreenWipe
( int           wipeno,
  int           x,
  int           y,
  int           width,
  int           height,
  int           ticks );

#endif
