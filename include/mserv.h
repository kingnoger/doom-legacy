// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.2  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.1.1.1  2002/11/16 14:18:24  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.3  2002/07/01 21:00:50  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:26  vberghol
// Version 133 Experimental!
//
// Revision 1.9  2000/10/22 00:20:53  hurdler
// Updated for the latest master server code
//
// Revision 1.8  2000/10/17 10:09:27  hurdler
// Update master server code for easy connect from menu
//
// Revision 1.7  2000/10/08 13:30:01  bpereira
// no message
//
// Revision 1.6  2000/10/01 15:20:23  hurdler
// Add private server
//
// Revision 1.5  2000/09/08 22:28:30  hurdler
// merge masterserver_ip/port in one cvar, add -private
//
// Revision 1.4  2000/09/02 15:38:24  hurdler
// Add master server to menus (temporaray)
//
// Revision 1.3  2000/08/29 15:53:47  hurdler
// Remove master server connect timeout on LAN (not connected to Internet)
//
// Revision 1.2  2000/08/16 17:21:50  hurdler
// update master server code (bis)
//
// Revision 1.1  2000/08/16 14:04:57  hurdler
// add master server code
//
//
//
// DESCRIPTION:
//      Header file for the master server routines
//
//-----------------------------------------------------------------------------

#ifndef _MSERV_H_
#define _MSERV_H_

// I want that structure 8 bytes aligned (current size is 80)
typedef struct
{
    char    header[16];     // information such as password
    char    ip[16];
    char    port[8];
    char    name[32];       
    char    version[8];     // format is: x.yy.z (like 1.30.2 or 1.31)
} msg_server_t;


// ================================ GLOBALS ===============================

char *GetMasterServerPort(void);
char *GetMasterServerIP(void);

void RegisterServer(int, int);
void UnregisterServer(void);

void SendPingToMasterServer(void);

msg_server_t *GetShortServersList(void);

#endif
