// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003 by DooM Legacy Team.
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
// Revision 1.1  2003/03/15 20:07:18  smite-meister
// Initial Hexen compatibility!
//
//
//
// DESCRIPTION:
//   Hexen weapon action functions
//
//-----------------------------------------------------------------------------


#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_map.h"

#include "command.h"

#include "p_maputl.h"
#include "m_random.h"

#include "p_pspr.h"
#include "p_enemy.h"
#include "r_main.h"
#include "screen.h" // palettes
#include "hu_stuff.h"

#include "s_sound.h"
#include "sounds.h"



#define LOWERSPEED FRACUNIT*6
#define RAISESPEED FRACUNIT*6
#define WEAPONBOTTOM 128*FRACUNIT
#define WEAPONTOP 32*FRACUNIT

Actor *P_RoughMonsterSearch(Actor *mo, int distance);
void A_UnHideThing(DActor *actor);
int P_FaceMobj(Actor *source, Actor *target, angle_t *delta);

extern tic_t gametic;
extern fixed_t FloatBobOffsets[64];
extern mobjtype_t PuffType;
extern Actor *PuffSpawned;
//fixed_t bulletslope;


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

  angle = R_PointToAngle2(pmo->x, pmo->y, linetarget->x, linetarget->y);
  difference = (int)angle-(int)pmo->angle;
  if(abs(difference) > MAX_ANGLE_ADJUST)
    {
      pmo->angle += difference > 0 ? MAX_ANGLE_ADJUST : -MAX_ANGLE_ADJUST;
    }
  else
    {
      pmo->angle = angle;
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
  int slope;

  damage = 3+(P_Random()&3);
  angle = player->angle;
  slope = player->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_SNOUTPUFF;
  PuffSpawned = NULL;
  player->LineAttack(angle, MELEERANGE, slope, damage);
  S_StartSound(player, SFX_PIG_ACTIVE1+(P_Random()&1));
  if(linetarget)
    {
      AdjustPlayerAngle(player);
      //		player->angle = R_PointToAngle2(player->x,
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
  int slope;
  int i;

  int damage = 60+(P_Random()&63);
  fixed_t power = 10*FRACUNIT;
  PuffType = MT_HAMMERPUFF;
  for(i = 0; i < 16; i++)
    {
      angle = player->angle+i*(ANG45/32);
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
	  player->special1 = false; // Don't throw a hammer
	  goto hammerdone;
	}
      angle = player->angle-i*(ANG45/32);
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
	  player->special1 = false; // Don't throw a hammer
	  goto hammerdone;
	}
    }
  // didn't find any targets in meleerange, so set to throw out a hammer
  PuffSpawned = NULL;
  angle = player->angle;
  slope = player->AimLineAttack(angle, HAMMER_RANGE);
  player->LineAttack(angle, HAMMER_RANGE, slope, damage);
  if(PuffSpawned)
    {
      player->special1 = false;
    }
  else
    {
      player->special1 = true;
    }
 hammerdone:
  if (player->ammo[am_mana2] < player->weaponinfo[player->readyweapon].ammopershoot)
    { // Don't spawn a hammer if the player doesn't have enough mana
      player->special1 = false;
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
  Actor *mo;

  if(!player->special1)
    {
      return;
    }
  player->ammo[am_mana2] -= player->weaponinfo[player->readyweapon].ammopershoot;
  mo = player->SpawnPlayerMissile(MT_HAMMER_MISSILE); 
  if(mo)
    {
      mo->special1 = 0;
    }	
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

  angle_t an = player->angle;
  DActor *m;
  m = player->SPMAngle(MT_FSWORD_MISSILE, an + ANG45/4);
  m->z -= 10*FRACUNIT;
  m = player->SPMAngle(MT_FSWORD_MISSILE, an+ANG45/8);
  m->z -= 5*FRACUNIT;
  player->SPMAngle(MT_FSWORD_MISSILE, an);
  m = player->SPMAngle(MT_FSWORD_MISSILE, an-ANG45/8);
  m->z += 5*FRACUNIT;
  m = player->SPMAngle(MT_FSWORD_MISSILE, an-ANG45/4);
  m->z += 10*FRACUNIT;
  S_StartSound(player, SFX_FIGHTER_SWORD_FIRE);
}

//============================================================================
//
// A_FSwordAttack2
//
//============================================================================

void A_FSwordAttack2(DActor *actor)
{
  angle_t angle = actor->angle;

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
  int i;

  for(i = 1+(P_Random()&3); i; i--)
    {
      actor->mp->SpawnDActor(actor->x+((P_Random()-128)<<12), actor->y
		  +((P_Random()-128)<<12), actor->z+((P_Random()-128)<<11),
		  MT_FSWORD_FLAME);
    }
}

//----------------------------------------------------------------------------
// Thinking function for mage wand projectile / cleric flame

void DActor::XBlasterMissileThink()
{
  int i;
  fixed_t xfrac;
  fixed_t yfrac;
  fixed_t zfrac;
  fixed_t mz;
  bool changexy;
  DActor *mo;

  // Handle movement
  if(px || py || (z != floorz) || pz)
    {
      xfrac = px>>3;
      yfrac = py>>3;
      zfrac = pz>>3;
      changexy = xfrac || yfrac;
      for(i = 0; i < 8; i++)
	{
	  if (changexy)
	    {
	      if(!TryMove(x+xfrac, y+yfrac, true))
		{ // Blocked move
		  ExplodeMissile();
		  return;
		}
	    }
	  z += zfrac;
	  if(z <= floorz)
	    { // Hit the floor
	      z = floorz;
	      HitFloor();
	      ExplodeMissile();
	      return;
	    }
	  if(z+height > ceilingz)
	    { // Hit the ceiling
	      z = ceilingz-height;
	      ExplodeMissile();
	      return;
	    }
	  if(changexy)
	    {
	      if(type == MT_MWAND_MISSILE && (P_Random() < 128))
		{
		  mz = z-8*FRACUNIT;
		  if(mz < floorz)
		    {
		      mz = floorz;
		    }
		  mp->SpawnDActor(x, y, mz, MT_MWANDSMOKE);
		}
	      else if(!--special1)
		{
		  special1 = 4;
		  mz = z-12*FRACUNIT;
		  if(mz < floorz)
		    {
		      mz = floorz;
		    }
		  mo = mp->SpawnDActor(x, y, mz, MT_CFLAMEFLOOR);
		  if(mo)
		    {
		      mo->angle = angle;
		    }
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

#define ZAGSPEED	FRACUNIT

void A_LightningClip(DActor *actor)
{
  Actor *cMo;
  Actor *target = NULL;
  int zigZag;

  if(actor->type == MT_LIGHTNING_FLOOR)
    {
      actor->z = actor->floorz;
      target = (Actor *)((Actor *)actor->special2)->special1;
    }
  else if(actor->type == MT_LIGHTNING_CEILING)
    {
      actor->z = actor->ceilingz-actor->height;
      target = (Actor *)actor->special1;
    }
  if(actor->type == MT_LIGHTNING_FLOOR)
    { // floor lightning zig-zags, and forces the ceiling lightning to mimic
      cMo = (Actor *)actor->special2;
      zigZag = P_Random();
      if((zigZag > 128 && actor->special1 < 2) || actor->special1 < -2)
	{
	  actor->Thrust(actor->angle+ANG90, ZAGSPEED);
	  if(cMo)
	    {
	      cMo->Thrust(actor->angle+ANG90, ZAGSPEED);
	    }
	  actor->special1++;
	}
      else
	{
	  actor->Thrust(actor->angle-ANG90, ZAGSPEED);
	  if(cMo)
	    {
	      cMo->Thrust(cMo->angle-ANG90, ZAGSPEED);
	    }
	  actor->special1--;
	}
    }
  if(target)
    {
      if(target->health <= 0)
	{
	  actor->ExplodeMissile();
	}
      else
	{
	  actor->angle = R_PointToAngle2(actor->x, actor->y, target->x,
					 target->y);
	  actor->px = 0;
	  actor->py = 0;
	  actor->Thrust(actor->angle, int(0.5 * actor->info->speed * FRACUNIT));
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
  if(actor->health <= 0)
    {
      actor->SetState(actor->info->deathstate);
      return;
    }
  if(actor->type == MT_LIGHTNING_FLOOR)
    {
      deltaZ = 10*FRACUNIT;
    }
  else
    {
      deltaZ = -10*FRACUNIT;
    }
  DActor *mo = actor->mp->SpawnDActor(actor->x+((P_Random()-128)*actor->radius/256), 
		   actor->y+((P_Random()-128)*actor->radius/256), 
		   actor->z+deltaZ, MT_LIGHTNING_ZAP);
  if(mo)
    {
      mo->special2 = (int)actor;
      mo->px = actor->px;
      mo->py = actor->py;
      mo->target = actor->target;
      if(actor->type == MT_LIGHTNING_FLOOR)
	{
	  mo->pz = 20*FRACUNIT;
	}
      else 
	{
	  mo->pz = -20*FRACUNIT;
	}
    }
  /*
    mo = actor->mp->SpawnDActor(actor->x+((P_Random()-128)*actor->radius/256), 
    actor->y+((P_Random()-128)*actor->radius/256), 
    actor->z+deltaZ, MT_LIGHTNING_ZAP);
    if(mo)
    {
    mo->special2 = (int)actor;
    mo->px = actor->px;
    mo->py = actor->py;
    mo->target = actor->target;
    if(actor->type == MT_LIGHTNING_FLOOR)
    {
    mo->pz = 16*FRACUNIT;
    }
    else 
    {
    mo->pz = -16*FRACUNIT;
    }
    }
  */
  if(actor->type == MT_LIGHTNING_FLOOR && P_Random() < 160)
    {
      S_StartSound(actor, SFX_MAGE_LIGHTNING_CONTINUOUS);
    }
}

//============================================================================
//
// A_MLightningAttack2
//
//============================================================================

void A_MLightningAttack2(PlayerPawn *actor)
{
  DActor *fmo, *cmo;

  fmo = actor->SpawnPlayerMissile(MT_LIGHTNING_FLOOR);
  cmo = actor->SpawnPlayerMissile(MT_LIGHTNING_CEILING);
  if(fmo)
    {
      fmo->z = ONFLOORZ;
      fmo->pz = 0;
      fmo->special1 = 0;
      fmo->special2 = (int)cmo;
      A_LightningZap(fmo);	
    }
  if(cmo)
    {
      cmo->z = ONCEILINGZ;
      cmo->pz = 0;
      cmo->special1 = 0;	// mobj that it will track
      cmo->special2 = (int)fmo;
      A_LightningZap(cmo);	
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
  DActor *mo = (DActor *)actor->special2;
  if(mo)
    {
      if(mo->state >= &states[mo->info->deathstate]
	 || mo->state == &states[S_FREETARGMOBJ])
	{
	  actor->ExplodeMissile();
	}
      else
	{
	  actor->px = mo->px;
	  actor->py = mo->py;
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
  DActor *mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_LIGHTNING_ZAP);
  if(mo)
    {
      mo->SetState(S_LIGHTNING_ZAP_X1);
      mo->pz = 40*FRACUNIT;
    }
}

//============================================================================
//
// A_LightningRemove
//
//============================================================================

void A_LightningRemove(DActor *actor)
{
  DActor *mo = (DActor *)actor->special2;
  if (mo)
    {
      mo->special2 = 0;
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
    {
      mo->target = pmo;
      mo->special1 = (int)P_RoughMonsterSearch(mo, 10);
    }
}

//============================================================================
//
// A_MStaffAttack
//
//============================================================================
#define STARTHOLYPAL    22
#define STARTSCOURGEPAL 25
void A_MStaffAttack(PlayerPawn *player, pspdef_t *psp)
{

  angle_t angle;

  int mana = player->weaponinfo[player->readyweapon].ammopershoot;
  player->ammo[am_mana1] -= mana;
  player->ammo[am_mana2] -= mana;
  angle = player->angle;
	
  MStaffSpawn(player, angle);
  MStaffSpawn(player, angle-ANGLE_1*5);
  MStaffSpawn(player, angle+ANGLE_1*5);
  S_StartSound(player, SFX_MAGE_STAFF_FIRE);
  if (player == displayplayer->pawn)
    {
      hud.damagecount = 0;
      hud.bonuscount = 0;
      vid.SetPalette(STARTSCOURGEPAL);
    }
}

//============================================================================
//
// A_MStaffPalette
//
//============================================================================

void A_MStaffPalette(PlayerPawn *player, pspdef_t *psp)
{
  int pal;

  if (player == displayplayer->pawn)
    {
      pal = STARTSCOURGEPAL+psp->state-(&weaponstates[S_MSTAFFATK_2]);
      if(pal == STARTSCOURGEPAL+3)
	{ // reset back to original playpal
	  pal = 0;
	}
      vid.SetPalette(pal);
    }
}

//============================================================================
//
// A_MStaffWeave
//
//============================================================================

void A_MStaffWeave(DActor *actor)
{
  fixed_t newX, newY;
  int weaveXY, weaveZ;
  int angle;

  weaveXY = actor->special2>>16;
  weaveZ = actor->special2&0xFFFF;
  angle = (actor->angle+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->x-FixedMul(finecosine[angle], 
			   FloatBobOffsets[weaveXY]<<2);
  newY = actor->y-FixedMul(finesine[angle],
			   FloatBobOffsets[weaveXY]<<2);
  weaveXY = (weaveXY+6)&63;
  newX += FixedMul(finecosine[angle], 
		   FloatBobOffsets[weaveXY]<<2);
  newY += FixedMul(finesine[angle], 
		   FloatBobOffsets[weaveXY]<<2);
  actor->TryMove(newX, newY, true);
  actor->z -= FloatBobOffsets[weaveZ]<<1;
  weaveZ = (weaveZ+3)&63;
  actor->z += FloatBobOffsets[weaveZ]<<1;
  if(actor->z <= actor->floorz)
    {
      actor->z = actor->floorz+FRACUNIT;
    }
  actor->special2 = weaveZ+(weaveXY<<16);
}


//============================================================================
//
// A_MStaffTrack
//
//============================================================================

void A_MStaffTrack(DActor *actor)
{
  if ((actor->special1 == 0) && (P_Random()<50))
    {
      actor->special1 = (int)P_RoughMonsterSearch(actor, 10);
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
      mo->target = actor;
      mo->special1 = (int)P_RoughMonsterSearch(mo, 10);
    }
}

//============================================================================
//
// A_MStaffAttack2 - for use by mage class boss
//
//============================================================================

void A_MStaffAttack2(DActor *actor)
{
  angle_t angle;
  angle = actor->angle;
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
  int damage;
  int slope;
  fixed_t power;
  int i;

  damage = 40+(P_Random()&15);
  power = 2*FRACUNIT;
  PuffType = MT_PUNCHPUFF;
  for(i = 0; i < 16; i++)
    {
      angle = player->angle+i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if(linetarget)
	{
	  player->special1++;
	  if(player->special1 == 3)
	    {
	      damage <<= 1;
	      power = 6*FRACUNIT;
	      PuffType = MT_HAMMERPUFF;
	    }
	  player->LineAttack(angle, 2*MELEERANGE, slope, damage);
	  if (linetarget->flags & MF_SHOOTABLE)
	    //(linetarget->flags&MF_COUNTKILL || linetarget->player)
	    {
	      linetarget->Thrust(angle, power);
	    }
	  AdjustPlayerAngle(player);
	  goto punchdone;
	}
      angle = player->angle-i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if(linetarget)
	{
	  player->special1++;
	  if(player->special1 == 3)
	    {
	      damage <<= 1;
	      power = 6*FRACUNIT;
	      PuffType = MT_HAMMERPUFF;
	    }
	  player->LineAttack(angle, 2*MELEERANGE, slope, damage);
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
  player->special1 = 0;

  angle = player->angle;
  slope = player->AimLineAttack(angle, MELEERANGE);
  player->LineAttack(angle, MELEERANGE, slope, damage);

 punchdone:
  if(player->special1 == 3)
    {
      player->special1 = 0;
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

#define AXERANGE	int(2.25*MELEERANGE)

void A_FAxeAttack(PlayerPawn *player, pspdef_t *psp)
{
  angle_t angle;
  
  fixed_t power;
  int damage;
  int slope;
  int i;
  int useMana;

  damage = 40+(P_Random()&15)+(P_Random()&7);
  power = 0;
  if(player->ammo[am_mana1] > 0)
    {
      damage <<= 1;
      power = 6*FRACUNIT;
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
      angle = player->angle+i*(ANG45/16);
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
      angle = player->angle-i*(ANG45/16);
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
  player->special1 = 0;

  angle = player->angle;
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
  int slope;
  int i;

  damage = 25+(P_Random()&15);
  PuffType = MT_HAMMERPUFF;
  for(i = 0; i < 16; i++)
    {
      angle = player->angle+i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, 2*MELEERANGE, slope, 
		       damage);
	  AdjustPlayerAngle(player);
	  //			player->angle = R_PointToAngle2(player->x,
	  //				player->y, linetarget->x, linetarget->y);
	  goto macedone;
	}
      angle = player->angle-i*(ANG45/16);
      slope = player->AimLineAttack(angle, 2*MELEERANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, 2*MELEERANGE, slope, damage);
	  AdjustPlayerAngle(player);
	  //			player->angle = R_PointToAngle2(player->x,
	  //				player->y, linetarget->x, linetarget->y);
	  goto macedone;
	}
    }
  // didn't find any creatures, so try to strike any walls
  player->special1 = 0;

  angle = player->angle;
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
#define STAFFRANGE int(1.5*MELEERANGE)
void A_CStaffCheck(PlayerPawn *player, pspdef_t *psp)
{
  int damage;
  int newLife;
  angle_t angle;
  int slope;
  int i;

  damage = 20+(P_Random()&15);
  PuffType = MT_CSTAFFPUFF;
  for(i = 0; i < 3; i++)
    {
      angle = player->angle+i*(ANG45/16);
      slope = player->AimLineAttack(angle, STAFFRANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, STAFFRANGE, slope, damage);
	  player->angle = R_PointToAngle2(player->x, player->y, 
				       linetarget->x, linetarget->y);
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
      angle = player->angle-i*(ANG45/16);
      slope = player->AimLineAttack(angle, STAFFRANGE);
      if(linetarget)
	{
	  player->LineAttack(angle, STAFFRANGE, slope, damage);
	  player->angle = R_PointToAngle2(player->x, player->y, 
				       linetarget->x, linetarget->y);
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

  DActor *mo = player->SPMAngle(MT_CSTAFF_MISSILE, player->angle-(ANG45/15));
  if(mo)
    {
      mo->special2 = 32;
    }
  mo = player->SPMAngle(MT_CSTAFF_MISSILE, player->angle+(ANG45/15));
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
  int weaveXY;
  int angle;

  weaveXY = actor->special2;
  angle = (actor->angle+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->x-FixedMul(finecosine[angle], 
			   FloatBobOffsets[weaveXY]);
  newY = actor->y-FixedMul(finesine[angle],
			   FloatBobOffsets[weaveXY]);
  weaveXY = (weaveXY+3)&63;
  newX += FixedMul(finecosine[angle], 
		   FloatBobOffsets[weaveXY]);
  newY += FixedMul(finesine[angle], 
		   FloatBobOffsets[weaveXY]);
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
  player->special1 = (P_Random()>>1)+20;
}

//============================================================================
//
// A_CStaffCheckBlink
//
//============================================================================

void A_CStaffCheckBlink(PlayerPawn *player, pspdef_t *psp)
{
  if(!--player->special1)
    {
      player->SetPsprite(ps_weapon, S_CSTAFFBLINK1);
      player->special1 = (P_Random()+50)>>2;
    }
}

//============================================================================
//
// A_CFlameAttack
//
//============================================================================

#define FLAMESPEED	int(0.45*FRACUNIT)
#define CFLAMERANGE	(12*64*FRACUNIT)

void A_CFlameAttack(PlayerPawn *player, pspdef_t *psp)
{
  Actor *mo;

  mo = player->SpawnPlayerMissile(MT_CFLAME_MISSILE);
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
  actor->px = 0;
  actor->py = 0;
  actor->pz = 0;
  S_StartSound(actor, SFX_CLERIC_FLAME_EXPLODE);
}

//============================================================================
//
// A_CFlameMissile
//
//============================================================================

void A_CFlameMissile(DActor *actor)
{
  int i;
  int an, an90;
  fixed_t dist;
  DActor *mo;
  extern Actor *BlockingMobj;

  A_UnHideThing(actor);
  S_StartSound(actor, SFX_CLERIC_FLAME_EXPLODE);
  if(BlockingMobj && BlockingMobj->flags&MF_SHOOTABLE)
    { // Hit something, so spawn the flame circle around the thing
      dist = BlockingMobj->radius+18*FRACUNIT;
      for(i = 0; i < 4; i++)
	{
	  an = (i*ANG45)>>ANGLETOFINESHIFT;
	  an90 = (i*ANG45+ANG90)>>ANGLETOFINESHIFT;
	  mo = actor->mp->SpawnDActor(BlockingMobj->x+FixedMul(dist, finecosine[an]),
			   BlockingMobj->y+FixedMul(dist, finesine[an]), 
			   BlockingMobj->z+5*FRACUNIT, MT_CIRCLEFLAME);
	  if(mo)
	    {
	      mo->angle = an<<ANGLETOFINESHIFT;
	      mo->target = actor->target;
	      mo->px = mo->special1 = FixedMul(FLAMESPEED, finecosine[an]);
	      mo->py = mo->special2 = FixedMul(FLAMESPEED, finesine[an]);
	      mo->tics -= P_Random()&3;
	    }
	  mo = actor->mp->SpawnDActor(BlockingMobj->x-FixedMul(dist, finecosine[an]),
			   BlockingMobj->y-FixedMul(dist, finesine[an]), 
			   BlockingMobj->z+5*FRACUNIT, MT_CIRCLEFLAME);
	  if(mo)
	    {
	      mo->angle = ANG180+(an<<ANGLETOFINESHIFT);
	      mo->target = actor->target;
	      mo->px = mo->special1 = FixedMul(-FLAMESPEED, 
						 finecosine[an]);
	      mo->py = mo->special2 = FixedMul(-FLAMESPEED, finesine[an]);
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
	angle = player->angle;
	if(player->refire)
	{
		angle += (P_Random()-P_Random())<<17;
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
		dist = linetarget->radius+18*FRACUNIT;
		for(i = 0; i < 4; i++)
		{
			an = (i*ANG45)>>ANGLETOFINESHIFT;
			an90 = (i*ANG45+ANG90)>>ANGLETOFINESHIFT;
			mo = actor->mp->SpawnDActor(linetarget->x+FixedMul(dist, finecosine[an]),
				linetarget->y+FixedMul(dist, finesine[an]), 
				linetarget->z+5*FRACUNIT, MT_CIRCLEFLAME);
			if(mo)
			{
				mo->angle = an<<ANGLETOFINESHIFT;
				mo->target = player;
				mo->px = mo->special1 = FixedMul(FLAMESPEED, finecosine[an]);
				mo->py = mo->special2 = FixedMul(FLAMESPEED, finesine[an]);
				mo->tics -= P_Random()&3;
			}
			mo = actor->mp->SpawnDActor(linetarget->x-FixedMul(dist, finecosine[an]),
				linetarget->y-FixedMul(dist, finesine[an]), 
				linetarget->z+5*FRACUNIT, MT_CIRCLEFLAME);
			if(mo)
			{
				mo->angle = ANG180+(an<<ANGLETOFINESHIFT);
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
//============================================================================

#define FLAMEROTSPEED	2*FRACUNIT

void A_CFlameRotate(DActor *actor)
{
  int an;

  an = (actor->angle+ANG90)>>ANGLETOFINESHIFT;
  actor->px = actor->special1+FixedMul(FLAMEROTSPEED, finecosine[an]);
  actor->py = actor->special2+FixedMul(FLAMEROTSPEED, finesine[an]);
  actor->angle += ANG90/15;
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
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_HOLY_FX);
      if(!mo)
	{
	  continue;
	}
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
      mo->z = actor->z;
      mo->angle = actor->angle+(ANGLE_45+ANGLE_45/2)-ANGLE_45*j;
      mo->Thrust(mo->angle, int(mo->info->speed * FRACUNIT));
      mo->target = actor->target;
      mo->args[0] = 10; // initial turn value
      mo->args[1] = 0; // initial look angle
      if (cv_deathmatch.value)
	{ // Ghosts last slightly less longer in DeathMatch
	  mo->health = 85;
	}
      if(linetarget)
	{
	  mo->special1 = (int)linetarget;
	  mo->flags |= MF_NOCLIPLINE|MF_NOCLIPTHING;
	  mo->flags &= ~MF_MISSILE;
	  mo->eflags |= MFE_SKULLFLY;
	}
      tail = actor->mp->SpawnDActor(mo->x, mo->y, mo->z, MT_HOLY_TAIL);
      tail->special2 = (int)mo; // parent
      for(i = 1; i < 3; i++)
	{
	  next = actor->mp->SpawnDActor(mo->x, mo->y, mo->z, MT_HOLY_TAIL);
	  next->SetState(statenum_t(next->info->spawnstate+1));
	  tail->special1 = (int)next;
	  tail = next;
	}
      tail->special1 = 0; // last tail bit
    }
}

//============================================================================
//
// A_CHolyAttack
//
//============================================================================

void A_CHolyAttack(PlayerPawn *player, pspdef_t *psp)
{
  Actor *mo;

  int mana = player->weaponinfo[player->readyweapon].ammopershoot;
  player->ammo[am_mana1] -= mana;
  player->ammo[am_mana2] -= mana;
  mo = player->SpawnPlayerMissile(MT_HOLY_MISSILE);
  if (player == displayplayer->pawn)
    {
      hud.damagecount = 0;
      hud.bonuscount = 0;
      vid.SetPalette(STARTHOLYPAL);
    }
  S_StartSound(player, SFX_CHOLY_FIRE);
}

//============================================================================
//
// A_CHolyPalette
//
//============================================================================

void A_CHolyPalette(PlayerPawn *player, pspdef_t *psp)
{
  int pal;

  if(player == displayplayer->pawn)
    {
      pal = STARTHOLYPAL+psp->state-(&weaponstates[S_CHOLYATK_6]);
      if(pal == STARTHOLYPAL+3)
	{ // reset back to original playpal
	  pal = 0;
	}
      vid.SetPalette(pal);
    }
}

//============================================================================
//
// CHolyFindTarget
//
//============================================================================

static void CHolyFindTarget(DActor *actor)
{
  Actor *target = P_RoughMonsterSearch(actor, 6);

  if (target)
    {
      actor->special1 = (int)target;
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
  int dir;
  int dist;
  angle_t delta;
  angle_t angle;
  Actor *target;
  fixed_t newZ;
  fixed_t deltaZ;

  target = (Actor *)actor->special1;
  if(target == NULL)
    {
      return;
    }
  if (!(target->flags & MF_SHOOTABLE) 
      || !(target->flags & (MF_COUNTKILL|MF_NOTMONSTER)))
    { // Target died/target isn't a player or creature
      actor->special1 = 0;
      actor->flags &= ~(MF_NOCLIPLINE|MF_NOCLIPTHING);
      actor->eflags &= ~MFE_SKULLFLY;
      actor->flags |= MF_MISSILE;
      CHolyFindTarget(actor);
      return;
    }
  dir = P_FaceMobj(actor, target, &delta);
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
      actor->angle += delta;
    }
  else
    { // Turn counter clockwise
      actor->angle -= delta;
    }
  angle = actor->angle>>ANGLETOFINESHIFT;
  actor->px = int(actor->info->speed * finecosine[angle]);
  actor->py = int(actor->info->speed * finesine[angle]);
  if (!(gametic & 15) 
     || actor->z > target->z+(target->height)
     || actor->z+actor->height < target->z)
    {
      newZ = target->z+((P_Random()*target->height)>>8);
      deltaZ = newZ-actor->z;
      if(abs(deltaZ) > 15*FRACUNIT)
	{
	  if(deltaZ > 0)
	    {
	      deltaZ = 15*FRACUNIT;
	    }
	  else
	    {
	      deltaZ = -15*FRACUNIT;
	    }
	}
      dist = P_AproxDistance(target->x-actor->x, target->y-actor->y);
      dist = dist / int(actor->info->speed * FRACUNIT);
      if(dist < 1)
	{
	  dist = 1;
	}
      actor->pz = deltaZ/dist;
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
  int weaveXY, weaveZ;
  int angle;

  weaveXY = actor->special2>>16;
  weaveZ = actor->special2&0xFFFF;
  angle = (actor->angle+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->x-FixedMul(finecosine[angle], 
			   FloatBobOffsets[weaveXY]<<2);
  newY = actor->y-FixedMul(finesine[angle],
			   FloatBobOffsets[weaveXY]<<2);
  weaveXY = (weaveXY+(P_Random()%5))&63;
  newX += FixedMul(finecosine[angle], 
		   FloatBobOffsets[weaveXY]<<2);
  newY += FixedMul(finesine[angle], 
		   FloatBobOffsets[weaveXY]<<2);
  actor->TryMove(newX, newY, true);
  actor->z -= FloatBobOffsets[weaveZ]<<1;
  weaveZ = (weaveZ+(P_Random()%5))&63;
  actor->z += FloatBobOffsets[weaveZ]<<1;	
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
      actor->px >>= 2;
      actor->py >>= 2;
      actor->pz = 0;
      actor->SetState(actor->info->deathstate);
      actor->tics -= P_Random()&3;
      return;
    }
  if(actor->special1)
    {
      CHolySeekerMissile(actor, actor->args[0]*ANGLE_1,
			 actor->args[0]*ANGLE_1*2);
      if(!((gametic + 7) & 15))
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
  Actor *child;
  int an;
  fixed_t oldDistance, newDistance;

  child = (Actor *)actor->special1;
  if(child)
    {
      an = R_PointToAngle2(actor->x, actor->y, child->x, 
			   child->y)>>ANGLETOFINESHIFT;
      oldDistance = P_AproxDistance(child->x-actor->x, child->y-actor->y);
      if(child->TryMove(actor->x+FixedMul(dist, finecosine[an]), 
			actor->y+FixedMul(dist, finesine[an]), true))
	{
	  newDistance = P_AproxDistance(child->x-actor->x, 
					child->y-actor->y)-FRACUNIT;
	  if(oldDistance < FRACUNIT)
	    {
	      if(child->z < actor->z)
		{
		  child->z = actor->z-dist;
		}
	      else
		{
		  child->z = actor->z+dist;
		}
	    }
	  else
	    {
	      child->z = actor->z+FixedMul(FixedDiv(newDistance, 
						    oldDistance), child->z-actor->z);
	    }
	}
      CHolyTailFollow(child, dist-FRACUNIT);
    }
}

//============================================================================
//
// CHolyTailRemove
//
//============================================================================

static void CHolyTailRemove(DActor *actor)
{
  DActor *child = (DActor *)actor->special1;
  if(child)
    {
      CHolyTailRemove(child);
    }
  actor->Remove();
}

//============================================================================
//
// A_CHolyTail
//
//============================================================================

void A_CHolyTail(DActor *actor)
{
  DActor *parent = (DActor *)actor->special2;

  if(parent)
    {
      if(parent->state >= &states[parent->info->deathstate])
	{ // Ghost removed, so remove all tail parts
	  CHolyTailRemove(actor);
	  return;
	}
      else if(actor->TryMove(parent->x-FixedMul(14*FRACUNIT,
						  finecosine[parent->angle>>ANGLETOFINESHIFT]),
			parent->y-FixedMul(14*FRACUNIT, 
					   finesine[parent->angle>>ANGLETOFINESHIFT]), true))
	{
	  actor->z = parent->z-5*FRACUNIT;
	}
      CHolyTailFollow(actor, 10*FRACUNIT);
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
  if(P_Random() < 20)
    {
      S_StartSound(actor, SFX_SPIRIT_ACTIVE);
    }
  if(!actor->special1)
    {
      CHolyFindTarget(actor);
    }
}

//============================================================================
//
// A_CHolySpawnPuff
//
//============================================================================

void A_CHolySpawnPuff(DActor *actor)
{
  actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_HOLY_MISSILE_PUFF);
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
  int slope;
  int i;
  bool conedone = false;

  player->ammo[am_mana1] -= player->weaponinfo[player->readyweapon].ammopershoot;

  S_StartSound(player, SFX_MAGE_SHARDS_FIRE);

  damage = 90+(P_Random()&15);
  for(i = 0; i < 16; i++)
    {
      angle = player->angle+i*(ANG45/16);
      slope = player->AimLineAttack(angle, MELEERANGE);
      if(linetarget)
	{
	  player->flags2 |= MF2_ICEDAMAGE;
	  linetarget->Damage(player, player, damage);
	  player->flags2 &= ~MF2_ICEDAMAGE;
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
	  mo->target = player;
	  mo->args[0] = 3;		// Mark Initial shard as super damage
	}
    }
}

void A_ShedShard(DActor *actor)
{
  Actor *mo;
  int spawndir = actor->special1;
  int spermcount = actor->special2;

  if (spermcount <= 0) return;				// No sperm left
  actor->special2 = 0;
  spermcount--;

  // every so many calls, spawn a new missile in it's set directions
  if (spawndir & SHARDSPAWN_LEFT)
    {
      // FIXME mo = actor->P_SpawnMissileAngleSpeed(MT_SHARDFX1, actor->angle+(ANG45/9), 0, (20+2*spermcount)<<FRACBITS);
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, actor->angle+(ANG45/9));
      if (mo)
	{
	  mo->special1 = SHARDSPAWN_LEFT;
	  mo->special2 = spermcount;
	  mo->pz = actor->pz;
	  mo->target = actor->target;
	  mo->args[0] = (spermcount==3)?2:0;
	}
    }
  if (spawndir & SHARDSPAWN_RIGHT)
    {
      //mo = P_SpawnMissileAngleSpeed(actor, MT_SHARDFX1, actor->angle-(ANG45/9), 0, (20+2*spermcount)<<FRACBITS);
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, actor->angle-(ANG45/9));
      if (mo)
	{
	  mo->special1 = SHARDSPAWN_RIGHT;
	  mo->special2 = spermcount;
	  mo->pz = actor->pz;
	  mo->target = actor->target;
	  mo->args[0] = (spermcount==3)?2:0;
	}
    }
  if (spawndir & SHARDSPAWN_UP)
    {
      //mo=P_SpawnMissileAngleSpeed(actor, MT_SHARDFX1, actor->angle, 0, (15+2*spermcount)<<FRACBITS);
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, actor->angle);
      if (mo)
	{
	  mo->pz = actor->pz;
	  mo->z += 8*FRACUNIT;
	  if (spermcount & 1)			// Every other reproduction
	    mo->special1 = SHARDSPAWN_UP | SHARDSPAWN_LEFT | SHARDSPAWN_RIGHT;
	  else
	    mo->special1 = SHARDSPAWN_UP;
	  mo->special2 = spermcount;
	  mo->target = actor->target;
	  mo->args[0] = (spermcount==3)?2:0;
	}
    }
  if (spawndir & SHARDSPAWN_DOWN)
    {
      //mo=P_SpawnMissileAngleSpeed(actor, MT_SHARDFX1, actor->angle, 0, (15+2*spermcount)<<FRACBITS);
      mo = actor->SpawnMissileAngle(MT_SHARDFX1, actor->angle);
      if (mo)
	{
	  mo->pz = actor->pz;
	  mo->z -= 4*FRACUNIT;
	  if (spermcount & 1)			// Every other reproduction
	    mo->special1 = SHARDSPAWN_DOWN | SHARDSPAWN_LEFT | SHARDSPAWN_RIGHT;
	  else
	    mo->special1 = SHARDSPAWN_DOWN;
	  mo->special2 = spermcount;
	  mo->target = actor->target;
	  mo->args[0] = (spermcount==3)?2:0;
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
	actor->z = actor->ceilingz+4*FRACUNIT;
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
  puff->pz += 1.8*FRACUNIT;
}
*/


