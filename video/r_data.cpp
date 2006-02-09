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
// $Log$
// Revision 1.53  2006/02/09 22:36:29  jussip
// Wall texturing works better now.
//
// Revision 1.52  2006/02/08 19:09:27  jussip
// Added beginnings of a new OpenGL renderer.
//
// Revision 1.51  2005/11/06 19:35:14  smite-meister
// ntexture
//
// Revision 1.50  2005/10/07 20:04:22  smite-meister
// sprite scaling
//
// Revision 1.48  2005/09/29 15:35:27  smite-meister
// JDS texture standard
//
// Revision 1.47  2005/09/12 18:33:45  smite-meister
// fixed_t, vec_t
//
// Revision 1.46  2005/07/20 20:27:23  smite-meister
// adv. texture cache
//
// Revision 1.45  2005/06/30 18:16:58  smite-meister
// texture anims fixed
//
// Revision 1.42  2005/01/04 18:32:45  smite-meister
// better colormap handling
//
// Revision 1.37  2004/11/28 18:02:24  smite-meister
// RPCs finally work!
//
// Revision 1.35  2004/11/09 20:38:53  smite-meister
// added packing to I/O structs
//
// Revision 1.33  2004/10/31 22:22:13  smite-meister
// Hasta la vista, pic_t!
//
// Revision 1.31  2004/10/14 19:35:52  smite-meister
// automap, bbox_t
//
// Revision 1.29  2004/09/14 21:41:57  hurdler
// rename "data" to "pixels" (I think it's more appropriate and that's how SDL and OpenGL name such data after all)
//
// Revision 1.28  2004/09/03 16:28:52  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.25  2004/08/18 14:35:23  smite-meister
// PNG support!
//
// Revision 1.23  2004/08/15 18:08:30  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.20  2004/07/25 20:16:43  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.18  2004/07/05 16:53:30  smite-meister
// Netcode replaced
//
// Revision 1.17  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.16  2004/04/17 12:53:42  hurdler
// now compile with gcc 3.3.3 under Linux
//
// Revision 1.15  2004/04/01 09:16:16  smite-meister
// Texture system bugfixes
//
// Revision 1.13  2004/01/10 16:03:00  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.11  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.10  2003/04/24 20:30:34  hurdler
// Remove lots of compiling warnings
//
// Revision 1.9  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.7  2003/03/23 14:24:14  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.6  2003/03/08 16:07:19  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.5  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.4  2003/01/25 21:33:07  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.3  2002/12/16 22:21:58  smite-meister
// Actor/DActor separation
//
// Revision 1.2  2002/12/03 10:07:13  smite-meister
// Video unit overhaul begins
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Texture generation and caching. Colormap loading.

#include <math.h>
#include <png.h>
#include <GL/glu.h>

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "cvars.h"
#include "parser.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"

#include "i_video.h"
#include "r_data.h"
#include "r_main.h"
#include "r_draw.h"
#include "m_swap.h"

#include "w_wad.h"
#include "z_zone.h"

extern byte gammatable[5][256];

static int R_TransmapNumForName(const char *name);


// TODO after exiting a mapcluster, flush unnecessary graphics...
//  R_LoadTextures ();
//  R_FlushTextureCache();
//  R_ClearColormaps();


//
// Graphics.
// DOOM graphics for walls and sprites
// are stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

#ifdef OLDWATER
int             firstwaterflat; //added:18-02-98:WATER!
#endif


//faB: for debugging/info purpose
int             flatmemory;
int             spritememory;
int             texturememory;


//faB: highcolor stuff
short    color8to16[256];       //remap color index to highcolor rgb value
short*   hicolormaps;           // test a 32k colormap remaps high -> high


//==================================================================
//  Utilities
//==================================================================


/// Store lists of lumps for F_START/F_END etc.
struct lumplist_t
{
  int wadfile;
  int firstlump;
  int numlumps;
};


static int R_CheckNumForNameList(const char *name, lumplist_t *ll, int listsize)
{
  for (int i = listsize - 1; i > -1; i--)
    {
      int lump = fc.FindNumForNameFile(name, ll[i].wadfile, ll[i].firstlump);
      if ((lump & 0xffff) >= (ll[i].firstlump + ll[i].numlumps) || lump == -1)
        continue;
      else
        return lump;
    }
  return -1;
}



// given an RGB triplet, returns the index of the nearest color in
// the current "zero"-palette using a quadratic distance measure.
// Thanks to quake2 source!
byte NearestColor(byte r, byte g, byte b)
{
  int bestdistortion = 256 * 256 * 4;
  int bestcolor = 0;

  for (int i = 0; i < 256; i++)
    {
      int dr = r - vid.palette[i].red;
      int dg = g - vid.palette[i].green;
      int db = b - vid.palette[i].blue;
      int distortion = dr*dr + dg*dg + db*db;

      if (distortion < bestdistortion)
        {
          if (!distortion)
            return i;

          bestdistortion = distortion;
          bestcolor = i;
        }
    }

  return bestcolor;
}


// create best possible colormap from one palette to another
static byte *R_CreatePaletteConversionColormap(int wadnum)
{
  int i = fc.FindNumForNameFile("PLAYPAL", wadnum);
  if (i == -1)
    {
      // no palette available
      return NULL;
    }

  if (fc.LumpLength(i) < int(256 * sizeof(RGB_t)))
    I_Error("Bad PLAYPAL lump in file %d!\n", wadnum);

  byte *usegamma = gammatable[cv_usegamma.value];
  byte *colormap = (byte *)Z_Malloc(256, PU_STATIC, NULL);
  RGB_t *pal = (RGB_t *)fc.CacheLumpNum(i, PU_STATIC);

  for (i=0; i<256; i++)
    colormap[i] = NearestColor(usegamma[pal[i].r], usegamma[pal[i].g], usegamma[pal[i].b]);

  Z_Free(pal);
  return colormap;
}


// applies a given colormap to a patch_t
static void R_ColormapPatch(patch_t *p, byte *colormap)
{
 for (int i=0; i<p->width; i++)
   {
     post_t *post = (post_t *)((byte *)p + p->columnofs[i]);

     while (post->topdelta != 0xff)
       {
         int count = post->length;
         for (int j=0; j<count; j++)
           post->data[j] = colormap[post->data[j]];

         post = (post_t *)&post->data[post->length + 1];
       }
   }
}



//==================================================================
//  Texture
//==================================================================

Texture::Texture(const char *n)
  : cacheitem_t(n)
{
  width = height = 0;
  leftoffset = topoffset = 0;
  xscale = yscale = 1;
  pixels = NULL;
  w_bits = h_bits = 0;

  // crap follows.
  id = 0;

  glid = NOTEXTURE; 
}


Texture::~Texture()
{
  if (pixels)
    Z_Free(pixels);

  if(glid != NOTEXTURE)
    glDeleteTextures(1, &glid);
}


void *Texture::operator new(size_t size)
{
  return Z_Malloc(size, PU_TEXTURE, NULL);
}


void Texture::operator delete(void *mem)
{
  Z_Free(mem);
}

// Creates a GL texture from the given row-major data. Width and
// height are taken from class variables.

void Texture::BuildGLTexture(byte *rgba) {

  // Discard old texture if we had one.
  if(glid != NOTEXTURE)
    glDeleteTextures(1, &glid);

  glGenTextures(1, &glid);
  glBindTexture(GL_TEXTURE_2D, glid);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		  GL_NEAREST_MIPMAP_NEAREST);
  gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, \
		    GL_UNSIGNED_BYTE, rgba);

  //  CONS_Printf("Created GL texture %d for %s.\n", glid, name);
}


//==================================================================
//  LumpTexture
//==================================================================


// Flats etc.
LumpTexture::LumpTexture(const char *n, int l, int w, int h)
  : Texture(n)
{
  lump = l;
  width = w;
  height = h;
  Initialize();
}



/// This method handles the Doom flats and raw pages.
byte *LumpTexture::Generate()
{
  if (!pixels)
    {
      int len = fc.LumpLength(lump);
      Z_Malloc(len, PU_TEXTURE, (void **)(&pixels));

      // to avoid unnecessary memcpy
      //fc.ReadLump(lump, pixels);

      byte *temp = (byte *)fc.CacheLumpNum(lump, PU_STATIC);

      // transposed to col-major order
      int dest = 0;
      for (int i=0; i<len; i++)
	{
	  pixels[dest] = temp[i];
	  dest += height;
	  if (dest >= len)
	    dest -= len - 1; // next column
	}
      Z_Free(temp);

      // convert to high color
      // short pix16 = ((color8to16[*data++] & 0x7bde) + ((i<<9|j<<4) & 0x7bde))>>1;
    }

  // Create OpenGL texture if required.
  if(oglrenderer != NULL && glid == NOTEXTURE) {
    byte *rgba;
    rgba = ColumnMajorToRGBA(pixels, width, height);
    BuildGLTexture(rgba);
    Z_Free(rgba);
  }
  return pixels;
}


byte *LumpTexture::GetColumn(int col)
{
  col %= width;
  if (col < 0)
    col += width; // wraparound

  return Generate() + col * height;
}



//==================================================================
//  PatchTexture
//==================================================================

// Clip and draw a column from a patch into a cached post.
static void R_DrawColumnInCache(column_t *col, byte *cache, int originy, int cacheheight)
{
  while (col->topdelta != 0xff)
    {
      byte *source = col->data; // go to the data
      int count = col->length;
      int position = originy + col->topdelta;

      if (position < 0)
        {
          count += position;
          source -= position;
          position = 0;
        }

      if (position + count > cacheheight)
        count = cacheheight - position;

      if (count > 0)
        memcpy(cache + position, source, count);

      col = (column_t *)&col->data[col->length + 1];
    }
}


PatchTexture::PatchTexture(const char *n, int l)
  : Texture(n)
{
  lump = l;

  patch_t p;
  fc.ReadLumpHeader(lump, &p, sizeof(patch_t));
  width = SHORT(p.width);
  height = SHORT(p.height);
  leftoffset = SHORT(p.leftoffset);
  topoffset = SHORT(p.topoffset);

  Initialize();
  // nothing more is needed until the texture is Generated.

  patch_data = NULL;

  if (fc.LumpLength(lump) <= width*4 + 8)
    I_Error("PatchTexture: lump %d (%s) is invalid\n", l, n);
}


PatchTexture::~PatchTexture()
{
  if (patch_data)
    Z_Free(patch_data);
}


/// sets patch_data
patch_t *PatchTexture::GeneratePatch()
{
  if (!patch_data)
    {
      int len = fc.LumpLength(lump);
      patch_t *p = (patch_t *)Z_Malloc(len, PU_TEXTURE, (void **)&patch_data);

      // to avoid unnecessary memcpy
      fc.ReadLump(lump, patch_data);

      // [segabor] necessary endianness conversion for p
      // [smite] should not be necessary, because the other fields are never used 
	  
      for (int i=0; i < width; i++)
        p->columnofs[i] = LONG(p->columnofs[i]);

      // do a palette conversion if needed
      byte *colormap = tc.GetPaletteConv(lump >> 16);
      if (colormap)
	{
	  p->width = SHORT(p->width);
	  R_ColormapPatch(p, colormap);
	}
    }

  return reinterpret_cast<patch_t*>(patch_data);
}


/// sets pixels (which implies that patch_data is also set)
byte *PatchTexture::GenerateData()
{
  if (!pixels)
    {
      // we need to draw the patch into a rectangular bitmap in column-major order
      int len = width*height;
      Z_Malloc(len, PU_TEXTURE, (void **)&pixels);
      memset(pixels, TRANSPARENTPIXEL, len);

      patch_t *p = GeneratePatch();

      for (int col = 0; col < width; col++)
	{
	  column_t *patchcol = reinterpret_cast<column_t*>(patch_data + p->columnofs[col]);
	  R_DrawColumnInCache(patchcol, pixels + col * height, 0, height);
	}

    }

  // Create OpenGL texture if required.
  if(oglrenderer != NULL && glid == NOTEXTURE) {
    byte *rgba;
    rgba = ColumnMajorToRGBA(pixels, width, height);
    BuildGLTexture(rgba);
    Z_Free(rgba);
  }

  return pixels;
}


column_t *PatchTexture::GetMaskedColumn(int col)
{
  col %= width;
  if (col < 0)
    col += width; // wraparound

  patch_t *p = GeneratePatch();
  return reinterpret_cast<column_t*>(patch_data + p->columnofs[col]);
}


byte *PatchTexture::GetColumn(int col)
{
  col %= width;
  if (col < 0)
    col += width; // wraparound

  return GenerateData() + col * height;
}



//==================================================================
//  DoomTexture
//==================================================================


DoomTexture::DoomTexture(const maptexture_t *mtex)
  : Texture(mtex->name)
{
  patchcount = SHORT(mtex->patchcount);
  patches = (texpatch_t *)Z_Malloc(sizeof(texpatch_t)*patchcount, PU_TEXTURE, 0);

  width  = SHORT(mtex->width);
  height = SHORT(mtex->height);
  xscale = mtex->xscale ? fixed_t(mtex->xscale) >> 3 : 1;
  yscale = mtex->yscale ? fixed_t(mtex->yscale) >> 3 : 1;

  Initialize();
  widthmask = (1 << w_bits) - 1;

  patch_data = NULL;
}


DoomTexture::~DoomTexture()
{
  Z_Free(patches);

  if (patch_data)
    Z_Free(patch_data);
}



// TODO better DoomTexture handling?
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.


//   Allocate space for full size texture, either single patch or 'composite'
//   Build the full textures from patches.
//   The texture caching system is a little more hungry of memory, but has
//   been simplified for the sake of highcolor, dynamic lighting, & speed.

patch_t *DoomTexture::GeneratePatch()
{
  // patchcount must be 1! no splicing!
  // single-patch textures can have holes in it and may be used on
  // 2-sided lines so they need to be kept in patch_t format
  if (!patch_data)
    {
      CONS_Printf("Generating patch for '%s'\n", name);
      texpatch_t *tp = patches;
      int blocksize = fc.LumpLength(tp->patchlump);
      //CONS_Printf ("R_GenTex SINGLE %.8s size: %d\n",name,blocksize);

      patch_t *p = (patch_t *)Z_Malloc(blocksize, PU_TEXTURE, (void **)&patch_data);
      texturememory += blocksize;
      fc.ReadLump(tp->patchlump, patch_data);


      p->width = SHORT(p->width); // endianness...

      // FIXME should use patch width here? texture may be wider!
      if (width > p->width)
	{
	  CONS_Printf("masked tex '%s' too wide\n", name); // FIXME TEMP behavior
	  width = p->width;
	  Initialize();
	  widthmask = (1 << w_bits) - 1;
	}

      // use the patch's column lookup (columnofs is reserved for the raw bitmap version!)
      // do not skip post_t info by default
      for (int i=0; i<width; i++)
        p->columnofs[i] = LONG(p->columnofs[i]);

      // do a palette conversion if needed
      byte *colormap = tc.GetPaletteConv(tp->patchlump >> 16);
      if (colormap)
	R_ColormapPatch(p, colormap);
    }

  return reinterpret_cast<patch_t*>(patch_data);
}


byte *DoomTexture::GenerateData()
{
  if (!pixels)
    {
      CONS_Printf("Generating data for '%s'\n", name);
      int i;
      // multi-patch (or 'composite') textures are stored as a simple bitmap

      int size = width * height;
      int blocksize = size + width*sizeof(int); // first raw col-major data, then columnofs table
      //CONS_Printf ("R_GenTex MULTI  %.8s size: %d\n",name,blocksize);

      Z_Malloc(blocksize, PU_TEXTURE, (void **)&pixels);
      texturememory += blocksize;

      // generate column offset lookup table
      columnofs = (Uint32 *)(pixels + size);
      for (i=0; i<width; i++)
        columnofs[i] = i * height;

      // prepare texture bitmap
      memset(pixels, 0, size); // TEST

      texpatch_t *tp;
      // Composite the patches together.
      for (i=0, tp = patches; i<patchcount; i++, tp++)
        {
	  patch_t *p = (patch_t *)fc.CacheLumpNum(tp->patchlump, PU_STATIC);
          int x1 = tp->originx;
          int x2 = x1 + SHORT(p->width);

          int x = x1;
          if (x < 0)
            x = 0;

          if (x2 > width)
            x2 = width;

          for ( ; x < x2; x++)
            {
              column_t *patchcol = (column_t *)((byte *)p + LONG(p->columnofs[x-x1]));
              R_DrawColumnInCache(patchcol, pixels + columnofs[x], tp->originy, height);
            }

	  Z_Free(p);
        }

      // Now that the texture has been built in column cache,
      //  it is purgable from zone memory.
      //Z_ChangeTag(pixels, PU_CACHE);
    }

  // Create OpenGL texture if required.
  if(oglrenderer != NULL && glid == NOTEXTURE) {
    byte *rgba;
    rgba = ColumnMajorToRGBA(pixels, width, height);
    BuildGLTexture(rgba);
    Z_Free(rgba);
  }

  return pixels;
}



// returns a pointer to column-major raw data
byte *DoomTexture::GetColumn(int col)
{
  return GenerateData() + columnofs[col & widthmask];
}


column_t *DoomTexture::GetMaskedColumn(int col)
{
  if (patchcount == 1)
    {
      patch_t *p = GeneratePatch();
      return (column_t *)(patch_data + p->columnofs[col & widthmask]);
    }
  else
    return NULL;
}


//==================================================================
//  Texture cache
//==================================================================

texturecache_t tc(PU_TEXTURE);


Texture *texturecache_t::operator[](unsigned id)
{
  if (id >= texture_ids.size())
    I_Error("Invalid texture ID %d (max %d)!\n", id, texture_ids.size());

  map<unsigned, Texture *>::iterator t = texture_ids.find(id);
  if (t == texture_ids.end())
    return NULL;
  else
    return t->second;
}



texturecache_t::texturecache_t(memtag_t tag)
{
  tagtype = tag;
  default_item = NULL;
  texture_ids[0] = NULL; // "no texture" id
}


void texturecache_t::SetDefaultItem(const char *name)
{
  if (default_item)
    CONS_Printf("Texturecache: Replacing default_item!\n");

  default_item = Load(name);
  if (!default_item)
    I_Error("Texturecache: New default_item '%s' not found!\n", name);
  // TODO delete the old default item?
}



void texturecache_t::Clear()
{
  new_tex.Clear();
  doom_tex.Clear();
  flat_tex.Clear();
  sprite_tex.Clear();
  lod_tex.Clear();

  texture_ids.clear();
  texture_ids[0] = NULL; // "no texture" id
}



/// Tries loading a texture on demand from a single lump, deducing the format.
Texture *texturecache_t::Load(const char *name)
{
  int lump = fc.FindNumForName(name);

  if (lump < 0)
    return NULL;

  // Because Doom datatypes contain no magic numbers, we have to rely on heuristics to deduce the format...

  Texture *t;

  byte data[8];
  fc.ReadLumpHeader(lump, &data, sizeof(data));
  int size = fc.LumpLength(lump);

  // first check possible magic numbers!
  if (!png_sig_cmp(data, 0, sizeof(data)))
    {
      // it's a PNG
      t = new PNGTexture(name, lump);
    } // then try some common sizes for raw picture lumps
  else if (size == 320*200)
    {
      // Heretic/Hexen fullscreen picture
      t = new LumpTexture(name, lump, 320, 200);
    }
  else if (size == 4)
    {
      // God-damn-it! Heretic "F_SKY1" tries to be funny!
      t = new LumpTexture(name, lump, 2, 2);
    }
  else if (data[2] == 0 && data[6] == 0 && data[7] == 0)
    {
      // likely a pic_t (the magic number is inadequate)
      CONS_Printf("A pic_t image '%s' was found, but this format is no longer supported.\n", name); // root 'em out!
      return NULL;
    }
  else
    {
      // finally assume a patch_t
      t = new PatchTexture(name, lump);
    }

  return t;
}


/// Inserts a Texture into the given source, replaces and possibly deletes the original
/// Returns true if an original was found.
bool texturecache_t::Insert(Texture *t, cachesource_t &s, bool keep_old)
{
  Texture *old = reinterpret_cast<Texture *>(s.Find(t->name));

  if (old)
    {
      // A Texture of that name is already there
      // Happens when generating animated textures.
      // Happens with H_START textures, and if other namespaces have duplicates.
      //CONS_Printf("Texture '%s' replaced!\n", old->name);

      t->id = old->id;        // Grab the ID of the original,
      texture_ids[t->id] = t; // and take its place here.

      s.Replace(t); // remove the old instance from the map, so there is room for the new one!

      if (!keep_old)
	delete old;
    }
  else
    {
      if (keep_old)
	I_Error("Bad animated texture replace!\n");

      t->id = texture_ids.size(); // First texture gets the ID 1, 'cos zero maps to NULL
      texture_ids[t->id] = t;

      s.Insert(t);
    }

  return (old != NULL);
}



void texturecache_t::InitPaletteConversion()
{
  // create the palette conversion colormaps
  unsigned i;
  for (i=0; i<palette_conversion.size(); i++)
    if (palette_conversion[i])
      Z_Free(palette_conversion[i]);

  unsigned n = fc.Size(); // number of resource files
  palette_conversion.resize(n);

  for (i=0; i<n; i++)
    palette_conversion[i] = R_CreatePaletteConversionColormap(i);
}



/// Returns pointer to an existing Texture, or tries creating it if nonexistant.
Texture *texturecache_t::GetPtr(const char *name, texture_class_t mode)
{
  // "No texture" marker.
  if (name[0] == '-')
    return NULL;

  // TODO unfortunate, could be avoided if we used a non-case-sensitive string comparison functor...
  char name8[9];
  strncpy(name8, name, 8);
  name8[8] = '\0';
  strupr(name8);

  cacheitem_t *t;

  switch (mode)
    {
    case TEX_wall: // walls
      if (!(t = new_tex.Find(name8)))
	if (!(t = doom_tex.Find(name8)))
	  t = flat_tex.Find(name8);
      break;

    case TEX_floor: // floors, ceilings
      if (!(t = new_tex.Find(name8)))
	if (!(t = flat_tex.Find(name8)))
	  t = doom_tex.Find(name8);
      break;

    case TEX_sprite: // sprite frames
      t = sprite_tex.Find(name8);
      break;

    case TEX_lod: // menu, HUD, console, background images
      if (!(t = new_tex.Find(name8)))
	if (!(t = lod_tex.Find(name8)))
	  {
	    // Not found there either, try loading on demand
	    Texture *temp = Load(name8);
	    if (temp)
	      Insert(temp, lod_tex);
	    t = temp;
	  }
      break;
    }

  if (!t)
    {
      // Item not found at all.
      // Some nonexistant items are asked again and again.
      // We use a special cacheitem_t to link their names to the default item.
      t = new cacheitem_t(name8);
      t->usefulness = -1; // negative usefulness marks them as links

      // NOTE insertion to cachesource only, not to id-map
      if (mode == TEX_sprite)
	sprite_tex.Insert(t);
      else
	new_tex.Insert(t); 
    }

  if (t->usefulness < 0)
    {
      // a "link" to default_item
      t->refcount++;
      t->usefulness--; // negated

      default_item->refcount++;
      default_item->usefulness++;

      // CONS_Printf("Def. texture used for '%s'\n", name8);
      return default_item;
    }
  else
    {
      t->refcount++;
      t->usefulness++;
      return reinterpret_cast<Texture *>(t); // should be safe
    }
}



/// Shorthand.
Texture *texturecache_t::GetPtrNum(int n)
{
  return GetPtr(fc.FindNameForNum(n), TEX_lod);
}



// semi-hack for linedeftype 242 and the like, where the
// "texture name" field can hold a colormap name instead.
int texturecache_t::GetTextureOrColormap(const char *name, fadetable_t*& cmap)
{
  // "NoTexture" marker. No texture, no colormap.
  if (name[0] == '-')
    {
      cmap = NULL;
      return 0;
    }

  fadetable_t *temp = R_FindColormap(name); // check C_*... lists

  if (temp)
    {
      // it is a colormap lumpname, not a texture
      cmap = temp;
      return 0;
    }

  cmap = NULL;

  // it could be a texture, let's check
  return GetID(name, TEX_wall);
}


// semi-hack for linedeftype 260, where the
// "texture name" field can hold a transmap name instead.
int texturecache_t::GetTextureOrTransmap(const char *name, int& map_num)
{
  // "NoTexture" marker. No texture, no colormap.
  if (name[0] == '-')
    {
      map_num = -1;
      return 0;
    }

  int temp = R_TransmapNumForName(name);

  if (temp >= 0)
    {
      // it is a transmap lumpname, not a texture
      map_num = temp;
      return 0;
    }

  map_num = -1;

  // it could be a texture, let's check
  return GetID(name, TEX_wall);
}


/// Creates a texture object of a given lump, inserts it into a container.
/// Lump number must be valid. Returns true if succesful.
bool texturecache_t::BuildLumpTexture(int lump, bool h_start, cachesource_t &source)
{
  const char *name = fc.FindNameForNum(lump); // not always NUL-terminated!
  Texture *orig = reinterpret_cast<Texture *>(source.Find(name));
  if ((h_start && !orig) || (!h_start && orig))
    return false; // not to be added
  
  byte data[8];
  fc.ReadLumpHeader(lump, &data, sizeof(data));

  Texture *t;

  // some texture formats have magic numbers
  if (!png_sig_cmp(data, 0, sizeof(data)))
    {
      // it's a PNG
      t = new PNGTexture(name, lump);
    }
  else
    {
      // assume a patch_t
      t = new PatchTexture(name, lump);
    }
  /*
  else
    {
      CONS_Printf(" Unknown texture format: lump '%8s' in the file '%s'.\n", name, fc.Name(lump >> 16));
      return false;
    }
  */

  if (t)
    {
      if (h_start)
	{
	  // replace the original, use proper scaling
	  t->xscale = fixed_t(t->width) / orig->width;
	  t->yscale = fixed_t(t->height) / orig->height;
	}
      Insert(t, source);
      return true;
    }

  return false;
}



bool Read_NTEXTURE(); // TEST FIXME

/// Initializes the texture cache, fills the cachesource_t containers with Texture objects.
/// Follows the JDS texture standard.
int texturecache_t::ReadTextures()
{
  int i, lump;
  int num_textures = 0;

  char name8[9];
  name8[8] = 0; // NUL-terminated


  // TEXTUREx/PNAMES:
  {
    // Load the patch names from the PNAMES lump
    struct pnames_t
    {
      Uint32 count;
      char names[][8]; // list of 8-byte patch names
    } __attribute__((packed));

    lump = fc.GetNumForName("PNAMES");
    pnames_t *pnames = (pnames_t *)fc.CacheLumpNum(lump, PU_STATIC);
    int numpatches = LONG(pnames->count);

    if (devparm)
      CONS_Printf(" PNAMES: lump %d:%d, %d patches\n", lump >> 16, lump & 0xffff, numpatches);

    if (fc.LumpLength(lump) != 4 + 8*numpatches)
      I_Error("Corrupted PNAMES lump.\n");

    vector<int> patchlookup(numpatches); // mapping from patchnumber to lumpnumber

    for (i=0 ; i<numpatches ; i++)
      {
	strncpy(name8, pnames->names[i], 8);
	patchlookup[i] = fc.FindNumForName(name8);
	if (patchlookup[i] < 0)
	  CONS_Printf(" Patch '%s' (%d) not found!\n", name8, i);
      }

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    int  *maptex, *maptex1, *maptex2;

    lump = fc.GetNumForName("TEXTURE1");
    maptex = maptex1 = (int *)fc.CacheLumpNum(lump, PU_STATIC);
    int numtextures1 = LONG(*maptex);
    int maxoff = fc.LumpLength(lump);
    if (devparm)
      CONS_Printf(" TEXTURE1: lump %d:%d, %d textures\n", lump >> 16, lump & 0xffff, numtextures1);
    int *directory = maptex+1;

    int numtextures2, maxoff2;
    if (fc.FindNumForName ("TEXTURE2") != -1)
      {
	lump = fc.GetNumForName("TEXTURE2");
	maptex2 = (int *)fc.CacheLumpNum(lump, PU_STATIC);
	numtextures2 = LONG(*maptex2);
	maxoff2 = fc.LumpLength(lump);
	if (devparm)
	  CONS_Printf(" TEXTURE2: lump %d:%d, %d textures\n", lump >> 16, lump & 0xffff, numtextures2);
      }
    else
      {
	maptex2 = NULL;
	numtextures2 = 0;
	maxoff2 = 0;
      }

    int numtextures = numtextures1 + numtextures2;

    for (i=0 ; i<numtextures ; i++, directory++)
      {
	//only during game startup
	//if (!(i&63))
	//    CONS_Printf (".");

	if (i == numtextures1)
	  {
	    // Start looking in second texture file.
	    maptex = maptex2;
	    maxoff = maxoff2;
	    directory = maptex+1;
	  }

	// offset to the current texture in TEXTURESn lump
	int offset = LONG(*directory);

	if (offset > maxoff)
	  I_Error("ReadTextures: bad texture directory");

	// maptexture describes texture name, size, and
	// used patches in z order from bottom to top
	maptexture_t *mtex = (maptexture_t *)((byte *)maptex + offset);
	DoomTexture *tex = new DoomTexture(mtex);

	mappatch_t *mp = mtex->patches;
	DoomTexture::texpatch_t *p = tex->patches;

	for (int j=0 ; j < tex->patchcount ; j++, mp++, p++)
	  {
	    p->originx = SHORT(mp->originx);
	    p->originy = SHORT(mp->originy);
	    p->patchlump = patchlookup[SHORT(mp->patch)];
	    if (p->patchlump == -1)
	      I_Error("ReadTextures: Missing patch %d in texture %.8s (%d)\n",
		      SHORT(mp->patch), mtex->name, i);
	  }

	Insert(tex, doom_tex);
      }

    Z_Free(maptex1);
    if (maptex2)
      Z_Free(maptex2);


    // then rest of PNAMES
    for (i=0 ; i<numpatches ; i++)
      if (patchlookup[i] >= 0)
	{
	  strncpy(name8, pnames->names[i], 8);
	  if (doom_tex.Find(name8))
	    continue; // already defined in TEXTUREx

	  PatchTexture *tex = new PatchTexture(name8, patchlookup[i]);
	  Insert(tex, doom_tex);
	  numtextures++;

          //CONS_Printf(" Bare PNAMES texture '%s' found!\n", name8);
	}

    Z_Free(pnames);

    num_textures += numtextures;
  }


  int nwads = fc.Size();

  // TX_START
  // later files override earlier ones
  for (i = nwads-1; i >= 0; i--)
    {
      int lump = fc.FindNumForNameFile("TX_START", i, 0);
      if (lump == -1)
	continue;
      else
        lump++;

      int end = fc.FindNumForNameFile("TX_END", i, 0);
      if (end == -1)
	{
	  CONS_Printf(" TX_END missing in file '%s'.\n", fc.Name(i));
	  continue; // no TX_END, nothing accepted
	}

      for ( ; lump < end; lump++)
	if (BuildLumpTexture(lump, false, new_tex))
	  num_textures++;
    }

  Read_NTEXTURE(); // TEST


  // F_START, FF_START
  for (i = nwads-1; i >= 0; i--)
    {
      // Only accept flats between F(F)_START and F(F)_END

      int lump = fc.FindNumForNameFile("F_START", i, 0);
      if (lump == -1)
        lump = fc.FindNumForNameFile("FF_START", i, 0); //deutex compatib.

      if (lump == -1)
	continue; // no flats in this wad
      else
        lump++;  // just after F_START

      int end = fc.FindNumForNameFile("F_END", i, 0);
      if (end == -1)
        end = fc.FindNumForNameFile("FF_END", i, 0);     //deutex compatib.

      if (end == -1)
	{
	  CONS_Printf(" F_END missing in file '%s'.\n", fc.Name(i));
	  continue; // no F_END, no flats accepted
	}

      for ( ; lump < end; lump++)
	{
	  LumpTexture *t;

	  const char *name = fc.FindNameForNum(lump); // not always NUL-terminated!
	  if (flat_tex.Find(name))
	    continue; // already defined

	  int size = fc.LumpLength(lump);
	    
	  if (size ==  64*64 || // normal flats
	      size ==  65*64)   // Damn you, Heretic animated flats!
	    // Flat is 64*64 bytes of raw paletted picture data in one lump
	    t = new LumpTexture(name, lump, 64, 64);
	  else if (size == 128*64) // Some Hexen flats (X_001-X_011) are larger! Why?
	    t = new LumpTexture(name, lump, 128, 64); // TEST
	  else if (size == 128*128)
	    t = new LumpTexture(name, lump, 128, 128); // TEST
	  else
	    {
	      if (size != 0) // markers are OK
		CONS_Printf(" Unknown flat type: lump '%8s' in the file '%s'.\n", name, fc.Name(i));
	      continue;
	    }

	  Insert(t, flat_tex);
	  num_textures++;
	}
    }



  // S_START, SS_START
  for (i = nwads-1; i >= 0; i--)
    {
      // Only accept patch_t's between S(S)_START and S(S)_END

      int lump = fc.FindNumForNameFile("S_START", i, 0);
      if (lump == -1)
        lump = fc.FindNumForNameFile("SS_START", i, 0); //deutex compatib.

      if (lump == -1)
	continue; // no spriteframes in this wad
      else
        lump++;  // just after S_START

      int end = fc.FindNumForNameFile("S_END", i, 0);
      if (end == -1)
        end = fc.FindNumForNameFile("SS_END", i, 0);     //deutex compatib.

      if (end == -1)
	{
	  CONS_Printf(" S_END missing in file '%s'.\n", fc.Name(i));
	  continue; // no S_END, no spriteframes accepted
	}

      for ( ; lump < end; lump++)
	if (BuildLumpTexture(lump, false, sprite_tex))
	  num_textures++;
    }


  // H_START: Variable-resolution textures that are scaled so they match
  // the size of a corresponding texture in TEXTUREx or F_START.
  for (i = nwads-1; i >= 0; i--)
    {
      int lump = fc.FindNumForNameFile("H_START", i, 0);
      if (lump == -1)
        continue;
      else
        lump++;

      int end = fc.FindNumForNameFile("H_END", i, 0);
      if (end == -1)
        {
          CONS_Printf(" H_END missing in file '%s'.\n", fc.Name(i));
          continue; // no H_END, nothing accepted
        }

      for ( ; lump < end; lump++)
	{
	  if (!BuildLumpTexture(lump, true, doom_tex) &&
	      !BuildLumpTexture(lump, true, flat_tex))
	    {
	      CONS_Printf(" H_START texture '%8s' in file '%s' has no original, ignored.\n",
			  fc.FindNameForNum(lump), fc.Name(i));
	    }
	}
    }

  return num_textures;
}




//==================================================================
//    Colormaps
//==================================================================



/// extra boom colormaps
/*
int             num_extra_colormaps;
extracolormap_t extra_colormaps[MAXCOLORMAPS];
*/


/// the fadetable "cache"
static vector<fadetable_t*> fadetables;

/// lumplist for C_START - C_END pairs in wads
static int         numcolormaplumps;
static lumplist_t *colormaplumps;



fadetable_t::~fadetable_t()
{
  if (colormap)
    Z_Free(colormap);
}


/// For custom, OpenGL-friendly colormaps
fadetable_t::fadetable_t()
{
  lump = -1;
  colormap = NULL;
}

/// For lump-based colormaps
fadetable_t::fadetable_t(int lumpnum)
{
  lump = lumpnum;

  int length = fc.LumpLength(lump);
  if (length != 34*256)
    {
      CONS_Printf("Unknown colormap size: %d bytes\n", length);
    }

  // (aligned on 8 bit for asm code) now 64k aligned for smokie...
  colormap = (lighttable_t *)Z_MallocAlign(length, PU_STATIC, 0, 16); // 8);
  fc.ReadLump(lump, colormap);

  // SoM: Added, we set all params of the colormap to normal because there
  // is no real way to tell how GL should handle a colormap lump anyway..
  // TODO try to analyze the colormap and somehow simulate it in GL...
  maskcolor = 0xffff;
  fadecolor = 0x0;
  maskamt = 0x0;
  fadestart = 0;
  fadeend = 33;
  fog = 0;
}



/// Returns the name of the fadetable
const char *R_ColormapNameForNum(const fadetable_t *p)
{
  if (!p)
    return "NONE";

  int temp = p->lump;
  if (temp == -1)
    return "INLEVEL";

  return fc.FindNameForNum(temp);
}



/// Called in R_Init, prepares the lightlevel colormaps and Boom extra colormaps.
void R_InitColormaps()
{
  // build the colormap lumplists (which record the locations of C_START and C_END in each wad)
  numcolormaplumps = 0;
  colormaplumps = NULL;

  int nwads = fc.Size();
  for (int file = 0; file < nwads; file++)
    {
      int start = fc.FindNumForNameFile("C_START", file, 0);
      if (start == -1)
        continue; // required

      int end = fc.FindNumForNameFile("C_END", file, 0);
      if (end == -1)
	{
	  CONS_Printf("R_InitColormaps: C_START without C_END in file '%s'!\n", fc.Name(file));
	  continue;
	}

      colormaplumps = (lumplist_t *)realloc(colormaplumps, sizeof(lumplist_t) * (numcolormaplumps + 1));
      colormaplumps[numcolormaplumps].wadfile = start >> 16;
      colormaplumps[numcolormaplumps].firstlump = (start & 0xFFFF) + 1; // after C_START
      colormaplumps[numcolormaplumps].numlumps = end - (start + 1);
      numcolormaplumps++;
    }

  //SoM: 3/30/2000: Init Boom colormaps.
  R_ClearColormaps();

  // Load in the basic lightlevel colormaps
  fadetables.push_back(new fadetable_t(fc.GetNumForName("COLORMAP")));
}


/// Clears out all extra colormaps.
void R_ClearColormaps()
{
  int n = fadetables.size();
  for (int i=0; i < n; i++)
    delete fadetables[i];

  fadetables.clear();
}


/// Auxiliary Get func for fadetables
fadetable_t *R_GetFadetable(unsigned n)
{
  if (n < fadetables.size())
    return fadetables[n];
  else
    return NULL;
}


/// Tries to retrieve a colormap by name. Only checks stuff between C_START/C_END.
fadetable_t *R_FindColormap(const char *name)
{
  int lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
  if (lump == -1)
    return NULL; // no such colormap
  
  int n = fadetables.size();
  for (int i=0; i < n; i++)
    if (fadetables[i]->lump == lump)
      return fadetables[i];

  // not already cached, we must read it from WAD
  if (n == MAXCOLORMAPS)
    I_Error("R_ColormapNumForName: Too many colormaps!\n");

  fadetable_t *temp = new fadetable_t(lump);
  fadetables.push_back(temp);
  return temp;
}


/// Sets the default lighlevel colormaps used by the software renderer in this map.
/// All lumps are searched.
bool Map::R_SetFadetable(const char *name)
{
  int lump = fc.FindNumForName(name);
  if (lump < 0)
    {
      CONS_Printf("R_SetFadetable: Fadetable '%s' not found.\n", name);
      fadetable = fadetables[0]; // use COLORMAP
      return false;
    }

  int n = fadetables.size();
  for (int i=0; i < n; i++)
    if (fadetables[i]->lump == lump)
      {
	fadetable = fadetables[i];
	return true;
      }

  // not already cached, we must read it from WAD
  if (n == MAXCOLORMAPS)
    I_Error("R_SetFadetable: Too many colormaps!\n");

  fadetable = new fadetable_t(lump);
  fadetables.push_back(fadetable);
  return true;
}




// Rounds off floating numbers and checks for 0 - 255 bounds
static int RoundUp(double number)
{
  if (number >= 255.0)
    return 255;
  if (number <= 0)
    return 0;

  if (int(number) <= (number - 0.5))
    return int(number) + 1;

  return int(number);
}


// R_CreateColormap
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.

fadetable_t *R_CreateColormap(char *p1, char *p2, char *p3)
{
  int    i, p;
  double cmaskr, cmaskg, cmaskb;
  double maskamt = 0, othermask = 0;
  unsigned int  cr, cg, cb, ca;
  unsigned int  maskcolor;
  int rgba;

  //TODO: Hurdler: check this entire function
  // for now, full support of toptexture only

#define HEX2INT(x) (x >= '0' && x <= '9' ? x - '0' : \
		    x >= 'a' && x <= 'f' ? x - 'a' + 10 : \
		    x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : \
		      x >= 'A' && x <= 'Z' ? x - 'A' : 0)
  if (p1[0] == '#')
    {
      cmaskr = cr = ((HEX2INT(p1[1]) << 4) + HEX2INT(p1[2]));
      cmaskg = cg = ((HEX2INT(p1[3]) << 4) + HEX2INT(p1[4]));
      cmaskb = cb = ((HEX2INT(p1[5]) << 4) + HEX2INT(p1[6]));
      maskamt = ca = ALPHA2INT(p1[7]);

      // 24-bit color
      rgba = cr + (cg << 8) + (cb << 16) + (ca << 24);

      // Create a rough approximation of the color (a 16 bit color)
      maskcolor = (cb >> 3) + ((cg >> 2) << 5) + ((cr >> 3) << 11);

      maskamt /= 24.0;

      othermask = 1 - maskamt;
      maskamt /= 0xff;
      cmaskr *= maskamt;
      cmaskg *= maskamt;
      cmaskb *= maskamt;
    }
  else
    {
      cmaskr = 0xff;
      cmaskg = 0xff;
      cmaskb = 0xff;
      maskamt = 0;
      rgba = 0;
      maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
    }
#undef ALPHA2INT


  unsigned int  fadestart = 0, fadeend = 33, fadedist = 33;
  int fog = 0;

#define NUMFROMCHAR(c)  (c >= '0' && c <= '9' ? c - '0' : 0)
  if (p2[0] == '#')
    {
      // SoM: Get parameters like, fadestart, fadeend, and the fogflag...
      fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
      fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
      if (fadestart > 32 || fadestart < 0)
        fadestart = 0;
      if (fadeend > 33 || fadeend < 1)
        fadeend = 33;
      fadedist = fadeend - fadestart;
      fog = NUMFROMCHAR(p2[1]) ? 1 : 0;
    }
#undef NUMFROMCHAR


  double cdestr, cdestg, cdestb;
  unsigned int fadecolor;
  if (p3[0] == '#')
    {
      cdestr = cr = ((HEX2INT(p3[1]) << 4) + HEX2INT(p3[2]));
      cdestg = cg = ((HEX2INT(p3[3]) << 4) + HEX2INT(p3[4]));
      cdestb = cb = ((HEX2INT(p3[5]) << 4) + HEX2INT(p3[6]));
      fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
    }
  else
    {
      cdestr = 0;
      cdestg = 0;
      cdestb = 0;
      fadecolor = 0;
    }
#undef HEX2INT

  fadetable_t *f;

  // check if we already have created one like it
  int n = fadetables.size();
  for (i = 0; i < n; i++)
    {
      f = fadetables[i];
      if (f->lump == -1 &&
	  maskcolor == f->maskcolor &&
	  fadecolor == f->fadecolor &&
	  maskamt == f->maskamt &&
	  fadestart == f->fadestart &&
	  fadeend == f->fadeend &&
	  fog == f->fog &&
	  rgba == f->rgba)
	return f;
    }

  // nope, we have to create it now

  if (n == MAXCOLORMAPS)
    I_Error("R_CreateColormap: Too many colormaps!\n");

  f = new fadetable_t();
  f->maskcolor = maskcolor;
  f->fadecolor = fadecolor;
  f->maskamt = maskamt;
  f->fadestart = fadestart;
  f->fadeend = fadeend;
  f->fog = fog;
  f->rgba = rgba;


#ifdef HWRENDER
  // OpenGL renderer does not need the colormap part
  if (rendermode == render_soft)
#endif
  {
    double deltas[256][3], cmap[256][3];

    for (i = 0; i < 256; i++)
    {
      double r, g, b;
      r = vid.palette[i].red;
      g = vid.palette[i].green;
      b = vid.palette[i].blue;
      double cbrightness = sqrt((r*r) + (g*g) + (b*b));

      cmap[i][0] = (cbrightness * cmaskr) + (r * othermask);
      if(cmap[i][0] > 255.0)
        cmap[i][0] = 255.0;
      deltas[i][0] = (cmap[i][0] - cdestr) / (double)fadedist;

      cmap[i][1] = (cbrightness * cmaskg) + (g * othermask);
      if(cmap[i][1] > 255.0)
        cmap[i][1] = 255.0;
      deltas[i][1] = (cmap[i][1] - cdestg) / (double)fadedist;

      cmap[i][2] = (cbrightness * cmaskb) + (b * othermask);
      if(cmap[i][2] > 255.0)
        cmap[i][2] = 255.0;
      deltas[i][2] = (cmap[i][2] - cdestb) / (double)fadedist;
    }

    char *colormap_p = (char *)Z_MallocAlign((256 * 34) + 1, PU_LEVEL, 0, 8);
    f->colormap = (lighttable_t *)colormap_p;

    for(p = 0; p < 34; p++)
    {
      for(i = 0; i < 256; i++)
      {
        *colormap_p = NearestColor(RoundUp(cmap[i][0]), RoundUp(cmap[i][1]), RoundUp(cmap[i][2]));
        colormap_p++;

        if((unsigned int)p < fadestart)
          continue;

#define ABS2(x) (x) < 0 ? -(x) : (x)

        if(ABS2(cmap[i][0] - cdestr) > ABS2(deltas[i][0]))
          cmap[i][0] -= deltas[i][0];
        else
          cmap[i][0] = cdestr;

        if(ABS2(cmap[i][1] - cdestg) > ABS2(deltas[i][1]))
          cmap[i][1] -= deltas[i][1];
        else
          cmap[i][1] = cdestg;

        if(ABS2(cmap[i][2] - cdestb) > ABS2(deltas[i][1]))
          cmap[i][2] -= deltas[i][2];
        else
          cmap[i][2] = cdestb;

#undef ABS2
      }
    }
  }

  return f;
}


//  build a table for quick conversion from 8bpp to 15bpp
static int makecol15(int r, int g, int b)
{
  return (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3)));
}

void R_Init8to16()
{
  int i;
  byte *palette = (byte *)fc.CacheLumpName("PLAYPAL", PU_CACHE);

  for (i=0;i<256;i++)
    {
      // doom PLAYPAL are 8 bit values
      color8to16[i] = makecol15(palette[0],palette[1],palette[2]);
      palette += 3;
    }

  // test a big colormap
  hicolormaps = (short int *)Z_Malloc(32768 /**34*/, PU_STATIC, 0);
  for (i=0;i<16384;i++)
    hicolormaps[i] = i<<1;
}

// Converts indexed data in column major order to RGBA data in row
// major order. The caller is responsible for Z_Free:ing the resulting
// graphic.

byte* ColumnMajorToRGBA(byte *pixeldata, int w, int h) {
  byte *result;
  int i, j;
  result = static_cast<byte*> (Z_Malloc(4*w*h, PU_STATIC, NULL));
  byte *palette = (byte *)fc.CacheLumpName("PLAYPAL", PU_CACHE);

  for(i=0; i<w; i++)
    for(j=0; j<h; j++) {
      byte curbyte = pixeldata[i*h + j];
      byte *rgb_in = palette + 3*curbyte;
      byte *rgba_out = result + 4*(j*w + i);
      *rgba_out++ = *rgb_in++;
      *rgba_out++ = *rgb_in++;
      *rgba_out++ = *rgb_in++;
      if(curbyte == TRANSPARENTPIXEL)
	*rgba_out = 0;
      else
	*rgba_out = 255;
    }

  return result;
}



//==================================================================
//    Translucency tables
//==================================================================


static int num_transtables;
int  transtable_lumps[MAXTRANSTABLES]; // i'm so lazy
byte *transtables[MAXTRANSTABLES];


/// Tries to retrieve a transmap by name. Returns transtable number, or -1 if unsuccesful.
static int R_TransmapNumForName(const char *name)
{
  int lump = fc.FindNumForName(name);
  if (lump >= 0 && fc.LumpLength(lump) == tr_size)
    {
      for (int i=0; i<num_transtables; i++)
	if (transtable_lumps[i] == lump)
	  return i;

      if (num_transtables == MAXTRANSTABLES)
	I_Error("R_TransmapNumForName: Too many transmaps!\n");
      
      int n = num_transtables++;
      transtables[n] = (byte *)Z_MallocAlign(tr_size, PU_STATIC, 0, 16);
      fc.ReadLump(lump, transtables[n]);
      return n;
    }
  return -1;
}


void R_InitTranslucencyTables()
{
  // NOTE: the translation tables MUST BE aligned on 64k for the asm optimised code
  //       (in other words, transtables pointer low word is 0)
  int i;

  for (i=0; i<MAXTRANSTABLES; i++)
    {
      transtable_lumps[i] = -1;
      transtables[i] = NULL;
    }
    
  transtables[0] = (byte *)Z_MallocAlign(tr_size, PU_STATIC, 0, 16);

  // load in translucency tables

  // first transtable
  // check for the Boom default transtable lump
  int lump = fc.FindNumForName("TRANMAP");
  if (lump >= 0)
    fc.ReadLump(lump, transtables[0]);
  else if (game.mode >= gm_heretic)
    fc.ReadLump(fc.GetNumForName("TINTTAB"), transtables[0]);
  else
    fc.ReadLump(fc.GetNumForName("TRANSMED"), transtables[0]); // in legacy.wad

  if (game.mode >= gm_heretic)
    {
      // all the basic transmaps are the same
      for (i=1; i<5; i++)
	transtables[i] = transtables[0];
    }
  else
    {
      // we can use the transmaps in legacy.wad
      for (i=1; i<5; i++)
	transtables[i] = (byte *)Z_MallocAlign(tr_size, PU_STATIC, 0, 16);

      fc.ReadLump(fc.GetNumForName("TRANSMOR"), transtables[1]);
      fc.ReadLump(fc.GetNumForName("TRANSHI"),  transtables[2]);
      fc.ReadLump(fc.GetNumForName("TRANSFIR"), transtables[3]);
      fc.ReadLump(fc.GetNumForName("TRANSFX1"), transtables[4]);
    }

  num_transtables = 5;


  // Compose a default linear filter map based on PLAYPAL.
  /*
  // Thanks to TeamTNT for prBoom sources!
  if (false)
    {
      // filter weights
      float w1 = 0.66;
      float w2 = 1 - w1;

      int i, j;
      byte *tp = transtables[0];

      for (i=0; i<256; i++)
	{
	  float r2 = vid.palette[i].red   * w2;
	  float g2 = vid.palette[i].green * w2;
	  float b2 = vid.palette[i].blue  * w2;

	  for (j=0; j<256; j++, tp++)
	    {
	      byte r = vid.palette[j].red   * w1 + r2;
	      byte g = vid.palette[j].green * w1 + g2;
	      byte b = vid.palette[j].blue  * w1 + b2;

	      *tp = NearestColor(r, g, b);
	    }
	}
    }
  */
}









//lumplist_t*  flats;
//int          numflatlists;
//extern int   numwadfiles;

/*
void R_InitFlats()
{
  int       startnum;
  int       endnum;
  int       cfile;
  int       clump;
  int nwads;

  numflatlists = 0;
  flats = NULL;
  cfile = clump = 0;

#ifdef OLDWATER
  //added:18-02-98: WATER! flatnum of the first waterflat
  firstwaterflat = fc.GetNumForName ("WATER0");
#endif
  nwads = fc.Size();

  for(;cfile < nwads;cfile ++, clump = 0)
  {
    startnum = fc.FindNumForNameFile("F_START", cfile, clump);
    if(startnum == -1)
    {
      clump = 0;
      startnum = fc.FindNumForNameFile("FF_START", cfile, clump);

      if(startnum == -1) //If STILL -1, search the whole file!
      {
        flats = (lumplist_t *)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
        flats[numflatlists].wadfile = cfile;
        flats[numflatlists].firstlump = 0;
        flats[numflatlists].numlumps = 0xffff; //Search the entire file!
        numflatlists ++;
        continue;
      }
    }

    endnum = fc.FindNumForNameFile("F_END", cfile, clump);
    if(endnum == -1)
      endnum = fc.FindNumForNameFile("FF_END", cfile, clump);

    if(endnum == -1 || (startnum &0xFFFF) > (endnum & 0xFFFF))
    {
      flats = (lumplist_t *)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
      flats[numflatlists].wadfile = cfile;
      flats[numflatlists].firstlump = 0;
      flats[numflatlists].numlumps = 0xffff; //Search the entire file!
      numflatlists ++;
      continue;
    }

    flats = (lumplist_t *)realloc(flats, sizeof(lumplist_t) * (numflatlists + 1));
    flats[numflatlists].wadfile = startnum >> 16;
    flats[numflatlists].firstlump = (startnum&0xFFFF) + 1;
    flats[numflatlists].numlumps = endnum - (startnum + 1);
    numflatlists++;
    continue;
  }

  if(!numflatlists)
    I_Error("R_InitFlats: No flats found!\n");
}
*/



//
// TODO Preloads all relevant graphics for the Map.
//
void Map::PrecacheMap()
{
  // Precache textures.
    //
    // no need to precache all software textures in 3D mode
    // (note they are still used with the reference software view)
    //texturepresent = (char *)alloca(numtextures);
  /*
  char *texturepresent = (char *)Z_Malloc(numtextures, PU_STATIC, 0);
  memset(texturepresent,0, numtextures);

    for (i=0 ; i<numsides ; i++)
    {
        //Hurdler: huh, a potential bug here????
        if (sides[i].toptexture < numtextures)
            texturepresent[sides[i].toptexture] = 1;
        if (sides[i].midtexture < numtextures)
            texturepresent[sides[i].midtexture] = 1;
        if (sides[i].bottomtexture < numtextures)
            texturepresent[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependend
    //  name.
    texturepresent[skytexture] = 1;

    //if (devparm)
    //    CONS_Printf("Generating textures..\n");

    texturememory = 0;
    for (i=0 ; i<numtextures ; i++)
    {
        if (!texturepresent[i])
            continue;

        //texture = textures[i];
        if( texturecache[i]==NULL )
            R_GenerateTexture (i);
        //numgenerated++;

        // note: pre-caching individual patches that compose textures became
        //       obsolete since we now cache entire composite textures

        //for (j=0 ; j<texture->patchcount ; j++)
        //{
        //    lump = texture->patches[j].patch;
        //    texturememory += fc.LumpLength(lump);
        //    fc.CacheLumpNum(lump , PU_CACHE);
        //}
    }
    Z_Free(texturepresent);
  */
    //CONS_Printf ("total mem for %d textures: %d k\n",numgenerated,texturememory>>10);

    //
    // Precache sprites.
    //
    /*
    spritepresent = (char *)alloca(numsprites);
    spritepresent = (char *)Z_Malloc(numsprites, PU_STATIC, 0);
    memset (spritepresent,0, numsprites);

    Thinker *th;

    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      if (th->Type() == Thinker::tt_dactor)
        spritepresent[((DActor *)th)->sprite] = 1;
    }

    spriteframe_t*      sf;
    spritememory = 0;
    for (i=0 ; i<numsprites ; i++)
    {
        if (!spritepresent[i])
            continue;

        for (j=0 ; j<sprites[i].numframes ; j++)
        {
            sf = &sprites[i].spriteframes[j];
            for (k=0 ; k<8 ; k++)
            {
                //Fab: see R_InitSprites for more about lumppat,lumpid
                lump = sf->lumppat[k];
                if(devparm)
                   spritememory += fc.LumpLength(lump);
                fc.CachePatchNum(lump , PU_CACHE);
            }
        }
    }
    Z_Free(spritepresent);

    //FIXME: this is no more correct with glide render mode
    if (devparm)
    {
        CONS_Printf("Precache level done:\n"
                    "flatmemory:    %ld k\n"
                    "texturememory: %ld k\n"
                    "spritememory:  %ld k\n", flatmemory>>10, texturememory>>10, spritememory>>10 );
    }
    */
}
