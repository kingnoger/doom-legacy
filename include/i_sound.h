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
// Revision 1.4  2002/09/20 22:41:34  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.3  2002/07/01 21:00:48  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:24  vberghol
// Version 133 Experimental!
//
// Revision 1.7  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.6  2001/02/24 13:35:20  bpereira
// no message
//
// Revision 1.5  2000/09/10 10:42:33  metzgermeister
// fixed qmus2mid SDL
//
// Revision 1.4  2000/04/19 15:21:02  hurdler
// add SDL midi support
//
// Revision 1.3  2000/03/22 18:49:38  metzgermeister
// added I_PauseCD() for Linux
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      System interface, sound.
//
//-----------------------------------------------------------------------------


#ifndef i_sound_h
#define i_sound_h 1

#include "doomdef.h"
#include "sounds.h"
#include "command.h"


// Init at program start...
void I_StartupSound();
// ... shut down and relase at program termination.
void I_ShutdownSound();

// ... update sound buffer and audio device at runtime...
void I_UpdateSound();
void I_SubmitSound();


//
//  SFX I/O
//

// Sound caching. Allows sound interface to decide
// in which format the sounds are cached.
void* I_GetSfx(sfxinfo_t*  sfx);
void  I_FreeSfx(sfxinfo_t* sfx);

void I_SetSfxVolume(int volume);

// Starts a sound in a particular sound channel, returns a handle
int I_StartSound(sfxinfo_t *sfx, int vol, int sep, int pitch);

// Stops a sound channel.
void I_StopSound(int handle);

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns false if no longer playing, true if playing.
bool I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//  and pitch of a sound channel.
// returns false if sound is already stopped / not found
bool I_UpdateSoundParams(int handle, int vol, int sep, int pitch);

//
//  MUSIC I/O
//

void I_InitMusic();
void I_ShutdownMusic();

void I_SetMusicVolume(int volume);

// PAUSE game handling.
void I_PauseSong(int handle);
void I_ResumeSong(int handle);

// Registers a song handle to song data.
#ifdef __MACOS__
int I_RegisterSong(int song);
#else
int I_RegisterSong(void* data, int len);
#endif

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int handle, int looping);

// Stops a song over 3 seconds.
void I_StopSong(int handle);

// See above (register), then think backwards
void I_UnRegisterSong(int handle);

#ifdef __MACOS__
void MusicEvents ();        //needed to give quicktime some processor
# define PLAYLIST_LENGTH 10
extern consvar_t user_songs[PLAYLIST_LENGTH];
#endif


// i_cdmus.h : cd music interface
//
extern byte    cdaudio_started;

void   I_InitCD ();
void   I_StopCD ();
void   I_PauseCD ();
void   I_ResumeCD ();
void   I_ShutdownCD ();
void   I_UpdateCD ();
void   I_PlayCD (int track, bool looping);
int    I_SetVolumeCD (int volume);  // return 0 on failure

#endif
