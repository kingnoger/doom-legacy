// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
//
// $Log$
// Revision 1.28  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.27  2004/10/27 17:37:06  smite-meister
// netcode update
//
// Revision 1.26  2004/09/13 20:43:29  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.24  2004/07/13 20:23:36  smite-meister
// Mod system basics
//
// Revision 1.23  2004/07/09 19:43:39  smite-meister
// Netcode fixes
//
// Revision 1.21  2004/07/05 16:53:24  smite-meister
// Netcode replaced
//
// Revision 1.20  2004/03/28 15:16:12  smite-meister
// Texture cache.
//
// Revision 1.16  2003/11/12 11:07:18  smite-meister
// Serialization done. Map progression.
//
// Revision 1.15  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.14  2003/06/01 18:56:29  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.13  2003/05/30 13:34:43  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/04/04 00:01:54  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.10  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.9  2003/03/15 20:07:14  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.8  2003/03/08 16:07:00  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.7  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.6  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.5  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.4  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//-----------------------------------------------------------------------------

/// \file
/// \brief PlayerInfo class implementation

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_player.h"
#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "n_connection.h"

#include "hud.h"
#include "tables.h"


//===================================
//           global data
//===================================

// PI's for both local players. Used by menu to setup their properties.
// These are only "models" for local players, the actual PI's
// that are added to a game are copy constructed from these.
PlayerInfo localplayer("Batman");
PlayerInfo localplayer2("Robin");

vector<PlayerInfo *> Consoleplayer;  // local players



static char default_weaponpref[NUMWEAPONS] =
{
  1,4,5,6,8,7,3,9,2,  // Doom
  1,4,5,7,8,3,9,6,0,  // Heretic
  2,2,2,4,4,4,6,6,6,0,0,0,0 // Hexen
};


TNL_IMPLEMENT_NETOBJECT(PlayerInfo);

PlayerInfo::PlayerInfo(const string & n)
{
  number = 0;
  team = 0;
  name = n;

  connection = NULL;
  client_hash = 0;

  playerstate = PST_NEEDMAP;
  spectator = false;
  map_completed = false;

  requestmap = entrypoint = 0;

  cmd.Clear();

  mp = NULL;
  pawn = NULL;
  pov = NULL;

  messagefilter = 0;

  ptype = -1;
  color = 0;
  skin  = 0;

  for (int i = 0; i<NUMWEAPONS; i++)
    weaponpref[i] = default_weaponpref[i];
  originalweaponswitch = true;
  autoaim = false;

  viewz = viewheight = deltaviewheight = bob_amplitude = 0;
  palette = -1;
  damagecount = bonuscount = 0;
  itemuse = 0;

  Reset(false, true);

  // net stuff
  mNetFlags.set(Ghostable);
};


bool PlayerInfo::onGhostAdd(class GhostConnection *theConnection)
{
  CONS_Printf("added new player (%d)\n", number);
  game.AddPlayer(this);
  return true;
}

void PlayerInfo::onGhostRemove()
{
  CONS_Printf("removed player (%d)\n", number);
  game.RemovePlayer(number);
}


U32 PlayerInfo::packUpdate(GhostConnection *c, U32 mask, class BitStream *stream)
{
  if (isInitialUpdate())
    mask = 0x1;

  // check which states need to be updated, and write updates
  if (stream->writeFlag(mask & 0x1))
    {
      CONS_Printf("--- pi stuff sent\n");
      stream->write(number);
      stream->writeString(name.c_str());
    }


  if (c != connection)
    return 0;

  // feedback (goes only to the owner)
  /*
  if (stream->writeFlag(palette >= 0))
    {
      stream->write(palette);
      palette = -1;
    }
  else
    {
      // palette change overrides flashes
      if (stream->writeFlag(damagecount))
	stream->write(damagecount);
      if (stream->writeFlag(bonuscount))
	stream->write(bonuscount);
      damagecount = bonuscount = 0;
    }

  if (stream->writeFlag(itemuse))
    stream->write(itemuse);
  itemuse = 0;
  */


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
      CONS_Printf("--- pi stuff received\n");
      stream->read(&number);
      stream->readString(temp);
      name = temp;
    }

  // feedback
  /*
  if (stream->readFlag())
    stream->read(&palette);
  else
    {
      // palette change overrides flashes
      if (stream->readFlag())
	stream->read(&damagecount);
      if (stream->readFlag())
	stream->read(&bonuscount);
    }

  if (stream->readFlag())
    stream->read(&itemuse);
  */
}



// send a message to the player
void PlayerInfo::SetMessage(const char *msg, int priority, int type)
{
  if (priority < messagefilter)
    return;  // not interested

  if (connection)
    connection->rpcMessage_s2c(number, msg, priority, type); // send it over network (usually server does this)
  else if (messages.size() < 20)
    {
      // the player is local and has room in her message queue
      // TODO high priority overrides low priority messages?
      message_t temp;
      temp.priority = priority;
      temp.type = type;
      temp.msg = msg; // makes a copy of msg, so we can use va() etc.

      messages.push_back(temp);
    }
}


// Separates the player from her current map, sets the new destination, handles pawn detachment.
// Does _not_ finish the map.
// NOTE may invalidate Map::players iterators.
void PlayerInfo::ExitLevel(int nextmap, int ep)
{
  // if a player is already going somewhere, let him keep his destination:
  if (!requestmap)
    {
      requestmap = nextmap;
      entrypoint = ep;
    }

  switch (playerstate) 
    {
    case PST_ALIVE:
      // save pawn for next level
      pawn->Detach();
      break;

    case PST_DEAD:
      // drop the pawn
      pawn->player = NULL;
      mp->QueueBody(pawn);
      pawn = NULL;
      break;

    case PST_REMOVE:
      if (pawn)
	pawn->Remove();
      pawn = NULL;
      break;

    default:
      // PST_NEEDMAP, PST_RESPAWN, PST_int, PST_wait, meaning the possible pawn is not connected to any Map
      break;
    }

  if (mp)
    mp->RemovePlayer(this); // NOTE that this may invalidate iterators to Map::players!

  if (playerstate == PST_REMOVE)  // you will still be removed!
    return;

  if (false) // TODO disable intermissions server option?
    {
      playerstate = PST_INTERMISSION;
      if (connection)
	connection->rpcStartIntermission_s2c(); // nonlocal players need intermission data
    }
  else
    playerstate = PST_NEEDMAP;
}


// Reset players between levels
void PlayerInfo::Reset(bool rpawn, bool rfrags)
{
  kills = items = secrets = time = 0;
  map_completed = false;

  if (pawn)
    {
      if (rpawn)
	pawn->Reset();
      pawn->powers[pw_allmap] = 0; // automap never carries over to the next map 
    }

  pov = NULL;

  // Initial height of PointOfView
  // will be set by player think.
  viewz = 1;

  if (rfrags)
    {
      score = 0;
      Frags.clear();
    }

  return;
}



//
// Calculate the walking / running height adjustment
//
void PlayerInfo::CalcViewHeight(bool onground)
{
  // 16 pixels of bob
#define MAXBOB  0x100000

  // Regular movement bobbing
  // (needs to be calculated for gun swing even if not on ground)
  // OPTIMIZE: tablify angle
  // Note: a LUT allows for effects like a ramp with low health.

  if ((pawn->eflags & MFE_FLY) && !onground)
    bob_amplitude = FRACUNIT/2;
  else
    {
      bob_amplitude = ((FixedMul(pawn->px,pawn->px) + FixedMul(pawn->py,pawn->py))*NEWTICRATERATIO) >> 2;

      if (bob_amplitude > MAXBOB)
	bob_amplitude = MAXBOB;
    }

  fixed_t eyes = (pawn->height * cv_viewheight.value) / 56; // default eye view height

  if ((pawn->cheats & CF_NOMOMENTUM) || !onground)
    {
      viewheight = eyes;
      viewz = pawn->z + eyes;
    }
  else
    {
      int phase = (FINEANGLES/20*game.tic/NEWTICRATERATIO) & FINEMASK;
      fixed_t bob = FixedMul(bob_amplitude/2, finesine[phase]);

      if (playerstate == PST_ALIVE)
	{
	  viewheight += deltaviewheight;
	  
	  if (viewheight > eyes)
	    {
	      viewheight = eyes;
	      deltaviewheight = 0;
	    }

	  if (viewheight < eyes/2)
	    {
	      viewheight = eyes/2;
	      if (deltaviewheight <= 0)
		deltaviewheight = 1;
	    }

	  if (deltaviewheight)
	    {
	      deltaviewheight += FRACUNIT/4;
	      if (!deltaviewheight)
		deltaviewheight = 1;
	    }
	}

      viewz = pawn->z + viewheight + bob - pawn->floorclip;

      if (viewz < pawn->floorz + 4*FRACUNIT)
	viewz = pawn->floorz + 4*FRACUNIT;
    }

  if (viewz > pawn->ceilingz - 4*FRACUNIT)
    viewz = pawn->ceilingz - 4*FRACUNIT;
}
