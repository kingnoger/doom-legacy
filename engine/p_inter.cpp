// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
// Revision 1.40  2004/11/18 20:30:10  smite-meister
// tnt, plutonia
//
// Revision 1.39  2004/11/13 22:38:42  smite-meister
// intermission works
//
// Revision 1.38  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.37  2004/10/27 17:37:06  smite-meister
// netcode update
//
// Revision 1.36  2004/09/23 23:21:16  smite-meister
// HUD updated
//
// Revision 1.35  2004/09/13 20:43:30  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.34  2004/09/06 19:58:03  smite-meister
// Doom linedefs done!
//
// Revision 1.30  2004/07/13 20:23:36  smite-meister
// Mod system basics
//
// Revision 1.29  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.28  2004/04/25 16:26:49  smite-meister
// Doxygen
//
// Revision 1.25  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.23  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.22  2003/12/03 10:49:50  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.21  2003/11/27 11:28:25  smite-meister
// Doom/Heretic startup bug fixed
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Actor interactions (collisions, damage, death).

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "dstrings.h"
#include "m_random.h"
#include "g_damage.h"

#include "g_game.h"
#include "g_type.h"
#include "g_player.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_spec.h"
#include "p_heretic.h"
#include "sounds.h"
#include "r_sprite.h"
#include "tables.h"


#define BASETHRESHOLD 100

extern int ArmorIncrement[NUMCLASSES][NUMARMOR];

void P_TeleportOther(Actor *v);
bool P_AutoUseChaosDevice(PlayerPawn *p);
void P_AutoUseHealth(PlayerPawn *p, int saveHealth);


//======================================================
//   Killing a PlayerPawn
//======================================================

// XXX::Killed is called when a class XXX member kills a PlayerPawn.
// It returns the proper death message and updates the score.

void Actor::Killed(PlayerPawn *victim, Actor *inflictor)
{
  CONS_Printf("%s is killed by an inanimate carbon rod.\n", victim->player->name.c_str());
}


void DActor::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // monster killer

  char *str = NULL;
  switch (type)
    {
    case MT_BARREL:    str = text[TXT_DEATHMSG_BARREL]; break;
    case MT_POSSESSED: str = text[TXT_DEATHMSG_POSSESSED]; break;
    case MT_SHOTGUY:   str = text[TXT_DEATHMSG_SHOTGUY];   break;
    case MT_VILE:      str = text[TXT_DEATHMSG_VILE];      break;
    case MT_FATSO:     str = text[TXT_DEATHMSG_FATSO];     break;
    case MT_CHAINGUY:  str = text[TXT_DEATHMSG_CHAINGUY];  break;
    case MT_TROOP:     str = text[TXT_DEATHMSG_TROOP];     break;
    case MT_SERGEANT:  str = text[TXT_DEATHMSG_SERGEANT];  break;
    case MT_SHADOWS:   str = text[TXT_DEATHMSG_SHADOWS];   break;
    case MT_HEAD:      str = text[TXT_DEATHMSG_HEAD];      break;
    case MT_BRUISER:   str = text[TXT_DEATHMSG_BRUISER];   break;
    case MT_UNDEAD:    str = text[TXT_DEATHMSG_UNDEAD];    break;
    case MT_KNIGHT:    str = text[TXT_DEATHMSG_KNIGHT];    break;
    case MT_SKULL:     str = text[TXT_DEATHMSG_SKULL];     break;
    case MT_SPIDER:    str = text[TXT_DEATHMSG_SPIDER];    break;
    case MT_BABY:      str = text[TXT_DEATHMSG_BABY];      break;
    case MT_CYBORG:    str = text[TXT_DEATHMSG_CYBORG];    break;
    case MT_PAIN:      str = text[TXT_DEATHMSG_PAIN];      break;
    case MT_WOLFSS:    str = text[TXT_DEATHMSG_WOLFSS];    break;
    default:           str = text[TXT_DEATHMSG_DEAD];      break;
    }

  victim->player->SetMessage(va(str, victim->player->name.c_str()));
}


void PlayerPawn::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // player killer
  game.gtype->Frag(player, victim->player);

  S_StartAmbSound(player, sfx_frag);

  if (game.mode == gm_heretic)
    {
      // Make a super chicken
      if (morphTics)
	GivePower(pw_weaponlevel2);
    }

  char *str = NULL;

  if (player == victim->player)
    {
      player->SetMessage(va(text[TXT_DEATHMSG_SUICIDE], player->name.c_str()));
      return;
    }

  if (victim->health < -9000) // telefrag !
    str = text[TXT_DEATHMSG_TELEFRAG];
  else
    {
      int w = -1;
      if (inflictor && (inflictor->IsOf(DActor::_type)))
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
	  str = text[TXT_DEATHMSG_FIST];
	  break;
	case wp_pistol:
	  str = text[TXT_DEATHMSG_GUN];
	  break;
	case wp_shotgun:
	  str = text[TXT_DEATHMSG_SHOTGUN];
	  break;
	case wp_chaingun:
	  str = text[TXT_DEATHMSG_MACHGUN];
	  break;
	case wp_missile:
	  str = text[TXT_DEATHMSG_ROCKET];
	  if (victim->health < -victim->maxhealth)
	    str = text[TXT_DEATHMSG_GIBROCKET];
	  break;
	case wp_plasma:
	  str = text[TXT_DEATHMSG_PLASMA];
	  break;
	case wp_bfg:
	  str = text[TXT_DEATHMSG_BFGBALL];
	  break;
	case wp_chainsaw:
	  str = text[TXT_DEATHMSG_CHAINSAW];
	  break;
	case wp_supershotgun:
	  str = text[TXT_DEATHMSG_SUPSHOTGUN];
	  break;
	case wp_barrel:
	  str = text[TXT_DEATHMSG_BARRELFRAG];
	  break;
	default:
	  str = text[TXT_DEATHMSG_PLAYUNKNOW];
	  break;
	}
    }

  // send message to both parties
  str = va(str, victim->player->name.c_str(), player->name.c_str());
  player->SetMessage(str);
  victim->player->SetMessage(str);
}




//======================================================
//    Touching another Actor
//======================================================

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
  int damage = info->damage & dt_DAMAGEMASK;
  int dtype = info->damage & dt_TYPEMASK;

  // check for skulls slamming into things
  if (eflags & MFE_SKULLFLY)
    {
      damage = ((P_Random()%8) + 1) * damage;

      p->Damage(this, this, damage, dtype);

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
      if (owner && owner->IsOf(DActor::_type) && p->IsOf(DActor::_type))
	{
	  DActor *ow = (DActor *)owner;
	  DActor *t = (DActor *)p;

	  if (ow->type == t->type ||
	      (ow->type == MT_KNIGHT  && t->type == MT_BRUISER) ||
	      (ow->type == MT_BRUISER && t->type == MT_KNIGHT)) // damn complicated!
	    return true; // Explode, but do no damage.
        }

      if (!(p->flags & MF_SHOOTABLE))
        {
	  // didn't do any damage
	  return (p->flags & MF_SOLID);
        }

      // ripping projectiles
      if (flags2 & MF2_RIP)
        {
	  damage = ((P_Random () & 3) + 2) * damage;
	  S_StartSound(this, sfx_ripslop);
	  if (p->Damage(this, owner, damage, dtype))
            {
	      if (!(p->flags & MF_NOBLOOD))
		mp->SpawnBlood(x, y, z, damage);
            }

	  if ((p->flags2 & MF2_PUSHABLE) && !(flags2 & MF2_CANNOTPUSH))
            { // Push thing
	      p->px += px >> 2;
	      p->py += py >> 2;
            }
	  //spechit.clear(); FIXME why?
	  return false;
        }

      // damage / explode
      damage = ((P_Random()%8)+1) * damage;
      if (p->Damage(this, owner, damage, dtype) &&
	  !(p->flags & MF_NOBLOOD))
	mp->SpawnBloodSplats(x, y, z, damage, p->px, p->py);

      // don't traverse any more
      return true;
    }

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
	  if (p->IsOf(DActor::_type))
	    {
	      DActor *dp = (DActor *)p;
	      TouchSpecialThing(dp); // this also Removes() the item
	    }
	  return false; // no collision, just possible pickup
        }
    }

  return (p->flags & MF_SOLID);
}



//======================================================
//    Giving damage
//======================================================

// Gives damage to the Actor. If the damage is "stopped" (absorbed), returns true.
// Inflictor is the thing that caused the damage
// (creature or missile, can also be NULL (slime, etc.))
// Source is the thing responsible for the damage (the one to get angry at).
// It may also be NULL. Source and inflictor are the same for melee attacks.

bool Actor::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (!(flags & MF_SHOOTABLE))
    return false;

  if ((dtype & dt_TYPEMASK) == dt_poison && flags & MF_NOBLOOD)
    return false; // only damage living things with poison

  // old recoil code
  // TODO Some close combat weapons should not
  // inflict thrust and push the victim out of reach
  if (inflictor && !(flags & MF_NOCLIPTHING) && (dtype & dt_oldrecoil)
      && !(inflictor->flags2 & MF2_NODMGTHRUST))      
    {
      fixed_t apx, apy, apz = 0;
      fixed_t thrust;  

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

      apx = FixedMul(thrust, finecosine[ang]);
      apy = FixedMul(thrust, finesine[ang]);
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
	  apz = (abs(apx) + abs(apy)) >> 1;
	  
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



bool DActor::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (!(flags & MF_SHOOTABLE))
    return false; // shouldn't happen...

  if (dtype & dt_always)
    return Actor::Damage(inflictor, source, damage, dtype);

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
  if (inflictor && (inflictor->IsOf(DActor::_type)))
    {
      DActor *inf = (DActor *)inflictor;
      switch (inf->type)
        {
        case MT_EGGFX:
	  Morph(MT_CHICKEN);
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

	  // Hexen
	case MT_TELOTHER_FX1:
	case MT_TELOTHER_FX2:
	case MT_TELOTHER_FX3:
	case MT_TELOTHER_FX4:
	case MT_TELOTHER_FX5:
	  P_TeleportOther(this);
	  return true;
	case MT_SHARDFX1:
	  damage <<= inf->special2; // shard damage depends on spermcount
	  break;

        default:
	  break;
        }
    }

  if (!Actor::Damage(inflictor, source, damage, dtype))
    return false; // not hurt after all

  if (health <= 0)
    {
      //special1 = damage; // wtf?
      return true;
    }

  // hurt but not dead

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
      if (source->IsOf(DActor::_type))
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



bool PlayerPawn::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{
  if (dtype & dt_always)
    {
      // unavoidable damage
      // pain flash
      player->damagecount += damage;
      return Actor::Damage(inflictor, source, damage, dtype);
    }

  // TODO this is nastily duplicated in DActor::Damage, but...
  if (inflictor && inflictor->IsOf(DActor::_type))
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

        case MT_EGGFX:
	  Morph(MT_CHICKEN);
	  return false; // Always return
        case MT_WHIRLWIND:
	  return P_TouchWhirlwind(this);

	  // Hexen
	case MT_TELOTHER_FX1:
	case MT_TELOTHER_FX2:
	case MT_TELOTHER_FX3:
	case MT_TELOTHER_FX4:
	case MT_TELOTHER_FX5:
	  P_TeleportOther(this);
	  return true;
	case MT_SHARDFX1:
	  damage <<= d->special2; // shard damage depends on spermcount
	  break;

	  // TODO Hexen damage effects
	  /*
	case MT_BISH_FX:
	  damage >>= 1; // Bishops are just too nasty TODO fix this elsewhere
	  break;
	case MT_ICEGUY_FX2:
	  damage >>= 1; // same here
	  break;
	case MT_CSTAFF_MISSILE:
	  // Cleric Serpent Staff does poison damage
	  P_PoisonPlayer(player, source, 20);
	  damage >>= 1;
	  break;
	case MT_POISONDART:
	  P_PoisonPlayer(player, source, 20);
	  damage >>= 1;
	  break;
	case MT_POISONCLOUD:
	  if (player->poisoncount < 4)
	    {
	      P_PoisonDamage(player, source, 15+(P_Random()&15), false); // Don't play painsound
	      P_PoisonPlayer(player, source, 50);
	      S_StartSound(target, SFX_PLAYER_POISONCOUGH);
	    }	
	  return;
	case MT_FSWORD_MISSILE:
	  if (player)
	    damage -= damage>>2;
	  break;
	  */

	default:
	  break;
	}
    }

  if (game.skill == sk_baby)
    damage >>= 1;   // take half damage in trainer mode

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
      if (source && source->IsOf(PlayerPawn::_type))
	s = (PlayerPawn *)source;

      // added teamplay and teamdamage
      if (s && (s->player->team == player->team) && !cv_teamdamage.value && (s != this))
	return false;

      // autosavers
      if (damage >= health && (game.skill == sk_baby) && !morphTics)
	{ // Try to use some inventory health
	  P_AutoUseHealth(this, damage-health+1);
	}

      // pain flash
      player->damagecount += damage;

      pres->SetAnim(presentation_t::Pain);
    }

  attacker = source;

  return Actor::Damage(inflictor, source, damage, dtype);
}



//======================================================
//   Dying
//======================================================

// inflictor is the bullet, source is the one who pulled the trigger ;)
void Actor::Die(Actor *inflictor, Actor *source)
{
  // switch physics to inanimate object mode
  eflags &= ~(MFE_INFLOAT | MFE_SKULLFLY | MFE_SWIMMING);

  // cream a corpse :)
  if (flags & MF_CORPSE)
    {
      flags &= ~(MF_SOLID | MF_SHOOTABLE);
      height = 0;
      radius = 0;
      return;
    }

  // dead target is no more shootable
  if (!cv_solidcorpse.value)
    flags &= ~MF_SHOOTABLE;

  // thing death actions
  if (special) // formerly also demanded MF_COUNTKILL (MT_ZBELL!)
    mp->ExecuteLineSpecial(special, args, NULL, 0, this);

  // if a player killed a monster, update kills
  if (flags & MF_COUNTKILL)
    {
      if (source && source->IsOf(PlayerPawn::_type))
	{
	  PlayerPawn *s = (PlayerPawn *)source;
	  // count for intermission
	  s->player->kills++;    
	}
      else if (!game.multiplayer)
	{
	  // count all monster deaths,
	  // even those caused by other monsters
	  game.Players.begin()->second->kills++;
	}
    }
}


bool P_CheckSpecialDeath(DActor *m, int dtype);

void DActor::Die(Actor *inflictor, Actor *source)
{
  if (!inflictor)
    inflictor = source;

  if (flags & MF_CORPSE)
    {
      if (flags & MF_NOBLOOD)
	Remove();
      else
	{
	  SetState(S_GIBS);
	  S_StartSound(this, sfx_gib); // lets have a neat 'crunch' sound!
	}
      Actor::Die(inflictor, source);      
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

  // TODO mobjinfo.damage, upper 16 bits could hold damage type, remove MF2_FIREDAMAGE et al.
  if (inflictor)
    {
      int dtype = 0;
      if (inflictor->flags2 & MF2_FIREDAMAGE)
	dtype |= dt_heat;
      if (inflictor->flags2 & MF2_ICEDAMAGE)
	dtype |= dt_cold;

      if (P_CheckSpecialDeath(this, dtype))
	return;
    }

  if (((game.mode != gm_heretic && health < -info->spawnhealth)
       ||(game.mode == gm_heretic && health < -(info->spawnhealth>>1)))
      && info->xdeathstate)
    SetState(info->xdeathstate);
  else
    SetState(info->deathstate);
  // Normally, A_Fall or A_NoBlocking follow the deathstate and make the thing a nonsolid corpse

  tics -= P_Random()&3;

  if (tics < 1)
    tics = 1;

  Actor::Die(inflictor, source);
}



void PlayerPawn::Die(Actor *inflictor, Actor *source)
{
  Actor::Die(inflictor, source);

  // start playing death anim. sequence
  if (health < -maxhealth >> 1)
    pres->SetAnim(presentation_t::Death2);
  else
    pres->SetAnim(presentation_t::Death1);

  // TODO the player death sounds!

  if (!source)
    {
      // environment kills
      int w = specialsector;
      char *str = text[TXT_DEATHMSG_SPECUNKNOW];

      if (w & SS_DAMAGEMASK)
	{
	  sector_t *sec = subsector->sector;
	  if (sec->damage <= 5)
	    str = text[TXT_DEATHMSG_NUKE];
	  else if (sec->damage <= 10)
	    str = text[TXT_DEATHMSG_HELLSLIME];
	  else
	    str = text[TXT_DEATHMSG_SUPHELLSLIME];
	}

      player->SetMessage(va(str, player->name.c_str()));

      // count environment kills against you (you fragged yourself!)
      game.gtype->Frag(player, player);
    }
  else if (!(flags & MF_CORPSE))
    source->Killed(this, inflictor); // you can kill 'em only once!

  // dead guy attributes
  eflags &= ~MFE_FLY;
  powers[pw_flight] = 0;
  powers[pw_weaponlevel2] = 0;
  DropWeapon();  // put weapon away

  player->playerstate = PST_DEAD;

  // (since A_Fall is not called for PlayerPawns)
  // actor is on ground, it can be walked over
  if (!cv_solidcorpse.value)
    flags &= ~MF_SOLID;
  else
    {
      height >>= 2;
      radius -= (radius >> 4);
      health = maxhealth >> 1;
    }

  flags |= MF_CORPSE|MF_DROPOFF;

  // So change this if corpse objects
  // are meant to be obstacles.

  // TODO Player flame and ice death
  // SetState(S_PLAY_FDTH1);
  //S_StartSound(this, sfx_hedat1); // Burn sound
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
