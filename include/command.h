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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Command buffer

#ifndef command_h
#define command_h 1

#include <stdio.h>

//===================================
// Command buffer & command execution
//===================================

typedef void (*com_func_t)();
extern class PlayerInfo *com_player;

void    COM_AddCommand(char *name, com_func_t func);

int     COM_Argc();
char    *COM_Argv(int arg);   // if argv>argc, returns empty string
char    *COM_Args();
int     COM_CheckParm(char *check); // like M_CheckParm :)

// match existing command or NULL
char    *COM_CompleteCommand(char *partial, int skips);

// insert at queu (at end of other command)
void    COM_BufAddText(char *text);

// insert in head (before other command)
void    COM_BufInsertText(char *text);

// Execute commands in buffer, flush them
void    COM_BufExecute();

// setup command buffer, at game tartup
void    COM_Init();




//==================
// Console variables
//==================

/// flags for console variables
enum cvflags_t
{
  CV_SAVE       = 0x0001,    ///< cvar is saved in the config file when the game exits
  CV_CALL       = 0x0002,    ///< call function on change
  CV_NETVAR     = 0x0004,    ///< sent over net from server to clients
  CV_NOINIT     = 0x0008,    ///< don't call function when cvar is registered (1st set)
  CV_FLOAT      = 0x0010,    ///< the value is 16.16 fixed point
  CV_NOTINNET   = 0x0020,    ///< can't be changed in netgame (NOTE: currently not used)
  CV_MODIFIED   = 0x0040,    ///< set when cvar is modified
  CV_ANNOUNCE   = 0x0080,    ///< print a message to console when modified
  CV_ANNOUNCE_ONCE = 0x0100, ///< same but will be reset when modified, set in toggle
  CV_HIDDEN     = 0x0200,    ///< cannot be accessed by the console (not part of the cvar list) (menu etc.)
};

/// \brief value restriction for consvar_t objects
struct CV_PossibleValue_t
{
  int   value;
  char *strvalue;
};

extern CV_PossibleValue_t CV_OnOff[];
extern CV_PossibleValue_t CV_YesNo[];
extern CV_PossibleValue_t CV_Unsigned[];


namespace TNL { class BitStream; };

/// \brief console variables
///
/// Console vars are variables that can be changed through code or console,
/// at RUN TIME. They can also act as simplified commands, because a func-
/// tion can be attached to a console var, which is called whenever the
/// variable is modified (using flag CV_CALL).

struct consvar_t
{
  char      *name;
  char      *defaultvalue;
  int        flags;        ///< flags from cvflags_t
  CV_PossibleValue_t *PossibleValue;  ///< array of possible values
  void     (*func)();      ///< called on change, if CV_CALL is set
  int        value;        ///< int/fixed_t value
  char      *str;          ///< value in string format
  unsigned short netid;    ///< unique network id for CV_NETVARs

  consvar_t *next;         ///< linked list

  static consvar_t *cvar_list; ///< list of registered console variables

protected:
  /// internal setting method
  void Setvalue(const char *s);

  static unsigned short ComputeNetid(const char *s);
  static consvar_t *FindNetVar(unsigned short netid);

public:

  /// as if "<varname> <value>" was entered at the console
  void Set(const char *value);

  /// expands value to a string before setting it
  void Set(int value);

  /// it a setvalue but with a modulo at the maximum
  void AddValue(int increment);

  /// register a variable for use at the console
  bool Reg();

  /// displays or changes variable through the console
  static bool Command();

  /// returns the cvar with the corresponding name or NULL
  static consvar_t *FindVar(const char *name);

  /// returns the name of the nearest console variable name found
  static const char *CompleteVar(const char *partial, int skips);

  /// write all CV_SAVE variables to config file
  static void SaveVariables(FILE *f);

  /// save all CV_NETVAR variables into a buffer
  static void SaveNetVars(TNL::BitStream &s);
  /// load all CV_NETVAR variables from a buffer
  static void LoadNetVars(TNL::BitStream &s);
  /// received a new value for a netvar
  static void GotNetVar(unsigned short id, const char *str);
};

#endif
