// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// FileCache: Interface to the WadFile class
//
//-----------------------------------------------------------------------------

#include <string>
#include <vector>

#ifndef __WIN32__
# include <unistd.h>
#endif

#include "d_netfil.h" // nameonly()
#include "z_zone.h"
#include "i_video.h" //rendermode
//#include "r_defs.h" // pic_t
#include "byteptr.h"

#ifdef HWRENDER
# include "hardware/hw_main.h"
#endif

#ifdef LINUX // prototypes, hopefully correct
char *strupr(char *n); // from dosstr.c
char *strlwr(char *n); // from dosstr.c
#endif

#include "w_wad.h"
#include "wad.h" // WadFile class


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
  for (int i = wadfiles.size()-1; i>=0; i--)
    {
      delete wadfiles[i];
    }
  wadfiles.clear();
  return;
}


//===========
// opening wad files


void FileCache::SetPath(const char *p)
{
  wadpath = p;
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

  for ( ; *filenames != NULL; filenames++) {
    if (LoadWadFile(*filenames) == -1) result = false;
  }

  if (wadfiles.size() == 0)
    I_Error("FileCache::InitMultipleFiles: no files found");

  // result = false : at least one file was missing
  return result;
}

// -----------------------------------------------------
// was W_LoadWadFile
// Loads a wad file into memory, adds it into wadfiles vector.
// returns the number of the wadfile (0 is the first one)
// or -1 in the case of error

int FileCache::LoadWadFile(const char *fname)
{
  int nwadfiles = wadfiles.size();
  
  if (nwadfiles >= MAX_WADFILES)
    {
      CONS_Printf("Maximum number of wad files reached\n");
      return -1;
    }

  string path = wadpath + '/' + fname;

  FILE *str = fopen(path.c_str(), "rb");
  if (!str)
    {
      str = fopen(fname, "rb");
      if (!str)
	{
	  CONS_Printf("FileCache::LoadWadFile: Can't open file %s (path %s)\n", fname, wadpath.c_str());
	  return -1;
	}
    }

  WadFile *wf = NULL;

  // detect dehacked file with the "deh" extension
  if (!stricmp(&fname[strlen(fname) - 3], "deh"))
    {
      // this code emulates a wadfile with one lump name "DEHACKED" 
      // at position 0 and size of the whole file
      // this allows deh files to be treated like wad files,
      // copied by network and loaded at the console

      Wad *p = new Wad;
      p->stream = str;
      p->header.numentries = 1; 
      p->directory = (waddir_t *)Z_Malloc(sizeof(waddir_t), PU_STATIC, NULL);
      p->directory->offset = 0;

      // get file system info about the file
      struct stat bufstat;
      fstat(fileno(str), &bufstat);
      p->directory->size = bufstat.st_size;
      strcpy(p->directory->name, "DEHACKED");
      wf = p;
    }
  else 
    {
      char magic[4];
      fread(magic, 4, 1, str);

      // check file type
      if (!strncmp(magic, "IWAD", 4) || !strncmp(magic, "PWAD", 4))
	{
	  Wad *p = new Wad;
	  p->Load(fname, str, nwadfiles);
	  wf = p;
	}
      else if (!strncmp(magic, "WAD2", 4) || !strncmp(magic, "WAD3", 4))
	{
	  Wad3 *p = new Wad3;
	  p->Load(fname, str);
	  wf = p;
	}
      else 
	{
	  if (!strncmp(magic, "PACK", 4))
	    CONS_Printf("PAK files are not yet implemented!\n");
	  else
	    CONS_Printf("FileCache::LoadWadFile: Unknown file signature %4c\n", magic);
	  fclose(str);
	  return -1;
	}
    }
  // *.PK3 magic number is recognized by zlib 

  // This is a less widely known property of C++.
  // First we allocate space for one WadFile class instance on the internal zone heap
  //void *temp = Z_Malloc(sizeof(WadFile), PU_STATIC, NULL);
  // then, we create a new instance of WadFile class _into_ the allocated space!
  // The new operator can be given an extra parameter denoting the address to be used.
  // If this is the case, no new memory is allocated, hopefully!?!
  //newfile = new(temp) WadFile(filename, nwadfiles);

  // of course, it's better to do this by just redefining
  // new and delete for the WadFile class ;)

  wadfiles.push_back(wf);
  return nwadfiles;
}


//=================
// basic info on open wadfiles


// -----------------------------------------------------
// size
// returns the number of open wadfiles

int FileCache::Size()
{
  return wadfiles.size();
}

// -----------------------------------------------------
// was W_LumpLength
// Returns the buffer size needed to load the given lump.

int FileCache::LumpLength(int lump)
{
  int llump = lump & 0xffff;
  unsigned int lfile = lump >> 16;
  
  if (lfile >= wadfiles.size())
    I_Error("FileCache::LumpLength: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::LumpLength: %i >= numlumps", llump);

  return wadfiles[lfile]->GetLumpSize(llump);
}

// was W_Name

const char *FileCache::Name(int i)
{
  return wadfiles[i]->filename.c_str();
}

// was W_md5

const byte *FileCache::md5(int i)
{
  return wadfiles[i]->md5sum;
}

//was W_GetLumpinfo

waddir_t *FileCache::GetLumpinfo(int wadnum)
{
  return wadfiles[wadnum]->GetDirectory();
}

//was W_GetNumLumps

unsigned int FileCache::GetNumLumps(int wadnum)
{
  return wadfiles[wadnum]->header.numentries;
}

// was W_WriteFileHeaders

void FileCache::WriteFileHeaders(byte *p)
{
  unsigned int i;
  char wadfilename[MAX_WADPATH];

  for(i=0; i<wadfiles.size(); i++)
    {
      WRITEULONG(p, wadfiles[i]->filesize);
      strcpy(wadfilename, wadfiles[i]->filename.c_str());
      nameonly(wadfilename);
      WRITESTRING(p, wadfilename);
      WRITEMEM(p, wadfiles[i]->md5sum, 16);
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
  int i, n = wadfiles.size();
  int res;

  if (!scanforward) {
    // scan wad files backwards so patch lump files take precedence
    for (i = n-1; i >= 0; i--) {
      res = wadfiles[i]->FindNumForName(name);
      // high word is the wad file number, low word the lump number
      if (res != -1) return ((i<<16) + res);
    }
  } else {  
    // scan wad files forward, when original wad resources
    //  must take precedence
    for (i = 0; i<n; i++) {
      res = wadfiles[i]->FindNumForName(name);
      if (res != -1) return ((i<<16) + res);
    }
  }  
  // not found.
  return -1;
}


// -----------------------------------------------------
// was W_CheckNumForNamePwad
// Same as the original, but checks in one wad only
// 'wadnum' is a wad number (Used for sprites loading)
// 'startlump' is the lump number to start the search

int FileCache::FindNumForNamePwad(const char* name, int wadnum, int startlump)
{
  // start at 'startlump', useful parameter when there are multiple
  // resources with the same name

  if (wadnum >= (int)wadfiles.size())
    I_Error("FileCache::FindNumForNamePwad: %i >= numwadfiles(%i)\n", wadnum, wadfiles.size());
 
  int res = wadfiles[wadnum]->FindNumForName(name, startlump);

  if (res != -1) return ((wadnum<<16) + res);
  
  // not found.
  return -1;
}

// -----------------------------------------------------
// was W_FindNameForNum

const char *FileCache::FindNameForNum(int lump)
{
  unsigned int lfile = lump >> 16;
  int llump = lump & 0xffff;

  if (lfile >= wadfiles.size())
    I_Error("FileCache::FindNameForNum: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::FindNameForNum: %i >= numlumps", llump);

  return wadfiles[lfile]->GetLumpName(llump);
}

// -----------------------------------------------------
// was W_GetNumForName, W_GetNumForNameFirst (scanforw == true)
// Calls FindNumForName, but bombs out if not found.

int FileCache::GetNumForName(const char* name, bool scanforward)
{
  int i;
  
  i = FindNumForName(name, scanforward);
  if (i == -1)
    I_Error("FileCache::GetNumForName: %s not found!\n", name);
  
  return i;
}


//=============
// reading and caching lumps

// -----------------------------------------------------
//  was W_ReadLump
//  Loads the lump into the given buffer,
//  which must be >= W_LumpLength().

// -----------------------------------------------------
// was W_ReadLumpHeader
// read 'size' bytes of lump. If size == 0, read the entire lump
// sometimes just the header is needed

int FileCache::ReadLumpHeader(int lump, void *dest, int size)
{
  int llump = lump & 0xffff;
  unsigned int lfile = lump >> 16;

  if (lfile >= wadfiles.size())
    I_Error("FileCache::ReadLumpHeader: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::ReadLumpHeader: %i >= numlumps", llump);

  return wadfiles[lfile]->ReadLumpHeader(llump, dest, size);
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
  int llump = lump & 0xffff;
  unsigned int lfile = lump >> 16;

  if (lfile >= wadfiles.size())
    I_Error("FileCache::CacheLumpNum: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::CacheLumpNum: %i >= numlumps", llump);

  lumpcache_t *lcache = wadfiles[lfile]->cache;
  if (!lcache[llump])
    {
      // read the lump in    
      //CONS_Printf("cache miss on lump %i\n",lump);
      // looks frightening, but actually Z_Malloc sets lcache[llump] to point to the memory
      //byte *ptr = (byte *)
      Z_Malloc(wadfiles[lfile]->GetLumpSize(llump), tag, &lcache[llump]);
      wadfiles[lfile]->ReadLumpHeader(llump, lcache[llump], 0);
    }
  else
    {
      //CONS_Printf("cache hit on lump %i\n",lump);
      Z_ChangeTag(lcache[llump], tag);
    }

  return lcache[llump];
}


// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

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

  int llump = lump & 0xffff;
  unsigned int lfile = lump >> 16;

  if (lfile >= wadfiles.size())
    I_Error("FileCache::CachePatchNum: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::CachePatchNum: %i >= numlumps", llump);


  GlidePatch_t *grPatch = &(wadfiles[lfile]->hwrcache[llump]);

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
  int llump = lump & 0xffff;
  unsigned int lfile = lump >> 16;

  if (lfile >= wadfiles.size())
    I_Error("FileCache::CacheRawAsPic: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::CacheRawAsPic: %i >= numlumps", llump);

  pic_t *pic;

  lumpcache_t *lcache = wadfiles[lfile]->cache;
  if (!lcache[llump])
    {
      // read the lump in
      pic = (pic_t *)Z_Malloc(wadfiles[lfile]->GetLumpSize(llump) + sizeof(pic_t), tag, &lcache[llump]);
      wadfiles[lfile]->ReadLumpHeader(llump, pic->data, 0);
      pic->width = SHORT(width);
      pic->height = SHORT(height);
      pic->mode = PALETTE;
    }
  else 
    Z_ChangeTag(lcache[llump], tag);

  return (pic_t *)lcache[llump];
}

//-----------------------------------------------------
// was W_GetHWRNum

GlidePatch_t *FileCache::GetHWRNum(int lump)
{
  unsigned int lfile = lump >> 16;
  int llump = lump & 0xffff;

  if (lfile >= wadfiles.size())
    I_Error("FileCache::CacheHWRNum: %i >= numwadfiles(%i)\n", lfile, wadfiles.size());
  if (llump >= wadfiles[lfile]->header.numentries)
    I_Error("FileCache::CacheHWRNum: %i >= numlumps", llump);

  return &(wadfiles[lfile]->hwrcache[llump]);
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
  unsigned int numlumps = wadfiles[wadnum]->numlumps;
  lumpinfo_t *lp = wadfiles[wadnum]->lumpinfo;
  char *name;
  // search for sound replacements

  int num, firstmapreplaced = 0;

  // this crap doesn't recognize Heretic sounds! What on Earth am I doing??
  for (i=0; i<numlumps; i++, lp++) {
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

  for (j=0; j<wadfiles.size(); j++)
    for (i=0; i<wadfiles[j]->header.numentries; i++) {
      GlidePatch_t *grpatch = &(wadfiles[j]->hwrcache[i]);
      while (grpatch->mipmap.nextcolormap)
	{
	  GlideMipmap_t *grmip = grpatch->mipmap.nextcolormap;
	  grpatch->mipmap.nextcolormap = grmip->nextcolormap;
	  free(grmip);
	}
    }
}

