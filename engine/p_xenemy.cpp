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
// Revision 1.10  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.9  2003/12/09 01:02:01  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.8  2003/11/27 11:28:26  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.7  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.6  2003/05/30 13:34:47  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.5  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.4  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.3  2003/04/14 08:58:28  smite-meister
// Hexen maps load.
//
// Revision 1.2  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.1  2003/03/15 20:07:18  smite-meister
// Initial Hexen compatibility!
//
//
//
// DESCRIPTION:
//   Hexen enemy AI
//
//-----------------------------------------------------------------------------

#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_game.h"
#include "g_player.h"
#include "p_enemy.h"
#include "p_maputl.h"

#include "command.h"
#include "d_items.h"
#include "m_random.h"

#include "r_main.h"
#include "sounds.h"
#include "s_sound.h"
#include "tables.h"

extern consvar_t  cv_deathmatch;
extern fixed_t FloatBobOffsets[64];

angle_t abs(angle_t a);
int  P_FaceMobj(Actor *source, Actor *target, angle_t *delta);
void P_SpawnDirt(DActor *actor, fixed_t radius);


//----------------------------------------------------------------------------
//
// CorpseQueue Routines
//
//----------------------------------------------------------------------------

// Corpse queue for monsters - this should be saved out
/*
#define CORPSEQUEUESIZE	64

void P_InitCreatureCorpseQueue(bool corpseScan)
{
  thinker_t *think;
  Actor *mo;

  // Initialize queue

  if (!corpseScan) return;

  // Search mobj list for corpses and place them in this queue
  for(think = thinkercap.next; think != &thinkercap; think = think->next)
    {
      if(think->function != P_MobjThinker) continue;
      mo = (Actor *)think;
      if (!(mo->flags & MF_CORPSE)) continue;	// Must be a corpse
      if (mo->flags & MF_ICECORPSE) continue;	// Not ice corpses
      // Only corpses that call A_QueueCorpse from death routine
      switch(mo->type)
	{
	case MT_CENTAUR:
	case MT_CENTAURLEADER:
	case MT_DEMON:
	case MT_DEMON2:
	case MT_WRAITH:
	case MT_WRAITHB:
	case MT_BISHOP:
	case MT_ETTIN:
	case MT_PIG:
	case MT_CENTAUR_SHIELD:
	case MT_CENTAUR_SWORD:
	case MT_DEMONCHUNK1:
	case MT_DEMONCHUNK2:
	case MT_DEMONCHUNK3:
	case MT_DEMONCHUNK4:
	case MT_DEMONCHUNK5:
	case MT_DEMON2CHUNK1:
	case MT_DEMON2CHUNK2:
	case MT_DEMON2CHUNK3:
	case MT_DEMON2CHUNK4:
	case MT_DEMON2CHUNK5:
	case MT_FIREDEMON_SPLOTCH1:
	case MT_FIREDEMON_SPLOTCH2:
	  A_QueueCorpse(mo);		// Add corpse to queue
	  break;
	default:
	  break;
	}
    }
}
*/

void A_QueueCorpse(DActor *actor)
{
  // do nothing for now.
}

//----------------------------------------------------------------------------
//
// FUNC P_CheckMeleeRange2
//
//----------------------------------------------------------------------------

bool P_CheckMeleeRange2(DActor *actor)
{
  Actor *mo;
  fixed_t dist;

  if(!actor->target)
    {
      return(false);
    }
  mo = actor->target;
  dist = P_AproxDistance(mo->x-actor->x, mo->y-actor->y);
  if(dist >= MELEERANGE*2 || dist < MELEERANGE)
    {
      return(false);
    }
  if(!actor->mp->CheckSight(actor, mo))
    {
      return(false);
    }
  if(mo->z > actor->z+actor->height)
    { // Target is higher than the attacker
      return(false);
    }
  else if(actor->z > mo->z+mo->height)
    { // Attacker is higher
      return(false);
    }
  return(true);
}



static void FaceMovementDirection(DActor *actor)
{
  switch (actor->movedir)
    {
    case DI_EAST:
      actor->angle = 0<<24;
      break;
    case DI_NORTHEAST:
      actor->angle = 32<<24;
      break;
    case DI_NORTH:
      actor->angle = 64<<24;
      break;
    case DI_NORTHWEST:
      actor->angle = 96<<24;
      break;
    case DI_WEST:
      actor->angle = 128<<24;
      break;
    case DI_SOUTHWEST:
      actor->angle = 160<<24;
      break;
    case DI_SOUTH:
      actor->angle = 192<<24;
      break;
    case DI_SOUTHEAST:
      actor->angle = 224<<24;
      break;
    }
}



//=========================================
//  HEXEN ACTION ROUTINES
//=========================================


//============================================================================
//
// A_SetInvulnerable
//
//============================================================================

void A_SetInvulnerable(DActor *actor)
{
  actor->flags2 |= MF2_INVULNERABLE;
}

//============================================================================
//
// A_UnSetInvulnerable
//
//============================================================================

void A_UnSetInvulnerable(DActor *actor)
{
  actor->flags2 &= ~MF2_INVULNERABLE;
}

//============================================================================
//
// A_SetReflective
//
//============================================================================

void A_SetReflective(DActor *actor)
{
  actor->flags2 |= MF2_REFLECTIVE;

  if ((actor->type == MT_CENTAUR) ||
      (actor->type == MT_CENTAURLEADER))
    {
      A_SetInvulnerable(actor);
    }
}

//============================================================================
//
// A_UnSetReflective
//
//============================================================================

void A_UnSetReflective(DActor *actor)
{
  actor->flags2 &= ~MF2_REFLECTIVE;

  if ((actor->type == MT_CENTAUR) ||
      (actor->type == MT_CENTAURLEADER))
    {
      A_UnSetInvulnerable(actor);
    }
}


//----------------------------------------------------------------------------
//
// PROC A_PigLook
//
//----------------------------------------------------------------------------

void A_PigLook(DActor *actor)
{
  if(actor->UpdateMorph(10))
    {
      return;
    }
  A_Look(actor);
}

//----------------------------------------------------------------------------
//
// PROC A_PigChase
//
//----------------------------------------------------------------------------

void A_PigChase(DActor *actor)
{
  if(actor->UpdateMorph(3))
    {
      return;
    }
  A_Chase(actor);
}

//============================================================================
//
// A_PigAttack
//
//============================================================================

void A_PigAttack(DActor *actor)
{
  if(actor->UpdateMorph(18))
    {
      return;
    }
  if(!actor->target)
    {
      return;
    }
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, 2+(P_Random()&1));
      S_StartSound(actor, SFX_PIG_ATTACK);
    }
}

//============================================================================
//
// A_PigPain
//
//============================================================================

void A_PigPain(DActor *actor)
{
  A_Pain(actor);
  if(actor->z <= actor->floorz)
    {
      actor->pz = int(3.5*FRACUNIT);
    }
}



//----------------------------------------------------------------------------
//
// Minotaur variables
//
// 	owner		pointer to player that spawned it (Actor)
//	special2		internal to minotaur AI
//	args[0]			args[0]-args[3] together make up minotaur start time
//	args[1]			|
//	args[2]			|
//	args[3]			V
//	args[4]			charge duration countdown
//----------------------------------------------------------------------------

void A_MinotaurFade0(DActor *actor)
{
  actor->flags &= ~MF_ALTSHADOW;
  actor->flags |= MF_SHADOW;
}

void A_MinotaurFade1(DActor *actor)
{
  // Second level of transparency
  actor->flags &= ~MF_SHADOW;
  actor->flags |= MF_ALTSHADOW;
}

void A_MinotaurFade2(DActor *actor)
{
  // Make fully visible
  actor->flags &= ~MF_SHADOW;
  actor->flags &= ~MF_ALTSHADOW;
}


//----------------------------------------------------------------------------
//
// A_MinotaurRoam - 
//
// 
//----------------------------------------------------------------------------

void A_MinotaurLook(DActor *actor);

void A_MinotaurRoam(DActor *actor)
{
  unsigned int *starttime = (unsigned int *)actor->args;

  actor->flags &= ~MF_SHADOW;			// In case pain caused him to 
  actor->flags &= ~MF_ALTSHADOW;		// skip his fade in.

  if ((gametic - *starttime) >= MAULATORTICS)
    {
      actor->Damage(NULL,NULL,10000);
      return;
    }

  if (P_Random()<30)
    A_MinotaurLook(actor);		// adjust to closest target

  if (P_Random()<6)
    {
      //Choose new direction
      actor->movedir = P_Random() % 8;
      FaceMovementDirection(actor);
    }
  if (!actor->P_Move())
    {
      // Turn
      if (P_Random() & 1)
	actor->movedir = (++actor->movedir)%8;
      else
	actor->movedir = (actor->movedir+7)%8;
      FaceMovementDirection(actor);
    }
}


//----------------------------------------------------------------------------
//
//	PROC A_MinotaurLook
//
// Look for enemy of player
//----------------------------------------------------------------------------
#define MINOTAUR_LOOK_DIST		(16*54*FRACUNIT)

void A_MinotaurLook(DActor *actor)
{
  Actor *mo = NULL;
  fixed_t dist;
  int i;
  Actor *master = actor->owner;

  actor->target = NULL;
  if (cv_deathmatch.value)  // Quick search for players
    {
      int n = actor->mp->players.size();
      for (i=0; i<n; i++)
	{
	  mo = actor->mp->players[i]->pawn;
	  if (!mo || mo == master)
	    continue;
	  if (mo->health <= 0)
	    continue;
	  dist = P_AproxDistance(actor->x - mo->x, actor->y - mo->y);
	  if (dist > MINOTAUR_LOOK_DIST)
	    continue;
	  actor->target = mo;
	  break;
	}
    }

  if (!actor->target) // Near player monster search
    {
      if (master && (master->health > 0))
	mo = actor->mp->RoughBlockSearch(master, master, 20, 1);
      else
	mo = actor->mp->RoughBlockSearch(actor, master, 20, 1);
      actor->target = mo;
    }

  if (!actor->target) // Normal monster search
    {
      /*
	// FIXME add a thinkerlist iterator function to p_map.cpp?
      for(think = thinkercap.next; think != &thinkercap; think = think->next)
	{
	  if(think->function != P_MobjThinker) continue;
	  mo = (Actor *)think;
	  if (!(mo->flags&MF_COUNTKILL)) continue;
	  if (mo->health <= 0) continue;
	  if (!(mo->flags&MF_SHOOTABLE)) continue;
	  dist = P_AproxDistance(actor->x - mo->x, actor->y - mo->y);
	  if (dist > MINOTAUR_LOOK_DIST) continue;
	  if ((mo == master) || (mo == actor)) continue;
	  if ((mo->type == MT_XMINOTAUR) &&
	  (mo->owner == actor->owner)) continue;
	  actor->target = mo;
	  break;			// Found mobj to attack
	}
      */
    }

  if (actor->target)
    {
      actor->SetState(S_XMNTR_WALK1, false);
    }
  else
    {
      actor->SetState(S_XMNTR_ROAM1, false);
    }
}




void A_MinotaurChase(DActor *actor)
{
  unsigned int *starttime = (unsigned int *)actor->args;

  actor->flags &= ~MF_SHADOW;			// In case pain caused him to 
  actor->flags &= ~MF_ALTSHADOW;		// skip his fade in.

  if ((gametic - *starttime) >= MAULATORTICS)
    {
      actor->Damage(NULL,NULL,10000);
      return;
    }

  if (P_Random()<30)
    A_MinotaurLook(actor);		// adjust to closest target

  if (!actor->target || (actor->target->health <= 0) ||
      !(actor->target->flags&MF_SHOOTABLE))
    { // look for a new target
      actor->SetState(S_XMNTR_LOOK1);
      return;
    }

  FaceMovementDirection(actor);
  actor->reactiontime=0;

  // Melee attack
  if (actor->info->meleestate && actor->CheckMeleeRange())
    {
      if(actor->info->attacksound)
	{
	  S_StartSound (actor, actor->info->attacksound);
	}
      actor->SetState(actor->info->meleestate);
      return;
    }

  // Missile attack
  if (actor->info->missilestate && actor->CheckMissileRange())
    {
      actor->SetState(actor->info->missilestate);
      return;
    }

  // chase towards target
  if (!actor->P_Move())
    {
      actor->P_NewChaseDir();
    }

  // Active sound
  if(actor->info->activesound && P_Random() < 6)
    {
      S_StartSound(actor, actor->info->activesound);
    }

}


//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk1
//
// Melee attack.
//
//----------------------------------------------------------------------------

void A_XMinotaurAtk1(DActor *actor)
{
  if (!actor->target) return;

  S_StartSound(actor, SFX_MAULATOR_HAMMER_SWING);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(4));
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurDecide
//
// Choose a missile attack.
//
//----------------------------------------------------------------------------

#define MNTR_CHARGE_SPEED (23*FRACUNIT)

void A_XMinotaurDecide(DActor *actor)
{
  angle_t angle;
  Actor *target = actor->target;
  int dist;

  if (!target) return;
  dist = P_AproxDistance(actor->x-target->x, actor->y-target->y);

  if(target->z+target->height > actor->z
     && target->z+target->height < actor->z+actor->height
     && dist < 16*64*FRACUNIT
     && dist > 1*64*FRACUNIT
     && P_Random() < 230)
    { // Charge attack
      // Don't call the state function right away
      actor->SetState(S_XMNTR_ATK4_1, false);
      actor->eflags |= MFE_SKULLFLY;
      A_FaceTarget(actor);
      angle = actor->angle>>ANGLETOFINESHIFT;
      actor->px = FixedMul(MNTR_CHARGE_SPEED, finecosine[angle]);
      actor->py = FixedMul(MNTR_CHARGE_SPEED, finesine[angle]);
      actor->args[4] = 35/2; // Charge duration
    }
  else if(target->z == target->floorz
	  && dist < 9*64*FRACUNIT
	  && P_Random() < 100)
    { // Floor fire attack
      actor->SetState(S_XMNTR_ATK3_1);
      actor->special2 = 0;
    }
  else
    { // Swing attack
      A_FaceTarget(actor);
      // Don't need to call P_SetMobjState because the current state
      // falls through to the swing attack
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurCharge
//
//----------------------------------------------------------------------------

void A_XMinotaurCharge(DActor *actor)
{
  DActor *puff;

  if (!actor->target) return;

  if(actor->args[4] > 0)
    {
      puff = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_PUNCHPUFF);
      puff->pz = 2*FRACUNIT;
      actor->args[4]--;
    }
  else
    {
      actor->eflags &= ~MFE_SKULLFLY;
      actor->SetState(actor->info->seestate);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk2
//
// Swing attack.
//
//----------------------------------------------------------------------------

void A_XMinotaurAtk2(DActor *actor)
{
  DActor *mo;
  angle_t angle;
  fixed_t momz;

  if(!actor->target) return;

  S_StartSound(actor, SFX_MAULATOR_HAMMER_SWING);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(3));
      return;
    }
  mo = actor->SpawnMissile(actor->target, MT_MNTRFX1);
  if(mo)
    {
      //S_StartSound(mo, sfx_minat2);
      momz = mo->pz;
      angle = mo->angle;
      actor->SpawnMissileAngle(MT_MNTRFX1, angle-(ANG45/8), momz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle+(ANG45/8), momz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle-(ANG45/16), momz);
      actor->SpawnMissileAngle(MT_MNTRFX1, angle+(ANG45/16), momz);
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MinotaurAtk3
//
// Floor fire attack.
//
//----------------------------------------------------------------------------

void A_XMinotaurAtk3(DActor *actor)
{
  Actor *t = actor->target;
  if (!t)
    return;

  if(actor->CheckMeleeRange())
    {
      t->Damage(actor, actor, HITDICE(3));
      if (t->Type() == Thinker::tt_ppawn)
	{ // Squish the player
	  ((PlayerPawn *)t)->player->deltaviewheight = -16*FRACUNIT;
	}
    }
  else
    {
      DActor *mo = actor->SpawnMissile(actor->target, MT_MNTRFX2);
      if(mo != NULL)
	{
	  S_StartSound(mo, SFX_MAULATOR_HAMMER_HIT);
	}
    }
  if(P_Random() < 192 && actor->special2 == 0)
    {
      actor->SetState(S_XMNTR_ATK3_4);
      actor->special2 = 1;
    }
}

//----------------------------------------------------------------------------
//
// PROC A_MntrFloorFire
//
//----------------------------------------------------------------------------
/*
void A_MntrFloorFire(DActor *actor)
{
  DActor *mo;

  actor->z = actor->floorz;
  mo = actor->mp->SpawnDActor(actor->x+((P_Random()-P_Random())<<10),
		   actor->y+((P_Random()-P_Random())<<10), ONFLOORZ, MT_MNTRFX3);
  mo->target = actor->target;
  mo->px = 1; // Force block checking
  mo->CheckMissileSpawn();
}
*/

//----------------------------------------------------------------------------
//
// PROC A_HexScream
//
//----------------------------------------------------------------------------

void A_HexScream(DActor *actor)
{
  int sound = 0, i = 0;

  S.Stop3DSound(actor);

  /*
  // Handle the different player death screams
  if (actor->pz <= -39*FRACUNIT)
    sound = SFX_PLAYER_FALLING_SPLAT; // Falling splat
  */

  if (actor->type == MT_PLAYER_FIGHTER ||
      actor->type == MT_PLAYER_CLERIC  ||
      actor->type == MT_PLAYER_MAGE)
    {
      if (actor->health > -50)
	i = 0;
      else if(actor->health > -100)
	i = 1; // Crazy death sound
      else
	i = 2 + P_Random() % 3; // Three different extreme deaths
    }

  switch (actor->type)
    {
    case MT_PLAYER_FIGHTER:
      sound = SFX_PLAYER_FIGHTER_NORMAL_DEATH + i;
      break;
    case MT_PLAYER_CLERIC:
      sound = SFX_PLAYER_CLERIC_NORMAL_DEATH + i;
      break;
    case MT_PLAYER_MAGE:
      sound = SFX_PLAYER_MAGE_NORMAL_DEATH + i;
      break;
    default:
      sound = actor->info->deathsound;
    }

  S_StartSound(actor, sound);
}

//---------------------------------------------------------------------------
//
// PROC P_DropItem
//
//---------------------------------------------------------------------------

/*
void P_DropItem(DActor *source, mobjtype_t type, int special, int chance)
{
	DActor *mo;

	if(P_Random() > chance)
	{
		return;
	}
	mo = P_SpawnMobj(source->x, source->y,
		source->z+(source->height>>1), type);
	mo->px = (P_Random()-P_Random())<<8;
	mo->py = (P_Random()-P_Random())<<8;
	mo->pz = FRACUNIT*5+(P_Random()<<10);
	mo->flags2 |= MF2_DROPPED;
	mo->health = special;
}
*/



//============================================================================
//
// A_SerpentUnHide
//
//============================================================================

void A_SerpentUnHide(DActor *actor)
{
  actor->flags2 &= ~MF2_DONTDRAW;
  actor->floorclip = 24*FRACUNIT;
}

//============================================================================
//
// A_SerpentHide
//
//============================================================================

void A_SerpentHide(DActor *actor)
{
  actor->flags2 |= MF2_DONTDRAW;
  actor->floorclip = 0;
}
//============================================================================
//
// A_SerpentChase
//
//============================================================================

void A_SerpentChase(DActor *actor)
{
  int delta;
  int oldX, oldY, oldFloor;

  if(actor->reactiontime)
    {
      actor->reactiontime--;
    }

  // Modify target threshold
  if(actor->threshold)
    {
      actor->threshold--;
    }

  if (cv_fastmonsters.value)
    { // Monsters move faster in nightmare mode
      actor->tics -= actor->tics/2;
      if(actor->tics < 3)
	{
	  actor->tics = 3;
	}
    }

  //
  // turn towards movement direction if not there yet
  //
  if(actor->movedir < 8)
    {
      actor->angle &= (7<<29);
      delta = actor->angle-(actor->movedir << 29);
      if(delta > 0)
	{
	  actor->angle -= ANG90/2;
	}
      else if(delta < 0)
	{
	  actor->angle += ANG90/2;
	}
    }

  if(!actor->target || !(actor->target->flags&MF_SHOOTABLE))
    { // look for a new target
      if(actor->LookForPlayers(true))
	{ // got a new target
	  return;
	}
      actor->SetState(actor->info->spawnstate);
      return;
    }

  //
  // don't attack twice in a row
  //
  if (actor->eflags & MFE_JUSTATTACKED)
    {
      actor->eflags &= ~MFE_JUSTATTACKED;
      if (!cv_fastmonsters.value)
	actor->P_NewChaseDir();
      return;
    }

  //
  // check for melee attack
  //
  if (actor->info->meleestate && actor->CheckMeleeRange())
    {
      if(actor->info->attacksound)
	{
	  S_StartSound (actor, actor->info->attacksound);
	}
      actor->SetState(actor->info->meleestate);
      return;
    }

  //
  // possibly choose another target
  //
  if (!actor->threshold && !actor->mp->CheckSight (actor, actor->target) )
    {
      if (actor->LookForPlayers(true))
	return;         // got a new target
    }

  //
  // chase towards player
  //
  oldX = actor->x;
  oldY = actor->y;
  oldFloor = actor->subsector->sector->floorpic;
  if (--actor->movecount<0 || !actor->P_Move())
    {
      actor->P_NewChaseDir();
    }
  if(actor->subsector->sector->floorpic != oldFloor)
    {
      actor->TryMove(oldX, oldY, true);
      actor->P_NewChaseDir();
    }

  //
  // make active sound
  //
  if(actor->info->activesound && P_Random() < 3)
    {
      S_StartSound(actor, actor->info->activesound);
    }
}

//============================================================================
//
// A_SerpentRaiseHump
// 
// Raises the hump above the surface by raising the floorclip level
//============================================================================

void A_SerpentRaiseHump(DActor *actor)
{
  actor->floorclip -= 4*FRACUNIT;
}

//============================================================================
//
// A_SerpentLowerHump
// 
//============================================================================

void A_SerpentLowerHump(DActor *actor)
{
  actor->floorclip += 4*FRACUNIT;
}

//============================================================================
//
// A_SerpentHumpDecide
//
//		Decided whether to hump up, or if the mobj is a serpent leader, 
//			to missile attack
//============================================================================

void A_SerpentHumpDecide(DActor *actor)
{
  if(actor->type == MT_SERPENTLEADER)
    {
      if(P_Random() > 30)
	{
	  return;
	}
      else if(P_Random() < 40)
	{ // Missile attack
	  actor->SetState(S_SERPENT_SURFACE1);
	  return;
	}
    }
  else if(P_Random() > 3)
    {
      return;
    }
  if(!actor->CheckMeleeRange())
    { // The hump shouldn't occur when within melee range
      if(actor->type == MT_SERPENTLEADER && P_Random() < 128)
	{
	  actor->SetState(S_SERPENT_SURFACE1);
	}
      else
	{	
	  actor->SetState(S_SERPENT_HUMP1);
	  S_StartSound(actor, SFX_SERPENT_ACTIVE);
	}
    }
}

//============================================================================
//
// A_SerpentBirthScream
//
//============================================================================

void A_SerpentBirthScream(DActor *actor)
{
  S_StartSound(actor, SFX_SERPENT_BIRTH);
}

//============================================================================
//
// A_SerpentDiveSound
//
//============================================================================

void A_SerpentDiveSound(DActor *actor)
{
  S_StartSound(actor, SFX_SERPENT_ACTIVE);
}

//============================================================================
//
// A_SerpentWalk
//
// Similar to A_Chase, only has a hardcoded entering of meleestate
//============================================================================

void A_SerpentWalk(DActor *actor)
{
  int delta;

  if(actor->reactiontime)
    {
      actor->reactiontime--;
    }

  // Modify target threshold
  if(actor->threshold)
    {
      actor->threshold--;
    }

  if (cv_fastmonsters.value)
    { // Monsters move faster in nightmare mode
      actor->tics -= actor->tics/2;
      if(actor->tics < 3)
	{
	  actor->tics = 3;
	}
    }

  //
  // turn towards movement direction if not there yet
  //
  if(actor->movedir < 8)
    {
      actor->angle &= (7<<29);
      delta = actor->angle-(actor->movedir << 29);
      if(delta > 0)
	{
	  actor->angle -= ANG90/2;
	}
      else if(delta < 0)
	{
	  actor->angle += ANG90/2;
	}
    }

  if(!actor->target || !(actor->target->flags&MF_SHOOTABLE))
    { // look for a new target
      if(actor->LookForPlayers(true))
	{ // got a new target
	  return;
	}
      actor->SetState(actor->info->spawnstate);
      return;
    }

  //
  // don't attack twice in a row
  //
  if (actor->eflags & MFE_JUSTATTACKED)
    {
      actor->eflags &= ~MFE_JUSTATTACKED;
      if (!cv_fastmonsters.value)
	actor->P_NewChaseDir();
      return;
    }

  //
  // check for melee attack
  //
  if (actor->info->meleestate && actor->CheckMeleeRange())
    {
      if (actor->info->attacksound)
	{
	  S_StartSound (actor, actor->info->attacksound);
	}
      actor->SetState(S_SERPENT_ATK1);
      return;
    }
  //
  // possibly choose another target
  //
  if (!actor->threshold && !actor->mp->CheckSight (actor, actor->target))
    {
      if (actor->LookForPlayers(true))
	return;         // got a new target
    }

  //
  // chase towards player
  //
  if (--actor->movecount<0 || !actor->P_Move())
    {
      actor->P_NewChaseDir();
    }
}

//============================================================================
//
// A_SerpentCheckForAttack
//
//============================================================================

void A_SerpentCheckForAttack(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  if(actor->type == MT_SERPENTLEADER)
    {
      if(!actor->CheckMeleeRange())
	{
	  actor->SetState(S_SERPENT_ATK1);
	  return;
	}
    }
  if(P_CheckMeleeRange2(actor))
    {
      actor->SetState(S_SERPENT_WALK1);
    }
  else if(actor->CheckMeleeRange())
    {
      if(P_Random() < 32)
	{
	  actor->SetState(S_SERPENT_WALK1);
	}
      else
	{
	  actor->SetState(S_SERPENT_ATK1);
	}
    }
}

//============================================================================
//
// A_SerpentChooseAttack
//
//============================================================================

void A_SerpentChooseAttack(DActor *actor)
{
  if(!actor->target || actor->CheckMeleeRange())
    {
      return;
    }
  if(actor->type == MT_SERPENTLEADER)
    {
      actor->SetState(S_SERPENT_MISSILE1);
    }
}
	
//============================================================================
//
// A_SerpentMeleeAttack
//
//============================================================================

void A_SerpentMeleeAttack(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(5));
      S_StartSound(actor, SFX_SERPENT_MELEEHIT);
    }
  if(P_Random() < 96)
    {
      A_SerpentCheckForAttack(actor);
    }
}

//============================================================================
//
// A_SerpentMissileAttack
//
//============================================================================
	
void A_SerpentMissileAttack(DActor *actor)
{
  DActor *mo;

  if(!actor->target)
    {
      return;
    }
  mo = actor->SpawnMissile(actor->target, MT_SERPENTFX);
}

//============================================================================
//
// A_SerpentHeadPop
//
//============================================================================

void A_SerpentHeadPop(DActor *actor)
{
  actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, MT_SERPENT_HEAD);
}

//============================================================================
//
// A_SerpentSpawnGibs
//
//============================================================================

void A_SerpentSpawnGibs(DActor *actor)
{
  DActor *mo;

  mo = actor->mp->SpawnDActor(actor->x+((P_Random()-128)<<12), 
		   actor->y+((P_Random()-128)<<12), actor->floorz+FRACUNIT,
		   MT_SERPENT_GIB1);	
  if(mo)
    {
      mo->px = (P_Random()-128)<<6;
      mo->py = (P_Random()-128)<<6;
      mo->floorclip = 6*FRACUNIT;
    }
  mo = actor->mp->SpawnDActor(actor->x+((P_Random()-128)<<12), 
		   actor->y+((P_Random()-128)<<12), actor->floorz+FRACUNIT,
		   MT_SERPENT_GIB2);	
  if(mo)
    {
      mo->px = (P_Random()-128)<<6;
      mo->py = (P_Random()-128)<<6;
      mo->floorclip = 6*FRACUNIT;
    }
  mo = actor->mp->SpawnDActor(actor->x+((P_Random()-128)<<12), 
		   actor->y+((P_Random()-128)<<12), actor->floorz+FRACUNIT,
		   MT_SERPENT_GIB3);	
  if(mo)
    {
      mo->px = (P_Random()-128)<<6;
      mo->py = (P_Random()-128)<<6;
      mo->floorclip = 6*FRACUNIT;
    }
}

//============================================================================
//
// A_FloatGib
//
//============================================================================

void A_FloatGib(DActor *actor)
{
  actor->floorclip -= FRACUNIT;
}

//============================================================================
//
// A_SinkGib
//
//============================================================================

void A_SinkGib(DActor *actor)
{
  actor->floorclip += FRACUNIT;
}

//============================================================================
//
// A_DelayGib
//
//============================================================================

void A_DelayGib(DActor *actor)
{
  actor->tics -= P_Random()>>2;
}

//============================================================================
//
// A_SerpentHeadCheck
//
//============================================================================

void A_SerpentHeadCheck(DActor *actor)
{
  if (actor->z <= actor->floorz)
    {
      if (actor->subsector->sector->floortype >= FLOOR_LIQUID)
	{
	  actor->HitFloor();
	  actor->SetState(S_NULL);
	}
      else
	{
	  actor->SetState(S_SERPENT_HEAD_X1);
	}
    }
}

//============================================================================
//
// A_CentaurAttack
//
//============================================================================

void A_CentaurAttack(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, P_Random()%7+3);
    }
}

//============================================================================
//
// A_CentaurAttack2
//
//============================================================================

void A_CentaurAttack2(DActor *actor)
{
  if(!actor->target)
    {
      return;
    }
  actor->SpawnMissile(actor->target, MT_CENTAUR_FX);
  S_StartSound(actor, SFX_CENTAURLEADER_ATTACK);
}

//============================================================================
//
// A_CentaurDropStuff
//
// 	Spawn shield/sword sprites when the centaur pulps //============================================================================

void A_CentaurDropStuff(DActor *actor)
{
  DActor *mo;
  angle_t angle;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_CENTAUR_SHIELD);
  if(mo)
    {
      angle = actor->angle+ANG90;
      mo->pz = FRACUNIT*8+(P_Random()<<10);
      mo->px = FixedMul(((P_Random()-128)<<11)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul(((P_Random()-128)<<11)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_CENTAUR_SWORD);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = FRACUNIT*8+(P_Random()<<10);
      mo->px = FixedMul(((P_Random()-128)<<11)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul(((P_Random()-128)<<11)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
}

//============================================================================
//
// A_CentaurDefend
//
//============================================================================

void A_CentaurDefend(DActor *actor)
{
  A_FaceTarget(actor);
  if(actor->CheckMeleeRange() && P_Random() < 32)
    {
      A_UnSetInvulnerable(actor);
      actor->SetState(actor->info->meleestate);
    }
}

//============================================================================
//
// A_BishopAttack
//
//============================================================================

void A_BishopAttack(DActor *actor)
{
  if (!actor->target)
      return;

  S_StartSound(actor, actor->info->attacksound);
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(4));
      return;
    }
  actor->special1 = (P_Random()&3)+5;
}

//============================================================================
//
// A_BishopAttack2
//
//		Spawns one of a string of bishop missiles
//============================================================================

void A_BishopAttack2(DActor *actor)
{
  DActor *mo;

  if (!actor->target || !actor->special1)
    {
      actor->special1 = 0;
      actor->SetState(S_BISHOP_WALK1);
      return;
    }
  mo = actor->SpawnMissile(actor->target, MT_BISH_FX);
  if (mo)
    {
      mo->target = actor->target;
      mo->special2 = 16; // High word == x/y, Low word == z
    }
  actor->special1--;
}

//============================================================================
//
// A_BishopMissileWeave
//
//============================================================================

void A_BishopMissileWeave(DActor *actor)
{
  fixed_t newX, newY;
  int weaveXY, weaveZ;
  int angle;

  weaveXY = actor->special2>>16;
  weaveZ = actor->special2&0xFFFF;
  angle = (actor->angle+ANG90)>>ANGLETOFINESHIFT;
  newX = actor->x-FixedMul(finecosine[angle], 
			   FloatBobOffsets[weaveXY]<<1);
  newY = actor->y-FixedMul(finesine[angle],
			   FloatBobOffsets[weaveXY]<<1);
  weaveXY = (weaveXY+2)&63;
  newX += FixedMul(finecosine[angle], 
		   FloatBobOffsets[weaveXY]<<1);
  newY += FixedMul(finesine[angle], 
		   FloatBobOffsets[weaveXY]<<1);
  actor->TryMove(newX, newY, true);
  actor->z -= FloatBobOffsets[weaveZ];
  weaveZ = (weaveZ+2)&63;
  actor->z += FloatBobOffsets[weaveZ];	
  actor->special2 = weaveZ+(weaveXY<<16);
}

//============================================================================
//
// A_BishopMissileSeek
//
//============================================================================

void A_BishopMissileSeek(DActor *actor)
{
  actor->SeekerMissile(ANGLE_1*2, ANGLE_1*3);
}

//============================================================================
//
// A_BishopDecide
//
//============================================================================

void A_BishopDecide(DActor *actor)
{
  if(P_Random() < 220)
    {
      return;
    }
  else
    {
      actor->SetState(S_BISHOP_BLUR1);
    }		
}

//============================================================================
//
// A_BishopDoBlur
//
//============================================================================

void A_BishopDoBlur(DActor *actor)
{
  actor->special1 = (P_Random()&3)+3; // Random number of blurs
  if(P_Random() < 120)
    {
      actor->Thrust(actor->angle+ANG90, 11*FRACUNIT);
    }
  else if(P_Random() > 125)
    {
      actor->Thrust(actor->angle-ANG90, 11*FRACUNIT);
    }
  else
    { // Thrust forward
      actor->Thrust(actor->angle, 11*FRACUNIT);
    }
  S_StartSound(actor, SFX_BISHOP_BLUR);
}

//============================================================================
//
// A_BishopSpawnBlur
//
//============================================================================

void A_BishopSpawnBlur(DActor *actor)
{
  DActor *mo;

  if(!--actor->special1)
    {
      actor->px = 0;
      actor->py = 0;
      if(P_Random() > 96)
	{
	  actor->SetState(S_BISHOP_WALK1);
	}
      else
	{
	  actor->SetState(S_BISHOP_ATK1);
	}
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_BISHOPBLUR);
  if(mo)
    {
      mo->angle = actor->angle;
    }
}

//============================================================================
//
// A_BishopChase
//
//============================================================================

void A_BishopChase(DActor *actor)
{
  actor->z -= FloatBobOffsets[actor->special2]>>1;
  actor->special2 = (actor->special2+4)&63;
  actor->z += FloatBobOffsets[actor->special2]>>1;
}

//============================================================================
//
// A_BishopPuff
//
//============================================================================

void A_BishopPuff(DActor *actor)
{
  DActor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+40*FRACUNIT, 	
		   MT_BISHOP_PUFF);
  if(mo)
    {
      mo->pz = FRACUNIT/2;
    }
}

//============================================================================
//
// A_BishopPainBlur
//
//============================================================================

void A_BishopPainBlur(DActor *actor)
{
  DActor *mo;

  if(P_Random() < 64)
    {
      actor->SetState(S_BISHOP_BLUR1);
      return;
    }
  mo = actor->mp->SpawnDActor(actor->x+((P_Random()-P_Random())<<12), actor->y
		   +((P_Random()-P_Random())<<12), actor->z+((P_Random()-P_Random())<<11),
		   MT_BISHOPPAINBLUR);
  if(mo)
    {
      mo->angle = actor->angle;
    }
}

//============================================================================
// Dragon variables
//   owner    TODO misuse. Used to denote the next pathnode.
//   target   normal usage
//
//============================================================================

static void DragonSeek(DActor *actor, angle_t thresh, angle_t turnMax)
{
  int i;
  Actor *t = actor->owner;
  if (!t)
    return;

  angle_t delta;
  int dir = P_FaceMobj(actor, t, &delta);
  if (delta > thresh)
    {
      delta >>= 1;
      if (delta > turnMax)
	delta = turnMax;
    }

  if (dir)
    actor->angle += delta; // Turn clockwise
  else
    actor->angle -= delta; // Turn counter clockwise

  angle_t an = actor->angle >> ANGLETOFINESHIFT;
  actor->px = int(actor->info->speed * finecosine[an]);
  actor->py = int(actor->info->speed * finesine[an]);

  int dist;
  if (actor->z + actor->height < t->z || t->z + t->height < actor->z)
    {
      dist = P_AproxDistance(t->x - actor->x, t->y - actor->y);
      dist = dist/int(actor->info->speed * FRACUNIT);
      if (dist < 1)
	dist = 1;

      actor->pz = (t->z-actor->z)/dist;
    }
  else
    {
      dist = P_AproxDistance(t->x-actor->x, t->y-actor->y);
      dist = dist/int(actor->info->speed * FRACUNIT);
    }

  if (t->flags & MF_SHOOTABLE && P_Random() < 64)
    { // attack the destination mobj if it's attackable
      Actor *oldTarget;
	
      if (abs(actor->angle - R_PointToAngle2(actor->x, actor->y, t->x, t->y)) < ANGLE_45/2)
	{
	  oldTarget = actor->target;
	  actor->target = t;
	  if (actor->CheckMeleeRange())
	    {
	      actor->target->Damage(actor, actor, HITDICE(10));
	      S_StartSound(actor, SFX_DRAGON_ATTACK);
	    }
	  else if (P_Random() < 128 && actor->CheckMissileRange())
	    {
	      actor->SpawnMissile(t, MT_DRAGON_FX);						
	      S_StartSound(actor, SFX_DRAGON_ATTACK);
	    }
	  actor->target = oldTarget;
	}
    }

  int search;
  if (dist < 4)
    { // Hit the target thing
      if (actor->target && P_Random() < 200)
	{
	  int bestArg = -1;
	  angle_t bestAngle = ANGLE_MAX;
	  angle_t angleToSpot;
	  angle_t angleToTarget = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
	  for (i = 0; i < 5; i++)
	    {
	      if (!t->args[i])
		continue;

	      search = -1;

	      Actor *mo = actor->mp->FindFromTIDmap(t->args[i], &search);
	      angleToSpot = R_PointToAngle2(actor->x, actor->y, mo->x, mo->y);
	      if (abs(angleToSpot-angleToTarget) < bestAngle)
		{
		  bestAngle = abs(angleToSpot-angleToTarget);
		  bestArg = i;
		}
	    }
	  if (bestArg != -1)
	    {
	      search = -1;
	      actor->owner = actor->mp->FindFromTIDmap(t->args[bestArg], &search);
	    }
	}
      else
	{
	  do
	    {
	      i = (P_Random()>>2)%5;
	    } while(!t->args[i]);
	  search = -1;
	  actor->owner = actor->mp->FindFromTIDmap(t->args[i], &search);
	}
    }
}

//============================================================================
//
// A_DragonInitFlight
//
//============================================================================

void A_DragonInitFlight(DActor *actor)
{
  // sets guidance to first tid identical to dragon's, removes dragon from tidmap.
  int search = -1;
  do
    { // find the first tid identical to the dragon's tid
      actor->owner = actor->mp->FindFromTIDmap(actor->tid, &search);
      if (search == -1)
	{
	  actor->SetState(actor->info->spawnstate);
	  return;
	}
    } while (actor->owner == actor);
  actor->mp->RemoveFromTIDmap(actor);
}

//============================================================================
//
// A_DragonFlight
//
//============================================================================

void A_DragonFlight(DActor *actor)
{
  DragonSeek(actor, 4*ANGLE_1, 8*ANGLE_1);
  if (actor->target)
    {
      if (!(actor->target->flags & MF_SHOOTABLE))
	{ // target died
	  actor->target = NULL;
	  return;
	}

      angle_t an = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
      if (abs(actor->angle - an) < ANGLE_45/2 && actor->CheckMeleeRange())
	{
	  actor->target->Damage(actor, actor, HITDICE(8));
	  S_StartSound(actor, SFX_DRAGON_ATTACK);
	}
      else if(abs(actor->angle - an) <= ANGLE_1*20)
	{
	  actor->SetState(actor->info->missilestate);
	  S_StartSound(actor, SFX_DRAGON_ATTACK);
	}
    }
  else
    actor->LookForPlayers(true);
}

//============================================================================
//
// A_DragonFlap
//
//============================================================================

void A_DragonFlap(DActor *actor)
{
  A_DragonFlight(actor);
  if (P_Random() < 240)
    S_StartSound(actor, SFX_DRAGON_WINGFLAP);
  else
    S_StartSound(actor, actor->info->activesound);
}

//============================================================================
//
// A_DragonAttack
//
//============================================================================

void A_DragonAttack(DActor *actor)
{
  actor->SpawnMissile(actor->target, MT_DRAGON_FX);						
}

//============================================================================
//
// A_DragonFX2
//
//============================================================================

void A_DragonFX2(DActor *actor)
{
  int delay = 16 + (P_Random() >> 3);

  for (int i = 1 + (P_Random()&3); i; i--)
    {
      DActor *mo = actor->mp->SpawnDActor(actor->x + ((P_Random()-128)<<14), 
	actor->y + ((P_Random()-128)<<14), actor->z + ((P_Random()-128)<<12), MT_DRAGON_FX2);

      if (mo)
	{
	  mo->tics = delay + (P_Random()&3)*i*2;
	  mo->owner = actor; // was actor->owner;
	  mo->target = actor->target;
	}
    } 
}

//============================================================================
//
// A_DragonPain
//
//============================================================================

void A_DragonPain(DActor *actor)
{
  A_Pain(actor);
  if (!actor->owner)
    { // no destination spot yet
      actor->SetState(S_DRAGON_INIT);
    }
}

//============================================================================
//
// A_DragonCheckCrash
//
//============================================================================

void A_DragonCheckCrash(DActor *actor)
{
  if (actor->z <= actor->floorz)
    actor->SetState(S_DRAGON_CRASH1);
}

//============================================================================
// Demon AI
//============================================================================

//
// A_DemonAttack1 (melee)
//
void A_DemonAttack1(DActor *actor)
{
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(2));
    }
}


//
// A_DemonAttack2 (missile)
//
void A_DemonAttack2(DActor *actor)
{
  DActor *mo;
  mobjtype_t fireBall;

  if(actor->type == MT_DEMON)
    {
      fireBall = MT_DEMONFX1;
    }
  else
    {
      fireBall = MT_DEMON2FX1;
    }
  mo = actor->SpawnMissile(actor->target, fireBall);
  if (mo)
    {
      mo->z += 30*FRACUNIT;
      S_StartSound(actor, SFX_DEMON_MISSILE_FIRE);
    }
}

//
// A_DemonDeath
//

void A_DemonDeath(DActor *actor)
{
  DActor *mo;
  angle_t angle;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMONCHUNK1);
  if(mo)
    {
      angle = actor->angle+ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMONCHUNK2);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMONCHUNK3);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMONCHUNK4);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMONCHUNK5);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
}

//===========================================================================
//
// A_Demon2Death
//
//===========================================================================

void A_Demon2Death(DActor *actor)
{
  DActor *mo;
  angle_t angle;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMON2CHUNK1);
  if(mo)
    {
      angle = actor->angle+ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMON2CHUNK2);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMON2CHUNK3);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMON2CHUNK4);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+45*FRACUNIT, 
		   MT_DEMON2CHUNK5);
  if(mo)
    {
      angle = actor->angle-ANG90;
      mo->pz = 8*FRACUNIT;
      mo->px = FixedMul((P_Random()<<10)+FRACUNIT,
			  finecosine[angle>>ANGLETOFINESHIFT]);
      mo->py = FixedMul((P_Random()<<10)+FRACUNIT, 
			  finesine[angle>>ANGLETOFINESHIFT]);
      mo->target = actor;
    }
}



//
// A_SinkMobj
// Sink a mobj incrementally into the floor
//

bool A_SinkMobj(DActor *actor)
{
  if (actor->floorclip <  actor->info->height)
    {
      switch(actor->type)
	{
	case MT_THRUSTFLOOR_DOWN:
	case MT_THRUSTFLOOR_UP:
	  actor->floorclip += 6*FRACUNIT;
	  break;
	default:
	  actor->floorclip += FRACUNIT;
	  break;
	}
      return false;
    }
  return true;
}

//
// A_RaiseMobj
// Raise a mobj incrementally from the floor to 
// 

bool A_RaiseMobj(DActor *actor)
{
  int done = true;

  // Raise a mobj from the ground
  if (actor->floorclip > 0)
    {
      switch(actor->type)
	{
	case MT_WRAITHB:
	  actor->floorclip -= 2*FRACUNIT;
	  break;
	case MT_THRUSTFLOOR_DOWN:
	case MT_THRUSTFLOOR_UP:
	  actor->floorclip -= actor->special2*FRACUNIT;
	  break;
	default:
	  actor->floorclip -= 2*FRACUNIT;
	  break;
	}
      if (actor->floorclip <= 0)
	{
	  actor->floorclip = 0;
	  done=true;
	}
      else
	{
	  done = false;
	}
    }
  return done;		// Reached target height
}


//============================================================================
// Wraith Variables
//
//	special1				Internal index into floatbob
//	special2
//============================================================================

//
// A_WraithInit
//

void A_WraithInit(DActor *actor)
{
  actor->z += 48<<FRACBITS;
  actor->special1 = 0;			// index into floatbob
}

void A_WraithRaiseInit(DActor *actor)
{
  actor->flags2 &= ~MF2_DONTDRAW;
  actor->flags2 &= ~MF2_NONSHOOTABLE;
  actor->flags |= MF_SHOOTABLE|MF_SOLID;
  actor->floorclip = actor->info->height;
}

void A_WraithRaise(DActor *actor)
{
  if (A_RaiseMobj(actor))
    {
      // Reached it's target height
      actor->SetState(S_WRAITH_CHASE1);
    }

  P_SpawnDirt(actor, actor->radius);
}


void A_WraithMelee(DActor *actor)
{
  int amount;

  // Steal health from target and give to player
  if(actor->CheckMeleeRange() && (P_Random()<220))
    {
      amount = HITDICE(2);
      actor->target->Damage(actor, actor, amount);
      actor->health += amount;
    }
}

void A_WraithMissile(DActor *actor)
{
  DActor *mo;

  mo = actor->SpawnMissile(actor->target, MT_WRAITHFX1);
  if (mo)
    {
      S_StartSound(actor, SFX_WRAITH_MISSILE_FIRE);
    }
}


//
// A_WraithFX2 - spawns sparkle tail of missile
//

void A_WraithFX2(DActor *actor)
{
  DActor *mo;
  angle_t angle;
  int i;

  for (i=0; i<2; i++)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_WRAITHFX2);
      if(mo)
	{
	  if (P_Random()<128)
	    {
	      angle = actor->angle+(P_Random()<<22);
	    }
	  else
	    {
	      angle = actor->angle-(P_Random()<<22);
	    }
	  mo->pz = 0;
	  mo->px = FixedMul((P_Random()<<7)+FRACUNIT,
			      finecosine[angle>>ANGLETOFINESHIFT]);
	  mo->py = FixedMul((P_Random()<<7)+FRACUNIT, 
			      finesine[angle>>ANGLETOFINESHIFT]);
	  mo->target = actor;
	  mo->floorclip = 10*FRACUNIT;
	}
    }
}


// Spawn an FX3 around the actor during attacks
void A_WraithFX3(DActor *actor)
{
  DActor *mo;
  int numdropped=P_Random()%15;
  int i;

  for (i=0; i<numdropped; i++)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_WRAITHFX3);
      if(mo)
	{
	  mo->x += (P_Random()-128)<<11;
	  mo->y += (P_Random()-128)<<11;
	  mo->z += (P_Random()<<10);
	  mo->target = actor;
	}
    }
}

// Spawn an FX4 during movement
void A_WraithFX4(DActor *actor)
{
  DActor *mo;
  int chance = P_Random();
  int spawn4,spawn5;

  if (chance < 10)
    {
      spawn4 = true;
      spawn5 = false;
    }
  else if (chance < 20)
    {
      spawn4 = false;
      spawn5 = true;
    }
  else if (chance < 25)
    {
      spawn4 = true;
      spawn5 = true;
    }
  else
    {
      spawn4 = false;
      spawn5 = false;
    }

  if (spawn4)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_WRAITHFX4);
      if(mo)
	{
	  mo->x += (P_Random()-128)<<12;
	  mo->y += (P_Random()-128)<<12;
	  mo->z += (P_Random()<<10);
	  mo->target = actor;
	}
    }
  if (spawn5)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_WRAITHFX5);
      if(mo)
	{
	  mo->x += (P_Random()-128)<<11;
	  mo->y += (P_Random()-128)<<11;
	  mo->z += (P_Random()<<10);
	  mo->target = actor;
	}
    }
}


void A_WraithLook(DActor *actor)
{
  //	A_WraithFX4(actor);		// too expensive
  A_Look(actor);
}


void A_WraithChase(DActor *actor)
{
  int weaveindex = actor->special1;
  actor->z += FloatBobOffsets[weaveindex];
  actor->special1 = (weaveindex+2)&63;
  //	if (actor->floorclip > 0)
  //	{
  //		actor->SetState(S_WRAITH_RAISE2);
  //		return;
  //	}
  A_Chase(actor);
  A_WraithFX4(actor);
}



//============================================================================
// Ettin AI
//============================================================================

void A_EttinAttack(DActor *actor)
{
  if(actor->CheckMeleeRange())
    {
      actor->target->Damage(actor, actor, HITDICE(2));
    }
}


void A_DropMace(DActor *actor)
{
  DActor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y,
		   actor->z+(actor->height>>1), MT_ETTIN_MACE);
  if (mo)
    {
      mo->px = (P_Random()-128)<<11;
      mo->py = (P_Random()-128)<<11;
      mo->pz = FRACUNIT*10+(P_Random()<<10);
      mo->target = actor;
    }
}


//============================================================================
// Fire Demon AI
//
// special1			index into floatbob
// special2			whether strafing or not
//============================================================================

void A_FiredSpawnRock(DActor *actor)
{
  DActor *mo;
  int x,y,z;
  mobjtype_t rtype;

  switch(P_Random()%5)
    {
    case 0:
      rtype = MT_FIREDEMON_FX1;
      break;
    case 1:
      rtype = MT_FIREDEMON_FX2;
      break;
    case 2:
      rtype = MT_FIREDEMON_FX3;
      break;
    case 3:
      rtype = MT_FIREDEMON_FX4;
      break;
    default:
      rtype = MT_FIREDEMON_FX5;
      break;
    }

  x = actor->x + ((P_Random()-128) << 12);
  y = actor->y + ((P_Random()-128) << 12);
  z = actor->z + ((P_Random()) << 11);
  mo = actor->mp->SpawnDActor(x,y,z,rtype);
  if (mo)
    {
      mo->target = actor;
      mo->px = (P_Random()-128)<<10;
      mo->py = (P_Random()-128)<<10;
      mo->pz = (P_Random()<<10);
      mo->special1 = 2;		// Number bounces
    }

  // Initialize fire demon
  actor->special2 = 0;
  actor->eflags &= ~MFE_JUSTATTACKED;
}

void A_FiredRocks(DActor *actor)
{
  A_FiredSpawnRock(actor);
  A_FiredSpawnRock(actor);
  A_FiredSpawnRock(actor);
  A_FiredSpawnRock(actor);
  A_FiredSpawnRock(actor);
}

void A_FiredAttack(DActor *actor)
{
  DActor *mo = actor->SpawnMissile(actor->target, MT_FIREDEMON_FX6);
  if (mo)
    S_StartSound(actor, SFX_FIRED_ATTACK);
}

void A_SmBounce(DActor *actor)
{
  // give some more momentum (x,y,&z)
  actor->z = actor->floorz + FRACUNIT;
  actor->pz = (2*FRACUNIT) + (P_Random()<<10);
  actor->px = P_Random()%3<<FRACBITS;
  actor->py = P_Random()%3<<FRACBITS;
}


#define FIREDEMON_ATTACK_RANGE	64*8*FRACUNIT

void A_FiredChase(DActor *actor)
{
  int weaveindex = actor->special1;
  Actor *t = actor->target;
  angle_t ang;
  fixed_t dist;

  if(actor->reactiontime) actor->reactiontime--;
  if(actor->threshold) actor->threshold--;

  // Float up and down
  actor->z += FloatBobOffsets[weaveindex];
  actor->special1 = (weaveindex+2)&63;

  // Insure it stays above certain height
  if (actor->z < actor->floorz + (64*FRACUNIT))
    {
      actor->z += 2*FRACUNIT;
    }

  if(!t || !(t->flags&MF_SHOOTABLE))
    {	// Invalid target
      actor->LookForPlayers(true);
      return;
    }

  // Strafe
  if (actor->special2 > 0)
    {
      actor->special2--;
    }
  else
    {
      actor->special2 = 0;
      actor->px = actor->py = 0;
      dist = P_AproxDistance(actor->x - t->x, actor->y - t->y);
      if (dist < FIREDEMON_ATTACK_RANGE)
	{
	  if (P_Random()<30)
	    {
	      ang = R_PointToAngle2(actor->x, actor->y, t->x, t->y);
	      if (P_Random()<128)
		ang += ANGLE_90;
	      else
		ang -= ANGLE_90;
	      ang>>=ANGLETOFINESHIFT;
	      actor->px = FixedMul(8*FRACUNIT, finecosine[ang]);
	      actor->py = FixedMul(8*FRACUNIT, finesine[ang]);
	      actor->special2 = 3;		// strafe time
	    }
	}
    }

  FaceMovementDirection(actor);

  // Normal movement
  if (!actor->special2)
    {
      if (--actor->movecount<0 || !actor->P_Move())
	{
	  actor->P_NewChaseDir();
	}
    }

  // Do missile attack
  if (!(actor->eflags&MFE_JUSTATTACKED))
    {
      if (actor->CheckMissileRange() && (P_Random()<20))
	{
	  actor->SetState(actor->info->missilestate);
	  actor->eflags |= MFE_JUSTATTACKED;
	  return;
	}
    }
  else
    {
      actor->eflags &= ~MFE_JUSTATTACKED;
    }

  // make active sound
  if(actor->info->activesound && P_Random() < 3)
    {
      S_StartSound(actor, actor->info->activesound);
    }
}

void A_FiredSplotch(DActor *actor)
{
  DActor *mo;

  mo = actor->mp->SpawnDActor(actor->x,actor->y,actor->z, MT_FIREDEMON_SPLOTCH1);
  if (mo)
    {
      mo->px = (P_Random()-128)<<11;
      mo->py = (P_Random()-128)<<11;
      mo->pz = FRACUNIT*3 + (P_Random()<<10);
    }
  mo = actor->mp->SpawnDActor(actor->x,actor->y,actor->z, MT_FIREDEMON_SPLOTCH2);
  if (mo)
    {
      mo->px = (P_Random()-128)<<11;
      mo->py = (P_Random()-128)<<11;
      mo->pz = FRACUNIT*3 + (P_Random()<<10);
    }
}


//============================================================================
//
// A_IceGuyLook
//
//============================================================================

void A_IceGuyLook(DActor *actor)
{
  fixed_t dist;
  fixed_t an;

  A_Look(actor);
  if(P_Random() < 64)
    {
      dist = ((P_Random()-128)*actor->radius)>>7;
      an = (actor->angle+ANG90)>>ANGLETOFINESHIFT;

      actor->mp->SpawnDActor(actor->x+FixedMul(dist, finecosine[an]),
		  actor->y+FixedMul(dist, finesine[an]), actor->z+60*FRACUNIT,
	          mobjtype_t(MT_ICEGUY_WISP1 + (P_Random() & 1)));
    }
}

//============================================================================
//
// A_IceGuyChase
//
//============================================================================

void A_IceGuyChase(DActor *actor)
{
  fixed_t dist;
  fixed_t an;
  DActor *mo;

  A_Chase(actor);
  if(P_Random() < 128)
    {
      dist = ((P_Random()-128)*actor->radius)>>7;
      an = (actor->angle+ANG90)>>ANGLETOFINESHIFT;

      mo = actor->mp->SpawnDActor(actor->x+FixedMul(dist, finecosine[an]),
		       actor->y+FixedMul(dist, finesine[an]), actor->z+60*FRACUNIT,
		       mobjtype_t(MT_ICEGUY_WISP1 + (P_Random()&1)));
      if(mo)
	{
	  mo->px = actor->px;
	  mo->py = actor->py;
	  mo->pz = actor->pz;
	  mo->target = actor;
	}
    }
}

//============================================================================
//
// A_IceGuyAttack
//
//============================================================================

static DActor *P_SpawnMissileXYZ(fixed_t x, fixed_t y, fixed_t z,
				 Actor *source, Actor *dest, mobjtype_t type)
{
  angle_t an;
  int dist;

  z -= source->floorclip;
  DActor *th = source->mp->SpawnDActor(x, y, z, type);
  if (th->info->seesound)
    {
      S_StartSound(th, th->info->seesound);
    }
  th->owner = source; // Originator
  an = R_PointToAngle2(source->x, source->y, dest->x, dest->y);
  if (dest->flags & MF_SHADOW)
    { // Invisible target
      an += (P_Random()-P_Random())<<21;
    }
  th->angle = an;
  an >>= ANGLETOFINESHIFT;
  th->px = int(th->info->speed * finecosine[an]);
  th->py = int(th->info->speed * finesine[an]);
  dist = P_AproxDistance(dest->x - source->x, dest->y - source->y);
  dist = dist/int(th->info->speed * FRACUNIT);
  if(dist < 1)
    {
      dist = 1;
    }
  th->pz = (dest->z-source->z)/dist;
  return (th->CheckMissileSpawn() ? th : NULL);
}

void A_IceGuyAttack(DActor *actor)
{
  if (!actor->target) 
    return;

  fixed_t an = (actor->angle+ANG90)>>ANGLETOFINESHIFT;
  P_SpawnMissileXYZ(actor->x + FixedMul(actor->radius>>1, finecosine[an]),
		    actor->y + FixedMul(actor->radius>>1, finesine[an]),
		    actor->z + 40*FRACUNIT, actor, actor->target, MT_ICEGUY_FX);
  an = (actor->angle-ANG90)>>ANGLETOFINESHIFT;
  P_SpawnMissileXYZ(actor->x + FixedMul(actor->radius>>1, finecosine[an]),
		    actor->y + FixedMul(actor->radius>>1, finesine[an]),
		    actor->z + 40*FRACUNIT, actor, actor->target, MT_ICEGUY_FX);
  S_StartSound(actor, actor->info->attacksound);
}

//============================================================================
//
// A_IceGuyMissilePuff
//
//============================================================================

void A_IceGuyMissilePuff(DActor *actor)
{
  DActor *mo;
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+2*FRACUNIT, MT_ICEFX_PUFF);
}

//============================================================================
//
// A_IceGuyDie
//
//============================================================================

void A_IceGuyDie(DActor *actor)
{
  void A_FreezeDeathChunks(DActor *actor);

  actor->px = 0;
  actor->py = 0;
  actor->pz = 0;
  actor->height <<= 2;
  A_FreezeDeathChunks(actor);
}

//============================================================================
//
// A_IceGuyMissileExplode
//
//============================================================================

void A_IceGuyMissileExplode(DActor *actor)
{
  DActor *mo;
  int i;

  for(i = 0; i < 8; i++)
    {
      mo = actor->SpawnMissileAngle(MT_ICEGUY_FX2, i*ANG45, int(-0.3*FRACUNIT));
      if(mo)
	{
	  mo->target = actor->target;
	}
    }
}









//============================================================================
//
//	Sorcerer stuff
//
// Sorcerer Variables
//		special1		Angle of ball 1 (all others relative to that)
//		special2		which ball to stop at in stop mode (MT_???)
//		args[0]			Denfense time
//		args[1]			Number of full rotations since stopping mode
//		args[2]			Target orbit speed for acceleration/deceleration
//		args[3]			Movement mode (see SORC_ macros)
//		args[4]			Current ball orbit speed
//	Sorcerer Ball Variables
//		special1		Previous angle of ball (for woosh)
//		special2		Countdown of rapid fire (FX4)
//		args[0]			If set, don't play the bounce sound when bouncing
//============================================================================

#define SORCBALL_INITIAL_SPEED 		7
#define SORCBALL_TERMINAL_SPEED		25
#define SORCBALL_SPEED_ROTATIONS 	5
#define SORC_DEFENSE_TIME			255
#define SORC_DEFENSE_HEIGHT			45
#define BOUNCE_TIME_UNIT			(35/2)
#define SORCFX4_RAPIDFIRE_TIME		(6*3)		// 3 seconds
#define SORCFX4_SPREAD_ANGLE		20

#define SORC_DECELERATE		0
#define SORC_ACCELERATE 	1
#define SORC_STOPPING		2
#define SORC_FIRESPELL		3
#define SORC_STOPPED		4
#define SORC_NORMAL			5
#define SORC_FIRING_SPELL	6

#define BALL1_ANGLEOFFSET	0
#define BALL2_ANGLEOFFSET	(ANGLE_MAX/3)
#define BALL3_ANGLEOFFSET	((ANGLE_MAX/3)*2)

void A_SorcBallOrbit(DActor *actor);
void A_SorcSpinBalls(DActor *actor);
void A_SpeedBalls(DActor *actor);
void A_SlowBalls(DActor *actor);
void A_StopBalls(DActor *actor);
void A_AccelBalls(DActor *actor);
void A_DecelBalls(DActor *actor);
void A_SorcBossAttack(DActor *actor);
void A_SpawnFizzle(DActor *actor);
void A_CastSorcererSpell(DActor *actor);
void A_SorcUpdateBallAngle(DActor *actor);
void A_BounceCheck(DActor *actor);
void A_SorcFX1Seek(DActor *actor);
void A_SorcOffense1(DActor *actor);
void A_SorcOffense2(DActor *actor);


// Spawn spinning balls above head - actor is sorcerer
void A_SorcSpinBalls(DActor *actor)
{
  DActor *mo;
  fixed_t z;

  A_SlowBalls(actor);
  actor->args[0] = 0;									// Currently no defense
  actor->args[3] = SORC_NORMAL;
  actor->args[4] = SORCBALL_INITIAL_SPEED;		// Initial orbit speed
  actor->special1 = ANGLE_1;
  z = actor->z - actor->floorclip + actor->info->height;
	
  mo = actor->mp->SpawnDActor(actor->x, actor->y, z, MT_SORCBALL1);
  if (mo)
    {
      mo->owner = actor;
      mo->special2 = SORCFX4_RAPIDFIRE_TIME;
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, z, MT_SORCBALL2);
  if (mo) mo->owner = actor;
  mo = actor->mp->SpawnDActor(actor->x, actor->y, z, MT_SORCBALL3);
  if (mo) mo->owner = actor;
}


//
// A_SorcBallOrbit() ==========================================
//

void A_SorcBallOrbit(DActor *actor)
{
  int x,y;
  angle_t angle = 0, baseangle;
  int mode = actor->owner->args[3];
  DActor *parent = (DActor *)actor->owner;
  int dist = parent->radius - (actor->radius<<1);
  angle_t prevangle = actor->special1;
	
  if (parent->health <= 0)
    actor->SetState(actor->info->painstate);

  baseangle = parent->special1;
  switch(actor->type)
    {
    case MT_SORCBALL1:
      angle = baseangle + BALL1_ANGLEOFFSET;
      break;
    case MT_SORCBALL2:
      angle = baseangle + BALL2_ANGLEOFFSET;
      break;
    case MT_SORCBALL3:
      angle = baseangle + BALL3_ANGLEOFFSET;
      break;
    default:
      I_Error("corrupted sorcerer");
      break;
    }
  actor->angle = angle;
  angle >>= ANGLETOFINESHIFT;

  switch(mode)
    {
    case SORC_NORMAL:				// Balls rotating normally
      A_SorcUpdateBallAngle(actor);
      break;
    case SORC_DECELERATE:		// Balls decelerating
      A_DecelBalls(actor);
      A_SorcUpdateBallAngle(actor);
      break;
    case SORC_ACCELERATE:		// Balls accelerating
      A_AccelBalls(actor);
      A_SorcUpdateBallAngle(actor);
      break;
    case SORC_STOPPING:			// Balls stopping
      if ((parent->special2 == actor->type) &&
	  (parent->args[1] > SORCBALL_SPEED_ROTATIONS) &&
	  (abs(angle - (parent->angle>>ANGLETOFINESHIFT)) < (30<<5)))
	{
				// Can stop now
	  parent->args[3] = SORC_FIRESPELL;
	  parent->args[4] = 0;
				// Set angle so ball angle == sorcerer angle
	  switch(actor->type)
	    {
	    case MT_SORCBALL1:
	      parent->special1 = (int)(parent->angle -
				       BALL1_ANGLEOFFSET);
	      break;
	    case MT_SORCBALL2:
	      parent->special1 = (int)(parent->angle -
				       BALL2_ANGLEOFFSET);
	      break;
	    case MT_SORCBALL3:
	      parent->special1 = (int)(parent->angle -
				       BALL3_ANGLEOFFSET);
	      break;
	    default:
	      break;
	    }
	}
      else
	{
	  A_SorcUpdateBallAngle(actor);
	}
      break;
    case SORC_FIRESPELL:			// Casting spell
      if (parent->special2 == actor->type)
	{
				// Put sorcerer into special throw spell anim
	  if (parent->health > 0)
	    parent->SetState(S_SORC_ATTACK1, false);

	  if (actor->type==MT_SORCBALL1 && P_Random()<200)
	    {
	      S_StartAmbSound(SFX_SORCERER_SPELLCAST);
	      actor->special2 = SORCFX4_RAPIDFIRE_TIME;
	      actor->args[4] = 128;
	      parent->args[3] = SORC_FIRING_SPELL;
	    }
	  else
	    {
	      A_CastSorcererSpell(actor);
	      parent->args[3] = SORC_STOPPED;
	    }
	}
      break;
    case SORC_FIRING_SPELL:
      if (parent->special2 == actor->type)
	{
	  if (actor->special2-- <= 0)
	    {
	      // Done rapid firing 
	      parent->args[3] = SORC_STOPPED;
	      // Back to orbit balls
	      if (parent->health > 0)
		parent->SetState(S_SORC_ATTACK4, false);
	    }
	  else
	    {
	      // Do rapid fire spell
	      A_SorcOffense2(actor);
	    }
	}
      break;
    case SORC_STOPPED:			// Balls stopped
    default:
      break;
    }

  if ((angle < prevangle) && (parent->args[4]==SORCBALL_TERMINAL_SPEED))
    {
      parent->args[1]++;			// Bump rotation counter
      // Completed full rotation - make woosh sound
      S_StartSound(actor, SFX_SORCERER_BALLWOOSH);
    }
  actor->special1 = angle;		// Set previous angle
  x = parent->x + FixedMul(dist, finecosine[angle]);
  y = parent->y + FixedMul(dist, finesine[angle]);
  actor->x = x;
  actor->y = y;
  actor->z = parent->z - parent->floorclip + parent->info->height;
}


//
// Set balls to speed mode - actor is sorcerer
//
void A_SpeedBalls(DActor *actor)
{
  actor->args[3] = SORC_ACCELERATE;				// speed mode
  actor->args[2] = SORCBALL_TERMINAL_SPEED;		// target speed
}


//
// Set balls to slow mode - actor is sorcerer
//
void A_SlowBalls(DActor *actor)
{
  actor->args[3] = SORC_DECELERATE;				// slow mode
  actor->args[2] = SORCBALL_INITIAL_SPEED;		// target speed
}


//
// Instant stop when rotation gets to ball in special2
//		actor is sorcerer
//
void A_StopBalls(DActor *actor)
{
  int chance = P_Random();
  actor->args[3] = SORC_STOPPING;				// stopping mode
  actor->args[1] = 0;							// Reset rotation counter

  if ((actor->args[0] <= 0) && (chance < 200))
    {
      actor->special2 = MT_SORCBALL2;			// Blue
    }
  else if((actor->health < (actor->info->spawnhealth >> 1)) &&
	  (chance < 200))
    {
      actor->special2 = MT_SORCBALL3;			// Green
    }
  else
    {
      actor->special2 = MT_SORCBALL1;			// Yellow
    }


}


//
// Increase ball orbit speed - actor is ball
//
void A_AccelBalls(DActor *actor)
{
  DActor *sorc = (DActor *)actor->owner;

  if (sorc->args[4] < sorc->args[2])
    {
      sorc->args[4]++;
    }
  else
    {
      sorc->args[3] = SORC_NORMAL;
      if (sorc->args[4] >= SORCBALL_TERMINAL_SPEED)
	{
	  // Reached terminal velocity - stop balls
	  A_StopBalls(sorc);
	}
    }
}


// Decrease ball orbit speed - actor is ball
void A_DecelBalls(DActor *actor)
{
  Actor *sorc = actor->owner;

  if (sorc->args[4] > sorc->args[2])
    {
      sorc->args[4]--;
    }
  else
    {
      sorc->args[3] = SORC_NORMAL;
    }
}


// Update angle if first ball - actor is ball
void A_SorcUpdateBallAngle(DActor *actor)
{
  DActor *parent = (DActor *)actor->owner;
  if (actor->type == MT_SORCBALL1)
    {
      parent->special1 += ANGLE_1*parent->args[4];
    }
}


// actor is ball
void A_CastSorcererSpell(DActor *actor)
{
  DActor *mo;
  int spell = actor->type;
  angle_t ang1,ang2;
  fixed_t z;
  DActor *parent = (DActor *)actor->owner;

  S_StartAmbSound(SFX_SORCERER_SPELLCAST);

  // Put sorcerer into throw spell animation
  if (parent->health > 0) parent->SetState(S_SORC_ATTACK4, false);

  switch(spell)
    {
    case MT_SORCBALL1:				// Offensive
      A_SorcOffense1(actor);
      break;
    case MT_SORCBALL2:				// Defensive
      z = parent->z - parent->floorclip + 
	SORC_DEFENSE_HEIGHT*FRACUNIT;
      mo = actor->mp->SpawnDActor(actor->x, actor->y, z, MT_SORCFX2);
      parent->flags2 |= MF2_REFLECTIVE|MF2_INVULNERABLE;
      parent->args[0] = SORC_DEFENSE_TIME;
      if (mo) mo->owner = parent;
      break;
    case MT_SORCBALL3:				// Reinforcements
      ang1 = actor->angle - ANGLE_45;
      ang2 = actor->angle + ANGLE_45;
      if(actor->health < (actor->info->spawnhealth/3))
	{	// Spawn 2 at a time
	  mo = parent->SpawnMissileAngle(MT_SORCFX3, ang1, 4*FRACUNIT);
	  mo = parent->SpawnMissileAngle(MT_SORCFX3, ang2, 4*FRACUNIT);
	}			
      else
	{
	  if (P_Random() < 128)
	    ang1 = ang2;
	  mo = parent->SpawnMissileAngle(MT_SORCFX3, ang1, 4*FRACUNIT);
	}
      break;
    default:
      break;
    }
}

/*
void A_SpawnReinforcements(DActor *actor)
{
	DActor *parent = actor->owner;
	DActor *mo;
	angle_t ang;

	ang = ANGLE_1 * P_Random();
	mo = actor->SpawnMissileAngle(MT_SORCFX3, ang, 5*FRACUNIT);
}
*/

// actor is ball
void A_SorcOffense1(DActor *actor)
{
  DActor *mo;
  angle_t ang1,ang2;
  DActor *parent = (DActor *)actor->owner;

  ang1 = actor->angle + ANGLE_1*70;
  ang2 = actor->angle - ANGLE_1*70;
  mo = parent->SpawnMissileAngle(MT_SORCFX1, ang1, 0);
  if (mo)
    {
      mo->target = parent->target;
      mo->args[4] = BOUNCE_TIME_UNIT;
      mo->args[3] = 15;				// Bounce time in seconds
    }
  mo = parent->SpawnMissileAngle(MT_SORCFX1, ang2, 0);
  if (mo)
    {
      mo->target = parent->target;
      mo->args[4] = BOUNCE_TIME_UNIT;
      mo->args[3] = 15;				// Bounce time in seconds
    }
}


// DActor is ball
void A_SorcOffense2(DActor *actor)
{
  angle_t ang1;
  DActor *mo;
  int delta, index;
  DActor *parent = (DActor *)actor->owner;
  Actor *dest = parent->target;
  int dist;

  index = actor->args[4] << 5;
  actor->args[4] += 15;
  delta = (finesine[index])*SORCFX4_SPREAD_ANGLE;
  delta = (delta>>FRACBITS)*ANGLE_1;
  ang1 = actor->angle + delta;
  mo = parent->SpawnMissileAngle(MT_SORCFX4, ang1, 0);
  if (mo)
    {
      mo->special2 = 35*5/2;		// 5 seconds
      dist = P_AproxDistance(dest->x - mo->x, dest->y - mo->y);
      dist = dist/int(mo->info->speed * FRACUNIT);
      if(dist < 1) dist = 1;
      mo->pz = (dest->z-mo->z)/dist;
    }
}


// Resume ball spinning
void A_SorcBossAttack(DActor *actor)
{
  actor->args[3] = SORC_ACCELERATE;
  actor->args[2] = SORCBALL_INITIAL_SPEED;
}


// spell cast magic fizzle
void A_SpawnFizzle(DActor *actor)
{
  fixed_t x,y,z;
  fixed_t dist = 5*FRACUNIT;
  angle_t angle = actor->angle >> ANGLETOFINESHIFT;
  int speed = int(actor->info->speed);
  angle_t rangle;
  DActor *mo;
  int ix;

  x = actor->x + FixedMul(dist,finecosine[angle]);
  y = actor->y + FixedMul(dist,finesine[angle]);
  z = actor->z - actor->floorclip + (actor->height>>1);
  for (ix=0; ix<5; ix++)
    {
      mo = actor->mp->SpawnDActor(x,y,z,MT_SORCSPARK1);
      if (mo)
	{
	  rangle = angle + ((P_Random()%5) << 1);
	  mo->px = FixedMul(P_Random()%speed,finecosine[rangle]);
	  mo->py = FixedMul(P_Random()%speed,finesine[rangle]);
	  mo->pz = FRACUNIT*2;
	}
    }
}


//============================================================================
// Yellow spell - offense
//============================================================================

void A_SorcFX1Seek(DActor *actor)
{
  A_BounceCheck(actor);
  actor->SeekerMissile(ANGLE_1*2,ANGLE_1*6);
}


//============================================================================
// Blue spell - defense
//============================================================================
//
// FX2 Variables
//		special1		current angle
//		special2
//		args[0]		0 = CW,  1 = CCW
//		args[1]		
//============================================================================

// Split ball in two
void A_SorcFX2Split(DActor *actor)
{
  DActor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_SORCFX2);
  if (mo)
    {
      mo->owner = actor->owner;
      mo->args[0] = 0;									// CW
      mo->special1 = actor->angle;					// Set angle
      mo->SetState(S_SORCFX2_ORBIT1, false);
    }
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_SORCFX2);
  if (mo)
    {
      mo->owner = actor->owner;
      mo->args[0] = 1;									// CCW
      mo->special1 = actor->angle;					// Set angle
      mo->SetState(S_SORCFX2_ORBIT1, false);
    }
  actor->SetState(S_NULL, false);
}


// Orbit FX2 about sorcerer
void A_SorcFX2Orbit(DActor *actor)
{
  angle_t angle;
  fixed_t x,y,z;
  DActor *parent = (DActor *)actor->owner;
  fixed_t dist = parent->info->radius;

  if ((parent->health <= 0) ||		// Sorcerer is dead
      (!parent->args[0]))				// Time expired
    {
      actor->SetState(actor->info->deathstate, false);
      parent->args[0] = 0;
      parent->flags2 &= ~MF2_REFLECTIVE;
      parent->flags2 &= ~MF2_INVULNERABLE;
    }

  if (actor->args[0] && (parent->args[0]-- <= 0))		// Time expired
    {
      actor->SetState(actor->info->deathstate, false);
      parent->args[0] = 0;
      parent->flags2 &= ~MF2_REFLECTIVE;
    }

  // Move to new position based on angle
  if (actor->args[0])		// Counter clock-wise
    {
      actor->special1 += ANGLE_1*10;
      angle = ((angle_t)actor->special1) >> ANGLETOFINESHIFT;
      x = parent->x + FixedMul(dist, finecosine[angle]);
      y = parent->y + FixedMul(dist, finesine[angle]);
      z = parent->z - parent->floorclip + SORC_DEFENSE_HEIGHT*FRACUNIT;
      z += FixedMul(15*FRACUNIT,finecosine[angle]);
      // Spawn trailer
      actor->mp->SpawnDActor(x,y,z, MT_SORCFX2_T1);
    }
  else							// Clock wise
    {
      actor->special1 -= ANGLE_1*10;
      angle = ((angle_t)actor->special1) >> ANGLETOFINESHIFT;
      x = parent->x + FixedMul(dist, finecosine[angle]);
      y = parent->y + FixedMul(dist, finesine[angle]);
      z = parent->z - parent->floorclip + SORC_DEFENSE_HEIGHT*FRACUNIT;
      z += FixedMul(20*FRACUNIT,finesine[angle]);
      // Spawn trailer
      actor->mp->SpawnDActor(x,y,z, MT_SORCFX2_T1);
    }

  actor->x = x;
  actor->y = y;
  actor->z = z;
}



//============================================================================
// Green spell - spawn bishops
//============================================================================

void A_SpawnBishop(DActor *actor)
{
  DActor *mo;
  mo=actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_BISHOP);
  if(mo)
    {
      if(!mo->TestLocation())
	{
	  mo->SetState(S_NULL);
	}
    }
  actor->SetState(S_NULL);
}

/*
void A_SmokePuffEntry(DActor *actor)
{
	actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_MNTRSMOKE);
}
*/

void A_SmokePuffExit(DActor *actor)
{
  actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_MNTRSMOKEEXIT);
}

void A_SorcererBishopEntry(DActor *actor)
{
  actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_SORCFX3_EXPLOSION);
  S_StartSound(actor, actor->info->seesound);
}


//============================================================================
// FX4 - rapid fire balls
//============================================================================

void A_SorcFX4Check(DActor *actor)
{
  if (actor->special2-- <= 0)
    {
      actor->SetState(actor->info->deathstate, false);
    }
}

//============================================================================
// Ball death - spawn stuff
//============================================================================

void A_SorcBallPop(DActor *actor)
{
  S_StartAmbSound(SFX_SORCERER_BALLPOP);
  actor->flags &= ~MF_NOGRAVITY;
  actor->flags2 |= MF2_LOGRAV;
  actor->px = ((P_Random()%10)-5) << FRACBITS;
  actor->py = ((P_Random()%10)-5) << FRACBITS;
  actor->pz = (2+(P_Random()%3)) << FRACBITS;
  actor->special2 = 4*FRACUNIT;		// Initial bounce factor
  actor->args[4] = BOUNCE_TIME_UNIT;	// Bounce time unit
  actor->args[3] = 5;					// Bounce time in seconds
}



void A_BounceCheck(DActor *actor)
{
  if (actor->args[4]-- <= 0)
    {
      if (actor->args[3]-- <= 0)
	{
	  actor->SetState(actor->info->deathstate);
	  switch(actor->type)
	    {
	    case MT_SORCBALL1:
	    case MT_SORCBALL2:
	    case MT_SORCBALL3:
	      S_StartAmbSound(SFX_SORCERER_BIGBALLEXPLODE);
	      break;
	    case MT_SORCFX1:
	      S_StartAmbSound(SFX_SORCERER_HEADSCREAM);
	      break;
	    default:
	      break;
	    }
	}
      else
	{
	  actor->args[4] = BOUNCE_TIME_UNIT;
	}
    }
}




//============================================================================
// Class Bosses
//============================================================================
#define CLASS_BOSS_STRAFE_RANGE	64*10*FRACUNIT

void A_FastChase(DActor *actor)
{
  int delta;
  fixed_t dist;
  angle_t ang;
  Actor *target;

  if(actor->reactiontime)
    {
      actor->reactiontime--;
    }

  // Modify target threshold
  if(actor->threshold)
    {
      actor->threshold--;
    }

  if (cv_fastmonsters.value)
    { // Monsters move faster in nightmare mode
      actor->tics -= actor->tics/2;
      if(actor->tics < 3)
	{
	  actor->tics = 3;
	}
    }

  //
  // turn towards movement direction if not there yet
  //
  if(actor->movedir < 8)
    {
      actor->angle &= (7<<29);
      delta = actor->angle-(actor->movedir << 29);
      if(delta > 0)
	{
	  actor->angle -= ANG90/2;
	}
      else if(delta < 0)
	{
	  actor->angle += ANG90/2;
	}
    }

  if(!actor->target || !(actor->target->flags&MF_SHOOTABLE))
    { // look for a new target
      if(actor->LookForPlayers(true))
	{ // got a new target
	  return;
	}
      actor->SetState(actor->info->spawnstate);
      return;
    }

  //
  // don't attack twice in a row
  //
  if (actor->eflags & MFE_JUSTATTACKED)
    {
      actor->eflags &= ~MFE_JUSTATTACKED;
      if (!cv_fastmonsters.value)
	actor->P_NewChaseDir();
      return;
    }

  // Strafe
  if (actor->special2 > 0)
    {
      actor->special2--;
    }
  else
    {
      target = actor->target;
      actor->special2 = 0;
      actor->px = actor->py = 0;
      dist=P_AproxDistance(actor->x - target->x,
			   actor->y - target->y);
      if (dist < CLASS_BOSS_STRAFE_RANGE)
	{
	  if (P_Random()<100)
	    {
	      ang = R_PointToAngle2(actor->x, actor->y,
				    target->x, target->y);
	      if (P_Random()<128)
		ang += ANGLE_90;
	      else
		ang -= ANGLE_90;
	      ang>>=ANGLETOFINESHIFT;
	      actor->px = FixedMul(13*FRACUNIT, finecosine[ang]);
	      actor->py = FixedMul(13*FRACUNIT, finesine[ang]);
	      actor->special2 = 3;		// strafe time
	    }
	}
    }

  //
  // check for missile attack
  //
  if (actor->info->missilestate)
    {
      if (!cv_fastmonsters.value && actor->movecount)
	goto nomissile;
      if (!actor->CheckMissileRange())
	goto nomissile;
      actor->SetState(actor->info->missilestate);
      actor->eflags |= MFE_JUSTATTACKED;
      return;
    }
 nomissile:

  //
  // possibly choose another target
  //
  if (!actor->threshold && !actor->mp->CheckSight(actor, actor->target))
    {
      if (actor->LookForPlayers(true))
	return;         // got a new target
    }

  //
  // chase towards player
  //
  if (!actor->special2)
    {
      if (--actor->movecount<0 || !actor->P_Move())
	{
	  actor->P_NewChaseDir();
	}
    }
}


void A_FighterAttack(DActor *actor)
{
  extern void A_FSwordAttack2(DActor *actor);

  if(!actor->target) return;
  A_FSwordAttack2(actor);
}


void A_ClericAttack(DActor *actor)
{
  extern void A_CHolyAttack3(DActor *actor);

  if(!actor->target) return;
  A_CHolyAttack3(actor);
}



void A_MageAttack(DActor *actor)
{
  extern void A_MStaffAttack2(DActor *actor);

  if(!actor->target) return;
  A_MStaffAttack2(actor);
}

void A_ClassBossHealth(DActor *actor)
{
  if (game.netgame && !cv_deathmatch.value) // co-op only
    {
      if (!actor->special1)
	{
	  actor->health *= 5;
	  actor->special1 = true;   // has been initialized
	}
    }
}


//===========================================================================
//
// A_CheckFloor - Checks if an object hit the floor
//
//===========================================================================

void A_CheckFloor(DActor *actor)
{
  if(actor->z <= actor->floorz)
    {
      actor->z = actor->floorz;
      actor->flags2 &= ~MF2_LOGRAV;
      actor->SetState(actor->info->deathstate);
    }
}

//============================================================================
//
// A_FreezeDeath
//
//============================================================================

void A_FreezeDeath(DActor *actor)
{
  actor->tics = 75+P_Random()+P_Random();
  actor->flags |= MF_SOLID|MF_SHOOTABLE|MF_NOBLOOD;
  actor->flags2 |= MF2_PUSHABLE|MF2_TELESTOMP|MF2_PASSMOBJ|MF2_SLIDE;
  actor->height <<= 2;
  S_StartSound(actor, SFX_FREEZE_DEATH);

  /*
  if(actor->player)
    {
      actor->player->damagecount = 0;
      actor->player->poisoncount = 0;
      actor->player->bonuscount = 0;
      if(actor->player == &players[consoleplayer])
        SB_PaletteFlash(false);
    }
  else
  */
  if (actor->flags&MF_COUNTKILL && actor->special)
    { // Initiate monster death actions
      actor->mp->ExecuteLineSpecial(actor->special, actor->args, NULL, 0, actor);
    }
}

//============================================================================
//
// A_IceSetTics
//
//============================================================================

void A_IceSetTics(DActor *actor)
{
  actor->tics = 70+(P_Random()&63);
  int floor = actor->subsector->sector->floortype;
  if(floor == FLOOR_LAVA)
    {
      actor->tics >>= 2;
    }
  else if(floor == FLOOR_ICE)
    {
      actor->tics <<= 1;
    }
}

//============================================================================
//
// A_IceCheckHeadDone
//
//============================================================================

void A_IceCheckHeadDone(DActor *actor)
{
  if(actor->special2 == 666)
    {
      actor->SetState(S_ICECHUNK_HEAD2);
    }
}

//============================================================================
//
// A_FreezeDeathChunks
//
//============================================================================

void A_FreezeDeathChunks(DActor *actor)
{
  int i;
  DActor *mo;
	
  if(actor->px || actor->py || actor->pz)
    {
      actor->tics = 105;
      return;
    }
  S_StartSound(actor, SFX_FREEZE_SHATTER);

  for(i = 12+(P_Random()&15); i >= 0; i--)
    {
      mo = actor->mp->SpawnDActor(actor->x+(((P_Random()-128)*actor->radius)>>7), 
		       actor->y+(((P_Random()-128)*actor->radius)>>7), 
		       actor->z+(P_Random()*actor->height/255), MT_ICECHUNK);
      mo->SetState(statenum_t(mo->info->spawnstate + P_Random()%3));
      if(mo)
	{
	  mo->pz = FixedDiv(mo->z-actor->z, actor->height)<<2;
	  mo->px = (P_Random()-P_Random())<<(FRACBITS-7);
	  mo->py = (P_Random()-P_Random())<<(FRACBITS-7);
	  A_IceSetTics(mo); // set a random tic wait
	}
    }
  for(i = 12+(P_Random()&15); i >= 0; i--)
    {
      mo = actor->mp->SpawnDActor(actor->x+(((P_Random()-128)*actor->radius)>>7), 
		       actor->y+(((P_Random()-128)*actor->radius)>>7), 
		       actor->z+(P_Random()*actor->height/255), MT_ICECHUNK);
      mo->SetState(statenum_t(mo->info->spawnstate + P_Random()%3));
      if(mo)
	{
	  mo->pz = FixedDiv(mo->z-actor->z, actor->height)<<2;
	  mo->px = (P_Random()-P_Random())<<(FRACBITS-7);
	  mo->py = (P_Random()-P_Random())<<(FRACBITS-7);
	  A_IceSetTics(mo); // set a random tic wait
	}
    }
  /*
  if(actor->player)
    { // attach the player's view to a chunk of ice
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+VIEWHEIGHT, MT_ICECHUNK);
      mo->SetState(S_ICECHUNK_HEAD);
      mo->pz = FixedDiv(mo->z-actor->z, actor->height)<<2;
      mo->px = (P_Random()-P_Random())<<(FRACBITS-7);
      mo->py = (P_Random()-P_Random())<<(FRACBITS-7);
      mo->flags2 |= MF2_ICEDAMAGE; // used to force blue palette
      mo->flags2 &= ~MF2_FLOORCLIP;
      mo->player = actor->player;
      actor->player = NULL;
      mo->health = actor->health;
      mo->angle = actor->angle;
      mo->player->mo = mo;
      mo->player->lookdir = 0;
    }
  */
  actor->mp->RemoveFromTIDmap(actor);
  actor->SetState(S_FREETARGMOBJ);
  actor->flags2 |= MF2_DONTDRAW;
}

//===========================================================================
// Korax Variables
//	special1	last teleport destination
//	special2	set if "below half" script not yet run
//
// Korax Scripts (reserved)
//	249		Tell scripts that we are below half health
//	250-254	Control scripts
//	255		Death script
//
// Korax TIDs (reserved)
//	245		Reserved for Korax himself
//  248		Initial teleport destination
//	249		Teleport destination
//	250-254	For use in respective control scripts
//	255		For use in death script (spawn spots)
//===========================================================================
#define KORAX_SPIRIT_LIFETIME	(5*(35/5))	// 5 seconds
#define KORAX_COMMAND_HEIGHT	(120*FRACUNIT)
#define KORAX_COMMAND_OFFSET	(27*FRACUNIT)

void KoraxFire1(DActor *actor, mobjtype_t type);
void KoraxFire2(DActor *actor, mobjtype_t type);
void KoraxFire3(DActor *actor, mobjtype_t type);
void KoraxFire4(DActor *actor, mobjtype_t type);
void KoraxFire5(DActor *actor, mobjtype_t type);
void KoraxFire6(DActor *actor, mobjtype_t type);
void KSpiritInit(DActor *spirit, DActor *korax);

#define KORAX_TID	   	       	(245)
#define KORAX_FIRST_TELEPORT_TID	(248)
#define KORAX_TELEPORT_TID	        (249)

void A_KoraxChase(DActor *actor)
{
  Actor *spot;
  int lastfound;
  byte args[3]={0,0,0};

  if ((!actor->special2) &&
      (actor->health <= (actor->info->spawnhealth/2)))
    {
      lastfound = 0;
      spot = actor->mp->FindFromTIDmap(KORAX_FIRST_TELEPORT_TID, &lastfound);
      if (spot)
	{
	  actor->Teleport(spot->x, spot->y, spot->angle, false);
	}

      actor->mp->StartACS(249, args, actor, NULL, 0);
      actor->special2 = 1;	// Don't run again

      return;
    }

  if (!actor->target) return;
  if (P_Random()<30)
    {
      actor->SetState(actor->info->missilestate);
    }
  else if (P_Random()<30)
    {
      S_StartAmbSound(SFX_KORAX_ACTIVE);
    }

  // Teleport away
  if (actor->health < (actor->info->spawnhealth>>1))
    {
      if (P_Random()<10)
	{
	  lastfound = actor->special1;
	  spot = actor->mp->FindFromTIDmap(KORAX_TELEPORT_TID, &lastfound);
	  actor->special1 = lastfound;
	  if (spot)
	    {
	      actor->Teleport(spot->x, spot->y, spot->angle, false);
	    }
	}
    }
}

void A_KoraxStep(DActor *actor)
{
  A_Chase(actor);
}

void A_KoraxStep2(DActor *actor)
{
  S_StartAmbSound(SFX_KORAX_STEP);
  A_Chase(actor);
}

void A_KoraxBonePop(DActor *actor)
{
  fixed_t x,y,z;
  DActor *mo;
  byte args[5];

  args[0]=args[1]=args[2]=args[3]=args[4]=0;
  x=actor->x, y=actor->y, z=actor->z;

  // Spawn 6 spirits equalangularly
  mo = actor->SpawnMissileAngle(MT_KORAX_SPIRIT1, ANGLE_60*0, 5*FRACUNIT);
  if (mo) KSpiritInit(mo,actor);
  mo = actor->SpawnMissileAngle(MT_KORAX_SPIRIT2, ANGLE_60*1, 5*FRACUNIT);
  if (mo) KSpiritInit(mo,actor);
  mo = actor->SpawnMissileAngle(MT_KORAX_SPIRIT3, ANGLE_60*2, 5*FRACUNIT);
  if (mo) KSpiritInit(mo,actor);
  mo = actor->SpawnMissileAngle(MT_KORAX_SPIRIT4, ANGLE_60*3, 5*FRACUNIT);
  if (mo) KSpiritInit(mo,actor);
  mo = actor->SpawnMissileAngle(MT_KORAX_SPIRIT5, ANGLE_60*4, 5*FRACUNIT);
  if (mo) KSpiritInit(mo,actor);
  mo = actor->SpawnMissileAngle(MT_KORAX_SPIRIT6, ANGLE_60*5, 5*FRACUNIT);
  if (mo) KSpiritInit(mo,actor);

  actor->mp->StartACS(255, args, actor, NULL, 0);		// Death script
}

void KSpiritInit(DActor *spirit, DActor *korax)
{
  int i;
  DActor *tail, *next;

  spirit->health = KORAX_SPIRIT_LIFETIME;

  // Swarm around korax
  spirit->special2 = 32+(P_Random()&7);		// Float bob index
  spirit->args[0] = 10; 						// initial turn value
  spirit->args[1] = 0; 						// initial look angle

  // Spawn a tail for spirit
  tail = spirit->mp->SpawnDActor(spirit->x, spirit->y, spirit->z, MT_HOLY_TAIL);
  tail->owner = spirit; // parent
  for(i = 1; i < 3; i++)
    {
      next = spirit->mp->SpawnDActor(spirit->x, spirit->y, spirit->z, MT_HOLY_TAIL);
      next->SetState(statenum_t(next->info->spawnstate + 1));
      tail->target = next;
      tail = next;
    }
  tail->target = NULL; // last tail bit
}

void A_KoraxDecide(DActor *actor)
{
  if (P_Random()<220)
    {
      actor->SetState(S_KORAX_MISSILE1);
    }
  else
    {
      actor->SetState(S_KORAX_COMMAND1);
    }
}

void A_KoraxMissile(DActor *actor)
{
  int i = P_Random()%6;
  mobjtype_t type;
  int sound;

  S_StartSound(actor, SFX_KORAX_ATTACK);

  switch (i)
    {
    case 0:
      type = MT_WRAITHFX1;
      sound = SFX_WRAITH_MISSILE_FIRE;
      break;
    case 1:
      type = MT_DEMONFX1;
      sound = SFX_DEMON_MISSILE_FIRE;
      break;
    case 2:
      type = MT_DEMON2FX1;
      sound = SFX_DEMON_MISSILE_FIRE;
      break;
    case 3:
      type = MT_FIREDEMON_FX6;
      sound = SFX_FIRED_ATTACK;
      break;
    case 4:
      type = MT_CENTAUR_FX;
      sound = SFX_CENTAURLEADER_ATTACK;
      break;
    case 5:
    default:
      type = MT_SERPENTFX;
      sound = SFX_CENTAURLEADER_ATTACK;
      break;
    }

  // Fire all 6 missiles at once
  S_StartAmbSound(sound);
  KoraxFire1(actor, type);
  KoraxFire2(actor, type);
  KoraxFire3(actor, type);
  KoraxFire4(actor, type);
  KoraxFire5(actor, type);
  KoraxFire6(actor, type);
}


// Call action code scripts (250-254)
void A_KoraxCommand(DActor *actor)
{
  byte args[5];
  fixed_t x,y,z;
  angle_t ang;
  int numcommands;

  S_StartSound(actor, SFX_KORAX_COMMAND);

  // Shoot stream of lightning to ceiling
  ang = (actor->angle - ANGLE_90) >> ANGLETOFINESHIFT;
  x=actor->x + FixedMul(KORAX_COMMAND_OFFSET,finecosine[ang]);
  y=actor->y + FixedMul(KORAX_COMMAND_OFFSET,finesine[ang]);
  z=actor->z + KORAX_COMMAND_HEIGHT;
  actor->mp->SpawnDActor(x,y,z, MT_KORAX_BOLT);

  args[0]=args[1]=args[2]=args[3]=args[4]=0;

  if (actor->health <= (actor->info->spawnhealth >> 1))
    {
      numcommands = 5;
    }
  else
    {
      numcommands = 4;
    }

  Map *mp = actor->mp;

  switch(P_Random() % numcommands)
    {
    case 0:
      mp->StartACS(250, args, actor, NULL, 0);
      break;
    case 1:
      mp->StartACS(251, args, actor, NULL, 0);
      break;
    case 2:
      mp->StartACS(252, args, actor, NULL, 0);
      break;
    case 3:
      mp->StartACS(253, args, actor, NULL, 0);
      break;
    case 4:
      mp->StartACS(254, args, actor, NULL, 0);
      break;
    }
}


#define KORAX_DELTAANGLE			(85*ANGLE_1)
#define KORAX_ARM_EXTENSION_SHORT	(40*FRACUNIT)
#define KORAX_ARM_EXTENSION_LONG	(55*FRACUNIT)

#define KORAX_ARM1_HEIGHT			(108*FRACUNIT)
#define KORAX_ARM2_HEIGHT			(82*FRACUNIT)
#define KORAX_ARM3_HEIGHT			(54*FRACUNIT)
#define KORAX_ARM4_HEIGHT			(104*FRACUNIT)
#define KORAX_ARM5_HEIGHT			(86*FRACUNIT)
#define KORAX_ARM6_HEIGHT			(53*FRACUNIT)


DActor *P_SpawnKoraxMissile(fixed_t x, fixed_t y, fixed_t z,
			    Actor *source, Actor *dest, mobjtype_t type)
{
  angle_t an;
  int dist;

  z -= source->floorclip;
  DActor *th = source->mp->SpawnDActor(x, y, z, type);
  if (th->info->seesound)
    S_StartSound(th, th->info->seesound);

  th->owner = source; // Originator
  an = R_PointToAngle2(x, y, dest->x, dest->y);
  if (dest->flags & MF_SHADOW)
    { // Invisible target
      an += (P_Random()-P_Random())<<21;
    }
  th->angle = an;
  an >>= ANGLETOFINESHIFT;
  th->px = int(th->info->speed * finecosine[an]);
  th->py = int(th->info->speed * finesine[an]);
  dist = P_AproxDistance(dest->x - x, dest->y - y);
  dist = dist/int(th->info->speed * FRACUNIT);
  if (dist < 1)
    dist = 1;

  th->pz = (dest->z-z+(30*FRACUNIT))/dist;
  return (th->CheckMissileSpawn() ? th : NULL);
}


// Arm projectiles
//		arm positions numbered:
//			1	top left
//			2	middle left
//			3	lower left
//			4	top right
//			5	middle right
//			6	lower right


// Arm 1 projectile
void KoraxFire1(DActor *actor, mobjtype_t type)
{
  DActor *mo;
  angle_t ang;
  fixed_t x,y,z;

  ang = (actor->angle - KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
  x = actor->x + FixedMul(KORAX_ARM_EXTENSION_SHORT, finecosine[ang]);
  y = actor->y + FixedMul(KORAX_ARM_EXTENSION_SHORT, finesine[ang]);
  z = actor->z - actor->floorclip + KORAX_ARM1_HEIGHT;
  mo = P_SpawnKoraxMissile(x,y,z,actor, actor->target, type);
}


// Arm 2 projectile
void KoraxFire2(DActor *actor, mobjtype_t type)
{
  DActor *mo;
  angle_t ang;
  fixed_t x,y,z;

  ang = (actor->angle - KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
  x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
  y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
  z = actor->z - actor->floorclip + KORAX_ARM2_HEIGHT;
  mo = P_SpawnKoraxMissile(x,y,z,actor, actor->target, type);
}

// Arm 3 projectile
void KoraxFire3(DActor *actor, mobjtype_t type)
{
  DActor *mo;
  angle_t ang;
  fixed_t x,y,z;

  ang = (actor->angle - KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
  x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
  y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
  z = actor->z - actor->floorclip + KORAX_ARM3_HEIGHT;
  mo = P_SpawnKoraxMissile(x,y,z,actor, actor->target, type);
}

// Arm 4 projectile
void KoraxFire4(DActor *actor, mobjtype_t type)
{
  DActor *mo;
  angle_t ang;
  fixed_t x,y,z;

  ang = (actor->angle + KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
  x = actor->x + FixedMul(KORAX_ARM_EXTENSION_SHORT, finecosine[ang]);
  y = actor->y + FixedMul(KORAX_ARM_EXTENSION_SHORT, finesine[ang]);
  z = actor->z - actor->floorclip + KORAX_ARM4_HEIGHT;
  mo = P_SpawnKoraxMissile(x,y,z,actor, actor->target, type);
}

// Arm 5 projectile
void KoraxFire5(DActor *actor, mobjtype_t type)
{
  DActor *mo;
  angle_t ang;
  fixed_t x,y,z;

  ang = (actor->angle + KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
  x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
  y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
  z = actor->z - actor->floorclip + KORAX_ARM5_HEIGHT;
  mo = P_SpawnKoraxMissile(x,y,z,actor, actor->target, type);
}

// Arm 6 projectile
void KoraxFire6(DActor *actor, mobjtype_t type)
{
  DActor *mo;
  angle_t ang;
  fixed_t x,y,z;

  ang = (actor->angle + KORAX_DELTAANGLE) >> ANGLETOFINESHIFT;
  x = actor->x + FixedMul(KORAX_ARM_EXTENSION_LONG, finecosine[ang]);
  y = actor->y + FixedMul(KORAX_ARM_EXTENSION_LONG, finesine[ang]);
  z = actor->z - actor->floorclip + KORAX_ARM6_HEIGHT;
  mo = P_SpawnKoraxMissile(x,y,z,actor, actor->target, type);
}


void A_KSpiritWeave(DActor *actor)
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

void A_KSpiritSeeker(DActor *actor, angle_t thresh, angle_t turnMax)
{
  int dir;
  angle_t delta;
  angle_t angle;
  fixed_t newZ;
  fixed_t deltaZ;

  DActor *target = (DActor *)actor->owner;
  if (target == NULL)
    return;

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

  if(!(gametic & 15) 
     || actor->z > target->z+(target->info->height)
     || actor->z+actor->height < target->z)
    {
      newZ = target->z+((P_Random()*target->info->height)>>8);
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
      float dist = P_AproxDistance(target->x-actor->x, target->y-actor->y);
      dist = dist/actor->info->speed;
      if(dist < 1)
	{
	  dist = 1;
	}
      actor->pz = int(deltaZ/dist);
    }
  return;
}


void A_KSpiritRoam(DActor *actor)
{
  if (actor->health-- <= 0)
    {
      S_StartSound(actor, SFX_SPIRIT_DIE);
      actor->SetState(S_KSPIRIT_DEATH1);
    }
  else
    {
      if (actor->owner)
	{
	  A_KSpiritSeeker(actor, actor->args[0]*ANGLE_1,
			  actor->args[0]*ANGLE_1*2);
	}
      A_KSpiritWeave(actor);
      if (P_Random()<50)
	{
	  S_StartAmbSound(SFX_SPIRIT_ACTIVE);
	}
    }
}

void A_KBolt(DActor *actor)
{
  // Countdown lifetime
  if (actor->special1-- <= 0)
    {
      actor->SetState(S_NULL);
    }
}


#define KORAX_BOLT_HEIGHT		48*FRACUNIT
#define KORAX_BOLT_LIFETIME		3

void A_KBoltRaise(DActor *actor)
{
  DActor *mo;
  fixed_t z;

  // Spawn a child upward
  z = actor->z + KORAX_BOLT_HEIGHT;

  if ((z + KORAX_BOLT_HEIGHT) < actor->ceilingz)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, z, MT_KORAX_BOLT);
      if (mo)
	{
	  mo->special1 = KORAX_BOLT_LIFETIME;
	}
    }
  else
    {
      // Maybe cap it off here
    }
}

