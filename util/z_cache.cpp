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
//
//
// DESCRIPTION:
//   Abstract second-level cache system with reference counting
//
//---------------------------------------------------------------------

#include "w_wad.h"
#include "z_cache.h"

cacheitem_t::cacheitem_t()
{
  usefulness = 0;
  refcount = 0;
}

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
    I_Error("cacheitem_t: Too many releases!\n");

  if (refcount == 0)
    return true;

  return false;
}

//==========================================================

// cache constructor
L2cache_t::L2cache_t(memtag_t tag)
{
  tagtype = tag;
  default_item = NULL;
}

// cache destructor
// just to shut up compiler, since entire caches are probably never destroyed
L2cache_t::~L2cache_t()
{
  /*
  if (default_item)
    {
      Free(default_item);
      delete default_item;
    }
  // should empty the map too and Free() + delete its items
  */
}

// Has to be separate from constructor. Static class instances are constructed when
// program starts, and at that time we do not yet have a working FileCache.
void L2cache_t::SetDefaultItem(const char *p)
{
  if (default_item)
    {
      CONS_Printf("L2cache: Replacing default_item!\n");
      // the old default item is made a normal cacheitem
      default_item->refcount--; // take away the extra reference
    }

  cacheitem_t *t = Load(p, NULL);
  if (!t)
    I_Error("L2cache: New default_item '%s' not found!\n", p);

  // it is also inserted into the map as a normal item
  c_map.insert(c_map_t::value_type(p, t));
  t->refcount++; // one extra reference so that it is never freed

  default_name = p;
  default_item = t;  
}


// Pure virtual, renew. This is a simple implementation.
/*
void cacheitem_t *L2cache_t::Load(const char *p, cacheitem_t *r)
{
  int lump = fc.FindNumForName(p, false);
  if (lump == -1)
    return NULL;

  derived_cacheitem_t *t = (derived_cacheitem_t *)r;
  if (t == NULL)
    t = new derived_cacheitem_t;

  t->lumpnum = lump;
  t->data = fc.CacheLumpNum(lump, tagtype);
  t->length = fc.LumpLength(lump);
  return t;
}
*/

// Pure virtual, renew. This is a simple implementation.
/*
void L2cache_t::Free(cacheitem_t *r)
{
  derived_cacheitem_t *t = (derived_cacheitem_t *)r;
  if (t->data)
    Z_ChangeTag(t->data, PU_CACHE)
}
*/

// Checks if p is already in cache. If so, increments refcount and returns it.
// If not, tries to cache and convert p. If succesful, returns it.
// If not, returns the defaultitem.
cacheitem_t *L2cache_t::Cache(const char *p)
{
  // data used through a L2 cache must not be used anywhere else (z_free and z_changetag will cause problems)

  cacheitem_t *t;
  c_iter_t i;

  if (p == NULL)
    t = default_item;
  else
    { 
      i = c_map.find(p);

      if (i == c_map.end())
	{
	  //CONS_Printf("--- cache miss, %s, %p\n", p, p);
	  // not found
	  t = Load(p, NULL);
	  if (t)
	    c_map.insert(c_map_t::value_type(p, t));
	  else
	    {
	      // TEST some nonexistant items are asked again and again.
	      // what if I just bind them to the defaultitem for good?
	      t = default_item;
	      c_map.insert(c_map_t::value_type(p, t));
	    }
	}
      else
	{
	  //CONS_Printf("+++ cache hit, %s, %p\n", p, p);
	  // found
	  t = (*i).second;
	}
    }

  t->refcount++;
  t->usefulness++;

  return t;
}

void L2cache_t::Inventory()
{
  c_iter_t i;
  cacheitem_t *t = default_item;

  if (t)
    CONS_Printf("Defitem %s, rc = %d, use = %d\n",
		default_name, t->refcount, t->usefulness);
 
  for (i = c_map.begin(); i != c_map.end(); i++)
    {
      const char *name = (*i).first;
      t = (*i).second;
      CONS_Printf("- %s, rc = %d, use = %d\n", name, t->refcount, t->usefulness);
    }
}


// Erases unused items (refcount == 0) from cache and makes
// their data purgable (it _may_ still remain in L1 cache)
int L2cache_t::Cleanup()
{
  int k = 0;
  c_iter_t i, j;
  for (i = j = c_map.begin(); i != c_map.end(); j = i)
    {
      cacheitem_t *t = (*i).second;
      i++;
      if (t->refcount == 0)
	{
	  c_map.erase(j); // erase it from the hash_map
	  // Once an iterator is erased, it becomes invalid
	  // and cannot be incremented! Therefore we have both i and j.
	  Free(t); // free the data
	  delete t; // delete the cacheitem itself
	  k++;
	}
    }
  return k;
}


// Recaches and converts every item in cache using their names.
void L2cache_t::Flush()
{
  Cleanup(); // remove unused items

  cacheitem_t *t, *s;
  // default item is also stored in the map with the others
  // recache the items preserving refcounts
  c_iter_t i;
  for (i = c_map.begin(); i != c_map.end(); i++)
    {
      const char *name = (*i).first;
      t = (*i).second;

      Free(t);
      s = Load(name, t);
      if (s == NULL)
	{
	  // not found
	  if (t == default_item)
	    I_Error("L2cache: Unable to recache default_item '%s'!\n", default_name);
	  // FIXME. Normally, the defaultitem would be used, but
	  // since we must preserve pointers to the cacheitem, we'll have to do something else.
	  // (*i).second = default_item;? what about refcount?
	  I_Error("L2cache: Unable to recache item '%s'\n", name);
	}
    }
}
