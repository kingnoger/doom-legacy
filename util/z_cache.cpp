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


cacheitem_t::~cacheitem_t()
{
  if (data)
    Z_ChangeTag(data, PU_CACHE); // release data
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

// just to shut up compiler, since entire caches are probably never destroyed
L2cache_t::~L2cache_t()
{
  if (default_item)
    delete default_item;
  // should empty the map too and delete its items
}

void L2cache_t::SetDefaultItem(const char *defitem)
{
  cacheitem_t *t = CreateItem(defitem);
  if (!t)
    I_Error("Default cache item '%s' not found!\n", defitem);

  default_name = defitem;
  default_item = t;  
}


// if the derived cache class uses a derived cacheitem_t, this must be redefined also
cacheitem_t *L2cache_t::CreateItem(const char *p)
{
  cacheitem_t *t = new cacheitem_t; // here

  int lump = fc.FindNumForName(p, false);
  if (lump == -1)
    return NULL;

  t->lumpnum = lump;
  t->refcount = 0;
  LoadAndConvert(t);
  
  return t;
}

// this must be redefined for a derived cache to convert the data properly.
void L2cache_t::LoadAndConvert(cacheitem_t *t)
{
  t->data = fc.CacheLumpNum(t->lumpnum, tagtype);
  t->length = fc.LumpLength(t->lumpnum);
}


// Checks if p is already in cache. If so, increments refcount and returns it.
// If not, tries to cache and convert p. If succesful, returns it.
// If not, returns the defaultitem.
cacheitem_t *L2cache_t::Cache(const char *p)
{
  // data used through a L2 cache must not be used anywhere else (z_free and z_changetag will cause problems)

  cacheitem_t *t;
  c_iter_t i = c_map.find(p);

  if (i == c_map.end())
    {
      // not found
      t = CreateItem(p);
      if (t)
	c_map.insert(c_map_t::value_type(p, t));
      else
	t = default_item;
    }
  else
    {
      // found
      t = (*i).second;
    }

  t->refcount++;

  return t;
}

void L2cache_t::Inventory()
{
  c_iter_t i;

  CONS_Printf("Defitem %s, rc = %d\n", default_name, default_item->refcount);
  for (i = c_map.begin(); i != c_map.end(); i++)
    {
      const char *name = (*i).first;
      cacheitem_t   *t = (*i).second;
      CONS_Printf("- %s, lump = %d, rc = %d, data = %p, len = %d\n", name, t->lumpnum, t->refcount, t->data, t->length);
    }
}


// Erases unused items (refcount == 0) from cache and makes
// their data purgable (it _may_ still remain in L1 cache)
int L2cache_t::Cleanup()
{
  int k = 0;
  c_iter_t i;
  for (i = c_map.begin(); i != c_map.end(); i++)
    {
      cacheitem_t *t = (*i).second;
      if (t->refcount == 0)
	{
	  c_map.erase(i);
	  delete t;
	  k++;
	}
    }
  return k;
}


// Recaches and converts every item in cache using their names.
void L2cache_t::Flush()
{
  Cleanup(); // remove unused items

  // recache the default item, preserve refcount
  int lump = fc.FindNumForName(default_name, false);
  if (lump == -1)
    I_Error("Default cache item '%s' not found!\n", default_name);

  default_item->lumpnum = lump;
  Z_Free(default_item->data);
  LoadAndConvert(default_item);

  // recache the other items preserving refcounts
  c_iter_t i;
  for (i = c_map.begin(); i != c_map.end(); i++)
    {
      const char *name = (*i).first;
      cacheitem_t *t   = (*i).second;

      lump = fc.FindNumForName(name, false);
      if (lump == -1)
	{
	  Z_Free(t->data);
	  // FIXME. Normally, the defaultitem would be used, but
	  // since we must preserve the cacheitem, we'll have to do something else.
	  // Now this becomes in essence another default_item, which is never released.
	  // another way: just put NULL into data?
	  t->data = default_item->data;
	  t->refcount += default_item->refcount + 1;
	  default_item->refcount = t->refcount; // not really necessary.
	}
      else
	{
	  t->lumpnum = lump;
	  Z_Free(t->data);
	  LoadAndConvert(t);
	}
    }
}
