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
// Revision 1.17  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.16  2003/12/09 01:02:00  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.15  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.14  2003/12/03 10:49:50  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.13  2003/11/30 00:09:46  smite-meister
// bugfixes
//
// Revision 1.12  2003/11/27 11:28:26  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.11  2003/11/23 19:07:41  smite-meister
// New startup order
//
// Revision 1.10  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.9  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.8  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.7  2003/06/01 18:56:30  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.6  2003/05/30 13:34:46  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.5  2003/04/24 20:30:16  hurdler
// Remove lots of compiling warnings
//
// Revision 1.4  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.3  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.2  2002/12/04 09:13:40  smite-meister
// Player clipping bug fixed
//
// Revision 1.1.1.1  2002/11/16 14:18:03  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"
#include "dstrings.h"

#include "command.h"
#include "console.h"

#include "m_archive.h"

#include "g_game.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_team.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_spec.h"
#include "p_setup.h" // FIXME levelflats...

#include "p_acs.h"
#include "r_data.h"
#include "d_netcmd.h"
#include "t_vari.h"
#include "t_script.h"
#include "t_parse.h"
#include "m_random.h"
#include "m_misc.h"
#include "m_menu.h"

#include "w_wad.h"
#include "z_zone.h"
#include "tables.h"

#include "am_map.h"
#include "hu_stuff.h"

extern  consvar_t  cv_deathmatch;

runningscript_t *new_runningscript();

// Pads save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.
//#define PADSAVEP()      save_p += (4 - ((int) save_p & 3)) & 3


enum consistency_marker_t
{
  // consistency markers in the savegame
  MARK_GROUP  = 0x71717171,
  MARK_MAP    = 0x8ae51d73,
  MARK_SCRIPT = 0x37c4fe01,
  MARK_THINK  = 0x1c02fa39,
  MARK_MISC   = 0xabababab,

  // this special marker is used to denote the end of a collection of objects
  MARK_END = 0xffffffff,
};


//==============================================
// Marshalling functions for various Thinkers
//==============================================

int acs_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = ((byte *)ip - mp->ActionCodeBase)); // byte offset
      a << (temp = line ? (line - mp->lines) : -1);
      Thinker::Serialize(activator, a);
      a.Write((byte *)stak, sizeof(stak));
      a.Write((byte *)vars, sizeof(vars));
    }
  else
    {
      a << temp;
      ip = (int *)(mp->ActionCodeBase + temp);
      a << temp;
      if (temp == -1)
	line = NULL;
      else
	line = mp->lines + temp;
      activator = (Actor *)Thinker::Unserialize(a);
      a.Read((byte *)stak, sizeof(stak));
      a.Read((byte *)vars, sizeof(vars));
    }

  a << side << number << infoIndex << delayCount << stackPtr;
  return 0;
}

int sectoreffect_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      if (!sector)
	I_Error("Sectoreffect with no sector!\n");
      a << (temp = sector - mp->sectors);
    }
  else
    {
      a << temp;
      if (temp >= mp->numsectors)
	I_Error("Invalid sector for a Sectoreffect!\n");
      sector = mp->sectors + temp;
    }

  return 0;
}

int lightfx_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->lightingdata = this;

  a << type << count << maxlight << minlight << maxtime << mintime;
  return 0;
}

int phasedlight_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->lightingdata = this;

  a << base << index;
  return 0;
}

int floor_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = this;

  a << type << crush << newspecial << texture << speed << destheight;
  return 0;
}

int elevator_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->floordata = sector->ceilingdata = this;

  a << type << crush << floordest << ceilingdest << floorspeed << ceilingspeed;
  return 0;
}

int plat_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    {
      sector->floordata = this;
      mp->AddActivePlat(this);
    }

  a << type << speed << low << high << wait << count << tag;
  a << status << oldstatus;
  
  return 0;
}

int ceiling_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    {
      sector->ceilingdata = this;
      mp->AddActiveCeiling(this);
    }

  a << type << crush << newspecial << texture << upspeed << downspeed << oldspeed;
  a << tag << olddirection << direction << bottomheight << topheight;

  return 0;
}

int vdoor_t::Marshal(LArchive &a)
{
  sectoreffect_t::Marshal(a);
  if (!a.IsStoring())
    sector->ceilingdata = this;

  a << type << direction << topheight << speed << topwait << topcount << boomlighttag;
  return 0;
}

int button_t::Marshal(LArchive &a)
{
  int temp;
  if (a.IsStoring())
    {
      a << (temp = line ? (line - mp->lines) : -1);
    }
  else
    {
      a << temp;
      if (temp == -1)
	line = NULL;
      else
	line = mp->lines + temp;
      soundorg = &line->frontsector->soundorg;
    }

  a << texture << timer << where;
  return 0;
}

int scroll_t::Marshal(LArchive &a)
{
  a << type << affectee << vx << vy;
  a << last_height << accel << vdx << vdy;

  int temp;
  if (a.IsStoring())
    {
      a << (temp = control ? (control - mp->sectors) : -1);
    }
  else
    {
      a << temp;
      if (temp == -1)
	control = NULL;
      else
	control = mp->sectors + temp;
    }
  return 0;
}

int pusher_t::Marshal(LArchive &a)
{
  a << type << x_mag << y_mag << magnitude << radius;
  a << x << y << affectee;
  if (a.IsStoring())
    Thinker::Serialize(source, a);
  else
    source = (DActor *)Thinker::Unserialize(a);
  return 0;
}

int polyobject_t::Marshal(LArchive &a)
{
  a << polyobj << speed << dist;
  return 0;
}

int polymove_t::Marshal(LArchive &a)
{
  polyobject_t::Marshal(a);
  a << angle << xs << ys;
  return 0;
}

int polydoor_t::Marshal(LArchive &a)
{
  polyobject_t::Marshal(a);
  a << type << totalDist << direction << xs << ys;
  a << tics << waitTics << close;
  return 0;
}


int Actor::Marshal(LArchive &a)
{
  a << x << y << z;
  a << angle << aiming;
  a << px << py << pz;
  a << mass << radius << height;
  a << health;

  a << flags << flags2 << eflags;

  a << tid << special;
  for (int i=0; i<5; i++)
    a << args[i];

  a << reactiontime << floorclip; 

  short stemp;

  if (a.IsStoring())
    {
      // TODO save presentation!?
      if (mp)
	stemp = short(spawnpoint - mp->mapthings);
      else
	stemp = 0;
      a << stemp;

      Thinker::Serialize(owner, a);
      Thinker::Serialize(target, a);
   }
  else
    {
      a << stemp;
      if (mp)
	{
	  spawnpoint = mp->mapthings + stemp;
	  mp->mapthings[stemp].mobj = this;
	}

      owner  = (Actor *)Thinker::Unserialize(a);
      target = (Actor *)Thinker::Unserialize(a);

      if (mp)
	{
	  CheckPosition(x, y);
	  SetPosition();
	}
    }

  return 0;
}


int DActor::Marshal(LArchive &a)
{ 
  // NOTE is it really worth the effort to do this delta-coding?
  // TODO save presentation!

  enum dactor_diff_e
  {
    MD_SPAWNPOINT = 0x000001,
    MD_TYPE       = 0x000002,
    MD_XY         = 0x000004,
    MD_Z          = 0x000008,
    MD_MOM        = 0x000010,

    MD_MASS       = 0x000020,
    MD_RADIUS     = 0x000040,
    MD_HEIGHT     = 0x000080,
    MD_HEALTH     = 0x000100,

    MD_FLAGS      = 0x000200,
    MD_FLAGS2     = 0x000400,
    MD_EFLAGS     = 0x000800,

    MD_SPECIAL1   = 0x001000,
    MD_SPECIAL2   = 0x002000,
    MD_TID        = 0x004000,
    MD_SPECIAL    = 0x008000,

    MD_RTIME      = 0x010000,
    MD_STATE      = 0x020000,
    MD_TICS       = 0x040000,
    MD_MOVEDIR    = 0x080000,
    MD_MOVECOUNT  = 0x100000,
    MD_THRESHOLD  = 0x200000,
    MD_LASTLOOK   = 0x400000,
    MD_TARGET     = 0x800000,
    MD_OWNER     = 0x1000000,
  };

  short stemp;
  int i;

  unsigned diff;

  if (a.IsStoring())
    {
      // find the differences
      if (spawnpoint && (info->doomednum != -1))
	{
	  // FIXME why the doomednum check?
	  diff = MD_SPAWNPOINT;
    
	  if ((x != spawnpoint->x << FRACBITS) ||
	      (y != spawnpoint->y << FRACBITS) ||
	      (angle != unsigned(ANG45 * (spawnpoint->angle/45))))
	    diff |= MD_XY;

	  if (info->doomednum != spawnpoint->type)
	    diff |= MD_TYPE;
	}
      else
	{
	  // not a map spawned thing so make it from scratch
	  diff = MD_XY | MD_TYPE;
	}

      // not the default but the most probable
      if (z != floorz)                   diff |= MD_Z;
      if (px != 0 || py != 0 || pz != 0) diff |= MD_MOM;
      if (mass   != info->mass)        diff |= MD_MASS;
      if (radius != info->radius)      diff |= MD_RADIUS;
      if (height != info->height)      diff |= MD_HEIGHT;
      if (health != info->spawnhealth) diff |= MD_HEALTH;
      if (flags  != info->flags)       diff |= MD_FLAGS;
      if (flags2 != info->flags2)      diff |= MD_FLAGS2;
      if (eflags)                      diff |= MD_EFLAGS;
 
      if (special1)  diff |= MD_SPECIAL1;
      if (special2)  diff |= MD_SPECIAL2;
      if (tid)       diff |= MD_TID;
      if (special)   diff |= MD_SPECIAL;

      if (reactiontime != info->reactiontime) diff |= MD_RTIME;
      if (state-states != info->spawnstate)   diff |= MD_STATE;
      if (tics         != state->tics)        diff |= MD_TICS;


      if (movedir)        diff |= MD_MOVEDIR;
      if (movecount)      diff |= MD_MOVECOUNT;
      if (threshold)      diff |= MD_THRESHOLD;
      if (lastlook != -1) diff |= MD_LASTLOOK;

      if (owner)   diff |= MD_TARGET;
      if (target)  diff |= MD_OWNER;

      a << diff;

      if (diff & MD_SPAWNPOINT)
	{
	  stemp = short(spawnpoint - mp->mapthings);
	  a << stemp;
	}

      if (diff & MD_TYPE) a << short(type);
      if (diff & MD_XY)   a << x << y << angle;
      if (diff & MD_Z)    a << z;
      if (diff & MD_MOM)  a << px << py << pz;

      if (diff & MD_MASS)   a << mass;
      if (diff & MD_RADIUS) a << radius;
      if (diff & MD_HEIGHT) a << height;
      if (diff & MD_HEALTH) a << health;

      if (diff & MD_FLAGS)  a << flags;
      if (diff & MD_FLAGS2) a << flags2;
      if (diff & MD_EFLAGS) a << eflags;

      if (diff & MD_SPECIAL1) a << special1;
      if (diff & MD_SPECIAL2) a << special2;
      if (diff & MD_TID)      a << tid;
      if (diff & MD_SPECIAL)
	{
	  a << special;
	  for (i=0; i<5; i++)
	    a << args[i];
	}

      if (diff & MD_RTIME) a << reactiontime;
      if (diff & MD_STATE)
	{
	  stemp = short(state - states);
	  a << stemp;
	}
      if (diff & MD_TICS)      a << tics;
      if (diff & MD_MOVEDIR)   a << movedir;
      if (diff & MD_MOVECOUNT) a << movecount;
      if (diff & MD_THRESHOLD) a << threshold;
      if (diff & MD_LASTLOOK)  a << lastlook;

      if (diff & MD_OWNER)  Thinker::Serialize(owner, a);
      if (diff & MD_TARGET) Thinker::Serialize(target, a);
    }
  else
    {
      // retrieving
      a << diff;

      if (diff & MD_SPAWNPOINT)
	{
	  a << stemp;
	  spawnpoint = mp->mapthings + stemp;
	  mp->mapthings[stemp].mobj = this;
	}

      if (diff & MD_TYPE)
	{
	  a << stemp;
	  type = mobjtype_t(stemp);
	}
      else
	type = mobjtype_t(spawnpoint->type); // Map::LoadThings() should set it to the correct value
      info = &mobjinfo[type];

      {
	// use info for init, correct later
	mass         = info->mass;
	radius       = info->radius;
	height       = info->height;
	health       = info->spawnhealth;
	flags        = info->flags;
	flags2       = info->flags2;
	reactiontime = info->reactiontime;
	state        = &states[info->spawnstate];
      }

      if (diff & MD_XY)
	a << x << y << angle;
      else
	{
	  x     = spawnpoint->x << FRACBITS;
	  y     = spawnpoint->y << FRACBITS;
	  angle = ANG45 * (spawnpoint->angle/45);
	}

      if (diff & MD_Z)
	a << z;

      if (diff & MD_MOM)
	a << px << py << pz; // else zero (by constructor)

      if (diff & MD_MASS)   a << mass;
      if (diff & MD_RADIUS) a << radius;
      if (diff & MD_HEIGHT) a << height;
      if (diff & MD_HEALTH) a << health;

      if (diff & MD_FLAGS)  a << flags;
      if (diff & MD_FLAGS2) a << flags2;
      if (diff & MD_EFLAGS) a << eflags;

      if (diff & MD_SPECIAL1) a << special1; // what about the pointers in these?
      if (diff & MD_SPECIAL2) a << special2;
      if (diff & MD_TID)      a << tid;
      if (diff & MD_SPECIAL)
	{
	  a << special;
	  for (i=0; i<5; i++)
	    a << args[i];
	}

      if (diff & MD_RTIME) a << reactiontime;
      if (diff & MD_STATE)
	{
	  a << stemp;
	  state = &states[stemp];
	}
      
      if (diff & MD_TICS)
	a << tics;
      else
	tics = state->tics;

      if (diff & MD_MOVEDIR)   a << movedir;
      if (diff & MD_MOVECOUNT) a << movecount;
      if (diff & MD_THRESHOLD) a << threshold;
      if (diff & MD_LASTLOOK)  a << lastlook;

      if (diff & MD_OWNER)  owner  = (Actor *)Thinker::Unserialize(a);
      if (diff & MD_TARGET) target = (Actor *)Thinker::Unserialize(a);

      // set sprev, snext, bprev, bnext, subsector
      CheckPosition(x, y); // TEST, sets tmfloorz, tmceilingz
      SetPosition();

      if (!(diff & MD_Z))
	z = floorz;
    }

  return 0;
}


int Pawn::Marshal(LArchive & a)
{
  Actor::Marshal(a);

  a << color;
  a << maxhealth << speed;
  a << attackphase;

  extern pawn_info_t pawndata[];
  int temp = pinfo - pawndata;
  a << temp;
  if (!a.IsStoring())
    pinfo = pawndata + temp;

  return 0;
}

int PlayerPawn::Marshal(LArchive &a)
{
  Pawn::Marshal(a);

  int i, n, diff;
  short stemp;

  enum player_diff
  {
    PD_POWERS       = 0x0001,
    PD_PMASK        = 0x0FFF,

    PD_REFIRE      = 0x01000,
    PD_MORPHTICS   = 0x02000,
    PD_FLYHEIGHT   = 0x04000,

    PD_BACKPACK   = 0x020000,
    PD_ATTACKDWN  = 0x040000,
    PD_USEDWN     = 0x080000,
    PD_JMPDWN     = 0x100000
  };

  if (a.IsStoring())
    {
      for (i=0; i<NUMPSPRITES; i++)
	{
	  if (psprites[i].state)
	    stemp = psprites[i].state - weaponstates + 1;
	  else
	    stemp = 0;

	  a << stemp;
	  a << psprites[i].tics << psprites[i].sx << psprites[i].sy;
	}

      a << player->number;

      // inventory is closed.
      a << invSlot;
      n = inventory.size();
      a << n;
      for (i=0; i<n; i++)
	a << inventory[i].type << inventory[i].count;

      diff = 0;
      for (i=0; i<NUMWEAPONS; i++)
	if (weaponowned[i])
	  diff |= 1 << i;
      a << diff; // we have already 32 weapon types. whew!

      // cheats vanish in saves;)

      diff = 0;
      for (i=0; i<NUMPOWERS; i++)
	if (powers[i])
	  diff |= PD_POWERS << i;

      if (refire)      diff |= PD_REFIRE;
      if (morphTics)   diff |= PD_MORPHTICS;
      if (flyheight)   diff |= PD_FLYHEIGHT;

      // booleans
      if (backpack)   diff |= PD_BACKPACK;
      if (attackdown) diff |= PD_ATTACKDWN;
      if (usedown)    diff |= PD_USEDWN;
      if (jumpdown)   diff |= PD_JMPDWN;

      a << diff;

      for (i=0; i<NUMPOWERS; i++)
	if (diff & (PD_POWERS << i))
	  a << powers[i];

      if (diff & PD_REFIRE) a << refire;
      if (diff & PD_MORPHTICS) a << morphTics;
      if (diff & PD_FLYHEIGHT) a << flyheight;
      // specialsector ???
    }
  else
    {
      // loading
      for (i=0; i<NUMPSPRITES; i++)
	{
	  a << stemp;	  
	  if (stemp)
	    psprites[i].state = &weaponstates[stemp - 1];

	  a << psprites[i].tics << psprites[i].sx << psprites[i].sy;
	}

      a << n; //player->number;
      player = game.FindPlayer(n);
      player->pawn = this;
      player->mp = mp;

      a << invSlot;
      a << n;
      inventory.resize(n);
      for (i=0; i<n; i++)
	a << inventory[i].type << inventory[i].count;

      a << diff;
      for (i=0; i<NUMWEAPONS; i++)
	weaponowned[i] = (diff & (1 << i));

      a << diff;
      for (i=0; i<NUMPOWERS; i++)
	if (diff & (PD_POWERS << i))
	  a << powers[i];

      if (diff & PD_REFIRE) a << refire;
      if (diff & PD_MORPHTICS) a << morphTics;
      if (diff & PD_FLYHEIGHT) a << flyheight;

      backpack     = diff & PD_BACKPACK;
      attackdown   = diff & PD_ATTACKDWN;
      usedown      = diff & PD_USEDWN;
      jumpdown     = diff & PD_JMPDWN;

      if (powers[pw_weaponlevel2])
	weaponinfo = wpnlev2info;
      else
	weaponinfo = wpnlev1info;

      if (backpack)
	maxammo = maxammo2;
      else
	maxammo = maxammo1;
    }

  // non-coded stuff (just read/write the numbers)
  a << pclass;
  a << keycards;
  a << int(pendingweapon) << int(readyweapon);

  for (i=0; i<NUMAMMO; i++)
    a << ammo[i];

  a << toughness;

  for (i=0; i<NUMARMOR; i++)
    a << armorfactor[i] << armorpoints[i];

  return 0;
}



//==============================================
//  Map serialization
//==============================================

enum mapdiff_e
{
    // sectors
    // diff
    SD_FLOORHT  = 0x01,
    SD_CEILHT   = 0x02,
    SD_FLOORPIC = 0x04,
    SD_CEILPIC  = 0x08,
    SD_LIGHT    = 0x10,
    SD_SPECIAL  = 0x20,
    SD_TAG      = 0x40,
    SD_DIFF2    = 0x80,

    // diff2
    SD_FXOFFS    = 0x01,
    SD_FYOFFS    = 0x02,
    SD_CXOFFS    = 0x04,
    SD_CYOFFS    = 0x08,
    SD_STAIRLOCK = 0x10,
    SD_PREVSEC   = 0x20,
    SD_NEXTSEC   = 0x40,

    // line- and sidedefs
    // diff
    LD_FLAG     = 0x01,
    LD_SPECIAL  = 0x02,
    LD_ARGS     = 0x04,
    LD_S1TEXOFF = 0x08,
    LD_S1TOPTEX = 0x10,
    LD_S1BOTTEX = 0x20,
    LD_S1MIDTEX = 0x40,
    LD_DIFF2    = 0x80,

    // diff2
    LD_S2TEXOFF = 0x01,
    LD_S2TOPTEX = 0x02,
    LD_S2BOTTEX = 0x04,
    LD_S2MIDTEX = 0x08,
};


int Map::Serialize(LArchive &a)
{
  unsigned temp;
  short stemp;
  int i, j, n;

  a.Marker(MARK_MAP);

  a << lumpname;
  // TODO save map md5 checksum, to make sure the correct map is loaded
  a << starttic << maptic;
  a << kills << items << secrets; // conceivably scripts could change these

  //----------------------------------------------
  // record the changes in static geometry (as compared to wad)
  // "reload" the map to check for differences
  // TODO helluvalot of unneeded stuff is saved because of linedef/sector special mappings...
  // Perhaps we should really fake-setup the comparison map... or perhaps we'll just live with it.
  int statsec = 0, statline = 0;

  byte diff, diff2;

  mapsector_t *ms = (mapsector_t *)fc.CacheLumpNum(lumpnum + ML_SECTORS, PU_CACHE);
  sector_t    *ss = sectors;

  for (i = 0; i<numsectors ; i++, ss++, ms++)
    {
      diff = diff2 = 0;
      if (ss->floorheight != SHORT(ms->floorheight)<<FRACBITS)
	diff |= SD_FLOORHT;
      if (ss->ceilingheight != SHORT(ms->ceilingheight)<<FRACBITS)
	diff |= SD_CEILHT;

      // P_AddLevelFlat should not add but just return the number
      if (ss->floorpic != P_AddLevelFlat(ms->floorpic,levelflats))
	diff |= SD_FLOORPIC;
      if (ss->ceilingpic != P_AddLevelFlat(ms->ceilingpic,levelflats))
	diff |= SD_CEILPIC;

      if (ss->lightlevel != SHORT(ms->lightlevel)) diff |= SD_LIGHT;
      if (ss->special != SHORT(ms->special))       diff |= SD_SPECIAL;
      if (ss->tag != SHORT(ms->tag))               diff |= SD_TAG;
      // floortype?
      if (ss->floor_xoffs != 0)   diff2 |= SD_FXOFFS;
      if (ss->floor_yoffs != 0)   diff2 |= SD_FYOFFS;
      if (ss->ceiling_xoffs != 0) diff2 |= SD_CXOFFS;
      if (ss->ceiling_yoffs != 0) diff2 |= SD_CYOFFS;
      if (ss->stairlock < 0)      diff2 |= SD_STAIRLOCK;
      if (ss->nextsec != -1)      diff2 |= SD_NEXTSEC;
      if (ss->prevsec != -1)      diff2 |= SD_PREVSEC;
      // TODO seqtype

      if (diff2)
	diff |= SD_DIFF2;

      if (diff)
        {
	  statsec++;

	  a << i;
	  a << diff;

	  if (diff & SD_DIFF2)
	    a << diff2;
	  if (diff & SD_FLOORHT) a << ss->floorheight;
	  if (diff & SD_CEILHT)  a << ss->ceilingheight;

	  if (diff & SD_FLOORPIC)
	    a.Write((byte *)levelflats[ss->floorpic].name, 8);
	  if (diff & SD_CEILPIC)
	    a.Write((byte *)levelflats[ss->ceilingpic].name, 8);

	  if (diff & SD_LIGHT)    a << ss->lightlevel;
	  if (diff & SD_SPECIAL)  a << ss->special;
	  if (diff & SD_TAG)      a << ss->tag;

	  if (diff2 & SD_FXOFFS)  a << ss->floor_xoffs;
	  if (diff2 & SD_FYOFFS)  a << ss->floor_yoffs;
	  if (diff2 & SD_CXOFFS)  a << ss->ceiling_xoffs;
	  if (diff2 & SD_CYOFFS)  a << ss->ceiling_yoffs;
	  if (diff2 & SD_STAIRLOCK) a << ss->stairlock;
	  if (diff2 & SD_NEXTSEC)   a << ss->nextsec;
	  if (diff2 & SD_PREVSEC)   a << ss->prevsec;
        }
    }
  a << (temp = MARK_END);

  doom_maplinedef_t *mld = (doom_maplinedef_t *)fc.CacheLumpNum(lumpnum + ML_LINEDEFS, PU_CACHE);
  hex_maplinedef_t *hld = (hex_maplinedef_t *)mld;
  mapsidedef_t *msd = (mapsidedef_t *)fc.CacheLumpNum(lumpnum + ML_SIDEDEFS, PU_CACHE);
  line_t *li = lines;
  side_t *si;

  // do lines
  for (i=0 ; i<numlines ; i++, li++)
    {
      diff = diff2 = 0;

      if (hexen_format)
	{
	  if (li->flags != SHORT(hld->flags))
	    diff |= LD_FLAG;
	  if (li->special != SHORT(hld->special))
	    diff |= LD_SPECIAL;
	  for (j=0; j<5; j++)
	    if (li->args[j] != hld->args[j])
	      diff |= LD_ARGS;
	  hld++;
	}
      else
	{
	  if (li->flags != SHORT(mld->flags))
	    diff |= LD_FLAG;
	  if (li->special != SHORT(mld->special))
	    diff |= LD_SPECIAL;
	  // TODO save tag?
	  mld++;
	}

      if (li->sidenum[0] != -1)
        {
	  si = &sides[li->sidenum[0]];
	  if (si->textureoffset != SHORT(msd[li->sidenum[0]].textureoffset)<<FRACBITS)
	    diff |= LD_S1TEXOFF;
	  //SoM: 4/1/2000: Some textures are colormaps. Don't worry about invalid textures.
	  if (R_CheckTextureNumForName(msd[li->sidenum[0]].toptexture) != -1)
	    if (si->toptexture != R_TextureNumForName(msd[li->sidenum[0]].toptexture))
	      diff |= LD_S1TOPTEX;
	  if (R_CheckTextureNumForName(msd[li->sidenum[0]].bottomtexture) != -1)
	    if (si->bottomtexture != R_TextureNumForName(msd[li->sidenum[0]].bottomtexture))
	      diff |= LD_S1BOTTEX;
	  if (R_CheckTextureNumForName(msd[li->sidenum[0]].midtexture) != -1)
	    if (si->midtexture != R_TextureNumForName(msd[li->sidenum[0]].midtexture))
	      diff |= LD_S1MIDTEX;
        }
      if (li->sidenum[1] != -1)
        {
	  si = &sides[li->sidenum[1]];
	  if (si->textureoffset != SHORT(msd[li->sidenum[1]].textureoffset)<<FRACBITS)
	    diff2 |= LD_S2TEXOFF;
	  if (R_CheckTextureNumForName(msd[li->sidenum[1]].toptexture) != -1)
	    if (si->toptexture != R_TextureNumForName(msd[li->sidenum[1]].toptexture))
	      diff2 |= LD_S2TOPTEX;
	  if (R_CheckTextureNumForName(msd[li->sidenum[1]].bottomtexture) != -1)
	    if (si->bottomtexture != R_TextureNumForName(msd[li->sidenum[1]].bottomtexture))
	      diff2 |= LD_S2BOTTEX;
	  if (R_CheckTextureNumForName(msd[li->sidenum[1]].midtexture) != -1)
	    if (si->midtexture != R_TextureNumForName(msd[li->sidenum[1]].midtexture))
	      diff2 |= LD_S2MIDTEX;
	  if (diff2)
	    diff |= LD_DIFF2;
        }

      if (diff)
        {
	  statline++;

	  a << i;
	  a << diff;
	  if (diff & LD_DIFF2  )  a << diff2;
	  if (diff & LD_FLAG   )  a << li->flags;
	  if (diff & LD_SPECIAL)  a << li->special;
	  if (diff & LD_ARGS)
	    for (j=0; j<5; j++)
	      a << li->args[j];

	  si = &sides[li->sidenum[0]];
	  if (diff & LD_S1TEXOFF) a << si->textureoffset;
	  if (diff & LD_S1TOPTEX) a << si->toptexture;
	  if (diff & LD_S1BOTTEX) a << si->bottomtexture;
	  if (diff & LD_S1MIDTEX) a << si->midtexture;

	  si = &sides[li->sidenum[1]];
	  if (diff2 & LD_S2TEXOFF) a << si->textureoffset;
	  if (diff2 & LD_S2TOPTEX) a << si->toptexture;
	  if (diff2 & LD_S2BOTTEX) a << si->bottomtexture;
	  if (diff2 & LD_S2MIDTEX) a << si->midtexture;
        }
    }
  a << (temp = MARK_END);
  CONS_Printf("%d/%d sectors, %d/%d lines saved\n", statsec, numsectors, statline, numlines);

  //----------------------------------------------
  // TODO polyobjs

  //----------------------------------------------
  // scripts
  a.Marker(MARK_SCRIPT);

  for (i = 0; i < ACScriptCount; i++)
    {
      a << int(ACSInfo[i].state);
      a << ACSInfo[i].waitValue;
    }
  a.Write((byte *)ACMapVars, sizeof(ACMapVars));

#ifdef FRAGGLESCRIPT
  // levelscript contains the map global variables (everything else can be loaded from the WAD)

  // count number of variables
  n = 0;
  for (i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = levelscript->variables[i];
      while (sv && sv->type != svt_label)
        {
          n++;
          sv = sv->next;
        }
    }
  a << n;

  // go thru hash chains, store each variable
  for (i=0; i<VARIABLESLOTS; i++)
    {
      // go thru this hashchain
      svariable_t *sv = levelscript->variables[i];
      
      // once we get to a label there can be no more actual
      // variables in the list to store
      while (sv && sv->type != svt_label)
        {
          a << sv->name;
          a << sv->type;
          switch (sv->type)
            {
            case svt_string:
	      a << sv->value.s;
	      break;
            case svt_int:
	      a << sv->value.i;
	      break;
            case svt_actor:
              Thinker::Serialize(sv->value.mobj, a);
	      break;
            case svt_fixed:
	      a << sv->value.fixed;
	      break;
            }
          sv = sv->next;
        }
    }

  //runningscripts (scripts currently suspended)
  runningscript_t *rs;
  
  // count runningscripts
  n = 0;
  for (rs = runningscripts; rs; rs = rs->next)
    n++;
  a << n;
  
  // now archive them
  for (rs = runningscripts; rs; rs = rs->next)
    {
      a << rs->script->scriptnum;
      a << (n = (rs->savepoint - rs->script->data)); // offset
      a << int(rs->wait_type);
      a << rs->wait_data;

      Thinker::Serialize(rs->trigger, a);
  
      // count number of variables
      n = 0;
      for (i=0; i<VARIABLESLOTS; i++)
	{
	  svariable_t *sv = rs->variables[i];
	  while (sv && sv->type != svt_label)
	    {
	      n++;
	      sv = sv->next;
	    }
	}
      a << n;

      // go thru hash chains, store each variable
      for (i=0; i<VARIABLESLOTS; i++)
	{
	  svariable_t *sv = rs->variables[i];
      
	  while (sv && sv->type != svt_label)
	    {
	      a << sv->name;
	      a << sv->type;
	      switch (sv->type)
		{
		case svt_string:
		  a << sv->value.s;
		  break;
		case svt_int:
		  a << sv->value.i;
		  break;
		case svt_actor:
		  Thinker::Serialize(sv->value.mobj, a);
		  break;
		case svt_fixed:
		  a << sv->value.fixed;
		  break;
		}
	      sv = sv->next;
	    }
	}
    }

  // TODO Archive the script camera.
#endif // FRAGGLESCRIPT

  //----------------------------------------------
  // Thinkers (including map objects aka Actors)
  a.Marker(MARK_THINK);

  Thinker *th;
  // Now we use a one-pass recursive iteration of thinkers.
  //   Another possible way to do this:
  //   First do a recursive iteration of the thinkers, ONLY to create the pointer->id map.
  //   Then iterate the map serializing each element in turn (first writing the ID, of course).

  i = 0;
  // only the stuff in the thinker ring is checked
  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      th->CheckPointers(); // clear pointers to deleted items
      i++;
    }

  a << i; // store the number of thinkers in ring

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    Thinker::Serialize(th, a);

  a.Marker(MARK_MISC);
  //----------------------------------------------
  // respawnqueue
  n = itemrespawnqueue.size();
  a << n;
  for (i=0; i<n; i++)
    {
      temp = itemrespawnqueue[i] - mapthings;
      a << temp;
      a << itemrespawntime[i];
    }

  //----------------------------------------------
  // the rest
  multimap<short, Actor *>::iterator t;
  n = TIDmap.size();
  a << n;
  for (t = TIDmap.begin(); t != TIDmap.end(); t++)
    {
      stemp = (*t).first;
      a << stemp;
      if (a.HasStored((*t).second, temp))
	a << temp;
      else
	I_Error("Crap in TIDmap!\n");
    }

  // TODO Sound sequences

  return 0;
}


int Map::Unserialize(LArchive &a)
{
  unsigned temp;
  short stemp;
  int i, n;

  if (!a.Marker(MARK_MAP))
    return -1;

  a.active_map = this; // so the Thinkers can be extracted OK

  // first we load and setup the map, but without spawning any Thinkers if possible
  a << lumpname;
  // TODO load map md5 checksum, make sure the correct map is loaded
  Setup(0, false);
  // remove all the current thinkers (could be done more elegantly by not spawning them
  // in the first place, but... well, at least the Actors are not spawned.)
  Thinker *th, *next;
  for (th = thinkercap.next; th != &thinkercap; th = next)
    {
      next = th->next;
      delete th;
    }
  InitThinkers();

  a << starttic << maptic;
  a << kills << items << secrets;

  line_t *li;
  side_t *si;
  byte    diff, diff2;

  // load changes in geometry
  while (1)
    {
      a << temp;
      if (temp == MARK_END)
	break;
      i = temp; // sector number

      a << diff;
      if (diff & SD_DIFF2)
	a << diff2;
      else
	diff2 = 0;

      char picname[8];

      if (diff & SD_FLOORHT ) a << sectors[i].floorheight;
      if (diff & SD_CEILHT  ) a << sectors[i].ceilingheight;
      if (diff & SD_FLOORPIC)
        {
	  a.Read((byte *)picname, 8);
	  sectors[i].floorpic = P_AddLevelFlat(picname, levelflats);
        }
      if (diff & SD_CEILPIC)
        {
	  a.Read((byte *)picname, 8);
	  sectors[i].ceilingpic = P_AddLevelFlat(picname, levelflats);
        }
      if (diff & SD_LIGHT)    a << sectors[i].lightlevel;
      if (diff & SD_SPECIAL)  a << sectors[i].special;
      if (diff & SD_TAG)      a << sectors[i].tag;

      if (diff2 & SD_FXOFFS)  a << sectors[i].floor_xoffs;
      if (diff2 & SD_FYOFFS)  a << sectors[i].floor_yoffs;
      if (diff2 & SD_CXOFFS)  a << sectors[i].ceiling_xoffs;
      if (diff2 & SD_CYOFFS)  a << sectors[i].ceiling_yoffs;

      if (diff2 & SD_STAIRLOCK) a << sectors[i].stairlock;
      else sectors[i].stairlock = 0;
      if (diff2 & SD_NEXTSEC)   a << sectors[i].nextsec;
      else sectors[i].nextsec = -1;
      if (diff2 & SD_PREVSEC)   a << sectors[i].prevsec;
      else sectors[i].prevsec = -1;
    }

  while (1)
    {
      a << temp; // line number
      if (temp == MARK_END)
	break;
      li = &lines[temp];

      a << diff;

      if (diff & LD_DIFF2) a << diff2;
      else diff2 = 0;
      if (diff & LD_FLAG)    a << li->flags;
      if (diff & LD_SPECIAL) a << li->special;
      if (diff & LD_ARGS)
	for (i=0; i<5; i++)
	  a << li->args[i];

      si = &sides[li->sidenum[0]];
      if (diff & LD_S1TEXOFF) a << si->textureoffset;
      if (diff & LD_S1TOPTEX) a << si->toptexture;
      if (diff & LD_S1BOTTEX) a << si->bottomtexture;
      if (diff & LD_S1MIDTEX) a << si->midtexture;

      si = &sides[li->sidenum[1]];
      if (diff2 & LD_S2TEXOFF) a << si->textureoffset;
      if (diff2 & LD_S2TOPTEX) a << si->toptexture;
      if (diff2 & LD_S2BOTTEX) a << si->bottomtexture;
      if (diff2 & LD_S2MIDTEX) a << si->midtexture;
    }

  //----------------------------------------------
  // TODO polyobjs

  //----------------------------------------------
  // scripts
  if (!a.Marker(MARK_SCRIPT))
    return -2;

  for (i = 0; i < ACScriptCount; i++)
    {
      a << int(ACSInfo[i].state);
      a << ACSInfo[i].waitValue;
    }
  a.Read((byte *)ACMapVars, sizeof(ACMapVars));

#ifdef FRAGGLESCRIPT
  // restore levelscript
  // free all the variables in the current levelscript first
  
  for (i=0; i<VARIABLESLOTS; i++)
    {
      svariable_t *sv = levelscript->variables[i];
      
      while (sv && sv->type != svt_label)
        {
          svariable_t *next = sv->next;
          Z_Free(sv);
          sv = next;
        }
      levelscript->variables[i] = sv;       // null or label
    }

  // now read the number of variables from the savegame file
  a << n;
  for (i=0; i<n; i++)
    {
      svariable_t *sv = (svariable_t*)Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);

      a << sv->name;
      Z_ChangeTag(sv->name, PU_LEVEL);
      a << sv->type;      
      switch (sv->type)
        {
        case svt_string:
	  a << sv->value.s;
	  Z_ChangeTag(sv->value.s, PU_LEVEL);
	  break;
        case svt_int:
	  a << sv->value.i;
	  break;
        case svt_actor:
	  sv->value.mobj = (Actor *)Thinker::Unserialize(a);
	  break;
        case svt_fixed:
	  a << sv->value.fixed;
	  break;
        }
      
      // link in the new variable
      int hashkey = variable_hash(sv->name);
      sv->next = levelscript->variables[hashkey];
      levelscript->variables[hashkey] = sv;
    }

  // restore runningscripts
  // remove all runningscripts first: levelscript may have started them
  T_ClearRunningScripts(); 

  a << n;
  for (i=0; i<n; i++)
    {
      // create a new runningscript
      runningscript_t *rs = new_runningscript();
  
      int scriptnum;
      a << scriptnum;
    
      // levelscript?
      if (scriptnum == -1)
	rs->script = levelscript;
      else
	rs->script = levelscript->children[scriptnum];

      a << n; // read out offset from save
      rs->savepoint = rs->script->data + n;
      a << n;
      rs->wait_type = wait_type_e(n);
      a << rs->wait_data;
      rs->trigger = (Actor *)Thinker::Unserialize(a);
    
      // read out the variables now (fun!)
      // start with basic script slots/labels
  
      for (i=0; i<VARIABLESLOTS; i++)
	rs->variables[i] = rs->script->variables[i];

      // get number of variables
      a << n;
      for (i=0; i<n; i++)
	{
	  svariable_t *sv = (svariable_t*)Z_Malloc(sizeof(svariable_t), PU_LEVEL, 0);
	  a << sv->name;
	  Z_ChangeTag(sv->name, PU_LEVEL);
	  a << sv->type;      
	  switch (sv->type)
	    {
	    case svt_string:
	      a << sv->value.s;
	      Z_ChangeTag(sv->value.s, PU_LEVEL);
	      break;
	    case svt_int:
	      a << sv->value.i;
	      break;
	    case svt_actor:
	      sv->value.mobj = (Actor *)Thinker::Unserialize(a);
	      break;
	    case svt_fixed:
	      a << sv->value.fixed;
	      break;
	    }

	  // link in the new variable
	  int hashkey = variable_hash(sv->name);
	  sv->next = rs->variables[hashkey];
	  rs->variables[hashkey] = sv;
	}
      
      // hook into chain
      T_AddRunningScript(rs);
    }

  // TODO Unarchive the script camera
#endif // FRAGGLESCRIPT

  //----------------------------------------------
  // Thinkers
  if (!a.Marker(MARK_THINK))
    return -2;

  a << n;
  for (i=0; i<n; i++)
    {
      Thinker *th = Thinker::Unserialize(a);
      AddThinker(th);
    }

  if (!a.Marker(MARK_MISC))
    return -2;
  //----------------------------------------------
  // respawnqueue
  a << n;
  for (i=0; i<n; i++)
    {
      a << temp;
      itemrespawnqueue.push_back(&mapthings[temp]);
      a << temp;
      itemrespawntime.push_back(temp);
    }

  //----------------------------------------------
  // the rest
  TIDmap.clear();
  a << n;
  for (i=0; i<n; i++)
    {
      Actor *p = NULL;
      a << stemp << temp;
      if (a.GetPtr(temp, (void *)p))
	TIDmap.insert(pair<const short, Actor*>(stemp, p));
      else
	I_Error("Crap in TIDmap!\n");
    }


  return 0;
}


//==============================================
//  PlayerInfo serialization
//==============================================

int PlayerInfo::Serialize(LArchive &a)
{
  int i;

  a << number << team;
  a << name;
  a << ptype << color << skin;
  a << int(playerstate);
  a << requestmap << entrypoint;

  a << score;
  a << kills << items << secrets << time;

  for (i=0; i<NUMWEAPONS; i++)
    a << favoriteweapon[i];

  a << originalweaponswitch << autoaim << spectator;

  a << (i = Frags.size());
  map<int, int>::iterator t;
  for (t = Frags.begin(); t != Frags.end(); t++)
    {
      int m = (*t).first;
      int n = (*t).second;
      a << m << n;
    }

  // the pawn is serialized by the map it is in, not here. mp?

  return 0;
}

int PlayerInfo::Unserialize(LArchive &a)
{
  int t1, t2, i, n;

  a << number << team;
  a << name;
  a << ptype << color << skin;
  a << int(playerstate);
  a << requestmap << entrypoint;

  a << score;
  a << kills << items << secrets << time;

  for (i=0; i<NUMWEAPONS; i++)
    a << favoriteweapon[i];

  a << originalweaponswitch << autoaim << spectator;

  Frags.clear();
  a << n;
  for (i=0; i<n; i++)
    {
      a << t1 << t2;
      Frags.insert(pair<int, int>(t1, t2));
    }

  // the pawn is unserialized by the map it is in, not here

  return 0;
}


int MapCluster::Serialize(LArchive &a)
{
  a << number;
  a << clustername;
  a << hub << keepstuff;

  a << kills << items << secrets;
  a << time << partime;

  a << interpic << intermusic;
  a << entertext << exittext << finalepic << finalemusic << episode;

  int n;
  a << (n = maps.size());
  for (int i=0; i<n; i++)
    a << maps[i]->mapnumber;
  return 0;
}


int MapCluster::Unserialize(LArchive &a)
{
  a << number;
  a << clustername;
  a << hub << keepstuff;

  a << kills << items << secrets;
  a << time << partime;

  a << interpic << intermusic;
  a << entertext << exittext << finalepic << finalemusic << episode;

  int n, temp;
  a << n;
  for (int i=0; i<n; i++)
    {
      a << temp;
      maps.push_back(game.FindMapInfo(temp));
    }
  return 0;
}

int MapInfo::Serialize(LArchive &a)
{
  int temp;
  a << int(state);

  a << lumpname << nicename << savename;
  a << cluster << mapnumber;

  // only save those items that cannot be found in the MapInfo separator lump for sure!
  // note that MAPINFO is not read when loading a game!
  a << partime;
  a << musiclump;

  a << nextlevel << secretlevel;
  a << doublesky << lightning;

  a << sky1 << sky1sp << sky2 << sky2sp;
  a << cdtrack << fadetablelump;
  a << BossDeathKey;

  if (state == MAP_SAVED)
    {
      // utilize the existing hubsave file
      a << (temp = 2);
      byte *buffer;
      int length = FIL_ReadFile(savename.c_str(), &buffer);
      if (!length)
	{
	  I_Error("Couldn't read hubsave file %s", savename.c_str());
	  return -1;
	}

      a << length;
      a.Write(buffer, length);
      Z_Free(buffer);
    }
  else if (me)
    {
      a << (temp = 1);
      me->Serialize(a);
    }
  else
    a << (temp = 0);

  return 0;
}

int MapInfo::Unserialize(LArchive &a)
{
  int temp;
  a << int(state);

  a << lumpname << nicename << savename;
  a << cluster << mapnumber;

  // only save those items that cannot be found in the MapInfo separator lump for sure!
  // note that MAPINFO is not read when loading a game!
  a << partime;
  a << musiclump;

  a << nextlevel << secretlevel;
  a << doublesky << lightning;

  a << sky1 << sky1sp << sky2 << sky2sp;
  a << cdtrack << fadetablelump;
  a << BossDeathKey;

  a << temp;
  if (temp == 2)
    {
      // extract the hubsave file
      int length;
      a << length;
      byte *buffer = (byte *)Z_Malloc(length, PU_STATIC, NULL);
      a.Read(buffer, length);
      FIL_WriteFile(savename.c_str(), buffer, length);
      Z_Free(buffer);
    }
  else if (temp == 1)
    {
      me = new Map(this);
      me->Unserialize(a);
    }
  else
    me = NULL;

  return 0;
}


int TeamInfo::Serialize(LArchive &a)
{
  a << name << color << score;
  return 0;
}

int TeamInfo::Unserialize(LArchive &a)
{
  a << name << color << score;
  return 0;
}


// takes a snapshot of the entire game state and stores it in the archive
int GameInfo::Serialize(LArchive &a)
{
  int i, n;

  // gameaction is always ga_nothing here

  // treat all enums as ints
  a << int(demoversion);
  a << int(mode);
  a << int(mission);
  a << int(state) << int(wipestate);
  a << int(skill);

  // flags
  a << netgame << multiplayer << modified << nomonsters << paused << inventory;

  a << maxteams;
  a << maxplayers;

  // teams
  a << (n = teams.size());
  for (i = 0; i < n; i++)
    teams[i]->Serialize(a);

  a.Marker(MARK_GROUP);
  // players 
  a << (n = Players.size());
  player_iter_t j;
  for (j = Players.begin(); j != Players.end(); j++)
    (*j).second->Serialize(a);

  a.Marker(MARK_GROUP);
  // mapinfo (and maps)
  a << (n = mapinfo.size());
  mapinfo_iter_t k;
  for (k = mapinfo.begin(); k != mapinfo.end(); k++)
    (*k).second->Serialize(a);

  a.Marker(MARK_GROUP);
  // clustermap
  a << (n = clustermap.size());
  cluster_iter_t l;
  for (l = clustermap.begin(); l != clustermap.end(); l++)
    (*l).second->Serialize(a);

  a.Marker(MARK_GROUP);
  a << currentcluster->number;

  // global script data
  a.Write((byte *)WorldVars, sizeof(WorldVars));
  acsstore_iter_t t;
  a << (n = ACS_store.size());
  for (t = ACS_store.begin(); t != ACS_store.end(); t++) 
    a.Write((byte *)&(*t).second, sizeof(acsstore_t));

  // TODO FS hub_script, global_script...

  // misc shit
  if (consoleplayer)
    a << consoleplayer->number;
  else
    a << (n = -1);

  if (consoleplayer2)
    a << consoleplayer2->number;
  else
    a << (n = -1);

  n = P_GetRandIndex();
  a << n;

  //CV_SaveNetVars((char**)&save_p);
  //CV_LoadNetVars((char**)&save_p);

  // TODO
  // client info and other net stuff (player authentication?)
  // consvars
  // check if required resource files are to be found / can be downloaded

  return 0;
}


int GameInfo::Unserialize(LArchive &a)
{
  // FIXME all the containers should be emptied and old contents deleted somewhere. SV_Reset?
  // ClearTeams();
  ClearPlayers();
  Clear_mapinfo_clusterdef();
  ACS_store.clear();
  Z_FreeTags(PU_LEVEL, MAXINT);

  int i, n;
  // treat all enums as ints
  a << int(demoversion);
  a << int(mode);
  a << int(mission);
  a << int(state) << int(wipestate);
  a << int(skill);

  // flags
  a << netgame << multiplayer << modified << nomonsters << paused << inventory;

  a << maxteams;
  a << maxplayers;

  // teams
  a << n;
  teams.resize(n);
  for (i = 0; i < n; i++)
    {
      teams[i] = new TeamInfo;
      if (teams[i]->Unserialize(a))
	return -1;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;
  // players 
  a << n;
  for (i = 0; i < n; i++)
    {
      PlayerInfo *p = new PlayerInfo;
      if (p->Unserialize(a))
	return -2;
      Players[p->number] = p;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;
  // mapinfo (and maps)
  a << n;
  for (i = 0; i < n; i++)
    {
      MapInfo *m = new MapInfo;
      if (m->Unserialize(a))
	return -3;
      mapinfo[m->mapnumber] = m;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;
  // clustermap
  a << n;
  for (i = 0; i < n; i++)
    {
      MapCluster *c = new MapCluster;
      if (c->Unserialize(a))
	return -4;
      clustermap[c->number] = c;
    }

  if (!a.Marker(MARK_GROUP))
    return -1;
  a << n;
  currentcluster = clustermap[n];

  // global script data
  a.Read((byte *)WorldVars, sizeof(WorldVars));
  a << n;
  for (i = 0; i < n; i++)
    {
      acsstore_t temp;
      a.Read((byte *)&temp, sizeof(acsstore_t));
      ACS_store.insert(pair<const int, acsstore_t>(temp.tmap, temp));
    }

  // misc shit
  a << n;
  consoleplayer = FindPlayer(n);
  a << n;
  consoleplayer2 = FindPlayer(n);

  a << n;
  P_SetRandIndex(n);

  return 0;
}



char savegamename[256];

void GameInfo::LoadGame(int slot)
{
  CONS_Printf("Loading game...\n");
  char  savename[255];
  byte *savebuffer;

  sprintf(savename, savegamename, slot);

  int length = FIL_ReadFile(savename, &savebuffer);
  if (!length)
    {
      CONS_Printf("Couldn't open save file %s", savename);
      return;
    }

  LArchive a;
  if (!a.Open(savebuffer, length))
    return;

  Z_Free(savebuffer); // the compressed buffer is no longer needed

  if (demoplayback)  // reset game engine
    StopDemo();

  Downgrade(VERSION); // reset the game version

  automap.Close();
  hud.ST_Stop();

  // dearchive all the modifications
  if (Unserialize(a))
    {
      M_StartMessage ("Savegame file corrupted\n\nPress ESC\n", NULL, MM_NOTHING);
      Command_ExitGame_f();
      return;
    }

  if (consoleplayer && consoleplayer->pawn)
    hud.ST_Start(consoleplayer->pawn);

  action = ga_nothing;
  state = GS_LEVEL;
  paused = false;

  displayplayer = consoleplayer;
  displayplayer2 = consoleplayer2;

  // done
  /*
  if (setsizeneeded)
    R_ExecuteSetViewSize();

  R_FillBackScreen();  // draw the pattern into the back screen
  */
  CON_ToggleOff();
}


void GameInfo::SaveGame(int savegameslot, char *description)
{
  if (action != ga_nothing)
    return; // not while changing state

  LArchive a;
  a.Create(description); // create a new save archive
  Serialize(a);          // store the game state into it

  byte *buffer;
  unsigned length = a.Compress(&buffer, 0);  // take out the compressed data

  char filename[256];
  sprintf(filename, savegamename, savegameslot);

  CONS_Printf("Savegame: %d bytes\n", length);
  FIL_WriteFile(filename, buffer, length);

  Z_Free(buffer);

  consoleplayer->message = text[TXT_GGSAVED];
  //R_FillBackScreen();  // draw the pattern into the back screen
}
