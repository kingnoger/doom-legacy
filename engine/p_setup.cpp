// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Map loading and setup
///
/// Loads the map lumps from the WAD, sets up the runtime structures,
/// spawns static Thinkers and the initial Actors.

#include <math.h>
#include <string.h>

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
#include "tables.h"

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

#include"oglrenderer.hpp"

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
      v->x = SHORT(mv->x);
      v->y = SHORT(mv->y);
      root_bbox.Add(v->x, v->y);
    }

  // Free buffer memory.
  Z_Free(data);
}



void Map::LoadSegs(int lump)
{
  numsegs = fc.LumpLength(lump) / sizeof(mapseg_t);
  segs = (seg_t *)Z_Malloc(numsegs*sizeof(seg_t), PU_LEVEL, NULL);
  memset(segs, 0, numsegs*sizeof(seg_t)); // clear everything
  byte *data = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

  mapseg_t *ms = (mapseg_t *)data;
  seg_t    *seg = segs;
  for (int i = 0; i < numsegs; i++, seg++, ms++)
    {
      seg->v1 = &vertexes[SHORT(ms->v1)];
      seg->v2 = &vertexes[SHORT(ms->v2)];

      line_t *ldef = &lines[SHORT(ms->linedef)];
      seg->linedef = ldef;
      seg->side = SHORT(ms->side);
      // NOTE: partner_seg is NULL, length is 0 (not needed in software)

      seg->sidedef = ldef->sideptr[seg->side];
      seg->offset = SHORT(ms->offset);
      seg->angle = SHORT(ms->angle) << 16;

      seg->frontsector = ldef->sideptr[seg->side]->sector;
      seg->backsector = (ldef->flags & ML_TWOSIDED) ? ldef->sideptr[seg->side^1]->sector : NULL;
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
      ss->num_segs = SHORT(mss->numsegs);
      ss->first_seg = SHORT(mss->firstseg);
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
      ss->floorheight = SHORT(ms->floorheight);
      ss->ceilingheight = SHORT(ms->ceilingheight);

      ss->floorpic = tc.GetID(ms->floorpic, TEX_floor);
      ss->ceilingpic = tc.GetID(ms->ceilingpic, TEX_floor);

      ss->lightlevel = SHORT(ms->lightlevel);

      ss->tag = SHORT(ms->tag);

      ss->thinglist = NULL;
      ss->touching_thinglist = NULL; //SoM: 4/7/2000

      ss->stairlock = 0;
      ss->nextsec = -1;
      ss->prevsec = -1;

      ss->heightsec = -1; //SoM: 3/17/2000: This causes some real problems
      ss->heightsec_type = sector_t::CS_boom;
      ss->floorlightsec = -1;
      ss->ceilinglightsec = -1;
      ss->ffloors = NULL;
      ss->lightlist = NULL;
      ss->numlights = 0;
      ss->attached = NULL;
      ss->numattached = 0;
      ss->moved = true;
      ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
      ss->bottommap = ss->midmap = ss->topmap = NULL;

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


// BSP 'child is subsector' from original/v2 to v5.
static void ConvertGLFlags16_32(Uint32 &in)
{
  if (in & CHILD_IS_SUBSECTOR_OLD)
    {
      in &= ~CHILD_IS_SUBSECTOR_OLD;
      in |= NF_SUBSECTOR;
    }
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
      no->x = SHORT(mn->x);
      no->y = SHORT(mn->y);
      no->dx = SHORT(mn->dx);
      no->dy = SHORT(mn->dy);
      for (int j=0 ; j<2 ; j++)
        {
          no->children[j] = SHORT(mn->children[j]);
	  ConvertGLFlags16_32(no->children[j]); // needed here too (internally we use GLv5 nodes)
          for (int k=0 ; k<4 ; k++)
            no->bbox[j].box[k] = SHORT(mn->bbox[j][k]);
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
              R_PointInSubsector(t->x, t->y)->sector->seqType = ednum - 1400;
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
          if (ld->dy / ld->dx > 0)
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
      ld->frontsector = ld->sideptr[0] ? ld->sideptr[0]->sector : NULL;
      ld->backsector =  ld->sideptr[1] ? ld->sideptr[1]->sector : NULL;
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

      sd->textureoffset = SHORT(msd->textureoffset);
      sd->rowoffset = SHORT(msd->rowoffset);

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
		sd->midtexture = tc.GetTextureOrTransmap(mtex, ld->transmap); // can also be a transmap lumpname
		sd->bottomtexture = tc.GetID(btex);
		break;

	      case 4: // Legacy: create a colormap
		if (ttex[0] == '#' || btex[0] == '#')
		  {
		    sec->midmap = R_CreateColormap(ttex, mtex, btex);
		    sd->toptexture = sd->bottomtexture = 0;
		    
		    if (rendermode != render_soft) // FIXME is this necessary?
		      sec->extra_colormap = sec->midmap;
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
  Uint16 *blockmaplump = (Uint16 *)fc.CacheLumpNum(lump, PU_LEVEL);
  Uint16 *blockmap = blockmaplump + 4; // the offsets array

  // Endianness: everything in blockmap is expressed in 2-byte shorts
  int size = fc.LumpLength(lump)/2;
  for (int i=0; i < size; i++)
    blockmaplump[i] = SHORT(blockmaplump[i]);

  // read the header
  blockmapheader_t *bm = (blockmapheader_t *)blockmaplump;
  bmaporgx = bm->origin_x;
  bmaporgy = bm->origin_y;
  bmapwidth = bm->width;
  bmapheight = bm->height;

  int count = bmapwidth * bmapheight;
  // check the blockmap for errors
  int errors = 0;
  int first = 4 + count; // first possible blocklist offset (in shorts)
  int list_size = size - first;

  // we make a new 32-bit blockmap index
  bmap.index = (Uint16 **)Z_Malloc(count * sizeof(Uint16 *), PU_LEVEL, 0);
  bmap.lists = (Uint16 *)Z_Malloc(list_size * sizeof(Uint16), PU_LEVEL, 0);
  memcpy(bmap.lists, &blockmaplump[first], 2*list_size); // copy the lists

  if (list_size < 2) // one empty blocklist (two shorts) is the minimal size
    {
      CONS_Printf(" Blockmap lump is too small!\n");
      errors++;
    }

  if (size > 0x10000+1) // largest, always fully addressable blockmap
    CONS_Printf(" Warning: Blockmap may be too large.\n");


  for (int i=0; i < count && errors < 50; i++)
    {
      int offs = blockmap[i] - first;
      if (offs < 0)
	{
	  //CONS_Printf(" Invalid blocklist offset for cell %d: %d\n", i, blockmap[i]);
	  //errors++;
	  // TEST: assume that (short) offset has overflowed, fix
	  offs += 0x10000;
	}

      if (offs >= list_size)
	{
	  CONS_Printf(" Blocklist %d offset points past the lump!\n", i);
	  errors++;
	  continue;
	}

      // build new blockmap index
      bmap.index[i] = &bmap.lists[offs+1]; // skip the zero marker

      if (bmap.lists[offs] != 0x0000)
	{
	  CONS_Printf(" Blocklist %d does not start with zero!\n", i);
	  errors++;
	}

      while (offs < list_size && bmap.lists[offs] != MAPBLOCK_END)
	offs++;

      if (offs >= list_size)
	{
	  CONS_Printf(" Blocklist %d is unterminated!\n", i);
	  errors++;
	  continue;
	}
    }

  if (errors)
    I_Error("Blockmap (%dx%d cells, %d bytes) had some errors.\n", bmapwidth, bmapheight, 2*size);

  Z_Free(blockmaplump);

  // init the mobj chains
  blocklinks = (Actor **)Z_Malloc(count * sizeof(Actor *), PU_LEVEL, 0);
  memset(blocklinks, 0, count * sizeof(Actor *));

  // init the polyblockmap
  PolyBlockMap = (polyblock_t **)Z_Malloc(count * sizeof(polyblock_t *), PU_LEVEL, 0);
  memset(PolyBlockMap, 0, count * sizeof(polyblock_t *));
}


//
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void Map::GroupLines()
{
  int i, j;

  // Look up sector number for each subsector. This is made more complex by the
  // presence of minisegs from GL-subsectors (they have no associated lines!)
  subsector_t *ss = subsectors;
  for (i=0 ; i<numsubsectors ; i++, ss++)
    {
      if (ss->sector) // This subsector is already processed.
	continue;

      // Go through segs one by one. Stop when any assignation is done.
      for (unsigned csnum = ss->first_seg; csnum < ss->first_seg + ss->num_segs; csnum++)
	{
	  seg_t *seg = &segs[csnum];
	  if (seg->sidedef)
	    {
	      ss->sector = seg->sidedef->sector;
	      break;
	    }
	}

      // Every gl subsector should have at least one non-miniseg.
      if (!ss->sector)
	CONS_Printf("GL subsector %d consists entirely of minisegs. Unpredictable things may happen at any time.\n", i);
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
      int block = (bb[BOXTOP]-bmaporgy+MAXRADIUS).floor() >> MAPBLOCKBITS;
      block = block >= bmapheight ? bmapheight-1 : block;
      sector->blockbox[BOXTOP]=block;

      block = (bb[BOXBOTTOM]-bmaporgy-MAXRADIUS).floor() >> MAPBLOCKBITS;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXBOTTOM]=block;

      block = (bb[BOXRIGHT]-bmaporgx+MAXRADIUS).floor() >> MAPBLOCKBITS;
      block = block >= bmapwidth ? bmapwidth-1 : block;
      sector->blockbox[BOXRIGHT]=block;

      block = (bb[BOXLEFT]-bmaporgx-MAXRADIUS).floor() >> MAPBLOCKBITS;
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

// Convert short integers to long ones with byte swapping and sign
// extension. Maps -1 (0xFFFF) to -1 (0xFFFFFFFF).
//
// This should probably be somewhere else.

static int LONG_FROM_USHORT(short i)
{
  Uint32 result = SHORT(i);
  if(result == 65535)
    result = 0xFFFFFFFF;
  return result;
}


int Map::LoadGLVertexes(const int lump)
{
  const int glheaderlen = 4;
  int glversion;
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);

  if(strncmp(GL2_HEADER, (const char*)data, glheaderlen) == 0)
    glversion = 2;
  else if(strncmp(GL5_HEADER, (const char*)data, glheaderlen) == 0)
    glversion = 5;
  else {
    return -1; // We don't handle other versions.
  }

  numglvertexes = (fc.LumpLength(lump) - glheaderlen) / sizeof(mapglvertex_t);
  glvertexes = (vertex_t *)Z_Malloc(numglvertexes*sizeof(vertex_t),PU_LEVEL,0);

  mapglvertex_t *in = (mapglvertex_t *)(data+glheaderlen);
  vertex_t *out = glvertexes;

  for (int i=0 ; i<numglvertexes ; i++, in++, out++)
    {
      out->x.setvalue(LONG(in->x));
      out->y.setvalue(LONG(in->y));
    }

  Z_Free(data);

  return glversion;
}


static float SegLength(float dx, float dy)
{
  return sqrt(dx*dx + dy*dy);
}

/// GL segs always override the normal segs.
void Map::LoadGLSegs(const int lump, const int glversion)
{
  if (glversion == 2)
    numsegs = fc.LumpLength(lump) / sizeof(mapgl2seg_t);
  else // Assume version 5.
    numsegs = fc.LumpLength(lump) / sizeof(mapgl5seg_t);

  segs = (seg_t *)Z_Malloc(numsegs*sizeof(seg_t), PU_LEVEL, 0);
  memset(segs, 0, numsegs*sizeof(seg_t)); // clear all fields to zero
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);

  seg_t *seg = segs;
  mapgl2seg_t *in2 = (mapgl2seg_t*)data;
  mapgl5seg_t *in  = (mapgl5seg_t*)data;

  for (int i=0; i<numsegs; i++, seg++)
    {
      line_t *ldef;
      if (glversion == 2)
	{
	  Uint16 v = SHORT(in2->start_vertex);
	  if (v & VERT_IS_GL_V2)
	    seg->v1 = &glvertexes[v & (~VERT_IS_GL_V2)];
	  else
	    seg->v1 = &vertexes[v];

	  v = SHORT(in2->end_vertex);
	  if(v & VERT_IS_GL_V2)
	    seg->v2 = &glvertexes[v & (~VERT_IS_GL_V2)];
	  else
	    seg->v2 = &vertexes[v];

	  v = SHORT(in2->linedef);
	  if (v == NULL_INDEX)
	    ldef = NULL;
	  else
	    ldef = &lines[v];

	  if (in2->side == 0)
	    seg->side = 0;
	  else
	    seg->side = 1;

	  v = in2->partner_seg;
	  if(v == NULL_INDEX)
	    seg->partner_seg = NULL;
	  else
	    seg->partner_seg = &segs[v];

	  in2++;
	}
      else
	{ // V5 again.
	  Uint32 v = LONG(in->start_vertex);

	  if (v & VERT_IS_GL_V5)
	    seg->v1 = &glvertexes[v&(~VERT_IS_GL_V5)];
	  else
	    seg->v1 = &vertexes[v];

	  v = LONG(in->end_vertex);
	  if (v & VERT_IS_GL_V5)
	    seg->v2 = &glvertexes[v&(~VERT_IS_GL_V5)];
	  else
	    seg->v2 = &vertexes[v];

	  Uint16 ld = SHORT(in->linedef);
	  if (ld == NULL_INDEX)
	    ldef = NULL;
	  else
	    ldef = &lines[ld];

	  if (SHORT(in->side))
	    seg->side = 1;
	  else
	    seg->side = 0;

	  v = LONG(in->partner_seg);
	  if (v == NULL_INDEX_32)
	    seg->partner_seg = NULL;
	  else
	    seg->partner_seg = &segs[v]; 

	  in++;
	}

      seg->linedef = ldef;

      // fill in the rest
      seg->sidedef = ldef ? ldef->sideptr[seg->side] : NULL;

      if (ldef)
	{ // not a miniseg
	  float sx = ((seg->side ? ldef->v2->x : ldef->v1->x) - seg->v1->x).Float();
	  float sy = ((seg->side ? ldef->v2->y : ldef->v1->y) - seg->v1->y).Float();
	  seg->offset = SegLength(sx, sy);
	}

      // make a vector (start at origin)
      float dx = (seg->v2->x - seg->v1->x).Float();
      float dy = (seg->v2->y - seg->v1->y).Float();

      if (dx == 0)
	seg->angle = (dy > 0) ? ANG90 : ANG270;
      else
	seg->angle = angle_t((atan2(dy, dx) * ANG180) / M_PI);

      seg->length = SegLength(dx, dy);

      seg->frontsector = ldef->sideptr[seg->side]->sector;
      seg->backsector = (ldef->flags & ML_TWOSIDED) ? ldef->sideptr[seg->side^1]->sector : NULL;
    }

  Z_Free(data);
}


void Map::LoadGLSubsectors(const int lump, const int glversion)
{
  if (glversion == 2)
    numsubsectors = fc.LumpLength(lump) / sizeof(mapgl2subsector_t);
  else // Assume version 5.
    numsubsectors = fc.LumpLength(lump) / sizeof(mapgl5subsector_t);

  subsectors = (subsector_t *)Z_Malloc(numsubsectors*sizeof(subsector_t), PU_LEVEL, 0);
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);

  subsector_t *ss = subsectors;
  if (glversion == 2)
    {
      mapgl2subsector_t *in = (mapgl2subsector_t*)data;

      for(int i=0; i<numsubsectors; i++, in++, ss++)
	{
	  ss->num_segs  = LONG_FROM_USHORT(in->count);
	  ss->first_seg = LONG_FROM_USHORT(in->first_seg);
	  ss->sector = NULL;
	}
    }
  else
    { // V5 again.
      mapgl5subsector_t *in = (mapgl5subsector_t*)data;

      for(int i=0; i<numsubsectors; i++, in++, ss++)
	{
	  ss->num_segs = LONG(in->count);
	  ss->first_seg = LONG(in->first_seg);
	  ss->sector = NULL;
	}
    }


  Z_Free(data);
}



void Map::LoadGLNodes(const int lump, const int glversion)
{
  if (glversion == 2)
    numnodes = fc.LumpLength(lump) / sizeof(mapnode_t);
  else // Assume version 5.
    numnodes = fc.LumpLength(lump) / sizeof(mapgl5node_t);

  nodes = (node_t *)Z_Malloc(numnodes*sizeof(node_t), PU_LEVEL, 0);
  byte *data = (byte*)fc.CacheLumpNum(lump, PU_STATIC);
  node_t *no = nodes;

  if (glversion == 2)
    {
      mapnode_t *in = (mapnode_t*)data;

      for (int i=0; i<numnodes; i++, in++, no++)
	{
	  no->x = SHORT(in->x);
	  no->y = SHORT(in->y);
	  no->dx = SHORT(in->dx);
	  no->dy = SHORT(in->dy);
	  for (int j=0 ; j<2 ; j++)
	    {
	      no->children[j] = SHORT(in->children[j]);
	      ConvertGLFlags16_32(no->children[j]);	  
	      for (int k=0 ; k<4 ; k++)
		no->bbox[j].box[k] = SHORT(in->bbox[j][k]);
	    }
	}
    }
  else
    { // V5 again.
      mapgl5node_t *in = (mapgl5node_t*)data;
      for (int i=0; i<numnodes; i++, in++, no++)
	{
	  no->x = SHORT(in->x);
	  no->y = SHORT(in->y);
	  no->dx = SHORT(in->dx);
	  no->dy = SHORT(in->dy);
	  for (int j=0 ; j<2 ; j++)
	    {
	      no->children[j] = LONG(in->children[j]);
	      for (int k=0 ; k<4 ; k++)
		no->bbox[j].box[k] = SHORT(in->bbox[j][k]);
	    }
	}
    }

  Z_Free(data);
}

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

  if (info->sky1.empty())
    info->sky1 = "SKY1";

  if (info->sky2.empty())
    info->sky2 = "SKY1";

  skytexture = tc.GetPtr(info->sky1.c_str(), TEX_wall);

  // scale up the old skies, if needed
  R_SetupSkyDraw(skytexture->height);

  // set the sky flat num  FIXME should be Map-dependant
  if (hexen_format)
    skyflatnum = tc.GetID("F_SKY", TEX_floor);
  else
    skyflatnum = tc.GetID("F_SKY1", TEX_floor);
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
  if (cv_gravity.value != int(info->gravity * fixed_t::UNIT))
    {
      COM_BufAddText(va("gravity %f\n", info->gravity));
      COM_BufExecute();
    }

  SetupSky();

  // NOTE: most of this ordering is important
  // Load GL data if it exists, otherwise use normal data
  char glname[9] = "GL_\0\0\0\0\0";
  strncat(glname, lumpname.c_str(), 5);

  int gllump = fc.FindNumForName(glname);
  int gl_version = 0;
  if (gllump != -1)
    {
      gl_version = LoadGLVertexes(gllump+LUMP_GL_VERTEXES);
      CONS_Printf("Map %s has v%d GL nodes.\n", lumpname.c_str(), gl_version);

      if (gl_version < 0)
	I_Error("Can not handle GL nodes that are not v2 or v5.");
    }
  else
    {
      CONS_Printf("Level %s has no GL nodes.\n", lumpname.c_str());
      LoadVertexes(lumpnum+LUMP_VERTEXES); // just loads the vertices
    }

  LoadBlockMap(lumpnum+LUMP_BLOCKMAP); // loads (and possibly fixes) the blockmap
  LoadSectors1(lumpnum+LUMP_SECTORS);  // allocates sectors
  LoadSideDefs(lumpnum+LUMP_SIDEDEFS); // allocates sidedefs
  LoadLineDefs(lumpnum+LUMP_LINEDEFS); // points to v, si(and back)

  if (!hexen_format)
    ConvertLineDefs(); // just type conversion to mod. Hexen system

  LoadSideDefs2(lumpnum+LUMP_SIDEDEFS); // points to se, uses l, processes some linedef specials
  LoadLineDefs2(); // uses si, points to se

  if (gllump != -1)
    {
      LoadGLSubsectors(gllump+LUMP_GL_SSECT, gl_version);
      LoadGLNodes(gllump+LUMP_GL_NODES, gl_version);
      LoadGLSegs(gllump+LUMP_GL_SEGS, gl_version);
    }
  else
    {
      LoadSubsectors(lumpnum+LUMP_SSECTORS); // just loads them
      LoadNodes(lumpnum+LUMP_NODES); // loads nodes
      LoadSegs(lumpnum+LUMP_SEGS); // points to v, l, si, se, uses l
    }

  LoadSectors2(lumpnum+LUMP_SECTORS); // rest of secs, uses nothing!!!
  rejectmatrix = (byte *)fc.CacheLumpNum(lumpnum+LUMP_REJECT,PU_LEVEL);
  GroupLines();

  for (int i=0; i<numsectors; i++)
    SpawnSectorSpecial(sectors[i].special, &sectors[i]);

  R_SetFadetable(info->fadetablelump.c_str());

  // fix renderer to this map
  R.SetMap(this);

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

  // Setup the OpenGL renderer.
  if (rendermode == render_opengl) {
    gllevel_t l;
    if(glvertexes == NULL)
      I_Error("Trying to use OpenGL renderer without GL nodes. Exiting.\n");

    l.vertexes = vertexes;
    l.numvertexes = numvertexes;

    l.lines = lines;
    l.numlines = numlines;

    l.sides = sides;
    l.numsides = numsides;

    l.sectors = sectors;
    l.numsectors = numsectors;

    l.glvertexes = glvertexes;
    l.numglvertexes = numglvertexes;

    l.glsegs = segs;
    l.numglsegs = numsegs;

    l.glsubsectors = subsectors;
    l.numglsubsectors = numsubsectors;

    l.glnodes = nodes;
    l.numglnodes = numnodes;

    l.polyobjs = polyobjs;
    l.numpolyobjs = NumPolyobjs;

    oglrenderer->SetLevelData(l);
  }

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
