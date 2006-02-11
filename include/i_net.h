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
// DESCRIPTION:
//      System specific network interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_NET__
#define __I_NET__

#ifdef __GNUG__
#pragma interface
#endif

#define DOOMCOM_ID       0x12345678l

#define MAXPACKETLENGTH  1450 // For use in a LAN
#define INETPACKETLENGTH 512 // For use on the internet

extern short hardware_MAXPACKETLENGTH;
extern int   net_bandwidth; // in byte/s

typedef struct
{
    // Supposed to be DOOMCOM_ID
    long                id;

    // DOOM executes an int to execute commands.
    short               intnum;
    // Communication between DOOM and the driver.
    // Is CMD_SEND or CMD_GET.
    short               command;
    // Is dest for send, set by get (-1 = no packet).
    short               remotenode;

    // Number of bytes in doomdata to be sent
    short               datalength;

    // Info common to all nodes.
    // Console is allways node 0.
    short               numnodes;
    // Flag: 1 = no duplication, 2-5 = dup for slow nets.
    short               ticdup;
    // Flag: 1 = send a backup tic in every packet.
    short               extratics;
    // deathmatch type 0=coop, 1=deathmatch 1 ,2 = deathmatch 2.
    short               deathmatch;
    // Flag: -1 = new game, 0-5 = load savegame
    short               savegame;
    short               episode;        // 1-3
    short               map;            // 1-9
    short               skill;          // 1-5

    // Info specific to this node.
    short               consoleplayer;
    // Number total of players
    short               numplayers;

    // These are related to the 3-display mode,
    //  in which two drones looking left and right
    //  were used to render two additional views
    //  on two additional computers.
    // Probably not operational anymore. (maybe a day in Legacy)
    // 1 = left, 0 = center, -1 = right
    short               angleoffset;
    // 1 = drone
    short               drone;

    // The packet data to be sent.
    char                data[MAXPACKETLENGTH];

} doomcom_t;

extern doomcom_t *doomcom;
// Called by D_DoomMain.

// to be defined by the network driver
extern void    (*I_NetGet) (void);                // return packet in doomcom struct
extern void    (*I_NetSend) (void);               // send packet within doomcom struct
extern bool (*I_NetCanSend) (void);            // ask to driver if all is ok to send data now
extern void    (*I_NetFreeNodenum) (int nodenum); // close a connection 
extern int     (*I_NetMakeNode) (char *address);  // open a connection with sp�cified address
extern bool (*I_NetOpenSocket) (void);         // opend all connections
extern void    (*I_NetCloseSocket) (void);        // close all connections no more allow geting any packet 

bool I_InitNetwork (void);

#endif
