// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
  mo->x = MaceSpots[spot]->x << FRACBITS;
  mo->y = MaceSpots[spot]->y << FRACBITS;
  mo->SetPosition();
  mo->z = mo->floorz;
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
  nx = MaceSpots[spot]->x << FRACBITS;
  ny = MaceSpots[spot]->y << FRACBITS;

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
  angle_t angle;
  int damage;
  int slope;

  damage = 1+(P_Random()&3);
  angle = p->angle;
  slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_BEAKPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      p->angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
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
  angle_t angle;
  int damage;
  int slope;

  damage = HITDICE(4);
  angle = p->angle;
  slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_BEAKPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      p->angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
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
  angle_t angle;
  int damage;
  int slope;

  damage = 5+(P_Random()&15);
  angle = p->angle;
  angle += P_SignedRandom()<<18;
  slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_STAFFPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      //S_StartSound(p, sfx_stfhit);
      // turn to face target
      p->angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_StaffAttackPL2
//
//----------------------------------------------------------------------------

void A_StaffAttackPL2(PlayerPawn *p, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  int slope;

  damage = 18+(P_Random()&63);
  angle = p->angle;
  angle += P_SignedRandom()<<18;
  slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_STAFFPUFF2;

  p->LineAttack(angle, MELEERANGE, slope, damage, dt_magic);
  if (linetarget)
    {
      linetarget->px += FixedMul(10*FRACUNIT, finecosine[angle]);
      linetarget->py += FixedMul(10*FRACUNIT, finesine[angle]);
      if (!(linetarget->flags & MF_NOGRAVITY))
	{
	  linetarget->pz += 5*FRACUNIT;
	}

      //S_StartSound(p, sfx_stfpow);
      // turn to face target
      p->angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
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
  angle_t angle = p->angle;
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
  int i;
  fixed_t xfrac;
  fixed_t yfrac;
  fixed_t zfrac;
  fixed_t nz;
  bool changexy;

  // Handle movement
  if(px || py ||
     (z != floorz) || pz)
    {
      xfrac = px>>3;
      yfrac = py>>3;
      zfrac = pz>>3;
      changexy = xfrac || yfrac;
      for(i = 0; i < 8; i++)
	{
	  if(changexy)
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
	  if(changexy && (P_Random() < 64))
	    {
	      nz = z - 8*FRACUNIT;
	      if (nz < floorz)
		{
		  nz = floorz;
		}
	      mp->SpawnDActor(x, y, nz, MT_BLASTERSMOKE);
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
      m->x += (m->px>>3)-(m->px>>1);
      m->y += (m->py>>3)-(m->py>>1);
      m->z += (m->pz>>3)-(m->pz>>1);
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
  angle_t angle = p->angle;
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
  
  p->SPMAngle(MT_GOLDWANDFX2, p->angle - (ANG45/8));
  p->SPMAngle(MT_GOLDWANDFX2, p->angle + (ANG45/8));

  angle_t angle = p->angle-(ANG45/8);
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

  DActor *ball = p->mp->SpawnDActor(p->x, p->y, p->z + 28*FRACUNIT - p->floorclip, MT_MACEFX2);
  ball->pz = 2*FRACUNIT+((p->aiming)<<(FRACBITS-5));
  angle_t angle = p->angle;
  ball->owner = p;
  ball->angle = angle;
  ball->z += (p->aiming)<<(FRACBITS-4);
  angle >>= ANGLETOFINESHIFT;
  ball->px = (p->px>>1)
    + int(ball->info->speed * finecosine[angle]);
  ball->py = (p->py>>1)
    + int(ball->info->speed * finesine[angle]);
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
  psp->sx = ((P_Random()&3)-2)*FRACUNIT;
  psp->sy = WEAPONTOP+(P_Random()&3)*FRACUNIT;
  DActor *ball = p->SPMAngle(MT_MACEFX1, p->angle +(((P_Random()&7)-4)<<24));
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
  angle = ball->angle>>ANGLETOFINESHIFT;
  ball->px = FixedMul(7*FRACUNIT, finecosine[angle]);
  ball->py = FixedMul(7*FRACUNIT, finesine[angle]);
  ball->pz -= ball->pz>>1;
}

//----------------------------------------------------------------------------
//
// PROC A_MaceBallImpact
//
//----------------------------------------------------------------------------
#define MAGIC_JUNK 1234

void A_MaceBallImpact(DActor *ball)
{
  if((ball->z <= ball->floorz) && (ball->HitFloor() != FLOOR_SOLID))
    { // Landed in some sort of liquid
      ball->Remove();
      return;
    }
  if((ball->health != MAGIC_JUNK) && (ball->z <= ball->floorz)
     && ball->pz)
    { // Bounce
      ball->health = MAGIC_JUNK;
      ball->pz = (ball->pz*192)>>8;
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

  if((ball->z <= ball->floorz) && (ball->HitFloor() != FLOOR_SOLID))
    { // Landed in some sort of liquid
      ball->Remove();
      return;
    }
  if((ball->z != ball->floorz) || (ball->pz < 2*FRACUNIT))
    { // Explode
      ball->px = ball->py = ball->pz = 0;
      ball->flags |= MF_NOGRAVITY;
      ball->flags2 &= ~(MF2_LOGRAV|MF2_FLOORBOUNCE);
    }
  else
    { // Bounce
      ball->pz = (ball->pz*192)>>8;
      ball->SetState(ball->info->spawnstate);

      DActor *tiny = ball->mp->SpawnDActor(ball->x, ball->y, ball->z, MT_MACEFX3);
      angle = ball->angle+ANG90;
      tiny->owner = ball->owner;
      tiny->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->px = (ball->px>>1)+FixedMul(ball->pz-FRACUNIT,
					    finecosine[angle]);
      tiny->py = (ball->py>>1)+FixedMul(ball->pz-FRACUNIT,
					    finesine[angle]);
      tiny->pz = ball->pz;
      tiny->CheckMissileSpawn();

      tiny = ball->mp->SpawnDActor(ball->x, ball->y, ball->z, MT_MACEFX3);
      angle = ball->angle-ANG90;
      tiny->owner = ball->owner;
      tiny->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      tiny->px = (ball->px>>1)+FixedMul(ball->pz-FRACUNIT,
					    finecosine[angle]);
      tiny->py = (ball->py>>1)+FixedMul(ball->pz-FRACUNIT,
					    finesine[angle]);
      tiny->pz = ball->pz;
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
      mo->px += p->px;
      mo->py += p->py;
      mo->pz = 2*FRACUNIT+((p->aiming)<<(FRACBITS-5));
      if (linetarget)
	{
	  mo->target = linetarget;
	}
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

  if ((ball->z <= ball->floorz) && (ball->HitFloor() != FLOOR_SOLID))
    { // Landed in some sort of liquid
      ball->Remove();
      return;
    }
  if ((ball->z <= ball->floorz) && ball->pz)
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
	      angle = R_PointToAngle2(ball->x, ball->y,
				      t->x, t->y);
	      newAngle = true;
	    }
	}
      else
	{ // Find new target
	  for(i = 0; i < 16; i++)
	    {
	      ball->AimLineAttack(angle, 10*64*FRACUNIT);
	      if(linetarget && ball->owner != linetarget)
		{
		  ball->target = linetarget;
		  angle = R_PointToAngle2(ball->x, ball->y,
					  linetarget->x, linetarget->y);
		  newAngle = true;
		  break;
		}
	      angle += ANG45/2;
	    }
	}
      if(newAngle)
	{
	  ball->angle = angle;
	  angle >>= ANGLETOFINESHIFT;
	  ball->px = int(ball->info->speed * finecosine[angle]);
	  ball->py = int(ball->info->speed * finesine[angle]);
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
  int i;
  angle_t angle;
  DActor *ripper;

  for(i = 0; i < 8; i++)
    {
      ripper = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_RIPPER);
      angle = i*ANG45;
      ripper->owner = actor->owner;
      ripper->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      ripper->px = int(ripper->info->speed * finecosine[angle]);
      ripper->py = int(ripper->info->speed * finesine[angle]);
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
  p->SPMAngle(MT_CRBOWFX3, p->angle-(ANG45/10));
  p->SPMAngle(MT_CRBOWFX3, p->angle+(ANG45/10));
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
  p->SPMAngle(MT_CRBOWFX2, p->angle-(ANG45/10));
  p->SPMAngle(MT_CRBOWFX2, p->angle+(ANG45/10));
  p->SPMAngle(MT_CRBOWFX3, p->angle-(ANG45/5));
  p->SPMAngle(MT_CRBOWFX3, p->angle+(ANG45/5));
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
      spark = bolt->mp->SpawnDActor(bolt->x, bolt->y, bolt->z, MT_CRBOWFX4);
      spark->x += P_SignedRandom()<<10;
      spark->y += P_SignedRandom()<<10;
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
    
  x = actor->x+((P_Random()&127)-64)*FRACUNIT;
  y = actor->y+((P_Random()&127)-64)*FRACUNIT;

  DActor *mo = actor->mp->SpawnDActor(x, y, ONCEILINGZ, mobjtype_t(MT_RAINPLR1 + actor->special2 % 4));
  mo->target = actor->target;
  mo->owner = actor->owner;
  mo->px = 1; // Force collision detection
  mo->pz = -int(mo->info->speed * FRACUNIT);
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
  if (actor->z > actor->floorz)
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
  actor->z = actor->ceilingz+4*FRACUNIT;
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

  angle_t angle = p->angle+ANG180;
  angle >>= ANGLETOFINESHIFT;
  p->px += FixedMul(4*FRACUNIT, finecosine[angle]);
  p->py += FixedMul(4*FRACUNIT, finesine[angle]);
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
  puff = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
  angle = actor->angle+ANG90;
  angle >>= ANGLETOFINESHIFT;
  puff->px = int(1.3 * finecosine[angle]);
  puff->py = int(1.3 * finesine[angle]);
  puff->pz = 0;
  puff = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
  angle = actor->angle-ANG90;
  angle >>= ANGLETOFINESHIFT;
  puff->px = int(1.3 * finecosine[angle]);
  puff->py = int(1.3 * finesine[angle]);
  puff->pz = 0;
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
  angle_t angle;
  fixed_t x, y, z;
  fixed_t slope;

  if(--p->attackphase == 0)
    { // Out of flame
      p->SetPsprite(ps_weapon, S_PHOENIXATK2_4);
      p->refire = 0;
      return;
    }

  angle = p->angle;
  x = p->x+(P_SignedRandom()<<9);
  y = p->y+(P_SignedRandom()<<9);
  z = p->z + 26*FRACUNIT + ((p->aiming)<<FRACBITS)/173 - p->floorclip;

  slope = AIMINGTOSLOPE(p->aiming);
  DActor *mo = p->mp->SpawnDActor(x, y, z, MT_PHOENIXFX2);
  mo->owner = p;
  mo->angle = angle;
  mo->px = p->px + int(mo->info->speed * finecosine[angle>>ANGLETOFINESHIFT]);
  mo->py = p->py + int(mo->info->speed * finesine[angle>>ANGLETOFINESHIFT]);
  mo->pz = int(mo->info->speed * slope);
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
  actor->pz += int(1.5*FRACUNIT);
}

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

void A_FloatPuff(DActor *puff)
{
  puff->pz += int(1.8*FRACUNIT);
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
  int slope;
  int randVal;
  fixed_t dist;

  psp->sx = ((P_Random() & 3) - 2)*FRACUNIT;
  psp->sy = WEAPONTOP + (P_Random() & 3)*FRACUNIT;
  angle = p->angle;
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
  angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
  if (angle - p->angle > ANG180)
    {
      if (angle - p->angle < -ANG90/20)
	p->angle = angle+ANG90/21;
      else
	p->angle -= ANG90/20;
    }
  else
    {
      if (angle - p->angle > ANG90/20)
	p->angle = angle-ANG90/21;
      else
	p->angle += ANG90/20;
    }
  p->eflags |= MFE_JUSTATTACKED;
}
