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
// Revision 1.3  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.5  2002/08/21 16:58:35  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.4  2002/07/01 21:00:45  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:57  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//
//    
//-----------------------------------------------------------------------------


#ifndef f_finale_h
#define f_finale_h 1

class clusterdef_t;

//
// FINALE
//

// Called by main loop.
//boolean F_Responder (event_t* ev);

// Called by main loop.
void F_Ticker();

// Called by main loop.
void F_Drawer();

void F_StartFinale(const class MapCluster *c, bool enter, bool endgame);


#endif
