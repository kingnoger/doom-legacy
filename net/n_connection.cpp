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


/*
More serverside stuff:

void objectLocalScopeAlways(o)
void objectLocalClearAlways(o)

void activateGhosting()
void resetGhosting()
bool isGhosting()

virtual void onStartGhosting()
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
  setFixedRateParameters(50, 50, 2000, 2000); // packet rates, sizes (send and receive)
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

  if (!cv_allownewplayer.value)
    {
      sprintf(temp, "The server is not accepting new players at the moment.");
      return false;
    }

  // how many players want to get in?
  byte n;
  stream->read(&n);

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
      // TODO check that name is unique, change if necessary

      player.push_back(p);
      if (!game.AddPlayer(p))
	I_Error("shouldn't happen! rotten!\n");
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

  game.type->WriteServerInfo(*stream);
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
  serverinfo_t s(getNetAddress());
  s.Read(*stream);

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
    }
  else
    {
      // server side
      n->client_con.push_back(this);

      int k = player.size();
      for (int i = 0; i<k; i++)
	{
	  CONS_Printf("%s entered the game (player %d)\n", player[i]->name.c_str(), player[i]->number);
	  // TODO multicast/RPC playerinfo to other players?
	}
      setScopeObject(game.type);
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
  CONS_Printf("Unsuccesful connect attempt\n");
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
	      game.RemovePlayer(player[i]->number); // PI is also deleted here
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
