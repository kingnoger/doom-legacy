// Emacs style mode select   -*- C++ -*-
//---------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
/// \brief Parser driver header for Flex/Lemon parsers.

#ifndef driver_h
#define driver_h 1

#include "doomdef.h"
#include "r_data.h"


#ifndef YYSTYPE
union yy_t
{
  float ftype;
  int   itype;
  const char *stype; // copy of the parsed string, must be destroyed later on
};
# define YYSTYPE yy_t // sizeof(yy_t) == small, no point in using a pointer
# define YYSTYPE_IS_TRIVIAL 1
#endif


/// temp variables for building Textures
struct ntexture_driver
{
  bool is_sprite;
  std::string texname;
  class Texture *t;
  LumpTexture dummy;
  bool texeloffsets;

  ntexture_driver() : dummy("dummy", 0, 0, 0) {}
};


/// temp variables for building ActorInfos
typedef void decorate_driver;


#endif
