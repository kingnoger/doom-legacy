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
// Revision 1.25  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.24  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.23  2003/06/10 22:39:57  smite-meister
// Bugfixes
//
// Revision 1.22  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.21  2003/06/01 18:56:30  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.20  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.19  2003/04/26 19:03:26  hurdler
// Being able to load an heretic map without sndseq (until it's officially put in legacy.wad)
//
// Revision 1.18  2003/04/26 12:01:13  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.17  2003/04/24 20:30:17  hurdler
// Remove lots of compiling warnings
//
// Revision 1.16  2003/04/20 17:02:01  smite-meister
// Damn
//
// Revision 1.15  2003/04/20 16:45:50  smite-meister
// partial SNDSEQ fix
//
// Revision 1.14  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.13  2003/04/14 08:58:27  smite-meister
// Hexen maps load.
//
// Revision 1.12  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.11  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.10  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.9  2003/03/08 16:07:08  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.8  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
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

#include "g_game.h"
#include "g_map.h"
#include "g_mapinfo.h"
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

#include "t_parse.h"
#include "t_func.h"

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

  int                 i;


  // Determine number of lumps:
  //  total lump length / vertex record length.
  numvertexes = fc.LumpLength(lump) / sizeof(mapvertex_t);
  CONS_Printf("vertices: %d, ", numvertexes);

  // Allocate zone memory for buffer.
  vertexes = (vertex_t*)Z_Malloc(numvertexes*sizeof(vertex_t), PU_LEVEL, 0);

  // Load data into cache.
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapvertex_t *mv = (mapvertex_t *)data;
  vertex_t *v = vertexes;

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (i=0 ; i<numvertexes ; i++, v++, mv++)
    {
      v->x = SHORT(mv->x)<<FRACBITS;
      v->y = SHORT(mv->y)<<FRACBITS;
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
  numsegs = fc.LumpLength(lump) / sizeof(mapseg_t);
  segs = (seg_t *)Z_Malloc(numsegs*sizeof(seg_t), PU_LEVEL, NULL);
  memset(segs, 0, numsegs*sizeof(seg_t));
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapseg_t *ml = (mapseg_t *)data;
  seg_t    *li = segs;
  for (int i = 0; i < numsegs; i++, li++, ml++)
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
      int linedef = SHORT(ml->linedef);
      line_t *ldef = &lines[linedef];
      li->linedef = ldef;
      int side = SHORT(ml->side);
      li->sidedef = &sides[ldef->sidenum[side]];
      li->frontsector = sides[ldef->sidenum[side]].sector;
      if (ldef-> flags & ML_TWOSIDED)
	li->backsector = sides[ldef->sidenum[side^1]].sector;
      else
	li->backsector = 0;

      li->numlights = 0;
      li->rlights = NULL;
    }

  Z_Free(data);
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
      levelflat->lumpnum = R_FlatNumForName (flatname);

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


void Map::LoadSectors1(int lump)
{
  // allocate and zero the sectors
  numsectors = fc.LumpLength(lump) / sizeof(mapsector_t);
  sectors = (sector_t *)Z_Malloc(numsectors*sizeof(sector_t), PU_LEVEL, 0);
  memset(sectors, 0, numsectors*sizeof(sector_t));
}

// was P_LoadSectors
void Map::LoadSectors2(int lump)
{
  int i;

  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  // FIXME here a real texture class should be used (with possible animation etc.)
  //Fab:FIXME: allocate for whatever number of flats
  //           512 different flats per level should be plenty
  levelflat_t *foundflats = (levelflat_t *)Z_Malloc(sizeof(levelflat_t) * MAXLEVELFLATS, PU_STATIC, 0);
  if (!foundflats)
    I_Error("P_LoadSectors: no mem\n");
  memset(foundflats, 0, sizeof(levelflat_t) * MAXLEVELFLATS);

  numlevelflats = 0;

  mapsector_t *ms = (mapsector_t *)data;
  sector_t *ss = sectors;
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

      // TODO cache flats here
      ss->floorpic = P_AddLevelFlat (ms->floorpic,foundflats);
      ss->ceilingpic = P_AddLevelFlat (ms->ceilingpic,foundflats);

      ss->lightlevel = SHORT(ms->lightlevel);

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

      // TEST
      ss->friction = normal_friction;
      ss->movefactor = 1.0f;
      ss->gravity = 1.0f;
      SpawnSectorSpecial(SHORT(ms->special), ss);
    }

  Z_Free (data);

  // whoa! there is usually no more than 25 different flats used per level!!
  //CONS_Printf ("%d flats found\n", numlevelflats);

  // set the sky flat num
  // FIXME we need a completely new flat/texture cache.
  if (game.mode == gm_hexen)
    skyflatnum = P_AddLevelFlat ("F_SKY",foundflats);
  else
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
  TIDmap.clear();

  if (hexen_format)
    nummapthings = fc.LumpLength(lump)/sizeof(hex_mapthing_t);
  else
    nummapthings = fc.LumpLength(lump)/sizeof(doom_mapthing_t);

  mapthings    = (mapthing_t *)Z_Malloc(nummapthings*sizeof(mapthing_t), PU_LEVEL, NULL);
  NumPolyobjs  = 0;

  char *data   = (char *)fc.CacheLumpNum(lump, PU_STATIC);

  doom_mapthing_t *mt = (doom_mapthing_t *)data;
  hex_mapthing_t *ht = (hex_mapthing_t *)data;

  mapthing_t *t = mapthings;

  int ffail = 0;
  int fskill = 0;
  int fmode = -1; // all bits on
  extern consvar_t cv_deathmatch;

  // check skill
  if (game.skill == sk_baby)
    fskill = 1;
  else if (game.skill == sk_nightmare)
    fskill = 4;
  else
    fskill = 1 << (game.skill-1);

  if (hexen_format)
    {
      // Hexen flags
      if (!game.multiplayer)
	fmode = MTF_GSINGLE;
      else if (cv_deathmatch.value)
	fmode = MTF_GDEATHMATCH;
      else
	fmode = MTF_GCOOP;
    }
  else
    {
      // Doom / Boom flags
      // multiplayer only thing flag
      if (!game.multiplayer)
	ffail |= MTF_MULTIPLAYER;

      // "not deathmatch"/"not coop" thing flags
      if (game.netgame && cv_deathmatch.value)
	ffail |= MTF_NOT_IN_DM;
      else if (game.netgame && !cv_deathmatch.value)
	ffail |= MTF_NOT_IN_COOP;
    }


  int i, n, low, high, ednum;
  for (i=0 ; i<nummapthings ; i++, t++)
    {
      if (hexen_format)
	{
	  t->tid = SHORT(ht->tid);
	  t->x = SHORT(ht->x);
	  t->y = SHORT(ht->y);
	  t->z = SHORT(ht->height); // temp
	  t->angle  = SHORT(ht->angle);
	  ednum     = SHORT(ht->type);
	  t->flags  = SHORT(ht->flags);

	  t->special = ht->special;
	  for (int j=0; j<5; j++)
	    t->args[j] = ht->args[j];
	  ht++;
	}
      else
	{
	  t->tid = 0;
	  t->x = SHORT(mt->x);
	  t->y = SHORT(mt->y);
	  t->z = 0;
	  t->angle = SHORT(mt->angle);
	  ednum    = SHORT(mt->type);
	  t->flags = SHORT(mt->flags);

	  t->special = 0;
	  for (int j=0; j<5; j++)
	    t->args[j] = 0;
	  mt++;
	}
      t->mobj = NULL;

      // convert editor number to mobjtype_t number right now
      if (!ednum)
	continue; // Ignore type-0 things as NOPs

      // deathmatch start positions
      if (ednum == 11)
	{
	  if (dmstarts.size() < MAX_DM_STARTS)
	    dmstarts.push_back(t);
	  t->type = 0;
	  continue;
	}

      // normal playerstarts (normal 4 + 28 extra)
      if ((ednum >= 1 && ednum <= 4) || (ednum >= 4001 && ednum <= 4028))
	{
	  if (ednum > 4000)
	    ednum -= 4001 - 5;

	  playerstarts.insert(pair<int, mapthing_t *>(ednum, t));
	  t->type = 0; // t->type is used as a timer
	  continue;
	}

      if (ednum == 14)
	{
	  // a bit of a hack
	  // same with doom / heretic / hexen, but only one mobjtype_t
	  t->type = MT_TELEPORTMAN;
	  continue;
	}

      low = 0;
      high = 0;

      // find which type to spawn
      // this is because the ednum ranges normally overlap in different games
      if (ednum >= info->doom_offs[0] && ednum <= info->doom_offs[1])
	{
	  ednum -= info->doom_offs[0];
	  low = MT_DOOM;
	  high = MT_DOOM_END;

	  // DoomII braintarget list
	  if (ednum == 87)
	    braintargets.push_back(t);
	}
      else if (ednum >= info->heretic_offs[0] && ednum <= info->heretic_offs[1])
	{
	  ednum -= info->heretic_offs[0];
	  low = MT_HERETIC;
	  high = MT_HERETIC_END;

	  // Ambient sound sequences
	  if (ednum >= 1200 && ednum < 1210)
	    {
	      if (SoundSeqs.find(ednum - 1200) != SoundSeqs.end())
	        AmbientSeqs.push_back(SoundSeqs[ednum - 1200]);
	      else
		CONS_Printf("WARNING: Ambient sequence %d not defined!\n", ednum - 1200);
	      t->type = 0;
	      continue;
	    }

	  // D'Sparil teleport spot (no Actor spawned)
	  if (ednum == 56)
	    {
	      BossSpots.push_back(t);
	      t->type = 0;
	      continue;
	    }

	  // Mace spot (no Actor spawned)
	  if (ednum == 2002)
	    {
	      MaceSpots.push_back(t);
	      t->type = 0;
	      continue;
	    }
	}
      else if (ednum >= info->hexen_offs[0] && ednum <= info->hexen_offs[1])
	{
	  ednum -= info->hexen_offs[0];
	  low = MT_HEXEN;
	  high = MT_HEXEN_END;

	  extern vector<mapthing_t *> polyspawn;
	  // The polyobject system is pretty stupid, since the mapthings are not always in
	  // any particular order. Polyobjects have to be picked apart
	  // from other things  => polyspawn vector
	  if (ednum == PO_ANCHOR_TYPE || ednum == PO_SPAWN_TYPE || ednum == PO_SPAWNCRUSH_TYPE)
	    {
	      t->type = ednum;
	      polyspawn.push_back(t);
	      if (ednum != PO_ANCHOR_TYPE)
		// a polyobj marker
		NumPolyobjs++;
	      continue;
	    }

	  // Check for player starts 5 to 8
	  if (ednum >= 9100 && ednum <= 9103)
	    {
	      ednum = 5 + ednum - 9100;
	      playerstarts.insert(pair<int, mapthing_t *>(ednum, t));
	      t->type = 0;
	      continue;
	    }

	  if(ednum >= 1400 && ednum < 1410)
	    {
	      // FIXME soundseqs
	      //R_PointInSubsector(t->x << FRACBITS, t->y << FRACBITS)->sector->seqType = ednum - 1400;
	      t->type = 0;
	      continue;
	    }
	}

      // Spawning flags don't apply to playerstarts, teleport exits or polyobjs! Why, pray, is that?
      // wrong flags?
      if ((t->flags & ffail) || !(t->flags & fskill) || !(t->flags & fmode))
	{
	  t->type = 0;
	  continue;
	}

      for (n = low; n <= high; n++)
	if (ednum == mobjinfo[n].doomednum)
	  break;

      if (n > high)
	{
	  CONS_Printf("\2Map::LoadThings: Unknown type %i at (%i, %i)\n", ednum, t->x, t->y);
	  t->type = 0;
	  continue;
	}

      t->type = mobjtype_t(n);
    }

  Z_Free(data);
}


//
// was P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void Map::LoadLineDefs(int lump)
{
  int i, j;

  vertex_t *v1, *v2;

  if (hexen_format)
    numlines = fc.LumpLength(lump)/sizeof(hex_maplinedef_t);
  else
    numlines = fc.LumpLength(lump)/sizeof(doom_maplinedef_t);

  CONS_Printf("lines: %d, ", numlines);

  lines = (line_t *)Z_Malloc(numlines*sizeof(line_t), PU_LEVEL, 0);
  memset(lines, 0, numlines*sizeof(line_t));
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  doom_maplinedef_t *mld = (doom_maplinedef_t *)data;
  hex_maplinedef_t *hld = (hex_maplinedef_t *)data;

  line_t *ld = lines;

  for (i=0 ; i<numlines ; i++, ld++)
    {
      if (hexen_format)
	{
	  ld->flags = SHORT(hld->flags);
	  ld->special = hld->special;
	  ld->tag = hld->args[0]; // 16-bit tags
	  
	  for (j=0; j<5; j++)
	    ld->args[j] = hld->args[j];

	  v1 = ld->v1 = &vertexes[SHORT(hld->v1)];
	  v2 = ld->v2 = &vertexes[SHORT(hld->v2)];

	  if (SHORT(hld->v1) > numvertexes)
	    CONS_Printf("v1 > numverts: %d\n", SHORT(hld->v1));
	  if (SHORT(hld->v2) > numvertexes)
	    CONS_Printf("v2 > numverts: %d\n", SHORT(hld->v2));

	  ld->sidenum[0] = SHORT(hld->sidenum[0]);
	  ld->sidenum[1] = SHORT(hld->sidenum[1]);
	  hld++;
	}
      else
	{
	  ld->flags = SHORT(mld->flags);
	  ld->special = SHORT(mld->special);
	  ld->tag = SHORT(mld->tag);
	  //for (j=0; j<5; j++) ld->args[j] = 0;

	  v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
	  v2 = ld->v2 = &vertexes[SHORT(mld->v2)];

	  ld->sidenum[0] = SHORT(mld->sidenum[0]);
	  ld->sidenum[1] = SHORT(mld->sidenum[1]);

	  if (ld->sidenum[0] != -1 && ld->special)
	    sides[ld->sidenum[0]].special = ld->special;
	  mld++;
	}

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

  PolyBlockMap = (polyblock_t **)Z_Malloc(bmapwidth*bmapheight*sizeof(polyblock_t *), PU_LEVEL, 0);
  memset(PolyBlockMap, 0, bmapwidth*bmapheight*sizeof(polyblock_t *));
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
  linebuffer = (line_t **)Z_Malloc(total*sizeof(line_t *), PU_LEVEL, NULL);
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

/*
TODO
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
  "BLOCKMAP",     // ML_BLOCKMAP  LUT, motion clipping, walls/grid element
  "BEHAVIOR" // ACS scripts
};
*/

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


// Setup sky texture to use for the level
//
// - in future, each level may use a different sky.
// - No, in future we can use a skybox!
// The sky texture to be used instead of the F_SKY1 dummy.
void Map::SetupSky()
{
  // where does the sky texture come from?
  // 1. MapInfo skyname
  // 2. MAPINFO lump skyname
  // 3. if everything else fails, use "SKY1" and hope for the best

  // original DOOM determined the sky texture to be used
  // depending on the current episode, and the game version.

  if (!info->sky1.empty())
    skytexture = R_TextureNumForName(info->sky1.c_str());
  else
    skytexture = R_TextureNumForName("SKY1");

  // scale up the old skies, if needed
  R_SetupSkyDraw();
}


extern int numtextures;


void P_Initsecnode();

//
// (re)loads the map from an already opened wad
//
bool Map::Setup(tic_t start, bool spawnthings)
{
  extern  bool precache;

  CONS_Printf("Map::Setup: %s\n", lumpname.c_str());

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

  InitThinkers();

  //
  //  load the map from internal game resource or external wad file
  //

  // internal game map
  lumpnum = fc.GetNumForName(lumpname.c_str());

  // textures are needed first
  //    R_LoadTextures ();
  //    R_FlushTextureCache();

  R_ClearColormaps();

#ifdef FRAGGLESCRIPT
  script_camera_on = false;
  T_ClearScripts();
#endif

  levelscript->data = info->Read(lumpnum); // load map separator lump info (map properties, FS...)

  // is the map in Hexen format?
  const char *acslumpname = fc.FindNameForNum(lumpnum + ML_BEHAVIOR);
  if (acslumpname && !strncmp(acslumpname, "BEHAVIOR", 8))
    {
      CONS_Printf("Map in Hexen format!\n");
      hexen_format = true;
    }
  else
    {
      hexen_format = false;
      // TODO cumulative SNDSEQ reading? First-to-last order.
      //S_Read_SNDSEQ(fc.FindNumForNamePwad("SNDSEQ", 0, 0)); // Doom and Heretic sequences in legacy.wad
    }

#ifdef FRAGGLESCRIPT
  T_PreprocessScripts();        // preprocess FraggleScript scripts (needs already added players)
  script_camera_on = false;
#endif

  // If the map defines its music in MapInfo_t, use it.
  if (!info->musiclump.empty())
    S.StartMusic(info->musiclump.c_str());

  //faB: now part of level loading since in future each level may have
  //     its own anim texture sequences, switches etc.
  P_InitPicAnims();
  SetupSky();

  // SoM: WOO HOO!
  // SoM: DOH!
  //R_InitPortals ();

  // note: most of this ordering is important
  LoadBlockMap (lumpnum+ML_BLOCKMAP);
  LoadVertexes (lumpnum+ML_VERTEXES);
  LoadSectors1 (lumpnum+ML_SECTORS);
  LoadSideDefs (lumpnum+ML_SIDEDEFS);
  LoadLineDefs (lumpnum+ML_LINEDEFS);
  LoadSideDefs2(lumpnum+ML_SIDEDEFS);
  LoadLineDefs2();

  if (!hexen_format)
    ConvertLineDefs(); // Doom => Hexen

  LoadSubsectors(lumpnum+ML_SSECTORS);
  LoadNodes(lumpnum+ML_NODES);
  LoadSegs (lumpnum+ML_SEGS);
  LoadSectors2(lumpnum+ML_SECTORS); // also spawns the sector special Thinkers
  rejectmatrix = (byte *)fc.CacheLumpNum(lumpnum+ML_REJECT,PU_LEVEL);

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
      //CONS_Printf("\n xxx seg(%d) v1 = %d, line(%d) v1 = %d\n", 1578, segs[1578].v1 - vertexes, segs[1578].linedef - lines, segs[1578].linedef->v1 - vertexes);
      R.HWR_CreatePlanePolygons(numnodes-1); // FIXME BUG this messes up the polyobjs
      //CONS_Printf(" xxx seg(%d) v1 = %d, line(%d) v1 = %d\n", 1578, segs[1578].v1 - vertexes, segs[1578].linedef - lines, segs[1578].linedef->v1 - vertexes);
    }
#endif

  LoadThings(lumpnum + ML_THINGS);

  if (hexen_format)
    InitPolyobjs(); // create the polyobjs, clear their mapthings

  // spawn the THINGS (Actors) if needed
  if (spawnthings)
    {
      for (int i=0; i<nummapthings; i++)
	if (mapthings[i].type)
	  SpawnMapThing(&mapthings[i]);

      PlaceWeapons(); // Heretic mace
    }

  if (hexen_format)
    LoadACScripts(lumpnum + ML_BEHAVIOR);

  SpawnSpecials(); // spawn Thinkers created by linedefs (also does some mandatory initializations!)

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


// does the Doom => Hexen linedef type conversion
void Map::ConvertLineDefs()
{
  // we use a pregenerated binary lookup table in legacy.wad

  int lump;
  if (game.mode == gm_heretic)
    lump = fc.GetNumForName("XHERETIC"); // TODO Heretic table...
  else
    lump = fc.GetNumForName("XDOOM");

  xtable_t *p, *xt = (xtable_t *)fc.CacheLumpNum(lump, PU_CACHE);
  int n = fc.LumpLength(lump) / sizeof(xtable_t);

  int i, j, trig;
  line_t *ld = lines;
  for (i=0; i<numlines; i++, ld++)
    {
      if (ld->special < n)
	{
	  bool passuse = ld->flags & ML_PASSUSE;
	  bool alltrigger = ld->flags & ML_ALLTRIGGER;
	  ld->flags &= 0x1ff; // only basic Doom flags are kept
	  
	  p = &xt[ld->special];
	  ld->special = p->type;
	  for (j=0; j<5; j++)
	    ld->args[j] = p->args[j];
	  trig = p->trigger;

	  if (ld->args[0] == 255)
	    //ld->args[0] = ld->tag; // nope, a byte is not enough
	    ld->args[0] = 0; // now ExecuteLineSpecial() uses 'tag'

	  // time to put the flags back
	  ld->flags |= (trig & 0x0f) << (ML_SPAC_SHIFT-1); // activation and repeat
	  
	  if (passuse && (GET_SPAC(ld->flags) == SPAC_USE))
	    {
	      ld->flags &= ~ML_SPAC_MASK;
	      ld->flags |= SPAC_PASSUSE << ML_SPAC_SHIFT;
	    }

	  if (trig & T_ALLOWMONSTER || alltrigger)
	    ld->flags |= ML_MONSTERS_CAN_ACTIVATE;
	}
    }
}
