// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.3  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      doom games standard types
//      Simple basic typedefs, isolated here to make it easier
//      separating modules.
//-----------------------------------------------------------------------------


#ifndef doomtype_h
#define doomtype_h 1

#ifdef __WIN32__
# include <windows.h>
#endif

#if !defined(_OS2EMX_H) && !defined(__WIN32__) 
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#ifdef __WIN32__
# define INT64  __int64
#else
# define INT64  long long
#endif

#ifdef __APPLE_CC__
# define __MACOS__
# define DEBUG_LOG
#endif

#if defined(__MSC__) || defined(__OS2__)
// Microsoft VisualC++
# define strncasecmp             strnicmp
# define strcasecmp              stricmp
# define inline                  __inline
#else
# ifdef __WATCOMC__
#  include <dos.h>
#  include <sys\types.h>
#  include <direct.h>
#  include <malloc.h>
#  define strncasecmp             strnicmp
#  define strcasecmp              strcmpi
# endif
#endif


#if defined(LINUX) // standard library differences
# define stricmp(x,y) strcasecmp(x,y)
# define strnicmp(x,y,n) strncasecmp(x,y,n)
# define lstrlen(x) strlen(x)
#endif

#ifdef __APPLE_CC__                //skip all boolean/Boolean crap
# define true 1
# define false 0
# define min(x,y) ( ((x)<(y)) ? (x) : (y) )
# define max(x,y) ( ((x)>(y)) ? (x) : (y) )
# define lstrlen(x) strlen(x)

# define stricmp strcmp
# define strnicmp strncmp

# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif //__MACOS__


typedef unsigned char byte;
typedef ULONG tic_t;
typedef unsigned int angle_t;

union RGBA_t
{
  ULONG rgba;
  struct {
    byte  red;
    byte  green;
    byte  blue;
    byte  alpha;
  } s;
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
