// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log$
// Revision 1.1  2003/06/10 22:39:54  smite-meister
// Bugfixes
//
//
//
// DESCRIPTION:
//   Implementation of LevelNode class
//
//-----------------------------------------------------------------------------

#include "g_game.h"
#include "g_level.h"
#include "p_info.h"
#include "dstrings.h"
#include "sounds.h"


LevelNode::LevelNode()
{
  number = 0;
  cluster = 0;

  entrypoint = 0;
  exitused = NULL;
  done = false;
};


// one MapInfo_t per level -constructor
LevelNode::LevelNode(MapInfo_t *info)
{
  number    = info->mapnumber;
  cluster   = info->cluster;

  levelname = info->nicename;

  entrypoint = 0;
  exitused = NULL;
  done = false;

  contents.push_back(info);

  kills = items = secrets = 0;
  time = partime = 0;

  BossDeathKey = 0;
  episode = 1;

  // FIXME what kind of intermission should we show?
  switch (game.mode)
    {
    case gm_doom2:
    case gm_hexen:
      interpic = "INTERPIC";
      break;
    case gm_heretic:
      interpic = "MAPE0";
      break;
    default:
      interpic = "WIMAP0";
    }

  // all set except exit (which cannot be set yet)
}



// creates a LevelNode graph based on the 'mapinfo' data.
int GameInfo::Create_MAPINFO_levelgraph(int lump)
{
  if (lump < 0)
    return -1;

  CONS_Printf("Reading MAPINFO lump... ");

  int num;
  int n = P_Read_MAPINFO(lump);
  if (n <= 0)
    return 0;

  map<int, LevelNode *>::iterator t;
  for (t = levelgraph.begin(); t != levelgraph.end(); t++)
    delete (*t).second;
  levelgraph.clear();

  map<int, MapInfo_t *>::iterator i;
  LevelNode *l;

  // One level per map.
  // When using MAPINFO, we don't really need the extra layer of flexibility
  // that the LevelNode::exit remapping offers. This creates a mapping in which
  // exit number x is mapped to map (or level) number x.

  map<int, LevelNode *> warptransmap;
  for (i = mapinfo.begin(); i != mapinfo.end(); i++)
    {
      l = new LevelNode((*i).second); // generate the corresponding LevelNode
      num = ((*i).second)->mapnumber;
      levelgraph[num] = l;
      // Now I'm starting to get pissed. warptrans, wtf were you thinking?!?!
      num = ((*i).second)->warptrans; 
      warptransmap[num] = l;      
    }

  // now just put the correct exit data in the LevelNodes
  for (t = levelgraph.begin(); t != levelgraph.end(); t++)
    {
      l = (*t).second;
      l->exit = levelgraph; // Blessed STL. The exits map directly to levels.

      MapInfo_t *info = l->contents[0];
      // set normal and secret exits
      // 'nextlevel' overrides 'next'
      if (info->nextlevel > 0)
	l->exit[0] = levelgraph[info->nextlevel];
      else if (info->warpnext > 0)
	l->exit[0] = warptransmap[info->warpnext];

      if (info->secretlevel > 0)
	l->exit[100] = levelgraph[info->secretlevel];
    }

  CONS_Printf(" ...done. %d maps.\n", n);

  return n;
}


//==================================================================
// This function recreates the classical Doom/DoomII/Heretic maplist
// using episode and game.mode info
// Most of the original game dependent crap is here,
// all the other code is general and clean.

// P_FindLevelName() moves into history.

int GameInfo::Create_Classic_levelgraph(int episode)
{
  const char *HereticSky[5] = {"SKY1", "SKY2", "SKY3", "SKY1", "SKY3"};

  const int DoomBossKey[4] = { 1, 2, 8, 16 };
  const int HereticBossKey[5] = { 0x200, 0x800, 0x1000, 0x400, 0x800 };

  const int DoomSecret[4] = { 3, 5, 6, 2 };
  const int HereticSecret[5] = { 6, 4, 4, 4, 3 };
  const int DoomPars[4][9] =
  {
    {30,75,120,90,165,180,180,30,165},
    {90,90,90,120,90,360,240,30,170},
    {90,45,90,150,90,90,165,30,135},
    {0}
  };
  const int HereticPars[5][9] =
  {
    {90,0,0,0,0,0,0,0,0},
    {0},
    {0},
    {0},
    {0}
  };
  const int DoomIIPars[32] =
  {
    30,90,120,120,90,150,120,120,270,90,        //  1-10
    210,150,150,150,210,150,420,150,210,150,    // 11-20
    240,150,180,150,150,300,330,420,300,180,    // 21-30
    120,30                                      // 31-32
  };
  // FIXME! check the partimes! Heretic and Doom I episode 4!
  // I made these Heretic partimes by myself...
  // finale flat names
  const char DoomFlat[4][9] = {"FLOOR4_8", "SFLR6_1", "MFLR8_4", "MFLR8_3"};
  const char HereticFlat[5][9] = {"FLOOR25", "FLATHUH1", "FLTWAWA2", "FLOOR28", "FLOOR08"};
  const char DoomIIFlat[6][9] = {"SLIME16", "RROCK14", "RROCK07", "RROCK17", "RROCK13", "RROCK19"};

  int i, n, base, base2;
  char name[9];
  MapInfo_t *p;
  LevelNode *l;
  clusterdef_t *cd;

  P_Clear_mapinfo_clusterdef();

  // convention: Doom/Heretic normal exit is exit number 0, secret is exit number 100.
  // (room for Hexen exits 1-99)
  switch (game.mode)
    {
    case gm_doom2:
      base2 = C1TEXT_NUM;
      switch (game.mission)
	{
	case gmi_tnt:
	  base = THUSTR_1_NUM;
	  base2 = T1TEXT_NUM;
	  break;
	case gmi_plut:
	  base = PHUSTR_1_NUM;
	  break;
	default:
	  base = HUSTR_1_NUM;
	}

      n = 32;
      for (i=0; i<n; i++)
	{
	  p = new MapInfo_t;
	  p->mapnumber = i;
	  sprintf(name, "MAP%2.2d", i+1);
	  p->lumpname = name;
	  p->nicename = text[base + i];
	  p->partime = DoomIIPars[i];
	  if (i < 11)
	    p->sky1 = "SKY1";
	  else if (i < 20)
	    p->sky1 = "SKY2";
	  else
	    p->sky1 = "SKY3";
	  p->musiclump = MusicNames[mus_runnin + i];

	  l = new LevelNode(p);
	  l->interpic = "INTERPIC";

	  mapinfo[i] = p;
	  levelgraph[i] = l;
	}

      for (i=0; i<29; i++)
	levelgraph[i]->exit[0] = levelgraph[i+1]; // next level

      levelgraph[29]->exit[0] = NULL; // finish
      levelgraph[30]->exit[0] = levelgraph[15]; // return from secret
      levelgraph[31]->exit[0] = levelgraph[15]; // return from ss

      levelgraph[14]->exit[100] = levelgraph[30]; // secret
      levelgraph[30]->exit[100] = levelgraph[31]; // super secret

      levelgraph[6]->BossDeathKey = 32+64; // fatsos and baby spiders
      levelgraph[29]->BossDeathKey = 256;  // brain
      levelgraph[31]->BossDeathKey = 128;  // keen

      levelgraph[6]->cluster  = 1;
      levelgraph[11]->cluster = 2;
      levelgraph[20]->cluster = 3;
      levelgraph[29]->cluster = 4;
      levelgraph[30]->cluster = 5;
      levelgraph[31]->cluster = 6;
      for (i=0; i<6; i++)
	{
	  clusterdef[i+1] = cd = new clusterdef_t(i+1);
	  cd->entertext = text[base2+i];
	  cd->flatlump = DoomIIFlat[i];
	  cd->musiclump = "D_READ_M";
	  cd->episode = episode;
	}
      clusterdef[3]->entertext = "";
      clusterdef[3]->exittext = text[base2+3];      
      break;

    case gm_doom1s:
      if (episode != 1) return 0;
    case gm_doom1:
      if (episode < 1 || episode > 3) return 0;
    case gm_udoom:
      if (episode < 1 || episode > 4) return 0;

      base = HUSTR_E1M1_NUM;

      n = 9;
      for (i=0; i<n; i++)
	{
	  p = new MapInfo_t;
	  p->mapnumber = i;
	  sprintf(name, "E%1.1dM%1.1d", episode, i+1);
	  p->lumpname = name;
	  p->nicename = text[base + (episode-1)*9 + i];
	  p->partime = DoomPars[episode-1][i];
	  p->sky1 = string("SKY") + char('0' + episode);
	  p->musiclump = MusicNames[mus_e1m1 + (episode-1)*9 + i];

	  l = new LevelNode(p);
	  l->episode = episode;
	  sprintf(name, "WIMAP%d", (episode-1)%3);
	  l->interpic = name;

	  mapinfo[i] = p;
	  levelgraph[i] = l;
	}

      for (i=0; i<7; i++)
	{
	  if (i == DoomSecret[episode-1] - 1)
	    continue;
	  levelgraph[i]->exit[0] = levelgraph[i+1];
	}
      levelgraph[7]->exit[0] = NULL;
      levelgraph[8]->exit[0] = levelgraph[DoomSecret[episode-1]];
      levelgraph[DoomSecret[episode-1] - 1]->exit[100] = levelgraph[8]; // secret

      levelgraph[7]->BossDeathKey = DoomBossKey[episode-1];
      if (episode == 4)
	levelgraph[5]->BossDeathKey = 4; // cyborg in E4M6...

      levelgraph[7]->cluster = 1;
      clusterdef[1] = cd = new clusterdef_t(1);
      cd->exittext = text[E1TEXT_NUM + episode-1];
      cd->flatlump = DoomFlat[episode-1];
      cd->musiclump = "D_VICTOR";
      cd->episode = episode;
      break;

    case gm_heretic:
      base = HERETIC_E1M1_NUM;

      n = 9;
      for (i=0; i<n; i++)
	{
	  p = new MapInfo_t;
	  p->mapnumber = i;
	  sprintf(name, "E%1.1dM%1.1d", episode, i+1);
	  p->lumpname = name;
	  p->nicename = text[base + (episode-1)*9 + i];
	  p->partime = HereticPars[episode-1][i];
	  p->sky1 = HereticSky[episode-1];
	  p->musiclump = MusicNames[mus_he1m1 + (episode-1)*9 + i];

	  l = new LevelNode(p);
	  l->episode = episode;
	  sprintf(name, "MAPE%d", ((episode-1)%3)+1);
	  l->interpic = name;

	  mapinfo[i] = p;
	  levelgraph[i] = l;
	}

      for (i=0; i<7; i++)
	{
	  if (i == HereticSecret[episode-1] - 1)
	    continue;
	  levelgraph[i]->exit[0] = levelgraph[i+1];
	}
      levelgraph[7]->exit[0] = NULL;
      levelgraph[8]->exit[0] = levelgraph[HereticSecret[episode-1]];
      levelgraph[HereticSecret[episode-1] - 1]->exit[100] = levelgraph[8]; //secret

      levelgraph[7]->BossDeathKey = HereticBossKey[episode-1];

      levelgraph[7]->cluster = 1;
      clusterdef[1] = cd = new clusterdef_t(1);
      cd->exittext = text[HERETIC_E1TEXT + episode-1];
      cd->flatlump = HereticFlat[episode-1];
      cd->musiclump = "MUS_CPTD";
      cd->episode = episode;
      break;

    default:
      n = 0;
    }
  return n;
}

