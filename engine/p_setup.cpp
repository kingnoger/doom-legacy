// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2002 by DooM Legacy Team.
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
// Revision 1.7  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.6  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.5  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.4  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.1.1.1  2002/11/16 14:18:11  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.38  2001/08/13 16:27:44  hurdler
// Added translucency to linedef 300 and colormap to 3d-floors
//
// Revision 1.37  2001/08/12 22:08:40  hurdler
// Add alpha value for 3d water
//
// Revision 1.36  2001/08/12 17:57:15  hurdler
// Beter support of sector coloured lighting in hw mode
//
// Revision 1.35  2001/08/11 15:18:02  hurdler
// Add sector colormap in hw mode (first attempt)
//
// Revision 1.34  2001/08/08 20:34:43  hurdler
// Big TANDL update
//
// Revision 1.33  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.32  2001/07/28 16:18:37  bpereira
// no message
//
// Revision 1.31  2001/06/16 08:07:55  bpereira
// no message
//
// Revision 1.30  2001/05/27 13:42:48  bpereira
// no message
//
// Revision 1.29  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.28  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.27  2001/03/30 17:12:51  bpereira
// no message
//
// Revision 1.26  2001/03/19 21:18:48  metzgermeister
//   * missing textures in HW mode are replaced by default texture
//   * fixed crash bug with P_SpawnMissile(.) returning NULL
//   * deep water trick and other nasty thing work now in HW mode (tested with tnt/map02 eternal/map02)
//   * added cvar gr_correcttricks
//
// Revision 1.25  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.24  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.23  2000/11/04 16:23:43  bpereira
// no message
//
// Revision 1.22  2000/11/03 03:27:17  stroggonmeth
// Again with the bug fixing...
//
// Revision 1.21  2000/11/02 19:49:36  bpereira
// no message
//
// Revision 1.20  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.19  2000/10/02 18:25:45  bpereira
// no message
//
// Revision 1.18  2000/08/31 14:30:56  bpereira
// no message
//
// Revision 1.17  2000/08/11 21:37:17  hurdler
// fix win32 compilation problem
//
// Revision 1.16  2000/08/11 19:10:13  metzgermeister
// *** empty log message ***
//
// Revision 1.15  2000/05/23 15:22:34  stroggonmeth
// Not much. A graphic bug fixed.
//
// Revision 1.14  2000/05/03 23:51:00  stroggonmeth
// A few, quick, changes.
//
// Revision 1.13  2000/04/19 15:21:02  hurdler
// add SDL midi support
//
// Revision 1.12  2000/04/18 12:55:39  hurdler
// join with Boris' code
//
// Revision 1.11  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.10  2000/04/15 22:12:57  stroggonmeth
// Minor bug fixes
//
// Revision 1.9  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.8  2000/04/12 16:01:59  hurdler
// ready for T&L code and true static lighting
//
// Revision 1.7  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.6  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.5  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.4  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   part of Map class implementation
//      Do all the WAD I/O, get map description,
//             set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"
#include "byteptr.h"

#include "g_map.h"
#include "g_game.h"
#include "g_level.h"

#include "p_setup.h"
#include "m_bbox.h"
#include "p_spec.h"

#include "i_sound.h" //for I_PlayCD()..
#include "r_sky.h"

#include "r_render.h"
#include "r_data.h"
#include "r_state.h"

#include "r_things.h"
#include "r_sky.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"
#include "p_info.h"
#include "t_func.h"
#include "t_script.h"

#include "hu_stuff.h"
#include "console.h"


#ifdef HWRENDER
#include "i_video.h"            //rendermode
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#endif

//
// was P_LoadVertexes
//
void Map::LoadVertexes(int lump)
{
  byte*               data;
  int                 i;
  mapvertex_t*        ml;
  vertex_t*           li;

  // Determine number of lumps:
  //  total lump length / vertex record length.
  numvertexes = fc.LumpLength (lump) / sizeof(mapvertex_t);

  // Allocate zone memory for buffer.
  vertexes = (vertex_t*)Z_Malloc(numvertexes*sizeof(vertex_t),PU_LEVEL,0);

  // Load data into cache.
  data = (byte *)fc.CacheLumpNum(lump,PU_STATIC);

  ml = (mapvertex_t *)data;
  li = vertexes;

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (i=0 ; i<numvertexes ; i++, li++, ml++)
    {
      li->x = SHORT(ml->x)<<FRACBITS;
      li->y = SHORT(ml->y)<<FRACBITS;
    }

  // Free buffer memory.
  Z_Free(data);
}


//
// Computes the line length in frac units, the glide render needs this
//
#define crapmul (1.0f / 65536.0f)

float P_SegLength (seg_t* seg)
{
  double      dx,dy;

  // make a vector (start at origin)
  dx = (seg->v2->x - seg->v1->x)*crapmul;
  dy = (seg->v2->y - seg->v1->y)*crapmul;

  return sqrt(dx*dx+dy*dy)*FRACUNIT;
}


//
// was P_LoadSegs
//
void Map::LoadSegs(int lump)
{
  byte*               data;
  int                 i;
  mapseg_t*           ml;
  seg_t*              li;
  line_t*             ldef;
  int                 linedef;
  int                 side;

  numsegs = fc.LumpLength (lump) / sizeof(mapseg_t);
  segs = (seg_t *)Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
  memset (segs, 0, numsegs*sizeof(seg_t));
  data = (byte *)fc.CacheLumpNum (lump,PU_STATIC);

  ml = (mapseg_t *)data;
  li = segs;
  for (i=0 ; i<numsegs ; i++, li++, ml++)
    {
      li->v1 = &vertexes[SHORT(ml->v1)];
      li->v2 = &vertexes[SHORT(ml->v2)];

#ifdef HWRENDER // not win32 only 19990829 by Kin
      // used for the hardware render
      if (rendermode != render_soft)
        {
	  li->length = P_SegLength (li);
	  //Hurdler: 04/12/2000: for now, only used in hardware mode
	  li->lightmaps = NULL; // list of static lightmap for this seg
        }
#endif

      li->angle = (SHORT(ml->angle))<<16;
      li->offset = (SHORT(ml->offset))<<16;
      linedef = SHORT(ml->linedef);
      ldef = &lines[linedef];
      li->linedef = ldef;
      side = SHORT(ml->side);
      li->sidedef = &sides[ldef->sidenum[side]];
      li->frontsector = sides[ldef->sidenum[side]].sector;
      if (ldef-> flags & ML_TWOSIDED)
	li->backsector = sides[ldef->sidenum[side^1]].sector;
      else
	li->backsector = 0;

      li->numlights = 0;
      li->rlights = NULL;
    }

  Z_Free (data);
}


//
// was P_LoadSubsectors
//
void Map::LoadSubsectors(int lump)
{
  byte*               data;
  int                 i;
  mapsubsector_t*     ms;
  subsector_t*        ss;

  numsubsectors = fc.LumpLength (lump) / sizeof(mapsubsector_t);
  subsectors = (subsector_t *)Z_Malloc(numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
  data = (byte *)fc.CacheLumpNum (lump,PU_STATIC);

  ms = (mapsubsector_t *)data;
  memset (subsectors,0, numsubsectors*sizeof(subsector_t));
  ss = subsectors;

  for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
      ss->numlines = SHORT(ms->numsegs);
      ss->firstline = SHORT(ms->firstseg);
    }

  Z_Free (data);
}



//
// P_LoadSectors
//

//
// levelflats
//
#define MAXLEVELFLATS   256

int                     numlevelflats;
levelflat_t*            levelflats;

//SoM: Other files want this info.
int P_PrecacheLevelFlats()
{
  int flatmemory = 0;
  int i;
  int lump;

  //SoM: 4/18/2000: New flat code to make use of levelflats.
  for(i = 0; i < numlevelflats; i++)
    {
      lump = levelflats[i].lumpnum;
      if(devparm)
	flatmemory += fc.LumpLength(lump);
      R_GetFlat (lump);
    }
  return flatmemory;
}




int P_FlatNumForName(char *flatname)
{
  return P_AddLevelFlat(flatname, levelflats);
}



// help function for P_LoadSectors, find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
//
int P_AddLevelFlat (char* flatname, levelflat_t* levelflat)
{
  union {
    char    s[9];
    int     x[2];
  } name8;

  int         i;
  int         v1,v2;

  strncpy (name8.s,flatname,8);   // make it two ints for fast compares
  name8.s[8] = 0;                 // in case the name was a fill 8 chars
  strupr (name8.s);               // case insensitive
  v1 = name8.x[0];
  v2 = name8.x[1];

  //
  //  first scan through the already found flats
  //
  for (i=0; i<numlevelflats; i++,levelflat++)
    {
      if ( *(int *)levelflat->name == v1
	   && *(int *)&levelflat->name[4] == v2)
        {
	  break;
        }
    }

  // that flat was already found in the level, return the id
  if (i==numlevelflats)
    {
      // store the name
      *((int*)levelflat->name) = v1;
      *((int*)&levelflat->name[4]) = v2;

      // store the flat lump number
      levelflat->lumpnum = R_GetFlatNumForName (flatname);

      if (devparm)
	CONS_Printf ("flat %#03d: %s\n", numlevelflats, name8.s);

      numlevelflats++;

      if (numlevelflats>=MAXLEVELFLATS)
	I_Error("P_LoadSectors: too many flats in level\n");
    }

  // level flat id
  return i;
}


// SoM: Do I really need to comment this?
char *P_FlatNameForNum(int num)
{
  if(num < 0 || num > numlevelflats)
    I_Error("P_FlatNameForNum: Invalid flatnum\n");

  return Z_Strdup(va("%.8s", levelflats[num].name), PU_STATIC, 0);
}


// was P_LoadSectors

void Map::LoadSectors(int lump)
{
  byte*               data;
  int                 i;
  mapsector_t*        ms;
  sector_t*           ss;

  levelflat_t*        foundflats;

  numsectors = fc.LumpLength (lump) / sizeof(mapsector_t);
  sectors = (sector_t *)Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
  memset (sectors, 0, numsectors*sizeof(sector_t));
  data = (byte *)fc.CacheLumpNum (lump,PU_STATIC);

  //Fab:FIXME: allocate for whatever number of flats
  //           512 different flats per level should be plenty
  foundflats = (levelflat_t *)Z_Malloc(sizeof(levelflat_t) * MAXLEVELFLATS, PU_STATIC, 0);
  if (!foundflats)
    I_Error ("P_LoadSectors: no mem\n");
  memset (foundflats, 0, sizeof(levelflat_t) * MAXLEVELFLATS);

  numlevelflats = 0;

  ms = (mapsector_t *)data;
  ss = sectors;
  for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
      ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

      //
      //  flats
      //
      if( strnicmp(ms->floorpic,"FWATER",6)==0 || 
	  strnicmp(ms->floorpic,"FLTWAWA1",8)==0 ||
	  strnicmp(ms->floorpic,"FLTFLWW1",8)==0 )
	ss->floortype = FLOOR_WATER;
      else
        if( strnicmp(ms->floorpic,"FLTLAVA1",8)==0 ||
            strnicmp(ms->floorpic,"FLATHUH1",8)==0 )
	  ss->floortype = FLOOR_LAVA;
        else
	  if( strnicmp(ms->floorpic,"FLTSLUD1",8)==0 )
            ss->floortype = FLOOR_SLUDGE;
	  else
            ss->floortype = FLOOR_SOLID;

      ss->floorpic = P_AddLevelFlat (ms->floorpic,foundflats);
      ss->ceilingpic = P_AddLevelFlat (ms->ceilingpic,foundflats);

      ss->lightlevel = SHORT(ms->lightlevel);
      ss->special = SHORT(ms->special);
      ss->tag = SHORT(ms->tag);

      //added:31-03-98: quick hack to test water with DCK
      /*        if (ss->tag < 0)
		CONS_Printf("Level uses dck-water-hack\n");*/

      ss->thinglist = NULL;
      ss->touching_thinglist = NULL; //SoM: 4/7/2000

      ss->stairlock = 0;
      ss->nextsec = -1;
      ss->prevsec = -1;

      ss->heightsec = -1; //SoM: 3/17/2000: This causes some real problems
      ss->altheightsec = 0; //SoM: 3/20/2000
      ss->floorlightsec = -1;
      ss->ceilinglightsec = -1;
      ss->ffloors = NULL;
      ss->lightlist = NULL;
      ss->numlights = 0;
      ss->attached = NULL;
      ss->numattached = 0;
      ss->moved = true;
      ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
      ss->bottommap = ss->midmap = ss->topmap = -1;
        
      // ----- for special tricks with HW renderer -----
      ss->pseudoSector = false;
      ss->virtualFloor = false;
      ss->virtualCeiling = false;
      ss->sectorLines = NULL;
      ss->stackList = NULL;
      ss->lineoutLength = -1.0;
      // ----- end special tricks -----
        
    }

  Z_Free (data);

  // whoa! there is usually no more than 25 different flats used per level!!
  //CONS_Printf ("%d flats found\n", numlevelflats);

  // set the sky flat num
  skyflatnum = P_AddLevelFlat ("F_SKY1",foundflats);

  // copy table for global usage
  levelflats = (levelflat_t *)Z_Malloc (numlevelflats*sizeof(levelflat_t),PU_LEVEL,0);
  memcpy (levelflats, foundflats, numlevelflats*sizeof(levelflat_t));
  Z_Free(foundflats);

  // search for animated flats and set up
  P_SetupLevelFlatAnims();
}


//
// was P_LoadNodes
//
void Map::LoadNodes(int lump)
{
  byte*       data;
  int         i;
  int         j;
  int         k;
  mapnode_t*  mn;
  node_t*     no;

  numnodes = fc.LumpLength (lump) / sizeof(mapnode_t);
  nodes = (node_t *)Z_Malloc(numnodes*sizeof(node_t),PU_LEVEL,0);
  data = (byte*)fc.CacheLumpNum (lump,PU_STATIC);

  mn = (mapnode_t *)data;
  no = nodes;

  for (i=0 ; i<numnodes ; i++, no++, mn++)
    {
      no->x = SHORT(mn->x)<<FRACBITS;
      no->y = SHORT(mn->y)<<FRACBITS;
      no->dx = SHORT(mn->dx)<<FRACBITS;
      no->dy = SHORT(mn->dy)<<FRACBITS;
      for (j=0 ; j<2 ; j++)
        {
	  no->children[j] = SHORT(mn->children[j]);
	  for (k=0 ; k<4 ; k++)
	    no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
        }
    }

  Z_Free (data);
}

//
// was P_LoadThings
//
void Map::LoadThings(int lump)
{
  char *data, *datastart;

  data = datastart = (char *)fc.CacheLumpNum (lump,PU_LEVEL);
  nummapthings     = fc.LumpLength (lump) / (5 * sizeof(short));
  mapthings        = (mapthing_t *)Z_Malloc(nummapthings * sizeof(mapthing_t), PU_LEVEL, NULL);

  //SoM: Because I put a new member into the mapthing_t for use with
  //fragglescript, the format has changed and things won't load correctly
  //using the old method.

  int ffail = 0;
  // multiplayer only thing flag
  if (!game.multiplayer)
    ffail |= MTF_MULTIPLAYER;

  extern consvar_t cv_deathmatch;

  // "not deathmatch"/"not coop" thing flags
  if (game.netgame && cv_deathmatch.value)
    ffail |= MTF_NOT_IN_DM;
  else if (game.netgame && !cv_deathmatch.value)
    ffail |= MTF_NOT_IN_COOP;

  // check skill
  int skillbit;
  if (game.skill == sk_baby)
    skillbit = 1;
  else if (game.skill == sk_nightmare)
    skillbit = 4;
  else
    skillbit = 1 << (game.skill-1);

  mapthing_t *mt = mapthings;

  int i, n, low, high, ednum;
  for (i=0 ; i<nummapthings ; i++, mt++)
    {
      mt->x = SHORT(READSHORT(data));
      mt->y = SHORT(READSHORT(data));
      mt->angle = SHORT(READSHORT(data));

      ednum = SHORT(READSHORT(data));

      mt->flags = SHORT(READSHORT(data));
      mt->mobj = NULL; //SoM:

      // wrong flags?
      if ((mt->flags & ffail) || !(mt->flags & skillbit))
	continue;

      // convert editor number to mobjtype_t number right now
      if (!ednum)
	continue; // Ignore type-0 things as NOPs

      // deathmatch start positions
      if (ednum == 11)
	{
	  if (dmstarts.size() < MAX_DM_STARTS)
	    {
	      dmstarts.push_back(mt);
	      mt->type = 0;
	    }
	  continue;
	}

      // normal playerstarts (normal 4 + 28 extra)
      if ((ednum >= 1 && ednum <= 4) || (ednum >= 4001 && ednum <= 4028))
	{
	  if (ednum > 4000)
	    ednum -= 4001 - 5;
	  // save spots for respawning in network games
	  if (playerstarts.size() < ednum)
	    playerstarts.resize(ednum);
	  playerstarts[ednum - 1] = mt;
	  mt->type = 0; // mt->type is used as a timer
	  continue;
	}

      // Ambient sound sequences
      if (ednum >= 1200 && ednum < 1300)
	{
	  AddAmbientSfx(ednum - 1200);
	  continue;
	}

      if (ednum == 14)
	{
	  // ugly HACK, FIXME somehow!
	  // same with doom and heretic, but only one mobjtype_t
	  mt->type = MT_TELEPORTMAN;
	  SpawnMapThing(mt);
	  continue;
	}

      low = 0;
      high = 0;

      // find which type to spawn
      if (ednum >= info->doom_offs[0] && ednum <= info->doom_offs[1])
	{
	  ednum -= info->doom_offs[0];
	  low = MT_DOOM;
	  high = MT_DOOM_END;
	}
      else if (ednum >= info->heretic_offs[0] && ednum <= info->heretic_offs[1])
	{
	  ednum -= info->heretic_offs[0];
	  low = MT_HERETIC;
	  high = MT_HERETIC_END;

	  // D'Sparil teleport spot (no Actor spawned)
	  if (ednum == 56)
	    {
	      BossSpots.push_back(mt);
	      /*
		BossSpots[BossSpotCount].x = mthing->x << FRACBITS;
		BossSpots[BossSpotCount].y = mthing->y << FRACBITS;
		BossSpots[BossSpotCount].angle = ANG45 * (mthing->angle/45);
	      */
	      continue;
	    }

	  // Mace spot (no Actor spawned)
	  if (ednum == 2002)
	    {
	      MaceSpots.push_back(mt);
	      /*
		MaceSpots[MaceSpotCount].x = mthing->x<<FRACBITS;
		MaceSpots[MaceSpotCount].y = mthing->y<<FRACBITS;
	      */
	      continue;
	    }
	}

      for (n = low; n <= high; n++)
	if (ednum == mobjinfo[n].doomednum)
	  break;

      if (n > high)
	{
	  CONS_Printf("\2P_SpawnMapThing: Unknown type %i at (%i, %i)\n",
		      ednum, mt->x, mt->y);
	  continue;
	}

      // DoomII braintarget list
      if (n == MT_BOSSTARGET)
	braintargets.push_back(mt);

      // spawn here
      mt->type = mobjtype_t(n); 
      SpawnMapThing(mt);
    }

  Z_Free(datastart);
}


//
// was P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void Map::LoadLineDefs(int lump)
{
  byte*               data;
  int                 i;
  maplinedef_t*       mld;
  line_t*             ld;
  vertex_t*           v1;
  vertex_t*           v2;

  numlines = fc.LumpLength (lump) / sizeof(maplinedef_t);
  lines = (line_t *)Z_Malloc(numlines*sizeof(line_t),PU_LEVEL,0);
  memset (lines, 0, numlines*sizeof(line_t));
  data = (byte *)fc.CacheLumpNum (lump,PU_STATIC);

  mld = (maplinedef_t *)data;
  ld = lines;
  for (i=0 ; i<numlines ; i++, mld++, ld++)
    {
      ld->flags = SHORT(mld->flags);
      ld->special = SHORT(mld->special);
      ld->tag = SHORT(mld->tag);
      v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
      v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
      ld->dx = v2->x - v1->x;
      ld->dy = v2->y - v1->y;

      if (!ld->dx)
	ld->slopetype = ST_VERTICAL;
      else if (!ld->dy)
	ld->slopetype = ST_HORIZONTAL;
      else
        {
	  if (FixedDiv (ld->dy , ld->dx) > 0)
	    ld->slopetype = ST_POSITIVE;
	  else
	    ld->slopetype = ST_NEGATIVE;
        }

      if (v1->x < v2->x)
        {
	  ld->bbox[BOXLEFT] = v1->x;
	  ld->bbox[BOXRIGHT] = v2->x;
        }
      else
        {
	  ld->bbox[BOXLEFT] = v2->x;
	  ld->bbox[BOXRIGHT] = v1->x;
        }

      if (v1->y < v2->y)
        {
	  ld->bbox[BOXBOTTOM] = v1->y;
	  ld->bbox[BOXTOP] = v2->y;
        }
      else
        {
	  ld->bbox[BOXBOTTOM] = v2->y;
	  ld->bbox[BOXTOP] = v1->y;
        }

      ld->sidenum[0] = SHORT(mld->sidenum[0]);
      ld->sidenum[1] = SHORT(mld->sidenum[1]);

      if (ld->sidenum[0] != -1 && ld->special)
	sides[ld->sidenum[0]].special = ld->special;

    }

  Z_Free (data);
}


// was P_LoadLineDefs2

void Map::LoadLineDefs2()
{
  int i;
  line_t* ld = lines;
  for(i = 0; i < numlines; i++, ld++)
    {
      if (ld->sidenum[0] != -1)
	ld->frontsector = sides[ld->sidenum[0]].sector;
      else
	ld->frontsector = 0;

      if (ld->sidenum[1] != -1)
	ld->backsector = sides[ld->sidenum[1]].sector;
      else
	ld->backsector = 0;
    }
}


//
// P_LoadSideDefs
//
/*void P_LoadSideDefs (int lump)
  {
  byte*               data;
  int                 i;
  mapsidedef_t*       msd;
  side_t*             sd;

  numsides = fc.LumpLength (lump) / sizeof(mapsidedef_t);
  sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);
  memset (sides, 0, numsides*sizeof(side_t));
  data = fc.CacheLumpNum (lump,PU_STATIC);

  msd = (mapsidedef_t *)data;
  sd = sides;
  for (i=0 ; i<numsides ; i++, msd++, sd++)
  {
  sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
  sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
  sd->toptexture = R_TextureNumForName(msd->toptexture);
  sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
  sd->midtexture = R_TextureNumForName(msd->midtexture);

  sd->sector = &sectors[SHORT(msd->sector)];
  }

  Z_Free (data);
  }*/

// was P_LoadSideDefs

void Map::LoadSideDefs(int lump)
{
  numsides = fc.LumpLength(lump) / sizeof(mapsidedef_t);
  sides = (side_t *)Z_Malloc(numsides*sizeof(side_t),PU_LEVEL,0);
  memset(sides, 0, numsides*sizeof(side_t));
}

// SoM: 3/22/2000: Delay loading texture names until after loaded linedefs.

//Hurdler: 04/04/2000: proto added
int R_ColormapNumForName(char *name);

// was P_LoadSideDefs2

void Map::LoadSideDefs2(int lump)
{
  byte *data = (byte *)fc.CacheLumpNum(lump,PU_STATIC);
  int  i;
  int  num;
  int  mapnum;

  for (i=0; i<numsides; i++)
    {
      register mapsidedef_t *msd = (mapsidedef_t *) data + i;
      register side_t *sd = sides + i;
      register sector_t *sec;

      sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
      sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

      // refined to allow colormaps to work as wall
      // textures if invalid as colormaps but valid as textures.

      sd->sector = sec = &sectors[SHORT(msd->sector)];
      switch (sd->special)
        {
        case 242:                       // variable colormap via 242 linedef
        case 280:                       //SoM: 3/22/2000: New water type.
#ifdef HWRENDER
          if(rendermode == render_soft)
	    {
#endif
	      num = R_CheckTextureNumForName(msd->toptexture);

	      if(num == -1)
		{
		  sec->topmap = mapnum = R_ColormapNumForName(msd->toptexture);
		  sd->toptexture = 0;
		}
	      else
		sd->toptexture = num;

	      num = R_CheckTextureNumForName(msd->midtexture);
	      if(num == -1)
		{
		  sec->midmap = mapnum = R_ColormapNumForName(msd->midtexture);
		  sd->midtexture = 0;
		}
	      else
		sd->midtexture = num;

	      num = R_CheckTextureNumForName(msd->bottomtexture);
	      if(num == -1)
		{
		  sec->bottommap = mapnum = R_ColormapNumForName(msd->bottomtexture);
		  sd->bottomtexture = 0;
		}
	      else
		sd->bottomtexture = num;
	      break;
#ifdef HWRENDER
	    }
          else
	    {
	      if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
		sd->toptexture = 0;
	      else
		sd->toptexture = num;

	      if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
		sd->midtexture = 0;
	      else
		sd->midtexture = num;

	      if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
		sd->bottomtexture = 0;
	      else
		sd->bottomtexture = num;

	      break;
	    }
#endif
        case 282:                       //SoM: 4/4/2000: Just colormap transfer
#ifdef HWRENDER
          if(rendermode == render_soft)
	    {
#endif
	      if(msd->toptexture[0] == '#' || msd->bottomtexture[0] == '#')
		{
		  sec->midmap = R_CreateColormap(msd->toptexture, msd->midtexture, msd->bottomtexture);
		  sd->toptexture = sd->bottomtexture = 0;
		}
	      else
		{
		  if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
		    sd->toptexture = 0;
		  else
		    sd->toptexture = num;
		  if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
		    sd->midtexture = 0;
		  else
		    sd->midtexture = num;
		  if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
		    sd->bottomtexture = 0;
		  else
		    sd->bottomtexture = num;
		}

#ifdef HWRENDER
	    }
          else
	    {
	      //Hurdler: for now, full support of toptexture only
	      if(msd->toptexture[0] == '#')// || msd->bottomtexture[0] == '#')
		{
		  char *col = msd->toptexture;

		  sec->midmap = R_CreateColormap(msd->toptexture, msd->midtexture, msd->bottomtexture);
		  sd->toptexture = sd->bottomtexture = 0;
# define HEX2INT(x) (x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
# define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : 0)
		  sec->extra_colormap = &extra_colormaps[sec->midmap];
		  sec->extra_colormap->rgba = 
		    (HEX2INT(col[1]) << 4) + (HEX2INT(col[2]) << 0) +
		    (HEX2INT(col[3]) << 12) + (HEX2INT(col[4]) << 8) +
		    (HEX2INT(col[5]) << 20) + (HEX2INT(col[6]) << 16) + 
		    (ALPHA2INT(col[7]) << 24);
# undef ALPHA2INT
# undef HEX2INT
		}
	      else
		{
		  if((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
		    sd->toptexture = 0;
		  else
		    sd->toptexture = num;

		  if((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
		    sd->midtexture = 0;
		  else
		    sd->midtexture = num;
		  
		  if((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
		    sd->bottomtexture = 0;
		  else
		    sd->bottomtexture = num;
		}
	      break;
	    }
#endif

        case 260:
          num = R_CheckTextureNumForName(msd->midtexture);
          if(num == -1)
            sd->midtexture = 1;
          else
            sd->midtexture = num;

          num = R_CheckTextureNumForName(msd->toptexture);
          if(num == -1)
            sd->toptexture = 1;
          else
            sd->toptexture = num;

          num = R_CheckTextureNumForName(msd->bottomtexture);
          if(num == -1)
            sd->bottomtexture = 1;
          else
            sd->bottomtexture = num;
          break;

	  /*        case 260: // killough 4/11/98: apply translucency to 2s normal texture
		    sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
		    (sd->special = fc.CheckNumForName(msd->midtexture)) < 0 ||
		    fc.LumpLength(sd->special) != 65536 ?
		    sd->special=0, R_TextureNumForName(msd->midtexture) :
		    (sd->special++, 0) : (sd->special=0);
		    sd->toptexture = R_TextureNumForName(msd->toptexture);
		    sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		    break;*/ //This code is replaced.. I need to fix this though


	  //Hurdler: added for alpha value with translucent 3D-floors/water
        case 300:
        case 301:
	  if(msd->toptexture[0] == '#')
            {
	      char *col = msd->toptexture;
	      sd->toptexture = sd->bottomtexture = ((col[1]-'0')*100+(col[2]-'0')*10+col[3]-'0')+1;
            }
	  else
	    sd->toptexture = sd->bottomtexture = 0;
	  sd->midtexture = R_TextureNumForName(msd->midtexture);
	  break;

        default:                        // normal cases
          sd->midtexture = R_TextureNumForName(msd->midtexture);
          sd->toptexture = R_TextureNumForName(msd->toptexture);
          sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
          break;
        }
    }
  Z_Free (data);
}




//
// was P_LoadBlockMap
//
void Map::LoadBlockMap(int lump)
{
  int         i;
  int         count;

  blockmaplump = (short int *)fc.CacheLumpNum(lump, PU_LEVEL);
  blockmap = blockmaplump+4;
  count = fc.LumpLength(lump)/2;

  for (i=0 ; i<count ; i++)
    blockmaplump[i] = SHORT(blockmaplump[i]);

  bmaporgx = blockmaplump[0]<<FRACBITS;
  bmaporgy = blockmaplump[1]<<FRACBITS;
  bmapwidth = blockmaplump[2];
  bmapheight = blockmaplump[3];

  // clear out mobj chains
  count = sizeof(*blocklinks)* bmapwidth*bmapheight;
  blocklinks = (Actor **)Z_Malloc(count, PU_LEVEL, 0);
  memset(blocklinks, 0, count);
}



//
// was P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void Map::GroupLines()
{
  line_t**            linebuffer;
  int                 i;
  int                 j;
  int                 total;
  line_t*             li;
  sector_t*           sector;
  subsector_t*        ss;
  seg_t*              seg;
  fixed_t             bbox[4];
  int                 block;

  // look up sector number for each subsector
  ss = subsectors;
  for (i=0 ; i<numsubsectors ; i++, ss++)
    {
      seg = &segs[ss->firstline];
      ss->sector = seg->sidedef->sector;
    }

  // count number of lines in each sector
  li = lines;
  total = 0;
  for (i=0 ; i<numlines ; i++, li++)
    {
      total++;
      li->frontsector->linecount++;

      if (li->backsector && li->backsector != li->frontsector)
        {
	  li->backsector->linecount++;
	  total++;
        }
    }

  // build line tables for each sector
  // FIXME! sizeof(something) into Z_malloc
  linebuffer = (line_t **)Z_Malloc (total*4, PU_LEVEL, 0);
  sector = sectors;
  for (i=0 ; i<numsectors ; i++, sector++)
    {
      M_ClearBox (bbox);
      sector->lines = linebuffer;
      li = lines;
      for (j=0 ; j<numlines ; j++, li++)
        {
	  if (li->frontsector == sector || li->backsector == sector)
            {
	      *linebuffer++ = li;
	      M_AddToBox (bbox, li->v1->x, li->v1->y);
	      M_AddToBox (bbox, li->v2->x, li->v2->y);
            }
        }
      if (linebuffer - sector->lines != sector->linecount)
	I_Error ("P_GroupLines: miscounted");

      // set the degenmobj_t to the middle of the bounding box
      sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
      sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
      sector->soundorg.z = sector->floorheight-10;

      // adjust bounding box to map blocks
      block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block >= bmapheight ? bmapheight-1 : block;
      sector->blockbox[BOXTOP]=block;

      block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXBOTTOM]=block;

      block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block >= bmapwidth ? bmapwidth-1 : block;
      sector->blockbox[BOXRIGHT]=block;

      block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXLEFT]=block;
    }

}


// SoM: 6/27: Don't restrict maps to MAPxx/ExMx any more!
static char *levellumps[] =
{
  "label",        // ML_LABEL,    A separator, name, ExMx or MAPxx
  "THINGS",       // ML_THINGS,   Monsters, items..
  "LINEDEFS",     // ML_LINEDEFS, LineDefs, from editing
  "SIDEDEFS",     // ML_SIDEDEFS, SideDefs, from editing
  "VERTEXES",     // ML_VERTEXES, Vertices, edited and BSP splits generated
  "SEGS",         // ML_SEGS,     LineSegs, from LineDefs split by BSP
  "SSECTORS",     // ML_SSECTORS, SubSectors, list of LineSegs
  "NODES",        // ML_NODES,    BSP nodes
  "SECTORS",      // ML_SECTORS,  Sectors, from editing
  "REJECT",       // ML_REJECT,   LUT, sector-sector visibility
  "BLOCKMAP"      // ML_BLOCKMAP  LUT, motion clipping, walls/grid element
};


//
// P_CheckLevel
// Checks a lump and returns whether or not it is a level header lump.
/*
  bool P_CheckLevel(int lumpnum)
  {
  int  i;
  int  file, lump;
  
  for(i=ML_THINGS; i<=ML_BLOCKMAP; i++)
  {
  file = lumpnum >> 16;
  lump = (lumpnum & 0xffff) + i;
  if(file > numwadfiles || lump > wadfiles[file]->numlumps ||
  strncmp(wadfiles[file]->lumpinfo[lump].name, levellumps[i], 8) )
  return false;
  }
  return true;    // all right
  }
*/

// was P_SetupLevelSky
// Setup sky texture to use for the level, actually moved the code
// from G_DoLoadLevel() which had nothing to do there.
//
// - in future, each level may use a different sky.
// - No, in future we can use a skybox!
// The sky texture to be used instead of the F_SKY1 dummy.
void Map::SetupSky()
{
  // where does the sky texture come from?
  // 1. MapInfo skyname
  // 2. LevelNode skyname (mimics original Doom behavior)
  // 3. if everything else fails, use "SKY1" and hope for the best

  // original DOOM determined the sky texture to be used
  // depending on the current episode, and the game version.

  if (!info->skylump.empty())
    skytexture = R_TextureNumForName(info->skylump.c_str());
  else if (!level->skylump.empty()) 
    skytexture = R_TextureNumForName(level->skylump.c_str());
  else
    skytexture = R_TextureNumForName("SKY1");

  // scale up the old skies, if needed
  R_SetupSkyDraw();
}


extern int numtextures;
//char       *maplumpname;
//int        lastloadedmaplumpnum; // for comparative savegame

void P_Initsecnode();

//
// was P_SetupLevel
//
// (re)loads the map from an already opened wad
bool Map::Setup(tic_t start)
{
  extern  bool precache;

  CONS_Printf("Map::Setup: %s\n", mapname.c_str());

  maptic = 0;
  starttic = start;
  kills = items = secrets = 0;

  CON_Drawer();  // let the user know what we are going to do
  I_FinishUpdate();              // page flip or blit buffer

  //Initialize sector node list.
  P_Initsecnode();

  // Make sure all sounds are stopped before Z_FreeTags.
  S.Stop3DSounds();

#if 0 // UNUSED
  if (debugfile)
    {
      Z_FreeTags (PU_LEVEL, MAXINT);
      Z_FileDumpHeap (debugfile);
    }
  else
#endif

#ifdef WALLSPLATS
  // clear the splats from previous map
  R_ClearLevelSplats();
#endif

  HU_ClearTips();

  // UNUSED fc.Profile ();
    
  InitThinkers();

  // if working with a devlopment map, reload it
  // FIXME! temporarily removed, is it ever even necessary?
  // fc.Reload ();

  //
  //  load the map from internal game resource or external wad file
  //
  /*if (wadname)
    {
    char *firstmap=NULL;

    // go back to title screen if no map is loaded
    if (!P_AddWadFile (wadname,&firstmap) ||
    firstmap==NULL)            // no maps were found
    {
    return false;
    }

    // P_AddWadFile() sets lumpname
    lumpnum = fc.GetNumForName(firstmap);
    maplumpname = firstmap;
    } else
  */
  // internal game map
  lumpnum = fc.GetNumForName(mapname.c_str());
	
  // textures are needed first
  //    R_LoadTextures ();
  //    R_FlushTextureCache();

  R_ClearColormaps();

#ifdef FRAGGLESCRIPT
  script_camera_on = false;
  T_ClearScripts();
#endif

  info = new MapInfo;
  info->Load(lumpnum); // load map separator lump info (map name etc)

  // If the map defines its music in MapInfo, use it.
  // Otherwise use given LevelNode data.
  if (!info->musiclump.empty())
    S.StartMusic(info->musiclump.c_str());
  else
    S.StartMusic(level->musiclump.c_str());

  //faB: now part of level loading since in future each level may have
  //     its own anim texture sequences, switches etc.
  P_InitPicAnims();
  P_InitLava();
  SetupSky();

  // SoM: WOO HOO!
  // SoM: DOH!
  //R_InitPortals ();

  // note: most of this ordering is important
  LoadBlockMap (lumpnum+ML_BLOCKMAP);
  LoadVertexes (lumpnum+ML_VERTEXES);
  LoadSectors  (lumpnum+ML_SECTORS);
  LoadSideDefs (lumpnum+ML_SIDEDEFS);

  LoadLineDefs (lumpnum+ML_LINEDEFS);
  LoadSideDefs2(lumpnum+ML_SIDEDEFS);
  LoadLineDefs2();
  LoadSubsectors (lumpnum+ML_SSECTORS);
  LoadNodes (lumpnum+ML_NODES);
  LoadSegs (lumpnum+ML_SEGS);
  rejectmatrix = (byte *)fc.CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
  GroupLines();

  // fix renderer to this map
  R.SetMap(this);

#ifdef HWRENDER // not win32 only 19990829 by Kin
  if (rendermode != render_soft)
    {
      // BP: reset light between levels (we draw preview frame lights on current frame)
      HWR_ResetLights();
      // Correct missing sidedefs & deep water trick
      R.HWR_CorrectSWTricks();
      R.HWR_CreatePlanePolygons (numnodes-1);
    }
#endif

  InitAmbientSound();
  LoadThings (lumpnum+ML_THINGS);
  PlaceWeapons();

  // set up world state
  SpawnSpecials();

  // build subsector connect matrix
  //  UNUSED P_ConnectSubsectors ();

  // preload graphics
#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR_PrepLevelCache (numtextures);
      R.HWR_CreateStaticLightmaps(numnodes-1);
    }
#endif

  if (precache)
    PrecacheMap();

  //CONS_Printf("%d vertexs %d segs %d subsector\n",numvertexes,numsegs,numsubsectors);
  return true;
}
