// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
// Revision 1.53  2004/12/31 16:19:38  smite-meister
// alpha fixes
//
// Revision 1.52  2004/11/28 18:02:21  smite-meister
// RPCs finally work!
//
// Revision 1.51  2004/11/19 16:51:04  smite-meister
// cleanup
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
// Revision 1.41  2004/08/12 18:30:24  smite-meister
// cleaned startup
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

#include "g_game.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_level.h"

#include "p_setup.h"
#include "p_spec.h"
#include "m_bbox.h"
#include "m_swap.h"

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
  //const double crapmul = 1.0 / 65536.0;
  double dx, dy;

  // make a vector (start at origin)
  dx = (seg->v2->x - seg->v1->x)*fixedtofloat;
  dy = (seg->v2->y - seg->v1->y)*fixedtofloat;

  return sqrt(dx*dx+dy*dy)*FRACUNIT;
}


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

#if 0 //FIXME: Hurdler: put it back when new renderer is OK
#ifdef HWRENDER
      // used for the hardware render
      if (rendermode != render_soft)
        {
          li->length = P_SegLength (li);
          //Hurdler: 04/12/2000: for now, only used in hardware mode
          li->lightmaps = NULL; // list of static lightmap for this seg
        }
#endif
#endif

      li->angle = (SHORT(ml->angle))<<16;
      li->offset = (SHORT(ml->offset))<<16;
      int linedef = SHORT(ml->linedef);
      line_t *ldef = &lines[linedef];
      li->linedef = ldef;
      li->side = SHORT(ml->side);
      li->sidedef = &sides[ldef->sidenum[li->side]];
      li->frontsector = sides[ldef->sidenum[li->side]].sector;
      if (ldef-> flags & ML_TWOSIDED)
        li->backsector = sides[ldef->sidenum[li->side^1]].sector;
      else
        li->backsector = 0;

      li->numlights = 0;
      li->rlights = NULL;
    }

  Z_Free(data);
}



void Map::LoadSubsectors(int lump)
{
  numsubsectors = fc.LumpLength (lump) / sizeof(mapsubsector_t);
  subsectors = (subsector_t *)Z_Malloc(numsubsectors*sizeof(subsector_t), PU_LEVEL, 0);
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapsubsector_t *ms = (mapsubsector_t *)data;
  memset (subsectors,0, numsubsectors*sizeof(subsector_t));
  subsector_t *ss = subsectors;

  for (int i=0; i<numsubsectors; i++, ss++, ms++)
    {
      ss->numlines = SHORT(ms->numsegs);
      ss->firstline = SHORT(ms->firstseg);
    }

  Z_Free (data);
}



void Map::LoadSectors1(int lump)
{
  // allocate and zero the sectors
  numsectors = fc.LumpLength(lump) / sizeof(mapsector_t);
  sectors = (sector_t *)Z_Malloc(numsectors*sizeof(sector_t), PU_LEVEL, 0);
  memset(sectors, 0, numsectors*sizeof(sector_t));
}


floortype_t P_GetFloorType(const char *pic);

void Map::LoadSectors2(int lump)
{
  extern float normal_friction;
  int i;
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapsector_t *ms = (mapsector_t *)data;
  sector_t *ss = sectors;
  for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
      ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

      ss->floortype = P_GetFloorType(ms->floorpic);

      ss->floorpic = tc.Get(ms->floorpic);
      ss->ceilingpic = tc.Get(ms->ceilingpic);

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
      ss->special = SHORT(ms->special);
    }

  Z_Free (data);

  // whoa! there is usually no more than 25 different flats used per level!!
  //CONS_Printf ("%d flats found\n", numlevelflats);

  // set the sky flat num
  if (game.mode == gm_hexen)
    skyflatnum = tc.Get("F_SKY");
  else
    skyflatnum = tc.Get("F_SKY1");
}



void Map::LoadNodes(int lump)
{
  numnodes = fc.LumpLength (lump) / sizeof(mapnode_t);
  nodes = (node_t *)Z_Malloc(numnodes*sizeof(node_t),PU_LEVEL,0);
  byte *data = (byte*)fc.CacheLumpNum (lump,PU_STATIC);

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

      // If there are _any_ clerics in the game, MTF_CLERIC stuff should be spawned etc.
      fclass = (MTF_FIGHTER | MTF_CLERIC | MTF_MAGE); // 0;
      /*
      for (player_iter_t k = game.Players.begin(); k != game.Players.end(); k++)
        switch ((*k)->pclass)
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
      */
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
              AmbientSeqs.push_back(ednum - 1200);
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

          // sector sound sequences
          if (ednum >= 1400 && ednum < 1410)
            {
              R_PointInSubsector(t->x << FRACBITS, t->y << FRACBITS)->sector->seqType = ednum - 1400;
              t->type = 0;
              continue;
            }
        }

      // Spawning flags don't apply to playerstarts, teleport exits or polyobjs! Why, pray, is that?
      // wrong flags?
      if ((t->flags & ffail) || !(t->flags & fskill) || !(t->flags & fmode) || !(t->flags & fclass))
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
      if (hexen_format)
        {
          ld->flags = SHORT(hld->flags);
          ld->special = hld->special;
          //ld->tag = hld->args[0]; // 16-bit tags

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

      ld->transmap = -1; // no transmap by default
    }

  Z_Free (data);
}


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


/*
void P_LoadSideDefs (int lump)
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
  }
*/


void Map::LoadSideDefs(int lump)
{
  numsides = fc.LumpLength(lump) / sizeof(mapsidedef_t);
  sides = (side_t *)Z_Malloc(numsides*sizeof(side_t),PU_LEVEL,0);
  memset(sides, 0, numsides*sizeof(side_t));
}


void Map::LoadSideDefs2(int lump)
{
  byte *data = (byte *)fc.CacheLumpNum(lump,PU_STATIC);

  for (int i=0; i<numsides; i++)
    {
      mapsidedef_t *msd = (mapsidedef_t *) data + i;
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
      if (sd->special)
	switch (sd->special)
	  {
	  case 1:  // BOOM: 242 fake ceiling/floor, variable colormaps
	  case 3:  // Legacy: swimmable water, colormaps
	    sd->toptexture = tc.GetTextureOrColormap(ttex, sec->topmap);
	    sd->midtexture = tc.GetTextureOrColormap(mtex, sec->midmap);
	    sd->bottomtexture = tc.GetTextureOrColormap(btex, sec->bottommap);
	    break;

	  case 2: // BOOM: 260 make middle texture translucent
	    sd->toptexture = tc.Get(ttex);
	    sd->midtexture = tc.GetTextureOrColormap(mtex, sd->special, true); // can also be a transmap lumpname
	    sd->bottomtexture = tc.Get(btex);
	    break;

	  case 4: // Legacy: create a colormap
#ifdef HWRENDER
	    if (rendermode == render_soft)
	      {
#endif
		if(ttex[0] == '#' || btex[0] == '#')
		  {
		    sec->midmap = R_CreateColormap(ttex, mtex, btex);
		    sd->toptexture = sd->bottomtexture = 0;
		  }
		else
		  {
		    sd->toptexture = tc.Get(ttex);
		    sd->midtexture = tc.Get(mtex);
		    sd->bottomtexture = tc.Get(btex);
		  }
#ifdef HWRENDER
	      }
	    else
	      {
		//TODO: Hurdler: for now, full support of toptexture only
		if (ttex[0] == '#')// || btex[0] == '#')
		  {
		    char *col = ttex;

		    sec->midmap = R_CreateColormap(ttex, mtex, btex);
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
		    sd->toptexture = tc.Get(ttex);
		    sd->midtexture = tc.Get(mtex);
		    sd->bottomtexture = tc.Get(btex);
		  }
	      }
#endif
	    break;

	    //Hurdler: added for alpha value with translucent 3D-floors/water
	    // TODO FIXME is this correct??
	  case 300:
	  case 301:
	    if (ttex[0] == '#')
	      sd->toptexture = sd->bottomtexture = ((ttex[1] - '0')*100 + (ttex[2] - '0')*10 + ttex[3] - '0') + 1;
	    else
	      sd->toptexture = sd->bottomtexture = 0;
	    sd->midtexture = tc.Get(mtex);
	    break;
	  }
      else
	{
	  // normal linedefs
          sd->toptexture = tc.Get(ttex);
          sd->midtexture = tc.Get(mtex);
          sd->bottomtexture = tc.Get(btex);
        }
    }
  Z_Free (data);
}


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
    S.StartMusic(info->musiclump.c_str(),  true);
  // I_PlayCD(info->mapnumber, true);  // FIXME cd music


  //faB: now part of level loading since in future each level may have
  //     its own anim texture sequences, switches etc.
  SetupSky();

  // note: most of this ordering is important
  LoadBlockMap (lumpnum+LUMP_BLOCKMAP);
  LoadVertexes (lumpnum+LUMP_VERTEXES);
  LoadSectors1 (lumpnum+LUMP_SECTORS);
  LoadSideDefs (lumpnum+LUMP_SIDEDEFS);
  LoadLineDefs (lumpnum+LUMP_LINEDEFS);

  if (!hexen_format)
    ConvertLineDefs(); // Doom => Hexen conversion

  LoadSideDefs2(lumpnum+LUMP_SIDEDEFS); // also processes some linedef specials
  LoadLineDefs2();

  LoadSubsectors(lumpnum+LUMP_SSECTORS);
  LoadNodes(lumpnum+LUMP_NODES);
  LoadSegs (lumpnum+LUMP_SEGS);
  LoadSectors2(lumpnum+LUMP_SECTORS);
  rejectmatrix = (byte *)fc.CacheLumpNum(lumpnum+LUMP_REJECT,PU_LEVEL);
  GroupLines();

  for (int i=0; i<numsectors; i++)
    SpawnSectorSpecial(sectors[i].special, &sectors[i]);

  // fix renderer to this map
  R.SetMap(this);

#ifdef HWRENDER
  if (rendermode != render_soft)
    {
      HWR.Setup(numnodes);
    }
#endif

  LoadThings(lumpnum + LUMP_THINGS);

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



// does the Doom => Hexen linedef type conversion
void Map::ConvertLineDefs()
{
  // we use a pregenerated binary lookup table in legacy.wad

  int lump;
  if (game.mode == gm_heretic)
    lump = fc.GetNumForName("XHERETIC");
  else
    lump = fc.GetNumForName("XDOOM");

  xtable_t *p, *xt = (xtable_t *)fc.CacheLumpNum(lump, PU_CACHE);
  int n = fc.LumpLength(lump) / sizeof(xtable_t);

  int i, j, trig;
  line_t *ld = lines;
  for (i=0; i<numlines; i++, ld++)
    {
      bool passuse = ld->flags & ML_PASSUSE;
      bool alltrigger = ld->flags & ML_ALLTRIGGER;
      ld->flags &= 0x1ff; // only basic Doom flags are kept

      if (ld->special < n)
        {
          p = &xt[ld->special];

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

      // HACK some linedefs use texture names as data fields
      if (ld->special == LEGACY_EXT && ld->args[0] == 4 && ld->sidenum[0] != -1)
	sides[ld->sidenum[0]].special = ld->args[1];
    }
}
