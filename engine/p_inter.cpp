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
// Revision 1.19  2003/11/12 11:07:22  smite-meister
// Serialization done. Map progression.
//
// Revision 1.18  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.17  2003/05/30 13:34:45  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.16  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.15  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.14  2003/04/24 20:30:08  hurdler
// Remove lots of compiling warnings
//
// Revision 1.13  2003/04/08 09:46:05  smite-meister
// Bugfixes
//
// Revision 1.12  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.11  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.10  2003/03/15 20:07:16  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.9  2003/03/08 16:07:08  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.8  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.7  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.6  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.5  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:11:40  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:17:59  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//  Handling Actor interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "d_netcmd.h" // cvars
#include "i_system.h"   //I_Tactile currently has no effect
#include "am_map.h"
#include "dstrings.h"
#include "m_random.h"
#include "g_damage.h"

#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_enemy.h"
#include "p_heretic.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_sprite.h"
#include "r_main.h"
#include "tables.h"

#include "hu_stuff.h" // HUD

#define BONUSADD        6


consvar_t cv_fragsweaponfalling = {"fragsweaponfalling"   ,"0",CV_SAVE,CV_OnOff};

// added 4-2-98 (Boris) for dehacked patch
// (i don't like that but do you see another solution ?)
//int max_health = 100;


// was P_DeathMessages
// Actor::Killed is called when PlayerPawn dies.
// It returns the proper death message and updates the score.
void Actor::Killed(PlayerPawn *victim, Actor *inflictor)
{
  CONS_Printf("%s is killed by an inanimate object\n", victim->player->name.c_str());
}

void DActor::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // monster killer
  // show death message only if it concerns the console players
  if (victim->player != consoleplayer && victim->player != consoleplayer2)
    return;

  char *str = NULL;
  switch (type)
    {
    case MT_BARREL:    str = text[DEATHMSG_BARREL]; break;
    case MT_POSSESSED: str = text[DEATHMSG_POSSESSED]; break;
    case MT_SHOTGUY:   str = text[DEATHMSG_SHOTGUY];   break;
    case MT_VILE:      str = text[DEATHMSG_VILE];      break;
    case MT_FATSO:     str = text[DEATHMSG_FATSO];     break;
    case MT_CHAINGUY:  str = text[DEATHMSG_CHAINGUY];  break;
    case MT_TROOP:     str = text[DEATHMSG_TROOP];     break;
    case MT_SERGEANT:  str = text[DEATHMSG_SERGEANT];  break;
    case MT_SHADOWS:   str = text[DEATHMSG_SHADOWS];   break;
    case MT_HEAD:      str = text[DEATHMSG_HEAD];      break;
    case MT_BRUISER:   str = text[DEATHMSG_BRUISER];   break;
    case MT_UNDEAD:    str = text[DEATHMSG_UNDEAD];    break;
    case MT_KNIGHT:    str = text[DEATHMSG_KNIGHT];    break;
    case MT_SKULL:     str = text[DEATHMSG_SKULL];     break;
    case MT_SPIDER:    str = text[DEATHMSG_SPIDER];    break;
    case MT_BABY:      str = text[DEATHMSG_BABY];      break;
    case MT_CYBORG:    str = text[DEATHMSG_CYBORG];    break;
    case MT_PAIN:      str = text[DEATHMSG_PAIN];      break;
    case MT_WOLFSS:    str = text[DEATHMSG_WOLFSS];    break;
    default:           str = text[DEATHMSG_DEAD];      break;
    }

  CONS_Printf(str, victim->player->name.c_str());
}


void PlayerPawn::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // player killer
  game.UpdateScore(player, victim->player);

  if (game.mode == gm_heretic)
    {
      if (player == displayplayer || player == displayplayer2)
	S_StartAmbSound(sfx_gfrag);

      // Make a super chicken
      if (morphTics)
	GivePower(pw_weaponlevel2);
    }

  // show death message only if it concerns the console players
  if (player != consoleplayer && player != consoleplayer2 &&
      victim->player != consoleplayer && victim->player != consoleplayer2)
    return;

  char *str = NULL;

  if (player == victim->player)
    {
      CONS_Printf(text[DEATHMSG_SUICIDE], player->name.c_str());
      // FIXME when console is rewritten to accept << >>
      //if (cv_splitscreen.value)
      // console << "\4" << t->player->name << text[DEATHMSG_SUICIDE];
      return;
    }

  if (victim->health < -9000) // telefrag !
    str = text[DEATHMSG_TELEFRAG];
  else
    {
      int w = -1;
      if (inflictor && (inflictor->Type() == Thinker::tt_dactor))
	{
	  DActor *inf = (DActor *)inflictor;
	  switch (inf->type)
	    {
	    case MT_BARREL:
	      w = wp_barrel;
	      break;
	    case MT_ROCKET   :
	      w = wp_missile;
	      break;
	    case MT_PLASMA   :
	      w = wp_plasma;
	      break;
	    case MT_EXTRABFG :
	    case MT_BFG      :
	      w = wp_bfg;
	      break;
	    default :
	      w = readyweapon;
	      break;
	    }
	}

      switch(w)
	{
	case wp_fist:
	  str = text[DEATHMSG_FIST];
	  break;
	case wp_pistol:
	  str = text[DEATHMSG_GUN];
	  break;
	case wp_shotgun:
	  str = text[DEATHMSG_SHOTGUN];
	  break;
	case wp_chaingun:
	  str = text[DEATHMSG_MACHGUN];
	  break;
	case wp_missile:
	  str = text[DEATHMSG_ROCKET];
	  if (victim->health < -victim->maxhealth)
	    str = text[DEATHMSG_GIBROCKET];
	  break;
	case wp_plasma:
	  str = text[DEATHMSG_PLASMA];
	  break;
	case wp_bfg:
	  str = text[DEATHMSG_BFGBALL];
	  break;
	case wp_chainsaw:
	  str = text[DEATHMSG_CHAINSAW];
	  break;
	case wp_supershotgun:
	  str = text[DEATHMSG_SUPSHOTGUN];
	  break;
	case wp_barrel:
	  str = text[DEATHMSG_BARRELFRAG];
	  break;
	default:
	  str = text[DEATHMSG_PLAYUNKNOW];
	  break;
	}
    }
  CONS_Printf(str, victim->player->name.c_str(), player->name.c_str());
  // FIXME when console is rewritten to accept << >>
  //if (cv_splitscreen.value)
  // console << "\4" << str...
}



//======================================================

//---------------------------------------------
// Called when two actors touch one another
// Returns true if any interaction takes place.

bool Actor::Touch(Actor *p)
{

  // missiles can hit other things
  if (flags & MF_MISSILE)
    {
      // Check for passing through a ghost (heretic)
      if ((p->flags & MF_SHADOW) && (flags2 & MF2_THRUGHOST))
	return false;

      // see if it went over / under
      if (z > p->z + p->height)
	return false; // overhead
      if (z + height < p->z)
	return false; // underneath

      if (!(p->flags & MF_SHOOTABLE))
        {
	  // didn't do any damage
	  return (p->flags & MF_SOLID);
        }

      // don't traverse any more
      return true;
    }


  return (flags & MF_SOLID) && (p->flags & MF_SOLID);
}


bool DActor::Touch(Actor *p)
{
  extern int numspechit;
  int damage;

  // check for skulls slamming into things
  if (eflags & MFE_SKULLFLY)
    {
      damage = ((P_Random()%8)+1) * info->damage;

      p->Damage(this, this, damage);

      eflags &= ~MFE_SKULLFLY;
      px = py = pz = 0;

      SetState(game.mode == gm_heretic ? info->seestate : info->spawnstate);

      return true; // stop moving
    }

  // missiles can hit other things
  if (flags & MF_MISSILE)
    {
      // Check for passing through a ghost (heretic)
      if ((p->flags & MF_SHADOW) && (flags2 & MF2_THRUGHOST))
	return false;

      // see if it went over / under
      if (z > p->z + p->height)
	return false; // overhead
      if (z + height < p->z)
	return false; // underneath

      // Don't hit the originator.
      if (p == owner)
	return false;

      // Don't damage the same species as the originator.
      /*
      if (owner && (owner->type == p->type ||
		    (owner->type == MT_KNIGHT  && p->type == MT_BRUISER)||
		    (owner->type == MT_BRUISER && p->type == MT_KNIGHT)))
        {

	  if (p->type != MT_PLAYER)
            {
	      // Explode, but do no damage.
	      // Let players missile other players.
	      return true;
            }
        }
      */

      if (!(p->flags & MF_SHOOTABLE))
        {
	  // didn't do any damage
	  return (p->flags & MF_SOLID);
        }

      // more heretic stuff
      if (flags2 & MF2_RIP)
        {
	  damage = ((P_Random () & 3) + 2) * info->damage;
	  S_StartSound (this, sfx_ripslop);
	  if (p->Damage(this, owner, damage))
            {
	      if (!(p->flags & MF_NOBLOOD))
                { // Ok to spawn some blood
		  mp->SpawnBlood(x, y, z, damage);
		  //P_RipperBlood (this);
                }
            }

	  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
            { // Push thing
	      p->px += px >> 2;
	      p->py += py >> 2;
            }
	  numspechit = 0;
	  return false;
        }

      // damage / explode
      damage = ((P_Random()%8)+1) * info->damage;
      if (p->Damage(this, owner, damage) && !(p->flags & MF_NOBLOOD))
	mp->SpawnBloodSplats(x,y,z, damage, p->px, p->py);

      // don't traverse any more
      return true;
    }

  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
    { // Push thing
      p->px += px >> 2;
      p->py += py >> 2;
    }

  // check for special pickup
  if (p->flags & MF_SPECIAL)
    {
      if (flags & MF_PICKUP)
        {
	  // can remove thing
	  // TODO: some beneficial effects for DActors too?
	  p->Remove();
        }
      return p->flags & MF_SOLID; // most specials are not solid...
    }

  /*
  // check again for special pickup (Why?)
  if (flags & MF_SPECIAL)
    {
      //if (p->flags & MF_PICKUP)
      if (p->Type() == Thinker::tt_ppawn)
        {
	  // can remove thing
	  ((PlayerPawn *)p)->TouchSpecialThing(this);
        }
      return flags & MF_SOLID;
    }
  */

  //if (game.demoversion<112 ||game.demoversion>=132 || !(flags & MF_SOLID))

  return (p->flags & MF_SOLID);

  /*
  //added:22-02-98: added z checking at last
  //SoM: 3/10/2000: Treat noclip things as non-solid!
  if ((p->flags & MF_SOLID) && (flags & MF_SOLID) &&
      !(p->flags & MF_NOCLIP) && !(flags & MF_NOCLIP))
    {
      // pass under
      tmtopz = z + height;

      if (tmtopz < p->z)
        {
	  if (p->z < tmceilingz)
	    tmceilingz = p->z;
	  return false;
        }

      topz = p->z + p->height + FRACUNIT;

      // block only when jumping not high enough,
      // (dont climb max. 24units while already in air)
      // if not in air, let P_TryMove() decide if its not too high
      // FIXME why test player here
      if (Type() == Thinker::tt_ppawn &&
	  z < topz &&
	  z > floorz)  // block while in air
	return true;


      if (topz > tmfloorz)
        {
	  tmfloorz = topz;
	  tmfloorthing = p;       //thing we may stand on
        }

    }
  // not solid not blocked
  return false;
  */
}

bool PlayerPawn::Touch(Actor *p)
{
  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
    {
      // Push thing
      p->px += px >> 2;
      p->py += py >> 2;
    }

  // check for special pickup
  if (p->flags & MF_SPECIAL)
    {
      if (flags & MF_PICKUP)
        {
	  // can remove thing
	  if (p->Type() == Thinker::tt_dactor)
	    {
	      DActor *dp = (DActor *)p;
	      TouchSpecialThing(dp); // this also Removes() the item
	    }
	  return false; // no collision, just possible pickup
        }
    }

  return (p->flags & MF_SOLID);
}

// Gives damage to the Actor. If the damage is "stopped" (absorbed), returns true.
// Damage comes directly from inflictor but is caused by source.
bool Actor::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (!(flags & MF_SHOOTABLE))
    return false;

  // old recoil code
  if (inflictor && !(flags & MF_NOCLIPTHING) && (dtype & dt_oldrecoil)
      && !(inflictor->flags2 & MF2_NODMGTHRUST))      
    {
      fixed_t apx, apy, apz = 0;
      fixed_t thrust;  
      extern consvar_t cv_allowrocketjump;

      angle_t ang = R_PointToAngle2(inflictor->x, inflictor->y, x, y);

      if (game.mode == gm_heretic)
	thrust = damage*(FRACUNIT>>3)*150/(mass+1);
      else
	thrust = damage*(FRACUNIT>>3)*100/(mass+1);

      // sometimes a target shot down might fall off a ledge forwards
      if (damage < 40 && damage > health
	  && (z - inflictor->z) > 64*FRACUNIT && (P_Random() & 1))
        {
	  ang += ANG180;
	  thrust *= 4;
        }

      ang >>= ANGLETOFINESHIFT;

      apx = FixedMul (thrust, finecosine[ang]);
      apy = FixedMul (thrust, finesine[ang]);
      px += apx;
      py += apy;
            
      // pz (do it better for explosions)
      if (!cv_allowrocketjump.value)
	{
	  fixed_t dist, sx, sy, sz;

	  sx = inflictor->x;
	  sy = inflictor->y;
	  sz = inflictor->z;

	  dist = R_PointToDist2(sx, sy, x, y);
                
	  ang = R_PointToAngle2(0, sz, dist, z);
                
	  ang >>= ANGLETOFINESHIFT;
	  apz = FixedMul(thrust, finesine[ang]);
	}
      else
	{
	  // rocket jump code
	  fixed_t delta1 = abs(inflictor->z - z);
	  fixed_t delta2 = abs(inflictor->z - (z + height));
	  apz = (abs(apx) + abs(apy))>>1;
	  
	  if (delta1 >= delta2 && inflictor->pz < 0)
	    apz = -apz;
	}
      pz += apz;
    }
  
  // do the damage
  health -= damage;
  if (health <= 0)
    Die(inflictor, source);

  return true;
}


#define BASETHRESHOLD 100

bool DActor::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (!(flags & MF_SHOOTABLE))
    return false; // shouldn't happen...

  if (dtype & dt_always)
    {
      return Actor::Damage(inflictor, source, damage, dtype);
    }

  if (health <= 0)
    return false;

  if (eflags & MFE_SKULLFLY)
    {
      // Minotaur is invulnerable during charge attack
      if (type == MT_MINOTAUR)
	return false;
      px = py = pz = 0;
    }

  // Special damage types
  if (inflictor && (inflictor->Type() == Thinker::tt_dactor))
    {
      DActor *inf = (DActor *)inflictor;
      switch (inf->type)
        {
        case MT_EGGFX:
	  Morph();
	  return false; // Always return
        case MT_WHIRLWIND:
	  return P_TouchWhirlwind(this);
        case MT_MINOTAUR:
	  if (inflictor->eflags & MFE_SKULLFLY)
            { // Slam only when in charge mode
	      P_MinotaurSlam(inflictor, this);
	      return true;
            }
	  break;
        case MT_MACEFX4: // Death ball
	  if ((flags2 & MF2_BOSS) || type == MT_HHEAD)
	    // Don't allow cheap boss kills
	    break;
	  damage = 10000; // Something's gonna die
	  break;
        case MT_RAINPLR1: // Rain missiles
        case MT_RAINPLR2:
        case MT_RAINPLR3:
        case MT_RAINPLR4:
	  if (flags2 & MF2_BOSS)
            { // Decrease damage for bosses
	      damage = (P_Random()&7)+1;
            }
	  break;
        case MT_HORNRODFX2:
        case MT_PHOENIXFX1:
	  if (type == MT_SORCERER2 && P_Random() < 96)
            { // D'Sparil teleports away
	      DSparilTeleport();
	      return false;
            }
	  break;
        case MT_BLASTERFX1:
        case MT_RIPPER:
	  if (type == MT_HHEAD)
            { // Less damage to Ironlich bosses
	      damage = P_Random()&1;
	      if (!damage)
		return false;
            }
	  break;
        default:
	  break;
        }
    }

  unsigned    ang;
  fixed_t     thrust;  

  // Some close combat weapons should not
  // inflict thrust and push the victim out of reach
  if (inflictor && !(flags & MF_NOCLIPTHING) && (dtype & dt_oldrecoil)
      && !(inflictor->flags2 & MF2_NODMGTHRUST))      
    {
      fixed_t            apx, apy, apz = 0;//SoM: 3/28/2000
      extern consvar_t   cv_allowrocketjump;

      ang = R_PointToAngle2(inflictor->x, inflictor->y, x, y);

      if (game.mode == gm_heretic )
	thrust = damage*(FRACUNIT>>3)*150/info->mass;
      else
	thrust = damage*(FRACUNIT>>3)*100/info->mass;

      // sometimes a target shot down might fall off a ledge forwards
      if (damage < 40 && damage > health
	  && (z - inflictor->z) > 64*FRACUNIT && (P_Random() & 1))
        {
	  ang += ANG180;
	  thrust *= 4;
        }

      ang >>= ANGLETOFINESHIFT;

      apx = FixedMul (thrust, finecosine[ang]);
      apy = FixedMul (thrust, finesine[ang]);
      px += apx;
      py += apy;
            
      // added pz (do it better for missiles explotion)
      if (source && !cv_allowrocketjump.value)
	{
	  fixed_t dist, sx, sy, sz;

	  sx = inflictor->x;
	  sy = inflictor->y;
	  sz = inflictor->z;

	  dist = R_PointToDist2(sx, sy, x, y);
                
	  ang = R_PointToAngle2(0, sz, dist, z);
                
	  ang >>= ANGLETOFINESHIFT;
	  apz = FixedMul (thrust, finesine[ang]);
	}
      else if (cv_allowrocketjump.value)
	{
	  fixed_t delta1 = abs(inflictor->z - z);
	  fixed_t delta2 = abs(inflictor->z - (z + height));
	  apz = (abs(apx) + abs(apy))>>1;
	  
	  if (delta1 >= delta2 && inflictor->pz < 0)
	    apz = -apz;
	}
      pz += apz;
    }

  
  // do the damage
  health -= damage;
  if (health <= 0)
    {
      special1 = damage;

      Die(inflictor, source);
      return true;
    }

  if ((P_Random () < info->painchance)
      && !(eflags & MFE_SKULLFLY || flags & MF_CORPSE))
    {
      eflags |= MFE_JUSTHIT;    // fight back!
      SetState(info->painstate);
    }

  // we're awake now...
  reactiontime = 0;

  // get angry
  if ((!threshold || type == MT_VILE) && source && (source != target))
      //&& source->type != MT_VILE
      //&& !(source->flags2 & MF2_BOSS)
      //&& !(type == MT_SORCERER2 && source->type == MT_WIZARD))
    {
      if (source->Type() == Thinker::tt_dactor)
	{
	  DActor *ds = (DActor *)source;
	  
	  // let's not get angry, after all
	  if ((ds->type == MT_VILE) ||
	      (ds->type == MT_WIZARD && type == MT_SORCERER2) ||
	      (ds->type == MT_MINOTAUR && type == MT_MINOTAUR))
	    return true;
	}

      // if not intent on another player,
      // chase after this one
      target = source;
      threshold = BASETHRESHOLD;
      if (state == &states[info->spawnstate]
	  && info->seestate != S_NULL)
	SetState(info->seestate);
    }

  return true;
}



// was P_KillMobj
//
// source is the killer

void Actor::Die(Actor *inflictor, Actor *source)
{
  extern consvar_t cv_solidcorpse;

  eflags &= ~(MFE_INFLOAT|MFE_SKULLFLY);

  // dead target is no more shootable
  if (!cv_solidcorpse.value)
    {
      flags &= ~(MF_SHOOTABLE | MF_SOLID);
    }

  // scream a corpse :)
  if (flags & MF_CORPSE)
    {
      flags &= ~(MF_SOLID | MF_SHOOTABLE);
      height = 0;
      radius = 0;
      return;
    }

  // if killed by a player
  if (flags & MF_COUNTKILL)
    {
      if (source && source->Type() == Thinker::tt_ppawn)
	{
	  PlayerPawn *s = (PlayerPawn *)source;
	  // count for intermission
	  s->player->kills++;    
	}
      else if (!game.multiplayer)
	{
	  // count all monster deaths,
	  // even those caused by other monsters
	  consoleplayer->kills++;
	}
    }
}


void DActor::Die(Actor *inflictor, Actor *source)
{
  Actor::Die(inflictor, source);

  if (flags & MF_CORPSE)
    {
      if (flags & MF_NOBLOOD)
	Remove();
      else
	{
	  SetState(S_GIBS);
	  S_StartSound(this, s_gibbed); // lets have a neat 'crunch' sound!
	}
      return;
    }

  if (type != MT_SKULL)
    flags &= ~MF_NOGRAVITY;

  //added:22-02-98: remember who exploded the barrel, so that the guy who
  //                shot the barrel which killed another guy, gets the frag!
  //                (source is passed from barrel to barrel also!)
  //                (only for multiplayer fun, does not remember monsters)
  if ((type == MT_BARREL || type == MT_POD) && source)
    owner = source;

  if (((game.mode != gm_heretic && health < -info->spawnhealth)
       ||(game.mode == gm_heretic && health < -(info->spawnhealth>>1)))
      && info->xdeathstate)
    {
      SetState(info->xdeathstate);
    }
  else
    SetState(info->deathstate);

  tics -= P_Random()&3;

  if (tics < 1)
    tics = 1;

  mobjtype_t item;
  // Drop stuff.
  // This determines the kind of object spawned
  // during the death frame of a thing.
  switch (type)
    {
    case MT_WOLFSS:
    case MT_POSSESSED:
      item = MT_CLIP;
      break;

    case MT_SHOTGUY:
      item = MT_SHOTGUN;
      break;

    case MT_CHAINGUY:
      item = MT_CHAINGUN;
      break;

    default:
      return;
    }

  DActor *mo = mp->SpawnDActor(x, y, floorz, item);
  mo->flags |= MF_DROPPED;    // special versions of items
}


//---------------------------------------------

void PlayerPawn::Die(Actor *inflictor, Actor *source)
{
  Actor::Die(inflictor, source);

  // start playing death anim. sequence
  if (health < -maxhealth >> 1)
    pres->SetAnim(presentation_t::Death2);
  else
    pres->SetAnim(presentation_t::Death1);

  if (!source)
    {
      // environment kills
      int w = specialsector;      //see p_spec.c
      char *str;

      if (w == 5)
	str = text[DEATHMSG_HELLSLIME];
      else if (w == 7)
	str = text[DEATHMSG_NUKE];
      else if (w == 16 || w == 4)
	str = text[DEATHMSG_SUPHELLSLIME];
      else
	str = text[DEATHMSG_SPECUNKNOW];

      if (player == consoleplayer || player == consoleplayer2)
	CONS_Printf(str, player->name.c_str());

      // count environment kills against you (you fragged yourself!)
      game.UpdateScore(player, player);
    }
  else
    source->Killed(this, inflictor);

  // dead guy attributes
  flags2 &= ~MF2_FLY;
  powers[pw_flight] = 0;
  powers[pw_weaponlevel2] = 0;
  DropWeapon();  // put weapon away

  player->playerstate = PST_DEAD;

  if (player == consoleplayer)
    {
      // don't die in auto map,
      // switch view prior to dying
      if (automap.active)
	automap.Close();

      // recenter view for next live...
      localaiming = 0;
    }
  if (player == consoleplayer2)
    {
      // recenter view for next live...
      localaiming2 = 0;
    }
}


//
// GET STUFF
//

static int p_sound; // pickupsound
static bool p_remove; // should the stuff be removed?

//
// was P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

bool PlayerPawn::GiveAmmo(ammotype_t at, int count)
{
  static const weapontype_t GetAmmoChange[] =
  {
    wp_chaingun, wp_shotgun, wp_plasma, wp_missile,
    wp_goldwand,
    wp_crossbow,
    wp_blaster,
    wp_skullrod,
    wp_phoenixrod,
    wp_mace
  };

  if (at == am_noammo)
    return false;

  if (at == am_manaboth)
    {
      bool ret = GiveAmmo(am_mana1, count) || GiveAmmo(am_mana2, count);
      return ret;
    }

  if (at < 0 || at >= NUMAMMO)
    {
      CONS_Printf ("\2P_GiveAmmo: bad type %i", at);
      return false;
    }

  if (ammo[at] >= maxammo[at])
    return false;

  if (game.skill == sk_baby || game.skill == sk_nightmare)
    {
      if (game.mode == gm_heretic || game.mode == gm_hexen)
	count += count>>1;
      else
	// give double ammo in trainer mode,
	// you'll need it in nightmare
	count <<= 1;
    }
  int oldammo = ammo[at];
  ammo[at] += count;

  if (ammo[at] > maxammo[at])
    ammo[at] = maxammo[at];

  // If non zero ammo,
  // don't change up weapons,
  // player was lower on purpose.
  if (oldammo)
    return true;

  // We were down to zero,
  // so select a new weapon.
  // Preferences are not user selectable.

  // Boris hack for preferred weapons order...
  if (!player->originalweaponswitch)
    {
      if (ammo[weaponinfo[readyweapon].ammo]
	  < weaponinfo[readyweapon].ammopershoot)
	UseFavoriteWeapon();
      return true;
    }
  else if (game.mode == gm_heretic)
    {
      if ((readyweapon == wp_staff || readyweapon == wp_gauntlets) 
	  && weaponowned[GetAmmoChange[at]])
	pendingweapon = GetAmmoChange[at];
    }
  else switch (at)
    {
    case am_clip:
      if (readyweapon == wp_fist)
        {
	  if (weaponowned[wp_chaingun])
	    pendingweapon = wp_chaingun;
	  else
	    pendingweapon = wp_pistol;
        }
      break;

    case am_shell:
      if (readyweapon == wp_fist
	  || readyweapon == wp_pistol)
        {
	  if (weaponowned[wp_shotgun])
	    pendingweapon = wp_shotgun;
        }
      break;

    case am_cell:
      if (readyweapon == wp_fist
	  || readyweapon == wp_pistol)
        {
	  if (weaponowned[wp_plasma])
	    pendingweapon = wp_plasma;
        }
      break;

    case am_misl:
      if (readyweapon == wp_fist)
        {
	  if (weaponowned[wp_missile])
	    pendingweapon = wp_missile;
        }
    default:
      break;
    }

  return true;
}


//
// was P_GiveWeapon
//
bool PlayerPawn::GiveWeapon(weapontype_t wt, bool dropped)
{
  bool     gaveammo;
  bool     gaveweapon;

  if (game.multiplayer && (cv_deathmatch.value != 2) && !dropped)
    {
      // leave placed weapons forever on net games
      if (weaponowned[wt])
	return false;

      if (displayplayer == player)
        hud.bonuscount += BONUSADD;
      weaponowned[wt] = true;

      if (cv_deathmatch.value)
	GiveAmmo(weaponinfo[wt].ammo, 5*clipammo[weaponinfo[wt].ammo]);
      else
	GiveAmmo(weaponinfo[wt].ammo, weapondata[wt].getammo);

      // Boris hack preferred weapons order...
      if (player->originalweaponswitch
	  || player->favoriteweapon[wt] > player->favoriteweapon[readyweapon])
	pendingweapon = wt;     // do like Doom2 original

      if (player == displayplayer || (cv_splitscreen.value && player == displayplayer2))
	S_StartAmbSound(sfx_wpnup);
      return false;
    }

  if (weaponinfo[wt].ammo != am_noammo)
    {
      // give one clip with a dropped weapon,
      // two clips with a found weapon
      if (dropped)
	gaveammo = GiveAmmo(weaponinfo[wt].ammo, clipammo[weaponinfo[wt].ammo]);
      else
	gaveammo = GiveAmmo(weaponinfo[wt].ammo, weapondata[wt].getammo);
    }
  else
    gaveammo = false;

  if (weaponowned[wt])
    gaveweapon = false;
  else
    {
      gaveweapon = true;
      weaponowned[wt] = true;
      if (player->originalweaponswitch
	  || player->favoriteweapon[wt] > player->favoriteweapon[readyweapon])
	pendingweapon = wt;    // Doom2 original stuff
    }

  p_sound = Actor::s_weaponpickup;
  return (gaveweapon || gaveammo);
}


int green_armor_class, blue_armor_class, soul_health, mega_health;

#define NUMCLASSES 5

static int ArmorIncrement[NUMCLASSES][NUMARMOR] =
{
  { 0, 0, 0, 0, 0 },
  { 0, 25, 20, 15, 5 },
  { 0, 10, 25, 5, 20 },
  { 0, 5, 15, 10, 25 },
  { 0, 0, 0, 0, 0 }
};

int MaxArmor[NUMCLASSES] = { 200, 100, 90, 80, 5 };

//
// was P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
bool PlayerPawn::GiveArmor(armortype_t type, float factor, int points)
{
  // Kludgy mess. The correct way would be making each pickup-item a separate class
  // with a Give method... same thing with weapons and artifacts
  if (factor > 0)
    {
      // new piece of armor
      if (points < 0) // means use standard Hexen armor increments
	points = ArmorIncrement[pclass][type];

      if (armorpoints[type] >= points)
	return false; // don't pick up

      armorfactor[type] = factor;
      armorpoints[type] = points;
    }
  else
    {
      // negative factor means bonus to current armor
      int i, total = int(100 * toughness);
      for (i = 0; i < NUMARMOR; i++)
	total += armorpoints[i];

      if (total >= MaxArmor[pclass])
	return false;

      if (armorfactor[type] < -factor)
	armorfactor[type] = -factor;
      armorpoints[type] += points;
    }

  return true;
}


//
// P_GiveCard
//
bool PlayerPawn::GiveKey(keycard_t k)
{
  if (keycards & k)
    return false;

  keycards |= k;

  int i, j = k;
  for (i = -1; j; i++)
    j >>= 1; // count the key number
    
  player->message = text[TXT_KEY_STEEL + i];

  if (displayplayer == player)
    hud.bonuscount = BONUSADD;
  p_sound = Actor::s_keypickup;

  return true;
}


// Boris stuff : dehacked patches hack
int maxsoul=200;


//---------------------------------------------------------------------------
//
// Removes the MF_SPECIAL flag, and initiates the artifact pickup
// animation. The artifact is restored after a number of tics.
//
//---------------------------------------------------------------------------

static void SetDormantArtifact(DActor *arti)
{
  arti->flags &= ~MF_SPECIAL;
  if (cv_deathmatch.value && !(arti->flags & MF_DROPPED))
    {
      // respawn delay
      if (arti->type == MT_XARTIINVULNERABILITY)
	arti->SetState(S_DORMANTARTI3_1);
      //else if (arti->type == MT_ARTIINVISIBILITY) 
      else if (arti->type == MT_SUMMONMAULATOR || arti->type == MT_XARTIFLY)
	arti->SetState(S_DORMANTARTI2_1);
      else
	arti->SetState(S_DORMANTARTI1_1);
    }
  else
    arti->SetState(S_DEADARTI1); // Don't respawn

  S_StartSound(arti, sfx_artiup);
}

//---------------------------------------------------------------------------
// was P_GiveArtifact
//
// Returns true if artifact accepted.

bool PlayerPawn::GiveArtifact(artitype_t arti, DActor *from)
{
  if (arti >= NUMARTIFACTS || arti <= arti_none)
    return false;

  vector<inventory_t>::iterator i = inventory.begin();

  // find the right slot
  while (i < inventory.end() && i->type != arti)
    i++;

  if (i == inventory.end())
    // give one artifact
    inventory.push_back(inventory_t(arti, 1));
  else
    {
      // player already has some of these
      if (i->count >= MAXARTECONT)
	// Player already has 16 of this item
	return false;
      
      i->count++; // one more
    }

  p_remove = false;

  if (from && (from->flags & MF_COUNTITEM))
    player->items++;

  int j;
  if (arti < arti_firstpuzzitem)
    {
      j = TXT_ARTIINVULNERABILITY_NUM - 1 + arti;
      if (from->type == MT_XARTIINVULNERABILITY)
	j = TXT_XARTIINVULNERABILITY;
      player->SetMessage(text[TXT_ARTIINVULNERABILITY_NUM - 1 + arti], false);
      SetDormantArtifact(from);
      p_sound = Actor::s_artipickup;
    }
  else
    {
      // Puzzle item
      j = TXT_ARTIPUZZSKULL - 1 + arti;
      if (arti >= arti_puzzgear1)
	j = TXT_ARTIPUZZGEAR;
      player->SetMessage(text[j], true);
      SetDormantArtifact(from);
      /*
      if (!game.multiplayer || deathmatch)
        // Remove puzzle items if not cooperative netplay
        P_RemoveMobj(artifact);
      */
    }

  return true;
}


//---------------------------------------------------------------------------
//
// PROC A_RestoreArtifact
//
//---------------------------------------------------------------------------

void A_RestoreArtifact(DActor *arti)
{
  arti->flags |= MF_SPECIAL;
  arti->SetState(arti->info->spawnstate);
  S_StartSound(arti, Actor::s_respawn);
}

//----------------------------------------------------------------------------
//
// PROC P_HideSpecialThing
//
//----------------------------------------------------------------------------

void P_HideSpecialThing(DActor *thing)
{
  thing->flags &= ~MF_SPECIAL;
  thing->flags2 |= MF2_DONTDRAW;
  thing->SetState(S_HIDESPECIAL1);
}

//---------------------------------------------------------------------------
//
// PROC A_RestoreSpecialThing1
//
// Make a special thing visible again.
//
//---------------------------------------------------------------------------

void A_RestoreSpecialThing1(DActor *thing)
{
  if (thing->type == MT_WMACE)
    { // Do random mace placement
      thing->mp->RepositionMace(thing);
    }
  thing->flags2 &= ~MF2_DONTDRAW;
  S_StartSound(thing, Actor::s_respawn);
}

//---------------------------------------------------------------------------
//
// PROC A_RestoreSpecialThing2
//
//---------------------------------------------------------------------------

void A_RestoreSpecialThing2(DActor *thing)
{
  thing->flags |= MF_SPECIAL;
  thing->SetState(thing->info->spawnstate);
}


//
// was P_TouchSpecialThing
//
void PlayerPawn::TouchSpecialThing(DActor *special)
{                  
  // Dead thing touching.
  // Can happen with a sliding player corpse.
  if (health <= 0 || flags & MF_CORPSE)
    return;

  p_remove = true; // should the item be removed from map?
  p_sound = s_pickup;

  // Identify item
  switch (special->type)
    {
    case MT_ARMOR_1:
      if (!GiveArmor(armor_armor, 3.0, -1))
	return;
      player->message = text[TXT_ARMOR1];
      break;
    case MT_ARMOR_2:
      if(!GiveArmor(armor_shield, 3.0, -1))
	return;
      player->message = text[TXT_ARMOR2];
      break;
    case MT_ARMOR_3:
      if(!GiveArmor(armor_helmet, 3.0, -1))
	return;
      player->message = text[TXT_ARMOR3];
      break;
    case MT_ARMOR_4:
      if(!GiveArmor(armor_amulet, 3.0, -1))
	return;
      player->message = text[TXT_ARMOR4];
      break;

    case MT_ITEMSHIELD1:
    case MT_GREENARMOR:
      if (!GiveArmor(armor_field, special->info->speed, special->health))
	return;
      player->message = GOTARMOR;
      break;

    case MT_ITEMSHIELD2:
    case MT_BLUEARMOR:
      if (!GiveArmor(armor_field, special->info->speed, special->health))
	return;
      player->message = GOTMEGA;
      break;

    case MT_HEALTHBONUS:  // health bonus
      health++;               // can go over 100%
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      if (cv_showmessages.value==1)
	player->message = GOTHTHBONUS;
      break;

    case MT_ARMORBONUS:  // spirit armor
      GiveArmor(armor_field, -0.333f, 1);
      if (cv_showmessages.value==1)
	player->message = GOTARMBONUS;
      break;

    case MT_SOULSPHERE:
      health += special->health;
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      player->message = GOTSUPER;
      p_sound = sfx_getpow;
      break;

    case MT_MEGA:
      health += special->health;
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      GiveArmor(armor_field, 0.5, special->health);
      player->message = GOTMSPHERE;
      p_sound = sfx_getpow;
      break;

      // keys
    case MT_KEY1:
    case MT_KEY2:
    case MT_KEY3:
    case MT_KEY4:
    case MT_KEY5:
    case MT_KEY6:
    case MT_KEY7:
    case MT_KEY8:
    case MT_KEY9:
    case MT_KEYA:
    case MT_KEYB:
      if (!GiveKey(keycard_t(1 << (special->type - MT_KEY1))))
	return;
      if (game.multiplayer) // Only remove keys in single player game
	p_remove = false;
      break;

      // leave cards for everyone
    case MT_BKEY: // Key_Blue
    case MT_BLUECARD:
      if (!GiveKey(it_bluecard))
	return;
      if (game.multiplayer)
	p_remove = false;
      break;

    case MT_CKEY: // Key_Yellow
    case MT_YELLOWCARD:
      if (!GiveKey(it_yellowcard))
	return;
      if (game.multiplayer)
	p_remove = false;
      break;

    case MT_AKEY: // Key_Green
    case MT_REDCARD:
      if (!GiveKey(it_redcard))
	return;
      if (game.multiplayer)
	p_remove = false;
      break;

    case MT_BLUESKULL:
      if (!GiveKey(it_blueskull))
	return;
      if (game.multiplayer)
	p_remove = false;
      break;

    case MT_YELLOWSKULL:
      if (!GiveKey(it_yellowskull))
        return;
      if (game.multiplayer)
	p_remove = false;
      break;

    case MT_REDSKULL:
      if (!GiveKey(it_redskull))
        return;
      if (game.multiplayer)
	p_remove = false;
      break;

      // medikits, heals
    case MT_XHEALINGBOTTLE:
    case MT_HEALINGBOTTLE:
    case MT_STIM:
      if (!GiveBody (10))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTSTIM;
      break;

    case MT_MEDI:
      if (!GiveBody (25))
	return;
      if(cv_showmessages.value==1)
        {
	  if (health < 25)
	    player->message = GOTMEDINEED;
	  else
	    player->message = GOTMEDIKIT;
        }
      break;

      // Artifacts :
    case MT_XHEALTHFLASK:
    case MT_HEALTHFLASK:
      if (!GiveArtifact(arti_health, special))
	return;
      break;
    case MT_XARTIFLY:
    case MT_ARTIFLY:
      if (!GiveArtifact(arti_fly, special))
	return;
      break;
    case MT_XARTIINVULNERABILITY:
    case MT_ARTIINVULNERABILITY:
      if (!GiveArtifact(arti_invulnerability, special))
	return;
      break;
    case MT_ARTITOMEOFPOWER:
      if (!GiveArtifact(arti_tomeofpower, special))
	return;
      break;
    case MT_ARTIINVISIBILITY:
      if (!GiveArtifact(arti_invisibility, special))
	return;
      break;
    case MT_ARTIEGG:
      if (!GiveArtifact(arti_egg, special))
	return;
      break;
    case MT_XARTISUPERHEAL:
    case MT_ARTISUPERHEAL:
      if (!GiveArtifact(arti_superhealth, special))
	return;
      break;
    case MT_XARTITORCH:
    case MT_ARTITORCH:
      if (!GiveArtifact(arti_torch, special))
	return;
      break;
    case MT_ARTIFIREBOMB:
      if (!GiveArtifact(arti_firebomb, special))
	return;
      break;
    case MT_XARTITELEPORT:
    case MT_ARTITELEPORT:
      if (!GiveArtifact(arti_teleport, special))
	return;
      break;

    case MT_SUMMONMAULATOR:
      if (!GiveArtifact(arti_summon, special))
	return;
      break;
    case MT_XARTIEGG:
      if (!GiveArtifact(arti_pork, special))
	return;
      break;
    case MT_HEALRADIUS:
      if (!GiveArtifact(arti_healingradius, special))
	return;
      break;
    case MT_TELEPORTOTHER:
      if (!GiveArtifact(arti_teleportother, special))
	return;
      break;
    case MT_ARTIPOISONBAG:
      if (!GiveArtifact(arti_poisonbag, special))
	return;
      break;
    case MT_SPEEDBOOTS:
      if (!GiveArtifact(arti_speed, special))
	return;
      break;
    case MT_BOOSTMANA:
      if (!GiveArtifact(arti_boostmana, special))
	return;
      break;
    case MT_BOOSTARMOR:
      if (!GiveArtifact(arti_boostarmor, special))
	return;
      break;
    case MT_BLASTRADIUS:
      if (!GiveArtifact(arti_blastradius, special))
	return;
      break;

      // Puzzle artifacts
    case MT_ARTIPUZZSKULL:
    case MT_ARTIPUZZGEMBIG:
    case MT_ARTIPUZZGEMRED:
    case MT_ARTIPUZZGEMGREEN1:
    case MT_ARTIPUZZGEMGREEN2:
    case MT_ARTIPUZZGEMBLUE1:
    case MT_ARTIPUZZGEMBLUE2:
    case MT_ARTIPUZZBOOK1:
    case MT_ARTIPUZZBOOK2:
    case MT_ARTIPUZZSKULL2:
    case MT_ARTIPUZZFWEAPON:
    case MT_ARTIPUZZCWEAPON:
    case MT_ARTIPUZZMWEAPON:
    case MT_ARTIPUZZGEAR:
    case MT_ARTIPUZZGEAR2:
    case MT_ARTIPUZZGEAR3:
    case MT_ARTIPUZZGEAR4:
      if (!GiveArtifact(artitype_t(arti_puzzskull + special->type - MT_ARTIPUZZSKULL), special))
	return;
      break;

      // power ups
    case MT_INV:
      if (!GivePower (pw_invulnerability))
	return;
      player->message = GOTINVUL;
      p_sound = sfx_getpow;
      break;

    case MT_BERSERKPACK:
      if (!GivePower (pw_strength))
	return;
      player->message = GOTBERSERK;
      if (readyweapon != wp_fist)
	pendingweapon = wp_fist;
      p_sound = sfx_getpow;
      break;

    case MT_INS:
      if (!GivePower (pw_invisibility))
	return;
      player->message = GOTINVIS;
      p_sound = sfx_getpow;
      break;

    case MT_RADSUIT:
      if (!GivePower (pw_ironfeet))
	return;
      player->message = GOTSUIT;
      p_sound = sfx_getpow;
      break;

    case MT_MAPSCROLL:
    case MT_COMPUTERMAP:
      if (!GivePower (pw_allmap))
	return;
      player->message = GOTMAP;
      if( game.mode != gm_heretic )
	p_sound = sfx_getpow;
      break;

    case MT_IRVISOR:
      if (!GivePower (pw_infrared))
	return;
      player->message = GOTVISOR;
      p_sound = sfx_getpow;
      break;

      // Mana
    case MT_MANA1:
      if (!GiveAmmo(am_mana1, 15))
	return;
      player->SetMessage(text[TXT_MANA_1], false);
      break;
    case MT_MANA2:
      if (!GiveAmmo(am_mana2, 15))
	return;
      player->SetMessage(text[TXT_MANA_2], false);
      break;
    case MT_MANA3:
      if (GiveAmmo(am_mana1, 20))
	{
	  if (!GiveAmmo(am_mana2, 20))
	    return;
	}
      else
	GiveAmmo(am_mana2, 20);
      player->SetMessage(text[TXT_MANA_BOTH], false);
      break;

      // heretic Ammo
    case MT_AMGWNDWIMPY:
      if(!GiveAmmo(am_goldwand, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOGOLDWAND1, false);
      break;

    case MT_AMGWNDHEFTY:
      if(!GiveAmmo(am_goldwand, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOGOLDWAND2, false);
      break;

    case MT_AMMACEWIMPY:
      if(!GiveAmmo(am_mace, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOMACE1, false);
      break;

    case MT_AMMACEHEFTY:
      if(!GiveAmmo(am_mace, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOMACE2, false);
      break;

    case MT_AMCBOWWIMPY:
      if(!GiveAmmo(am_crossbow, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOCROSSBOW1, false);
      break;

    case MT_AMCBOWHEFTY:
      if(!GiveAmmo(am_crossbow, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOCROSSBOW2, false);
      break;

    case MT_AMBLSRWIMPY:
      if(!GiveAmmo(am_blaster, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOBLASTER1, false);
      break;

    case MT_AMBLSRHEFTY:
      if(!GiveAmmo(am_blaster, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOBLASTER2, false);
      break;

    case MT_AMSKRDWIMPY:
      if(!GiveAmmo(am_skullrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOSKULLROD1, false);
      break;

    case MT_AMSKRDHEFTY:
      if(!GiveAmmo(am_skullrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOSKULLROD2, false);
      break;

    case MT_AMPHRDWIMPY:
      if(!GiveAmmo(am_phoenixrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOPHOENIXROD1, false);
      break;

    case MT_AMPHRDHEFTY:
      if(!GiveAmmo(am_phoenixrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(TXT_AMMOPHOENIXROD2, false);
      break;

      // ammo
    case MT_CLIP:
      if (special->flags & MF_DROPPED)
        {
	  if (!GiveAmmo(am_clip,clipammo[am_clip]/2))
	    return;
        }
      else
        {
	  if (!GiveAmmo(am_clip,clipammo[am_clip]))
	    return;
        }
      if(cv_showmessages.value==1)
	player->message = GOTCLIP;
      break;

    case MT_AMMOBOX:
      if (!GiveAmmo (am_clip,5*clipammo[am_clip]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTCLIPBOX;
      break;

    case MT_ROCKETAMMO:
      if (!GiveAmmo (am_misl,clipammo[am_misl]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTROCKET;
      break;

    case MT_ROCKETBOX:
      if (!GiveAmmo (am_misl,5*clipammo[am_misl]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTROCKBOX;
      break;

    case MT_CELL:
      if (!GiveAmmo (am_cell,clipammo[am_cell]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTCELL;
      break;

    case MT_CELLPACK:
      if (!GiveAmmo (am_cell,5*clipammo[am_cell]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTCELLBOX;
      break;

    case MT_SHELL:
      if (!GiveAmmo (am_shell,clipammo[am_shell]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTSHELLS;
      break;

    case MT_SHELLBOX:
      if (!GiveAmmo (am_shell,5*clipammo[am_shell]))
	return;
      if(cv_showmessages.value==1)
	player->message = GOTSHELLBOX;
      break;

    case MT_BACKPACK:
      if (!backpack)
        {
	  maxammo = maxammo2;
	  backpack = true;
        }
      for (int i=0 ; i<am_heretic ; i++)
	GiveAmmo (ammotype_t(i), clipammo[i]);
      player->message = GOTBACKPACK;
      break;

    case MT_BAGOFHOLDING:
      if(!backpack)
        {
	  maxammo = maxammo2;
	  backpack = true;
        }
      GiveAmmo(am_goldwand, AMMO_GWND_WIMPY);
      GiveAmmo(am_blaster, AMMO_BLSR_WIMPY);
      GiveAmmo(am_crossbow, AMMO_CBOW_WIMPY);
      GiveAmmo(am_skullrod, AMMO_SKRD_WIMPY);
      GiveAmmo(am_phoenixrod, AMMO_PHRD_WIMPY);
      player->SetMessage(TXT_ITEMBAGOFHOLDING, false);
      break;

        // weapons
    case MT_BFG9000:
      if (!GiveWeapon (wp_bfg, false) )
	return;
      player->message = GOTBFG9000;
      break;

    case MT_CHAINGUN:
      if (!GiveWeapon (wp_chaingun, special->flags & MF_DROPPED) )
	return;
      player->message = GOTCHAINGUN;
      break;

    case MT_SHAINSAW:
      if (!GiveWeapon (wp_chainsaw, false) )
	return;
      player->message = GOTCHAINSAW;
      break;

    case MT_ROCKETLAUNCH:
      if (!GiveWeapon (wp_missile, false) )
	return;
      player->message = GOTLAUNCHER;
      break;

    case MT_PLASMAGUN:
      if (!GiveWeapon (wp_plasma, false) )
	return;
      player->message = GOTPLASMA;
      break;

    case MT_SHOTGUN:
      if (!GiveWeapon (wp_shotgun, special->flags&MF_DROPPED ) )
	return;
      player->message = GOTSHOTGUN;
      break;

    case MT_SUPERSHOTGUN:
      if (!GiveWeapon (wp_supershotgun, special->flags&MF_DROPPED ) )
	return;
      player->message = GOTSHOTGUN2;
      break;

      // heretic weapons
    case MT_WMACE:
      if(!GiveWeapon(wp_mace,false))
	return;
      player->SetMessage(TXT_WPNMACE, false);
      break;
    case MT_WCROSSBOW:
      if(!GiveWeapon(wp_crossbow,false))
	return;
      player->SetMessage(TXT_WPNCROSSBOW, false);
      break;
    case MT_WBLASTER:
      if(!GiveWeapon(wp_blaster,false))
	return;
      player->SetMessage(TXT_WPNBLASTER, false);
      break;
    case MT_WSKULLROD:
      if(!GiveWeapon(wp_skullrod, false))
	return;
      player->SetMessage(TXT_WPNSKULLROD, false);
      break;
    case MT_WPHOENIXROD:
      if(!GiveWeapon(wp_phoenixrod, false))
	return;
      player->SetMessage(TXT_WPNPHOENIXROD, false);
      break;
    case MT_WGAUNTLETS:
      if(!GiveWeapon(wp_gauntlets, false))
	return;
      player->SetMessage(TXT_WPNGAUNTLETS, false);
      break;

      // Hexen weapons
    case MT_MW_CONE: // Frost Shards
      if(!GiveWeapon(wp_cone_of_shards, false))
	return;
      player->message = text[TXT_WEAPON_M2];
      break;

    case MT_MW_LIGHTNING: // Arc of Death
      if(!GiveWeapon(wp_arc_of_death, false))
	return;
      player->message = text[TXT_WEAPON_M3];
      break;

    case MT_FW_AXE: // Timon's Axe
      if(!GiveWeapon(wp_timons_axe, false))
	return;
      player->message = text[TXT_WEAPON_F2];
      break;
    case MT_FW_HAMMER: // Hammer of Retribution
      if(!GiveWeapon(wp_hammer_of_retribution, false))
	return;
      player->message = text[TXT_WEAPON_F3];
      break;

      // 2nd and 3rd Cleric Weapons
    case MT_CW_SERPSTAFF: // Serpent Staff
      if(!GiveWeapon(wp_serpent_staff, false))
	return;
      player->message = text[TXT_WEAPON_C2];
      break;
    case MT_CW_FLAME: // Firestorm
    if(!GiveWeapon(wp_firestorm, false))
      return;
    player->message = text[TXT_WEAPON_C3];
      break;

      // TODO Fourth Weapon Pieces
      /*
    case SPR_WFR1:
      TryPickupWeaponPiece(player, PCLASS_FIGHTER, WPIECE1,
			   special);
      return;
		case SPR_WFR2:
			TryPickupWeaponPiece(player, PCLASS_FIGHTER, WPIECE2,
				special);
			return;
		case SPR_WFR3:
			TryPickupWeaponPiece(player, PCLASS_FIGHTER, WPIECE3,
				special);
			return;
		case SPR_WCH1:
			TryPickupWeaponPiece(player, PCLASS_CLERIC, WPIECE1,
				special);
			return;
		case SPR_WCH2:
			TryPickupWeaponPiece(player, PCLASS_CLERIC, WPIECE2,
				special);
			return;
		case SPR_WCH3:
			TryPickupWeaponPiece(player, PCLASS_CLERIC, WPIECE3,
				special);
			return;
		case SPR_WMS1:
			TryPickupWeaponPiece(player, PCLASS_MAGE, WPIECE1,
				special);
			return;
		case SPR_WMS2:
			TryPickupWeaponPiece(player, PCLASS_MAGE, WPIECE2,
				special);
			return;
		case SPR_WMS3:
			TryPickupWeaponPiece(player, PCLASS_MAGE, WPIECE3,
				special);
			return;
      */

    default:
      // SoM: New gettable things with FraggleScript!
      //CONS_Printf ("\2P_TouchSpecialThing: Unknown gettable thing\n");
      return;
    }

  if (special->flags & MF_COUNTITEM)
    player->items++;

  if (displayplayer == player)
    hud.bonuscount += BONUSADD;

  if (player == displayplayer || (cv_splitscreen.value && player == displayplayer2))
    S_StartAmbSound(p_sound);

  // pickup special (Hexen)
  if (special->special)
    {
      mp->ExecuteLineSpecial(special->special, special->args, NULL, 0, this);
      special->special = 0;
    }

  if (p_remove)
    special->Remove();
}


/*
#ifdef thatsbuggycode
//
//  Tell each supported thing to check again its position,
//  because the 'base' thing has vanished or diminished,
//  the supported things might fall.
//
//added:28-02-98:
void P_CheckSupportThings (Actor* mobj)
{
  fixed_t   supportz = mobj->z + mobj->height;

  while ((mobj = mobj->supportthings))
    {
      // only for things above support thing
      if (mobj->z > supportz)
	mobj->eflags |= MF_CHECKPOS;
    }
}


//
//  If a thing moves and supportthings,
//  move the supported things along.
//
//added:28-02-98:
void P_MoveSupportThings (Actor* mobj, fixed_t xmove, fixed_t ymove, fixed_t zmove)
{
  fixed_t   supportz = mobj->z + mobj->height;
  Actor    *mo = mobj->supportthings;

  while (mo)
    {
      //added:28-02-98:debug
      if (mo==mobj)
        {
	  mobj->supportthings = NULL;
	  break;
        }
      
      // only for things above support thing
      if (mobj->z > supportz)
        {
	  mobj->eflags |= MF_CHECKPOS;
	  mobj->px += xmove;
	  mobj->py += ymove;
	  mobj->pz += zmove;
        }

      mo = mo->supportthings;
    }
}


//
//  Link a thing to it's 'base' (supporting) thing.
//  When the supporting thing will move or change size,
//  the supported will then be aware.
//
//added:28-02-98:
void P_LinkFloorThing(Actor*   mobj)
{
    Actor*     mo;
    Actor*     nmo;

    // no supporting thing
    if (!(mo = mobj->floorthing))
        return;

    // link mobj 'above' the lower mobjs, so that lower supporting
    // mobjs act upon this mobj
    while ( (nmo = mo->supportthings) &&
            (nmo->z<=mobj->z) )
    {
        // dont link multiple times
        if (nmo==mobj)
            return;

        mo = nmo;
    }
    mo->supportthings = mobj;
    mobj->supportthings = nmo;
}


//
//  Unlink a thing from it's support,
//  when it's 'floorthing' has changed,
//  before linking with the new 'floorthing'.
//
//added:28-02-98:
void P_UnlinkFloorThing(Actor*   mobj)
{
  Actor*     mo;

  if (!(mo = mobj->floorthing))      // just to be sure (may happen)
    return;

  while (mo->supportthings)
    {
      if (mo->supportthings == mobj)
        {
	  mo->supportthings = NULL;
	  break;
        }
      mo = mo->supportthings;
    }
}
#endif
*/




//---------------------------------------------------------------------------
//
// FUNC P_AutoUseChaosDevice
//
//---------------------------------------------------------------------------

bool P_AutoUseChaosDevice(PlayerPawn *p)
{
  int i, n = p->inventory.size();
    
  for (i = 0; i < n; i++)
    {
      if (p->inventory[i].type == arti_teleport)
        {
	  p->UseArtifact(arti_teleport);
	  p->health = (p->health + 1) / 2;
	  return true;
        }
    }
  return false;
}

//---------------------------------------------------------------------------
//
// PROC P_AutoUseHealth
//
//---------------------------------------------------------------------------

void P_AutoUseHealth(PlayerPawn *p, int saveHealth)
{
  int i, n = p->inventory.size();
  int count;
  int normalCount;
  int normalSlot;
  int superCount;
  int superSlot;
    
  normalCount = superCount = 0;
  for(i = 0; i < n; i++)
    {
      if (p->inventory[i].type == arti_health)
        {
	  normalSlot = i;
	  normalCount = p->inventory[i].count;
        }
      else if (p->inventory[i].type == arti_superhealth)
        {
	  superSlot = i;
	  superCount = p->inventory[i].count;
        }
    }
  if((game.skill == sk_baby) && (normalCount*25 >= saveHealth))
    { // Use quartz flasks
      count = (saveHealth+24)/25;
      for(i = 0; i < count; i++)
	p->UseArtifact(arti_health);
    }
  else if(superCount*100 >= saveHealth)
    { // Use mystic urns
      count = (saveHealth+99)/100;
      for(i = 0; i < count; i++)
	p->UseArtifact(arti_superhealth);
    }
  else if((game.skill == sk_baby) && (superCount*100+normalCount*25 >= saveHealth))
    { // Use mystic urns and quartz flasks
      count = (saveHealth+24)/25;
      for(i = 0; i < count; i++)
	p->UseArtifact(arti_health);

      saveHealth -= count*25;
      count = (saveHealth+99)/100;
      for(i = 0; i < count; i++)
	p->UseArtifact(arti_superhealth);
    }
}


//---------------------------------------------
// was P_DamageMobj
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
// TODO the damage/thrust logic should be changed altogether, using functions like
// Staff::Hit(actor) {
//    actor->Damage(2,this);
//    if (actor.mass == small) actor->Thrust()...
// }

bool PlayerPawn::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (dtype & dt_always)
    {
      // unavoidable damage
      // pain flash
      if (player == displayplayer)
	hud.damagecount += damage;
      return Actor::Damage(inflictor, source, damage, dtype);
    }

  if (game.skill == sk_baby)
    damage >>= 1;   // take half damage in trainer mode
  
  if (inflictor && (inflictor->Type() == Thinker::tt_dactor))
    {
      DActor *d = (DActor *)inflictor;
      switch (d->type)
	{
	case MT_MACEFX4: // Death ball
	  if (powers[pw_invulnerability])
	    // Can't hurt invulnerable players
	    damage = 0;
	    break;	  
	  if (P_AutoUseChaosDevice(this))
	    // Player was saved using chaos device
	    return false;	
	  damage = 10000; // Something's gonna die
	  break;
        case MT_PHOENIXFX2: // Flame thrower
	  if (P_Random() < 128)
            { // Freeze player for a bit
	      reactiontime += 4;
            }
	  break;
	default:
	  break;
	}
    }

  int i, temp;
  // player specific
  if (!(flags & MF_CORPSE))
    {
      // end of game hellslime hack
      if (subsector->sector->special == 11 && damage >= health)
	damage = health - 1;

      // ignore damage in GOD mode, or with INVUL power.
      if ((cheats & CF_GODMODE) || powers[pw_invulnerability])
	return false;

      // doom armor
      temp = armorpoints[armor_field];
      if (temp > 0)
        {
	  int saved = int(damage * armorfactor[armor_field]);

	  if (temp <= saved)
            {
	      // armor is used up
	      saved = temp;
	      armorfactor[armor_field] = 0;
            }
	  armorpoints[armor_field] -= saved;
	  damage -= saved;
        }

      // hexen armor
      float save = toughness;
      for (i = armor_armor; i < NUMARMOR; i++)
	save += float(armorpoints[i])/100;
      if (save > 0)
	{
	  // armor absorbed some damage
	  if (save > 1)
	    save = 1;

	  // armor deteriorates
	  for (i = armor_armor; i < NUMARMOR; i++)
	    if (armorpoints[i])
	      {
		armorpoints[i] -= int(damage * ArmorIncrement[pclass][i] / (100 * armorfactor[i]));
		if (armorpoints[i] <= 2)
		  armorpoints[i] = 0;
	      }

	  int saved = int(damage * save);
	  if (damage > 200)
	    saved = int(200 * save);
	  damage -= saved;
	}      

      PlayerPawn *s = NULL;
      if (source && source->Type() == Thinker::tt_ppawn)
	s = (PlayerPawn *)source;

      // added team play and teamdamage (view logboris at 13-8-98 to understand)
      if (s && (s->player->team == player->team) && !cv_teamdamage.value && (s != this))
	return false;

      // autosavers
      if (damage >= health && ((game.skill == sk_baby) || cv_deathmatch.value) && !morphTics)
	{ // Try to use some inventory health
	  P_AutoUseHealth(this, damage-health+1);
	}

      // pain flash
      if (player == displayplayer)
	hud.damagecount += damage;

      pres->SetAnim(presentation_t::Pain);

      //added:22-02-98: force feedback ??? electro-shock???
      if (player == consoleplayer)
	I_Tactile (40,10,40+min(damage, 100)*2);
    }

  attacker = source;

  bool ret = Actor::Damage(inflictor, source, damage, dtype);

  return ret;

  /* TODO
     if (health <= 0 && inflictor && !morphTics)
    { // Check for flame death
      if ((inflictor->flags2 & MF2_FIREDAMAGE) ||
	  ((inflictor->type == MT_PHOENIXFX1)
	   && (health > -50) && (damage > 25)))
	{
	  flags2 |= MF2_FIREDAMAGE;
	}
    }

     if(flags2&MF2_FIREDAMAGE)
     { // Player flame death
     SetState(S_PLAY_FDTH1);
     //S_StartSound(this, sfx_hedat1); // Burn sound
     return;
     }
  */  
}




//---------------------------------------------------------------------------
// was P_ChickenMorphPlayer
// Returns true if the player gets turned into a chicken.

#define CHICKENTICS     (40*TICRATE)

bool PlayerPawn::Morph()
{
  // TODO morph should take a mobjtype_t parameter into which to morph
  if (morphTics)
    {
      if ((morphTics < CHICKENTICS-TICRATE)
	  && !powers[pw_weaponlevel2])
        { // Make a super chicken
	  GivePower(pw_weaponlevel2);
        }
      return false;
    }
  if (powers[pw_invulnerability])
    { // Immune when invulnerable
      return false;
    }

  // store x,y,z, angle, flags2
  //SetState(S_FREETARGMOBJ);

  DActor *fog = mp->SpawnDActor(x, y, z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_telept);

  // FIXME again this Morph/FREETARGMOBJ problem...
  // set chicken attributes here, change appearance.
  // DActor *chicken = mp->SpawnDActor(x, y, z, MT_CHICPLAYER);
  //chicken->special1 = readyweapon;
  //chicken->angle = angle;
  //chicken->player = player;
  //chicken->health = MAXCHICKENHEALTH;
  //player->mo = chicken;
  armorfactor[0] = armorpoints[0] = 0;
  powers[pw_invisibility] = 0;
  powers[pw_weaponlevel2] = 0;
  weaponinfo = wpnlev1info;
  if (flags2 & MF2_FLY)
    {
      //chicken->flags2 |= MF2_FLY;
    }
  morphTics = CHICKENTICS;
  ActivateMorphWeapon();
  return true;
}

//---------------------------------------------------------------------------
// was P_ChickenMorph

bool Actor::Morph()
{
  return false;
}

bool DActor::Morph()
{
  switch (type)
    {
    case MT_POD:
    case MT_CHICKEN:
    case MT_HHEAD:
    case MT_MINOTAUR:
    case MT_SORCERER1:
    case MT_SORCERER2:
      return false;
    default:
      break;
    }

  // turn the original into a harmless invisible thingy
  SetState(S_FREETARGMOBJ);

  DActor *fog = mp->SpawnDActor(x, y, z + TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_telept);

  // make the chicken
  DActor *chicken = mp->SpawnDActor(x, y, z, MT_CHICKEN);
  chicken->special2 = type;
  chicken->special1 = CHICKENTICS+P_Random();
  chicken->flags |= (flags & MF_SHADOW);
  chicken->target = target;
  chicken->angle = angle;
  return true;
}
