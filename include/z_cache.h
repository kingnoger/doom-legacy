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
//
//
// DESCRIPTION:
//   Abstract second-level cache system with reference counting
//
//---------------------------------------------------------------------

#ifndef z_cache_h
#define z_cache_h 1

#if (__GNUC__ != 2)
# include <ext/hash_map>
#else
# include <hash_map>
#endif
#include <string.h>
#include "z_zone.h"

using namespace std;

class cacheitem_t
{
  friend class L2cache_t;
protected:
  int   usefulness; // how many times has it been used?
  int   refcount;   // reference count, number of current users

public:

  cacheitem_t();

  bool  Release();

  void *operator new(size_t size);
  void  operator delete(void *mem);
};

// c-string comparison functor
struct compare_strings
{
  bool operator()(const char* s1, const char* s2) const
  { return strcmp(s1, s2) == 0; }
};

class L2cache_t
{
protected:
  // annoying namespace declarations, because hash_map is an extension...
  // Arrr, matey! STL designers be cursed with scurvy and lice! The default hash function
  // is okay but default key comparison function compares pointers, not c-strings!
#if (__GNUC__ != 2)
  typedef __gnu_cxx::hash_map<const char*, cacheitem_t*,
    __gnu_cxx::hash<const char *>, compare_strings> c_map_t;
#else
  typedef hash_map<const char*, cacheitem_t*, hash<const char *>, compare_strings> c_map_t;
#endif

  typedef c_map_t::iterator c_iter_t;
  c_map_t c_map;

  memtag_t     tagtype; // tag type used for cached data
  const char  *default_name;
  cacheitem_t *default_item; // default replace item

  virtual cacheitem_t *Load(const char *p, cacheitem_t *t = NULL) = 0;
  virtual void Free(cacheitem_t *t) = 0;

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
