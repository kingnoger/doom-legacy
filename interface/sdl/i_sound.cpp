// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2000-2003 by Doom Legacy team
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log$
// Revision 1.19  2004/08/29 20:48:49  smite-meister
// bugfixes. wow.
//
// Revision 1.18  2004/08/18 14:35:21  smite-meister
// PNG support!
//
// Revision 1.17  2004/07/13 20:23:38  smite-meister
// Mod system basics
//
// Revision 1.16  2004/07/07 17:27:20  smite-meister
// bugfixes
//
// Revision 1.15  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.14  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.13  2004/01/02 14:25:02  smite-meister
// cleanup
//
// Revision 1.12  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.11  2003/12/21 18:35:15  jussip
// Minor cleanup.
//
// Revision 1.10  2003/04/24 00:25:43  hurdler
// Ok, since it doesn't work otherwise, add an ifdef
//
// Revision 1.9  2003/04/24 00:03:02  hurdler
// Should fix compiling problem
//
// Revision 1.8  2003/04/23 21:12:02  hurdler
// Do it again more properly
//
// Revision 1.7  2003/04/23 21:02:00  hurdler
// no more linking warning
//
// Revision 1.6  2003/04/14 08:58:31  smite-meister
// Hexen maps load.
//
// Revision 1.5  2003/03/08 16:07:18  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.4  2003/02/16 16:54:52  smite-meister
// L2 sound cache done
//
// Revision 1.3  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.2  2002/12/23 23:25:53  smite-meister
// Ogg Vorbis works!
//
// Revision 1.1.1.1  2002/11/16 14:18:31  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   SDL system interface for sound.
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id$";

#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef FREEBSD
# include <SDL.h>
# include <SDL_mixer.h>
#else
# include <SDL/SDL.h>
# include <SDL/SDL_mixer.h>
#endif

#include "doomdef.h"
#include "doomtype.h"
#include "command.h"
#include "cvars.h"

#include "hardware/hw3sound.h"
#include "z_zone.h"

#include "m_swap.h"
#include "i_system.h"
#include "i_sound.h"
#include "s_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "s_sound.h"
#include "sounds.h"

#include "d_main.h"

#include "qmus2mid.h"


#define MIDBUFFERSIZE   128*1024
#define SAMPLERATE      11025   // Hz

class chan_t
{
public:
  channel_t *ch; // the corresponding SoundSystem channel

  // The channel data pointers, start and end.
  Uint8* data;
  Uint8* end;

  // pitch
  Uint32 step;          // The channel step amount...
  Uint32 stepremainder; // ... and a 0.16 bit remainder of last step.

  // Hardware left and right channel volume lookup.
  int*  leftvol_lookup;
  int*  rightvol_lookup;

  // calculate sound parameters using ch
  void CalculateParams();
};



static vector<chan_t> channels;

const int samplecount = 512; // requested audio buffer size (about 46 ms at 11 kHz)

// Pitch to stepping lookup. 64 pitch units = 1 octave
//  0 = 0.25x, 128 = 1x, 256 = 4x
static int steptable[256];

// Volume lookups.
static int vol_lookup[128*256];

// Buffer for MIDI
static char* musicbuffer;


// Flags for the -nosound and -nomusic options
extern bool nosound;
extern bool nomusic;

static bool musicStarted = false;
static bool soundStarted = false;

static SDL_AudioSpec audio;
static Mix_Music *music[2] = { NULL, NULL };




static void I_SetChannels()
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int i, j;

  // This table provides step widths for pitch parameters.
  for (i = 0; i < 256; i++)
    steptable[i] = (int)(pow(2.0, ((i-128)/64.0))*65536.0);

  // Generates volume lookup tables
  //  which also turn the U8 samples
  //  into S16 samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/127;
}

/*
static int FindChannel(int handle)
{
  int i;

  for (i = 0; i < NUM_CHANNELS; i++)
    if (channels[i].handle == handle)
      return i;
  
  // not found
  return -1;
}
*/


//----------------------------------------------


void I_SetSfxVolume(int volume)
{
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  // Since the variable is cv_soundvolume,
  // nothing needs to be done.
}

// used to (re)calculate channel params
void chan_t::CalculateParams()
{
  step = steptable[ch->opitch];

  // x^2 separation, that is, orientation/stereo.
  //  range is: 0 (left) - 255 (right)

  // Volume arrives in range 0..255 and it must be in 0..cv_soundvolume...
  int vol = (ch->ovol * cv_soundvolume.value) >> 7;
  // note: >> 6 would use almost the entire dynamical range, but
  // then there would be no "dynamical room" for other sounds :-/
  int sep = ch->osep;

  int leftvol  = vol - ((vol*sep*sep) >> 16); ///(256*256);
  sep = 255 - sep;
  int rightvol = vol - ((vol*sep*sep) >> 16);

  // Sanity check, clamp volume.
  if (rightvol < 0 || rightvol > 127)
    I_Error("rightvol out of bounds");

  if (leftvol < 0 || leftvol > 127)
    I_Error("leftvol out of bounds");

  // Get the proper lookup table piece
  //  for this volume level???
  leftvol_lookup = &vol_lookup[leftvol*256];
  rightvol_lookup = &vol_lookup[rightvol*256];
}

//----------------------------------------------
// I_StartSound
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.

int I_StartSound(channel_t *s_channel)
{
  if (nosound)
    return 0;

  //SDL_LockAudio();

  int n = channels.size();
  //CONS_Printf("SDL channels: %d\n", n);

  // Tales from the cryptic.
  // Because SoundSystem class takes care of channel management,
  // we know that a new channel can be created.
  channels.resize(n+1);
  chan_t *c = &channels[n];
  c->ch = s_channel;

  // Set pointer to raw data.
  c->data = (Uint8 *)s_channel->si->sdata;
  // Set pointer to end of raw data.
  c->end = c->data + s_channel->si->length;
  
  c->stepremainder = 0;

  // Set pitch, volume and separation
  c->CalculateParams();

  s_channel->playing = true;

  //SDL_UnlockAudio();

  /*
  // Assign current handle number.
  static unsigned short handlenum = 100;
  c->handle = handlenum--;

  // Reset current handle number, limited to 1..100.
  if (!handlenum)
    handlenum = 100;

  return c->handle;
  */
  return 1;
}


void I_StopSound(channel_t *c)
{
  vector<chan_t>::iterator i;

  for (i = channels.begin(); i != channels.end(); i++)
    if ((*i).ch == c)
      break;
 
  if (i != channels.end())
    channels.erase(i);
}

/*
bool I_SoundIsPlaying(int handle)
{
  return (FindChannel(handle) < 0);
}
*/

// Not used by SDL version
//void I_SubmitSound() {}

// Not used by SDL version. Nevertheless, please 
// witness the wit below:
// void I_UpdateSound () {}

/* Pour une raison que j'ignore, la version SDL n'appelle jamais
   ce truc directement. Fonction vide pour garder une compatibilité
   avec le point de vue de legacy... */

// Himmel, Arsch und Zwirn

//
// The SDL audio callback.
//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the given
//  mixing buffer, and clamping it to the allowed
//  range.
//
// This function currently supports only 16bit.
//
static void I_UpdateSound_sdl(void *unused, Uint8 *stream, int len)
{
  if (nosound)
    return;

  // stream contains len bytes of outgoing stereo (LR order) music data.
  // Here we mix in current sound data.

  // Data, from raw sound, for right and left.
  register unsigned int sample;
  register int dl, dr;

  // Left and right channel
  // are in S16 audio stream, alternating.
  Sint16 *leftout = (Sint16 *)stream;
  Sint16 *rightout = ((Sint16 *)stream) + 1;

  // Step in stream, left and right, thus two.
  int step = 2;

  // Determine end, for left channel only
  //  (right channel is implicit).
  Sint16 *leftend = leftout + len/step;

  // Mix sounds into the mixing buffer.
  // Loop over entire buffer
  while (leftout != leftend)
    {
      // Reset left/right value.
      dl = *leftout;
      dr = *rightout;

      // Love thy L2 chache - made this a loop.
      // Now more channels could be set at compile time
      //  as well. Thus loop those  channels.
      int i, n = channels.size();
      for (i = 0; i < n; i++)
        {
	  chan_t *c = &channels[i];
	  // Check if channel is active.
	  if (c->data)
            {
	      // Get the raw data from the channel.
	      sample = *(c->data);
	      // Add left and right part for this channel (sound)
	      //  to the current data.
	      // Adjust volume accordingly.
	      dl += c->leftvol_lookup[sample];
	      dr += c->rightvol_lookup[sample];
	      // pitch.
	      c->stepremainder += c->step;
	      // 16.16 fixed point: high word is the current stride
	      c->data += c->stepremainder >> 16;
	      // cut away high word
	      c->stepremainder &= 0xffff;
	      
	      // Check whether data is exhausted.
	      if (c->data >= c->end)
		{
		  c->data = NULL;
		  c->ch->playing = false; // notify SoundSystem
		}
            }
        }

      // Clamp to range. Left hardware channel.
      if (dl > 0x7fff)
	*leftout = 0x7fff;
      else if (dl < -0x8000)
	*leftout = -0x8000;
      else
	*leftout = dl;

      // Same for right hardware channel.
      if (dr > 0x7fff)
	*rightout = 0x7fff;
      else if (dr < -0x8000)
	*rightout = -0x8000;
      else
	*rightout = dr;

      // Increment current pointers in stream
      leftout += step;
      rightout += step;
    }
}

/*
bool I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
  // Would be using the handle to identify
  //  on which channel the sound might be active,
  //  and resetting the channel parameters.

  int i = FindChannel(handle);
  if (i < 0)
    return false;

  chan_t *c = &channels[i];
  if (c->data)
    {
      I_SetChannelParams(c, vol, sep, pitch);
      return true;
    }
  return false;
}
*/




void I_StartupSound()
{
  if (nosound)
    return;

  // Configure sound device
  if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
      CONS_Printf("Couldn't initialize SDL Audio: %s\n",SDL_GetError());
      nosound = true;
      return;
    }

  // Open the audio device
  audio.freq = SAMPLERATE;
#if ( SDL_BYTEORDER == SDL_BIG_ENDIAN )
  audio.format = AUDIO_S16MSB;
#else
  audio.format = AUDIO_S16LSB;
#endif
  audio.channels = 2;
  audio.samples = samplecount;
  audio.callback = I_UpdateSound_sdl;

  // SDL_mixer controls the audio device, see I_InitMusic.

  I_SetChannels();
  // Finished initialization.
  CONS_Printf("I_InitSound: sound module ready\n");
}




// initializes both sound and music
void I_InitMusic()
{
  if (nosound)
    {
      // FIXME: workaround for shitty programming undoc'ed features
      nomusic = true;
      return;
    }

  // because we use SDL_mixer, audio is opened here.
  if (Mix_OpenAudio(audio.freq, audio.format, audio.channels, audio.samples) < 0)
    {
      CONS_Printf("Unable to open audio: %s\n", Mix_GetError());
      nosound = nomusic = true;
      return;
    }

  Mix_SetPostMix(audio.callback, NULL);  // after mixing music, add sound effects

  CONS_Printf(" configured audio device with %d samples/slice\n", samplecount);
  //SDL_PauseAudio(0);
  Mix_Resume(-1); // start all sound channels (although they are not used)

  soundStarted = true;

  if (nomusic)
    return;

  Mix_ResumeMusic();  // start music playback
  musicbuffer = (char *)Z_Malloc(MIDBUFFERSIZE, PU_MUSIC, NULL); // FIXME: catch return value
  CONS_Printf("I_InitMusic: music initialized\n");
  musicStarted = true;
}



// finish sound and music
void I_ShutdownSound()
{
  if (!soundStarted)
    return;
    
  CONS_Printf("I_ShutdownSound: ");
  Mix_CloseAudio();
  //SDL_CloseAudio();

  CONS_Printf("shut down\n");
  soundStarted = false;

  if (!musicStarted)
    return;

  Z_Free(musicbuffer);
  CONS_Printf("I_ShutdownMusic: shut down\n");
  musicStarted = false;  
}



void I_PlaySong(int handle, int looping)
{
  if (nomusic)
    return;

  if (music[handle])
    {
      Mix_FadeInMusic(music[handle], looping ? -1 : 0, 500);
    }
}

void I_PauseSong (int handle)
{
  if (nomusic)
    return;

  I_StopSong(handle);
}

void I_ResumeSong (int handle)
{
  if (nomusic)
    return;

  I_PlaySong(handle, true);
}

void I_StopSong(int handle)
{
  if (nomusic)
    return;
  Mix_FadeOutMusic(500);
}

static char *MIDI_tmpfilename = NULL;

void I_UnRegisterSong(int handle)
{
  if (nomusic)
    return;

  if (music[handle])
    {
      Mix_FreeMusic(music[handle]);
      music[handle] = NULL;
    }

  if (MIDI_tmpfilename)
    {
      unlink(MIDI_tmpfilename);
      MIDI_tmpfilename = NULL;
    }
}


int I_RegisterSong(void* data, int len)
{
  if (nomusic)
    return 0;

  MIDI_tmpfilename = "Legacy_music.tmp";

  FILE *midfile = fopen(MIDI_tmpfilename, "wb");
  if (midfile == NULL)
    {
      CONS_Printf("Couldn't create a tmpfile for music!\n");
      return 0;
    }

  if (memcmp(data,"MUS",3) == 0)
    {
      int err;
      Uint32 midlength;
      // convert mus to mid with a wonderful function
      // thanks to S.Bacquet for the source of qmus2mid
      // convert mus to mid and load it in memory
      if ((err = qmus2mid((byte *)data, (byte *)musicbuffer, 89, 64, 0, len, MIDBUFFERSIZE, &midlength)) != 0)
	{
	  CONS_Printf("Cannot convert mus to mid, converterror :%d\n",err);
	  return 0;
	}
      fwrite(musicbuffer, 1, midlength, midfile);
    }
  else if (memcmp(data,"MThd",4) == 0 || memcmp(data, "Ogg", 3) == 0)
    { // MIDI and Ogg Vorbis
      fwrite(data, 1, len, midfile);
    }
  else
    {
      CONS_Printf("Music Lump is not MID or MUS lump\n");
      return 0;
    }
  
  fclose(midfile);

  //music[0] = Mix_LoadMUS("compilation-ogg-q0.ogg"); // Ogg Vorbis works!
  //music[0] = Mix_LoadMUS("first_call.mp3"); // mp3 test
  music[0] = Mix_LoadMUS(MIDI_tmpfilename);
    
  if (music[0] == NULL)
    {
      CONS_Printf("Couldn't load music from tempfile %s: %s\n", MIDI_tmpfilename, Mix_GetError());
    }
  return 0;
}

void I_SetMusicVolume(int volume)
{
  if (nomusic)
    return;

  // acceptable volume range : 0-128
  Mix_VolumeMusic(volume*2);
}
