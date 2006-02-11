// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2005 Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//
//--------------------------------------------------------------------------


#ifndef t_func_h
#define t_func_h 1

#define AngleToFixed(x)  fixed_t(float(x) / (ANG45 / 45.0f))
#define FixedToAngle(x)  angle_t((x).Float() * (ANG45 / 45.0f))

#endif
