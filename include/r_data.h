// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.3  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.2  2003/01/12 12:56:42  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.1.1.1  2002/11/16 14:18:26  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Refresh module, data I/O, caching, retrieval of graphics
//      by name.
//
//-----------------------------------------------------------------------------

#ifndef R_DATA_H
#define R_DATA_H 1


#ifdef __GNUG__
#pragma interface
#endif

// Texture definitions in the WAD file. Assumes that int = 32 bits, short = 16 bits

// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
struct mappatch_t
{
  short       originx;
  short       originy;
  short       patch;
  short       stepdir;
  short       colormap;
};


// Texture definition struct in the WAD file.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
struct maptexture_t
{
  char        name[8];
  int         masked;
  short       width;
  short       height;
  void      **columndirectory;      // OBSOLETE
  short       patchcount;
  mappatch_t  patches[1];
};


// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
struct texpatch_t
{
  // Block origin (allways UL),
  // which has allready accounted
  // for the internal origin of the patch.
  int         originx;
  int         originy;
  int         patch;
};

// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.
struct texture_t
{
  // Keep name for switch changing, etc.
  char        name[8];
  short       width;
  short       height;

  // All the patches[patchcount]
  //  are drawn back to front into the cached texture.
  short       patchcount;
  texpatch_t  patches[1];
};


// all loaded and prepared textures from the start of the game
extern texture_t**     textures;

//extern lighttable_t    *colormaps;
extern struct CV_PossibleValue_t Color_cons_t[];

// Load TEXTURE1/TEXTURE2/PNAMES definitions, create lookup tables
void  R_LoadTextures();
void  R_FlushTextureCache();

// Retrieve column data for span blitting.
byte* R_GetColumn(int tex, int col);

byte* R_GetFlat(int  flatnum);

// I/O, setting up the stuff.
void R_InitData();
void R_PrecacheMap();


// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
int R_FlatNumForName(const char *name);


// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
int R_TextureNumForName(const char *name);
int R_CheckTextureNumForName(const char *name);


void R_ClearColormaps();
int R_ColormapNumForName(const char *name);
int R_CreateColormap(char *p1, char *p2, char *p3);
const char *R_ColormapNameForNum(int num);

#endif
