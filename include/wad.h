// Emacs style mode select   -*- C++ -*- 
//--------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
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
//   Wad, Wad2, Wad3 and PACK file formats
//--------------------------------------------------------------

#ifndef wad_h
#define wad_h 1

#include "doomtype.h"
#include "vfile.h"

using namespace std;

// We assume 32 bit int, 16 bit short, 8 bit char
// WAD3 files (and probably all WAD files) use little-endian byte ordering (LSB)


//========================================================================
// class Wad
// This class handles all IWAD / PWAD i/o and keeps a cache for the data lumps.
// (almost) no knowledge of a game type is assumed.


class Wad : public VDataFile
{
  friend class FileCache;
private:
  struct waddir_t *directory;  // wad directory

public:
  // constructor and destructor
  Wad();
  virtual ~Wad();

  Wad(string &fname, const char *lumpname); // special constructor for single lump files

  // open a new wadfile
  virtual bool Open(const char *fname);

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();

  // search
  virtual int FindNumForName(const char* name, int startlump = 0);
  virtual int FindPartialName(int iname, int startlump, const char **fullname);

  // retrieval
  virtual int ReadItemHeader(int item, void *dest, int size);

  // process any DeHackEd lumps in this wad
  void LoadDehackedLumps();
};


//========================================================================
// class Wad3
// This class handles all WAD2 / WAD3 i/o and keeps a cache for the data lumps.
// (almost) no knowledge of a game type is assumed.
// WAD2 (Quake, Hexen 2 etc.) and WAD3 (Half-Life) are
// nearly identical file formats (really!). Therefore only one class.


class Wad3 : public VDataFile
{
  friend class FileCache;
private:
  struct wad3dir_t *directory;  // wad directory

public:
  // constructor and destructor
  Wad3();
  virtual ~Wad3();

  // open a new wadfile
  virtual bool Open(const char *fname);

  // query data item properties
  virtual const char *GetItemName(int i);
  virtual int  GetItemSize(int i);
  virtual void ListItems();

  // search
  virtual int FindNumForName(const char* name, int startlump = 0);

  // retrieval
  virtual int ReadItemHeader(int item, void *dest, int size);
};


//======================================
// PACK files
// Quake et al.


class Pak : public VDataFile
{
private:
  struct pakdir_t *directory;

public:
  // constructor and destructor
  Pak();
  virtual ~Pak();

  // open a new wadfile
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


#endif
