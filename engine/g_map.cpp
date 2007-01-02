// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Map class implementation

#include "g_map.h"
#include "g_mapinfo.h"
#include "g_game.h"
#include "g_team.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_decorate.h"

#include "command.h"
#include "cvars.h"

#include "b_path.h"

#include "p_spec.h"
#include "p_hacks.h"
#include "r_poly.h"

#include "r_splats.h"

#include "hud.h"
#include "m_random.h"
#include "wi_stuff.h"

#include "sounds.h"
#include "s_sound.h"
#include "z_zone.h"
#include "tables.h"


const fixed_t FLOATRANDZ = fixed_t::FMAX-1;


// Map class constructor
Map::Map(MapInfo *i)
{
  info = i;
  lumpname = i->lumpname;
  hexen_format = false;

  vertexes = NULL;
  segs = NULL;
  sectors = NULL;
  subsectors = NULL;
  nodes = NULL;
  lines = NULL;
  sides = NULL;
  polyobjs = NULL;
  PolyBlockMap = NULL;
  linebuffer = NULL;

  glvertexes   = NULL;
  glvis = NULL;

  bmap.index = NULL;
  bmap.lists = NULL;
  blocklinks = NULL;

  rejectmatrix = NULL;

  fadetable = NULL;

  levelscript = NULL;
  runningscripts = NULL;

  ACScriptCount = 0;
  ACSInfo = NULL;
  ActionCodeBase = NULL;
  ACStrings = NULL;

  mapthings = NULL;

  botnodes = NULL;

  ActiveAmbientSeq = NULL;

  force_pointercheck = false;

  braintargeton = 0;
};


// destructor
Map::~Map()
{
  Z_Free(vertexes);
  Z_Free(segs);
  Z_Free(subsectors);
  Z_Free(sectors);
  Z_Free(nodes);
  Z_Free(lines);
  Z_Free(sides);
  Z_Free(linebuffer);

  // Remove GL nodes if they exist.
  if (glvertexes)
    Z_Free(glvertexes);
  if(glvis)
    Z_Free(glvis);

  Z_Free(bmap.index);
  Z_Free(bmap.lists);

  if (polyobjs)
    {
      for (int i=0; i < NumPolyobjs; i++)
	{
	  if (polyobjs[i].segs)
	    Z_Free(polyobjs[i].segs);

	  if (polyobjs[i].originalPts)
	    Z_Free(polyobjs[i].originalPts);
	}

      Z_Free(polyobjs);
      Z_Free(blocklinks);
      Z_Free(PolyBlockMap);
      // FIXME there is a lot of PU_LEVEL polyblockmap stuff that is not yet freed
    }

  Z_Free(rejectmatrix);

  // FIXME free FS stuff
  FS_ClearRunningScripts();

  if (ACSInfo)
    {
      Z_Free(ACSInfo);
      Z_Free(ActionCodeBase);
    }

  if (ACStrings)
    Z_Free(ACStrings);

  Z_Free(mapthings);

  if (botnodes)
    delete botnodes;

  Thinker *t, *next;
  for (t = thinkercap.next; t != &thinkercap; t = next)
    {
      next = t->next;
      delete t;
    }

  int n = DeletionList.size();
  for (int i=0; i<n; i++)
    delete DeletionList[i];

  // TODO levelflats? anims? sound sequences?

  // Some things may well be left undeleted, because their memory will be freed
  // during the next cluster change using Z_FreeTags...

  // clear the splats from deleted map
  R_ClearLevelSplats(); // FIXME find a better way
  // 3D sounds must be stopped when their sources are deleted...
  S.Stop3DSounds(); // TODO not correct, since several maps may run simultaneously.
}



//===================================================
//             GAME SPAWN FUNCTIONS
//===================================================


void Map::SpawnActor(Actor *p)
{
  AddThinker(p);     // AddThinker sets Map *mp
  if (game.server)
    p->TestLocation(); // TEST, sets tmfloorz, tmceilingz
  p->SetPosition();  // set subsector and/or block links
}



/// when something disturbs a liquid surface, we get a splash
DActor *Map::SpawnSplash(const vec_t<fixed_t>& pos, fixed_t z, int sound, mobjtype_t base,
		      mobjtype_t chunk, bool randtics)
{
  // spawn a base splash
  DActor *p = SpawnDActor(pos.x, pos.y, z, base);
  S_StartSound(p, sound);

  if (randtics)
    {
      p->tics -= P_Random() & 3;

      if (p->tics < 1)
	p->tics = 1;
    }

  if (chunk == MT_NONE)
    return p;

  // and possibly an additional chunk
  p = SpawnDActor(pos.x, pos.y, z, chunk);
  return p;
}



// spawn a blood sprite with falling z movement, at location
// the duration and first sprite frame depends on the damage level
// the more damage, the longer is the sprite animation
DActor *Map::SpawnBlood(const vec_t<fixed_t>& r, int damage)
{
  DActor *th = SpawnDActor(r.x, r.y, r.z + P_SignedFRandom(6), MT_BLOOD);

  th->vel.Set(P_SignedFRandom(4), P_SignedFRandom(4), fixed_t(2));
  th->tics -= P_Random()&3;

  if (th->tics < 1)
    th->tics = 1;

  if (damage <= 12 && damage >= 9)
    th->SetState(S_BLOOD2);
  else if (damage < 9)
    th->SetState(S_BLOOD3);

  return th;
}



/// when player gets hurt by lava/slime, spawn at feet
void Map::SpawnSmoke(fixed_t x, fixed_t y, fixed_t z)
{
  x += (P_Random() & 8) - 4;
  y += (P_Random() & 8) - 4;
  z += P_Random() & 3;

  DActor *th = SpawnDActor(x,y,z, MT_SMOK);
  th->vel.z = 1;
  th->tics -= P_Random() & 3;

  if (th->tics < 1)
    th->tics = 1;
}



/// adds a DActor to a Map
DActor *Map::SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t)
{
  DActor *p = new DActor(nx, ny, nz, aid[t]);
  AddThinker(p);

  //p->TestLocation(nx, ny); // TODO sets tmfloorz, tmceilingz. Wrong, since for missiles owner is not yet set => collides
  // set subsector and/or block links
  p->SetPosition();

  p->floorz = p->subsector->sector->floorheight;
  p->ceilingz = p->subsector->sector->ceilingheight;

  if (nz == ONFLOORZ || (p->flags2 & MF2_FLOORHUGGER))
    {
      //added:28-02-98: defaults onground
      p->eflags |= MFE_ONGROUND;
      p->pos.z = p->floorz;
    }
  else if (nz == ONCEILINGZ || (p->flags2 & MF2_CEILINGHUGGER))
    p->pos.z = p->ceilingz - p->height;
  else if (nz == FLOATRANDZ)
    {
      fixed_t space = p->ceilingz - p->height - p->floorz;
      if (space > 48)
        {
	  space -= 40;
	  p->pos.z = ((space*P_Random()) >> 8) + p->floorz + 40;
        }
      else
	p->pos.z = p->floorz;
    }
  else
    p->pos.z = nz;

  if ((p->flags2 & MF2_FOOTCLIP) && (p->subsector->sector->floortype >= FLOOR_LIQUID)
      && (p->floorz == p->subsector->sector->floorheight))
    p->floorclip = FOOTCLIPSIZE;
  else
    p->floorclip = 0;


  // TEST monster teams
  if (t < MT_HERETIC)
    p->team = TeamInfo::TEAM_Doom;
  else if (t < MT_HEXEN)
    p->team = TeamInfo::TEAM_Heretic;
  else
    p->team = TeamInfo::TEAM_Hexen;

  return p;
}



// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
void Map::SpawnPlayer(PlayerInfo *pi, mapthing_t *mthing)
{
  fixed_t     nx, ny, nz;

  nx = mthing->x;
  ny = mthing->y;
  nz = ONFLOORZ;

  PlayerPawn *p;

  // the player may have his old pawn from the previous level
  if (!pi->pawn)
    {
      p = new PlayerPawn(nx, ny, nz, pi->options.ptype, pi->options.pclass);
      p->player = pi;
      p->team = pi->team;
      pi->pawn  = p;
    }
  else
    {
      p = pi->pawn;
      p->pos.Set(nx, ny, nz);
      p->vel.Set(0, 0, 0);
    }

  SpawnActor(p); // spawn the pawn

  // Boris stuff
  if (!pi->options.originalweaponswitch)
    p->UseFavoriteWeapon();

  p->eflags |= MFE_ONGROUND;
  p->pos.z = p->floorz;

  // FIXME set skin sprite here
  // set color translations for player sprites
  p->color = pi->options.color;

  p->yaw = ANG45 * (mthing->angle/45);

  // setup gun psprite
  p->SetupPsprites();

  // give all cards in death match mode
  if (cv_deathmatch.value)
    p->keycards = it_allkeys;

  p->spawnpoint = mthing;
  mthing->mobj = p;
  mthing->type = short((maptic + 20) & 0xFFFF); // set the timer

  // TODO spawn a teleport fog?
  /*
    unsigned an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;
    Actor *fog = SpawnDActor(nx+20*finecosine[an], ny+20*finesine[an], nz, MT_TFOG);
    S_StartSound(fog, sfx_telept); // don't start sound on first frame?
  */

  pi->pov = p;
  pi->viewheight = cv_viewheight.value;
  pi->viewz = p->pos.z + pi->viewheight;
  pi->playerstate = PST_INMAP;
  pi->setMaskBits(PlayerInfo::M_PAWN); // notify network system
}



// The fields of the mapthing are already in host byte order.
DActor *Map::SpawnMapThing(mapthing_t *mt, bool initial)
{
  mobjtype_t t = mobjtype_t(mt->type);
  ActorInfo *ai = aid[t];

  // don't spawn keycards in deathmatch
  if (cv_deathmatch.value && ai->flags & MF_NOTDMATCH)
    return NULL;

  // don't spawn any monsters if -nomonsters
  if (cv_nomonsters.value && (ai->flags & MF_MONSTER))
    return NULL;

  // spawn it
  fixed_t nx = mt->x;
  fixed_t ny = mt->y;
  fixed_t nz;

  if (ai->flags & MF_SPAWNCEILING)
    nz = ONCEILINGZ;
  else if (ai->flags & MF_SPAWNFLOAT)
    nz = FLOATRANDZ;
  else
    {
      //nz = mt->z << FRACBITS + R_PointInSubsector(nx, ny)->sector->floorheight;
      nz = ONFLOORZ;
    }

  DActor *p = SpawnDActor(nx, ny, nz, t);
  if (nz == ONFLOORZ)
    p->pos.z += mt->z;
  else if (nz == ONCEILINGZ)
    p->pos.z -= mt->z;

  p->spawnpoint = mt;
  p->tid = mt->tid;
  p->special = mt->special;
  p->args[0] = mt->args[0];
  p->args[1] = mt->args[1];
  p->args[2] = mt->args[2];
  p->args[3] = mt->args[3];
  p->args[4] = mt->args[4];

  if (p->tid)
    InsertIntoTIDmap(p, p->tid);

  // Seed random starting index for bobbing motion
  if (p->flags2 & MF2_FLOATBOB)
    {
      p->reactiontime = P_Random();
      p->special1 = mt->z; // floating height (integer, NOT fixed_t)
    }

  if (p->tics > 0)
    p->tics = 1 + (P_Random () % p->tics);

  if (initial)
    {
      if (p->flags & MF_COUNTKILL)
	kills++;
      if (p->flags & MF_COUNTITEM)
	items++;
    }
  else
    {
      // spawn teleport fog (not in Heretic?)
      Actor *fog = SpawnDActor(nx, ny, nz, MT_IFOG);
      S_StartSound(fog, sfx_itemrespawn);
    }

  // yaw
  if (p->flags & MF_MONSTER)
    p->yaw = ANG45 * (mt->angle/45);
  else
    p->yaw = ((mt->angle << 8)/360) << 24; // full angle resolution

  if (mt->flags & MTF_AMBUSH)
    p->flags |= MF_AMBUSH;

  if (hexen_format && mt->flags & MTF_DORMANT)
    {
      p->flags2 |= MF2_DORMANT;
      if (t == MT_ICEGUY)
	p->SetState(S_ICEGUY_DORMANT, true);
      p->tics = -1;
    }

  mt->mobj = p;

  return p;
}


//
// Returns false if the player p cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
bool Map::CheckRespawnSpot(PlayerInfo *p, mapthing_t *mthing)
{
  if (!p || !mthing)
    return false;

  if (mthing->args[0] != p->entrypoint)
    return false;

  // has the spawn spot been used just recently?
  // (less stupid telefrag this way!)
  // damn short int! it's just 16 bits long! and signed too!
  if ((maptic & 0xFFFF) < (unsigned short)mthing->type)
    return false;

  fixed_t x = mthing->x;
  fixed_t y = mthing->y;
  subsector_t *ss = R_PointInSubsector(x,y);

  // check for respawn in team-sector
  if (ss->sector->teamstartsec)
    if (cv_teamplay.value)
      if (p->team != ss->sector->teamstartsec)
	return false; // start is meant for another team

  // will it fit there?
  // FIXME! at this point p has no longer a pawn, besides,
  // if the new pawn is larger than the previous, it wouldn't help anyway
  // no size checking right now.
  //if (!p->pawn->TestLocation(x, y)) return false;

  return true;
}



// Spawns a player at one of the random death match spots
// called at level load and each death
bool Map::DeathMatchRespawn(PlayerInfo *p)
{
  int n = dmstarts.size();
  if (n == 0)
    I_Error("No deathmatch start in this map !");

  int i, j;

  // TODO  create a random n-permutation and use it!
  j = i = P_Random() % n;
  do {
    if (CheckRespawnSpot(p, dmstarts[j]))
      {
	SpawnPlayer(p, dmstarts[j]);
	return true;
      }
    j++;
    if (j == n)
      j = 0;
  } while (j != i);

  return false;
}


bool Map::CoopRespawn(PlayerInfo *p)
{
  int i = p->number;
  multimap<int, mapthing_t *>::iterator s, t;

  // let's check his own start first
  s = playerstarts.lower_bound(i);
  t = playerstarts.upper_bound(i);
  for ( ; s != t--; )
    {
      mapthing_t *m = t->second;
      // the pawn is spawned at the last playerstart
      if (CheckRespawnSpot(p, m))
	{
	  SpawnPlayer(p, m);

	  // spawn voodoo dolls in all other playerstarts
	  if (cv_voodoodolls.value)
	    {
	      for ( ; s != t; s++)
		{
		  m = s->second;
		  if (CheckRespawnSpot(p, m))
		    VoodooDoll::Spawn(p, m);
		}
	    }

	  return true;
	}
    }

  // try to spawn at one of the other players' spots, then
  s = playerstarts.begin();
  t = playerstarts.end();
  for ( ; s != t; s++)
    {
      mapthing_t *m = s->second;
      if (CheckRespawnSpot(p, m))
	{
	  SpawnPlayer(p, m);
	  return true;
	}
    }

  return false;
}


// tries to respawn the players waiting in respawnqueue
int Map::RespawnPlayers()
{
  // Let's try to empty the respawnqueue!
  // players in respawnqueue may or may not have pawns

  int count = 0;
  PlayerInfo *p;

  bool ok;
  do {
    //CONS_Printf("RespawnPlayers: %d, count = %d\n", respawnqueue.size(), count);
    p = respawnqueue.front();
    if (p == NULL)
      {
	// player has been removed while in respawn queue
	respawnqueue.pop_front();
	ok = true;
	continue;
      }

    ok = false;

    // spawn at random spot if in death match
    if (cv_deathmatch.value)
      {
	if (DeathMatchRespawn(p))
	  ok = true;
	else
	  ok = CoopRespawn(p);
      }
    else
      ok = CoopRespawn(p);

    if (ok)
      {
	respawnqueue.pop_front();
	count++;
      }
    else
      {
	// check that the requested entrypoint even exists
	multimap<int, mapthing_t *>::iterator s, t;	
	s = playerstarts.begin();
	t = playerstarts.end();
	for ( ; s != t; s++)
	  {
	    mapthing_t *m = s->second;
	    if (m->args[0] == p->entrypoint)
	      break;
	  }

	if (s == t)
	  {
	    CONS_Printf("Nonexistant entrypoint (%d) requested!\n", p->entrypoint);
	    p->entrypoint = 0;
	  }

	if (p->time++ > 10)
	  {
	    // this player has already waited for respawn for 10 ticks!
	    I_Error("Could not respawn player %d to entrypoint %d\n", p->number, p->entrypoint);
	  }
      }
  } while (ok && !respawnqueue.empty());

  return count;
}


// called when a dead player pushes USE
void Map::RebornPlayer(PlayerInfo *p)
{
  // at this point p->playerstate should be PST_INMAP or PST_FINISHEDMAP
  // and p->pawn not NULL

  // first dissociate the corpse
  if (p->pawn)
    {
      p->pawn->player = NULL;
      QueueBody(p->pawn);
      p->pawn = NULL;
    }

  p->pov = NULL;

  if (!game.multiplayer)
    {
      // in single player games, the map is reset after death
      info->state = MapInfo::MAP_RESET;
      RemovePlayer(p);
      p->requestmap = info->mapnumber; // get back here after the reset...
      p->playerstate = PST_NEEDMAP;
    }
  else
    {
      respawnqueue.push_back(p);
      p->time = 0; // respawn delay counter
      p->playerstate = PST_RESPAWN;
    }

  // should we load a saved playerpawn?  
  if (info->hub)
    p->LoadPawn();

  p->setMaskBits(PlayerInfo::M_PAWN); // notify network system
}


// Adds a player to a Map. The player is immediately queued for respawn.
// Only called from MapInfo.
void Map::AddPlayer(PlayerInfo *p)
{
  // At this point the player may or may not have a pawn.
  p->mp = this;
  players.push_back(p);

  if (!game.server)
    return;

  respawnqueue.push_back(p);
  p->time = 0; // respawn delay counter
  p->playerstate = PST_RESPAWN;
  p->requestmap = 0;

  // TODO if (p->spectator) spawn PST_SPECTATOR

  if (p->pawn)
    {
      p->pawn->eflags &= ~MFE_REMOVE;
      Z_ChangeTag(p->pawn, PU_LEVSPEC);
      if (p->pawn->pres)
	Z_ChangeTag(p->pawn->pres, PU_LEVSPEC);

      // save the playerpawns as they enter the map
      if (info->hub)
	p->SavePawn();
    }
}


// removes a player from map (but does not remove the pawn!)
// used by server in sp games, and by client
bool Map::RemovePlayer(PlayerInfo *p)
{
  vector<PlayerInfo *>::iterator i;
  for (i = players.begin(); i != players.end(); i++)
    if (*i == p)
      {
	p->mp = NULL;
	players.erase(i);
	return true;
      }
  return false;
}


// post-tick:
// Check if any players are leaving, handle the pawns properly, remove the players from the map.
int Map::HandlePlayers()
{
  if (info->state == MapInfo::MAP_RESET)
    return 0; // single player, special case

  // Clean respawnqueue from removed players
  int n = respawnqueue.size();
  for (int i=0; i<n; i++)
    if (respawnqueue[i]->playerstate == PST_REMOVE)
      respawnqueue[i] = NULL;

  unsigned fin = 0;
  int r = 0;

  // we iterate the vector in reversed order since we may have to delete some items on the way
  vector<PlayerInfo *>::iterator t;
  for (t = players.end(); t != players.begin(); )
    {
      --t; // since t was not equal to begin(), this must be valid

      PlayerInfo *p = *t;
      

      if (p->playerstate == PST_LEAVINGMAP || p->playerstate == PST_REMOVE)
	{
	  //CONS_Printf(" Map:: Removing player %s.\n", p->name.c_str());
	  r++;
	  players.erase(t);
	  p->mp = NULL;

	  // handle the pawn
	  PlayerPawn *w = p->pawn;
	  if (w)
	    switch (p->playerstate) 
	      {
	      case PST_LEAVINGMAP:
		if (w->flags & MF_CORPSE)
		  {
		    // drop the dead pawn
		    w->player = NULL;
		    QueueBody(w);
		    p->pawn = NULL;
		  }
		else
		  {
		    // save pawn for next level
		    w->Detach();
		  }

		// intermission?
		if (game.server)
		  {
		    // Starts an intermission for the player if appropriate.
		    MapInfo *next = game.FindMapInfo(p->requestmap); // do we need an intermission?

		    // only use intermissions if mapchanges are synchronous
		    if (next && cv_intermission.value)
		      {
			if (p->connection)
			  // nonlocal players need intermission data
			  // TODO need we send Map kills, items or secrets?
			  p->s2cStartIntermission(info->mapnumber, next->mapnumber, maptic, kills, items, secrets);
			else
			  {
			    // for locals, the intermission is started
			    wi.Start(info, next, maptic, kills, items, secrets);
			    game.StartIntermission();
			  }

			p->playerstate = PST_INTERMISSION; // c2sIntermissionDone rpc resets this
			return true;
		      }
		    else
		      p->playerstate = PST_NEEDMAP;
		  }
		break;

	      case PST_REMOVE:
		w->Remove();
		p->pawn = NULL;
		break;

	      default:
		// not possible
		break;
	      }
	}
      else if (p->playerstate == PST_FINISHEDMAP)
	fin++;
    }

  if (players.size() != fin)
    return r;

  // TODO handle spectators...

  // all the remaining players are finished, so let's finish the Map.
  if (game.server)
    info->state = MapInfo::MAP_FINISHED;
  else
    info->state = MapInfo::MAP_INSTASIS; // TEST


  return r;
}



// returns player 'number' if he is in the map, otherwise NULL
PlayerInfo *Map::FindPlayer(int num)
{
  int i, n = players.size();
  for (i = 0; i < n; i++)
    if (players[i]->number == num)
      return players[i];

  return NULL;
}



void Map::QueueBody(Actor *p)
{
  p->flags2 &= ~MF2_DONTDRAW;

  // flush an old corpse if needed
  if (bodyqueue.size() >= BODYQUESIZE)
    {
      bodyqueue.front()->Remove();
      bodyqueue.pop_front();
    }
  bodyqueue.push_back(p);
}


/// Kills all monsters.
int Map::Massacre()
{
  int count = 0;

  for (Thinker *th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      if (!th->IsOf(DActor::_type))
	continue; // Not a dactor
	
      Actor *mo = reinterpret_cast<Actor*>(th);
      if ((mo->flags & MF_MONSTER) && (mo->health > 0))
	{
	  mo->flags2 &= ~(MF2_NONSHOOTABLE + MF2_INVULNERABLE);
	  mo->flags |= MF_SHOOTABLE;
	  mo->Damage(NULL, NULL, 10000, dt_always);
	  count++;
	}
    }
  return count;
}


// helper function for Map::BossDeath
static const state_t *P_FinalState(const state_t *state)
{
  while (state->tics != -1)
    state = state->nextstate;

  return state;
}


void Map::BossDeath(const DActor *mo)
{
  // TODO! This is how it is supposed to work:
  // A map knows what it should do when, say, the last monster of type X dies.

  // It may be doing something to tags 666 or 667, or maybe even exiting the
  // map (but not necessarily the cluster!)
  // But how to code it generally and efficiently?
  // Maybe just copy existing Doom/Doom2/Heretic 666 and 667 tricks and do everything else by FS?

  // Ways to end level:
  // Baron of Hell, Cyberdemon, Spider Mastermind,
  // Mancubus, Arachnotron, Keen, Brain

  int key = info->BossDeathKey;
  if (key == 0)
    return; // no action taken

  int b = 0;

  // cyborgs, spiders and ironliches have two different ways of acting
  switch (mo->type)
    {
    case MT_BRUISER:
      b = 1; break;
    case MT_CYBORG:
      b = 2+4; break;
    case MT_SPIDER:
      b = 8+16; break;
    case MT_FATSO:
      b = 32; break;
    case MT_BABY:
      b = 64; break;
    case MT_KEEN:
      b = 128; break;
    case MT_BOSSBRAIN:
      b = 256; break;
    case MT_HHEAD:
      b = 0x200+0x400; break;
    case MT_MINOTAUR:
      b = 0x800; break;
    case MT_SORCERER2:
      b = 0x1000; break;
    default:
      return;
    }

  if ((key & b) == 0)
    // wrong boss type for this level
    return;
 
  int i, n = players.size();

  // make sure there is a player alive for victory
  for (i=0 ; i<n ; i++)
    if ((players[i]->playerstate == PST_INMAP ||
	 players[i]->playerstate == PST_FINISHEDMAP) &&
	players[i]->pawn && !(players[i]->pawn->flags & MF_CORPSE))
      break;

  if (i == n)
    return; // no one left alive, so do not end game


  Thinker *th;
  DActor   *a;
  const state_t *finalst = P_FinalState(mo->info->deathstate);

  // scan the remaining thinkers to see
  // if all bosses are dead
  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      if (!th->IsOf(DActor::_type))
	continue;

      a = (DActor *)th;
      if (a != mo && a->type == mo->type
	  // && a->health > 0           // the old one (doom original 1.9)
	  // && !(a->flags & MF_CORPSE) // the Heretic one
	  && a->state != finalst)
	// this is better because a thing becomes MF_CORPSE while still falling down.
	// We want to see the deaths completely.
        {
	  // other boss not dead
	  return;
        }
    }

  // victory!

  switch (mo->type)
    {
    case MT_BRUISER:
      EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
      return;

    case MT_CYBORG:
      if ((key & 2) != 0)
	break;
      else
	{
	  // used in ult. Doom, map 6
	  EV_DoDoor(666, NULL, NULL, vdoor_t::Open | vdoor_t::Blazing, 4*VDOORSPEED, VDOORWAIT);
	  return;
	}

    case MT_SPIDER:
      if ((key & 8) != 0)
	break;
      else
	{
	  // ult. Doom, map 8
	  EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
	  return;
	}

    case MT_FATSO:
      EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
      return;

    case MT_BABY:
      EV_DoFloor(667, NULL,  floor_t::UpSLT, FLOORSPEED, 0, 0); // correct
      return;

    case MT_KEEN:
      EV_DoDoor(666, NULL, NULL, vdoor_t::Open, VDOORSPEED, VDOORWAIT);
      return;

    case MT_BOSSBRAIN:
      break;

    case MT_HHEAD:
      if ((key & 0x400) == 0)
	goto nomassacre;
    case MT_MINOTAUR:
    case MT_SORCERER2:
      // if (gameepisode > 1)
      // Kill any remaining monsters
      Massacre();
    nomassacre:
      EV_DoFloor(666, NULL, floor_t::LnF, -FLOORSPEED, 0, 0);
      return;

    default:
      // no action taken
      return;
    }

  ExitMap(NULL, 0); // to the next level
}


// try respawning items from the itemrespawnqueue
void Map::RespawnSpecials()
{
  // nothing left to respawn?
  if (itemrespawnqueue.empty())
    return;

  // the first item in the queue is the first to respawn
  if (maptic - itemrespawntime.front() < tic_t(cv_itemrespawntime.value)*TICRATE)
    return;

  mapthing_t *mthing = itemrespawnqueue.front();
  if (mthing != NULL)
    SpawnMapThing(mthing, false);

  // pull it from the queue anyway
  itemrespawnqueue.pop_front();
  itemrespawntime.pop_front();
}


// used when we are going from deathmatch 2 to deathmatch 1
// picks out all weapons from itemrespawnqueue and respawns them
void Map::RespawnWeapons()
{
  deque<mapthing_t *>::iterator i = itemrespawnqueue.begin();

  for ( ; i != itemrespawnqueue.end(); i++)
    {
      mapthing_t *mthing = *i;
      if (!mthing)
	continue;

      switch (mthing->type)
	{
	case MT_SHAINSAW:
	case MT_SHOTGUN:
	case MT_SUPERSHOTGUN:
	case MT_CHAINGUN:
	case MT_ROCKETLAUNCH:
	case MT_PLASMAGUN:
	case MT_BFG9000:
	case MT_WGAUNTLETS:
	case MT_WCROSSBOW:
	case MT_WBLASTER:
	case MT_WPHOENIXROD:
	case MT_WSKULLROD:
	case MT_WMACE:
	case MT_FW_AXE:
	case MT_CW_SERPSTAFF:
	case MT_MW_CONE:
	case MT_FW_HAMMER:
	case MT_CW_FLAME:
	case MT_MW_LIGHTNING:
	case MT_FW_SWORD1:
	case MT_FW_SWORD2:
	case MT_FW_SWORD3:
	case MT_CW_HOLY1:
	case MT_CW_HOLY2:
	case MT_CW_HOLY3:
	case MT_MW_STAFF1:
	case MT_MW_STAFF2:
	case MT_MW_STAFF3:
	  // it's a weapon, remove it from queue and respawn it!
	  *i = NULL; // stays in sync with itemrespawntime...
	  SpawnMapThing(mthing, false);
	  break;

	default:
	  // not a weapon, continue search
	  break;
	}
    }
}


// Called when a Map exit method is activated, such as:
// exit teleporter, endgame teleporter, E1M8 sectordamage, bossdeath, timelimit
void Map::ExitMap(Actor *activator, int next, int ep)
{
  //CONS_Printf("ExitMap => %d, %d\n", next, ep);

  if (!game.server || !cv_exitmode.value)
    return; // exit not allowed

  if (next == 0)
    next = info->nextlevel; // zero means "normal exit"

  if (next == 100)
    next = info->secretlevel; // 100 means "secret exit"

  // HACK...
  PlayerInfo *quitter = (activator && activator->IsOf(PlayerPawn::_type)) ?
    ((PlayerPawn *)activator)->player : NULL;

  int mode = quitter ? cv_exitmode.value : 1; // timed exit, bossdeath => everyone leaves at once.

  // laitetaan joko m_fin tai p_leavmap, aina reqmap

  switch (mode)
    {
    case 1: // first: all players exit the map when someone activates an exit
      {
	int n = players.size();
	for (int i = 0; i<n; i++)
	  players[i]->ExitLevel(next, ep); // save the pawn if needed
      }
      break;

    case 2: // ind: players may exit individually
      // the new maps are launched immediately, the old map keeps running until it is empty.
      quitter->ExitLevel(next, ep);
      break;

    case 3: // last: all players need to reach the exit, others have to wait for the last one
      quitter->playerstate = PST_FINISHEDMAP;
      quitter->requestmap = next;
      quitter->entrypoint = ep;

    default:
      break;
    }
}


//==========================================================================
// TID system (Hexen)
//==========================================================================


void Map::InsertIntoTIDmap(Actor *p, int tid)
{
  if (TIDmap.size() >= 300)
    I_Error("Map::InsertIntoTIDmap: MAX_TID_COUNT (%d) exceeded.", 300);

  // TODO multiple inserts possible
  TIDmap.insert(pair<const short, Actor*>(tid, p));
}

void Map::RemoveFromTIDmap(Actor *p)
{
  if (p->tid == 0)
    return;

  int tid = p->tid;
  p->tid = 0;

  multimap<short, Actor*>::iterator i, j;
  i = TIDmap.lower_bound(tid);
  if (i == TIDmap.end())
    return; // not found (early out)

  j = TIDmap.upper_bound(tid);

  for ( ; i != j; ++i)
    if (i->second == p)
      {
	TIDmap.erase(i);
	return;
      }
  // not found
}


Actor *Map::FindFromTIDmap(int tid, int *pos)
{
  multimap<short, Actor*>::iterator i, j;
  i = TIDmap.lower_bound(tid);
  j = TIDmap.upper_bound(tid);

  ++(*pos); // this is how many entries we must pass

  for (int k = 0; k < *pos; k++)
    {
      if (i == j)
	{
	  // ran out of entries
	  *pos = -1;
	  return NULL;
	}
      i++; // pass it
    }

  // do we have anything left?
  if (i == j)
    {
      // not found
      *pos = -1; // this is needed too (TODO damn old code)
      return NULL;
    }

  return i->second;
}
