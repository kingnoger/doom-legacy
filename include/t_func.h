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
// $Log$
// Revision 1.6  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.5  2005/04/22 19:44:50  smite-meister
// bugs fixed
//
// Revision 1.4  2004/08/12 18:30:30  smite-meister
// cleaned startup
//
// Revision 1.3  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.2  2002/12/16 22:05:17  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
//--------------------------------------------------------------------------


#ifndef t_func_h
#define t_func_h 1

#define AngleToFixed(x)  fixed_t(float(x) / (ANG45 / 45.0f))
#define FixedToAngle(x)  angle_t((x).Float() * (ANG45 / 45.0f))

#endif
