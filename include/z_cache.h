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
// Revision 1.9  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.8  2004/08/12 18:30:30  smite-meister
// cleaned startup
//
// Revision 1.7  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.6  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.5  2003/04/21 15:58:33  hurdler
// Fix compiling problem with gcc 3.x under Linux
//
// Revision 1.4  2003/03/15 20:15:49  smite-meister
// Fixed namespace problem
//
// Revision 1.3  2003/03/08 16:07:17  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1  2003/02/18 20:03:18  smite-meister
// L2 cache added
//
//---------------------------------------------------------------------

/// \file
/// \brief Abstract cache class with reference counting

#ifndef z_cache_h
#define z_cache_h 1

#if (__GNUC__ != 2)
# include <ext/hash_map>
#else
# include <hash_map>
#endif
#include <string.h>
#include "functors.h"
#include "z_zone.h"

using namespace std;


/// \brief BC for cache items
class cacheitem_t
{
  friend class cache_t;
protected:
  char  name[9]; // TEST
  int   usefulness; ///< how many times has it been used?
  int   refcount;   ///< reference count, number of current users

public:

  cacheitem_t(const char *name);
  virtual ~cacheitem_t();

  /// releases the item by decrementing the refcount
  bool  Release();

  /// returns the name of the item
  const char *GetName() { return name; };

  void *operator new(size_t size);
  void  operator delete(void *mem);
};


/// \brief ABC for different types of caches
class cache_t
{
protected:
  // annoying namespace declarations, because hash_map is an extension...
#if (__GNUC__ != 2)
  //typedef __gnu_cxx::hash_map<const char*, cacheitem_t*, __gnu_cxx::hash<const char *>, equal_cstring> c_map_t;
  typedef __gnu_cxx::hash_map<const char*, cacheitem_t*, hash_cstring8, equal_cstring8> c_map_t;
#else
  //typedef hash_map<const char*, cacheitem_t*, hash<const char *>, equal_cstring> c_map_t;
  typedef hash_map<const char*, cacheitem_t*, hash_cstring8, equal_cstring8> c_map_t;
#endif

  typedef c_map_t::iterator c_iter_t;
  c_map_t c_map; ///< hash_map from data item names to cacheitem_t's

  memtag_t     tagtype;       ///< memory tag used for the cached data
  const char  *default_name;  ///< name of the default data item
  cacheitem_t *default_item;  ///< the default data item itself

  /// Does the actual loading and conversion of the data during a Cache() operation
  virtual cacheitem_t *Load(const char *p) = 0;

public:
  cache_t(memtag_t tag);
  virtual ~cache_t();

  /// Defines the default data item for the cache
  void SetDefaultItem(const char *defitem);

  /// Caches and returns the requested data item.
  /// NOTE! p's content must remain constant as long as the cacheitem lives!
  cacheitem_t *Cache(const char *p);

  /// Prints current cache contents
  void Inventory();

  /// Removes unused data items from cache
  int  Cleanup();

  /// does not work yet
  void Flush();
};

#endif
