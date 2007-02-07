// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2003-2006 by DooM Legacy Team.
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
/// \brief Implementation for VFile, VDir and VDataFile classes.

#include <sys/stat.h>
#include <dirent.h>

#include "doomdef.h"
#include "md5.h"
#include "vfile.h"
#include "z_zone.h"


//====================================
//    All VFiles
//====================================


void *VFile::operator new(size_t size)
{
  return Z_Malloc(size, PU_STATIC, NULL);
}

void VFile::operator delete(void *mem)
{
  Z_Free(mem);
}


VFile::VFile()
{
  filename = "";
  numitems = 0;
  cache = NULL;
}

VFile::~VFile()
{
  if (cache)
    Z_Free(cache);
}


int VFile::FindNumForName(const char* name, int startitem)
{
  nmap_t::iterator i, j;
  i = imap.lower_bound(name);
  if (i == imap.end())
    return -1;

  j = imap.upper_bound(name);
  for (; i != j; i++)
    {
      // TODO does this work? Are the elements with same key always in insertion order?
      if (i->second < startitem)
	continue;

      return i->second;
    }

  return -1;
}

void *VFile::CacheItem(int item, int tag)
{
  if (!cache[item])
    {
      // read the lump in    
      //CONS_Printf("cache miss on lump %i\n",lump);
      // looks frightening, but actually Z_Malloc sets cache[item] to point to the memory
      Z_Malloc(GetItemSize(item), tag, &cache[item]);
      ReadItemHeader(item, cache[item], 0);
    }
  else
    {
      //CONS_Printf("cache hit on lump %i\n",lump);
      /*
      if (Z_GetTag(cache[item]) != tag)
	CONS_Printf("Memtag type changed on item %d!\n", item);
      */
    }

  return cache[item];
}



//====================================
//    Real directories
//====================================

#define MAX_VDIR_ITEM_NAME 64

struct vdiritem_t
{
  int  size;  // item size in bytes
  char name[MAX_VDIR_ITEM_NAME+1];  // item name c-string ('\0'-terminated)
};


VDir::VDir()
{
  dstream = NULL;
}

VDir::~VDir()
{
  if (dstream)
    {
      closedir(dstream);
      dstream = NULL; // needed?
    }
}

bool VDir::Open(const char *fname)
{
  struct stat tempstat;
  /*
  // check that it really is a directory
  stat(fname, &tempstat);
  if (!S_ISDIR(tempstat.st_mode))
    return false;
  */

  dstream = opendir(fname);
  if (!dstream)
    {
      CONS_Printf("Could not open directory '%s'\n", fname);
      return false;
    }

  filename = fname;

  dirent *d; // d points to a static structure (not thread-safe)
  while ((d = readdir(dstream)))
    {
      // count the items
      //printf("%s\n", name);
      if (d->d_name[0] != '.')
	numitems++; // skip dotted files, . and ..
    }

  // build the contents table
  rewinddir(dstream);
  contents = (vdiritem_t *)Z_Malloc(numitems * sizeof(vdiritem_t), PU_STATIC, NULL); 
  int i = 0;
  while ((d = readdir(dstream)))
    {
      const char *temp = d->d_name;
      if (temp[0] != '.')
	{
	  string fullname = filename + temp;
	  strncpy(contents[i].name, fullname.c_str(), MAX_VDIR_ITEM_NAME);

	  if (stat(fullname.c_str(), &tempstat))
	    {
	      CONS_Printf("Could not stat file '%s'.\n", fullname.c_str());
	      contents[i].size = 0;
	    }
	  else
	    contents[i].size = tempstat.st_size;

	  imap.insert(pair<const char *, int>(contents[i].name, i)); // fill the name map
	  // TODO also insert just last part of filename to map?
	  i++;
	}
    }

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(numitems * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, numitems * sizeof(lumpcache_t));

  //ListItems();

  return true;
}

const char *VDir::GetItemName(int i)
{
  return contents[i].name;
}

int VDir::GetItemSize(int i)
{
  return contents[i].size;
}

void VDir::ListItems()
{
  for (int i=0; i<numitems; i++)
    printf("%s\n", contents[i].name);
}

// this should not be used directly, CacheItem() is better
int VDir::ReadItemHeader(int i, void *dest, int size)
{
  vdiritem_t *item = contents + i;

  // there should not be any empty resources in a directory
  if (item->size == 0)
    return 0;

  // 0 size means read all the lump
  if (size == 0 || size > item->size)
    size = item->size;

  // cache cannot be used here, because this function is used in the caching process
  FILE *str = fopen(item->name, "rb");
  if (!str)
    {
      I_Error("Could not access file '%s'!\n", item->name);
      return 0;
    }

  int n = fread(dest, 1, size, str);
  fclose(str);

  return n;
}



//====================================
//    Real files
//====================================

VDataFile::VDataFile()
{
  stream = NULL;
}

VDataFile::~VDataFile()
{
  if (stream)
    {
      fclose(stream);
      stream = NULL; // needed?
    }
}

bool VDataFile::Open(const char *fname)
{
  // at this point we have already checked the magic number and opened the file once
  stream = fopen(fname, "rb");
  if (!stream)
    return false;

  filename = fname;

  // get file system info about the file
  struct stat tempstat;
  if (fstat(fileno(stream), &tempstat))
    {
      CONS_Printf("Could not stat file '%s'.\n", fname);
      return false;
    }

  size = tempstat.st_size;

  // generate md5sum 
  md5_stream(stream, md5sum);

  return true;
}


bool VDataFile::GetNetworkInfo(int *s, unsigned char *md5)
{
  *s = size;
  for (int i=0; i<16; i++)
    md5[i] = md5sum[i];

  return true; // TODO some IWADs should not be transferred
}
