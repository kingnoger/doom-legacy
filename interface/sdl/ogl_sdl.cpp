// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.2  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.1.1.1  2002/11/16 14:18:32  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      SDL specific part of the OpenGL API for Doom Legacy
//
//-----------------------------------------------------------------------------

#ifdef FREEBSD
# include <SDL.h>
#else
# include <SDL/SDL.h>
#endif

#include "v_video.h"
#include "hardware/hw_drv.h"

static SDL_Surface *vidSurface = NULL; //use the one from i_video_sdl.c instead?

void HWR_Startup();

bool OglSdlSurface(int w, int h, int isFullscreen)
{
  Uint32 surfaceFlags;

  if (NULL != vidSurface)
    {
      SDL_FreeSurface(vidSurface);
      vidSurface = NULL;
#ifdef VOODOOSAFESWITCHING
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
      SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
    }

  if (isFullscreen)
    surfaceFlags = SDL_OPENGL|SDL_FULLSCREEN;
  else
    surfaceFlags = SDL_OPENGL;

  // We want at least 1 bit R, G, and B,
  // and at least 16 bpp. Why 1 bit? May be more?
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

  int cbpp = SDL_VideoModeOK(w, h, 16, surfaceFlags);
  if (cbpp < 16)
    return false;
  if ((vidSurface = SDL_SetVideoMode(w, h, cbpp, surfaceFlags)) == NULL)
    return false;

  HWD.pfnInitVidMode(w, h, cbpp);

  HWR_Startup();

  return true;
}

void OglSdlFinishUpdate(bool vidwait)
{
  SDL_GL_SwapBuffers();
}

void OglSdlShutdown()
{
  if (NULL != vidSurface)
    {
      SDL_FreeSurface(vidSurface);
      vidSurface = NULL;
    }
}

/*
void OglSdlSetPalette(RGBA_t *palette, RGBA_t *gamma)
{
    int i;

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((palette[i].s.red   * gamma->s.red)  /127, 255);
        myPaletteData[i].s.green = MIN((palette[i].s.green * gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((palette[i].s.blue  * gamma->s.blue) /127, 255);
        myPaletteData[i].s.alpha = palette[i].s.alpha;
    }
    // on a changé de palette, il faut recharger toutes les textures
    // jaja, und noch viel mehr ;-)
    Flush();
}
*/
