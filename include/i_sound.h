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
// Revision 1.3  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.2  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      System interface, sound.
//
//-----------------------------------------------------------------------------


#ifndef i_sound_h
#define i_sound_h 1

#include "doomdef.h"

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

class channel_t;

void I_SetSfxVolume(int volume);

// Starts a sound in a particular sound channel, returns a handle
int I_StartSound(channel_t *c);

// Stops a sound channel.
void I_StopSound(channel_t *c);

// Returns false if no longer playing, true if playing.
//bool I_SoundIsPlaying(int handle);

// Updates the volume, separation,
//  and pitch of a sound channel.
// returns false if sound is already stopped / not found
//bool I_UpdateSoundParams(int handle, int vol, int sep, int pitch);

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
