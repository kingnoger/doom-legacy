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
// Revision 1.2  2002/12/03 10:23:46  smite-meister
// Video system overhaul
//
// Revision 1.3  2002/07/01 21:00:48  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:24  vberghol
// Version 133 Experimental!
//
// Revision 1.6  2001/08/20 20:40:39  metzgermeister
// *** empty log message ***
//
// Revision 1.5  2001/06/10 21:16:01  bpereira
// no message
//
// Revision 1.4  2001/02/24 13:35:20  bpereira
// no message
//
// Revision 1.3  2000/11/02 19:49:35  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef i_video_h
#define i_video_h 1

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

typedef enum {
  render_soft   = 1,
  render_glide  = 2,
  render_d3d    = 3,
  render_opengl = 4, //Hurdler: the same for render_minigl
  render_none   = 5  // for dedicated server
} rendermode_t;

extern rendermode_t rendermode;

// use highcolor modes if true
//extern bool highcolor;


bool I_StartupGraphics();    //setup video mode
void I_ShutdownGraphics();   //restore old video mode

// Takes full 8 bit values.
void I_SetPalette (RGBA_t* palette);

#ifdef __MACOS__
void macConfigureInput();
void VID_Pause(int pause);
#endif

int   I_NumVideoModes();
char *I_GetVideoModeName(int modenum);
void I_PrepareVideoModeList();

void I_UpdateNoBlit();
void I_FinishUpdate();

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

void I_ReadScreen(byte* scr);

void I_BeginRead();
void I_EndRead();

#ifdef __BIG_ENDIAN__
# define UINT2RGBA(a) a
#else
# define UINT2RGBA(a) ((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|(((ULONG)a&0xff000000)>>24)
#endif

#endif
