// Emacs style mode select   -*- C++ -*-
//--------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003 by DooM Legacy Team.
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
//   Virtual Files. Base class for all persistent data sources
//   like WADs, PAKs etc.
//--------------------------------------------------------------

#ifndef vfile_h
#define vfile_h 1

#include <map>
#include <string>
#include <dirent.h>

#include "functors.h"

using namespace std;


typedef void* lumpcache_t;

// TODO temporary
#ifdef HWRENDER
# include "hardware/hw_data.h"
#else
typedef void GlidePatch_t;
#endif


//======================================
// ABC for all VFiles

class VFile
{
  friend class FileCache;

protected:
  typedef multimap<const char*, int, less_cstring> nmap_t;

  string filename;    // the name of the associated physical file
  int    numitems;    // number of data items inside this VFile

  nmap_t       imap;  // mapping from item names to item numbers
  lumpcache_t *cache; // from item numbers to item data (L1 cache)

  GlidePatch_t *hwrcache; // patches are cached in renderer's native format TODO remove

public:

  // constructor and destructor
  VFile();
  virtual ~VFile();

  void *operator new(size_t size);
  void  operator delete(void *mem);

  // open a new file (kinda like a constructor but returns false if not succesful)
  virtual bool Open(const char *fname) = 0;

  // returns true and passes the asked data if the file can be transferred
  virtual bool GetNetworkInfo(int *size, unsigned char *md5) {return false;};

  // query data item properties
  virtual const char *GetItemName(int i) = 0;
  virtual int  GetItemSize(int i) = 0;
  virtual void ListItems() = 0;

  // search
  virtual int FindNumForName(const char* name, int startitem = 0);
  virtual int FindPartialName(int iname, int startlump, const char **fullname) {return -1;};

  // retrieval
  void *CacheItem(int item, int tag);
  virtual int ReadItemHeader(int item, void *dest, int size) = 0;
};


//======================================
// class for physical directories

#define MAX_VDIR_ITEM_NAME 64

struct vdiritem_t
{
  int  size;  // item size in bytes
  char name[MAX_VDIR_ITEM_NAME];  // item name c-string ('\0'-terminated)
};

class VDir : public VFile
{
protected:
  DIR        *dstream;  // associated stream
  vdiritem_t *contents; // mapping from numbers to item properties

public:
  VDir();
  virtual ~VDir();

  virtual bool Open(const char *fname);

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();

  // search
  //virtual int FindNumForName(const char* name, int startlump = 0);

  // retrieval
  virtual int ReadItemHeader(int item, void *dest, int size);
};


//======================================
// ABC for physical files (not directories!)

class VDataFile : public VFile
{
protected:
  FILE *stream;    // associated stream
  int   size;      // file size in bytes
  int   diroffset; // offset to file directory (if needed)
  unsigned char md5sum[16]; // checksum for data integrity checks

public:
  VDataFile();
  virtual ~VDataFile();

  virtual bool Open(const char *fname);

  virtual bool GetNetworkInfo(int *size, unsigned char *md5);

  // query data item properties
  virtual const char *GetItemName(int i) = 0;
  virtual int  GetItemSize(int i) = 0;
  virtual void ListItems() = 0;

  // search
  //virtual int FindNumForName(const char* name, int startlump = 0);

  // retrieval
  virtual int ReadItemHeader(int item, void *dest, int size) = 0;
};


#endif
