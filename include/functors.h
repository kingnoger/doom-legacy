// Emacs style mode select   -*- C++ -*- 
//---------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003 by DooM Legacy Team.
//
//---------------------------------------------------------------------

/// \file
/// \brief Auxiliary STL functors for various purposes

#ifndef functors_h
#define functors_h 1

#include <string.h>


struct equal_pointer
{
  bool operator()(const void* p1, const void* p2) const
  { return p1 == p2; }
};


struct less_pointer
{
  bool operator()(const void* p1, const void* p2) const
  { return p1 < p2; }
};


struct equal_cstring
{
  bool operator()(const char* s1, const char* s2) const
  { return strcmp(s1, s2) == 0; }
};


struct less_cstring
{
  bool operator()(const char* s1, const char* s2) const
  { return strcmp(s1, s2) < 0; }
};



struct equal_cstring8
{
  bool operator()(const char* s1, const char* s2) const
  { return strncmp(s1, s2, 8) == 0; }
};

struct hash_cstring8
{
  size_t operator()(const char* s) const
  {
    unsigned long h = 0; 
    for (int i=0; s[i] && i < 8; ++i)
      h = 5*h + s[i];
  
    return size_t(h);
  }
};

#endif
