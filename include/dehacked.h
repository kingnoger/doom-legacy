// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// $Log$
// Revision 1.4  2004/12/31 16:19:40  smite-meister
// alpha fixes
//
// Revision 1.3  2004/12/19 23:30:17  smite-meister
// More BEX support
//
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1.1.1  2002/11/16 14:18:22  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.5  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief DeHackEd and BEX support

#ifndef dehacked_h
#define dehacked_h 1

#include "parser.h"

class dehacked_t
{
private:
  Parser p;
  int  num_errors;

  int FindValue();
  int ReadFlags(struct flag_mnemonic_t *mnemonics);

  void Read_Thing(int num);
  void Read_Frame(int num);
  void Read_Sound(int num);
  void Read_Text(int len1, int len2);
  void Read_Weapon(int num);
  void Read_Ammo(int num);
  void Read_Misc();
  void Read_Cheat();
  void Read_CODEPTR();
  void Read_STRINGS();

public:
  bool loaded;

  dehacked_t();
  bool LoadDehackedLump(const char *buf, int len);
  void error(char *first, ...);

  int   idfa_armor;
  float idfa_armorfactor;
  int   idkfa_armor;
  float idkfa_armorfactor;
  int   god_health;

  int max_health;
  int max_soul_health;
};

extern dehacked_t DEH;

#endif
