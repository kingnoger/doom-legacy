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
   /*
     TODO: send local player data
     stream->writeString(playerName.getString());
   */
}


bool LConnection::readConnectRequest(BitStream *stream, const char **errorString)
{
  if (!Parent::readConnectRequest(stream, errorString))
    return false;

  char temp[255];
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

  if (game.Players.size() >= unsigned(cv_maxplayers.value))
    {
      sprintf(temp, "Maximum number of players reached (%d).", cv_maxplayers.value);
      return false;
    }

  if (!cv_allownewplayer.value)
    {
      sprintf(temp, "The server is not accepting new players at the moment.");
      return false;
    }


  // TODO read playerdata,

  return true; // server accepts
}


void LConnection::writeConnectAccept(BitStream *stream)
{
  // TODO check that name is unique, send corrected info back etc.

  //SV_SendServerConfig(node)
}


bool LConnection::readConnectAccept(BitStream *stream, const char **errorString)
{
  // TEST
  char temp[255];
  *errorString = temp; // in case we need it
  sprintf(temp, "up yours, server!\n");

  return true;
}



/// connection response functions

void LConnection::onConnectTerminated(TerminationReason r, const char *reason)
{
  CONS_Printf("%s\n", reason);
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
      setGhostFrom(false);
      setGhostTo(true);
      CONS_Printf("%s - connected to server.\n", getNetAddressString());
      LNetInterface *n = (LNetInterface *)getInterface();
      n->server_con = this;
      n->netstate = LNetInterface::CL_Connected;
    }
  else
    {
      // TODO: make scope object
      ((LNetInterface *)getInterface())->client_con.push_back(this);

      /*
	Player *player = new Player;
	myPlayer = player;
	myPlayer->setInterface(getInterface());
	myPlayer->addToGame(((TestNetInterface *) getInterface())->game);
	setScopeObject(myPlayer);
      */

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
	;// drop client, inform others
    }
}














/*
void Got_UseArtefact (char **cp,int playernum)
{
  int art = READBYTE(*cp);
  game.FindPlayer(playernum)->pawn->UseArtifact((artitype_t)art);
}
*/


/*
void Got_AddPlayer(char **s, int playernum)
{
  int newplayernum = READBYTE(*s);
  bool splitscreenplayer = newplayernum & 0x80;

  PlayerInfo *p = new PlayerInfo();
  p = game.AddPlayer(p);

  CONS_Printf("Player %d entered the game (node %d)\n",newplayernum,node);


      D_SendPlayerConfig();
}
*/
