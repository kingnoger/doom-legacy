// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// DESCRIPTION:
// FileCache: Container of VFiles
//
//-----------------------------------------------------------------------------

#include <string>
#include <vector>

#ifndef __WIN32__
# include <unistd.h>
#endif

#include "d_netfil.h" // nameonly()
#include "byteptr.h"
#include "i_video.h" //rendermode

#ifdef HWRENDER
# include "hardware/hw_main.h"
#endif

#include "vfile.h" // VFile class
#include "wad.h"
#include "w_wad.h"
#include "z_zone.h"


// ====================================================================
// FileCache class implementation
//
// this replaces all W_* functions of old Doom code. W_* -> fc.*

FileCache fc;


// -----------------------------------------------------
// destructor, was W_ShutDown
// destroys (closes) all open WadFiles
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected

FileCache::~FileCache()
{
  for (int i = vfiles.size()-1; i>=0; i--)
    {
      delete vfiles[i];
    }
  vfiles.clear();
  return;
}


//===========
// opening wad files


void FileCache::SetPath(const char *p)
{
  datapath = p;
}

// -----------------------------------------------------
// was W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file must be found.
// Files with a .wad extension are idlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file does override all earlier ones.

bool FileCache::InitMultipleFiles(const char *const*filenames)
{
  bool result = true;

  for ( ; *filenames != NULL; filenames++)
    {
      if (AddFile(*filenames) == -1)
	result = false;
    }

  if (vfiles.size() == 0)
    I_Error("FileCache::InitMultipleFiles: no files found");

  // result = false : at least one file was missing
  return result;
}

// -----------------------------------------------------
// was W_LoadWadFile
// Loads a wad file into memory, adds it into vfiles vector.
// returns the number of the wadfile (0 is the first one)
// or -1 in the case of error

int FileCache::AddFile(const char *fname)
{
  int nfiles = vfiles.size();
  
  if (nfiles >= MAX_WADFILES)
    {
      CONS_Printf("Maximum number of resource files reached\n");
      return -1;
    }

  string name = datapath + '/' + fname;

  if (access(name.c_str(), F_OK))
    {
      name = fname; // try just the name
      if (access(fname, F_OK))
	{
	  CONS_Printf("FileCache::AddFile: Can't access file %s (path %s)\n", fname, datapath.c_str());
	  return -1;
	}
    }

  VFile *vf = NULL;
  int l = strlen(fname);

  if (!stricmp(&fname[l - 4], ".deh"))
    {
      // detect dehacked file with the "deh" extension
      vf = new Wad(name);
    }
  else if (fname[l-1] == '/')
    {
      // directory
      vf = new VDir();
      vf->Open(name.c_str());
    }
  else
    {
      union
      {
	char magic[5];
	int  imagic;
      };
      // open the file to read magic number
      FILE *str = fopen(name.c_str(), "rb");
      if (!str)
	return -1;

      fread(magic, 4, 1, str);
      magic[4] = 0;
      
      // check file type
      if (imagic == *reinterpret_cast<const int *>("IWAD")
	  || imagic == *reinterpret_cast<const int *>("PWAD"))
	{
	  vf = new Wad();
	  extern int file_num;
	  file_num = nfiles; // HACK
	}
      else if (imagic == *reinterpret_cast<const int *>("WAD2")
	       || imagic == *reinterpret_cast<const int *>("WAD3"))
	{
	  vf = new Wad3();
	}
      else if (imagic == *reinterpret_cast<const int *>("PACK"))
	{
	  vf = new Pak();
	}
      else
	{
	  // TODO PK3 magic number is recognized by zlib?
	  CONS_Printf("FileCache::AddFile: Unknown file signature %4c\n", magic);
	  fclose(str);
	  return -1;
	}
      fclose(str);
      vf->Open(name.c_str());
    }

  vfiles.push_back(vf);
  return nfiles;
}


//=================
// basic info on open wadfiles


// -----------------------------------------------------
// was W_LumpLength
// Returns the buffer size needed to load the given lump.

int FileCache::LumpLength(int lump)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;
  
  if (file >= vfiles.size())
    I_Error("FileCache::LumpLength: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::LumpLength: %i >= numitems", item);

  return vfiles[file]->GetItemSize(item);
}

// was W_Name
const char *FileCache::Name(int i)
{
  return vfiles[i]->filename.c_str();
}

// was W_md5
bool FileCache::GetNetworkInfo(int i, int *size, unsigned char *md5)
{
  return vfiles[i]->GetNetworkInfo(size, md5);
}

//was W_GetNumLumps
unsigned int FileCache::GetNumLumps(int filenum)
{
  return vfiles[filenum]->numitems;
}

// was W_WriteFileHeaders
void FileCache::WriteFileHeaders(byte *p)
{
  unsigned int i;
  char wadfilename[MAX_WADPATH];

  int size;
  unsigned char md5[16];
  for(i=0; i<vfiles.size(); i++)
    {
      if (vfiles[i]->GetNetworkInfo(&size, md5))
	{
	  WRITEULONG(p, size);
	  strcpy(wadfilename, vfiles[i]->filename.c_str());
	  nameonly(wadfilename);
	  WRITESTRING(p, wadfilename);
	  WRITEMEM(p, md5, 16);
	}
    }
}


//==============
// searching 

// -----------------------------------------------------
// was W_CheckNumForName, W_CheckNumForNameFirst (scanforward == true)
// Returns -1 if name not found.
// scanforward: this is normally always false, so external pwads take precedence,
// this is set true if W_GetNumForNameFirst() is called

int FileCache::FindNumForName(const char* name, bool scanforward)
{
  int i, n = vfiles.size();
  int res;

  if (!scanforward) {
    // scan wad files backwards so patch lump files take precedence
    for (i = n-1; i >= 0; i--) {
      res = vfiles[i]->FindNumForName(name);
      // high word is the wad file number, low word the lump number
      if (res != -1) return ((i<<16) + res);
    }
  } else {  
    // scan wad files forward, when original wad resources
    //  must take precedence
    for (i = 0; i<n; i++) {
      res = vfiles[i]->FindNumForName(name);
      if (res != -1) return ((i<<16) + res);
    }
  }  
  // not found.
  return -1;
}


// -----------------------------------------------------
// was W_CheckNumForNamePwad
// Same as the original, but checks in one wad only
// 'filenum' is a wad number (Used for sprites loading)
// 'startlump' is the lump number to start the search

int FileCache::FindNumForNameFile(const char* name, unsigned filenum, int startlump)
{
  // start at 'startlump', useful parameter when there are multiple
  // resources with the same name

  if (filenum >= vfiles.size())
    I_Error("FileCache::FindNumForNamePwad: %i >= numvfiles(%i)\n", filenum, vfiles.size());
 
  int res = vfiles[filenum]->FindNumForName(name, startlump);

  if (res != -1) return ((filenum<<16) + res);
  
  // not found.
  return -1;
}

// -----------------------------------------------------
// was W_FindNameForNum

const char *FileCache::FindNameForNum(int lump)
{
  unsigned int file = lump >> 16;
  int item = lump & 0xffff;

  if (file >= vfiles.size())
    I_Error("FileCache::FindNameForNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::FindNameForNum: %i >= numitems", item);
    //return NULL;

  return vfiles[file]->GetItemName(item);
}

// -----------------------------------------------------
// was W_GetNumForName, W_GetNumForNameFirst (scanforw == true)
// Calls FindNumForName, but bombs out if not found.

int FileCache::GetNumForName(const char *name, bool scanforward)
{
  int i = FindNumForName(name, scanforward);
  if (i == -1)
    I_Error("FileCache::GetNumForName: %s not found!\n", name);
  
  return i;
}

int FileCache::FindPartialName(int iname, unsigned filenum, int startlump, const char **fullname)
{
  if (filenum >= vfiles.size())
    I_Error("FileCache::FindNumForNamePwad: %i >= numvfiles(%i)\n", filenum, vfiles.size());
 
  return vfiles[filenum]->FindPartialName(iname, startlump, fullname);
}


//================================
//    reading and caching

// -----------------------------------------------------
// was W_ReadLumpHeader
// read 'size' bytes of lump. If size == 0, read the entire lump
// sometimes just the header is needed

int FileCache::ReadLumpHeader(int lump, void *dest, int size)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;

  if (file >= vfiles.size())
    I_Error("FileCache::ReadLumpHeader: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::ReadLumpHeader: %i >= numitems", item);

  return vfiles[file]->ReadItemHeader(item, dest, size);
}

// -----------------------------------------------------
// was W_CacheLumpName

void *FileCache::CacheLumpName(const char *name, int tag)
{
  return CacheLumpNum(GetNumForName(name), tag);
}


// -----------------------------------------------------
// was W_CacheLumpNum

void *FileCache::CacheLumpNum(int lump, int tag)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;

  if (file >= vfiles.size())
    I_Error("FileCache::CacheLumpNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::CacheLumpNum: %i >= numitems", item);

  return vfiles[file]->CacheItem(item, tag);
}


// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================
// FIXME TODO replace all this with texture L2 cache

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current rendermode. For software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, the
// heap memory cache then act as a 'level 2' cache just after the graphics
// card memory.



// -----------------------------------------------------
// was W_CachePatchName

patch_t *FileCache::CachePatchName(const char* name, int tag)
{
  int i = FindNumForName(name, false);
  if (i < 0) // not found, FIXME! cache some default patch from legacy.wad!
    return CachePatchNum(GetNumForName("BRDR_MM"), tag);
  return CachePatchNum(i, tag);
}


// -----------------------------------------------------
// was W_CachePatchNum
// Cache a patch into heap memory, convert the patch format as necessary
// Software-only compile cache the data without conversion

#ifdef HWRENDER
patch_t *FileCache::CachePatchNum(int lump, int tag)
{
  if (rendermode == render_soft)
    return (patch_t*)CacheLumpNum(lump, tag);

  int item = lump & 0xffff;
  unsigned int file = lump >> 16;

  if (file >= vfiles.size())
    I_Error("FileCache::CachePatchNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::CachePatchNum: %i >= numitems", item);


  GlidePatch_t *grPatch = &(vfiles[file]->hwrcache[item]);

  if (grPatch->mipmap.grInfo.data) 
    {   
      if (tag == PU_CACHE)
	tag = PU_HWRCACHE;
      Z_ChangeTag(grPatch->mipmap.grInfo.data, tag); 
    }
  else
    { // first time init grPatch fields
      // we need patch w,h,offset,...
      // well this code will be executed latter in GetPatch, anyway 
      // do it now ...
      patch_t *ptr = (patch_t *)CacheLumpNum(grPatch->patchlump, PU_STATIC);
      HWR_MakePatch(ptr, grPatch, &grPatch->mipmap);
      Z_Free(ptr); 
      //Hurdler: why not do a Z_ChangeTag(grPatch->mipmap.grInfo.data, tag) here?
      //BP: mmm, yes we can...
    }

  // return GlidePatch_t, which can be casted to (patch_t) with valid patch header info
  return (patch_t *)grPatch;
}
#endif // HWRENDER


// -----------------------------------------------------
// was W_CacheRawAsPic
// convert raw heretic picture to legacy pic_t format

pic_t *FileCache::CacheRawAsPic(int lump, int width, int height, int tag)
{
  int item = lump & 0xffff;
  unsigned int file = lump >> 16;

  if (file >= vfiles.size())
    I_Error("FileCache::CacheRawAsPic: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::CacheRawAsPic: %i >= numitems", item);

  pic_t *pic;

  lumpcache_t *lcache = vfiles[file]->cache;
  if (!lcache[item])
    {
      // read the lump in
      pic = (pic_t *)Z_Malloc(vfiles[file]->GetItemSize(item) + sizeof(pic_t), tag, &lcache[item]);
      vfiles[file]->ReadItemHeader(item, pic->data, 0);
      pic->width = SHORT(width);
      pic->height = SHORT(height);
      pic->mode = PALETTE;
    }
  else 
    Z_ChangeTag(lcache[item], tag);

  return (pic_t *)lcache[item];
}

//-----------------------------------------------------
// was W_GetHWRNum

GlidePatch_t *FileCache::GetHWRNum(int lump)
{
  unsigned int file = lump >> 16;
  int item = lump & 0xffff;

  if (file >= vfiles.size())
    I_Error("FileCache::CacheHWRNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    I_Error("FileCache::CacheHWRNum: %i >= numitems", item);

  return &(vfiles[file]->hwrcache[item]);
}



//========================
// other cache functions


// -----------------------------------------------------
// Replace, crappy idea, FIXME!, was W_Replace
// called when a wadfile is loaded after the game has been started (map xxx.wad)
// invalidates cached data which is to be replaced so it will
// be recached the next time it is used

/*
#include "info.h"

int FileCache::Replace(unsigned int wadnum, char **firstmapname)
{
  void S_ReplaceSound(char *name);
  void R_AddSpriteDefs(char** namelist, int wadnum);
  void R_LoadTextures(void);
  void R_FlushTextureCache(void);
  void R_AddSkins(int wadnum);

  unsigned int i;
  int r1 = 0, r2 = 0;
  bool texturechange = false;
  unsigned int numitems = vfiles[wadnum]->numitems;
  lumpinfo_t *lp = vfiles[wadnum]->lumpinfo;
  char *name;
  // search for sound replacements

  int num, firstmapreplaced = 0;

  // this crap doesn't recognize Heretic sounds! What on Earth am I doing??
  for (i=0; i<numitems; i++, lp++) {
    name = lp->name;
    num = firstmapreplaced;
    if (name[0]=='D' && name[1]=='S') {
      S_ReplaceSound(name);
      r1++;
    }
    else if (name[0]=='D' && name[1]=='_') {
      r2++;
    }
    // search for maps
    //if (game.mode==commercial) etc., not really necessary?
    else if (name[0]=='M' && name[1]=='A' && name[2]=='P'){
      num = (name[3]-'0')*10 + (name[4]-'0');
      CONS_Printf ("Map %d\n", num);
    }
    else if (name[0]=='E' && ((unsigned)name[1]-'0')<='9' &&   // a digit
	     name[2]=='M' && ((unsigned)name[3]-'0')<='9' &&
	     name[4]==0) {
      num = ((name[1]-'0')<<16) + (name[3]-'0');
      CONS_Printf ("Episode %d map %d\n", name[1]-'0',
		   name[3]-'0');
    }
    else if (memcmp(name,"TEXTURE1",8)==0    // find texture replacement too
	     || memcmp(name,"TEXTURE2",8)==0
	     || memcmp(name,"PNAMES",6)==0)
      texturechange = true;

    if (num && (num < firstmapreplaced || !firstmapreplaced))
      {
	firstmapreplaced = num;
	if (firstmapname != NULL) *firstmapname = name;
      } 
  }

  //if (!devparm && (r1 || r2)) CONS_Printf ("%d sounds, %d musics replaced\n", r1, r2);

  // search for sprite replacements
  R_AddSpriteDefs(sprnames, wadnum);

  // search for texturechange replacements
  if (texturechange) // inited in the sound check
    R_LoadTextures();       // numtexture changes
  else
    R_FlushTextureCache();  // just reload it from file
  
  // look for skins
  R_AddSkins (wadnum);

  if (!firstmapreplaced)
    CONS_Printf ("no maps added\n");
  return 0;
}
*/


//
// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
bool P_AddWadFile(char* wadfilename, char **firstmapname)
{
  // FIXME if a new wadfile is added to the resource list,
  // the entire cache should be purged instead of fc.Replace !

  /*
  //int         firstmapreplaced;
  //wadfile_t*  wadfile;
  //char*       name;
  int         wadfilenum;
  //lumpinfo_t* lumpinfo;
  //int         replaces;
  //bool     texturechange;

  if ((wadfilenum = fc.LoadWadFile(wadfilename)) == -1)
    {
      CONS_Printf ("couldn't load wad file %s\n", wadfilename);
      return false;
    }
  
  fc.Replace(wadfilenum, firstmapname);

  // reload status bar (warning should have valide player !)
  if (game.state == GS_LEVEL)
    ST_Start();
  */
  return true;
}



// -----------------------------------------------------
//was W_FreeTextureCache

void FileCache::FreeTextureCache()
{
  unsigned int j;
  int i;

  for (j=0; j<vfiles.size(); j++)
    for (i=0; i<vfiles[j]->numitems; i++) {
      GlidePatch_t *grpatch = &(vfiles[j]->hwrcache[i]);
      while (grpatch->mipmap.nextcolormap)
	{
	  GlideMipmap_t *grmip = grpatch->mipmap.nextcolormap;
	  grpatch->mipmap.nextcolormap = grmip->nextcolormap;
	  free(grmip);
	}
    }
}
