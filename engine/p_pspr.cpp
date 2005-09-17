// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.26  2005/09/17 17:36:09  smite-meister
// fixed_t fixes
//
// Revision 1.25  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.24  2005/07/05 18:29:05  smite-meister
// damage types
//
// Revision 1.23  2005/07/05 17:36:42  smite-meister
// small fixes
//
// Revision 1.22  2005/04/19 18:28:28  smite-meister
// new RPCs
//
// Revision 1.20  2005/03/29 17:20:45  smite-meister
// state and mobjinfo tables fixed
//
// Revision 1.18  2004/09/06 19:58:03  smite-meister
// Doom linedefs done!
//
// Revision 1.17  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.16  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.12  2003/12/03 10:49:50  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.11  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.10  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.9  2003/05/30 13:34:46  smite-meister
// Cleanup, HUD improved, serialization
//
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Weapon sprite animation, weapon objects.
/// Action functions for weapons.

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "d_event.h"

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

#include "r_sprite.h"
#include "sounds.h"
#include "m_random.h"
#include "tables.h"

#include "hardware/hw3sound.h"


// TESTING: play any creature!
// changes: S_PLAY => info->spawnstate
// S_PLAY_ATK1 => info->missilestate
// S_PLAY_ATK2 => info->missilestate+1


// changes to the best owned weapon for which the player has ammo
void PlayerPawn::UseFavoriteWeapon()
{
  if (pendingweapon != wp_none)
    return; // already changing weapon

  int priority = -1;

  for (int i=0; i<NUMWEAPONS; i++)
    if (weaponowned[i] && player->options.weaponpref[i] > priority)
      {
	int c;
	if (weaponinfo[i].ammo == am_manaboth) // damn!
	  c = min(ammo[am_mana1], ammo[am_mana2]);
	else
	  c = ammo[weaponinfo[i].ammo];

	if (c >= weaponinfo[i].ammopershoot)
	  {
	    pendingweapon = weapontype_t(i);
	    priority = player->options.weaponpref[i];
	  }
      }

  if (pendingweapon == readyweapon)
    pendingweapon = wp_none;
}


//
// Called at start of level for each player.
//
void PlayerPawn::SetupPsprites()
{
  // remove all psprites
  for (int i=0 ; i<NUMPSPRITES ; i++)
    {
      psprites[i].state = NULL;
      psprites[i].tics = 0;
      psprites[i].sx = 0;
      psprites[i].sy = WEAPONBOTTOM;
    }

  // spawn the gun
  pendingweapon = readyweapon;
  BringUpWeapon();
}



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
	  psp->sx = state->misc1;
	}
      if(state->misc2)
	{
	  psp->sy = state->misc2;
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
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void PlayerPawn::BringUpWeapon()
{
  if (pendingweapon == wp_none)
    {
      if (readyweapon == wp_none)
	return; // well, no weapon held
      else
	pendingweapon = readyweapon;
    }

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
    
  pendingweapon = wp_none;
  attackphase = 0;
  psprites[ps_weapon].sy = WEAPONBOTTOM;

  SetPsprite(ps_weapon, newstate);
}



// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
bool PlayerPawn::CheckAmmo()
{
  ammotype_t at = weaponinfo[readyweapon].ammo;

  // Minimal amount for one shot varies.
  int count = weaponinfo[readyweapon].ammopershoot;

  // Some do not need ammunition anyway.
  // Return if current ammunition sufficient.
  if (at == am_manaboth)
    {
      // ugly special case...
      if (ammo[am_mana1] >= count && ammo[am_mana2] >= count)
	return true;
    }
  else if (at == am_noammo || ammo[at] >= count)
    return true;

  // Out of ammo, pick a weapon to change to.
  UseFavoriteWeapon();

  // Now set appropriate weapon overlay.
  SetPsprite(ps_weapon, weaponinfo[readyweapon].downstate);

  return false;
}



void PlayerPawn::FireWeapon()
{
  weaponstatenum_t  newstate;

  if (!CheckAmmo())
    return;

  // go to melee state (later move to shooting state if needed....)
  pres->SetAnim(presentation_t::Melee);
  //  P_SetMobjState(player->mo, PStateAttack[player->class]); // S_PLAY_ATK1);

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
// Player died, so put the weapon away.
//
void PlayerPawn::DropWeapon()
{
  SetPsprite(ps_weapon, weaponinfo[readyweapon].downstate);
}


//
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady(PlayerPawn *p, pspdef_t *psp)
{
  // get out of attack state
  int anim = p->pres->GetAnim();
  if (anim == presentation_t::Shoot || anim == presentation_t::Melee)
    p->pres->SetAnim(presentation_t::Idle);

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
  if (p->pendingweapon != wp_none || !p->health)
    {
      // change weapon
      //  (pending weapon should allready be validated)
      p->SetPsprite(ps_weapon, p->weaponinfo[p->readyweapon].downstate);
      return;
    }

  // check for fire
  //  the missile launcher and bfg do not auto fire. why not?
  if (p->player->cmd.buttons & ticcmd_t::BT_ATTACK)
    {
      p->attackdown = true;
      p->FireWeapon();
      return;
    }
  else
    p->attackdown = false;
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
  if ((p->player->cmd.buttons & ticcmd_t::BT_ATTACK)
       && p->pendingweapon == wp_none
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
  p->pres->SetAnim(presentation_t::Shoot);
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
  int damage = (P_Random ()%10+1)<<1;

  if (p->powers[pw_strength])
    damage *= 10;

  angle_t angle = p->yaw;
  angle += P_SignedRandom() << 18;

  fixed_t slope = p->AimLineAttack(angle, MELEERANGE);
  p->LineAttack(angle, MELEERANGE, slope, damage);

  // turn to face target
  if (linetarget)
    {
      S_StartAttackSound(p, sfx_punch);
      p->yaw = R_PointToAngle2(p->pos.x, p->pos.y, linetarget->pos.x, linetarget->pos.y);
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
  int damage = 2*(P_Random ()%10+1);
  angle_t angle = p->yaw + (P_SignedRandom() << 18);

  // use meleerange + 1 se the puff doesn't skip the flash
  fixed_t slope = p->AimLineAttack(angle, MELEERANGE+1);
  p->LineAttack(angle, MELEERANGE+1, slope, damage, dt_cutting | dt_norecoil);

  if (!linetarget)
    {
      S_StartAttackSound(p, sfx_sawful);
      return;
    }
  S_StartAttackSound(p, sfx_sawhit);

  // turn to face target
  angle = R_PointToAngle2 (p->pos.x, p->pos.y, linetarget->pos.x, linetarget->pos.y);
  if (angle - p->yaw > ANG180)
    {
      if (angle - p->yaw < -ANG90/20)
	p->yaw = angle + ANG90/21;
      else
	p->yaw -= ANG90/20;
    }
  else
    {
      if (angle - p->yaw > ANG90/20)
	p->yaw = angle - ANG90/21;
      else
	p->yaw += ANG90/20;
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

//added:16-02-98: Fab comments: autoaim for the bullet-type weapons
fixed_t P_BulletSlope(PlayerPawn *p)
{
  angle_t an = p->yaw;
  fixed_t slope;

  //added:18-02-98: if AUTOAIM, try to aim at something
  if(!p->player->options.autoaim || !cv_allowautoaim.value)
    goto notargetfound;

  // see which target is to be aimed at
  slope = p->AimLineAttack(an, AIMRANGE);

  if (!linetarget)
    {
      an += 1<<26;
      slope = p->AimLineAttack(an, AIMRANGE);
      if (!linetarget)
        {
	  an -= 2<<26;
	  slope = p->AimLineAttack(an, AIMRANGE);
        }
      if(!linetarget)
        {
	notargetfound:
	  slope = Tan(p->pitch);
        }
    }

  return slope;
}


//
// P_GunShot
//
//added:16-02-98: used only for player (pistol,shotgun,chaingun)
//                supershotgun use p_lineattack directely
static fixed_t bulletslope;

static void P_GunShot(PlayerPawn *p, bool accurate)
{
  int damage = 5*(P_Random()%3 + 1);
  angle_t angle = p->yaw;
  fixed_t slope = bulletslope;

  if (!accurate)
    {
      angle += P_SignedRandom() << 18;
      slope += P_SignedFRandom(12); // TEST vertical scatter
    }

  p->LineAttack(angle, MISSILERANGE, slope, damage);
}


//
// A_FirePistol
//
void A_FirePistol(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_pistol);
  p->pres->SetAnim(presentation_t::Shoot);

  p->ammo[p->weaponinfo[p->readyweapon].ammo]--;
  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);

  bulletslope = P_BulletSlope(p);
  P_GunShot(p, !p->refire);
}


//
// A_FireShotgun
//
void A_FireShotgun(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_shotgn);
  p->pres->SetAnim(presentation_t::Shoot);

  p->ammo[p->weaponinfo[p->readyweapon].ammo]--;
  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);

  bulletslope = P_BulletSlope(p);
  for (int i=0; i<7; i++)
    P_GunShot(p, false);
}



//
// A_FireShotgun2 (SuperShotgun)
//
void A_FireShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dshtgn);
  p->pres->SetAnim(presentation_t::Shoot);

  p->ammo[p->weaponinfo[p->readyweapon].ammo]-=2;

  p->SetPsprite(ps_flash, p->weaponinfo[p->readyweapon].flashstate);

  bulletslope = P_BulletSlope(p);

  for (int i=0 ; i<20 ; i++)
    {
      fixed_t slope = bulletslope + P_SignedFRandom(11);
      int damage = 5*(P_Random ()%3+1);
      angle_t angle = p->yaw + (P_SignedRandom() << 19);
      p->LineAttack(angle, MISSILERANGE, slope, damage);
    }
}


void A_OpenShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dbopn);
}


void A_LoadShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dbload);
}


void A_CloseShotgun2(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_dbcls);
  A_ReFire(p, psp);
}


//
// A_FireCGun
//
void A_FireCGun(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_chgun);

  if (!p->ammo[p->weaponinfo[p->readyweapon].ammo])
    return;

  p->pres->SetAnim(presentation_t::Shoot);
  p->ammo[p->weaponinfo[p->readyweapon].ammo]--;

  p->SetPsprite(ps_flash, weaponstatenum_t(p->weaponinfo[p->readyweapon].flashstate
				     + psp->state - &weaponstates[S_CHAIN1]));
  bulletslope = P_BulletSlope(p);
  P_GunShot(p, !p->refire);
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
  // offset angles from its attack angle
  for (int i=0 ; i<40 ; i++)
    {
      angle_t an = mo->yaw - ANG90/2 + ANG90/40*i;

      // mo->owner is the originator (player)
      //  of the missile
      mo->owner->AimLineAttack(an, 16*64);

      if (!linetarget)
	continue;

      DActor *extrabfg = mo->mp->SpawnDActor(linetarget->pos.x, linetarget->pos.y,
        linetarget->pos.z + (linetarget->height>>2), MT_EXTRABFG);
      extrabfg->owner = mo->owner;

      int damage = 0;
      for (int j=0; j<15; j++)
	damage += (P_Random()&7) + 1;

      // to get correct recoil, we must use either the player or the BFG ball as the inflictor (matter of taste)
      linetarget->Damage(mo, mo->owner, damage);
    }
}


//
// A_BFGsound
//
void A_BFGsound(PlayerPawn *p, pspdef_t *psp)
{
  S_StartAttackSound(p, sfx_bfg);
}

