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
// $Log$
// Revision 1.10  2005/07/20 20:27:23  smite-meister
// adv. texture cache
//
// Revision 1.9  2004/11/28 18:02:24  smite-meister
// RPCs finally work!
//
// Revision 1.8  2004/09/03 16:28:52  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.7  2004/08/12 18:30:33  smite-meister
// cleaned startup
//
// Revision 1.6  2004/03/28 15:16:15  smite-meister
// Texture cache.
//
// Revision 1.5  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.4  2003/04/04 00:01:58  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.3  2003/03/08 16:07:18  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1  2003/02/18 20:03:19  smite-meister
// L2 cache added
//
//---------------------------------------------------------------------

/// \file
/// \brief Abstract cache system with reference counting

#include "doomdef.h"
#include "z_cache.h"


//=================================================================================

cacheitem_t::cacheitem_t(const char *n)
{
  strncpy(name, n, 8); // we make a copy so it stays intact as long as this cacheitem lives
  name[8] = '\0';      // NUL-terminated to be safe
  usefulness = 0;
  refcount = 0;
}


cacheitem_t::~cacheitem_t() {}


void *cacheitem_t::operator new(size_t size)
{
  return Z_Malloc(size, PU_STATIC, NULL);
}


void cacheitem_t::operator delete(void *mem)
{
  Z_Free(mem);
}


bool cacheitem_t::Release()
{
  if (--refcount < 0)
    I_Error("cacheitem_t '%s': Too many releases!\n", name);

  return (refcount == 0);
}


//=================================================================================

/// Lists contents.
void cachesource_t::Inventory()
{
  for (c_iter_t s = c_map.begin(); s != c_map.end(); s++)
    {
      cacheitem_t *p = s->second;
      CONS_Printf("- %s, rc = %d, use = %d\n", p->name, p->refcount, p->usefulness);
    }
}


/// Erases unused items (refcount == 0) and makes
/// their data purgable (it _may_ still remain in the filecache)
int cachesource_t::Cleanup()
{
  int k = 0;
  for (c_iter_t s = c_map.begin(); s != c_map.end(); )
    {
      cacheitem_t *p = s->second;
      c_iter_t t = s++; // first copy s to t, then increment s

      if (p->refcount == 0)
	{
	  c_map.erase(t); // erase it from the hash_map
	  // Once an iterator is erased, it becomes invalid
	  // and cannot be incremented! Therefore we have both s and t.
	  delete p; // delete the cacheitem itself
	  k++;
	}
    }
  return k;
}


/// Erases all cacheitems.
void cachesource_t::Clear()
{
  for (c_iter_t t = c_map.begin(); t != c_map.end(); t++)
    delete t->second;

  c_map.clear();
}

//=================================================================================


/// Pure virtual, renew. This is a sample implementation.
/*
cacheitem_t *cache_t::Load(const char *n)
{
  int lump = fc.FindNumForName(n, false);
  if (lump == -1)
    return NULL;

  derived_cacheitem_t *p = new derived_cacheitem_t(n);

  p->lumpnum = lump;
  p->data = fc.CacheLumpNum(lump, tagtype);
  p->length = fc.LumpLength(lump);
  return p;
}
*/


/// cache constructor
cache_t::cache_t(memtag_t tag)
{
  tagtype = tag;
  default_item = NULL;
}


/// cache destructor
// just to shut up compiler, since entire caches are probably never destroyed
cache_t::~cache_t()
{
  // should empty the map and delete the items
}



/// Has to be separate from constructor. Static class instances are constructed when
/// program starts, and at that time we do not yet have a working FileCache.
void cache_t::SetDefaultItem(const char *name)
{
  if (default_item)
    CONS_Printf("cache: Replacing default_item!\n");

  default_item = Load(name);
  if (!default_item)
    I_Error("cache: New default_item '%s' not found!\n", name);
  // TODO delete the old default item?
}



/// Checks if item is already in cache. If so, increments refcount and returns it.
/// If not, tries to load (and convert) it. If succesful, returns the item.
/// If not, returns a link to the defaultitem.
cacheitem_t *cache_t::Cache(const char *name)
{
  cacheitem_t *p;

  if (name == NULL)
    p = default_item;
  else
    {
      p = source.Find(name);

      if (!p)
	{
	  // Not found in source.
	  p = Load(name);
	  if (!p)
	    {
	      // Item not found at all.
	      // Some nonexistant items are asked again and again.
	      // We use a special cacheitem_t to link their names to the default item.
	      p = new cacheitem_t(name);
	      p->usefulness = -1; // negative usefulness marks them as links
	    }

	  source.Insert(p);
	}
    }

  if (p->usefulness < 0)
    {
      // a "link" to default_item
      p->refcount++;
      p->usefulness--; // negated

      default_item->refcount++;
      default_item->usefulness++;
      return default_item;
    }

  p->refcount++;
  p->usefulness++;
  return p;  
}



/// Lists the contents of the cache.
void cache_t::Inventory()
{
  cacheitem_t *p = default_item;

  if (p)
    CONS_Printf("Defitem %s, rc = %d, use = %d\n", p->name, p->refcount, p->usefulness);

  source.Inventory();
}




/// Recaches and converts every item in cache using their names.
/// Existing pointers to cacheitems are preserved.
// TODO does not work yet, needs redesign. We may need an additional pointer layer...
/*
void cache_t::Flush()
{
  Cleanup(); // remove unused items

  cacheitem_t *old_default = default_item;

  if (!default_item->Reload()) // virtual method
    {
      // not found
      I_Error("cache: Unable to recache default item '%s'!\n", default_item->name);
    }

  for (c_iter_t s = c_map.begin(); s != c_map.end(); )
    {
      s->second->Reload();
    }
}


bool cacheitem_t::Reload()
{
  // always a "link"
}


bool derived_cacheitem_t::Reload()
{
  // might be anything
}
*/
