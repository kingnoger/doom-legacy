// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.16  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.15  2004/11/18 20:30:10  smite-meister
// tnt, plutonia
//
// Revision 1.14  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.13  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.12  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.9  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.8  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.7  2003/11/12 11:07:21  smite-meister
// Serialization done. Map progression.
//
// Revision 1.5  2003/03/15 20:07:16  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.4  2003/03/08 16:07:07  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:11:38  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:17:58  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Heretic weapon action functions

#include "doomdef.h"

#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h" // remove this somehow!
#include "g_map.h"

#include "command.h"
#include "info.h"

#include "p_enemy.h"
#include "r_sprite.h"
#include "r_defs.h"
#include "sounds.h"
#include "m_random.h"
#include "tables.h"

// Macros

#define FLAME_THROWER_TICS      (10*TICRATE)


inline angle_t R_PointToAngle2(vec_t<fixed_t>& a, vec_t<fixed_t>& b)
{
  return R_PointToAngle2(a.x, a.y, b.x, b.y);
}

extern consvar_t  cv_deathmatch;
extern mobjtype_t PuffType;
extern Actor *linetarget;

//---------------------------------------------------------------------------
//
// was P_RepositionMace
//
// Chooses the next spot to place the mace.
//
//---------------------------------------------------------------------------

void Map::RepositionMace(DActor *mo)
{
  mo->UnsetPosition();
  int spot = P_Random() % MaceSpots.size();
  mo->pos.x = MaceSpots[spot]->x;
  mo->pos.y = MaceSpots[spot]->y;
  mo->SetPosition();
  mo->pos.z = mo->floorz;
}

//---------------------------------------------------------------------------
//
// was P_CloseWeapons
//
// Called at level load after things are loaded.
//
//---------------------------------------------------------------------------

void Map::PlaceWeapons()
{
  if (MaceSpots.size() == 0)
    { // No maces placed
      return;
    }

  if(!cv_deathmatch.value && P_Random() < 64)
    { // Sometimes doesn't show up if not in deathmatch
      return;
    }
  int spot = P_Random() % MaceSpots.size();
  fixed_t nx, ny;
  nx = MaceSpots[spot]->x;
  ny = MaceSpots[spot]->y;

  SpawnDActor(nx, ny, ONFLOORZ, MT_WMACE);
}



void PlayerPawn::ActivateMorphWeapon()
{
  pendingweapon = wp_none;
  psprites[ps_weapon].sy = WEAPONTOP;

  if (game.mode == gm_hexen)
    {
      readyweapon = wp_snout;
      SetPsprite(ps_weapon, S_SNOUTREADY);
    }
  else
    {
      readyweapon = wp_beak;
      SetPsprite(ps_weapon, S_BEAKREADY);
    }
}

void PlayerPawn::PostMorphWeapon(weapontype_t weapon)
{
  pendingweapon = wp_none;
  readyweapon = weapon;
  psprites[ps_weapon].sy = WEAPONBOTTOM;
  SetPsprite(ps_weapon, weaponinfo[weapon].upstate);
}


//---------------------------------------------------------------------------
//
// PROC A_BeakReady
//
//---------------------------------------------------------------------------

void A_BeakReady(PlayerPawn *p, pspdef_t *psp)
{
  if (p->player->cmd.buttons & ticcmd_t::BT_ATTACK)
    {
      // Chicken beak attack
      p->attackdown = true;
      p->FireWeapon();
    }
  else
    {
      // get out of attack state
      int anim = p->pres->GetAnim();
      if (anim == presentation_t::Shoot || anim == presentation_t::Melee)
	p->pres->SetAnim(presentation_t::Idle);

      p->attackdown = false;
    }
}

//---------------------------------------------------------------------------
//
// PROC A_BeakRaise
//
//---------------------------------------------------------------------------

void A_BeakRaise(PlayerPawn *p, pspdef_t *psp)
{
  psp->sy = WEAPONTOP;
  p->SetPsprite(ps_weapon, wpnlev1info[p->readyweapon].readystate);
}

//****************************************************************************
//
// WEAPON ATTACKS
//
//****************************************************************************

//----------------------------------------------------------------------------
//
// PROC A_BeakAttackPL1
//
//----------------------------------------------------------------------------

void A_BeakAttackPL1(PlayerPawn *p, pspdef_t *psp)
{
  int damage = 1+(P_Random()&3);
  angle_t angle = p->yaw;
  fixed_t slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_BEAKPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      p->yaw = R_PointToAngle2(p->pos, linetarget->pos);
    }
  S_StartSound(p, sfx_chicpk1+(P_Random()%3));
  p->attackphase = 12;
  psp->tics -= P_Random()&7;
}

//----------------------------------------------------------------------------
//
// PROC A_BeakAttackPL2
//
//----------------------------------------------------------------------------

void A_BeakAttackPL2(PlayerPawn *p, pspdef_t *psp)
{
  int damage = HITDICE(4);
  angle_t angle = p->yaw;
  fixed_t slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_BEAKPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      p->yaw = R_PointToAngle2(p->pos, linetarget->pos);
    }
  S_StartSound(p, sfx_chicpk1+(P_Random()%3));
  p->attackphase = 12;
  psp->tics -= P_Random()&3;
}

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL1
//
//----------------------------------------------------------------------------

void A_StaffAttackPL1(PlayerPawn *p, pspdef_t *psp)
{
  int damage = 5+(P_Random()&15);
  angle_t angle = p->yaw;
  angle += P_SignedRandom()<<18;
  fixed_t slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_STAFFPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      //S_StartSound(p, sfx_stfhit);
      // turn to face target
      p->yaw = R_PointToAngle2(p->pos, linetarget->pos);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL2
//
//----------------------------------------------------------------------------

void A_StaffAttackPL2(PlayerPawn *p, pspdef_t *psp)
{
  int damage = 18+(P_Random()&63);
  angle_t angle = p->yaw;
  angle += P_SignedRandom()<<18;
  fixed_t slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_STAFFPUFF2;

  p->LineAttack(angle, MELEERANGE, slope, damage, dt_magic);
  if (linetarget)
    {
      linetarget->vel.x += 10 * finecosine[angle];
      linetarget->vel.y += 10 * finesine[angle];
      if (!(linetarget->flags & MF_NOGRAVITY))
	{
	  linetarget->vel.z += 5;
	}

      //S_StartSound(p, sfx_stfpow);
      // turn to face target
      p->yaw = R_PointToAngle2(p->pos, linetarget->pos);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireBlasterPL1
//
//----------------------------------------------------------------------------

void A_FireBlasterPL1(PlayerPawn *p, pspdef_t *psp)
{
  S_StartSound(p, sfx_gldhit);
  p->ammo[am_blaster] -= wpnlev1info[wp_blaster].ammopershoot;

  int damage = HITDICE(4);
  angle_t angle = p->yaw;
  if (p->refire)
    angle += P_SignedRandom()<<18;
  PuffType = MT_BLASTERPUFF1;
  p->LineAttack(angle, MISSILERANGE, P_BulletSlope(p), damage);
  S_StartSound(p, sfx_blssht);
}

//----------------------------------------------------------------------------
//
// was P_BlasterMobjThinker
//
// Thinker for the ultra-fast blaster PL2 ripper-spawning missile.
//
//----------------------------------------------------------------------------

void DActor::BlasterMissileThink()
{
  // Handle movement
  if (vel.x != 0 || vel.y != 0 || pos.z != floorz || vel.z != 0)
    {
      vec_t<fixed_t> frac = vel >> 3;

      bool changexy = frac.x != 0 || frac.y != 0;
      for (int i = 0; i < 8; i++)
	{
	  if (changexy)
	    {
	      if(!TryMove(pos.x + frac.x, pos.y + frac.y, true))
		{ // Blocked move
		  ExplodeMissile();
		  return;
		}
	    }
	  pos.z += frac.z;
	  if(pos.z <= floorz)
	    { // Hit the floor
	      pos.z = floorz;
	      HitFloor();
	      ExplodeMissile();
	      return;
	    }
	  if (Top() > ceilingz)
	    { // Hit the ceiling
	      pos.z = ceilingz-height;
	      ExplodeMissile();
	      return;
	    }
	  if (changexy && (P_Random() < 64))
	    {
	      fixed_t nz = pos.z - 8;
	      if (nz < floorz)
		{
		  nz = floorz;
		}
	      mp->SpawnDActor(pos.x, pos.y, nz, MT_BLASTERSMOKE);
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

//----------------------------------------------------------------------------
//
// PROC A_FireBlasterPL2
//
//----------------------------------------------------------------------------

void A_FireBlasterPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_blaster] -= cv_deathmatch.value ? wpnlev1info[wp_blaster].ammopershoot
    : wpnlev2info[wp_blaster].ammopershoot;

  DActor *m = p->SpawnPlayerMissile(MT_BLASTERFX1);
  if (m)
    { // Ultra-fast ripper spawning missile
      m->pos += m->vel >> 3;
      m->pos -= m->vel >> 1;
    }

  S_StartSound(p, sfx_blssht);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL1
//
//----------------------------------------------------------------------------

void A_FireGoldWandPL1(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_goldwand] -= wpnlev1info[wp_goldwand].ammopershoot;

  int damage = 7+(P_Random()&7);
  angle_t angle = p->yaw;
  if (p->refire)
    angle += P_SignedRandom()<<18;
  PuffType = MT_GOLDWANDPUFF1;
  p->LineAttack(angle, MISSILERANGE, P_BulletSlope(p), damage);
  S_StartSound(p, sfx_gldhit);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL2
//
//----------------------------------------------------------------------------

void A_FireGoldWandPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_goldwand] -= cv_deathmatch.value ? wpnlev1info[wp_goldwand].ammopershoot
    : wpnlev2info[wp_goldwand].ammopershoot;

  PuffType = MT_GOLDWANDPUFF2;

  fixed_t slope = P_BulletSlope(p);
  
  p->SPMAngle(MT_GOLDWANDFX2, p->yaw - (ANG45/8));
  p->SPMAngle(MT_GOLDWANDFX2, p->yaw + (ANG45/8));

  angle_t angle = p->yaw-(ANG45/8);
  for (int i = 0; i < 5; i++)
    {
      int damage = 1+(P_Random()&7);
      p->LineAttack(angle, MISSILERANGE, slope, damage);
      angle += ((ANG45/8)*2)/4;
    }
  S_StartSound(p, sfx_gldhit);
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1B
//
//----------------------------------------------------------------------------

void A_FireMacePL1B(PlayerPawn *p, pspdef_t *psp)
{
  if (p->ammo[am_mace] < wpnlev1info[wp_mace].ammopershoot)
    return;

  p->ammo[am_mace] -= wpnlev1info[wp_mace].ammopershoot;

  DActor *ball = p->mp->SpawnDActor(p->pos.x, p->pos.y, p->pos.z + 28 - p->floorclip, MT_MACEFX2);
  ball->vel.z = 2 + (fixed_t(int(p->pitch)) >> 5);
  angle_t angle = p->yaw;
  ball->owner = p;
  ball->yaw = angle;
  ball->pos.z += fixed_t(int(p->pitch)) >> 4;
  angle >>= ANGLETOFINESHIFT;
  ball->vel.x = (p->vel.x >> 1) + ball->info->speed * finecosine[angle];
  ball->vel.y = (p->vel.y >> 1) + ball->info->speed * finesine[angle];
  S_StartSound(ball, sfx_lobsht);
  ball->CheckMissileSpawn();
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL1
//
//----------------------------------------------------------------------------

void A_FireMacePL1(PlayerPawn *p, pspdef_t *psp)
{
  if (P_Random() < 28)
    {
      A_FireMacePL1B(p, psp);
      return;
    }
  if (p->ammo[am_mace] < wpnlev1info[wp_mace].ammopershoot)
    return;

  p->ammo[am_mace] -= wpnlev1info[wp_mace].ammopershoot;
  psp->sx = (P_Random()&3)-2;
  psp->sy = WEAPONTOP + (P_Random() & 3);
  DActor *ball = p->SPMAngle(MT_MACEFX1, p->yaw +(((P_Random()&7)-4)<<24));
  if (ball)
    ball->special1 = 16; // tics till dropoff
}

//----------------------------------------------------------------------------
//
// PROC A_MacePL1Check
//
//----------------------------------------------------------------------------

void A_MacePL1Check(DActor *ball)
{
  angle_t angle;

  if(ball->special1 == 0)
    {
      return;
    }
  ball->special1 -= 4;
  if(ball->special1 > 0)
    {
      return;
    }
  ball->special1 = 0;
  ball->flags2 |= MF2_LOGRAV;
  angle = ball->yaw>>ANGLETOFINESHIFT;
  ball->vel.x = 7 * finecosine[angle];
  ball->vel.y = 7 * finesine[angle];
  ball->vel.z -= ball->vel.z >> 1;
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact
//
//----------------------------------------------------------------------------
#define MAGIC_JUNK 1234

void A_MaceBallImpact(DActor *ball)
{
  if ((ball->pos.z <= ball->floorz) && (ball->HitFloor() != FLOOR_SOLID))
    { // Landed in some sort of liquid
      ball->Remove();
      return;
    }
  if ((ball->health != MAGIC_JUNK) && (ball->pos.z <= ball->floorz) && ball->vel.z != 0)
    { // Bounce
      ball->health = MAGIC_JUNK;
      ball->vel.z *= 0.75f;
      ball->flags2 &= ~MF2_FLOORBOUNCE;
      ball->SetState(ball->info->spawnstate);
      S_StartSound(ball, sfx_bounce);
    }
  else
    { // Explode
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~MF2_LOGRAV;
      S_StartSound(ball, sfx_lobhit);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact2
//
//----------------------------------------------------------------------------

void A_MaceBallImpact2(DActor *ball)
{
  angle_t angle;

  if((ball->pos.z <= ball->floorz) && (ball->HitFloor() != FLOOR_SOLID))
    { // Landed in some sort of liquid
      ball->Remove();
      return;
    }
  if((ball->pos.z != ball->floorz) || (ball->vel.z < 2))
    { // Explode
      ball->vel.Set(0,0,0);
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~(MF2_LOGRAV|MF2_FLOORBOUNCE);
    }
  else
    { // Bounce
      ball->vel.z *= 0.75f;
      ball->SetState(ball->info->spawnstate);

      DActor *tiny = ball->mp->SpawnDActor(ball->pos, MT_MACEFX3);
      angle = ball->yaw+ANG90;
      tiny->owner = ball->owner;
      tiny->yaw = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->vel.x = (ball->vel.x>>1) + (ball->vel.z - 1) * finecosine[angle];
      tiny->vel.y = (ball->vel.y>>1) + (ball->vel.z - 1) * finesine[angle];
      tiny->vel.z = ball->vel.z;
      tiny->CheckMissileSpawn();

      tiny = ball->mp->SpawnDActor(ball->pos, MT_MACEFX3);
      angle = ball->yaw-ANG90;
      tiny->owner = ball->owner;
      tiny->yaw = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->vel.x = (ball->vel.x>>1) + (ball->vel.z - 1) * finecosine[angle];
      tiny->vel.y = (ball->vel.y>>1) + (ball->vel.z - 1) * finesine[angle];
      tiny->vel.z = ball->vel.z;
      tiny->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireMacePL2
//
//----------------------------------------------------------------------------

void A_FireMacePL2(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_mace] -= cv_deathmatch.value ? wpnlev1info[wp_mace].ammopershoot
    : wpnlev2info[wp_mace].ammopershoot;

  DActor *mo = p->SpawnPlayerMissile(MT_MACEFX4);
  if (mo)
    {
      mo->vel.x += p->vel.x;
      mo->vel.y += p->vel.y;
      mo->vel.z = 2 + (fixed_t(int(p->pitch)) >> 5);
      if (linetarget)
	mo->target = linetarget;
    }
  S_StartSound(p, sfx_lobsht);
}

//----------------------------------------------------------------------------
//
// PROC A_DeathBallImpact
//
//----------------------------------------------------------------------------

void A_DeathBallImpact(DActor *ball)
{
  int i;
  angle_t angle = 0;
  bool newAngle;

  if ((ball->pos.z <= ball->floorz) && (ball->HitFloor() != FLOOR_SOLID))
    { // Landed in some sort of liquid
      ball->Remove();
      return;
    }
  if ((ball->pos.z <= ball->floorz) && ball->vel.z != 0)
    { // Bounce
      newAngle = false;
      Actor *t = ball->target;
      if (t)
	{
	  if(!(t->flags & MF_SHOOTABLE))
	    { // Target died
	      ball->target = NULL;
	    }
	  else
	    { // Seek
	      angle = R_PointToAngle2(ball->pos, t->pos);
	      newAngle = true;
	    }
	}
      else
	{ // Find new target
	  for(i = 0; i < 16; i++)
	    {
	      ball->AimLineAttack(angle, 10*64);
	      if(linetarget && ball->owner != linetarget)
		{
		  ball->target = linetarget;
		  angle = R_PointToAngle2(ball->pos, linetarget->pos);
		  newAngle = true;
		  break;
		}
	      angle += ANG45/2;
	    }
	}
      if(newAngle)
	{
	  ball->yaw = angle;
	  angle >>= ANGLETOFINESHIFT;
	  ball->vel.x = ball->info->speed * finecosine[angle];
	  ball->vel.y = ball->info->speed * finesine[angle];
	}
      ball->SetState(ball->info->spawnstate);
      S_StartSound(ball, sfx_platstop);
    }
  else
    { // Explode
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~MF2_LOGRAV;
      S_StartSound(ball, sfx_phohit);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SpawnRippers
//
//----------------------------------------------------------------------------

void A_SpawnRippers(DActor *actor)
{
  for (int i = 0; i < 8; i++)
    {
      DActor *ripper = actor->mp->SpawnDActor(actor->pos, MT_RIPPER);
      angle_t angle = i*ANG45;
      ripper->owner = actor->owner;
      ripper->yaw = angle;
      angle >>= ANGLETOFINESHIFT;
      ripper->vel.x = ripper->info->speed * finecosine[angle];
      ripper->vel.y = ripper->info->speed * finesine[angle];
      ripper->CheckMissileSpawn();
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL1
//
//----------------------------------------------------------------------------

void A_FireCrossbowPL1(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_crossbow] -= wpnlev1info[wp_crossbow].ammopershoot;
  p->SpawnPlayerMissile(MT_CRBOWFX1);
  p->SPMAngle(MT_CRBOWFX3, p->yaw-(ANG45/10));
  p->SPMAngle(MT_CRBOWFX3, p->yaw+(ANG45/10));
}

//----------------------------------------------------------------------------
//
// PROC A_FireCrossbowPL2
//
//----------------------------------------------------------------------------

void A_FireCrossbowPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_crossbow] -= cv_deathmatch.value ? wpnlev1info[wp_crossbow].ammopershoot
    : wpnlev2info[wp_crossbow].ammopershoot;
  p->SpawnPlayerMissile(MT_CRBOWFX2);
  p->SPMAngle(MT_CRBOWFX2, p->yaw-(ANG45/10));
  p->SPMAngle(MT_CRBOWFX2, p->yaw+(ANG45/10));
  p->SPMAngle(MT_CRBOWFX3, p->yaw-(ANG45/5));
  p->SPMAngle(MT_CRBOWFX3, p->yaw+(ANG45/5));
}

//----------------------------------------------------------------------------
//
// PROC A_BoltSpark
//
//----------------------------------------------------------------------------

void A_BoltSpark(DActor *bolt)
{
  DActor *spark;
    
  if(P_Random() > 50)
    {
      spark = bolt->mp->SpawnDActor(bolt->pos, MT_CRBOWFX4);
      spark->pos.x += P_SignedFRandom(6);
      spark->pos.y += P_SignedFRandom(6);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL1
//
//----------------------------------------------------------------------------

void A_FireSkullRodPL1(PlayerPawn *p, pspdef_t *psp)
{
  if (p->ammo[am_skullrod] < wpnlev1info[wp_skullrod].ammopershoot)
    return;

  p->ammo[am_skullrod] -= wpnlev1info[wp_skullrod].ammopershoot;
  DActor *mo = p->SpawnPlayerMissile(MT_HORNRODFX1);
  // Randomize the first frame
  if(mo && P_Random() > 128)
    {
      mo->SetState(S_HRODFX1_2);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_FireSkullRodPL2
//
// The special2 field holds the player number that shot the rain missile.
// The target field is used for the seeking routines, then as a counter
// for the sound looping.
//
//----------------------------------------------------------------------------

void A_FireSkullRodPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_skullrod] -=
    cv_deathmatch.value ? wpnlev1info[wp_skullrod].ammopershoot
    : wpnlev2info[wp_skullrod].ammopershoot;
 
  DActor *mi = p->SpawnPlayerMissile(MT_HORNRODFX2);
  // Use mi instead of the return value from
  // P_SpawnPlayerMissile because we need to give info to the mobj
  // even if it exploded immediately.
  if (mi)
    {
      if (game.multiplayer)
        { // Multi-player game
	  mi->special2 = p->player->number;
	  mi->owner = p;
        }
      else
        { // Always use red missiles in single player games
	  mi->special2 = 2;
        }
      if (linetarget)
	mi->target = linetarget;
        
      S_StartSound(mi, sfx_hrnpow);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodPL2Seek
//
//----------------------------------------------------------------------------

void A_SkullRodPL2Seek(DActor *actor)
{
  actor->SeekerMissile(ANGLE_1*10, ANGLE_1*30);
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerRain
//
//----------------------------------------------------------------------------

void A_AddPlayerRain(DActor *actor)
{
  /*
    // this code used to limit the number of active rains for each player, but
    // I removed it for convenience. Besides, each rain only lasts for 4 seconds.

  Actor *o = actor->owner;

  if (o == NULL || o->health <= 0)
    return;

  if (o->Type() != Thinker::tt_ppawn)
    return;

  PlayerPawn *p = (PlayerPawn *)o;

  if (p->rain1 && p->rain2)
    { // Terminate an active rain
      if (p->rain1->health < p->rain2->health)
	{
	  if(p->rain1->health > 16)
	    {
	      p->rain1->health = 16;
	    }
	  p->rain1 = NULL;
	}
      else
	{
	  if(p->rain2->health > 16)
	    {
	      p->rain2->health = 16;
	    }
	  p->rain2 = NULL;
	}
    }
  // Add rain mobj to list
  if (p->rain1)
    {
      p->rain2 = actor;
    }
  else
    {
      p->rain1 = actor;
    }
  */
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodStorm
//
//----------------------------------------------------------------------------

void A_SkullRodStorm(DActor *actor)
{
  fixed_t x, y;

  if (actor->health-- == 0)
    {
      actor->SetState(S_NULL);

      /*
      Actor *o = actor->owner;
      if (o == NULL)
	return;

      if (o->Type() == Thinker::tt_ppawn)
	{
	  PlayerPawn *p = (PlayerPawn *)o;
	  if (p->rain1 == actor)
	    {
	      p->rain1 = NULL;
	    }
	  else if (p->rain2 == actor)
	    {
	      p->rain2 = NULL;
	    }
	}
      */
      return;
    }

  if (P_Random() < 25)
    // Fudge rain frequency
    return;
    
  x = actor->pos.x + ((P_Random()&127)-64);
  y = actor->pos.y + ((P_Random()&127)-64);

  DActor *mo = actor->mp->SpawnDActor(x, y, ONCEILINGZ, mobjtype_t(MT_RAINPLR1 + actor->special2 % 4));
  mo->target = actor->target;
  mo->owner = actor->owner;
  mo->vel.x = 1; // Force collision detection
  mo->vel.z = -mo->info->speed;
  mo->special2 = actor->special2; // Transfer player number
  mo->CheckMissileSpawn();
  if(!(actor->special1 & 31))
    {
      S_StartSound(actor, sfx_ramrain);
    }
  actor->special1++;
}

//----------------------------------------------------------------------------
//
// PROC A_RainImpact
//
//----------------------------------------------------------------------------

void A_RainImpact(DActor *actor)
{
  if (actor->pos.z > actor->floorz)
    actor->SetState(statenum_t(S_RAINAIRXPLR1_1 + actor->special2 % 4));
  else if(P_Random() < 40)
    actor->HitFloor();
}

//----------------------------------------------------------------------------
//
// PROC A_HideInCeiling
//
//----------------------------------------------------------------------------

void A_HideInCeiling(DActor *actor)
{
  actor->pos.z = actor->ceilingz + 4;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL1
//
//----------------------------------------------------------------------------

void A_FirePhoenixPL1(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_phoenixrod] -= wpnlev1info[wp_phoenixrod].ammopershoot;
  p->SpawnPlayerMissile(MT_PHOENIXFX1);
  //p->SpawnPlayerMissile(MT_MNTRFX2);

  angle_t angle = p->yaw+ANG180;
  angle >>= ANGLETOFINESHIFT;
  p->vel.x += 4 * finecosine[angle];
  p->vel.y += 4 * finesine[angle];
}

//----------------------------------------------------------------------------
//
// PROC A_PhoenixPuff
//
//----------------------------------------------------------------------------

void A_PhoenixPuff(DActor *actor)
{
  DActor *puff;
  angle_t angle;

  actor->SeekerMissile(ANGLE_1*5, ANGLE_1*10);
  puff = actor->mp->SpawnDActor(actor->pos, MT_PHOENIXPUFF);
  angle = actor->yaw+ANG90;
  angle >>= ANGLETOFINESHIFT;
  puff->vel.x = 1.3f * finecosine[angle];
  puff->vel.y = 1.3f * finesine[angle];
  puff->vel.z = 0;
  puff = actor->mp->SpawnDActor(actor->pos, MT_PHOENIXPUFF);
  angle = actor->yaw-ANG90;
  angle >>= ANGLETOFINESHIFT;
  puff->vel.x = 1.3f * finecosine[angle];
  puff->vel.y = 1.3f * finesine[angle];
  puff->vel.z = 0;
}

//----------------------------------------------------------------------------
//
// PROC A_InitPhoenixPL2
//
//----------------------------------------------------------------------------

void A_InitPhoenixPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->attackphase = FLAME_THROWER_TICS;
}

//----------------------------------------------------------------------------
//
// PROC A_FirePhoenixPL2
//
// Flame thrower effect.
//
//----------------------------------------------------------------------------

void A_FirePhoenixPL2(PlayerPawn *p, pspdef_t *psp)
{
  if (--p->attackphase == 0)
    { // Out of flame
      p->SetPsprite(ps_weapon, S_PHOENIXATK2_4);
      p->refire = 0;
      return;
    }

  angle_t angle = p->yaw;
  fixed_t x = P_SignedFRandom(7);
  fixed_t y = P_SignedFRandom(7);
  vec_t<fixed_t> r(x, y, 26 + fixed_t(int(p->pitch))/173 - p->floorclip);
  r += p->pos;

  fixed_t slope = AIMINGTOSLOPE(p->pitch);
  DActor *mo = p->mp->SpawnDActor(r, MT_PHOENIXFX2);
  mo->owner = p;
  mo->yaw = angle;
  mo->vel.x = p->vel.x + mo->info->speed * finecosine[angle>>ANGLETOFINESHIFT];
  mo->vel.y = p->vel.y + mo->info->speed * finesine[angle>>ANGLETOFINESHIFT];
  mo->vel.z = mo->info->speed * slope;
  if(!p->refire || !(p->mp->maptic % 38))
    {
      S_StartSound(p, sfx_phopow);
    }
  mo->CheckMissileSpawn();
}

//----------------------------------------------------------------------------
//
// PROC A_ShutdownPhoenixPL2
//
//----------------------------------------------------------------------------

void A_ShutdownPhoenixPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[am_phoenixrod] -= wpnlev2info[wp_phoenixrod].ammopershoot;
}

//----------------------------------------------------------------------------
//
// PROC A_FlameEnd
//
//----------------------------------------------------------------------------

void A_FlameEnd(DActor *actor)
{
  actor->vel.z += 1.5f;
}

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

void A_FloatPuff(DActor *puff)
{
  puff->vel.z += 1.8f;
}

//---------------------------------------------------------------------------
//
// PROC A_GauntletAttack
//
//---------------------------------------------------------------------------

void A_GauntletAttack(PlayerPawn *p, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  fixed_t slope;
  int randVal;
  fixed_t dist;

  psp->sx = (P_Random() & 3) - 2;
  psp->sy = WEAPONTOP + (P_Random() & 3);
  angle = p->yaw;
  if (p->powers[pw_weaponlevel2])
    {
      damage = HITDICE(2);
      dist = 4*MELEERANGE;
      angle += P_SignedRandom()<<17;
      PuffType = MT_GAUNTLETPUFF2;
    }
  else
    {
      damage = HITDICE(2);
      dist = MELEERANGE+1;
      angle += P_SignedRandom()<<18;
      PuffType = MT_GAUNTLETPUFF1;
    }
  slope = p->AimLineAttack(angle, dist);
  p->LineAttack(angle, dist, slope, damage);

  if (!linetarget)
    {
      if (P_Random() > 64)
	{
	  p->extralight = !p->extralight;
	}
      S_StartSound(p, sfx_gntful);
      return;
    }
  randVal = P_Random();
  if (randVal < 64)
    {
      p->extralight = 0;
    }
  else if (randVal < 160)
    {
      p->extralight = 1;
    }
  else
    {
      p->extralight = 2;
    }
  if (p->powers[pw_weaponlevel2])
    {
      p->GiveBody(damage>>1);
      S_StartSound(p, sfx_gntpow);
    }
  else
    {
      S_StartSound(p, sfx_gnthit);
    }
  // turn to face target
  angle = R_PointToAngle2(p->pos, linetarget->pos);
  if (angle - p->yaw > ANG180)
    {
      if (angle - p->yaw < -ANG90/20)
	p->yaw = angle+ANG90/21;
      else
	p->yaw -= ANG90/20;
    }
  else
    {
      if (angle - p->yaw > ANG90/20)
	p->yaw = angle-ANG90/21;
      else
	p->yaw += ANG90/20;
    }
  p->eflags |= MFE_JUSTATTACKED;
}
