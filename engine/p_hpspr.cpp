// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by Raven Software, Corp.
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
// Revision 1.1  2002/11/16 14:17:58  hurdler
// Initial revision
//
// Revision 1.12  2002/09/20 22:41:30  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.11  2002/08/23 09:53:42  vberghol
// fixed Actor:: target/owner/tracer
//
// Revision 1.10  2002/08/17 21:21:48  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.9  2002/08/17 16:02:04  vberghol
// final compile for engine!
//
// Revision 1.8  2002/08/11 17:16:49  vberghol
// ...
//
// Revision 1.7  2002/08/08 12:01:27  vberghol
// pian engine on valmis!
//
// Revision 1.6  2002/08/02 20:14:50  vberghol
// p_enemy.cpp done!
//
// Revision 1.5  2002/07/10 19:56:59  vberghol
// g_pawn.cpp tehty
//
// Revision 1.4  2002/07/08 20:46:33  vberghol
// More files compile!
//
// Revision 1.3  2002/07/01 21:00:17  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:13  vberghol
// Version 133 Experimental!
//
// Revision 1.3  2001/05/27 13:42:47  bpereira
// no message
//
// Revision 1.2  2001/02/24 13:35:20  bpereira
// no message
//
//
//
// DESCRIPTION:
//   this file is include by P_pspr.c
//   it contain all heretic player sprite specific
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "doomdata.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h" // remove this somehow!
#include "g_map.h" // this too!

#include "command.h"
#include "info.h"
#include "d_items.h"
#include "p_enemy.h"
#include "p_maputl.h"
#include "r_main.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_random.h"

// Macros

#define LOWERSPEED FRACUNIT*6
#define RAISESPEED FRACUNIT*6
#define WEAPONBOTTOM 128*FRACUNIT
#define WEAPONTOP 32*FRACUNIT
#define MAGIC_JUNK 1234

#define FLAME_THROWER_TICS      (10*TICRATE)

extern  consvar_t  cv_deathmatch;

extern fixed_t bulletslope; // this suck so badly.

mobjtype_t PuffType;


weaponinfo_t wpnlev1info[NUMWEAPONS] =
{
  { // Staff
    am_noammo,              // ammo
    0,
    S_STAFFUP,              // upstate
    S_STAFFDOWN,            // downstate
    S_STAFFREADY,           // readystate
    S_STAFFATK1_1,          // atkstate
    S_STAFFATK1_1,          // holdatkstate
    S_NULL                  // flashstate
  },
  { // Gold wand
    am_goldwand,            // ammo
    USE_GWND_AMMO_1,
    S_GOLDWANDUP,           // upstate
    S_GOLDWANDDOWN,         // downstate
    S_GOLDWANDREADY,        // readystate
    S_GOLDWANDATK1_1,       // atkstate
    S_GOLDWANDATK1_1,       // holdatkstate
    S_NULL                  // flashstate
  },
  { // Crossbow
    am_crossbow,            // ammo
    USE_CBOW_AMMO_1,
    S_CRBOWUP,              // upstate
    S_CRBOWDOWN,            // downstate
    S_CRBOW1,               // readystate
    S_CRBOWATK1_1,          // atkstate
    S_CRBOWATK1_1,          // holdatkstate
    S_NULL                  // flashstate
  },
  { // Blaster
    am_blaster,             // ammo
    USE_BLSR_AMMO_1,
    S_BLASTERUP,            // upstate
    S_BLASTERDOWN,          // downstate
    S_BLASTERREADY,         // readystate
    S_BLASTERATK1_1,        // atkstate
    S_BLASTERATK1_3,        // holdatkstate
    S_NULL                  // flashstate
  },
  { // Skull rod
    am_skullrod,            // ammo
    USE_SKRD_AMMO_1,
    S_HORNRODUP,            // upstate
    S_HORNRODDOWN,          // downstate
    S_HORNRODREADY,         // readystae
    S_HORNRODATK1_1,        // atkstate
    S_HORNRODATK1_1,        // holdatkstate
    S_NULL                  // flashstate
  },
  { // Phoenix rod
    am_phoenixrod,          // ammo
    USE_PHRD_AMMO_1,
    S_PHOENIXUP,            // upstate
    S_PHOENIXDOWN,          // downstate
    S_PHOENIXREADY,         // readystate
    S_PHOENIXATK1_1,        // atkstate
    S_PHOENIXATK1_1,        // holdatkstate
    S_NULL                  // flashstate
  },
  { // Mace
    am_mace,                // ammo
    USE_MACE_AMMO_1,
    S_MACEUP,               // upstate
    S_MACEDOWN,             // downstate
    S_MACEREADY,            // readystate
    S_MACEATK1_1,           // atkstate
    S_MACEATK1_2,           // holdatkstate
    S_NULL                  // flashstate
  },
  { // Gauntlets
    am_noammo,              // ammo
    0,
    S_GAUNTLETUP,           // upstate
    S_GAUNTLETDOWN,         // downstate
    S_GAUNTLETREADY,        // readystate
    S_GAUNTLETATK1_1,       // atkstate
    S_GAUNTLETATK1_3,       // holdatkstate
    S_NULL                  // flashstate
  },
  { // Beak
    am_noammo,              // ammo
    0,
    S_BEAKUP,               // upstate
    S_BEAKDOWN,             // downstate
    S_BEAKREADY,            // readystate
    S_BEAKATK1_1,           // atkstate
    S_BEAKATK1_1,           // holdatkstate
    S_NULL                  // flashstate
  }
};

weaponinfo_t wpnlev2info[NUMWEAPONS] =
{
  { // Staff
    am_noammo,              // ammo
    0,
    S_STAFFUP2,             // upstate
    S_STAFFDOWN2,           // downstate
    S_STAFFREADY2_1,        // readystate
    S_STAFFATK2_1,          // atkstate
    S_STAFFATK2_1,          // holdatkstate
    S_NULL                  // flashstate
  },
  { // Gold wand
    am_goldwand,            // ammo
    USE_GWND_AMMO_2,
    S_GOLDWANDUP,           // upstate
    S_GOLDWANDDOWN,         // downstate
    S_GOLDWANDREADY,        // readystate
    S_GOLDWANDATK2_1,       // atkstate
    S_GOLDWANDATK2_1,       // holdatkstate
    S_NULL                  // flashstate
  },
  { // Crossbow
    am_crossbow,            // ammo
    USE_CBOW_AMMO_2,
    S_CRBOWUP,              // upstate
    S_CRBOWDOWN,            // downstate
    S_CRBOW1,               // readystate
    S_CRBOWATK2_1,          // atkstate
    S_CRBOWATK2_1,          // holdatkstate
    S_NULL                  // flashstate
  },
  { // Blaster
    am_blaster,             // ammo
    USE_BLSR_AMMO_2,
    S_BLASTERUP,            // upstate
    S_BLASTERDOWN,          // downstate
    S_BLASTERREADY,         // readystate
    S_BLASTERATK2_1,        // atkstate
    S_BLASTERATK2_3,        // holdatkstate
    S_NULL                  // flashstate
  },
  { // Skull rod
    am_skullrod,            // ammo
    USE_SKRD_AMMO_2,
    S_HORNRODUP,            // upstate
    S_HORNRODDOWN,          // downstate
    S_HORNRODREADY,         // readystae
    S_HORNRODATK2_1,        // atkstate
    S_HORNRODATK2_1,        // holdatkstate
    S_NULL                  // flashstate
  },
  { // Phoenix rod
    am_phoenixrod,          // ammo
    USE_PHRD_AMMO_2,
    S_PHOENIXUP,            // upstate
    S_PHOENIXDOWN,          // downstate
    S_PHOENIXREADY,         // readystate
    S_PHOENIXATK2_1,        // atkstate
    S_PHOENIXATK2_2,        // holdatkstate
    S_NULL                  // flashstate
  },
  { // Mace
    am_mace,                // ammo
    USE_MACE_AMMO_2,
    S_MACEUP,               // upstate
    S_MACEDOWN,             // downstate
    S_MACEREADY,            // readystate
    S_MACEATK2_1,           // atkstate
    S_MACEATK2_1,           // holdatkstate
    S_NULL                  // flashstate
  },
  { // Gauntlets
    am_noammo,              // ammo
    0,
    S_GAUNTLETUP2,          // upstate
    S_GAUNTLETDOWN2,        // downstate
    S_GAUNTLETREADY2_1,     // readystate
    S_GAUNTLETATK2_1,       // atkstate
    S_GAUNTLETATK2_3,       // holdatkstate
    S_NULL                  // flashstate
  },
  { // Beak
    am_noammo,              // ammo
    0,
    S_BEAKUP,               // upstate
    S_BEAKDOWN,             // downstate
    S_BEAKREADY,            // readystate
    S_BEAKATK2_1,           // atkstate
    S_BEAKATK2_1,           // holdatkstate
    S_NULL                  // flashstate
  }
};

//---------------------------------------------------------------------------
//
// was P_OpenWeapons
//
// Called at level load before things are loaded.
//
//---------------------------------------------------------------------------

//void P_OpenWeapons() { MaceSpotCount = 0; }

//---------------------------------------------------------------------------
// here was P_AddMaceSpot


//---------------------------------------------------------------------------
//
// was P_RepositionMace
//
// Chooses the next spot to place the mace.
//
//---------------------------------------------------------------------------

void Map::RepositionMace(Actor *mo)
{
  int spot;

  mo->UnsetPosition();
  spot = P_Random() % MaceSpots.size();
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

  SpawnActor(nx, ny, ONFLOORZ, MT_WMACE);
}

//---------------------------------------------------------------------------
//
// was P_ActivateBeak
//
//---------------------------------------------------------------------------

void PlayerPawn::ActivateBeak()
{
  pendingweapon = wp_nochange;
  readyweapon = wp_beak;
  psprites[ps_weapon].sy = WEAPONTOP;
  SetPsprite(ps_weapon, S_BEAKREADY);
}

//---------------------------------------------------------------------------
//
// PROC P_PostChickenWeapon
//
//---------------------------------------------------------------------------

void P_PostChickenWeapon(PlayerPawn *p, weapontype_t weapon)
{
  if (weapon == wp_beak)
    { // Should never happen
      weapon = wp_staff;
    }
  p->pendingweapon = wp_nochange;
  p->readyweapon = weapon;
  p->psprites[ps_weapon].sy = WEAPONBOTTOM;
  p->SetPsprite(ps_weapon, statenum_t(wpnlev1info[weapon].upstate));
}

//---------------------------------------------------------------------------
//
// PROC P_UpdateBeak
//
//---------------------------------------------------------------------------

void P_UpdateBeak(PlayerPawn *p, pspdef_t *psp)
{
  psp->sy = WEAPONTOP+(p->chickenPeck << (FRACBITS-1));
}

//---------------------------------------------------------------------------
//
// PROC A_BeakReady
//
//---------------------------------------------------------------------------

void A_BeakReady(PlayerPawn *p, pspdef_t *psp)
{
  if (p->player->cmd.buttons & BT_ATTACK)
    { // Chicken beak attack
      p->attackdown = true;
      p->SetState(S_CHICPLAY_ATK1);
      if(p->powers[pw_weaponlevel2])
	{
	  p->SetPsprite(ps_weapon, S_BEAKATK2_1);
	}
      else
	{
	  p->SetPsprite(ps_weapon, S_BEAKATK1_1);
	}
      P_NoiseAlert(p, p);
    }
  else
    {
      if(p->state == &states[S_CHICPLAY_ATK1])
	{ // Take out of attack state
	  p->SetState(S_CHICPLAY);
	}
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
  p->SetPsprite(ps_weapon,
	       statenum_t(wpnlev1info[p->readyweapon].readystate));
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
  //      PuffType = MT_BEAKPUFF;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
      p->angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
    }
  S_StartSound(p, sfx_chicpk1+(P_Random()%3));
  p->chickenPeck = 12;
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
  p->chickenPeck = 12;
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

  // P_inter.c:P_DamageMobj() handles target momentums
  damage = 18+(P_Random()&63);
  angle = p->angle;
  angle += P_SignedRandom()<<18;
  slope = p->AimLineAttack(angle, MELEERANGE);
  PuffType = MT_STAFFPUFF2;
  p->LineAttack(angle, MELEERANGE, slope, damage);
  if(linetarget)
    {
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
  angle_t angle;
  int damage;
    
  S_StartSound(p, sfx_gldhit);
  p->ammo[am_blaster] -= USE_BLSR_AMMO_1;
  P_BulletSlope(p);
  damage = HITDICE(4);
  angle = p->angle;
  if (p->refire)
    angle += P_SignedRandom()<<18;
  PuffType = MT_BLASTERPUFF1;
  p->LineAttack(angle, MISSILERANGE, bulletslope, damage);
  S_StartSound(p, sfx_blssht);
}

//----------------------------------------------------------------------------
//
// was P_BlasterMobjThinker
//
// Thinker for the ultra-fast blaster PL2 ripper-spawning missile.
//
//----------------------------------------------------------------------------

void Actor::BlasterMissileThink()
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
	      mp->SpawnActor(x, y, nz, MT_BLASTERSMOKE);
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
  p->ammo[am_blaster] -= cv_deathmatch.value ? USE_BLSR_AMMO_1 : USE_BLSR_AMMO_2;

  p->SpawnPlayerMissile(MT_BLASTERFX1);
  //if (mo) mo->thinker.function.acp1 = (actionf_p1)P_BlasterMobjThinker;

  S_StartSound(p, sfx_blssht);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL1
//
//----------------------------------------------------------------------------

void A_FireGoldWandPL1(PlayerPawn *p, pspdef_t *psp)
{
  angle_t angle;
  int damage;

  p->ammo[am_goldwand] -= USE_GWND_AMMO_1;
  P_BulletSlope(p);
  damage = 7+(P_Random()&7);
  angle = p->angle;
  if(p->refire)
    angle += P_SignedRandom()<<18;
  PuffType = MT_GOLDWANDPUFF1;
  p->LineAttack(angle, MISSILERANGE, bulletslope, damage);
  S_StartSound(p, sfx_gldhit);
}

//----------------------------------------------------------------------------
//
// PROC A_FireGoldWandPL2
//
//----------------------------------------------------------------------------

void A_FireGoldWandPL2(PlayerPawn *p, pspdef_t *psp)
{
  int i;
  angle_t angle;
  int damage;
  fixed_t pz;

  p->ammo[am_goldwand] -=
    cv_deathmatch.value ? USE_GWND_AMMO_1 : USE_GWND_AMMO_2;
  PuffType = MT_GOLDWANDPUFF2;
  P_BulletSlope(p);
  pz = FixedMul(mobjinfo[MT_GOLDWANDFX2].speed, bulletslope);
  //      P_SpawnMissileAngle(p, MT_GOLDWANDFX2, p->angle-(ANG45/8), pz);
  //      P_SpawnMissileAngle(p, MT_GOLDWANDFX2, p->angle+(ANG45/8), pz);
  angle = p->angle-(ANG45/8);
  for(i = 0; i < 5; i++)
    {
      damage = 1+(P_Random()&7);
      p->LineAttack(angle, MISSILERANGE, bulletslope, damage);
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
  Actor *ball;
  angle_t angle;

  if(p->ammo[am_mace] < USE_MACE_AMMO_1)
    {
      return;
    }
  p->ammo[am_mace] -= USE_MACE_AMMO_1;

  ball = p->mp->SpawnActor(p->x, p->y, p->z+28*FRACUNIT
			   - FOOTCLIPSIZE*((p->flags2&MF2_FEETARECLIPPED) != 0), MT_MACEFX2);
  ball->pz = 2*FRACUNIT+((p->aiming)<<(FRACBITS-5));
  angle = p->angle;
  ball->owner = p;
  ball->angle = angle;
  ball->z += (p->aiming)<<(FRACBITS-4);
  angle >>= ANGLETOFINESHIFT;
  ball->px = (p->px>>1)
    +FixedMul(ball->info->speed, finecosine[angle]);
  ball->py = (p->py>>1)
    +FixedMul(ball->info->speed, finesine[angle]);
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
  Actor *ball;

  if(P_Random() < 28)
    {
      A_FireMacePL1B(p, psp);
      return;
    }
  if(p->ammo[am_mace] < USE_MACE_AMMO_1)
    {
      return;
    }
  p->ammo[am_mace] -= USE_MACE_AMMO_1;
  psp->sx = ((P_Random()&3)-2)*FRACUNIT;
  psp->sy = WEAPONTOP+(P_Random()&3)*FRACUNIT;
  ball = p->SPMAngle(MT_MACEFX1, p->angle +(((P_Random()&7)-4)<<24));
  if (ball)
    {
      ball->special1 = 16; // tics till dropoff
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MacePL1Check
//
//----------------------------------------------------------------------------

void A_MacePL1Check(Actor *ball)
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

void A_MaceBallImpact(Actor *ball)
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

void A_MaceBallImpact2(Actor *ball)
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

      Actor *tiny = ball->mp->SpawnActor(ball->x, ball->y, ball->z, MT_MACEFX3);
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

      tiny = ball->mp->SpawnActor(ball->x, ball->y, ball->z, MT_MACEFX3);
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
  p->ammo[am_mace] -= cv_deathmatch.value ? USE_MACE_AMMO_1 : USE_MACE_AMMO_2;

  Actor *mo = p->SpawnPlayerMissile(MT_MACEFX4);
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

void A_DeathBallImpact(Actor *ball)
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
	      angle += ANGLE_45/2;
	    }
	}
      if(newAngle)
	{
	  ball->angle = angle;
	  angle >>= ANGLETOFINESHIFT;
	  ball->px = FixedMul(ball->info->speed, finecosine[angle]);
	  ball->py = FixedMul(ball->info->speed, finesine[angle]);
	}
      ball->SetState(ball->info->spawnstate);
      S_StartSound(ball, sfx_pstop);
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

void A_SpawnRippers(Actor *actor)
{
  int i;
  angle_t angle;
  Actor *ripper;

  for(i = 0; i < 8; i++)
    {
      ripper = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_RIPPER);
      angle = i*ANG45;
      ripper->owner = actor->owner;
      ripper->angle = angle;
      angle >>= ANGLETOFINESHIFT;
      ripper->px = FixedMul(ripper->info->speed, finecosine[angle]);
      ripper->py = FixedMul(ripper->info->speed, finesine[angle]);
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
  p->ammo[am_crossbow] -= USE_CBOW_AMMO_1;
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
  p->ammo[am_crossbow] -= cv_deathmatch.value ? USE_CBOW_AMMO_1 : USE_CBOW_AMMO_2;
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

void A_BoltSpark(Actor *bolt)
{
  Actor *spark;
    
  if(P_Random() > 50)
    {
      spark = bolt->mp->SpawnActor(bolt->x, bolt->y, bolt->z, MT_CRBOWFX4);
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
  Actor *mo;

  if(p->ammo[am_skullrod] < USE_SKRD_AMMO_1)
    {
      return;
    }
  p->ammo[am_skullrod] -= USE_SKRD_AMMO_1;
  mo = p->SpawnPlayerMissile(MT_HORNRODFX1);
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
    cv_deathmatch.value ? USE_SKRD_AMMO_1 : USE_SKRD_AMMO_2;
 
 Actor *mi = p->SpawnPlayerMissile(MT_HORNRODFX2);
  // Use mi instead of the return value from
  // P_SpawnPlayerMissile because we need to give info to the mobj
  // even if it exploded immediately.
  if (mi)
    {
      if (game.multiplayer)
        { // Multi-player game
	  //mi->special2 = p-players;
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

void A_SkullRodPL2Seek(Actor *actor)
{
  actor->SeekerMissile(ANGLE_1*10, ANGLE_1*30);
}

//----------------------------------------------------------------------------
//
// PROC A_AddPlayerRain
//
//----------------------------------------------------------------------------

void A_AddPlayerRain(Actor *actor)
{
  /*
    int playerNum = game.multiplayer ? actor->special2 : 0;
    if (!playeringame[playerNum])
    { // Player left the game
    return;
    }
    player = &players[playerNum];
    if (p->health <= 0)
    { // Player is dead
    return;
    }
  */
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
}

//----------------------------------------------------------------------------
//
// PROC A_SkullRodStorm
//
//----------------------------------------------------------------------------

void A_SkullRodStorm(Actor *actor)
{
  fixed_t x, y;
  //int playerNum;
  Actor *o = actor->owner;

  if (actor->health-- == 0)
    {
      actor->SetState(S_NULL);
      /*
      playerNum = game.multiplayer ? actor->special2 : 0;
      if(!playeringame[playerNum])
	{ // Player left the game
	  return;
	}
      p = &players[playerNum];
      */
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
      return;
    }

  if (P_Random() < 25)
    // Fudge rain frequency
    return;
    
  x = actor->x+((P_Random()&127)-64)*FRACUNIT;
  y = actor->y+((P_Random()&127)-64)*FRACUNIT;

  Actor *mo = actor->mp->SpawnActor(x, y, ONCEILINGZ, mobjtype_t(MT_RAINPLR1 + actor->special2 % 4));
  mo->target = actor->target;
  mo->owner = actor->owner;
  mo->px = 1; // Force collision detection
  mo->pz = -mo->info->speed;
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

void A_RainImpact(Actor *actor)
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

void A_HideInCeiling(Actor *actor)
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
  angle_t angle;

  p->ammo[am_phoenixrod] -= USE_PHRD_AMMO_1;
  p->SpawnPlayerMissile(MT_PHOENIXFX1);
  //p->SpawnPlayerMissile(MT_MNTRFX2);
  angle = p->angle+ANG180;
  angle >>= ANGLETOFINESHIFT;
  p->px += FixedMul(4*FRACUNIT, finecosine[angle]);
  p->py += FixedMul(4*FRACUNIT, finesine[angle]);
}

//----------------------------------------------------------------------------
//
// PROC A_PhoenixPuff
//
//----------------------------------------------------------------------------

void A_PhoenixPuff(Actor *actor)
{
  Actor *puff;
  angle_t angle;

  actor->SeekerMissile(ANGLE_1*5, ANGLE_1*10);
  puff = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
  angle = actor->angle+ANG90;
  angle >>= ANGLETOFINESHIFT;
  puff->px = FixedMul(FRACUNIT*1.3, finecosine[angle]);
  puff->py = FixedMul(FRACUNIT*1.3, finesine[angle]);
  puff->pz = 0;
  puff = actor->mp->SpawnActor(actor->x, actor->y, actor->z, MT_PHOENIXPUFF);
  angle = actor->angle-ANG90;
  angle >>= ANGLETOFINESHIFT;
  puff->px = FixedMul(FRACUNIT*1.3, finecosine[angle]);
  puff->py = FixedMul(FRACUNIT*1.3, finesine[angle]);
  puff->pz = 0;
}

//----------------------------------------------------------------------------
//
// PROC A_InitPhoenixPL2
//
//----------------------------------------------------------------------------

void A_InitPhoenixPL2(PlayerPawn *p, pspdef_t *psp)
{
  p->flamecount = FLAME_THROWER_TICS;
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

  if(--p->flamecount == 0)
    { // Out of flame
      p->SetPsprite(ps_weapon, S_PHOENIXATK2_4);
      p->refire = 0;
      return;
    }

  angle = p->angle;
  x = p->x+(P_SignedRandom()<<9);
  y = p->y+(P_SignedRandom()<<9);
  z = p->z+26*FRACUNIT+((p->aiming)<<FRACBITS)/173;
  if(p->flags2 & MF2_FEETARECLIPPED)
    {
      z -= FOOTCLIPSIZE;
    }
  slope = AIMINGTOSLOPE(p->aiming);
  Actor *mo = p->mp->SpawnActor(x, y, z, MT_PHOENIXFX2);
  mo->owner = p;
  mo->angle = angle;
  mo->px = p->px+FixedMul(mo->info->speed,
				finecosine[angle>>ANGLETOFINESHIFT]);
  mo->py = p->py+FixedMul(mo->info->speed,
				finesine[angle>>ANGLETOFINESHIFT]);
  mo->pz = FixedMul(mo->info->speed, slope);
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
  p->ammo[am_phoenixrod] -= USE_PHRD_AMMO_2;
}

//----------------------------------------------------------------------------
//
// PROC A_FlameEnd
//
//----------------------------------------------------------------------------

void A_FlameEnd(Actor *actor)
{
  actor->pz += 1.5*FRACUNIT;
}

//----------------------------------------------------------------------------
//
// PROC A_FloatPuff
//
//----------------------------------------------------------------------------

void A_FloatPuff(Actor *puff)
{
  puff->pz += 1.8*FRACUNIT;
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
  p->flags |= MF_JUSTATTACKED;
}
