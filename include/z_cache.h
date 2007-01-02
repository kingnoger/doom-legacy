// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
//---------------------------------------------------------------------

/// \file
/// \brief Abstract cache class with reference counting.

#ifndef z_cache_h
#define z_cache_h 1

#include <string.h>
#include "dictionary.h"
#include "z_zone.h"


/// \brief BC for cache items
class cacheitem_t
{
  friend class cachesource_t;
  friend class cache_t;
  friend class texturecache_t;
#define CACHE_NAME_LEN 63 // was 8
protected:
  char  name[CACHE_NAME_LEN+1]; ///< name of the item, NUL-terminated
  int   usefulness; ///< how many times has it been used?
  int   refcount;   ///< reference count, number of current users

public:

  cacheitem_t(const char *name);
  virtual ~cacheitem_t();

  /// releases the item by decrementing the refcount
  bool  Release();

  /// returns the name of the item
  const char *GetName() { return name; };

  /// change the name of the item
  void SetName(const char *n) { strncpy(name, n, CACHE_NAME_LEN); };

  void *operator new(size_t size);
  void  operator delete(void *mem);
};



/// \brief Data source within a cache_t.
///
/// This is an extra layer of abstraction for complex caches with multiple data sources
/// which can have different priorities depending on the query. See texturecache_t.
class cachesource_t : public HashDictionary<cacheitem_t>
{
public:
  /// Prints current contents
  void Inventory();

  /// Removes unused data items
  int  Cleanup();
};



/// \brief ABC for different types of simple caches.
///
/// Data used through a cache_t must not be used anywhere
/// else, because Z_Free and Z_ChangeTag will cause problems.
class cache_t
{
protected:
  cachesource_t source;       ///< a simple cache has only one source

  memtag_t      tagtype;      ///< memory tag used for the cached data
  cacheitem_t  *default_item; ///< the default data item


  /// Creates a new cacheitem_t, does the actual loading and conversion of the data during a Cache() operation.
  virtual cacheitem_t *Load(const char *p) = 0;

  /// Caches and returns the requested data item, or the default_item.
  cacheitem_t *Cache(const char *p);

public:
  cache_t(memtag_t tag);
  virtual ~cache_t();

  /// Defines the default data item for the cache.
  void SetDefaultItem(const char *defitem);

  /// Derived cache classes should define a function like this:
  //inline derived_item_t *Get(const char *p) { return (derived_item_t *)Cache(p); };

  /// Prints current cache contents
  void Inventory();

  /// Removes unused data items from cache
  inline int  Cleanup() { return source.Cleanup(); };

  /// does not work yet
  //void Flush();
};

#endif
