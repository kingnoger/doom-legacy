// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.3  2002/07/01 21:00:44  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:22  vberghol
// Version 133 Experimental!
//
// Revision 1.5  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.3  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   DeHackEd support
//
//-----------------------------------------------------------------------------

#ifndef dehacked_h
#define dehacked_h 1

class dehacked_t
{
private:
  int  num_errors;
  void Read_Misc(class Parser &p);

public:
  bool loaded;

  dehacked_t();
  bool LoadDehackedLump(const char *buf, int len);
  void error(char *first, ...);

  int idfa_armor;
  int idfa_armor_class;
  int idkfa_armor;
  int idkfa_armor_class;
  int god_health;

  int initial_health;
  int initial_bullets;
  int max_health;
  int maxsoul;

  int green_armor_class;
  int blue_armor_class;
  int soul_health;
  int mega_health;
};

extern dehacked_t DEH;

#endif
