// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.7  2003/03/15 20:07:16  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.6  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.5  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:11:55  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:02  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Weapon sprite animation, weapon objects.
//      Action functions for weapons.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "d_event.h"
#include "d_netcmd.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_input.h"

#include "g_damage.h"
#include "p_pspr.h"
#include "p_enemy.h"
#include "p_maputl.h"

#include "s_sound.h"
#include "sounds.h"
#include "r_main.h"
#include "m_random.h"

#include "hardware/hw3sound.h"

#define LOWERSPEED              FRACUNIT*6
#define RAISESPEED              FRACUNIT*6

#define WEAPONBOTTOM            128*FRACUNIT
#define WEAPONTOP               32*FRACUNIT

// FIXME TESTING: play any creature!
// changes: S_PLAY => info->spawnstate
// S_PLAY_ATK1 => info->missilestate
// S_PLAY_ATK2 => info->missilestate+1

// was VerifFavoritWeapon
// added by Boris : preferred weapons order
void PlayerPawn::UseFavoriteWeapon()
{
  int i;

  if (pendingweapon != wp_nochange)
    return;

  int priority = -1;

  for (i=0; i<NUMWEAPONS; i++)
    {
      // skip super shotgun for non-Doom2
      if (game.mode != gm_doom2 && i == wp_supershotgun)
	continue;
      // skip plasma-bfg in sharware
      if (game.mode == gm_doom1s && (i==wp_plasma || i==wp_bfg))
	continue;

      if (weaponowned[i] && priority < player->favoriteweapon[i] &&
	  ammo[weaponinfo[i].ammo] >= weaponinfo[i].ammopershoot )
        {
	  pendingweapon = weapontype_t(i);
	  priority = player->favoriteweapon[i];
        }
    }

  if (pendingweapon == readyweapon)
    pendingweapon = wp_nochange;
}

//
// was P_SetupPsprites
// Called at start of level for each player.
//
void PlayerPawn::SetupPsprites()
{
  int i;

  // remove all psprites
  for (i=0 ; i<NUMPSPRITES ; i++)
    psprites[i].state = NULL;

  // spawn the gun
  pendingweapon = readyweapon;
  BringUpWeapon();
}


//
// was P_SetPsprite
//
void PlayerPawn::SetPsprite(int position, weaponstatenum_t stnum, bool call)
{
  weaponstate_t  *state;
  pspdef_t *psp = &psprites[position];

  do
    {
      if (!stnum)
        {
	  // object removed itself
	  psp->state = NULL;
	  break;
        }
#ifdef PARANOIA
      if(stnum>=NUMWEAPONSTATES)
	I_Error("P_SetPsprite : state %d unknown\n",stnum);
#endif
      state = &weaponstates[stnum];
      psp->state = state;
      psp->tics = state->tics;        // could be 0
      if(state->misc1)
	{ // Set coordinates.
	  psp->sx = state->misc1<<FRACBITS;
	}
      if(state->misc2)
	{
	  psp->sy = state->misc2<<FRACBITS;
	}
      if (state->action && call)
	{
	  // Call action routine.
	  state->action(this, psp);
	  if(!psp->state)
	    break;
	}
      stnum = psp->state->nextstate;
    } while(!psp->tics); // An initial state of 0 could cycle through.
}


//
// was P_MovePsprites
// Called every tic by player thinking routine.
//
void PlayerPawn::MovePsprites()
{
  int       i;
  pspdef_t *psp = &psprites[0];
  //  state_t  *st;

  for (i=0 ; i<NUMPSPRITES ; i++, psp++)
    {
      // a null state means not active
      if (psp->state)
        {
	  // drop tic count and possibly change state

	  // a -1 tic count never changes
	  if (psp->tics != -1)
            {
	      psp->tics--;
	      if (!psp->tics)
		SetPsprite(i, psp->state->nextstate);
            }
        }
    }

  psprites[ps_flash].sx = psprites[ps_weapon].sx;
  psprites[ps_flash].sy = psprites[ps_weapon].sy;
}


//
// P_CalcSwing
//
/* BP: UNUSED

   fixed_t         swingx;
   fixed_t         swingy;

   void P_CalcSwing (PlayerPawn *p)
   {
   fixed_t     swing;
   int         angle;

   // OPTIMIZE: tablify this.
   // A LUT would allow for different modes,
   //  and add flexibility.

   swing = player->bob;

   angle = (FINEANGLES/70*cmap.leveltic)&FINEMASK;
   swingx = FixedMul ( swing, finesine[angle]);

   angle = (FINEANGLES/70*cmap.leveltic+FINEANGLES/2)&FINEMASK;
   swingy = -FixedMul ( swingx, finesine[angle]);
   }
*/


//
// was P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void PlayerPawn::BringUpWeapon()
{
  if (pendingweapon == wp_nochange)
    pendingweapon = readyweapon;

#ifdef PARANOIA
  if (pendingweapon >= NUMWEAPONS )
    {
      I_Error("P_BringUpWeapon : pendingweapon %d\n",pendingweapon);
    }
#endif

  weaponstatenum_t newstate = weaponinfo[pendingweapon].upstate;

  switch (pendingweapon)
    {
    case wp_chainsaw:
      S_StartAttackSound(this, sfx_sawup);
      break;
    case wp_gauntlets:
      S_StartAttackSound(this, sfx_gntact);
      break;
    case wp_timons_axe:
      if (ammo[am_mana1])
	newstate = S_FAXEUP_G;
      break;
    default:
      break;
    }
    
  pendingweapon = wp_nochange;
  psprites[ps_weapon].sy = WEAPONBOTTOM;

  SetPsprite(ps_weapon, newstate);
}

//
// was P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
bool PlayerPawn::CheckAmmo()
{
  ammotype_t at = weaponinfo[readyweapon].ammo;

  // Minimal amount for one shot varies.
  int i, count = weaponinfo[readyweapon].ammopershoot;

  // Some do not need ammunition anyway.
  // Return if current ammunition sufficient.
  if (at == am_noammo || ammo[at] >= count)
    return true;

  if (at == am_manaboth)
    if (ammo[am_mana1] >= count && ammo[am_mana2] >= count)
      return true;

  // Out of ammo, pick a weapon to change to.
  // Preferences are set here.
  // added by Boris : preferred weapons order
  if (!player->originalweaponswitch)
    UseFavoriteWeapon();
  else
    if (game.mode == gm_hexen)
      for (i = wp_arc_of_death; i >= wp_hexen; i--)
	{
	  at = weaponinfo[i].ammo;
	  count = weaponinfo[i].ammopershoot;
	  if (weaponowned[i] && ammo[at] >= count)
	    {
	      pendingweapon = weapontype_t(i);
	      break;
	    }
	}
    else if (game.mode == gm_heretic)
      do
        {
	  if(weaponowned[wp_skullrod]
	     && ammo[am_skullrod] > weaponinfo[wp_skullrod].ammopershoot)
            {
	      pendingweapon = wp_skullrod;
            }
	  else if(weaponowned[wp_blaster]
		  && ammo[am_blaster] > weaponinfo[wp_blaster].ammopershoot)
            {
	      pendingweapon = wp_blaster;
            }
	  else if(weaponowned[wp_crossbow]
		  && ammo[am_crossbow] > weaponinfo[wp_crossbow].ammopershoot)
            {
	      pendingweapon = wp_crossbow;
            }
	  else if(weaponowned[wp_mace]
		  && ammo[am_mace] > weaponinfo[wp_mace].ammopershoot)
            {
	      pendingweapon = wp_mace;
            }
	  else if(ammo[am_goldwand] > weaponinfo[wp_goldwand].ammopershoot)
            {
	      pendingweapon = wp_goldwand;
            }
	  else if(weaponowned[wp_gauntlets])
            {
	      pendingweapon = wp_gauntlets;
            }
	  else if(weaponowned[wp_phoenixrod]
		  && ammo[am_phoenixrod] > weaponinfo[wp_phoenixrod].ammopershoot)
            {
	      pendingweapon = wp_phoenixrod;
            }
	  else
            {
	      pendingweapon = wp_staff;
            }
        } while(pendingweapon == wp_nochange);
    else
      do
        {
	  if (weaponowned[wp_plasma]
	      && ammo[am_cell]>=weaponinfo[wp_plasma].ammopershoot
	      && (game.mode != gm_doom1s) )
            {
	      pendingweapon = wp_plasma;
            }
	  else if (weaponowned[wp_supershotgun]
		   && ammo[am_shell]>=weaponinfo[wp_supershotgun].ammopershoot
		   && (game.mode == gm_doom2) )
            {
	      pendingweapon = wp_supershotgun;
            }
	  else if (weaponowned[wp_chaingun]
		   && ammo[am_clip]>=weaponinfo[wp_chaingun].ammopershoot)
            {
	      pendingweapon = wp_chaingun;
            }
	  else if (weaponowned[wp_shotgun]
		   && ammo[am_shell]>=weaponinfo[wp_shotgun].ammopershoot)
            {
	      pendingweapon = wp_shotgun;
            }
	  else if (ammo[am_clip]>=weaponinfo[wp_pistol].ammopershoot)
            {
	      pendingweapon = wp_pistol;
            }
	  else if (weaponowned[wp_chainsaw])
            {
	      pendingweapon = wp_chainsaw;
            }
	  else if (weaponowned[wp_missile]
		   && ammo[am_misl]>=weaponinfo[wp_missile].ammopershoot)
            {
	      pendingweapon = wp_missile;
            }
	  else if (weaponowned[wp_bfg]
		   && ammo[am_cell]>=weaponinfo[wp_bfg].ammopershoot
		   && (game.mode != gm_doom1s) )
            {
	      pendingweapon = wp_bfg;
            }
	  else
            {
	      // If everything fails.
	      pendingweapon = wp_fist;
            }
            
        } while (pendingweapon == wp_nochange);

  // Now set appropriate weapon overlay.
  SetPsprite(ps_weapon, weaponinfo[readyweapon].downstate);

  return false;
}


//
// was P_FireWeapon.
//
void PlayerPawn::FireWeapon()
{
  weaponstatenum_t  newstate;

  if (!CheckAmmo())
    return;

  // FIXME go to shooting state
  //SetState(weaponstatenum_t(info->missilestate + 1));
  //  P_SetMobjState(player->mo, PStateAttack[player->class]); // S_PLAY_ATK1);
  //SetState(info->missilestate); // refire? +1 or not?

  if (readyweapon == wp_timons_axe && ammo[am_mana1] > 0)
    newstate = S_FAXEATK_G1;
  else
    {
      newstate = refire ? weaponinfo[readyweapon].holdatkstate
	: weaponinfo[readyweapon].atkstate;

      // Play the sound for the initial gauntlet attack
      if (readyweapon == wp_gauntlets && !refire)
	S_StartSound(this, sfx_gntuse);
    }

  SetPsprite(ps_weapon, newstate);
  P_NoiseAlert(this, this);
}



//
// was P_DropWeapon
// Player died, so put the weapon away.
//
void PlayerPawn::DropWeapon()
{
  SetPsprite(ps_weapon, weaponinfo[readyweapon].downstate);
}


//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady(PlayerPawn *p, pspdef_t *psp)
{
  if (p->morphTics)
    { // Change to the chicken beak
      p->ActivateMorphWeapon();
      return;
    }

  // get out of attack state
  /*
  if (p->state == &states[p->info->missilestate]
      || p->state == &states[p->info->missilestate + 1] )
    {
      p->SetState(p->info->spawnstate);
    }
  */

  if (p->readyweapon == wp_chainsaw
      && psp->state == &weaponstates[S_SAW])
    {
      S_StartAttackSound(p, sfx_sawidl);
    }
  // Check for staff PL2 active sound
  if((p->readyweapon == wp_staff)
     && (psp->state == &weaponstates[S_STAFFREADY2_1])
     && P_Random() < 128)
    {
      S_StartAttackSound(p, sfx_stfcrk);
    }

  // check for change
  //  if player is dead, put the weapon away
  if (p->pendingweapon != wp_nochange || !p->health)
    {
      // change weapon
      //  (pending weapon should allready be validated)
      p->SetPsprite(ps_weapon, p->weaponinfo[p->readyweapon].downstate);
      return;
    }

  // check for fire
  //  the missile launcher and bfg do not auto fire. why not?
  if (p->player->cmd.buttons & BT_ATTACK)
    {
      p->attackdown = true;
      p->FireWeapon();
      return;
    }
  else
    p->attackdown = false;

#ifndef CLIENTPREDICTION2    
  {
    int         angle;
    // bob the weapon based on movement speed
    angle = (128*p->mp->maptic / NEWTICRATERATIO) & FINEMASK;
    psp->sx = FRACUNIT + FixedMul (p->player->bob, finecosine[angle]);
    angle &= FINEANGLES/2-1;
    psp->sy = WEAPONTOP + FixedMul (p->player->bob, finesine[angle]);
  }
#endif
}

// client prediction stuff
void A_TicWeapon(PlayerPawn *p, pspdef_t *psp)
{
  if (psp->state->action == A_WeaponReady &&
      psp->tics == psp->state->tics)
    {
      int         ang;
        
#ifdef CLIENTPREDICTION2
      extern  tic_t           localgametic;
#else
#define localgametic  p->mp->maptic
#endif

      // bob the weapon based on movement speed
      ang = (128*localgametic/NEWTICRATERATIO)&FINEMASK;
      psp->sx = FRACUNIT + FixedMul(p->player->bob, finecosine[ang]);
      ang &= FINEANGLES/2-1;
      psp->sy = WEAPONTOP + FixedMul(p->player->bob, finesine[ang]);
    }
}


//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire(PlayerPawn *p, pspdef_t *psp)
{

  // check for fire
  //  (if a weaponchange is pending, let it go through instead)
  if ((p->player->cmd.buttons & BT_ATTACK)
       && p->pendingweapon == wp_nochange
       && p->health)
    {
      p->refire++;
      p->FireWeapon();
    }
  else
    {
      p->refire = 0;
      p->CheckAmmo();
    }
}


void A_CheckReload(PlayerPawn *p, pspdef_t *psp)
{
  p->CheckAmmo();
#if 0
  if (p->ammo[am_shell] < 2)
    p->SetPsprite(ps_weapon, S_DSNR1);
#endif
}



//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void A_Lower(PlayerPawn *p, pspdef_t *psp)
{
  if(p->morphTics)
    psp->sy = WEAPONBOTTOM;
  else
    psp->sy += LOWERSPEED;

  // Is already down.
  if (psp->sy < WEAPONBOTTOM )
    return;

  // Player is dead.
  if (p->player->playerstate == PST_DEAD)
    {
      psp->sy = WEAPONBOTTOM;
      // don't bring weapon back up
      return;
    }

  // The old weapon has been lowered off the screen,
  // so change the weapon and start raising it
  if (!p->health)
    {
      // Player is dead, so keep the weapon off screen.
      p->SetPsprite(ps_weapon, S_WNULL);
      return;
    }

  p->readyweapon = p->pendingweapon;

  p->BringUpWeapon();
}


//
// A_Raise
//
void A_Raise(PlayerPawn *p, pspdef_t *psp)
{
  psp->sy -= RAISESPEED;

  if (psp->sy > WEAPONTOP )
    return;

  psp->sy = WEAPONTOP;

  // The weapon has been raised all the way,
  //  so change to the ready state.
  if (p->readyweapon == wp_timons_axe && p->ammo[am_mana1])
    p->SetPsprite(ps_weapon, S_FAXEREADY_G);
  else
    p->SetPsprite(ps_weapon, p->weaponinfo[p->readyweapon].readystate);
}



//
// A_GunFlash
//
void A_GunFlash(PlayerPawn *p, pspdef_t *psp)
{
  //p->SetState(weaponstatenum_t(p->info->missilestate + 1));
  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);
}



//
// WEAPON ATTACKS
//


//
// A_Punch
//
void A_Punch(PlayerPawn *p, pspdef_t *psp)
{
  angle_t     angle;
  int         damage;
  int         slope;

  damage = (P_Random ()%10+1)<<1;

  if (p->powers[pw_strength])
    damage *= 10;

  angle = p->angle;
  angle += (P_Random()<<18); // WARNING: don't put this in one line 
  angle -= (P_Random()<<18); // else this expretion is ambiguous (evaluation order not diffined)

  slope = p->AimLineAttack(angle, MELEERANGE);
  p->LineAttack(angle, MELEERANGE, slope, damage);

  // turn to face target
  if (linetarget)
    {
      S_StartAttackSound(p, sfx_punch);
      p->angle = R_PointToAngle2(p->x, p->y, linetarget->x, linetarget->y);
    }
  else if (p->pinfo->nproj != MT_NONE)
    {
      // this way e.g. revenants can both hit and shoot...
      p->SpawnPlayerMissile(p->pinfo->nproj);
    }
}


//
// A_Saw
//
void A_Saw(PlayerPawn *p, pspdef_t *psp)
{
  angle_t     angle;
  int         damage;
  int         slope;

  damage = 2*(P_Random ()%10+1);
  angle = p->angle;
  angle += (P_Random()<<18); // WARNING: don't put this in one line 
  angle -= (P_Random()<<18); // else this expretion is ambiguous (evaluation order not diffined)

  // use meleerange + 1 se the puff doesn't skip the flash
  slope = p->AimLineAttack(angle, MELEERANGE+1);
  p->LineAttack(angle, MELEERANGE+1, slope, damage, dt_cutting); // no recoil!

  if (!linetarget)
    {
      S_StartAttackSound(p, sfx_sawful);
      return;
    }
  S_StartAttackSound(p, sfx_sawhit);

  // turn to face target
  angle = R_PointToAngle2 (p->x, p->y,
			   linetarget->x, linetarget->y);
  if (angle - p->angle > ANG180)
    {
      if (angle - p->angle < -ANG90/20)
	p->angle = angle + ANG90/21;
      else
	p->angle -= ANG90/20;
    }
  else
    {
      if (angle - p->angle > ANG90/20)
	p->angle = angle - ANG90/21;
      else
	p->angle += ANG90/20;
    }
  p->eflags |= MFE_JUSTATTACKED;
}



//
// A_FireMissile : rocket launcher fires a rocket
//
void A_FireMissile(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[p->weaponinfo[p->readyweapon].ammo] -= p->weaponinfo[p->readyweapon].ammopershoot;
  p->SpawnPlayerMissile(MT_ROCKET);
}


//
// A_FireBFG
//
void A_FireBFG(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[p->weaponinfo[p->readyweapon].ammo] -= p->weaponinfo[p->readyweapon].ammopershoot;
  p->SpawnPlayerMissile(MT_BFG);
}


//
// A_FirePlasma
//
void A_FirePlasma(PlayerPawn *p, pspdef_t *psp)
{
  p->ammo[p->weaponinfo[p->readyweapon].ammo] -= p->weaponinfo[p->readyweapon].ammopershoot;

  p->SetPsprite(ps_flash, weaponstatenum_t(p->weaponinfo[p->readyweapon].flashstate
				     + (P_Random() & 1)));
  p->SpawnPlayerMissile(MT_PLASMA);
}



//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//
fixed_t bulletslope;

//added:16-02-98: Fab comments: autoaim for the bullet-type weapons
void P_BulletSlope(PlayerPawn *p)
{
  angle_t an;

  //added:18-02-98: if AUTOAIM, try to aim at something
  if(!p->player->autoaim || !cv_allowautoaim.value)
    goto notargetfound;

  // see which target is to be aimed at
  an = p->angle;
  bulletslope = p->AimLineAttack(an, 16*64*FRACUNIT);

  if (!linetarget)
    {
      an += 1<<26;
      bulletslope = p->AimLineAttack(an, 16*64*FRACUNIT);
      if (!linetarget)
        {
	  an -= 2<<26;
	  bulletslope = p->AimLineAttack(an, 16*64*FRACUNIT);
        }
      if(!linetarget)
        {
	notargetfound:
	  bulletslope = AIMINGTOSLOPE(p->aiming);
        }
    }
}


//
// P_GunShot
//
//added:16-02-98: used only for player (pistol,shotgun,chaingun)
//                supershotgun use p_lineattack directely
void P_GunShot(Actor *mo, bool accurate)
{
  angle_t     angle;
  int         damage;

  damage = 5*(P_Random ()%3+1);
  angle = mo->angle;

  if (!accurate)
    {
      angle += (P_Random()<<18); // WARNING: don't put this in one line 
      angle -= (P_Random()<<18); // else this expretion is ambiguous (evaluation order not diffined)
    }

  mo->LineAttack(angle, MISSILERANGE, bulletslope, damage);
}


//
// A_FirePistol
//
void A_FirePistol(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_pistol);

  //p->SetState(weaponstatenum_t(p->info->missilestate + 1));
  p->ammo[p->weaponinfo[p->readyweapon].ammo]--;

  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);

  P_BulletSlope(p);
  P_GunShot(p, !p->refire);
}


//
// A_FireShotgun
//
void A_FireShotgun(PlayerPawn *p, pspdef_t *psp)
{
  int         i;

  S_StartAttackSound(p, sfx_shotgn);
  //p->SetState(weaponstatenum_t(p->info->missilestate + 1));

  p->ammo[p->weaponinfo[p->readyweapon].ammo]--;
  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);

  P_BulletSlope(p);
  for (i=0 ; i<7 ; i++)
    P_GunShot (p, false);
}



//
// A_FireShotgun2 (SuperShotgun)
//
void A_FireShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  int         i;
  angle_t     angle;
  int         damage;

  S_StartAttackSound(p, sfx_dshtgn);
  //p->SetState(weaponstatenum_t(p->info->missilestate + 1));

  p->ammo[p->weaponinfo[p->readyweapon].ammo]-=2;

  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);

  P_BulletSlope (p);

  for (i=0 ; i<20 ; i++)
    {
      int slope = bulletslope + (P_SignedRandom()<<5);
      damage = 5*(P_Random ()%3+1);
      angle = p->angle + (P_SignedRandom() << 19);
      p->LineAttack(angle, MISSILERANGE, slope, damage);
    }
}


//
// A_FireCGun
//
void A_FireCGun(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_chgun);

  if (!p->ammo[p->weaponinfo[p->readyweapon].ammo])
    return;

  //p->SetState(weaponstatenum_t(p->info->missilestate + 1));
  p->ammo[p->weaponinfo[p->readyweapon].ammo]--;

  p->SetPsprite(ps_flash, weaponstatenum_t(p->weaponinfo[p->readyweapon].flashstate
				     + psp->state - &weaponstates[S_CHAIN1]));
  P_BulletSlope (p);
  P_GunShot (p, !p->refire);
}



//
// Flash light when fire gun
//
void A_Light0 (PlayerPawn *p, pspdef_t *psp)
{
  p->extralight = 0;
}

void A_Light1 (PlayerPawn *p, pspdef_t *psp)
{
  p->extralight = 1;
}

void A_Light2 (PlayerPawn *p, pspdef_t *psp)
{
  p->extralight = 2;
}


//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray(DActor *mo)
{
  int     i;
  int     j;
  int     damage;
  angle_t an;

  // offset angles from its attack angle
  for (i=0 ; i<40 ; i++)
    {
      an = mo->angle - ANG90/2 + ANG90/40*i;

      // mo->owner is the originator (player)
      //  of the missile
      mo->owner->AimLineAttack(an, 16*64*FRACUNIT);

      if (!linetarget)
	continue;

      DActor *extrabfg = mo->mp->SpawnDActor(linetarget->x, linetarget->y,
        linetarget->z + (linetarget->height>>2), MT_EXTRABFG);
      extrabfg->owner = mo->owner;

      damage = 0;
      for (j=0;j<15;j++)
	damage += (P_Random()&7) + 1;

      //BP: use extramobj as inflictor so we have the good death message
      linetarget->Damage(extrabfg, mo->owner, damage);
    }
}


//
// A_BFGsound
//
void A_BFGsound(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_bfg);
}

