// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
/// \brief Sprites and sprite skins.

#ifndef r_sprite_h
#define r_sprite_h 1

#include <vector>
#include "doomtype.h"
#include "z_cache.h"


//========================================================
//                       Sprites
//========================================================

/// \brief One frame of a sprite_t
///
/// A sprite is a set of Materials that represent
/// a three dimensional object seen from different angles.
/// Doom sprites in wads are patches with a special naming convention
/// so they can be recognized by R_InitSprites.
/// The base name is NNNNFx or NNNNFxFx, with x indicating the rotation, x = 0--7.
/// Horizontal flipping is used to save space, thus NNNNF2F5 defines a mirrored patch.
/// Some sprites only have one picture used for all directions: NNNNF0
struct spriteframe_t
{
  // If false use 0 for any position.
  // NOTE: as eight entries are available,
  //  we might as well insert the same name eight times.
  char   rotate;

  class Material *tex[8]; ///< Material to use for view angles 0-7.
  bool           flip[8]; ///< Flip bit to use for view angles 0-7.
};



/// \brief An animated collection of 2D frames
class sprite_t : public cacheitem_t
{
  friend class spritecache_t;
  friend class spritepres_t;
  //protected: // FIXME R_DrawPSprite() wants to use spriteframes directly.
public:
  sprite_t(const char *name);
  virtual ~sprite_t();

  Sint32  iname;  ///< sprite name (4 chars) as an int
  int            numframes;
  spriteframe_t *spriteframes;
};



/// \brief Cache for sprite_t's.
class spritecache_t : public cache_t<sprite_t>
{
protected:
  virtual sprite_t *Load(const char *name);
};

extern spritecache_t sprites;


//========================================================
//                     Presentations
//========================================================

/// \brief Abstract base class for presentation objects
///
/// Idea: Game entities have a pointer to a graphic presentation.
/// The animation data is stored in the presentation object.
/// The actual implementation of the "graphic presentation" can be a sprite, MD3 or anything.
class presentation_t
{
protected:
  //vector<animation_t> anim; // all known animation sequences
public:
  enum animseq_e
  {
    // Legacy "animation sequences"
    Idle = 0, ///< spawnstate
    Run,      ///< seestate
    Back,     ///< walk backwards (used only with players)
    LAST_LOOPING = Back,
    Pain,     ///< painstate
    Melee,    ///< meleestate (attack 1)
    Shoot,    ///< missilestate (attack 2)
    
    Death1,   ///< deathstate
    Death2,   ///< xdeathstate (explode, die)
    Death3,   ///< crashstate, one more way to die (heretic/hexen imps)

    Raise     ///< raisestate, being raised from death by an arch-vile
  };

  char  color;       ///< Skin colormap.
  animseq_e animseq; ///< Current animation sequence.
  int   flags;       ///< Effects. Translucency, fullbright etc.
  int   lastupdate;  ///< Time of last update in tics.

  virtual ~presentation_t() {};

  virtual void SetFrame(const struct state_t *st) = 0; // Only used by DActors with sprites
  virtual void SetAnim(animseq_e seq) = 0;  // starts a requested animation sequence
  int GetAnim() { return animseq; };

  virtual bool Update(int nowtic)      = 0; // Updates the animation, called before drawing
  virtual void Project(class Actor *p) = 0; ///< Drawing in SW renderer. Generates a vissprite_t.
  virtual bool Draw(const Actor *p) = 0; ///< Drawing in OpenGL.
  virtual spriteframe_t *GetFrame() { return NULL; }; // Menu uses this.
  virtual int Marshal(class LArchive &a) = 0;

  void *operator new(size_t size);
  void  operator delete(void *mem);

  static int Serialize(presentation_t *p, LArchive &a);
  static presentation_t *Unserialize(LArchive &a);

  /// Netcode
  virtual void   Pack(class TNL::BitStream *s) = 0;
  virtual void Unpack(class TNL::BitStream *s) = 0;
  virtual void   PackAnim(class TNL::BitStream *s) = 0;
  virtual void UnpackAnim(class TNL::BitStream *s) = 0;
};


/// \brief Sprite presentation
///
/// Sprites can be animated in two ways, either using SetAnim or SetFrame.
/// Both use the states table. A sprite presentation is defined by an ActorInfo.
class spritepres_t : public presentation_t
{
protected:
  sprite_t *spr;
  const class ActorInfo *info; ///< this is used to know which sequence corresponds to which state
  const state_t *state; ///< animation frames are tied to the states table

public:
  spritepres_t() {}; ///< simple constructor for unserialization
  spritepres_t(const ActorInfo *inf, int col = 0); ///< normal constructor
  spritepres_t(class TNL::BitStream *s);
  virtual ~spritepres_t();

  virtual void SetFrame(const state_t *st); // Only used by DActors with sprites
  virtual void SetAnim(animseq_e seq);

  virtual bool Update(int nowtic);
  virtual void Project(Actor *p);
  virtual bool Draw(const Actor *p);
  virtual spriteframe_t *GetFrame();
  virtual int  Marshal(LArchive &a);

  /// Netcode
  virtual void   Pack(class TNL::BitStream *s);
  virtual void Unpack(class TNL::BitStream *s) {}
  virtual void   PackAnim(class TNL::BitStream *s);
  virtual void UnpackAnim(class TNL::BitStream *s);
};



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


/// \brief Animation state for an MD3 model.
struct MD3_animstate
{
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

  int   seq;    ///< current animation sequence
  float interp; ///< [0, 1) interpolation phase between frame and nextframe
  int   frame, nextframe; ///< current and next animation frames

  /// advance the animation sequence by time dt using definitions 'anim'
  void Advance(struct MD3_anim *anim, float dt);
};


/// \brief MD3 model presentation
class modelpres_t : public presentation_t
{
  class MD3_player *mdl; ///< MD3 player model to be used
  MD3_animstate st[3];   ///< animation states for legs, torso, head

public:
  modelpres_t() {}; ///< simple constructor for unserialization
  modelpres_t(const char *mname, int col = 0, const char *skin = "default");
  virtual ~modelpres_t();

  virtual void SetFrame(const state_t *st) {}; // do nothing
  virtual void SetAnim(animseq_e seq);

  virtual bool Update(int nowtic);
  virtual void Project(Actor *p);
  virtual bool Draw(const Actor *p);
  virtual int  Marshal(LArchive &a);

  /// Netcode
  virtual void   Pack(class TNL::BitStream *s);
  virtual void Unpack(class TNL::BitStream *s);
  virtual void   PackAnim(class TNL::BitStream *s);
  virtual void UnpackAnim(class TNL::BitStream *s);
};



//========================================================
//                    Sprite skins
//========================================================

#define SKINNAMESIZE 16
#define MAXSKINS 10


struct skin_t
{
  // 10 customisable sounds for Skins
  enum skinsound_t
  {
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
  };

  char        name[SKINNAMESIZE+1];   // short descriptive name of the skin
  sprite_t   *spritedef;
  char        faceprefix[4];          // 3 chars+'\0', default is "STF"

  // specific sounds per skin
  short       soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table
};


extern int       numskins;
extern skin_t    skins[MAXSKINS+1];


void    SetPlayerSkin(int playernum,char *skinname);
int     R_SkinAvailable(char* name);
void    R_AddSkins(int wadnum);

#endif
