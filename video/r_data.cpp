// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.37  2004/11/28 18:02:24  smite-meister
// RPCs finally work!
//
// Revision 1.36  2004/11/19 16:51:07  smite-meister
// cleanup
//
// Revision 1.35  2004/11/09 20:38:53  smite-meister
// added packing to I/O structs
//
// Revision 1.34  2004/11/04 21:12:55  smite-meister
// save/load fixed
//
// Revision 1.33  2004/10/31 22:22:13  smite-meister
// Hasta la vista, pic_t!
//
// Revision 1.32  2004/10/27 17:37:11  smite-meister
// netcode update
//
// Revision 1.31  2004/10/14 19:35:52  smite-meister
// automap, bbox_t
//
// Revision 1.30  2004/09/23 23:21:20  smite-meister
// HUD updated
//
// Revision 1.29  2004/09/14 21:41:57  hurdler
// rename "data" to "pixels" (I think it's more appropriate and that's how SDL and OpenGL name such data after all)
//
// Revision 1.28  2004/09/03 16:28:52  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.27  2004/08/29 20:48:50  smite-meister
// bugfixes. wow.
//
// Revision 1.26  2004/08/19 19:42:42  smite-meister
// bugfixes
//
// Revision 1.25  2004/08/18 14:35:23  smite-meister
// PNG support!
//
// Revision 1.23  2004/08/15 18:08:30  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.22  2004/08/13 18:25:11  smite-meister
// sw renderer fix
//
// Revision 1.21  2004/08/12 18:30:33  smite-meister
// cleaned startup
//
// Revision 1.20  2004/07/25 20:16:43  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.19  2004/07/07 17:27:20  smite-meister
// bugfixes
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
// Revision 1.12  2003/11/23 00:41:55  smite-meister
// bugfixes
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
// Revision 1.8  2003/03/29 20:08:04  smite-meister
// Cast added
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
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

extern byte gammatable[5][256];

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

#ifdef OLDWATER
int             firstwaterflat; //added:18-02-98:WATER!
#endif


/// lightlevel colormaps from COLORMAP lump
lighttable_t    *colormaps;


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
  RGB_t* pal = (RGB_t *)fc.CacheLumpNum(i, PU_CACHE);

  for (i=0; i<256; i++)
    colormap[i] = NearestColor(usegamma[pal[i].r], usegamma[pal[i].g], usegamma[pal[i].b]);

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
//  Textures
//==================================================================

Texture::Texture(const char *n)
  : cacheitem_t(n)
{
  width = height = 0;
  leftoffset = topoffset = 0;
  xscale = yscale = FRACUNIT;
  pixels = NULL;

  // crap follows.
  id = 0;
}


Texture::~Texture()
{
  if (pixels)
    Z_Free(pixels);
}


void *Texture::operator new(size_t size)
{
  return Z_Malloc(size, PU_TEXTURE, NULL);
}

void Texture::operator delete(void *mem)
{
  Z_Free(mem);
}


//==================================================================
//  LumpTexture
//==================================================================


// Flats etc.
//
LumpTexture::LumpTexture(const char *n, int l, int w, int h)
  : Texture(n)
{
  lump = l;
  width = w;
  height = h;
}


byte *LumpTexture::Generate()
{
  if (!pixels)
    {
      int len = fc.LumpLength(lump);
      Z_Malloc(len, PU_TEXTURE, (void **)(&pixels));

      // to avoid unnecessary memcpy
      fc.ReadLump(lump, pixels);

      // convert to high color
      // short pix16 = ((color8to16[*data++] & 0x7bde) + ((i<<9|j<<4) & 0x7bde))>>1;
    }

  return pixels;
}


byte *LumpTexture::GetColumn(int col)
{
  return Generate(); // TODO not correct but better than nothing?
}


//==================================================================
//  PatchTexture
//==================================================================

PatchTexture::PatchTexture(const char *n, int l)
  : Texture(n)
{
  lump = l;
  patch_t *p = (patch_t *)fc.CacheLumpNum(lump, PU_CACHE);

  width = SHORT(p->width);
  height = SHORT(p->height);
  leftoffset = SHORT(p->leftoffset);
  topoffset = SHORT(p->topoffset);

  if (fc.LumpLength(lump) <= width*4 + 8)
    I_Error("PatchTexture: lump %d (%s) is invalid\n", l, n);
}


byte *PatchTexture::Generate()
{
  if (!pixels)
    {
      int len = fc.LumpLength(lump);
      patch_t *p = (patch_t *)Z_Malloc(len, PU_TEXTURE, (void **)&pixels);

      // to avoid unnecessary memcpy
      fc.ReadLump(lump, pixels);

      // necessary endianness conversion
      for (int i=0; i < width; i++)
        p->columnofs[i] = LONG(p->columnofs[i]);

      // do a palette conversion if needed
      byte *colormap = tc.GetPaletteConv(lump >> 16);
      if (colormap)
        R_ColormapPatch(p, colormap);
    }

  return pixels;
}


byte *PatchTexture::GetColumn(int col)
{
  col %= width;
  if (col < 0)
    col += width; // wraparound

  patch_t *p = (patch_t *)Generate();
  return pixels + p->columnofs[col] + 3; // skip the post_t info
}


column_t *PatchTexture::GetMaskedColumn(int col)
{
  col %= width;
  if (col < 0)
    col += width; // wraparound

  patch_t *p = (patch_t *)Generate();
  return (column_t *)(pixels + p->columnofs[col]);
}


byte *PatchTexture::GetData()
{
  return Generate(); // TODO not correct but better than nothing?
}


//==================================================================
//  DoomTexture
//==================================================================


DoomTexture::DoomTexture(const maptexture_t *mtex)
  : Texture(mtex->name)
{
  patchcount = SHORT(mtex->patchcount);
  patches = (texpatch_t *)Z_Malloc(sizeof(texpatch_t)*patchcount, PU_STATIC, 0);

  width  = SHORT(mtex->width);
  height = SHORT(mtex->height);
  xscale = mtex->xscale ? mtex->xscale << (FRACBITS - 3) : FRACUNIT;
  yscale = mtex->yscale ? mtex->yscale << (FRACBITS - 3) : FRACUNIT;

  int j = 1;
  while (j*2 <= width)
    j <<= 1;
  widthmask = j-1;
}


DoomTexture::~DoomTexture()
{
  Z_Free(patches);
}


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
//   been simplified for the sake of highcolor, dynamic ligthing, & speed.

byte *DoomTexture::Generate()
{
  if (pixels)
    return texdata; // still in cache!

  // allocate texture column offset lookup
  int i, blocksize;
  texpatch_t *tp;
  patch_t    *p;

  // single-patch textures can have holes in it and may be used on
  // 2-sided lines so they need to be kept in patch_t format
  if (patchcount == 1)
    {
      tp = patches;

      blocksize = fc.LumpLength(tp->patchlump);
      //CONS_Printf ("R_GenTex SINGLE %.8s size: %d\n",name,blocksize);

      Z_Malloc(blocksize, PU_TEXTURE, (void **)&pixels); // change tag at end of function
      fc.ReadLump(tp->patchlump, pixels);
      p = (patch_t *)pixels; // TODO would it be possible to use just any lumptexture here?

      // use the patch's column lookup
      columnofs = p->columnofs;
      texdata = pixels;

      // FIXME should use patch width here? texture may be wider!
      if (width > SHORT(p->width))
        I_Error("masked tex too wide\n"); // FIXME TEMP behavior

      for (i=0; i<width; i++)
        columnofs[i] = LONG(columnofs[i]) + 3; // skip post_t info by default
    }
  else
    {
      // multi-patch (or 'composite') textures are stored as a simple bitmap

      blocksize = (width * sizeof(int)) + (width * height);
      //CONS_Printf ("R_GenTex MULTI  %.8s size: %d\n",name,blocksize);

      Z_Malloc(blocksize, PU_TEXTURE, (void **)&pixels);

      // columns lookup table
      columnofs = (int *)pixels;
      // texture data after the lookup table
      texdata = pixels + (width * sizeof(int));

      memset(texdata, 0, width * height); // TEST

      // generate column offset lookup
      for (i=0; i<width; i++)
        columnofs[i] = i * height;

      // Composite the patches together.
      for (i=0, tp = patches; i<patchcount; i++, tp++)
        {
          p = (patch_t *)fc.CacheLumpNum(tp->patchlump, PU_CACHE);
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
              R_DrawColumnInCache(patchcol, texdata + columnofs[x], tp->originy, height);
            }
        }
    }

  texturememory += blocksize;
  // Now that the texture has been built in column cache,
  //  it is purgable from zone memory.
  Z_ChangeTag(pixels, PU_CACHE);

  return texdata;
}



// returns a pointer to column-major raw data
byte *DoomTexture::GetColumn(int col)
{
  return Generate() + columnofs[col & widthmask];
  // FIXME tuttifrutti effect?? % width
}


column_t *DoomTexture::GetMaskedColumn(int col)
{
  if (patchcount == 1)
    return (column_t *)(Generate() + columnofs[col & widthmask] - 3); // put back post_t info
  else
    return NULL;
}


byte *DoomTexture::GetData()
{
  return Generate(); // TODO not correct but better than nothing?
}


//==================================================================
//  Texture cache
//==================================================================

texturecache_t tc(PU_TEXTURE);


Texture *texturecache_t::operator[](unsigned id)
{
  if (id >= texture_ids.size())
    I_Error("Invalid texture ID %d (max %d)!\n", id, texture_ids.size());

  map<unsigned, Texture *>::iterator i = texture_ids.find(id);
  if (i == texture_ids.end())
    return NULL;
  else
    return (*i).second;
}


texturecache_t::texturecache_t(memtag_t tag)
  : cache_t(tag)
{
  texture_ids[0] = NULL; // "no texture" id
}


void texturecache_t::Clear()
{
  for (c_iter_t t = c_map.begin(); t != c_map.end(); t++)
    delete t->second;

  c_map.clear();
  texture_ids.clear();
  texture_ids[0] = NULL; // "no texture" id
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



/// tries to retrieve a transmap by name. returns transtable number, -1 if unsuccesful.
static int R_TransmapNumForName(const char *name)
{
  int lump = fc.FindNumForName(name);
  if (lump >= 0 && fc.LumpLength(lump) == tr_size)
    {
      fc.ReadLump(lump, transtables); // FIXME just testing, we can't clobber existing transmaps
      return 0;
    }
  return -1;
}


// semi-hack for linedeftype 242 and the like, where the
// "texture name" field can hold a colormap or a transmap name instead.
int texturecache_t::GetTextureOrColormap(const char *name, int &colmap, bool transmap)
{
  // "NoTexture" marker. No texture, no colormap.
  if (name[0] == '-')
    {
      colmap = -1;
      return 0;
    }

#ifdef HWRENDER
  if (rendermode == render_soft)
    {
#endif
      int temp;
      if (transmap)
        temp = R_TransmapNumForName(name);
      else
        temp = R_ColormapNumForName(name); // check C_*... lists

      if (temp >= 0)
        {
          // it is a trans/colormap lumpname, not a texture
          colmap = temp;
          return 0;
        }
#ifdef HWRENDER
    }
#endif

  // it could be a texture, let's check
  Texture *t = (Texture *)Cache(name);

  if (t == default_item)
    CONS_Printf("Def. texture used for '%s'\n", name);

  colmap = -1;
  return t->id;
}


Texture *texturecache_t::GetPtr(const char *name, bool substitute)
{
  // "NoTexture" marker.
  if (name[0] == '-')
    return NULL;

  // Unfortunate but necessary.
  // Texture names should be NUL-terminated....
  // Also, they should be uppercase (e.g. Doom E1M2)

  char name8[9];
  strncpy(name8, name, 8);
  name8[8] = '\0';
  strupr(name8);

  Texture *t = (Texture *)Cache(name8);
  /*
    FIXME remove! should be already fixed!
  Texture *t2 = (Texture *)Cache(name);
  if (t != t2)
    CONS_Printf("!!!!! '%s', '%s'\n", name8, name);
  */

  if (!substitute && t == default_item)
    return NULL;

  /*
  if (t == default_item)
    CONS_Printf("Def. texture used for '%s'\n", name8);
  */

  return t;
}


Texture *texturecache_t::GetPtrNum(int n)
{
  return GetPtr(fc.FindNameForNum(n));
}


void texturecache_t::Insert(Texture *t)
{
  t->id = texture_ids.size(); // First texture gets the id 1, 'cos zero maps to NULL
  texture_ids[t->id] = t;

  c_iter_t i = c_map.find(t->name);
  if (i != c_map.end())
    {
      Texture *old = (Texture *)i->second;
      CONS_Printf("Texture %s replaced!\n", old->name);
      // A Texture of that name is already there, so the old Texture
      // needs to be renamed and reinserted to the map.
      // Happens when generating animated textures.
      old->name[0] = '!';
      c_map.insert(c_map_t::value_type(old->name, old));
    }

  c_map.insert(c_map_t::value_type(t->name, t));
}



cacheitem_t *texturecache_t::Load(const char *name)
{
  int lump = fc.FindNumForName(name);

  if (lump < 0)
    return NULL;

  // Since there is no texture definition present, we must assume
  // that the user meant a single-lump texture.
  // Because Doom datatypes contain no magic numbers, we have to rely on heuristics...

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
  else if (size ==  64*64 || // normal flats
           size ==  65*64 || // Damn you, Heretic animated flats!
           size == 128*64)   // TODO Some Hexen flats are different! Why?
    {
      // Flat is 64*64 bytes of raw paletted picture data in one lump
      t = new LumpTexture(name, lump, 64, 64);
    }
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
      I_Error(" !!! pic_t '%s' found!\n", name); // root 'em out!
      // likely a pic_t
      // TODO pic_t has an inadequate magic number.
      // pic_t's should be replaced with a better format
    }
  else
    {
      // finally assume a patch_t
      t = new PatchTexture(name, lump);
    }

  t->id = texture_ids.size(); // unique id (used instead of pointers)
  texture_ids[t->id] = t;
  return t;
}



/// Reads the texture definitions from the PNAMES, TEXTURE1 and TEXTURE2 lumps,
/// constructs the corresponding texture objects and inserts them into the cache.

int texturecache_t::ReadTextures()
{
  int i;
  char name[9];

  // Load the patch names from PNAMES
  name[8] = 0;
  int lump = fc.GetNumForName("PNAMES");
  char *pnames = (char *)fc.CacheLumpNum(lump, PU_STATIC);
  int nummappatches = LONG(*((int *)pnames)); // sizeof(int) better be 4
  CONS_Printf("PNAMES: lump %d:%d, %d patches\n", lump >> 16, lump & 0xffff, nummappatches);

  char *name_p = pnames + 4;
  vector<int> patchlookup(nummappatches); // mapping from patchnumber to lumpnumber

  for (i=0 ; i<nummappatches ; i++)
    {
      strncpy(name, name_p + i*8, 8);
      patchlookup[i] = fc.FindNumForName(name);
      if (patchlookup[i] < 0)
        CONS_Printf("patch '%s' (%d) not found!\n", name, i);
    }
  Z_Free(pnames);


  // Load the map texture definitions from textures.lmp.
  // The data is contained in one or two lumps,
  //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
  int  *maptex, *maptex1, *maptex2;

  lump = fc.GetNumForName("TEXTURE1");
  maptex = maptex1 = (int *)fc.CacheLumpNum(lump, PU_STATIC);
  int numtextures1 = LONG(*maptex);
  int maxoff = fc.LumpLength(lump);
  CONS_Printf("TEXTURE1: lump %d:%d, %d textures, maxoff %d\n", lump >> 16, lump & 0xffff, numtextures1, maxoff);
  int *directory = maptex+1;

  int numtextures2, maxoff2;
  if (fc.FindNumForName ("TEXTURE2") != -1)
    {
      lump = fc.GetNumForName("TEXTURE2");
      maptex2 = (int *)fc.CacheLumpNum(lump, PU_STATIC);
      numtextures2 = LONG(*maptex2);
      maxoff2 = fc.LumpLength(lump);
      CONS_Printf("TEXTURE2: lump %d:%d, %d textures, maxoff %d\n", lump >> 16, lump & 0xffff, numtextures2, maxoff2);
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
        I_Error("R_LoadTextures: bad texture directory");

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
            I_Error("R_InitTextures: Missing patch %d in texture %.8s (%d)\n",
                    SHORT(mp->patch), mtex->name, i);
        }

      Insert(tex);
    }

  Z_Free(maptex1);
  if (maptex2)
    Z_Free(maptex2);

  // this is a HACK for "fixing" a nasty Doom1/2 texture/flat namespace overlap.
  if (game.mode < gm_heretic)
    {
      Insert(new LumpTexture("STEP1", fc.FindNumForName("STEP1"), 64, 64));
      Insert(new LumpTexture("STEP2", fc.FindNumForName("STEP2"), 64, 64));
    }

  return numtextures;
}



//==================================================================
//  Colormaps
//==================================================================


//SoM: 4/13/2000: Store lists of lumps for F_START/F_END ect.
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


// lumplist for C_START - C_END pairs in wads
static lumplist_t *colormaplumps;
static int         numcolormaplumps;

static int foundcolormaps[MAXCOLORMAPS]; // colormap number to lumpnumber



/// called in R_Init
void R_InitColormaps()
{
  // Load in the lightlevel colormaps, now 64k aligned for smokie...

  int lump = fc.GetNumForName("COLORMAP");
  colormaps = (lighttable_t *)Z_MallocAlign(fc.LumpLength(lump), PU_STATIC, 0, 16);
  fc.ReadLump(lump, colormaps);

  //SoM: 3/30/2000: Init Boom colormaps.

  num_extra_colormaps = 0;
  memset(extra_colormaps, 0, sizeof(extra_colormaps));

  // build the colormap lumplists (which record the locations of C_START and C_END in each wad)
  numcolormaplumps = 0;
  colormaplumps = NULL;

  int clump = 0;
  int nwads = fc.Size();

  for (int cfile = 0; cfile < nwads; cfile++, clump = 0)
    {
      int startnum = fc.FindNumForNameFile("C_START", cfile, clump);
      if (startnum == -1)
        continue; // required

      int endnum = fc.FindNumForNameFile("C_END", cfile, clump);
      if (endnum == -1)
        I_Error("R_InitColormaps: C_START without C_END\n");

      if ((startnum >> 16) != (endnum >> 16))
        I_Error("R_InitColormaps: C_START and C_END in different wad files!\n"); // cannot happen...

      colormaplumps = (lumplist_t *)realloc(colormaplumps, sizeof(lumplist_t) * (numcolormaplumps + 1));
      colormaplumps[numcolormaplumps].wadfile = startnum >> 16;
      colormaplumps[numcolormaplumps].firstlump = (startnum & 0xFFFF) + 1; // after C_START
      colormaplumps[numcolormaplumps].numlumps = endnum - (startnum + 1);
      numcolormaplumps++;
    }

  // TODO Hexen maps can have custom lightmaps ("fadetable" MAPINFO command)... just a note.
}




//SoM: Clears out extra colormaps between levels.
void R_ClearColormaps()
{
  num_extra_colormaps = 0;
  for (int i = 0; i < MAXCOLORMAPS; i++)
    foundcolormaps[i] = -1;
  memset(extra_colormaps, 0, sizeof(extra_colormaps));
}



// tries to retrieve a colormap by name. returns -1 if unsuccesful.
int R_ColormapNumForName(const char *name)
{
  int lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
  if (lump == -1)
    return -1;

  for (int i = 0; i < num_extra_colormaps; i++)
    if (lump == foundcolormaps[i])
      return i;

  if (num_extra_colormaps == MAXCOLORMAPS)
    I_Error("R_ColormapNumForName: Too many colormaps!\n");

  foundcolormaps[num_extra_colormaps] = lump;

  // aligned on 8 bit for asm code
  extra_colormaps[num_extra_colormaps].colormap = (lighttable_t *)Z_MallocAlign (fc.LumpLength (lump), PU_LEVEL, 0, 8);
  fc.ReadLump(lump, extra_colormaps[num_extra_colormaps].colormap);

  // SoM: Added, we set all params of the colormap to normal because there
  // is no real way to tell how GL should handle a colormap lump anyway..
  extra_colormaps[num_extra_colormaps].maskcolor = 0xffff;
  extra_colormaps[num_extra_colormaps].fadecolor = 0x0;
  extra_colormaps[num_extra_colormaps].maskamt = 0x0;
  extra_colormaps[num_extra_colormaps].fadestart = 0;
  extra_colormaps[num_extra_colormaps].fadeend = 33;
  extra_colormaps[num_extra_colormaps].fog = 0;

  return num_extra_colormaps++;
}



const char *R_ColormapNameForNum(int num)
{
  if (num == -1)
    return "NONE";

  if (num < 0 || num >= MAXCOLORMAPS)
    I_Error("R_ColormapNameForNum: num is invalid!\n");

  if (foundcolormaps[num] == -1)
    return "INLEVEL";

  return fc.FindNameForNum(foundcolormaps[num]);
}





// Rounds off floating numbers and checks for 0 - 255 bounds
int RoundUp(double number)
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
double  deltas[256][3], cmap[256][3];

int R_CreateColormap(char *p1, char *p2, char *p3)
{
  double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
  double r, g, b;
  double cbrightness;
  double maskamt = 0, othermask = 0;
  int    i, p;
  char   *colormap_p;
  unsigned int  cr, cg, cb;
  unsigned int  maskcolor, fadecolor;
  unsigned int  fadestart = 0, fadeend = 33, fadedist = 33;
  int           fog = 0;
  int           mapnum = num_extra_colormaps;

#define HEX2INT(x) (x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
  if(p1[0] == '#')
    {
      cmaskr = cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
      cmaskg = cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
      cmaskb = cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));
      // Create a rough approximation of the color (a 16 bit color)
      maskcolor = ((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11);
      if (p1[7] >= 'a' && p1[7] <= 'z')
        maskamt = (p1[7] - 'a');
      else if (p1[7] >= 'A' && p1[7] <= 'Z')
        maskamt = (p1[7] - 'A');

      maskamt /= (double)24;

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
      maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
    }


#define NUMFROMCHAR(c)  (c >= '0' && c <= '9' ? c - '0' : 0)
  if(p2[0] == '#')
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


  if(p3[0] == '#')
    {
      cdestr = cr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
      cdestg = cg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
      cdestb = cb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));
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


/*  for(i = 0; i < num_extra_colormaps; i++)
  {
    if(foundcolormaps[i] != -1)
      continue;
    if(maskcolor == extra_colormaps[i].maskcolor &&
       fadecolor == extra_colormaps[i].fadecolor &&
       maskamt == extra_colormaps[i].maskamt &&
       fadestart == extra_colormaps[i].fadestart &&
       fadeend == extra_colormaps[i].fadeend &&
       fog == extra_colormaps[i].fog)
      return i;
  }*/

  if(num_extra_colormaps == MAXCOLORMAPS)
    I_Error("R_CreateColormap: Too many colormaps!\n");
  num_extra_colormaps++;

#ifdef HWRENDER
  // TODO: Hurdler: see why we need to have a separate code here
  if(rendermode == render_soft)
#endif
  {
    for(i = 0; i < 256; i++)
    {
      r = vid.palette[i].red;
      g = vid.palette[i].green;
      b = vid.palette[i].blue;
      cbrightness = sqrt((r*r) + (g*g) + (b*b));

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
  }

  foundcolormaps[mapnum] = -1;

  // aligned on 8 bit for asm code
  extra_colormaps[mapnum].colormap = NULL;
  extra_colormaps[mapnum].maskcolor = maskcolor;
  extra_colormaps[mapnum].fadecolor = fadecolor;
  extra_colormaps[mapnum].maskamt = maskamt;
  extra_colormaps[mapnum].fadestart = fadestart;
  extra_colormaps[mapnum].fadeend = fadeend;
  extra_colormaps[mapnum].fog = fog;

#define ABS2(x) (x) < 0 ? -(x) : (x)
#ifdef HWRENDER
  // TODO: Hurdler: see why we need to have a separate code here
  if(rendermode == render_soft)
#endif
  {
    colormap_p = (char *)Z_MallocAlign ((256 * 34) + 1, PU_LEVEL, 0, 8);
    extra_colormaps[mapnum].colormap = (lighttable_t *)colormap_p;

    for(p = 0; p < 34; p++)
    {
      for(i = 0; i < 256; i++)
      {
        *colormap_p = NearestColor(RoundUp(cmap[i][0]), RoundUp(cmap[i][1]), RoundUp(cmap[i][2]));
        colormap_p++;

        if((unsigned int)p < fadestart)
          continue;

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
      }
    }
  }
#undef ABS2

  return mapnum;
}



//
//  build a table for quick conversion from 8bpp to 15bpp
//
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
// FIXME Preloads all relevant graphics for the Map.
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
    /*  FIXME
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
