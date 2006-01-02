// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// $Log$
// Revision 1.25  2006/01/02 17:02:30  smite-meister
// small fixes
//
// Revision 1.24  2005/11/06 19:30:36  smite-meister
// ntexture
//
// Revision 1.22  2005/09/29 15:35:27  smite-meister
// JDS texture standard
//
// Revision 1.21  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.20  2005/07/20 20:27:23  smite-meister
// adv. texture cache
//
// Revision 1.19  2005/06/30 18:16:58  smite-meister
// texture anims fixed
//
// Revision 1.18  2005/01/04 18:32:44  smite-meister
// better colormap handling
//
// Revision 1.17  2004/12/08 10:16:03  segabor
// "segabor: byte alignment fix"
//
// Revision 1.16  2004/10/31 22:24:53  smite-meister
// pic_t moves into history
//
// Revision 1.15  2004/09/23 23:21:19  smite-meister
// HUD updated
//
// Revision 1.14  2004/09/14 21:41:57  hurdler
// rename "data" to "pixels" (I think it's more appropriate and that's how SDL and OpenGL name such data after all)
//
// Revision 1.13  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.11  2004/08/18 14:35:20  smite-meister
// PNG support!
//
// Revision 1.10  2004/08/15 18:08:29  smite-meister
// palette-to-palette colormaps etc.
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


/// \brief patch_t, the strange Doom graphics format.
///
/// A patch holds one or more columns, which consist of posts separated by holes.
/// A post is a vertical run of pixels.
/// Patches are used for sprites and all masked pictures,
/// and we compose textures from the TEXTURE1/2 lists of patches.
struct patch_t
{
  Uint16 width;         /// bounding box size
  Uint16 height;
  Sint16 leftoffset;    /// pixels to the left of origin
  Sint16 topoffset;     /// pixels below the origin
  Uint32 columnofs[0];  /// [width] byte offsets to the columns
} __attribute__((packed));


/// posts are vertical runs of nonmasked source pixels in a patch_t
struct post_t
{
  byte topdelta; ///< how many pixels to skip, -1 (0xff) means the column ends
  byte length;   ///< number of data bytes
  byte crap;     ///< post border, should not be drawn
  byte data[0];  ///< data starts here, ends with another crap byte (not included in length)
} __attribute__((packed));


/// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;



/// \brief pic_t, a graphics format native to Legacy. Deprecated.
///
/// a pic is an unmasked block of pixels, stored in horizontal way
/*
struct pic_t
{
  enum pic_mode_t
  {
    PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
    INTENSITY       = 1,  // 1 byte intensity
    INTENSITY_ALPHA = 2,  // 2 byte : alpha then intensity
    RGB24           = 3,  // 24 bit rgb
    RGBA32          = 4,  // 32 bit rgba
  };

  short  width;
  byte   zero;   // set to 0 allow autodetection of pic_t 
                   // mode instead of patch or raw
  byte   mode;   // see pic_mode_t above
  short  height;
  short  reserved1;  // set to 0
  byte   data[0];
};
*/


//===============================================================================

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
  int     id;  ///< unique texture ID  TODO temp solution, replace with pointers?
  short   width, height;          ///< bitmap dimensions in texels
  short   leftoffset, topoffset;
  fixed_t xscale, yscale;         ///< texel-size / world-size
  byte    w_bits, h_bits;         ///< largest power-of-two sizes <= actual bitmap size

  union
  {
    byte  *pixels; ///< raw bitmap data in column-major order for sw renderer
#ifdef HWRENDER
    class GLTexture *gltex;  // for hardware renderer
#endif
  };

protected:
  /// Prepare the texture for use.
  virtual void HWR_Prepare() = 0;

  /// To be called after bitmap size is known
  inline void Initialize()
  {
    for (w_bits = 0; 1 << (w_bits+1) <= width;  w_bits++);
    for (h_bits = 0; 1 << (h_bits+1) <= height; h_bits++);
  }

public:
  Texture(const char *name);
  virtual ~Texture();

  void *operator new(size_t size);
  void  operator delete(void *mem);

  /// sw renderer: True means that texture is available in column_t format using GetMaskedColumn
  virtual bool Masked() { return false; };

  /// sw renderer: Get masked column data. if Masked() is false, returns NULL
  virtual column_t *GetMaskedColumn(int col) = 0;

  /// sw renderer: Get raw unmasked texture column data.
  virtual byte *GetColumn(int col) = 0;

  /// Get raw column-major texture data.
  virtual byte *GetData() = 0;

  /// draw the Texture flat on screen.
  virtual void Draw(int x, int y, int scrn) {}; // scrn may contain flags
  virtual void HWR_Draw(int x, int y, int flags) {};

  /// tile an area of the screen with the Texture
  virtual void DrawFill(int x, int y, int w, int h) {};
};



/// \brief Textures which reside in a single lump.
/// This class handles raw pages and Doom flats, derived classes handle png and jpeg,
/// ideally by redefining the constructor and the function Generate() only.
class LumpTexture : public Texture
{
public:
  int    lump;

protected:
  virtual byte *Generate();         ///< subclasses should redefine this
  virtual void HWR_Prepare();

public:
  LumpTexture(const char *name, int lump, int w, int h);

  virtual column_t *GetMaskedColumn(int col) { return NULL; }
  virtual byte *GetColumn(int col);
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

  virtual byte *Generate();
  virtual void HWR_Prepare();

public:
  PNGTexture(const char *name, int lump);
};




/// \brief Class for patch_t's.
///
/// This is trickier than a LumpTexture, because the data could be required in two different formats:
/// patch_t (masked) and raw.
class PatchTexture : public Texture
{
  int   lump;
  byte *patch_data; ///< texture in patch_t format, pixels has it in raw column-major format

protected:
  patch_t *GeneratePatch();
  byte    *GenerateData();
  virtual void HWR_Prepare();

public:
  PatchTexture(const char *name, int lump);
  virtual ~PatchTexture();

  virtual bool Masked() { return true; };
  virtual column_t *GetMaskedColumn(int col);
  virtual byte *GetColumn(int col);
  virtual byte *GetData() { return GenerateData(); }

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

  short       patchcount; ///< number of patches in the texture
  texpatch_t *patches;    ///< array for the patch definitions

  Uint32     *columnofs;   ///< offsets from texdata to raw column data
  byte       *patch_data;  ///< texture data in patch_t format
  //byte       *bitmap_data; ///< texture data in raw column-major format

protected:
  short       widthmask;  ///<  (1 << w_bits) - 1

protected:
  patch_t *GeneratePatch();
  byte    *GenerateData();
  virtual void HWR_Prepare();

public:
  DoomTexture(const struct maptexture_t *mtex);
  virtual ~DoomTexture();

  virtual bool Masked() { return (patchcount == 1); };
  virtual column_t *GetMaskedColumn(int col);
  virtual byte *GetColumn(int col);
  virtual byte *GetData() { return GenerateData(); }
};



//===============================================================================

/// Texture lookup orders
enum texture_class_t
{
  TEX_wall = 0,
  TEX_floor,
  TEX_sprite,
  TEX_lod
};


/// \brief Second-level cache for Textures
///
/// There are two ways to add Texture definitions to the cache:
/// Insert(Texture*) inserts a finished texture to cache (anims, doomtextures)
/// Cache("name") (->Load("name")) tries to create a texture from a lump if it is not already found in cache

class texturecache_t
{
protected:
  cachesource_t new_tex;    ///< advanced textures, TX_START
  cachesource_t doom_tex;   ///< TEXTUREx/PNAMES
  cachesource_t flat_tex;   ///< F_START
  cachesource_t sprite_tex; ///< advanced spritetextures, S_START
  cachesource_t lod_tex;    ///< load-on-demand textures, mostly for misc. graphics

  memtag_t      tagtype;      ///< memory tag used for the cached data
  Texture      *default_item; ///< the default data item

  /// Mapping from Texture id's to pointers.
  std::map<unsigned, Texture *> texture_ids;

  /// sw renderer: colormaps for palette conversions (one for each resource file)
  std::vector<byte *> palette_conversion;

  /// inserts a Texture into the given source
  bool Insert(Texture *t, cachesource_t &s, bool keep_old = false);

  /// Creates a Texture from the lump, inserts it to the given source.
  bool BuildLumpTexture(int lump, bool h_start, cachesource_t &source);

public:
  texturecache_t(memtag_t tag);

  /// sets the default Texture
  void SetDefaultItem(const char *name);

  /// empties the cache, deletes all Textures
  void Clear();

  /// generates a Texture from a single data lump
  static Texture *Load(const char *p);

  /// Insert a Texture into the new_tex source, used by NTEXTURE parser.
  inline void InsertTexture(Texture *t) { Insert(t, new_tex, false); };

  /// Insert a Texture into the sprite_tex source, used by NTEXTURE parser.
  inline void InsertSprite(Texture *t) { Insert(t, sprite_tex, false); };

  /// Insert a Texture into the flat source, used by animated textures.
  /// Pointers to original Textures are preserved in the master animation.
  inline void InsertFlat(Texture *t) { Insert(t, flat_tex, true); };

  /// Insert a Texture into the doomtex source, used by animated textures.
  /// Pointers to original Textures are preserved in the master animation.
  inline void InsertDoomTex(Texture *t) { Insert(t, doom_tex, true); };

  /// Returns pointer to an existing Texture, or tries creating it if nonexistant.
  Texture *GetPtr(const char *name, texture_class_t mode = TEX_lod);

  /// like GetPtr, but takes a lump number instead of a name. For TEX_lod ONLY!
  Texture *GetPtrNum(int n);

  /// Returns the id of an existing Texture, or tries creating it if nonexistant.
  inline int GetID(const char *name, texture_class_t mode = TEX_wall)
  {
    Texture *t = GetPtr(name, mode);
    return t ? t->id : 0;
  };

  /// For animated textures only
  inline int GetNoSubstitute(const char *p, texture_class_t mode)
  {
    Texture *t = GetPtr(p, mode);
    if (t == default_item)
      return 0;

    return t ? t->id : 0;
  };

  /// First checks if the lump is a valid colormap (or transmap). If not, acts like GetID(name, TEX_wall);
  int GetTextureOrColormap(const char *name, class fadetable_t*& cmap);
  int GetTextureOrTransmap(const char *name, int& map_num);

  /// returns the Texture with the corresponding id
  Texture *operator[](unsigned id);

  /// Reads the TEXTUREn/PNAMES lumps and F_START lists, generates the corresponding Textures.
  int ReadTextures();

  /// creates the palette conversion colormaps
  void InitPaletteConversion();

  /// returns the palette conversion colormap for the given file
  inline byte *GetPaletteConv(int i) { return palette_conversion[i]; }
};


extern texturecache_t tc;


/// \brief Set of 34 lightlevel colormaps from COLORMAP lump, also called a fadetable.
///
/// Used also for "colored lighting" colormaps from Boom etc.
class fadetable_t
{
  typedef byte lighttable_t;
public:
  int             lump; ///< the lump number of the colormap
  lighttable_t   *colormap;

  // GL emulation for the fadetable
  unsigned short  maskcolor;
  unsigned short  fadecolor;
  double          maskamt;
  unsigned short  fadestart, fadeend;
  int             fog;

  //Hurdler: rgba is used in hw mode for coloured sector lighting
  int             rgba; // similar to maskcolor in sw mode

public:
  fadetable_t();
  fadetable_t(int lumpnum);
  ~fadetable_t();
};



// Quantizes an RGB color into current palette
byte NearestColor(byte r, byte g, byte b);

// initializes the part of the renderer that even a dedicated server needs
void R_ServerInit();

// colormap management
void R_InitColormaps();
void R_ClearColormaps();
fadetable_t *R_GetFadetable(unsigned n);
fadetable_t *R_FindColormap(const char *name);
fadetable_t *R_CreateColormap(char *p1, char *p2, char *p3);
const char *R_ColormapNameForNum(const fadetable_t *p);

#endif
