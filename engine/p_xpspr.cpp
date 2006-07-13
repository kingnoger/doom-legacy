// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003-2005 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Hexen weapon action functions

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_map.h"

#include "command.h"

#include "p_maputl.h"
#include "m_random.h"

#include "p_pspr.h"
#include "p_enemy.h"

#include "sounds.h"
#include "tables.h"


void A_UnHideThing(DActor *actor);
int P_FaceMobj(Actor *source, Actor *target, angle_t *delta);

extern fixed_t FloatBobOffsets[64];
extern mobjtype_t PuffType;
extern Actor *PuffSpawned;


//============================================================================
//
//	AdjustPlayerAngle
//
//============================================================================

#define MAX_ANGLE_ADJUST (5*ANGLE_1)

void AdjustPlayerAngle(Actor *pmo)
{
  angle_t angle;
  int difference;

  angle = R_PointToAngle2(pmo->pos, linetarget->pos);
  difference = int(angle) - int(pmo->yaw);
  if(abs(difference) > MAX_ANGLE_ADJUST)
    {
      pmo->yaw += difference > 0 ? MAX_ANGLE_ADJUST : -MAX_ANGLE_ADJUST;
    }
  else
    {
      pmo->yaw = angle;
    }
}

//============================================================================
//
// A_SnoutAttack
//
//============================================================================

void A_SnoutAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  fixed_t slope;

  damage = 3+(P_Random()&3);
  angle = player->yaw;
  slope = player->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_SNOUTPUFF;
  PuffSpawned = NULL;
  player->LineAttack(angle, MELEERANGE, slope, damage);
  S_StartSound(player, SFX_PIG_ACTIVE1+(P_Random()&1));
  if(linetarget)
    {
      AdjustPlayerAngle(player);
      //		player->yaw = R_PointToAngle2(player->x,
      //			player->y, linetarget->x, linetarget->y);
      if(PuffSpawned)
	{ // Bit something
	  S_StartSound(player, SFX_PIG_ATTACK);
	}
    }
}

//============================================================================
//
// A_FHammerAttack
//
//============================================================================

#define HAMMER_RANGE	(MELEERANGE+MELEERANGE/2)

void A_FHammerAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  fixed_t slope;
  int i;

  int damage = 60+(P_Random()&63);
  fixed_t power = 10;
  PuffType = MT_HAMMERPUFF;
  for(i = 0; i < 16; i++)
    {
      angle = player->yaw+i*(ANG45/32);
      slope = player->AimLineAttack(angle, HAMMER_RANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, HAMMER_RANGE, slope, damage);
	  AdjustPlayerAngle(player);
	  if (linetarget->flags & MF_SHOOTABLE)
	    //was (linetarget->flags&MF_COUNTKILL || linetarget->player)
	    {
	      linetarget->Thrust(angle, power);
	    }
	  player->attackphase = false; // Don't throw a hammer
	  goto hammerdone;
	}
      angle = player->yaw-i*(ANG45/32);
      slope = player->AimLineAttack(angle, HAMMER_RANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, HAMMER_RANGE, slope, damage);
	  AdjustPlayerAngle(player);
	  if (linetarget->flags & MF_SHOOTABLE)
	    //(linetarget->flags&MF_COUNTKILL || linetarget->player)
	    {
	      linetarget->Thrust(angle, power);
	    }
	  player->attackphase = false; // Don't throw a hammer
	  goto hammerdone;
	}
    }
  // didn't find any targets in meleerange, so set to throw out a hammer
  PuffSpawned = NULL;
  angle = player->yaw;
  slope = player->AimLineAttack(angle, HAMMER_RANGE);
  player->LineAttack(angle, HAMMER_RANGE, slope, damage);
  if(PuffSpawned)
    {
      player->attackphase = false;
    }
  else
    {
      player->attackphase = true;
    }
 hammerdone:
  if (player->ammo[am_mana2] < player->weaponinfo[player->readyweapon].ammopershoot)
    { // Don't spawn a hammer if the player doesn't have enough mana
      player->attackphase = false;
    }
  return;		
}

//============================================================================
//
// A_FHammerThrow
//
//============================================================================

void A_FHammerThrow(PlayerPawn *player, pspdef_t *psp)
{
  if (!player->attackphase)
    return;

  player->ammo[am_mana2] -= player->weaponinfo[player->readyweapon].ammopershoot;
  DActor *mo = player->SpawnPlayerMissile(MT_HAMMER_MISSILE); 
  if (mo)
    mo->special1 = 0;
}

//============================================================================
//
// A_FSwordAttack
//
//============================================================================

void A_FSwordAttack(PlayerPawn *player, pspdef_t *psp)
{
  int mana = player->weaponinfo[player->readyweapon].ammopershoot;
  player->ammo[am_mana1] -= mana;
  player->ammo[am_mana2] -= mana;

  angle_t an = player->yaw;
  DActor *m;
  if ((m = player->SPMAngle(MT_FSWORD_MISSILE, an + ANG45/4)))
    m->pos.z -= 10;
  if ((m = player->SPMAngle(MT_FSWORD_MISSILE, an+ANG45/8)))
    m->pos.z -= 5;
  player->SPMAngle(MT_FSWORD_MISSILE, an);
  if ((m = player->SPMAngle(MT_FSWORD_MISSILE, an-ANG45/8)))
    m->pos.z += 5;
  if ((m = player->SPMAngle(MT_FSWORD_MISSILE, an-ANG45/4)))
    m->pos.z += 10;
  S_StartSound(player, SFX_FIGHTER_SWORD_FIRE);
}

//============================================================================
//
// A_FSwordAttack2
//
//============================================================================

void A_FSwordAttack2(DActor *actor)
{
  angle_t angle = actor->yaw;

  actor->SpawnMissileAngle(MT_FSWORD_MISSILE,angle+ANG45/4);
  actor->SpawnMissileAngle(MT_FSWORD_MISSILE,angle+ANG45/8);
  actor->SpawnMissileAngle(MT_FSWORD_MISSILE,angle);
  actor->SpawnMissileAngle(MT_FSWORD_MISSILE,angle-ANG45/8);
  actor->SpawnMissileAngle(MT_FSWORD_MISSILE,angle-ANG45/4);
  S_StartSound(actor, SFX_FIGHTER_SWORD_FIRE);
}

//============================================================================
//
// A_FSwordFlames
//
//============================================================================

void A_FSwordFlames(DActor *actor)
{
  for (int i = 1+(P_Random()&3); i; i--)
    {
      fixed_t x = P_SFRandom(4);
      fixed_t y = P_SFRandom(4);
      fixed_t z = P_SFRandom(5);
      vec_t<fixed_t> temp(x,y,z);
      temp += actor->pos;
      actor->mp->SpawnDActor(temp, MT_FSWORD_FLAME);
    }
}

//----------------------------------------------------------------------------
// Thinking function for mage wand projectile / cleric flame

void DActor::XBlasterMissileThink()
{
  fixed_t xfrac;
  fixed_t yfrac;
  fixed_t zfrac;
  bool changexy;
  DActor *mo;

  // Handle movement
  if(vel.x != 0 || vel.y != 0|| pos.z != floorz || vel.z != 0)
    {
      xfrac = vel.x>>3;
      yfrac = vel.y>>3;
      zfrac = vel.z>>3;
      changexy = xfrac != 0 || yfrac != 0;
      for(int i = 0; i < 8; i++)
	{
	  if (changexy)
	    {
	      if(!TryMove(pos.x+xfrac, pos.y+yfrac, true))
		{ // Blocked move
		  ExplodeMissile();
		  return;
		}
	    }
	  pos.z += zfrac;
	  if(pos.z <= floorz)
	    { // Hit the floor
	      pos.z = floorz;
	      HitFloor();
	      ExplodeMissile();
	      return;
	    }
	  if(Top() > ceilingz)
	    { // Hit the ceiling
	      pos.z = ceilingz-height;
	      ExplodeMissile();
	      return;
	    }
	  if(changexy)
	    {
	      fixed_t mz;
	      if(type == MT_MWAND_MISSILE && (P_Random() < 128))
		{
		  mz = pos.z-8;
		  if (mz < floorz)
 		    mz = floorz;

		  mp->SpawnDActor(pos.x, pos.y, mz, MT_MWANDSMOKE);
		}
	      else if(!--special1)
		{
		  special1 = 4;
		  mz = pos.z-12;
		  if(mz < floorz)
		    mz = floorz;

		  mo = mp->SpawnDActor(pos.x, pos.y, mz, MT_CFLAMEFLOOR);
		  if(mo)
		    mo->yaw = yaw;
		}
	    }
	}
    }
  // Advance the state
  if(tics != -1)
    {
      tics--;
      while(!tics)
	{
	  if(!SetState(state->nextstate))
	    { // mobj was removed
	      return;
	    }
	}
    }
}



//============================================================================
//
// A_MWandAttack
//
//============================================================================

void A_MWandAttack(PlayerPawn *player, pspdef_t *psp)
{
  Actor *mo;

  mo = player->SpawnPlayerMissile(MT_MWAND_MISSILE);

  S_StartSound(player, SFX_MAGE_WAND_FIRE);
}

// ===== Mage Lightning Weapon =====

//============================================================================
//
// A_LightningReady
//
//============================================================================
void A_WeaponReady(PlayerPawn* player, pspdef_t* psp);

void A_LightningReady(PlayerPawn *player, pspdef_t *psp)
{
  A_WeaponReady(player, psp);
  if(P_Random() < 160)
    {
      S_StartSound(player, SFX_MAGE_LIGHTNING_READY);
    }
}

//============================================================================
//
// A_LightningClip
//
//============================================================================

#define ZAGSPEED 1

void A_LightningClip(DActor *actor)
{
  Actor *t = NULL;

  if (actor->type == MT_LIGHTNING_FLOOR)
    {
      actor->pos.z = actor->floorz;
      Actor *twin = actor->target;
      //t = twin->realtarget;

      // floor lightning zig-zags, and forces the ceiling lightning to mimic
      int zigZag = P_Random();
      if ((zigZag > 128 && actor->special1 < 2) || actor->special1 < -2)
	{
	  actor->Thrust(actor->yaw+ANG90, ZAGSPEED);
	  if (twin)
	    twin->Thrust(twin->yaw+ANG90, ZAGSPEED);

	  actor->special1++;
	}
      else
	{
	  actor->Thrust(actor->yaw-ANG90, ZAGSPEED);
	  if (twin)
	    twin->Thrust(twin->yaw-ANG90, ZAGSPEED);

	  actor->special1--;
	}
    }
  else if (actor->type == MT_LIGHTNING_CEILING)
    {
      actor->pos.z = actor->ceilingz - actor->height;
      //t = actor->realtarget;
    }

  if (t)
    {
      if (t->health <= 0)
	actor->ExplodeMissile();
      else
	{
	  actor->yaw = R_PointToAngle2(actor->pos, t->pos);
	  actor->vel.x = 0;
	  actor->vel.y = 0;
	  actor->Thrust(actor->yaw, 0.5f * actor->info->speed);
	}
    }
}

//============================================================================
//
// A_LightningZap
//
//============================================================================

void A_LightningZap(DActor *actor)
{
  fixed_t deltaZ;

  A_LightningClip(actor);

  actor->health -= 8;
  if (actor->health <= 0)
    {
      actor->SetState(actor->info->deathstate);
      return;
    }

  if (actor->type == MT_LIGHTNING_FLOOR)
    deltaZ = 10;
  else
    deltaZ = -10;

  fixed_t x = ((P_Random()-128) * actor->radius) >> 8;
  fixed_t y = ((P_Random()-128) * actor->radius) >> 8;

  DActor *mo = actor->mp->SpawnDActor(actor->pos.x + x, actor->pos.y + y, actor->pos.z+deltaZ, MT_LIGHTNING_ZAP);
  if (mo)
    {
      mo->owner = actor->owner;
      mo->target = actor;
      mo->vel.x = actor->vel.x;
      mo->vel.y = actor->vel.y;
      if (actor->type == MT_LIGHTNING_FLOOR)
	mo->vel.z = 20;
      else 
	mo->vel.z = -20;
    }

  if (actor->type == MT_LIGHTNING_FLOOR && P_Random() < 160)
    S_StartSound(actor, SFX_MAGE_LIGHTNING_CONTINUOUS);
}

//============================================================================
//
// A_MLightningAttack2
//
//============================================================================

void A_MLightningAttack2(PlayerPawn *actor)
{
  // TODO I really have no idea how the lightning pair is supposed to work.
  // Do they track some target, or just zigzag on? 

  DActor *f = actor->SpawnPlayerMissile(MT_LIGHTNING_FLOOR);
  DActor *c = actor->SpawnPlayerMissile(MT_LIGHTNING_CEILING);
  if (f)
    {
      f->pos.z = ONFLOORZ;
      f->vel.z = 0;
      f->target = c; // special case, a lightning pair
      f->special1 = 0; // zigzag counter
      A_LightningZap(f);	
    }

  if (c)
    {
      c->pos.z = ONCEILINGZ;
      c->vel.z = 0;
      c->target = f; // special case, a lightning pair
      //c->owner = ; // this could in principle be used to point to the real target... but what is it?
      A_LightningZap(c);	
    }
  S_StartSound(actor, SFX_MAGE_LIGHTNING_FIRE);
}

//============================================================================
//
// A_MLightningAttack
//
//============================================================================

void A_MLightningAttack(PlayerPawn *player, pspdef_t *psp)
{
  A_MLightningAttack2(player);
  player->ammo[am_mana2] -= player->weaponinfo[player->readyweapon].ammopershoot;
}

//============================================================================
//
// A_ZapMimic
//
//============================================================================

void A_ZapMimic(DActor *actor)
{
  DActor *mo = (DActor *)actor->target;
  if (mo)
    {
      if (mo->state >= &states[mo->info->deathstate]
	  || mo->state == &states[S_FREETARGMOBJ])
	{
	  actor->ExplodeMissile();
	}
      else
	{
	  actor->vel.x = mo->vel.x;
	  actor->vel.y = mo->vel.y;
	}
    }
}

//============================================================================
//
// A_LastZap
//
//============================================================================

void A_LastZap(DActor *actor)
{
  DActor *mo = actor->mp->SpawnDActor(actor->pos, MT_LIGHTNING_ZAP);
  if(mo)
    {
      mo->SetState(S_LIGHTNING_ZAP_X1);
      mo->vel.z = 40;
    }
}

//============================================================================
//
// A_LightningRemove
//
//============================================================================

void A_LightningRemove(DActor *actor)
{
  DActor *mo = (DActor *)actor->target;
  if (mo)
    {
      mo->target = NULL;
      mo->ExplodeMissile();
    }
}


//============================================================================
//
// MStaffSpawn
//
//============================================================================
void MStaffSpawn(PlayerPawn *pmo, angle_t angle)
{
  Actor *mo = pmo->SPMAngle(MT_MSTAFF_FX2, angle);
  if (mo)
    mo->target = pmo->mp->RoughBlockSearch(mo, pmo, 10, 2);
}

//============================================================================
//
// A_MStaffAttack
//
//============================================================================
#define STARTHOLYPAL    22
#define STARTSCOURGEPAL 25
void A_MStaffAttack(PlayerPawn *p, pspdef_t *psp)
{
  int mana = p->weaponinfo[p->readyweapon].ammopershoot;
  p->ammo[am_mana1] -= mana;
  p->ammo[am_mana2] -= mana;
  angle_t angle = p->yaw;
	
  MStaffSpawn(p, angle);
  MStaffSpawn(p, angle-ANGLE_1*5);
  MStaffSpawn(p, angle+ANGLE_1*5);
  S_StartSound(p, SFX_MAGE_STAFF_FIRE);

  p->player->palette = STARTSCOURGEPAL;
}

//============================================================================
//
// A_MStaffPalette
//
//============================================================================

void A_MStaffPalette(PlayerPawn *p, pspdef_t *psp)
{
  int pal = STARTSCOURGEPAL+psp->state-(&weaponstates[S_MSTAFFATK_2]);
  if (pal == STARTSCOURGEPAL+3)
    pal = 0; // reset back to original playpal
    
  p->player->palette = pal;
}

//============================================================================
//
// A_MStaffWeave
//
//============================================================================

void A_MStaffWeave(DActor *actor)
{
  fixed_t newX, newY;

  int weaveXY = actor->special2>>16;
  int weaveZ = actor->special2&0xFFFF;
  int angle = (actor->yaw+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->pos.x - (finecosine[angle] * FloatBobOffsets[weaveXY] << 2);
  newY = actor->pos.y - (finesine[angle] * FloatBobOffsets[weaveXY] << 2);
  weaveXY = (weaveXY+6)&63;
  newX += finecosine[angle] * FloatBobOffsets[weaveXY] << 2;
  newY += finesine[angle] * FloatBobOffsets[weaveXY] << 2;
  actor->TryMove(newX, newY, true);
  actor->pos.z -= FloatBobOffsets[weaveZ]<<1;
  weaveZ = (weaveZ+3)&63;
  actor->pos.z += FloatBobOffsets[weaveZ]<<1;

  if (actor->pos.z <= actor->floorz)
    actor->pos.z = actor->floorz + 1;

  actor->special2 = weaveZ+(weaveXY<<16);
}


//============================================================================
//
// A_MStaffTrack
//
//============================================================================

void A_MStaffTrack(DActor *actor)
{
  if ((actor->target == NULL) && (P_Random()<50))
    {
      actor->target = actor->mp->RoughBlockSearch(actor, actor->owner, 10, 2);
    }
  actor->SeekerMissile(ANGLE_1*2, ANGLE_1*10);
}


//============================================================================
//
// MStaffSpawn2 - for use by mage class boss
//
//============================================================================

void MStaffSpawn2(DActor *actor, angle_t angle)
{
  Actor *mo;

  mo = actor->SpawnMissileAngle(MT_MSTAFF_FX2, angle, 0);
  if (mo)
    {
      mo->owner = actor;
      mo->target = actor->mp->RoughBlockSearch(mo, actor, 10, 2);
    }
}

//============================================================================
//
// A_MStaffAttack2 - for use by mage class boss
//
//============================================================================

void A_MStaffAttack2(DActor *actor)
{
  angle_t angle = actor->yaw;
  MStaffSpawn2(actor, angle);
  MStaffSpawn2(actor, angle-ANGLE_1*5);
  MStaffSpawn2(actor, angle+ANGLE_1*5);
  S_StartSound(actor, SFX_MAGE_STAFF_FIRE);
}

//============================================================================
//
// A_FPunchAttack
//
//============================================================================

void A_FPunchAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  fixed_t slope;

  int damage = 40+(P_Random()&15);
  fixed_t power = 2;
  PuffType = MT_PUNCHPUFF;
  for(int i = 0; i < 16; i++)
    {
      // find the target most directly in front of the player
      angle = player->yaw+i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if (!linetarget)
	{
	  // try the other side
	  angle = player->yaw-i*(ANG45/16);
	  slope = player->AimLineAttack(angle, 2*MELEERANGE);
	}

      if(linetarget)
	{
	  player->attackphase++;
	  if (player->attackphase == 3)
	    {
	      damage <<= 1;
	      power = 6;
	      PuffType = MT_HAMMERPUFF;
	    }
	  player->LineAttack(angle, 2*MELEERANGE, slope, damage);
	  if (!linetarget)
	    {
	      CONS_Printf("AimLineAttack mismatch (bug)\n");
	      return;
	    }

	  if (linetarget->flags & MF_SHOOTABLE)	  
	    //(linetarget->flags&MF_COUNTKILL || linetarget->player)
	    {
	      linetarget->Thrust(angle, power);
	    }
	  AdjustPlayerAngle(player);
	  goto punchdone;
	}
    }
  // didn't find any creatures, so try to strike any walls
  player->attackphase = 0;

  angle = player->yaw;
  slope = player->AimLineAttack(angle, MELEERANGE);
  player->LineAttack(angle, MELEERANGE, slope, damage);

 punchdone:
  if(player->attackphase == 3)
    {
      player->attackphase = 0;
      player->SetPsprite(ps_weapon, S_PUNCHATK2_1);
      S_StartSound(player, SFX_FIGHTER_GRUNT);
    }
  return;		
}

//============================================================================
//
// A_FAxeAttack
//
//============================================================================

#define AXERANGE 2.25f*MELEERANGE

void A_FAxeAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  
  fixed_t power;
  int damage;
  fixed_t slope;
  int i;
  int useMana;

  damage = 40+(P_Random()&15)+(P_Random()&7);
  power = 0;
  if(player->ammo[am_mana1] > 0)
    {
      damage <<= 1;
      power = 6;
      PuffType = MT_AXEPUFF_GLOW;
      useMana = 1;
    }
  else
    {
      PuffType = MT_AXEPUFF;
      useMana = 0;
    }
  for(i = 0; i < 16; i++)
    {
      angle = player->yaw+i*(ANG45/16);
      slope = player->AimLineAttack(angle, AXERANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, AXERANGE, slope, damage);
	  if (linetarget->flags & MF_SHOOTABLE)
	    //(linetarget->flags&MF_COUNTKILL || linetarget->player)
	    {
	      linetarget->Thrust(angle, power);
	    }
	  AdjustPlayerAngle(player);
	  useMana++; 
	  goto axedone;
	}
      angle = player->yaw-i*(ANG45/16);
      slope = player->AimLineAttack(angle, AXERANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, AXERANGE, slope, damage);
	  if (linetarget->flags & MF_SHOOTABLE)
	    //(linetarget->flags&MF_COUNTKILL)
	    {
	      linetarget->Thrust(angle, power);
	    }
	  AdjustPlayerAngle(player);
	  useMana++; 
	  goto axedone;
	}
    }
  // didn't find any creatures, so try to strike any walls
  player->attackphase = 0;

  angle = player->yaw;
  slope = player->AimLineAttack(angle, MELEERANGE);
  player->LineAttack(angle, MELEERANGE, slope, damage);

 axedone:
  if(useMana == 2)
    {
      player->ammo[am_mana1] -= 
	player->weaponinfo[player->readyweapon].ammopershoot;
      if(player->ammo[am_mana1] <= 0)
	{
	  player->SetPsprite(ps_weapon, S_FAXEATK_5);
	}
    }
  return;		
}

//===========================================================================
//
// A_CMaceAttack
//
//===========================================================================

void A_CMaceAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  fixed_t slope;
  int i;

  damage = 25+(P_Random()&15);
  PuffType = MT_HAMMERPUFF;
  for(i = 0; i < 16; i++)
    {
      angle = player->yaw+i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, 2*MELEERANGE, slope, 
		       damage);
	  AdjustPlayerAngle(player);
	  //			player->yaw = R_PointToAngle2(player->x,
	  //				player->y, linetarget->x, linetarget->y);
	  goto macedone;
	}
      angle = player->yaw-i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, 2*MELEERANGE, slope, damage);
	  AdjustPlayerAngle(player);
	  //			player->yaw = R_PointToAngle2(player->x,
	  //				player->y, linetarget->x, linetarget->y);
	  goto macedone;
	}
    }
  // didn't find any creatures, so try to strike any walls
  player->attackphase = 0;

  angle = player->yaw;
  slope = player->AimLineAttack(angle, MELEERANGE);
  player->LineAttack(angle, MELEERANGE, slope, damage);
 macedone:
  return;		
}

//============================================================================
//
// A_CStaffCheck
//
//============================================================================
#define STAFFRANGE 1.5f*MELEERANGE
void A_CStaffCheck(PlayerPawn *player, pspdef_t *psp)
{
  int damage;
  int newLife;
  angle_t angle;
  fixed_t slope;
  int i;

  damage = 20+(P_Random()&15);
  PuffType = MT_CSTAFFPUFF;
  for(i = 0; i < 3; i++)
    {
      angle = player->yaw+i*(ANG45/16);
      slope = player->AimLineAttack(angle, STAFFRANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, STAFFRANGE, slope, damage);
	  player->yaw = R_PointToAngle2(player->pos, linetarget->pos);
	  if ((linetarget->flags & (MF_COUNTKILL|MF_NOTMONSTER))
	      && !(linetarget->flags2 & (MF2_DORMANT|MF2_INVULNERABLE)))
	    {
	      newLife = player->health+(damage>>3);
	      newLife = newLife > 100 ? 100 : newLife;
	      player->health = player->health = newLife;
	      player->SetPsprite(ps_weapon, S_CSTAFFATK2_1);
	    }
	  player->ammo[am_mana1] -= 
	    player->weaponinfo[player->readyweapon].ammopershoot;
	  break;
	}
      angle = player->yaw-i*(ANG45/16);
      slope = player->AimLineAttack(angle, STAFFRANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, STAFFRANGE, slope, damage);
	  player->yaw = R_PointToAngle2(player->pos, linetarget->pos);
	  if (linetarget->flags & (MF_COUNTKILL|MF_NOTMONSTER))
	    {
	      newLife = player->health+(damage>>4);
	      newLife = newLife > 100 ? 100 : newLife;
	      player->health = newLife;
	      player->SetPsprite(ps_weapon, S_CSTAFFATK2_1);
	    }
	  player->ammo[am_mana1] -= player->weaponinfo[player->readyweapon].ammopershoot;
	  break;
	}
    }
}

//============================================================================
//
// A_CStaffAttack
//
//============================================================================

void A_CStaffAttack(PlayerPawn *player, pspdef_t *psp)
{
  player->ammo[am_mana1] -= player->weaponinfo[player->readyweapon].ammopershoot;

  DActor *mo = player->SPMAngle(MT_CSTAFF_MISSILE, player->yaw-(ANG45/15));
  if(mo)
    {
      mo->special2 = 32;
    }
  mo = player->SPMAngle(MT_CSTAFF_MISSILE, player->yaw+(ANG45/15));
  if(mo)
    {
      mo->special2 = 0;
    }
  S_StartSound(player, SFX_CLERIC_CSTAFF_FIRE);
}

//============================================================================
//
// A_CStaffMissileSlither
//
//============================================================================

void A_CStaffMissileSlither(DActor *actor)
{
  fixed_t newX, newY;

  int weaveXY = actor->special2;
  int angle = (actor->yaw+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->pos.x - finecosine[angle] * FloatBobOffsets[weaveXY];
  newY = actor->pos.y - finesine[angle] * FloatBobOffsets[weaveXY];
  weaveXY = (weaveXY+3)&63;
  newX += finecosine[angle] * FloatBobOffsets[weaveXY];
  newY += finesine[angle] * FloatBobOffsets[weaveXY];
  actor->TryMove(newX, newY, true);
  actor->special2 = weaveXY;
}

//============================================================================
//
// A_CStaffInitBlink
//
//============================================================================

void A_CStaffInitBlink(PlayerPawn *player, pspdef_t *psp)
{
  player->attackphase = (P_Random()>>1)+20;
}

//============================================================================
//
// A_CStaffCheckBlink
//
//============================================================================

void A_CStaffCheckBlink(PlayerPawn *player, pspdef_t *psp)
{
  if(!--player->attackphase)
    {
      player->SetPsprite(ps_weapon, S_CSTAFFBLINK1);
      player->attackphase = (P_Random()+50)>>2;
    }
}

//============================================================================
//
// A_CFlameAttack
//
//============================================================================

#define FLAMESPEED	0.45f
#define CFLAMERANGE	12*64

void A_CFlameAttack(PlayerPawn *player, pspdef_t *psp)
{
  DActor *mo = player->SpawnPlayerMissile(MT_CFLAME_MISSILE);
  if (mo)
    mo->special1 = 2;

  player->ammo[am_mana2] -= player->weaponinfo[player->readyweapon].ammopershoot;
  S_StartSound(player, SFX_CLERIC_FLAME_FIRE);
}

//============================================================================
//
// A_CFlamePuff
//
//============================================================================

void A_CFlamePuff(DActor *actor)
{
  A_UnHideThing(actor);
  actor->vel.Set(0,0,0);
  S_StartSound(actor, SFX_CLERIC_FLAME_EXPLODE);
}

//============================================================================
//
// A_CFlameMissile
//
//============================================================================

void A_CFlameMissile(DActor *actor)
{
  A_UnHideThing(actor);
  S_StartSound(actor, SFX_CLERIC_FLAME_EXPLODE);

  if (Blocking.thing && Blocking.thing->flags & MF_SHOOTABLE)
    {
      // Hit something, so spawn the flame circle around the thing
      fixed_t dist = Blocking.thing->radius+18;
      for (int i = 0; i < 4; i++)
	{
	  int an = (i*ANG45) >> ANGLETOFINESHIFT;
	  //int an90 = (i*ANG45 + ANG90) >> ANGLETOFINESHIFT;
	  vec_t<fixed_t> temp(dist * finecosine[an], dist * finesine[an], 5);
	  temp += Blocking.thing->pos;

	  DActor *mo = actor->mp->SpawnDActor(temp, MT_CIRCLEFLAME);
	  if (mo)
	    {
	      mo->yaw = an<<ANGLETOFINESHIFT;
	      mo->owner = actor->owner;
	      mo->vel.x = FLAMESPEED * finecosine[an];
	      mo->vel.y = FLAMESPEED * finesine[an];
	      mo->special1 = an;
	      mo->tics -= P_Random()&3;
	    }

	  temp.Set(-dist * finecosine[an], -dist * finesine[an], fixed_t(5));
	  temp += Blocking.thing->pos;

	  mo = actor->mp->SpawnDActor(temp, MT_CIRCLEFLAME);
	  if (mo)
	    {
	      mo->yaw = ANG180 + (an<<ANGLETOFINESHIFT);
	      mo->owner = actor->owner;
	      mo->vel.x = -FLAMESPEED * finecosine[an];
	      mo->vel.y = -FLAMESPEED * finesine[an];
	      mo->special1 = mo->yaw >> ANGLETOFINESHIFT;
	      mo->tics -= P_Random()&3;
	    }
	}
      actor->SetState(S_FLAMEPUFF2_1);
    }
}

/*
void A_CFlameAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
	int damage;
	int i;
	int an, an90;
	fixed_t dist;
	Actor *mo;

	P_BulletSlope(player);
	damage = 25+HITDICE(3);
	angle = player->yaw;
	if(player->refire)
	{
		angle += P_SignedRandom() << 17;
	}
	player->AimLineAttack(angle, CFLAMERANGE); // Correctly set linetarget
	if(!linetarget)
	{
		angle += ANGLE_1*2;
		player->AimLineAttack(angle, CFLAMERANGE);
		if(!linetarget)
		{
			angle -= ANGLE_1*4;
			player->AimLineAttack(angle, CFLAMERANGE);
			if(!linetarget)
			{
				angle += ANGLE_1*2;
			}
		}		
	}
	if(linetarget)
	{
		PuffType = MT_FLAMEPUFF2;
	}
	else
	{
		PuffType = MT_FLAMEPUFF;
	}
	player->LineAttack(angle, CFLAMERANGE, bulletslope, damage);
	if(linetarget)
	{ // Hit something, so spawn the flame circle around the thing
		dist = linetarget->radius+18;
		for(i = 0; i < 4; i++)
		{
			an = (i*ANG45)>>ANGLETOFINESHIFT;
			an90 = (i*ANG45+ANG90)>>ANGLETOFINESHIFT;
			mo = actor->mp->SpawnDActor(linetarget->x+FixedMul(dist, finecosine[an]),
				linetarget->y+FixedMul(dist, finesine[an]), 
				linetarget->z+5, MT_CIRCLEFLAME);
			if(mo)
			{
				mo->yaw = an<<ANGLETOFINESHIFT;
				mo->target = player;
				mo->px = mo->special1 = FixedMul(FLAMESPEED, finecosine[an]);
				mo->py = mo->special2 = FixedMul(FLAMESPEED, finesine[an]);
				mo->tics -= P_Random()&3;
			}
			mo = actor->mp->SpawnDActor(linetarget->x-FixedMul(dist, finecosine[an]),
				linetarget->y-FixedMul(dist, finesine[an]), 
				linetarget->z+5, MT_CIRCLEFLAME);
			if(mo)
			{
				mo->yaw = ANG180+(an<<ANGLETOFINESHIFT);
				mo->target = player;
				mo->px = mo->special1 = FixedMul(-FLAMESPEED, 
					finecosine[an]);
				mo->py = mo->special2 = FixedMul(-FLAMESPEED, finesine[an]);
				mo->tics -= P_Random()&3;
			}
		}
	}
// Create a line of flames from the player to the flame puff
	CFlameCreateFlames(player);

	player->ammo[am_mana2] -= player->weaponinfo[player->readyweapon].ammopershoot;
	S_StartSound(player, SFX_CLERIC_FLAME_FIRE);
}
*/

//============================================================================
//
// A_CFlameRotate
//
// special1 holds the original fineangle for the radial movement.
//
//============================================================================

#define FLAMEROTSPEED	2

void A_CFlameRotate(DActor *actor)
{
  int an = (actor->yaw + ANG90) >> ANGLETOFINESHIFT;
  int phi = actor->special1;
  actor->vel.x =  FLAMESPEED * finecosine[phi] + FLAMEROTSPEED * finecosine[an];
  actor->vel.y = FLAMESPEED * finesine[phi] + FLAMEROTSPEED * finesine[an];
  actor->yaw += ANG90/15;
}


//============================================================================
//
// A_CHolyAttack3
//
// 	Spawns the spirits
//============================================================================

void A_CHolyAttack3(DActor *actor)
{
  actor->SpawnMissile(actor->target, MT_HOLY_MISSILE);
  S_StartSound(actor, SFX_CHOLY_FIRE);
}


//============================================================================
//
// A_CHolyAttack2 
//
// 	Spawns the spirits
//============================================================================

void A_CHolyAttack2(DActor *actor)
{
  extern  consvar_t cv_deathmatch;

  int i, j;
  DActor *mo;
  DActor *tail, *next;

  for(j = 0; j < 4; j++)
    {
      mo = actor->mp->SpawnDActor(actor->pos, MT_HOLY_FX);
      if (!mo)
	continue;

      switch(j)
	{ // float bob index
	case 0:
	  mo->special2 = P_Random()&7; // upper-left
	  break;
	case 1:
	  mo->special2 = 32+(P_Random()&7); // upper-right
	  break;
	case 2:
	  mo->special2 = (32+(P_Random()&7))<<16; // lower-left
	  break;
	case 3:
	  mo->special2 = ((32+(P_Random()&7))<<16)+32+(P_Random()&7);
	  break;
	}
      mo->pos.z = actor->pos.z;
      mo->yaw = actor->yaw + (ANG45 + ANG45/2) - ANG45*j;
      mo->Thrust(mo->yaw, mo->info->speed);
      mo->owner = actor->owner;
      mo->args[0] = 10; // initial turn value
      mo->args[1] = 0; // initial look angle
      if (cv_deathmatch.value)
	{ // Ghosts last slightly less longer in DeathMatch
	  mo->health = 85;
	}
      if (linetarget)
	{
	  mo->target = linetarget;
	  mo->flags |= MF_NOCLIPLINE|MF_NOCLIPTHING;
	  mo->flags &= ~MF_MISSILE;
	  mo->eflags |= MFE_SKULLFLY;
	}
      tail = actor->mp->SpawnDActor(mo->pos, MT_HOLY_TAIL);
      tail->owner = mo; // parent
      for(i = 1; i < 3; i++)
	{
	  next = actor->mp->SpawnDActor(mo->pos, MT_HOLY_TAIL);
	  next->SetState(statenum_t(next->info->spawnstate+1));
	  tail->target = next; // tail pieces use target field to point to next piece
	  tail = next;
	}
      tail->target = NULL; // last tail bit
    }
}

//============================================================================
//
// A_CHolyAttack
//
//============================================================================

void A_CHolyAttack(PlayerPawn *p, pspdef_t *psp)
{
  int mana = p->weaponinfo[p->readyweapon].ammopershoot;
  p->ammo[am_mana1] -= mana;
  p->ammo[am_mana2] -= mana;
  p->SpawnPlayerMissile(MT_HOLY_MISSILE);

  p->player->palette = STARTHOLYPAL;
  S_StartSound(p, SFX_CHOLY_FIRE);
}

//============================================================================
//
// A_CHolyPalette
//
//============================================================================

void A_CHolyPalette(PlayerPawn *p, pspdef_t *psp)
{
  int pal = STARTHOLYPAL + psp->state-(&weaponstates[S_CHOLYATK_6]);
  if (pal == STARTHOLYPAL+3)
    pal = 0; // reset back to original playpal

  p->player->palette = pal;
}

//============================================================================
//
// CHolyFindTarget
//
//============================================================================

static void CHolyFindTarget(DActor *actor)
{
  Actor *target = actor->mp->RoughBlockSearch(actor, actor->owner, 6, 0);

  if (target)
    {
      actor->target = target;
      actor->flags |= MF_NOCLIPLINE|MF_NOCLIPTHING;
      actor->eflags |= MFE_SKULLFLY;
      actor->flags &= ~MF_MISSILE;
    }
}

//============================================================================
//
// CHolySeekerMissile
//
// 	 Similar to P_SeekerMissile, but seeks to a random Z on the target
//============================================================================

static void CHolySeekerMissile(DActor *actor, angle_t thresh, angle_t turnMax)
{
  angle_t delta;
  angle_t angle;
  fixed_t newZ;
  fixed_t deltaZ;

  Actor *target = actor->target;
  if(target == NULL)
    {
      return;
    }
  if (!(target->flags & MF_SHOOTABLE) 
      || !(target->flags & (MF_COUNTKILL|MF_NOTMONSTER)))
    { // Target died/target isn't a player or creature
      actor->target = NULL;
      actor->flags &= ~(MF_NOCLIPLINE|MF_NOCLIPTHING);
      actor->eflags &= ~MFE_SKULLFLY;
      actor->flags |= MF_MISSILE;
      CHolyFindTarget(actor);
      return;
    }
  int dir = P_FaceMobj(actor, target, &delta);
  if(delta > thresh)
    {
      delta >>= 1;
      if(delta > turnMax)
	{
	  delta = turnMax;
	}
    }
  if(dir)
    { // Turn clockwise
      actor->yaw += delta;
    }
  else
    { // Turn counter clockwise
      actor->yaw -= delta;
    }
  angle = actor->yaw>>ANGLETOFINESHIFT;
  actor->vel.x = actor->info->speed * finecosine[angle];
  actor->vel.y = actor->info->speed * finesine[angle];
  if (!(game.tic & 15) || actor->pos.z > target->Top() || actor->Top() < target->pos.z)
    {
      newZ = target->pos.z + (P_Random()*target->height >>8);
      deltaZ = newZ - actor->pos.z;
      if(abs(deltaZ) > 15)
	{
	  if(deltaZ > 0)
	    {
	      deltaZ = 15;
	    }
	  else
	    {
	      deltaZ = -15;
	    }
	}
      fixed_t fdist = P_AproxDistance(target->pos.x - actor->pos.x, target->pos.y - actor->pos.y) / actor->info->speed;
      int dist = fdist.floor();
      if (dist < 1)
	dist = 1;

      actor->vel.z = deltaZ/dist;
    }
  return;
}

//============================================================================
//
// A_CHolyWeave
//
//============================================================================

static void CHolyWeave(DActor *actor)
{
  fixed_t newX, newY;

  int weaveXY = actor->special2>>16;
  int weaveZ = actor->special2&0xFFFF;
  int angle = (actor->yaw+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->pos.x - finecosine[angle] * FloatBobOffsets[weaveXY] << 2;
  newY = actor->pos.y - finesine[angle] * FloatBobOffsets[weaveXY] << 2;
  weaveXY = (weaveXY+(P_Random()%5))&63;
  newX += finecosine[angle] * FloatBobOffsets[weaveXY] << 2;
  newY += finesine[angle] * FloatBobOffsets[weaveXY] << 2;
  actor->TryMove(newX, newY, true);
  actor->pos.z -= FloatBobOffsets[weaveZ]<<1;
  weaveZ = (weaveZ+(P_Random()%5))&63;
  actor->pos.z += FloatBobOffsets[weaveZ]<<1;	
  actor->special2 = weaveZ+(weaveXY<<16);
}

//============================================================================
//
// A_CHolySeek
//
//============================================================================

void A_CHolySeek(DActor *actor)
{
  actor->health--;
  if(actor->health <= 0)
    {
      actor->vel.x >>= 2;
      actor->vel.y >>= 2;
      actor->vel.z = 0;
      actor->SetState(actor->info->deathstate);
      actor->tics -= P_Random()&3;
      return;
    }
  if (actor->target)
    {
      CHolySeekerMissile(actor, actor->args[0]*ANGLE_1,
			 actor->args[0]*ANGLE_1*2);
      if(!((game.tic + 7) & 15))
	{
	  actor->args[0] = 5+(P_Random()/20);
	}
    }
  CHolyWeave(actor);
}

//============================================================================
//
// CHolyTailFollow
//
//============================================================================

static void CHolyTailFollow(Actor *actor, fixed_t dist)
{
  fixed_t oldDistance, newDistance;

  Actor *child = actor->target;
  if (child)
    {
      int an = R_PointToAngle2(actor->pos, child->pos) >> ANGLETOFINESHIFT;
      oldDistance = P_AproxDistance(child->pos.x - actor->pos.x, child->pos.y - actor->pos.y);
      if (child->TryMove(actor->pos.x + dist * finecosine[an], 
			 actor->pos.y + dist * finesine[an], true))
	{
	  newDistance = P_AproxDistance(child->pos.x-actor->pos.x, child->pos.y-actor->pos.y) - 1;
	  if(oldDistance < 1)
	    {
	      if (child->pos.z < actor->pos.z)
		child->pos.z = actor->pos.z-dist;
	      else
		child->pos.z = actor->pos.z+dist;
	    }
	  else
	    {
	      child->pos.z = actor->pos.z + (newDistance / oldDistance) * (child->pos.z - actor->pos.z);
	    }
	}
      CHolyTailFollow(child, dist-1);
    }
}

//============================================================================
//
// CHolyTailRemove
//
//============================================================================

static void CHolyTailRemove(Actor *actor)
{
  Actor *child = actor->target;
  if (child)
    CHolyTailRemove(child);

  actor->Remove();
}

//============================================================================
//
// A_CHolyTail
//
//============================================================================

void A_CHolyTail(DActor *actor)
{
  DActor *parent = (DActor *)actor->owner;

  if (parent)
    {
      if(parent->state >= &states[parent->info->deathstate])
	{ // Ghost removed, so remove all tail parts
	  CHolyTailRemove(actor);
	  return;
	}
      else if (actor->TryMove(parent->pos.x - 14 * finecosine[parent->yaw>>ANGLETOFINESHIFT],
			      parent->pos.y - 14 * finesine[parent->yaw>>ANGLETOFINESHIFT], true))
	{
	  actor->pos.z = parent->pos.z-5;
	}
      CHolyTailFollow(actor, 10);
    }
}
//============================================================================
//
// A_CHolyCheckScream
//
//============================================================================

void A_CHolyCheckScream(DActor *actor)
{
  A_CHolySeek(actor);
  if (P_Random() < 20)
    S_StartSound(actor, SFX_SPIRIT_ACTIVE);

  if (!actor->target)
    CHolyFindTarget(actor);
}

//============================================================================
//
// A_CHolySpawnPuff
//
//============================================================================

void A_CHolySpawnPuff(DActor *actor)
{
  actor->mp->SpawnDActor(actor->pos, MT_HOLY_MISSILE_PUFF);
}

//----------------------------------------------------------------------------
//
// PROC A_FireConePL1
//
//----------------------------------------------------------------------------

#define SHARDSPAWN_LEFT		1
#define SHARDSPAWN_RIGHT	2
#define SHARDSPAWN_UP		4
#define SHARDSPAWN_DOWN		8

void A_FireConePL1(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  fixed_t slope;
  int i;
  bool conedone = false;

  player->ammo[am_mana1] -= player->weaponinfo[player->readyweapon].ammopershoot;

  S_StartSound(player, SFX_MAGE_SHARDS_FIRE);

  damage = 90+(P_Random()&15);
  for(i = 0; i < 16; i++)
    {
      angle = player->yaw+i*(ANG45/16);
      slope = player->AimLineAttack(angle, MELEERANGE);
      if(linetarget)
	{
	  linetarget->Damage(player, player, damage | dt_cold);
	  conedone = true;
	  break;
	}
    }

  // didn't find any creatures, so fire projectiles
  if (!conedone)
    {
      DActor *mo = player->SpawnPlayerMissile(MT_SHARDFX1);
      if (mo)
	{
	  mo->special1 = SHARDSPAWN_LEFT|SHARDSPAWN_DOWN|SHARDSPAWN_UP
	    |SHARDSPAWN_RIGHT;
	  mo->special2 = 3; // Set sperm count (levels of reproductivity)
	  //mo->args[0] = 3; // Mark Initial shard as super damage
	}
    }
}

void A_ShedShard(DActor *actor)
{
  int spermcount = actor->special2;

  if (spermcount <= 0)
    return; // No sperm left

  int spawndir = actor->special1;

  actor->special2 = 0; // this shard is emptied of sperm
  spermcount--; // next generation shards have less sperm

  DActor *mo;

  // every so many calls, spawn a new missile in it's set directions
  if (spawndir & SHARDSPAWN_LEFT)
    {
      angle_t m_angle = actor->yaw+(ANG45/9);
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, m_angle);
      if (mo)
	{
	  mo->vel.x = (20 + 2*spermcount) * finecosine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.y = (20 + 2*spermcount) * finesine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.z = actor->vel.z;
	  mo->special1 = SHARDSPAWN_LEFT;
	  mo->special2 = spermcount;
	  mo->owner = actor->owner;
	  //mo->args[0] = (spermcount==3)?2:0;
	}
    }
  if (spawndir & SHARDSPAWN_RIGHT)
    {
      angle_t m_angle = actor->yaw-(ANG45/9);
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, m_angle);
      if (mo)
	{
	  mo->vel.x = (20 + 2*spermcount) * finecosine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.y = (20 + 2*spermcount) * finesine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.z = actor->vel.z;
	  mo->special1 = SHARDSPAWN_RIGHT;
	  mo->special2 = spermcount;
	  mo->owner = actor->owner;
	  //mo->args[0] = (spermcount==3)?2:0;
	}
    }
  if (spawndir & SHARDSPAWN_UP)
    {
      angle_t m_angle = actor->yaw;
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, m_angle);
      if (mo)
	{
	  mo->vel.x = (15 + 2*spermcount) * finecosine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.y = (15 + 2*spermcount) * finesine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.z = actor->vel.z;
	  mo->pos.z += 8;
	  if (spermcount & 1)			// Every other reproduction
	    mo->special1 = SHARDSPAWN_UP | SHARDSPAWN_LEFT | SHARDSPAWN_RIGHT;
	  else
	    mo->special1 = SHARDSPAWN_UP;
	  mo->special2 = spermcount;
	  mo->owner = actor->owner;
	  //mo->args[0] = (spermcount==3)?2:0;
	}
    }
  if (spawndir & SHARDSPAWN_DOWN)
    {
      angle_t m_angle = actor->yaw;
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, m_angle);
      if (mo)
	{
	  mo->vel.x = (15 + 2*spermcount) * finecosine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.y = (15 + 2*spermcount) * finesine[m_angle >> ANGLETOFINESHIFT];
	  mo->vel.z = actor->vel.z;
	  mo->pos.z -= 4;
	  if (spermcount & 1)			// Every other reproduction
	    mo->special1 = SHARDSPAWN_DOWN | SHARDSPAWN_LEFT | SHARDSPAWN_RIGHT;
	  else
	    mo->special1 = SHARDSPAWN_DOWN;
	  mo->special2 = spermcount;
	  mo->owner = actor->owner;
	  //mo->args[0] = (spermcount==3)?2:0;
	}
    }
}

//----------------------------------------------------------------------------
//
// PROC A_HideInCeiling
//
//----------------------------------------------------------------------------

/*
void A_HideInCeiling(DActor *actor)
{
	actor->pos.z = actor->ceilingz+4;
}
*/

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

/*
void A_FloatPuff(Actor *puff)
{
  puff->vel.z += 1.8f;
}
*/


