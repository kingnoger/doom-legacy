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
// Revision 1.3  2002/07/01 21:00:57  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:31  vberghol
// Version 133 Experimental!
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------


#ifndef __SPEC_H__
#define __SPEC_H__

void spec_brace();

int  spec_if();  //SoM: returns weather or not the if statement was true.
int  spec_elseif(bool lastif);
void spec_else(bool lastif);
void spec_while();
void spec_for();
void spec_goto();

// variable types

bool spec_variable();

void spec_script();     // in t_script.c btw

#endif

//---------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2002/11/16 14:18:28  hurdler
// Initial revision
//
// Revision 1.3  2002/07/01 21:00:57  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:31  vberghol
// Version 133 Experimental!
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//---------------------------------------------------------------------------

