// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
/// \brief Actor class implementation.

#include "g_actor.h"

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_input.h"
#include "p_enemy.h" // #defines

#include "p_spec.h"
#include "p_maputl.h"

#include "r_sprite.h"
#include "r_splats.h"
#include "sounds.h"
#include "s_sound.h"

#include "m_random.h"
#include "tables.h"

/*!
  \defgroup g_thing The Doom mapthing system

  These data structures are used to define the Doom mapthing system,
  including the monster AI.
*/

/*!
  \defgroup g_actor Common types of Actors

  Certain types of Actors appear throughout the code.
*/

/*!
  \page actor_monster Monster
  \ingroup g_actor
  Basic monster, e.g. a Doom imp.
  Has typically the flags MF_SOLID | MF_SHOOTABLE | MF_COUNTKILL and the flags2 MF2_PUSHWALL | MF2_MCROSS.
*/

/*!
  \page actor_floater Floater
  \ingroup g_actor
  Floating monster, e.g. a cacodemon.
  Has the flags MF_FLOAT | MF_DROPOFF | MF_NOGRAVITY in addition to the normal @ref actor_monster flags.
  In DActor::P_Move, if it encounters a wall with an opening during its move,
  it will try to float vertically towards the opening and set the flag MFE_INFLOAT.
  If it has a target, during ZMovement it will float towards it in the vertical direction
  unless MFE_INFLOAT is on.
*/

/*!
  \page actor_missile Missile
  \ingroup g_actor
  Moving projectile, e.g. a rocket. Often does damage. Definition: has the flag MF_MISSILE.
  Usually also has the flags MF_DROPOFF | MF_NOBLOCKMAP | MF_NOGRAVITY
  and the flags2 MF2_NOTELEPORT | MF2_IMPACT | MF2_PCROSS.
  Many flags only make sense if they are combined with MF_MISSILE... TODO explain

  MF2_SEEKERMISSILE homes towards its target. If reflected from a MF2_REFLECTIVE target, owner becomes target and v.v.
*/

/*!
  \page actor_item Pickup
  \ingroup g_actor
  Item that can be picked up by the players, e.g. a health bonus. Definition: has the flag MF_SPECIAL.
  Often also has the flags MF_COUNTITEM and MF2_FLOATBOB.
*/



//===========================================================
//             Actor class implementation
//===========================================================

IMPLEMENT_CLASS(Actor, Thinker);

Actor::~Actor()
{
  if (pres)
    delete pres; // delete the presentation object too
}


/// trick constructor
Actor::Actor()
{
  mp = NULL;

  pres = NULL;
  snext = sprev = bnext = bprev = NULL;
  subsector = NULL;

  floorz = ceilingz = 0;
  touching_sectorlist = NULL;
  spawnpoint = NULL;

  yaw = pitch = 0;

  flags = flags2 = eflags = 0;

  special = tid = 0;
  args[0] = args[1] = args[2] = args[3] = args[4] = 0;

  owner = target = NULL;

  reactiontime = 0;
  floorclip = 0;
  team = 0;

  // net stuff
  mNetFlags.set(Ghostable);
}


///  Normal constructor
Actor::Actor(fixed_t nx, fixed_t ny, fixed_t nz)
  : Thinker(), pos(nx, ny, nz)
{
  // note! here Map *mp is not yet set! This means you can't call functions such as
  // SetPosition that have something to do with a map.
  pres = NULL;
  snext = sprev = bnext = bprev = NULL;
  subsector = NULL;
  floorz = ceilingz = 0;
  touching_sectorlist = NULL;
  spawnpoint = NULL;

  yaw = pitch = 0;

  // NOTE some attributes left uninitialized here!
  flags = flags2 = eflags = 0;

  special = tid = 0;
  args[0] = args[1] = args[2] = args[3] = args[4] = 0;

  owner = target = NULL;

  reactiontime = 0;
  floorclip = 0;
  team = 0;

  // net stuff
  mNetFlags.set(Ghostable);
}



//===========================================================
//                     Actor netcode
//===========================================================

TNL_IMPLEMENT_NETOBJECT(Actor);
TNL_IMPLEMENT_NETOBJECT(DActor);

#define ACTOR_AR 8 // angle resolution for netcode (in bits)

/// This is called on clients when a new Actor comes into scope
bool Actor::onGhostAdd(class GhostConnection *c)
{
  CONS_Printf("Adding a %s.\n", Type()->name);
  return true;
}


/// This is called on clients when an Actor leaves scope
void Actor::onGhostRemove()
{
  CONS_Printf("Removing a %s.\n", Type()->name);
  // TODO CHECK automatically destroyed after this?
  //Remove();
}



/// This is the function the server uses to ghost Actor data to clients.
U32 Actor::packUpdate(GhostConnection *c, U32 mask, class BitStream *stream)
{
  if (isInitialUpdate())
    {
      mask = M_EVERYTHING;
    }

  // check which states need to be updated, and write updates
  if (stream->writeFlag(mask & M_MOVE))
    {
      // as often as possible
      pos.Pack(stream);
      vel.Pack(stream);
      stream->writeInt(yaw   >> (32-ACTOR_AR), ACTOR_AR); // we do not need full resolution
      stream->writeInt(pitch >> (32-ACTOR_AR), ACTOR_AR);
    }

  // NOTE: here we are in fact transferring presentation_t data.
  // Decided not to make presentation_t a NetObject of its own, since
  // it is always associated with an Actor and this is simpler.

  if (stream->writeFlag(mask & M_PRES))
    {
      // very rarely
      // send over model/sprite info (mobjinfo/modeltype, color, drawing flags)
      pres->Pack(stream);
      mask &= ~M_ANIM; // it's already included here
    }

  if (stream->writeFlag(mask & M_ANIM))
    {
      // often
      // send over animation sequence and interpolation / state
      pres->PackAnim(stream);
    }

  if (isInitialUpdate())
    {
      // map number (only needed on initial update)
      stream->write(mp->info->mapnumber);
    }

  // TODO floorclip, team

  // the return value from packUpdate can set which states still
  // need to be updated for this object.
  return 0;
}


/// the clientside pair of packUpdate
void Actor::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
  // NOTE: the unpackUpdate function must be symmetrical to packUpdate
  if (stream->readFlag()) // M_MOVE
    {
      // movement data
      apos.Unpack(stream); // NOTE: unpacked to apos, not pos! then we interpolate...
      avel.Unpack(stream);
      yaw   = stream->readInt(ACTOR_AR); // we do not need full resolution
      pitch = stream->readInt(ACTOR_AR);
    }

  if (stream->readFlag()) // M_PRES
    {
      // get model/sprite info (mobjinfo/modeltype, color)
      // TODO read presentation type, create it
      if (pres)
	delete pres;
      pres = new spritepres_t(stream);
    }

  if (stream->readFlag()) // M_ANIM
    {
      // get animation sequence and interpolation
      pres->UnpackAnim(stream);
    }

  if (isInitialUpdate())
    {
      // map number (only needed on initial update)
      int temp;
      stream->read(&temp);
      MapInfo *m = game.FindMapInfo(temp);
      if (!m)
	I_Error("Got a ghosted Actor for an unknown map %d!", temp);

      //if (!m->me) I_Error("Got a ghosted Actor for an inactive map %d!", temp);

      if (!m->me)
	if (!m->Activate(NULL)) // clientside map activation
	  I_Error("Crap!\n");

      m->me->SpawnActor(this); // link the ghost to the correct Map
    }
}



/// Interpolate movement
void Actor::ClientThink()
{
  // FIXME interpolate movement
  if (pos != apos)
    {
      UnsetPosition();
      pos = apos;
      vel = avel;
      SetPosition();
    }
}


//===========================================================
//              Rest of Actor implementation
//===========================================================

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

  if (tid)
    {
      mp->RemoveFromTIDmap(this);
      tid = 0;
    }

  UnsetPosition(); // blockmap and sector links

  if (touching_sectorlist)
    {
      P_DelSeclist(touching_sectorlist);
      touching_sectorlist = NULL;
    }

  // free up the spawnpoint
  if (spawnpoint && spawnpoint->mobj == this)
    {
      spawnpoint->mobj = NULL;
      spawnpoint = NULL;
    }

  owner = target = NULL;

  // stop any playing sound
  S.Stop3DSound(this);

  // save the presentation too
  if (pres)
    Z_ChangeTag(pres, PU_STATIC);

  eflags |= MFE_REMOVE; // so that pointers to it will be NULLed
  mp->DetachThinker(this);
}


// Lazy delete: Memory freed only after pointers to this object
// have been NULLed in Map::RunThinkers()
void Actor::Remove()
{
  if (mp == NULL)
    {
      // should not happen
      delete this;
      return;
    }

  if (eflags & MFE_REMOVE)
    return; // already marked for removal

  if (tid)
    mp->RemoveFromTIDmap(this);

  // unlink from sector and block lists
  UnsetPosition();

  if (touching_sectorlist)
    {
      P_DelSeclist(touching_sectorlist);
      touching_sectorlist = NULL;
    }

  // free up the spawnpoint
  if (spawnpoint && spawnpoint->mobj == this)
    spawnpoint->mobj = NULL;

  // stop any playing sound
  S.Stop3DSound(this);

  // remove it from active Thinker list, add it to the removal list
  eflags |= MFE_REMOVE;
  mp->RemoveThinker(this);
}



void P_NoiseAlert(Actor *target, Actor *emitter);
void PlayerPawn::LandedOnThing(Actor *onmobj)
{
  // TODO damage also the thing! kill imps in super mario fashion!

  player->deltaviewheight = vel.z >> 3;

  if (vel.z < -23)
    {
      FallingDamage();
      P_NoiseAlert(this, this);
      return;
    }

  if (morphTics)
    return;

  if (vel.z < -8)
    S_StartSound(this, sfx_land);

  if (vel.z < -12)
    {
      switch (pclass)
	{
	case PCLASS_FIGHTER:
	  S_StartSound(this, SFX_PLAYER_FIGHTER_GRUNT);
	  break;
	case PCLASS_CLERIC:
	  S_StartSound(this, SFX_PLAYER_CLERIC_GRUNT);
	  break;
	case PCLASS_MAGE:
	  S_StartSound(this, SFX_PLAYER_MAGE_GRUNT);
	  break;
	default:
	  break;
	}
    }
}



void Actor::Think()
{
  PlayerPawn *p = NULL;
  if (IsOf(PlayerPawn::_type))
    p = (PlayerPawn *)this;

  // check possible sector water content, set water eflags, cause splashes on ffloors
  CheckWater();

  // XY movement
  if (vel.x != 0 || vel.y != 0)
    XYMovement();
  else
    {
      if (eflags & MFE_BLASTED)
	{
	  // Reset to not blasted when xy-speed is gone
	  eflags &= ~MFE_BLASTED;
	  //if (!(flags & MF_ICECORPSE)) TODO ICECORPSE?
	  //  flags2 &= ~MF2_SLIDE;
	}

      if (eflags & MFE_SKULLFLY)
	{
	  // the actor slammed into something
	  eflags &= ~MFE_SKULLFLY;
	  vel.z = 0;
	}
    }

  // Z movement
  eflags &= ~MFE_JUSTHITFLOOR;

  if (flags2 & MF2_FLOATBOB)
    {
      // Floating item bobbing motion (maybe move to DActor::Think ?)
      pos.z = floorz + FloatBobOffsets[(reactiontime++) & 63]; // floating height is added later
    }
  else if (!(eflags & MFE_ONGROUND) || (pos.z != floorz) || vel.z != 0)
    {
      // time to fall...

      if (flags2 & MF2_NOPASSMOBJ)
	{
	  // simplified treatment
	  ZMovement();
	  return;
	}

      Actor *onmo = CheckOnmobj();
      if (!onmo)
	{
	  ZMovement();
	  eflags &= ~MFE_ONMOBJ;
	}
      else
	{
	  if (p) // FIXME is this ok? For all Actors?
	    {
	      if (vel.z < -8 && !(eflags & MFE_FLY))
		p->LandedOnThing(onmo);

	      if (onmo->Top() <= pos.z + 24) // FIXME magic number
		{
		  p->player->viewheight -= onmo->Top() - pos.z;
		  p->player->deltaviewheight = (cv_viewheight.value - p->player->viewheight) >> 3;
		  pos.z = onmo->Top();
		  eflags |= MFE_ONMOBJ;
		  vel.z = 0;
		}                               
	      else
		{ // hit the bottom of the blocking mobj
		  vel.z = 0;
		}
	    }
	}
    }
}



// returns the value by which the x,y
// movements are multiplied to add to player movement.
float normal_friction = 0.90625f; // 0xE800

float Actor::GetMoveFactor()
{
  extern int variable_friction;
  float mf  = 1.0f;

  // less control if not onground.
  bool onground = (pos.z <= floorz) || (eflags & (MFE_ONMOBJ | MFE_FLY));

  if (boomsupport && variable_friction && onground && !(flags & (MF_NOGRAVITY | MF_NOCLIPLINE)))
    {
      float frict = normal_friction;
      msecnode_t *node = touching_sectorlist;
      sector_t *sec;
      while (node)
	{
	  sec = node->m_sector;
	  if ((sec->special & SS_friction) && (pos.z <= sec->floorheight))
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
	  const float MORE_FRICTION_MOMENTUM = 0.22888f; // 15000  // mud factor based on momentum

          float momentum = P_AproxDistance(vel.x, vel.y).Float();

	  if (momentum < MORE_FRICTION_MOMENTUM)
	    mf *= 0.125;
          else if (momentum < 2*MORE_FRICTION_MOMENTUM)
	    mf *= 0.25; 
          else if (momentum < 4*MORE_FRICTION_MOMENTUM)
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



/// Handles horizontal movement
void Actor::XYMovement()
{
  extern int skyflatnum;
  const fixed_t MAXMOVE = 30;

  if (vel.x > MAXMOVE)
    vel.x = MAXMOVE;
  else if (vel.x < -MAXMOVE)
    vel.x = -MAXMOVE;

  if (vel.y > MAXMOVE)
    vel.y = MAXMOVE;
  else if (vel.y < -MAXMOVE)
    vel.y = -MAXMOVE;

  fixed_t xmove = vel.x; // this is so that we can change velocity in the collision code during the move
  fixed_t ymove = vel.y;

  fixed_t oldx = pos.x;
  fixed_t oldy = pos.y;

  fixed_t ptryx, ptryy;

  do
    {
      // TODO try size should depend on mobj radius
      if (abs(xmove) > MAXMOVE/2 || abs(ymove) > MAXMOVE/2)
        {
	  ptryx = pos.x + xmove/2;
	  ptryy = pos.y + ymove/2;
	  xmove >>= 1;
	  ymove >>= 1;
        }
      else
        {
	  ptryx = pos.x + xmove;
	  ptryy = pos.y + ymove;
	  xmove = ymove = 0;
        }

      if (!TryMove(ptryx, ptryy, true))
        {
	  // blocked move
	  if (flags2 & MF2_SLIDE)
            {
	      // try to slide along whatever blocked us
	      if (!Blocking.thing)
		{
		  SlideMove(ptryx, ptryy); // Slide against wall
		}
	      else
		{
		  // Slide against mobj TODO is this needed? wasn't before!
		  if (TryMove(pos.x, ptryy, true))
		    vel.x = 0;
		  else if (TryMove(ptryx, pos.y, true))
		    vel.y = 0;
		  else
		    vel.x = vel.y = 0;
		}
            }
	  else if (flags & MF_MISSILE)
            {
	      if (Blocking.thing)
		return; // explosions handled at Actor::Touch()

	      // must have been blocked by a line

	      if ((flags2 & MF2_FULLBOUNCE))
		{
		  // Struck a wall
		  BounceWall(ptryx, ptryy);
		  DActor *t = IsOf(DActor::_type) ? reinterpret_cast<DActor*>(this) : NULL;
		  if (t)
		    switch (t->type)
		      {
		      case MT_SORCBALL1:
		      case MT_SORCBALL2:
		      case MT_SORCBALL3:
		      case MT_SORCFX1:
			break;
		      default:
			if (t->info->seesound)
			  S_StartSound(this, t->info->seesound);
			break;
		      }

		  return; // no blow
		}


	      // explode a missile, but not against the sky (hack)
	      if (ceilingline &&
		  ceilingline->backsector &&
		  ceilingline->backsector->ceilingpic == skyflatnum &&
		  ceilingline->frontsector &&
		  ceilingline->frontsector->ceilingpic == skyflatnum &&
		  subsector->sector->ceilingheight == ceilingz)
		if (!boomsupport ||
                    pos.z > ceilingline->backsector->ceilingheight)
                  {
                    // Hack to prevent missiles exploding against the sky.
                    // Does not handle sky floors.
		    
		    // if (type == MT_HOLY_FX) ExplodeMissile(); // TODO some things do explode against sky

		    Remove();
		    return;
                  }

	      // draw damage on wall
	      if (Blocking.line && !(flags & MF_NOSCORCH))  // set by last TryMove() that failed
                {
		  divline_t   divl;
		  divline_t   misl;

		  divl.MakeDivline(Blocking.line);
		  misl.x = pos.x;
		  misl.y = pos.y;
		  misl.dx = vel.x;
		  misl.dy = vel.y;
		  fixed_t frac = P_InterceptVector(&divl, &misl);
		  mp->R_AddWallSplat(Blocking.line, P_PointOnLineSide(pos.x, pos.y, Blocking.line),
				     "A_DMG3", pos.z, frac, SPLATDRAWMODE_SHADE);
                }

	      flags &= ~MF_MISSILE;
	      return;
            }
	  else
	    vel.x = vel.y = 0;
        }
    } while (xmove != 0 || ymove != 0);

  // here some code was moved to PlayerPawn::XYMovement

  // Friction.

  // no friction for missiles ever
  if (flags & MF_MISSILE || eflags & MFE_SKULLFLY)
    return;

  XYFriction(oldx, oldy);
}


const float friction_underwater = 0.75 * normal_friction;

/// Friction on the xy plane
void Actor::XYFriction(fixed_t oldx, fixed_t oldy)
{
  const fixed_t STOPSPEED  = 0.0625f; // 0x1000
  const fixed_t SLIDESPEED = 0.25f;
  const float friction_fly = 0.918f;  // 0xeb00

  // slow down in water, not too much for playability issues
  if (eflags & MFE_UNDERWATER)
    {
      vel.x *= friction_underwater;
      vel.y *= friction_underwater;
      return;
    }

  // no friction when airborne
  if (pos.z > floorz && !(eflags & (MFE_FLY | MFE_ONMOBJ)))
    return;

  if (flags & MF_CORPSE)
    {
      // do not stop sliding if halfway off a step with some momentum
      if (vel.x > SLIDESPEED || vel.x < -SLIDESPEED || vel.y > SLIDESPEED || vel.y < -SLIDESPEED)
        {
	  if (floorz != subsector->sector->floorheight)
	    return;
        }
    }

  if (vel.x > -STOPSPEED && vel.x < STOPSPEED && vel.y > -STOPSPEED && vel.y < STOPSPEED)
    {
      vel.x = vel.y = 0;
      return;
    }

  sector_t *sec;
  float fri = normal_friction;

  if (flags & (MF_NOGRAVITY | MF_NOCLIPLINE))
    //if (thing->Type() != Thinker::tt_ppawn)
    ;
  else if ((oldx == pos.x) && (oldy == pos.y)) // Did you go anywhere?
    ;
  else if ((eflags & MFE_FLY) && (pos.z > floorz) && !(eflags & MFE_ONMOBJ))
    fri = friction_fly;
  else
    {
      msecnode_t *node = touching_sectorlist;
      while (node)
	{
	  sec = node->m_sector;

	  if ((sec->special & SS_friction) && (pos.z <= sec->floorheight))
	    if (fri == normal_friction || fri > sec->friction)
	      {
		fri = sec->friction;
		//thing->movefactor = movefactor;
	      }
	  node = node->m_tnext;
	}
    }

  vel.x *= fri;
  vel.y *= fri;
}



// vertical movement
void Actor::ZMovement()
{
  extern int skyflatnum;

  // adjust height
  pos.z += vel.z;

  if ((flags & MF_FLOAT) && target)
    {
      // float down towards target if too close
      if (!(eflags & MFE_SKULLFLY) && !(eflags & MFE_INFLOAT))
        {
	  fixed_t dist = P_AproxDistance(pos.x - target->pos.x, pos.y - target->pos.y);
	  fixed_t delta = (target->pos.z + (height>>1)) - pos.z; // assumed to be taller than target so this looks good...

	  if (delta < 0 && dist < -(delta*3) )
	    pos.z -= FLOATSPEED;
	  else if (delta > 0 && dist < (delta*3) )
	    pos.z += FLOATSPEED;
        }

    }

  // was only for PlayerPawns, but why?
  if ((eflags & MFE_FLY) && (pos.z > floorz) && (mp->maptic & 2))
    {
      pos.z += finesine[(FINEANGLES / 20 * mp->maptic >> 2) & FINEMASK];
    }

  // clip this movement

  if (pos.z <= floorz)
    {
      // hit the floor
      if (flags & MF_MISSILE)
        {
	  pos.z = floorz;
	  eflags |= MFE_JUSTHITFLOOR;
	  return;
	}
        
      // Did we actually fall to the ground?
      if (pos.z - vel.z > floorz)
	{
	  HitFloor();
	  eflags |= MFE_JUSTHITFLOOR;

	  if (eflags & MFE_SKULLFLY)
	    {
	      // the skull slammed into floor
	      vel.z = -vel.z;
	    }
	  else if (flags & MF_COUNTKILL) // usually blasted mobj falling
	    {
	      if (vel.z < -23)
		FallingDamage();
	      vel.z = 0;
	    }
	  else
	    vel.z = 0;
	}

      pos.z = floorz;
    }
  else if (!(flags & MF_NOGRAVITY)) // Gravity!
    {
      // TODO per-sector gravity
      fixed_t gravityadd;
      gravityadd.setvalue(-cv_gravity.value);

      if (flags2 & MF2_LOGRAV)
	gravityadd >> 3; // feels just one-eight gravity

      // if waist under water, slow down the fall
      if (eflags & MFE_UNDERWATER)
	{
	  if (eflags & MFE_SWIMMING)
	    gravityadd = 0;     // gameplay: no gravity while swimming
	  else
	    gravityadd >>= 2;
	}
      else if (vel.z == 0)
	// mobj at stop, no floor, so feel the push of gravity!
	gravityadd <<= 1;

      vel.z += gravityadd;
    }

  if (Top() > ceilingz)
    {
      if (flags & MF_MISSILE)
        {
	  pos.z = ceilingz - height;
	  eflags |= MFE_JUSTHITFLOOR; // with missiles works also with ceiling hits

	  // Don't explode on the sky!
	  if (subsector->sector->ceilingpic == skyflatnum &&
	      subsector->sector->ceilingheight == ceilingz &&
	      !(flags2 & MF2_FULLBOUNCE))
            {
	      // if (type == MT_HOLY_FX) return; // TODO some things do explode against sky

	      Remove();
            }

	  return;
        }

      // Did we actually hit the ceiling?
      if (Top() - vel.z <= ceilingz)
	{
	  if (eflags & MFE_SKULLFLY)
	    {
	      vel.z = -vel.z; // skull slammed into roof
	    }
	  else
	    vel.z = 0;
	}

      pos.z = ceilingz - height;
    }

  // no z friction for missiles and skulls
  if (flags & MF_MISSILE || eflags & MFE_SKULLFLY)
    return;

  // z friction in water
  if ((eflags & MFE_TOUCHWATER) || (eflags & MFE_UNDERWATER)) 
    vel.z *= friction_underwater;
}



// Gives the actor a velocity impulse along a given angle.
void Actor::Thrust(angle_t angle, fixed_t move)
{
  angle >>= ANGLETOFINESHIFT;
  vel.x += move * finecosine[angle];
  vel.y += move * finesine[angle];
}



// check for water in the sector, set MFE_TOUCHWATER and MFE_UNDERWATER
void Actor::CheckWater()
{
  if (flags & MF_NOSPLASH)
    return;

  // see if we are in water, and set some flags for later
  sector_t *sector = subsector->sector;
  fixed_t fz = sector->floorheight;
  int oldeflags = eflags;

  //SoM: 3/28/2000: Only use 280 water type of water. Some boom levels get messed up.
  if ((sector->heightsec > -1 && sector->heightsec_type == sector_t::CS_water) ||
      (sector->heightsec == -1 && sector->floortype == FLOOR_WATER))
    {
      if (sector->heightsec > -1)  // swimmable Boom water
	fz = mp->sectors[sector->heightsec].floorheight;
      else
	fz = sector->floorheight + 0.25f; // water texture

      if (Feet() <= fz && Top() > fz)
	eflags |= MFE_TOUCHWATER;
      else
	eflags &= ~MFE_TOUCHWATER;

      if (Center() <= fz)
	eflags |= MFE_UNDERWATER;
      else
	eflags &= ~MFE_UNDERWATER;
    }
  else if (sector->ffloors)
    {
      eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER);

      for (ffloor_t *rover = sector->ffloors; rover; rover = rover->next)
	{
	  if (!(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
	    continue;
	  if (*rover->topheight <= Feet() || *rover->bottomheight > Center())
	    continue;

	  if (Top() > *rover->topheight)
            eflags |= MFE_TOUCHWATER;
	  else
            eflags &= ~MFE_TOUCHWATER;

	  if (Center() < *rover->topheight)
            eflags |= MFE_UNDERWATER;
	  else
            eflags &= ~MFE_UNDERWATER;

	  if (!(oldeflags & (MFE_TOUCHWATER|MFE_UNDERWATER))
	      && (eflags & (MFE_TOUCHWATER|MFE_UNDERWATER))) // && game.mode != gm_heretic
	    mp->SpawnSplash(pos, *rover->topheight, sfx_splash, MT_SPLASH);
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



/// Creates a splash if needed and returns the floor type
int Actor::HitFloor()
{
  if (floorz != subsector->sector->floorheight)
    { // don't splash if landing on the edge above water/lava/etc....
      return FLOOR_SOLID;
    }

  // need to touch the surface because the splashes only appear at surface
  //if (pos.z > z || pos.z + mo->height < z)  return;

  int floortype = subsector->sector->floortype;

  // some things do not cause a splash
  if (flags & MF_NOSPLASH)
    return floortype;

  // get rough idea of speed
  /*
    thrust = (mo->px + mo->py) >> FRACBITS+1;

    if (thrust >= 2 && thrust<=3)
    th->SetState(S_SPLASH2);
    else
    if (thrust < 2)
    th->SetState(S_SPLASH3);
  */

  // TODO send noise alerts if this is a player and splash is big?

  DActor *p;

  if (game.mode == gm_hexen)
    {
      const fixed_t SMALLSPLASHCLIP = 12;
      int smallsplash = false;

      // Small splash for small masses
      if (mass < 10)
	smallsplash = true;

      switch (floortype)
	{
	case FLOOR_WATER:
	  if (smallsplash)
	    {
	      p = mp->SpawnSplash(pos, floorz, SFX_AMBIENT10, MT_XSPLASHBASE, MT_NONE, false);
	      if (p)
		p->floorclip += SMALLSPLASHCLIP;
	    }
	  else
	    {
	      p = mp->SpawnSplash(pos, floorz, sfx_splash, MT_XSPLASHBASE, MT_XSPLASH, false);
	      if (p)
		p->vel.Set(P_SignedFRandom(8), P_SignedFRandom(8), 2 + P_FRandom(8));
	    }
	  break;

	case FLOOR_LAVA:
	  if (smallsplash)
	    {
	      p = mp->SpawnSplash(pos, floorz, SFX_LAVA_SIZZLE, MT_XLAVASPLASH, MT_NONE, false);
	      if (p)
		p->floorclip += SMALLSPLASHCLIP;
	    }
	  else
	    {
	      p = mp->SpawnSplash(pos, floorz, SFX_LAVA_SIZZLE, MT_XLAVASPLASH, MT_XLAVASMOKE, false);
	      if (p)
		p->vel.z = 1 + P_FRandom(9);
	    }

	  // FIXME Hexen lava damage
	  /*
	  if (thing->player && leveltime&31)
	    P_DamageMobj(thing, &LavaInflictor, NULL, 5);
	  */
	  break;

	case FLOOR_SLUDGE:
	  if (smallsplash)
	    {
	      p = mp->SpawnSplash(pos, floorz, SFX_SLUDGE_GLOOP, MT_XSLUDGESPLASH, MT_NONE, false);
	      if (p)
		p->floorclip += SMALLSPLASHCLIP;
	    }
	  else
	    {
	      p = mp->SpawnSplash(pos, floorz, SFX_SLUDGE_GLOOP, MT_XSLUDGESPLASH, MT_XSLUDGECHUNK, false);
	      if (p)
		p->vel.Set(P_SignedFRandom(8), P_SignedFRandom(8), 1 + P_FRandom(8));
	    }
	  break;
	}
    }
  else if (game.mode == gm_heretic)
    {
      switch (floortype)
	{
	case FLOOR_WATER:
	  p = mp->SpawnSplash(pos, floorz, sfx_splash, MT_SPLASHBASE, MT_HSPLASH, false);
	  if (p)
	    p->vel.Set(P_SignedFRandom(8), P_SignedFRandom(8), 2 + P_FRandom(8));
	  break;

	case FLOOR_LAVA:
	  p = mp->SpawnSplash(pos, floorz, sfx_burn, MT_LAVASPLASH, MT_LAVASMOKE, false);
	  if (p)
	    p->vel.z = 1 + P_FRandom(9);
	  break;

	case FLOOR_SLUDGE:
	  p = mp->SpawnSplash(pos, floorz, sfx_splash, MT_SLUDGESPLASH, MT_SLUDGECHUNK, false);
	  if (p)
	  p->vel.Set(P_SignedFRandom(8), P_SignedFRandom(8), 1 + P_FRandom(8));
	  break;
	}
    }
  else if (floortype == FLOOR_WATER)
    mp->SpawnSplash(pos, floorz, sfx_splash, MT_SPLASH);

  return floortype;
}




//==============================================================
//              DActor class implementation
//==============================================================

IMPLEMENT_CLASS(DActor, Actor);


// trick constructor
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


// trick constructor
DActor::DActor(mobjtype_t t)
  : Actor()
{
  type = t;
}


// normal constructor
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

  if (game.skill != sk_nightmare)
    reactiontime = info->reactiontime;

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
  pres = new spritepres_t(info, 0);
}



void BlasterMissileThink();

void DActor::Think()
{
  if (type == MT_BLASTERFX1 || type == MT_MWAND_MISSILE || type == MT_CFLAME_MISSILE)
    {
      BlasterMissileThink();
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

  if (eflags & MFE_REMOVE)
    return; // was removed

  // must have hit something
  if ((oldeflags & MFE_SKULLFLY) && !(eflags & MFE_SKULLFLY))
    SetState(game.mode >= gm_heretic ? info->seestate : info->spawnstate); // TODO depends on monster, not gametype
  
  // must have exploded
  if ((oldflags & MF_MISSILE) && !(flags & MF_MISSILE))
    ExplodeMissile();

  // missiles hitting the floor
  if ((flags & MF_MISSILE) && (eflags & MFE_JUSTHITFLOOR))
    {
      if ((flags2 & MF2_CEILINGHUGGER) || (flags2 & MF2_FLOORHUGGER))
	return;

      if (flags2 & (MF2_FULLBOUNCE | MF2_FLOORBOUNCE))
	{
	  // NOTE: originally ceilingbounce zeroed vz, now it reflects it
	  FloorBounceMissile();
	  return;
	}	

      if (type == MT_HOLY_FX)
	{ // The spirit struck the ground
	  vel.z = 0;
	  HitFloor();
	  return;
	}

      if (!(flags & MF_NOCLIPLINE))
	{
	  // TODO HitFloor();
	  ExplodeMissile();
	}

      return;
    }
  else if (flags2 & MF2_FLOATBOB)
    {
      pos.z += special1; // floating height
    }


  // crashing to ground
  if (info->crashstate && (flags & MF_CORPSE) && (eflags & MFE_JUSTHITFLOOR))
    SetState(info->crashstate);


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

      if (mp->maptic & 31)
	return;

      if (P_Random() > 4)
	return;

      NightmareRespawn();
    }
}


/*
// Returns true if the mobj is still present.
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
	state = &states[S_NULL]; // was state = NULL;
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

  if (state != &states[S_NULL])
    pres->SetFrame(state); // set the sprite frame (if pres is not a sprite, do nothing)

  if (ret && !tics)  // killough 4/9/98: detect state cycles
    CONS_Printf("Warning: State Cycle Detected");
    
  if (!--recursion)
    for ( ; (ns = seenstate[i]) ; i = statenum_t(ns-1))
      seenstate[i] = statenum_t(0);  // killough 4/9/98: erase memory of states
        
  return ret;
}
*/


// DActors are basically finite state machines. This changes the state.
// Returns true if the mobj is still present.
bool DActor::SetState(statenum_t ns, bool call)
{
  do {
    if (ns == S_NULL)
      {
	state = &states[S_NULL]; // was state = NULL;
	Remove(); // does not Think after this
	return false;
      }
        
    state = &states[ns];
    tics = state->tics;

    // Call action functions when the state is set
    if (call && state->action)
      state->action(this); // NOTE that the action function can in turn call SetState...
        
    ns = state->nextstate;
  } while (!tics);

  if (state == &states[S_NULL])
    return false;

  pres->SetFrame(state); // set the sprite frame (if pres is not a sprite, do nothing)
  return true;
}



// The old corpse is removed, a new monster appears at the old one's spawnpoint
void DActor::NightmareRespawn()
{
  mapthing_t *mthing = spawnpoint;
  if (!mthing)
    return; // no spawnpoint, no respawn

  fixed_t  nx, ny, nz;
  nx = mthing->x;
  ny = mthing->y;

  // somthing is occupying it's position?
  if (!TestLocation(nx, ny))
    return; // no respwan (will try again later!)

  // spawn a teleport fog at old spot
  // because of removal of the body?
  DActor *mo = mp->SpawnDActor(pos.x, pos.y, subsector->sector->floorheight + 
			       (game.mode == gm_heretic ? TELEFOGHEIGHT : 0), MT_TFOG);
  // initiate teleport sound
  S_StartSound(mo, sfx_teleport);

  // spawn a teleport fog at the new spot
  subsector_t *ss = mp->R_PointInSubsector(nx, ny);

  mo = mp->SpawnDActor(nx, ny, ss->sector->floorheight +
		   (game.mode == gm_heretic ? TELEFOGHEIGHT : 0) , MT_TFOG);
  S_StartSound(mo, sfx_teleport);

  // spawn it
  if (info->flags & MF_SPAWNCEILING)
    nz = ONCEILINGZ;
  else
    nz = ONFLOORZ;

  // spawn the new monster
  // inherit attributes from deceased one
  mo = mp->SpawnDActor(nx, ny, nz, type);
  mo->spawnpoint = mthing;
  mo->yaw = ANG45 * (mthing->angle/45);

  if (mthing->flags & MTF_AMBUSH)
    mo->flags |= MF_AMBUSH;

  mo->reactiontime = 18;

  // remove the old monster,
  Remove();
}



// send a missile towards another Actor
DActor *DActor::SpawnMissile(Actor *dest, mobjtype_t type)
{
  fixed_t mz = pos.z;

#ifdef PARANOIA
  if (!dest)
    I_Error("P_SpawnMissile : no dest");
#endif
  switch (type)
    {
    case MT_MNTRFX1: // Minotaur swing attack missile
      mz += 40;
      break;
    case MT_MNTRFX2: // Minotaur floor fire missile
      mz = ONFLOORZ +floorclip;
      break;
    case MT_SRCRFX1: // Sorcerer Demon fireball
      mz += 48;
      break;
    case MT_KNIGHTAXE: // Knight normal axe
    case MT_REDAXE: // Knight red power axe
      mz += 36;
      break;
    case MT_CENTAUR_FX:
      mz += 45;
      break;
    case MT_ICEGUY_FX:
      mz += 40;
      break;
    case MT_HOLY_MISSILE:
      mz += 40;
      break;

    default:
      mz += 32;
      break;
    }

  mz -= floorclip;

  DActor *th = mp->SpawnDActor(pos.x, pos.y, mz, type);

  if (th->info->seesound)
    S_StartSound(th, th->info->seesound);

  th->owner = this; // where it came from

  angle_t an = R_PointToAngle2(pos, dest->pos);

  // fuzzy player
  if (dest->flags & MF_SHADOW)
    {
      if (game.mode >= gm_heretic)
	an += P_SignedRandom()<<21; 
      else
	an += P_SignedRandom()<<20;
    }

  th->yaw = an;
  th->Thrust(an, th->info->speed);

  int dist = int(P_AproxDistance(dest->pos.x - pos.x, dest->pos.y - pos.y).Float() / th->info->speed);

  if (dist < 1)
    dist = 1;

  th->vel.z = (dest->pos.z - pos.z) / dist;

  return (th->CheckMissileSpawn()) ? th : NULL;
}



// Moves the missile forward a bit and possibly explodes it right there.
bool DActor::CheckMissileSpawn()
{
  if (game.mode < gm_heretic)
    {
      tics -= P_Random()&3;
      if (tics < 1)
	tics = 1;
    }

  // TODO if very fast, move forward less (ideally just one radius...)
  // move a little forward so an angle can
  // be computed if it immediately explodes
  pos += vel >> 1;

  if (!TryMove(pos.x, pos.y, true))
    {
      ExplodeMissile();
      return false;
    }
  return true;
}



// kaboom.
void DActor::ExplodeMissile()
{
  if (type == MT_WHIRLWIND)
    if (++special2 < 60)
      return;

  vel.Set(0, 0, 0);

  SetState(mobjinfo[type].deathstate);

  if (game.mode < gm_heretic)
    {
      tics -= P_Random()&3;
        
      if (tics < 1)
	tics = 1;
    }

  flags &= ~MF_MISSILE;

  switch (type)
    {
    case MT_SORCBALL1:
    case MT_SORCBALL2:
    case MT_SORCBALL3:
      S_StartSound(this, SFX_SORCERER_BIGBALLEXPLODE); // TODO see if these can be made deathsounds...
      break;
    case MT_SORCFX1:
      S_StartSound(this, SFX_SORCERER_HEADSCREAM);
      break;

    default:
      if (info->deathsound)
	S_StartSound(this, info->deathsound);
      break;
    }
}



/// MF2_FLOORBOUNCE missiles bounce once from floor and immediately explode.
void DActor::FloorBounceMissile()
{
  if (!(flags2 & MF2_FULLBOUNCE))
    {
      vel.z = -vel.z;
      SetState(info->deathstate);
      return;
    }

  // most missiles sink
  if (HitFloor() >= FLOOR_LIQUID)
    {
      switch (type)
	{
	case MT_SORCFX1:
	case MT_SORCBALL1:
	case MT_SORCBALL2:
	case MT_SORCBALL3:
	  break;
	default:
	  Remove();
	  return;
	}
    }

  // lose energy TODO store the Q factor in mobjinfo....
  switch (type)
    {
    case MT_SORCFX1:
      vel.z = -vel.z; // no energy absorbed
      break;
    case MT_SGSHARD1:
    case MT_SGSHARD2:
    case MT_SGSHARD3:
    case MT_SGSHARD4:
    case MT_SGSHARD5:
    case MT_SGSHARD6:
    case MT_SGSHARD7:
    case MT_SGSHARD8:
    case MT_SGSHARD9:
    case MT_SGSHARD0:
      vel.z *= -0.3f;
      if (abs(vel.z) < 0.5f) // TODO into action func
	{
	  SetState(S_NULL);
	  return;
	}
      break;
    default:
      vel.z *= -0.7f;
      break;
    }

  vel.x *= 2.0f/3;
  vel.y *= 2.0f/3;

  if (info->seesound)
    {
      switch (type)
	{
	case MT_SORCBALL1:
	case MT_SORCBALL2:
	case MT_SORCBALL3:
	  if (!args[0])
	    S_StartSound(this, info->seesound);
	  break;
	default:
	  S_StartSound(this, info->seesound);
	  break;
	}
    }

}
