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
/// \brief Network connections

#ifndef n_connection_h
#define n_connection_h 1

#include "tnl/tnlGhostConnection.h"

using namespace TNL;


/// \brief TNL GhostConnection between a server and a client
///
/// Does connection housekeeping

class LConnection : public GhostConnection
{
  typedef GhostConnection Parent;

  /// Makes this a valid connection class to the TNL network system.
  TNL_DECLARE_NETCONNECTION(LConnection);

public:

  LConnection();


  virtual void writeConnectRequest(BitStream *stream);

  virtual bool readConnectRequest(BitStream *stream, const char **errorString);


  // FIXME the onXXX() methods are under change in TNL. Fix them when we decide which version to use...

  /// Called when the attempt to connect to a remote host fails due to lack of response.
  virtual void onConnectTimedOut();

  /// Called when the remote host rejects this connection.
  virtual void onConnectionRejected(const char *reason);

  /// called on both ends of a connection when the connection is established.  
  virtual void onConnectionEstablished(bool isInitiator);

  /// Called when this instance is unable to elicit a response from the remote host for the timeout period.
  virtual void onTimedOut();  

  /// Called when a connection receives a bogus packet or invalid data from the remote host
  virtual void onConnectionError(const char *errorString);

  /// Called when the remote host issues a disconnect packet to this instance.
  virtual void onDisconnect(const char *reason);           

  /// called when an established connection is terminated, whether
  /// from the local or remote hosts explicitly disconnecting, timing out or network error.
  void ConnectionTerminated(const char *reason);



  //============ RPCs =============== 


  /// Remote function that client calls to set the position of the player on the server.
  TNL_DECLARE_RPC(rpcTest, (F32 x, F32 y));
};


#endif
