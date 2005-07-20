// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 2002-2005 by DooM Legacy Team.
//
// $Log$
// Revision 1.38  2005/07/20 20:27:19  smite-meister
// adv. texture cache
//
// Revision 1.37  2005/07/11 16:58:33  smite-meister
// msecnode_t bug fixed
//
// Revision 1.34  2005/04/19 18:28:16  smite-meister
// new RPCs
//
// Revision 1.33  2005/04/17 18:36:33  smite-meister
// netcode
//
// Revision 1.30  2004/11/18 20:30:07  smite-meister
// tnt, plutonia
//
// Revision 1.29  2004/11/13 22:38:42  smite-meister
// intermission works
//
// Revision 1.28  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.24  2004/07/13 20:23:36  smite-meister
// Mod system basics
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

#include "a_functions.h"
#include "bots/b_bot.h"

#include "n_connection.h"

#include "r_sprite.h"
#include "wi_stuff.h"
#include "tables.h"
#include "z_zone.h"


//============================================================
//           Global (nonstatic) client data
//============================================================

/// Local human players, then local bot player
LocalPlayerInfo LocalPlayers[NUM_LOCALPLAYERS] =
{
  LocalPlayerInfo("Batman", 0), LocalPlayerInfo("Robin", 1)
};

/// Locally observed players (multiple viewports...)
vector<PlayerInfo *> ViewPlayers;


bool G_RemoveLocalPlayer(PlayerInfo *p)
{
  if (p)
    {
      // remove the given player
      for (int k=0; k < NUM_LOCALPLAYERS; k++)
	if (LocalPlayers[k].info == p)
	  {
	    LocalPlayers[k].info = NULL;
	    return true;
	  }

      return false;
    }

  // remove all players
  for (int k=0; k < NUM_LOCALPLAYERS; k++)
    LocalPlayers[k].info = NULL;

  return true;
}


//============================================================
//   PlayerOptions class
//============================================================

static char default_weaponpref[NUMWEAPONS] =
{
  1,5,6,7,3,9,2,4,8,  // Doom
  1,4,5,7,8,3,9,6,0,  // Heretic
  2,2,2,4,4,4,6,6,6,0,0,0,0 // Hexen
};


PlayerOptions::PlayerOptions(const string &n)
{
  name = n;

  ptype = -1;
  pclass = PCLASS_NONE;
  color = 0;
  skin  = 0; // "marine"?

  autoaim = true;
  originalweaponswitch = true;
  for (int i=0; i<NUMWEAPONS; i++)
    weaponpref[i] = default_weaponpref[i];

  messagefilter = 2;
}


/// Sends the player preferences to the server.
void PlayerOptions::Write(BitStream *stream)
{
  stream->writeString(name.c_str());
  stream->write(ptype);
  stream->write(pclass);
  stream->write(color);
  stream->write(skin);

  stream->write(autoaim);
  stream->write(originalweaponswitch);
  for (int i=0; i<NUMWEAPONS; i++)
    stream->write(weaponpref[i]);

  stream->write(messagefilter);
}

/// server reads remote player preferences sent by client
void PlayerOptions::Read(BitStream *stream)
{
  char temp[256];

  stream->readString(temp);
  temp[32] = '\0'; // limit name length
  name = temp;

  stream->read(&ptype);
  stream->read(&pclass);
  stream->read(&color);
  stream->read(&skin);

  stream->read(&autoaim);
  stream->read(&originalweaponswitch);
  for (int i=0; i<NUMWEAPONS; i++)
    stream->read(&weaponpref[i]);

  stream->read(&messagefilter);
}



//============================================================
//   LocalPlayerInfo class
//============================================================

LocalPlayerInfo::LocalPlayerInfo(const string &n, int keyset)
  : PlayerOptions(n)
{
  controlkeyset = keyset;
  autorun = false;
  crosshair = 0;

  info = NULL;
  ai = NULL;
}


/// Builds the control struct (ticcmd) based on human input or AI decisions
void LocalPlayerInfo::GetInput(int elapsed)
{
  if (!info)
    return;

  // inventory timer (TODO does it belong here?)
  if (info->invTics)
    info->invTics--;

  if (ai)
    ai->BuildInput(info, elapsed);
  else
    info->cmd.Build(this, elapsed);
}



//============================================================
//   PlayerInfo class
//============================================================

TNL_IMPLEMENT_NETOBJECT(PlayerInfo);

PlayerInfo::PlayerInfo(const LocalPlayerInfo *p)
{
  if (p)
    {
      options = *p;
      name = options.name;
    }

  number = 0;
  team = 0;

  connection = NULL;
  client_hash = 0;

  playerstate = PST_NEEDMAP;
  spectator = false;
  map_completed = false;
  leaving_map = false;

  requestmap = entrypoint = 0;

  mp = NULL;
  pawn = NULL;
  pov = NULL;
  hubsavepawn = NULL;

  viewz = viewheight = deltaviewheight = 0;
  palette = -1;
  damagecount = bonuscount = 0;
  itemuse = false;

  Reset(false, true);  // clear score, frags...

  cmd.Clear();
  invTics = invSlot = invPos = 0;

  // net stuff
  mNetFlags.set(Ghostable);
};


/// This is called on clients when a new player joins
bool PlayerInfo::onGhostAdd(class GhostConnection *c)
{
  CONS_Printf("%s has joined the game (player %d)\n", name.c_str(), number);
  game.AddPlayer(this);

  if (!LConnection::joining_players.empty())
    {
      list<LocalPlayerInfo *>::iterator t = LConnection::joining_players.begin();
      for ( ; t != LConnection::joining_players.end(); t++)
	{
	  LocalPlayerInfo *p = *t;
	  if (p->pnumber == number)
	    {
	      if (p->info)
		I_Error("Multiple ghostadds!\n");

	      p->info = this;
	      LConnection::joining_players.erase(t); // the PlayerInfo found its owner
	      break;
	    }
	}
    }

  return true;
}


/// This is called on clients when a player leaves
void PlayerInfo::onGhostRemove()
{
  CONS_Printf("%s has left the game.\n", name.c_str());
  //game.RemovePlayer(number);
  playerstate = PST_REMOVE;
}


enum mask_e
{
  M_IDENTITY   = BIT(1),
  M_STATE      = BIT(2),
  M_SCORE      = BIT(3),
  M_PALETTE    = BIT(4),
  M_HUDFLASH   = BIT(5),
  M_EVERYTHING = 0xFFFFFFFF,
};

/// This is the function the server uses to ghost PlayerInfo data to clients.
U32 PlayerInfo::packUpdate(GhostConnection *c, U32 mask, class BitStream *stream)
{
  if (isInitialUpdate())
    mask = M_EVERYTHING;

  // check which states need to be updated, and write updates
  if (stream->writeFlag(mask & M_IDENTITY))
    {
      CONS_Printf("--- pi stuff sent\n");
      stream->writeString(name.c_str());
      stream->write(number);
      stream->write(team);
    }

  if (stream->writeFlag(mask & M_STATE))
    {
      U8 temp = playerstate;
      stream->write(temp);
    }

  if (stream->writeFlag(mask & M_SCORE))
    {
      stream->write(score);
    }

  // TODO: mp, pawn, pov?

  // feedback (goes only to the owner)
  if (c != connection)
    {
      stream->writeFlag(false); // nothing further for others
      return 0;
    }
  else
    stream->writeFlag(true);


  if (stream->writeFlag(mask & M_PALETTE))
    {
      stream->write(palette);
      palette = -1;
    }

  if (stream->writeFlag(mask & M_HUDFLASH))
    {
      // palette change overrides flashes
      if (stream->writeFlag(damagecount))
	stream->write(damagecount);
      if (stream->writeFlag(bonuscount))
	stream->write(bonuscount);
      damagecount = bonuscount = 0;
    }

  stream->writeFlag(itemuse);
  itemuse = false;


  // the return value from packUpdate can set which states still
  // need to be updated for this object.
  return 0;
}


///
void PlayerInfo::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
  char temp[256];

  // NOTE: the unpackUpdate function must be symmetrical to packUpdate
  if (stream->readFlag())
    {
      CONS_Printf("--- pi stuff received\n");
      stream->readString(temp);
      name = temp;
      stream->read(&number);
      stream->read(&team);
    }

  if (stream->readFlag())
    {
      U8 temp;
      stream->read(&temp);
      playerstate = playerstate_t(temp);
    }

  if (stream->readFlag())
    stream->read(&score);

  if (!stream->readFlag())
    return; // nothing more

  // feedback

  if (stream->readFlag())
    stream->read(&palette);

  if (stream->readFlag())
    {
      // palette change overrides flashes
      if (stream->readFlag())
	stream->read(&damagecount);
      if (stream->readFlag())
	stream->read(&bonuscount);
    }

  itemuse = stream->readFlag();
}



// send a message to the player
void PlayerInfo::SetMessage(const char *msg, int priority, int type)
{
  if (priority > options.messagefilter)
    return;  // not interested (lesser is more important!)

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


// Sets the new destination Map for the player. Does _not_ finish the current Map.
void PlayerInfo::ExitLevel(int nextmap, int ep)
{
  // if a player is already going somewhere, let him keep his destination:
  if (!requestmap)
    {
      requestmap = nextmap;
      entrypoint = ep;
    }

  leaving_map = true;
}


// Starts an intermission for the player if appropriate.
bool PlayerInfo::CheckIntermission(const Map *m)
{
  if (playerstate == PST_REMOVE)
    return false; // no intermission for you!

  MapInfo *info = game.FindMapInfo(requestmap); // do we need an intermission?

  if (info) // TODO disable intermissions server option?
    {
      if (connection)
	connection->rpcStartIntermission_s2c(); // nonlocal players need intermission data
      else
	{
	  // for locals, the intermission is started
	  wi.Start(m, info);
	  game.StartIntermission();
	}
      playerstate = PST_INTERMISSION; // rpc_end_intermission resets this
      return true;
    }
  else
    playerstate = PST_NEEDMAP;

  return false;
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




/// Client: Calculate the walking / running viewpoint bobbing and weapon swing
void PlayerInfo::CalcViewHeight()
{
  if (!pawn)
    return;

  bool onground = (pawn->z <= pawn->floorz);

  // 16 pixels of bob
  const fixed_t MAXBOB = 16*FRACUNIT;

  // Regular movement bobbing
  // basically pawn speed squared, affects weapon swing
  fixed_t bob_amplitude; 

  if ((pawn->eflags & MFE_FLY) && !onground)
    bob_amplitude = FRACUNIT/2;
  else
    {
      bob_amplitude = (FixedMul(pawn->px, pawn->px) + FixedMul(pawn->py, pawn->py)) >> 2;

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
      int phase = (FINEANGLES/20 * game.tic) & FINEMASK;
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


  // server decides the rising/lowering of weapons (sy coord),
  // but client does the bobbing independently

  pspdef_t *psp = pawn->psprites;

  if (psp[ps_weapon].state && psp[ps_weapon].state->action == A_WeaponReady)
    {
      // bob the weapon based on movement speed
      int angle = (128 * game.tic) & FINEMASK;
      psp[ps_weapon].sx = FRACUNIT + FixedMul(bob_amplitude, finecosine[angle]);
      angle &= FINEANGLES/2-1;
      psp[ps_weapon].sy = WEAPONTOP + FixedMul(bob_amplitude, finesine[angle]);

      psp[ps_flash].sx = psp[ps_weapon].sx;
      psp[ps_flash].sy = psp[ps_weapon].sy;
    }
}




/// Makes a copy of the pawn (when the player enters a map)
void PlayerInfo::SavePawn()
{
  if (hubsavepawn)
    delete hubsavepawn;

  if (pawn)
    {
      hubsavepawn = new PlayerPawn(*pawn);
      hubsavepawn->player = NULL; // to avoid conflicts with the real pawn... ok, it's bad design:)
      hubsavepawn->pres = NULL;
    }
  else
    hubsavepawn = NULL;
}


/// Copies the saved pawn as the actual pawn
void PlayerInfo::LoadPawn()
{
  pawn = new PlayerPawn(*hubsavepawn);
  pawn->player = this;

  // and a new presentation
  const mobjinfo_t *info = &mobjinfo[pawn->pinfo->mt];
  pawn->pres = new spritepres_t(sprnames[states[info->spawnstate].sprite], info, 0);
}
