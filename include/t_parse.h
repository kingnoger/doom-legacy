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
// Revision 1.6  2002/09/25 15:17:42  vberghol
// Intermission fixed?
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
// Revision 1.3  2001/08/06 23:57:10  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.2  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------


#ifndef __PARSE_H__
#define __PARSE_H__

#include "m_fixed.h"

class Actor;
class PlayerInfo;

#define T_MAXTOKENS 128
#define TOKENLENGTH 128

#define intvalue(v)                                               \
  ( (v).type == svt_string ? atoi((v).value.s) :                  \
    (v).type == svt_fixed ? ((v).value.f / FRACUNIT) :            \
    (v).type == svt_mobj ? (v).value.mobj ? 1 : 0 : (v).value.i )

#define fixedvalue(v)                                             \
  ( (v).type == svt_fixed ? (v).value.f :                         \
    (v).type == svt_string ? (atof((v).value.s) * FRACUNIT) :     \
    intvalue(v) * FRACUNIT )

struct svariable_t;

struct svalue_t
{
  int type;
  union
  {
    long i;
    fixed_t f;
    const char *s;
    char *labelptr; // goto() label
    Actor *mobj;
  } value;
};

const char *stringvalue(svalue_t v);

#include "t_vari.h"
#include "t_prepro.h"

#define MAXSCRIPTS 256

struct script_t
{
  // script data
  
  char *data;
  int scriptnum;  // this script's number
  int len;
  
  // {} sections
  
  section_t *sections[SECTIONSLOTS];
  
  // variables:
  
  svariable_t *variables[VARIABLESLOTS];
  
  // ptr to the parent script
  // the parent script is the script above this level
  // eg. individual linetrigger scripts are children
  // of the levelscript, which is a child of the
  // global_script
  script_t *parent;

  // child scripts.
  // levelscript holds ptrs to all of the level's scripts
  // here.
  
  script_t *children[MAXSCRIPTS];
  
  Actor *trigger;       // object which triggered this script
  PlayerInfo *player;   // player who triggered this script. may be NULL

  //SoM: Used for if/elseif/else statements
  bool  lastiftrue;
};

struct operator_t
{
  char *string;
  svalue_t (*handler)(int, int, int); // left, mid, right
  int direction;
};

enum
{
  forward,
  backward
};

void run_script(script_t *script);
void continue_script(script_t *script, char *continue_point);
void parse_include(char *lumpname);
void run_statement();
void script_error(char *s, ...);

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);
int find_operator_backwards(int start, int stop, char *value);

/******* tokens **********/

typedef enum
{
  to_name,   // a name, eg 'count1' or 'frag'
  to_number,
  to_oper,
  to_string,
  to_unset,
  to_function          // function name
} tokentype_t;

enum    // brace types: where current_section is a { or }
{
  bracket_open,
  bracket_close
};

extern svalue_t nullvar;
extern int script_debug;

extern script_t *current_script;
extern Actor *trigger_obj;
extern int killscript;

extern char *tokens[T_MAXTOKENS];
extern tokentype_t tokentype[T_MAXTOKENS];
extern int num_tokens;
extern char *rover;     // current point reached in parsing
extern char *linestart; // start of the current expression

extern section_t *current_section;
extern section_t *prev_section;
extern int bracetype;

// the global_script is the root
// script and contains only built-in
// FraggleScript variables/functions

extern script_t global_script; 
extern script_t hub_script;

#endif
