// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Command buffer and console variables
///   
/// Parse and execute commands from console input/scripts/
/// and remote server.
///
/// Handles console variables, which are a simplified version
/// of commands. Each consvar can have a function which is called
/// when it is modified.
///
/// Code shamelessly inspired by the QuakeC sources, thanks Id :)

#include "tnl/tnlBitStream.h"

#include "doomdef.h"
#include "command.h"
#include "console.h"
#include "z_zone.h"

#include "n_interface.h"

#include "m_misc.h"
#include "m_fixed.h"

#include "g_game.h"
#include "g_player.h"

extern bool devparm;			//in d_main.cpp

// =========================================================================
//                      VARIABLE SIZE BUFFERS
// =========================================================================

/// \brief Variable size buffer
struct vsbuf_t
{
  enum
  {
    VSBUFMINSIZE = 256
  };

  bool  allowoverflow;  // if false, do a I_Error
  bool  overflowed;     // set to true if the buffer size failed
  byte *data;
  int   maxsize;
  int   cursize;

  void  VS_Alloc(int initsize);
  void  VS_Free();
  void  VS_Clear();
  void *VS_GetSpace(int length);
  void  VS_Write(void *data, int length);  ///<  Copy data at end of variable sized buffer
  void  VS_Print(char *data); ///<  Print text in variable size buffer, like VS_Write + trailing 0
};




void vsbuf_t::VS_Alloc(int initsize)
{
  if (initsize < VSBUFMINSIZE)
    initsize = VSBUFMINSIZE;
  data = (byte *)Z_Malloc(initsize, PU_STATIC, NULL);
  maxsize = initsize;
  cursize = 0;

  allowoverflow = false;
  overflowed = false;
}


void vsbuf_t::VS_Free()
{
  //  Z_Free(data);
  cursize = 0;
}


void vsbuf_t::VS_Clear()
{
  cursize = 0;
}


void *vsbuf_t::VS_GetSpace(int length)
{
  if (cursize + length > maxsize)
    {
      if (!allowoverflow)
        I_Error("overflow 111");

      if (length > maxsize)
        I_Error("overflow l%i 112", length);

      overflowed = true;
      CONS_Printf("VS buffer overflow");
      VS_Clear();
    }

  void *temp = data + cursize;
  cursize += length;

  return temp;
}


void vsbuf_t::VS_Write(void *newdata, int length)
{
  memcpy(VS_GetSpace(length), newdata, length);
}


void vsbuf_t::VS_Print(char *newdata)
{
  int len = strlen(newdata) + 1;

  if (data[cursize - 1])
    memcpy((byte *)VS_GetSpace(len), newdata, len); // no trailing 0
  else
    memcpy((byte *)VS_GetSpace(len - 1) - 1, newdata, len); // write over trailing 0
}




//=========================================================================
//                           COMMAND BUFFER
//=========================================================================

static bool COM_Exists (char *com_name);
static void COM_ExecuteString (char *text);

static void COM_Alias_f();
static void COM_Echo_f();
static void COM_Exec_f();
static void COM_Wait_f();
static void COM_Help_f();
static void COM_Toggle_f();

static char com_token[1024];
static char *COM_Parse(char *data);

CV_PossibleValue_t CV_OnOff[] =    {{0,"Off"}, {1,"On"},    {0,NULL}};
CV_PossibleValue_t CV_YesNo[] =    {{0,"No"} , {1,"Yes"},   {0,NULL}};
CV_PossibleValue_t CV_Unsigned[] = {{0,"MIN"}, {999999999,"MAX"}, {0,NULL}};

#define COM_BUF_SIZE    8192   // command buffer size

int     com_wait;       // one command per frame (for cmd sequences)


// command aliases
//
struct cmdalias_t
{
  cmdalias_t *next;
  char    *name;
  char    *value;     // the command string to replace the alias
};

cmdalias_t  *com_alias; // aliases list



static vsbuf_t com_text;     // variable sized buffer


//  Add text in the command buffer (for later execution)
//
void COM_BufAddText(char *text)
{
  int l = strlen(text);

  if (com_text.cursize + l >= com_text.maxsize)
    {
      CONS_Printf("Command buffer full!\n");
      return;
    }
  com_text.VS_Write(text, l);
}


// Adds command text immediately after the current command
// Adds a \n to the text
//
void COM_BufInsertText(char *text)
{
  char    *temp;
  int     templen;

  // copy off any commands still remaining in the exec buffer
  templen = com_text.cursize;
  if (templen)
    {
      temp = (char *)ZZ_Alloc(templen);
      memcpy(temp, com_text.data, templen);
      com_text.VS_Clear();
    }
  else
    temp = NULL;    // shut up compiler

  // add the entire text of the file (or alias)
  COM_BufAddText(text);

  // add the copied off data
  if (templen)
    {
      com_text.VS_Write(temp, templen);
      Z_Free(temp);
    }
}


//  Flush (execute) console commands in buffer
//   does only one if com_wait
//
void COM_BufExecute()
{
  int     i;
  char    *text;
  char    line[1024];
  int     quotes;

  if (com_wait)
    {
      com_wait--;
      return;
    }

  while (com_text.cursize)
    {
      // find a '\n' or ; line break
      text = (char *)com_text.data;

      quotes = 0;
      for (i=0 ; i< com_text.cursize ; i++)
        {
          if (text[i] == '"')
            quotes++;
          if ( !(quotes&1) &&  text[i] == ';')
            break;  // don't break if inside a quoted string
          if (text[i] == '\n' || text[i] == '\r')
            break;
        }

      memcpy (line, text, i);
      line[i] = 0;

      // flush the command text from the command buffer, _BEFORE_
      // executing, to avoid that 'recursive' aliases overflow the
      // command text buffer, in that case, new commands are inserted
      // at the beginning, in place of the actual, so it doesn't
      // overflow
      if (i == com_text.cursize)
        // the last command was just flushed
        com_text.cursize = 0;
      else
        {
          i++;
          com_text.cursize -= i;
          memcpy (text, text+i, com_text.cursize);
        }

      // execute the command line
      COM_ExecuteString(line);

      // delay following commands if a wait was encountered
      if (com_wait)
        {
          com_wait--;
          break;
        }
    }
}


// =========================================================================
//                            COMMAND EXECUTION
// =========================================================================

struct xcommand_t
{
  char        *name;
  xcommand_t  *next;
  com_func_t   function;
};

static  xcommand_t  *com_commands = NULL;     // current commands


#define MAX_ARGS        80
static int         com_argc;
static char       *com_argv[MAX_ARGS];
static char       *com_null_string = "";
static char       *com_args = NULL;          // current command args or NULL

PlayerInfo *com_player; // player associated with the current command. 
// Only "interactive" commands may require this.


//  Initialise command buffer and add basic commands
void COM_Init()
{
  CONS_Printf("COM_Init: Init the command buffer\n");

  // allocate command buffer
  com_text.VS_Alloc(COM_BUF_SIZE);

  // add standard commands
  COM_AddCommand ("alias",COM_Alias_f);
  COM_AddCommand ("echo", COM_Echo_f);
  COM_AddCommand ("exec", COM_Exec_f);
  COM_AddCommand ("wait", COM_Wait_f);
  COM_AddCommand ("help", COM_Help_f);
  COM_AddCommand ("toggle", COM_Toggle_f);
}


// Returns how many args for last command
//
int COM_Argc()
{
  return com_argc;
}


// Returns string pointer for given argument number
//
char *COM_Argv (int arg)
{
  if ( arg >= com_argc || arg < 0 )
    return com_null_string;
  return com_argv[arg];
}


// Returns string pointer of all command args
//
char *COM_Args()
{
  return com_args;
}


int COM_CheckParm(char *check)
{
  for (int i = 1; i < com_argc; i++)
    {
      if (!strcasecmp(check, com_argv[i]))
        return i;
    }
  return 0;
}


// Parses the given string into command line tokens.
//
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.
static void COM_TokenizeString(char *text)
{
  // clear the args from the last string
  for (int i=0 ; i<com_argc ; i++)
    if (com_argv[i])
      {
	Z_Free(com_argv[i]);
	com_argv[i] = NULL;
      }

  com_argc = 0;
  com_args = NULL;

  while (1)
    {
      // skip whitespace up to a /n
      while (*text && *text <= ' ' && *text != '\n')
        text++;

      if (*text == '\n')
        {   // a newline means end of command in buffer,
          // thus end of this command's args too
          text++;
          break;
        }

      if (!*text)
        return;

      if (com_argc == 1)
        com_args = text;

      text = COM_Parse(text);
      if (!text)
        return;

      if (com_argc < MAX_ARGS)
        {
          com_argv[com_argc] = (char *)ZZ_Alloc(strlen(com_token) + 1);
          strcpy(com_argv[com_argc], com_token);
          com_argc++;
        }
    }
}


// Add a command before existing ones.
//
void COM_AddCommand(char *name, com_func_t func)
{
  xcommand_t  *cmd;

  // fail if the command is a variable name
  if (consvar_t::FindVar(name))
    {
      CONS_Printf ("%s is a variable name\n", name);
      return;
    }

  // fail if the command already exists
  for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
      if (!strcmp (name, cmd->name))
        {
          CONS_Printf ("Command %s already exists\n", name);
          return;
        }
    }

  cmd = (xcommand_t *)ZZ_Alloc(sizeof(xcommand_t));
  cmd->name = name;
  cmd->function = func;
  cmd->next = com_commands;
  com_commands = cmd;
}


//  Returns true if a command by the name given exists
//
static bool COM_Exists(char *com_name)
{
  xcommand_t  *cmd;

  for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
      if (!strcmp (com_name,cmd->name))
        return true;
    }

  return false;
}


//  Command completion using TAB key like '4dos'
//  Will skip 'skips' commands
//
char *COM_CompleteCommand(char *partial, int skips)
{
  xcommand_t  *cmd;
  int len = strlen(partial);

  if (!len)
    return NULL;

  // check functions
  for (cmd=com_commands ; cmd ; cmd=cmd->next)
    if (!strncmp (partial,cmd->name, len))
      if (!skips--)
        return cmd->name;

  return NULL;
}



// Parses a single line of text into arguments and tries to execute it.
// The text can come from the command buffer, a remote client, or stdin.
//
static void COM_ExecuteString(char *text)
{
  xcommand_t  *cmd;
  cmdalias_t *a;

  COM_TokenizeString(text);

  // execute the command line
  if (!COM_Argc())
    return;     // no tokens

  // try to find the player "using" the command buffer
  for (int i=0; i<NUM_LOCALPLAYERS; i++)
    {
      com_player = LocalPlayers[0].info;
      if (com_player)
	break;
    }

  // check functions
  for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
      if (!strcmp(com_argv[0],cmd->name))
        {
          cmd->function();
          return;
        }
    }

  // check aliases
  for (a=com_alias ; a ; a=a->next)
    {
      if (!strcmp (com_argv[0], a->name))
        {
          COM_BufInsertText (a->value);
          return;
        }
    }

  // check cvars
  // Hurdler: added at Ebola's request ;)
  // (don't flood the console in software mode with bad gr_xxx command)
  if (!consvar_t::Command())
    {
      CONS_Printf("Unknown command '%s'\n", COM_Argv(0));
    }
}



//============================================================================
//                            SCRIPT PARSE
//============================================================================

//  Parse a token out of a string, handles script files too
//  returns the data pointer after the token
static char *COM_Parse(char *data)
{
  if (!data)
    return NULL;

  int c;
  int len = 0;
  com_token[0] = 0;

  // skip whitespace
 skipwhite:
  while ( (c = *data) <= ' ')
    {
      if (c == 0)
        return NULL;            // end of file;
      data++;
    }

  // skip // comments
  if (c=='/' && data[1] == '/')
    {
      while (*data && *data != '\n')
        data++;
      goto skipwhite;
    }


  // handle quoted strings specially
  if (c == '\"')
    {
      data++;
      while (1)
        {
          c = *data++;
          if (c=='\"' || !c)
            {
              com_token[len] = 0;
              return data;
            }
          com_token[len] = c;
          len++;
        }
    }

  // parse single characters
  if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
    {
      com_token[len] = c;
      len++;
      com_token[len] = 0;
      return data+1;
    }

  // parse a regular word
  do
    {
      com_token[len] = c;
      data++;
      len++;
      c = *data;
      if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
        break;
    } while (c>32);

  com_token[len] = 0;
  return data;
}


// =========================================================================
//                            SCRIPT COMMANDS
// =========================================================================


// alias command : a command name that replaces another command
//
static void COM_Alias_f()
{
  cmdalias_t  *a;
  char        cmd[1024];
  int         i, c;

  if (COM_Argc()<3)
    {
      CONS_Printf("alias <name> <command>\n");
      return;
    }

  a = (cmdalias_t *)ZZ_Alloc(sizeof(cmdalias_t));
  a->next = com_alias;
  com_alias = a;

  a->name = Z_StrDup (COM_Argv(1));

  // copy the rest of the command line
  cmd[0] = 0;     // start out with a null string
  c = COM_Argc();
  for (i=2 ; i< c ; i++)
    {
      strcat(cmd, COM_Argv(i));
      if (i != c)
        strcat(cmd, " ");
    }
  strcat (cmd, "\n");

  a->value = Z_StrDup(cmd);
}


// Echo a line of text to console
//
static void COM_Echo_f()
{
  for (int i = 1; i < COM_Argc(); i++)
    CONS_Printf("%s ",COM_Argv(i));
  CONS_Printf("\n");
}


// Execute a script file
//
static void COM_Exec_f()
{
  if (COM_Argc() != 2)
    {
      CONS_Printf ("exec <filename> : run a script file\n");
      return;
    }

  byte *buf = NULL;

  // load file
  FIL_ReadFile(COM_Argv(1), &buf);

  if (!buf)
    {
      CONS_Printf ("Couldn't execute file %s\n",COM_Argv(1));
      return;
    }

  CONS_Printf ("Executing %s\n",COM_Argv(1));

  // insert text file into the command buffer
  COM_BufInsertText((char *)buf);

  // free buffer
  Z_Free(buf);
}


// Delay execution of the rest of the commands to the next frame,
// allows sequences of commands like "jump; fire; backward"
//
static void COM_Wait_f()
{
  if (COM_Argc()>1)
    com_wait = atoi(COM_Argv(1));
  else
    com_wait = 1;   // 1 frame
}

static void COM_Help_f()
{
  xcommand_t  *cmd;
  consvar_t  *cvar;
  int i=0;

  if(COM_Argc()>1)
    {
      cvar = consvar_t::FindVar(COM_Argv(1));
      if (cvar)
        {
          CONS_Printf("Variable %s:\n",cvar->name);
          CONS_Printf("  flags :");
          if( cvar->flags & CV_SAVE )
            CONS_Printf("AUTOSAVE ");
          if( cvar->flags & CV_FLOAT )
            CONS_Printf("FLOAT ");
          if( cvar->flags & CV_NETVAR )
            CONS_Printf("NETVAR ");
          if( cvar->flags & CV_CALL )
            CONS_Printf("ACTION ");
          CONS_Printf("\n");
          if( cvar->PossibleValue )
            {
              if(strcmp(cvar->PossibleValue[0].strvalue,"MIN")==0)
                {
                  for(i=1;cvar->PossibleValue[i].strvalue!=NULL;i++)
                    if(!strcmp(cvar->PossibleValue[i].strvalue,"MAX"))
                      break;
                  CONS_Printf("  range from %d to %d\n",cvar->PossibleValue[0].value,cvar->PossibleValue[i].value);
                }
              else
                {
                  CONS_Printf("  possible value :\n",cvar->name);
                  while(cvar->PossibleValue[i].strvalue)
                    {
                      CONS_Printf("    %-2d : %s\n",cvar->PossibleValue[i].value,cvar->PossibleValue[i].strvalue);
                      i++;
                    }
                }
            }
        }
      else
        CONS_Printf("No Help for this command/variable\n");
    }
  else
    {
      // commands
      CONS_Printf("\2Commands\n");
      for (cmd=com_commands ; cmd ; cmd=cmd->next)
        {
          CONS_Printf("%s ",cmd->name);
          i++;
        }

      // varibale
      CONS_Printf("\2\nVariable\n");
      for (cvar = consvar_t::cvar_list; cvar; cvar = cvar->next)
        {
          CONS_Printf("%s ",cvar->name);
          i++;
        }

      CONS_Printf("\2\nread console.txt for more or type help <command or variable>\n");

      if( devparm )
        CONS_Printf("\2Total : %d\n",i);
    }
}

static void COM_Toggle_f()
{
  if (COM_Argc() != 2 && COM_Argc() != 3)
    {
      CONS_Printf("Toggle <cvar_name> [-1]\n"
                  "Toggle the value of a cvar\n");
      return;
    }
  consvar_t *cvar = consvar_t::FindVar(COM_Argv(1));
  if (!cvar)
    {
      CONS_Printf("%s is not a cvar\n",COM_Argv(1));
      return;
    }

  // netcvar don't change imediately
  cvar->flags |= CV_ANNOUNCE_ONCE;
  if (COM_Argc() == 3)
    cvar->AddValue(atol(COM_Argv(2)));
  else
    cvar->AddValue(1);
}


// =========================================================================
//
//                           CONSOLE VARIABLES
//
//   console variables are a simple way of changing variables of the game
//   through the console or code, at run time.
//
//   console vars acts like simplified commands, because a function can be
//   attached to them, and called whenever a console var is modified
//
// =========================================================================

consvar_t *consvar_t::cvar_list = NULL;

//  Search if a variable has been registered
//  returns true if given variable has been registered
consvar_t *consvar_t::FindVar(const char *name)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (!strcmp(name, cvar->name))
      return cvar;

  return NULL;
}


//  Build a unique Net Variable identifier number, that is used
//  in network packets instead of the fullname
unsigned short consvar_t::ComputeNetid(const char *s)
{
  static int premiers[16] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53};

  unsigned short ret = 0;
  int i = 0;
  while (*s)
    {
      ret += (*s)*premiers[i];
      s++;
      i = (i+1)%16;
    }
  return ret;
}


//  Return the Net Variable, from it's identifier number
consvar_t *consvar_t::FindNetVar(unsigned short netid)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (cvar->netid == netid)
      return cvar;

  return NULL;
}

//
// set value to the variable, no checking, only for internal use
//
void consvar_t::Setvalue(const char *s)
{
  int i;
  char temp[100];

  if (PossibleValue)
    {
      char *tail;
      int v = strtol(s, &tail, 0);

      if (!strcmp(PossibleValue[0].strvalue,"MIN"))
        {
	  // bounded cvar
          // search for maximum
          for (i=1; PossibleValue[i].strvalue; i++)
            if (!strcmp(PossibleValue[i].strvalue,"MAX"))
              break;
#ifdef PARANOIA
          if (!PossibleValue[i].strvalue)
            I_Error("Bounded cvar \"%s\" without Maximum !", name);
#endif
          if (v < PossibleValue[0].value)
	    v = PossibleValue[0].value;
          else if (v > PossibleValue[i].value)
	    v = PossibleValue[i].value;

	  sprintf(temp, "%d", v);
	  s = temp;
        }
      else
        {
	  if (tail == s)
	    {
	      // no succesful number conversion, so it's a string
	      for (i=0; PossibleValue[i].strvalue; i++)
		if (!strcasecmp(PossibleValue[i].strvalue, s))
		  break;
	    }
	  else
	    {
	      // int value then
	      for (i=0; PossibleValue[i].strvalue; i++)
		if (v == PossibleValue[i].value)
		  break;
	    }

	  if (!PossibleValue[i].strvalue)
	    {
	      CONS_Printf("\"%s\" is not a possible value for \"%s\"\n", s, name);
	      if (defaultvalue == s)
		I_Error("Variable %s default value \"%s\" is not a possible value\n", name, defaultvalue);
	      return;
	    }

          value = PossibleValue[i].value;
          str = PossibleValue[i].strvalue;
          goto finish; // must not free str!
        }
    }

  // free the old value string
  if (str)
    Z_Free(str);

  str = Z_StrDup(s);

  if (flags & CV_FLOAT)
    value = int(atof(str) * fixed_t::UNIT);
  else
    value = atoi(str);

 finish:
  if (flags & CV_ANNOUNCE_ONCE || flags & CV_ANNOUNCE)
    {
      CONS_Printf("%s set to %s\n", name, str);
      flags &= ~CV_ANNOUNCE_ONCE;
    }

  flags |= CV_MODIFIED;
  // raise 'on change' code
  if (flags & CV_CALL)
    func();
}


//  Register a variable, that can be used later at the console
bool consvar_t::Reg()
{
  // link the variable in, unless it is for internal use only
  if (!(flags & CV_HIDDEN))
    {
      // first check to see if it has already been defined
      if (FindVar(name))
	{
	  CONS_Printf("Variable %s is already defined\n", name);
	  return false;
	}

      // check for overlap with a command
      if (COM_Exists(name))
	{
	  CONS_Printf("%s is a command name\n", name);
	  return false;
	}

      // check net variables
      if (flags & CV_NETVAR)
	{
	  netid = ComputeNetid(name);
	  if (FindNetVar(netid))
	    {
	      I_Error("Variable %s has same netid\n", name);
	      return false;
	    }
	}

      next = cvar_list;
      cvar_list = this;
    }
  else
    {
      netid = 0;
      next = NULL;
    }

  str = NULL;

#ifdef PARANOIA
  if ((flags & CV_NOINIT) && !(flags & CV_CALL))
    I_Error("variable %s has CV_NOINIT without CV_CALL\n", name);
  if ((flags & CV_CALL) && !func)
    I_Error("variable %s has CV_CALL without func", name);
#endif

  if (flags & CV_NOINIT)
    flags &= ~CV_CALL;

  Setvalue(defaultvalue);

  if (flags & CV_NOINIT)
    flags |= CV_CALL;

  // the SetValue will set this bit
  flags &= ~CV_MODIFIED;
  return true;
}



//  Completes the name of a console var
const char *consvar_t::CompleteVar(const char *partial, int skips)
{
  int len = strlen(partial);

  if (!len)
    return NULL;

  // check functions
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (!strncmp(partial, cvar->name, len))
      if (!skips--)
        return cvar->name;

  return NULL;
}


// set a new value to a netvar
void consvar_t::GotNetVar(unsigned short id, const char *str)
{
  consvar_t *cvar = consvar_t::FindNetVar(id);
  if (!cvar)
    {
      CONS_Printf("\2Netvar not found\n");
      return;
    }
  cvar->Setvalue(str);
}


// write the netvars into a packet
void consvar_t::SaveNetVars(TNL::BitStream &s)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (cvar->flags & CV_NETVAR)
      {
	s.write(cvar->netid);
        s.writeString(cvar->str);
      }
}

// read the netvars from a packet
void consvar_t::LoadNetVars(TNL::BitStream &s)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar = cvar->next)
    if (cvar->flags & CV_NETVAR)
      {
	// the for loop is just used for count
	unsigned short id;
	s.read(&id);
	char temp[256];
        s.readString(temp);
	GotNetVar(id, temp);
      }
}


// as if "<varname> <value>" is entered at the console
void consvar_t::Set(const char *s)
{
  if (!(flags & CV_HIDDEN))
    {
      consvar_t *cv;
      // am i registered?
      for (cv = cvar_list; cv; cv = cv->next)
	if (cv == this)
	  break;

      if (!cv)
	Reg();

      if (flags & CV_NOTINNET && game.netgame)
	{
	  CONS_Printf("This variable cannot be changed while in a netgame.\n");
	  return;
	}
    }

#ifdef PARANOIA
  if (!str)
    I_Error("CV_Set : %s no string set ?!\n",name);
#endif

  if (!strcmp(str, s))
    return; // no changes

  if (flags & CV_NETVAR && game.netgame)
    {
      // send the value of the variable
      if (!game.server)
        {
          CONS_Printf("Only the server can change this variable\n");
          return;
        }

      game.net->SendNetVar(netid, s);
    }

  Setvalue(s);
}


//  Expands value to string before calling CV_Set()
void consvar_t::Set(int newval)
{
  char val[32];
  sprintf(val, "%d", newval);
  Set(val);
}

// increments the cvar
void consvar_t::AddValue(int increment)
{
  int n;
  int newvalue = value + increment;

  if (PossibleValue)
    {
      if (!strcmp(PossibleValue[0].strvalue, "MIN"))
        {
          // seach the next to last
          for (n=0; PossibleValue[n+1].strvalue; n++)
            ;
	  int minval = PossibleValue[0].value;
	  int maxval = PossibleValue[n].value;

          if (newvalue < minval)
            newvalue += maxval - minval + 1; // TODO not safe (increment -1000, for example)

          newvalue = minval + (newvalue - minval) % (maxval - minval + 1);
          Set(newvalue);
        }
      else
        {
          int i = -1;

          // this code do not support more than same value for differant PossibleValue
          for (n=0; PossibleValue[n].strvalue; n++)
            if (PossibleValue[n].value == value)
              i = n;

#ifdef PARANOIA
          if (i == -1)
            I_Error("CV_AddValue : current value %d not found in possible values\n", value);
#endif
          i = (i + increment + n) % n;
          Set(PossibleValue[i].strvalue);
        }
    }
  else
    Set(newvalue);
}


//  Displays or changes variable from the console
//
//  Returns false if the passed command was not recognised as
//  console variable.
bool consvar_t::Command()
{
  // check variables
  consvar_t *v = FindVar(COM_Argv(0));
  if (!v)
    return false;

  // perform a variable print or set
  if (COM_Argc() == 1)
    {
      CONS_Printf("\"%s\" is \"%s\" default is \"%s\"\n", v->name, v->str, v->defaultvalue);
      return true;
    }

  v->Set(COM_Argv(1));
  return true;
}


//  Save console variables that have the CV_SAVE flag set
void consvar_t::SaveVariables(FILE *f)
{
  for (consvar_t *cvar = cvar_list; cvar; cvar=cvar->next)
    if (cvar->flags & CV_SAVE)
      fprintf(f, "%s \"%s\"\n", cvar->name, cvar->str);
}
