// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.62  2005/07/31 14:50:24  smite-meister
// thing spawning fix
//
// Revision 1.61  2005/07/20 20:27:21  smite-meister
// adv. texture cache
//
// Revision 1.60  2005/07/01 16:45:12  smite-meister
// FS cameras work
//
// Revision 1.59  2005/06/23 17:25:37  smite-meister
// map conversion command added
//
// Revision 1.58  2005/06/16 18:18:10  smite-meister
// bugfixes
//
// Revision 1.57  2005/06/08 17:29:38  smite-meister
// FS bugfixes
//
// Revision 1.56  2005/06/05 19:32:25  smite-meister
// unsigned map structures
//
// Revision 1.54  2005/01/04 18:32:41  smite-meister
// better colormap handling
//
// Revision 1.52  2004/11/28 18:02:21  smite-meister
// RPCs finally work!
//
// Revision 1.50  2004/11/18 20:30:11  smite-meister
// tnt, plutonia
//
// Revision 1.49  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.48  2004/10/14 19:35:30  smite-meister
// automap, bbox_t
//
// Revision 1.47  2004/09/24 21:19:59  jussip
// Joystick axis unbinding.
//
// Revision 1.46  2004/09/23 23:21:17  smite-meister
// HUD updated
//
// Revision 1.45  2004/09/03 16:28:50  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.42  2004/08/15 18:08:28  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.40  2004/07/25 20:19:21  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.39  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.38  2004/05/01 23:29:19  hurdler
// add dummy new renderer
//
// Revision 1.37  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.34  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.31  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.28  2003/12/03 10:49:50  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.25  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.24  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Map loading and setup
///
/// Loads the map lumps from the WAD, sets up the runtime structures,
/// spawns static Thinkers and the initial Actors.

#include <math.h>

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "console.h"
#include "cvars.h"

#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_level.h"

#include "p_setup.h"
#include "p_spec.h"
#include "p_camera.h"
#include "m_bbox.h"
#include "m_swap.h"
#include "m_misc.h"

#include "i_sound.h" //for I_PlayCD()..
#include "r_sky.h"

#include "r_render.h"
#include "r_data.h"
#include "r_main.h"
#include "r_sky.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"

#include "t_parse.h"

#ifdef HWRENDER
#include "i_video.h"            //rendermode
#include "hardware/hwr_render.h"
#endif


extern vector<mapthing_t *> polyspawn; // for spawning polyobjects


void Map::LoadVertexes(int lump)
{
  // Determine number of lumps: total lump length / vertex record length.
  numvertexes = fc.LumpLength(lump) / sizeof(mapvertex_t);
  //CONS_Printf("vertices: %d, ", numvertexes);

  // Allocate zone memory for buffer.
  vertexes = (vertex_t*)Z_Malloc(numvertexes*sizeof(vertex_t), PU_LEVEL, 0);

  // Load data into cache.
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapvertex_t *mv = (mapvertex_t *)data;
  vertex_t *v = vertexes;
  root_bbox.Clear(); // we also build the root bounding box here

  // Copy and convert vertex coordinates,
  // internal representation as fixed.
  for (int i=0 ; i<numvertexes ; i++, v++, mv++)
    {
      v->x = SHORT(mv->x)<<FRACBITS;
      v->y = SHORT(mv->y)<<FRACBITS;
      root_bbox.Add(v->x, v->y);
    }

  // Free buffer memory.
  Z_Free(data);
}


//
// Computes the line length in frac units, the glide render needs this
//
float P_SegLength(seg_t *seg)
{
  // make a vector (start at origin)
  double dx = (seg->v2->x - seg->v1->x)*fixedtofloat;
  double dy = (seg->v2->y - seg->v1->y)*fixedtofloat;

  return sqrt(dx*dx+dy*dy)*FRACUNIT;
}


void Map::LoadSegs(int lump)
{
  numsegs = fc.LumpLength(lump) / sizeof(mapseg_t);
  segs = (seg_t *)Z_Malloc(numsegs*sizeof(seg_t), PU_LEVEL, NULL);
  memset(segs, 0, numsegs*sizeof(seg_t));
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapseg_t *ms = (mapseg_t *)data;
  seg_t    *seg = segs;
  for (int i = 0; i < numsegs; i++, seg++, ms++)
    {
      seg->v1 = &vertexes[SHORT(ms->v1)];
      seg->v2 = &vertexes[SHORT(ms->v2)];

#if 0 //FIXME: Hurdler: put it back when new renderer is OK
#ifdef HWRENDER
      // used for the hardware render
      if (rendermode != render_soft)
        {
          seg->length = P_SegLength(seg);
          //Hurdler: 04/12/2000: for now, only used in hardware mode
          seg->lightmaps = NULL; // list of static lightmap for this seg
        }
#endif
#endif

      seg->angle = (SHORT(ms->angle))<<16;
      seg->offset = (SHORT(ms->offset))<<16;
      line_t *ldef = &lines[SHORT(ms->linedef)];
      seg->linedef = ldef;
      seg->side = SHORT(ms->side);
      seg->sidedef = ldef->sideptr[seg->side];

      seg->frontsector = ldef->sideptr[seg->side]->sector;
      if (ldef->flags & ML_TWOSIDED)
        seg->backsector = ldef->sideptr[seg->side^1]->sector;
      else
        seg->backsector = NULL;

      seg->numlights = 0;
      seg->rlights = NULL;
    }

  Z_Free(data);
}



void Map::LoadSubsectors(int lump)
{
  numsubsectors = fc.LumpLength(lump) / sizeof(mapsubsector_t);
  subsectors = (subsector_t *)Z_Malloc(numsubsectors*sizeof(subsector_t), PU_LEVEL, 0);
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapsubsector_t *mss = (mapsubsector_t *)data;
  memset(subsectors, 0, numsubsectors*sizeof(subsector_t));
  subsector_t *ss = subsectors;

  for (int i=0; i<numsubsectors; i++, ss++, mss++)
    {
      ss->numlines = SHORT(mss->numsegs);
      ss->firstline = SHORT(mss->firstseg);
    }

  Z_Free(data);
}



void Map::LoadSectors1(int lump)
{
  // allocate and zero the sectors
  numsectors = fc.LumpLength(lump) / sizeof(mapsector_t);
  sectors = (sector_t *)Z_Malloc(numsectors*sizeof(sector_t), PU_LEVEL, 0);
  memset(sectors, 0, numsectors*sizeof(sector_t));
}



void Map::LoadSectors2(int lump)
{
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapsector_t *ms = (mapsector_t *)data;
  sector_t *ss = sectors;
  for (int i=0; i < numsectors; i++, ss++, ms++)
    {
      ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

      ss->floorpic = tc.GetID(ms->floorpic, TEX_flat);
      ss->ceilingpic = tc.GetID(ms->ceilingpic, TEX_flat);

      ss->lightlevel = SHORT(ms->lightlevel);

      ss->tag = SHORT(ms->tag);

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

      // set floortype-dependent attributes
      ss->SetFloorType(ms->floorpic);

      ss->gravity = 1.0f;
      ss->special = SHORT(ms->special);
    }

  Z_Free(data);

  // whoa! there is usually no more than 25 different flats used per level!!
  //CONS_Printf("%d flats found\n", numlevelflats);
}



void Map::LoadNodes(int lump)
{
  numnodes = fc.LumpLength(lump) / sizeof(mapnode_t);
  nodes = (node_t *)Z_Malloc(numnodes*sizeof(node_t),PU_LEVEL,0);
  byte *data = (byte*)fc.CacheLumpNum(lump,PU_STATIC);

  mapnode_t *mn = (mapnode_t *)data;
  node_t *no = nodes;

  for (int i=0 ; i<numnodes ; i++, no++, mn++)
    {
      no->x = SHORT(mn->x)<<FRACBITS;
      no->y = SHORT(mn->y)<<FRACBITS;
      no->dx = SHORT(mn->dx)<<FRACBITS;
      no->dy = SHORT(mn->dy)<<FRACBITS;
      for (int j=0 ; j<2 ; j++)
        {
          no->children[j] = SHORT(mn->children[j]);
          for (int k=0 ; k<4 ; k++)
            no->bbox[j].box[k] = SHORT(mn->bbox[j][k]) << FRACBITS;
        }
    }

  Z_Free(data);
}



void Map::LoadThings(int lump)
{
  TIDmap.clear();

  if (hexen_format)
    nummapthings = fc.LumpLength(lump)/sizeof(hex_mapthing_t);
  else
    nummapthings = fc.LumpLength(lump)/sizeof(doom_mapthing_t);

  mapthings    = (mapthing_t *)Z_Malloc(nummapthings*sizeof(mapthing_t), PU_LEVEL, NULL);
  // this memset is crucial, it initializes the mapthings to all zeroes.
  memset(mapthings, 0, nummapthings*sizeof(mapthing_t));

  NumPolyobjs  = 0;

  char *data   = (char *)fc.CacheLumpNum(lump, PU_STATIC);

  doom_mapthing_t *mt = (doom_mapthing_t *)data;
  hex_mapthing_t *ht = (hex_mapthing_t *)data;

  mapthing_t *t = mapthings;

  // flag checks
  int ffail  = 0;
  int fskill;
  int fmode  = -1; // all bits on
  int fclass = -1;

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

      // If there are _any_ clerics in the game, MTF_CLERIC stuff should be spawned etc.
      //fclass = (MTF_FIGHTER | MTF_CLERIC | MTF_MAGE);
      fclass = 0;
      for (GameInfo::player_iter_t k = game.Players.begin(); k != game.Players.end(); k++)
        switch (k->second->options.pclass)
          {
          case PCLASS_FIGHTER:
            fclass |= MTF_FIGHTER;
            break;
          case PCLASS_CLERIC:
            fclass |= MTF_CLERIC;
            break;
          case PCLASS_MAGE:
            fclass |= MTF_MAGE;
            break;
          default:
            fclass |= (MTF_FIGHTER | MTF_CLERIC | MTF_MAGE);
            break;
          }
    }
  else
    {
      // Doom / Boom flags
      // multiplayer only thing flag
      // "not deathmatch"/"not coop" thing flags
      if (!game.multiplayer)
        ffail |= MTF_MULTIPLAYER;
      else if (cv_deathmatch.value)
        ffail |= MTF_NOT_IN_DM;
      else
        ffail |= MTF_NOT_IN_COOP;
    }


  int i, n, ednum;
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
          t->x = SHORT(mt->x);
          t->y = SHORT(mt->y);
          t->angle = SHORT(mt->angle);
          ednum    = SHORT(mt->type);
          t->flags = SHORT(mt->flags);
          mt++;
        }


      // convert editor number to mobjtype_t number right now
      if (!ednum)
        continue; // Ignore type-0 things as NOPs

      //======== first we spawn common THING types not affected by spawning flags ========

      // deathmatch start positions
      if (ednum == 11)
        {
          if (dmstarts.size() < MAX_DM_STARTS)
            dmstarts.push_back(t);
          continue;
        }

      // normal playerstarts (normal 4 + 28 extra)
      if ((ednum >= 1 && ednum <= 4) || (ednum >= 4001 && ednum <= 4028))
        {
          if (ednum > 4000)
            ednum -= 4001 - 5;

          playerstarts.insert(pair<int, mapthing_t *>(ednum, t));
          // t->type is used as a timer
          continue;
        }

      if (ednum == 14)
        {
          // a bit of a hack
          // same with doom / heretic / hexen, but only one mobjtype_t
          t->type = MT_TELEPORTMAN;
          continue;
        }

      // common polyobjects
#define PO_BASE 9300-3000 // currently ZDoom compatible
      if (ednum == PO_BASE+PO_ANCHOR_TYPE || ednum == PO_BASE+PO_SPAWN_TYPE || ednum == PO_BASE+PO_SPAWNCRUSH_TYPE)
	{
	  t->type = ednum - PO_BASE; // internally we use Hexen numbers
	  polyspawn.push_back(t);
	  if (ednum != PO_BASE+PO_ANCHOR_TYPE)
	    NumPolyobjs++; // a polyobj spawn spot
	  continue;
	}

      // cameras etc.
      for (n = MT_LEGACY_S; n <= MT_LEGACY_S_END; n++)
        if (ednum == mobjinfo[n].doomednum)
	  {
	    t->type = mobjtype_t(n);
	    break;
	  }

      if (t->type)
	continue; // was found

      //======== then game-specific things not affected by spawning flags ========

      int orig_ednum = ednum;
      int low = 0;
      int high = 0;

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
              AmbientSeqs.push_back(ednum - 1200);
              continue;
            }

          // D'Sparil teleport spot (no Actor spawned)
          if (ednum == 56)
            {
              BossSpots.push_back(t);
              continue;
            }

          // Mace spot (no Actor spawned)
          if (ednum == 2002)
            {
              MaceSpots.push_back(t);
              continue;
            }
        }
      else if (ednum >= info->hexen_offs[0] && ednum <= info->hexen_offs[1])
        {
          ednum -= info->hexen_offs[0];
          low = MT_HEXEN;
          high = MT_HEXEN_END;

          // The polyobject system is pretty stupid, since the mapthings are not always in
          // any particular order. Polyobjects have to be picked apart
          // from other things  => polyspawn vector
          if (ednum == PO_ANCHOR_TYPE || ednum == PO_SPAWN_TYPE || ednum == PO_SPAWNCRUSH_TYPE)
            {
              t->type = ednum;
              polyspawn.push_back(t);
              if (ednum != PO_ANCHOR_TYPE)
                NumPolyobjs++; // a polyobj spawn spot
              continue;
            }

          // Check for player starts 5 to 8
          if (ednum >= 9100 && ednum <= 9103)
            {
              ednum = 5 + ednum - 9100;
              playerstarts.insert(pair<int, mapthing_t *>(ednum, t));
              continue;
            }

          // sector sound sequences
          if (ednum >= 1400 && ednum < 1410)
            {
              R_PointInSubsector(t->x << FRACBITS, t->y << FRACBITS)->sector->seqType = ednum - 1400;
              continue;
            }
        }

      // Spawning flags don't apply to playerstarts, teleport exits or polyobjs! Why, pray, is that?
      // wrong flags?
      if ((t->flags & ffail) || !(t->flags & fskill) || !(t->flags & fmode) || !(t->flags & fclass))
	continue;

      //======== now common things affected by spawning flags ========

      for (n = MT_LEGACY; n <= MT_LEGACY_END; n++)
        if (orig_ednum == mobjinfo[n].doomednum)
	  {
	    t->type = mobjtype_t(n);
	    break;
	  }

      if (t->type)
	continue; // was found

      //======== finally game-specific things affected by spawning flags ========

      for (n = low; n <= high; n++)
        if (ednum == mobjinfo[n].doomednum)
	  {
	    t->type = mobjtype_t(n);
	    break;
	  }

      if (!t->type)
	CONS_Printf("\2Map::LoadThings: Unknown type %d at (%d, %d)\n", orig_ednum, t->x, t->y);
    }

  Z_Free(data);
}


void Map::LoadLineDefs(int lump)
{
  int i, j;

  vertex_t *v1, *v2;

  if (hexen_format)
    numlines = fc.LumpLength(lump)/sizeof(hex_maplinedef_t);
  else
    numlines = fc.LumpLength(lump)/sizeof(doom_maplinedef_t);

  //CONS_Printf("lines: %d\n", numlines);

  lines = (line_t *)Z_Malloc(numlines*sizeof(line_t), PU_LEVEL, 0);
  memset(lines, 0, numlines*sizeof(line_t));
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  doom_maplinedef_t *mld = (doom_maplinedef_t *)data;
  hex_maplinedef_t *hld = (hex_maplinedef_t *)data;

  line_t *ld = lines;

  for (i=0 ; i<numlines ; i++, ld++)
    {
      int temp[2];
      if (hexen_format)
        {
          ld->flags = SHORT(hld->flags);
          ld->special = hld->special;
          //ld->tag = hld->args[0]; // 16-bit tags

          for (j=0; j<5; j++)
            ld->args[j] = hld->args[j];

          v1 = ld->v1 = &vertexes[SHORT(hld->v1)];
          v2 = ld->v2 = &vertexes[SHORT(hld->v2)];

	  /*
          if (SHORT(hld->v1) > numvertexes)
            CONS_Printf("v1 > numverts: %d\n", SHORT(hld->v1));
          if (SHORT(hld->v2) > numvertexes)
            CONS_Printf("v2 > numverts: %d\n", SHORT(hld->v2));
	  */

	  temp[0] = SHORT(hld->sidenum[0]);
	  temp[1] = SHORT(hld->sidenum[1]);
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

	  temp[0] = SHORT(mld->sidenum[0]);
	  temp[1] = SHORT(mld->sidenum[1]);
          mld++;
        }

      // index -1 means no sidedef
      ld->sideptr[0] = (temp[0] == NULL_INDEX) ? NULL : &sides[temp[0]];
      ld->sideptr[1] = (temp[1] == NULL_INDEX) ? NULL : &sides[temp[1]];

      ld->dx = v2->x - v1->x;
      ld->dy = v2->y - v1->y;

      if (!ld->dx)
        ld->slopetype = ST_VERTICAL;
      else if (!ld->dy)
        ld->slopetype = ST_HORIZONTAL;
      else
        {
          if (FixedDiv(ld->dy , ld->dx) > 0)
            ld->slopetype = ST_POSITIVE;
          else
            ld->slopetype = ST_NEGATIVE;
        }

      //ld->bbox.Add(v1->x, v1->y);
      //ld->bbox.Add(v2->x, v2->y);

      if (v1->x < v2->x)
        {
          ld->bbox.box[BOXLEFT] = v1->x;
          ld->bbox.box[BOXRIGHT] = v2->x;
        }
      else
        {
          ld->bbox.box[BOXLEFT] = v2->x;
          ld->bbox.box[BOXRIGHT] = v1->x;
        }

      if (v1->y < v2->y)
        {
          ld->bbox.box[BOXBOTTOM] = v1->y;
          ld->bbox.box[BOXTOP] = v2->y;
        }
      else
        {
          ld->bbox.box[BOXBOTTOM] = v2->y;
          ld->bbox.box[BOXTOP] = v1->y;
        }

      // set line references to the sidedefs
      if (ld->sideptr[0])
        ld->sideptr[0]->line = ld;
      if (ld->sideptr[1])
        ld->sideptr[1]->line = ld;

      ld->transmap = -1; // no transmap by default
    }

  Z_Free(data);
}


void Map::LoadLineDefs2()
{
  line_t* ld = lines;
  for (int i=0; i < numlines; i++, ld++)
    {
      if (ld->sideptr[0])
        ld->frontsector = ld->sideptr[0]->sector;
      else
        ld->frontsector = NULL;

      if (ld->sideptr[1])
        ld->backsector = ld->sideptr[1]->sector;
      else
        ld->backsector = NULL;
    }
}


/*
void P_LoadSideDefs(int lump)
  {
  byte*               data;
  int                 i;
  mapsidedef_t*       msd;
  side_t*             sd;

  numsides = fc.LumpLength(lump) / sizeof(mapsidedef_t);
  sides = Z_Malloc(numsides*sizeof(side_t),PU_LEVEL,0);
  memset (sides, 0, numsides*sizeof(side_t));
  data = fc.CacheLumpNum(lump,PU_STATIC);

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

  Z_Free(data);
  }
*/


void Map::LoadSideDefs(int lump)
{
  numsides = fc.LumpLength(lump) / sizeof(mapsidedef_t);
  sides = (side_t *)Z_Malloc(numsides*sizeof(side_t), PU_LEVEL, 0);
  memset(sides, 0, numsides*sizeof(side_t));
}


void Map::LoadSideDefs2(int lump)
{
  byte *data = (byte *)fc.CacheLumpNum(lump,PU_STATIC);

  for (int i=0; i<numsides; i++)
    {
      mapsidedef_t *msd = (mapsidedef_t *)data + i;
      side_t *sd = sides + i;
      sector_t *sec;

      sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
      sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

      // shorthand
      char *ttex = msd->toptexture;
      char *mtex = msd->midtexture;
      char *btex = msd->bottomtexture;

      // refined to allow colormaps to work as wall
      // textures if invalid as colormaps but valid as textures.

      sd->sector = sec = &sectors[SHORT(msd->sector)];

      // specials where texture names might be something else
      line_t *ld = sd->line;
      if (ld && ld->special == LEGACY_EXT) // TODO shouldn't all sidedefs have a corresponding linedef?
	{
	  if (ld->args[0] == LEGACY_BOOM_EXOTIC)
	    switch (ld->args[1])
	      {
	      case 1:  // BOOM: 242 fake ceiling/floor, variable colormaps
	      case 3:  // Legacy: swimmable water, colormaps
		sd->toptexture = tc.GetTextureOrColormap(ttex, sec->topmap);
		sd->midtexture = tc.GetTextureOrColormap(mtex, sec->midmap);
		sd->bottomtexture = tc.GetTextureOrColormap(btex, sec->bottommap);
		break;

	      case 2: // BOOM: 260 make middle texture translucent
		sd->toptexture = tc.GetID(ttex);
		sd->midtexture = tc.GetTextureOrColormap(mtex, ld->transmap, true); // can also be a transmap lumpname
		sd->bottomtexture = tc.GetID(btex);
		break;

	      case 4: // Legacy: create a colormap
		if (ttex[0] == '#' || btex[0] == '#')
		  {
		    sec->midmap = R_CreateColormap(ttex, mtex, btex);
		    sd->toptexture = sd->bottomtexture = 0;
		    
		    if (rendermode != render_soft) // FIXME is this necessary?
		      sec->extra_colormap = &extra_colormaps[sec->midmap];
		  }
		else
		  {
		    sd->toptexture = tc.GetID(ttex);
		    sd->midtexture = tc.GetID(mtex);
		    sd->bottomtexture = tc.GetID(btex);
		  }
		break;

	      default:
		I_Error("Unknown linedef type extension (%d)\n.", ld->args[1]);
	      }
	  else if (ld->args[0] == LEGACY_FAKEFLOOR)
	    switch (ld->args[1])
	      {
		// Legacy: alpha value for translucent 3D-floors/water, TODO not good
	      case 19:
	      case 20:
		if (ttex[0] == '#')
		  sd->toptexture = sd->bottomtexture = ((ttex[1] - '0')*100 + (ttex[2] - '0')*10 + ttex[3] - '0') + 1;
		else
		  sd->toptexture = sd->bottomtexture = 0;
		sd->midtexture = tc.GetID(mtex);
		break;

	      default:
		goto normal;
	      }
	  else
	    goto normal;
	}
      else
	{
	normal:
	  // normal texture names
          sd->toptexture = tc.GetID(ttex);
          sd->midtexture = tc.GetID(mtex);
          sd->bottomtexture = tc.GetID(btex);
        }
    }

  Z_Free(data);
}


void Map::LoadBlockMap(int lump)
{
  blockmaplump = (Uint16 *)fc.CacheLumpNum(lump, PU_LEVEL);
  blockmap = blockmaplump+4; // the offsets array

  // Endianness: everything in blockmap is expressed in 2-byte shorts
  int size = fc.LumpLength(lump)/2;
  for (int i=0; i < size; i++)
    blockmaplump[i] = SHORT(blockmaplump[i]);

  // read the header
  blockmapheader_t *bm = (blockmapheader_t *)blockmaplump;
  bmaporgx = bm->origin_x << FRACBITS;
  bmaporgy = bm->origin_y << FRACBITS;
  bmapwidth = bm->width;
  bmapheight = bm->height;

  // check the blockmap for errors
  int errors = 0;
  int first = 4 + bmapwidth*bmapheight; // first possible blocklist offset (in shorts)

  if (size < first+2) // one empty blocklist (two shorts) is the minimal size
    {
      CONS_Printf(" Blockmap lump is too small!\n");
      errors++;
    }

  if (size > 0x10000+1) // largest, always fully addressable blockmap
    CONS_Printf(" Warning: Blockmap may be too large.\n");

  int count = bmapwidth*bmapheight;
  for (int i=0; i < count && errors < 50; i++)
    if (blockmap[i] < first)
      {
	CONS_Printf(" Invalid blocklist offset for cell %d: %d\n", i, blockmap[i]);
	errors++;
      }
    else
      {
	int offs = blockmap[i];
	if (blockmaplump[offs] != 0x0000)
	  {
	    CONS_Printf(" Blocklist %d does not start with zero!\n", i);
	    errors++;
	  }

	while (offs < size && blockmaplump[offs] != 0xFFFF)
	  offs++;

	if (offs >= size)
	  {
	    CONS_Printf(" Blocklist %d extends beyond the lump size!\n", i);
	    errors++;
	  }
      }

  if (errors)
    I_Error("Blockmap (%dx%d cells, %d bytes) had some errors.\n", bmapwidth, bmapheight, 2*size);

  // init the mobj chains
  count = bmapwidth*bmapheight*sizeof(Actor *);
  blocklinks = (Actor **)Z_Malloc(count, PU_LEVEL, 0);
  memset(blocklinks, 0, count);

  // init the polyblockmap
  count = bmapwidth*bmapheight*sizeof(polyblock_t *);
  PolyBlockMap = (polyblock_t **)Z_Malloc(count, PU_LEVEL, 0);
  memset(PolyBlockMap, 0, count);
}


//
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void Map::GroupLines()
{
  int i, j;

  // look up sector number for each subsector
  subsector_t *ss = subsectors;
  for (i=0 ; i<numsubsectors ; i++, ss++)
    {
      seg_t *seg = &segs[ss->firstline];
      ss->sector = seg->sidedef->sector;
    }

  // count number of lines in each sector
  line_t *li = lines;
  int total = 0;
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
  line_t **lb = linebuffer = (line_t **)Z_Malloc(total*sizeof(line_t *), PU_LEVEL, NULL);
  sector_t *sector = sectors;
  for (i=0 ; i<numsectors ; i++, sector++)
    {
      bbox_t bb; // temporary bounding box
      bb.Clear();

      sector->lines = lb;
      li = lines;
      for (j=0 ; j<numlines ; j++, li++)
        {
          if (li->frontsector == sector || li->backsector == sector)
            {
              *lb++ = li;
              bb.Add(li->v1->x, li->v1->y);
              bb.Add(li->v2->x, li->v2->y);
            }
        }
      if (lb - sector->lines != sector->linecount)
        I_Error("Map::GroupLines: miscounted");

      // set the degenmobj_t to the middle of the bounding box
      sector->soundorg.x = (bb[BOXRIGHT]+bb[BOXLEFT])/2;
      sector->soundorg.y = (bb[BOXTOP]+bb[BOXBOTTOM])/2;
      sector->soundorg.z = sector->floorheight-10;

      // adjust bounding box to map blocks
      int block = (bb[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block >= bmapheight ? bmapheight-1 : block;
      sector->blockbox[BOXTOP]=block;

      block = (bb[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXBOTTOM]=block;

      block = (bb[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block >= bmapwidth ? bmapwidth-1 : block;
      sector->blockbox[BOXRIGHT]=block;

      block = (bb[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXLEFT]=block;
    }

}

/*
TODO
static char *levellumps[] =
{
  "label",        // LUMP_LABEL,    A separator, name, ExMx or MAPxx
  "THINGS",       // LUMP_THINGS,   Monsters, items..
  "LINEDEFS",     // LUMP_LINEDEFS, LineDefs, from editing
  "SIDEDEFS",     // LUMP_SIDEDEFS, SideDefs, from editing
  "VERTEXES",     // LUMP_VERTEXES, Vertices, edited and BSP splits generated
  "SEGS",         // LUMP_SEGS,     LineSegs, from LineDefs split by BSP
  "SSECTORS",     // LUMP_SSECTORS, SubSectors, list of LineSegs
  "NODES",        // LUMP_NODES,    BSP nodes
  "SECTORS",      // LUMP_SECTORS,  Sectors, from editing
  "REJECT",       // LUMP_REJECT,   LUT, sector-sector visibility
  "BLOCKMAP",     // LUMP_BLOCKMAP  LUT, motion clipping, walls/grid element
  "BEHAVIOR"      // LUMP_BEHAVIOR, ACS scripts
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

  for(i=LUMP_THINGS; i<=LUMP_BLOCKMAP; i++)
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
    skytexture = tc.GetPtr(info->sky1.c_str());
  else
    skytexture = tc.GetPtr("SKY1");

  // scale up the old skies, if needed
  R_SetupSkyDraw();

  // set the sky flat num
  if (hexen_format)
    skyflatnum = tc.GetID("F_SKY", TEX_flat);
  else
    skyflatnum = tc.GetID("F_SKY1", TEX_flat);
}


//
// (re)loads the map from an already opened wad
//
bool Map::Setup(tic_t start, bool spawnthings)
{
  extern  bool precache;

  CONS_Printf("Map::Setup: %s\n", lumpname.c_str());
  con.Drawer();     // let the user know what we are going to do
  I_FinishUpdate(); // page flip or blit buffer

  maptic = 0;
  starttic = start;
  kills = items = secrets = 0;

  // internal game map
  lumpnum = fc.GetNumForName(lumpname.c_str());

  InitThinkers();

  FS_ClearScripts();
  // NOTE Hexen map separators are not empty!!! They contain a version string "version 2.3\0"
  levelscript->data = info->Read(lumpnum); // load map separator lump info (map properties, FS...)

  // is the map in Hexen format?
  const char *acslumpname = fc.FindNameForNum(lumpnum + LUMP_BEHAVIOR);
  if (acslumpname && !strncmp(acslumpname, "BEHAVIOR", 8))
    {
      CONS_Printf("Map in Hexen format!\n");
      hexen_format = true;
    }
  else
    {
      hexen_format = false;
    }

  // If the map defines its music in MapInfo_t, use it.
  if (!info->musiclump.empty())
    S.StartMusic(info->musiclump.c_str(), true);
  // I_PlayCD(info->mapnumber, true);  // FIXME cd music

  // Set the gravity for the level
  if (cv_gravity.value != int(info->gravity * FRACUNIT))
    {
      COM_BufAddText(va("gravity %f\n", info->gravity));
      COM_BufExecute();
    }

  SetupSky();

  // note: most of this ordering is important
  LoadBlockMap(lumpnum+LUMP_BLOCKMAP);
  LoadVertexes(lumpnum+LUMP_VERTEXES);
  LoadSectors1(lumpnum+LUMP_SECTORS);
  LoadSideDefs(lumpnum+LUMP_SIDEDEFS);
  LoadLineDefs(lumpnum+LUMP_LINEDEFS);

  if (!hexen_format)
    ConvertLineDefs();

  LoadSideDefs2(lumpnum+LUMP_SIDEDEFS); // also processes some linedef specials
  LoadLineDefs2();

  LoadSubsectors(lumpnum+LUMP_SSECTORS);
  LoadNodes(lumpnum+LUMP_NODES);
  LoadSegs(lumpnum+LUMP_SEGS);
  LoadSectors2(lumpnum+LUMP_SECTORS);
  rejectmatrix = (byte *)fc.CacheLumpNum(lumpnum+LUMP_REJECT,PU_LEVEL);
  GroupLines();

  for (int i=0; i<numsectors; i++)
    SpawnSectorSpecial(sectors[i].special, &sectors[i]);

  // fix renderer to this map
  R.SetMap(this);
  if (!info->fadetablelump.empty())
    R_SetFadetable(info->fadetablelump.c_str());
  else
    R_SetFadetable("COLORMAP");

#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR.Setup(numnodes);
    }
#endif

  LoadThings(lumpnum + LUMP_THINGS);

  if (!polyspawn.empty())
    InitPolyobjs(); // create the polyobjs, clear their mapthings (before spawning other things!)

  // spawn the THINGS (Actors) if needed
  if (spawnthings)
    {
      for (int i=0; i<nummapthings; i++)
        if (mapthings[i].type)
          SpawnMapThing(&mapthings[i]);

      PlaceWeapons(); // Heretic mace
    }

  SpawnLineSpecials(); // spawn Thinkers created by linedefs (also does some mandatory initializations!)

  if (hexen_format)
    LoadACScripts(lumpnum + LUMP_BEHAVIOR);

  FS_PreprocessScripts();        // preprocess FraggleScript scripts (needs already added players)

  InitLightning(); // Hexen lightning effect

  if (precache)
    PrecacheMap();

  //CONS_Printf("%d vertexs %d segs %d subsector\n",numvertexes,numsegs,numsubsectors);
  return true;
}


// TEMP linedef conversion tables
xtable_t *linedef_xtable = NULL;
int  linedef_xtable_size = 0;

/// Does the Doom => Hexen linedef type conversion for the given linedef.
void ConvertLineDef(line_t *ld)
{
  int j, trig;
  bool passuse = ld->flags & ML_PASSUSE;
  bool alltrigger = ld->flags & ML_ALLTRIGGER;
  ld->flags &= 0x1ff; // only basic Doom flags are kept

  if (ld->special < linedef_xtable_size)
    {
      // we use a pregenerated binary lookup table in legacy.wad
      xtable_t *p = &linedef_xtable[ld->special];

      if (p->type)
	ld->special = p->type; // some specials are unaffected

      for (j=0; j<5; j++)
	ld->args[j] = p->args[j];

      trig = p->trigger;

      // flags
      ld->flags |= (trig & 0x0f) << (ML_SPAC_SHIFT-1); // activation and repeat

      if (passuse && (GET_SPAC(ld->flags) == SPAC_USE))
	{
	  ld->flags &= ~ML_SPAC_MASK;
	  ld->flags |= SPAC_PASSUSE << ML_SPAC_SHIFT;
	}

      if (trig & T_ALLOWMONSTER)
	ld->flags |= ML_MONSTERS_CAN_ACTIVATE;
    }
  else if (ld->special >= GenCrusherBase)
    {
      // Boom generalized linedefs
      ld->flags |= ML_BOOM_GENERALIZED;
      if (passuse)
	ld->flags |= SPAC_PASSUSE << ML_SPAC_SHIFT;
    }

  // put flags back
  if (alltrigger)
    ld->flags |= ML_MONSTERS_CAN_ACTIVATE;

}


/// Converts all linedefs in the map using the conversion table.
void Map::ConvertLineDefs()
{
  for (int i=0; i<numlines; i++)
    ConvertLineDef(&lines[i]); // Doom => Hexen conversion
}



/// Converts the relevant parts of a Doom/Heretic map to Hexen format,
/// writes the lumps on disk.
bool ConvertMapToHexen(int lumpnum)
{
  const char *maplumpname = fc.FindNameForNum(lumpnum);

  if (!maplumpname)
    return false;

  CONS_Printf("Converting map %s to Hexen format...\n", maplumpname);

  // is it a map?
  const char *lumpname = fc.FindNameForNum(lumpnum+LUMP_THINGS);
  if (!lumpname || strncmp(lumpname, "THINGS", 8))
    {
      CONS_Printf(" No THINGS lump found!\n");
      return false;
    }

  lumpname = fc.FindNameForNum(lumpnum+LUMP_LINEDEFS);
  if (!lumpname || strncmp(lumpname, "LINEDEFS", 8))
    {
      CONS_Printf(" No LINEDEFS lump found!\n");
      return false;
    }

  // is the map in Hexen format?
  lumpname = fc.FindNameForNum(lumpnum+LUMP_BEHAVIOR);
  if (lumpname && !strncmp(lumpname, "BEHAVIOR", 8))
    {
      CONS_Printf(" Map already is in Hexen format!\n");
      return false;
    }

  // first convert THINGS

  int lump = lumpnum + LUMP_THINGS;
  int numthings = fc.LumpLength(lump)/sizeof(doom_mapthing_t);

  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);
  doom_mapthing_t *t = (doom_mapthing_t *)data;

  int length = numthings * sizeof(hex_mapthing_t);
  hex_mapthing_t *ht = (hex_mapthing_t *)Z_Malloc(length, PU_STATIC, NULL);
  memset(ht, 0, length); // no TID, height, special or args

  int bflags = MTF_FIGHTER | MTF_CLERIC | MTF_MAGE;

  for (int i=0; i < numthings; i++, t++)
    {
      ht[i].x = t->x;
      ht[i].y = t->y;
      ht[i].angle = t->angle;
      ht[i].type = t->type;

      int dflags = SHORT(t->flags); // since we use this data, it must be of correct endianness

      ht[i].flags = bflags | (dflags & 0xF); // lowest four flags are the same

      if (!(dflags & MTF_MULTIPLAYER))
	ht[i].flags |= MTF_GSINGLE;

      if (!(dflags & MTF_NOT_IN_COOP))
	ht[i].flags |= MTF_GCOOP;

      if (!(dflags & MTF_NOT_IN_DM))
	ht[i].flags |= MTF_GDEATHMATCH;

      ht[i].flags = SHORT(ht[i].flags); // and back to little-endian
    }

  Z_Free(data);

  string outfilename = string(maplumpname) + "_THINGS.lmp";
  FIL_WriteFile(outfilename.c_str(), ht, length);
  Z_Free(ht);

  // then convert LINEDEFS

  lump = lumpnum + LUMP_LINEDEFS;
  int numlines = fc.LumpLength(lump)/sizeof(doom_maplinedef_t);

  data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);
  doom_maplinedef_t *ld = (doom_maplinedef_t *)data;

  length = numlines * sizeof(hex_maplinedef_t);
  hex_maplinedef_t *hld = (hex_maplinedef_t *)Z_Malloc(length, PU_STATIC, NULL);

  for (int i=0; i < numlines; i++, ld++)
    {
      // trivialities
      hld[i].v1 = ld->v1;
      hld[i].v2 = ld->v2;
      hld[i].sidenum[0] = ld->sidenum[0];
      hld[i].sidenum[1] = ld->sidenum[1];

      // flags, special, args
      line_t temp;

      temp.flags = SHORT(ld->flags);
      temp.special = SHORT(ld->special);
      ConvertLineDef(&temp);

      hld[i].flags = SHORT(temp.flags);
      hld[i].special = temp.special;
      for (int j=0; j<5; j++)
	hld[i].args[j] = temp.args[j];

      // tag
      int tag = SHORT(ld->tag);

      if (hld[i].special == LEGACY_EXT) // different tag handling (full 16-bit tag stored)
	{
	  hld[i].args[3] = tag & 0xFF;
	  hld[i].args[4] = tag >> 8;
	}
      else
	{
	  // we must make do with 8-bit tags
	  if (tag > 255)
	    {
	      CONS_Printf(" Warning: Linedef %d: tag (%d) is above 255, set to zero.\n.", i, tag);
	      hld[i].args[0] = 0;
	    }
	  else
	    hld[i].args[0] = tag;
	}
    }

  Z_Free(data);

  outfilename = string(maplumpname) + "_LINEDEFS.lmp";
  FIL_WriteFile(outfilename.c_str(), hld, length);
  Z_Free(hld);

  CONS_Printf(" done. %d THINGS and %d LINEDEFS.\n", numthings, numlines);
  return true;
}
