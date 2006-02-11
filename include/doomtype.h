// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Basic typedefs and platform-dependent #defines

#ifndef doomtype_h
#define doomtype_h 1


// Standard library differences

#ifdef __WIN32__
# include <windows.h>
# define ASMCALL __cdecl
#else
# define ASMCALL
#endif

#ifdef __APPLE_CC__
# define __MACOS__
# define DEBUG_LOG
# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif

#if defined(__MSC__) || defined(__OS2__)   // Microsoft VisualC++
# define strncasecmp             strnicmp
# define strcasecmp              stricmp
# define inline                  __inline
#elseif defined(__WATCOMC__)
# define strncasecmp             strnicmp
# define strcasecmp              strcmpi
#endif

#if defined(LINUX)
# define O_BINARY 0
#endif


// Basic typedefs.
// Throughout the code, we assume that int = 32 bits, short = 16 bits

typedef signed char  Sint8;
typedef signed short Sint16;
typedef signed int   Sint32;
#ifdef __WIN32__
typedef __int64 Sint64;
#else
typedef signed long long Sint64;
#endif

typedef unsigned char  byte;
typedef unsigned char  Uint8;
typedef unsigned short Uint16;
typedef unsigned int   Uint32;

typedef Uint32 tic_t;
typedef Uint32 angle_t;


struct RGB_t
{
  byte r, g, b;
};


union RGBA_t
{
  Uint32 rgba;
  struct
  {
    byte  red;
    byte  green;
    byte  blue;
    byte  alpha;
  };
};



// Predefined with some OS.
#ifndef __WIN32__
# ifndef __MACOS__
#  ifndef FREEBSD
#   include <values.h>
#  else
#   include <limits.h>
#  endif
# endif
#endif

#ifndef MAXCHAR
# define MAXCHAR   ((char)0x7f)
#endif
#ifndef MAXSHORT
# define MAXSHORT  ((short)0x7fff)
#endif
#ifndef MAXINT
# define MAXINT    ((int)0x7fffffff)
#endif
#ifndef MAXLONG
# define MAXLONG   ((long)0x7fffffff)
#endif

#ifndef MINCHAR
# define MINCHAR   ((char)0x80)
#endif
#ifndef MINSHORT
# define MINSHORT  ((short)0x8000)
#endif
#ifndef MININT
# define MININT    ((int)0x80000000)
#endif
#ifndef MINLONG
# define MINLONG   ((long)0x80000000)
#endif

#endif
