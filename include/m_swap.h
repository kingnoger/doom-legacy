// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
/// \brief Endianess handling, byte order in 16 bit and 32 bit numbers.

#ifndef m_swap_h
#define m_swap_h 1

#include "doomtype.h"

// WAD files are always little endian.

#ifdef __BIG_ENDIAN__
# define SHORT(x) ((Sint16)( \
(((Uint16)(x) & (Uint16)0x00ffU) << 8) | \
(((Uint16)(x) & (Uint16)0xff00U) >> 8) ))
# define LONG(x) ((Sint32)( \
(((Uint32)(x) & (Uint32)0x000000ffUL) << 24) | \
(((Uint32)(x) & (Uint32)0x0000ff00UL) <<  8) | \
(((Uint32)(x) & (Uint32)0x00ff0000UL) >>  8) | \
(((Uint32)(x) & (Uint32)0xff000000UL) >> 24) ))
#else
# define SHORT(x) (x)
# define LONG(x)  (x)
#endif


#endif
