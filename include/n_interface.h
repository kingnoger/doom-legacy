// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004 by DooM Legacy Team.
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
// Revision 1.1  2004/06/18 08:17:02  smite-meister
// New TNL netcode!
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Legacy Network Interface

#ifndef n_interface_h
#define n_interface_h 1

#include <vector>
#include "tnl/tnlNetInterface.h"

using namespace TNL;

/// \brief TNL NetInterface class for Legacy
///
/// Wrapper for a socket and more.
/// Takes care of low-level network stuff, handles connections and authenthications,
/// server pinging etc.

class LNetInterface : public NetInterface
{
  typedef NetInterface Parent;

public:

  enum netstate_t
  {
    NS_None,
    CL_PingingServers,
    CL_Connecting,
    SV_WaitingForClients,
  };

  /// network state
  netstate_t netstate;


  /// local constants
  enum
  {
    PingDelay = 1000,  ///< ms to wait between sending server pings

    // Different types of info packets
    PT_ServerPing = FirstValidInfoPacketId,  ///< Client pinging for servers
    PT_PingResponse,   ///< Server answering a ping
    PT_ServerQuery,    ///< Client querying server info
    PT_QueryResponse,  ///< Server answering a query
  };


  /// server pinging
  U32     lastpingtime;
  Nonce   pingnonce;
  Address ping_address; ///< Network address to ping in searching for a server. Specific host address or a LAN broadcast address.

  // active connections
  class MasterConnection *master_con;   ///< connection to master server
  class LConnection      *server_con;   ///< Current connection to the server, if this is a client.
  std::vector<LConnection *> client_con;   ///< Current client connections, if this is a server.

  bool nodownload;  ///< CheckParm of -nodownload

public:

  /// The constructor initializes and binds the network interface
  LNetInterface(const Address &bind);

  void SetPingAddress(const Address &a);

  /// overrides the method in the NetInterface class to handle ping and info packets
  virtual void handleInfoPacket(const Address &a, U8 packetType, BitStream *stream);

  /// Sends out a server ping
  void SendPing(const Address &a, const Nonce &cn);

  /// Sends out a server query
  void SendQuery(const Address &a, const Nonce &cn, U32 token);

  /// Checks for incoming packets, sends out packets if needed, processes connections.
  void Tick();

  /// Starts searching for LAN servers
  void CL_StartPinging();

  /// Tries to connect to a server
  void CL_Connect(const Address &a);

  void SV_StartServer();
  void SV_StopServer();
};




#endif
