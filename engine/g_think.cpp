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


map<unsigned, TypeInfo*> TypeInfo::IDmap; // filled during static initialization


TypeInfo::TypeInfo(const char *n, thinker_factory_t f)
{
  unsigned id = IDmap.size() + 1; // get the next available id (zero is reserved)
  IDmap[id] = this;  // store the mapping
  name = n;
  factory = f;
}


TypeInfo *TypeInfo::Find(unsigned c)
{
  map<unsigned, TypeInfo*>::iterator i;

  i = IDmap.find(c);
  if (i == IDmap.end())
    return NULL; // not found
  return (*i).second;
}


IMPLEMENT_CLASS(Thinker, "Thinker");

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
  a << p->_Type()->type_id; // hmm. basically an unnecessary call, but less trouble this way?
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
    return NULL; // TODO: How to handle errors? using exceptions?
  p = t->factory();
  p->mp = a.active_map; // a small kludge, some Marshal functions need to have a valid Map*
  p->Marshal(a);
  a.SetPtr(id, p);
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
