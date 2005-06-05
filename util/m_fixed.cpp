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
// $Log$
// Revision 1.2  2005/06/05 19:32:27  smite-meister
// unsigned map structures
//
// Revision 1.1.1.1  2002/11/16 14:18:38  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Fixed point math.

#include "i_system.h"
#include "m_fixed.h"

// Fixme. __USE_C_FIXED__ or something.
#ifndef USEASM

fixed_t FixedMul(fixed_t a, fixed_t b)
{
  return (Sint64(a) * Sint64(b)) >> FRACBITS;
}

fixed_t FixedDiv2(fixed_t a, fixed_t b)
{
# if 0
  Sint64 c = (Sint64(a) << 16) / Sint64(b);
  return fixed_t(c);
# endif

  double c = double(a) / double(b) * FRACUNIT;

  if (c >= 2147483648.0 || c < -2147483648.0)
    I_Error("FixedDiv: divide by zero");
  return fixed_t(c);
}

/*
//
// FixedDiv, C version.
//
fixed_t FixedDiv ( fixed_t   a, fixed_t    b )
{
    //I_Error("<a: %ld, b: %ld>",(long)a,(long)b);

    if ( (abs(a)>>14) >= abs(b))
        return (a^b)<0 ? MININT : MAXINT;

    return FixedDiv2 (a,b);
}
*/
#endif // useasm
