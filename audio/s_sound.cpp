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
// Revision 1.5  2003/02/16 16:54:49  smite-meister
// L2 sound cache done
//
// Revision 1.4  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.3  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.2  2002/12/29 18:57:02  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:17:48  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:  
//
//
//-----------------------------------------------------------------------------


// FIXME!  _ALL_ musserv and sndserv stuff has to be moved away! This file has
// nothing to do with the sound output device. Move to linux_x11 interface or whatever.
/*
#ifdef MUSSERV
# include <sys/msg.h>
struct musmsg {
  long msg_type;
  char msg_text[12];
};
extern int msg_id;
#endif
*/

#include <stdlib.h> // rand

#include "doomdef.h"

#include "command.h"
#include "g_actor.h"
#include "g_game.h"
#include "g_pawn.h"
#include "g_player.h"

#include "m_argv.h"
#include "m_random.h"
#include "r_main.h"     //R_PointToAngle2() used to calc stereo sep.
#include "r_things.h"     // for skins
#include "p_info.h"
#include "d_main.h"

#include "i_sound.h"
#include "s_sound.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"
#include "z_cache.h"


// 3D Sound Interface
#include "hardware/hw3sound.h"

// FIXME move to linux_x interface
// commands for music and sound servers
/*
#ifdef MUSSERV
consvar_t musserver_cmd = {"musserver_cmd","musserver",CV_SAVE};
consvar_t musserver_arg = {"musserver_arg","-t 20 -f -u 0",CV_SAVE};
#endif
#ifdef SNDSERV
consvar_t sndserver_cmd = {"sndserver_cmd","llsndserv",CV_SAVE};
consvar_t sndserver_arg = {"sndserver_arg","-quiet",CV_SAVE};
#endif
*/
#ifdef __MACOS__
consvar_t  play_mode = {"play_mode","0",CV_SAVE,CV_Unsigned};
#endif

// stereo reverse 1=true, 0=false
consvar_t stereoreverse = {"stereoreverse","0",CV_SAVE ,CV_OnOff};

// if true, all sounds are loaded at game startup
consvar_t precachesound = {"precachesound","0",CV_SAVE ,CV_OnOff};

// general sound & music volumes, saved into the config
CV_PossibleValue_t soundvolume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};
consvar_t cv_soundvolume = {"soundvolume","15",CV_SAVE,soundvolume_cons_t};
consvar_t cv_musicvolume = {"musicvolume","15",CV_SAVE,soundvolume_cons_t};

// number of channels available
consvar_t cv_numChannels = {"snd_channels","16",CV_SAVE, CV_Unsigned};

#if defined (HW3SOUND) && !defined (SURROUND)
#define SURROUND
#endif

#ifdef SURROUND
consvar_t surround = {"surround", "0", CV_SAVE, CV_OnOff};
#endif


// when to clip out sounds
// Does not fit the large outdoor areas.
const float S_CLIPPING_DIST = 1200;

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: 200.
const float S_CLOSE_DIST = 160;

#define NORM_PITCH              128
#define NORM_PRIORITY           64
#define NORM_SEP                128

#define S_STEREO_SWING          (96*0x10000)

#ifdef SURROUND
#define SURROUND_SEP            -128
#endif


SoundSystem S;


class soundcache_t : public L2cache_t
{
protected:
  virtual cacheitem_t *CreateItem(const char *p);
  virtual void LoadAndConvert(cacheitem_t *t);
public:
  soundcache_t(memtag_t tag);
};

static soundcache_t sc(PU_SOUND);

soundcache_t::soundcache_t(memtag_t tag)
  : L2cache_t(tag)
{}

cacheitem_t *soundcache_t::CreateItem(const char *p)
{
  scacheitem_t *t = new scacheitem_t; // here

  int lump = fc.FindNumForName(p, false);
  if (lump == -1)
    return NULL;

  CONS_Printf(" Sound: %s  |  ", p);

  t->lumpnum = lump;
  t->refcount = 0;
  LoadAndConvert(t);
  
  return t;
}


// We assume that the sound is in Doom sound format (for now).
// TODO: Make it recognize other formats as well!
void soundcache_t::LoadAndConvert(cacheitem_t *r)
{
  scacheitem_t *t = (scacheitem_t *)r;
  t->data = fc.CacheLumpNum(t->lumpnum, tagtype);
  int size = fc.LumpLength(t->lumpnum);

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
    }
#endif

  doomsfx_t *ds = (doomsfx_t *)t->data;
  // TODO: endianness conversion

  CONS_Printf("m = %d, r = %d, s = %d, z = %d, length = %d\n",
	      ds->magic, ds->rate, ds->samples, ds->zero, size);

  t->length = size - 8;  // 8 byte header
  t->sdata = &ds->data;
}


// returns a value in the range [0.0, 1.0]
static float S_ObservedVolume(Actor *listener, soundsource_t *source)
{
  if (!listener)
    return 0;

  // special case, same source and listener, absolute volume
  if (source->origin == listener)
    return 1.0f;

  // calculate the distance to sound origin
  //  and clip it if necessary
  fixed_t adx = abs(listener->x - source->x);
  fixed_t ady = abs(listener->y - source->y);
  fixed_t adz = abs(listener->z - source->z);

  // From _GG1_ p.428. Approx. euclidean distance fast.
  fixed_t dist = adx + ady - ((adx < ady ? adx : ady)>>1);
  dist = dist + adz - ((dist < adz ? dist : adz)>>1);

  if (dist > S_CLIPPING_DIST)
    return 0.0f;

  if (dist < S_CLOSE_DIST)
    return 1.0f;

  // linear distance attenuation
  return (S_CLIPPING_DIST - (dist >> FRACBITS)) / (S_CLIPPING_DIST - S_CLOSE_DIST);
}



// Changes stereo-separation and pitch variables for a channel.
// Otherwise, modifies parameters and returns sound volume.
static int S_AdjustChannel(Actor *l, channel3D_t *c)
{
  if (!l)
    return 0;

  soundsource_t &s = c->source;

  // special case, same source and listener, absolute volume
  if (s.origin == l)
    return c->volume;

  // angle of source to listener
  angle_t angle = R_PointToAngle2(l->x, l->y, s.x, s.y);

  if (angle > l->angle)
    angle -= l->angle;
  else
    angle += (0xffffffff - l->angle);

  int sep;
#ifdef SURROUND
  // Produce a surround sound for angle from 105 till 255
  if (surround.value == 1 && (angle > (ANG90 + (ANG45/3)) && angle < (ANG270 - (ANG45/3))))
    sep = SURROUND_SEP;
  else
    {
#endif
      angle >>= ANGLETOFINESHIFT;

      // stereo separation
      sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

      if (stereoreverse.value)
	sep = (~sep) & 255;
      //CONS_Printf("stereo %d reverse %d\n", sep, stereoreverse.value);
#ifdef SURROUND
    }
#endif

  // TODO Doppler effect (approximate)
  /*
  if (s.origin)
    {
      Actor *orig = s.origin;
      float dx, dy, dz;
      dx = float(l->px - orig->px) / FRACUNIT;
      dy = float(l->py - orig->py) / FRACUNIT;
      dz = float(l->pz - orig->pz) / FRACUNIT;
      float v_os = sqrt(dx*dx + dy*dy + dz*dz);
      float cos_th = 
      //  We need a vector class.
      c->opitch = ...;
    }
  */
  return c->ovol;
}



SoundSystem::SoundSystem()
{
  mus_playing = NULL;
  mus_paused = false;
  soundvolume = musicvolume = -1; // force hardware update
}

// was S_Init
// Initializes sound hardware, sets up channels.
// Sound and music volume is set before first update.
// allocates channel buffer, sets S_sfx lookup.
void SoundSystem::Startup()
{
  if (dedicated)
    return;

  I_StartupSound();
  I_InitMusic();

  ResetChannels(8, 16);

  sc.SetDefaultItem("DSGLOOP"); // default sound

  //  precache sounds if requested by cmdline, or precachesound var true
  if (!nosound && (M_CheckParm("-precachesound") || precachesound.value))
    {
      // Initialize external data (all sounds) at start, keep static.
      CONS_Printf("Precaching sounds... ");

      for (int i=1 ; i<NUMSFX ; i++)
        {
	  // NOTE: linked sounds use the link's data at StartSound time
	  if (S_sfx[i].name && !S_sfx[i].link)
	    sc.Cache(S_sfx[i].name); // one extra reference => never released
	    //I_GetSfx(&S_sfx[i]); 
        }
      CONS_Printf(" pre-cached all sound data\n");
    }

  nextcleanup = gametic + 100;
}


void SoundSystem::SetMusicVolume(int vol)
{
  if (vol < 0 || vol > 31)
    {
      CONS_Printf("Music volume should be between 0-31\n");
      if (vol < 0)
	vol = 0;
      else
	vol = 31;
    }

  musicvolume = vol;
  I_SetMusicVolume(vol);
}


void SoundSystem::SetSoundVolume(int vol)
{
  if (vol < 0 || vol > 31)
    {
      CONS_Printf("Sound volume should be between 0-31\n");
      if (vol < 0)
	vol = 0;
      else
	vol = 31;
    }

  soundvolume = vol;
#ifdef HW3SOUND
  hws_mode == HWS_DEFAULT_MODE ? I_SetSfxVolume(vol) : HW3S_SetSfxVolume(vol);
#else
  // now hardware volume
  I_SetSfxVolume(vol);
#endif
}



//=================================================================
// Music

//--------------------------------------------
// was S_PauseSound
// Stop and resume music, during game PAUSE.
void SoundSystem::PauseMusic()
{
  if (mus_playing && !mus_paused)
    {
      I_PauseSong(mus_playing->handle);
      mus_paused = true;
    }

  // pause cd music
  I_PauseCD();
}

//--------------------------------------------
// was S_ResumeSound
void SoundSystem::ResumeMusic()
{
  if (mus_playing && mus_paused)
    {
      I_ResumeSong(mus_playing->handle);
      mus_paused = false;
    }

  // resume cd music
  I_ResumeCD();
}


// FIXME hack: the only "2nd level cached" piece of music
static musicinfo_t mu = {"\0", 0, 0, NULL, 0};

//--------------------------------------------
// was S_ChangeMusicName
// caches music lump "name", starts playing it
// returns true if succesful
bool SoundSystem::StartMusic(const char *name, bool loop)
{
  if (dedicated || nomusic)
    return false;

  // FIXME what is this? And why?
  if (!strncmp(name, "-", 6))
    {
      StopMusic();
      return true;
    }

  if (mus_playing && !strcmp(mus_playing->name, name))
    return true;

  // so music needs to be changed.

  // shutdown old music
  // TODO: add several music channels, crossfade;)
  StopMusic();

  // FIXME temp hack: no 2nd level music cache, just one music plays at a time.
  // here we would check if the music 'name' already is in the 2nd level cache.
  // if not, cache it:
  int musiclump = fc.FindNumForName(name);

  if (musiclump < 0)
    {
      CONS_Printf("Music lump '%s' not found!\n", name);
      return false;
    }

  musicinfo_t *m = &mu;
  strcpy(m->name, name);
  m->lumpnum = musiclump;
  m->data = (void *)fc.CacheLumpNum(musiclump, PU_MUSIC);
  m->length = fc.LumpLength(musiclump);

#ifdef __MACOS__
  // FIXME make Mac interface similar to the other interfaces
  m->handle = I_RegisterSong(music_num);
#else
  m->handle = I_RegisterSong(m->data, m->length);
#endif

  // FIXME move to linux_x interface
  /*
#ifdef MUSSERV
  if (msg_id != -1) {
    struct musmsg msg_buffer;

    msg_buffer.msg_type=6;
    memset(msg_buffer.msg_text,0,sizeof(msg_buffer.msg_text));
    sprintf(msg_buffer.msg_text,"%s", music->name);
    msgsnd(msg_id,(struct msgbuf *)&msg_buffer,sizeof(msg_buffer.msg_text),IPC_NOWAIT);
  }
#endif
  */

  // play it
  I_PlaySong(m->handle, loop);

  mus_paused = false;
  mus_playing = m;
  return true;
}

//--------------------------------------------
// was S_StopMusic
void SoundSystem::StopMusic()
{
  if (mus_playing)
    {
      if (mus_paused)
	I_ResumeSong(mus_playing->handle);

      I_StopSong(mus_playing->handle);
      I_UnRegisterSong(mus_playing->handle);
      Z_ChangeTag(mus_playing->data, PU_CACHE);

      mus_playing->data = NULL;
      mus_playing = NULL;
    }
}


//=================================================================
// Sound effects

//--------------------------------------------
// change number of sound channels
void SoundSystem::ResetChannels(int stat, int dyn)
{
  int i, n = channels.size();
  for (i=stat; i<n; i++)
    StopChannel(i);

  channels.resize(stat);

  for (i=n; i<stat; i++)
    {
      channels[i].cip = NULL;
      channels[i].playing = false;
    }

  n = channel3Ds.size();
  for (i=dyn; i<n; i++)
    Stop3DChannel(i);

  channel3Ds.resize(dyn);

  for (i=n; i<dyn; i++)
    {
      channel3Ds[i].cip = NULL;
      channel3Ds[i].playing = false;
    }
}

//--------------------------------------------
// Tries to make a sound channel available.
int SoundSystem::GetChannel(int pri)
{
  // channel number to use
  int i, n = channels.size();

  int min = 1000, chan = -1;

  // Find a free channel
  for (i=0; i<n; i++)
    {
      if (channels[i].cip == NULL)
	break;
      else if (channels[i].priority < min)
	{
	  min = channels[i].priority;
	  chan = i;
	}
    }

  // None available?
  if (i == n)
    {
      // kick out minimum priority sound?
      if (pri > min)
	{
	  StopChannel(chan);
	  i = chan;
        }
      else
        {
	  // FUCK!  No lower priority.  Sorry, Charlie.
	  return -1;
        }
    }

  // channel i is chosen
  return i;
}

//--------------------------------------------
// was S_getChannel
// Tries to make a 3D channel available.
// If none available, returns -1.  Otherwise channel #.
int SoundSystem::Get3DChannel(int pri)
{
  // channel number to use
  int i, n = channel3Ds.size();

  int min = 1000, chan = -1;

  // Find a free channel
  for (i=0; i<n; i++)
    {
      if (channel3Ds[i].cip == NULL)
	break;
      else if (channel3Ds[i].priority < min)
	{
	  min = channel3Ds[i].priority;
	  chan = i;
	}
    }

  // None available?
  if (i == n)
    {
      // kick out minimum priority sound?
      if (pri > min)
	{
	  Stop3DChannel(chan);
	  i = chan;
        }
      else
        {
	  // FUCK!  No lower priority.  Sorry, Charlie.
	  return -1;
        }
    }

  // i it is.
  return i;
}

//--------------------------------------------
// Starts a normal mono(stereo) sound
void SoundSystem::StartAmbSound(const char *name, int volume, int separation, int pitch, int pri)
{
  if (nosound)
    return;

  // try to find a channel
  int i = GetChannel(pri);
  if (i == -1)
    return;

  channel_t *c = &channels[i];

  // copy source data
  c->ovol= c->volume = volume;
  c->opitch = c->pitch = pitch;
  c->osep = separation;
  c->priority = pri;

  c->cip = (scacheitem_t *)sc.Cache(name);

  I_StartSound(c);
}

//--------------------------------------------
// was S_StartSoundAtVolume
void SoundSystem::Start3DSound(const char *name, soundsource_t *source, int volume, int pitch, int pri)
{
  if (nosound)
    return;

  Actor *listener = NULL;
  if (displayplayer)
    listener = displayplayer->pawn;

  float v1 = S_ObservedVolume(listener, source);
  if (cv_splitscreen.value && displayplayer2)
    {
      float v2 = S_ObservedVolume(displayplayer2->pawn, source);
      if (v2 > v1)
	{
	  listener = displayplayer2->pawn;
	  v1 = v2;
	}
    }

  if (v1 <= 0.0f)
    return;

  // kill old sound if any (max. one sound per source)
  if (source->mpoint == NULL)
    Stop3DSound(source->origin);
  else
    Stop3DSound(source->mpoint);

  // try to find a channel
  int i = Get3DChannel(pri);
  if (i == -1)
    return;

  channel3D_t *c = &channel3Ds[i];

  // 64 pitch units = 1 octave
  pitch += 16 - (rand() & 31);

  if (pitch < 0)
    pitch = 0;
  if (pitch > 255)
    pitch = 255;

  // copy source data
  c->volume = volume;
  c->pitch = pitch;
  c->priority = pri;
  c->source = *source;

  c->ovol = int(volume * v1);
  c->opitch = pitch;
  c->osep = 128;

  // Check sound parameters, attenuation and stereo effects.
  S_AdjustChannel(listener, c);

  c->cip = (scacheitem_t *)sc.Cache(name);

  I_StartSound(c);
}


//--------------------------------------------
// was S_StopSounds
// Kills all positional sounds
void SoundSystem::Stop3DSounds()
{

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_StopSounds();
      return;
    }
#endif

  // kill all playing sounds at start of level
  //  (trust me - a good idea)
  int cnum, n = channel3Ds.size();

  for (cnum = 0; cnum < n; cnum++)
    if (channel3Ds[cnum].cip)
      Stop3DChannel(cnum);
}

//--------------------------------------------
// was S_StopSound
void SoundSystem::Stop3DSound(void *origin)
{
  // SoM: Sounds without origin can have multiple sources, they shouldn't
  // be stopped by new sounds.
  if (!origin)
    return;

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_StopSound(origin);
      return;
    }
#endif

  int cnum, n = channel3Ds.size();
  for (cnum=0; cnum<n; cnum++)
    {
      if (channel3Ds[cnum].cip && channel3Ds[cnum].source.origin == origin)
	{
	  Stop3DChannel(cnum);
	  break; // what about several sounds from one origin?
	}
    }
}

//--------------------------------------------
// was S_StopChannel
void SoundSystem::StopChannel(int cnum)
{

  channel_t *c = &channels[cnum];

  if (c->cip)
    {
      I_StopSound(c);

      // degrade reference count of sound data
      c->cip->Release();
      c->cip = NULL;
    }
  c->playing = false;
}

//--------------------------------------------
void SoundSystem::Stop3DChannel(int cnum)
{

  channel_t *c = &channel3Ds[cnum];

  if (c->cip)
    {
      I_StopSound(c);

      // degrade reference count of sound data
      c->cip->Release();
      c->cip = NULL;
    }
  c->playing = false;
}

//--------------------------------------------
// was S_UpdateSounds
// Updates music & sounds, called once a gametic
void SoundSystem::UpdateSounds()
{
  if (dedicated)
    return;
    
  // check if relevant consvars have been changed
  if (soundvolume != cv_soundvolume.value)
    SetSoundVolume(cv_soundvolume.value);
  if (musicvolume != cv_musicvolume.value)
    SetMusicVolume(cv_musicvolume.value);

  if (channel3Ds.size() != cv_numChannels.value)
    {
#ifdef HW3SOUND
      if (hws_mode != HWS_DEFAULT_MODE)
	HW3S_SetSourcesNum();
#endif
      S.ResetChannels(8, cv_numChannels.value);
    }

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_UpdateSources();
      return;
    }
#endif

  // Go through L2 cache,
  // clean up unused data.
  if (gametic > nextcleanup)
    {
      CONS_Printf("Sound cache cleanup...\n");
      sc.Inventory();

      int i = sc.Cleanup();
      if (i > 0)
	CONS_Printf ("Flushed %d sounds\n", i);
      sc.Inventory();
      nextcleanup = gametic + 100;
    }

  // static sound channels
  int cnum, n = channels.size();

  for (cnum = 0; cnum < n; cnum++)
    {
      channel_t *c = &channels[cnum];
      if (!c->playing)
        StopChannel(cnum);
    }

  // 3D sound channels
  Actor *listener1 = NULL, *listener2 = NULL;

  if (displayplayer)
    listener1 = displayplayer->pawn;
  if (cv_splitscreen.value && displayplayer2)
    listener2 = displayplayer2->pawn;

  if (!listener1 && !listener2)
    return;

  Actor *listener;

  n = channel3Ds.size();
  for (cnum = 0; cnum < n; cnum++)
    {
      channel3D_t *c = &channel3Ds[cnum];

      if (c->playing)
	{
	  // check non-local sounds for distance clipping
	  //  or modify their params
	  float v1 = S_ObservedVolume(listener1, &c->source);
	  float v2 = S_ObservedVolume(listener2, &c->source);
	  if (v2 > v1)
	    {
	      listener = listener2;
	      v1 = v2;
	    }
	  else
	    listener = listener1;
            
	  if (v1 <= 0.0)
	    Stop3DChannel(cnum);
	  else
	    {
	      c->ovol = int(c->volume * v1);
	      S_AdjustChannel(listener, c);
	    }
	}
      else
	// if channel is allocated but sound has stopped, free it
	Stop3DChannel(cnum);
    }
}


//=====================================================================
// non-class functions

void S_RegisterSoundStuff()
{
  if (dedicated)
    return;
    
  //added:11-04-98: stereoreverse
  CV_RegisterVar(&stereoreverse);
  CV_RegisterVar(&precachesound);

#ifdef SNDSERV
  CV_RegisterVar(&sndserver_cmd);
  CV_RegisterVar(&sndserver_arg);
#endif
#ifdef MUSSERV
  CV_RegisterVar(&musserver_cmd);
  CV_RegisterVar(&musserver_arg);
#endif
#ifdef SURROUND
  CV_RegisterVar(&surround);
#endif

#ifdef __MACOS__        //mp3 playlist stuff
  {
    int i;
    for (i=0;i<PLAYLIST_LENGTH;i++)
      {
	user_songs[i].name = malloc(7);
	sprintf(user_songs[i].name, "song%i%i",i/10,i%10);
	user_songs[i].defaultvalue = malloc(1);
	*user_songs[i].defaultvalue = 0;
	user_songs[i].flags = CV_SAVE;
	user_songs[i].PossibleValue = NULL;
	CV_RegisterVar (&user_songs[i]);
      }
    CV_RegisterVar (&play_mode);
  }
#endif
}



/*
//  Retrieve the lump number of sfx
//
int S_GetSfxLumpNum(sfxinfo_t *sfx)
{
  int sfxlump = fc.FindNumForName(sfx->name);
  if (sfxlump > 0)
    return sfxlump;
  else
    return fc.FindNumForName("dsouch");
  // TODO pick a better replacement sound
}
*/


/*
// SoM: Searches through the channels and checks for origin or id.
// returns 0 of not found, returns 1 if found.
// if id == -1, the don't check it...
int S_SoundPlaying(void *origin, int id)
{
  int         cnum;

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      return HW3S_SoundPlaying(origin, id);
    }
#endif

  for (cnum=0 ; cnum<cv_numChannels.value ; cnum++)
    {
      if (origin &&  channels[cnum].origin ==  origin)
	return 1;
      if (id != -1 && channels[cnum].sfxinfo - S_sfx == id)
	return 1;
    }
  return 0;
}
*/


//=========================================================
// wrappers for original hardwired sounds (in S_sfx array)


//--------------------------------------------
// was S_Start, S_ChangeMusic etc.
// Starts some music with the music id found in sounds.h.
bool S_StartMusic(int m_id, bool loop)
{
  if (m_id > mus_None && m_id < NUMMUSIC)
    return S.StartMusic(MusicNames[m_id], loop);
  else
    I_Error("Bad music id: %d\n", m_id);

  return false;
}


// wrapper
void S_StartAmbSound(int sfx_id, int volume)
{
#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      volume += 30;
      if (volume > 255)
	volume = 255;
      HW3S_StartSoundTypeAtVolume(NULL, sfx_id, CT_AMBIENT, volume);
      return;
    }
#endif

#ifdef PARANOIA
  // check for bogus sound #
  if (sfx_id < 1 || sfx_id >= NUMSFX)
    I_Error("Bad sfx number: %d\n", sfx_id);
#endif

  sfxinfo_t *sfx = &S_sfx[sfx_id];

  // Initialize sound parameters
  int pitch;
  if (sfx->link)
    {
      pitch = sfx->pitch;
      volume += sfx->volume;
    }
  else
    pitch = NORM_PITCH;

  if (volume < 1)
    return;

  const char *name = sfx->name;
  if (sfx->link)
    name = sfx->link->name;

  S.StartAmbSound(name, volume, NORM_SEP, pitch, sfx->priority);
}

// wrapper
static void S_Start3DSound(sfxinfo_t *sfx, soundsource_t *source)
{
  int pitch, volume = 255;

  // Initialize sound parameters
  if (sfx->link)
    {
      pitch = sfx->pitch;
      volume += sfx->volume;

      if (volume < 1)
        return;
    }
  else
    pitch = NORM_PITCH;

  const char *name = sfx->name;
  // just one layer of links is accepted
  if (sfx->link)
    name = sfx->link->name;

  // NOTE: sfx->singularity is completely ignored!
  S.Start3DSound(name, source, volume, pitch, sfx->priority);
}

// wrapper
void S_StartSound(mappoint_t *m, int sfx_id)
{
  soundsource_t s;
  s.x = m->x;
  s.y = m->y;
  s.z = m->z;
  s.mpoint = m;
  s.origin = NULL;

#ifdef PARANOIA
  // check for bogus sound #
  if (sfx_id < 1 || sfx_id >= NUMSFX)
    I_Error("Bad sfx number: %d\n", sfx_id);
#endif

  sfxinfo_t *sfx = &S_sfx[sfx_id];


#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    HW3S_StartSound(NULL, sfx_id);
  else
#endif
    S_Start3DSound(sfx, &s);
}

// wrapper
void S_StartSound(Actor *orig, int sfx_id)
{
  soundsource_t s;
  s.x = orig->x;
  s.y = orig->y;
  s.z = orig->z;
  s.mpoint = NULL;
  s.origin = orig;

#ifdef PARANOIA
  // check for bogus sound #
  if (sfx_id < 1 || sfx_id >= NUMSFX)
    I_Error("Bad sfx number: %d\n", sfx_id);
#endif

  sfxinfo_t *sfx = &S_sfx[sfx_id];

  /* FIXME skins are temporarily removed until a better system is made
    if (sfx->skinsound!=-1 && origin && origin->skin)
    {
    // it redirect player sound to the sound in the skin table
    sfx_id = ((skin_t *)origin->skin)->soundsid[sfx->skinsound];
    sfx    = &S_sfx[sfx_id];
    }
  */

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    HW3S_StartSound(orig, sfx_id);
  else
#endif
    S_Start3DSound(sfx, &s);
}
