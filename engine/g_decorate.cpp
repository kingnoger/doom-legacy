// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief ActorInfo class implementation, ActorInfo dictionary.

#include <stdarg.h>
#include "doomdef.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_decorate.h"
#include "dehacked.h" // flags are shared with BEX
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"


ActorInfoDictionary aid;

bool Read_DECORATE(int lump);


ActorInfo::~ActorInfo()
{
  if (owned_states)
    Z_Free(owned_states);
}


// fill fields with default values
ActorInfo::ActorInfo(const string& n)
{
  owned_states = NULL;

  strncpy(classname, n.c_str(), 63);
  mobjtype = MT_NONE;

  game = gm_doom2;
  obituary = string("%s got killed by an instance of ") + classname;
  spawn_always = false;

  doomednum = -1;
  spawnhealth = 1000;
  reactiontime = 8;
  painchance = 0;
  speed  = 0;
  radius = 20;
  height = 16;
  mass   = 100;
  damage = 0;

  flags = 0;
  flags2 = 0;

  seesound    = sfx_None;
  attacksound = sfx_None;
  painsound   = sfx_None;
  deathsound  = sfx_None;
  activesound = sfx_None;

  spawnstate   = NULL;
  seestate     = NULL;
  meleestate   = NULL;
  missilestate = NULL;
  painstate    = NULL;
  deathstate   = NULL;
  xdeathstate  = NULL;
  crashstate   = NULL;
  raisestate   = NULL;

  touchf = NULL;
}


// copy values from mobjinfo_t
ActorInfo::ActorInfo(const mobjinfo_t& m, int gm)
{
  owned_states = NULL;

  mobjtype = mobjtype_t(&m - mobjinfo);

  if (m.classname)
    strncpy(classname, m.classname, 63);
  else
    sprintf(classname, "class_%d", mobjtype);

  game = gm;
  obituary = string("%s got killed by an instance of ") + classname;
  spawn_always = false;

  doomednum = m.doomednum;
  spawnhealth = m.spawnhealth;
  reactiontime = m.reactiontime;
  painchance = m.painchance;
  speed = m.speed;
  radius = m.radius;
  height = m.height;
  mass = m.mass;
  damage = m.damage;

  flags = m.flags;
  flags2 = m.flags2;

  seesound    = m.seesound;
  attacksound = m.attacksound;
  painsound   = m.painsound;
  deathsound  = m.deathsound;
  activesound = m.activesound;

  spawnstate   = m.spawnstate;
  seestate     = m.seestate;
  meleestate   = m.meleestate;
  missilestate = m.missilestate;
  painstate    = m.painstate;
  deathstate   = m.deathstate;
  xdeathstate  = m.xdeathstate;
  crashstate   = m.crashstate;
  raisestate   = m.raisestate;

  touchf = m.touchf;
}


void ActorInfo::SetName(const char *n)
{
  strncpy(classname, n, 63);
}


void ActorInfo::SetFlag(const char *flagname, bool on)
{
  flag_mnemonic_t *p = BEX_FlagMnemonics;

  for ( ; p->name; p++)
    if (!strcasecmp(flagname, p->name))
      {
	switch (p->flagword)
	  {
	  case 1:
	    on ? (flags |= p->flag) : (flags &= ~p->flag);
	    break;

	  case 2:
	  default:
	    on ? (flags2 |= p->flag) : (flags2 &= ~p->flag);
	    break;
	  }

	break;
      }

  if (!p->name)
    {
      if (!strcasecmp(flagname, "MONSTER"))
	{
	  int f1 = MF_MONSTER | MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL;
	  int f2 = MF2_PUSHWALL | MF2_MCROSS;
	  on ? (flags |= f1, flags2 |= f2) : (flags &= ~f1, flags2 &= ~f2);
	  // TODO missing MONSTER property: CANPASS 
	}
      else if (!strcasecmp(flagname, "PROJECTILE"))
	{
	  int f1 = MF_MISSILE | MF_DROPOFF | MF_NOBLOCKMAP | MF_NOGRAVITY;
	  int f2 = MF2_NOTELEPORT | MF2_IMPACT | MF2_PCROSS;
	  on ? (flags |= f1, flags2 |= f2) : (flags &= ~f1, flags2 &= ~f2);
	}
      else
	Error("Unknown flag '%s'.\n", flagname);
    }
}


struct statelabel_t
{
#define SL_LEN 20
  char label[SL_LEN];
  int  statenum;
};
static vector<statelabel_t> state_labels;

struct statemodel_t
{
  spritenum_t sprite;    ///< Sprite to use.
  int         frame;
  int         tics;
  actionf_p1  action;    ///< Action function to call when entering this state, or NULL if none.
  char        gotolabel[SL_LEN];
  int         offset;
};
static vector<statemodel_t> new_states;


void ActorInfo::ResetStates()
{
  state_labels.clear();
  new_states.clear();
}


void ActorInfo::AddLabel(const char *l)
{
  statelabel_t temp;
  strncpy(temp.label, l, SL_LEN);
  temp.statenum = new_states.size(); // label points to next state to be defined
  state_labels.push_back(temp);
}


void ActorInfo::AddStates(const char *spr, const char *frames, int tics, const char *func)
{
  actionf_p1 f;
  if (func)
    {
      dactor_mnemonic_t *m;
      for (m = BEX_DActorMnemonics; m->name && strcasecmp(&func[2], m->name); m++); // ignore the initial "A_"
      if (!m->name)
	Error("Unknown action function mnemonic '%s'.\n", func);

      f = m->ptr;
    }
  else
    f = NULL;

  // find sprite num TODO if not found, create!
  spritenum_t spr_num = SPR_NONE;
  for (int i=0; i<NUMSPRITES; i++)
    if (!strcasecmp(sprnames[i], spr))
      {
	spr_num = static_cast<spritenum_t>(i);
	break;
      }

  statemodel_t s = {spr_num, 0, tics, f, "", 1};

  for ( ; *frames; frames++)
    {
      char c = toupper(*frames);
      if (c >= 'A' && c <= ']')
	{
	  s.frame = c - 'A';
	  new_states.push_back(s);
	}
    }
}


void ActorInfo::FinishSequence(const char *label, int offset)
{
  if (label)
    {
      strncpy(new_states.back().gotolabel, label, SL_LEN);
      new_states.back().offset = offset;
    }
  else
    {
      // loop to first state in sequence (equivalent to "goto current_label", but save some effort here)
      new_states.back().gotolabel[0] = 1; // HACK
      new_states.back().offset = state_labels.back().statenum;
    }
}


int ActorInfo::FindLabel(const char *label)
{
  int n = state_labels.size();
  for (int i=0; i<n; i++)
    if (!strncasecmp(label, state_labels[i].label, SL_LEN))
      {
	// label found
	return state_labels[i].statenum;
      }

  return -1; // not found
}


bool ActorInfo::CreateStates()
{
  int n = new_states.size();
  owned_states = static_cast<state_t*>(Z_Malloc(n*sizeof(state_t), PU_STATIC, NULL));

  state_t *s = owned_states;
  for (int i=0; i<n; i++, s++)
    {
      s->sprite = new_states[i].sprite;
      s->frame  = new_states[i].frame;
      s->tics   = new_states[i].tics;
      s->action = new_states[i].action;

      const char *p = new_states[i].gotolabel;

      if (!p[0]) // "next"
	s->nextstate = s+1;
      else if (p[0] == 1) // "loop", HACK
	s->nextstate = &owned_states[new_states[i].offset];
      else if (!strcmp(p, "NULL")) // "stop"
	s->nextstate = &states[S_NULL];
      else // "goto"
	{
	  int temp = FindLabel(p);
	  if (temp >= 0)
	    s->nextstate = &owned_states[temp];
	  else
	    {
	      Error("Unknown state label '%s'.\n", p);
	      s->nextstate = &states[S_NULL];
	    }
	}
    }

  n = state_labels.size();
  for (int i=0; i<n; i++)
    {
      const char *label = state_labels[i].label;
      state_t **p = NULL;
      if (!strcasecmp("spawn", label))        p = &spawnstate;
      else if (!strcasecmp("see", label))     p = &seestate;
      else if (!strcasecmp("melee", label))   p = &meleestate;
      else if (!strcasecmp("missile", label)) p = &missilestate;
      else if (!strcasecmp("pain", label))    p = &painstate;
      else if (!strcasecmp("death", label))   p = &deathstate;
      else if (!strcasecmp("xdeath", label))  p = &xdeathstate;
      else if (!strcasecmp("crash", label))   p = &crashstate;
      else if (!strcasecmp("raise", label))   p = &raisestate;

      if (p)
	*p = &owned_states[state_labels[i].statenum];
    }

  if (!spawnstate)
    {
      Error("Actor '%d' has no spawnstate!\n", classname);
      return false;
    }

  return true;
}



void ActorInfo::Error(const char *format, ...)
{
  va_list p;

  va_start(p, format);
  fprintf(stderr, "DECORATE: ");
  fprintf(stderr, format, p);
  va_end(p);
}



void ActorInfo::PrintDECORATEclass()
{
  printf("actor %s %d\n{\n", classname, doomednum);
  if (!obituary.empty())
    printf("  obituary \"%s\"\n", obituary.c_str());
  if (!hitobituary.empty())
    printf("  hitobituary \"%s\"\n", hitobituary.c_str());
  if (!modelname.empty())
    printf("  model \"%s\"\n", modelname.c_str());

  printf("  health %d\n", spawnhealth);
  printf("  reactiontime %d\n", reactiontime);
  printf("  painchance %d\n", painchance);
  printf("  speed %g\n", speed);
  printf("  damage %d\n", damage);
  printf("  radius %g\n", radius.Float());
  printf("  height %g\n", height.Float());
  printf("  mass %g\n", mass);

  // TODO flags

  if (seesound != sfx_None)
    printf("  seesound \"%s\"\n", S_GetSoundTag(seesound));
  if (attacksound != sfx_None)
    printf("  attacksound \"%s\"\n", S_GetSoundTag(attacksound));
  if (painsound != sfx_None)
    printf("  painsound \"%s\"\n", S_GetSoundTag(painsound));
  if (deathsound != sfx_None)
    printf("  deathsound \"%s\"\n", S_GetSoundTag(deathsound));
  if (activesound != sfx_None)
    printf("  activesound \"%s\"\n", S_GetSoundTag(activesound));

  // TODO states
  /*
  printf("  states\n  {\n");
  char temp[64];
  if (spawnstate)
    {
      printf("  Spawn:\n");
      printf("    %s %s %d %s\n", sprnames[s->sprite], temp, s->tics, BEX_DActorMnemonics[i].name);
    }
  printf("  }\n");
  */
  printf("}\n\n");
}


void ConvertMobjInfo()
{
  int i;
  /*
  printf("Named DECORATE classes:\n");
  for (i=0; i<NUMMOBJTYPES; i++)
    {
      //mobjinfo[i].reactiontime *= NEWTICRATERATIO;
      //mobjinfo[i].speed        /= NEWTICRATERATIO;

      if (mobjinfo[i].classname)
	printf(" +%s\n", mobjinfo[i].classname);
    }
  */

  for (i = MT_LEGACY; i <= MT_LEGACY_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i], gm_none);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, true);

      if (i >= MT_LEGACY_S && i <= MT_LEGACY_S_END)
	ai->spawn_always = true;
    }

  for (i = MT_DOOM; i <= MT_DOOM_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i], gm_doom2);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode <= gm_doom2);
    }

  for (i = MT_HERETIC; i <= MT_HERETIC_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i], gm_heretic);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode == gm_heretic);
    }

  for (i = MT_HEXEN; i <= MT_HEXEN_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i], gm_hexen);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode == gm_hexen);
    }

  CONS_Printf("Reading DECORATE definitions...\n");

  int n = fc.Size();
  for (int i = 0; i < n; i++)
    {
      // cumulative reading
      Read_DECORATE(fc.FindNumForNameFile("DECORATE", i));
    }

  CONS_Printf(" %d Actor types defined.\n", aid.Size());
  /*
  for (i=0;i<NUMSTATES;i++)
    {
      //states[i].tics *= NEWTICRATERATIO;
    }
  */
}
