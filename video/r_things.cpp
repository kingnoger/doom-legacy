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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Software renderer: Sprite rendering

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "r_render.h"
#include "r_main.h"
#include "r_bsp.h"
#include "r_state.h"
#include "r_plane.h"
#include "r_sprite.h"
#include "r_draw.h"
#include "r_data.h"
#include "v_video.h"

#include "p_pspr.h"

#include "w_wad.h"
#include "wad.h"

#include "z_zone.h"
#include "z_cache.h"

#include "oglrenderer.hpp"

#include "i_video.h"            //rendermode

extern bool devparm;			//in d_main.cpp

#define MINZ                  (4)
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

int *spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
short           negonearray[MAXVIDWIDTH];
short           screenheightarray[MAXVIDWIDTH];



//=============================================================
//                     VisSprites
//=============================================================


/// A vissprite_t represents a sprite object that will be drawn during a refresh.
/// I.e. a mapthing that is (at least) partly visible.
struct vissprite_t
{
  // Doubly linked list.
  vissprite_t* prev;
  vissprite_t* next;

  /// left and right screen coordinate limits for the sprite
  int x1, x2;

  /// bottom and top screen coordinate limits for the sprite.
  int yb, yt;

  /// thing x,y position in world coords (for line side calculation)
  fixed_t px, py;

  /// thing bottom and top in world coords (for sorting with 3D floors)
  fixed_t pz, pzt;

  /// sprite bottom and top edges in world coords (for silhouette clipping)
  fixed_t gz, gzt;

  /// world coords * scale = screen coords
  fixed_t xscale, yscale;

  /// screen coords * xiscale = texture coords
  fixed_t xiscale;

  /// horizontal position of x1 (in texture coords)
  fixed_t startfrac;

  /// sprite top edge z relative to viewer in world coords
  fixed_t   sprite_top;
  Texture  *tex;

  /// colormap index used for lightlevel changes
  int lightmap;

  /// colormap used for color translation
  byte           *translationmap;

  /// which translucency table to use?
  byte           *transmap;

  /// SoM: 3/6/2000: height sector for underwater/fake ceiling support
  int                 heightsec;

  ///SoM: 4/3/2000: Global colormaps!
  fadetable_t    *extra_colormap;


  enum spritecut_e
  {
    SC_NONE = 0,
    SC_TOP = 1,
    SC_BOTTOM = 2
  };

  /// 0 for none, bit 1 for top, bit 2 for bottom
  int cut;  

public:
  vissprite_t *SplitSprite(Actor *thing, int cut_y, lightlist_t *ll);
  void DrawVisSprite();
};


#define MAXVISSPRITES   256
static vissprite_t  vissprites[MAXVISSPRITES];
static vissprite_t *vissprite_p;
static vissprite_t  overflowsprite;

static vissprite_t* R_NewVisSprite()
{
  if (vissprite_p == &vissprites[MAXVISSPRITES])
    return &overflowsprite;

  vissprite_p++;
  return vissprite_p-1;
}


// Called at frame start.
void R_ClearSprites()
{
  vissprite_p = vissprites;
}






//=================================================================
//                     presentation_t 
//=================================================================


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


/// This function does all the necessary frame changes for DActors and other state machines
/// (but what about 3d models, which have no frames???)
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
  flags = st->frame & ~TFF_FRAMEMASK; // the low 15 bits are wasted, but so what
  lastupdate = -1; // hack, since it will be Updated on this same tic TODO lastupdate should be set to nowtic
}


/// This is needed for PlayerPawns.
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
  return &spr->spriteframes[state->frame && TFF_FRAMEMASK];
}



// sprite rendering hacks
static fixed_t proj_tz, proj_tx;


/// this does the actual work of drawing the sprite in the SW renderer
void spritepres_t::Project(Actor *p)
{
  int frame = state->frame & TFF_FRAMEMASK;

  if (frame >= spr->numframes)
    {
      frame = spr->numframes - 1; // Thing may require more frames than the defaultsprite has...
      //I_Error("spritepres_t::Project: invalid sprite frame %d (%d)\n", frame, spr->numframes);
    }
    

  spriteframe_t *sprframe = &spr->spriteframes[frame];

  Texture  *t;
  bool      flip;

  // decide which patch to use for sprite relative to player
  if (sprframe->rotate)
    {
      // choose a different rotation based on player view
      angle_t ang = R.R_PointToAngle(p->pos.x, p->pos.y); // uses viewx,viewy
      unsigned rot = (ang - p->yaw + unsigned(ANG45/2) * 9) >> 29;

      t = sprframe->tex[rot];
      flip = sprframe->flip[rot];
    }
  else
    {
      // use single rotation for all views
      t = sprframe->tex[0];
      flip = sprframe->flip[0];
    }

#ifdef HWRENDER
  // hardware renderer part
  if (rendermode != render_soft)
    {
      return;
    }
#endif

  // software renderer part
  // aspect ratio stuff :
  fixed_t  xscale = projection  / proj_tz;
  fixed_t  yscale = projectiony / proj_tz; //added:02-02-98:aaargll..if I were a math-guy!!!

  // calculate edges of the shape
  proj_tx -= t->leftoffset / t->xscale; // left edge of sprite, world    TODO scaled offsets?**
  int x1 = (centerxfrac + (proj_tx * xscale)).floor(); // in screen coords

  // off the right side?
  if (x1 > viewwidth)
    return;

  proj_tx += t->width / t->xscale;
  int x2 = (centerxfrac + (proj_tx * xscale)).floor() - 1;

  // off the left side
  if (x2 < 0)
    return;

  //SoM: 3/17/2000: Disregard sprites that are out of view..
  fixed_t gzt = p->pos.z + (t->topoffset / t->yscale); // top edge of sprite, world **
  int light = 0;

  sector_t *sec = p->subsector->sector;

  if (sec->numlights)
    {
      int lightnum;
      light = R_GetPlaneLight(sec, gzt, false);
      if (sec->lightlist[light].caster && sec->lightlist[light].caster->flags & FF_FOG)
        lightnum = (*sec->lightlist[light].lightlevel  >> LIGHTSEGSHIFT);
      else
        lightnum = (*sec->lightlist[light].lightlevel  >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
	spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
      else
	spritelights = scalelight[lightnum];
    }

  int heightsec = sec->heightsec;

  if (heightsec != -1)   // only clip things which are in special sectors
    {
      int phs = R.viewplayer->subsector->sector->heightsec;
      if (phs != -1 && R.viewz < R.sectors[phs].floorheight ?
          p->pos.z >= R.sectors[heightsec].floorheight :
          gzt < R.sectors[heightsec].floorheight)
        return;
      if (phs != -1 && R.viewz > R.sectors[phs].ceilingheight ?
          gzt < R.sectors[heightsec].ceilingheight &&
          R.viewz >= R.sectors[heightsec].ceilingheight :
          p->pos.z >= R.sectors[heightsec].ceilingheight)
        return;
    }

  // store information in a vissprite
  vissprite_t *vis = R_NewVisSprite();
  vis->heightsec = heightsec; //SoM: 3/17/2000

  //vis->mobjflags = p->flags;

  vis->xscale = xscale;
  vis->yscale = yscale;           //<<detailshift;

  vis->px = p->pos.x;
  vis->py = p->pos.y;
  vis->pz = p->Feet();
  vis->pzt = p->Top();

  vis->gz = gzt - (t->height / t->yscale);
  vis->gzt = gzt;
  vis->sprite_top = vis->gzt - R.viewz;
  // foot clipping FIXME applies also elsewhere! gz, gzt!
  if (p->pos.z <= sec->floorheight)
    vis->sprite_top -= p->floorclip;

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->yt = (centeryfrac - ((vis->gzt - R.viewz) * yscale)).floor();
  vis->yb =  (centeryfrac - ((vis->gz - R.viewz) * yscale)).floor();

  vis->cut = 0;
  if (sec->numlights)
    vis->extra_colormap = sec->lightlist[light].extra_colormap;
  else
    vis->extra_colormap = sec->extra_colormap;

  if (flip)
    {
      vis->startfrac = t->width - fixed_epsilon;
      vis->xiscale = -t->xscale / xscale;
    }
  else
    {
      vis->startfrac = 0;
      vis->xiscale = t->xscale / xscale;
    }

  if (vis->x1 > x1)
    vis->startfrac += (vis->x1 - x1) * vis->xiscale;

  // texture to use
  vis->tex = t;

  // determine the lightlevel & special effects

  vis->transmap = NULL;

  // specific translucency
  if (flags & TFF_SMOKESHADE)
    vis->transmap = VIS_SMOKESHADE; // not really a transmap ... see R_DrawVisSprite
  else
    {
      if (flags & TFF_TRANSMASK)
        vis->transmap = transtables[((flags & TFF_TRANSMASK) >> TFF_TRANSSHIFT) - 1];
      else if (p->flags & MF_SHADOW)
        // actually only the player should use this (temporary invisibility)
        // because now the translucency is set through TFF_TRANSMASK
        vis->transmap = transtables[tr_transhi - 1];

      if (fixedcolormap)
        {
          // fixed map : all the screen has the same colormap
          //  eg: negative effect of invulnerability
          vis->lightmap = fixedcolormap;
        }
      else if ((flags & (TFF_FULLBRIGHT | TFF_TRANSMASK) || p->flags & MF_SHADOW) &&
	       (!vis->extra_colormap || !vis->extra_colormap->fog))
        {
          // full bright : goggles
          vis->lightmap = 0;
        }
      else
        {
          // diminished light
          int index = (xscale << (LIGHTSCALESHIFT+detailshift)).floor();

          if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE-1;

          vis->lightmap = spritelights[index];
        }
    }

  // color translation
  if (color)
    vis->translationmap = translationtables[color];
  else
    vis->translationmap = NULL;

  // split the vissprite into several if necessary
  for (int i = 1; i < sec->numlights; i++)
    {
      if (sec->lightlist[i].height >= vis->gzt || !(sec->lightlist[i].caster->flags & FF_CUTSPRITES))
	continue;
      if (sec->lightlist[i].height <= vis->gz)
	return;

      int cut_y = (centeryfrac - ((sec->lightlist[i].height - R.viewz) * vis->yscale)).floor();
      if (cut_y < 0)
	continue;
      if (cut_y > vid.height)
	return;

      // Found a split! Make a new sprite, copy the old sprite to it, and
      // adjust the heights.
      vis = vis->SplitSprite(p, cut_y, &sec->lightlist[i]);
    }

}



// ==========================================================================
//
//  New sprite loading routines for Legacy : support sprites in pwad,
//  dehacked sprite renaming, replacing not all frames of an existing
//  sprite, add sprites at run-time, add wads at run-time.
//
// ==========================================================================

spritecache_t sprites(PU_SPRITE);


sprite_t::sprite_t(const char *n)
  : cacheitem_t(n)
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

  Texture *t = tc.GetPtr(name, TEX_sprite);

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

  //CONS_Printf(" Sprite: %s  |  ", p);

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

  sprite_t *t = new sprite_t(p);

  // allocate this sprite's frames
  t->iname = intname;
  t->numframes = maxframe;
  t->spriteframes = (spriteframe_t *)Z_Malloc(maxframe*sizeof(spriteframe_t), PU_STATIC, NULL);
  memcpy(t->spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));

  //CONS_Printf("frames = %d\n", maxframe);

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

  sprites.SetDefaultItem("SMOK");
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
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short*          mfloorclip;
short*          mceilingclip;

fixed_t         spryscale; ///< texture coord * spryscale = screen coord
/// sprite screen coords
fixed_t         sprtopscreen;
fixed_t         sprbotscreen;
fixed_t         windowtop;
fixed_t         windowbottom;

void R_DrawMaskedColumn(column_t* column)
{
  fixed_t basetexturemid = dc_texturemid;

  for ( ; column->topdelta != 0xff ; )
    {
      // calculate unclipped screen coordinates
      //  for post
      fixed_t topscreen = sprtopscreen + spryscale*column->topdelta;
      fixed_t bottomscreen = sprbotscreen == fixed_t::FMAX ? topscreen + spryscale*column->length :
	sprbotscreen + spryscale*column->length;

      dc_yl = 1 + (topscreen - fixed_epsilon).floor();
      dc_yh = (bottomscreen - fixed_epsilon).floor();

      if (windowtop != fixed_t::FMAX && windowbottom != fixed_t::FMAX)
        {
          if (windowtop > topscreen)
            dc_yl = 1 + (windowtop - fixed_epsilon).floor();
          if (windowbottom < bottomscreen)
            dc_yh = (windowbottom - fixed_epsilon).floor();
        }

      if (dc_yh >= mfloorclip[dc_x])
	dc_yh = mfloorclip[dc_x]-1;
      if (dc_yl <= mceilingclip[dc_x])
	dc_yl = mceilingclip[dc_x]+1;

      if (dc_yl <= dc_yh && dc_yl < vid.height && dc_yh > 0)
        {
	  dc_source = column->data;
	  dc_texturemid = basetexturemid - column->topdelta;

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

      column = (column_t *)&column->data[column->length + 1];
    }

  dc_texturemid = basetexturemid;
}



//  mfloorclip and mceilingclip should also be set.
void vissprite_t::DrawVisSprite()
{
  if (transmap == VIS_SMOKESHADE)
    // shadecolfunc uses 'colormaps'
    colfunc = shadecolfunc;
  else if (transmap)
    {
      colfunc = fuzzcolfunc;
      dc_transmap = transmap;    //Fab:29-04-98: translucency table
    }

  if (translationmap)
    {
      // translate green skin to another color
      colfunc = transcolfunc;
      /*
	// Support for translated and translucent sprites. SSNTails 11-11-2002
      if (transmap)
        colfunc = transtransfunc; // TODO
      */

      dc_translation = translationmap;
    }


  if (extra_colormap && !fixedcolormap)
    dc_colormap = extra_colormap->colormap;
  else
    dc_colormap = R.base_colormap;

  dc_colormap += lightmap;

  spryscale = yscale / tex->yscale;
  sprtopscreen = centeryfrac - (sprite_top * yscale);
  windowtop = windowbottom = sprbotscreen = fixed_t::FMAX;

  // initialize drawers
  dc_iscale = tex->yscale / yscale; // = 1 / spryscale;
  dc_texturemid = sprite_top * tex->yscale;
  dc_texheight = 0; // clever way of drawing nonrepeating textures

  fixed_t frac = startfrac;
  for (dc_x = x1; dc_x <= x2; dc_x++, frac += xiscale)
    {
      int texturecolumn = frac.floor();
#ifdef RANGECHECK
      if (texturecolumn < 0 || texturecolumn >= t->width)
        I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
      if (tex->Masked())
	R_DrawMaskedColumn(tex->GetMaskedColumn(texturecolumn));
      // TODO unmasked drawer...
    }

  colfunc = basecolfunc;
}



// splits the vissprite into two  (different light conditions on different parts of the sprite!)
vissprite_t *vissprite_t::SplitSprite(Actor *thing, int cut_y, lightlist_t *ll)
{
  vissprite_t *newsprite = R_NewVisSprite();
  memcpy(newsprite, this, sizeof(vissprite_t));

  cut |= vissprite_t::SC_BOTTOM;
  gz = ll->height;

  newsprite->gzt = gz;

  yb = cut_y;
  newsprite->yt = yb - 1;

  if (ll->height < pzt && ll->height > pz)
    pz = newsprite->pzt = ll->height;
  else
    {
      // TODO this cannot be correct?!?
      newsprite->pz = newsprite->gz;
      newsprite->pzt = newsprite->gzt;
    }

  newsprite->cut |= vissprite_t::SC_TOP;
  if (!(ll->caster->flags & FF_NOSHADE))
    {
      int lightnum;

      if (ll->caster->flags & FF_FOG)
	lightnum = (*ll->lightlevel >> LIGHTSEGSHIFT);
      else
	lightnum = (*ll->lightlevel >> LIGHTSEGSHIFT) + extralight;

      if (lightnum < 0)
	spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
      else
	spritelights = scalelight[lightnum];

      newsprite->extra_colormap = ll->extra_colormap;

      // TODO ??? FIXME
      if (thing->pres->flags & TFF_SMOKESHADE)
	;
      else
	{
	  /*
	    if (thing->frame & TFF_TRANSMASK)
	    ;
	    else if (thing->flags & MF_SHADOW)
	    ;
	  */

	  if (fixedcolormap)
	    ;
	  else if ((thing->pres->flags & (TFF_FULLBRIGHT | TFF_TRANSMASK) || thing->flags & MF_SHADOW)
		   && (!newsprite->extra_colormap || !newsprite->extra_colormap->fog))
	    ;
	  else
	    {
	      int index = (xscale << (LIGHTSCALESHIFT+detailshift)).floor();

	      if (index >= MAXLIGHTSCALE)
		index = MAXLIGHTSCALE-1;
	      newsprite->lightmap = spritelights[index];
	    }
	}
    }
  return newsprite;
}




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
        fixed_t  tr_x = thing->pos.x - viewx;
        fixed_t  tr_y = thing->pos.y - viewy;

        proj_tz = (tr_x * viewcos) + (tr_y * viewsin);

        // thing is behind view plane?
        if (proj_tz < MINZ)
          continue;

        proj_tx = (tr_x * viewsin) - (tr_y * viewcos);

        // too far off the side?
        if (abs(proj_tx) > (proj_tz << 2))
          continue;

        thing->pres->Project(thing);
      }
}




const fixed_t PSpriteSY[NUMWEAPONS] =
{
     0,             // staff
     5,    // goldwand
    15,    // crossbow
    15,    // blaster
    15,    // skullrod
    15,    // phoenix rod
    15,    // mace
    15,    // gauntlets
    15     // beak
};

//
// R_DrawPSprite
//
void Rend::R_DrawPSprite(pspdef_t *psp)
{
  // decide which patch to use

  sprite_t *sprdef = sprites.Get(sprnames[psp->state->sprite]);
  spriteframe_t *sprframe = &sprdef->spriteframes[psp->state->frame & TFF_FRAMEMASK];

#ifdef PARANOIA
  if (!sprframe)
    I_Error("sprframes NULL for state %d\n", psp->state - weaponstates);
#endif

  Texture *t = sprframe->tex[0];

  // calculate edges of the shape

  //added:08-01-98:replaced mul by shift
  fixed_t tx = psp->sx - (BASEVIDWIDTH/2);

  //added:02-02-98:spriteoffset should be abs coords for psprites, based on
  //               320x200
  tx -= t->leftoffset / t->xscale;
  int x1 = (centerxfrac + (tx * pspritescale)).floor();

  // off the right side
  if (x1 > viewwidth)
    return;

  tx += t->width / t->xscale;
  int x2 = (centerxfrac + (tx * pspritescale)).floor() - 1;

  // off the left side
  if (x2 < 0)
    return;

  // store information in a vissprite
  vissprite_t avis;
  vissprite_t *vis = &avis;
  if (cv_splitscreen.value)
    vis->sprite_top = 120;
  else
    vis->sprite_top = BASEYCENTER;

  vis->sprite_top += 0.5f - psp->sy + (t->topoffset / t->yscale);

  /*
  if (game.mode >= gm_heretic)
    if (viewheight == vid.height || (!cv_scalestatusbar.value && vid.dupy>1))
      vis->sprite_top -= PSpriteSY[viewplayer->readyweapon];
  */

  //vis->sprite_top += 0.5f;

  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
  vis->yscale = pspriteyscale;  //<<detailshift;

  if (sprframe->flip[0])
    {
      vis->xiscale = -pspriteiscale * t->xscale;
      vis->startfrac = t->width - fixed_epsilon;
    }
  else
    {
      vis->xiscale = pspriteiscale * t->xscale;
      vis->startfrac = 0;
    }

  if (vis->x1 > x1)
    vis->startfrac += (vis->x1 - x1) * vis->xiscale;

  vis->tex = t;
  vis->transmap = NULL;
  vis->translationmap = NULL;

  if (viewplayer->flags & MF_SHADOW)      // invisibility effect
    {
      vis->lightmap = 0;   // use translucency

      // in Doom2, it used to switch between invis/opaque the last seconds
      // now it switch between invis/less invis the last seconds
      if (viewplayer->powers[pw_invisibility] > 4*TICRATE
          || viewplayer->powers[pw_invisibility] & 8)
        vis->transmap = transtables[tr_transhi - 1];
      else
        vis->transmap = transtables[tr_transmed - 1];
    }
  else if (fixedcolormap)
    {
      // fixed color
      vis->lightmap = fixedcolormap;
    }
  else if (psp->state->frame & TFF_FULLBRIGHT)
    {
      // full bright
      vis->lightmap = 0;
    }
  else
    {
      // local light
      vis->lightmap = spritelights[MAXLIGHTSCALE-1];
    }

  if(viewplayer->subsector->sector->numlights)
    {
      int lightnum;
      int light = R_GetPlaneLight(viewplayer->subsector->sector, viewplayer->Feet() + 41, false);
      vis->extra_colormap = viewplayer->subsector->sector->lightlist[light].extra_colormap;
      lightnum = (*viewplayer->subsector->sector->lightlist[light].lightlevel  >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];

      vis->lightmap = spritelights[MAXLIGHTSCALE-1];
    }
  else
    vis->extra_colormap = viewplayer->subsector->sector->extra_colormap;

  if(rendermode == render_soft)
    vis->DrawVisSprite();
  else if(rendermode == render_opengl) {
    if(vis->tex->glid == vis->tex->NOTEXTURE)
      vis->tex->GetData(); // Generates OpenGL texture id.
    // FIXME, the number 100 was taken at random to look good.
    oglrenderer->Draw2DGraphic_Doom(vis->x1, -100-vis->sprite_top.Float()+BASEVIDHEIGHT, vis->tex->width, vis->tex->height, vis->tex->glid);
  }
}



/// Renders the first person (weapon) sprites
void Rend::R_DrawPlayerSprites()
{
  int         lightnum;
  int         light = 0;

  if (rendermode != render_soft && 
      rendermode != render_opengl)
    return;

  // get light level
  if (viewplayer->subsector->sector->numlights)
    {
      light = R_GetPlaneLight(viewplayer->subsector->sector, viewplayer->Top(), false);
      lightnum = (*viewplayer->subsector->sector->lightlist[0].lightlevel >> LIGHTSEGSHIFT) + extralight;
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
  int kikhak = centery;
  centery = centerypsp;             //for R_DrawColumn
  centeryfrac = centery;  //for R_DrawVisSprite

  // add all active psprites
  pspdef_t *psp = viewplayer->psprites;
  for (int i=0; i<NUMPSPRITES; i++, psp++)
    {
      if (psp->state)
	R_DrawPSprite(psp);
    }

  //added:06-02-98: oooo dirty boy
  centery = kikhak;
  centeryfrac = centery;
}



//
// R_SortVisSprites
//
static vissprite_t vsprsortedhead;

void R_SortVisSprites()
{
  vissprite_t *ds;
  vissprite_t *best = NULL;      //shut up compiler

  int count = vissprite_p - vissprites;
  if (!count)
    return;

  vissprite_t unsorted;
  unsorted.next = unsorted.prev = &unsorted;

  for (ds=vissprites ; ds<vissprite_p ; ds++)
    {
      ds->next = ds+1;
      ds->prev = ds-1;
    }

  vissprites[0].prev = &unsorted;
  unsorted.next = &vissprites[0];
  (vissprite_p - 1)->next = &unsorted;
  unsorted.prev = vissprite_p-1;

  // pull the vissprites out by scale
  vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
  for (int i=0 ; i<count ; i++)
    {
      fixed_t bestscale = fixed_t::FMAX;
      for (ds=unsorted.next ; ds!= &unsorted ; ds=ds->next)
        {
	  if (ds->yscale < bestscale)
            {
	      bestscale = ds->yscale;
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



//=============================================================================
//    Drawnodes
//=============================================================================

static drawnode_t *R_CreateDrawNode(drawnode_t *link);
static drawnode_t  nodebankhead;
static drawnode_t  nodehead;

/// Creates and sorts a list of drawnodes for the scene being rendered.
void Rend::R_CreateDrawNodes()
{
  drawnode_t*   entry;
  int           i, p, x1, x2;
  


  fixed_t       gzm;
  fixed_t       scale;

  // Add the 3D floors, thicksides, and masked textures...
  for (drawseg_t *ds = ds_p; ds-- > drawsegs;)
    {
      if (ds->numthicksides)
	{
	  for (i = 0; i < ds->numthicksides; i++)
	    {
	      entry = R_CreateDrawNode(&nodehead);
	      entry->thickseg = ds;
	      entry->ffloor = ds->thicksides[i];
	    }
	}
      if (ds->maskedtexturecol)
	{
	  entry = R_CreateDrawNode(&nodehead);
	  entry->seg = ds;
	}
      if (ds->numffloorplanes)
	{
	  for(i = 0; i < ds->numffloorplanes; i++)
	    {
	      int best = -1;
	      fixed_t bestdelta = 0;
	      for(p = 0; p < ds->numffloorplanes; p++)
		{
		  if (!ds->ffloorplanes[p])
		    continue;
		  visplane_t *plane = ds->ffloorplanes[p];
		  R_PlaneBounds(plane);
		  if (plane->low < con_clipviewtop || plane->high > vid.height || plane->high > plane->low)
		    {
		      ds->ffloorplanes[p] = NULL;
		      continue;
		    }

		  fixed_t delta = abs(plane->height - viewz);
		  if (delta > bestdelta)
		    {
		      best = p;
		      bestdelta = delta;
		    }
		}
	      if (best != -1)
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

  if (vissprite_p == vissprites)
    return;

  R_SortVisSprites();
  for (vissprite_t *rover = vsprsortedhead.prev; rover != &vsprsortedhead; rover = rover->prev)
    {
      if (rover->yt > vid.height || rover->yb < 0)
        continue;

      int sintersect = (rover->x1 + rover->x2) / 2;
      gzm = (rover->gz + rover->gzt) / 2;

      drawnode_t *r2;
      for (r2 = nodehead.next; r2 != &nodehead; r2 = r2->next)
	{
	  if (r2->plane)
	    {
	      if (r2->plane->minx > rover->x2 || r2->plane->maxx < rover->x1)
		continue;
	      if (rover->yt > r2->plane->low || rover->yb < r2->plane->high)
		continue;

	      if ((r2->plane->height < viewz && rover->pz < r2->plane->height) ||
		  (r2->plane->height > viewz && rover->pzt > r2->plane->height))
		{
		  // SoM: NOTE: Because a visplane's shape and scale is not directly
		  // bound to any single lindef, a simple poll of it's frontscale is
		  // not adiquate. We must check the entire frontscale array for any
		  // part that is in front of the sprite.

		  x1 = rover->x1;
		  x2 = rover->x2;
		  if (x1 < r2->plane->minx) x1 = r2->plane->minx;
		  if (x2 > r2->plane->maxx) x2 = r2->plane->maxx;

		  for(i = x1; i <= x2; i++)
		    {
		      if (r2->seg->frontscale[i] > rover->yscale)
			break;
		    }
		  if (i > x2)
		    continue;

		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	  else if (r2->thickseg)
	    {
	      if (rover->x1 > r2->thickseg->x2 || rover->x2 < r2->thickseg->x1)
		continue;

	      scale = r2->thickseg->scale1 > r2->thickseg->scale2 ? r2->thickseg->scale1 : r2->thickseg->scale2;
	      if (scale <= rover->yscale)
		continue;
	      scale = r2->thickseg->scale1 + (r2->thickseg->scalestep * (sintersect - r2->thickseg->x1));
	      if (scale <= rover->yscale)
		continue;

	      if ((*r2->ffloor->topheight > viewz && *r2->ffloor->bottomheight < viewz) ||
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
	  else if (r2->seg)
	    {
	      if (rover->x1 > r2->seg->x2 || rover->x2 < r2->seg->x1)
		continue;

	      scale = r2->seg->scale1 > r2->seg->scale2 ? r2->seg->scale1 : r2->seg->scale2;
	      if (scale <= rover->yscale)
		continue;
	      scale = r2->seg->scale1 + (r2->seg->scalestep * (sintersect - r2->seg->x1));

	      if (rover->yscale < scale)
		{
		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	  else if (r2->sprite)
	    {
	      if (r2->sprite->x1 > rover->x2 || r2->sprite->x2 < rover->x1)
		continue;
	      if (r2->sprite->yt > rover->yb || r2->sprite->yb < rover->yt)
		continue;

	      if (r2->sprite->yscale > rover->yscale)
		{
		  entry = R_CreateDrawNode(NULL);
		  (entry->prev = r2->prev)->next = entry;
		  (entry->next = r2)->prev = entry;
		  entry->sprite = rover;
		  break;
		}
	    }
	}
      if (r2 == &nodehead)
	{
	  entry = R_CreateDrawNode(&nodehead);
	  entry->sprite = rover;
	}
    }
}




static drawnode_t *R_CreateDrawNode(drawnode_t *link)
{
  drawnode_t *node = nodebankhead.next;
  if (node == &nodebankhead)
    {
      node = (drawnode_t *)malloc(sizeof(drawnode_t));
    }
  else
    (nodebankhead.next = node->next)->prev = &nodebankhead;

  if (link)
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



static void R_DoneWithNode(drawnode_t *node)
{
  (node->next->prev = node->prev)->next = node->next;
  (node->next = nodebankhead.next)->prev = node;
  (node->prev = &nodebankhead)->next = node;
}



static void R_ClearDrawNodes()
{
  for (drawnode_t *rover = nodehead.next; rover != &nodehead; )
    {
      drawnode_t *next = rover->next;
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



// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of sprites hidden under the console
void Rend::R_DrawSprite(vissprite_t *spr)
{
  short               clipbot[MAXVIDWIDTH];
  short               cliptop[MAXVIDWIDTH];
  int                 x;

  for (x = spr->x1 ; x<=spr->x2 ; x++)
    clipbot[x] = cliptop[x] = -2;

  // Scan drawsegs from end to start for obscuring segs.
  // The first drawseg that has a greater scale
  //  is the clip seg.
  //SoM: 4/8/2000:
  // Pointer check was originally nonportable
  // and buggy, by going past LEFT end of array:

  //    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code
  for (drawseg_t *ds = ds_p; ds-- > drawsegs; )
    {

      // determine if the drawseg obscures the sprite
      if (ds->x1 > spr->x2
	  || ds->x2 < spr->x1
	  || (!ds->silhouette && !ds->maskedtexturecol))
        {
	  // does not cover sprite
	  continue;
        }

      int r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
      int r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

      fixed_t scale, lowscale;

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

      if (scale < spr->yscale
	  || (lowscale < spr->yscale
	      && !R_PointOnSegSide(spr->px, spr->py, ds->curline)))
        {
	  // masked mid texture?
	  /*if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (ds, r1, r2);*/
	  // seg is behind sprite
	  continue;
        }

      // clip this piece of the sprite
      int silhouette = ds->silhouette;

      if (spr->gz >= ds->bsilheight)
	silhouette &= ~SIL_BOTTOM;

      if (spr->gzt <= ds->tsilheight)
	silhouette &= ~SIL_TOP;

      if (silhouette == SIL_BOTTOM)
        {
	  // bottom sil
	  for (x=r1 ; x<=r2 ; x++)
	    if (clipbot[x] == -2)
	      clipbot[x] = ds->sprbottomclip[x];
        }
      else if (silhouette == SIL_TOP)
        {
	  // top sil
	  for (x=r1 ; x<=r2 ; x++)
	    if (cliptop[x] == -2)
	      cliptop[x] = ds->sprtopclip[x];
        }
      else if (silhouette == SIL_BOTH)
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
      fixed_t mh, temp;
      int h;
      int phs = viewplayer->subsector->sector->heightsec;
      if ((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
	  (temp = centeryfrac - ((mh -= viewz) * spr->yscale)) >= 0 &&
	  (h = temp.floor()) < viewheight)
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
	  (temp = centeryfrac - ((mh-viewz) * spr->yscale)) >= 0 &&
	  (h = temp.floor()) < viewheight)
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
  if(spr->cut & vissprite_t::SC_TOP && spr->cut & vissprite_t::SC_BOTTOM)
    {
      int h;
      for(x = spr->x1; x <= spr->x2; x++)
	{
	  h = spr->yt;
	  if(cliptop[x] == -2 || h > cliptop[x])
	    cliptop[x] = h;

	  h = spr->yb;
	  if(clipbot[x] == -2 || h < clipbot[x])
	    clipbot[x] = h;
	}
    }
  else if(spr->cut & vissprite_t::SC_TOP)
    {
      int h;
      for(x = spr->x1; x <= spr->x2; x++)
	{
	  h = spr->yt;
	  if(cliptop[x] == -2 || h > cliptop[x])
	    cliptop[x] = h;
	}
    }
  else if(spr->cut & vissprite_t::SC_BOTTOM)
    {
      int h;
      for(x = spr->x1; x <= spr->x2; x++)
	{
	  h = spr->yb;
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
  spr->DrawVisSprite();
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
