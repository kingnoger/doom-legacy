// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.5  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.4  2003/11/23 19:07:42  smite-meister
// New startup order
//
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
//   Texture cache, texture data structures
//
//-----------------------------------------------------------------------------

#ifndef r_data_h
#define r_data_h 1

#include "doomtype.h"
#include "z_cache.h"


// flags for drawing Textures
enum texture_draw_e
{
  V_SLOC     =  0x10000,   // scale starting location
  V_SSIZE    =  0x20000,   // scale size
  V_FLIPX    =  0x40000,   // mirror the patch in the vertical direction
  V_MAP      =  0x80000,   // use a colormap
  V_WHITEMAP = 0x100000,   // white colormap (for V_DrawString)
  V_TL       = 0x200000,   // translucency using transmaps

  V_SCALE = V_SLOC | V_SSIZE,

  V_FLAGMASK = 0xFFFF0000,
  V_SCREENMASK = 0xF
};



// Hasta la vista, old texture/flat system!
// This ABC represents one logical texture at runtime.
// It is usually built out of patch_t's and/or flats during loading.
// It may be static, animated, fractal, procedural etc.

class Texture : public cacheitem_t
{
  friend class texturecache_t;
public:
  enum tex_storage_t
  {
    PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
    INTENSITY       = 1,  // 1 byte intensity
    INTENSITY_ALPHA = 2,  // 2 byte : alpha then intensity
    RGB24           = 3,  // 24 bit rgb
    RGBA32          = 4,  // 32 bit rgba
  };

  int    id;  // TODO temp solution, replace with pointers?
  char   name[9];

  short width, height;
  short leftoffset, topoffset;
  union
  {
    byte  *data;
    struct Mipmap_t *mipmap; // for hardware renderer
  };

public:
  Texture(const char *name);
  ~Texture();

  void *operator new(size_t size);
  void  operator delete(void *mem);

  virtual bool Masked() { return false; }; // HACK for sw renderer

  virtual byte *Generate() = 0;         // get row-major data
  virtual byte *GetColumn(int col) = 0; // get texture column data for span blitting.
  virtual void HWR_Prepare() = 0;       // prepare the texture for hw use
  virtual void Draw(int x, int y, int scrn) {}; // scrn may contain flags
  virtual void HWR_Draw(int x, int y, int flags) {};
};



// Class for row-major textures which reside in a single lump.
// flat, raw, pic_t, png, jpeg...
class LumpTexture : public Texture
{
public:
  enum type_e
  {
    Raw = 0,
    Pic,
  };

  int    lump;
  byte   type;
  byte   mode;

public:
  LumpTexture(const char *name, int lump, int w, int h);

  virtual byte *Generate();
  virtual byte *GetColumn(int col);
  virtual void HWR_Prepare();
  virtual void Draw(int x, int y, int scrn);
  virtual void HWR_Draw(int x, int y, int flags);
};



// Class for patch_t's.
class PatchTexture : public Texture
{
  int lump;

public:
  PatchTexture(const char *name, int lump);


  virtual bool Masked() { return true; };

  virtual byte *Generate();
  virtual byte *GetColumn(int col);
  virtual void HWR_Prepare();
  virtual void Draw(int x, int y, int scrn);
  virtual void HWR_Draw(int x, int y, int flags);
};



// class for Doom textures (which are build out of patches)
class DoomTexture : public Texture
{
public:
  // A single patch from a texture definition,
  //  basically a rectangular area within
  //  the texture rectangle.
  struct texpatch_t
  {
    // Block origin (always UL), which has already accounted
    // for the internal origin of the patch.
    int originx, originy;
    int patch;
  };

  int         widthmask;

  // All the patches[patchcount]
  //  are drawn back to front into the cached texture.
  short       patchcount;
  texpatch_t *patches;

  unsigned   *columnofs;  // offsets to
  byte       *texdata;

public:
  DoomTexture(const struct maptexture_t *mtex);

  virtual bool Masked() { return (patchcount == 1); };

  virtual byte *Generate();
  virtual byte *GetColumn(int col); // Retrieve texture column data for span blitting.
  virtual void HWR_Prepare();
};




class texturecache_t : public L2cache_t
{
protected:
  cacheitem_t *Load(const char *p);

public:
  texturecache_t(memtag_t tag);

  void Clear();
  void Insert(class Texture *t); // insert a special texture into the cache (animation, doomtexture...)
  int Get(const char *p, bool substitute = true);
  Texture *GetPtr(const char *p, bool substitute = true);
  Texture *GetPtrNum(int n);

  int ReadTextures();
};



extern texturecache_t tc;


// I/O, setting up the stuff.
void R_InitData();

// TODO temp solution for handles
Texture *R_GetTexture(unsigned handle);


// colormap management
void R_ClearColormaps();
int  R_ColormapNumForName(const char *name);
int  R_CreateColormap(char *p1, char *p2, char *p3);
const char *R_ColormapNameForNum(int num);

#endif
