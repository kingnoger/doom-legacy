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
// Revision 1.1  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
//
// DESCRIPTION:
//   This zone is for loading and unloading of Doom Legacy DLL's only.
//
//-----------------------------------------------------------------------------

// Why oh why can't there be just one standard DLL system inteface?!?
#ifdef __WIN32__
  // windows
# include <windows.h>
#else
  // unices
# include <dlfcn.h>
#endif

#include "m_dll.h"


// Dynamically loads a DLL file 
dll_handle_t OpenDLL(const char *dllname)
{
#ifdef __WIN32__
  return LoadLibrary(dllname);
#else
  return dlopen(dllname, RTLD_LAZY);
#endif
}

// Frees a DLL file loaded using OpenDLL
void CloseDLL(dll_handle_t handle)
{
#ifdef __WIN32__
  FreeLibrary(handle);
#else  
  dlclose(handle);
#endif
}

// Returns a pointer to 'symbol', which _must_ belong
// to the ones exported by the DLL in question.
// Otherwise returns NULL.
void *GetSymbol(dll_handle_t handle, const char *symbol)
{
#ifdef __WIN32__
  return (void *)GetProcAddress(handle, symbol);
#else
  return dlsym(handle, symbol);
#endif
}
