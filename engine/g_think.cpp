// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
// $Log$
// Revision 1.9  2004/04/25 16:26:49  smite-meister
// Doxygen
//
// Revision 1.8  2004/01/02 14:21:21  smite-meister
// save bugfix
//
// Revision 1.7  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.6  2003/11/13 00:33:02  smite-meister
// Static initialization order fiasco fixed
//
// Revision 1.5  2003/11/12 11:07:19  smite-meister
// Serialization done. Map progression.
//
//
//
// DESCRIPTION:
//   TypeInfo and Thinker class implementation
//
//-----------------------------------------------------------------------------

#include "g_think.h"
#include "m_archive.h"
#include "z_zone.h"


map<unsigned, TypeInfo*>& TypeInfo::id_map()
{
  // This is to overcome the "static initialization order fiasco" :/
  // We MUST make sure that id_map is constructed before it is used during
  // the construction of the TypeInfo instances, so a normal static
  // member object is not enough.

  // When this function is first executed, the map is constructed.
  static map<unsigned, TypeInfo*> *temp = new map<unsigned, TypeInfo*>;
  return *temp;
} 


TypeInfo::TypeInfo(const char *n, thinker_factory_t f, TypeInfo *p)
{
  static map<unsigned, TypeInfo*>& id_ref = TypeInfo::id_map();

  type_id = id_ref.size() + 1; // get the next available id (zero is reserved for NULL)
  id_ref[type_id] = this;  // store the mapping
  name = n;
  factory = f;
  parent = p;
}


TypeInfo *TypeInfo::Find(unsigned c)
{
  static map<unsigned, TypeInfo*>& id_ref = TypeInfo::id_map();
  map<unsigned, TypeInfo*>::iterator i;

  i = id_ref.find(c);
  if (i == id_ref.end())
    return NULL; // not found
  return (*i).second;
}


// Since this class has no parent, we must implement it like this:
TypeInfo Thinker::_type("Thinker", Thinker::Create, NULL);


Thinker::Thinker()
{
  mp = NULL;
}

Thinker::~Thinker()
{}

int Thinker::Marshal(LArchive &a)
{
  return 0;
}

int Thinker::Serialize(Thinker *p, LArchive &a)
{
  unsigned id;
  // is it already there?
  if (a.HasStored(p, id))
    {
      // handles the case p == NULL as well
      a << id;
      return id;
    }

  a << id;
  a << p->Type()->type_id; // hmm. basically an unnecessary call, but less trouble this way?
  p->Marshal(a);
  return id;
}


Thinker *Thinker::Unserialize(LArchive &a)
{
  unsigned id;
  a << id;

  Thinker *p = NULL;
  if (a.GetPtr(id, (void *)p))
    return p; // already unserialized, just return the appropriate pointer

  // not unserialized yet, so let's do it now:
  unsigned type;
  a << type;
  TypeInfo *t = TypeInfo::Find(type);
  if (!t)
    {
      return NULL; // TODO: How to handle errors? using exceptions?
    }
  p = t->factory();
  a.SetPtr(id, p); // order is important: consider the case a -> b -> a, where -> is a pointer

  p->mp = a.active_map; // a small kludge, some Marshal functions need to have a valid Map*
  p->Marshal(a);
  return p;
}


void *Thinker::operator new (size_t size)
{
  return Z_Malloc(size, PU_LEVSPEC, NULL);    
}

void Thinker::operator delete (void *mem)
{
  Z_Free(mem);
}
