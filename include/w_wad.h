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
//-----------------------------------------------------------------------------

/// \file
/// \brief FileCache, a class for storing VFiles

#ifndef w_wad_h
#define w_wad_h 1

#include <vector>
#include <string>

#include "doomtype.h"

using namespace std;
namespace TNL { class BitStream; };

#define MAX_WADPATH   128
#define MAX_WADFILES  32       // maximum of wad files used at the same time
                               // (there is a max of simultaneous open files
                               // anyway, and this should be plenty)

//=========================================================================
/// \brief First-level cache for data files
///
/// A class for holding open VFile objects and giving out information about them
/// This class also handles game dependent operations concerning wad files.
/// The data items (lumps) inside the files are numbered using a 16.16 scheme,
/// where the first short is the file number and the second short the number of the lump
/// inside the file.

class FileCache
{
private:
  string datapath;              ///< absolute path for searching files
  vector<class VFile *> vfiles; ///< open virtual files

public:
  ~FileCache();

  void SetPath(const char *p);        ///< set the default path
  const char *Access(const char *f);  ///< tries to find the given file, returns path where found

  int  AddFile(const char *filename); ///< opens a new VFile, returns -1 on error

  /// returns true if everything is OK, false if at least one file was missing
  bool InitMultipleFiles(const char *const*filenames);

  // info about open vfiles
  int Size() { return vfiles.size(); };  ///< number of open VFiles
  const char *Name(int i);               ///< returns the name of the VFile
  unsigned int GetNumLumps(int filenum); ///< returns the number of lumps in the VFile
  void WriteNetInfo(TNL::BitStream &s);

  int LumpLength(int lump);              ///< length of a lump in bytes

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
