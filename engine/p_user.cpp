// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.15  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.14  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.13  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.12  2003/05/30 13:34:47  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.10  2003/04/26 12:01:13  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.9  2003/04/24 20:30:19  hurdler
// Remove lots of compiling warnings
//
// Revision 1.8  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.7  2003/04/14 08:58:28  smite-meister
// Hexen maps load.
//
// Revision 1.6  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.5  2003/03/08 16:07:09  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:12:03  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:04  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   part of PlayerPawn class implementation
//      Bobbing POV/weapon, movement.
//      Pending weapon.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"

#include "g_game.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"

#include "command.h"
#include "p_camera.h"

#include "d_event.h"

#include "r_sprite.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_random.h"
#include "tables.h"

#include "hardware/hw3sound.h"


//
// Movement.
//


bool PlayerPawn::Teleport(fixed_t nx, fixed_t ny, angle_t nangle)
{
  bool ret = Actor::Teleport(nx,ny,nangle);

  // don't move for a bit
  if (!powers[pw_weaponlevel2])
    reactiontime = 18;
  // added : absolute angle position
  if (this == consoleplayer->pawn)
    localangle = nangle;
  if (this == displayplayer2->pawn)
    localangle2 = nangle;

  // move chasecam at new player location
  if (camera.chase && displayplayer == player)
    camera.ResetCamera(this);
    
  // FIXME code below is useless, right?
  // Adjust player's view, in case there has been a height change
  
  // Save the current deltaviewheight, used in stepping
  //  fixed_t deltaviewheight = player->deltaviewheight;
  // Clear deltaviewheight, since we don't want any changes
  //  player->deltaviewheight = 0;
  // Set player's view according to the newly set parameters
  //  CalcHeight(player);
  // Reset the delta to have the same dynamics as before
  // player->deltaviewheight = deltaviewheight;

  return ret;
}


//
// was P_Thrust
// Moves the given origin along a given angle.
//
void PlayerPawn::Thrust(angle_t angle, fixed_t move)
{
  // now exactly like Actor::Thrust... remove?
  angle >>= ANGLETOFINESHIFT;
  px += FixedMul(move, finecosine[angle]);
  py += FixedMul(move, finesine[angle]);
}


extern int ticruned,ticmiss;

#define JUMPGRAVITY     (6*FRACUNIT/NEWTICRATERATIO)

//
// was P_MovePlayer
//
void PlayerPawn::Move()
{
  //extern int variable_friction; TODO

  ticcmd_t *cmd = &player->cmd;

#ifndef ABSOLUTEANGLE
  angle += (cmd->angleturn<<16);
#else
  angle = (cmd->angleturn<<16);
#endif

  aiming = cmd->aiming<<16;

  ticruned++;
  if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
    ticmiss++;

  fixed_t movepushforward = 0, movepushside = 0;

  //CONS_Printf("p = (%d, %d), v = %f / tic\n", px, py, sqrt(px*px+py*py)/FRACUNIT);

  float mf = GetMoveFactor();

  // limit speed = push/(1-friction) => magic multiplier = 2*(1-friction) = 0.1875
  float magic = 0.1875 * FRACUNIT * speed * mf;

  if (cmd->forwardmove)
    {
      //CONS_Printf("::m %d, %f, magic = %f\n", cmd->forwardmove, mf, magic);
      movepushforward = int(magic * cmd->forwardmove/100);
      Thrust(angle, movepushforward);
    }

  if (cmd->sidemove)
    {
      movepushside = int(magic * cmd->sidemove/100);
      Thrust(angle-ANG90, movepushside);
    }

  // mouselook swim when waist underwater
  eflags &= ~MFE_SWIMMING;
  if (eflags & MFE_UNDERWATER)
    {
      fixed_t a;
      // swim up/down full move when forward full speed
      a = FixedMul(movepushforward*50, finesine[aiming >> ANGLETOFINESHIFT] >>5 );
      
      if ( a != 0 )
	{
	  eflags |= MFE_SWIMMING;
	  pz += a;
	}
    }

  bool onground = (z <= floorz) || (flags2 & (MF2_ONMOBJ | MF2_FLY)) || (cheats & CF_FLYAROUND);

  //added:22-02-98: jumping
  if (cmd->buttons & BT_JUMP)
    {
      if (flags2 & MF2_FLY)
	flyheight = 10;
      else if (eflags & MFE_UNDERWATER)
	//TODO: goub gloub when push up in water
	pz = JUMPGRAVITY/2;
      else if (onground && !jumpdown) 
	// can't jump while in air, can't jump while jumping
	{
	  pz = JUMPGRAVITY;
	  if (!(cheats & CF_FLYAROUND))
	    {
	      S_StartScreamSound(this, sfx_jump);
	      // keep jumping ok if FLY mode.
	      jumpdown = true;
	    }
	}
    }
  else
    jumpdown = false;

  if (cmd->forwardmove || cmd->sidemove)
    {
      // set the running state if nothing more important is going on
      int anim = pres->GetAnim();
      if (anim == presentation_t::Idle)
	pres->SetAnim(presentation_t::Run);
    }

  if (game.mode == gm_heretic && (cmd->angleturn & BT_FLYDOWN))
    {
      flyheight = -10;
    }
  /* HERETODO
     fly = cmd->lookfly>>4;
     if(fly > 7)
     fly -= 16;
     if(fly && player->powers[pw_flight])
     {
     if(fly != TOCENTER)
     {
     player->flyheight = fly*2;
     if(!(flags2&MF2_FLY))
     {
     flags2 |= MF2_FLY;
     flags |= MF_NOGRAVITY;
     }
     }
     else
     {
     flags2 &= ~MF2_FLY;
     flags &= ~MF_NOGRAVITY;
     }
     }
     else if(fly > 0)
     {
     P_PlayerUseArtifact(player, arti_fly);
     }*/
  if (flags2 & MF2_FLY)
    {
      pz = flyheight*FRACUNIT;
      if (flyheight)
	flyheight /= 2;
    }
}



//----------------------------------------------------------------------------
//
// PROC P_ArtiTele
//
//----------------------------------------------------------------------------

void P_ArtiTele(PlayerPawn *p)
{
  fixed_t destX;
  fixed_t destY;
  angle_t destAngle;
  extern  consvar_t  cv_deathmatch;
  int i;

  mapthing_t *m;
  if (cv_deathmatch.value)
    {
      int n = p->mp->dmstarts.size();
      i = P_Random() % n;
      m = p->mp->dmstarts[i];
    }
  else
    {
      multimap<int, mapthing_t *>::iterator s;
      s = p->mp->playerstarts.begin(); // TODO probably not the smartest possible behavior
      m = (*s).second;
    }
  destX = m->x << FRACBITS;
  destY = m->y << FRACBITS;
  destAngle = ANG45*(m->angle/45);

  p->Teleport(destX, destY, destAngle);
  S_StartAmbSound(sfx_wpnup); // Full volume laugh
}


//----------------------------------------------------------------------------
//
// FUNC P_UseArtifact
//
// Returns true if artifact was used.
//
//----------------------------------------------------------------------------

bool P_UseArtifact(PlayerPawn *p, artitype_t arti)
{
  DActor *mo;
  angle_t ang;
  int count;

  switch(arti)
    {
    case arti_invulnerability:
      return p->GivePower(pw_invulnerability);

    case arti_invisibility:
      return p->GivePower(pw_invisibility);

    case arti_health:
      return p->GiveBody(25);

    case arti_superhealth:
      return p->GiveBody(100);

    case arti_tomeofpower:
      if (p->morphTics)
        { // Attempt to undo chicken
	  if (p->UndoMorph() == false)
            { // Failed
	      p->Damage(NULL, NULL, 10000, dt_always);
            }
	  else
            { // Succeeded
	      p->morphTics = 0;
	      S_StartScreamSound(p, sfx_wpnup);
            }
        }
      else
        {
	  if (!p->GivePower(pw_weaponlevel2))
	    return false;

	  if (p->readyweapon == wp_staff)
	    p->SetPsprite(ps_weapon, S_STAFFREADY2_1);
	  else if (p->readyweapon == wp_gauntlets)
	    p->SetPsprite(ps_weapon, S_GAUNTLETREADY2_1);
        }
      break;

    case arti_torch:
      return p->GivePower(pw_infrared);

    case arti_firebomb:
      ang = p->angle >> ANGLETOFINESHIFT;
      mo = p->mp->SpawnDActor(p->x+24*finecosine[ang], p->y+24*finesine[ang], p->z - p->floorclip, MT_FIREBOMB);
      mo->owner = p;
      break;

    case arti_egg:
      p->SpawnPlayerMissile(MT_EGGFX);
      p->SPMAngle(MT_EGGFX, p->angle-(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->angle+(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->angle-(ANG45/3));
      p->SPMAngle(MT_EGGFX, p->angle+(ANG45/3));
      break;

    case arti_fly:
      return p->GivePower(pw_flight);

    case arti_teleport:
      P_ArtiTele(p);
      break;

    case arti_healingradius:
      //return P_HealRadius(p);
      break;

    case arti_summon:
      mo = p->SpawnPlayerMissile(MT_SUMMON_FX);
      if (mo)
	{
	  mo->owner = p;
	  mo->pz = 5*FRACUNIT;
	}
      break;

    case arti_pork:
      p->SpawnPlayerMissile(MT_XEGGFX);
      p->SPMAngle(MT_XEGGFX, p->angle-(ANG45/6));
      p->SPMAngle(MT_XEGGFX, p->angle+(ANG45/6));
      p->SPMAngle(MT_XEGGFX, p->angle-(ANG45/3));
      p->SPMAngle(MT_XEGGFX, p->angle+(ANG45/3));
      break;

    case arti_blastradius:
      //P_BlastRadius(p);
      break;

    case arti_poisonbag:
      /* FIXME flechettes
      ang = p->angle >> ANGLETOFINESHIFT;
      if (p->pclass == PCLASS_CLERIC)
	{
	  mo = P_SpawnMobj(p->x+16*finecosine[angle], p->y+24*finesine[angle],
			   player->mo->z - p->floorclip+8*FRACUNIT, MT_POISONBAG);
	  if (mo)
	    mo->owner = p;
	}
      else if (p->pclass == PCLASS_MAGE)
	{
	  mo = P_SpawnMobj(p->x+16*finecosine[angle],
			   p->y+24*finesine[angle], player->mo->z-
			   p->floorclip+8*FRACUNIT, MT_FIREBOMB);
	  if (mo)
	    mo->owner = p;
	}			
      else // PCLASS_FIGHTER, obviously (also pig, not so obviously)
	{
	  mo = P_SpawnMobj(p->x, p->y, p->z - p->floorclip+35*FRACUNIT, MT_THROWINGBOMB);
	  if (mo)
	    {
	      mo->angle = p->angle+(((P_Random()&7)-4)<<24);
	      mo->pz = 4*FRACUNIT+((player->lookdir)<<(FRACBITS-4));
	      mo->z += player->lookdir<<(FRACBITS-4);
	      P_ThrustMobj(mo, mo->angle, mo->info->speed);
	      mo->px += p->px>>1;
	      mo->py += p->py>>1;
	      mo->owner = p;
	      mo->tics -= P_Random()&3;
	      P_CheckMissileSpawn(mo);											
	    } 
	}
      break;
      */
    case arti_teleportother:
      //P_ArtiTeleportOther(p);
      break;

    case arti_speed:
      return p->GivePower(pw_speed);

    case arti_boostmana:
      if (!p->GiveAmmo(am_mana1, 200))
	return p->GiveAmmo(am_mana2, 200);
      else
	p->GiveAmmo(am_mana2, 200);
      break;

    case arti_boostarmor:
      count = 0;
      for (int i = armor_armor; i < NUMARMOR; ++i)
	count += p->GiveArmor(armortype_t(i), -3.0, 1); // 1 point per armor type
      return count;

    case arti_puzzskull:
    case arti_puzzgembig:
    case arti_puzzgemred:
    case arti_puzzgemgreen1:
    case arti_puzzgemgreen2:
    case arti_puzzgemblue1:
    case arti_puzzgemblue2:
    case arti_puzzbook1:
    case arti_puzzbook2:
    case arti_puzzskull2:
    case arti_puzzfweapon:
    case arti_puzzcweapon:
    case arti_puzzmweapon:
    case arti_puzzgear1:
    case arti_puzzgear2:
    case arti_puzzgear3:
    case arti_puzzgear4:
      /*
	TODO puzzle items
      if (p->UsePuzzleItem(arti - arti_firstpuzzitem))
	return true;
      else
	{
	  P_SetYellowMessage(player, TXT_USEPUZZLEFAILED, false);
	  return false;
	}
      */
      break;

    default:
      return false;
    }
  return true;
}
