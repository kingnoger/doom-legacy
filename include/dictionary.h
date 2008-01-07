// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
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
/// \brief Hashed dictionary template class.


#ifndef dictionary_h
#define dictionary_h 1

#if (__GNUC__ != 2)
# include <ext/hash_map>
#else
# include <hash_map>
#endif

#include "functors.h"


/// \brief Dictionary class based on hash_map.
///
/// NOTE: T must have an (inlined) method const char *GetName().
/// The pointer must stay valid until the object is removed from the dictionary.
template<typename T>
class HashDictionary
{
protected:
  // annoying namespace declarations, because hash_map is an extension...
#if (__GNUC__ != 2)
  typedef typename __gnu_cxx::hash_map<const char*, T*, __gnu_cxx::hash<const char*>, equal_cstring> dict_map_t;
#else
  typedef typename            hash_map<const char*, T*, hash<const char*>, equal_cstring> dict_map_t;
#endif

  typedef typename dict_map_t::iterator dict_iter_t;
  dict_map_t dict_map; ///< hash_map from object names to corresponding object pointers.

public:
  /// Destructor: delete the contents.
  ~HashDictionary()
  {
    Clear();
  }

  /// The safe way of inserting stuff into the hash_map.
  /// The main point is that 'name' is stored within the T structure itself.
  /// If the hash_map already contains an item with the same key (name) as p, nothing is done.
  /// \return true if p was succesfully inserted.
  inline bool Insert(T *p)
  {
    return dict_map.insert(typename dict_map_t::value_type(p->GetName(), p)).second;
  }


  /// Since hash_map is a unique associative container we need this, cannot just Insert new stuff with same key.
  /// \return 1 if an old element was replaced, 0 if not.
  inline int Replace(T *p)
  {
    int n = dict_map.erase(p->GetName()); // erase the old instance by key
    dict_map.insert(typename dict_map_t::value_type(p->GetName(), p));
    return n;
  }


  /// Remove the named element, if it exists.
  /// \return Number of elements removed (0 or 1).
  inline int Remove(const char *name)
  {
    return dict_map.erase(name); // erase the old instance by key
  }


  /// Tries to find the named item from the dictionary.
  /// \return The element associated with the key name, or NULL if none is found.
  inline T *Find(const char *name)
  {
    dict_iter_t s = dict_map.find(name);

    if (s == dict_map.end())
      {
	// not found
	return NULL;
      }
    else
      {
	// got it
	return s->second;
      }
  }

  /// Dictionary size.
  /// \return The number of items in the dictionary.
  inline unsigned Size() const { return dict_map.size(); }

  /// Erases all dictionary items.
  void Clear()
  {
    for (dict_iter_t t = dict_map.begin(); t != dict_map.end(); )
      {
	dict_iter_t s = t++; // because... well, incrementing the iterator apparently uses its name, which must not yet be deleted.
	delete s->second;
      }

    dict_map.clear();
  }
};

#endif
