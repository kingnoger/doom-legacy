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
// $Log$
// Revision 1.7  2004/09/06 22:28:53  jussip
// Beginnings of new joystick code.
//
// Revision 1.6  2004/08/18 14:35:20  smite-meister
// PNG support!
//
// Revision 1.5  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.4  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.3  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.2  2002/12/03 10:23:46  smite-meister
// Video system overhaul
//
//-----------------------------------------------------------------------------

/// \file
/// \brief System specific interface stuff.

#ifndef i_system_h
#define i_system_h 1

#include "d_ticcmd.h"


// See Shutdown_xxx() routines.
//extern byte graphics_started;
extern byte keyboard_started;
extern byte sound_started;

#ifdef PC_DOS
/* flag for 'win-friendly' mode used by interface code */
extern int i_love_bill;
#endif

extern volatile tic_t ticcount;

#ifdef WIN32_DIRECTX
extern boolean winnt;
extern BOOL   bDX0300;
#endif

// basic system initialization
void I_SysInit();

// Joystick init and cleanup.
void I_JoystickInit();
void I_ShutdownJoystick();

// return free and total physical memory in the system
Uint32 I_GetFreeMem(Uint32 *total);


/// returns current time in 1/TICRATE second tics
tic_t I_GetTics();

/// returns current time in ms
unsigned int I_GetTime();

/// sleeps for a given amount of ms (accurate to about 10 ms)
void I_Sleep(unsigned int ms);


void I_GetEvent ();


//
// Called by D_DoomLoop,
// called before processing any tics in a frame
// (just after displaying a frame).
// Time consuming syncronous operations
// are performed here (joystick reading).
// Can call D_PostEvent.
//
void I_StartFrame ();


//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_OsPolling ();

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t* I_BaseTiccmd ();


// Called by M_Responder when quit is selected, return code 0.
void I_Quit ();

void I_Error (char *error, ...);

// Allocates from low memory under dos,
// just mallocs under unix
byte* I_AllocLow (int length);

void I_Tactile (int on, int off, int total);

//added:18-02-98: write a message to stderr (use before I_Quit)
//                for when you need to quit with a msg, but need
//                the return code 0 of I_Quit();
void I_OutputMsg (char *error, ...);

void I_InitJoystick();
void I_StartupMouse();
void I_StartupMouse2();

// setup timer irq and user timer routine.
void I_TimerISR ();      //timer callback routine.
void I_StartupTimer ();

/* list of functions to call at program cleanup */
void I_AddExitFunc (void (*func)());
void I_RemoveExitFunc (void (*func)());

// Setup signal handler, plus stuff for trapping errors and cleanly exit.
int  I_StartupSystem ();
void I_ShutdownSystem ();

void I_GetDiskFreeSpace(INT64 *freespace);
char *I_GetUserName();
int  I_mkdir(const char *dirname, int unixright);


/// returns the path to the default wadfile location (usually the current working directory)
char *I_GetWadPath();


#endif
