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
// Revision 1.10  2004/01/10 16:03:00  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.9  2003/12/09 01:02:02  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.8  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.7  2003/04/24 20:30:27  hurdler
// Remove lots of compiling warnings
//
// Revision 1.6  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.5  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.4  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/16 22:14:50  smite-meister
// Video unit fix
//
// Revision 1.2  2002/12/03 10:24:47  smite-meister
// Video system overhaul
//
//
// DESCRIPTION:
//   SDL video interface
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>

#ifdef FREEBSD
# include <SDL.h>
#else
# include <SDL/SDL.h>
#endif

#include "doomdef.h"
#include "command.h"

#include "i_system.h"
#include "i_video.h"

#include "screen.h"
#include "m_argv.h"

#include "m_dll.h"
#include "hardware/r_opengl/r_opengl.h"
#include "sdl/ogl_sdl.h"

void I_UngrabMouse();
void I_GrabMouse();


#ifdef DYNAMIC_LINKAGE
static dll_handle_t ogl_handle = NULL;
static const dll_info_t *ogl_info = NULL;
#endif

// SDL vars
static const SDL_VideoInfo *vidInfo = NULL;
static SDL_Rect     **modeList = NULL;
static SDL_Surface   *vidSurface = NULL;
static SDL_Color      localPalette[256];

//const static Uint32       surfaceFlags = SDL_HWSURFACE|SDL_HWPALETTE|SDL_DOUBLEBUF;
// FIXME : VB: since fullscreen HW surfaces wont't work, we'll use a SW surface.
const static Uint32 surfaceFlags = SDL_SWSURFACE | SDL_HWPALETTE;

static int numVidModes = 0;
static char vidModeName[33][32]; // allow 33 different modes


// maximum number of windowed modes (see windowedModes[][])
#define MAXWINMODES (6) 

//Hudler: 16/10/99: added for OpenGL gamma correction
RGBA_t  gamma_correction = {0x7F7F7F7F};
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;

extern consvar_t cv_fullscreen; // for fullscreen support 

extern consvar_t cv_usemouse;

rendermode_t    rendermode = render_soft;

// synchronize page flipping with screen refresh
// unused and for compatibility reason 
consvar_t       cv_vidwait = {"vid_wait","1",CV_SAVE,CV_OnOff};

bool graphics_started = false; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in I_PrepareVideoModeList
bool allow_fullscreen = false;


// first entry in the modelist which is not bigger than 1024x768
static int firstEntry = 0;


// windowed video modes from which to choose from.
static int windowedModes[MAXWINMODES][2] =
{
  {MAXVIDWIDTH /*1024*/, MAXVIDHEIGHT/*768*/},
  {800, 600},
  {640, 480},
  {512, 384},
  {400, 300},
  {320, 200}
};


//
// I_StartFrame
//
void I_StartFrame()
{
  if (render_soft == rendermode)
    {
      if (SDL_MUSTLOCK(vidSurface))
        {
	  if (SDL_LockSurface(vidSurface) < 0)
	    return;
        }
    }

  return;
}

//
// I_OsPolling
//
void I_OsPolling()
{
  if (!graphics_started)
    return;

  I_GetEvent();
    
  return;
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit()
{
  /* this function intentionally left empty */
}

//
// I_FinishUpdate
//
void I_FinishUpdate()
{
  if (rendermode == render_soft)
    {
      if (vid.screens[0] != vid.direct)
	{
	  memcpy(vid.direct, vid.screens[0], vid.height*vid.rowbytes);
	  //vid.screens[0] = vid.direct; //FIXME: we MUST render directly into the surface
	}
      //SDL_Flip(vidSurface);
      SDL_UpdateRect(vidSurface, 0, 0, 0, 0);
      if (SDL_MUSTLOCK(vidSurface))
	SDL_UnlockSurface(vidSurface);
    }
  else
    OglSdlFinishUpdate(cv_vidwait.value);
    
  I_GetEvent();
    
  return;
}


//
// I_ReadScreen
//
void I_ReadScreen(byte* scr)
{
  if (rendermode != render_soft)
    I_Error ("I_ReadScreen: called while in non-software mode");
    
  memcpy (scr, vid.screens[0], vid.height*vid.rowbytes);
}



//
// I_SetPalette
//
void I_SetPalette(RGBA_t* palette)
{
  for (int i=0; i<256; i++)
    {
      localPalette[i].r = palette[i].s.red;
      localPalette[i].g = palette[i].s.green;
      localPalette[i].b = palette[i].s.blue;
    }

  SDL_SetColors(vidSurface, localPalette, 0, 256);
}


// return number of fullscreen or windowed modes
int I_NumVideoModes() 
{
  if (cv_fullscreen.value) 
    return numVidModes - firstEntry;
  else
    return MAXWINMODES;
}

char *I_GetVideoModeName(int modeNum) 
{
  if (cv_fullscreen.value)
    {
      modeNum += firstEntry;
      if (modeNum >= numVidModes)
	return NULL;
        
      sprintf(&vidModeName[modeNum][0], "%dx%d",
	      modeList[modeNum]->w,
	      modeList[modeNum]->h);
    }
  else
    { // windowed modes
      if (modeNum > MAXWINMODES)
	return NULL;
        
      sprintf(&vidModeName[modeNum][0], "win %dx%d",
	      windowedModes[modeNum][0],
	      windowedModes[modeNum][1]);
    }
  return &vidModeName[modeNum][0];
}

int I_GetVideoModeForSize(int w, int h)
{
  int matchMode = -1;
  int i;
  
  if (cv_fullscreen.value)
    {
      for (i = firstEntry; i<numVidModes; i++)
	{
	  if (modeList[i]->w == w && modeList[i]->h == h)
	    {
	      matchMode = i;
	      break;
	    }
	}

      if (matchMode == -1) // use smallest mode
	matchMode = numVidModes-1;

      matchMode -= firstEntry;
    }
  else
    {
      for(i = 0; i<MAXWINMODES; i++)
	{
	  if (windowedModes[i][0] == w && windowedModes[i][1] == h)
	    {
	      matchMode = i;
	      break;
	    }
	}

      if (matchMode == -1) // use smallest mode
	  matchMode = MAXWINMODES-1;
    }
  
  return matchMode;
}


void I_PrepareVideoModeList()
{
  int i;
    
  if (cv_fullscreen.value) // only fullscreen needs preparation
    {
      if(numVidModes != -1) 
        {
	  for(i=0; i<numVidModes; i++)
            {
	      if(modeList[i]->w <= MAXVIDWIDTH &&
		 modeList[i]->h <= MAXVIDHEIGHT)
                {
		  firstEntry = i;
		  break;
                }
            }
        }
    }
    
  allow_fullscreen = true;
  return;
}

int I_SetVideoMode(int modeNum)
{
  I_UngrabMouse();

  Uint32 flags;

  vid.modenum = modeNum;
  // For some reason, under Win98 the combination SDL_HWSURFACE | SDL_FULLSCREEN
  // doesn't give a valid pixels pointer. Odd.
  if (cv_fullscreen.value)
    {
      modeNum += firstEntry;        
      vid.width = modeList[modeNum]->w;
      vid.height = modeList[modeNum]->h;
      flags = surfaceFlags | SDL_FULLSCREEN;

      CONS_Printf ("I_SetVideoMode: fullscreen %d x %d (%d bpp)\n", vid.width, vid.height, vid.BitsPerPixel);
    }
  else
    { // !cv_fullscreen.value
      vid.width = windowedModes[modeNum][0];
      vid.height = windowedModes[modeNum][1];
      flags = surfaceFlags;

      CONS_Printf("I_SetVideoMode: windowed %d x %d (%d bpp)\n", vid.width, vid.height, vid.BitsPerPixel);
        
      // Window title
      SDL_WM_SetCaption("Legacy", "Legacy");
    }

  if (rendermode == render_soft)
    {
      SDL_FreeSurface(vidSurface);
	
      vidSurface = SDL_SetVideoMode(vid.width, vid.height, vid.BitsPerPixel, flags);
      if (vidSurface == NULL)
	I_Error("Could not set vidmode\n");
	              
      if (vidSurface->pixels == NULL)
	I_Error("Didn't get a valid pixels pointer (SDL). Exiting.\n");

      vid.direct = (byte *)vidSurface->pixels;
      // VB: FIXME this stops execution at the latest
      *((Uint8 *)vidSurface->pixels) = 1;
    }
  else
    {
      if (!OglSdlSurface(vid.width, vid.height, cv_fullscreen.value))
	I_Error("Could not set vidmode\n");
    }
    
  I_StartupMouse();

  return 1;
}

bool I_StartupGraphics()
{
  if (graphics_started)
    return true;
  
  CV_RegisterVar(&cv_vidwait);
  
  // Initialize Audio as well, otherwise DirectX can not use audio
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
    {
      CONS_Printf("Couldn't initialize SDL: %s\n", SDL_GetError());
      return false;
    }

  // Get video info for screen resolutions  
  vidInfo = SDL_GetVideoInfo();
  // now we _could_ do all kinds of cool tests to determine which
  // video modes are available, but...
  // vidInfo->vfmt is the pixelformat of the "best" video mode available

  //CONS_Printf("Bpp = %d, bpp = %d\n", vidInfo->vfmt->BytesPerPixel, vidInfo->vfmt->BitsPerPixel);

  // list all available video modes corresponding to the "best" pixelformat
  modeList = SDL_ListModes(NULL, SDL_FULLSCREEN | surfaceFlags);

  numVidModes = 0;
  if (modeList == NULL)
    {
      CONS_Printf("No video modes present\n");
      return false;
    }

  while (modeList[numVidModes])
    numVidModes++;
    
  CONS_Printf("Found %d video modes\n", numVidModes);
  
  //for(k=0; modeList[k]; ++k)
  //  CONS_Printf("  %d x %d\n", modeList[k]->w, modeList[k]->h);

  // even if I set vid.bpp and highscreen properly it does seem to
  // support only 8 bit  ...  strange
  // so lets force 8 bit (software mode only)
  // TODO why not use hicolor in sw mode too? it must work...
  // Set color depth; either 1=256pseudocolor or 2=hicolor
  vid.BytesPerPixel = 1;  //videoInfo->vfmt->BytesPerPixel
  vid.BitsPerPixel = 8;
  //highcolor = (vid.bpp == 2) ? true:false;

  // default resolution
  vid.width = BASEVIDWIDTH;
  vid.height = BASEVIDHEIGHT;

  // Window title
  SDL_WM_SetCaption("Legacy", "Legacy");
  
  if (M_CheckParm("-opengl")) 
    {
      rendermode = render_opengl;

#ifdef DYNAMIC_LINKAGE
      // dynamic linkage
      ogl_handle = OpenDLL("r_opengl.dll");
      if (!ogl_handle)
	I_Error("Could not load r_opengl.dll!\n");

      ogl_info = (dll_info_t *)GetSymbol(ogl_handle, "dll_info");
      if (ogl_info->interface_version != R_OPENGL_INTERFACE_VERSION)
	I_Error("r_opengl.dll interface version does not match with Legacy.exe!\n"
		"You must use the r_opengl.dll that came in the same distribution as your Legacy.exe.");

      hw_renderer_export_t *temp = (hw_renderer_export_t *)GetSymbol(ogl_handle, "r_export");
      memcpy(&HWD, temp, sizeof(hw_renderer_export_t));
      CONS_Printf("%s loaded.\n", ogl_info->dll_name);
#else
      // static linkage
      memcpy(&HWD, &r_export, sizeof(hw_renderer_export_t));
#endif

      vid.width = 640; // hack to make voodoo cards work in 640x480
      vid.height = 480;

      if (!OglSdlSurface(vid.width, vid.height, cv_fullscreen.value))
	rendermode = render_soft;
    }
    
  if (rendermode == render_soft)
    {
      // not fullscreen
      CONS_Printf("I_StartupGraphics: windowed %d x %d x %d bpp\n", vid.width, vid.height, vid.BitsPerPixel);
      vidSurface = SDL_SetVideoMode(vid.width, vid.height, vid.BitsPerPixel, surfaceFlags);
      
      if (vidSurface == NULL)
        {
	  CONS_Printf("Could not set vidmode\n");
	  return false;
        }
      vid.direct = (byte *)vidSurface->pixels;
    }
    
  SDL_ShowCursor(SDL_DISABLE);
  I_UngrabMouse();
  
  graphics_started = true;
    
  return true;
}

void I_ShutdownGraphics()
{
  // was graphics initialized anyway?
  if (!graphics_started)
    return;

  if (rendermode == render_soft)
    {
      // vidSurface should be automatically freed
    }
  else
    {
      OglSdlShutdown();

#ifdef DYNAMIC_LINKAGE
      if (ogl_handle)
	CloseDLL(ogl_handle);
#endif
    }
  SDL_Quit(); 
}
