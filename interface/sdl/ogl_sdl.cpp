// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.1  2002/11/16 14:18:32  hurdler
// Initial revision
//
// Revision 1.4  2002/08/22 14:44:06  jpakkane
// Fix missing cast.
//
// Revision 1.3  2002/07/01 21:01:04  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:35  vberghol
// Version 133 Experimental!
//
// Revision 1.6  2001/06/25 20:08:06  bock
// Fix bug (BSD?) with color depth > 16 bpp
//
// Revision 1.5  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.4  2001/03/09 21:53:56  metzgermeister
// *** empty log message ***
//
// Revision 1.3  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.2  2000/09/10 10:56:01  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
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

#include <GL/gl.h>
#include <GL/glu.h>

#include "hardware/r_opengl/r_opengl.h"
#include "v_video.h"


#ifdef DEBUG_TO_FILE
HANDLE logstream = (HANDLE)((void *)-1);
#endif

static SDL_Surface *vidSurface = NULL; //use the one from i_video_sdl.c instead?
int     oglflags = 0;

void HWR_Startup(void);

bool OglSdlSurface(int w, int h, int isFullscreen)
{
    Uint32 surfaceFlags;
    int cbpp;

    if(NULL != vidSurface)
    {
        SDL_FreeSurface(vidSurface);
        vidSurface = NULL;
#ifdef VOODOOSAFESWITCHING
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
    }

    if(isFullscreen)
    {
        surfaceFlags = SDL_OPENGL|SDL_FULLSCREEN;
    }
    else
    {
        surfaceFlags = SDL_OPENGL;
    }

    /*
     * We want at least 1 bit R, G, and B,
     * and at least 16 bpp. Why 1 bit? May be more?
     */
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    cbpp = SDL_VideoModeOK(w, h, 16, surfaceFlags);
    if (cbpp < 16)
        return false;
    if((vidSurface = SDL_SetVideoMode(w, h, cbpp, surfaceFlags)) == NULL)
        return false;


    SetModelView(w, h);
    SetStates();
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    HWR_Startup();
    textureformatGL = (cbpp > 16)?GL_RGBA:GL_RGB5_A1;

    return true;
}

void OglSdlFinishUpdate(bool vidwait)
{
    SDL_GL_SwapBuffers();
}

void OglSdlShutdown(void)
{
    if(NULL != vidSurface)
    {
        SDL_FreeSurface(vidSurface);
        vidSurface = NULL;
    }
}
void OglSdlSetPalette(RGBA_t *palette, RGBA_t *gamma)
{
    int i;

    for (i=0; i<256; i++) {
        myPaletteData[i].s.red   = MIN((palette[i].s.red   * gamma->s.red)  /127, 255);
        myPaletteData[i].s.green = MIN((palette[i].s.green * gamma->s.green)/127, 255);
        myPaletteData[i].s.blue  = MIN((palette[i].s.blue  * gamma->s.blue) /127, 255);
        myPaletteData[i].s.alpha = palette[i].s.alpha;
    }
    // on a chang� de palette, il faut recharger toutes les textures
    // jaja, und noch viel mehr ;-)
    Flush();
}
