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
#include "g_player.h"




TNL_IMPLEMENT_NETCONNECTION(LConnection, NetClassGroupGame, true);


LConnection::LConnection()
{
  //setIsAdaptive();
  setFixedRateParameters(50, 50, 2000, 2000); // packet rates, sizes (send and receive)
}




void LConnection::writeConnectRequest(BitStream *stream)
{
   Parent::writeConnectRequest(stream);

   stream->write(VERSION);
   stream->writeString(VERSIONSTRING);

   // send local player data
   byte n = cv_splitscreen.value + 1; // number of local players
   stream->write(n);

   stream->writeString(localplayer.name.c_str());

   if (n == 2)
     {
       stream->writeString(localplayer2.name.c_str());
     }
}


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

      player.push_back(p);
      if (!game.AddPlayer(p))
	I_Error("shouldn't happen! rotten!\n");
    }

  return true; // server accepts
}


void LConnection::writeConnectAccept(BitStream *stream)
{
  Parent::writeConnectAccept(stream);  
  // TODO check that name is unique, send corrected info back, send pnum

  // TODO SV_SendServerConfig(node)
  /*
    stream->writeString(cv_servername.str);
    stream->write(VERSION);
    stream->writeString(VERSIONSTRING);
    stream->write(game.Players.size());
    stream->write(game.maxplayers);
  */
  // TODO server load, game type, current map, how long it has been running, how long to go...
  // what WADs are needed, can they be downloaded from server...

  consvar_t::SaveNetVars(stream);
}


bool LConnection::readConnectAccept(BitStream *stream, const char **errorString)
{
  if(!Parent::readConnectAccept(stream, errorString))
    return false;

  // TODO read serverconfig
  consvar_t::LoadNetVars(stream);

  return true;
}



/// connection response functions

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
   
  if (isInitiator())
    {
      // client side
      setGhostFrom(false);
      setGhostTo(true);
      CONS_Printf("%s - connected to server.\n", getNetAddressString());
      LNetInterface *n = (LNetInterface *)getInterface();
      n->server_con = this;
      n->netstate = LNetInterface::CL_Connected;
    }
  else
    {
      // server side
      ((LNetInterface *)getInterface())->client_con.push_back(this);

      int n = player.size();
      for (int i = 0; i<n; i++)
	{
	  CONS_Printf("%s entered the game (player %d)\n", player[i]->name.c_str(), player[i]->number);
	  //SendPlayerConfig();
	}
      //setScopeObject(x); // TODO

      setGhostFrom(true);
      setGhostTo(false);
      activateGhosting();
      CONS_Printf("%s - client connected.\n", getNetAddressString());
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
	      game.RemovePlayer(player[i]->number); // PI is also deleted here
	    }
	  // drop client, inform others
	}
      else
	{
	  CONS_Printf("Unsuccesful connect attempt\n");
	}
    }
}












// TEST ghosting
/*
  typedef NetObject Parent;
  TNL_DECLARE_CLASS(PlayerInfo);
  virtual bool onGhostAdd(class GhostConnection *theConnection);
  virtual void onGhostRemove();
  virtual U32  packUpdate(GhostConnection *connection, U32 updateMask, class BitStream *stream);
  virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);
  virtual void performScopeQuery(GhostConnection *connection);


bool PlayerInfo::onGhostAdd(class GhostConnection *theConnection)
{
  CONS_Printf("added new player\n");
  game.AddPlayer(this);
  return true;
}

void PlayerInfo::onGhostRemove()
{
  game.RemovePlayer(number);
}

const int UM_INIT = 0x1; // TEMP

U32 PlayerInfo::packUpdate(GhostConnection *connection, U32 mask, class BitStream *stream)
{
  // check which states need to be updated, and write updates
  if (stream->writeFlag(mask & UM_INIT))
    {
      stream->write(number);
      stream->writeString(name.c_str());
    }

  // the return value from packUpdate can set which states still
  // need to be updated for this object.
  return 0;
}

void PlayerInfo::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
  char temp[256];

  // the unpackUpdate function must be symmetrical to packUpdate
  if (stream->readFlag())
    {
      stream->read(&number);
      stream->readString(temp);
      name = temp;
    }
}


// scope query on server
void PlayerInfo::performScopeQuery(GhostConnection *c)
{
  for (GameInfo::player_iter_t t = game.Players.begin(); t != game.Players.end(); t++)
    {
      PlayerInfo *p = (*t).second;
      c->objectInScope(p); // player information is always in scope
    }
}
*/
