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
// Revision 1.1  2004/06/18 08:15:30  smite-meister
// New TNL netcode!
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Legacy Network Interface code
///
/// Uses OpenTNL

#include "tnl/tnlAsymmetricKey.h"

#include "doomdef.h"
#include "n_interface.h"
#include "n_connection.h"

#include "command.h"
#include "g_game.h"

#include "i_system.h"


using namespace TNL;

extern bool server;
extern consvar_t cv_servername;

static char *ConnectionState[] =
{
  "Not connected",               ///< Initial state of a NetConnection instance - not connected.
  "Awaiting challenge response", ///< We've sent a challenge request, awaiting the response.
  "Sending punch packets",       ///< The state of a pending arranged connection when both sides haven't heard from the other yet
  "Computing puzzle solution",   ///< We've received a challenge response, and are in the process of computing a solution to its puzzle.
  "Awaiting connect response",   ///< We've received a challenge response and sent a connect request.
  "Connection timed out",        ///< The connection timed out during the connection process.
  "Connection rejected",         ///< The connection was rejected.
  "Connected.",                  ///< We've accepted a connect request, or we've received a connect response accept.
  "Disconnected",                ///< The connection has been disconnected.
  "Connection timed out"         ///< The connection timed out.
};



LNetInterface::LNetInterface(const Address &bind)
  : NetInterface(bind)
{
  master_con = NULL;
  server_con = NULL;
  netstate = NS_None;

  lastpingtime = I_GetTime();

  nodownload = false;

  // Asymmetric cipher based on elliptic curves
  setPrivateKey(new AsymmetricKey(32));
  setRequiresKeyExchange(true);
}



void LNetInterface::SetPingAddress(const Address &a)
{
  ping_address = a;
}


void LNetInterface::CL_StartPinging()
{
  netstate = CL_PingingServers;
  pingnonce.getRandom();
}


/// send out a server ping
void LNetInterface::SendPing(const Address &a, const Nonce &cn)
{
  CONS_Printf("Sending out server ping to %s...\n", a.toString());

  PacketStream out;

  out.write(U8(PT_ServerPing));
  cn.write(&out);           // client nonce
  out.writeString(VERSIONSTRING);
  out.write(I_GetTime());  // this is used to calculate the ping value

  out.sendto(mSocket, a);
}


// send out server query
void LNetInterface::SendQuery(const Address &a, const Nonce &cn, U32 token)
{
  CONS_Printf("Querying server %s...\n", a.toString());

  PacketStream out;

  out.write(U8(PT_ServerQuery));
  cn.write(&out);
  out.write(token);

  out.sendto(mSocket, a);
}



/// Handles received info packets
void LNetInterface::handleInfoPacket(const Address &address, U8 packetType, BitStream *stream)
{
  // first 8 bits denote the packet type
  switch (packetType)
    {
    case PT_ServerPing:
      // ping packet only contains a client nonce and the Legacy ID string(s)
      if (server)
	{
	  CONS_Printf("received ping from %s\n", address.toString());

	  // read nonce
	  Nonce cn;
	  cn.read(stream);

	  // check version TODO different versioning for protocol? no?
	  char temp[256];
	  stream->readString(temp);
	  if (strcmp(temp, VERSIONSTRING))
	    break;

	  CONS_Printf("version OK, '%s'\n", temp);

	  // local sending time
	  unsigned time;
	  stream->read(&time);

	  // TODO should we answer back or ignore?

	  // two-part server query protocol for defending agains DOS attacks ;)

	  U32 token = computeClientIdentityToken(address, cn); // this client is now identified with this token

	  PacketStream out;
	  out.write(U8(PT_PingResponse));
	  cn.write(&out);   // write same nonce
	  out.write(token); // and the token
	  out.write(time); // return back the time value received so client can compute ping
	  out.sendto(mSocket, address);
	}
      break;

    case PT_PingResponse:
      if (netstate == CL_PingingServers)
	{
	  CONS_Printf("received ping response from %s\n", address.toString());

	  Nonce cn;
	  cn.read(stream);

	  if (cn != pingnonce)
	    return; // wrong nonce. kind of marginal concern.

	  U32 token; // our identity with this server
	  stream->read(&token);

	  unsigned time;
	  stream->read(&time);
	  CONS_Printf("ping: %d ms\n", time - I_GetTime());

	  // TODO if no autoconnect, now we should add it to the server table...
	  SendQuery(address, cn, token);
	}
      break;

    case PT_ServerQuery:
      // packet contains the client nonce and the id token
      if (server)
	{
	  CONS_Printf("Got server query from %s\n", address.toString());

	  Nonce cn;
	  cn.read(stream);

	  U32 token;
	  stream->read(&token);

	  if (token == computeClientIdentityToken(address, cn))
            {
	      // was SV_SendServerInfo
	      // was SV_SendServerConfig              
	      PacketStream out;
	      out.write(U8(PT_QueryResponse));
	      cn.write(&out);

	      out.writeString(cv_servername.str);
	      out.writeString(VERSIONSTRING); // SUBVERSION?
	      out.write(game.Players.size());
	      out.write(game.maxplayers);

	      // server load, game type, current map, how long it has been running, how long to go...
	      //CV_SaveNetVars((char**)&p); TODO
	      
	      out.sendto(mSocket, address);
	    }	  
	}
      break;

    case PT_QueryResponse:
      if (netstate == CL_PingingServers)
	{
	  CONS_Printf("Got query response from %s", address.toString());

	  Nonce cn;
	  cn.read(stream);

	  char servername[256], serverversion[256];
	  U32 players;
	  stream->readString(servername);
	  stream->readString(serverversion);
	  stream->read(&players);
	  stream->read(&game.maxplayers);

	  // TODO if no autoconnect, now we should update the server table...	  
	  CL_Connect(address);
	}
      break;

    default:
      CONS_Printf("unknown packet %d from %s", packetType, address.toString());
      break;
    }
}



// client making a connection
void LNetInterface::CL_Connect(const Address &a)
{
  LConnection *s = new LConnection();
  server_con = s; // not yet!

  // Set player info

  //if (local) s->connectLocal(net, net);
  s->connect(this, a);
  netstate = CL_Connecting;
}



void LNetInterface::SV_StartServer()
{
  setAllowsConnections(true);
  netstate = SV_WaitingForClients;
}



void LNetInterface::SV_StopServer()
{
  setAllowsConnections(false);

  //disconnect(NetConnection *conn, const char *reason)
  netstate = NS_None;
}





void LNetInterface::Tick()
{
  U32 now = I_GetTime();

  checkIncomingPackets();

  switch (netstate)
    {
    case CL_PingingServers:
      if (lastpingtime + PingDelay < now)
	{
	  lastpingtime = now;
	  pingnonce.getRandom();
	  SendPing(ping_address, pingnonce);
	}
      break;

    case CL_Connecting:
      if (server_con)
	CONS_Printf(ConnectionState[server_con->getConnectionState()]);
      break;

    default:
      break;
    }

  processConnections();
}
