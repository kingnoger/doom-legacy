// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief New action functions.
///
/// Separated from the other code, so that you can include it or remove it easily.

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "g_pawn.h"

#include "info.h"
#include "p_fab.h"
#include "p_pspr.h"
#include "m_random.h"
#include "r_draw.h"

// action function for running FS
void A_StartFS(DActor *actor)
{
  int script = actor->tics;
  actor->tics = 0; // takes no time
  actor->mp->FS_RunScript(script, actor);
}


// weapon action function for running FS
void A_StartWeaponFS(PlayerPawn *p, pspdef_t *psp)
{
  int script = psp->tics;
  psp->tics = 0; // takes no time
  p->mp->FS_RunScript(script, p);
}


// action function for running ACS
void A_StartACS(DActor *actor)
{
  int script = actor->tics;
  byte args[5] = {0,0,0,0,0}; // TODO take args from sprite and frame fields

  actor->tics = 0; // takes no time
  actor->mp->StartACS(script, args, actor, NULL, 0);
}


// weapon action function for running ACS
void A_StartWeaponACS(PlayerPawn *p, pspdef_t *psp)
{
  int script = psp->tics;
  byte args[5] = {0,0,0,0,0};

  psp->tics = 0; // takes no time
  p->mp->StartACS(script, args, p, NULL, 0);
}



// Action routine, for the ROCKET thing.
// This one adds trails of smoke to the rocket.
// The action pointer of the S_ROCKET state must point here to take effect.
// This routine is based on the Revenant Fireball Tracer code A_Tracer()
//
void A_SmokeTrailer(DActor *actor)
{
  if (game.tic % 4)
    return;

  // add the smoke behind the rocket
  DActor *th = actor->mp->SpawnDActor(actor->pos.x - actor->vel.x,
				      actor->pos.y - actor->vel.y, actor->pos.z, MT_SMOK);

  th->vel.z = 1;
  th->tics -= P_Random()&3;
  if (th->tics < 1)
    th->tics = 1;
}


static bool resettrans = false;
//  Set the translucency map for each frame state of mobj
//
static void R_SetTrans(statenum_t state1, statenum_t state2, transnum_t transmap)
{
  state_t *state = &states[state1];
  int s1 = state1;
  int s2 = state2;
  do {
    state->frame &= ~TFF_TRANSMASK;
    if (!resettrans)
      state->frame |= (transmap << TFF_TRANSSHIFT);
    state++;
  } while (s1++ < s2);
}


//  hack the translucency in the states for a set of standard doom sprites
//
void P_SetTranslucencies()
{
  //revenant fireball
  R_SetTrans (S_TRACER    , S_TRACER2    , tr_transfir);
  R_SetTrans (S_TRACEEXP1 , S_TRACEEXP3  , tr_transmed);
                                           
    //rev. fireball. smoke trail           
    R_SetTrans (S_SMOKE1    , S_SMOKE5     , tr_transmed);
                                           
    //imp fireball                         
    R_SetTrans (S_TBALL1    , S_TBALL2     , tr_transfir);
    R_SetTrans (S_TBALLX1   , S_TBALLX3    , tr_transmed);
                                           
    //archvile attack                      
    R_SetTrans (S_FIRE1     , S_FIRE30     , tr_transfir);
                                           
    //bfg ball                             
    R_SetTrans (S_BFGSHOT   , S_BFGSHOT2   , tr_transfir);
    R_SetTrans (S_BFGLAND   , S_BFGLAND3   , tr_transmed);
    R_SetTrans (S_BFGLAND4  , S_BFGLAND6   , tr_transmor);
    R_SetTrans (S_BFGEXP    , S_NULL       , tr_transmed);
    R_SetTrans (S_BFGEXP2   , S_BFGEXP4    , tr_transmor);
                                           
    //plasma bullet                        
    R_SetTrans (S_PLASBALL  , S_PLASBALL2  , tr_transfir);
    R_SetTrans (S_PLASEXP   , S_PLASEXP2   , tr_transmed);
    R_SetTrans (S_PLASEXP3  , S_PLASEXP5   , tr_transmor);
                                           
    //bullet puff                          
    R_SetTrans (S_PUFF1     , S_PUFF4      , tr_transmor);
                                           
    //teleport fog                         
    R_SetTrans (S_TFOG      , S_TFOG5      , tr_transmed);
    R_SetTrans (S_TFOG6     , S_TFOG10     , tr_transmor);
                                           
    //respawn item fog                     
    R_SetTrans (S_IFOG      , S_IFOG5      , tr_transmed);
                                           
    //soulsphere                           
    R_SetTrans (S_SOUL      , S_SOUL6      , tr_transmed);
    //invulnerability                      
    R_SetTrans (S_PINV      , S_PINV4      , tr_transmed);
    //blur artifact                        
    R_SetTrans (S_PINS      , S_PINS4      , tr_transmed);
    //megasphere                           
    R_SetTrans (S_MEGA      , S_MEGA4      , tr_transmed);
                            
    R_SetTrans (S_GREENTORCH, S_REDTORCH4  , tr_transfx1); // blue torch
    R_SetTrans (S_GTORCHSHRT, S_RTORCHSHRT4, tr_transfx1); // short blue torch

    // flaming barrel !!
    R_SetTrans (S_BBAR1     , S_BBAR3      , tr_transfx1);

    //lost soul
    R_SetTrans (S_SKULL_STND, S_SKULL_DIE6 , tr_transfx1);
    //baron shot
    R_SetTrans (S_BRBALL1   , S_BRBALL2    , tr_transfir);
     R_SetTrans (S_BRBALLX1 , S_BRBALLX3   , tr_transmed);
    //demon spawnfire
    R_SetTrans (S_SPAWNFIRE1, S_SPAWNFIRE3 , tr_transfir);
    R_SetTrans (S_SPAWNFIRE4, S_SPAWNFIRE8 , tr_transmed);
    //caco fireball
    R_SetTrans (S_RBALL1    , S_RBALL2     , tr_transfir);
    R_SetTrans (S_RBALLX1   , S_RBALLX3    , tr_transmed);

    //arachno shot
    R_SetTrans (S_ARACH_PLAZ, S_ARACH_PLAZ2, tr_transfir);
    R_SetTrans (S_ARACH_PLEX, S_ARACH_PLEX2, tr_transmed);
    R_SetTrans (S_ARACH_PLEX3,S_ARACH_PLEX4, tr_transmor);
    R_SetTrans (S_ARACH_PLEX5,       S_NULL, tr_transhi);

    //blood puffs!
    //R_SetTrans (S_BLOOD1   ,            0, tr_transmed);
    //R_SetTrans (S_BLOOD2   , S_BLOOD3    , tr_transmor);

    //eye in symbol
    R_SetTrans (S_EVILEYE    , S_EVILEYE4  , tr_transmed);
                                          
    //mancubus fireball
    R_SetTrans (S_FATSHOT1   , S_FATSHOT2  , tr_transfir);
    R_SetTrans (S_FATSHOTX1  , S_FATSHOTX3 , tr_transmed);

    // rockets explosion
    R_SetTrans (S_EXPLODE1   , S_EXPLODE2  , tr_transfir);
    R_SetTrans (S_EXPLODE3   ,       S_NULL, tr_transmed);

    //Fab: lava/slime damage smoke test
    R_SetTrans (S_SMOK1      , S_SMOK5     , tr_transmed);
    R_SetTrans (S_SPLASH1    , S_SPLASH3   , tr_transmor);
}

void Translucency_OnChange()
{
  if (cv_translucency.value == 0)
    resettrans = true;
  if (cv_fuzzymode.value == 0)
    P_SetTranslucencies();
  resettrans = false;
}


// =======================================================================
//                    FUNKY DEATHMATCH COMMANDS
// =======================================================================


// Called when var. 'bloodtime' is changed : set the blood states duration
//
void BloodTime_OnChange()
{
    states[S_BLOOD1].tics = 8;
    states[S_BLOOD2].tics = 8;
    states[S_BLOOD3].tics = (cv_bloodtime.value*TICRATE) - 16;

    CONS_Printf ("blood lasts for %d seconds\n", cv_bloodtime.value);
}


/*
FIXME touch functions for actors (wraithverge, mage lightning

// extra pain
void DActor::Howl()
{
  int sound = 0;

  switch (type)
    {
    case MT_CENTAUR:
    case MT_CENTAURLEADER:
    case MT_ETTIN:
      sound = SFX_PUPPYBEAT;
      break;
    default:
      break;
    }

  if (!S_GetSoundPlayingInfo(this, sound))
    S_StartSound(this, sound);
}





int MT_HOLY_FX_touchfunc(this, Actor *p)
{
  if (p->flags & MF_SHOOTABLE && p != this->owner)
    {
      if (owner->team && p->team == owner->team)
	{
	  // don't attack other co-op players
	  return false;
	}

      if (p->flags2 & MF2_REFLECTIVE
	  && (p->player || p->flags2 & MF2_BOSS))
	{
	  this->target = this->owner;
	  this->owner = p;
	  return false;
	}

      if (p->flags & MF_COUNTKILL || p->player)
	this->target = p;

      if (P_Random() < 96)
	{
	  damage = 12;
	  if (p->player || p->flags2 & MF2_BOSS)
	    {
	      damage = 3;
	      // ghost burns out faster when attacking players/bosses
	      this->health -= 6;
	    }

	  p->Damage(this, this->owner, damage);
	  if (P_Random() < 128)
	    {
	      P_SpawnMobj(this->x, this->y, this->z, MT_HOLY_PUFF);
	      S_StartSound(this, SFX_SPIRIT_ATTACK);
	      if (p->flags & MF_COUNTKILL && P_Random() < 128)
		p->Howl()
	    }
	}

      if (p->health <= 0)
	this->target = NULL;

    }
  return false;
}



int MT_LIGHTNING_touchfunc(this, Actor *p)
{
  //if(this->type == MT_LIGHTNING_FLOOR || this->type == MT_LIGHTNING_CEILING)

  if (p->flags & MF_SHOOTABLE && p != this->ultimateowner)
    {
      if (p->mass != MAXINT)
	{
	  p->vel.x += this->vel.x>>4;
	  p->vel.y += this->vel.y>>4;
	}
      // players and bosses take less damage
      if ((!p->player && !(p->flags2 & MF2_BOSS))
	 || !(leveltime&1))
	{
	  if (p->type == MT_CENTAUR ||
	      p->type == MT_CENTAURLEADER)
	    { // Lightning does more damage to centaurs
	      p->Damage(this, this->ultimateowner, 9);
	    }
	  else
	    {
	      p->Damage(this, this->ultimateowner, 3);
	    }

 	  if (!(S_GetSoundPlayingInfo(this, SFX_MAGE_LIGHTNING_ZAP)))
	    S_StartSound(this, SFX_MAGE_LIGHTNING_ZAP);
					}
	  if (p->flags&MF_COUNTKILL && P_Random() < 64)
	    p->Howl();
	}
      this->health--;
      if (this->health <= 0 || p->health <= 0)
	return true;

      if (this->type == MT_LIGHTNING_FLOOR)
	{
	  if (this->twin 
	     && !(this->twin)->target)
	    {
	      (this->twin)->target = p;
	    }
	}
      else if (!this->target)
	{
	  this->target = p;
	}
    }
  return false; // lightning zaps through all sprites
}



int MT_LIGHTNING_ZAP_touchfunc(this, Actor *p)
{
  //if(this->type == MT_LIGHTNING_ZAP)

  if (p->flags & MF_SHOOTABLE && p != this->ultimateowner)
    {			
      Actor *lmo = this->emitter;
      if(lmo)
	{
	  if(lmo->type == MT_LIGHTNING_FLOOR)
	    {
	      if(lmo->twin 
		 && !(lmo->twin)->target)
		{
		  (lmo->twin)->target = p;
		}
	    }
	  else if(!lmo->target)
	    {
	      lmo->target = p;
	    }
	  if(!(leveltime&3))
	    {
	      lmo->health--;
	    }
	}
    }
  return -1; // do not force return
}


int MT_MSTAFF_FX2_touchfunc(this, Actor *p)
{
  //if(this->type == MT_MSTAFF_FX2
  if (p != this->ultimateowner &&
      !p->player && !(p->flags2&MF2_BOSS))
    {
      switch(p->type)
	{
	case MT_FIGHTER_BOSS:	// these not flagged boss
	case MT_CLERIC_BOSS:	// so they can be blasted
	case MT_MAGE_BOSS:
	  break;
	default:
	  p->Damage(this, this->ultimateowner, 10);
	  return false; // force return
	  break;
	}
    }

  //do not force return
  return -1
}


*/
