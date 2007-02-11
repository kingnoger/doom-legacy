// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Sprites and sprite presentations.

#include "doomdef.h"

#include "g_actor.h"
#include "g_decorate.h"

#include "r_data.h"
#include "r_sprite.h"
#include "r_things.h"
#include "r_render.h"
#include "i_video.h"
#include "hardware/oglrenderer.hpp"

#include "info.h"
#include "tables.h"
#include "w_wad.h"

#include "hardware/md3.h"

//==========================================================================
//                         Global functions
//==========================================================================

static void R_InitSkins();

/// Initialize sprite system. Called at program start.
void R_InitSprites(char** namelist)
{
  for (int i=0 ; i<MAXVIDWIDTH ; i++)
    negonearray[i] = -1;

  sprites.SetDefaultItem("SMOK");
#ifdef TEST_MD3
  models.SetDefaultItem("models/players/imp/");
#endif
  //MD3_InitNormLookup();

  // now check for sprite skins
  R_InitSkins();
}



//==========================================================================
//                              SKINS CODE
//==========================================================================

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
  int i;
#ifdef SKINVALUES


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

    int nwads = fc.Size();
    for (i=0; i<nwads; i++)
      R_AddSkins(i);
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


//==========================================================================
//                              sprite_t
//==========================================================================


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
      // TODO Release() the textures of each frame...
    }
  */
  if (spriteframes)
    Z_Free(spriteframes);
}


//==========================================================================
//                           spritecache_t
//==========================================================================

spritecache_t sprites(PU_SPRITE);

spritecache_t::spritecache_t(memtag_t tag)
  : cache_t(tag)
{}


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



// We assume that the sprite is in Doom sprite format
cacheitem_t *spritecache_t::Load(const char *p)
{
  // seeks a S_START lump in each file, adds any
  // consequental lumps starting with p to the sprite

  int i, l;
  int intname = *reinterpret_cast<const Sint32 *>(p);

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



//=================================================================
//                      presentation_t 
//=================================================================


void *presentation_t::operator new(size_t size)
{
  return Z_Malloc(size, PU_LEVSPEC, NULL); // same tag as with thinkers
}


void presentation_t::operator delete(void *mem)
{
  Z_Free(mem);
}


void spritepres_t::Pack(BitStream *s)
{
  s->writeInt(info->GetMobjType(), 16); // DActorInfo defines the spritepres
  s->writeInt(color, 8);
  PackAnim(s);
  // flags and lastupdate need not be sent
}

/// replaces Unpack()
spritepres_t::spritepres_t(BitStream *s)
{
  int temp = s->readInt(16);
  info = aid[mobjtype_t(temp)];
  color = s->readInt(8);

  spr = NULL;
  UnpackAnim(s);
}


void spritepres_t::PackAnim(BitStream *s)
{
  s->writeInt(animseq, 8);
  s->writeInt(state - states, 16);
}


void spritepres_t::UnpackAnim(BitStream *s)
{
  animseq = animseq_e(s->readInt(8));
  SetFrame(&states[s->readInt(16)]);
}


spritepres_t::spritepres_t(const ActorInfo *inf, int col)
{
  info = inf;
  spr = NULL;
  color = col;
  animseq = Idle;

  if (info && info->spawnstate)
    SetFrame(info->spawnstate); // corresponds to Idle animation
  else
    {
      state = &states[S_NULL]; // SetFrame or SetAnim fixes this
      flags = 0;
      lastupdate = -1;
    }
}


spritepres_t::~spritepres_t()
{
  if (spr)
    spr->Release();
}


/// This function does all the necessary frame changes for DActors and other state machines
/// (but what about 3d models, which have no frames???)
void spritepres_t::SetFrame(const state_t *st)
{
  // FIXME for now the name of SPR_NONE is "NONE", fix it when we have the default sprite

  // some sprites change name during animation (!!!)
  char *name = sprnames[st->sprite];

  if (!spr || spr->iname != *reinterpret_cast<Sint32 *>(name))
    {
      if (spr)
	spr->Release();
      spr = sprites.Get(name);
    }

  state = st;
  flags = st->frame & ~TFF_FRAMEMASK; // the low 15 bits are wasted, but so what
  lastupdate = -1; // hack, since it will be Updated on this same tic TODO lastupdate should be set to nowtic
}


/// This is needed for PlayerPawns.
void spritepres_t::SetAnim(animseq_e seq)
{
  /*
  const state_t *st;

  if (animseq == seq)
    return; // do not interrupt the animation

  animseq = seq;
  switch (seq)
    {
    case Idle:
    default:
      st = info->spawnstate;
      break;

    case Run:
      st = info->seestate;
      break;

    case Pain:
      st = info->painstate;
      break;

    case Melee:
      st = info->meleestate;
      if (st)
        break;
      // if no melee anim, fallthrough to shoot anim

    case Shoot:
      st = info->missilestate;
      break;

    case Death1:
      st = info->deathstate;
      break;

    case Death2:
      st = info->xdeathstate;
      break;

    case Death3:
      st = info->crashstate;
      break;

    case Raise:
      st = info->raisestate;
      break;
    }

  SetFrame(st);
  */
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
      st = st->nextstate;
    }

  if (st != state)
    SetFrame(st);

  lastupdate = advance;

  if (state == info->spawnstate)
    animseq = Idle; // another HACK, since higher animations often end up here

  return true;
}


spriteframe_t *spritepres_t::GetFrame()
{
  return &spr->spriteframes[state->frame && TFF_FRAMEMASK];
}



bool spritepres_t::Draw(const Actor *p)
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
      angle_t ang = R_PointToAngle2(fixed_t(oglrenderer->x), fixed_t(oglrenderer->y), p->pos.x, p->pos.y);
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

  /*
  float alpha;
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // normal

  // TODO color translation maps, TFF_FULLBRIGHT
  if (state->frame & TFF_TRANSMASK)
    {
      const float convert[] = {1, 0.5, 0.2, 0.1, 0.5, 1}; // hack, not perfect
      int tr_table = (state->frame & TFF_TRANSMASK) >> TFF_TRANSSHIFT;
      alpha = convert[tr_table];

      if (tr_table == tr_transfir)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive
    }
  else if (state->frame & TFF_SMOKESHADE)
    alpha = 0.5;
  else if (p->flags & (MF_SHADOW | MF_ALTSHADOW))
    alpha = 0.25; // TODO ALTSHADOW reverses src and dest (transposes the transmap)
  else
    alpha = 1.0;
  */

  // hardware renderer part
  oglrenderer->DrawSpriteItem(p->pos, t, flip);


  //CONS_Printf("spritepres_t::Draw: Not yet implemented\n");
  return true;
}
