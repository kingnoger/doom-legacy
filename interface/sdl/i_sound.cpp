// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.2  2002/12/23 23:25:53  smite-meister
// Ogg Vorbis works!
//
// Revision 1.1.1.1  2002/11/16 14:18:31  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.6  2002/09/20 22:41:35  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.5  2002/08/23 18:05:39  vberghol
// idiotic segfaults fixed
//
// Revision 1.4  2002/08/19 18:06:45  vberghol
// renderer somewhat fixed
//
// Revision 1.3  2002/07/01 21:01:03  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:34  vberghol
// Version 133 Experimental!
//
// Revision 1.10  2001/08/20 20:40:42  metzgermeister
// *** empty log message ***
//
// Revision 1.9  2001/05/16 22:33:35  bock
// Initial FreeBSD support.
//
// Revision 1.8  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.7  2001/04/14 14:15:14  metzgermeister
// fixed bug no sound device
//
// Revision 1.6  2001/04/09 20:21:56  metzgermeister
// dummy for I_FreeSfx
//
// Revision 1.5  2001/03/25 18:11:24  metzgermeister
//   * SDL sound bug with swapped stereo channels fixed
//   * separate hw_trick.c now for HW_correctSWTrick(.)
//
// Revision 1.4  2001/03/09 21:53:56  metzgermeister
// *** empty log message ***
//
// Revision 1.3  2000/11/02 19:49:40  bpereira
// no message
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
//
// DESCRIPTION:
//   SDL system interface for sound.
// VB: got SDL_mixer library! Fixed music! Mostly!
//     What were open_music, close_music and music_mixer??
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id$";

#include <math.h>
#include <unistd.h>

#ifdef FREEBSD
# include <SDL.h>
# include <SDL_mixer.h>
#else
# include <SDL/SDL.h>
# include <SDL/SDL_mixer.h>
#endif

#include "hardware/hw3sound.h"
#include "z_zone.h"

#include "m_swap.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomtype.h"
#include "s_sound.h"
#include "sounds.h"

#include "d_main.h"

#include "qmus2mid.h"

extern tic_t gametic;

//#define PIPE_CHECK(fh) if (broken_pipe) { fclose(fh); fh = NULL; broken_pipe = 0; }

#define MIDBUFFERSIZE   128*1024

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.

// Needed for calling the actual sound output.
#define NUM_CHANNELS            8

#define SAMPLERATE              11025   // Hz

struct chan_t
{
  // The channel data pointers, start and end.
  Uint8* data;
  Uint8* end;

  // pitch
  Uint32 step;          // The channel step amount...
  Uint32 stepremainder; // ... and a 0.16 bit remainder of last step.

  // Time/gametic that the channel started playing,
  //  used to determine oldest, which automatically
  //  has lowest priority.
  int starttic;

  // The sound handle, determined on registration,
  // used to unregister/stop/modify,
  int handle;

  // SFX id of the playing sound effect.
  // Used to catch duplicates (like chainsaw).
  //int id;

  // Hardware left and right channel volume lookup.
  int*  leftvol_lookup;
  int*  rightvol_lookup;
};

static chan_t channels[NUM_CHANNELS];

const int samplecount = 512; // requested audio buffer size

//static int lengths[NUMSFX]; // The actual lengths of all sound effects.

// Pitch to stepping lookup. 64 pitch units = 1 octave
//  0 = 0.25x, 128 = 1x, 256 = 4x
static int steptable[256];

// Volume lookups.
static int vol_lookup[128*256];

// Buffer for MIDI
static char* musicbuffer;

//#define MIDI_TMPFILE    "/tmp/.lsdlmidi"
static const char *MIDI_tmpfilename;

// Flags for the -nosound and -nomusic options
extern bool nosound;
extern bool nomusic;

static bool musicStarted = false;
static bool soundStarted = false;


// Well... To keep compatibility with legacy doom, I have to call this in
// I_InitSound since it is not called in S_Init... (emanne@absysteme.fr)

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

static int FindChannel(int handle)
{
  int i;

  for (i = 0; i < NUM_CHANNELS; i++)
    if (channels[i].handle == handle)
      return i;
  
  // not found
  return -1;
}

//----------------------------------------------
// This function loads the sound data from the WAD lump,
//  for a single sound and sets up sfx fields.
//  sfx must be a valid pointer.
// We assume that the sound is in Doom sound format (for now).
// TODO: Make it recognize other formats as well!

void *I_GetSfx(sfxinfo_t* sfx)
{
  // we must set up data, length and lumpnum

  if (sfx->link)
    {
      // recursive handling of links!
      if (!sfx->link->data)
	I_GetSfx(sfx->link);

      sfx->data = sfx->link->data;
      sfx->length = sfx->link->length;
      return sfx->data;
    }

  // now Heretic works fine
  if (sfx->lumpnum < 0)
    sfx->lumpnum = S_GetSfxLumpNum(sfx);

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      sfx->data = fc.CacheLumpNum(sfx->lumpnum, PU_SOUND);
      return sfx->data;
    }
#endif

  Uint8 *data = (Uint8 *)fc.CacheLumpNum(sfx->lumpnum, PU_SOUND);
  int size = fc.LumpLength(sfx->lumpnum);

  doomsfx_t *ds = (doomsfx_t *)data;
  // TODO: endianness conversion

  CONS_Printf(" Sound: %s, m = %d, r = %d, s = %d, z = %d, length = %d\n",
	      sfx->name, ds->magic, ds->rate, ds->samples, ds->zero, size);

  // Pads the sound effect out to the mixing buffer size.
  // The original realloc would interfere with zone memory.
  //int paddedsize = ((size-8 + (samplecount-1)) / samplecount) * samplecount;
  // Allocate from zone memory.
  //Uint8 *paddeddata = (unsigned char*)Z_Malloc(paddedsize + 8, PU_STATIC, 0);
  // This should interfere with zone memory handling,
  //  which does not kick in in the soundserver.

  //int i;

  // Now copy and pad.
  //memcpy(paddeddata, data, size);
  //for (i=size ; i<paddedsize+8 ; i++)
  //  paddeddata[i] = 128;

  // Remove the cached lump.
  //Z_Free(data);

  // Preserve padded length.
  //*len = paddedsize;
  sfx->length = size - 8;  // 8 byte header
  sfx->data = &ds->data;
  // Return allocated padded data.
  return sfx->data;//(paddeddata + 8)
}

// FIXME: handle links gracefully
void I_FreeSfx(sfxinfo_t* sfx)
{
  sfx->data = NULL;
  sfx->length = 0;
}


//
// SFX API
//


//----------------------------------------------
//
void I_SetSfxVolume(int volume)
{
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  // Since the variable is cv_soundvolume,
  // nothing needs to be done.
  //CV_SetValue(&cv_soundvolume, volume);
}

// used to (re)calculate channel params based on vol, sep, pitch
static void I_SetChannelParams(chan_t *c, int vol, int sep, int pitch)
{
  c->step = steptable[pitch];

  // x^2 separation, that is, orientation/stereo.
  //  range is: 0 (left) - 255 (right)

  // Volume arrives in range 0..255 and it must be in 0..cv_soundvolume...
  vol = (vol * cv_soundvolume.value) >> 7;
  // note: >> 6 would use almost the entire dynamical range, but
  // then there would be no "dynamical room" for other sounds :-/

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
  c->leftvol_lookup = &vol_lookup[leftvol*256];
  c->rightvol_lookup = &vol_lookup[rightvol*256];
}

//----------------------------------------------
// I_StartSound
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.

int I_StartSound(sfxinfo_t *sfx, int vol, int sep, int pitch)
{
  // int sfxid 
  if (nosound)
    return 0;

  SDL_LockAudio();

  int i;

  // Chainsaw troubles.
  // Play these sound effects only one at a time.
  /*
  if ( sfxid == sfx_sawup
       || sfxid == sfx_sawidl
       || sfxid == sfx_sawful
       || sfxid == sfx_sawhit
       || sfxid == sfx_stnmov
       || sfxid == sfx_pistol )
    {
      // Loop all channels, check.
      for (i=0 ; i<NUM_CHANNELS ; i++)
	{
	  // Active, and using the same SFX?
	  if ( (channels[i])
	       && (channelids[i] == sfxid) )
	    {
	      // Reset.
	      channels[i] = 0;
	      // We are sure that iff,
	      //  there will only be one.
	      break;
	    }
	}
    }
  */

  int oldest = gametic;
  int oldestnum = 0;

  // Loop all channels to find a free channel / the oldest SFX.
  for (i=0; i<NUM_CHANNELS; i++)
    {
      if (!channels[i].data)
	break;
      if (channels[i].starttic < oldest)
	{
	  oldestnum = i;
	  oldest = channels[i].starttic;
	}
    }

  // Tales from the cryptic.
  // If we found a free channel, fine.
  // If not, we simply overwrite the oldest one.

  if (i == NUM_CHANNELS)
    i = oldestnum;
  
  chan_t *c = &channels[i]; 

  // Okay, in the less recent channel,
  //  we will handle the new SFX.

  // Set pointer to raw data.
  c->data = (Uint8 *)sfx->data;
  // Set pointer to end of raw data.
  c->end = c->data + sfx->length;
  
  // Should be gametic, I presume.
  c->starttic = gametic;
  c->stepremainder = 0;

  // Set pitch, volume and separation
  I_SetChannelParams(c, vol, sep, pitch);

  // Preserve sound SFX id,
  //  e.g. for avoiding duplicates of chainsaw.
  //channelids[slot] = sfxid;

  SDL_UnlockAudio();

  // Assign current handle number.
  static unsigned short handlenum = 100;
  c->handle = handlenum--;

  // Reset current handle number, limited to 1..100.
  if (!handlenum)
    handlenum = 100;

  return c->handle;
}


void I_StopSound(int handle)
{
  int i = FindChannel(handle);
  if (i >= 0)
    {
      channels[i].data = NULL;
      channels[i].handle = -1;
    }
}


bool I_SoundIsPlaying(int handle)
{
  return (FindChannel(handle) < 0);
}


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
  //Sint16 *leftend = leftout + samplecount*step;
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
      int i;
      for (i = 0; i < NUM_CHANNELS; i++)
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
		  c->handle = -1;
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

void I_ShutdownSound()
{
  if (nosound)
    return;

  if (!soundStarted)
    return;
    
  CONS_Printf("I_ShutdownSound: ");
  SDL_CloseAudio();
  CONS_Printf("shut down\n");
  soundStarted = false;
}

static SDL_AudioSpec audio;

void I_StartupSound()
{
  if (nosound)
    return;

  // Configure sound device
  CONS_Printf("I_StartupSound()\n");

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

  /*
    // VB: if we have SDL_mixer, audio device is opened later in InitMusic. Ugly, I know.
    if ( SDL_OpenAudio(&audio, NULL) < 0 ) {
    CONS_Printf("Couldn't open audio with desired format\n");
    SDL_CloseAudio();
    nosound = true;
    return;
    }
    samplecount = audio.samples;
    CONS_Printf(" configured audio device with %d samples/slice\n", samplecount);
    SDL_PauseAudio(0);
    soundStarted = true;
  */

  // Initialize external data (all sounds) at start, keep static.
  //CONS_Printf("I_InitSound: (%d sfx)", NUMSFX);

  /*
  int i;
  for (i=1 ; i<NUMSFX ; i++)
    {
      // Alias? Example is the chaingun sound linked to pistol.
      if (S_sfx[i].name)
	if (!S_sfx[i].link)
	  {
	    // Load data from WAD file.
	    S_sfx[i].data = getsfx(&S_sfx[i], &lengths[i]);
	  } 
	else
	  {
	    // Previously loaded already?
	    S_sfx[i].data = S_sfx[i].link->data;
	    lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
	  }
    }
    CONS_Printf(" pre-cached all sound data\n"); 
  */

  I_SetChannels();
  // Finished initialization.
  CONS_Printf("I_InitSound: sound module ready\n");
}




//
// MUSIC API.
//


static Mix_Music *music[2] = { NULL, NULL };

void I_ShutdownMusic()
{
  /* Should this be exposed in mixer.h? */
  if (nomusic)
    return;

  if (!musicStarted)
    return;
    
  Mix_CloseAudio();

  CONS_Printf("I_ShutdownMusic: shut down\n");
  musicStarted = false;  
}

void I_InitMusic()
{
  /* Should this be exposed in mixer.h? */
  if (nosound)
    {
      // FIXME: workaround for shitty programming undoc'ed features
      nomusic = true;
      return;
    }
    
  if (nomusic)
    return;

  // because we use SDL_mixer, audio is opened here.
  if (Mix_OpenAudio(audio.freq, audio.format, audio.channels, audio.samples) < 0)
    {
      CONS_Printf("Unable to open music: %s\n", Mix_GetError());
      nosound = nomusic = true;
      return;
    }

  Mix_SetPostMix(audio.callback, NULL);  // after mixing music, add sound effects

  CONS_Printf(" configured audio device with %d samples/slice\n", samplecount);
  SDL_PauseAudio(0);
  soundStarted = true;

  // musicbuffer is never freed
  musicbuffer = (char *)Z_Malloc(MIDBUFFERSIZE,PU_STATIC,NULL); // FIXME: catch return value
  CONS_Printf("I_InitMusic: music initialized\n");
  musicStarted = true;
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

void I_UnRegisterSong(int handle)
{
  if (nomusic)
    return;

  if ( music[handle] ) {
    Mix_FreeMusic(music[handle]);
    music[handle] = NULL;
  }
  unlink(MIDI_tmpfilename);
}


int I_RegisterSong(void* data, int len)
{
  int err;
  ULONG midlength;
  FILE *midfile;

  if (nomusic)
    return 0;

  // This isn't necessarily POSIX, pal! // midfile = fopen(MIDI_TMPFILE, "wb");
  MIDI_tmpfilename = tmpnam(NULL); // create an unused name
  midfile = fopen(MIDI_tmpfilename, "wb");
  if (midfile == NULL)
    {
      CONS_Printf("Couldn't write MIDI data to a tmpfile\n");
      return 0;
    }

  if (memcmp(data,"MUS",3) == 0)
    {
      // convert mus to mid with a wonderfull function
      // thanks to S.Bacquet for the source of qmus2mid
      // convert mus to mid and load it in memory
      if ((err = qmus2mid((byte *)data, (byte *)musicbuffer, 89, 64, 0, len, MIDBUFFERSIZE, &midlength)) != 0)
	{
	  CONS_Printf("Cannot convert mus to mid, converterror :%d\n",err);
	  return 0;
	}
      fwrite(musicbuffer, 1, midlength, midfile);
    }
  else if (memcmp(data,"MThd",4) == 0)
    {     // support mid file in WAD !!!
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
      CONS_Printf("Couldn't load MIDI from %s: %s\n", MIDI_tmpfilename, Mix_GetError());
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
