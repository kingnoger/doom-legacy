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
// Revision 1.2  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.1.1.1  2002/11/16 14:18:27  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      The not so system specific sound interface.
//
//-----------------------------------------------------------------------------


#ifndef s_sound_h
#define s_sound_h 1

#include <vector>
#include "m_fixed.h"

using namespace std;
// killough 4/25/98: mask used to indicate sound origin is player item pickup
//#define PICKUP_SOUND (0x8000)

struct consvar_t;
struct CV_PossibleValue_t;
class Actor;
struct mappoint_t;
struct sfxinfo_t;

extern consvar_t stereoreverse;
extern consvar_t cv_soundvolume;
extern consvar_t cv_musicvolume;
extern consvar_t cv_numChannels;

// FIXME move these to linux_x interface
/*
#ifdef SNDSERV
 extern consvar_t sndserver_cmd;
 extern consvar_t sndserver_arg;
#endif
#ifdef MUSSERV
 extern consvar_t musserver_cmd;
 extern consvar_t musserver_arg;
#endif

#ifdef LINUX_X
extern consvar_t cv_jigglecdvol;
#endif
*/

extern CV_PossibleValue_t soundvolume_cons_t[];
//part of i_cdmus.c
extern consvar_t cd_volume;
extern consvar_t cdUpdate;


#ifdef __MACOS__
typedef enum
{
    music_normal,
    playlist_random,
    playlist_normal
} playmode_t;

extern consvar_t  play_mode;
#endif


// register sound vars and commands at game startup
void S_RegisterSoundStuff();


// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
void S_Init(int sfxVolume, int musicVolume);



//
// MusicInfo struct.
//
struct musicinfo_t
{
  char  name[9];   // up to 8-character lumpname
  int   lumpnum;   // lump number of music
  int   length;    // lump length in bytes
  void* data;      // music data
  int   handle;    // music handle once registered
};

// defines a moving 3D sound source
struct soundsource_t
{
  fixed_t x, y, z;
  fixed_t vx, vy, vz;
  void* origin; // pointer to the game object making the sound (Actor etc.)
};

// normal "static" mono(stereo) sound channel
struct channel_t
{
  // sound information (if NULL, channel is unused)
  sfxinfo_t *sfxinfo;
  // handle of the sound being played
  int handle;  
};

// 3D sound channel
struct channel3D_t : public channel_t
{
  // origin of sound
  soundsource_t source;
};

class SoundSystem
{
private:
  // sound effects
  // the set of channels available
  vector<channel_t>   channels; // static stereo or mono sounds
  vector<channel3D_t> channel3Ds; // dynamic 3D positional sounds

  int sfxvolume;

  // music
  int musicvolume;

  // whether music is paused
  bool mus_paused;

  // music currently being played
  musicinfo_t *mus_playing;

  // gametic when to do cleanup
  int nextcleanup;

  channel_t   *GetChannel(sfxinfo_t *sfx);
  channel3D_t *Get3DChannel(sfxinfo_t *sfx);
  void StopChannel(int cnum);
  void Stop3DChannel(int cnum);

public:
  SoundSystem();

  void SetSfxVolume(int volume);
  void SetMusicVolume(int volume);

  // sound
  void ResetChannels(int stereo, int dynamic);

  // normal mono/stereo sound
  void StartAmbSound(sfxinfo_t *sfx, int volume = 255);
  // positional 3D sound
  void Start3DSound(soundsource_t *source, sfxinfo_t *sfx, int volume = 255);

  void Stop3DSounds();
  void Stop3DSound(void *origin);

  // music
  void PauseMusic();
  void ResumeMusic();

  // caches music lump "name", starts playing it
  bool StartMusic(const char *name, bool looping = false);
  // same for old Doom/Heretic musics. See sounds.h.
  bool StartMusic(int music_id, bool looping = false);

  // Stops the music fer sure.
  void StopMusic();


  // Updates music & sounds
  void UpdateSounds();

};

extern SoundSystem S;


// returns a lumpnum, either of sfx or of a replacing sound
int S_GetSfxLumpNum(sfxinfo_t* sfx);

// wrappers
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
void S_StartAmbSound(int sfx_id, int volume = 255);
void S_StartSound(mappoint_t *origin, int sfx_id);
void S_StartSound(Actor *origin, int sfx_id);


//void S_StartSoundName(Actor *mo, const char *soundname);
//int S_SoundPlaying(void *origin, int id);


#endif
