// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.13  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.12  2004/08/12 18:30:30  smite-meister
// cleaned startup
//
// Revision 1.11  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.10  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.9  2004/01/02 14:25:02  smite-meister
// cleanup
//
// Revision 1.8  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief The not so system specific sound interface.

#ifndef s_sound_h
#define s_sound_h 1

#include <vector>
#include <string>
#include <map>
#include "m_fixed.h"
#include "z_cache.h"

using namespace std;

//
// A playing piece of music
//
struct musicinfo_t
{
  char  name[9];   // up to 8-character lumpname
  int   lumpnum;   // lump number of music
  int   length;    // lump length in bytes
  void* data;      // music data
  int   handle;    // music handle once registered
};


// struct for Doom native sound format:
// first a 8-byte header composed of 4 unsigned (16-bit) short integers (LE/BE ?),
// then the data (8-bit 11 kHz mono sound)
// max # of samples = 65535 = about 6 seconds of sound
struct doomsfx_t
{
  unsigned short magic; // always 3
  unsigned short rate;  // always 11025
  unsigned short samples; // number of 1-byte samples
  unsigned short zero; // always 0
  byte data[0]; // actual data begins here
};


#define S_TAGLEN 32 // change with care

// describes an abstract sound effect
class sfxinfo_t
{
public:
  char  tag[S_TAGLEN + 1]; // SNDINFO tag (+ \0)
  int   number;       // internal sound number
  char  lumpname[9];  // up to 8-character name (+ \0)
  byte  multiplicity; // how many instances of the sound can be heard simultaneously. 0 means not limited.
  byte  priority;     // bigger is better
  byte  pitch;        // 128 means unchanged, 64 pitch units = 1 octave

public:
  sfxinfo_t(const char *t, int n);
};


// sound sequence definition
struct sndseq_t
{
  int    number;
  int    stopsound;
  string name;
  vector<int> data; // the sequence script

public:
  void clear();
};


// for sound cache
class sounditem_t : public cacheitem_t
{
  friend class soundcache_t;
protected:
  int   lumpnum; // lump number of data

public:
  sounditem_t(const char *name);
  virtual ~sounditem_t();

  void *data;    // unconverted data
  int   length;  // in bytes
  void *sdata;   // raw converted sound data
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
    struct mappoint_t *mpt;
    class  Actor      *act;
  };

public:
  void Update();
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

  class sounditem_t *si;

  bool playing;
  soundsource_t source;  // origin of sound (or NULL)

public:
  int Adjust(Actor *listener);
};


/// \brief Sound and music subsystem
///
/// Manages sound channels for both ambient (normal stereo) and 3D sound effects.
/// Also takes care of playing music.
/// There is only one global instance in use, called "S".
class SoundSystem
{
private:
  // sound
  int soundvolume;
  vector<channel_t> channels;   // the set of channels available

  // music
  int  musicvolume;  
  bool mus_paused; // whether music is paused
  musicinfo_t *mus_playing;   // music currently being played

  // gametic when to do cleanup
  unsigned nextcleanup;

  // these are only used internally, user can change them using consvars
  void SetSoundVolume(int volume);
  void SetMusicVolume(int volume);
  void ResetChannels(int num);

  int  GetChannel(int pri);

public:
  SoundSystem();

  void Startup(); // initialization when game starts

  // --------- sound

  // normal mono/stereo sound
  int StartAmbSound(sfxinfo_t *s, float volume = 1.0, int separation = 128);
  // positional 3D sound
  int Start3DSound(sfxinfo_t *s, soundsource_t *source, float volume = 1.0);

  void Stop3DSounds();
  void Stop3DSound(void *origin);

  void StopChannel(unsigned cnum);
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

extern  map<int, sfxinfo_t*> SoundID;  // ID-number => sound
typedef map<int, sfxinfo_t*>::iterator soundID_iter_t;


#endif
