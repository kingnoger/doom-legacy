// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
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
// Revision 1.1  2002/11/16 14:18:28  hurdler
// Initial revision
//
// Revision 1.5  2002/08/17 21:21:55  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.4  2002/07/18 19:16:42  vberghol
// renamed a few files
//
// Revision 1.3  2002/07/01 21:00:57  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:30  vberghol
// Version 133 Experimental!
//
// Revision 1.2  2000/11/03 03:27:17  stroggonmeth
// Again with the bug fixing...
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------

#ifndef __T_SCRIPT_H__
#define __T_SCRIPT_H__


#include "t_parse.h"
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
void T_ClearScripts();
void T_RunScript(int n);
void T_RunThingScript(int);
void T_PreprocessScripts();
void T_DelayedScripts();
Actor *MobjForSvalue(svalue_t svalue);

        // console commands
void T_Dump();
void T_ConsRun();

extern script_t levelscript;
//extern script_t *scripts[MAXSCRIPTS];       // the scripts
extern Actor *t_trigger;

void T_AddCommands();

#endif

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2002/11/16 14:18:28  hurdler
// Initial revision
//
// Revision 1.5  2002/08/17 21:21:55  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.4  2002/07/18 19:16:42  vberghol
// renamed a few files
//
// Revision 1.3  2002/07/01 21:00:57  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:30  vberghol
// Version 133 Experimental!
//
// Revision 1.2  2000/11/03 03:27:17  stroggonmeth
// Again with the bug fixing...
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//---------------------------------------------------------------------------

