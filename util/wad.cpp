// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Wad, Wad3 and Pak classes: datafile I/O
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <sys/stat.h>

#include "wad.h"

#include "doomdef.h"
#include "dehacked.h"
#include "i_system.h"
#include "z_zone.h"


struct wadheader_t 
{
  char magic[4];   // "IWAD", "PWAD", "WAD2" or "WAD3"
  int  numentries; // number of entries in WAD
  int  diroffset;  // offset to WAD directory
};

// a WAD directory entry
struct waddir_t
{
  int  offset;  // file offset of the resource
  int  size;    // size of the resource
  union
  {
    char name[8]; // name of the resource (NUL-padded)
    int  iname[2];
  };
};

// a WAD2 or WAD3 directory entry
struct wad3dir_t
{
  int  offset; // offset of the data lump
  int  dsize;  // data lump size in file (compressed)
  int  size;   // data lump size in memory (uncompressed)
  char type;   // type (data format) of entry. not needed.
  char compression; // kind of compression used. 0 means none.
  char padding[2];  // unused
  union
  {
    char name[16]; // name of the entry, padded with '\0'
    int  iname[4];
  };
};


struct pakheader_t
{
  char magic[4];   // "PACK"
  int  diroffset;  // offset to directory
  int  dirsize;    // numentries * sizeof(pakdir_t) == numentries * 64
};

// PACK directory entry
struct pakdir_t
{
  char name[56]; // item name, NUL-padded
  int  offset;
  int  size;
};


static bool TestPadding(char *name, int len)
{
  // TEST padding of lumpnames
  int j;
  bool warn = false;
  for (j=0; j<len; j++)
    if (name[j] == 0)
      {
	for (j++; j<len; j++)
	  if (name[j] != 0)
	    {
	      name[j] = 0; // fix it
	      warn = true;
	    }
	if (warn)
	  CONS_Printf("Warning: Lumpname %s not padded with zeros!\n", name);
	break;
      }
  return warn;
}


//=============================
//  Wad class implementation
//=============================

// constructor
Wad::Wad()
{
  directory = NULL;
}

Wad::~Wad()
{
  Z_Free(directory);
}


// trick constructor for .lmp and .deh files
Wad::Wad(const char *fname, const char *lumpname)
{
  // this code emulates a wadfile with one lump
  // at position 0 and size of the whole file
  // this allows deh files to be treated like wad files,
  // copied by network and loaded at the console

  filename = fname;
  stream = fopen(fname, "rb");
  if (!stream)
    {
      directory = NULL;
      return;
    }

  // diroffset, md5sum, cache, hwrcache unset
  numitems = 1;
  directory = (waddir_t *)Z_Malloc(sizeof(waddir_t), PU_STATIC, NULL);
  directory->offset = 0;

  // get file system info about the file
  struct stat bufstat;
  fstat(fileno(stream), &bufstat);
  size = directory->size = bufstat.st_size;
  strncpy(directory->name, lumpname, 8);

  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  LoadDehackedLumps();
}


// -----------------------------------------------------
// Loads a WAD file, sets up the directory and cache.
// Returns false in case of problem

bool Wad::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // read header
  wadheader_t h;
  rewind(stream);
  fread(&h, sizeof(wadheader_t), 1, stream);
  // endianness swapping
  numitems = LONG(h.numentries);
  diroffset = LONG(h.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  // read wad file directory
  fseek(stream, diroffset, SEEK_SET);
  waddir_t *p = directory = (waddir_t *)Z_Malloc(numitems * sizeof(waddir_t), PU_STATIC, NULL); 
  fread(directory, sizeof(waddir_t), numitems, stream);  

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->size   = LONG(p->size);
      TestPadding(p->name, 8);
    }

  h.numentries = 0; // what a great hack!

  CONS_Printf("Added %s file %s (%i lumps)\n", h.magic, filename.c_str(), numitems);
  LoadDehackedLumps();
  return true;
}


int Wad::GetItemSize(int i)
{
  return directory[i].size;
}

const char *Wad::GetItemName(int i)
{
  return directory[i].name;
}


int Wad::ReadItemHeader(int lump, void *dest, int size)
{
  waddir_t *l = directory + lump;

  // empty resource (usually markers like S_START, F_END ..)
  if (l->size == 0)
    return 0;
  
  // 0 size means read all the lump
  if (size == 0 || size > l->size)
    size = l->size;

  fseek(stream, l->offset, SEEK_SET);
  return fread(dest, 1, size, stream); 
}


void Wad::ListItems()
{
  waddir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-8s\n", p->name);
}


// -----------------------------------------------------
// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int Wad::FindNumForName(const char *name, int startlump)
{
  union
  {
    char s[9];
    int  x[2];
  };

  // make the name into two integers for easy compares
  strncpy(s, name, 8);

  // in case the name was 8 chars long
  s[8] = 0;
  // case insensitive TODO make it case sensitive if possible
  strupr(s);

  // FIXME doom.wad and doom2.wad PNAMES lumps have exactly ONE (1!) patch
  // entry with a lowcase name: w94_1. Of course the actual lump is
  // named W94_1, so it won't be found if we have case sensitive search! damn!
  // heretic.wad and hexen.wad have no such problems.
  // The right way to fix this is either to fix the WADs (yeah, right!) or handle
  // this special case in the texture loading routine.

  waddir_t *p = directory + startlump;

  // a slower alternative could use strncasecmp()
  for (int j = startlump; j < numitems; j++, p++)
    if (p->iname[0] == x[0] && p->iname[1] == x[1])
      return j;

  // not found
  return -1;
}


int Wad::FindPartialName(int iname, int startlump, const char **fullname)
{
  // checks only first 4 characters, returns full name
  // a slower alternative could use strncasecmp()

  waddir_t *p = directory + startlump;

  for (int j = startlump; j < numitems; j++, p++)
    if (p->iname[0] == iname)
      {
	*fullname = p->name;
	return j;
      }

  // not found
  return -1;
}

// -----------------------------------------------------
// LoadDehackedLumps
// search for DEHACKED lumps in a loaded wad and process them

void Wad::LoadDehackedLumps()
{
  // just the lump number, nothing else
  int clump = 0;

  while (1)
    { 
      clump = FindNumForName("DEHACKED", clump);
      if (clump == -1)
	break;
      CONS_Printf("Loading DEHACKED lump %d from %s\n", clump, filename.c_str());

      DEH.LoadDehackedLump((char *)CacheItem(clump, PU_CACHE), GetItemSize(clump));
      clump++;
    }
}




//==============================
//  Wad3 class implementation
//==============================

// constructor
Wad3::Wad3()
{
  directory = NULL;
}

// destructor
Wad3::~Wad3()
{
  Z_Free(directory);
}


// -----------------------------------------------------
// Loads a WAD2 or WAD3 file, sets up the directory and cache.
// Returns false in case of problem

bool Wad3::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // read header
  wadheader_t h;
  rewind(stream);
  fread(&h, sizeof(wadheader_t), 1, stream);
  // endianness swapping
  numitems = LONG(h.numentries);
  diroffset = LONG(h.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  // read in directory
  fseek(stream, diroffset, SEEK_SET);
  wad3dir_t *p = directory = (wad3dir_t *)Z_Malloc(numitems * sizeof(wad3dir_t), PU_STATIC, NULL);
  fread(directory, sizeof(wad3dir_t), numitems, stream);

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->dsize  = LONG(p->dsize);
      p->size   = LONG(p->size);
      TestPadding(p->name, 16);
    }
    
  h.numentries = 0; // what a great hack!

  CONS_Printf("Added %s file %s (%i lumps)\n", h.magic, filename.c_str(), numitems);
  return true;
}


int Wad3::GetItemSize(int i)
{
  return directory[i].size;
}

const char *Wad3::GetItemName(int i)
{
  return directory[i].name;
}

void Wad3::ListItems()
{
  wad3dir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-16s\n", p->name);
}


int Wad3::ReadItemHeader(int lump, void *dest, int size)
{
  wad3dir_t *l = directory + lump;

  if (l->compression)
    return -1;
  
  if (l->size == 0)
    return 0;
  
  // 0 size means read all the lump
  if (size == 0 || size > l->size)
    size = l->size;
    
  fseek(stream, l->offset, SEEK_SET);

  return fread(dest, 1, size, stream);
}


// -----------------------------------------------------
// FindNumForName
// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int Wad3::FindNumForName(const char *name, int startlump)
{
  union
  {
    char s[16];
    int  x[4];
  };

  // make the name into 4 integers for easy compares
  // strncpy pads the target string with zeros if needed
  strncpy(s, name, 16);

  // comparison is case sensitive

  wad3dir_t *p = directory + startlump;

  int j;
  for (j = startlump; j < numitems; j++, p++)
    {
      if (p->iname[0] == x[0] && p->iname[1] == x[1] &&
	  p->iname[2] == x[2] && p->iname[3] == x[3])
	return j; 
    }
  // not found
  return -1;
}



//=============================
//  Pak class implementation
//=============================

// constructor
Pak::Pak()
{
  directory = NULL;
}

// destructor
Pak::~Pak()
{
  Z_Free(directory);
}


// -----------------------------------------------------
// Loads a PACK file, sets up the directory and cache.
// Returns false in case of problem

bool Pak::Open(const char *fname)
{
  // common to all files
  if (!VDataFile::Open(fname))
    return false;

  // read header
  pakheader_t h;
  rewind(stream);
  fread(&h, sizeof(pakheader_t), 1, stream);
  // endianness swapping
  numitems = LONG(h.dirsize) / sizeof(pakdir_t);
  diroffset = LONG(h.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  // read in directory
  fseek(stream, diroffset, SEEK_SET);
  pakdir_t *p = directory = (pakdir_t *)Z_Malloc(numitems * sizeof(pakdir_t), PU_STATIC, NULL);
  fread(directory, sizeof(pakdir_t), numitems, stream);

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->size   = LONG(p->size);
      TestPadding(p->name, 56);
      p->name[55] = 0; // precaution
      imap.insert(pair<const char *, int>(p->name, i)); // fill the name map
    }
    
  h.diroffset = 0; // what a great hack!

  CONS_Printf("Added %s file %s (%i lumps)\n", h.magic, filename.c_str(), numitems);
  return true;
}


int Pak::GetItemSize(int i)
{
  return directory[i].size;
}

const char *Pak::GetItemName(int i)
{
  return directory[i].name;
}

void Pak::ListItems()
{
  pakdir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    printf("%-56s\n", p->name);
}


int Pak::ReadItemHeader(int item, void *dest, int size)
{
  pakdir_t *l = directory + item;

  if (l->size == 0)
    return 0;
  
  // 0 size means read all the item
  if (size == 0 || size > l->size)
    size = l->size;
    
  fseek(stream, l->offset, SEEK_SET);

  return fread(dest, 1, size, stream);
}
