// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003 by DooM Legacy Team.
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
// Revision 1.4  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.3  2003/11/30 00:09:48  smite-meister
// bugfixes
//
// Revision 1.2  2003/11/12 11:14:52  smite-meister
// cr/lf
//
// Revision 1.1  2003/11/12 11:13:15  smite-meister
// Serialization done. Map progression.
//
//
//
// DESCRIPTION:
//   AC Script interpreter
//
//-----------------------------------------------------------------------------

#ifndef p_acs_h
#define p_acs_h 1

#include <map>


#define MAX_ACS_SCRIPT_VARS 10
#define MAX_ACS_WORLD_VARS 64
#define ACS_STACK_DEPTH 32

void P_ACSInitNewGame();
bool P_AddToACSStore(int tmap, int number, byte *args);

class acs_t : public Thinker
{
  DECLARE_CLASS(acs_t);
public:
  Actor *activator;
  line_t *line;
  int side;
  int number;
  int infoIndex;
  int delayCount;
  int stackPtr;
  int stak[ACS_STACK_DEPTH];
  int vars[MAX_ACS_SCRIPT_VARS];
  int *ip;

public:
  acs_t(int num, int infoindex, int *ip);

  virtual void Think();
};


struct acsstore_t
{
  int tmap;	// Target map
  int script;	// Script number on target map
  byte args[4];	// Padded to 4 for alignment
};

extern int WorldVars[MAX_ACS_WORLD_VARS];
extern multimap<int, acsstore_t> ACS_store;

typedef multimap<int, acsstore_t>::iterator acsstore_iter_t;


enum acs_state_t
{
  ACS_inactive,
  ACS_running,
  ACS_suspended,
  ACS_waitfortag,
  ACS_waitforpoly,
  ACS_waitforscript,
  ACS_terminating
};

struct acsInfo_t
{
  int  number;
  int *address;
  int  argCount;
  acs_state_t state;
  int  waitValue;
};


#endif
