// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2005 by DooM Legacy Team.
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
// Revision 1.12  2005/04/19 18:28:33  smite-meister
// new RPCs
//
// Revision 1.11  2005/04/17 17:47:54  smite-meister
// netcode
//
// Revision 1.10  2005/03/24 16:59:17  smite-meister
// upgrade to OpenTNL 1.5
//
// Revision 1.9  2004/11/28 18:02:23  smite-meister
// RPCs finally work!
//
// Revision 1.8  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.3  2004/07/05 16:53:29  smite-meister
// Netcode replaced
//
// Revision 1.1  2004/06/18 08:17:02  smite-meister
// New TNL netcode!
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Network connections

#ifndef n_connection_h
#define n_connection_h 1

#include <vector>
#include <list>
#include "tnl/tnlGhostConnection.h"

using namespace TNL;


/// \brief TNL GhostConnection between a server and a client
///
/// Does connection housekeeping, ghosting, RPC's etc.

class LConnection : public GhostConnection
{
  typedef GhostConnection Parent;

public:
  std::vector<class PlayerInfo *> player; ///< Serverside: Players beyond this connection.

  /// Clientside: Local players that wish to join a remote game.
  static std::list<class LocalPlayerInfo *> joining_players;

public:
  LConnection();

  /// client sends info to server and requests a connection
  virtual void writeConnectRequest(BitStream *stream);

  /// server decides whether to accept the connection
  virtual bool readConnectRequest(BitStream *stream, const char **errorString);

  /// server sends info to client
  virtual void writeConnectAccept(BitStream *stream);

  /// client decides whether to accept the connection
  virtual bool readConnectAccept(BitStream *stream, const char **errorString);


  /// Called when a pending connection is terminated
  virtual void onConnectTerminated(TerminationReason r, const char *reason); 

  /// Called when an established connection is terminated
  virtual void onConnectionTerminated(TerminationReason r, const char *error); 

  /// called on both ends of a connection when the connection is established.
  virtual void onConnectionEstablished();

  /// called when a connection or connection attempt is terminated, whether
  /// from the local or remote hosts explicitly disconnecting, timing out or network error.
  void ConnectionTerminated(bool established);

  /// called at the client when ghosting starts
  virtual void onStartGhosting();

  /// called at the client when ghosting stops
  virtual void onEndGhosting();


  //============ RPCs =============== 

  TNL_DECLARE_RPC(rpcTest, (U8 num));

  /// Transmits chat messages between client and server.
  TNL_DECLARE_RPC(rpcChat, (S8 from, S8 to, StringPtr msg));

  /// Pauses/unpauses the game, or, when used by a client, requests this from the server.
  TNL_DECLARE_RPC(rpcPause, (U8 pnum, bool on));


  /// Server prints a message on client's console/HUD
  TNL_DECLARE_RPC(rpcMessage_s2c, (S8 pnum, StringPtr msg, S8 priority, S8 type));

  /// When the server changes a netvar during the game, this RPC notifies the clients.
  TNL_DECLARE_RPC(rpcSendNetVar_s2c, (U16 netid, StringPtr str));

  /// Server asks client to load a map
  //TNL_DECLARE_RPC(rpcStartMap_s2c, (U8 pnum));

  /// server tells client to play a sound/music/sequence
  //TNL_DECLARE_RPC(rpcStartAmbSound_s2c, (U8 pnum));
  //TNL_DECLARE_RPC(rpcStart3DSound_s2c, (U8 pnum));
  //TNL_DECLARE_RPC(rpcStop3DSound_s2c, (U8 pnum));

  /// Server tells the client to start the intermission.
  TNL_DECLARE_RPC(rpcStartIntermission_s2c, ());

  /// Server kicks a player away.
  TNL_DECLARE_RPC(rpcKick_s2c, (U8 pnum, StringPtr str));


  /// Client updates his player info.
  TNL_DECLARE_RPC(rpcSendOptions_c2s, (U8 pnum, ByteBufferPtr buf));

  /// Client requests a suicide.
  TNL_DECLARE_RPC(rpcSuicide_c2s, (U8 pnum));

  /// Client tells server that it has finished playing the intermission.
  TNL_DECLARE_RPC(rpcIntermissionDone_c2s, ());

  /// Client asks for a new POV ("spy mode").
  TNL_DECLARE_RPC(rpcRequestPOVchange_c2s, (S32 pnum));



  /// Makes this a valid connection class to the TNL network system.
  TNL_DECLARE_NETCONNECTION(LConnection);

  /// A little shorthand for implementing RPCs
#define LCONNECTION_RPC(rpc_name, args, call_args, guarantee, direction, version) \
TNL_IMPLEMENT_RPC(LConnection, rpc_name, args, call_args, NetClassGroupGameMask, guarantee, direction, version)
};


#endif
