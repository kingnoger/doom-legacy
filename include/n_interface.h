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
// Revision 1.5  2004/07/09 19:43:40  smite-meister
// Netcode fixes
//
// Revision 1.4  2004/07/07 17:27:19  smite-meister
// bugfixes
//
// Revision 1.3  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.2  2004/06/25 19:53:23  smite-meister
// Netcode
//
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
#include <string>
#include "tnl/tnlNetInterface.h"

using namespace std;
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
    NS_Unconnected,    ///< uninitialized or no network connections
    CL_PingingServers, ///< client looking for servers
    CL_Connecting,     ///< client trying to connect to a server
    CL_Connected,      ///< client connected to a server
    SV_Loading,        ///< server loading a map (clients should be loading it also)
    SV_WaitingClients, ///< server ready but not starting the game yet
    SV_Running         ///< server running the game
  };

  /// network state
  netstate_t netstate;

protected:
  /// local constants
  enum
  {
    PingDelay  = 5000, ///< ms to wait between sending server pings
    QueryDelay = 8000, ///< ms to wait between sending server queries

    /// Different types of info packets
    PT_ServerPing = FirstValidInfoPacketId,  ///< Client pinging for servers
    PT_PingResponse,   ///< Server answering a ping
    PT_ServerQuery,    ///< Client querying server info
    PT_QueryResponse,  ///< Server answering a query
  };


  U32          nowtime; ///< time of current/last update in ms

  /// server pinging
  U32     nextpingtime;
  Nonce      pingnonce;
public:
  Address ping_address; ///< Host or a LAN broadcast address used in server search
  bool     autoconnect;

  /// information about currently known servers
  vector<class serverinfo_t *> serverlist;

protected:
  /// server list manipulation
  serverinfo_t *SL_FindServer(const Address &a);
  serverinfo_t *SL_AddServer(const Address &a);
  void SL_Update();
  void SL_Clear();


  /// overrides the method in the NetInterface class to handle ping and info packets
  virtual void handleInfoPacket(const Address &a, U8 packetType, BitStream *stream);

  /// Sends out a server ping
  void SendPing(const Address &a, const Nonce &cn);

  /// Sends out a server query
  void SendQuery(serverinfo_t *s);


public:

  // active connections
  class MasterConnection *master_con;   ///< connection to master server
  class LConnection      *server_con;   ///< Current connection to the server, if this is a client.
  std::vector<LConnection *> client_con;   ///< Current client connections, if this is a server.

  bool nodownload;  ///< CheckParm of -nodownload

public:

  /// The constructor initializes and binds the network interface
  LNetInterface(const Address &bind);

  /// Checks for incoming packets, sends out packets if needed, processes connections.
  void Update();

  /// Starts searching for LAN servers
  void CL_StartPinging(bool connectany = false);

  /// Tries to connect to a server
  void CL_Connect(const Address &a);

  void CL_Reset();

  /// Opens a server to the world
  void SV_Open();

  /// Closes all connections
  void SV_Reset();

  void QuitNetGame();

  //================================================

  void SayCmd(int from, int to, const char *msg);
};



/// \brief Vital server properties visible to prospective clients
///
/// A client makes one of these for each server found during server search.
class serverinfo_t
{
public:
  Address         addr; ///< server address
  Nonce             cn; ///< client nonce sent to server
  U32            token; ///< id token the server returned
  unsigned   nextquery; ///< when should the next server query be sent?


  unsigned        ping;
  string          name;
  int          version; ///< server version
  string versionstring;
  int          players;
  int       maxplayers;

  int         gametype;


  serverinfo_t(const Address &a);
  void Draw(int x, int y);
};


#endif
