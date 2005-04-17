// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2005 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//
// $Log$
// Revision 1.6  2005/04/17 17:59:00  smite-meister
// netcode
//
// Revision 1.3  2004/11/09 20:38:51  smite-meister
// added packing to I/O structs
//
// Revision 1.1  2004/10/17 01:57:05  smite-meister
// bots!
//
// Revision 1.2  2002/09/27 16:40:07  tonyd
// First commit of acbot
//
//-----------------------------------------------------------------------------

/// \file
/// \brief ACBot implementation

#include <math.h>
#include <stdlib.h>

#include "command.h"
#include "cvars.h"

#include "d_ticcmd.h"
#include "b_path.h"
#include "acbot.h"

#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_pawn.h"

#include "d_items.h"
#include "p_maputl.h"
#include "r_defs.h"

#include "tables.h"
//#include "m_dll.h"

#ifndef M_PI
# define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

#define ANG5 (ANG90/18)


/*
// TODO eliminate dependence on various externs, convert into a DLL plugin
DATAEXPORT dll_info_t dll_info = {0, 1, "Doom Legacy ACBot plugin"};
*/


static fixed_t botforwardmove[2] = {50, 100};
static fixed_t botsidemove[2]    = {48, 80};
static fixed_t botangleturn[4]   = {500, 1000, 2000, 4000};


/// A target Actor for ACBot
struct ai_target_t
{
  Actor   *a;
  fixed_t  dist;
};

// temp variables
static ai_target_t cMissile; // closest dangerous missile (must be avoided)

static ai_target_t cEnemy;       // closest directly visible enemy
static ai_target_t cUnseenEnemy; // closest nonvisible enemy (node-reachable)

static ai_target_t fTeammate;       // furthest directly reachable teammate
static ai_target_t cUnseenTeammate; // closest not directly reachable teammate (node-reachable)

static ai_target_t bItem; // best directly reachable AND visible item
static double  bItemWeight;
static ai_target_t bUnseenItem; // best non-visible item (node-reachable)
static double  bUnseenItemWeight;

static int bWeaponValue; // value of the best currently usable weapon
static bool HaveWeaponFor[NUMAMMO]; // does the bot have a weapon for the given ammotype?


//=================================================================
//     Simple straight-line reachability checks
//=================================================================
static Actor	*bot, *goal;
static sector_t *last_sector;

/// returns true if the intercepting object can be bypassed
static bool PTR_QuickReachable(intercept_t *in)
{
  if (in->isaline)
    {
      line_t *line = in->line;

      if (!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING))
	return false; //Cannot continue.
      else
	{
	  // determine the next sector to be traversed
	  sector_t *s = (line->backsector == last_sector) ? line->frontsector : line->backsector;
	  fixed_t ceilingheight = s->ceilingheight;
	  fixed_t floorheight = s->floorheight;

	  if (((floorheight <= last_sector->floorheight + 37*FRACUNIT) ||
	       ((floorheight <= last_sector->floorheight + 45*FRACUNIT) &&
		(last_sector->floortype != FLOOR_WATER))) && // can we jump there?
	      ((ceilingheight == floorheight && line->special) ||
	       (ceilingheight - floorheight >= bot->height))) // do we fit?
	    {
	      last_sector = s;
	      return true;
	    }
	  else
	    return false;
	}
    }
  else
    {
      Actor *thing = in->thing;
      if (thing != bot && thing != goal && (thing->flags & MF_SOLID))
	return false;
    }

  return true;
}


// Checks if the bot can get to the actor 'goal' by simply
// walking towards it, using "doors" and possibly jumping.
bool ACBot::QuickReachable(Actor *g)
{
  bot = pawn;
  goal = g;
  last_sector = pawn->subsector->sector;

  //#ifdef FIXROVERBUGS
  // Bots shouldn't try to get stuff that's on a 3dfloor they can't get to. SSNTails 06-10-2003
  if (pawn->subsector == goal->subsector)
    for (ffloor_t *rover = last_sector->ffloors; rover; rover = rover->next)
      {
        if (!(rover->flags & FF_SOLID) || !(rover->flags & FF_EXISTS))
	  continue;

	if (*rover->topheight <= pawn->z && goal->z < *rover->topheight)
	  return false;

	if (*rover->bottomheight >= pawn->z + pawn->height
	    && goal->z > *rover->bottomheight)
	  return false;
      }
  //#endif

  return mp->PathTraverse(pawn->x, pawn->y, goal->x, goal->y,
			  PT_ADDLINES|PT_ADDTHINGS, PTR_QuickReachable);
}


// Checks if the bot can get to the point (x,y) by simply
// walking towards it, using "doors" and possibly jumping.
bool ACBot::QuickReachable(fixed_t x, fixed_t y)
{
  bot = pawn;
  goal = NULL;
  last_sector = pawn->subsector->sector;

  return mp->PathTraverse(pawn->x, pawn->y, x, y, PT_ADDLINES|PT_ADDTHINGS, PTR_QuickReachable);
}





//=================================================================
//   ACBot basics
//=================================================================

ACBot::ACBot(int sk)
{
  skill = sk;
  num_weapons = 0;

  avoidtimer = 0;
  blockedcount = 0;
  strafetimer = 0;
  weaponchangetimer = 0;

  lastTarget = NULL;
  destination = NULL;
}


/// Clears the current path
void ACBot::ClearPath()
{
#ifdef SHOWBOTPATH
  while (!path.empty())
    {
      if (path.front()->mo)
	path.front()->mo->Remove();

      path.pop_front();
    }
#endif
  path.clear();
}




//=================================================================
//  ACBot pickup AI
//=================================================================

enum ai_item_e
{
  F_WEAPON,
  F_AMMO,
  F_HEAL,
  F_ARMOR,
  F_KEY,
  F_POWERUP,
};

/// defines pickup values for the AI
struct ai_item_t
{
  mobjtype_t mtype; ///< item type
  float weight;     ///< value
  int type;         ///< classification
  int misc; ///< different enum/int values depending on type
};


static ai_item_t item_ai[] =
{
  // weapon (if not in possession, x-2 if having adequate weapon and ammo for it, x if not.
  // if already have it and need ammo and teammates don't need it, 3
  // good: chaingun, missile, plasma, supershotgun
  {MT_SHAINSAW,     3, F_WEAPON, wp_chainsaw},
  {MT_SHOTGUN,      6, F_WEAPON, wp_shotgun},
  {MT_SUPERSHOTGUN, 7, F_WEAPON, wp_supershotgun},
  {MT_CHAINGUN,     6, F_WEAPON, wp_chaingun}, // 6 5 3 ???
  {MT_ROCKETLAUNCH, 7, F_WEAPON, wp_missile},
  {MT_PLASMAGUN,    7, F_WEAPON, wp_plasma},
  {MT_BFG9000,      7, F_WEAPON, wp_bfg},
  {MT_WGAUNTLETS,   3, F_WEAPON, wp_gauntlets},
  {MT_WCROSSBOW,    6, F_WEAPON, wp_crossbow},
  {MT_WBLASTER,     6, F_WEAPON, wp_blaster},
  {MT_WPHOENIXROD,  7, F_WEAPON, wp_phoenixrod},
  {MT_WSKULLROD,    7, F_WEAPON, wp_skullrod},
  {MT_WMACE,        7, F_WEAPON, wp_mace},
  {MT_FW_AXE,       6, F_WEAPON, wp_timons_axe},
  {MT_CW_SERPSTAFF, 6, F_WEAPON, wp_serpent_staff},
  {MT_MW_CONE,      6, F_WEAPON, wp_cone_of_shards},
  {MT_FW_HAMMER,    7, F_WEAPON, wp_hammer_of_retribution},
  {MT_CW_FLAME,     7, F_WEAPON, wp_firestorm},
  {MT_MW_LIGHTNING, 7, F_WEAPON, wp_arc_of_death},
  {MT_FW_SWORD1,    8, F_WEAPON, wp_quietus},
  {MT_FW_SWORD2,    8, F_WEAPON, wp_quietus},
  {MT_FW_SWORD3,    8, F_WEAPON, wp_quietus},
  {MT_CW_HOLY1,     8, F_WEAPON, wp_wraithverge},
  {MT_CW_HOLY2,     8, F_WEAPON, wp_wraithverge},
  {MT_CW_HOLY3,     8, F_WEAPON, wp_wraithverge},
  {MT_MW_STAFF1,    8, F_WEAPON, wp_bloodscourge},
  {MT_MW_STAFF2,    8, F_WEAPON, wp_bloodscourge},
  {MT_MW_STAFF3,    8, F_WEAPON, wp_bloodscourge},

  // ammo (if dry on this type and have a weapon for it and using an inferior weapon, x, otherwise if room to take it, x-3
  {MT_CLIP,       6,   F_AMMO, am_clip},
  {MT_AMMOBOX,    6.5, F_AMMO, am_clip},
  {MT_SHELL,      6,   F_AMMO, am_shell},
  {MT_SHELLBOX,   6.5, F_AMMO, am_shell},
  {MT_CELL,       6,   F_AMMO, am_cell},
  {MT_CELLPACK,   6.5, F_AMMO, am_cell},
  {MT_ROCKETAMMO, 6,   F_AMMO, am_misl},
  {MT_ROCKETBOX,  6.5, F_AMMO, am_misl},
  {MT_BACKPACK,   7,   F_AMMO, am_clip},
  {MT_AMGWNDWIMPY, 6,   F_AMMO, am_goldwand},
  {MT_AMGWNDHEFTY, 6.5, F_AMMO, am_goldwand},
  {MT_AMCBOWWIMPY, 6,   F_AMMO, am_crossbow},
  {MT_AMCBOWHEFTY, 6.5, F_AMMO, am_crossbow},
  {MT_AMBLSRWIMPY, 6,   F_AMMO, am_blaster},
  {MT_AMBLSRHEFTY, 6.5, F_AMMO, am_blaster},
  {MT_AMSKRDWIMPY, 6,   F_AMMO, am_skullrod},
  {MT_AMSKRDHEFTY, 6.5, F_AMMO, am_skullrod},
  {MT_AMPHRDWIMPY, 6,   F_AMMO, am_phoenixrod},
  {MT_AMPHRDHEFTY, 6.5, F_AMMO, am_phoenixrod},
  {MT_AMMACEWIMPY, 6,   F_AMMO, am_mace},
  {MT_AMMACEHEFTY, 6.5, F_AMMO, am_mace},
  {MT_BAGOFHOLDING, 7,  F_AMMO, am_goldwand},
  {MT_MANA1, 6,   F_AMMO, am_mana1},
  {MT_MANA2, 6,   F_AMMO, am_mana2},
  {MT_MANA3, 6.5, F_AMMO, am_mana1},

#define MAXSOUL  200
#define MAXARMOR 200

  // medication, take if you need or are selfish, low health => more interest, low skill => less interest
  {MT_MEGA,          9, F_HEAL, MAXSOUL}, // MAXARMOR too...
  {MT_BERSERKPACK,   9, F_HEAL, MAXSOUL}, // pw_strength too..., also, more skill => less interest?
  {MT_SOULSPHERE,    8, F_HEAL, MAXSOUL},
  {MT_MEDI,          6, F_HEAL, MAXSOUL/2},
  {MT_STIM,          6, F_HEAL, MAXSOUL/2},
  {MT_HEALTHBONUS,   1, F_HEAL, MAXSOUL},
  {MT_ARTISUPERHEAL, 8, F_HEAL, MAXSOUL},
  {MT_HEALINGBOTTLE, 1, F_HEAL, MAXSOUL/2},

  // armor, take if you can (can => need), low skill => less interest
  {MT_BLUEARMOR,   8, F_ARMOR, MAXARMOR},
  {MT_GREENARMOR,  5, F_ARMOR, MAXARMOR/2},
  {MT_ARMORBONUS,  1, F_ARMOR, MAXARMOR},
  {MT_ITEMSHIELD2, 8, F_ARMOR, MAXARMOR},
  {MT_ITEMSHIELD1, 5, F_ARMOR, MAXARMOR/2},
  {MT_ARMOR_1,     5, F_ARMOR, 15}, // roughly
  {MT_ARMOR_2,     6, F_ARMOR, 15},
  {MT_ARMOR_3,     5, F_ARMOR, 15},
  {MT_ARMOR_4,     6, F_ARMOR, 15},

  // keys (take it if you don't already have it)
  {MT_BLUECARD,    5, F_KEY, it_bluecard},
  {MT_YELLOWCARD,  5, F_KEY, it_yellowcard},
  {MT_REDCARD,     5, F_KEY, it_redcard},
  {MT_BLUESKULL,   5, F_KEY, it_blueskull},
  {MT_YELLOWSKULL, 5, F_KEY, it_yellowskull},
  {MT_REDSKULL,    5, F_KEY, it_redskull},
  {MT_BKEY,        5, F_KEY, it_bluecard},
  {MT_CKEY,        5, F_KEY, it_yellowcard},
  {MT_AKEY,        5, F_KEY, it_redcard},
  {MT_KEY1,        5, F_KEY, it_key_1},
  {MT_KEY2,        5, F_KEY, it_key_2},
  {MT_KEY3,        5, F_KEY, it_key_3},
  {MT_KEY4,        5, F_KEY, it_key_4},
  {MT_KEY5,        5, F_KEY, it_key_5},
  {MT_KEY6,        5, F_KEY, it_key_6},
  {MT_KEY7,        5, F_KEY, it_key_7},
  {MT_KEY8,        5, F_KEY, it_key_8},
  {MT_KEY9,        5, F_KEY, it_key_9},
  {MT_KEYA,        5, F_KEY, it_key_A},
  {MT_KEYB,        5, F_KEY, it_key_B},

  // doom powerups, take if you need or are selfish
  {MT_INV, 10, F_POWERUP, pw_invulnerability},
  {MT_INS,  9, F_POWERUP, pw_invisibility},
  {MT_ARTIINVULNERABILITY, 10, F_POWERUP, pw_invulnerability},
  {MT_ARTIINVISIBILITY,     9, F_POWERUP, pw_invisibility},
  {MT_RADSUIT, 5, F_POWERUP, pw_ironfeet},

  // TODO inventory items (acbot must also be made to use them!)

  {MT_NONE}
};







void ACBot::LookForThings()
{
  cMissile.a = NULL;
  cMissile.dist = MAXINT;

  cEnemy.a = NULL;
  cEnemy.dist = MAXINT;
  cUnseenEnemy.a = NULL;
  cUnseenEnemy.dist = MAXINT;

  fTeammate.a = NULL;
  fTeammate.dist = 0;
  cUnseenTeammate.a = NULL;
  cUnseenTeammate.dist = MAXINT;

  bItem.a = NULL;
  bItem.dist = MAXINT;
  bItemWeight = 0.0;
  bUnseenItem.a = NULL;
  bUnseenItem.dist = MAXINT;
  bUnseenItemWeight = 0.0;

  // search through the list of all thinkers

  for (Thinker *th = mp->thinkercap.Next(); th != &mp->thinkercap; th = th->Next())
    {
      if (!th->IsOf(Actor::_type))
	continue; // keep looking

      Actor *actor = (Actor *)th;

      mobjtype_t type = actor->IsOf(DActor::_type) ? ((DActor *)actor)->type : MT_NONE;
      fixed_t dist = P_AproxDistance(pawn->x - actor->x, pawn->y - actor->y);
      bool enemyFound = false;
      SearchNode_t *node;

      if ((actor->flags & MF_COUNTKILL || type == MT_SKULL || type == MT_BARREL) &&
	  (actor->flags & MF_SOLID))
	enemyFound = true; // a live monster
      else if (actor->flags & MF_NOTMONSTER && actor->flags & MF_SOLID && actor != pawn)
	{
	  // a playerpawn or equivalent, but not ours
	  if (actor->team != subject->team)
	    enemyFound = true;
	  else
	    {
	      // a teammate (TODO prefer "unseen" humans to "seen" bots)
	      if (QuickReachable(actor)) // is there a direct route to the teammate?
		{
		  if ((dist > fTeammate.dist))
		    {
		      fTeammate.dist = dist;
		      fTeammate.a = actor;
		    }
		}
	      else
		{
		  node = mp->botnodes->GetNodeAt(actor->x, actor->y);
		  if (node && dist < cUnseenTeammate.dist)
		    {
		      cUnseenTeammate.dist = dist;
		      cUnseenTeammate.a = actor;
		    }
		}
	    }
	}
      else if ((actor->flags & MF_MISSILE) && actor->owner != pawn) // a threatening missile
	{
	  // see if the missile is heading my way, if the missile will be closer to me, next tick
	  //then its heading at least somewhat towards me, so better dodge it
	  // TODO dot product...
	  if (P_AproxDistance(pawn->x + pawn->px - (actor->x + actor->px), pawn->y+pawn->py - (actor->y+actor->py)) < dist)
	    {
	      //if its the closest missile and its reasonably close I should try and avoid it
	      if (dist && (dist < cMissile.dist) && (dist <= 300*FRACUNIT))
		{
		  cMissile.dist = dist;
		  cMissile.a = actor;
		}
	    }
	}
      else if ((actor->flags & MF_SPECIAL) || (actor->flags & MF_DROPPED)) // most likely a pickup
	{
	  float weight = 0.0;
	  bool selfish = cv_deathmatch.value;
	  for (ai_item_t *t = item_ai; t->mtype != MT_NONE; t++)
	    if (t->mtype == type)
	      {
		switch (t->type)
		  {
		  case F_WEAPON:
		    if (!pawn->weaponowned[t->misc])		
		      {
			weight = t->weight;
			if (bWeaponValue >= 50)
			  weight -= 2;
		      }
		    else if (selfish || (actor->flags & MF_DROPPED))
		      {
			int atype = wpnlev1info[t->misc].ammo;
			if (pawn->ammo[atype] < pawn->maxammo[atype])
			  weight = 3;
		      }
		    break;

		  case F_AMMO:
		    if (!pawn->ammo[t->misc] && HaveWeaponFor[t->misc] && bWeaponValue < 10) // fist, chainsaw
		      weight = t->weight;
		    else if (pawn->ammo[t->misc] < pawn->maxammo[t->misc])
		      weight = t->weight - 3;
		    break;

		  case F_HEAL:
		    if (selfish)
		      weight = t->weight;
		    else if (pawn->health < t->misc)
		      {
			weight = t->weight + (skill - sk_nightmare); // harder skill => health is valued higher
			if (pawn->health >= 80) weight -= 3;
			else if (pawn->health >= 60) weight -= 2;
			else if (pawn->health >= 50) weight -= 1;
		
			if (weight < 1)
			  weight = 1;
		      }
		    break;

		  case F_ARMOR:
		    if (selfish || (pawn->armorpoints[0] < t->misc))
		      weight = t->weight;
		    break;

		  case F_KEY:
		    if (!(pawn->keycards & t->misc))
		      weight = t->weight;
		    break;

		  case F_POWERUP:
		    if (selfish || !pawn->powers[t->misc])
		      weight = t->weight;
		    break;
		  }
		break;
	      }

	  if (mp->CheckSight(pawn, actor) && QuickReachable(actor))
	    {
	      if (weight > bItemWeight || (weight == bItemWeight && dist < bItem.dist))
		{
		  bItem.a = actor;
		  bItem.dist = dist;
		  bItemWeight = weight;
		}
	    }
	  else
	    {
	      // item is not gettable atm, may use a search later to find a path to it
	      node = mp->botnodes->GetNodeAt(actor->x, actor->y);
	      if (node  //&& P_AproxDistance(posX2x(node->x) - actor->x, posY2y(node->y) - actor->y) < (BOTNODEGRIDSIZE << 1)
		  && (weight > bUnseenItemWeight || (weight == bUnseenItemWeight && dist < bUnseenItem.dist)))
		{
		  bUnseenItem.a = actor;
		  bUnseenItem.dist = dist;
		  bUnseenItemWeight = weight;
		  //CONS_Printf("best item set to x:%d y:%d for type:%d\n", actor->x>>FRACBITS, actor->y>>FRACBITS, actor->type);
		}

	      //if (!node)
	      // CONS_Printf("could not find a node here x:%d y:%d for type:%d\n", actor->x>>FRACBITS, actor->y>>FRACBITS, actor->type);
	    }
	}

      if (enemyFound)
	{
	  if (mp->CheckSight(pawn, actor))
	    {
	      // TODO prefer player enemies to monster enemies
	      // 
	      if (dist < cEnemy.dist || (actor->flags & MF_NOTMONSTER && !(cEnemy.a->flags & MF_NOTMONSTER)))
		{
		  cEnemy.dist = dist;
		  cEnemy.a = actor;
		}
	    }
	  else
	    {
	      node = mp->botnodes->GetNodeAt(actor->x, actor->y);
	      if (node &&
		  (dist < cUnseenEnemy.dist ||
		   (actor->flags & MF_NOTMONSTER && !(cEnemy.a->flags & MF_NOTMONSTER))))
		{
		  cUnseenEnemy.dist = dist;
		  cUnseenEnemy.a = actor;
		}
	    }
	}
    }
}




//=================================================================
//    Bots using special linedefs
//=================================================================

// This function looks through the sector the bot is in and, one sector level outwards,
// very inefficient cause searches of sectors are done multiple times
// when a sector has many linedefs between a single sector-sector boundary
//  must fix this, perhaps use the visited bool
// maybe should do search through thes switches array instead
bool ACBot::LookForSpecialLine(fixed_t *x, fixed_t *y)
{
  msecnode_t *insectornode = pawn->touching_sectorlist;
  while (insectornode)
    {
      sector_t *sec = insectornode->m_sector;
      for (int i = 0; i < sec->linecount; i++)
	{
	  line_t *edge = sec->lines[i];
	  int spac = GET_SPAC(edge->flags);
	  int special = edge->special;
	  //sector_t *specialsector = (sec == edge->frontsector) ? edge->backsector : edge->frontsector;
	  if (!(edge->flags & ML_REPEAT_SPECIAL) && (spac == SPAC_USE || spac == SPAC_PASSUSE)
	      && (special == 11 || special == 21 || special == 200))
	    //(!specialsector || !specialsector->ceilingdata))
	    //!(line->flags & ML_TWOSIDED) || (line->flags & ML_BLOCKING))
	    //(edge->frontsector == sec) //if its a pressable switch
	    {
	      *x = (edge->v1->x + edge->v2->x) / 2;
	      *y = (edge->v1->y + edge->v2->y) / 2;

	      return true;
	    }
	  else if (edge->sidenum[1] >= 0) //if its a double sided sector
	    {
	      sector_t *sector;
	      if (edge->frontsector == sec)
		sector = edge->backsector;
	      else
		sector = edge->frontsector;

	      for (int j = 0; j < sector->linecount; j++)
		{
		  edge = sector->lines[j];
		  //specialsector = (sector == edge->frontsector) ? edge->backsector : edge->frontsector;
		  spac = GET_SPAC(edge->flags);
		  special = edge->special;
		  //sector_t *specialsector = (sec == edge->frontsector) ? edge->backsector : edge->frontsector;

		  if (!(edge->flags & ML_REPEAT_SPECIAL) && (spac == SPAC_USE || spac == SPAC_PASSUSE)
		      && (special == 11 || special == 21 || special == 200))
		    {
		      *x = (edge->v1->x + edge->v2->x) / 2;
		      *y = (edge->v1->y + edge->v2->y) / 2;

		      return true;
		    }
		}
	    }
	}
      insectornode = insectornode->m_snext;
    }

  return false;
}




//=================================================================
//   ACBot combat AI
//=================================================================

/// Weapon information for the AI system
struct ai_weapon_t
{
  byte       type;       // flags
  byte       value;      // preference when changing weapon (not exactly the original ones!)
  mobjtype_t missile;    // or MT_NONE
  fixed_t    dangerdist; // blast radius
};

enum ai_weapon_e
{
  MELEE   = 0,
  INSTANT = 1,
  MISSILE = 2,
  TYPEMASK = 0x3,
  AF  = 0x4 // use area fire (shoot in the general direction of the enemy)
};


static ai_weapon_t ai_weapon_data[NUMWEAPONS] =
{
  /*
  wp_fist, wp_pistol, wp_shotgun, wp_chaingun,
  wp_missile,  wp_plasma,  wp_bfg,
  wp_chainsaw, wp_supershotgun
  */
  {0, 0, MT_NONE, 0}, {1|AF, 0, MT_NONE, 0}, {1, 30, MT_NONE, 0}, {1|AF, 50, MT_NONE, 0},
  {2, 50, MT_ROCKET, 100*FRACUNIT}, {2|AF, 50, MT_PLASMA, 0}, {2, 20, MT_BFG, 0},
  {0, 0, MT_NONE, 0}, {1, 55, MT_NONE, 0},
  /*
  wp_staff, wp_gauntlets, wp_goldwand,
  wp_crossbow, wp_blaster, wp_phoenixrod,
  wp_skullrod, wp_mace, wp_beak,
  */
  {0, 0, MT_NONE, 0}, {0, 0, MT_NONE, 0}, {1|AF, 0, MT_NONE, 0},
  {2, 35, MT_CRBOWFX1, 0}, {1|AF, 50, MT_NONE, 0}, {2, 50, MT_PHOENIXFX1, 100*FRACUNIT},
  {2|AF, 55, MT_HORNRODFX1, 0}, {2|AF, 50, MT_MACEFX1, 0}, {0, 0, MT_NONE, 0},

  /*
  wp_fpunch, wp_cmace, wp_mwand,
  wp_timons_axe, wp_serpent_staff, wp_cone_of_shards,
  wp_hammer_of_retribution, wp_firestorm, wp_arc_of_death,
  wp_quietus, wp_wraithverge, wp_bloodscourge, wp_snout
  */
  {0, 10, MT_NONE, 0}, {0, 0, MT_NONE, 0}, {1|AF, 20, MT_NONE, 0},
  {0, 30, MT_NONE, 0}, {2, 30, MT_NONE, 0}, {2, 30, MT_NONE, 0},
  {2, 40, MT_NONE, 0}, {2, 40, MT_NONE, 0}, {2, 40, MT_NONE, 0},
  {2, 50, MT_NONE, 0}, {2, 50, MT_NONE, 0}, {2, 50, MT_NONE, 0}, {0, 0, MT_NONE, 0}
};



void ACBot::AvoidMissile(const Actor *missile)
{
  fixed_t delta = pawn->angle - R_PointToAngle2(pawn->x, pawn->y, missile->x, missile->y);

  if (delta >= 0)
    cmd->side = -botsidemove[1];
  else
    cmd->side = botsidemove[1];
}


void ACBot::ChangeWeapon()
{
  bool weapon_usable[NUMWEAPONS]; // good for at least one shot
  int i, n = 0;

  bWeaponValue = 0;

  for (i=0; i<NUMAMMO; i++)
    HaveWeaponFor[i] = false;

  for (i=0; i<NUMWEAPONS; i++)
    {
      ammotype_t at = pawn->weaponinfo[i].ammo;
      HaveWeaponFor[at] = true;

      weapon_usable[i] = pawn->weaponowned[i] &&
	(at == am_noammo || pawn->ammo[at] >= pawn->weaponinfo[i].ammopershoot);
      // should not use fist? ((i == wp_fist) && p->powers[pw_strength]))

      if (weapon_usable[i])
	{
	  n++;
	  if (ai_weapon_data[i].value > bWeaponValue)
	    bWeaponValue = ai_weapon_data[i].value;
	}
    }

  // should we change?
  if (!weaponchangetimer || !weapon_usable[pawn->readyweapon] || n != num_weapons)
    {
      weapon_usable[pawn->readyweapon] = false; // must change

      int sum = 0;
      for (i=0; i<NUMWEAPONS; i++)
	if (weapon_usable[i])
	  sum += ai_weapon_data[i].value;

      if (sum > 0)
	{
	  int r = (std::rand() * sum) / RAND_MAX;

	  for (i=0; i<NUMWEAPONS; i++)
	    if (weapon_usable[i] && r < ai_weapon_data[i].value)
	      break;
	    else
	      r -= ai_weapon_data[i].value;

	  cmd->buttons &= ~ticcmd_t::BT_ATTACK; // stop rocket from jamming;
	}
      else if (weapon_usable[wp_pistol])
	i = wp_pistol;
      else if (weapon_usable[wp_chainsaw] && !pawn->powers[pw_strength]) // has chainsaw, and no berserk
	i = wp_chainsaw;
      else // resort to fists, if have powered fists, better with fists than with chainsaw
	i = wp_fist;

      cmd->buttons |= (i + 1) << ticcmd_t::WEAPONSHIFT;
      weaponchangetimer = (std::rand() << 11) / RAND_MAX + 1000; // how long until I next change my weapon
    }
  else
    weaponchangetimer--;

  if (n != num_weapons)
    cmd->buttons &= ~ticcmd_t::BT_ATTACK; //stop rocket from jamming;
  num_weapons = n;

  //CONS_Printf("weaponchangetimer is %d\n", weaponchangetimer);
}



void ACBot::TurnTowardsPoint(fixed_t x, fixed_t y)
{
  fixed_t ang = R_PointToAngle2(pawn->x, pawn->y, x, y);
  fixed_t delta = ang - pawn->angle;

  int botspeed;

  if (abs(delta) < (ANG45 >> 2))
    botspeed = 0;
  else if (abs(delta) < ANG45)
    botspeed = 2; // 1
  else
    botspeed = 3; // 1

  if (abs(delta) < ANG5)
    cmd->yaw = ang >> FRACBITS; //perfect aim
  else if (delta > 0)
    cmd->yaw += botangleturn[botspeed];
  else
    cmd->yaw -= botangleturn[botspeed];
}



void ACBot::AimWeapon()
{
  ai_weapon_t *wdata = &ai_weapon_data[pawn->readyweapon];

  mobjtype_t miss = wdata->missile;

  float m_speed = 0;
  if (miss != MT_NONE)
    m_speed = mobjinfo[miss].speed;

  if (skill < sk_hard)
    m_speed = 0; // no leading

  Actor *dest = cEnemy.a;
  fixed_t dist = P_AproxDistance(dest->x - pawn->x, dest->y - pawn->y);


  if (dist > wdata->dangerdist)
    {
      fixed_t nx, ny, nz;

      if (m_speed > 0)
	{
	  fixed_t t = fixed_t(dist / m_speed);
	  t = fixed_t(P_AproxDistance(dest->x + dest->px * t - pawn->x,
				      dest->y + dest->py * t - pawn->y) / m_speed) + 4;

	  do
	    {
	      t -= 4;
	      if (t < 0)
		t = 0;
	      nx = dest->x + dest->px * t;
	      ny = dest->y + dest->py * t;
	      nz = dest->z + dest->pz * t;
          //} while (!mp->CheckSight2(pawn, dest, nx, ny, nz) && (t > 0));
	    } while (false); // FIXME

	  subsector_t *sec = mp->R_PointInSubsector(nx, ny);
	  if (!sec)
	    sec = dest->subsector;

	  if (nz < sec->sector->floorheight)
	    nz = sec->sector->floorheight;
	  else if (nz > sec->sector->ceilingheight)
	    nz = sec->sector->ceilingheight - dest->height;
	}
      else
	{
	  nx = dest->x;
	  ny = dest->y;
	  nz = dest->z;
	}

      angle_t angle, realAngle;
      realAngle = angle = R_PointToAngle2(pawn->x, pawn->y, nx, ny);

      cmd->pitch = int(atan((nz - pawn->z + (dest->height - pawn->height)/2) / double(dist)) * (ANG180 / M_PI)) >> FRACBITS;

      int spread = (rand() - rand())*ANG45 / RAND_MAX; // was P_SignedRandom()<<21;

      if (P_AproxDistance(dest->px, dest->py) > 8*FRACUNIT) //enemy is moving reasonably fast, so not perfectly acurate
	{
	  if (dest->flags & MF_SHADOW)
	    angle += spread << 2;
	  else if (!m_speed)
	    angle += spread << 1;
	}
      else
	{
	  if (dest->flags & MF_SHADOW)
	    angle += spread << 1;
	  else if (!m_speed)
	    angle += spread;
	}


      int botspeed;

      int delta = angle - pawn->angle;
      if (abs(delta) < (ANG45 >> 1))
	botspeed = 0;
      else if (abs(delta) < ANG45)
	botspeed = 1;
      else
	botspeed = 3;

      if (abs(delta) < ANG45)
	{
	  if (wdata->type & AF)
	    cmd->buttons |= ticcmd_t::BT_ATTACK;

	  if (abs(delta) <= ANG5)
	    {
	      if (skill <= sk_medium)  // check skill, if anything but nightmare bot aim is imperfect
		cmd->yaw = angle>>FRACBITS;	// not so perfect aim
	      else
		cmd->yaw = realAngle>>FRACBITS; // perfect aim

	      delta = 0;
	      /*if (p->readyweapon == wp_missile)
		{
		Actor*     tempRocket = Z_Malloc (sizeof(*tempRocket), PU_LEVEL, NULL);
		mobjinfo_t* info;

		info = &mobjinfo[MT_ROCKET];
		tempRocket->type = MT_ROCKET;
		tempRocket->info = info;
		tempRocket->x = pawn->x;
		tempRocket->y = pawn->y;
		tempRocket->z = pawn->z + 1835008;
		tempRocket->radius = info->radius;
		tempRocket->height = info->height;
		if  (B_ReachablePoint(p, R_PointInSubsector(nx, ny)->sector, nx, ny))
		cmd->buttons |= ticcmd_t::BT_ATTACK;
 
		//B_ChangeWeapon(p);
		Z_Free(tempRocket);
		}
		else*/
	      cmd->buttons |= ticcmd_t::BT_ATTACK;
	    }
	}

      if (delta > 0)
	cmd->yaw += botangleturn[botspeed]; //turn right
      else if (delta < 0)
	cmd->yaw -= botangleturn[botspeed]; //turn left
    }
}


//=================================================================
// Main ACBot AI
//=================================================================

void ACBot::BuildInput(PlayerInfo *p, int elapsed)
{
  subject = p;
  pawn = p->pawn;
  mp = p->mp;
  cmd = &p->cmd;

  bool use_down = (cmd->buttons & ticcmd_t::BT_USE);
  // needed so bot doesn't hold down use before reaching a switch

  cmd->Clear();

  if (subject->playerstate != PST_ALIVE)
    {
      cmd->buttons |= ticcmd_t::BT_USE; // I want to respawn
      return;
    }

  int botspeed = 1;
  fixed_t forwardmove = 0, sidemove = 0;

  cmd->yaw = pawn->angle >> 16;
  cmd->pitch = 0; //pawn->aiming >> 16;

  ChangeWeapon();
  LookForThings();


  if (avoidtimer)
    {
      avoidtimer--;
      if (pawn->eflags & MFE_UNDERWATER)
	{
	  forwardmove = botforwardmove[1];
	  cmd->buttons |= ticcmd_t::BT_JUMP;
	}
      else
	{
	  if (cmd->forward > 0)
	    forwardmove = -botforwardmove[1];
	  else
	    forwardmove = botforwardmove[1];
	  sidemove = botsidemove[1];
	}
    }
  else
    {
      int dist; // how far away (in FRACUNITs) is the enemy or the wanted thing

      if (bItem.a && (bItemWeight > 5 || !cEnemy.a))
	{
	  // If a nearby item has a good weight, get it no matter what. Else only if we have no target/enemy.
	  dist = P_AproxDistance(pawn->x - bItem.a->x, pawn->y - bItem.a->y) >> FRACBITS;
	  if (dist > 64)
	    botspeed = 1;
	  else
	    botspeed = 0;
	  TurnTowardsPoint(bItem.a->x, bItem.a->y);
	  forwardmove = botforwardmove[botspeed];
	  if (((bItem.a->floorz - pawn->z) >> FRACBITS) > 24 && dist <= 100)
	    cmd->buttons |= ticcmd_t::BT_JUMP;
	}
      else if (cEnemy.a)
	{
	  // attack the enemy
	  weapontype_t w = pawn->readyweapon;

	  //CONS_Printf("heading for an enemy\n");
	  dist = P_AproxDistance(pawn->x - cEnemy.a->x, pawn->y - cEnemy.a->y) >> FRACBITS;
	  if (dist > 300 || (ai_weapon_data[w].type & TYPEMASK) == MELEE)
	    forwardmove = botforwardmove[botspeed];
	  if (ai_weapon_data[w].dangerdist > 50*FRACUNIT && dist < 400)
	    forwardmove = -botforwardmove[botspeed];

	  PlayerPawn *pp = cEnemy.a->IsOf(PlayerPawn::_type) ? (PlayerPawn *)cEnemy.a : NULL;

	  // if we are at close range or player enemy has a long range weapon, better strafe
	  // skill setting determines when bot will start strafing
	  int ai_strafedist[5] = { 32, 150, 150, 350, 650 };

	  if (dist <= ai_strafedist[skill] ||
	      (pp && skill >= sk_medium && (ai_weapon_data[pp->readyweapon].type & TYPEMASK) == INSTANT))
	    sidemove = botsidemove[botspeed];

	  AimWeapon();
	  lastTarget = cEnemy.a;
	  lastTargetX = cEnemy.a->x;
	  lastTargetY = cEnemy.a->y;
	}
      else
	{
	  // nothing urgent to do

	  fixed_t x, y;
	  if (LookForSpecialLine(&x, &y) && QuickReachable(x, y))
	    {
	      // look for an unactivated switch/door
	      //CONS_Printf("found a special line\n");
	      TurnTowardsPoint(x, y);
	      if (P_AproxDistance(pawn->x - x, pawn->y - y) <= USERANGE)
		{
		  if (!use_down)
		    cmd->buttons |= ticcmd_t::BT_USE;
		}
	      else
		forwardmove = botforwardmove[1];
	    }
	  else if (fTeammate.a)
	    {
	      // keep together with team (go to furthest visible teammate)
	      dist = P_AproxDistance(pawn->x - fTeammate.a->x, pawn->y - fTeammate.a->y) >> FRACBITS;
	      if (dist > 100)
		{
		  TurnTowardsPoint(fTeammate.a->x, fTeammate.a->y);
		  forwardmove = botforwardmove[botspeed];
		}

	      lastTarget = fTeammate.a;
	      lastTargetX = fTeammate.a->x;
	      lastTargetY = fTeammate.a->y;
	    }
	  else if (lastTarget && (lastTarget->flags & MF_SOLID))
	    {
	      // nothing else to do, go where the last enemy/teamate was seen
	      if ((pawn->px == 0 && pawn->py == 0) ||
		  !mp->botnodes->DirectlyReachable(NULL, pawn->x, pawn->y, lastTargetX, lastTargetY))
		lastTarget = NULL; //just went through teleporter
	      else
		{
		  //CONS_Printf("heading towards last mobj\n");
		  TurnTowardsPoint(lastTargetX, lastTargetY);
		  forwardmove = botforwardmove[botspeed];
		}
	    }
	  else
	    {
	      // we see nothing interesting and have nothing urgent to do... lets do some pathing!
	      lastTarget = NULL;
	      SearchNode_t *newdest;
	      if (bUnseenItem.a)
		{
		  newdest = mp->botnodes->GetNodeAt(bUnseenItem.a->x, bUnseenItem.a->y);
		  //CONS_Printf("found a best item at x:%d, y:%d\n", bUnseenItem->x>>FRACBITS, bUnseenItem.a->y>>FRACBITS);
		}
	      else if (cUnseenTeammate.a)
		newdest = mp->botnodes->GetNodeAt(cUnseenTeammate.a->x, cUnseenTeammate.a->y);
	      else if (cUnseenEnemy.a)
		newdest = mp->botnodes->GetNodeAt(cUnseenEnemy.a->x, cUnseenEnemy.a->y);
	      else 
		newdest = NULL;

	      if (newdest != destination)
		ClearPath();
	      destination = newdest;

	      if (destination)
		{
		  // we have a valid destination
		  if (!path.empty() &&
		      P_AproxDistance(pawn->x - path.front()->mx, pawn->y - path.front()->my) < (BOTNODEGRIDSIZE<<1))//BOTNODEGRIDSIZE>>1))
		    {
		      // next path node reached!
#ifdef SHOWBOTPATH
		      SearchNode_t* temp = path.front();
		      temp->mo->Remove();
#endif
		      path.pop_front();
		    }

		  // do we need to rebuild the path?
		  if (path.empty() ||
		      !mp->botnodes->DirectlyReachable(NULL, pawn->x, pawn->y, path.front()->mx, path.front()->my))
		    if (!mp->botnodes->FindPath(path, mp->botnodes->GetNodeAt(pawn->x, pawn->y), destination))
		      {
			//CONS_Printf("Bot stuck at x:%d y:%d could not find a path to x:%d y:%d\n",pawn->x>>FRACBITS, pawn->y>>FRACBITS, posX2x(destination->x)>>FRACBITS, posY2y(destination->y)>>FRACBITS);
			destination = NULL; // can't get there
		      }

		  // follow the path
		  if (!path.empty())
		    {
		      //CONS_Printf("turning towards node at x%d, y%d\n", (nextItemNode->x>>FRACBITS), (nextItemNode->y>>FRACBITS));
		      //CONS_Printf("it has a distance %d\n", (P_AproxDistance(pawn->x - nextItemNode->x, pawn->y - nextItemNode->y)>>FRACBITS));
		      TurnTowardsPoint(path.front()->mx, path.front()->my);
		      forwardmove = botforwardmove[1];//botspeed];
		    }
		}
	    }
	}

      fixed_t forwardAngle = pawn->angle >> ANGLETOFINESHIFT;
      fixed_t sideAngle = (pawn->angle - ANG90) >> ANGLETOFINESHIFT;
      fixed_t cpx = FixedMul(forwardmove*2048, finecosine[forwardAngle]) + FixedMul(sidemove*2048, finecosine[sideAngle]);
      fixed_t cpy = FixedMul(forwardmove*2048, finesine[forwardAngle]) + FixedMul(sidemove*2048, finesine[sideAngle]);
      fixed_t nx = pawn->x + pawn->px + cpx;
      fixed_t ny = pawn->y + pawn->py + cpy;

      bool blocked = !pawn->CheckPosition(nx, ny) ||
	tmfloorz - pawn->z > 24*FRACUNIT ||
	tmceilingz - tmfloorz < pawn->height;
      //if its time to change strafe directions, 
      if (sidemove && ((pawn->eflags & MFE_JUSTHIT) || blocked))
	{
	  straferight = !straferight;
	  pawn->eflags &= ~MFE_JUSTHIT;
	}

      if (blocked)
	{
	  if (++blockedcount > 20 &&
	      (P_AproxDistance(pawn->px, pawn->py) < 4*FRACUNIT ||
	       (Blocking.thing && (Blocking.thing->flags & MF_SOLID))))
	    avoidtimer = 20;

	  if (tmfloorz - pawn->z > 24*FRACUNIT &&
	      (tmfloorz - pawn->z <= 37*FRACUNIT ||
	       (tmfloorz - pawn->z <= 45*FRACUNIT &&
		pawn->subsector->sector->floortype != FLOOR_WATER)))
	    cmd->buttons |= ticcmd_t::BT_JUMP;

	  for (unsigned i=0; i < spechit.size(); i++)
	    if (spechit[i]->backsector)
	      {
		if (!spechit[i]->backsector->ceilingdata && !spechit[i]->backsector->floordata)
		  cmd->buttons |= ticcmd_t::BT_USE;
	      }
	}
      else
	blockedcount = 0;
    }

  if (sidemove)
    {
      if (strafetimer)
	strafetimer--;
      else
	{
	  straferight = !straferight;
	  strafetimer = (std::rand()*85) / RAND_MAX;
	}
    }

  if (weaponchangetimer)
    weaponchangetimer--;

  cmd->forward = forwardmove;
  cmd->side = straferight ? sidemove : -sidemove;
  if (cMissile.a)
    AvoidMissile(cMissile.a);
}
