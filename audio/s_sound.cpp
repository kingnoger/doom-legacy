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
// Revision 1.2  2002/12/29 18:57:02  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:17:48  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.14  2002/09/20 22:41:25  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.11  2002/09/06 17:18:31  vberghol
// added most of the changes up to RC2
//
// Revision 1.10  2002/08/24 17:26:32  vberghol
// bug fixes
//
// Revision 1.9  2002/08/24 17:25:32  vberghol
// bug fixes
//
// Revision 1.8  2002/08/21 16:58:28  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.7  2002/08/19 18:06:37  vberghol
// renderer somewhat fixed
//
// Revision 1.6  2002/07/18 19:16:34  vberghol
// renamed a few files
//
// Revision 1.5  2002/07/16 19:16:18  vberghol
// Hardware sound interface again somewhat fixed
//
// Revision 1.4  2002/07/01 20:59:48  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:52  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.27  2001/08/20 20:40:39  metzgermeister
// *** empty log message ***
//
// Revision 1.26  2001/05/27 13:42:48  bpereira
// no message
//
// Revision 1.25  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.24  2001/04/18 19:32:26  hurdler
// no message
//
// Revision 1.23  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.22  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.21  2001/04/02 18:54:32  bpereira
// no message
//
// Revision 1.20  2001/04/01 17:35:07  bpereira
// no message
//
// Revision 1.19  2001/03/03 11:11:49  hurdler
// I hate warnigs ;)
//
// Revision 1.18  2001/02/24 13:35:21  bpereira
// no message
//
// Revision 1.17  2001/01/27 11:02:36  bpereira
// no message
//
// Revision 1.16  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.15  2000/11/21 21:13:18  stroggonmeth
// Optimised 3D floors and fixed crashing bug in high resolutions.
//
// Revision 1.14  2000/11/12 21:59:53  hurdler
// Please verify that sound bug
//
// Revision 1.13  2000/11/03 11:48:40  hurdler
// Fix compiling problem under win32 with 3D-Floors and FragglScript (to verify!)
//
// Revision 1.12  2000/11/02 17:50:10  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.11  2000/10/27 20:38:20  judgecutor
// - Added the SurroundSound support
//
// Revision 1.10  2000/09/28 20:57:18  bpereira
// no message
//
// Revision 1.9  2000/05/07 08:27:57  metzgermeister
// no message
//
// Revision 1.8  2000/04/22 16:16:50  emanne
// Correction de l'interface.
// Une erreur s'y était glissé, d'où un segfault si on compilait sans SDL.
//
// Revision 1.7  2000/04/21 08:23:47  emanne
// To have SDL working.
// Makefile: made the hiding by "@" optional. See the CC variable at
// the begining. Sorry, but I like to see what's going on while building
//
// qmus2mid.h: force include of qmus2mid_sdl.h when needed.
// s_sound.c: ??!
// s_sound.h: with it.
// (sorry for s_sound.* : I had problems with cvs...)
//
// Revision 1.6  2000/03/29 19:39:48  bpereira
// no message
//
// Revision 1.5  2000/03/22 18:51:08  metzgermeister
// introduced I_PauseCD() for Linux
//
// Revision 1.4  2000/03/12 23:21:10  linuxcub
// Added consvars which hold the filenames and arguments which will be used
// when running the soundserver and musicserver (under Linux). I hope I
// didn't break anything ... Erling Jacobsen, linuxcub@email.dk
//
// Revision 1.3  2000/03/06 15:13:08  hurdler
// maybe a bug detected
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
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

CV_PossibleValue_t soundvolume_cons_t[]={{0,"MIN"},{31,"MAX"},{0,NULL}};
// actual general (maximum) sound & music volume, saved into the config
consvar_t cv_soundvolume = {"soundvolume","15",CV_SAVE,soundvolume_cons_t};
consvar_t cv_musicvolume = {"musicvolume","15",CV_SAVE,soundvolume_cons_t};

// number of channels available
void SetChannelsNum();
consvar_t cv_numChannels = {"snd_channels","16",CV_SAVE | CV_CALL, CV_Unsigned,SetChannelsNum};

#if defined (HW3SOUND) && !defined (SURROUND)
#define SURROUND
#endif

#ifdef SURROUND
consvar_t surround = {"surround", "0", CV_SAVE, CV_OnOff};
#endif


#define S_MAX_VOLUME            127

// when to clip out sounds
// Does not fit the large outdoor areas.
// added 2-2-98 in 8 bit volume control (befort  (1200*0x10000))
#define S_CLIPPING_DIST         (1200*0x10000)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
// added 2-2-98 in 8 bit volume control (befort  (160*0x10000))
#define S_CLOSE_DIST            (160*0x10000)

// added 2-2-98 in 8 bit volume control (befort  remove the +4)
#define S_ATTENUATOR            ((S_CLIPPING_DIST-S_CLOSE_DIST)>>(FRACBITS+4))

// Adjustable by menu.
//#define NORM_VOLUME             snd_MaxVolume

#define NORM_PITCH              128
#define NORM_PRIORITY           64
#define NORM_SEP                128

#define S_PITCH_PERTURB         1
#define S_STEREO_SWING          (96*0x10000)

#ifdef SURROUND
#define SURROUND_SEP            -128
#endif

// percent attenuation from front to back
#define S_IFRACVOL              30

SoundSystem S;


static int S_AdjustSoundParams(Actor *listener, soundsource_t *source, int vol, int *sep, int *pitch);


SoundSystem::SoundSystem()
{
  mus_playing = NULL;
  mus_paused = false;
  ResetChannels(8, 16);
}


void SoundSystem::SetMusicVolume(int volume)
{
  if (volume < 0 || volume > 31)
    {
      CONS_Printf("Music volume should be between 0-31\n");
      if (volume < 0)
	volume = 0;
      else
	volume = 31;
    }

  CV_SetValue(&cv_musicvolume, volume);
  musicvolume = cv_musicvolume.value;   //check for change of var

  //I_SetMusicVolume(31); //faB: this is a trick for buggy dos drivers.. I think.
  I_SetMusicVolume(volume);
}


void SoundSystem::SetSfxVolume(int volume)
{
  if (volume < 0 || volume > 31)
    {
      CONS_Printf("Sound volume should be between 0-31\n");
      if (volume < 0)
	volume = 0;
      else
	volume = 31;
    }

  CV_SetValue(&cv_soundvolume, volume);
  sfxvolume = cv_soundvolume.value;       //check for change of var

#ifdef HW3SOUND
  hws_mode == HWS_DEFAULT_MODE 
    ? I_SetSfxVolume(volume)
    : HW3S_SetSfxVolume(volume);
#else
  // now hardware volume
  I_SetSfxVolume(volume);
#endif
}



//=================================================================
// Music


// was S_PauseSound
// Stop and resume music, during game PAUSE.
//
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


// was S_Start, S_ChangeMusic etc.
// Starts some music with the music id found in sounds.h.
bool SoundSystem::StartMusic(int m_id, bool loop)
{
  if (m_id > mus_None && m_id < NUMMUSIC)
    return StartMusic(MusicNames[m_id], loop);
  else
    I_Error("Bad music id: %d\n", m_id);

  return false;
}

// FIXME hack: the only "2nd level cached" piece of music
static musicinfo_t mu = {"\0", 0, 0, NULL, 0};

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

  //if (mus_playing == music)
  if (mus_playing && !strcmp(mus_playing->name, name))
    return true;

  // so music needs to be changed.

  // FIXME temp hack: no 2nd level music cache, just one music plays at a time.
  // here we would check if the music 'name' already is in the 2nd level cache.
  // if not, cache it:
  int musiclump = fc.FindNumForName(name);

  if (musiclump < 0)
    {
      CONS_Printf("Music lump '%s' not found!\n", name);
      StopMusic(); // stop music anyway
      return false;
    }
  musicinfo_t *m = &mu; // = new musicinfo_t...

  // shutdown old music
  // TODO: add several music channels, crossfade;)
  StopMusic();

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
  nextcleanup = gametic + 15;
  return true;
}


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


// change number of sound channels
void SoundSystem::ResetChannels(int stat, int dyn)
{
  channels.resize(stat);
  channel3Ds.resize(dyn);

  // stop sounds????

  int i;
  // Free all channels for use
  for (i=0; i<stat; i++)
    channels[i].sfxinfo = NULL;

  for (i=0; i<dyn; i++)
    channel3Ds[i].sfxinfo = NULL;
}

channel_t *SoundSystem::GetChannel(sfxinfo_t *sfx)
{
  // channel number to use
  int i, n = channels.size();

  int min = 1000, chan = -1;

  // Find a free channel
  for (i=0; i<n; i++)
    {
      if (channels[i].sfxinfo == NULL)
	break;
      else if (channels[i].sfxinfo->priority < min)
	{
	  min = channels[i].sfxinfo->priority;
	  chan = i;
	}
    }

  // None available?
  if (i == n)
    {
      // kick out minimum priority sound?
      if (sfx->priority > min)
	{
	  StopChannel(chan);
	  i = chan;
        }
      else
        {
	  // FUCK!  No lower priority.  Sorry, Charlie.
	  return NULL;
        }
    }

  // channel is decided to be i.
  channel_t *c = &channels[i];
  c->sfxinfo = sfx;

  return c;
}

// was S_getChannel
// Tries to make a 3D channel available.
// If none available, returns -1.  Otherwise channel #.
//
channel3D_t *SoundSystem::Get3DChannel(sfxinfo_t *sfx)
{
  // channel number to use
  int i, n = channel3Ds.size();

  int min = 1000, chan = -1;

  // Find a free channel
  for (i=0; i<n; i++)
    {
      if (channel3Ds[i].sfxinfo == NULL)
	break;
      else if (channel3Ds[i].sfxinfo->priority < min)
	{
	  min = channel3Ds[i].sfxinfo->priority;
	  chan = i;
	}
    }

  // None available?
  if (i == n)
    {
      // kick out minimum priority sound?
      if (sfx->priority > min)
	{
	  Stop3DChannel(chan);
	  i = chan;
        }
      else
        {
	  // FUCK!  No lower priority.  Sorry, Charlie.
	  return NULL;
        }
    }

  // channel is decided to be i.
  channel3D_t *c = &channel3Ds[i];
  c->sfxinfo = sfx;

  return c;
}

void SoundSystem::StartAmbSound(sfxinfo_t* sfx, int volume)
{
  if (nosound)
    return;

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

  int sep = NORM_SEP;

  // try to find a channel
  channel_t *c = GetChannel(sfx);

  if (!c)
    return;

  //TODO cache invalidation on linked sounds....how?!
  if (sfx->link)
    sfx->data = sfx->link->data;

  if (!sfx->data)
    I_GetSfx(sfx);

  // increase the reference count (-1 stays -1)
  if (sfx->refcount++ < 0)
    sfx->refcount = -1;
    
  c->handle = I_StartSound(sfx, volume, sep, pitch);
}


// was S_StartSoundAtVolume
void SoundSystem::Start3DSound(soundsource_t* source, sfxinfo_t* sfx, int volume)
{
  if (nosound) // || (source->origin->type == MT_SPIRIT))
    return;

  // Initialize sound parameters
  int pitch;
  if (sfx->link)
    {
      pitch = sfx->pitch;
      volume += sfx->volume;

      if (volume < 1)
        return;
    }
  else
    pitch = NORM_PITCH;

  // Check sound parameters, attenuation and stereo effects.
  int sep = NORM_SEP;

  if (cv_splitscreen.value)
    {
      int sep2 = sep;
      int pitch2 = pitch;
      int volume2 = S_AdjustSoundParams(displayplayer2->pawn, source, volume, &sep2, &pitch2);
      volume = S_AdjustSoundParams(displayplayer->pawn, source, volume, &sep, &pitch);

      if (volume2 > volume)
	{
	  volume = volume2;
	  sep = sep2;
	  pitch = pitch2;
	}
    }
  else
    volume = S_AdjustSoundParams(displayplayer->pawn, source, volume, &sep, &pitch);

  if (!volume)
    return;
 
  // hacks to vary the sfx pitches
  // 64 pitch units = 1 octave

  //added:16-02-98: removed by Fab, because it used M_Random() and it
  //                was a big bug, and then it doesnt change anything
  //                dont hear any diff. maybe I'll put it back later
  //                but of course not using M_Random().


  //added 16-08-02: added back by Judgecutor
  //Sound pitching for both Doom and Heretic
  /*  if (game.mode != heretic)
    {
      if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
	pitch += 8 - (M_Random()&15);
      else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
	pitch += 16 - (M_Random()&31);
    }
  else
    pitch = 128 + (M_Random() & 7) - (M_Random() & 7);
  */
  pitch += 16 - (rand() & 31);

  if (pitch < 0)
    pitch = 0;
  if (pitch > 255)
    pitch = 255;

  // kill old sound if any (max. one sound per source)
  Stop3DSound(source->origin);

  // try to find a channel
  channel3D_t *c = Get3DChannel(sfx);

  if (!c)
    return;

  // copy source data
  c->source = *source;

  // cache data if necessary
  // NOTE : set sfx->data NULL sfx->lump -1 to force a reload
  if (sfx->link)
    sfx->data = sfx->link->data;

  if (!sfx->data)
    {
      I_GetSfx(sfx);
      //CONS_Printf ("cached sound %s\n", sfx->name);
    }

  // increase the reference count (-1 stays -1)
  if (sfx->refcount++ < 0)
    sfx->refcount = -1;
    
#ifdef SURROUND
  // judgecutor:
  // Avoid channel reverse if surround
  if (stereoreverse.value && sep != SURROUND_SEP)
    sep = (~sep) & 255;
#else
  //added:11-04-98:
  if (stereoreverse.value)
    sep = (~sep) & 255;
#endif

  //CONS_Printf("stereo %d reverse %d\n", sep, stereoreverse.value);

  // Assigns the handle to one of the channels in the
  //  mix/output buffer.
  c->handle = I_StartSound(sfx, volume, sep, pitch);
}


// was S_StopSounds
// Kills all positional sounds at start of level
//SoM: Stop all sounds, load level info, THEN start sounds.
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
    if (channel3Ds[cnum].sfxinfo)
      Stop3DChannel(cnum);
}

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
      if (channel3Ds[cnum].sfxinfo && channel3Ds[cnum].source.origin == origin)
	{
	  Stop3DChannel(cnum);
	  break; // what about several sounds from one origin?
	}
    }
}

// was S_StopChannel
void SoundSystem::StopChannel(int cnum)
{

  channel_t *c = &channels[cnum];

  if (c->sfxinfo)
    {
      // stop the sound playing
      if (I_SoundIsPlaying(c->handle))
	I_StopSound(c->handle);

      int i, n = channels.size();
      // check to see if other channels are playing the sound
      for (i=0; i<n; i++)
	if (c->sfxinfo == channels[i].sfxinfo && cnum != i)
	  break;
      // FIXME then what?

      // degrade reference count of sound data
      c->sfxinfo->refcount--;

      c->sfxinfo = NULL;
    }
}

void SoundSystem::Stop3DChannel(int cnum)
{

  channel_t *c = &channel3Ds[cnum];

  if (c->sfxinfo)
    {
      // stop the sound playing
      if (I_SoundIsPlaying(c->handle))
	I_StopSound(c->handle);

      int i, n = channel3Ds.size();
      // check to see if other channels are playing the sound
      for (i=0; i<n; i++)
	if (c->sfxinfo == channel3Ds[i].sfxinfo && cnum != i)
	  break;
      // FIXME then what?

      // degrade reference count of sound data
      c->sfxinfo->refcount--;

      c->sfxinfo = NULL;
    }
}


// was S_UpdateSounds
// Updates music & sounds
void SoundSystem::UpdateSounds()
{
  if (dedicated)
    return;
    
  // Update sound/music volumes, if changed manually at console
  if (sfxvolume != cv_soundvolume.value)
    SetSfxVolume(cv_soundvolume.value);
  if (musicvolume != cv_musicvolume.value)
    SetMusicVolume(cv_musicvolume.value);

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_UpdateSources();
      return;
    }
#endif

  /*
   // TODO Go through 2nd level cache,
   // clean up unused data.
   if (gametic > nextcleanup)
     {
     for (i=1 ; i<NUMSFX ; i++)
     {
     if (S_sfx[i].refcount==0)
     {
     //S_sfx[i].refcount--;

     // don't forget to unlock it !!!
     // __dmpi_unlock_....
     //Z_ChangeTag(S_sfx[i].data, PU_CACHE);
     //S_sfx[i].data = NULL;

     CONS_Printf ("\2flushed sfx %.6s\n", S_sfx[i].name);
     }
     }
     nextcleanup = gametic + 15;
     }*/

  // static sound channels
  int cnum, n = channels.size();
  for (cnum = 0; cnum < n; cnum++)
    {
      channel_t *c = &channels[cnum];
      sfxinfo_t *sfx = c->sfxinfo;
      if (sfx && !I_SoundIsPlaying(c->handle))
	{
	  StopChannel(cnum);
	}
    }
  
  Actor *listener = NULL;
  Actor *listener2 = NULL;
  if (displayplayer)
    listener = displayplayer->pawn;
  if (displayplayer2)
    listener2 = displayplayer2->pawn;

  // 3D sound channels
  n = channel3Ds.size();
  for (cnum = 0; cnum < n; cnum++)
    {
      channel3D_t *c = &channel3Ds[cnum];
      sfxinfo_t *sfx = c->sfxinfo;

      if (sfx)
	if (I_SoundIsPlaying(c->handle))
	  {
	    // initialize parameters
	    int volume = 255;            //8 bits internal volume precision
	    int pitch = NORM_PITCH;

	    if (sfx->link)
	      {
		pitch = sfx->pitch;
		volume += sfx->volume;
		if (volume < 1)
		  {
		    Stop3DChannel(cnum);
		    continue;
		  }
	      }

	    // check non-local sounds for distance clipping
	    //  or modify their params

	    int sep = NORM_SEP;
	    
	    if (cv_splitscreen.value)
	      {
		int sep2 = sep;
		int pitch2 = pitch;
		int volume2 = S_AdjustSoundParams(listener2, &c->source, volume, &sep2, &pitch2);
		volume = S_AdjustSoundParams(listener, &c->source, volume, &sep, &pitch);

		if (volume2 > volume)
		  {
		    volume = volume2;
		    sep = sep2;
		    pitch = pitch2;
		  }
	      }
	    else
	      volume = S_AdjustSoundParams(listener, &c->source, volume, &sep, &pitch);
            
	    if (!volume)
	      Stop3DChannel(cnum);
	    else
	      I_UpdateSoundParams(c->handle, volume, sep, pitch);
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

void SetChannelsNum()
{
  // Allocating the internal channels for mixing
  // (the maximum number of sounds rendered
  // simultaneously) within zone memory.

#ifdef HW3SOUND
  if (hws_mode != HWS_DEFAULT_MODE)
    {
      HW3S_SetSourcesNum();
      return;
    }
#endif
  S.ResetChannels(8, cv_numChannels.value);
}




//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume)
{
  if (dedicated)
    return;
    
  S.SetSfxVolume(sfxVolume);
  S.SetMusicVolume(musicVolume);

  SetChannelsNum();

  int i;
  // Note that sounds have not been cached (yet).
  for (i=1 ; i<NUMSFX ; i++)
    {
      S_sfx[i].data = NULL;
      S_sfx[i].length = 0;
      S_sfx[i].lumpnum = S_sfx[i].refcount = -1;      // for I_GetSfx()
    }

  //  precache sounds if requested by cmdline, or precachesound var true
  if (!nosound && (M_CheckParm("-precachesound") || precachesound.value))
    {
      // Initialize external data (all sounds) at start, keep static.
      CONS_Printf("Loading sounds... ");

      for (i=1 ; i<NUMSFX ; i++)
        {
	  // NOTE: linked sounds use the link's data at StartSound time
	  if (S_sfx[i].name && !S_sfx[i].link)
	    I_GetSfx(&S_sfx[i]); 
        }
      CONS_Printf(" pre-cached all sound data\n");
    }
  //S_InitRuntimeMusic();
}




//  Retrieve the lump number of sfx
//
int S_GetSfxLumpNum(sfxinfo_t *sfx)
{
  // FIXME make this funct. static

  int sfxlump = fc.FindNumForName(sfx->name);
  if (sfxlump > 0)
    return sfxlump;
  else
    return fc.FindNumForName("dsouch");
  // TODO pick a better replacement sound

  /*
  char namebuf[9];

  if( game.mode == heretic )
    strncpy(namebuf, sfx->name, 9);
  else
    sprintf(namebuf, "ds%s", sfx->name); 

  int sfxlump = fc.FindNumForName(namebuf);
  if (sfxlump > 0)
    return sfxlump;

  if( game.mode != heretic )
    strncpy(namebuf, sfx->name, 9);
  else
    sprintf(namebuf, "ds%s", sfx->name);

  sfxlump=fc.FindNumForName(namebuf);
  if (sfxlump>0)
    return sfxlump;

  if( game.mode == heretic)
    return fc.GetNumForName ("keyup");
  else
    return fc.GetNumForName ("dspistol");
  */
}


// Changes stereo-separation and pitch variables
// for a sound effect to be played.
// If the sound is not audible, returns 0.
// Otherwise, modifies parameters and returns sound volume.
static int S_AdjustSoundParams(Actor *listener, soundsource_t *source, int vol, int *sep, int *pitch)
{
  if (!listener)
    return 0;

  // special case, same source and listener, absolute volume
  if (source->origin == listener)
    return vol;

  fixed_t     approx_dist;
  fixed_t     adx;
  fixed_t     ady;
  // calculate the distance to sound origin
  //  and clip it if necessary
  adx = abs(listener->x - source->x);
  ady = abs(listener->y - source->y);

  // From _GG1_ p.428. Approx. euclidean distance fast.
  approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

  if (approx_dist > S_CLIPPING_DIST)
    // && gamemap != 8 // FIXME gamemap doesn't exist anymore. Besides, this is annoying, right?
    return 0;

  // angle of source to listener
  angle_t angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

  if (angle > listener->angle)
    angle -= listener->angle;
  else
    angle += (0xffffffff - listener->angle);

#ifdef SURROUND
  // Produce a surround sound for angle from 105 till 255
  if (surround.value == 1 && (angle > (ANG90 + (ANG45/3)) && angle < (ANG270 - (ANG45/3))))
    *sep = SURROUND_SEP;
  else
    {
#endif

      angle >>= ANGLETOFINESHIFT;

      // stereo separation
      *sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

#ifdef SURROUND
    }
#endif

  // volume calculation
  if (approx_dist < S_CLOSE_DIST)
    {
      vol = 255;
    }
  // removed hack here for gamemap==8 (it made far sound still present)
  else
    {
      // distance effect
      vol = (15 * ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
	/ S_ATTENUATOR;
    }

  // TODO Doppler effect (approximate)
  /*
  float dx, dy, dz;
  dx = (listener->px - source->vx) / FRACUNIT;
  dy = (listener->py - source->vy) / FRACUNIT;
  dz = (listener->pz - source->vz) / FRACUNIT;
  float v_os = sqrt(dx*dx + dy*dy + dz*dz);
  //...and so on. We need a vector class.
  */
  return vol;
}

// FIXME these two functions are used by FS. Make a real 2nd level hashed sound cache.
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
/*
//
// S_StartSoundName
// Starts a sound using the given name.
#define MAXNEWSOUNDS 10
int     newsounds[MAXNEWSOUNDS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void S_StartSoundName(Actor *mo, const char *soundname)
{
  int  i;
  int  soundnum = 0;
  //Search existing sounds...
  for(i = sfx_None + 1; i < NUMSFX; i++)
    {
      if(!S_sfx[i].name)
	continue;
      if(!stricmp(S_sfx[i].name, soundname))
	{
	  soundnum = i;
	  break;
	}
    }

  if(!soundnum)
    {
      for(i = 0; i < MAXNEWSOUNDS; i++)
	{
	  if(newsounds[i] == 0)
	    break;
	  if(!S_SoundPlaying(NULL, newsounds[i]))
	    {S_RemoveSoundFx(newsounds[i]); break;}
	}

      if(i == MAXNEWSOUNDS)
	{
	  CONS_Printf("Cannot load another extra sound!\n");
	  return;
	}

      soundnum = S_AddSoundFx(soundname, false);
      newsounds[i] = soundnum;
    }

  S_StartSound(mo, soundnum);
}
*/

//=========================================================
// TODO things that should be fixed follow this line

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

  S.StartAmbSound(sfx, volume);
}

// wrapper
void S_StartSound(mappoint_t *or, int sfx_id)
{
  soundsource_t s;
  s.x = or->x;
  s.y = or->y;
  s.z = or->z;
  s.vx = s.vy = s.vz = 0;
  s.origin = or;

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
    S.Start3DSound(&s, sfx, 255);
}

// wrapper
void S_StartSound(Actor *or, int sfx_id)
{
  soundsource_t s;
  s.x = or->x;
  s.y = or->y;
  s.z = or->z;
  s.vx = or->px;
  s.vy = or->py;
  s.vz = or->pz;
  s.origin = or;

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
    HW3S_StartSound(or, sfx_id);
  else
#endif
    S.Start3DSound(&s, sfx, 255);
}



// FIXME! sux. See FileCache::Replace()
void S_ReplaceSound(const char *name)
{
  int j;

  for (j=1 ; j<NUMSFX ; j++) {
    if (S_sfx[j].name &&
	!S_sfx[j].link &&
	!strnicmp(S_sfx[j].name, name+2, 6))
      {
	// the sound will be reloaded when needed,
	// since sfx->data will be NULL
	if (devparm)
	  CONS_Printf ("Sound %.8s replaced\n", name);
	
	I_FreeSfx(&S_sfx[j]);
      }
  }
  return;
}


// unused old stuff

/*
void S_InitRuntimeMusic()
{
  int i;

  for(i = mus_firstfreeslot; i < mus_lastfreeslot; i++)
    S_music[i].name = NULL;
}
*/

/*
int S_FindMusic(const char *name)
{ 
  int   i;

  for(i = 0; i < NUMMUSIC; i++)
  {
    if(!S_music[i].name)
      continue;
    if(!stricmp(name, S_music[i].name)) return i;
  }

  return S_AddMusic(name);
  }
*/

/*
//
// S_AddMusic
// Adds a single song to the runtime songs.
int S_AddMusic(const char *name)
{
  int    i;
  char   lumpname[9];

  //sprintf(lumpname, "d_%.6s", name);
  sprintf(lumpname, "%.8s", name);

  for(i = mus_firstfreeslot; i < mus_lastfreeslot; i++)
  {
    if(S_music[i].name == NULL)
    {
      S_music[i].name = Z_Strdup(name, PU_STATIC, 0);
      S_music[i].lumpnum = fc.GetNumForName(lumpname);
      S_music[i].data = 0;
      return i;
    }
  }

  CONS_Printf("All music slots are full!\n");
  return 0;
  }
*/
