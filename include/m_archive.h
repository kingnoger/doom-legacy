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
// $Log$
// Revision 1.7  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.6  2005/06/16 18:18:11  smite-meister
// bugfixes
//
// Revision 1.5  2005/03/16 21:16:08  smite-meister
// menu cleanup, bugfixes
//
// Revision 1.4  2004/08/18 14:35:20  smite-meister
// PNG support!
//
// Revision 1.3  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.2  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.1  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.8  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.7  2003/06/01 18:56:30  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.6  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.5  2003/04/08 09:46:07  smite-meister
// Bugfixes
//
// Revision 1.4  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Saving and loading, serialization

#ifndef m_archive_h
#define m_archive_h 1

#include <vector>
#include <map>

#include "doomtype.h"
#include "vect.h"
#include "m_fixed.h"

using namespace std;

#define SAVEGAME_DESC_SIZE 32

// little-endian
struct savegame_header_t
{
  char id_string[24];
  char version_string[8];
  char description[SAVEGAME_DESC_SIZE];
  Sint32 version;
  Sint32 compression_method;
  Uint32 uncompressed_size;
  Uint32 num_objects;
  // TODO: store a small screenshot to the savegame, display it in savegame menu...
  //byte screenshot[ggg];
};


/// \brief A class for archiving data, used for savegames.
///
/// This class only stores/retrieves simple general data types such as
/// bytes, ints, fixed_t's, vect's etc.
/// It takes care of the correct endianness.
/// Serialization of complex data types (classes etc.) 
/// is handled by the classes themselves (object.Serialize(archive))
///
/// The system can serialize a group classes belonging to a class hierarchy,
/// perhaps having pointers to one another. The pointer references must be saved also.
/// The group of classes forms a directed graph (pointers being the edges), possibly containing
/// cycles and joins.
///
/// Easiest done if the pointers are replaced by object indices that are unique to the savegame.
/// Conversion can be done using a STL map<Thinker *, int>, where the int is the index.
///
/// The simple way:
/// Two-pass algorithm, where the first pass builds the map...
/// De-serialization can be done with a two-pass algo and a STL vector.
///
/// But in fact we will use a recursive one-pass algorithm instead.
///
/// This should not be made any harder than it has to be;
/// Pointers to static Map items (sectors, mapthing_t's etc.) can be directly
/// replaced with the item's indices in their appropriate containers.

class LArchive
{
private:
  bool storing;  ///< an archive is either storing (saving) or retrieving (loading)

  savegame_header_t header;

  vector<byte> m_sbuf; ///< uncompressed buffer for storing
  byte *m_buf, *m_buf_end, *m_pos; ///< and for retrieving

  map<void *, int> pointermap;
  vector<void *>   pointervec;

public:
  class Map *active_map; ///< a small kludge for Thinker::Unserialize()

  LArchive();
  ~LArchive();

  inline bool IsStoring() const {return storing;};
  int Size() const;

  void Create(const char *descr);
  bool Open(byte *buffer, size_t length);
  int  Compress(byte **result, int method);

  int Read(byte *dest, size_t length);
  int Write(const byte *source, size_t length);

  LArchive &operator<<(byte &c);    ///< 8 bits
  LArchive &operator<<(Uint16 &c);  ///< 16 bits
  LArchive &operator<<(Uint32 &c);  ///< 32 bits

  inline LArchive &operator<<(int &c) { return operator<<(reinterpret_cast<Uint32 &>(c)); }
  inline LArchive &operator<<(long int &c) { return operator<<(reinterpret_cast<Uint32 &>(c)); }
  inline LArchive &operator<<(float &c) { return operator<<(reinterpret_cast<Uint32 &>(c)); }
  inline LArchive &operator<<(short &c) { return operator<<(reinterpret_cast<Uint16 &>(c)); }

  inline LArchive &operator<<(char &c) { return operator<<(reinterpret_cast<byte &>(c)); }
  inline LArchive &operator<<(bool &c) { return operator<<(reinterpret_cast<byte &>(c)); }

  //inline LArchive &operator<<(tic_t &t) { return operator<<(reinterpret_cast<Uint32 &>(t)); }

  LArchive &operator<<(fixed_t& c);

  template<typename U>
  LArchive &operator<<(vec_t<U>& v)
  {
    operator<<(v.x);
    operator<<(v.y);
    operator<<(v.z);
    return *this;
  }

  /*
  LArchive &operator << (double c);
  */

  LArchive &operator<<(string &s);
  LArchive &operator<<(char *&s); // Z_Mallocs the memory for the c-string when retrieving

  bool HasStored(void *p, unsigned &id);
  bool GetPtr(unsigned id, void *&p);
  void SetPtr(unsigned id, void *p);

  bool Marker(int m);
};


#endif
