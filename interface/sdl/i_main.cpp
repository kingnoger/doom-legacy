// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.1  2002/11/16 14:18:31  hurdler
// Initial revision
//
// Revision 1.3  2002/07/01 21:01:03  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:34  vberghol
// Version 133 Experimental!
//
// Revision 1.2  2000/09/10 10:56:00  metzgermeister
// clean up & made it work again
//
// Revision 1.1  2000/08/21 21:17:32  metzgermeister
// Initial import to CVS
//
// 
//
// DESCRIPTION:
//      Main program, simply calls D_DoomMain high level loop.
// VB: rewrite
//-----------------------------------------------------------------------------

//#include "doomdef.h"
//#include "m_argv.h"
//#include "d_main.h"

// in m_argv.h
extern  int     myargc;
extern  char**  myargv;

// in d_main.h
void D_DoomLoop (void);
void D_DoomMain (void);

int main (int argc, char **argv ) 
{ 
    myargc = argc; 
    myargv = argv; 
 
    D_DoomMain (); 
    D_DoomLoop ();
    return 0;
} 
