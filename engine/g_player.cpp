// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2004 by DooM Legacy Team.
//
// $Log$
// Revision 1.23  2004/07/09 19:43:39  smite-meister
// Netcode fixes
//
// Revision 1.22  2004/07/07 17:27:19  smite-meister
// bugfixes
//
// Revision 1.21  2004/07/05 16:53:24  smite-meister
// Netcode replaced
//
// Revision 1.20  2004/03/28 15:16:12  smite-meister
// Texture cache.
//
// Revision 1.19  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.18  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
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

#include "g_input.h"
#include "d_event.h"
#include "tables.h"

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

static char default_weaponpref[NUMWEAPONS] =
{
  1,4,5,6,8,7,3,9,2,  // Doom
  1,4,5,7,8,3,9,6,0,  // Heretic
  2,2,2,4,4,4,6,6,6,0,0,0,0 // Hexen
};


// TNL_IMPLEMENT_NETOBJECT(PlayerInfo);

PlayerInfo::PlayerInfo(const string & n)
{
  number = 0;
  team = 0;
  name = n;

  ptype = -1;
  color = 0;
  skin  = 0;

  connection = NULL;
  spectator = false;
  playerstate = PST_WAITFORMAP;
  memset(&cmd, 0, sizeof(ticcmd_t));

  requestmap = entrypoint = 0;

  viewz = viewheight = deltaviewheight = bob_amplitude = 0;
  message = NULL;

  for (int i = 0; i<NUMWEAPONS; i++)
    weaponpref[i] = default_weaponpref[i];
  originalweaponswitch = true;
  autoaim = false;

  pawn = NULL;
  pov = NULL;
  mp = NULL;
  time = 0;
  Reset(false, true);

  // net stuff
  // mNetFlags.set(Ghostable);
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
      // PST_WAITFORMAP, PST_RESPAWN, meaning there is no pawn
      break;
    }

  if (mp)
    mp->RemovePlayer(this); // NOTE that this may invalidate iterators to Map::players!

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

  if ((pawn->flags2 & MF2_FLY) && !onground)
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



bool PlayerInfo::InventoryResponder(int (*gc)[2], event_t *ev)
{
  //gc is a pointer to array[num_gamecontrols][2]
  extern int st_curpos; // TODO: what about splitscreenplayer??

  if (!game.inventory)
    return false;

  if (!pawn)
    return false;

  switch (ev->type)
    {
    case ev_keydown :
      if (ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1])
        {
          if (pawn->invTics)
            {
              if (--(pawn->invSlot) < 0)
                pawn->invSlot = 0;
              else if (--st_curpos < 0)
                st_curpos = 0;
            }
          pawn->invTics = 5*TICRATE;
          return true;
        }
      else if (ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
        {
          int n = pawn->inventory.size();

          if (pawn->invTics)
            {
              if (++(pawn->invSlot) >= n)
                pawn->invSlot = n-1;
              else if (++st_curpos > 6)
                st_curpos = 6;
            }
          pawn->invTics = 5*TICRATE;
          return true;
        }
      else if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1])
        {
          if (pawn->invTics)
            pawn->invTics = 0;
          else if (pawn->inventory[pawn->invSlot].count > 0)
	    cmd.item = pawn->inventory[pawn->invSlot].type + 1;

          return true;
        }
      break;

    case ev_keyup:
      if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1] ||
          ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1] ||
          ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
        return true;
      break;

    default:
      break; // shut up compiler
    }
  return false;
}
