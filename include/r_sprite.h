// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.7  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.6  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.5  2003/03/08 16:07:16  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.4  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
//
// DESCRIPTION:
//   Sprite and sprite skin definitions
//-----------------------------------------------------------------------------


#ifndef r_sprite_h
#define r_sprite_h 1

#include <vector>
#include "doomtype.h"
#include "z_cache.h"

/*
  Doom "sequences"
  spawnstate  // stand idle
  seestate    // run
  painstate   // pain
  meleestate   // melee attack (1)
  missilestate // missile attack (2)

  deathstate   // die (1)
  xdeathstate  // explode, die (2)
  crashstate   // one more way to die (3) (heretic/hexen imps)

  raisestate   // being raised by an arch-vile (extra 1)
 */

/*
MD2 sequences?

IDLE1         The first of four idle animations
RUN           Animation for the model running. RUN FORREST RUN!
SHOT_STAND    Animation for when the model gets shot, but stays standing
SHOT_SHOULDER Animation for when the model gets shot in the shoulder (still standing though)
JUMP          Animation for the model jumping
IDLE2         The second of four idle animations
SHOT_FALLDOWN Animation for the model getting shot, and falling to the ground (used for getting shot by big weapons)
IDLE3         The third of four idle animations
IDLE4         The fourth of four idle animations
CROUCH        Animation for making the model crouch
CROUCH_CRAWL  Having the model crawl while crouching
CROUCH_IDLE   An idle animation while in a crouching position
CROUCH_DEATH  The model dying while in a crouching position
DEATH_FALLBACK     The model dying while falling backwards (death shot from the front)
DEATH_FALLFORWARD  The model dying while falling forwards (death shot from the back)
DEATH_FALLBACKSLOW The model dying while falling backwards slowly
*/


enum MD3_animseq_e
{
  BOTH_DEATH1 = 0,
  BOTH_DEAD1  = 1,
  BOTH_DEATH2 = 2,
  BOTH_DEAD2  = 3,
  BOTH_DEATH3 = 4,
  BOTH_DEAD3  = 5,

  TORSO_GESTURE = 6,
  TORSO_ATTACK  = 7,
  TORSO_ATTACK2 = 8,
  TORSO_DROP    = 9,
  TORSO_RAISE   = 10,
  TORSO_STAND   = 11,
  TORSO_STAND2  = 12,

  LEGS_WALKCR   = 13,
  LEGS_WALK     = 14,
  LEGS_RUN      = 15,
  LEGS_BACK     = 16,
  LEGS_SWIM     = 17,
  LEGS_JUMP     = 18,
  LEGS_LAND     = 19,
  LEGS_JUMPB    = 20,
  LEGS_LANDB    = 21,
  LEGS_IDLE     = 22,
  LEGS_IDLECR   = 23,
  LEGS_TURN     = 24,
  MAX_ANIMATIONS = 25
};


//================================
//           Sprites
//================================

// Doom sprites in wads are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0

struct spriteframe_t
{
  // If false use 0 for any position.
  // Note: as eight entries are available,
  //  we might as well insert the same name eight times.
  char   rotate;

  // Lump to use for view angles 0-7.
  int    lumppat[8];   // lump number 16:16 wad:lump
  short  lumpid[8];    // id in the spriteoffset,spritewidth.. tables

  bool   flip[8];   // Flip bit to use for view angles 0-7.
};


class sprite_t : public cacheitem_t
{
  friend class spritecache_t;
  friend class spritepres_t;
  //protected: // FIXME some old code wants to use spriteframes directly. Fix the old code...
public:
  int  iname; // sprite name (4 chars) as an int
  int            numframes;
  spriteframe_t *spriteframes;
};


class spritecache_t : public L2cache_t
{
protected:
  virtual cacheitem_t *Load(const char *p, cacheitem_t *t = NULL);
  virtual void Free(cacheitem_t *t);
public:
  spritecache_t(memtag_t tag);

  inline sprite_t *Get(const char *p) { return (sprite_t *)Cache(p); };
};


extern spritecache_t sprites;

extern int numspritelumps;


//================================
//        Presentations
//================================

class Actor;

// Idea: Game entities have a pointer to a graphic presentation.
// The animation data is stored in the presentation object.
// The actual implementation of the "graphic presentation" can be a sprite, md3 or anything.

// abstract base class
class presentation_t
{
protected:
  //vector<animation_t> anim; // all known animation sequences
public:
  char color;
  int  frame; // temp solution, for flags

  virtual ~presentation_t() = 0;

  virtual bool IsSprite() {return false;}; // damn Doom sprites.

  virtual void SetFrame(int fr) = 0; // bad
  virtual void SetAnim(int seq) = 0; // good

  // this is hopefully a temporary hack.. it generates a vissprite
  virtual void Project(Actor *p) = 0;
  virtual bool Draw(const Actor *p) = 0;

  void *operator new(size_t size);
  void  operator delete(void *mem);
};


class spritepres_t : public presentation_t
{
  //protected:
public: // for sw renderer, FIXME
  sprite_t *spr;

public:
  spritepres_t(const char *name, int startframe, int col);
  virtual ~spritepres_t();

  virtual bool IsSprite() {return true;}; 
  virtual void SetFrame(int fr);
  virtual void SetAnim(int seq);
  virtual void Project(Actor *p);
  virtual bool Draw(const Actor *p);
};


struct MD3_animstate
{
  MD3_animseq_e seq; // current animation sequence
  float interp;      // [0, 1) interpolation phase between frame and nextframe
  int   frame, nextframe;
};


class modelpres_t : public presentation_t
{
  class MD3_player *mdl;

  fixed_t lastupdate; // time it was last updated
  MD3_animstate st[3]; // legs, torso, head

public:

  modelpres_t(const char *mname, int col = 0, const char *skin = "default");
  virtual ~modelpres_t();

  virtual void SetFrame(int fr);
  virtual void SetAnim(int seq);
  virtual void Project(Actor *p);
  virtual bool Draw(const Actor *p);
};




// -----------
// SKINS STUFF
// -----------
#define SKINNAMESIZE 16
#define DEFAULTSKIN  "marine"   // name of the standard doom marine as skin
#define MAXSKINS 10

// 10 customisable sounds for Skins
typedef enum {
  SKSPLPAIN,
  SKSSLOP,
  SKSOOF,
  SKSPLDETH,
  SKSPDIEHI,
  SKSNOWAY,
  SKSPUNCH,
  SKSRADIO,
  SKSJUMP,
  SKSOUCH,
  NUMSKINSOUNDS
} skinsound_t;

struct skin_t
{
  char        name[SKINNAMESIZE+1];   // short descriptive name of the skin
  sprite_t    spritedef;
  char        faceprefix[4];          // 3 chars+'\0', default is "STF"

  // specific sounds per skin
  short       soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table
};

struct consvar_t;

extern int       numskins;
extern skin_t    skins[MAXSKINS+1];
//extern CV_PossibleValue_t skin_cons_t[MAXSKINS+1];
extern consvar_t cv_skin;

//void    R_InitSkins (void);
void    SetPlayerSkin(int playernum,char *skinname);
int     R_SkinAvailable(char* name);
void    R_AddSkins(int wadnum);


#endif
