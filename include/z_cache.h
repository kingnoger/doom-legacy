// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// $Log$
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1  2003/02/18 20:03:18  smite-meister
// L2 cache added
//
//
//
// DESCRIPTION:
//   Abstract second-level cache system with reference counting
//
//---------------------------------------------------------------------

#ifndef z_cache_h
#define z_cache_h 1

#include <ext/hash_map>
#include "z_zone.h"

class cacheitem_t
{
  friend class L2cache_t;
protected:
  int   lumpnum;    // lump number of data
  int   usefulness; // how many times has it been used?
  int   refcount;   // reference count, number of current users

public:
  void *data;       // data
  int   length;     // in bytes

  ~cacheitem_t();

  bool  Release();

  void *operator new(size_t size);
  void  operator delete(void *mem);
};


class L2cache_t
{
protected:
  // annoying namespace declarations, because hash_map is an extension...
  typedef __gnu_cxx::hash_map<const char*, cacheitem_t*> c_map_t;
  typedef __gnu_cxx::hash_map<const char*, cacheitem_t*>::iterator c_iter_t;
  c_map_t c_map;

  memtag_t     tagtype; // tag type used for cached data
  const char  *default_name;
  cacheitem_t *default_item; // default replace item

  virtual cacheitem_t *CreateItem(const char *p);
  virtual void LoadAndConvert(cacheitem_t *t);

public:

  L2cache_t(memtag_t tag);
  virtual ~L2cache_t();

  void SetDefaultItem(const char *defitem);

  cacheitem_t *Cache(const char *p);

  void Inventory();
  int  Cleanup();
  void Flush();
};

#endif
