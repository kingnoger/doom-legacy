// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief SDL system interface. Everything that does not fit into the other i_ files.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef FREEBSD
# include <SDL.h>
#else
# include <SDL/SDL.h>
#endif

#ifdef LINUX
# ifndef FREEBSD
#  include <sys/vfs.h>
# else
#  include <sys/param.h>
#  include <sys/mount.h>
# endif
#endif

#ifdef __APPLE_CC__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#endif

#ifdef LMOUSE2
#include <termios.h>
#endif

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "d_event.h"
#include "d_ticcmd.h"
#include "d_main.h"
#include "m_misc.h"

#include "i_video.h"
#include "i_sound.h"
#include "i_system.h"

#include "screen.h"
#include "g_game.h"
#include "keys.h"

void ShowEndTxt();
vector<SDL_Joystick*> joysticks;


#ifdef LMOUSE2
static int fdmouse2 = -1;
static bool mouse2_started = false;
#endif

static int lastmousex = 0;
static int lastmousey = 0;



//
//  Translates the SDL key into Doom key
//
static int xlatekey(SDLKey sym)
{
  // leave ASCII codes unchanged
  if (sym >= SDLK_BACKSPACE && sym <= SDLK_DELETE)
    return sym;

  // modifiers
  if (sym >= SDLK_NUMLOCK && sym <= SDLK_COMPOSE)
    return sym - SDLK_NUMLOCK + KEY_NUMLOCK;

  // keypads, function keys
  if (sym >= SDLK_KP0 && sym <= SDLK_F12)
    return sym - SDLK_KP0 + KEY_KEYPAD0;

  // unfortunately most international keys fall here!
  if (sym >= SDLK_WORLD_0 && sym <= SDLK_WORLD_95)
    return KEY_CONSOLE;

  // some individual special keys
  switch (sym)
    {
    case SDLK_MENU:  return KEY_MENU;
    default:
      break;
      // unknown keys
    }

  return KEY_NULL;
}


//! Translates a SDL joystick button to a doom key_input_e number.
static int TranslateJoybutton(Uint8 which, Uint8 button)
{
  int base;

  if(which >= MAXJOYSTICKS) 
    which = MAXJOYSTICKS-1;

  if(button >= JOYBUTTONS)
    button = JOYBUTTONS-1;

  switch(which)
    {
    case 0:  base = KEY_JOY0BUT0; break;
    case 1:  base = KEY_JOY1BUT0; break;
    case 2:  base = KEY_JOY2BUT0; break;
    default: base = KEY_JOY3BUT0; break;
    }

  return base + button;
}


void I_GetEvent()
{
  SDL_Event inputEvent;
  event_t event;    

#ifdef LMOUSE2
  I_GetMouse2Event();
#endif

  //SDL_PumpEvents(); //SDL_PollEvent calls this automatically
  int temp;

  while (SDL_PollEvent(&inputEvent))
    {
      switch (inputEvent.type)
        {
        case SDL_KEYDOWN:
	  event.type = ev_keydown;
	  event.data1 = xlatekey(inputEvent.key.keysym.sym);

	  // TODO actually this belongs in D_PostEvent, but not until we have another interface...
	  temp = inputEvent.key.keysym.mod; // modifier key status
	  shiftdown = (temp & KMOD_SHIFT);
	  altdown = (temp & KMOD_ALT);

	  D_PostEvent(&event);
	  break;
        case SDL_KEYUP:
	  event.type = ev_keyup;
	  event.data1 = xlatekey(inputEvent.key.keysym.sym);
	  D_PostEvent(&event);
	  break;
        case SDL_MOUSEMOTION:
	  if (cv_usemouse[0].value)
            {
	      // If the event is from warping the pointer back to middle
	      // of the screen then ignore it.
	      if ((inputEvent.motion.x == vid.width/2) &&
		  (inputEvent.motion.y == vid.height/2)) 
                {
		  lastmousex = inputEvent.motion.x;
		  lastmousey = inputEvent.motion.y;
		  break;
                } 
	      else 
                {
		  event.data2 = (inputEvent.motion.x - lastmousex) << 2;
		  lastmousex = inputEvent.motion.x;
		  event.data3 = (lastmousey - inputEvent.motion.y) << 2;
		  lastmousey = inputEvent.motion.y;
                }
	      event.type = ev_mouse;
	      event.data1 = 0;
            
	      D_PostEvent(&event);
            
	      // Warp the pointer back to the middle of the window
	      //  or we cannot move any further if it's at a border.
	      if ((inputEvent.motion.x < (vid.width/2)-(vid.width/4)) || 
		  (inputEvent.motion.y < (vid.height/2)-(vid.height/4)) || 
		  (inputEvent.motion.x > (vid.width/2)+(vid.width/4)) || 
		  (inputEvent.motion.y > (vid.height/2)+(vid.height/4)))
                {
		  SDL_WarpMouse(vid.width/2, vid.height/2);
                }
            }
	  break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
	  if (cv_usemouse[0].value)
            {
	      if (inputEvent.type == SDL_MOUSEBUTTONDOWN)
		event.type = ev_keydown;
	      else
		event.type = ev_keyup;
	      event.data1 = KEY_MOUSE1 + inputEvent.button.button - SDL_BUTTON_LEFT;
	      D_PostEvent(&event);
            }
	  break;

	case SDL_JOYBUTTONDOWN : 
	  event.type = ev_keydown;
	  event.data1 = TranslateJoybutton(inputEvent.jbutton.which, 
					   inputEvent.jbutton.button);
	  D_PostEvent(&event);
	  break;

	case SDL_JOYBUTTONUP : 
	  event.type = ev_keyup;
	  event.data1 = TranslateJoybutton(inputEvent.jbutton.which, 
					   inputEvent.jbutton.button);
	  D_PostEvent(&event);
	  break;

        case SDL_QUIT:
	  I_Quit();
	  break;
        default:
	  break;	  
        }
    }
}


void I_GrabMouse()
{
  if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_OFF)
    SDL_WM_GrabInput(SDL_GRAB_ON);
}


void I_UngrabMouse()
{
  if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}


void I_StartupMouse()
{
  SDL_Event inputEvent;
    
  // warp to center 
  SDL_WarpMouse(vid.width/2, vid.height/2);
  lastmousex = vid.width/2;
  lastmousey = vid.height/2;
  // remove the mouse event by reading the queue
  SDL_PollEvent(&inputEvent);

  /*    
  if (cv_usemouse.value) 
    I_GrabMouse();
  else
    I_UngrabMouse();
  */

  return;
}



void I_OutputMsg(char *fmt, ...) 
{
  va_list     argptr;

  va_start (argptr,fmt);
  vfprintf (stdout, fmt, argptr);
  va_end (argptr);
}


int I_GetKey()
{
  // Warning: I_GetKey empties the event queue till next keypress
  event_t*    ev;
  int rc=0;

  // return the first keypress from the event queue
  for ( ; eventtail != eventhead ; eventtail = (++eventtail)&(MAXEVENTS-1) )
    {
      ev = &events[eventtail];
      if(ev->type == ev_keydown)
        {
	  rc = ev->data1;
	  continue;
        }
    }

  return rc;
}



#ifdef LMOUSE2
void I_GetMouse2Event()
{
  static unsigned char mdata[5];
  static int i = 0,om2b = 0;
  int di,j,mlp,button;
  event_t event;
  const int mswap[8] = {0,4,1,5,2,6,3,7};
  if(!mouse2_started) return;
  for(mlp=0;mlp<20;mlp++) {
    for(;i<5;i++) {
      di = read(fdmouse2,mdata+i,1);
      if(di==-1) return;
    }
    if((mdata[0]&0xf8)!=0x80) {
      for(j=1;j<5;j++) {
        if((mdata[j]&0xf8)==0x80) {
          for(i=0;i<5-j;i++) { // shift
            mdata[i] = mdata[i+j];
          }
        }
      }
      if(i<5) continue;
    } else {
      button = mswap[~mdata[0]&0x07];
      for(j=0;j<MOUSEBUTTONS;j++) {
        if(om2b&(1<<j)) {
          if(!(button&(1<<j))) { //keyup
            event.type = ev_keyup;
            event.data1 = KEY_2MOUSE1+j;
            D_PostEvent(&event);
            om2b ^= 1 << j;
          }
        } else {
          if(button&(1<<j)) {
            event.type = ev_keydown;
            event.data1 = KEY_2MOUSE1+j;
            D_PostEvent(&event);
            om2b ^= 1 << j;
          }
        }
      }
      event.data2 = ((signed char)mdata[1])+((signed char)mdata[3]);
      event.data3 = ((signed char)mdata[2])+((signed char)mdata[4]);
      if(event.data2&&event.data3) {
        event.type = ev_mouse2;
        event.data1 = 0;
        D_PostEvent(&event);
      }
    }
    i = 0;
  }
}


void I_ShutdownMouse2()
{
  if (fdmouse2 != -1)
    close(fdmouse2);
  mouse2_started = false;
}
#endif // LMOUSE2


void I_StartupMouse2()
{
#ifdef LMOUSE2
  struct termios m2tio;
  int i,dtr,rts;

  I_ShutdownMouse2();
  if(cv_usemouse2.value == 0) return;
  if((fdmouse2 = open(cv_mouse2port.string,O_RDONLY|O_NONBLOCK|O_NOCTTY))==-1) {
    CONS_Printf("Error opening %s!\n",cv_mouse2port.string);
    return;
  }
  tcflush(fdmouse2, TCIOFLUSH);
  m2tio.c_iflag = IGNBRK;
  m2tio.c_oflag = 0;
  m2tio.c_cflag = CREAD|CLOCAL|HUPCL|CS8|CSTOPB|B1200;
  m2tio.c_lflag = 0;
  m2tio.c_cc[VTIME] = 0;
  m2tio.c_cc[VMIN] = 1;
  tcsetattr(fdmouse2, TCSANOW, &m2tio);
  strupr(cv_mouse2opt.string);
  for(i=0,rts = dtr = -1;i<strlen(cv_mouse2opt.string);i++) {
    if(cv_mouse2opt.string[i]=='D') {
      if(cv_mouse2opt.string[i+1]=='-') {
        dtr = 0;
      } else {
        dtr = 1;
      }
    }
    if(cv_mouse2opt.string[i]=='R') {
      if(cv_mouse2opt.string[i+1]=='-') {
        rts = 0;
      } else {
        rts = 1;
      }
    }
  }
  if((dtr!=-1)||(rts!=-1)) {
    if(!ioctl(fdmouse2, TIOCMGET, &i)) {
      if(!dtr) {
        i &= ~TIOCM_DTR;
      } else {
        if(dtr>0) i |= TIOCM_DTR;
      }
      if(!rts) {
        i &= ~TIOCM_RTS;
      } else {
        if(rts>0) i |= TIOCM_RTS;
      }
      ioctl(fdmouse2, TIOCMSET, &i);
    }
  }
  mouse2_started = true;
#endif
}



ticcmd_t *I_BaseTiccmd()
{
  static ticcmd_t emptycmd;
  return &emptycmd;
}


/// returns time in 1/TICRATE second tics
tic_t I_GetTics()
{
  static Uint32 basetime = SDL_GetTicks(); // executed only on the first call

  // milliseconds since SDL initialization
  Uint32 ms = SDL_GetTicks();
  
  return tic_t(ms - basetime)*TICRATE/1000;
}


/// returns time in ms
unsigned int I_GetTime()
{
  // milliseconds since SDL initialization
  return SDL_GetTicks();
}


/// sleeps for a while, giving CPU time to other processes
void I_Sleep(unsigned int ms)
{
  SDL_Delay(ms);
}


/// initialize SDL
void I_SysInit()
{
  // Initialize Audio as well, otherwise DirectX can not use audio
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
      CONS_Printf("Couldn't initialize SDL: %s\n", SDL_GetError());
      I_Quit();
    }

  char title[30];
  sprintf(title, LEGACY_VERSION_BANNER, LEGACY_VERSION/100, 
	  LEGACY_VERSION%100, LEGACY_SUBVERSION, LEGACY_VERSIONSTRING);

  // Window title
  SDL_WM_SetCaption(title, "Doom Legacy");

  I_StartupGraphics(); // we need a window for grabbing input!
}


/// Initialize joysticks and print information.
void I_JoystickInit()
{
  int numjoysticks, i;
  SDL_Joystick *joy;

  // Joystick subsystem was initialized at the same time as video,
  // because otherwise it won't work. (don't know why, though ...)

  numjoysticks = SDL_NumJoysticks();
  CONS_Printf("%d joystick(s) found.\n", numjoysticks);

  // Start receiving joystick events.
  SDL_JoystickEventState(SDL_ENABLE);

  for(i=0; i<numjoysticks; i++) {
    joy = SDL_JoystickOpen(i);
    joysticks.push_back(joy);
    CONS_Printf("Properties of joystick %d:\n", i);
    CONS_Printf("    %s.\n", SDL_JoystickName(i));
    CONS_Printf("    %d axes.\n", SDL_JoystickNumAxes(joy));
    CONS_Printf("    %d buttons.\n", SDL_JoystickNumButtons(joy));
    CONS_Printf("    %d hats.\n", SDL_JoystickNumHats(joy));
    CONS_Printf("    %d trackballs.\n", SDL_JoystickNumBalls(joy));
  }
}


/// Close all joysticks.
void I_ShutdownJoystick()
{
  int i;

  CONS_Printf("Shutting down joysticks.\n");
  for(i=0; i< (int)joysticks.size(); i++) {
    CONS_Printf("Closing joystick %s.\n", SDL_JoystickName(i));
    SDL_JoystickClose(joysticks[i]);
  }
  joysticks.clear();
  CONS_Printf("Joystick subsystem closed cleanly.\n");
}



#define MAX_QUIT_FUNCS     16
typedef void (*quitfuncptr)();
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
{
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


//  Adds a function to the list that need to be called by I_SystemShutdown().
void I_AddExitFunc(void (*func)())
{
  for (int c = 0; c<MAX_QUIT_FUNCS; c++)
    {
      if (!quit_funcs[c])
	{
	  quit_funcs[c] = func;
	  break;
	}
    }
}


//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
void I_RemoveExitFunc(void (*func)())
{
  for (int c=0; c<MAX_QUIT_FUNCS; c++)
    {
      if (quit_funcs[c] == func)
	{
	  while (c<MAX_QUIT_FUNCS-1)
	    {
	      quit_funcs[c] = quit_funcs[c+1];
	      c++;
	    }
	  quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
	  break;
	}
    }
}


//  Closes down everything. This includes restoring the initial
//  palette and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
void I_ShutdownSystem()
{
  for (int c = MAX_QUIT_FUNCS-1; c>=0; c--)
    if (quit_funcs[c])
      (*quit_funcs[c])();

}



/// quits the game.
void I_Quit()
{
  static bool quitting = false; // prevent recursive I_Quit()

  if (quitting) return;
  quitting = true;

  game.SV_Reset(true);
  I_ShutdownSound();
  I_ShutdownCD();

  M_SaveConfig(NULL);
  I_ShutdownJoystick();
  I_ShutdownGraphics();
  I_ShutdownSystem();
  printf("\r");
#ifndef __APPLE_CC__
  // on Mac OS X it's pointless
  ShowEndTxt();
#endif
  exit(0);
}



void I_Error(const char *error, ...)
{
  static bool recursive = false;

  if (recursive)
    {
      fprintf(stderr, "Error: I_Error called recursively!\n");
      return;
    }

  recursive = true;

  // Message first.
  va_list argptr;
  va_start(argptr,error);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, error, argptr);
  fprintf(stderr, "\n");
  va_end(argptr);
  fflush(stderr);

  // Shutdown. Here might be other errors.
  //game.net->QuitNetGame();

  I_ShutdownSound();
  I_ShutdownGraphics();
  I_ShutdownJoystick();
  // shutdown everything else which was registered
  I_ShutdownSystem();

  exit(-1);
}



char *I_GetUserName()
{
  static char username[MAXPLAYERNAME];
  char  *p;

#ifdef __WIN32__
  DWORD i = MAXPLAYERNAME;

  int ret = GetUserName(username, &i);
  if (!ret)
    {
#endif
  if (!(p = getenv("USER")))
    if (!(p = getenv("user")))
      if (!(p = getenv("USERNAME")))
	if (!(p = getenv("username")))
	  return NULL;
  strncpy(username, p, MAXPLAYERNAME);
#ifdef __WIN32__
    }
#endif

  if (strcmp(username, "") == 0)
    return NULL;
  return username;
}



int I_mkdir(const char *dirname, int unixright)
{
#ifdef __WIN32__
  return mkdir(dirname);
#else
  return mkdir(dirname, unixright);
#endif
}


/// returns the path to the default wadfile location (usually the current working directory)
char *I_GetWadPath()
{
  static char temp[256];

  // get the current directory (possible problem on NT with "." as current dir)
  if (getcwd(temp, 255))
    {
#warning FIX it Later!

	  /*
#if defined (__MACOS__) || defined(__APPLE_CC__)
      // cwd is always "/" when app is dbl-clicked
      if (!strcmp(temp, "/"))
	return I_GetWadDir();
#endif
	   */

      return temp;
    }
  else
    return ".";
}



#ifdef LINUX
# define MEMINFO_FILE "/proc/meminfo"
# define MEMTOTAL "MemTotal:"
# define MEMFREE "MemFree:"
#endif


Uint32 I_GetFreeMem(Uint32 *total)
{
#ifdef LINUX
  char buf[1024];    
  char *memTag;
  Uint32 freeKBytes;
  Uint32 totalKBytes;
  int n;
  int meminfo_fd = -1;

  meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
  n = read(meminfo_fd, buf, 1023);
  close(meminfo_fd);
    
  if(n<0)
    {
      // Error
      *total = 0L;
      return 0;
    }
    
  buf[n] = '\0';
  if(NULL == (memTag = strstr(buf, MEMTOTAL)))
    {
      // Error
      *total = 0L;
      return 0;
    }
        
  memTag += sizeof(MEMTOTAL);
  totalKBytes = atoi(memTag);
    
  if(NULL == (memTag = strstr(buf, MEMFREE)))
    {
      // Error
      *total = 0L;
      return 0;
    }
        
  memTag += sizeof(MEMFREE);
  freeKBytes = atoi(memTag);
    
  *total = totalKBytes << 10;
  return freeKBytes << 10;
    
#else
  *total = 16<<20;
  return 16<<20;
#endif
}



void I_GetDiskFreeSpace(Sint64 *freespace)
{
#if defined (LINUX) || defined (__MACOS__) || defined(__APPLE_CC__)
  struct statfs stfs;
  if (statfs(".", &stfs) == -1)
    {
      *freespace = MAXINT;
      return;
    }
  *freespace = stfs.f_bavail*stfs.f_bsize;
#endif

#ifdef __WIN32__
  typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes); 
  static MyFunc pfnGetDiskFreeSpaceEx=NULL;
  static bool testwin95 = false;

  INT64 usedbytes;

  if (!testwin95)
    {
      HINSTANCE h = LoadLibraryA("kernel32.dll");

      if (h)
	{
	  pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h, "GetDiskFreeSpaceExA");
	  FreeLibrary(h);
	}
      testwin95 = true;
    }

  if (pfnGetDiskFreeSpaceEx)
    {
      if (!pfnGetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)freespace, (PULARGE_INTEGER)&usedbytes, NULL))
	*freespace = MAXINT;
    }
  else
    {
      DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;

      GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
		       &NumberOfFreeClusters, &TotalNumberOfClusters);
      *freespace = BytesPerSector*SectorsPerCluster*NumberOfFreeClusters;
    }

#endif

#if !defined (LINUX) && !defined (__WIN32__)
  // Dummy for platform independent; 1GB should be enough
  *freespace = 1024*1024*1024;
#endif
}
