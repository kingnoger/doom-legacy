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
// Revision 1.9  2004/11/09 20:38:52  smite-meister
// added packing to I/O structs
//
// Revision 1.8  2004/11/04 21:12:54  smite-meister
// save/load fixed
//
// Revision 1.7  2004/10/27 17:37:10  smite-meister
// netcode update
//
// Revision 1.6  2004/08/12 18:30:31  smite-meister
// cleaned startup
//
// Revision 1.5  2004/08/06 18:54:39  smite-meister
// netcode update
//
// Revision 1.4  2004/07/13 20:23:38  smite-meister
// Mod system basics
//
// Revision 1.3  2004/07/11 14:32:01  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.2  2004/07/09 19:43:40  smite-meister
// Netcode fixes
//
// Revision 1.1  2004/07/05 16:53:30  smite-meister
// Netcode replaced
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Network connections

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "n_interface.h"
#include "n_connection.h"

#include "g_game.h"
#include "g_type.h"
#include "g_player.h"
#include "g_pawn.h"


/*
More serverside stuff:

void objectLocalScopeAlways(o)
void objectLocalClearAlways(o)
bool isGhosting()

virtual void onEndGhosting()
*/


// netobject:
/*
  virtual void onGhostAvailable (GhostConnection *theConnection) // serverside
  virtual F32  getUpdatePriority (NetObject *scopeObject, U32 updateMask, S32 updateSkips)
*/



TNL_IMPLEMENT_NETCONNECTION(LConnection, NetClassGroupGame, true);


LConnection::LConnection()
{
  //setIsAdaptive();
  //setTranslatesStrings();
  //setFixedRateParameters(50, 50, 2000, 2000); // packet rates, sizes (send and receive)
}



// client
void LConnection::writeConnectRequest(BitStream *stream)
{
   Parent::writeConnectRequest(stream);

   stream->write(VERSION);
   stream->writeString(VERSIONSTRING);

   // send local player data
   byte n = cv_splitscreen.value ? 2 : 1; // number of local players
   stream->write(n);

   stream->writeString(localplayer.name.c_str());

   if (n == 2)
     {
       stream->writeString(localplayer2.name.c_str());
     }
}


// server
bool LConnection::readConnectRequest(BitStream *stream, const char **errorString)
{
  if (!Parent::readConnectRequest(stream, errorString))
    return false;

  char temp[256];
  *errorString = temp; // in case we need it

  int version;
  stream->read(&version);
  stream->readString(temp);

  if (version != VERSION || strcmp(temp, VERSIONSTRING))
    {
      sprintf(temp, "Different Legacy versions cannot play a net game! (Server version %d.%d%s)",
	      VERSION/100, VERSION%100, VERSIONSTRING);
      return false;
    }

  if (!cv_allownewplayers.value)
    {
      sprintf(temp, "The server is not accepting new players at the moment.");
      return false;
    }

  // how many players want to get in?
  byte n;
  stream->read(&n);


  LNetInterface *net = (LNetInterface *)getInterface();

  if (net->netstate == LNetInterface::SV_Running)
    {
      if (game.Players.size() + n > unsigned(cv_maxplayers.value))
	{
	  sprintf(temp, "Maximum number of players reached (%d).", cv_maxplayers.value);
	  return false;
	}

      // read playerdata
      for (int i = 0; i<n; i++)
	{
	  stream->readString(temp);
	  temp[32] = '\0'; // limit name length

	  PlayerInfo *p = new PlayerInfo(temp);
	  p->connection = this;
	  p->client_hash = getNetAddress().hash();

	  // TODO check that name is unique, change if necessary

	  player.push_back(p);
	  if (!game.AddPlayer(p))
	    I_Error("shouldn't happen! rotten!\n");
	}
    }
  else
    {
      // TODO waiting for specific clients to return (check names and hashes!)
    }

  return true; // server accepts
}


// server
void LConnection::writeConnectAccept(BitStream *stream)
{
  Parent::writeConnectAccept(stream);

  byte n = player.size();
  stream->write(n);                   // how many players were accepted?
  for (int i = 0; i < n; i++)
    stream->write(player[i]->number); // send pnums

  game.gtype->WriteServerInfo(*stream);
}


// client
bool LConnection::readConnectAccept(BitStream *stream, const char **errorString)
{
  if(!Parent::readConnectAccept(stream, errorString))
    return false;

  byte n;
  stream->read(&n);
  if (n > 2 || n == 0)
    return false;

  stream->read(&localplayer.number);
  if (n == 2)
    stream->read(&localplayer2.number);

  // read server properties
  LNetInterface *net = (LNetInterface *)getInterface();

  serverinfo_t *s = net->SL_FindServer(getNetAddress());
  if (!s)
    s = net->SL_AddServer(getNetAddress());

  s->Read(*stream);

  consvar_t::LoadNetVars(*stream);
  // FIXME read needed files, check them, download them...

  return true;
}





void LConnection::onConnectTerminated(TerminationReason r, const char *reason)
{
  CONS_Printf("Connect terminated (%d), %s\n", r, reason);
  ConnectionTerminated(false);
}



void LConnection::onConnectionEstablished()
{
  // initiator becomes the "client", the other one "server"
  Parent::onConnectionEstablished();

  // To see how this program performs with 50% packet loss,
  // Try uncommenting the next line :)
  //setSimulatedNetParams(0.5, 0);

  LNetInterface *n = (LNetInterface *)getInterface();
   
  if (isInitiator())
    {
      // client side
      setGhostFrom(false);
      setGhostTo(true);
      n->server_con = this;
      n->netstate = LNetInterface::CL_Connected;
      CONS_Printf("Connected to server at %s.\n", getNetAddressString());

      rpcTest(7467);
    }
  else
    {
      rpcSay(0, 0, "sdoysdfsha!\n");
      // server side
      n->client_con.push_back(this);

      int k = player.size();
      for (int i = 0; i<k; i++)
	{
	  CONS_Printf("%s entered the game (player %d)\n", player[i]->name.c_str(), player[i]->number);
	  // TODO multicast/RPC playerinfo to other players?
	}
      setScopeObject(game.gtype);
      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
    }
}



void LConnection::onConnectionTerminated(TerminationReason r, const char *error)
{
  CONS_Printf("%s - connection to %s: %s\n.", getNetAddressString(), isConnectionToServer()
	      ? "server" : "client", error);
  ConnectionTerminated(true);
}



void LConnection::ConnectionTerminated(bool established)
{
  if (isConnectionToServer())
    {
      ((LNetInterface *)getInterface())->CL_Reset();
    }
  else
    {
      if (established)
	{
	  int n = player.size();
	  for (int i = 0; i<n; i++)
	    {
	      CONS_Printf("Player (%d) dropped.\n", player[i]->number);
	      player[i]->playerstate = PST_REMOVE;
	    }
	  resetGhosting();
	  // TODO inform others
	}
      else
	{
	  CONS_Printf("Unsuccesful connect attempt\n");
	}
    }
}



void LConnection::onStartGhosting()
{
  I_Error("Ghosting started...\n");
}



//========================================================
//            Remote Procedure Calls
//========================================================

TNL_IMPLEMENT_RPC(LConnection, rpcTest, (U8 num), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  CONS_Printf("client sent this: %d\n", num);
};


// to: 0 means everyone, positive numbers are players, negative numbers are teams
TNL_IMPLEMENT_RPC(LConnection, rpcSay, (S8 from, S8 to, const char *msg), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
  if (isConnectionToServer())
    {
      // client
      CONS_Printf("%s: %s\n", game.Players[from]->name.c_str(), msg);
    }
  else
    {
      from = player[0]->number;

      CONS_Printf("message!\n");
      CONS_Printf("\3%s: %s\n", game.Players[from]->name.c_str(), msg);
      game.net->SayCmd(player[0]->number, to, msg);
    }
};




TNL_IMPLEMENT_RPC(LConnection, rpcMessage_s2c, (S32 pnum, const char *msg, S8 priority, S8 type), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
  int n = Consoleplayer.size();
  for (int i=0; i<n; i++)
    if (Consoleplayer[i]->number == pnum)
      {
	Consoleplayer[i]->SetMessage(msg, priority, type);
	return;
      }

  I_Error("Received someone else's message!\n");
}


TNL_IMPLEMENT_RPC(LConnection, rpcStartIntermission_s2c, (), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirServerToClient, 0)
{
  //wi.StartIntermission();
}


TNL_IMPLEMENT_RPC(LConnection, rpcIntermissionDone_c2s, (), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  //player[0]->playerstate = PST_NEEDMAP; 
}


TNL_IMPLEMENT_RPC(LConnection, rpcRequestPOVchange_c2s, (S32 pnum), 
		  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
  // spy mode
  if (game.state == GameInfo::GS_LEVEL && !cv_hiddenplayers.value)
    {
      PlayerInfo *p = NULL;

      /* FIXME NOW
      if (pnum <= 0)
	{
	  // simply "next available POV"
	  player_iter_t i = Players.upper_bound(player[0]->povnum + 1);
	  if (i == Players.end())
	    i = Players.begin();
	  
	  p = i->second;
	}
      else
	p = game.FindPlayer(pnum);
      */

      if (p)
	player[0]->pov = p->pawn;
      else
	player[0]->pov = player[0]->pawn;

      // tell who's the view
      player[0]->SetMessage(va("Viewpoint: %s\n", p->name.c_str()));
    }

  // TODO client should start HUD on the new pov...
}
