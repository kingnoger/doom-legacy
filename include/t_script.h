// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2003 Doom Legacy Team
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
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
//
//--------------------------------------------------------------------------

#ifndef t_script_h
#define t_script_h 1

#include "t_vari.h"
class Actor;
struct script_t;


typedef enum {
  wt_none,        // not waiting
  wt_delay,       // wait for a set amount of time
  wt_tagwait,     // wait for sector to stop moving
  wt_scriptwait,  // wait for script to finish
} wait_type_e;

struct runningscript_t
{
  script_t *script;
  
  // where we are
  char *savepoint;

  wait_type_e wait_type;
  int wait_data;  // data for wait: tagnum, counter, script number etc
	
  // saved variables
  svariable_t *variables[VARIABLESLOTS];
  
  runningscript_t *prev, *next;  // for chain
  Actor *trigger;
};

void T_Init();
void T_AddCommands();
void T_RunThingScript(int);
Actor *MobjForSvalue(svalue_t svalue);

// console commands
void T_Dump();
void T_ConsRun();

extern Actor *t_trigger;

#endif
