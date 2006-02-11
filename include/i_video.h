// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Video system interface


#ifndef i_video_h
#define i_video_h 1

#include<GL/gl.h>

#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

// The new OpenGL renderer.
class OGLRenderer;
extern OGLRenderer *oglrenderer; // If this is not NULL, we are using OpenGL.
extern GLuint missingtexture;

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
/* void macConfigureInput(); */
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
