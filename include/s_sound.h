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
// Revision 1.7  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.6  2003/04/14 08:58:31  smite-meister
// Hexen maps load.
//
// Revision 1.5  2003/04/05 12:20:00  smite-meister
// Makefiles fixed
//
// Revision 1.4  2003/03/08 16:07:16  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.3  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
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
#include "z_cache.h"

using namespace std;

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

#ifdef LINUX_X11
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


// defines a 3D sound source. We can't just use an Actor* because of
// sounds originating from mappoint_t's. A bit clumsy but works.
struct soundsource_t
{
  fixed_t x, y, z;
  fixed_t vx, vy, vz;
  bool     isactor; // is origin an Actor (or a mappoint_t)?
  union
  { // the sound origin
    mappoint_t *mpt;
    Actor      *act;
  };

public:
  void Update();
};


class sounditem_t : public cacheitem_t
{
  friend class soundcache_t;
protected:
  int   lumpnum; // lump number of data

public:
  void *data;    // unconverted data
  int   length;  // in bytes
  void *sdata;   // raw converted sound data
};


// normal "static" mono(stereo) sound channel
struct channel_t
{
  int volume;
  int pitch;
  int priority;

  // observed variables
  int ovol;
  int opitch;
  int osep; // left/right x^2 separation. 128 is front, 0 is totally left, 256 is totally right

  sounditem_t *si;

  bool playing;
  soundsource_t source;  // origin of sound (or NULL)

public:
  int Adjust(Actor *listener);
};

/*
// 3D sound channel
struct channel3D_t : public channel_t
{
  soundsource_t source;  // origin of sound
};
*/

class SoundSystem
{
private:
  // sound effects

  // the set of channels available
  vector<channel_t>   channels; // static stereo or mono sounds
  //vector<channel3D_t> channel3Ds; // dynamic 3D positional sounds

  int soundvolume;

  // music
  int  musicvolume;  
  bool mus_paused; // whether music is paused
  musicinfo_t *mus_playing;   // music currently being played

  // gametic when to do cleanup
  int nextcleanup;

  // these are only used internally, user can change them using consvars
  void SetSoundVolume(int volume);
  void SetMusicVolume(int volume);
  void ResetChannels(int num);

  int   GetChannel(int pri);
  //int Get3DChannel(int pri);

public:
  SoundSystem();

  void Startup(); // initialization when game starts

  // --------- sound

  // normal mono/stereo sound
  int StartAmbSound(const char *name, float volume = 1.0, int separation = 128, int pitch = 128, int pri = 64);
  // positional 3D sound
  int Start3DSound(const char *name, soundsource_t *source, float volume = 1.0, int pitch = 128, int pri = 64);

  void Stop3DSounds();
  void Stop3DSound(void *origin);

  void StopChannel(unsigned cnum);
  //void Stop3DChannel(int cnum);
  bool ChannelPlaying(unsigned cnum);

  // --------- music
  void PauseMusic();
  void ResumeMusic();

  // caches music lump "name", starts playing it
  bool StartMusic(const char *name, bool looping = false);

  // Stops the music fer sure.
  void StopMusic();


  // Updates music & sounds
  void UpdateSounds();
};

extern SoundSystem S;

int S_GetSoundID(const char *tag);

// wrappers
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
int S_StartAmbSound(int sfx_id, float volume = 1.0);
int S_StartSound(mappoint_t *origin, int sfx_id, float volume = 1.0);
int S_StartSound(Actor *origin, int sfx_id, float volume = 1.0);

// for old Doom/Heretic musics. See sounds.h.
bool S_StartMusic(int music_id, bool looping = false);


#endif
