// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003-2004 by DooM Legacy Team.
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
// Revision 1.6  2004/12/19 23:30:18  smite-meister
// More BEX support
//
// Revision 1.5  2004/10/14 19:35:50  smite-meister
// automap, bbox_t
//
// Revision 1.4  2004/09/23 23:21:19  smite-meister
// HUD updated
//
// Revision 1.3  2004/09/03 16:28:51  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Simple parsing functions for plaintext script lumps

#ifndef parser_h
#define parser_h 1

#include "stddef.h"


enum parseritem_t
{
  P_ITEM_IGNORE = 0,
  P_ITEM_BOOL, // bool. true if it item exists.
  P_ITEM_INT,
  P_ITEM_INT_INT, // two ints separated by whitespace
  P_ITEM_FLOAT,
  P_ITEM_STR,   // STL string
  P_ITEM_STR16, // 16 chars
  P_ITEM_STR16_FLOAT // max 16 char string and float
};


struct parsercmd_t
{
  parseritem_t type;
  char   *name;
  size_t  offset1, offset2;
};


/// \brief Class for parsing plaintext lumps.
class Parser
{
private:
  int length;    // length of the lump
  char *ms, *me; // start and past-the-end pointers
  char *s, *e;   // start and past-the-end pointers for the current line

public:
  Parser();
  ~Parser();

  int  Open(int lump);
  int  Open(const char *buf, int len);
  void Clear();

  void RemoveComments(char c);
  int  ReplaceChars(char from, char to);
  inline int RemoveCRs() { return ReplaceChars('\r', ' '); };

  bool NewLine(bool pass_ws = true);
  void PassWS();
  int  LineReplaceChars(char from, char to);
  char *GetToken(const char *delim);
  int  GetString(char *to, int n);
  int  GetStringN(char *to, int n);
  bool GetChar(char *to);
  int  GetInt();
  bool MustGetInt(int *to);

  inline int   LineLen() { return e - s - 1; }; // remaining length of the current line (without the \n)
  inline char  Peek() { return *s; };
  inline int   Location() { return s - ms; };
  inline char *Pointer() { return s; }; // dangerous but useful
  inline void  SetPointer(char *ptr) { s = ptr; }; // even more so

  bool ParseCmd(const parsercmd_t *commands, char *structure);
  void GoToNext(const char *str);
};


bool IsNumeric(const char *p);
int  P_MatchString(const char *p, const char *strings[]);

#ifndef __WIN32__
/// converts a c-string to upper case
char *strupr(char *s);

/// converts a c-string to lower case
char *strlwr(char *s);
#endif

/// converts up to n chars to upper case
char *strnupr(char *s, int n);

#endif
