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
/// \brief Fixed point math.

#include "tnl/tnlBitStream.h"
#include "i_system.h"
#include "m_fixed.h"

/// smallest possible increment
fixed_t fixed_epsilon(1/float(fixed_t::UNIT));

/// OpenTNL packing method
void fixed_t::Pack(class BitStream *s)
{
  // TODO: save some bandwidth
  s->writeInt(val, 32);
}

/// OpenTNL unpacking method
void fixed_t::Unpack(class BitStream *s)
{
  val = s->readInt(32);
}
