// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
#include "g_save.h"
#include "p_enemy.h" // #defines

#include "hu_stuff.h"
#include "p_maputl.h"

#include "p_setup.h"    //levelflats to test if mobj in water sector
#include "r_main.h"
#include "r_state.h"

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


static const fixed_t FloatBobOffsets[64] =
{
  0, 51389, 102283, 152192,
  200636, 247147, 291278, 332604,
  370727, 405280, 435929, 462380,
  484378, 501712, 514213, 521763,
  524287, 521763, 514213, 501712,
  484378, 462380, 435929, 405280,
  370727, 332604, 291278, 247147,
  200636, 152192, 102283, 51389,
  -1, -51390, -102284, -152193,
  -200637, -247148, -291279, -332605,
  -370728, -405281, -435930, -462381,
  -484380, -501713, -514215, -521764,
  -524288, -521764, -514214, -501713,
  -484379, -462381, -435930, -405280,
  -370728, -332605, -291279, -247148,
  -200637, -152193, -102284, -51389
};

int Actor::Serialize(LArchive & a)
{ 
  // FIXME the entire serialization system!
  /* crap?
    // not a monster nor a picable item so don't save it
    if( (((flags & (MF_COUNTKILL | MF_PICKUP | MF_SHOOTABLE )) == 0)
    && (flags & MF_MISSILE)
    && (info->doomednum !=-1) )
    || (type == MT_BLOOD) )
    continue;
  */


  /*
  ULONG       diff;
  if (spawnpoint && (info->doomednum !=-1)) {
    // spawnpoint is not moddified but we must save it since it is a indentifier
    diff = MD_SPAWNPOINT;
    
    if((x != spawnpoint->x << FRACBITS) ||
       (y != spawnpoint->y << FRACBITS) ||
       (angle != (unsigned)(ANG45 * (spawnpoint->angle/45))) ) diff |= MD_POS;
    if(info->doomednum != spawnpoint->type)        diff |= MD_TYPE;
  }
  else
    {
      // not a map spawned thing so make it from scratch
      diff = MD_POS | MD_TYPE;
    }

  // not the default but the most probable
  if( z             != floorz)                       diff |= MD_Z;
  if((px != 0)||(py != 0)||(pz != 0) )   diff |= MD_MOM;
  if( radius        != info->radius       )          diff |= MD_RADIUS;
  if( height        != info->height       )          diff |= MD_HEIGHT;
  if( flags         != info->flags        )          diff |= MD_FLAGS;
  if( flags2        != info->flags2       )          diff |= MD_FLAGS2;
  if( health        != info->spawnhealth  )          diff |= MD_HEALTH;
  if( reactiontime  != info->reactiontime )          diff |= MD_RTIME;
  if( state-states  != info->spawnstate   )          diff |= MD_STATE;
  if( tics          != state->tics        )          diff |= MD_TICS;
  if( sprite        != state->sprite      )          diff |= MD_SPRITE;
  if( frame         != state->frame       )          diff |= MD_FRAME;
  if( eflags        )                                      diff |= MD_EFLAGS;
  if( player        )                                      diff |= MD_PLAYER;

  if( movedir       )                                      diff |= MD_MOVEDIR;
  if( movecount     )                                      diff |= MD_MOVECOUNT;
  if( threshold     )                                      diff |= MD_THRESHOLD;
  if( lastlook      != -1 )                                diff |= MD_LASTLOOK;
  if( target        )                                      diff |= MD_TARGET;
  if( tracer        )                                      diff |= MD_TRACER;
  if( friction      !=ORIG_FRICTION             )          diff |= MD_FRICTION;
  if( movefactor    !=ORIG_FRICTION_FACTOR      )          diff |= MD_MOVEFACTOR;
  if( special1      )                                      diff |= MD_SPECIAL1;
  if( special2      )                                      diff |= MD_SPECIAL2;

  PADSAVEP();
  *save_p++ = tc_mobj;
  WRITEULONG(save_p, diff);
  // save pointer, at load time we will search this pointer to reinitilize pointers
  WRITEULONG(save_p, (ULONG)mobj);

  if( diff & MD_SPAWNPOINT )   WRITESHORT(save_p, spawnpoint-mapthings);
  if( diff & MD_TYPE       )   WRITEULONG(save_p, type);
  if( diff & MD_POS        ) { WRITEFIXED(save_p, x);
  WRITEFIXED(save_p, y);
  WRITEANGLE(save_p, angle);     }
  if( diff & MD_Z          )   WRITEFIXED(save_p, z);
  if( diff & MD_MOM        ) { WRITEFIXED(save_p, px);
  WRITEFIXED(save_p, py);
  WRITEFIXED(save_p, pz);      }
  if( diff & MD_RADIUS     )   WRITEFIXED(save_p, radius      );
  if( diff & MD_HEIGHT     )   WRITEFIXED(save_p, height      );
  if( diff & MD_FLAGS      )   WRITELONG (save_p, flags       );
  if( diff & MD_FLAGS2     )   WRITELONG (save_p, flags2      );
  if( diff & MD_HEALTH     )   WRITELONG (save_p, health      );
  if( diff & MD_RTIME      )   WRITELONG (save_p, reactiontime);
  if( diff & MD_STATE      )  WRITEUSHORT(save_p, state-states);
  if( diff & MD_TICS       )   WRITELONG (save_p, tics        );
  if( diff & MD_SPRITE     )  WRITEUSHORT(save_p, sprite      );
  if( diff & MD_FRAME      )   WRITEULONG(save_p, frame       );
  if( diff & MD_EFLAGS     )   WRITEULONG(save_p, eflags      );
  if( diff & MD_PLAYER     )   *save_p++ = player-players;
  if( diff & MD_MOVEDIR    )   WRITELONG (save_p, movedir     );
  if( diff & MD_MOVECOUNT  )   WRITELONG (save_p, movecount   );
  if( diff & MD_THRESHOLD  )   WRITELONG (save_p, threshold   );
  if( diff & MD_LASTLOOK   )   WRITELONG (save_p, lastlook    );
  if( diff & MD_TARGET     )   WRITEULONG(save_p, (ULONG)target      );
  if( diff & MD_TRACER     )   WRITEULONG(save_p, (ULONG)tracer      );
  if( diff & MD_FRICTION   )   WRITELONG (save_p, friction    );
  if( diff & MD_MOVEFACTOR )   WRITELONG (save_p, movefactor  );
  if( diff & MD_SPECIAL1   )   WRITELONG (save_p, special1    );
  if( diff & MD_SPECIAL2   )   WRITELONG (save_p, special2    );
  */
  return 0;
}

int DActor::Serialize(LArchive & a)
{
  //Actor::Serialize(a);

  a << byte(type);
  
  return 0;
}

//----------------------------------------------
// trick constructors
//

Actor::Actor()
{
  snext = sprev = bnext = bprev = NULL;
}


DActor::DActor(mobjtype_t t)
  : Actor()
{
  type = t;
}

//----------------------------------------------
// was P_SpawnMobj, see Map::SpawnActor
//
Actor::Actor(fixed_t nx, fixed_t ny, fixed_t nz)
  : Thinker()
{
  // note! here Map *mp is not yet set! This means you can't call functions such as
  // SetPosition that have something to do with a map.
  snext = sprev = bnext = bprev = NULL;
  x = nx;
  y = ny;
  z = nz;

  touching_sectorlist = NULL;
  friction = ORIG_FRICTION;
  movefactor = ORIG_FRICTION_FACTOR;

  //FIXME temporary exploding imp
  sprite = spritenum_t(0);
  frame = 15;
}


DActor::DActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t)
  : Actor(nx, ny, nz)
{
  type = t;
  info = &mobjinfo[t];
  mass = info->mass;
  radius = info->radius;
  height = info->height;
  flags  = info->flags;
  flags2 = info->flags2;
  health = info->spawnhealth;

  if (game.skill != sk_nightmare)
    reactiontime = info->reactiontime;

  lastlook = -1;  // stuff moved in P_enemy.P_LookForPlayer

  // do not set the state with SetState,
  // because action routines can not be called yet
  state = &states[info->spawnstate];
  tics = state->tics;
  sprite = state->sprite;
  frame = state->frame; // FF_FRAMEMASK for frame, and other bits..
}


//----------------------------------------------
// was P_RemoveMobj
//
void Actor::Remove()
{
  extern msecnode_t *sector_list;
  // lazy deallocation/destruction: memory freed in P_RemoveThinker

  if (mp == NULL)
    {
      // no Map, must be in transit between maps/levels
      delete this;
      return;
    }

  if ((flags & MF_SPECIAL) && !(flags & MF_DROPPED) && !(flags & MF_NORESPAWN))
    {
      mp->itemrespawnqueue.push_back(spawnpoint);
      mp->itemrespawntime.push_back(mp->maptic);
    }

  // unlink from sector and block lists
  UnsetPosition();

  //SoM: 4/7/2000: Remove touching_sectorlist from mobj.
  if (sector_list)
    {
      P_DelSeclist(sector_list);
      sector_list = NULL;
    }

  // stop any playing sound
  S.Stop3DSound(this);

  // free block
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

  /*
    - is it a ppawn
    - checkwater
    - jos on pxy tai MF_SKULLFLY, xymovement
    - floatbob
    - z movement, 3 koodia, MF_ONGROUND, MF2_PASSMOBJ, MF2_ONMOBJ
    - state update
    - nightmare respawn
   */
  int oldflags = flags;
  
  Actor::Think();

  // must have hit something
  if ((oldflags & MF_SKULLFLY) && !(flags & MF_SKULLFLY))
    SetState(game.mode == gm_heretic ? info->seestate : info->spawnstate);
  
  // must have exploded
  if ((oldflags & MF_MISSILE) && !(flags & MF_MISSILE))
    ExplodeMissile();

  // missiles hitting the floor
  if ((flags & MF_MISSILE) && (eflags & MF_JUSTHITFLOOR))
    {
      if (flags2 & MF2_FLOORBOUNCE)
	{
	  FloorBounceMissile();
	}
      else if (type == MT_MNTRFX2)
	{ // Minotaur floor fire can go up steps
	}
      else if (!(flags & MF_NOCLIP))
	{
	  ExplodeMissile();
	}
    }

  // crashing to ground
  if (info->crashstate && (flags & MF_CORPSE) && (eflags & MF_JUSTHITFLOOR))
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
  else if (flags & MF_SKULLFLY)
    {
      // the skull slammed into something
      flags &= ~MF_SKULLFLY;
      pz = 0;
    }

  // Z movement
  eflags &= ~MF_JUSTHITFLOOR;

  if (flags2 & MF2_FLOATBOB)
    {
      // Floating item bobbing motion (maybe move to DActor::Think ?)
      z = floorz + FloatBobOffsets[(health++) & 63];
    }
  else if (!(eflags & MF_ONGROUND) || (z != floorz) || pz)
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

  // SoM: Floorhuggers stay on the floor allways...
  // BP: tested here but never set ?!
  if (flags & MF_FLOORHUGGER)
    {
      z = floorz;
    }
}


// was P_GetMoveFactor()
// returns the value by which the x,y
// movements are multiplied to add to player movement.

int Actor::GetMoveFactor()
{
  extern int variable_friction;
  int mf = ORIG_FRICTION_FACTOR;

  // If the floor is icy or muddy, it's harder to get moving. This is where
  // the different friction factors are applied to 'trying to move'. In
  // Actor::XYFriction, the friction factors are applied as you coast and slow down.

  if (boomsupport && variable_friction &&
      !(flags & (MF_NOGRAVITY | MF_NOCLIP)))
    {
      int frict = friction;
      if (frict == ORIG_FRICTION)            // normal floor
	;
      else if (frict > ORIG_FRICTION)        // ice
	{
          mf = movefactor;
          movefactor = ORIG_FRICTION_FACTOR;  // reset
	}
      else                                      // sludge
	{
          // phares 3/11/98: you start off slowly, then increase as
          // you get better footing
          
          int momentum = P_AproxDistance(px,py);
          mf = movefactor;
          if (momentum > MORE_FRICTION_MOMENTUM<<2)
	    mf <<= 3;
          
          else if (momentum > MORE_FRICTION_MOMENTUM<<1)
	    mf <<= 2;
          
          else if (momentum > MORE_FRICTION_MOMENTUM)
	    mf <<= 1;
          
          movefactor = ORIG_FRICTION_FACTOR;  // reset
	}
    }
  return mf;
}



//-----------------------------------------
// was P_XYMovement
//

void Actor::XYMovement()
{
  extern line_t *ceilingline;
  extern int skyflatnum;

  //when up against walls
  static int windTab[3] = {2048*5, 2048*10, 2048*25};

  // stupid heretic wind
  if (flags2 & MF2_WINDTHRUST)
    {
      int special = subsector->sector->special;
      switch(special)
        {
        case 40: case 41: case 42: // Wind_East
	  Thrust(0, windTab[special-40]);
	  break;
        case 43: case 44: case 45: // Wind_North
	  Thrust(ANG90, windTab[special-43]);
	  break;
        case 46: case 47: case 48: // Wind_South
	  Thrust(ANG270, windTab[special-46]);
	  break;
        case 49: case 50: case 51: // Wind_West
	  Thrust(ANG180, windTab[special-49]);
	  break;
        }
    }

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
  if (flags & (MF_MISSILE | MF_SKULLFLY))
    return;

  XYFriction(oldx, oldy);
}

//----------------------------------------------------------------------------
// was P_XYFriction
//
// adds friction on the xy plane

#define STOPSPEED            (0x1000/NEWTICRATERATIO)
#define FRICTION_LOW          0xf900  // 0.973
#define FRICTION_FLY          0xeb00
#define FRICTION              0xe800  // 0.90625
#define FRICTION_UNDERWATER  (FRICTION*3/4)

void Actor::XYFriction(fixed_t oldx, fixed_t oldy)
{
  // slow down in water, not too much for playability issues
  if (eflags & MF_UNDERWATER)
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
      if (px > FRACUNIT/4 || px < -FRACUNIT/4
	  || py > FRACUNIT/4 || py < -FRACUNIT/4)
        {
	  if (floorz != subsector->sector->floorheight)
	    return;
        }
    }

  if (px > -STOPSPEED && px < STOPSPEED && py > -STOPSPEED && py < STOPSPEED)
    {
      px = 0;
      py = 0;
    }
  else
    {
      if (game.mode == gm_heretic)
        {
	  if ((flags2 & MF2_FLY) && (z > floorz)
	     && !(flags2 & MF2_ONMOBJ))
            {
	      px = FixedMul(px, FRICTION_FLY);
	      py = FixedMul(py, FRICTION_FLY);
            }
	  else if (subsector->sector->special == 15) // Friction_Low
            {
	      px = FixedMul(px, FRICTION_LOW);
	      py = FixedMul(py, FRICTION_LOW);
            }
	  else
            {
	      px = FixedMul(px, FRICTION);
	      py = FixedMul(py, FRICTION);
            }
        }
      else
	{
	  //SoM: 3/28/2000: Use boom friction.
	  if ((oldx == x) && (oldy == y)) // Did you go anywhere?
	    {
	      px = FixedMul(px, ORIG_FRICTION);
	      py = FixedMul(py, ORIG_FRICTION);
	    }
	  else
	    {
	      px = FixedMul(px, friction);
	      py = FixedMul(py, friction);
	    }
	  friction = ORIG_FRICTION;
	}
    }
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
      if (!(flags & MF_SKULLFLY)
	  && !(flags & MF_INFLOAT))
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
	  eflags |= MF_JUSTHITFLOOR;
	  return;
	}
        
      // Did we actually fall to the ground?
      if (z - pz > floorz)
	{
	  HitFloor();
	  eflags |= MF_JUSTHITFLOOR;
	}

      z = floorz;

      if (flags & MF_SKULLFLY)
        {
	  // the skull slammed into something
	  pz = -pz;
        }
      else
	pz = 0;

    }
  else if (flags2 & MF2_LOGRAV)
    {
      if (pz == 0)
	pz = -(cv_gravity.value>>3)*2;
      else
	pz -= cv_gravity.value>>3;
    }
  else if (!(flags & MF_NOGRAVITY)) // Gravity!
    {
      fixed_t gravityadd = -cv_gravity.value/NEWTICRATERATIO;

      // if waist under water, slow down the fall
      if (eflags & MF_UNDERWATER)
	{
	  if (eflags & MF_SWIMMING)
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
	if (flags & MF_SKULLFLY)
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

	  eflags |= MF_JUSTHITFLOOR; // with missiles works also with ceiling hits
	  return;
        }
    }

  // no z friction for missiles and skulls
  if (flags & (MF_MISSILE | MF_SKULLFLY))
    return;

  // z friction in water
  if ((eflags & MF_TOUCHWATER) || (eflags & MF_UNDERWATER)) 
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
// check for water in the sector, set MF_TOUCHWATER and MF_UNDERWATER
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
	eflags |= MF_TOUCHWATER;
      else
	eflags &= ~MF_TOUCHWATER;

      if (z+(height>>1) <= fz)
	eflags |= MF_UNDERWATER;
      else
	eflags &= ~MF_UNDERWATER;
    }
  else if (sector->ffloors)
    {
      ffloor_t*  rover;

      eflags &= ~(MF_UNDERWATER|MF_TOUCHWATER);

      for (rover = sector->ffloors; rover; rover = rover->next)
	{
	  if (!(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
	    continue;
	  if (*rover->topheight <= z || *rover->bottomheight > (z + (height >> 1)))
	    continue;

	  if (z + height > *rover->topheight)
            eflags |= MF_TOUCHWATER;
	  else
            eflags &= ~MF_TOUCHWATER;

	  if (z + (height >> 1) < *rover->topheight)
            eflags |= MF_UNDERWATER;
	  else
            eflags &= ~MF_UNDERWATER;

	  if (!(oldeflags & (MF_TOUCHWATER|MF_UNDERWATER))
	      && (eflags & (MF_TOUCHWATER|MF_UNDERWATER))) // && game.mode != gm_heretic
            mp->SpawnSplash(this, *rover->topheight);
	}
      return;
    }
  else
    eflags &= ~(MF_UNDERWATER|MF_TOUCHWATER);

  /*
    if( (eflags ^ oldeflags) & MF_TOUCHWATER)
    CONS_Printf("touchewater %d\n",eflags & MF_TOUCHWATER ? 1 : 0);
    if( (eflags ^ oldeflags) & MF_UNDERWATER)
    CONS_Printf("underwater %d\n",eflags & MF_UNDERWATER ? 1 : 0);
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
bool DActor::SetState(statenum_t ns, bool call = true)
{
  state_t *st;
    
  //remember states seen, to detect cycles:    
  static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
  static int recursion;                       // detects recursion

  statenum_t *seenstate = seenstate_tab;      // pointer to table

  statenum_t i = ns;                       // initial state
  bool ret = true;                         // return value
  statenum_t tempstate[NUMSTATES];            // for use with recursion
    
  if (recursion++)                            // if recursion detected,
    memset(seenstate = tempstate,0,sizeof tempstate); // clear state table
    
  do {
    if (ns == S_NULL)
      {
	state = (state_t *)S_NULL;
	Remove();
	ret = false;
	break;                 // killough 4/9/98
      }
        
    st = &states[ns];
    state = st;
    tics = st->tics;
    sprite = st->sprite;
    frame = st->frame;
    
    // Modified handling.
    // Call action functions when the state is set
    if (call == true && st->action.acp1)
      st->action.acp1(this);
        
    seenstate[ns] = statenum_t(1 + st->nextstate);   // killough 4/9/98
        
    ns = st->nextstate;
  } while (!tics && !seenstate[ns]);   // killough 4/9/98
    
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
    
  if(state == S_NULL)
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
  if(!dest)
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
  if (flags2 & MF2_FEETARECLIPPED)
    mz -= FOOTCLIPSIZE;

  DActor *th = mp->SpawnDActor(x, y, mz, type);

  if (th->info->seesound)
    S_StartSound(th, th->info->seesound);

  th->owner = this; // where it came from

  angle_t an = R_PointToAngle2(x, y, dest->x, dest->y);

  // fuzzy player
  if (dest->flags & MF_SHADOW)
    {
      if( game.mode == gm_heretic )
	an += P_SignedRandom()<<21; 
      else
	an += P_SignedRandom()<<20;
    }

  th->angle = an;
  an >>= ANGLETOFINESHIFT;
  th->px = FixedMul (th->info->speed, finecosine[an]);
  th->py = FixedMul (th->info->speed, finesine[an]);

  int dist = P_AproxDistance (dest->x - x, dest->y - y);
  dist = dist / th->info->speed;

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
  if (game.mode != gm_heretic)
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
