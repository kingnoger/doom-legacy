// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
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
//   FileCache, a class for storing VFiles
//
//-----------------------------------------------------------------------------


#ifndef w_wad_h
#define w_wad_h 1

#include <vector>
#include <string>

#include "doomtype.h"

using namespace std;

#define MAX_WADPATH   128
#define MAX_WADFILES  32       // maximum of wad files used at the same time
                               // (there is a max of simultaneous open files
                               // anyway, and this should be plenty)


//=========================================================================
// class FileCache
// A class for holding open VFiles and giving out information about them
// This class also handles game dependent operations concerning wad files

class FileCache
{
private:
  string datapath; // absolute path for searching files
  vector<class VFile *> vfiles; // open virtual files

public:
  ~FileCache();

  void SetPath(const char *p);    // set file path

  int  AddFile(const char *filename);  // opens a new vfile, returns -1 on error
  // returns true if everything is OK, false if at least one file was missing
  bool InitMultipleFiles(const char *const*filenames);

  // info about open vfiles
  int Size() { return vfiles.size(); }; // number of open VFiles
  int LumpLength(int lump);
  const char *Name(int i);
  bool GetNetworkInfo(int filenum, int *size, unsigned char *md5);
  unsigned int GetNumLumps(int filenum);
  void WriteFileHeaders(byte *p);

  // searching: Find gives -1 if not found, Get gives an error.
  int FindNumForName(const char *name, bool scanforward = false);
  int GetNumForName(const char *name, bool scanforward = false);
  int FindNumForNameFile(const char *name, unsigned filenum, int startlump = 0);
  const char *FindNameForNum(int lump);
  int FindPartialName(int iname, unsigned filenum, int startlump, const char **fullname);

  // reading and caching lumps
  int ReadLumpHeader(int lump, void *dest, int size);
  void *CacheLumpNum(int lump, int tag);

  inline int ReadLump(int lump, void *dest)
  {
    return ReadLumpHeader(lump, dest, 0);
  };

  inline void *CacheLumpName(const char* name, int tag)
  {
    return CacheLumpNum(GetNumForName(name), tag);
  };
};


extern FileCache fc;


#endif
