// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
//
// $Log$
// Revision 1.17  2003/11/30 00:09:43  smite-meister
// bugfixes
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
// Revision 1.12  2003/05/11 21:23:50  smite-meister
// Hexen fixes
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
//
//
//
// DESCRIPTION:
//   PlayerInfo class implementation
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "g_player.h"
#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "d_netcmd.h" // consvars

// global data

// PI's for both local players. Used by menu to setup their properties.
// These are only "models" for local players, the actual PI's
// that are added to a game are copy constructed from these.
PlayerInfo localplayer("Batman");
PlayerInfo localplayer2("Robin");

PlayerInfo *consoleplayer = NULL;   // player taking events
PlayerInfo *consoleplayer2 = NULL;   // secondary player taking events
PlayerInfo *displayplayer = NULL;   // view being displayed
PlayerInfo *displayplayer2 = NULL;  // secondary view (splitscreen)



PlayerInfo::PlayerInfo(const string & n)
{
  number = 0;
  team = 0;
  name = n;

  ptype = -1;
  color = 0;
  skin  = 0;

  spectator = false;
  playerstate = PST_WAITFORMAP;
  memset(&cmd, 0, sizeof(ticcmd_t));

  requestmap = entrypoint = 0;

  viewz = viewheight = deltaviewheight = bob = 0;
  message = NULL;

  for (int i = 0; i<NUMWEAPONS; i++)
    favoriteweapon[i] = 0;
  originalweaponswitch = true;
  autoaim = false;

  pawn = NULL;
  mp = NULL;
  time = 0;
  Reset(false, true);
};



static bool ultimatemsg;

void PlayerInfo::SetMessage(const char *msg, bool ultmsg)
{
  if ((ultimatemsg || !cv_showmessages.value) && !ultmsg)
    return;
    
  message = msg;
  //player->messageTics = MESSAGETICS;
  //BorderTopRefresh = true;
  if (ultmsg)
    ultimatemsg = true;
}


void PlayerInfo::ExitLevel(int nextmap, int ep, bool force)
{
  if (!requestmap || force)
    {
      requestmap = nextmap;
      entrypoint = ep;
    }

  switch (playerstate) 
    {
    case PST_ALIVE:
    case PST_DONE:
      // save pawn for next level
      pawn->Detach();
      break;

    case PST_DEAD:
      // drop the pawn
      pawn->player = NULL;
      mp->QueueBody(pawn);
      pawn = NULL;
      break;

    case PST_SPECTATOR:
    case PST_REMOVE:
      pawn->Remove();
      pawn = NULL;

    default:
      break;
    }

  mp->RemovePlayer(this);
  mp = NULL;
  playerstate = PST_WAITFORMAP;
}


// Reset players between levels
void PlayerInfo::Reset(bool rpawn, bool rfrags)
{
  kills = items = secrets = 0;

  if (pawn)
    {
      if (rpawn)
	pawn->Reset();
      pawn->powers[pw_allmap] = 0; // automap never carries over to the next map 
    }

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
