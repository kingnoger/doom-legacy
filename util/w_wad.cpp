// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
/// \brief FileCache: Container of VFiles

#include <string>
#include <vector>

#ifndef __WIN32__
# include <unistd.h>
#endif

#include "doomdef.h"

#include "vfile.h" // VFile class
#include "wad.h"
#include "w_wad.h"
#include "z_zone.h"


//====================================================================
// FileCache class implementation
//
// this replaces all W_* functions of old Doom code. W_* -> fc.*
//====================================================================

FileCache fc;


// -----------------------------------------------------
// destroys (closes) all open VFiles
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



//======================
// opening wad files
//======================

void FileCache::SetPath(const char *p)
{
  datapath = p;
}


// Not like libc access(). If file exists, returns the path+filename where it was found, otherwise NULL.
const char *FileCache::Access(const char *f)
{
  if (!access(f, F_OK))
    return f; // first try the current dir

  static string n;

  n.assign(datapath);
  n.push_back('/');
  n.append(f);

  if (!access(n.c_str(), F_OK))
    return n.c_str();

  return NULL;
}


// -----------------------------------------------------
// Pass a null terminated list of files to use.
// All files are optional, but at least one file must be found.
// The name searcher looks backwards, so a later file does override all earlier ones.
// Also adds GWA files if they exist.

bool FileCache::InitMultipleFiles(const char *const*filenames)
{
  CONS_Printf("W_Init: Init WADfiles.\n");
  bool result = true;

  for ( ; *filenames != NULL; filenames++)
    {
      const char *curfile = *filenames;
      if (AddFile(curfile) == -1)
	result = false;

      string gwafile(curfile);
      // Try both upper and lower case.
      gwafile.replace(gwafile.length()-3, 3, "gwa");
      if (AddFile(gwafile.c_str(), true) != -1)
	CONS_Printf("Added GL information from file %s.\n", gwafile.c_str());
      else
	{
	  gwafile.replace(gwafile.length()-3, 3, "GWA");
	  if (AddFile(gwafile.c_str(), true) != -1)
	    CONS_Printf("Added GL information from file %s.\n", gwafile.c_str());
	  //else CONS_Printf("No GL information for file %s.\n", curfile);
	}
    }

  if (vfiles.size() == 0)
    I_Error("FileCache::InitMultipleFiles: no files found");

  // result = false : at least one file was missing
  return result;
}

// -----------------------------------------------------
// Loads a wad file into memory, adds it into vfiles vector.
// returns the number of the wadfile (0 is the first one)
// or -1 in the case of error
// Files with a .wad extension are idlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
// Lump names can appear multiple times.

int FileCache::AddFile(const char *fname, bool silent)
{
  int nfiles = vfiles.size();
  
  if (nfiles >= MAX_WADFILES)
    {
      CONS_Printf("Maximum number of resource files reached\n");
      return -1;
    }

  const char *name = Access(fname);

  if (!name && !silent)
    {
      CONS_Printf("FileCache::AddFile: Can't access file %s (path %s)\n", fname, datapath.c_str());
      return -1;
    }

  VFile *vf = NULL;
  int l = strlen(fname);
  bool ok = false;

  if (!strcasecmp(&fname[l - 4], ".deh"))
    {
      // detect dehacked file with the "deh" extension
      vf = new Wad();
      ok = vf->Create(name, "DEHACKED");
    }
  else if (!strcasecmp(&fname[l - 4], ".lmp"))
    {
      // a single lump file
      char lumpname[9];
      strncpy(lumpname, fname, 8);
      lumpname[min(l-4, 8)] = '\0';
      vf = new Wad();
      ok = vf->Create(name, lumpname);
    }
  else if (fname[l-1] == '/')
    {
      // directory
      vf = new VDir();
      ok = vf->Open(name);
    }
  else
    {
      union
      {
	char magic[5];
	int  imagic;
      };
      // open the file to read the magic number
      FILE *str = fopen(name, "rb");
      if (!str)
	return -1;

      fread(magic, 4, 1, str);
      magic[4] = 0;
      
      // check file type
      if (imagic == *reinterpret_cast<const int *>("IWAD")
	  || imagic == *reinterpret_cast<const int *>("PWAD"))
	{
	  vf = new Wad();
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
      else if (imagic == *reinterpret_cast<const int *>("PK\3\4"))
	{
	  vf = new ZipFile(); // ZIP/PK3 file
	}
      else
	{
	  CONS_Printf("FileCache::AddFile: Unknown file signature %4c\n", magic);
	  fclose(str);
	  return -1;
	}
      fclose(str);
      ok = vf->Open(name);
    }

  if (ok)
    {
      vfiles.push_back(vf);
      return nfiles;
    }

  delete vf;
  return -1;
}


//=============================
// basic info on open wadfiles
//=============================


// -----------------------------------------------------
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


const char *FileCache::Name(int i)
{
  return vfiles[i]->filename.c_str();
}


unsigned int FileCache::GetNumLumps(int filenum)
{
  return vfiles[filenum]->numitems;
}



//==============
//  searching 
//==============

// -----------------------------------------------------
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


const char *FileCache::FindNameForNum(int lump)
{
  unsigned int file = lump >> 16;
  int item = lump & 0xffff;

  if (file >= vfiles.size())
    I_Error("FileCache::FindNameForNum: %i >= numvfiles(%i)\n", file, vfiles.size());
  if (item >= vfiles[file]->numitems)
    //I_Error("FileCache::FindNameForNum: %i >= numitems", item);
    return NULL;

  return vfiles[file]->GetItemName(item);
}


// -----------------------------------------------------
// Calls FindNumForName, but bombs out if not found.

int FileCache::GetNumForName(const char *name, bool scanforward, bool errorifnotfound)
{
  int i = FindNumForName(name, scanforward);
  if (i == -1 && errorifnotfound)
    I_Error("FileCache::GetNumForName: %s not found!\n", name);
  
  return i;
}


int FileCache::FindPartialName(Uint32 iname, unsigned filenum, int startlump, const char **fullname)
{
  if (filenum >= vfiles.size())
    I_Error("FileCache::FindNumForNamePwad: %i >= numvfiles(%i)\n", filenum, vfiles.size());
 
  return vfiles[filenum]->FindPartialName(iname, startlump, fullname);
}



//================================
//    reading and caching
//================================

//-----------------------------------------------------
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


//-----------------------------------------------------
// caches the given lump

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
