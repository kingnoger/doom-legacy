// Emacs style mode select   -*- C++ -*- 
//
// $Id$
//
// Auxiliary STL functors for various purposes

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

#endif
