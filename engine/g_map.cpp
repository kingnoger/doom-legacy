// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
// $Id$
//
// Copyright (C) 1998-2003 by DooM Legacy Team.
//
// $Log$
// Revision 1.22  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.21  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.20  2003/06/10 22:39:54  smite-meister
// Bugfixes
//
// Revision 1.19  2003/05/30 13:34:43  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.18  2003/05/11 21:23:49  smite-meister
// Hexen fixes
//
// Revision 1.17  2003/05/05 00:24:48  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.16  2003/04/26 12:01:12  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.15  2003/04/19 17:38:46  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.14  2003/04/14 08:58:25  smite-meister
// Hexen maps load.
//
// Revision 1.13  2003/04/08 09:46:05  smite-meister
// Bugfixes
//
// Revision 1.12  2003/04/04 00:01:53  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.11  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.10  2003/03/15 20:07:14  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.9  2003/03/08 16:07:00  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.8  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.7  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.6  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.5  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
//
// DESCRIPTION:
//   Map class implementation
//-----------------------------------------------------------------------------

#include "doomdata.h"
#include "g_map.h"
#include "g_game.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_spec.h"
#include "p_info.h"
#include "command.h"

#include "r_main.h"
#include "hu_stuff.h"
#include "p_camera.h"
#include "m_random.h"
#include "s_sound.h"
#include "sounds.h"
#include "z_zone.h"

extern consvar_t cv_deathmatch;

consvar_t cv_itemrespawntime={"respawnitemtime","30",CV_NETVAR,CV_Unsigned};
consvar_t cv_itemrespawn    ={"respawnitem"    , "0",CV_NETVAR,CV_OnOff};
// TODO combine these to cv_itemrespawn CV_Unsigned, zero means no respawn, otherwise it is the respawntime

#define FLOATRANDZ      (MAXINT-1)

// Map class constructor
Map::Map(const string & mname)
{
  mapname = mname;
  level = NULL;
  info = NULL;
  levelscript = NULL;
  runningscripts = NULL;

  hexen_format = false;
  ActiveAmbientSeq = NULL;
};

// destructor
Map::~Map()
{
  // not much is needed because most memory is freed
  // in Z_FreeTags before a new level is started.
}


//
// GAME SPAWN FUNCTIONS
//


void Map::SpawnActor(Actor *p)
{
  AddThinker(p);     // AddThinker sets Map *mp
  p->CheckPosition(p->x, p->y); // TEST, sets tmfloorz, tmceilingz
  p->SetPosition();  // set subsector and/or block links
}

void Map::DetachActor(Actor *p)
{
  void P_DelSeclist(msecnode_t *p);

  p->UnsetPosition();

  if (p->touching_sectorlist)
    {
      P_DelSeclist(p->touching_sectorlist);
      p->touching_sectorlist = NULL;
    }

  // save the presentation too
  if (p->pres)
    Z_ChangeTag(p->pres, PU_STATIC);

  DetachThinker(p);
}


// was P_SpawnSplash
//
// when player moves in water
// SoM: Passing the Z height saves extra calculations...
void Map::SpawnSplash(Actor *mo, fixed_t z)
{
  // need to touch the surface because the splashes only appear at surface
  if (mo->z > z || mo->z + mo->height < z)
    return;

  // note pos +1 +1 so it doesn't eat the sound of the player..
  DActor *th = SpawnDActor(mo->x+1, mo->y+1, z, MT_SPLASH);
  //if( z - mo->subsector->sector->floorheight > 4*FRACUNIT)
  S_StartSound(th, sfx_gloop);
  //else
  //    S_StartSound (th,sfx_splash);
  th->tics -= P_Random() & 3;

  if (th->tics < 1)
    th->tics = 1;

  // get rough idea of speed
  /*
    thrust = (mo->px + mo->py) >> FRACBITS+1;

    if (thrust >= 2 && thrust<=3)
    th->SetState(S_SPLASH2);
    else
    if (thrust < 2)
    th->SetState(S_SPLASH3);
  */
}


// ---------------------------------------
// Blood spawning
// ---------------------------------------

static Actor   *bloodthing;

#ifdef WALLSPLATS
static fixed_t  bloodspawnpointx, bloodspawnpointy;

bool PTR_BloodTraverse (intercept_t *in)
{
  line_t *li;
  divline_t   divl;
  fixed_t     frac;

  fixed_t     z;

  if (in->isaline)
    {
      li = in->d.line;

      z = bloodthing->z + (P_SignedRandom()<<(FRACBITS-3));
      if ( !(li->flags & ML_TWOSIDED) )
	goto hitline;

      P_LineOpening (li);

      // hit lower texture ?
      if (li->frontsector->floorheight != li->backsector->floorheight)
        {
	  if( openbottom>z )
	    goto hitline;
        }

      // hit upper texture ?
      if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
	  if( opentop<z )
	    goto hitline;
        }

      // else don't hit
      return true;

    hitline:
      P_MakeDivline (li, &divl);
      frac = P_InterceptVector (&divl, &trace);
      if( game.mode == gm_heretic )
	R_AddWallSplat (li, P_PointOnLineSide(bloodspawnpointx,bloodspawnpointy,li),"BLODC0", z, frac, SPLATDRAWMODE_TRANS);
      else
	R_AddWallSplat (li, P_PointOnLineSide(bloodspawnpointx,bloodspawnpointy,li),"BLUDC0", z, frac, SPLATDRAWMODE_TRANS);
      return false;
    }

  //continue
  return true;
}
#endif

// was P_SpawnBloodSplats
// the new SpawnBlood : this one first calls P_SpawnBlood for the usual blood sprites
// then spawns blood splats around on walls
//
void Map::SpawnBloodSplats(fixed_t x, fixed_t y, fixed_t z, int damage, fixed_t px, fixed_t py)
{
  // spawn the usual falling blood sprites at location
  bloodthing = SpawnBlood(x,y,z,damage);

#ifdef WALLSPLATS
  fixed_t x2,y2;
  angle_t angle, anglesplat;
  int     distance;
  angle_t anglemul=1;  
  int     numsplats;
  int     i;

  // traverse all linedefs and mobjs from the blockmap containing t1,
  // to the blockmap containing the dest. point.
  // Call the function for each mobj/line on the way,
  // starting with the mobj/linedef at the shortest distance...

  if(!px && !py)
    {   
      // from inside
      angle=0;
      anglemul=2; 
    }
  else
    {
      // get direction of damage
      x2 = x + px;
      y2 = y + py;
      angle = R_PointToAngle2 (x,y,x2,y2);
    }
  distance = damage * 6;
  numsplats = damage / 3+1;
  // BFG is funy without this check
  if (numsplats > 20)
    numsplats = 20;

  //CONS_Printf ("spawning %d bloodsplats at distance of %d\n", numsplats, distance);
  //CONS_Printf ("damage %d\n", damage);
  bloodspawnpointx = x;
  bloodspawnpointy = y;
  //uses 'bloodthing' set by P_SpawnBlood()
  for (i=0; i<numsplats; i++)
    {
      // find random angle between 0-180deg centered on damage angle
      anglesplat = angle + (((P_Random() - 128) * FINEANGLES/512*anglemul)<<ANGLETOFINESHIFT);
      x2 = x + distance*finecosine[anglesplat>>ANGLETOFINESHIFT];
      y2 = y + distance*finesine[anglesplat>>ANGLETOFINESHIFT];
      
      P_PathTraverse(x, y, x2, y2, PT_ADDLINES, PTR_BloodTraverse);
  }
#endif

#ifdef FLOORSPLATS
  // add a test floor splat
  R_AddFloorSplat(bloodthing->subsector, "STEP2", x, y, bloodthing->floorz, SPLATDRAWMODE_SHADE);
#endif
}

// was P_SpawnBlood
// spawn a blood sprite with falling z movement, at location
// the duration and first sprite frame depends on the damage level
// the more damage, the longer is the sprite animation
DActor *Map::SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage)
{
  z += P_SignedRandom() << 10;
  DActor *th = SpawnDActor(x,y,z, MT_BLOOD);
  if (game.demoversion >= 128)
    {
      th->px  = P_SignedRandom()<<12; //faB:19jan99
      th->py  = P_SignedRandom()<<12; //faB:19jan99
    }
  th->pz = FRACUNIT*2;
  th->tics -= P_Random()&3;

  if (th->tics < 1)
    th->tics = 1;

  if (damage <= 12 && damage >= 9)
    th->SetState(S_BLOOD2);
  else if (damage < 9)
    th->SetState(S_BLOOD3);

  return th;
}


// ---------------------------------------
// was P_SpawnSmoke
// when player gets hurt by lava/slime, spawn at feet

void Map::SpawnSmoke(fixed_t x, fixed_t y, fixed_t z)
{
  x = x - ((P_Random()&8) * FRACUNIT) - 4*FRACUNIT;
  y = y - ((P_Random()&8) * FRACUNIT) - 4*FRACUNIT;
  z += (P_Random()&3) * FRACUNIT;

  DActor *th = SpawnDActor(x,y,z, MT_SMOK);
  th->pz = FRACUNIT;
  th->tics -= P_Random() & 3;

  if (th->tics < 1)
    th->tics = 1;
}


// adds a DActor to a Map
DActor *Map::SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t)
{
  DActor *p = new DActor(nx, ny, nz, t);
  AddThinker(p);

  //CONS_Printf("Spawn, type: %d\n", t);
  p->CheckPosition(nx, ny); // TEST, sets tmfloorz, tmceilingz
  // set subsector and/or block links
  p->SetPosition();

  if (nz == ONFLOORZ)
    {
      //if (!P_CheckPosition(mobj,x,y))
      // we could send a message to the console here, saying
      // "no place for spawned thing"...

      //added:28-02-98: defaults onground
      p->eflags |= MFE_ONGROUND;

      //added:28-02-98: dirty hack : dont stack monsters coz it blocks
      //                moving floors and anyway whats the use of it?
      /*if (flags & MF_NOBLOOD)
        {
	z = floorz;
	
	// first check the tmfloorz
	P_CheckPosition(mobj,x,y);
	z = tmfloorz+FRACUNIT;

	// second check at the good z pos
	P_CheckPosition(mobj,x,y);

	floorz = tmfloorz;
	ceilingz = tmsectorceilingz;
	z = tmfloorz;
	// thing not on solid ground
	if (tmfloorthing)
	eflags &= ~MFE_ONGROUND;

	//if (type == MT_BARREL)
	//   fprintf(stderr,"barrel at z %d floor %d ceiling %d\n",z,floorz,ceilingz);
        }
        else*/
      p->z = p->floorz;
    }
  else if (nz == ONCEILINGZ)
    p->z = p->ceilingz - p->info->height;
  else if (nz == FLOATRANDZ)
    {
      fixed_t space = p->ceilingz - p->info->height - p->floorz;
      if (space > 48*FRACUNIT)
        {
	  space -= 40*FRACUNIT;
	  p->z = ((space*P_Random()) >> 8) + p->floorz + 40*FRACUNIT;
        }
      else
	p->z = p->floorz;
    }
  else
    p->z = nz;

  if ((p->flags2 & MF2_FOOTCLIP) && (p->subsector->sector->floortype >= FLOOR_LIQUID)
      && (p->floorz == p->subsector->sector->floorheight))
    p->floorclip = FOOTCLIPSIZE;
  else
    p->floorclip = 0;

  return p;
}


extern byte weapontobutton[NUMWEAPONS];


void SV_SpawnPlayer(int playernum, int x, int y, angle_t angle);
// was P_SpawnPlayer
// was G_PlayerReborn
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
void Map::SpawnPlayer(PlayerInfo *pi, mapthing_t *mthing)
{
  fixed_t     nx, ny, nz;

  nx = mthing->x << FRACBITS;
  ny = mthing->y << FRACBITS;
  nz = ONFLOORZ;

  PlayerPawn *p;
  CONS_Printf("SpawnPlayer: pawn == %p\n", pi->pawn);

  // the player may have his old pawn from the previous level
  if (!pi->pawn)
    {
      const float AutoArmorSave[] = { 0.0, 0.15, 0.10, 0.05, 0.0 };

      p = new PlayerPawn(nx, ny, nz, pi->pawntype);
      p->player = pi;
      pi->pawn  = p;
      p->pclass = pi->pclass;
      p->toughness = AutoArmorSave[p->pclass];
      CONS_Printf("-- new pawn, health == %d\n", p->health);
    }
  else
    {
      p = pi->pawn;
      p->x = nx;
      p->y = ny;
      p->z = nz;
      p->px = p->py = p->pz = 0;
      CONS_Printf("--- old pawn, health == %d\n", p->health);
    }

  AddThinker(p); // AddThinker sets Map *mp
  // set subsector and/or block links
  p->CheckPosition(p->x, p->y); // TEST, sets tmfloorz, tmceilingz
  p->SetPosition();

  // Boris stuff
  if (!pi->originalweaponswitch)
    p->UseFavoriteWeapon();

  p->eflags |= MFE_ONGROUND;
  p->z = p->floorz;

  mthing->mobj = p;

  // FIXME set skin sprite here
  // set color translations for player sprites
  p->color = pi->color;

  p->angle = ANG45 * (mthing->angle/45);
  if (pi == consoleplayer)
    localangle = p->angle;
  else if (pi == displayplayer2)
    localangle2 = p->angle;

  pi->viewheight = cv_viewheight.value<<FRACBITS;
  pi->viewz = p->z + pi->viewheight;

  pi->playerstate = PST_LIVE;

  // setup gun psprite
  p->SetupPsprites();

  // give all cards in death match mode
  if (cv_deathmatch.value)
    p->keycards = it_allkeys;

  if (pi == consoleplayer)
    {
      // wake up the status bar
      hud.ST_Start(p);
    }

  // FIXME what does it do?
  SV_SpawnPlayer(pi->number, p->x, p->y, p->angle);

  if (camera.chase && displayplayer == pi)
    camera.ResetCamera(p);
  CONS_Printf("spawn done\n");
}


//
// was P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void Map::SpawnMapThing(mapthing_t *mt)
{
  int t = mt->type;

  // don't spawn keycards and players in deathmatch
  if (cv_deathmatch.value && mobjinfo[t].flags & MF_NOTDMATCH)
    return;

  // don't spawn any monsters if -nomonsters
  if (game.nomonsters && (t == MT_SKULL || (mobjinfo[t].flags & MF_COUNTKILL)))
    return;

  fixed_t nx, ny, nz;
  // spawn it
  nx = mt->x << FRACBITS;
  ny = mt->y << FRACBITS;

  if (mobjinfo[t].flags & MF_SPAWNCEILING)
    nz = ONCEILINGZ;
  else if (mobjinfo[t].flags2 & MF2_SPAWNFLOAT)
    nz = FLOATRANDZ;
  else
    {
      // FIXME think how the z spawning is supposed to work... really.
      mt->z += R_PointInSubsector(nx, ny)->sector->floorheight >> FRACBITS;
      //nz = ONFLOORZ;
      nz = mt->z << FRACBITS;
    }

  DActor *p = SpawnDActor(nx,ny,nz, mobjtype_t(t));
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
  if(p->flags2 & MF2_FLOATBOB)
    p->health = P_Random();

  if (p->tics > 0)
    p->tics = 1 + (P_Random () % p->tics);

  if (p->flags & MF_COUNTKILL)
    kills++;
  if (p->flags & MF_COUNTITEM)
    items++;

  p->angle = ANG45 * (mt->angle/45);
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
}


//
// was G_CheckSpot
// Returns false if the player p cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
bool Map::CheckRespawnSpot(PlayerInfo *p, mapthing_t *mthing)
{
  extern consvar_t cv_teamplay;

  if (mthing == NULL)
    return false;

  // has the spawn spot been used just recently?
  // (less stupid telefrag this way!)
  // damn short int! it's just 16 bits long! and signed too!
  if ((maptic & 0xFFFF) < (unsigned short)mthing->type)
    return false;

  fixed_t x, y;
  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;
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
  //if (!P_CheckPosition (p.mo, x, y)) return false;

  return true;
}


//
// was G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
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
	// set the timer
	dmstarts[j]->type = (short)((maptic + 20) & 0xFFFF);
	SpawnPlayer(p, dmstarts[j]);
	return true;
      }
    j++;
    if (j == n)
      j = 0;
  } while (j != i);

  return false;
}


// was G_CoopSpawnPlayer
bool Map::CoopRespawn(PlayerInfo *p)
{
  CONS_Printf("CoopRespawn, p = %p, pnum = %d\n", p, p->number - 1);

  int n = playerstarts.size();
  int i = p->number - 1;
  if (i < n)
    {
      // let's check his own start first
      if (CheckRespawnSpot(p, playerstarts[i]))
	{
	  // set the timer
	  playerstarts[i]->type = (short)((maptic + 20) & 0xFFFF);
	  SpawnPlayer(p, playerstarts[i]);
	  return true;
	}
    }

  // try to spawn at one of the other players' spots
  for (i=0 ; i<n ; i++)
    {
      if (CheckRespawnSpot(p, playerstarts[i]))
	{
	  // set the timer
	  playerstarts[i]->type = (short)((maptic + 20) & 0xFFFF);
	  SpawnPlayer(p, playerstarts[i]);
	  return true;
	}
    }

  return false;
}


// was G_DoReborn
int Map::RespawnPlayers()
{
  // Let's try to empty the respawnqueue!
  // players in respawnqueue may or may not have pawns

  int count = 0;
  PlayerInfo *p;

  // TODO make me better sometime
  //deque<PlayerInfo *>::iterator i;

  bool ok;
  do {
    CONS_Printf("RespawnPlayers: %d, count = %d\n", respawnqueue.size(), count);
    p = respawnqueue.front();
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
  } while (ok && !respawnqueue.empty());

  return count;
}

// called when a dead player pushes USE
void Map::RebornPlayer(PlayerInfo *p)
{
  // at this point p->playerstate should be PST_DEAD
  // and p->pawn not NULL

  // first dissociate the corpse
  if (p->pawn)
    {
      if (p == displayplayer)
	{
	  // shutdown the status bar
	  hud.ST_Stop();
	}

      p->pawn->player = NULL;
      p->pawn->flags2 &= ~MF2_DONTDRAW;

      // flush an old corpse if needed
      if (bodyqueue.size() >= BODYQUESIZE)
	{
	  bodyqueue.front()->Remove();
	  bodyqueue.pop_front();
	}
      bodyqueue.push_back(p->pawn);
      p->pawn = NULL;
    }

  // spawn a teleport fog
  /*
  unsigned an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;

  Actor *mo = SpawnActor(x+20*finecosine[an], y+20*finesine[an]
		    , ss->sector->floorheight
		    , MT_TFOG);
  */
  //if (displayplayer->viewz != 1)
  //  S_StartSound(mo, sfx_telept);  // don't start sound on first frame

  respawnqueue.push_back(p);
  p->playerstate = PST_RESPAWN;
}

// Adds a player to a Map. The player is immediately queued for respawn.
void Map::AddPlayer(PlayerInfo *p)
{
  // At this point the player may or may not have a pawn.
  players.push_back(p); // add p to the Map's playerlist
  respawnqueue.push_back(p);
  p->playerstate = PST_RESPAWN;

  if (p->pawn)
    Z_ChangeTag(p->pawn, PU_LEVSPEC);
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

//----------------------------------------------------------------------------
// was P_Massacre
// Kills all monsters. Except skulls.

int Map::Massacre()
{
  Actor   *mo;
  Thinker *th;
  int count = 0;

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      //if (th->function.acp1 != (actionf_p1)P_MobjThinker)
      if (th->Type() != Thinker::tt_dactor)
	// Not an actor
	continue;
	
      mo = (Actor *)th;
      if ((mo->flags & MF_COUNTKILL) && (mo->health > 0))
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
static state_t *P_FinalState(statenum_t state)
{
  while(states[state].tics!=-1)
    state=states[state].nextstate;

  return &states[state];
}


void Map::BossDeath(const DActor *mo)
{
  extern consvar_t cv_allowexitlevel;

  // FIXME! this is how it is supposed to work:
  // A map knows what it should do when, say, the last monster of type X dies.
  // this information is taken from the LevelNode when the map is initialized.

  // It may be doing something to tags 666 or 667, or maybe even exiting the
  // map (but not necessarily the level!)
  // But how to code it generally and efficiently?
  // Maybe just copy existing Doom/Doom2/Heretic 666 and 667 tricks and do everything else by FS?

  // Ways to end level:
  // Baron of Hell, Cyberdemon, Spider Mastermind,
  // Mancubus, Arachnotron, Keen, Brain

  if (BossDeathKey == 0)
    // no action taken
    return;

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

  if (BossDeathKey & b == 0)
    // wrong boss type for this level
    return;
 
  int      i, n = players.size();

  // make sure there is a player alive for victory
  for (i=0 ; i<n ; i++)
    if (players[i]->playerstate == PST_LIVE)
    // if (players[i]->pawn->health > 0) // crashes if pawn==NULL!
      break;

  if (i == n)
    return; // no one left alive, so do not end game


  Thinker *th;
  DActor   *a;
  line_t   junk;
  const state_t *finalst = P_FinalState(mo->info->deathstate);

  // scan the remaining thinkers to see
  // if all bosses are dead
  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      //if (th->function.acp1 != (actionf_p1)P_MobjThinker)
      if (th->Type() != Thinker::tt_dactor)
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
      junk.tag = 666;
      EV_DoFloor(&junk, floor_t::LnF, FLOORSPEED, 0, 0);
      return;

    case MT_CYBORG:
      if (BossDeathKey & 2 != 0)
	break;
      else
	{
	  // used in ult. Doom, map 6
	  junk.tag = 666;
	  EV_DoDoor(&junk, NULL, vdoor_t::Open | vdoor_t::Blazing, 4*VDOORSPEED, VDOORWAIT);
	  return;
	}

    case MT_SPIDER:
      if (BossDeathKey & 8 != 0)
	break;
      else
	{
	  // ult. Doom, map 8
	  junk.tag = 666;
	  EV_DoFloor (&junk, floor_t::LnF, FLOORSPEED, 0, 0);
	  return;
	}

    case MT_FATSO:
      junk.tag = 666;
      EV_DoFloor(&junk, floor_t::LnF, FLOORSPEED, 0, 0);
      return;

    case MT_BABY:
      junk.tag = 667;
      EV_DoFloor(&junk,  floor_t::SLT, FLOORSPEED, 0, 0);
      return;

    case MT_KEEN:
      junk.tag = 666;
      EV_DoDoor(&junk, NULL, vdoor_t::Open,VDOORSPEED, VDOORWAIT);
      return;

    case MT_BOSSBRAIN:
      break;

    case MT_HHEAD:
      if (BossDeathKey & 0x400 == 0)
	goto nomassacre;
    case MT_MINOTAUR:
    case MT_SORCERER2:
      // if (gameepisode > 1)
      // Kill any remaining monsters
      Massacre();
    nomassacre:
      junk.tag = 666;
      EV_DoFloor(&junk, floor_t::LnF, FLOORSPEED, 0, 0);
      return;

    default:
      // no action taken
      return;
    }

  if (cv_allowexitlevel.value)
    ExitMap(-1);
}

//
// was P_RespawnSpecials
//
void Map::RespawnSpecials()
{
  // nothing left to respawn?
  if (itemrespawnqueue.empty()) //(iquehead == iquetail)
    return;

  // the first item in the queue is the first to respawn
  if (maptic - itemrespawntime.front() < (tic_t)cv_itemrespawntime.value*TICRATE)
    return;

  mapthing_t *mthing = itemrespawnqueue.front();
  if (mthing != NULL)
    {
      // TODO somewhat redundant code, see SpawnMapThing()
      fixed_t x, y, z;

      x = mthing->x << FRACBITS;
      y = mthing->y << FRACBITS;

      DActor *mo;

      // spawn a teleport fog at the new spot
      if (game.mode != gm_heretic)
	{
	  subsector_t *ss = R_PointInSubsector (x,y);
	  mo = SpawnDActor(x, y, ss->sector->floorheight, MT_IFOG);
	  S_StartSound (mo, Actor::s_respawn);
	}

      int t = mthing->type;

      // spawn it
      if (mobjinfo[t].flags & MF_SPAWNCEILING)
	z = ONCEILINGZ;
      else
	z = ONFLOORZ;

      mo = SpawnDActor(x,y,z, mobjtype_t(t));
      mo->spawnpoint = mthing;
      mo->angle = ANG45 * (mthing->angle/45);

      if (game.mode == gm_heretic)
	S_StartSound (mo, Actor::s_respawn);
    }
  // pull it from the queue anyway
  //iquetail = (iquetail+1)&(ITEMQUESIZE-1);
  itemrespawnqueue.pop_front();
  itemrespawntime.pop_front();
}

// was P_RespawnWeapons
// used when we are going from deathmatch 2 to deathmatch 1
// picks out all weapons from itemrespawnqueue and respawns them
void Map::RespawnWeapons()
{
  fixed_t x, y, z;

  deque<mapthing_t *>::iterator i = itemrespawnqueue.begin();
  deque<tic_t>::iterator j = itemrespawntime.begin();

  for( ; i != itemrespawnqueue.end() ; i++, j++)
    {
      mapthing_t *mthing = *i;

      int n = 0;
      switch(mthing->type)
	{
	case 2001 : //mobjinfo[MT_SHOTGUN].doomednum  :
	  n=MT_SHOTGUN;
	  break;
	case 82   : //mobjinfo[MT_SUPERSHOTGUN].doomednum :
	  n=MT_SUPERSHOTGUN;
	  break;
	case 2002 : //mobjinfo[MT_CHAINGUN].doomednum :
	  n=MT_CHAINGUN;
	  break;
	case 2006 : //mobjinfo[MT_BFG9000].doomednum   : // bfg9000
	  n=MT_BFG9000;
	  break;
	case 2004 : //mobjinfo[MT_PLASMAGUN].doomednum   : // plasma launcher
	  n=MT_PLASMAGUN;
	  break;
	case 2003 : //mobjinfo[MT_ROCKETLAUNCH].doomednum   : // rocket launcher
	  n=MT_ROCKETLAUNCH;
	  break;
	case 2005 : //mobjinfo[MT_SHAINSAW].doomednum   : // shainsaw
	  n=MT_SHAINSAW;
	  break;
	default:
	  // not a weapon, continue search
	  continue;
	}
      // it's a weapon, remove it from queue!
      *i = NULL;

      // and respawn it
      x = mthing->x << FRACBITS;
      y = mthing->y << FRACBITS;

      // spawn a teleport fog at the new spot
      subsector_t *ss = R_PointInSubsector(x,y);
      DActor *mo = SpawnDActor(x, y, ss->sector->floorheight, MT_IFOG);
      S_StartSound(mo, Actor::s_respawn);

      // spawn it
      if (mobjinfo[n].flags & MF_SPAWNCEILING)
	z = ONCEILINGZ;
      else
	z = ONFLOORZ;

      mo = SpawnDActor(x,y,z, mobjtype_t(n));
      mo->spawnpoint = mthing;
      mo->angle = ANG45 * (mthing->angle/45);
    }
}

void Map::ExitMap(int exit, unsigned entrypoint)
{
  // TODO maybe in future you can exit and load maps asynchronously
  // (level consists of 2 maps, one is exited and replaced with a new one,
  // the other keeps running)
  game.ExitLevel(exit, entrypoint);
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
    if ((*i).second == p)
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
  if (i == j)
    {
      // not found
      *pos = -1; // this is needed too (TODO damn old code)
      return NULL;
    }

  ++(*pos);
  for (int k = 0; k < *pos; ++i, ++k)
    if (i == j)
      {
	// not found
	*pos = -1;
	return NULL;
      }

  return (*i).second;
}
