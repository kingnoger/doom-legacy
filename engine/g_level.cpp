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
// Revision 1.8  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.7  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.6  2003/11/30 00:09:42  smite-meister
// bugfixes
//
// Revision 1.5  2003/11/27 11:28:25  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.4  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.3  2003/11/12 11:07:17  smite-meister
// Serialization done. Map progression.
//
// Revision 1.2  2003/07/02 17:52:46  smite-meister
// VDir fix
//
// Revision 1.1  2003/06/10 22:39:54  smite-meister
// Bugfixes
//
//
//
// DESCRIPTION:
//   Implementation of MapCluster class
//
//-----------------------------------------------------------------------------

#include "g_game.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "dstrings.h"
#include "sounds.h"

#include "z_zone.h"

// default cluster constructor
MapCluster::MapCluster()
{
  number = 0;
  keepstuff = hub = false;
  episode = 0;

  kills = items = secrets = 0;
  time = partime = 0;
};

// cluster constructor
MapCluster::MapCluster(int n)
{
  number = n;
  keepstuff = hub = false;
  episode = 0;

  kills = items = secrets = 0;
  time = partime = 0;
}


// ticks the entire cluster forward in time
void MapCluster::Ticker()
{
  int i, n = maps.size();
  for (i=0; i<n; i++)
    maps[i]->Ticker(hub);
}


// called before moving on to a new cluster
void MapCluster::Finish(int nextmap, int ep)
{
  CONS_Printf("Cluster %d finished!\n", number);
  int i, n = maps.size();
  for (i=0; i<n; i++)
    {
      maps[i]->KickPlayers(nextmap, ep, true);
      maps[i]->Close();
    }

  Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1); // destroys pawns if they are not Detached
}


//==================================================================
// creates a MapCluster graph based on the MAPINFO data.
int GameInfo::Create_MAPINFO_game(int lump)
{
  if (lump < 0)
    return -1;

  CONS_Printf("Reading MAPINFO lump... ");

  int num;
  int n = Read_MAPINFO(lump);
  if (n <= 0)
    return 0;

  MapInfo *m;

  // warptrans. wtf were you thinking?!?! time to unravel the warptrans numbering.
  map<int, MapInfo *> warptransmap;
  mapinfo_iter_t i;
  for (i = mapinfo.begin(); i != mapinfo.end(); i++)
    {
      m = (*i).second;
      num = m->warptrans; 
      warptransmap[num] = m;
    }

  // now just put the correct exit data in the MapInfos
  for (i = mapinfo.begin(); i != mapinfo.end(); i++)
    {
      m = (*i).second;

      // set normal and secret exits
      // 'nextlevel' overrides 'next'
      if (m->nextlevel < 0 && m->warpnext > 0)
	m->nextlevel = warptransmap[m->warpnext]->mapnumber;
    }

  CONS_Printf(" ...done. %d maps.\n", n);

  return n;
}


//==================================================================
// This function recreates the classical Doom/DoomII/Heretic maplist
// using episode and game.mode
// Most of the original game dependent crap is here,
// all the other code is general and clean.

int GameInfo::Create_classic_game(int episode)
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
  MapInfo *p;
  MapCluster *c;

  Clear_mapinfo_clusterdef();

  // convention: Doom/Heretic normal exit is exit number 0, secret is exit number 100.
  // (room for Hexen exits 1-99)
  switch (game.mode)
    {
    case gm_doom2:
      base2 = TXT_C1TEXT;
      switch (game.mission)
	{
	case gmi_tnt:
	  base = TXT_THUSTR_1;
	  base2 = TXT_T1TEXT;
	  break;
	case gmi_plut:
	  base = TXT_PHUSTR_1;
	  break;
	default:
	  base = TXT_HUSTR_1;
	}

      n = 32;
      for (i=0; i<n; i++)
	{
	  p = new MapInfo;
	  p->mapnumber = i + 1;
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
	  p->nextlevel = i + 2;
	  mapinfo[i+1] = p;
	}

      mapinfo[30]->nextlevel = -1; // finish
      mapinfo[31]->nextlevel = 16; // return from secret
      mapinfo[32]->nextlevel = 16; // return from ss

      mapinfo[15]->secretlevel = 31; // secret
      mapinfo[31]->secretlevel = 32; // super secret
      
      mapinfo[7]->BossDeathKey = 32+64; // fatsos and baby spiders
      mapinfo[30]->BossDeathKey = 256;  // brain
      mapinfo[32]->BossDeathKey = 128;  // keen

      // 1-6,7-11,12-20,21-30, ,31,32
      for (i=1; i<7; i++)   mapinfo[i]->cluster = 1;
      for (i=7; i<12; i++)  mapinfo[i]->cluster = 2;
      for (i=12; i<21; i++) mapinfo[i]->cluster = 3;
      for (i=21; i<31; i++) mapinfo[i]->cluster = 4;
      mapinfo[31]->cluster = 5;
      mapinfo[32]->cluster = 6;

      for (i=0; i<6; i++)
	{
	  clustermap[i+1] = c = new MapCluster(i+1);
	  c->interpic = "INTERPIC";
	  if (i < 4)
	    c->exittext = text[base2+i];
	  else
	    c->entertext = text[base2+i];
	  c->finalepic = DoomIIFlat[i];
	  c->finalemusic = "D_READ_M";
	}
      break;

    case gm_doom1s:
      if (episode != 1) return 0;
    case gm_doom1:
      if (episode < 1 || episode > 3) return 0;
    case gm_udoom:
      if (episode < 1 || episode > 4) return 0;

      base = TXT_HUSTR_E1M1;

      n = 9;
      for (i=0; i<n; i++)
	{
	  p = new MapInfo;
	  p->mapnumber = i + 1;
	  sprintf(name, "E%1.1dM%1.1d", episode, i+1);
	  p->lumpname = name;
	  p->nicename = text[base + (episode-1)*9 + i];
	  p->partime = DoomPars[episode-1][i];
	  p->sky1 = string("SKY") + char('0' + episode);
	  p->musiclump = MusicNames[mus_e1m1 + (episode-1)*9 + i];
	  p->nextlevel = i + 2;
	  p->cluster = 1;
	  mapinfo[i+1] = p;
	}

      mapinfo[8]->nextlevel = -1; // finish
      mapinfo[9]->nextlevel = DoomSecret[episode-1] + 1; // return from secret
      mapinfo[DoomSecret[episode-1]]->secretlevel = 9; // secret

      mapinfo[8]->BossDeathKey = DoomBossKey[episode-1];
      if (episode == 4)
	mapinfo[6]->BossDeathKey = 4; // cyborg in E4M6...

      clustermap[1] = c = new MapCluster(1);
      c->exittext = text[TXT_E1TEXT + episode-1];
      c->finalepic = DoomFlat[episode-1];
      c->finalemusic = "D_VICTOR";
      c->episode = episode;
      sprintf(name, "WIMAP%d", (episode-1)%3);
      c->interpic = name;
      break;

    case gm_heretic:
      base = TXT_HERETIC_E1M1;

      n = 9;
      for (i=0; i<n; i++)
	{
	  p = new MapInfo;
	  p->mapnumber = i + 1;
	  sprintf(name, "E%1.1dM%1.1d", episode, i+1);
	  p->lumpname = name;
	  p->nicename = text[base + (episode-1)*9 + i];
	  p->partime = HereticPars[episode-1][i];
	  p->sky1 = HereticSky[episode-1];
	  p->musiclump = MusicNames[mus_he1m1 + (episode-1)*9 + i];
	  p->nextlevel = i + 2;
	  p->cluster = 1;
	  mapinfo[i+1] = p;
	}

      mapinfo[8]->nextlevel = -1; // finish
      mapinfo[9]->nextlevel = HereticSecret[episode-1] + 1; // return from secret
      mapinfo[HereticSecret[episode-1]]->secretlevel = 9; // secret

      mapinfo[8]->BossDeathKey = HereticBossKey[episode-1];

      clustermap[1] = c = new MapCluster(1);
      c->exittext = text[TXT_HERETIC_E1TEXT + episode-1];
      c->finalepic = HereticFlat[episode-1];
      c->finalemusic = "MUS_CPTD";
      c->episode = episode;
      sprintf(name, "MAPE%d", ((episode-1)%3)+1);
      c->interpic = name;
      break;

    default:
      n = 0;
    }

  // and populate the clusters
  mapinfo_iter_t t;
  for (t = mapinfo.begin(); t != mapinfo.end(); t++)
    {
      p = (*t).second;
      n = p->cluster;
      if (!clustermap.count(n))
	CONS_Printf("Missing cluster %d (map %d)!\n", n, p->mapnumber);
      else
	clustermap[n]->maps.push_back(p);
    }

  return n;
}
