// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// Revision 1.10  2003/04/14 08:58:26  smite-meister
// Hexen maps load.
//
// Revision 1.9  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.8  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.7  2003/03/15 20:07:16  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.6  2003/03/08 16:07:07  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.5  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.4  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:11:35  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:17:58  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Heretic/Hexen specific extra game routines, gametype patching
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_player.h"

#include "p_enemy.h"
#include "p_maputl.h"
#include "r_main.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_random.h"
#include "dstrings.h"
#include "p_heretic.h"
#include "wi_stuff.h"

//---------------------------------------------------------------------------
// P_MinotaurSlam

void P_MinotaurSlam(Actor *source, Actor *target)
{
  angle_t angle;
  fixed_t thrust;
    
  angle = R_PointToAngle2(source->x, source->y, target->x, target->y);
  angle >>= ANGLETOFINESHIFT;
  thrust = 16*FRACUNIT+(P_Random()<<10);
  target->px += FixedMul(thrust, finecosine[angle]);
  target->py += FixedMul(thrust, finesine[angle]);
  target->Damage(NULL, NULL, HITDICE(6));

  /*
  if(target->player)
    {
      target->reactiontime = 14+(P_Random()&7);
    }
  */
}

//---------------------------------------------------------------------------
// P_TouchWhirlwind

bool P_TouchWhirlwind(Actor *target)
{
  int randVal;
    
  target->angle += P_SignedRandom()<<20;
  target->px += P_SignedRandom()<<10;
  target->py += P_SignedRandom()<<10;
  if (target->mp->maptic & 16 && !(target->flags2 & MF2_BOSS))
    {
      randVal = P_Random();
      if(randVal > 160)
        {
	  randVal = 160;
        }
      target->pz += randVal<<10;
      if(target->pz > 12*FRACUNIT)
        {
	  target->pz = 12*FRACUNIT;
        }
    }
  if(!(target->mp->maptic & 7))
    {
      return target->Damage(NULL, NULL, 3);
    }
  return false;
}


//----------------------------------------------------------------------------
//
// was P_FaceMobj
//
// Returns 1 if 'source' needs to turn clockwise, or 0 if 'source' needs
// to turn counter clockwise.  'delta' is set to the amount 'source'
// needs to turn.
//
//----------------------------------------------------------------------------
int P_FaceMobj(Actor *source, Actor *target, angle_t *delta)
{
  angle_t diff;
  angle_t angle1;
  angle_t angle2;

  angle1 = source->angle;
  angle2 = R_PointToAngle2(source->x, source->y, target->x, target->y);
  if(angle2 > angle1)
    {
      diff = angle2-angle1;
      if(diff > ANGLE_180)
	{
	  *delta = ANGLE_MAX-diff;
	  return(0);
	}
      else
	{
	  *delta = diff;
	  return(1);
	}
    }
  else
    {
      diff = angle1-angle2;
      if(diff > ANGLE_180)
	{
	  *delta = ANGLE_MAX-diff;
	  return(1);
	}
      else
	{
	  *delta = diff;
	  return(0);
	}
    }
}

//----------------------------------------------------------------------------
//
// was P_SeekerMissile
//
// Returns true if target was tracked, false if not.
//
//----------------------------------------------------------------------------

bool DActor::SeekerMissile(angle_t thresh, angle_t turnMax)
{
  int dir;
  angle_t delta;
  angle_t ang;

  Actor *t = target;

  if (t == NULL)
    return false;
   
  if (!(t->flags & MF_SHOOTABLE))
    { // Target died
      target = NULL;
      return false;
    }
  dir = P_FaceMobj(this, t, &delta);
  if (delta > thresh)
    {
      delta >>= 1;
      if (delta > turnMax)
	delta = turnMax;
    }
  if (dir)
    { // Turn clockwise
      angle += delta;
    }
  else
    { // Turn counter clockwise
      angle -= delta;
    }
  ang = angle>>ANGLETOFINESHIFT;
  px = int(info->speed * finecosine[ang]);
  py = int(info->speed * finesine[ang]);
  if (z+height < t->z || t->z+t->height < z)
    { // Need to seek vertically
      int dist = P_AproxDistance(t->x-x, t->y-y);
      dist = dist / int(info->speed * FRACUNIT);
      if (dist < 1)
	dist = 1;
      pz = (t->z+(t->height>>1) - (z+(height>>1))) / dist;
    }
  return true;
}

//---------------------------------------------------------------------------
//
// was P_SpawnMissileAngle
//
// Returns NULL if the missile exploded immediately, otherwise returns
// a Actor pointer to the missile.
//
//---------------------------------------------------------------------------

DActor *DActor::SpawnMissileAngle(mobjtype_t t, angle_t angle, fixed_t momz)
{
  fixed_t mz;

  switch(t)
    {
    case MT_MNTRFX1: // Minotaur swing attack missile
      mz = z+40*FRACUNIT;
      break;
    case MT_MNTRFX2: // Minotaur floor fire missile
      mz = ONFLOORZ; // +floorclip; 
      break;
    case MT_SRCRFX1: // Sorcerer Demon fireball
      mz = z+48*FRACUNIT;
      break;
    case MT_ICEGUY_FX2: // Secondary Projectiles of the Ice Guy
      mz = z+3*FRACUNIT;
      break;
    case MT_MSTAFF_FX2:
      mz = z+40*FRACUNIT;
      break;
    default:
      mz = z+32*FRACUNIT;
      break;

    }
  if (flags2 & MF2_FEETARECLIPPED)
    mz -= FOOTCLIPSIZE;
    
  DActor *mo = mp->SpawnDActor(x, y, mz, t);
  if (mo->info->seesound)
    S_StartSound(mo, mo->info->seesound);

  mo->owner = this; // Originator
  mo->angle = angle;
  angle >>= ANGLETOFINESHIFT;
  mo->px = int(mo->info->speed * finecosine[angle]);
  mo->py = int(mo->info->speed * finesine[angle]);
  mo->pz = momz;
  return (mo->CheckMissileSpawn() ? mo : NULL);
}



extern int item_pickup_sound;

void DoomPatchEngine()
{
  Intermission::s_count = sfx_pistol;
  ceiling_t::ceilmovesound = sfx_stnmov;
  vldoor_t::doorclosesound = sfx_dorcls;
  button_t::buttonsound = sfx_swtchn;
  game.inventory = false;

  item_pickup_sound = sfx_itemup;
}

void HexenPatchEngine()
{
  // FIXME sounds
  Intermission::s_count = SFX_SWITCH1;
  ceiling_t::ceilmovesound = SFX_SWITCH1;
  vldoor_t::doorclosesound = SFX_SWITCH1;
  button_t::buttonsound = SFX_SWITCH1;
  game.inventory = true;

  item_pickup_sound = SFX_PICKUP_ITEM;


  sprnames[SPR_BLUD] = "BLOD";
}

void HereticPatchEngine()
{
  Intermission::s_count = sfx_keyup;
  ceiling_t::ceilmovesound = sfx_dormov;
  vldoor_t::doorclosesound = sfx_doropn;
  button_t::buttonsound = sfx_switch;
  game.inventory = true;

  item_pickup_sound = sfx_hitemup;
  // FIXME rationalize here. Above, good. Below, bad.

  // instead of this, make a default skin (marine, heretic)
  // with appropriate sounds.
  strcpy(S_sfx[sfx_oof].lumpname, "plroof");
  S_sfx[sfx_oof].priority    = 32;

  text[PD_BLUEK_NUM]   = "YOU NEED A BLUE KEY TO OPEN THIS DOOR";
  text[PD_YELLOWK_NUM] = "YOU NEED A YELLOW KEY TO OPEN THIS DOOR";
  text[PD_REDK_NUM]    = "YOU NEED A GREEN KEY TO OPEN THIS DOOR";

  text[GOTARMOR_NUM] = "SILVER SHIELD";
  text[GOTMEGA_NUM ] = "ENCHANTED SHIELD";
  text[GOTSTIM_NUM ] = "CRYSTAL VIAL";
  text[GOTMAP_NUM  ] = "MAP SCROLL";
  text[GOTBLUECARD_NUM] = "BLUE KEY";
  text[GOTYELWCARD_NUM] = "YELLOW KEY";
  text[GOTREDCARD_NUM ] = "GREEN KEY";

  S_sfx[sfx_telept].priority = 50;

  // console alert
  strcpy(S_sfx[sfx_tink].lumpname, "chat");
  S_sfx[sfx_tink].priority = 100;
  // item respawns
  strcpy(S_sfx[sfx_itmbk].lumpname, "respawn");
  S_sfx[sfx_itmbk].priority = 10;

  // teleport fog
  mobjinfo[MT_TFOG].spawnstate = S_HTFOG1;
  // guess
  sprnames[SPR_BLUD] = "BLOD";
}

static DActor *LavaInflictor;

//----------------------------------------------------------------------------
//
// PROC P_InitLava
//
//----------------------------------------------------------------------------

void P_InitLava()
{
  LavaInflictor = new DActor(MT_PHOENIXFX2);
  LavaInflictor->flags =  MF_NOBLOCKMAP | MF_NOGRAVITY;
  LavaInflictor->flags2 = MF2_FIREDAMAGE|MF2_NODMGTHRUST;
}

//----------------------------------------------------------------------------
//
// PROC P_HerePlayerInSpecialSector
//
// Called every tic frame that the player origin is in a special sector.
//
//----------------------------------------------------------------------------

void PlayerPawn::HerePlayerInSpecialSector()
{
  static int pushTab[5] = {
    2048*5,
    2048*10,
    2048*25,
    2048*30,
    2048*35
  };
    
  sector_t *sector = subsector->sector;
  // Player is not touching the floor
  if (z != sector->floorheight)
    return;
    
  switch (sector->special)
    {
    case 7: // Damage_Sludge
      if(!(mp->maptic & 31))
        {
	  Damage(NULL, NULL, 4);
        }
      break;
    case 5: // Damage_LavaWimpy
      if(!(mp->maptic & 15))
        {
	  Damage(LavaInflictor, NULL, 5);
	  HitFloor();
        }
      break;
    case 16: // Damage_LavaHefty
      if(!(mp->maptic & 15))
        {
	  Damage(LavaInflictor, NULL, 8);
	  HitFloor();
        }
      break;
    case 4: // Scroll_EastLavaDamage
      Thrust(0, 2048*28);
      if(!(mp->maptic & 15))
        {
	  Damage(LavaInflictor, NULL, 5);
	  HitFloor();
        }
      break;
    case 9: // SecretArea
      player->secrets++;
      sector->special = 0;
      break;
    case 11: // Exit_SuperDamage (DOOM E1M8 finale)
      /*
	cheats &= ~CF_GODMODE;
	if(!(mp->maptic &0x1f))
	{
	Damage(NULL, NULL, 20);
	}
	if(health <= 10)
	{
	G_ExitLevel();
	}
      */
      break;
        
    case 25: case 26: case 27: case 28: case 29: // Scroll_North
      Thrust(ANG90, pushTab[sector->special-25]);
      break;
    case 20: case 21: case 22: case 23: case 24: // Scroll_East
      Thrust(0, pushTab[sector->special-20]);
      break;
    case 30: case 31: case 32: case 33: case 34: // Scroll_South
      Thrust(ANG270, pushTab[sector->special-30]);
      break;
    case 35: case 36: case 37: case 38: case 39: // Scroll_West
      Thrust(ANG180, pushTab[sector->special-35]);
      break;
        
    case 40: case 41: case 42: case 43: case 44: case 45:
    case 46: case 47: case 48: case 49: case 50: case 51:
      // Wind specials are handled in (P_mobj):P_XYMovement
      break;
        
    case 15: // Friction_Low
      // Only used in (P_mobj):P_XYMovement and (P_user):P_Thrust
      break;
        
    default:
      CONS_Printf("P_PlayerInSpecialSector: "
		  "unknown special %i\n", sector->special);
    }
}


// was P_GetThingFloorType
/*
int Actor::GetThingFloorType()
{
  return subsector->sector->floortype;
}
*/
