// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2002 by DooM Legacy Team.
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
/*
Quake III player sequences
enum
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
*/

/*
// this struct describes one animation sequence
struct animation_t
{
  int firstframe; // lastframe = firstframe + numframes - 1
  int numframes;
  int loopingframes;
  fixed_t fps; // frames per second (not always used)
  // (in principle each frame can have its own duration
  //  independently of others in the sequence)
};

// Idea: Game entities have a pointer to a graphic presentation, frame, nextframe,
// current sequence and location/rotation information stored in them.
// The actual implementation of the "graphic presentation" can be a sprite, md3 or anything.

// abstract base class
class graph_presentation_t
{
protected:
  vector<animation_t> anim; // all known animation sequences
  
public:

  void DrawInterpolated(fixed_t frame, int nextframe, pos, rot) = 0;
};

class MD3 : public graph_presentation_t
{
  
};
*/


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
  bool   rotate;

  // Lump to use for view angles 0-7.
  int    lumppat[8];   // lump number 16:16 wad:lump
  short  lumpid[8];    // id in the spriteoffset,spritewidth.. tables

  // Flip bit (1 = flip) to use for view angles 0-7.
  byte   flip[8];
};


//
// A sprite definition:  a number of animation frames.
//
struct spritedef_t
{
  int            numframes;
  spriteframe_t *spriteframes;
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
  spritedef_t spritedef;
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
