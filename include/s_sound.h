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
// $Log$
// Revision 1.17  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.16  2005/06/16 18:18:11  smite-meister
// bugfixes
//
// Revision 1.15  2005/03/19 13:51:29  smite-meister
// sound samplerate fix
//
// Revision 1.14  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.13  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.11  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.10  2004/03/28 15:16:14  smite-meister
// Texture cache.
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
#include "vect.h"
#include "m_fixed.h"
#include "z_cache.h"

using namespace std;


/// \brief A playing piece of music
struct musicinfo_t
{
  string name;      ///< music lumpname
  int    lumpnum;   ///< lump number of music
  int    length;    ///< lump length in bytes
  void  *data;      ///< music data
  int    handle;    ///< music handle once registered

public:
  musicinfo_t();
};



/// \brief A sound effect definition, generated from the SNDINFO lump.
class sfxinfo_t
{
public:
#define S_TAGLEN 32 // change with care
  char  tag[S_TAGLEN + 1]; ///< SNDINFO tag (+ \0)
  int   number;       ///< internal sound number
  char  lumpname[9];  ///< up to 8-character name (+ \0)
  byte  multiplicity; ///< how many instances of the sound can be heard simultaneously. 0 means not limited.
  byte  priority;     ///< bigger is better
  byte  pitch;        ///< 128 means unchanged, 64 pitch units = 1 octave

public:
  sfxinfo_t(const char *t, int n);
};



/// \brief A sound sequence definition, generated from the SNDSEQ lump.
struct sndseq_t
{
  int    number;
  int    stopsound;
  string name;
  vector<int> data; ///< the sequence script

public:
  void clear();
};



/// \brief Sound cache item, 
class sounditem_t : public cacheitem_t
{
  friend class soundcache_t;
protected:
  int   lumpnum; ///< lump number of data

public:
  sounditem_t(const char *name);
  virtual ~sounditem_t();

  unsigned  rate;   ///< sample rate in Hz
  void     *data;   ///< unconverted data
  int       length; ///< length in bytes
  void     *sdata;  ///< raw converted sound data (for now assumed always to be U8 depth)
};



/// \brief A 3D sound source.
///
/// We can't just use an Actor* because of
/// sounds originating from mappoint_t's. A bit clumsy but works.
struct soundsource_t
{
  vec_t<fixed_t> pos;
  vec_t<fixed_t> vel;

  bool     isactor; ///< is origin an Actor (or a mappoint_t)?
  union
  { // the sound origin
    struct mappoint_t *mpt;
    class  Actor      *act;
  };

public:
  void Update();
};


/// \brief Normal "static" mono(stereo) sound channel.
struct channel_t
{
  int volume;
  int pitch;
  int priority;

  // observed variables
  int ovol;
  int opitch;
  int osep; ///< left/right x^2 separation. 128 is front, 0 is totally left, 256 is totally right

  class sounditem_t *si;

  bool playing;
  soundsource_t source;  ///< origin of sound (or NULL)

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
  vector<channel_t> channels; ///< the set of channels available

  // music
  int  musicvolume;
  bool mus_paused;            ///< whether music is paused
  musicinfo_t *mus_playing;   ///< music currently being played

  unsigned nextcleanup;       ///< gametic when to do cleanup

  // these are only used internally, user can change them using consvars
  void SetSoundVolume(int volume);
  void SetMusicVolume(int volume);
  void ResetChannels(int num);

  int  GetChannel(int pri);

public:
  SoundSystem();

  /// initialization when game starts
  void Startup(); 

  // --------- sound ---------

  /// normal mono/stereo sound
  int StartAmbSound(sfxinfo_t *s, float volume = 1.0, int separation = 128);
  /// positional 3D sound
  int Start3DSound(sfxinfo_t *s, soundsource_t *source, float volume = 1.0);

  void Stop3DSounds();
  void Stop3DSound(void *origin);

  void StopChannel(unsigned cnum);
  bool ChannelPlaying(unsigned cnum);

  // --------- music ---------
  void PauseMusic();
  void ResumeMusic();

  /// Caches music lump "name", starts playing it.
  bool StartMusic(const char *name, bool looping = false);
  /// Stops the music fer sure.
  void StopMusic();
  /// Returns the lumpname of the currently playing piece of music (or NULL if none).
  const char *GetMusic() const { return mus_playing ? mus_playing->name.c_str() : NULL; }


  /// Updates music & sounds.
  void UpdateSounds();
};

extern SoundSystem S;

extern  map<int, sfxinfo_t*> SoundID;  // ID-number => sound
typedef map<int, sfxinfo_t*>::iterator soundID_iter_t;


#endif
