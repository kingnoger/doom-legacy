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
// Revision 1.20  2004/08/18 14:35:23  smite-meister
// PNG support!
//
// Revision 1.19  2004/08/13 18:25:11  smite-meister
// sw renderer fix
//
// Revision 1.18  2004/08/12 18:30:33  smite-meister
// cleaned startup
//
// Revision 1.17  2004/07/25 20:16:43  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.16  2004/07/05 16:53:45  smite-meister
// Netcode replaced
//
// Revision 1.15  2004/04/01 09:16:16  smite-meister
// Texture system bugfixes
//
// Revision 1.13  2003/12/21 12:29:09  smite-meister
// bugfixes
//
// Revision 1.12  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.11  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.10  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.9  2003/05/11 21:23:53  smite-meister
// Hexen fixes
//
// Revision 1.8  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.7  2003/03/23 14:24:14  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.6  2003/03/15 20:07:22  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.5  2003/03/08 16:07:20  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.4  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/23 23:22:47  smite-meister
// WAD2+WAD3 support, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:22:02  smite-meister
// Actor/DActor separation
//
// Revision 1.1.1.1  2002/11/16 14:18:49  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "r_render.h"
#include "r_local.h"
#include "r_state.h"
#include "r_sprite.h"
#include "v_video.h"

#include "p_pspr.h"

#include "w_wad.h"
#include "wad.h"

#include "z_zone.h"
#include "z_cache.h"

#include "i_video.h"            //rendermode


#define MINZ                  (FRACUNIT*4)
#define BASEYCENTER           (BASEVIDHEIGHT/2)
#define min(x,y) ( ((x)<(y)) ? (x) : (y) )

// put this in transmap of visprite to draw a shade
// ahaha, the pointer cannot be changed but its target can!
lighttable_t* VIS_SMOKESHADE = (lighttable_t *)-1;


struct maskdraw_t
{
  int  x1, x2;
  int  column;
  int  topclip;
  int  bottomclip;
};


// SoM: A drawnode is something that points to a 3D floor, 3D side or masked
// middle texture. This is used for sorting with sprites.
struct drawnode_t
{
  visplane_t*   plane;
  drawseg_t*    seg;
  drawseg_t*    thickseg;
  ffloor_t*     ffloor;
  vissprite_t*  sprite;

  drawnode_t *next, *prev;
};


//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t         pspritescale;
fixed_t         pspriteyscale;  //added:02-02-98:aspect ratio for psprites
fixed_t         pspriteiscale;

lighttable_t**  spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
short           negonearray[MAXVIDWIDTH];
short           screenheightarray[MAXVIDWIDTH];



void *presentation_t::operator new(size_t size)
{
  return Z_Malloc(size, PU_LEVSPEC, NULL); // same tag as with thinkers
}


void presentation_t::operator delete(void *mem)
{
  Z_Free(mem);
}

presentation_t::~presentation_t()
{}

spritepres_t::spritepres_t(const char *name, const mobjinfo_t *inf, int col)
{
  color = col;
  animseq = Idle;
  spr = sprites.Get(name); // cache the sprite
  info = inf;

  flags = 0;
  lastupdate = -1;
  if (info)
    SetFrame(&states[info->spawnstate]); // corresponds to Idle animation
  else
    state = &states[S_NULL]; // SetFrame or SetAnim fixes this
}


spritepres_t::~spritepres_t()
{
  spr->Release();
}


void spritepres_t::SetFrame(const state_t *st)
{
  // FIXME for now the name of SPR_NONE is "NONE", fix it when we have the default sprite

  // some sprites change name during animation (!!!)
  char *name = sprnames[st->sprite];

  if (spr->iname != *reinterpret_cast<int *>(name))
    {
      spr->Release();
      spr = sprites.Get(name);
    }

  state = st;
  flags = st->frame & FF_FRAMEMASK; // TODO the low 16 bits are wasted, but so what
  lastupdate = -1; // hack TODO lastupdate should be set to nowtic
}


void spritepres_t::SetAnim(int seq)
{
  const state_t *st;

  if (animseq == seq)
    return; // do not interrupt the animation

  animseq = seq;
  switch (seq)
    {
    case Idle:
    default:
      st = &states[info->spawnstate];
      break;

    case Run:
      st = &states[info->seestate];
      break;

    case Pain:
      st = &states[info->painstate];
      break;

    case Melee:
      st = &states[info->meleestate];
      if (st != &states[S_NULL])
        break;
      // if no melee anim, fallthrough to shoot anim

    case Shoot:
      st = &states[info->missilestate];
      break;

    case Death1:
      st = &states[info->deathstate];
      break;

    case Death2:
      st = &states[info->xdeathstate];
      break;

    case Death3:
      st = &states[info->crashstate];
      break;

    case Raise:
      st = &states[info->raisestate];
      break;
    }

  SetFrame(st);
}


bool spritepres_t::Update(int advance)
{
  // TODO replace "tics elapsed since last update" with "nowtic"...
  // the idea is to keep the count inside the presentation, not outside
  advance += lastupdate; // how many tics to advance, lastupdate is the remainder from last update

  const state_t *st = state;
  while (st->tics >= 0 && advance >= st->tics)
    {
      advance -= st->tics;
      int ns = st->nextstate;
      st = &states[ns];
    }

  if (st != state)
    SetFrame(st);

  lastupdate = advance;

  if (state == &states[info->spawnstate])
    animseq = Idle; // another HACK, since higher animations often end up here

  return true;
}


spriteframe_t *spritepres_t::GetFrame()
{
  return &spr->spriteframes[state->frame && FF_FRAMEMASK];
}

// ==========================================================================
//
//  New sprite loading routines for Legacy : support sprites in pwad,
//  dehacked sprite renaming, replacing not all frames of an existing
//  sprite, add sprites at run-time, add wads at run-time.
//
// ==========================================================================

spritecache_t sprites(PU_SPRITE);


sprite_t::sprite_t()
{
  iname = numframes = 0;
  spriteframes = NULL;
}

sprite_t::~sprite_t()
{
  /*
  for (int i = 0; i < numframes; i++)
    {
      spriteframe_t *frame = &spriteframes[i];
      // TODO release the textures of each frame...
    }
  */
  if (spriteframes)
    Z_Free(spriteframes);
}


// used when building a sprite from lumps
static spriteframe_t sprtemp[29];
static int maxframe;

static void R_InstallSpriteLump(const char *name, int frame, int rot, bool flip)
{
  // uses sprtemp array, maxframe
  if (frame >= 29 || rot > 8)
    I_Error("R_InstallSpriteLump: Bad frame characters in lump %s", name);

  if (frame > maxframe)
    maxframe = frame;

  Texture *t = tc.GetPtr(name);

#ifdef HWRENDER
  //BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
  // TODO what is this?
  //  -> Hurdler: this is because of the "monster feet under the ground" bug (see if we cannot fix that properly)
  //if (rendermode != render_soft && t->topoffset > 0 && t->topoffset < t->height)
    // perfect is patch.height but sometime it is too high
    //t->topoffset = min(t->topoffset+4, t->height);
#endif

  if (rot == 0)
    {
      // the lump should be used for all rotations
      if (sprtemp[frame].rotate == 0 && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s frame %c has "
                     "multiple rot=0 lump\n", name, 'A'+frame);

      if (sprtemp[frame].rotate == 1 && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s frame %c has rotations "
                     "and a rot=0 lump\n", name, 'A'+frame);

      sprtemp[frame].rotate = 0;
      for (int r = 0; r < 8; r++)
        {
          sprtemp[frame].tex[r] = t;
          sprtemp[frame].flip[r] = flip;
        }
    }
  else
    {
      // the lump is only used for one rotation
      if (sprtemp[frame].rotate == 0 && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s frame %c has rotations "
                     "and a rot=0 lump\n", name, 'A'+frame);

      sprtemp[frame].rotate = 1;
      rot--; // make 0 based

      if (sprtemp[frame].tex[rot] != (Texture *)(-1) && devparm)
        CONS_Printf ("R_InitSprites: Sprite %s : %c : %c "
                     "has two lumps mapped to it\n",
                     name, 'A'+frame, '1'+rot);

      // lumppat & lumpid are the same for original Doom, but different
      // when using sprites in pwad : the lumppat points the new graphics
      sprtemp[frame].tex[rot] = t;
      sprtemp[frame].flip[rot] = flip;
    }
}

//==============================================

spritecache_t::spritecache_t(memtag_t tag)
  : cache_t(tag)
{}


// We assume that the sprite is in Doom sprite format
cacheitem_t *spritecache_t::Load(const char *p)
{
  // seeks a S_START lump in each file, adds any
  // consequental lumps starting with p to the sprite

  int i, l;
  int intname = *(int *)p;

  memset(sprtemp, -1, sizeof(sprtemp));
  maxframe = -1;

  CONS_Printf(" Sprite: %s  |  ", p);

  int nwads = fc.Size();
  for (i=0; i<nwads; i++)
    {
      // find the sprites section in this file
      // we need at least the S_END
      // (not really, but for speedup)

      int start = fc.FindNumForNameFile("S_START", i, 0);
      if (start == -1)
        start = fc.FindNumForNameFile("SS_START", i, 0); //deutex compatib.

      if (start == -1)
        start = 0;      // search frames from start of wad (lumpnum low word is 0)
      else
        start++;   // just after S_START

      start &= 0xFFFF;    // 0 based in waddir

      int end = fc.FindNumForNameFile("S_END", i, 0);
      if (end == -1)
        end = fc.FindNumForNameFile("SS_END", i, 0);     //deutex compatib.

      if (end == -1)
        continue; // no S_END, no sprites accepted
      end &= 0xFFFF;

      const char *fullname;

      // scan the lumps, filling in the frames for whatever is found
      l = fc.FindPartialName(intname, i, start, &fullname);
      while (l != -1 && l < end)
        {
          int lump = (i << 16) + l;
          int frame = fullname[4] - 'A';
          int rotation = fullname[5] - '0';

          // skip NULL sprites from very old dmadds pwads
          if (fc.LumpLength(lump) <= 8)
            continue;

          const char *name = fc.FindNameForNum(lump);

          R_InstallSpriteLump(name, frame, rotation, false);
          if (fullname[6])
            {
              frame = fullname[6] - 'A';
              rotation = fullname[7] - '0';
              R_InstallSpriteLump(name, frame, rotation, true);
            }

          l = fc.FindPartialName(intname, i, l+1, &fullname);
        }
    }

  // if no frames found for this sprite
  if (maxframe == -1)
    return NULL;

  maxframe++;

  //  some checks to help development
  for (i = 0; i < maxframe; i++)
    switch (sprtemp[i].rotate)
      {
      case -1:
        // no rotations were found for that frame at all
        I_Error("No patches found for sprite %s frame %c", p, i+'A');
        break;

      case 0:
        // only the first rotation is needed
        break;

      case 1:
        // must have all 8 frames
        for (l = 0; l < 8; l++)
          // we test the patch lump, or the id lump whatever
          // if it was not loaded the two are -1
          if (sprtemp[i].tex[l] == (Texture *)(-1))
            I_Error("Sprite %s frame %c is missing rotations", p, i+'A');
        break;
      }

  sprite_t *t = new sprite_t;

  // allocate this sprite's frames
  t->iname = intname;
  t->numframes = maxframe;
  t->spriteframes = (spriteframe_t *)Z_Malloc(maxframe*sizeof(spriteframe_t), PU_STATIC, NULL);
  memcpy(t->spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));

  CONS_Printf("frames = %d\n", maxframe);

  return t;
}



/*

//
// Search for sprites replacements in a wad whose names are in namelist
//
static void R_AddSpriteDefs (char** namelist, int wadnum)
{
  int         i;

  // find the sprites section in this pwad
  // we need at least the S_END
  // (not really, but for speedup)

  int start = fc.FindNumForNamePwad("S_START", wadnum, 0);
  if (start == -1)
    start = fc.FindNumForNamePwad("SS_START", wadnum, 0); //deutex compatib.

  if (start == -1)
    start = 0;      // search frames from start of wad (lumpnum low word is 0)
  else
    start++;   // just after S_START

  start &= 0xFFFF;    // 0 based in lumpinfo

  int end = fc.FindNumForNamePwad("S_END",wadnum,0);
  if (end == -1)
    end = fc.FindNumForNamePwad("SS_END",wadnum,0);     //deutex compatib.

  if (end == -1)
    {
      if (devparm)
        CONS_Printf ("no sprites in pwad %d\n", wadnum);
      return;
      //I_Error ("R_AddSpriteDefs: S_END, or SS_END missing for sprites "
      //         "in pwad %d\n",wadnum);
    }
  end &= 0xFFFF;

  //
  // scan through lumps, for each sprite, find all the sprite frames
  //
  int addsprites = 0;
  for (i=0 ; i<numsprites ; i++)
    {
      spritename = namelist[i];

      if (R_AddSingleSpriteDef (spritename, &sprites[i], wadnum, start, end) )
        {
          // if a new sprite was added (not just replaced)
          addsprites++;
          if (devparm)
            CONS_Printf ("sprite %s set in pwad %d\n", namelist[i], wadnum);//Fab
        }
    }

  CONS_Printf ("%d sprites added from file %d\n", addsprites, wadnum);//Fab
  //CONS_Error ("press enter\n");
}
*/


//
// GAME FUNCTIONS
//
static vissprite_t  vissprites[MAXVISSPRITES];
static vissprite_t *vissprite_p;

static void R_InitSkins();
//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(char** namelist)
{
  int         i;

  for (i=0 ; i<MAXVIDWIDTH ; i++)
    negonearray[i] = -1;

  sprites.SetDefaultItem("PLAY");
  //models.SetDefaultItem("models/sarge/");
  //MD3_InitNormLookup();

  //
  // now check for sprite skins
  //

  // it can be is do before loading config for skin cvar possible value
  int nwads = fc.Size();
  R_InitSkins();
  for (i=0; i<nwads; i++)
    R_AddSkins(i);
}



//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites()
{
  vissprite_p = vissprites;
}


//
// R_NewVisSprite
//
static vissprite_t     overflowsprite;

vissprite_t* R_NewVisSprite()
{
  if (vissprite_p == &vissprites[MAXVISSPRITES])
    return &overflowsprite;

  vissprite_p++;
  return vissprite_p-1;
}



//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short*          mfloorclip;
short*          mceilingclip;

fixed_t         spryscale;
fixed_t         sprtopscreen;
fixed_t         sprbotscreen;
fixed_t         windowtop;
fixed_t         windowbottom;

void R_DrawMaskedColumn(column_t* column)
{
    int         topscreen;
    int         bottomscreen;
    fixed_t     basetexturemid;

    basetexturemid = dc_texturemid;

    for ( ; column->topdelta != 0xff ; )
    {
        // calculate unclipped screen coordinates
        //  for post
        topscreen = sprtopscreen + spryscale*column->topdelta;
        bottomscreen = sprbotscreen == MAXINT ? topscreen + spryscale*column->length :
                                                sprbotscreen + spryscale*column->length;

        dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
        dc_yh = (bottomscreen-1)>>FRACBITS;

        if(windowtop != MAXINT && windowbottom != MAXINT)
        {
          if(windowtop > topscreen)
            dc_yl = (windowtop + FRACUNIT - 1) >> FRACBITS;
          if(windowbottom < bottomscreen)
            dc_yh = (windowbottom - 1) >> FRACBITS;
        }

        if (dc_yh >= mfloorclip[dc_x])
            dc_yh = mfloorclip[dc_x]-1;
        if (dc_yl <= mceilingclip[dc_x])
            dc_yl = mceilingclip[dc_x]+1;

        if (dc_yl <= dc_yh && dc_yl < vid.height && dc_yh > 0)
        {
            dc_source = (byte *)column + 3;
            dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
            // dc_source = (byte *)column + 3 - column->topdelta;

            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            // FIXME a quick fix
            if (!ylookup[dc_yl] && colfunc==R_DrawColumn_8)
              {
                static int first = 1;
                if (first)
                  {
                    CONS_Printf("WARNING: avoiding a crash in %s %d\n", __FILE__, __LINE__);
                    first = 0;
                  }
              }
            else
              colfunc();
        }
        column = (column_t *)(  (byte *)column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
}



//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
static void R_DrawVisSprite(vissprite_t *vis, int  x1, int x2)
{
  int                 texturecolumn;
  fixed_t             frac;

  Texture *t = vis->tex;

  dc_colormap = vis->colormap;

  if (vis->transmap == VIS_SMOKESHADE)
    // shadecolfunc uses 'colormaps'
    colfunc = shadecolfunc;
  else if (vis->transmap)
    {
      colfunc = fuzzcolfunc;
      dc_transmap = vis->transmap;    //Fab:29-04-98: translucency table
    }
  /*
    FIXME color translation
  else if ()
    {
      // translate green skin to another color
      colfunc = transcolfunc;
      dc_translation = vis->colormap;
    }
  */

  if(vis->extra_colormap && !fixedcolormap)
    {
      if(!dc_colormap)
        dc_colormap = vis->extra_colormap->colormap;
      else
        dc_colormap = &vis->extra_colormap->colormap[dc_colormap - colormaps];
    }
  if(!dc_colormap)
    dc_colormap = colormaps;

  //dc_iscale = abs(vis->xiscale)>>detailshift;  ???
  dc_iscale = FixedDiv (FRACUNIT, vis->scale);
  dc_texturemid = vis->texturemid;
  dc_texheight = 0;

  frac = vis->startfrac;
  spryscale = vis->scale;
  sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);
  windowtop = windowbottom = sprbotscreen = MAXINT;


  for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
    {
      texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
      if (texturecolumn < 0 || texturecolumn >= t->width)
        I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
      column_t *column = t->GetMaskedColumn(texturecolumn);
      R_DrawMaskedColumn(column);
    }
  // TODO unmasked drawer...

  colfunc = basecolfunc;
}




//
// was R_SplitSprite
// runs through a sector's lightlist and
void Rend::R_SplitSprite(vissprite_t* sprite, Actor* thing)
{
  int           i, lightnum, index;
  fixed_t               cutfrac;
  sector_t*     sector;
  vissprite_t*  newsprite;

  sector = sprite->sector;

  for(i = 1; i < sector->numlights; i++)
  {
    if(sector->lightlist[i].height >= sprite->gzt || !(sector->lightlist[i].caster->flags & FF_CUTSPRITES))
      continue;
    if(sector->lightlist[i].height <= sprite->gz)
      return;

    cutfrac = (centeryfrac - FixedMul(sector->lightlist[i].height - viewz, sprite->scale)) >> FRACBITS;
        if(cutfrac < 0)
            continue;
        if(cutfrac > vid.height)
            return;

        // Found a split! Make a new sprite, copy the old sprite to it, and
    // adjust the heights.
    newsprite = R_NewVisSprite ();
    memcpy(newsprite, sprite, sizeof(vissprite_t));

    sprite->cut |= SC_BOTTOM;
    sprite->gz = sector->lightlist[i].height;

    newsprite->gzt = sprite->gz;

    sprite->sz = cutfrac;
    newsprite->szt = sprite->sz - 1;

        if(sector->lightlist[i].height < sprite->pzt && sector->lightlist[i].height > sprite->pz)
                sprite->pz = newsprite->pzt = sector->lightlist[i].height;
        else
        {
                newsprite->pz = newsprite->gz;
                newsprite->pzt = newsprite->gzt;
        }

    newsprite->cut |= SC_TOP;
    if(!(sector->lightlist[i].caster->flags & FF_NOSHADE))
    {
      if(sector->lightlist[i].caster->flags & FF_FOG)
        lightnum = (*sector->lightlist[i].lightlevel >> LIGHTSEGSHIFT);
      else
        lightnum = (*sector->lightlist[i].lightlevel >> LIGHTSEGSHIFT) + extralight;

      if (lightnum < 0)
          spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
          spritelights = scalelight[LIGHTLEVELS-1];
      else
          spritelights = scalelight[lightnum];

      newsprite->extra_colormap = sector->lightlist[i].extra_colormap;

      if (thing->pres->flags & FF_SMOKESHADE)
        ;
      else
      {
/*        if (thing->frame & FF_TRANSMASK)
          ;
        else if (thing->flags & MF_SHADOW)
          ;*/

        if (fixedcolormap )
          ;
        else if ((thing->pres->flags & (FF_FULLBRIGHT|FF_TRANSMASK) || thing->flags & MF_SHADOW) && (!newsprite->extra_colormap || !newsprite->extra_colormap->fog))
          ;
        else
        {
          index = sprite->xscale>>(LIGHTSCALESHIFT-detailshift);

          if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE-1;
          newsprite->colormap = spritelights[index];
        }
      }
    }
    sprite = newsprite;
  }
}




// hacks
fixed_t proj_tz, proj_tx;

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void Rend::R_AddSprites(sector_t* sec, int lightlevel)
{
  if (rendermode != render_soft)
    return;

  // BSP is traversed by subsector.
  // A sector might have been split into several
  //  subsectors during BSP building.
  // Thus we check whether its already added.
  if (sec->validcount == validcount)
    return;

  // Well, now it will be done.
  sec->validcount = validcount;

  if (!sec->numlights)
    {
      if (sec->heightsec == -1)
        lightlevel = sec->lightlevel;

      int lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];
    }

  // Handle all things in sector.
  for (Actor *thing = sec->thinglist; thing; thing = thing->snext)
    if (!(thing->flags2 & MF2_DONTDRAW))
      {
        // transform the origin point
        fixed_t  tr_x = thing->x - viewx;
        fixed_t  tr_y = thing->y - viewy;

        proj_tz = FixedMul(tr_x,viewcos) + FixedMul(tr_y,viewsin);

        // thing is behind view plane?
        if (proj_tz < MINZ)
          continue;

        proj_tx = FixedMul(tr_x,viewsin) - FixedMul(tr_y,viewcos);

        // too far off the side?
        if (abs(proj_tx) > (proj_tz << 2))
          continue;

        thing->pres->Project(thing);
        //R_ProjectSprite(thing);
      }
}




const int PSpriteSY[NUMWEAPONS] =
{
     0,             // staff
     5*FRACUNIT,    // goldwand
    15*FRACUNIT,    // crossbow
    15*FRACUNIT,    // blaster
    15*FRACUNIT,    // skullrod
    15*FRACUNIT,    // phoenix rod
    15*FRACUNIT,    // mace
    15*FRACUNIT,    // gauntlets
    15*FRACUNIT     // beak
};

//
// R_DrawPSprite
//
void Rend::R_DrawPSprite(pspdef_t *psp)
{
  // decide which patch to use
#ifdef RANGECHECK
  if ( (unsigned)psp->state->sprite >= numsprites)
    I_Error ("R_ProjectSprite: invalid sprite number %i ",
             psp->state->sprite);
#endif
  sprite_t *sprdef = sprites.Get(sprnames[psp->state->sprite]);
#ifdef RANGECHECK
  if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
    I_Error ("R_ProjectSprite: invalid sprite frame %i : %i for %s",
             psp->state->sprite, psp->state->frame, sprnames[psp->state->sprite]);
#endif
  spriteframe_t *sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

#ifdef PARANOIA
  //Fab:debug
  if (sprframe==NULL)
    I_Error("sprframes NULL for state %d\n", psp->state - weaponstates);
#endif

  Texture *t = sprframe->tex[0];

  // calculate edges of the shape

  //added:08-01-98:replaced mul by shift
  fixed_t tx = psp->sx-((BASEVIDWIDTH/2)<<FRACBITS); //*FRACUNITS);

  //added:02-02-98:spriteoffset should be abs coords for psprites, based on
  //               320x200
  tx -= (t->leftoffset << FRACBITS);
  int x1 = (centerxfrac + FixedMul (tx,pspritescale) ) >>FRACBITS;

  // off the right side
  if (x1 > viewwidth)
    return;

  tx += (t->width << FRACBITS);
  int x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

  // off the left side
  if (x2 < 0)
    return;

  // store information in a vissprite
  vissprite_t avis;
  vissprite_t *vis = &avis;
  if (cv_splitscreen.value)
    vis->texturemid = 120 << FRACBITS;
  else
    vis->texturemid = BASEYCENTER << FRACBITS;

  vis->texturemid += FRACUNIT/2 - psp->sy + (t->topoffset << FRACBITS);

  if (game.mode >= gm_heretic)
    if (viewheight == vid.height || (!cv_scalestatusbar.value && vid.dupy>1))
      vis->texturemid -= PSpriteSY[viewplayer->readyweapon];

  //vis->texturemid += FRACUNIT/2;

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->scale = pspriteyscale;  //<<detailshift;

  if (sprframe->flip[0])
    {
      vis->xiscale = -pspriteiscale;
      vis->startfrac = (t->width << FRACBITS) - 1;
    }
  else
    {
      vis->xiscale = pspriteiscale;
      vis->startfrac = 0;
    }

  if (vis->x1 > x1)
    vis->startfrac += vis->xiscale*(vis->x1-x1);

  //Fab: see above for more about lumpid,lumppat
  vis->tex = t;
  vis->transmap = NULL;
  if (viewplayer->flags & MF_SHADOW)      // invisibility effect
    {
      vis->colormap = NULL;   // use translucency

      // in Doom2, it used to switch between invis/opaque the last seconds
      // now it switch between invis/less invis the last seconds
      if (viewplayer->powers[pw_invisibility] > 4*TICRATE
          || viewplayer->powers[pw_invisibility] & 8)
        vis->transmap = ((tr_transhi-1)<<FF_TRANSSHIFT) + transtables;
      else
        vis->transmap = ((tr_transmed-1)<<FF_TRANSSHIFT) + transtables;
    }
  else if (fixedcolormap)
    {
      // fixed color
      vis->colormap = fixedcolormap;
    }
  else if (psp->state->frame & FF_FULLBRIGHT)
    {
      // full bright
      vis->colormap = colormaps;
    }
  else
    {
      // local light
      vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }

  if(viewplayer->subsector->sector->numlights)
    {
      int lightnum;
      int light = R_GetPlaneLight(viewplayer->subsector->sector, viewplayer->z + (41 << FRACBITS), false);
      vis->extra_colormap = viewplayer->subsector->sector->lightlist[light].extra_colormap;
      lightnum = (*viewplayer->subsector->sector->lightlist[light].lightlevel  >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];

      vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }
  else
    vis->extra_colormap = viewplayer->subsector->sector->extra_colormap;

  R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// was R_DrawPlayerSprites
//
void Rend::R_DrawPlayerSprites()
{
    int         i = 0;
    int         lightnum;
    int         light = 0;
    pspdef_t*   psp;

    int kikhak;

    if (rendermode != render_soft)
        return;

    // get light level
    if(viewplayer->subsector->sector->numlights)
    {
      light = R_GetPlaneLight(viewplayer->subsector->sector, viewplayer->z + viewplayer->height, false);
      lightnum = (*viewplayer->subsector->sector->lightlist[i].lightlevel >> LIGHTSEGSHIFT) + extralight;
    }
    else
      lightnum = (viewplayer->subsector->sector->lightlevel >> LIGHTSEGSHIFT) + extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
    else
        spritelights = scalelight[lightnum];

    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    //added:06-02-98: quickie fix for psprite pos because of freelook
    kikhak = centery;
    centery = centerypsp;             //for R_DrawColumn
    centeryfrac = centery<<FRACBITS;  //for R_DrawVisSprite

    // add all active psprites
    for (i=0, psp=viewplayer->psprites;
         i<NUMPSPRITES;
         i++,psp++)
    {
        if (psp->state)
            R_DrawPSprite (psp);
    }

    //added:06-02-98: oooo dirty boy
    centery = kikhak;
    centeryfrac = centery<<FRACBITS;
}



//
// R_SortVisSprites
//
vissprite_t     vsprsortedhead;


void R_SortVisSprites (void)
{
    int                 i;
    int                 count;
    vissprite_t*        ds;
    vissprite_t*        best=NULL;      //shut up compiler
    vissprite_t         unsorted;
    fixed_t             bestscale;

    count = vissprite_p - vissprites;

    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
        return;

    for (ds=vissprites ; ds<vissprite_p ; ds++)
    {
        ds->next = ds+1;
        ds->prev = ds-1;
    }

    vissprites[0].prev = &unsorted;
    unsorted.next = &vissprites[0];
    (vissprite_p-1)->next = &unsorted;
    unsorted.prev = vissprite_p-1;

    // pull the vissprites out by scale
    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
    for (i=0 ; i<count ; i++)
    {
        bestscale = MAXINT;
        for (ds=unsorted.next ; ds!= &unsorted ; ds=ds->next)
        {
            if (ds->scale < bestscale)
            {
                bestscale = ds->scale;
                best = ds;
            }
        }
        best->next->prev = best->prev;
        best->prev->next = best->next;
        best->next = &vsprsortedhead;
        best->prev = vsprsortedhead.prev;
        vsprsortedhead.prev->next = best;
        vsprsortedhead.prev = best;
    }
}



//
// R_CreateDrawNodes
// Creates and sorts a list of drawnodes for the scene being rendered.
static drawnode_t*    R_CreateDrawNode (drawnode_t* link);

static drawnode_t     nodebankhead;
static drawnode_t     nodehead;

void Rend::R_CreateDrawNodes()
{
  drawnode_t*   entry;
  drawseg_t*    ds;
  int           i, p, best, x1, x2;
  fixed_t       bestdelta, delta;
  vissprite_t*  rover;
  drawnode_t*   r2;
  visplane_t*   plane;
  int           sintersect;
  fixed_t       gzm;
  fixed_t       scale;

    // Add the 3D floors, thicksides, and masked textures...
    for(ds = ds_p; ds-- > drawsegs;)
    {
      if(ds->numthicksides)
      {
        for(i = 0; i < ds->numthicksides; i++)
        {
          entry = R_CreateDrawNode(&nodehead);
          entry->thickseg = ds;
          entry->ffloor = ds->thicksides[i];
        }
      }
      if(ds->maskedtexturecol)
      {
        entry = R_CreateDrawNode(&nodehead);
        entry->seg = ds;
      }
      if(ds->numffloorplanes)
      {
        for(i = 0; i < ds->numffloorplanes; i++)
        {
          best = -1;
          bestdelta = 0;
          for(p = 0; p < ds->numffloorplanes; p++)
          {
            if(!ds->ffloorplanes[p])
              continue;
            plane = ds->ffloorplanes[p];
            R_PlaneBounds(plane);
            if(plane->low < con_clipviewtop || plane->high > vid.height || plane->high > plane->low)
            {
              ds->ffloorplanes[p] = NULL;
              continue;
            }

            delta = abs(plane->height - viewz);
            if(delta > bestdelta)
            {
              best = p;
              bestdelta = delta;
            }
          }
          if(best != -1)
          {
            entry = R_CreateDrawNode(&nodehead);
            entry->plane = ds->ffloorplanes[best];
            entry->seg = ds;
            ds->ffloorplanes[best] = NULL;
          }
          else
            break;
        }
      }
    }

    if(vissprite_p == vissprites)
      return;

    R_SortVisSprites();
    for(rover = vsprsortedhead.prev; rover != &vsprsortedhead; rover = rover->prev)
    {
      if(rover->szt > vid.height || rover->sz < 0)
        continue;

      sintersect = (rover->x1 + rover->x2) / 2;
      gzm = (rover->gz + rover->gzt) / 2;

      for(r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
      {
        if(r2->plane)
        {
          if(r2->plane->minx > rover->x2 || r2->plane->maxx < rover->x1)
            continue;
          if(rover->szt > r2->plane->low || rover->sz < r2->plane->high)
            continue;

          if((r2->plane->height < viewz && rover->pz < r2->plane->height) ||
            (r2->plane->height > viewz && rover->pzt > r2->plane->height))
          {
            // SoM: NOTE: Because a visplane's shape and scale is not directly
            // bound to any single lindef, a simple poll of it's frontscale is
            // not adiquate. We must check the entire frontscale array for any
            // part that is in front of the sprite.

            x1 = rover->x1;
            x2 = rover->x2;
            if(x1 < r2->plane->minx) x1 = r2->plane->minx;
            if(x2 > r2->plane->maxx) x2 = r2->plane->maxx;

            for(i = x1; i <= x2; i++)
            {
              if(r2->seg->frontscale[i] > rover->scale)
                break;
            }
            if(i > x2)
              continue;

            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
        else if(r2->thickseg)
        {
          if(rover->x1 > r2->thickseg->x2 || rover->x2 < r2->thickseg->x1)
            continue;

          scale = r2->thickseg->scale1 > r2->thickseg->scale2 ? r2->thickseg->scale1 : r2->thickseg->scale2;
          if(scale <= rover->scale)
            continue;
          scale = r2->thickseg->scale1 + (r2->thickseg->scalestep * (sintersect - r2->thickseg->x1));
          if(scale <= rover->scale)
            continue;

          if((*r2->ffloor->topheight > viewz && *r2->ffloor->bottomheight < viewz) ||
            (*r2->ffloor->topheight < viewz && rover->gzt < *r2->ffloor->topheight) ||
            (*r2->ffloor->bottomheight > viewz && rover->gz > *r2->ffloor->bottomheight))
          {
            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
        else if(r2->seg)
        {
          if(rover->x1 > r2->seg->x2 || rover->x2 < r2->seg->x1)
            continue;

          scale = r2->seg->scale1 > r2->seg->scale2 ? r2->seg->scale1 : r2->seg->scale2;
          if(scale <= rover->scale)
            continue;
          scale = r2->seg->scale1 + (r2->seg->scalestep * (sintersect - r2->seg->x1));

          if(rover->scale < scale)
          {
            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
        else if(r2->sprite)
        {
          if(r2->sprite->x1 > rover->x2 || r2->sprite->x2 < rover->x1)
            continue;
          if(r2->sprite->szt > rover->sz || r2->sprite->sz < rover->szt)
            continue;

          if(r2->sprite->scale > rover->scale)
          {
            entry = R_CreateDrawNode(NULL);
            (entry->prev = r2->prev)->next = entry;
            (entry->next = r2)->prev = entry;
            entry->sprite = rover;
            break;
          }
        }
      }
      if(r2 == &nodehead)
      {
        entry = R_CreateDrawNode(&nodehead);
        entry->sprite = rover;
      }
    }
}




static drawnode_t* R_CreateDrawNode (drawnode_t* link)
{
  drawnode_t* node;

  node = nodebankhead.next;
  if(node == &nodebankhead)
  {
    node = (drawnode_t *)malloc(sizeof(drawnode_t));
  }
  else
    (nodebankhead.next = node->next)->prev = &nodebankhead;

  if(link)
  {
    node->next = link;
    node->prev = link->prev;
    link->prev->next = node;
    link->prev = node;
  }

  node->plane = NULL;
  node->seg = NULL;
  node->thickseg = NULL;
  node->ffloor = NULL;
  node->sprite = NULL;
  return node;
}



static void R_DoneWithNode(drawnode_t* node)
{
  (node->next->prev = node->prev)->next = node->next;
  (node->next = nodebankhead.next)->prev = node;
  (node->prev = &nodebankhead)->next = node;
}



static void R_ClearDrawNodes()
{
  drawnode_t* rover;
  drawnode_t* next;

  for(rover = nodehead.next; rover != &nodehead; )
  {
    next = rover->next;
    R_DoneWithNode(rover);
    rover = next;
  }

  nodehead.next = nodehead.prev = &nodehead;
}



void R_InitDrawNodes()
{
  nodebankhead.next = nodebankhead.prev = &nodebankhead;
  nodehead.next = nodehead.prev = &nodehead;
}



//
// R_DrawSprite
//
//Fab:26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of sprites hidden under the console
void Rend::R_DrawSprite (vissprite_t* spr)
{
    drawseg_t*          ds;
    short               clipbot[MAXVIDWIDTH];
    short               cliptop[MAXVIDWIDTH];
    int                 x;
    int                 r1;
    int                 r2;
    fixed_t             scale;
    fixed_t             lowscale;
    int                 silhouette;

    for (x = spr->x1 ; x<=spr->x2 ; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    //SoM: 4/8/2000:
    // Pointer check was originally nonportable
    // and buggy, by going past LEFT end of array:

    //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code
    for (ds=ds_p ; ds-- > drawsegs ; )
    {

        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2
         || ds->x2 < spr->x1
         || (!ds->silhouette
             && !ds->maskedtexturecol) )
        {
            // does not cover sprite
            continue;
        }

        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if (scale < spr->scale
            || ( lowscale < spr->scale
                 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
        {
            // masked mid texture?
            /*if (ds->maskedtexturecol)
                R_RenderMaskedSegRange (ds, r1, r2);*/
            // seg is behind sprite
            continue;
        }

        // clip this piece of the sprite
        silhouette = ds->silhouette;

        if (spr->gz >= ds->bsilheight)
            silhouette &= ~SIL_BOTTOM;

        if (spr->gzt <= ds->tsilheight)
            silhouette &= ~SIL_TOP;

        if (silhouette == 1)
        {
            // bottom sil
            for (x=r1 ; x<=r2 ; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        }
        else if (silhouette == 2)
        {
            // top sil
            for (x=r1 ; x<=r2 ; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        }
        else if (silhouette == 3)
        {
            // both
            for (x=r1 ; x<=r2 ; x++)
            {
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }
    }
    //SoM: 3/17/2000: Clip sprites in water.
    if (spr->heightsec != -1)  // only things in specially marked sectors
    {
        fixed_t h,mh;
        int phs = viewplayer->subsector->sector->heightsec;
        if ((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
           (h = centeryfrac - FixedMul(mh-=viewz, spr->scale)) >= 0 &&
           (h >>= FRACBITS) < viewheight)
        {
            if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
            {                          // clip bottom
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
            }
            else                        // clip top
            {
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = h;
            }
        }

        if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
           (h = centeryfrac - FixedMul(mh-viewz, spr->scale)) >= 0 &&
           (h >>= FRACBITS) < viewheight)
        {
            if (phs != -1 && viewz >= sectors[phs].ceilingheight)
            {                         // clip bottom
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (clipbot[x] == -2 || h < clipbot[x])
                  clipbot[x] = h;
            }
            else                       // clip top
            {
              for (x=spr->x1 ; x<=spr->x2 ; x++)
                if (cliptop[x] == -2 || h > cliptop[x])
                  cliptop[x] = h;
            }
        }
    }
    if(spr->cut & SC_TOP && spr->cut & SC_BOTTOM)
    {
      fixed_t   h;
      for(x = spr->x1; x <= spr->x2; x++)
      {
        h = spr->szt;
        if(cliptop[x] == -2 || h > cliptop[x])
          cliptop[x] = h;

        h = spr->sz;
        if(clipbot[x] == -2 || h < clipbot[x])
          clipbot[x] = h;
      }
    }
    else if(spr->cut & SC_TOP)
    {
      fixed_t   h;
      for(x = spr->x1; x <= spr->x2; x++)
      {
        h = spr->szt;
        if(cliptop[x] == -2 || h > cliptop[x])
          cliptop[x] = h;
      }
    }
    else if(spr->cut & SC_BOTTOM)
    {
      fixed_t   h;
      for(x = spr->x1; x <= spr->x2; x++)
      {
        h = spr->sz;
        if(clipbot[x] == -2 || h < clipbot[x])
          clipbot[x] = h;
      }
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            //Fab:26-04-98: was -1, now clips against console bottom
            cliptop[x] = con_clipviewtop;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr, spr->x1, spr->x2);
}


//
// was R_DrawMasked
//
void Rend::R_DrawMasked()
{
    drawnode_t*           r2;
    drawnode_t*           next;

    R_CreateDrawNodes();

    for(r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
    {
      if(r2->plane)
      {
        next = r2->prev;
        R_DrawSinglePlane(r2->plane, true);
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->seg && r2->seg->maskedtexturecol != NULL)
      {
        next = r2->prev;
        R_RenderMaskedSegRange(r2->seg, r2->seg->x1, r2->seg->x2);
        r2->seg->maskedtexturecol = NULL;
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->thickseg)
      {
        next = r2->prev;
        R.R_RenderThickSideRange(r2->thickseg, r2->thickseg->x1, r2->thickseg->x2, r2->ffloor);
        R_DoneWithNode(r2);
        r2 = next;
      }
      else if(r2->sprite)
      {
        next = r2->prev;
        R.R_DrawSprite(r2->sprite);
        R_DoneWithNode(r2);
        r2 = next;
      }
    }
    R_ClearDrawNodes();
}





// ==========================================================================
//
//                              SKINS CODE
//
// ==========================================================================

int         numskins=0;
skin_t      skins[MAXSKINS+1];


// don't work because it must be inistilised before the config load
//#define SKINVALUES
#ifdef SKINVALUES
CV_PossibleValue_t skin_cons_t[MAXSKINS+1];
#endif

void Sk_SetDefaultValue(skin_t *skin)
{
    //
    // setup the 'marine' as default skin
    //
    memset (skin, 0, sizeof(skin_t));
    strcpy (skin->name, "marine");
    strcpy (skin->faceprefix, "STF");
    /*
      // FIXME skin sounds must be stored with the skins, not in S_Sfx
    for (int i=0;i<sfx_freeslot0;i++)
        if (S_sfx[i].skinsound!=-1)
        {
            skin->soundsid[S_sfx[i].skinsound] = i;
        }
    */
    memcpy(&skins[0].spritedef, sprites.Get("PLAY"), sizeof(sprite_t));
}

//
// Initialize the basic skins
//
void R_InitSkins()
{
#ifdef SKINVALUES
    int i;

    for(i=0;i<=MAXSKINS;i++)
    {
        skin_cons_t[i].value=0;
        skin_cons_t[i].strvalue=NULL;
    }
#endif

    // initialize free sfx slots for skin sounds
    //S_InitRuntimeSounds ();

    // skin[0] = marine skin
    Sk_SetDefaultValue(&skins[0]);
#ifdef SKINVALUES
    skin_cons_t[0].strvalue=skins[0].name;
#endif

    // make the standard Doom2 marine as the default skin
    numskins = 1;
}

// returns true if the skin name is found (loaded from pwad)
// warning return 0 (the default skin) if not found
int R_SkinAvailable (char* name)
{
    int  i;

    for (i=0;i<numskins;i++)
    {
        if (strcasecmp(skins[i].name,name)==0)
            return i;
    }
    return 0;
}

// network code calls this when a 'skin change' is received
void SetPlayerSkin(int playernum, char *skinname)
{
  /*
  int   i;

    // FIXME!
    for (i=0;i<numskins;i++)
    {
        // search in the skin list
        if (strcasecmp(skins[i].name,skinname)==0)
        {

            // change the face graphics
          // FIXME this should be done in HUD, not here.
            if (playernum == hud.sbpawn->player->number &&
            // for save time test it there is a real change
                strcmp (skins[players[playernum].skin].faceprefix, skins[i].faceprefix) )
            {
                ST_unloadFaceGraphics ();
                ST_loadFaceGraphics (skins[i].faceprefix);
            }

            players[playernum].skin = i;
            if (players[playernum].mo)
                players[playernum].mo->skin = &skins[i];

            return;
        }
    }

    CONS_Printf("Skin %s not found\n",skinname);
    players[playernum].skin = 0;  // not found put the old marine skin

    // a copy of the skin value
    // so that dead body detached from respawning player keeps the skin
    if (players[playernum].mo)
        players[playernum].mo->skin = &skins[0];
  */
}

//
// Add skins from a pwad, each skin preceded by 'S_SKIN' marker
//

// Does the same is in w_wad, but check only for
// the first 6 characters (this is so we can have S_SKIN1, S_SKIN2..
// for wad editors that don't like multiple resources of the same name)
//
int W_CheckForSkinMarkerInPwad (int wadid, int startlump)
{
  int iname = *reinterpret_cast<const int *>("S_SK");
  int nlumps = fc.GetNumLumps(wadid);
  const char *fullname;

  // scan forward, start at <startlump>
  if (startlump < nlumps)
    {
      int i = fc.FindPartialName(iname, wadid, startlump, &fullname);
      while (i != -1)
        {
          if (fullname[4] == 'I' && fullname[5] == 'N')
            return ((wadid<<16)+i);

          i = fc.FindPartialName(iname, wadid, i+1, &fullname);
        }
    }
  return -1; // not found
}

//
// Find skin sprites, sounds & optional status bar face, & add them
//
void R_AddSkins (int wadnum)
{
  /*
    int         lumpnum;
    int         lastlump;

    waddir_t* lumpinfo;
    char*       sprname=NULL;
    int         intname;

    char*       buf;
    char*       buf2;

    char*       token;
    char*       value;

    int         i,size;

    //
    // search for all skin markers in pwad
    //


      // FIXME the skin system is now commented out, fix it!

    lastlump = 0;
    while ( (lumpnum=W_CheckForSkinMarkerInPwad (wadnum, lastlump))!=-1 )
    {
        if (numskins>MAXSKINS)
        {
            CONS_Printf ("ignored skin (%d skins maximum)\n",MAXSKINS);
            lastlump++;
            continue; //faB:so we know how many skins couldn't be added
        }

        buf  = (char *)fc.CacheLumpNum (lumpnum, PU_CACHE);
        size = fc.LumpLength (lumpnum);

        // for strtok
        buf2 = (char *) malloc (size+1);
        if(!buf2)
             I_Error("R_AddSkins: No more free memory\n");
        memcpy (buf2,buf,size);
        buf2[size] = '\0';

        // set defaults
        Sk_SetDefaultValue(&skins[numskins]);
        sprintf (skins[numskins].name,"skin %d",numskins);

        // parse
        token = strtok (buf2, "\r\n= ");
        while (token)
        {
          if(token[0]=='/' && token[1]=='/') // skip comments
            {
              token = strtok (NULL, "\r\n"); // skip end of line
              goto next_token;               // find the real next token
            }

          value = strtok (NULL, "\r\n= ");
          //            CONS_Printf("token = %s, value = %s",token,value);
          //            CONS_Error("ga");

          if (!value)
            I_Error ("R_AddSkins: syntax error in S_SKIN lump# %d in WAD %d\n", lumpnum&0xFFFF, wadnum);

          if (!strcasecmp(token,"name"))
            {
                // the skin name must uniquely identify a single skin
                // I'm lazy so if name is already used I leave the 'skin x'
                // default skin name set above
                if (!R_SkinAvailable (value))
                {
                    strncpy (skins[numskins].name, value, SKINNAMESIZE);
                    strlwr (skins[numskins].name);
                }
            }
            else
            if (!strcasecmp(token,"face"))
            {
                strncpy (skins[numskins].faceprefix, value, 3);
                skins[numskins].faceprefix[3] = 0;
                strupr (skins[numskins].faceprefix);
            }
            else
            if (!strcasecmp(token,"sprite"))
            {
                sprname = value;
                strupr(sprname);
            }
            else
            {
                int found=false;
                // copy name of sounds that are remapped for this skin
                for (i=0;i<sfx_freeslot0;i++)
                {
                    if (!S_sfx[i].name)
                      continue;
                    if (S_sfx[i].skinsound!=-1 &&
                        !strcasecmp(S_sfx[i].name, token+2) )
                    {
                        skins[numskins].soundsid[S_sfx[i].skinsound]=
                            S_AddSoundFx(value+2,S_sfx[i].singularity);
                        found=true;
                    }
                }
                if(!found)
                  I_Error("R_AddSkins: Unknown keyword '%s' in S_SKIN lump# %d (WAD %i)\n",token,lumpnum&0xFFFF, wadnum);
            }
next_token:
            token = strtok (NULL,"\r\n= ");
        }

        // if no sprite defined use spirte juste after this one
        if( !sprname )
        {
            lumpnum &= 0xFFFF;      // get rid of wad number
            lumpnum++;
            lumpinfo = fc.GetLumpinfo(wadnum);

            // get the base name of this skin's sprite (4 chars)
            sprname = lumpinfo[lumpnum].name;
            intname = *(int *)sprname;

            // skip to end of this skin's frames
            lastlump = lumpnum;
            while (*(int *)lumpinfo[lastlump].name == intname)
                lastlump++;
            // allocate (or replace) sprite frames, and set spritedef
            R_AddSingleSpriteDef (sprname, &skins[numskins].spritedef, wadnum, lumpnum, lastlump);
        }
        else
        {
            // search in the normal sprite tables
            char **name;
            bool found = false;
            for(name = sprnames;*name;name++)
                if( strcmp(*name, sprname) == 0 )
                {
                    found = true;
                    skins[numskins].spritedef = sprites[sprnames-name];
                }

            // not found so make a new one
            if( !found )
                R_AddSingleSpriteDef (sprname, &skins[numskins].spritedef, wadnum, 0, MAXINT);
        }

        CONS_Printf ("added skin '%s'\n", skins[numskins].name);
#ifdef SKINVALUES
        skin_cons_t[numskins].value=numskins;
        skin_cons_t[numskins].strvalue=skins[numskins].name;
#endif

        numskins++;
        free(buf2);
    }
    */
    return;
}
