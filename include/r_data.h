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
// Revision 1.14  2004/09/14 21:41:57  hurdler
// rename "data" to "pixels" (I think it's more appropriate and that's how SDL and OpenGL name such data after all)
//
// Revision 1.13  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.12  2004/08/29 20:48:49  smite-meister
// bugfixes. wow.
//
// Revision 1.11  2004/08/18 14:35:20  smite-meister
// PNG support!
//
// Revision 1.10  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.9  2004/08/13 18:25:11  smite-meister
// sw renderer fix
//
// Revision 1.8  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.7  2004/07/07 17:27:19  smite-meister
// bugfixes
//
// Revision 1.6  2004/04/25 16:26:51  smite-meister
// Doxygen
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Texture cache, texture data structures

#ifndef r_data_h
#define r_data_h 1

#include <map>
#include "doomtype.h"
#include "m_fixed.h"
#include "z_cache.h"

class GLTexture;

/// posts are vertical runs of nonmasked source pixels in a patch_t
struct post_t
{
  byte topdelta; ///< how many pixels to skip, -1 (0xff) means the column ends
  byte length;   ///< number of data bytes
  byte crap;     ///< post border, should not be drawn
  byte data[0];  ///< data starts here, ends with another crap byte (not included in length)
};

/// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;



/// flags for drawing Textures
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


/// \brief ABC for all 2D bitmaps.
///
/// Hasta la vista, old texture/flat system!
/// This ABC represents one logical texture at runtime.
/// It is usually built out of patch_t's and/or flats during loading.
/// It may be static, animated, fractal, procedural etc.

class Texture : public cacheitem_t
{
  friend class texturecache_t;
public:
  enum tex_storage_t
  {
    PALETTE         = 0,  ///< 1 byte is the index in the doom palette (as usual)
    INTENSITY       = 1,  ///< 1 byte intensity
    INTENSITY_ALPHA = 2,  ///< 2 byte : alpha then intensity
    RGB24           = 3,  ///< 24 bit rgb
    RGBA32          = 4,  ///< 32 bit rgba
  };

  int    id;  // TODO temp solution, replace with pointers?
  //char   name[9]; TEST

  short width, height;
  short leftoffset, topoffset;
  fixed_t xscale, yscale;

  union
  {
    byte  *pixels;
#ifdef HWRENDER
    GLTexture *gltex;  // for hardware renderer
#endif
  };

protected:
  /// Prepare the texture for use. Returned data layout depends on subclass.
  virtual byte *Generate() = 0;
  virtual void HWR_Prepare() = 0;

public:
  Texture(const char *name);
  virtual ~Texture();

  void *operator new(size_t size);
  void  operator delete(void *mem);

  /// HACK for sw renderer, means that texture is in column_t format
  virtual bool Masked() { return false; };

  /// sw renderer: get raw texture column data for span blitting.
  /// NOTE if Masked() is true, we only return the first post.
  virtual byte *GetColumn(int col) = 0;

  /// sw renderer: masked textures. if Masked() is false, returns NULL
  virtual column_t *GetMaskedColumn(int col) = 0;

  /// returns raw data in row-major form
  virtual byte *GetData() = 0;

  /// draw the Texture flat on screen.
  virtual void Draw(int x, int y, int scrn) {}; // scrn may contain flags
  virtual void HWR_Draw(int x, int y, int flags) {};

  /// tile an area of the screen with the Texture
  virtual void DrawFill(int x, int y, int w, int h) {};
};



/// \brief Row-major Textures which reside in a single lump.
/// raw (flat, fullscreen pic), pic_t, png, jpeg...

class LumpTexture : public Texture
{
public:
  enum type_e
  {
    Raw = 0,
    Pic,
    PNG,
  };

  int    lump;
  byte   type;
  byte   mode;

protected:
  virtual byte *Generate();   ///< returns row-major data
  virtual void HWR_Prepare();

public:
  LumpTexture(const char *name, int lump, int w, int h);

  virtual byte *GetColumn(int col);
  virtual column_t *GetMaskedColumn(int col) { return NULL; }
  virtual byte *GetData() { return Generate(); }
  virtual void Draw(int x, int y, int scrn);
  virtual void HWR_Draw(int x, int y, int flags);
  virtual void DrawFill(int x, int y, int w, int h);
};




/// \brief Class for PNG Textures
class PNGTexture : public LumpTexture
{
protected:
  byte *ReadData(bool read_image);

  virtual byte *Generate();   ///< returns row-major data
  virtual void HWR_Prepare();

public:
  PNGTexture(const char *name, int lump);
};




/// \brief Class for patch_t's.
class PatchTexture : public Texture
{
  int lump;

protected:
  virtual byte *Generate();  ///<
  virtual void HWR_Prepare();

public:
  PatchTexture(const char *name, int lump);

  virtual bool Masked() { return true; };
  virtual byte *GetColumn(int col);
  virtual column_t *GetMaskedColumn(int col);
  virtual byte *GetData();
  virtual void Draw(int x, int y, int scrn);
  virtual void HWR_Draw(int x, int y, int flags);
};



/// \brief Doom Textures (which are built out of patches)
///
/// All the patches are drawn back to front into the cached texture.
/// In SW mode, the texture data is stored in column-major order (like patches)

class DoomTexture : public Texture
{
public:
  // A single patch from a texture definition,
  // basically a rectangular area within the texture rectangle.
  struct texpatch_t
  {
    /// Block origin (always UL), which has already accounted for the internal origin of the patch.
    int originx, originy;
    /// lump number for the patch
    int patchlump;
  };

  int         widthmask;

  short       patchcount; ///< number of patches in the texture
  texpatch_t *patches;    ///< array for the patch definitions

  int        *columnofs; ///< offsets from texdata to raw column data
  byte       *texdata;   ///< texture data

protected:
  virtual byte *Generate();
  virtual void HWR_Prepare();

public:
  DoomTexture(const struct maptexture_t *mtex);
  virtual ~DoomTexture();

  virtual bool Masked() { return (patchcount == 1); };
  virtual byte *GetColumn(int col);
  virtual column_t *GetMaskedColumn(int col);
  virtual byte *GetData();
};



//===============================================================================

/// \brief Second-level cache for Textures
///
/// There are two ways to add Texture definitions to the cache:
/// Cache("name") (->Load("name"))creates a texture from a lump (flats...)
/// Insert(Texture*) inserts a finished texture to cache (anims, doomtextures)

class texturecache_t : public cache_t
{
protected:
  /// generates a Texture from a single data lump
  cacheitem_t *Load(const char *p);

  /// mapping from Texture id's to pointers
  map<unsigned, Texture *> texture_ids;

  /// sw renderer: colormaps for palette conversions (one for each resource file)
  vector<byte *> palette_conversion;

public:
  texturecache_t(memtag_t tag);

  /// empties the cache, deletes all Textures
  void Clear();

  /// insert a special Texture into the cache (animation, DoomTexture...)
  void Insert(class Texture *t);

  /// returns the id of an existing Texture, or tries Caching it if nonexistant
  inline int Get(const char *p, bool substitute = true)
  {
    Texture *t = GetPtr(p, substitute);
    return t ? t->id : 0;
  };

  /// like Get, but returns a pointer
  Texture *GetPtr(const char *p, bool substitute = true);

  /// like GetPtr, but takes a lump number instead of a name.
  Texture *GetPtrNum(int n);

  /// returns the Texture with the corresponding id
  Texture *operator[](unsigned id);

  /// First checks if the lump is a valid colormap (or transmap). If not, acts like Get.
  int GetTextureOrColormap(const char *name, int &colmap, bool transmap = false);

  /// reads the PNAMES and TEXTUREn lumps, generates the corresponding Textures
  int ReadTextures();

  /// creates the palette conversion colormaps
  void InitPaletteConversion();

  /// returns the pal. conversion colormap for the given file
  inline byte *GetPalConv(int i) { return palette_conversion[i]; }
};


extern texturecache_t tc;


byte NearestColor(byte r, byte g, byte b);

// initializes the part of the renderer that even a dedicated server needs
void R_ServerInit();

// colormap management
void R_InitColormaps();
void R_ClearColormaps();
int  R_ColormapNumForName(const char *name);
const char *R_ColormapNameForNum(int num);
int  R_CreateColormap(char *p1, char *p2, char *p3);

#endif
