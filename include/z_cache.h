// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003-2005 by DooM Legacy Team.
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


/// \brief BC for cache items
class cacheitem_t
{
  friend class cachesource_t;
  friend class cache_t;
  friend class texturecache_t;

protected:
  char  name[9];    ///< name of the item, NUL-terminated
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
  void SetName(const char *n) { strncpy(name, n, 8); };

  void *operator new(size_t size);
  void  operator delete(void *mem);
};



/// \brief Data source within a cache_t.
///
/// This is an extra layer of abstraction for complex caches with multiple data sources
/// which can have different priorities depending on the query. See texturecache_t.
class cachesource_t
{
protected:
  // annoying namespace declarations, because hash_map is an extension...
#if (__GNUC__ != 2)
  //typedef __gnu_cxx::hash_map<const char*, cacheitem_t*, __gnu_cxx::hash<const char *>, equal_cstring> c_map_t;
  typedef __gnu_cxx::hash_map<const char*, cacheitem_t*, hash_cstring8, equal_cstring8> c_map_t;
#else
  //typedef hash_map<const char*, cacheitem_t*, hash<const char *>, equal_cstring> c_map_t;
  typedef std::hash_map<const char*, cacheitem_t*, hash_cstring8, equal_cstring8> c_map_t;
#endif

  typedef c_map_t::iterator c_iter_t;
  c_map_t c_map; ///< hash_map from data item names to cacheitem_t's

public:
  /// The safe way of inserting stuff into the hash_map.
  /// The main point is that 'name' is stored within the cacheitem structure itself.
  inline void Insert(cacheitem_t *p)
  {
    c_map.insert(c_map_t::value_type(p->name, p));
  };

  /// Since hash_map is a unique associative container we need this, cannot just Insert new stuff with same key.
  inline int Replace(cacheitem_t *p)
  {
    int n = c_map.erase(p->name); // erase the old instance by key
    c_map.insert(c_map_t::value_type(p->name, p));
    return n;
  }

  /// Tries to find the named item from this datasource.
  inline cacheitem_t *Find(const char *name)
  {
    c_iter_t s = c_map.find(name);

    if (s == c_map.end())
      {
	// cache miss
	return NULL;
      }
    else
      {
	// cache hit
	return s->second;
      }
  };

  /// Prints current contents
  void Inventory();

  /// Removes unused data items
  int  Cleanup();

  /// Removes all data items
  void Clear();
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
