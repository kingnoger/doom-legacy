// Emacs style mode select   -*- C++ -*- 
//--------------------------------------------------------------
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
//   WadFile class definition
//   Wad, Wad2, Wad3 formats
//--------------------------------------------------------------

#ifndef wad_h
#define wad_h 1

#include <string>
#include "doomtype.h"

using namespace std;

// We assume 32 bit int, 16 bit short, 8 bit char
// WAD3 files (and probably all WAD files) use little-endian byte ordering (LSB)

//========================================================================

struct waddir_t;
typedef void* lumpcache_t;
#ifdef HWRENDER
# include "hardware/hw_data.h"
#else
typedef void GlidePatch_t;
#endif

struct wadheader_t 
{
  char magic[4];   // "IWAD", "PWAD", "WAD2" or "WAD3"
  int  numentries; // number of entries in WAD
  int  diroffset;  // offset to WAD directory
};

// abstract parent class for all WAD file types
class WadFile
{
  friend class FileCache;
protected:
  string         filename;
  FILE          *stream;   // associated stream
  unsigned int   filesize; // for network
  wadheader_t    header;

  lumpcache_t   *cache;    // memory cache for wad data
  GlidePatch_t  *hwrcache; // patches are cached in renderer's native format
  // TODO think again hwrcache?

  unsigned char  md5sum[16]; // checksum for data integrity checks

public:
  // constructor and destructor
  WadFile();
  virtual ~WadFile();

  void *operator new(size_t size);
  void  operator delete(void *mem);

  // open a new wadfile
  //WadFile(const char *fname, int num);

  // flushes certain caches (used when a new wad is added)
  //int Replace(void);

  // not used at the moment
  //void Reload(void);

  // open a new wadfile
  bool Load(const char *fname, FILE *str);

  virtual int GetLumpSize(int i) = 0;
  virtual char *GetLumpName(int i) = 0;
  virtual waddir_t *GetDirectory() = 0; // ugly
  virtual int ReadLumpHeader(int lump, void *dest, int size) = 0;
  virtual void ListDir() = 0;
  virtual int FindNumForName(const char* name, int startlump = 0) = 0;

  // TODO: Maybe sort the wad directory to speed up searches,
  // remove the 16:16 lump numbering system?
  // move functionality from FileCache to here?
};


//========================================================================
// class Wad
// This class handles all IWAD / PWAD i/o and keeps a cache for the data lumps.
// (almost) no knowledge of a game type is assumed.

// a WAD directory entry
struct waddir_t
{
  int  offset;  // file offset of the resource
  int  size;    // size of the resource
  char name[8]; // name of the resource
};

class Wad : public WadFile
{
  friend class FileCache;
private:
  waddir_t *directory;  // wad directory

public:
  // constructor and destructor
  Wad();
  virtual ~Wad();

  // open a new wadfile
  bool Load(const char *fname, FILE *str, int num);

  virtual int GetLumpSize(int i);
  virtual char *GetLumpName(int i);
  virtual waddir_t *GetDirectory() {return directory;}; // ugly
  virtual int ReadLumpHeader(int lump, void *dest, int size);
  virtual void ListDir();
  virtual int FindNumForName(const char* name, int startlump = 0);

  // process any DeHackEd lumps in this wad
  void LoadDehackedLumps();
};


//========================================================================
// class Wad3
// This class handles all WAD2 / WAD3 i/o and keeps a cache for the data lumps.
// (almost) no knowledge of a game type is assumed.
// WAD2 (Quake, Hexen 2 etc.) and WAD3 (Half-Life) are
// nearly identical file formats (really!). Therefore only one class.

// a WAD2 or WAD3 directory entry

struct wad3dir_t
{
  int  offset; // offset of the data lump
  int  dsize;  // data lump size in file (compressed)
  int  size;   // data lump size in memory (uncompressed)
  char type;   // type (data format) of entry. not needed.
  char compression; // kind of compression used. 0 means none.
  char padding[2];  // unused
  char name[16]; // name of the entry, padded with '\0'
};

class Wad3 : public WadFile
{
  friend class FileCache;
private:
  wad3dir_t *directory;  // wad directory

public:
  // constructor and destructor
  Wad3();
  virtual ~Wad3();

  // open a new wadfile
  bool Load(const char *fname, FILE *str);

  virtual int GetLumpSize(int i);
  virtual char *GetLumpName(int i);
  virtual waddir_t *GetDirectory() {return NULL;}; // ugly
  virtual int ReadLumpHeader(int lump, void *dest, int size);
  virtual void ListDir();
  virtual int FindNumForName(const char* name, int startlump = 0);
};


#endif
