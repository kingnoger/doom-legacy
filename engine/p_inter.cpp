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
// Revision 1.32  2004/08/19 19:42:40  smite-meister
// bugfixes
//
// Revision 1.31  2004/08/12 18:30:23  smite-meister
// cleaned startup
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
// Revision 1.26  2004/01/02 14:25:01  smite-meister
// cleanup
//
// Revision 1.25  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.24  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
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
// Revision 1.20  2003/11/23 00:41:55  smite-meister
// bugfixes
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
#include "command.h"
#include "cvars.h"

#include "am_map.h"
#include "dstrings.h"
#include "m_random.h"
#include "g_damage.h"

#include "g_game.h"
#include "g_type.h"
#include "g_player.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "p_enemy.h"
#include "p_heretic.h"
#include "sounds.h"
#include "r_sprite.h"
#include "r_main.h"
#include "tables.h"

#include "hu_stuff.h" // HUD

#define BONUSADD        6


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

  CONS_Printf(str, victim->player->name.c_str());
}


void PlayerPawn::Killed(PlayerPawn *victim, Actor *inflictor)
{
  // player killer
  game.gtype->Frag(player, victim->player);

  if (player == displayplayer || player == displayplayer2)
    S_StartAmbSound(sfx_frag);

  if (game.mode == gm_heretic)
    {
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
      CONS_Printf(text[TXT_DEATHMSG_SUICIDE], player->name.c_str());
      // FIXME when console is rewritten to accept << >>
      //if (cv_splitscreen.value)
      // console << "\4" << t->player->name << text[TXT_DEATHMSG_SUICIDE];
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
	  //spechit.clear(); FIXME why?
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
	  consoleplayer->kills++;
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


//---------------------------------------------

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
      int w = specialsector;      //see p_spec.c
      char *str;

      if (w == 5)
	str = text[TXT_DEATHMSG_HELLSLIME];
      else if (w == 7)
	str = text[TXT_DEATHMSG_NUKE];
      else if (w == 16 || w == 4)
	str = text[TXT_DEATHMSG_SUPHELLSLIME];
      else
	str = text[TXT_DEATHMSG_SPECUNKNOW];

      if (player == consoleplayer || player == consoleplayer2)
	CONS_Printf(str, player->name.c_str());

      // count environment kills against you (you fragged yourself!)
      game.gtype->Frag(player, player);
    }
  else
    source->Killed(this, inflictor);

  // dead guy attributes
  flags2 &= ~MF2_FLY;
  powers[pw_flight] = 0;
  powers[pw_weaponlevel2] = 0;
  DropWeapon();  // put weapon away

  player->playerstate = PST_DEAD;

  if (player == consoleplayer || player == consoleplayer2)
    {
      // don't die in auto map,
      // switch view prior to dying
      if (automap.active)
	automap.Close();
    }

  // TODO Player flame and ice death
  // SetState(S_PLAY_FDTH1);
  //S_StartSound(this, sfx_hedat1); // Burn sound
}



//============================================================
//  The Heretic way of respawning items. Unused.

void P_HideSpecialThing(DActor *thing)
{
  thing->flags &= ~MF_SPECIAL;
  thing->flags2 |= MF2_DONTDRAW;
  thing->SetState(S_HIDESPECIAL1);
}

// Make a special thing visible again.
void A_RestoreSpecialThing1(DActor *thing)
{
  if (thing->type == MT_WMACE)
    { // Do random mace placement
      thing->mp->RepositionMace(thing);
    }
  thing->flags2 &= ~MF2_DONTDRAW;
  S_StartSound(thing, sfx_itemrespawn);
}

void A_RestoreSpecialThing2(DActor *thing)
{
  thing->flags |= MF_SPECIAL;
  thing->SetState(thing->info->spawnstate);
}



int  p_sound;  // pickupsound
bool p_remove; // should the stuff be removed?

// pickups.
void PlayerPawn::TouchSpecialThing(DActor *special)
{                  
  // Dead thing touching.
  // Can happen with a sliding player corpse.
  if (health <= 0 || flags & MF_CORPSE)
    return;

  p_remove = true; // should the item be removed from map?
  p_sound = sfx_itemup;

  int stype = special->type;

  // Identify item
  switch (stype)
    {
    case MT_ARMOR_1:
      if (!GiveArmor(armor_armor, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR1]);
      break;
    case MT_ARMOR_2:
      if(!GiveArmor(armor_shield, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR2]);
      break;
    case MT_ARMOR_3:
      if(!GiveArmor(armor_helmet, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR3]);
      break;
    case MT_ARMOR_4:
      if(!GiveArmor(armor_amulet, 3.0, -1))
	return;
      player->SetMessage(text[TXT_ARMOR4]);
      break;

    case MT_ITEMSHIELD1:
    case MT_ITEMSHIELD2:
      if (!GiveArmor(armor_field, special->info->speed, special->health))
	return;
      player->SetMessage(text[stype - MT_ITEMSHIELD1 + TXT_ITEMSHIELD1]);
      break;

    case MT_GREENARMOR:
    case MT_BLUEARMOR:
      if (!GiveArmor(armor_field, special->info->speed, special->health))
	return;
      player->SetMessage(text[stype - MT_GREENARMOR + TXT_GOTARMOR]);
      break;

    case MT_HEALTHBONUS:  // health bonus
      health++;               // can go over 100%
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      if (cv_showmessages.value==1)
	player->SetMessage(GOTHTHBONUS);
      break;

    case MT_ARMORBONUS:  // spirit armor
      GiveArmor(armor_field, -0.333f, 1);
      if (cv_showmessages.value==1)
	player->SetMessage(GOTARMBONUS);
      break;

    case MT_SOULSPHERE:
      health += special->health;
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      player->SetMessage(GOTSUPER);
      p_sound = sfx_powerup;
      break;

    case MT_MEGA:
      health += special->health;
      if (health > 2*maxhealth)
	health = 2*maxhealth;
      GiveArmor(armor_field, 0.5, special->health);
      player->SetMessage(GOTMSPHERE);
      p_sound = sfx_powerup;
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
      if (!GiveKey(keycard_t(1 << (stype - MT_KEY1))))
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
      if (cv_showmessages.value == 1)
	if (stype == MT_STIM)
	  player->SetMessage(GOTSTIM);
	else
	  player->SetMessage(text[TXT_ITEMHEALTH]);
      break;

    case MT_MEDI:
      if (!GiveBody (25))
	return;
      if (cv_showmessages.value == 1)
        {
	  if (health < 25)
	    player->SetMessage(GOTMEDINEED);
	  else
	    player->SetMessage(GOTMEDIKIT);
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
      if (!GiveArtifact(artitype_t(arti_puzzskull + stype - MT_ARTIPUZZSKULL), special))
	return;
      break;

      // power ups
    case MT_INV:
      if (!GivePower(pw_invulnerability))
	return;
      player->SetMessage(GOTINVUL);
      break;

    case MT_BERSERKPACK:
      if (!GivePower(pw_strength))
	return;
      player->SetMessage(GOTBERSERK);
      if (readyweapon != wp_fist)
	pendingweapon = wp_fist;
      break;

    case MT_INS:
      if (!GivePower(pw_invisibility))
	return;
      player->SetMessage(GOTINVIS);
      break;

    case MT_RADSUIT:
      if (!GivePower(pw_ironfeet))
	return;
      player->SetMessage(GOTSUIT);
      break;

    case MT_MAPSCROLL:
    case MT_COMPUTERMAP:
      if (!GivePower(pw_allmap))
	return;
      if (stype == MT_MAPSCROLL)
	player->SetMessage(text[TXT_ITEMSUPERMAP]);
      else
	player->SetMessage(GOTMAP);
      break;

    case MT_IRVISOR:
      if (!GivePower (pw_infrared))
	return;
      player->SetMessage(GOTVISOR);
      break;

      // Mana
    case MT_MANA1:
      if (!GiveAmmo(am_mana1, 15))
	return;
      player->SetMessage(text[TXT_MANA_1]);
      break;
    case MT_MANA2:
      if (!GiveAmmo(am_mana2, 15))
	return;
      player->SetMessage(text[TXT_MANA_2]);
      break;
    case MT_MANA3:
      if (GiveAmmo(am_mana1, 20))
	{
	  if (!GiveAmmo(am_mana2, 20))
	    return;
	}
      else
	GiveAmmo(am_mana2, 20);
      player->SetMessage(text[TXT_MANA_BOTH]);
      break;

      // heretic Ammo
    case MT_AMGWNDWIMPY:
      if(!GiveAmmo(am_goldwand, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOGOLDWAND1);
      break;

    case MT_AMGWNDHEFTY:
      if(!GiveAmmo(am_goldwand, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOGOLDWAND2);
      break;

    case MT_AMMACEWIMPY:
      if(!GiveAmmo(am_mace, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOMACE1);
      break;

    case MT_AMMACEHEFTY:
      if(!GiveAmmo(am_mace, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOMACE2);
      break;

    case MT_AMCBOWWIMPY:
      if(!GiveAmmo(am_crossbow, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOCROSSBOW1);
      break;

    case MT_AMCBOWHEFTY:
      if(!GiveAmmo(am_crossbow, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOCROSSBOW2);
      break;

    case MT_AMBLSRWIMPY:
      if(!GiveAmmo(am_blaster, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOBLASTER1);
      break;

    case MT_AMBLSRHEFTY:
      if(!GiveAmmo(am_blaster, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOBLASTER2);
      break;

    case MT_AMSKRDWIMPY:
      if(!GiveAmmo(am_skullrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOSKULLROD1);
      break;

    case MT_AMSKRDHEFTY:
      if(!GiveAmmo(am_skullrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOSKULLROD2);
      break;

    case MT_AMPHRDWIMPY:
      if(!GiveAmmo(am_phoenixrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOPHOENIXROD1);
      break;

    case MT_AMPHRDHEFTY:
      if(!GiveAmmo(am_phoenixrod, special->health))
	return;
      if( cv_showmessages.value==1 )
	player->SetMessage(GOT_AMMOPHOENIXROD2);
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
	player->SetMessage(GOTCLIP);
      break;

    case MT_AMMOBOX:
      if (!GiveAmmo (am_clip,5*clipammo[am_clip]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTCLIPBOX);
      break;

    case MT_ROCKETAMMO:
      if (!GiveAmmo (am_misl,clipammo[am_misl]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTROCKET);
      break;

    case MT_ROCKETBOX:
      if (!GiveAmmo (am_misl,5*clipammo[am_misl]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTROCKBOX);
      break;

    case MT_CELL:
      if (!GiveAmmo (am_cell,clipammo[am_cell]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTCELL);
      break;

    case MT_CELLPACK:
      if (!GiveAmmo (am_cell,5*clipammo[am_cell]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTCELLBOX);
      break;

    case MT_SHELL:
      if (!GiveAmmo (am_shell,clipammo[am_shell]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTSHELLS);
      break;

    case MT_SHELLBOX:
      if (!GiveAmmo (am_shell,5*clipammo[am_shell]))
	return;
      if(cv_showmessages.value==1)
	player->SetMessage(GOTSHELLBOX);
      break;

    case MT_BACKPACK:
      if (!backpack)
        {
	  maxammo = maxammo2;
	  backpack = true;
        }
      for (int i=0 ; i<am_heretic ; i++)
	GiveAmmo (ammotype_t(i), clipammo[i]);
      player->SetMessage(GOTBACKPACK);
      break;

    case MT_BAGOFHOLDING:
      if (!backpack)
        {
	  maxammo = maxammo2;
	  backpack = true;
        }
      GiveAmmo(am_goldwand, AMMO_GWND_WIMPY);
      GiveAmmo(am_blaster, AMMO_BLSR_WIMPY);
      GiveAmmo(am_crossbow, AMMO_CBOW_WIMPY);
      GiveAmmo(am_skullrod, AMMO_SKRD_WIMPY);
      GiveAmmo(am_phoenixrod, AMMO_PHRD_WIMPY);
      player->SetMessage(text[TXT_ITEMBAGOFHOLDING]);
      break;

        // weapons
    case MT_BFG9000:
      if (!GiveWeapon (wp_bfg, false) )
	return;
      player->SetMessage(GOTBFG9000);
      break;

    case MT_CHAINGUN:
      if (!GiveWeapon (wp_chaingun, special->flags & MF_DROPPED) )
	return;
      player->SetMessage(GOTCHAINGUN);
      break;

    case MT_SHAINSAW:
      if (!GiveWeapon (wp_chainsaw, false) )
	return;
      player->SetMessage(GOTCHAINSAW);
      break;

    case MT_ROCKETLAUNCH:
      if (!GiveWeapon (wp_missile, false) )
	return;
      player->SetMessage(GOTLAUNCHER);
      break;

    case MT_PLASMAGUN:
      if (!GiveWeapon (wp_plasma, false) )
	return;
      player->SetMessage(GOTPLASMA);
      break;

    case MT_SHOTGUN:
      if (!GiveWeapon (wp_shotgun, special->flags&MF_DROPPED ) )
	return;
      player->SetMessage(GOTSHOTGUN);
      break;

    case MT_SUPERSHOTGUN:
      if (!GiveWeapon (wp_supershotgun, special->flags&MF_DROPPED ) )
	return;
      player->SetMessage(GOTSHOTGUN2);
      break;

      // heretic weapons
    case MT_WMACE:
      if(!GiveWeapon(wp_mace,false))
	return;
      player->SetMessage(GOT_WPNMACE);
      break;
    case MT_WCROSSBOW:
      if(!GiveWeapon(wp_crossbow,false))
	return;
      player->SetMessage(GOT_WPNCROSSBOW);
      break;
    case MT_WBLASTER:
      if(!GiveWeapon(wp_blaster,false))
	return;
      player->SetMessage(GOT_WPNBLASTER);
      break;
    case MT_WSKULLROD:
      if(!GiveWeapon(wp_skullrod, false))
	return;
      player->SetMessage(GOT_WPNSKULLROD);
      break;
    case MT_WPHOENIXROD:
      if(!GiveWeapon(wp_phoenixrod, false))
	return;
      player->SetMessage(GOT_WPNPHOENIXROD);
      break;
    case MT_WGAUNTLETS:
      if(!GiveWeapon(wp_gauntlets, false))
	return;
      player->SetMessage(GOT_WPNGAUNTLETS);
      break;

      // Hexen weapons
    case MT_MW_CONE: // Frost Shards
      if(!GiveWeapon(wp_cone_of_shards, false))
	return;
      player->SetMessage(text[TXT_WEAPON_M2]);
      break;

    case MT_MW_LIGHTNING: // Arc of Death
      if(!GiveWeapon(wp_arc_of_death, false))
	return;
      player->SetMessage(text[TXT_WEAPON_M3]);
      break;

    case MT_FW_AXE: // Timon's Axe
      if(!GiveWeapon(wp_timons_axe, false))
	return;
      player->SetMessage(text[TXT_WEAPON_F2]);
      break;
    case MT_FW_HAMMER: // Hammer of Retribution
      if(!GiveWeapon(wp_hammer_of_retribution, false))
	return;
      player->SetMessage(text[TXT_WEAPON_F3]);
      break;

      // 2nd and 3rd Cleric Weapons
    case MT_CW_SERPSTAFF: // Serpent Staff
      if(!GiveWeapon(wp_serpent_staff, false))
	return;
      player->SetMessage(text[TXT_WEAPON_C2]);
      break;
    case MT_CW_FLAME: // Firestorm
    if(!GiveWeapon(wp_firestorm, false))
      return;
    player->SetMessage(text[TXT_WEAPON_C3]);
      break;

      // Fourth Weapon Pieces
    case MT_FW_SWORD1:
    case MT_FW_SWORD2:
    case MT_FW_SWORD3:
    case MT_CW_HOLY1:
    case MT_CW_HOLY2:
    case MT_CW_HOLY3:
    case MT_MW_STAFF1:
    case MT_MW_STAFF2:
    case MT_MW_STAFF3:
      if (!GiveArtifact(artitype_t(arti_fsword1 + stype - MT_FW_SWORD1), special))
	return;
      break;

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
