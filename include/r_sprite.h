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
// Revision 1.2  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
//
// DESCRIPTION:
//   Sprite and sprite skin definitions
//-----------------------------------------------------------------------------


#ifndef r_sprite_h
#define r_sprite_h 1

#include "doomtype.h"


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
