// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
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
// Revision 1.2  2003/04/26 12:01:13  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.1  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
//
// DESCRIPTION:
//   This zone is for loading and unloading of Doom Legacy DLL's only.
//
//-----------------------------------------------------------------------------

#ifndef m_dll_h
#define m_dll_h 1

#ifdef __WIN32__
# include <windows.h>
typedef HINSTANCE dll_handle_t;
#else
typedef void* dll_handle_t;
#endif

struct dll_info_t
{
  int  interface_version; // the interface version of the dll
  int  dll_version;  // dll version number
  char dll_name[40]; // official dll name FIXME longer field!
};


// common DLL interface
dll_handle_t OpenDLL(const char *dllname);
void  CloseDLL(dll_handle_t handle);
void *GetSymbol(dll_handle_t handle, const char *symbol);

// A Legacy DLL plugin exports usually just two symbols:
//  dll_info, the dll_info_t struct of the DLL, and
//  some way to get the actual function pointers to the DLL functions.

#endif
