// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002 by DooM Legacy Team.
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
//   FileCache class definition
//
//-----------------------------------------------------------------------------


#ifndef w_wad_h
#define w_wad_h 1

#include <vector>
#include <string>

#ifdef HWRENDER
# include "hardware/hw_data.h"
#else
typedef void GlidePatch_t;
#endif

#include "v_video.h" // patch_t, pic_t

using namespace std;

class WadFile;
struct waddir_t;

//SoM: 4/13/2000: Store lists of lumps for F_START/F_END ect.
struct lumplist_t
{
  int         wadfile;
  int         firstlump;
  int         numlumps;
};

// ========================================================================
// class FileCache
// A class for holding open wad (and maybe other) files and giving out information about them
// This class also handles game dependent operations concerning wad files

class FileCache
{
private:
  string wadpath; // absolute path for searching wadfiles
  vector<WadFile *> wadfiles;    // open .wad files

public:
  ~FileCache();

  // set wad path
  void SetPath(const char *p);

  // loads one wadfile, returns -1 on error
  int LoadWadFile(const char *filename);
  // Loads multiple wadfiles.
  // returns true if everything is OK, false if at least one file was missing
  bool InitMultipleFiles(const char *const*filenames);

  // basic info about open wadfiles
  int Size();
  int LumpLength(int lump);
  const char *Name(int i);
  const byte *md5(int i);
  waddir_t *GetLumpinfo(int wadnum);
  unsigned int GetNumLumps(int wadnum);
  void WriteFileHeaders(byte *p);

  // searching: Find gives -1 if not found, Get gives an error.
  int FindNumForName(const char *name, bool scanforward = false);
  int FindNumForNamePwad(const char *name, int wadnum, int startlump);
  const char *FindNameForNum(int lump);
  int GetNumForName(const char* name, bool scanforward = false);

  // reading and caching lumps
  inline int ReadLump(int lump, void *dest) { return ReadLumpHeader(lump, dest, 0); };
  int ReadLumpHeader(int lump, void *dest, int size);
  void *CacheLumpName(const char* name, int tag);
  void *CacheLumpNum(int lump, int tag);

  // caching graphics
  patch_t *CachePatchName(const char* name, int tag);
#ifdef HWRENDER
  patch_t *CachePatchNum(int lump, int tag);
#else
# define CachePatchNum(lump, tag)  (patch_t*)CacheLumpNum((lump), (tag))
#endif
  pic_t *CacheRawAsPic(int lump, int width, int height, int tag);
  GlidePatch_t *GetHWRNum(int lump);

  // other stuff
  //int Replace(unsigned int wadnum, char **firstmapname);
  void FreeTextureCache();
};

extern FileCache fc;

//#define WADFILENUM(lump)       (lump>>16)   // wad file number in upper word
//#define LUMPNUM(lump)          (lump&0xffff)    // lump number in lower word


bool P_AddWadFile (char* wadfilename,char **firstmapname);

#define MAX_WADPATH   128
#define MAX_WADFILES  32       // maximum of wad files used at the same time
                               // (there is a max of simultaneous open files
                               // anyway, and this should be plenty)

// FIXME! this is crap! used to replace already cached resources when loading a
// new wadfile in the middle of a game
//int W_Replace(unsigned int wadnum, char **fmn);


#endif
