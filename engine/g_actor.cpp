// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.23  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.22  2003/11/27 11:28:25  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.21  2003/11/23 00:41:54  smite-meister
// bugfixes
//
// Revision 1.20  2003/11/12 11:07:16  smite-meister
// Serialization done. Map progression.
//
// Revision 1.19  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.18  2003/06/10 22:39:53  smite-meister
// Bugfixes
//
// Revision 1.17  2003/06/01 18:56:29  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.16  2003/05/11 21:23:49  smite-meister
// Hexen fixes
//
// Revision 1.15  2003/05/05 00:24:48  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.14  2003/04/26 12:01:12  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.13  2003/04/19 17:38:46  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.12  2003/04/14 08:58:24  smite-meister
// Hexen maps load.
//
// Revision 1.11  2003/04/04 00:01:53  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.10  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.9  2003/03/15 20:07:13  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.8  2003/03/08 16:06:59  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.7  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.6  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.5  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.4  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/29 18:57:02  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/16 22:10:59  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:05  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Actor class implementation.
//
//-----------------------------------------------------------------------------

#include "g_actor.h"

#include "doomdef.h"
#include "doomdata.h"

#include "g_game.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_input.h"
#include "p_enemy.h" // #defines

#include "hu_stuff.h"
#include "p_spec.h"
#include "p_maputl.h"

#include "p_setup.h"    //levelflats to test if mobj in water sector
#include "r_main.h"

#include "r_sprite.h"

#include "r_things.h"
#include "s_sound.h"
#include "sounds.h"

#include "m_random.h"
#include "d_clisrv.h"


#define VIEWHEIGHT  41
#define MAXMOVE     (30*FRACUNIT/NEWTICRATERATIO)

CV_PossibleValue_t viewheight_cons_t[]={{16,"MIN"},{56,"MAX"},{0,NULL}};

consvar_t cv_viewheight = {"viewheight", "41",0,viewheight_cons_t,NULL};

consvar_t cv_gravity = {"gravity","1",CV_NETVAR|CV_FLOAT|CV_SHOWMODIF};
consvar_t cv_splats  = {"splats","1",CV_SAVE,CV_OnOff};

consvar_t cv_respawnmonsters = {"respawnmonsters","0",CV_NETVAR,CV_OnOff};
consvar_t cv_respawnmonsterstime = {"respawnmonsterstime","12",CV_NETVAR,CV_Unsigned};


extern fixed_t FloatBobOffsets[64];

IMPLEMENT_CLASS(Actor,"Actor");
IMPLEMENT_CLASS(DActor,"DActor");

int Actor::s_pickup    = sfx_None;
int Actor::s_keypickup = sfx_None;
int Actor::s_weaponpickup = sfx_None;
int Actor::s_artipickup = sfx_None;
int Actor::s_powerup  = sfx_None;
int Actor::s_teleport = sfx_None;
int Actor::s_respawn  = sfx_None;
int Actor::s_gibbed   = sfx_None;


//----------------------------------------------
// trick constructors

Actor::Actor()
{
  mp = NULL;

  pres = NULL;
  snext = sprev = bnext = bprev = NULL;
  subsector = NULL;

  floorz = ceilingz = 0;
  touching_sectorlist = NULL;
  spawnpoint = NULL;

  x = y = z = 0;
  angle = aiming = 0;
  px = py = pz = 0;

  flags = flags2 = eflags = 0;

  special = tid = 0;
  args[0] = args[1] = args[2] = args[3] = args[4] = 0;

  owner = target = NULL;

  reactiontime = 0;
  floorclip = 0;
}

DActor::DActor()
  : Actor()
{
  type = MT_NONE;
  info = NULL;
  state = NULL;
  tics = movedir = movecount = threshold = 0;
  lastlook = -1;
  special1 = special2 = 0;
}

DActor::DActor(mobjtype_t t)
  : Actor()
{
  type = t;
}

//----------------------------------------------
//  Normal constructors

Actor::Actor(fixed_t nx, fixed_t ny, fixed_t nz)
  : Thinker()
{
  // note! here Map *mp is not yet set! This means you can't call functions such as
  // SetPosition that have something to do with a map.
  pres = NULL;
  snext = sprev = bnext = bprev = NULL;
  subsector = NULL;
  floorz = ceilingz = 0;
  touching_sectorlist = NULL;
  spawnpoint = NULL;

  x = nx;
  y = ny;
  z = nz;
  angle = aiming = 0;
  px = py = pz = 0;

  // some attributes left uninitialized here
  flags = flags2 = eflags = 0;

  special = tid = 0;
  args[0] = args[1] = args[2] = args[3] = args[4] = 0;

  owner = target = NULL;

  reactiontime = 0;
  floorclip = 0;
}


DActor::DActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t)
  : Actor(nx, ny, nz)
{
  type = t;
  info = &mobjinfo[t];

  mass = info->mass;
  radius = info->radius;
  height = info->height;
  health = info->spawnhealth;

  flags  = info->flags;
  flags2 = info->flags2;
  eflags = 0;

  if (game.skill != sk_nightmare)
    reactiontime = info->reactiontime;
  else
    reactiontime = 0;

  movedir = movecount = threshold = 0;
  lastlook = -1;
  special1 = special2 = 0;

  // do not set the state with SetState,
  // because action routines can not be called yet
  state = &states[info->spawnstate];
  tics = state->tics;

  /*
  if (t == MT_SHOTGUY)
    pres = new modelpres_t("models/sarge/");
  else
  */
    pres = new spritepres_t(sprnames[state->sprite], info, 0);
}


// NULLs pointers to objects that will be deleted soon
void Actor::CheckPointers()
{
  if (owner && (owner->eflags & MFE_REMOVE))
    owner = NULL;

  if (target && (target->eflags & MFE_REMOVE))
    target = NULL;
}


// detaches the Actor from the Map
void Actor::Detach()
{
  void P_DelSeclist(msecnode_t *p);

  UnsetPosition();

  if (touching_sectorlist)
    {
      P_DelSeclist(touching_sectorlist);
      touching_sectorlist = NULL;
    }

  eflags |= MFE_REMOVE; // so that pointers to it will be NULLed

  // save the presentation too
  if (pres)
    Z_ChangeTag(pres, PU_STATIC);

  mp->DetachThinker(this);
}



//----------------------------------------------

void Actor::Remove()
{
  // Lazy deallocation: Memory freed only after pointers to this object
  // have been NULLed in Map::RunThinkers()

  if (mp == NULL)
    {
      // in transit between maps/levels
      delete this;
      return;
    }

  if (eflags & MFE_REMOVE)
    return; // already marked for removal

  eflags |= MFE_REMOVE;

  extern consvar_t cv_itemrespawn;

  if ((flags & MF_SPECIAL) && !(flags & MF_DROPPED) &&
      !(flags & MF_NORESPAWN) && cv_itemrespawn.value)
    {
      mp->itemrespawnqueue.push_back(spawnpoint);
      mp->itemrespawntime.push_back(mp->maptic);
    }

  if (tid)
    mp->RemoveFromTIDmap(this);

  // unlink from sector and block lists
  UnsetPosition();

  if (touching_sectorlist)
    {
      P_DelSeclist(touching_sectorlist);
      touching_sectorlist = NULL;
    }

  // stop any playing sound
  S.Stop3DSound(this);

  // remove it from active Thinker list, add it to the removal list
  mp->RemoveThinker(this);
}



//----------------------------------------
// PlayerLandedOnThing, helper function
//
void P_NoiseAlert(Actor *target, Actor *emitter);

static void PlayerLandedOnThing(PlayerPawn *p, Actor *onmobj)
{
  p->player->deltaviewheight = p->pz >> 3;
  if (p->pz < -23*FRACUNIT)
    {
      //P_FallingDamage(mo->player);
      P_NoiseAlert(p, p);
    }
  else if (p->pz < -8*FRACUNIT && !p->morphTics)
    {
      S_StartSound(p, sfx_oof);
    }
}


void BlasterMissileThink();

//---------------------------------
// Doom mobj_t statechange etc.
void DActor::Think()
{
  if (type == MT_BLASTERFX1)
    {
      BlasterMissileThink();
      return;
    }

  if (type == MT_MWAND_MISSILE || type == MT_CFLAME_MISSILE)
    {
      XBlasterMissileThink();
      return;
    }

  /*
    - is it a ppawn
    - checkwater
    - if pxy or MF_SKULLFLY, xymovement
    - floatbob
    - z movement, 3 codes, MF_ONGROUND, MF2_PASSMOBJ, MF2_ONMOBJ
    - state update
    - nightmare respawn
   */
  int oldflags = flags;
  int oldeflags = eflags;
  
  Actor::Think();

  // must have hit something
  if ((oldeflags & MFE_SKULLFLY) && !(eflags & MFE_SKULLFLY))
    SetState(game.mode == gm_heretic ? info->seestate : info->spawnstate);
  
  // must have exploded
  if ((oldflags & MF_MISSILE) && !(flags & MF_MISSILE))
    ExplodeMissile();

  // missiles hitting the floor
  if ((flags & MF_MISSILE) && (eflags & MFE_JUSTHITFLOOR))
    {
      if (flags2 & MF2_FLOORBOUNCE)
	{
	  FloorBounceMissile();
	}
      else if (type == MT_MNTRFX2)
	{ // Minotaur floor fire can go up steps
	}
      else if (!(flags & MF_NOCLIPLINE))
	{
	  ExplodeMissile();
	}
    }

  // crashing to ground
  if (info->crashstate && (flags & MF_CORPSE) && (eflags & MFE_JUSTHITFLOOR))
    {
      SetState(info->crashstate);
      flags &= ~MF_CORPSE;
    }

  // cycle through states,
  // calling action functions at transitions
  if (tics != -1)
    {
      // you can cycle through multiple states in a tic
      if (--tics == 0)
	if (!SetState(state->nextstate))
	  return; // freed itself
    }
  else
    {
      // check for nightmare respawn
      if (!cv_respawnmonsters.value)
	return;

      if (!(flags & MF_COUNTKILL))
	return;

      movecount++;

      if (movecount < cv_respawnmonsterstime.value*TICRATE)
	return;

      if (mp->maptic % (32*NEWTICRATERATIO))
	return;

      if (P_Random() > 4)
	return;

      NightmareRespawn();
    }
}

//----------------------------------------------
// was P_MobjThinker
//
void Actor::Think()
{
  PlayerPawn *p = NULL;
  if (Type() == Thinker::tt_ppawn)
    p = (PlayerPawn *)this;

  // check possible sector water content, set a few eflags
  CheckWater();

  // XY movement
  if (px || py)
    XYMovement();
  else
    {
      if (eflags & MFE_BLASTED)
	{
	  // Reset to not blasted when momentums are gone
	  eflags &= ~MFE_BLASTED;
	  // if (!(flags & MF_ICECORPSE)) TODO ICECORPSE
	    flags2 &= ~MF2_SLIDE;
	}

      if (eflags & MFE_SKULLFLY)
	{
	  // the skull slammed into something
	  eflags &= ~MFE_SKULLFLY;
	  pz = 0;
	}
    }

  // Z movement
  eflags &= ~MFE_JUSTHITFLOOR;

  if (flags2 & MF2_FLOATBOB)
    {
      // Floating item bobbing motion (maybe move to DActor::Think ?)
      z = floorz + FloatBobOffsets[(health++) & 63];
    }
  else if (!(eflags & MFE_ONGROUND) || (z != floorz) || pz)
    {
      // time to fall...
      if (flags2 & MF2_PASSMOBJ)
	{
	  // Heretic z code
	  Actor *onmo = CheckOnmobj();
	  if (!onmo)
	    {
	      ZMovement();
	      flags2 &= ~MF2_ONMOBJ;
	    }
	  else
	    {
	      if (p) // FIXME is this ok? For all Actors?
		{
		  if (pz < -8*FRACUNIT && !(flags2 & MF2_FLY))
		    {
		      PlayerLandedOnThing(p, onmo);
		    }

		  if (onmo->z + onmo->height - z <= 24*FRACUNIT)
		    {
		      p->player->viewheight -= onmo->z + onmo->height - z;
		      p->player->deltaviewheight = 
			(VIEWHEIGHT - p->player->viewheight)>>3;
		      z = onmo->z+onmo->height;
		      flags2 |= MF2_ONMOBJ;
		      pz = 0;
		    }                               
		  else
		    { // hit the bottom of the blocking mobj
		      pz = 0;
		    }
		}
	    }
	}
      else
	ZMovement();
    }
}


// returns the value by which the x,y
// movements are multiplied to add to player movement.
const float normal_friction = 0.90625f; // 0xE800

float Actor::GetMoveFactor()
{
  extern int variable_friction;
  float mf  = 1.0f;

  // less control if not onground.
  bool onground = (z <= floorz) || (flags2 & (MF2_ONMOBJ | MF2_FLY));

  if (boomsupport && variable_friction && onground && !(flags & (MF_NOGRAVITY | MF_NOCLIPLINE)))
    {
      float frict = normal_friction;
      msecnode_t *node = touching_sectorlist;
      sector_t *sec;
      while (node)
	{
	  sec = node->m_sector;
	  if ((sec->special & SS_friction) && (z <= sec->floorheight))
	    if (frict == normal_friction || frict > sec->friction)
	      {
		frict = sec->friction;
		mf = sec->movefactor;
	      }
	  node = node->m_tnext;
	}

      // If the floor is icy or muddy, it's harder to get moving. This is where
      // the different friction factors are applied to 'trying to move'. In
      // Actor::XYFriction, the friction factors are applied as you coast and slow down.

      if (frict < normal_friction) // sludge
	{
          // phares 3/11/98: you start off slowly, then increase as
          // you get better footing
#define MORE_FRICTION_MOMENTUM 15000       // mud factor based on momentum

          fixed_t momentum = P_AproxDistance(px,py);

	  if (momentum < MORE_FRICTION_MOMENTUM)
	    mf *= 0.125;
          else if (momentum < MORE_FRICTION_MOMENTUM<<1)
	    mf *= 0.25; 
          else if (momentum < MORE_FRICTION_MOMENTUM<<2)
	    mf *= 0.5;

	}
    }

  if (eflags & MFE_UNDERWATER)
    {
      // half forward speed when waist under water
      // a little better grip if feet touch the ground
      if (onground)
	mf = 0.5 * (mf + 0.5);
      else
	mf = 0.5; // swimming
    }
  else if (!onground)
    {
      // allow very small movement while in air for playability
      mf = 0.125;
    }

  return mf;
}



//-----------------------------------------

void Actor::XYMovement()
{
  extern line_t *ceilingline;
  extern int skyflatnum;

  if (px > MAXMOVE)
    px = MAXMOVE;
  else if (px < -MAXMOVE)
    px = -MAXMOVE;

  if (py > MAXMOVE)
    py = MAXMOVE;
  else if (py < -MAXMOVE)
    py = -MAXMOVE;

  fixed_t xmove = px;
  fixed_t ymove = py;

  //reducing bobbing/momentum on ice
  fixed_t oldx = x;
  fixed_t oldy = y;

  fixed_t ptryx, ptryy;

  do
    {
      if (xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
        {
	  ptryx = x + xmove/2;
	  ptryy = y + ymove/2;
	  xmove >>= 1;
	  ymove >>= 1;
        }
      else
        {
	  ptryx = x + xmove;
	  ptryy = y + ymove;
	  xmove = ymove = 0;
        }

      if (!TryMove(ptryx, ptryy, true))
        {
	  // blocked move

	  if (flags2 & MF2_SLIDE)
            {   // try to slide along it
	      mp->SlideMove(this);
            }
	  else if (flags & MF_MISSILE)
            {
	      // explode a missile
	      if (ceilingline &&
		  ceilingline->backsector &&
		  ceilingline->backsector->ceilingpic == skyflatnum &&
		  ceilingline->frontsector &&
		  ceilingline->frontsector->ceilingpic == skyflatnum &&
		  subsector->sector->ceilingheight == ceilingz)
		if (!boomsupport ||
                    z > ceilingline->backsector->ceilingheight)
                  {
                    // Hack to prevent missiles exploding
                    // against the sky.
                    // Does not handle sky floors.
                    //SoM: 4/3/2000: Check frontsector as well..
		    /*
		    if (type == MT_BLOODYSKULL)
                      {
			px = py = 0;
			pz = -FRACUNIT;
                      }
		    else
		    */
		      Remove();
		    return;
                  }

#ifdef WALLSPLATS
	      // draw damage on wall
	      if (blockingline)   //set by last P_TryMove() that failed
                {
		  divline_t   divl;
		  divline_t   misl;
		  fixed_t     frac;

		  P_MakeDivline (blockingline, &divl);
		  misl.x = x;
		  misl.y = y;
		  misl.dx = px;
		  misl.dy = py;
		  frac = P_InterceptVector (&divl, &misl);
		  R_AddWallSplat (blockingline, P_PointOnLineSide(x,y,blockingline)
				  ,"A_DMG3", z, frac, SPLATDRAWMODE_SHADE);
                }
#endif

	      flags &= ~MF_MISSILE;
	      //ExplodeMissile();
            }
	  else
	    px = py = 0;
        }
    } while (xmove || ymove);

  // here some code was moved to PlayerPawn::XYMovement

  // Friction.

  // no friction for missiles ever
  if (flags & MF_MISSILE || eflags & MFE_SKULLFLY)
    return;

  XYFriction(oldx, oldy);
}

//----------------------------------------------------------------------------
// was P_XYFriction
//
// adds friction on the xy plane

#define STOPSPEED            (0x1000/NEWTICRATERATIO)
#define FRICTION_LOW          0xf900  // 0.973
const float friction_fly = 0.918f;
#define FRICTION_FLY          0xeb00  // 0.918
#define FRICTION              0xe800  // 0.90625
#define FRICTION_UNDERWATER  (FRICTION*3/4)

void Actor::XYFriction(fixed_t oldx, fixed_t oldy)
{
  // slow down in water, not too much for playability issues
  if (eflags & MFE_UNDERWATER)
    {
      px = FixedMul (px, FRICTION_UNDERWATER);
      py = FixedMul (py, FRICTION_UNDERWATER);
      return;
    }

  // no friction when airborne
  if (z > floorz && !(flags2 & MF2_FLY) && !(flags2 & MF2_ONMOBJ))
    return;

  if (flags & MF_CORPSE)
    {
      // do not stop sliding if halfway off a step with some momentum
      if (px > FRACUNIT/4 || px < -FRACUNIT/4 || py > FRACUNIT/4 || py < -FRACUNIT/4)
        {
	  if (floorz != subsector->sector->floorheight)
	    return;
        }
    }

  if (px > -STOPSPEED && px < STOPSPEED && py > -STOPSPEED && py < STOPSPEED)
    {
      px = py = 0;
      return;
    }

  sector_t *sec;
  float fri = normal_friction;

  if (flags & (MF_NOGRAVITY | MF_NOCLIPLINE))
    //if (thing->Type() != Thinker::tt_ppawn)
    ;
  else if ((oldx == x) && (oldy == y)) // Did you go anywhere?
    ;
  else if ((flags2 & MF2_FLY) && (z > floorz) && !(flags2 & MF2_ONMOBJ))
    fri = friction_fly;
  else
    {
      msecnode_t *node = touching_sectorlist;
      while (node)
	{
	  sec = node->m_sector;

	  if ((sec->special & SS_friction) && (z <= sec->floorheight))
	    if (fri == normal_friction || fri > sec->friction)
	      {
		fri = sec->friction;
		//thing->movefactor = movefactor;
	      }
	  node = node->m_tnext;
	}
    }

  px = int(px * fri);
  py = int(py * fri);

  //friction = normal_friction;
}


//-----------------------------------------
// was P_ZMovement
//
void Actor::ZMovement()
{
  extern int skyflatnum;
  fixed_t     dist;
  fixed_t     delta;

  // adjust height
  z += pz;

  if ((flags & MF_FLOAT) && target)
    {
      // float down towards target if too close
      if (!(eflags & MFE_SKULLFLY)
	  && !(eflags & MFE_INFLOAT))
        {
	  dist = P_AproxDistance(x - target->x, y - target->y);
	  delta = (target->z + (height>>1)) - z;

	  if (delta < 0 && dist < -(delta*3) )
	    z -= FLOATSPEED;
	  else if (delta > 0 && dist < (delta*3) )
	    z += FLOATSPEED;
        }

    }

  // was only for PlayerPawns, but why?
  if ((flags2 & MF2_FLY) && (z > floorz) && (mp->maptic & 2))
    {
      z += finesine[(FINEANGLES / 20 * mp->maptic >> 2) & FINEMASK];
    }

  // clip this movement

  if (z <= floorz)
    {
      // hit the floor
      if (flags & MF_MISSILE)
        {
	  z = floorz;
	  eflags |= MFE_JUSTHITFLOOR;
	  return;
	}
        
      // Did we actually fall to the ground?
      if (z - pz > floorz)
	{
	  HitFloor();
	  eflags |= MFE_JUSTHITFLOOR;
	}

      z = floorz;

      if (eflags & MFE_SKULLFLY)
        {
	  // the skull slammed into something
	  pz = -pz;
        }
      else
	pz = 0;

    }
  else if (flags2 & MF2_LOGRAV)
    {
      // TODO sector gravity
      if (pz == 0)
	pz = -(cv_gravity.value>>3)*2;
      else
	pz -= cv_gravity.value>>3;
    }
  else if (!(flags & MF_NOGRAVITY)) // Gravity!
    {
      fixed_t gravityadd = -cv_gravity.value/NEWTICRATERATIO;

      // if waist under water, slow down the fall
      if (eflags & MFE_UNDERWATER)
	{
	  if (eflags & MFE_SWIMMING)
	    gravityadd = 0;     // gameplay: no gravity while swimming
	  else
	    gravityadd >>= 2;
	}
      else if (pz == 0)
	// mobj at stop, no floor, so feel the push of gravity!
	gravityadd <<= 1;

      pz += gravityadd;
    }

  if (z + height > ceilingz)
    {
      z = ceilingz - height;

      // hit the ceiling
      if (pz > 0)
	if (eflags & MFE_SKULLFLY)
	  {       // the skull slammed into something
	    pz = -pz;
	  }
	else
	  pz = 0;

      if (flags & MF_MISSILE)
        {
	  //SoM: 4/3/2000: Don't explode on the sky!
	  if (subsector->sector->ceilingpic == skyflatnum &&
	      subsector->sector->ceilingheight == ceilingz)
            {
	      Remove();
	      return;
            }

	  eflags |= MFE_JUSTHITFLOOR; // with missiles works also with ceiling hits
	  return;
        }
    }

  // no z friction for missiles and skulls
  if (flags & MF_MISSILE || eflags & MFE_SKULLFLY)
    return;

  // z friction in water
  if ((eflags & MFE_TOUCHWATER) || (eflags & MFE_UNDERWATER)) 
    {
      pz = FixedMul(pz, FRICTION_UNDERWATER);
    }
}



//-------------------------------------------------
//
// was P_ThrustMobj
//

void Actor::Thrust(angle_t angle, fixed_t move)
{
  angle >>= ANGLETOFINESHIFT;
  px += FixedMul(move, finecosine[angle]);
  py += FixedMul(move, finesine[angle]);
}



//-------------------------------------------------
// was P_MobjCheckWater
// check for water in the sector, set MFE_TOUCHWATER and MFE_UNDERWATER
// called by Actor::Think()
void Actor::CheckWater()
{
  if (flags & MF_NOSPLASH)
    return;

  // see if we are in water, and set some flags for later
  sector_t *sector = subsector->sector;
  fixed_t fz = sector->floorheight;
  int oldeflags = eflags;

  //SoM: 3/28/2000: Only use 280 water type of water. Some boom levels get messed up.
  if ((sector->heightsec > -1 && sector->altheightsec == 1) ||
      (sector->floortype == FLOOR_WATER && sector->heightsec == -1))
    {
      if (sector->heightsec > -1)  //water hack
	fz = (mp->sectors[sector->heightsec].floorheight);
      else
	fz = sector->floorheight + (FRACUNIT/4); // water texture

      if (z <= fz && z+height > fz)
	eflags |= MFE_TOUCHWATER;
      else
	eflags &= ~MFE_TOUCHWATER;

      if (z+(height>>1) <= fz)
	eflags |= MFE_UNDERWATER;
      else
	eflags &= ~MFE_UNDERWATER;
    }
  else if (sector->ffloors)
    {
      ffloor_t*  rover;

      eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER);

      for (rover = sector->ffloors; rover; rover = rover->next)
	{
	  if (!(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
	    continue;
	  if (*rover->topheight <= z || *rover->bottomheight > (z + (height >> 1)))
	    continue;

	  if (z + height > *rover->topheight)
            eflags |= MFE_TOUCHWATER;
	  else
            eflags &= ~MFE_TOUCHWATER;

	  if (z + (height >> 1) < *rover->topheight)
            eflags |= MFE_UNDERWATER;
	  else
            eflags &= ~MFE_UNDERWATER;

	  if (!(oldeflags & (MFE_TOUCHWATER|MFE_UNDERWATER))
	      && (eflags & (MFE_TOUCHWATER|MFE_UNDERWATER))) // && game.mode != gm_heretic
            mp->SpawnSplash(this, *rover->topheight);
	}
      return;
    }
  else
    eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER);

  /*
    if ((eflags ^ oldeflags) & MFE_TOUCHWATER)
    CONS_Printf("touchewater %d\n",eflags & MFE_TOUCHWATER ? 1 : 0);
    if ((eflags ^ oldeflags) & MFE_UNDERWATER)
    CONS_Printf("underwater %d\n",eflags & MFE_UNDERWATER ? 1 : 0);
  */
}


//---------------------------------------------------------------------------
// was P_HitFloor
// Creates a splash if needed and returns the floor type
int Actor::HitFloor()
{
  if (floorz != subsector->sector->floorheight)
    { // don't splash if landing on the edge above water/lava/etc....
      return FLOOR_SOLID;
    }

  int floortype = subsector->sector->floortype;

  if (flags & MF_NOSPLASH)
    return floortype;

  if (game.mode == gm_heretic)
    {
      DActor *p;
      switch (floortype)
	{
	case FLOOR_WATER:
	  mp->SpawnDActor(x, y, ONFLOORZ, MT_SPLASHBASE);
	  p = mp->SpawnDActor(x, y, ONFLOORZ, MT_HSPLASH);
	  p->owner = this;
	  p->px = P_SignedRandom()<<8;
	  p->py = P_SignedRandom()<<8;
	  p->pz = 2*FRACUNIT+(P_Random()<<8);
	  S_StartSound(p, sfx_gloop);
	  break;

	case FLOOR_LAVA:
	  mp->SpawnDActor(x, y, ONFLOORZ, MT_LAVASPLASH);
	  p = mp->SpawnDActor(x, y, ONFLOORZ, MT_LAVASMOKE);
	  p->pz = FRACUNIT+(P_Random()<<7);
	  S_StartSound(p, sfx_burn);
	  break;

	case FLOOR_SLUDGE:
	  mp->SpawnDActor(x, y, ONFLOORZ, MT_SLUDGESPLASH);
	  p = mp->SpawnDActor(x, y, ONFLOORZ, MT_SLUDGECHUNK);
	  p->owner = this;
	  p->px = P_SignedRandom()<<8;
	  p->py = P_SignedRandom()<<8;
	  p->pz = FRACUNIT+(P_Random()<<8);
	  break;
	}
      return floortype;
    }
  else if (floortype == FLOOR_WATER)
    mp->SpawnSplash(this, floorz);

  // do not down the viewpoint
  return FLOOR_SOLID;
}


//==============================================================
// DActor methods


//---------------------------------------
// was P_SetMobjState
// was P_SetMobjStateNF
// Returns true if the mobj is still present.
//
//SoM: 4/7/2000: Boom code...
bool DActor::SetState(statenum_t ns, bool call)
{
  //remember states seen, to detect cycles:    
  static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
  static int recursion;                       // detects recursion

  statenum_t *seenstate = seenstate_tab;      // pointer to table

  statenum_t i = ns;                       // initial state
  bool ret = true;                         // return value
  statenum_t tempstate[NUMSTATES];            // for use with recursion
    
  if (recursion++)                            // if recursion detected,
    memset(seenstate = tempstate, 0, sizeof(tempstate)); // clear state table
    
  do {
    if (ns == S_NULL)
      {
	state = &states[S_NULL];
	Remove();
	ret = false;
	break;                 // killough 4/9/98
      }
        
    state = &states[ns];
    tics = state->tics;
    
    // Modified handling.
    // Call action functions when the state is set
    if (call == true && state->action)
      state->action(this);
        
    seenstate[ns] = statenum_t(1 + state->nextstate);   // killough 4/9/98
        
    ns = state->nextstate;
  } while (!tics && !seenstate[ns]);   // killough 4/9/98

  pres->SetFrame(state); // set the sprite frame (if pres is not a sprite, do nothing)

  if (ret && !tics)  // killough 4/9/98: detect state cycles
    CONS_Printf("Warning: State Cycle Detected");
    
  if (!--recursion)
    for ( ; (ns = seenstate[i]) ; i = statenum_t(ns-1))
      seenstate[i] = statenum_t(0);  // killough 4/9/98: erase memory of states
        
  return ret;
}


/*
bool P_SetMobjStateNF(Actor *mobj, statenum_t state)
{
  state_t *st;
    
  if (state == S_NULL)
    { // Remove mobj
      P_RemoveMobj(mobj);
      return(false);
    }
  st = &states[state];
  mobj->state = st;
  mobj->tics = st->tics;
  mobj->sprite = st->sprite;
  mobj->frame = st->frame;
  return(true);
}
*/


//-------------------------------------------------
// was P_NightmareRespawn
//
void DActor::NightmareRespawn()
{
  fixed_t  nx, ny, nz;

  nx = spawnpoint->x << FRACBITS;
  ny = spawnpoint->y << FRACBITS;

  // somthing is occupying it's position?
  if (!CheckPosition(nx, ny))
    return; // no respwan

  // spawn a teleport fog at old spot
  // because of removal of the body?
  DActor *mo = mp->SpawnDActor(x, y, subsector->sector->floorheight + 
			       (game.mode == gm_heretic ? TELEFOGHEIGHT : 0), MT_TFOG);
  // initiate teleport sound
  S_StartSound(mo, sfx_telept);

  // spawn a teleport fog at the new spot
  subsector_t *ss = mp->R_PointInSubsector(nx, ny);

  mo = mp->SpawnDActor(nx, ny, ss->sector->floorheight +
		   (game.mode == gm_heretic ? TELEFOGHEIGHT : 0) , MT_TFOG);
  S_StartSound(mo, sfx_telept);

  // spawn the new monster
  mapthing_t *mthing = spawnpoint;

  // spawn it
  if (info->flags & MF_SPAWNCEILING)
    nz = ONCEILINGZ;
  else
    nz = ONFLOORZ;

  // inherit attributes from deceased one
  mo = mp->SpawnDActor(nx, ny, nz, type);
  mo->spawnpoint = spawnpoint;
  mo->angle = ANG45 * (mthing->angle/45);

  if (mthing->flags & MTF_AMBUSH)
    mo->flags |= MF_AMBUSH;

  mo->reactiontime = 18;

  // remove the old monster,
  Remove();
}


//---------------------------------------------
// was P_SpawnMissile
//

DActor *DActor::SpawnMissile(Actor *dest, mobjtype_t type)
{
  fixed_t  mz;

#ifdef PARANOIA
  if (!dest)
    I_Error("P_SpawnMissile : no dest");
#endif
  switch (type)
    {
    case MT_MNTRFX1: // Minotaur swing attack missile
      mz = z+40*FRACUNIT;
      break;
    case MT_MNTRFX2: // Minotaur floor fire missile
      mz = ONFLOORZ;
      break;
    case MT_SRCRFX1: // Sorcerer Demon fireball
      mz = z+48*FRACUNIT;
      break;
    case MT_KNIGHTAXE: // Knight normal axe
    case MT_REDAXE: // Knight red power axe
      mz = z+36*FRACUNIT;
      break;
    default:
      mz = z+32*FRACUNIT;
      break;
    }

  mz -= floorclip;

  DActor *th = mp->SpawnDActor(x, y, mz, type);

  if (th->info->seesound)
    S_StartSound(th, th->info->seesound);

  th->owner = this; // where it came from

  angle_t an = R_PointToAngle2(x, y, dest->x, dest->y);

  // fuzzy player
  if (dest->flags & MF_SHADOW)
    {
      if (game.mode == gm_heretic || game.mode == gm_hexen)
	an += P_SignedRandom()<<21; 
      else
	an += P_SignedRandom()<<20;
    }

  th->angle = an;
  an >>= ANGLETOFINESHIFT;
  th->px = int(th->info->speed * finecosine[an]);
  th->py = int(th->info->speed * finesine[an]);

  int dist = P_AproxDistance(dest->x - x, dest->y - y);
  dist = dist / int(th->info->speed * FRACUNIT);

  if (dist < 1)
    dist = 1;

  th->pz = (dest->z - z) / dist;

  return (th->CheckMissileSpawn()) ? th : NULL;
}


//---------------------------------------------
// was P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
bool DActor::CheckMissileSpawn()
{
  if (game.mode != gm_heretic && game.mode != gm_hexen)
    {
      tics -= P_Random()&3;
      if (tics < 1)
	tics = 1;
    }

  // move a little forward so an angle can
  // be computed if it immediately explodes
  x += (px>>1);
  y += (py>>1);
  z += (pz>>1);

  if (!TryMove(x, y, false))
    {
      ExplodeMissile();
      return false;
    }
  return true;
}



//---------------------------------------------
//
// was P_ExplodeMissile
//

void DActor::ExplodeMissile()
{
  if (type == MT_WHIRLWIND)
    if (++special2 < 60)
      return;

  px = py = pz = 0;

  SetState(mobjinfo[type].deathstate);

  if (game.mode != gm_heretic)
    {
      tics -= P_Random()&3;
        
      if (tics < 1)
	tics = 1;
    }

  flags &= ~MF_MISSILE;

  if (info->deathsound)
    S_StartSound(this, info->deathsound);
}



//----------------------------------------------
//
// was P_FloorBounceMissile
//

void DActor::FloorBounceMissile()
{
  pz = -pz;
  SetState(mobjinfo[type].deathstate);
}
