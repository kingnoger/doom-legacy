// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.5  2003/12/09 01:02:02  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.4  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.3  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.2  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.1.1.1  2002/11/16 14:18:31  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   System interface. Everything that does not fit into the other i_ files.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
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

#ifdef LMOUSE2
#include <termios.h>
#endif

#ifdef LJOYSTICK // linux joystick 1.x
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/joystick.h>
#endif

#include "doomdef.h"
#include "d_event.h"
#include "d_main.h"
#include "m_misc.h"

#include "i_video.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_joy.h"

#include "screen.h"
#include "d_net.h"
#include "g_game.h"
#include "d_clisrv.h"
#include "keys.h"

#include "sdl/endtxt.h"


#ifdef LJOYSTICK
static int joyfd = -1;
static int joyaxes = 0;
static int joystick_started = 0;
static int joy_scale = 1;
#endif
JoyType_t Joystick;

#ifdef LMOUSE2
static int fdmouse2 = -1;
static bool mouse2_started = false;
#endif

static int lastmousex = 0;
static int lastmousey = 0;


void I_StartupKeyboard () {}
void I_StartupTimer    () {}

//
//  Translates the SDL key into Doom key
//
static int xlatekey(SDLKey sym)
{
  int rc = KEY_NULL;

  switch (sym)
    {
    case SDLK_LEFT:  rc = KEY_LEFTARROW;     break;
    case SDLK_RIGHT: rc = KEY_RIGHTARROW;    break;
    case SDLK_DOWN:  rc = KEY_DOWNARROW;     break;
    case SDLK_UP:    rc = KEY_UPARROW;       break;

    case SDLK_ESCAPE:   rc = KEY_ESCAPE;        break;
    case SDLK_RETURN:   rc = KEY_ENTER;         break;
    case SDLK_TAB:      rc = KEY_TAB;           break;
    case SDLK_F1:       rc = KEY_F1;            break;
    case SDLK_F2:       rc = KEY_F2;            break;
    case SDLK_F3:       rc = KEY_F3;            break;
    case SDLK_F4:       rc = KEY_F4;            break;
    case SDLK_F5:       rc = KEY_F5;            break;
    case SDLK_F6:       rc = KEY_F6;            break;
    case SDLK_F7:       rc = KEY_F7;            break;
    case SDLK_F8:       rc = KEY_F8;            break;
    case SDLK_F9:       rc = KEY_F9;            break;
    case SDLK_F10:      rc = KEY_F10;           break;
    case SDLK_F11:      rc = KEY_F11;           break;
    case SDLK_F12:      rc = KEY_F12;           break;

    case SDLK_BACKSPACE: rc = KEY_BACKSPACE;    break;
    case SDLK_PAUSE:     rc = KEY_PAUSE;        break;

    case SDLK_EQUALS:
    case SDLK_PLUS:      rc = KEY_EQUALS;       break;

    case SDLK_MINUS:     rc = KEY_MINUS;        break;

    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      rc = KEY_SHIFT;
      break;
          
      //case SDLK_XK_Caps_Lock:
      //rc = KEY_CAPSLOCK;
      //break;

    case SDLK_LCTRL:
    case SDLK_RCTRL:
      rc = KEY_CTRL;
      break;
          
    case SDLK_LALT:
    case SDLK_RALT:
      rc = KEY_ALT;
      break;
        
    case SDLK_PAGEUP:   rc = KEY_PGUP; break;
    case SDLK_PAGEDOWN: rc = KEY_PGDN; break;
    case SDLK_END:      rc = KEY_END;  break;
    case SDLK_HOME:     rc = KEY_HOME; break;
    case SDLK_INSERT:   rc = KEY_INS;  break;
      
    case SDLK_KP0: rc = KEY_KEYPAD0;  break;
    case SDLK_KP1: rc = KEY_KEYPAD1;  break;
    case SDLK_KP2: rc = KEY_KEYPAD2;  break;
    case SDLK_KP3: rc = KEY_KEYPAD3;  break;
    case SDLK_KP4: rc = KEY_KEYPAD4;  break;
    case SDLK_KP5: rc = KEY_KEYPAD5;  break;
    case SDLK_KP6: rc = KEY_KEYPAD6;  break;
    case SDLK_KP7: rc = KEY_KEYPAD7;  break;
    case SDLK_KP8: rc = KEY_KEYPAD8;  break;
    case SDLK_KP9: rc = KEY_KEYPAD9;  break;

    case SDLK_KP_MINUS:  rc = KEY_KPADDEL;  break;
    case SDLK_KP_DIVIDE: rc = KEY_KPADSLASH; break;
    case SDLK_KP_ENTER:  rc = KEY_ENTER;    break;

      // TODO add support for more special keys
    default:
      // most ascii chars
      if (sym >= SDLK_SPACE && sym <= SDLK_DELETE)
	rc = sym - SDLK_SPACE + KEY_SPACE;
      break;
    }
    
  return rc;
}


void I_GetEvent()
{
  SDL_Event inputEvent;
  event_t event;    

#ifdef LJOYSTICK
  I_GetJoyEvent();
#endif
#ifdef LMOUSE2
  I_GetMouse2Event();
#endif

  //SDL_PumpEvents(); //SDL_PollEvent calls this automatically
    
  while(SDL_PollEvent(&inputEvent))
    {
      switch(inputEvent.type)
        {
        case SDL_KEYDOWN:
	  event.type = ev_keydown;
	  event.data1 = xlatekey(inputEvent.key.keysym.sym);
	  D_PostEvent(&event);
	  break;
        case SDL_KEYUP:
	  event.type = ev_keyup;
	  event.data1 = xlatekey(inputEvent.key.keysym.sym);
	  D_PostEvent(&event);
	  break;
        case SDL_MOUSEMOTION:
	  if(cv_usemouse.value)
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
	  if (cv_usemouse.value)
            {
	      if (inputEvent.type == SDL_MOUSEBUTTONDOWN)
		event.type = ev_keydown;
	      else
		event.type = ev_keyup;
	      event.data1 = KEY_MOUSE1 + inputEvent.button.button - SDL_BUTTON_LEFT;
	      D_PostEvent(&event);
            }
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
    
#ifdef HAS_SDL_BEEN_FIXED // FIXME
  if (cv_usemouse.value) 
    I_GrabMouse();
  else
    I_UngrabMouse();
#endif
  return;
}

//
//I_OutputMsg
//
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


//
// I_Tactile
//
void I_Tactile(int on, int off, int total)
{
  // UNUSED.
  on = off = total = 0;
}


void I_InitJoystick()
{
#ifdef LJOYSTICK
  I_ShutdownJoystick();
  if(!strcmp(cv_usejoystick.string,"0"))
    return;
  if(!joy_open(cv_joyport.string)) return;
  joystick_started = 1;
  return;
#endif
}


#ifdef LJOYSTICK
void I_JoyScale()
{
  joy_scale = (cv_joyscale.value==0)?1:cv_joyscale.value;
}


void I_GetJoyEvent()
{
  struct js_event jdata;
  static event_t event = {0,0,0,0};
  static int buttons = 0;
  if (!joystick_started)
    return;
  while (read(joyfd,&jdata,sizeof(jdata))!=-1)
    {
      switch (jdata.type)
	{
	case JS_EVENT_AXIS:
	  event.type = ev_joystick;
	  event.data1 = 0;
	  switch (jdata.number)
	    {
	    case 0:
	      event.data2 = ((jdata.value >> 5)/joy_scale)*joy_scale;
	      D_PostEvent(&event);
	      break;
	    case 1:
	      event.data3 = ((jdata.value >> 5)/joy_scale)*joy_scale;
	      D_PostEvent(&event);
	    default:
	      break;
	    }
	  break;

	case JS_EVENT_BUTTON:
	  if (jdata.number<JOYBUTTONS)
	    {
	      if (jdata.value)
		{
		  if (!((buttons >> jdata.number)&1))
		    {
		      buttons |= 1 << jdata.number;
		      event.type = ev_keydown;
		      event.data1 = KEY_JOY1+jdata.number;
		      D_PostEvent(&event);
		    }
		}
	      else
		{
		  if ((buttons>>jdata.number)&1)
		    {
		      buttons ^= 1 << jdata.number;
		      event.type = ev_keyup;
		      event.data1 = KEY_JOY1+jdata.number;
		      D_PostEvent(&event);
		    }
		}
	    }
	  break;
	}
    }
}


void I_ShutdownJoystick()
{
  if (joyfd != -1)
    {
      close(joyfd);
      joyfd = -1;
    }
  joyaxes = 0;
  joystick_started = 0;
}


int joy_open(char *fname)
{
  joyfd = open(fname,O_RDONLY|O_NONBLOCK);
  if (joyfd == -1)
    {
      CONS_Printf("Error opening %s!\n",fname);
      return 0;
    }
  ioctl(joyfd,JSIOCGAXES,&joyaxes);
  if (joyaxes<2)
    {
      CONS_Printf("Not enought axes?\n");
      joyaxes = 0;
      joyfd = -1;
      close(joyfd);
      return 0;
    }
  return joyaxes;
}
/*int joy_waitb(int fd, int *xpos,int *ypos,int *hxpos,int *hypos) {
  int i,xps,yps,hxps,hyps;
  struct js_event jdata;
  for(i=0;i<1000;i++) {
    while(read(fd,&jdata,sizeof(jdata))!=-1) {
      switch(jdata.type) {
      case JS_EVENT_AXIS:
        switch(jdata.number) {
        case 0: // x
          xps = jdata.value;
          break;
        case 1: // y
          yps = jdata.value;
          break;
        case 3: // hat x
          hxps = jdata.value;
          break;
        case 4: // hat y
          hyps = jdata.value;
        default:
          break;
        }
        break;
      case JS_EVENT_BUTTON:
        break;
      }
    }
  }
  }*/
#endif // LJOYSTICK

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

//
// I_GetTime
// returns time in 1/TICRATE second tics
//
tic_t I_GetTime()
{
  Uint32        ticks;
  static Uint32 basetime=0;

  // milliseconds since SDL initialization
  ticks = SDL_GetTicks();
  
  if (basetime == 0) // first call
    basetime = ticks;
  
  return (tic_t)(ticks - basetime)*TICRATE/1000;
}



//
// I_Init
//
void I_SysInit()
{
  I_StartupTimer(); // does nothing in SDL
}

//
// I_Quit
//
void I_Quit()
{
  static bool quitting = false; // prevent recursive I_Quit()

  if (quitting) return;
  quitting = true;

  //added:16-02-98: when recording a demo, should exit using 'q' key,
  //        but sometimes we forget and use 'F10'.. so save here too.
  if (demorecording)
    game.CheckDemoStatus();
  D_QuitNetGame ();
  I_ShutdownMusic();
  I_ShutdownSound();
  I_ShutdownCD();
  // use this for 1.28 19990220 by Kin
  M_SaveConfig (NULL);
  I_ShutdownGraphics();
  I_ShutdownSystem();
  printf("\r");
  ShowEndTxt();
  exit(0);
}

void I_WaitVBL(int count)
{
  SDL_Delay(1);
}

void I_BeginRead()
{
}

void I_EndRead()
{
}

byte* I_AllocLow(int length)
{
  byte*       mem;

  mem = (byte *)malloc (length);
  memset (mem,0,length);
  return mem;
}


//
// I_Error
//
extern bool demorecording;

void I_Error(char *error, ...)
{
  va_list     argptr;
  static bool recursive = false;

  if (recursive)
    {
      fprintf(stderr, "Error: I_Error called recursively!\n");
      return;
    }

  recursive = true;

  // Message first.
  va_start(argptr,error);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, error, argptr);
  fprintf(stderr, "\n");
  va_end(argptr);

  fflush(stderr);

  // Shutdown. Here might be other errors.
  if (demorecording)
    game.CheckDemoStatus();

  D_QuitNetGame ();
  I_ShutdownMusic();
  I_ShutdownSound();
  I_ShutdownGraphics();
  // shutdown everything else which was registered
  I_ShutdownSystem();

  exit(-1);
}

#define MAX_QUIT_FUNCS     16
typedef void (*quitfuncptr)();
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
{
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
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


//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
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

//
//  Closes down everything. This includes restoring the initial
//  palette and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
//
void I_ShutdownSystem()
{
  for (int c = MAX_QUIT_FUNCS-1; c>=0; c--)
    if (quit_funcs[c])
      (*quit_funcs[c])();

}

void I_GetDiskFreeSpace(long long *freespace)
{
#ifdef LINUX
  struct statfs stfs;
  if (statfs(".",&stfs)==-1)
    {
      *freespace = MAXINT;
      return;
    }
  *freespace = stfs.f_bavail*stfs.f_bsize;
#endif

#ifdef __WIN32__
  // VB: added this definition from win32/win_sys.c
  typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes); 
  static MyFunc pfnGetDiskFreeSpaceEx=NULL;
  static bool testwin95 = false;

  INT64 usedbytes;

  if(!testwin95)
    {
      HINSTANCE h = LoadLibraryA("kernel32.dll");

      if (h) {
	pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h,"GetDiskFreeSpaceExA");
	FreeLibrary(h);
      }
      testwin95 = true;
    }
  if (pfnGetDiskFreeSpaceEx) {
    if (!pfnGetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)freespace,(PULARGE_INTEGER)&usedbytes,NULL))
      *freespace = MAXINT;
  }
  else
    {
      ULONG SectorsPerCluster, BytesPerSector, NumberOfFreeClusters;
      ULONG TotalNumberOfClusters;
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

char *I_GetUserName()
{
#ifdef LINUX

  static char username[MAXPLAYERNAME];
  char  *p;
  if((p=getenv("USER"))==NULL)
    if((p=getenv("user"))==NULL)
      if((p=getenv("USERNAME"))==NULL)
	if((p=getenv("username"))==NULL)
	  return NULL;
  strncpy(username,p,MAXPLAYERNAME);
  if( strcmp(username,"")==0 )
    return NULL;
  return username;

#endif

#ifdef __WIN32__

  static char username[MAXPLAYERNAME];
  char  *p;
  int   ret;
  ULONG i=MAXPLAYERNAME;

  ret = GetUserName(username,&i);
  if(!ret)
    {
      if((p=getenv("USER"))==NULL)
	if((p=getenv("user"))==NULL)
	  if((p=getenv("USERNAME"))==NULL)
	    if((p=getenv("username"))==NULL)
	      return NULL;
      strncpy(username,p,MAXPLAYERNAME);
    }
  if( strcmp(username,"")==0 )
    return NULL;
  return username;

#endif

#if !defined (LINUX) && !defined (__WIN32__)

  // dummy for platform independent version
  return NULL;
#endif
}

int I_mkdir(const char *dirname, int unixright)
{
#ifdef __WIN32__
  return mkdir(dirname);
#else
  return mkdir(dirname, unixright);
#endif
}


void I_LocateWad()
{
  // relict from the Linux version
  return;
}

#ifdef LINUX
# define MEMINFO_FILE "/proc/meminfo"
# define MEMTOTAL "MemTotal:"
# define MEMFREE "MemFree:"
#endif

// quick fix for compil
ULONG I_GetFreeMem(ULONG *total)
{
#ifdef LINUX
  char buf[1024];    
  char *memTag;
  ULONG freeKBytes;
  ULONG totalKBytes;
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
