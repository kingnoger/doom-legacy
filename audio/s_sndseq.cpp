// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003 by DooM Legacy Team.
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
// Revision 1.3  2003/04/24 20:29:57  hurdler
// Remove lots of compiling warnings
//
// Revision 1.2  2003/04/20 16:45:49  smite-meister
// partial SNDSEQ fix
//
// Revision 1.1  2003/04/19 17:38:46  smite-meister
// SNDSEQ support, tools, linedef system...
//
//
//
// DESCRIPTION:  
//   Sound sequences. Incorporates Hexen sequences, Heretic ambient sequences
//   and Legacy-specific sequences.
//
//-----------------------------------------------------------------------------

#include <vector>
#include <string.h>

#include "m_random.h"

#include "g_map.h"
#include "s_sound.h"
#include "sounds.h"
#include "s_sndseq.h"
#include "w_wad.h"
#include "z_zone.h"

using namespace std;

/*
typedef enum
{
  SEQTYPE_STONE,
  SEQTYPE_HEAVY,
  SEQTYPE_METAL,
  SEQTYPE_CREAK,
  SEQTYPE_SILENCE,
  SEQTYPE_LAVA,
  SEQTYPE_WATER,
  SEQTYPE_ICE,
  SEQTYPE_EARTH,
  SEQTYPE_METAL2,
  SEQTYPE_NUMSEQ
} seqtype_t;
*/


// This sucks, but is necessary for Hexen
struct hexen_seq_t
{
  const char *tag;
  int seq[3]; // max 3 are needed
};

hexen_seq_t HexenSeqs[NUMSEQ] =
{
  { "Platform", {0, 1, 3}}, // 'heavy' and 'creak' platforms are just platforms
  { "PlatformMetal", {2, -1, -1}},
  { "Silence", {4, 14, -1}},
  { "Lava",    {5, 15, -1}},
  { "Water",   {6, 16, -1}},
  { "Ice",     {7, 17, -1}},
  { "Earth",   {8, 18, -1}},
  { "PlatformMetal2", {9, -1, -1}},
  { "DoorNormal", {10, -1, -1}},
  { "DoorHeavy",  {11, -1, -1}},
  { "DoorMetal",  {12, -1, -1}},
  { "DoorCreak",  {13, -1, -1}},
  { "DoorMetal2", {19, -1, -1}},
  { "Wind", {20, -1, -1}}
};

// both an index to SOUNDSEQ_cmds and the actual token used in sequence
enum soundseq_cmd_t
{
  SSEQ_NONE,
  SSEQ_PLAY,
  SSEQ_PLAYUNTILDONE,
  SSEQ_WAITUNTILDONE = SSEQ_PLAYUNTILDONE, // used by PLAYUNTILDONE
  SSEQ_PLAYTIME,
  SSEQ_PLAYREPEAT,
  SSEQ_DELAY,
  SSEQ_DELAYRAND,
  SSEQ_VOLUME,
  SSEQ_STOPSOUND,
  SSEQ_END,

  // Legacy additions
  SSEQ_VOLUMERAND,
  SSEQ_CHVOL,

  /*
  SSEQ_ATTENUATION,
  SSEQ_PLAYLOOP,
  SSEQ_DOOR,
  SSEQ_PLATFORM,
  SSEQ_ENVIRONMENT,
  SSEQ_NOSTOPCUTOFF,
  */
  SSEQ_NUMCMDS
};

static const char *SOUNDSEQ_cmds[SSEQ_NUMCMDS + 1] =
{
  "xxx",
  "play",
  "playuntildone",
  "playtime",
  "playrepeat",
  "delay",
  "delayrand",
  "volume",
  "stopsound",
  "end",

  "volumerand", // like delayrand but for volume
  "chvol",      // additive volume adjustment
  /* // zdoom additions
    "attenuation",
    "playloop",
    "door",
    "platform",
    "environment",
    "nostopcutoff",
  */
  NULL
};

// static data
struct sndseq_t
{
  int  number;
  char name[28];
  int  stopsound;
  int  length; // instructions
  int  seq[0]; // data begins here

public:
  void clear()
  {
    number  = 0;
    name[0] = '\0';
    stopsound = 0;
    length = 0;
  };
};

// dynamic data
class ActiveSndSeq
{
  friend class Map;

  const sndseq_t *sequence;
  const int *ip;
  int   delay;
  float volume;
  int   currentsound;
  int   channel;
  int   stopsound;
  bool  isactor;
  union
  { // the sound origin. if NULL, the sound is ambient
    mappoint_t *mpt;
    Actor      *act;
  };

public:

  ActiveSndSeq(sndseq_t *s, Actor *orig)
  {
    channel = -1;
    sequence = s;
    ip = s->seq;
    delay = 0;
    volume = 1.0f;
    currentsound = 0;
    stopsound = s->stopsound;
    act = orig;
    isactor = true;
  };

  ActiveSndSeq(sndseq_t *s, mappoint_t *orig)
  {
    channel = -1;
    sequence = s;
    ip = s->seq;
    delay = 0;
    volume = 1.0f;
    currentsound = 0;
    stopsound = s->stopsound;
    mpt = orig;
    isactor = false;
  };

  bool Update();
  void StartSnd(int snd);
  void Stop();

  void *operator new(size_t size) { return Z_Malloc(size, PU_LEVSPEC, NULL); };
  void  operator delete(void *mem) { Z_Free(mem); };
};


void ActiveSndSeq::StartSnd(int snd)
{
  // TODO for now, sequences can only use sounds that are in S_sfx
  if (act)
    {
      if (isactor)
	channel = S_StartSound(act, snd, volume);
      else
	channel = S_StartSound(mpt, snd, volume);
    }
  else
    channel = S.StartAmbSound(S_sfx[snd].lumpname, volume);
}

void ActiveSndSeq::Stop()
{
  S.StopChannel(channel);
  if (stopsound)
    StartSnd(stopsound);
}

bool Map::SN_StartSequence(Actor *a, unsigned s)
{
  map<unsigned, struct sndseq_t*>::iterator i = SoundSeqs.find(s);

  if (i == SoundSeqs.end())
    return false;

  SN_StopSequence(a); // Stop any previous sequence
  ActiveSndSeq *temp = new ActiveSndSeq((*i).second, a);
  ActiveSeqs.push_back(temp);
  return true;
}

bool Map::SN_StartSequence(mappoint_t *m, unsigned s)
{
  map<unsigned, struct sndseq_t*>::iterator i = SoundSeqs.find(s);

  if (i == SoundSeqs.end())
    return false;

  SN_StopSequence(m); // Stop any previous sequence
  ActiveSndSeq *temp = new ActiveSndSeq((*i).second, m);
  ActiveSeqs.push_back(temp);

  return true;
}


bool Map::SN_StartSequenceName(Actor *a, const char *name)
{
  map<unsigned, struct sndseq_t*>::iterator i;

  for (i = SoundSeqs.begin(); i != SoundSeqs.end(); i++)
    {
      if (!strcmp(name, (*i).second->name))
	{
	  SN_StartSequence(a, (*i).first);
	  return true;
	}
    }
  return false;
}


bool Map::SN_StopSequence(void *origin)
{
  list<ActiveSndSeq*>::iterator i;

  for (i = ActiveSeqs.begin(); i != ActiveSeqs.end(); i++)
    {
      if ((*i)->act == origin)
	{
	  (*i)->Stop();
	  ActiveSeqs.erase(i);
	  return true;
	}
    }
  return false;
}


void Map::UpdateSoundSequences()
{
  list<ActiveSndSeq*>::iterator i, j;

  for (i = ActiveSeqs.begin(); i != ActiveSeqs.end(); )
    {
      if ((*i)->Update())
	{
	  // finished
	  j = i++; // because erasing invalidates it
	  delete *j;
	  ActiveSeqs.erase(j);
	}
      else
	i++;
    }

  // ambient sequence
  int n = AmbientSeqs.size();

  if (n == 0)
    return;

  if (ActiveAmbientSeq)
    {
      if (ActiveAmbientSeq->Update())
	{
	  // ambient sequence finished, pick a new one next tick
	  delete ActiveAmbientSeq;
	  ActiveAmbientSeq = NULL;
	}
    }
  else
    ActiveAmbientSeq = new ActiveSndSeq(AmbientSeqs[P_Random()%n], (Actor *)NULL);
}


bool ActiveSndSeq::Update()
{
  if (delay > 0)
    {
      delay--;
      return false;
    }

  bool playing = S.ChannelPlaying(channel);

  switch (*ip)
    {
    case SSEQ_PLAY:
      if (!playing)
	{
	  currentsound = ip[1];
	  StartSnd(currentsound);
	}
      ip += 2;
      break;

    case SSEQ_WAITUNTILDONE:
      if (!playing)
	{
	  currentsound = 0;
	  ip++;
	}
      break;

    case SSEQ_PLAYREPEAT:
      if (!playing)
	{
	  currentsound = ip[1];
	  StartSnd(currentsound); // TODO should make looping sound
	}
      break;

    case SSEQ_DELAY:
      delay = ip[1];
      ip += 2;
      currentsound = 0;
      break;

    case SSEQ_DELAYRAND:
      delay = ip[1] + P_Random() % (ip[2] - ip[1] + 1);
      ip += 3;
      currentsound = 0;
      break;

    case SSEQ_VOLUME:
      // volume is in range 0..100
      volume = float(ip[1])/100;
      ip += 2;
      break;

    case SSEQ_STOPSOUND:
      // Wait until something else stops the sequence
      break;

    case SSEQ_END:
      S.StopChannel(channel);
      return true;

    case SSEQ_VOLUMERAND:
      volume = float(ip[1] + P_Random() % (ip[2] - ip[1] + 1))/100;
      ip += 3;
      break;

    case SSEQ_CHVOL:
      volume += float(ip[1])/100;
      ip += 2;
      break;


    default:	
      break;
    }

  return false;
}


static int P_MatchString(const char *p, const char *strings[])
{
  int i;
  for (i=0; strings[i]; i++)
    if (!strcasecmp(p, strings[i]))
      return i;

  return -1;
}

static int P_GetString(char **ptr, char *buf)
{
  char *p = *ptr;
  // get a max. 40 character string, ignoring starting whitespace
  while (isspace(*p))
    p++;

  int i = 0;
  while (*p && !isspace(*p) && i < 40)
    {
      *buf = *p;
      buf++, p++, i++;
    }
  *buf = '\0';
  *ptr = p;
  return i;
}

static int P_GetInt(char **ptr)
{
  char *p = *ptr;
  char *tail = NULL;
  int val = strtol(p, &tail, 0);
  if (tail == p)
    {
      CONS_Printf("SNDSEQ: Expected an integer, got '%s'.\n", p);
      return 0;
    }

  *ptr = tail;
  return val;
}



// reads the SNDINFO lump
void Map::S_Read_SNDSEQ(int lump)
{
  if (lump == -1)
    return;

  CONS_Printf("Reading SNDSEQ...\n");

  int length = fc.LumpLength(lump);
  char *ms = (char *)fc.CacheLumpNum(lump, PU_STATIC);
  char *me = ms + length; // past-the-end pointer

  char *s, *p;
  s = p = ms;

  vector<int> script;
  sndseq_t temp;
  temp.number = -1;
  int i, n, hseq = -1;;

  char line[45];

  while (p < me)
    {
      if (*p == '\n') // line ends
	{
	  if (p > s)
	    {
	      // parse the line from s to p
	      *p = '\0';  // mark the line end

	      n = P_GetString(&s, line);
	      if (n > 0 && line[0] != ';') // not a blank line or comment
		{
		  if (line[0] == ':')
		    {
		      if (temp.number != -1)
			CONS_Printf("SNDSEQ: Nested sequence in sequence %d.\n", temp.number);
		      else
			{
			  // new sequence
			  script.clear();
			  temp.clear();
			  strncpy(temp.name, line, 25);
			  temp.name[25] = '\0'; // to be sure

			  if (n >= 2)
			    temp.number = P_GetInt(&s);
			  else
			    {
			      // old Hexen style kludge
			      for (i=0; i<NUMSEQ; i++)
				if (!strcasecmp(HexenSeqs[i].tag, line+1))
				  break;
			      if (i == NUMSEQ)
				{
				  CONS_Printf("SNDSEQ: No sequence number given for '%s'.\n", line);
				  temp.number = -2;
				}
			      else
				{
				  temp.number = HexenSeqs[i].seq[0];
				  hseq = i;
				}
			    }
			}
		    }
		  else if (temp.number != -1)
		    switch (P_MatchString(line, SOUNDSEQ_cmds))
		      {
		      case SSEQ_PLAY:
			if (P_GetString(&s, line))
			  {
			    script.push_back(SSEQ_PLAY);
			    script.push_back(S_GetSoundID(line));
			  }
			break;

		      case SSEQ_PLAYUNTILDONE:
			if (P_GetString(&s, line))
			  {
			    script.push_back(SSEQ_PLAY);
			    script.push_back(S_GetSoundID(line));
			    script.push_back(SSEQ_WAITUNTILDONE);
			  }
			break;

		      case SSEQ_PLAYTIME:
			if (P_GetString(&s, line))
			  {
			    script.push_back(SSEQ_PLAY);
			    script.push_back(S_GetSoundID(line));
			    script.push_back(SSEQ_DELAY);
			    script.push_back(P_GetInt(&s));
			  }

		      case SSEQ_PLAYREPEAT:
			if (P_GetString(&s, line))
			  {
			    script.push_back(SSEQ_PLAYREPEAT);
			    script.push_back(S_GetSoundID(line));
			  }
			break;

		      case SSEQ_DELAY:
			script.push_back(SSEQ_DELAY);
			script.push_back(P_GetInt(&s));
			break;


		      case SSEQ_DELAYRAND:
			script.push_back(SSEQ_DELAYRAND);
			script.push_back(P_GetInt(&s));
			script.push_back(P_GetInt(&s));
			break;

		      case SSEQ_VOLUME:
			script.push_back(SSEQ_VOLUME);
			script.push_back(P_GetInt(&s));
			break;

		      case SSEQ_STOPSOUND:
			if (P_GetString(&s, line))
			  {
			    script.push_back(SSEQ_STOPSOUND);
			    temp.stopsound = S_GetSoundID(line);
			  }
			break;

		      case SSEQ_END:
			script.push_back(SSEQ_END);
			if (temp.number >= 0)
			  {
			    // create and store the sequence
			    temp.length = script.size();
			    CONS_Printf("crash\n");
			    sndseq_t *tempseq = (sndseq_t *)Z_Malloc(sizeof(sndseq_t) + script.size(), PU_STATIC, NULL);
			    CONS_Printf("neverseen\n");
			    *tempseq = temp; // copy the fields

			    for (n=0; n < (int)script.size(); n++)
			      tempseq->seq[n] = script[n];

			    if (SoundSeqs.count(temp.number)) // already there
			      {
				CONS_Printf("SNDSEQ: Sequence %d defined more than once!\n", temp.number);
				Z_Free(SoundSeqs[temp.number]); // later one takes precedence
			      }
			    SoundSeqs[temp.number] = tempseq; // insert into the map

			    if (hseq >= 0)
			      { // other half of the Hexen kludge:
				// some sequences need to be copied
				for (n=1; n<3; n++)
				  if (HexenSeqs[hseq].seq[n] != -1)
				    SoundSeqs[HexenSeqs[hseq].seq[n]] = tempseq;

				hseq = -1;
			      }
			  }

			temp.number = -1;
			break;

		      case SSEQ_VOLUMERAND:
			script.push_back(SSEQ_VOLUMERAND);
			script.push_back(P_GetInt(&s));
			script.push_back(P_GetInt(&s));
			break;

		      case SSEQ_CHVOL:
			script.push_back(SSEQ_CHVOL);
			script.push_back(P_GetInt(&s));
			break;

		      default:
			CONS_Printf("SNDSEQ: Unknown command '%s'.\n", line);
			break;
		      }
		}
	    }
	  s = p + 1;  // pass the line
	}
      p++;      
    }

  Z_Free(ms);
  CONS_Printf("done. %d sequences.\n", SoundSeqs.size());
}
