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
#include "tnl/tnlGhostConnection.h"

using namespace TNL;


/// \brief TNL GhostConnection between a server and a client
///
/// Does connection housekeeping, RPC's etc.

class LConnection : public GhostConnection
{
  typedef GhostConnection Parent;

public:
  std::vector<class PlayerInfo *> player; ///< players beyond this connection

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

  /*
RPC's from server to client:
 - startsound/stopsound/sequence/music
 - HUD colormap and other effects?
 - pause
 - console/HUD message (unicast/multicast)
 - map change/load


Player ticcmd contains both guaranteed_ordered and unguaranteed elements.
Shooting and artifact use should be guaranteed...

    XD_NAMEANDCOLOR=1,
    XD_WEAPONPREF,
    XD_NETVAR,
    XD_SAY,
    XD_MAP,
    XD_EXITLEVEL,
    XD_LOADGAME,
    XD_SAVEGAME,
    XD_PAUSE,
    XD_ADDPLAYER,
    XD_USEARTEFACT,
  */

  /// client updates his player info (or asks the server to add a new local player?)
  //TNL_DECLARE_RPC(rpcUpdatePlayerInfo_c2s, (U8 pnum, const char *name, U8 color, U8 team));

  TNL_DECLARE_RPC(rpcTest, (U8 num));



  /// Transmits chat messages between client and server.
  TNL_DECLARE_RPC(rpcChat, (S8 from, S8 to, const char *msg));

  /// server prints a message on client's console/HUD
  TNL_DECLARE_RPC(rpcMessage_s2c, (S32 pnum, const char *msg, S8 priority, S8 type));

  /// Pauses/unpauses the game, or, when used by a client, requests this from the server.
  TNL_DECLARE_RPC(rpcPause, (bool on, U8 playernum));

  /// When the server changes a netvar during the game, this rpc notifies the clients.
  TNL_DECLARE_RPC(rpcSendNetVar, (U16 netid, const char *str));

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



  /// Makes this a valid connection class to the TNL network system.
  TNL_DECLARE_NETCONNECTION(LConnection);

  /// A little shorthand for implementing RPCs
#define LCONNECTION_RPC(rpc_name, args, guarantee, direction, version) \
TNL_IMPLEMENT_RPC(LConnection, rpc_name, args, NetClassGroupGameMask, guarantee, direction, version)
};


#endif
