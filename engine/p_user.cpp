// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.25  2004/11/04 21:12:53  smite-meister
// save/load fixed
//
// Revision 1.24  2004/10/27 17:37:07  smite-meister
// netcode update
//
// Revision 1.23  2004/09/13 20:43:30  smite-meister
// interface cleanup, sp map reset fixed
//
// Revision 1.22  2004/07/05 16:53:26  smite-meister
// Netcode replaced
//
// Revision 1.21  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.19  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.18  2004/01/02 14:25:01  smite-meister
// cleanup
//
// Revision 1.17  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.16  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.15  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.14  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.13  2003/06/29 17:33:59  smite-meister
// VFile system, PAK support, Hexen bugfixes
//
// Revision 1.12  2003/05/30 13:34:47  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.10  2003/04/26 12:01:13  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.9  2003/04/24 20:30:19  hurdler
// Remove lots of compiling warnings
//
// Revision 1.8  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.7  2003/04/14 08:58:28  smite-meister
// Hexen maps load.
//
// Revision 1.6  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.5  2003/03/08 16:07:09  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:12:03  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:04  hurdler
// Initial C++ version of Doom Legacy
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Morphing, artifacts, inventory responder.


#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "cvars.h"
#include "d_event.h"

#include "g_game.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"
#include "g_input.h"

#include "p_camera.h"

#include "p_enemy.h"
#include "p_maputl.h"
#include "r_sprite.h"

#include "sounds.h"
#include "m_random.h"
#include "tables.h"
#include "dstrings.h"

#include "hardware/hw3sound.h"



//============================================================
//  Morphing
//============================================================

const int MORPHTICS = 40*TICRATE;

bool Actor::Morph(mobjtype_t form)
{
  return false;
}

bool DActor::Morph(mobjtype_t form)
{
  if (!(flags & MF_COUNTKILL))
    return false;
  if (flags2 & MF2_BOSS)
    return false;

  switch (type)
    {
    case MT_POD:
    case MT_CHICKEN:
    case MT_HHEAD:
    case MT_PIG:
    case MT_FIGHTER_BOSS:
    case MT_CLERIC_BOSS:
    case MT_MAGE_BOSS:
      return false;

    default:
      break;
    }

  // remove the old monster
  int oldtid = tid;
  Remove(); // zeroes tid

  DActor *fog = mp->SpawnDActor(x, y, z + TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_teleport);

  // create the morphed monster
  DActor *monster = mp->SpawnDActor(x, y, z, form);
  monster->special2 = type;
  monster->special1 = MORPHTICS + P_Random();
  monster->flags |= (flags & MF_SHADOW);
  monster->owner = owner;
  monster->target = target;
  monster->angle = angle;

  monster->tid = oldtid;
  mp->InsertIntoTIDmap(monster, oldtid);

  monster->special = special;
  memcpy(monster->args, args, 5);

  return true;
}


bool Pawn::Morph(mobjtype_t form) { return false; }


bool PlayerPawn::Morph(mobjtype_t form)
{
  if (morphTics)
    {
      if ((morphTics < MORPHTICS-TICRATE) && !powers[pw_weaponlevel2])
	GivePower(pw_weaponlevel2); // Make a super beast
      return false;
    }

  if (powers[pw_invulnerability])
    return false; // Immune when invulnerable

  DActor *fog = mp->SpawnDActor(x, y, z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_teleport);

  const mobjinfo_t *i = &mobjinfo[form];

  //MT_PIGPLAYER, MT_CHICPLAYER
  morphTics = MORPHTICS;

  //const int MAXMORPHHEALTH = 30;
  health = maxhealth = i->spawnhealth;
  speed  = i->speed;
  radius = i->radius;
  height = i->height;
  mass   = i->mass;

  attackphase = readyweapon; // store current weapon
  armorfactor[0] = armorpoints[0] = 0;
  powers[pw_invisibility] = 0;
  powers[pw_weaponlevel2] = 0;
  weaponinfo = wpnlev1info;

  pclass = PCLASS_PIG;
  ActivateMorphWeapon();
  return true;
}


bool PlayerPawn::UndoMorph()
{
  // store the current values
  fixed_t r = radius;
  fixed_t h = height;

  const mobjinfo_t *i = &mobjinfo[pinfo->mt];

  radius = i->radius;
  height = i->height;

  if (TestLocation() == false)
    {
      // Didn't fit, continue morph
      morphTics = 2*35;
      radius = r;
      height = h;
      // some sound to indicate unsuccesful morph?
      return false;
    }

  morphTics = 0;

  health = maxhealth = i->spawnhealth;
  speed  = i->speed;
  mass = i->mass;

  reactiontime = 18;
  powers[pw_weaponlevel2] = 0;
  weaponinfo = wpnlev1info;

  angle_t ang = angle >> ANGLETOFINESHIFT;
  DActor *fog = mp->SpawnDActor(x+20*finecosine[ang], y+20*finesine[ang],
				z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_teleport);
  PostMorphWeapon(weapontype_t(attackphase));

  return true;
}



//============================================================
//  Artifacts
//============================================================

extern consvar_t cv_deathmatch;

void P_AutoUseHealth(PlayerPawn *p, int saveHealth)
{
  int i, n = p->inventory.size();
  int count;
  int normalSlot;
  int superSlot;
  int normalCount = 0;
  int superCount = 0;

  for (i = 0; i < n; i++)
    {
      if (p->inventory[i].type == arti_health)
        {
	  normalSlot = i;
	  normalCount = p->inventory[i].count;
        }
      else if (p->inventory[i].type == arti_superhealth)
        {
	  superSlot = i;
	  superCount = p->inventory[i].count;
        }
    }

  if (game.skill == sk_baby && normalCount*25 >= saveHealth)
    { // Use quartz flasks
      count = (saveHealth+24)/25;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_health);
    }
  else if(superCount*100 >= saveHealth)
    { // Use mystic urns
      count = (saveHealth+99)/100;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_superhealth);
    }
  else if (game.skill == sk_baby && superCount*100 + normalCount*25 >= saveHealth)
    { // Use mystic urns and quartz flasks
      count = (saveHealth+24)/25;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_health);

      saveHealth -= count*25;
      count = (saveHealth+99)/100;
      for (i = 0; i < count; i++)
	p->UseArtifact(arti_superhealth);
    }
}


static void P_TeleportToPlayerStarts(Actor *v, int n, int ep)
{
  Map *m = v->mp;
  mapthing_t *mt = NULL;
  multimap<int, mapthing_t *>::iterator s, t;

  if (n)
    {
      s = m->playerstarts.lower_bound(n);
      t = m->playerstarts.upper_bound(n);
    }
  else
    {
      // TODO teleport to random ep 0 start, not first
      s = m->playerstarts.begin();
      t = m->playerstarts.end();
    }

  for ( ; s != t; s++)
    {
      mt = (*s).second;
      if (mt->args[0] == ep)
	break;
    }

  // if nothing suitable is found, just pick any start
  if (s == t)
    mt = (*m->playerstarts.begin()).second;

  v->Teleport(mt->x << FRACBITS, mt->y << FRACBITS, ANG45 * (mt->angle / 45));
}


static bool P_TeleportToDeathmatchStarts(Actor *v)
{
  int n = v->mp->dmstarts.size();
  if (n == 0)
    return false;

  n = P_Random() % n;
  mapthing_t *m = v->mp->dmstarts[n];
  return v->Teleport(m->x << FRACBITS, m->y << FRACBITS, ANG45 * (m->angle / 45));
}



//============================================================
// Chaos Device, teleports the player back to a playerstart

bool P_AutoUseChaosDevice(PlayerPawn *p)
{
  int i, n = p->inventory.size();
    
  for (i = 0; i < n; i++)
    {
      if (p->inventory[i].type == arti_teleport)
        {
	  p->UseArtifact(arti_teleport);
	  p->health = (p->health + 1) / 2;
	  return true;
        }
    }
  return false;
}



void P_ArtiTele(PlayerPawn *p)
{
  if (!cv_deathmatch.value || !P_TeleportToDeathmatchStarts(p))
    P_TeleportToPlayerStarts(p, p->player->number, p->player->entrypoint);

  if (p->morphTics)
    p->UndoMorph(); // Teleporting away will undo any morph effects (pig)

  S_StartAmbSound(p->player, sfx_weaponup); // Full volume laugh
}


//============================================================
// Banishment Device

void P_TeleportOther(Actor *v)
{
  Map *m = v->mp;

  if (v->flags & MF_NOTMONSTER)
    {
      if (!cv_deathmatch.value || !P_TeleportToDeathmatchStarts(v))
	P_TeleportToPlayerStarts(v, 0, 0);
    }
  else
    {
      // For death actions, teleporting is as good as killing
      // TODO possible bug: see A_SorcDeath
      if (v->flags & MF_COUNTKILL && v->special)
	{
	  m->RemoveFromTIDmap(v);
	  m->ExecuteLineSpecial(v->special, v->args, NULL, 0, v);
	  v->special = 0;
	}

      // Send all monsters to deathmatch spots
      if (!P_TeleportToDeathmatchStarts(v))
	P_TeleportToPlayerStarts(v, 0, 0);
    }
}


//============================================================
// Mystic Ambit Incantation, class specific effect for everyone in radius
// if only C++ had functions inside functions!

static Actor *caster;
static bool   given;

static bool IT_HealRadius(Thinker *th)
{
  if (!th->IsOf(PlayerPawn::_type))
    return true;
	
  PlayerPawn *t = (PlayerPawn *)th;
  if (t->health <= 0)
    return true;
		
  fixed_t dist = P_AproxDistance(caster->x - t->x, caster->y - t->y);
  const fixed_t HEAL_RADIUS_DIST = 255*FRACUNIT;

  if (dist > HEAL_RADIUS_DIST)
    return true;

  int amount;
  switch (t->pclass)
    {
    case PCLASS_FIGHTER: // armor boost
      if (t->GiveArmor(armor_armor, -3, 1)  || t->GiveArmor(armor_shield, -3, 1) ||
	  t->GiveArmor(armor_helmet, -3, 1) || t->GiveArmor(armor_amulet, -3, 1))
	{
	  given = true;
	  S_StartSound(t, SFX_MYSTICINCANT);
	}
      break;

    case PCLASS_CLERIC: // heal
      if (t->GiveBody(50 + P_Random() % 50))
	{
	  given = true;
	  S_StartSound(t, SFX_MYSTICINCANT);
	}
	  break;

    case PCLASS_MAGE: // mana boost
      amount = 50 + (P_Random()%50);
      if (t->GiveAmmo(am_mana1, amount) || t->GiveAmmo(am_mana2, amount))
	{
	  given = true;
	  S_StartSound(t, SFX_MYSTICINCANT);
	}
      break;

    default:
      break;
    }

  return true;
}


bool P_HealRadius(Actor *p)
{
  given = false;
  caster = p;
  p->mp->IterateThinkers(IT_HealRadius);
  return given;
}


//============================================================
// Disc of Repulsion

static const fixed_t BLAST_SPEED = 20*FRACUNIT;
static const fixed_t BLAST_FULLSTRENGTH = 255;

static void P_BlastMobj(Actor *source, Actor *victim, fixed_t strength)
{
  angle_t angle = R_PointToAngle2(source->x, source->y, victim->x, victim->y);
  angle_t ang = (angle + ANG180) >> ANGLETOFINESHIFT;
  angle >>= ANGLETOFINESHIFT;

  if (strength < BLAST_FULLSTRENGTH)
    {
      victim->px = FixedMul(strength, finecosine[angle]);
      victim->py = FixedMul(strength, finesine[angle]);
    }
  else // full strength blast from artifact
    {
      if (victim->flags & MF_MISSILE)
	{
	  // guided missiles change owner!
	  if (victim->target == source)
	    {
	      victim->target = victim->owner;
	      victim->owner = source;
	    }
	}

      victim->px = FixedMul(BLAST_SPEED, finecosine[angle]);
      victim->py = FixedMul(BLAST_SPEED, finesine[angle]);

      // Spawn blast puff
      fixed_t x, y, z;
      x = victim->x + FixedMul(victim->radius+FRACUNIT, finecosine[ang]);
      y = victim->y + FixedMul(victim->radius+FRACUNIT, finesine[ang]);
      z = victim->z - victim->floorclip + (victim->height>>1);

      DActor *m = victim->mp->SpawnDActor(x, y, z, MT_BLASTEFFECT);
      if (m)
	{
	  m->px = victim->px;
	  m->py = victim->py;
	}

      if (victim->flags & MF_MISSILE)
	{
	  victim->pz = 8*FRACUNIT;
	  m->pz = victim->pz;
	}
      else
	victim->pz = (1000 / victim->mass) << FRACBITS;
    }


  victim->flags2 |= MF2_SLIDE;
  victim->eflags |= MFE_BLASTED;
}


static bool IT_BlastRadius(Thinker *th)
{
  Actor *a = NULL;
  if (th->IsOf(DActor::_type))
    {
      DActor *m = (DActor *)th;

      // must be missile, monster, corpse or poisoncloud
      // must not be boss or dormant
      if (!(m->type == MT_POISONCLOUD || (m->flags & MF_CORPSE) ||
	    (m->flags & MF_COUNTKILL) || (m->flags & MF_MISSILE))
	  || (m->flags2 & MF2_DORMANT) || (m->flags2 & MF2_BOSS))
	return true;

      switch (m->type)
	{
	case MT_SORCBALL1: // don't blast sorcerer balls
	case MT_SORCBALL2:
	case MT_SORCBALL3:
	  return true;

	default:
	  break;
	}

      if (m->type == MT_WRAITHB && (m->flags2 & MF2_DONTDRAW))
	return true; // no underground wraiths

      if (m->type == MT_SPLASHBASE || m->type == MT_SPLASH)
	return true;

      if (m->type == MT_SERPENT || m->type == MT_SERPENTLEADER)
	return true; // no swimmers

      a = m;
    }
  else if (th->IsDescendantOf(Actor::_type))
    a = (Actor *)th;
  else
    return true;

  // dactors and playerpawns are blasted
  fixed_t dist = P_AproxDistance(caster->x - a->x, caster->y - a->y);
  if (dist > 255*FRACUNIT)
    return true; // Out of range

  P_BlastMobj(caster, a, BLAST_FULLSTRENGTH);
  return true;
}


// Blast all mobj things away
static void P_BlastRadius(PlayerPawn *p)
{
  S_StartSound(p, SFX_ARTIFACT_BLAST);
  P_NoiseAlert(p, p);
  caster = p;
  p->mp->IterateThinkers(IT_BlastRadius);
}


//============================================================
//  Activating an artifact

bool P_UseArtifact(PlayerPawn *p, artitype_t arti)
{
  DActor *mo;
  angle_t ang;
  int count;
  vector<inventory_t>::iterator t;

  switch (arti)
    {
    case arti_invulnerability:
      return p->GivePower(pw_invulnerability);

    case arti_invisibility:
      return p->GivePower(pw_invisibility);

    case arti_health:
      return p->GiveBody(25);

    case arti_superhealth:
      return p->GiveBody(100);

    case arti_tomeofpower:
      if (p->morphTics)
        { // Attempt to undo chicken
	  if (p->UndoMorph() == false)
            { // Failed
	      p->Damage(NULL, NULL, 10000, dt_always);
            }
	  else
            { // Succeeded
	      p->morphTics = 0;
	      S_StartScreamSound(p, sfx_weaponup);
            }
        }
      else
        {
	  if (!p->GivePower(pw_weaponlevel2))
	    return false;

	  if (p->readyweapon == wp_staff)
	    p->SetPsprite(ps_weapon, S_STAFFREADY2_1);
	  else if (p->readyweapon == wp_gauntlets)
	    p->SetPsprite(ps_weapon, S_GAUNTLETREADY2_1);
        }
      break;

    case arti_torch:
      return p->GivePower(pw_infrared);

    case arti_firebomb:
      ang = p->angle >> ANGLETOFINESHIFT;
      mo = p->mp->SpawnDActor(p->x+24*finecosine[ang], p->y+24*finesine[ang], p->z - p->floorclip, MT_FIREBOMB);
      mo->owner = p;
      break;

    case arti_egg:
      p->SpawnPlayerMissile(MT_EGGFX);
      p->SPMAngle(MT_EGGFX, p->angle-(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->angle+(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->angle-(ANG45/3));
      p->SPMAngle(MT_EGGFX, p->angle+(ANG45/3));
      break;

    case arti_fly:
      return p->GivePower(pw_flight);

    case arti_teleport:
      P_ArtiTele(p);
      break;

    case arti_healingradius:
      return P_HealRadius(p);

    case arti_summon:
      mo = p->SpawnPlayerMissile(MT_SUMMON_FX);
      if (mo)
	{
	  mo->owner = p;
	  mo->pz = 5*FRACUNIT;
	}
      break;

    case arti_pork:
      p->SpawnPlayerMissile(MT_XEGGFX);
      p->SPMAngle(MT_XEGGFX, p->angle-(ANG45/6));
      p->SPMAngle(MT_XEGGFX, p->angle+(ANG45/6));
      p->SPMAngle(MT_XEGGFX, p->angle-(ANG45/3));
      p->SPMAngle(MT_XEGGFX, p->angle+(ANG45/3));
      break;

    case arti_blastradius:
      P_BlastRadius(p);
      break;

    case arti_poisonbag:
      ang = p->angle >> ANGLETOFINESHIFT;
      if (p->pclass == PCLASS_CLERIC)
	{
	  mo = p->mp->SpawnDActor(p->x + 16*finecosine[ang], p->y + 24*finesine[ang],
				  p->z - p->floorclip + 8*FRACUNIT, MT_POISONBAG);
	}
      else if (p->pclass == PCLASS_MAGE)
	{
	  mo = p->mp->SpawnDActor(p->x + 16*finecosine[ang], p->y + 24*finesine[ang],
				  p->z - p->floorclip + 8*FRACUNIT, MT_FIREBOMB);
	}			
      else // others
	{
	  mo = p->mp->SpawnDActor(p->x, p->y, p->z - p->floorclip + 35*FRACUNIT, MT_THROWINGBOMB);
	  if (mo)
	    {
	      mo->owner = p;
	      mo->angle = p->angle + (((P_Random() & 7)-4) << 24);
	      ang = p->aiming >> ANGLETOFINESHIFT;
	      float sp = mo->info->speed;
	      mo->Thrust(mo->angle, int(sp * finecosine[ang]));
	      mo->pz = 4*FRACUNIT + int(sp * finesine[ang]);
	      mo->px += p->px >> 1;
	      mo->py += p->py >> 1;
	      mo->tics -= P_Random() & 3;
	      mo->CheckMissileSpawn();
	    }
	}
      if (mo)
	mo->owner = p;
      break;
      
    case arti_teleportother:
      mo = p->SpawnPlayerMissile(MT_TELOTHER_FX1);
      if (mo)
	mo->owner = p;
      break;

    case arti_speed:
      return p->GivePower(pw_speed);

    case arti_boostmana:
      if (!p->GiveAmmo(am_mana1, 200))
	return p->GiveAmmo(am_mana2, 200);
      else
	p->GiveAmmo(am_mana2, 200);
      break;

    case arti_boostarmor:
      count = 0;
      for (int i = armor_armor; i < NUMARMOR; ++i)
	count += p->GiveArmor(armortype_t(i), -3.0, 1); // 1 point per armor type
      return count;

    case arti_puzzskull:
    case arti_puzzgembig:
    case arti_puzzgemred:
    case arti_puzzgemgreen1:
    case arti_puzzgemgreen2:
    case arti_puzzgemblue1:
    case arti_puzzgemblue2:
    case arti_puzzbook1:
    case arti_puzzbook2:
    case arti_puzzskull2:
    case arti_puzzfweapon:
    case arti_puzzcweapon:
    case arti_puzzmweapon:
    case arti_puzzgear1:
    case arti_puzzgear2:
    case arti_puzzgear3:
    case arti_puzzgear4:
      if (p->UsePuzzleItem(arti - arti_firstpuzzitem))
	return true;
      else
	{
	  p->player->SetMessage(text[TXT_USEPUZZLEFAILED], false);
	  return false;
	}
      break;

    case arti_fsword1:
    case arti_fsword2:
    case arti_fsword3:
    case arti_choly1:
    case arti_choly2:
    case arti_choly3:
    case arti_mstaff1:
    case arti_mstaff2:
    case arti_mstaff3:
      {
	// first check which pieces we have
	count = 0;
	for (t = p->inventory.begin(); t < p->inventory.end(); t++)
	  if (t->type >= arti_fsword1)
	    count |= (1 << (t->type - arti_fsword1));

	int wp = (arti - arti_fsword1) / 3; // 0, 1 or 2
	int mask = 7 << (3 * wp); // hack...

	// have we enough pieces to assemble a weapon?
	if ((count & mask) == mask)
	  {
	    p->weaponowned[wp + wp_quietus] = true;
	    p->pendingweapon = weapontype_t(wp + wp_quietus);
	    p->player->SetMessage(text[TXT_WEAPON_F4 + 3*wp], false);
	    S_StartAmbSound(NULL, SFX_WEAPON_BUILD); // warn all players!
	  }
	else
	  {
	    p->player->SetMessage("you don't yet have all the pieces", false);
	    return false;
	  }
      }
      break;

    default:
      return false;
    }
  return true;
}



// called by the server
void PlayerPawn::UseArtifact(artitype_t arti)
{
  extern int st_curpos;
  int n;
  vector<inventory_t>::iterator i;

  for(i = inventory.begin(); i < inventory.end(); i++) 
    if (i->type == arti)
      {
	// Found match - try to use
	if (P_UseArtifact(this, arti))
	  {
	    // Artifact was used - remove it from inventory
	    if (--(i->count) == 0)
	      {
		if (inventory.size() > 1)
		  {
		    // Used last of a type - compact the artifact list
		    inventory.erase(i);
		    // Set position markers and get next readyArtifact
		    if (--invSlot < 6)
		      if (--st_curpos < 0) st_curpos = 0;
		    n = inventory.size();
		    if (invSlot >= n)
		      invSlot = n - 1; // necessary?
		    if (invSlot < 0)
		      invSlot = 0;
		  }
		else
		  i->type = arti_none; // leave always 1 empty slot
	      }

	    S_StartSound(this, sfx_artiuse);
	    player->itemuse = 4;
	  }
	else
	  { // Unable to use artifact, advance pointer
	    n = inventory.size();
	    if (--invSlot < 6)
	      if (--st_curpos < 0) st_curpos = 0;
	      
	    if (invSlot < 0)
	      {
		invSlot = n-1;
		if (invSlot < 6)
		  st_curpos = invSlot;
		else
		  st_curpos = 6;
	      }
	  }
	break;
      }
}




bool PlayerPawn::InventoryResponder(short (*gc)[2], event_t *ev)
{
  //gc is a pointer to array[num_gamecontrols][2]
  extern int st_curpos; // TODO: what about splitscreenplayer??

  switch (ev->type)
    {
    case ev_keydown :
      if (ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1])
        {
          if (invTics)
            {
              if (--(invSlot) < 0)
                invSlot = 0;
              else if (--st_curpos < 0)
                st_curpos = 0;
            }
          invTics = 5*TICRATE;
          return true;
        }
      else if (ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
        {
          int n = inventory.size();

          if (invTics)
            {
              if (++(invSlot) >= n)
                invSlot = n-1;
              else if (++st_curpos > 6)
                st_curpos = 6;
            }
          invTics = 5*TICRATE;
          return true;
        }
      else if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1])
        {
          if (invTics)
            invTics = 0;
          else if (inventory[invSlot].count > 0)
	    player->cmd.item = inventory[invSlot].type + 1;

          return true;
        }
      break;

    case ev_keyup:
      if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1] ||
          ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1] ||
          ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
        return true;
      break;

    default:
      break; // shut up compiler
    }
  return false;
}
