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
// Revision 1.7  2004/10/27 17:37:09  smite-meister
// netcode update
//
// Revision 1.6  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.5  2004/08/06 19:33:49  smite-meister
// netcode
//
// Revision 1.4  2004/07/09 19:43:40  smite-meister
// Netcode fixes
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Network connections

#ifndef n_connection_h
#define n_connection_h 1

#include <vector>
#include "tnl/tnlGhostConnection.h"

using namespace TNL;


/// \brief TNL GhostConnection between a server and a client
///
/// Does connection housekeeping, RPC's etc.

class LConnection : public GhostConnection
{
  typedef GhostConnection Parent;

public:
  /// Makes this a valid connection class to the TNL network system.
  TNL_DECLARE_NETCONNECTION(LConnection);

public:
  std::vector<class PlayerInfo *> player; ///< players beyond this connection


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


  //============ RPCs =============== 

  /// client updates his player info (or asks the server to add a new local player?)
  //TNL_DECLARE_RPC(rpcUpdatePlayerInfo_c2s, (U8 pnum, const char *name, U8 color, U8 team));

  /// transmits chat messages between client and server
  TNL_DECLARE_RPC(rpcSay, (S8 from, S8 to, const char *msg));

  /// server prints a message on client's console/HUD
  TNL_DECLARE_RPC(rpcMessage_s2c, (S32 pnum, const char *msg, S8 priority, S8 type));

  /// server starts a positional sound on the client
  //TNL_DECLARE_RPC(rpcStartSound_s2c, (origin, sfx_id, volume));

  /// server asks client to load a map
  //TNL_DECLARE_RPC(rpcStartMap_c2s, (U8 pnum));

  /// server tells the client to start intermission
  TNL_DECLARE_RPC(rpcStartIntermission_s2c, ());
  /// client tells server that it has finished playing the intermission
  TNL_DECLARE_RPC(rpcIntermissionDone_c2s, ());

  /// client asks for a new POV ("spy mode")
  TNL_DECLARE_RPC(rpcRequestPOVchange_c2s, (S32 pnum));

  /// server tells client to play a sound
  // TODO how to make a client stop a sound? pseudorandom sound netIDs?
  //TNL_DECLARE_RPC(rpcStartAmbSound_s2c, (U8 pnum));
  //TNL_DECLARE_RPC(rpcStart3DSound_s2c, (U8 pnum));
};


#endif
