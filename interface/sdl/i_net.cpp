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
// Revision 1.2  2004/08/18 14:35:21  smite-meister
// PNG support!
//
// Revision 1.1.1.1  2002/11/16 14:18:31  hurdler
// Initial C++ version of Doom Legacy
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
// DESCRIPTION:
//      network interface
//      
//-----------------------------------------------------------------------------


#include <errno.h>

#include "doomdef.h"

#include "i_system.h"
#include "d_event.h"
#include "m_argv.h"

//#include "doomstat.h"

#include "i_net.h"

#include "z_zone.h"

#if 0
int I_InitTcpNetwork(void);
//
// NETWORKING
//

void Internal_Get(void)
{
     I_Error("Get without netgame\n");
}

void Internal_Send(void)
{
     I_Error("Send without netgame\n");
}

void Internal_FreeNodenum(int nodenum)
{}
#endif

//
// I_InitNetwork
// Only required for DOS, so this is more a dummy
//
bool I_InitNetwork (void)
{
  if( M_CheckParm ("-net") )
    {
      I_Error("-net not supported, use -server and -connect\n"
	      "see docs for more\n");
    }
  return false;
}
