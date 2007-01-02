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

#include "g_game.h"
#include "g_decorate.h"
#include "sounds.h"
#include "w_wad.h"


ActorInfoDictionary aid;

bool Read_DECORATE(int lump);


// fill fields with default values
ActorInfo::ActorInfo(const string& n)
{
  strncpy(classname, n.c_str(), 63);
  mobjtype = MT_NONE;
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
ActorInfo::ActorInfo(const mobjinfo_t& m)
{
  mobjtype = mobjtype_t(&m - mobjinfo);

  if (m.classname)
    strncpy(classname, m.classname, 63);
  else
    sprintf(classname, "class_%d", mobjtype);

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



void ConvertMobjInfo()
{
  int i;
  printf("Named DECORATE classes:\n");

  for (i=0; i<NUMMOBJTYPES; i++)
    {
      //mobjinfo[i].reactiontime *= NEWTICRATERATIO;
      //mobjinfo[i].speed        /= NEWTICRATERATIO;

      if (mobjinfo[i].classname)
	printf(" +%s\n", mobjinfo[i].classname);
    }

  for (i = MT_LEGACY; i <= MT_LEGACY_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i]);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, true);

      if (i >= MT_LEGACY_S && i <= MT_LEGACY_S_END)
	ai->spawn_always = true;
    }

  for (i = MT_DOOM; i <= MT_DOOM_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i]);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode <= gm_doom2);
    }

  for (i = MT_HERETIC; i <= MT_HERETIC_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i]);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode == gm_heretic);
    }

  for (i = MT_HEXEN; i <= MT_HEXEN_END; i++)
    {
      ActorInfo *ai = new ActorInfo(mobjinfo[i]);
      aid.Insert(ai);
      aid.InsertDoomEd(ai, game.mode == gm_hexen);
    }


  Read_DECORATE(fc.FindNumForName("DECORATE"));

  for (i=0;i<NUMSTATES;i++)
    {
      //states[i].tics *= NEWTICRATERATIO;
    }

  // this is because the ednum ranges normally overlap in different games
}
