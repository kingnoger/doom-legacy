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
// Revision 1.4  2003/05/30 13:34:41  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.3  2003/05/05 00:24:48  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.2  2003/04/14 08:58:24  smite-meister
// Hexen maps load.
//
// Revision 1.1  2003/03/15 20:07:13  smite-meister
// Initial Hexen compatibility!
//
//
//
// DESCRIPTION:
//   Hexen Actor AI
//
//-----------------------------------------------------------------------------

#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_player.h"
#include "g_game.h"
#include "m_random.h"

#include "p_maputl.h"
#include "p_enemy.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_main.h"
#include "tables.h"
#include "info.h"



bool A_RaiseMobj(DActor *actor);
bool A_SinkMobj(DActor *actor);
void P_ThrustSpike(Actor *actor);

// PUBLIC DATA DEFINITIONS -------------------------------------------------

fixed_t FloatBobOffsets[64] =
{
  0, 51389, 102283, 152192,
  200636, 247147, 291278, 332604,
  370727, 405280, 435929, 462380,
  484378, 501712, 514213, 521763,
  524287, 521763, 514213, 501712,
  484378, 462380, 435929, 405280,
  370727, 332604, 291278, 247147,
  200636, 152192, 102283, 51389,
  -1, -51390, -102284, -152193,
  -200637, -247148, -291279, -332605,
  -370728, -405281, -435930, -462381,
  -484380, -501713, -514215, -521764,
  -524288, -521764, -514214, -501713,
  -484379, -462381, -435930, -405280,
  -370728, -332605, -291279, -247148,
  -200637, -152193, -102284, -51389
};


int orbitTableX[256]=
{
  983025, 982725, 981825, 980340, 978255, 975600, 972330, 968490,
  964065, 959070, 953475, 947325, 940590, 933300, 925440, 917025,
  908055, 898545, 888495, 877905, 866775, 855135, 842985, 830310,
  817155, 803490, 789360, 774735, 759660, 744120, 728130, 711690,
  694845, 677565, 659880, 641805, 623340, 604500, 585285, 565725,
  545820, 525600, 505050, 484200, 463065, 441645, 419955, 398010,
  375840, 353430, 330810, 307995, 285000, 261825, 238485, 215010,
  191400, 167685, 143865, 119955, 95970, 71940, 47850, 23745,
  -375, -24495, -48600, -72690, -96720, -120705, -144600, -168420,
  -192150, -215745, -239220, -262545, -285720, -308715, -331530, -354135,
  -376530, -398700, -420630, -442320, -463725, -484860, -505695, -526230,
  -546450, -566340, -585885, -605085, -623925, -642375, -660435, -678105,
  -695370, -712215, -728625, -744600, -760125, -775200, -789795, -803925,
  -817575, -830715, -843375, -855510, -867135, -878235, -888810, -898845,
  -908340, -917295, -925695, -933540, -940815, -947520, -953670, -959235,
  -964215, -968625, -972450, -975690, -978330, -980400, -981870, -982740,
  -983025, -982725, -981825, -980340, -978255, -975600, -972330, -968490,
  -964065, -959070, -953475, -947325, -940590, -933300, -925440, -917025,
  -908055, -898545, -888495, -877905, -866775, -855135, -842985, -830310,
  -817155, -803490, -789360, -774735, -759660, -744120, -728130, -711690,
  -694845, -677565, -659880, -641805, -623340, -604485, -585285, -565725,
  -545820, -525600, -505050, -484200, -463065, -441645, -419955, -398010,
  -375840, -353430, -330810, -307995, -285000, -261825, -238485, -215010,
  -191400, -167685, -143865, -119955, -95970, -71940, -47850, -23745,
  375, 24495, 48600, 72690, 96720, 120705, 144600, 168420,
  192150, 215745, 239220, 262545, 285720, 308715, 331530, 354135,
  376530, 398700, 420630, 442320, 463725, 484860, 505695, 526230,
  546450, 566340, 585885, 605085, 623925, 642375, 660435, 678105,
  695370, 712215, 728625, 744600, 760125, 775200, 789795, 803925,
  817575, 830715, 843375, 855510, 867135, 878235, 888810, 898845,
  908340, 917295, 925695, 933540, 940815, 947520, 953670, 959235,
  964215, 968625, 972450, 975690, 978330, 980400, 981870, 982740
};

int orbitTableY[256]=
{
  375, 24495, 48600, 72690, 96720, 120705, 144600, 168420,
  192150, 215745, 239220, 262545, 285720, 308715, 331530, 354135,
  376530, 398700, 420630, 442320, 463725, 484860, 505695, 526230,
  546450, 566340, 585885, 605085, 623925, 642375, 660435, 678105,
  695370, 712215, 728625, 744600, 760125, 775200, 789795, 803925,
  817575, 830715, 843375, 855510, 867135, 878235, 888810, 898845,
  908340, 917295, 925695, 933540, 940815, 947520, 953670, 959235,
  964215, 968625, 972450, 975690, 978330, 980400, 981870, 982740,
  983025, 982725, 981825, 980340, 978255, 975600, 972330, 968490,
  964065, 959070, 953475, 947325, 940590, 933300, 925440, 917025,
  908055, 898545, 888495, 877905, 866775, 855135, 842985, 830310,
  817155, 803490, 789360, 774735, 759660, 744120, 728130, 711690,
  694845, 677565, 659880, 641805, 623340, 604500, 585285, 565725,
  545820, 525600, 505050, 484200, 463065, 441645, 419955, 398010,
  375840, 353430, 330810, 307995, 285000, 261825, 238485, 215010,
  191400, 167685, 143865, 119955, 95970, 71940, 47850, 23745,
  -375, -24495, -48600, -72690, -96720, -120705, -144600, -168420,
  -192150, -215745, -239220, -262545, -285720, -308715, -331530, -354135,
  -376530, -398700, -420630, -442320, -463725, -484860, -505695, -526230,
  -546450, -566340, -585885, -605085, -623925, -642375, -660435, -678105,
  -695370, -712215, -728625, -744600, -760125, -775200, -789795, -803925,
  -817575, -830715, -843375, -855510, -867135, -878235, -888810, -898845,
  -908340, -917295, -925695, -933540, -940815, -947520, -953670, -959235,
  -964215, -968625, -972450, -975690, -978330, -980400, -981870, -982740,
  -983025, -982725, -981825, -980340, -978255, -975600, -972330, -968490,
  -964065, -959070, -953475, -947325, -940590, -933300, -925440, -917025,
  -908055, -898545, -888495, -877905, -866775, -855135, -842985, -830310,
  -817155, -803490, -789360, -774735, -759660, -744120, -728130, -711690,
  -694845, -677565, -659880, -641805, -623340, -604485, -585285, -565725,
  -545820, -525600, -505050, -484200, -463065, -441645, -419955, -398010,
  -375840, -353430, -330810, -307995, -285000, -261825, -238485, -215010,
  -191400, -167685, -143865, -119955, -95970, -71940, -47850, -23745
};


// A Hexen hack to store a mobjtype number into one byte
mobjtype_t TranslateThingType[] =
{
  MT_MAPSPOT,				// T_NONE
  MT_CENTAUR,				// T_CENTAUR
  MT_CENTAURLEADER,		// T_CENTAURLEADER
  MT_DEMON,				// T_DEMON
  MT_ETTIN,				// T_ETTIN
  MT_FIREDEMON,			// T_FIREGARGOYLE
  MT_SERPENT,				// T_WATERLURKER
  MT_SERPENTLEADER,		// T_WATERLURKERLEADER
  MT_WRAITH,				// T_WRAITH
  MT_WRAITHB,				// T_WRAITHBURIED
  MT_FIREBALL1,			// T_FIREBALL1
  MT_MANA1,				// T_MANA1
  MT_MANA2,				// T_MANA2
  MT_SPEEDBOOTS,			// T_ITEMBOOTS
  MT_ARTIEGG,				// T_ITEMEGG
  MT_ARTIFLY,				// T_ITEMFLIGHT
  MT_SUMMONMAULATOR,		// T_ITEMSUMMON
  MT_TELEPORTOTHER,		// T_ITEMTPORTOTHER
  MT_ARTITELEPORT,		// T_ITEMTELEPORT
  MT_BISHOP,				// T_BISHOP
  MT_ICEGUY,				// T_ICEGOLEM
  MT_BRIDGE,				// T_BRIDGE
  MT_BOOSTARMOR,			// T_DRAGONSKINBRACERS
  MT_HEALINGBOTTLE,		// T_ITEMHEALTHPOTION
  MT_HEALTHFLASK,			// T_ITEMHEALTHFLASK
  MT_ARTISUPERHEAL,		// T_ITEMHEALTHFULL
  MT_BOOSTMANA,			// T_ITEMBOOSTMANA
  MT_FW_AXE,				// T_FIGHTERAXE
  MT_FW_HAMMER,			// T_FIGHTERHAMMER
  MT_FW_SWORD1,			// T_FIGHTERSWORD1
  MT_FW_SWORD2,			// T_FIGHTERSWORD2
  MT_FW_SWORD3,			// T_FIGHTERSWORD3
  MT_CW_SERPSTAFF,		// T_CLERICSTAFF
  MT_CW_HOLY1,			// T_CLERICHOLY1
  MT_CW_HOLY2,			// T_CLERICHOLY2
  MT_CW_HOLY3,			// T_CLERICHOLY3
  MT_MW_CONE,				// T_MAGESHARDS
  MT_MW_STAFF1,			// T_MAGESTAFF1
  MT_MW_STAFF2,			// T_MAGESTAFF2
  MT_MW_STAFF3,			// T_MAGESTAFF3
  MT_EGGFX,				// T_MORPHBLAST
  MT_ROCK1,				// T_ROCK1
  MT_ROCK2,				// T_ROCK2
  MT_ROCK3,				// T_ROCK3
  MT_DIRT1,				// T_DIRT1
  MT_DIRT2,				// T_DIRT2
  MT_DIRT3,				// T_DIRT3
  MT_DIRT4,				// T_DIRT4
  MT_DIRT5,				// T_DIRT5
  MT_DIRT6,				// T_DIRT6
  MT_ARROW,				// T_ARROW
  MT_DART,				// T_DART
  MT_POISONDART,			// T_POISONDART
  MT_RIPPERBALL,			// T_RIPPERBALL
  MT_SGSHARD1,			// T_STAINEDGLASS1
  MT_SGSHARD2,			// T_STAINEDGLASS2
  MT_SGSHARD3,			// T_STAINEDGLASS3
  MT_SGSHARD4,			// T_STAINEDGLASS4
  MT_SGSHARD5,			// T_STAINEDGLASS5
  MT_SGSHARD6,			// T_STAINEDGLASS6
  MT_SGSHARD7,			// T_STAINEDGLASS7
  MT_SGSHARD8,			// T_STAINEDGLASS8
  MT_SGSHARD9,			// T_STAINEDGLASS9
  MT_SGSHARD0,			// T_STAINEDGLASS0
  MT_PROJECTILE_BLADE,	// T_BLADE
  MT_ICESHARD,			// T_ICESHARD
  MT_FLAME_SMALL,			// T_FLAME_SMALL
  MT_FLAME_LARGE,			// T_FLAME_LARGE
  MT_ARMOR_1,				// T_MESHARMOR
  MT_ARMOR_2,				// T_FALCONSHIELD
  MT_ARMOR_3,				// T_PLATINUMHELM
  MT_ARMOR_4,				// T_AMULETOFWARDING
  MT_ARTIPOISONBAG,		// T_ITEMFLECHETTE
  MT_ARTITORCH,			// T_ITEMTORCH
  MT_BLASTRADIUS,			// T_ITEMREPULSION
  MT_MANA3,				// T_MANA3
  MT_ARTIPUZZSKULL,		// T_PUZZSKULL
  MT_ARTIPUZZGEMBIG,		// T_PUZZGEMBIG
  MT_ARTIPUZZGEMRED,		// T_PUZZGEMRED
  MT_ARTIPUZZGEMGREEN1,	// T_PUZZGEMGREEN1
  MT_ARTIPUZZGEMGREEN2,	// T_PUZZGEMGREEN2
  MT_ARTIPUZZGEMBLUE1,	// T_PUZZGEMBLUE1
  MT_ARTIPUZZGEMBLUE2,	// T_PUZZGEMBLUE2
  MT_ARTIPUZZBOOK1,		// T_PUZZBOOK1
  MT_ARTIPUZZBOOK2,		// T_PUZZBOOK2
  MT_KEY1,				// T_METALKEY
  MT_KEY2,				// T_SMALLMETALKEY
  MT_KEY3,				// T_AXEKEY
  MT_KEY4,				// T_FIREKEY
  MT_KEY5,				// T_GREENKEY
  MT_KEY6,				// T_MACEKEY
  MT_KEY7,				// T_SILVERKEY
  MT_KEY8,				// T_RUSTYKEY
  MT_KEY9,				// T_HORNKEY
  MT_KEYA,				// T_SERPENTKEY
  MT_WATER_DRIP,			// T_WATERDRIP
  MT_FLAME_SMALL_TEMP,	// T_TEMPSMALLFLAME
  MT_FLAME_SMALL,			// T_PERMSMALLFLAME
  MT_FLAME_LARGE_TEMP,	// T_TEMPLARGEFLAME
  MT_FLAME_LARGE,			// T_PERMLARGEFLAME
  MT_DEMON_MASH,			// T_DEMON_MASH
  MT_DEMON2_MASH,			// T_DEMON2_MASH
  MT_ETTIN_MASH,			// T_ETTIN_MASH
  MT_CENTAUR_MASH,		// T_CENTAUR_MASH
  MT_THRUSTFLOOR_UP,		// T_THRUSTSPIKEUP
  MT_THRUSTFLOOR_DOWN,	// T_THRUSTSPIKEDOWN
  MT_WRAITHFX4,			// T_FLESH_DRIP1
  MT_WRAITHFX5,			// T_FLESH_DRIP2
  MT_WRAITHFX2			// T_SPARK_DRIP
};


// angle_t is an unsigned int (wraps around nicely)
// This is used on angle differences
angle_t abs(angle_t a)
{
  if (a <= ANGLE_180)
    return a;
  else
    return -a; // effectively 2pi - a since it wraps
};

//--------------------------------------------------------------------------
//
// Environmental Action routines
//
//--------------------------------------------------------------------------

//============================================================================
//
// A_SpeedFade
//
//============================================================================

void A_SpeedFade(DActor *actor)
{
  actor->flags |= MF_SHADOW;
  actor->flags &= ~MF_ALTSHADOW;
  //actor->sprite = actor->target->sprite;
}

//==========================================================================
//
// A_DripBlood
//
//==========================================================================

/*
void A_DripBlood(DActor *actor)
{
	Actor *mo;

	mo = actor->mp->SpawnDActor(actor->x+((P_Random()-P_Random())<<11),
		actor->y+((P_Random()-P_Random())<<11), actor->z, MT_BLOOD);
	mo->px = (P_Random()-P_Random())<<10;
	mo->py = (P_Random()-P_Random())<<10;
	mo->flags2 |= MF2_LOGRAV;
}
*/

//============================================================================
//
// A_PotteryExplode
//
//============================================================================

void A_PotteryExplode(DActor *actor)
{
  DActor *mo=NULL;
  int i;

  for(i = (P_Random()&3)+3; i; i--)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_POTTERYBIT1);
      mo->SetState(statenum_t(mo->info->spawnstate + P_Random() % 5));
      if(mo)
	{
	  mo->pz = ((P_Random()&7)+5)*(3*FRACUNIT/4);
	  mo->px = (P_Random()-P_Random())<<(FRACBITS-6);
	  mo->py = (P_Random()-P_Random())<<(FRACBITS-6);
	}
    }
  S_StartSound(mo, SFX_POTTERY_EXPLODE);
  if(actor->args[0])
    { // Spawn an item
      if(!game.nomonsters || !(mobjinfo[TranslateThingType[actor->args[0]]].flags & MF_COUNTKILL))
	{ // Only spawn monsters if not -nomonsters
	  actor->mp->SpawnDActor(actor->x, actor->y, actor->z,
				 TranslateThingType[actor->args[0]]);
	}
    }
  actor->Remove();
}

//============================================================================
//
// A_PotteryChooseBit
//
//============================================================================

void A_PotteryChooseBit(DActor *actor)
{
  actor->SetState(statenum_t(actor->info->deathstate + P_Random()%5 + 1));
  actor->tics = 256+(P_Random()<<1);
}

//============================================================================
//
// A_PotteryCheck
//
//============================================================================

void A_PotteryCheck(DActor *actor)
{
  int i;
  Actor *pmo;

  if(!game.netgame)
    {
      pmo = consoleplayer->pawn;
      if (actor->mp->CheckSight(actor, pmo)
	  && (abs(R_PointToAngle2(pmo->x, pmo->y, actor->x, actor->y)-pmo->angle) <= ANGLE_45))
	{ // Previous state (pottery bit waiting state)
	  actor->SetState(statenum_t(actor->state - &states[0] - 1));
	}
      else
	{
	  return;
	}
    }
  else
    {
      int n = actor->mp->players.size();
      for(i = 0; i < n; i++)
	{
	  pmo = actor->mp->players[i]->pawn;
	  if (actor->mp->CheckSight(actor, pmo) &&
	      (abs(R_PointToAngle2(pmo->x, pmo->y, actor->x, actor->y)-pmo->angle) <= ANGLE_45))
	    { // Previous state (pottery bit waiting state)
	      actor->SetState(statenum_t(actor->state - &states[0] - 1));
	      return;
	    }
	}
    }		
}

//============================================================================
//
// A_CorpseBloodDrip
//
//============================================================================

void A_CorpseBloodDrip(DActor *actor)
{
  if(P_Random() > 128)
    {
      return;
    }
  actor->mp->SpawnDActor(actor->x, actor->y, actor->z+actor->height/2, 
	      MT_CORPSEBLOODDRIP);
}

//============================================================================
//
// A_CorpseExplode
//
//============================================================================

void A_CorpseExplode(DActor *actor)
{
  DActor *mo;
  int i;

  for(i = (P_Random()&3)+3; i; i--)
    {
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_CORPSEBIT);
      mo->SetState(statenum_t(mo->info->spawnstate + P_Random() % 3));
      if(mo)
	{
	  mo->pz = ((P_Random()&7)+5)*(3*FRACUNIT/4);
	  mo->px = (P_Random()-P_Random())<<(FRACBITS-6);
	  mo->py = (P_Random()-P_Random())<<(FRACBITS-6);
	}
    }
  // Spawn a skull
  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_CORPSEBIT);
  mo->SetState(S_CORPSEBIT_4);
  if(mo)
    {
      mo->pz = ((P_Random()&7)+5)*(3*FRACUNIT/4);
      mo->px = (P_Random()-P_Random())<<(FRACBITS-6);
      mo->py = (P_Random()-P_Random())<<(FRACBITS-6);
      S_StartSound(mo, SFX_FIRED_DEATH);
    }
  actor->Remove();
}

//============================================================================
//
// A_LeafSpawn
//
//============================================================================

void A_LeafSpawn(DActor *actor)
{
  Actor *mo;
  int i;

  for(i = (P_Random()&3)+1; i; i--)
    {
      mo = actor->mp->SpawnDActor(actor->x + ((P_Random()-P_Random())<<14), actor->y+
				  ((P_Random()-P_Random())<<14), actor->z+(P_Random()<<14), 
				  mobjtype_t(MT_LEAF1 + (P_Random() & 1)));
      if(mo)
	{
	  mo->Thrust(actor->angle, (P_Random()<<9)+3*FRACUNIT);
	  mo->owner = actor;
	  mo->special1 = 0;
	}
    }
}

//============================================================================
//
// A_LeafThrust
//
//============================================================================

void A_LeafThrust(DActor *actor)
{
  if(P_Random() > 96)
    {
      return;
    }
  actor->pz += (P_Random()<<9)+FRACUNIT;
}

//============================================================================
//
// A_LeafCheck
//
//============================================================================

void A_LeafCheck(DActor *actor)
{
  actor->special1++;
  if(actor->special1 >= 20)
    {
      actor->SetState(S_NULL);
      return;
    }
  if(P_Random() > 64)
    {
      if(!actor->px && !actor->py)
	{
	  actor->Thrust(actor->owner->angle,
		       (P_Random()<<9)+FRACUNIT);
	}
      return;
    }
  actor->SetState(S_LEAF1_8);
  actor->pz = (P_Random()<<9)+FRACUNIT;
  actor->Thrust(actor->owner->angle, (P_Random()<<9)+2*FRACUNIT);
  actor->flags |= MF_MISSILE;
}

/*
#define ORBIT_RADIUS	(15*FRACUNIT)
void GenerateOrbitTable(void)
{
	int angle;

	for (angle=0; angle<256; angle++)
	{
		orbitTableX[angle] = FixedMul(ORBIT_RADIUS, finecosine[angle<<5]);
		orbitTableY[angle] = FixedMul(ORBIT_RADIUS, finesine[angle<<5]);
	}

	printf("int orbitTableX[256]=\n{\n");
	for (angle=0; angle<256; angle+=8)
	{
		printf("%d, %d, %d, %d, %d, %d, %d, %d,\n",
			orbitTableX[angle],
			orbitTableX[angle+1],
			orbitTableX[angle+2],
			orbitTableX[angle+3],
			orbitTableX[angle+4],
			orbitTableX[angle+5],
			orbitTableX[angle+6],
			orbitTableX[angle+7]);
	}
	printf("};\n\n");

	printf("int orbitTableY[256]=\n{\n");
	for (angle=0; angle<256; angle+=8)
	{
		printf("%d, %d, %d, %d, %d, %d, %d, %d,\n",
			orbitTableY[angle],
			orbitTableY[angle+1],
			orbitTableY[angle+2],
			orbitTableY[angle+3],
			orbitTableY[angle+4],
			orbitTableY[angle+5],
			orbitTableY[angle+6],
			orbitTableY[angle+7]);
	}
	printf("};\n");
}
*/

// New bridge stuff
//	Parent
//		special1	true == removing from world
//
//	Child
//		owner		pointer to center mobj
//		args[0]		angle of ball

void A_BridgeOrbit(DActor *actor)
{
  //  if (actor->owner->special1)
  if (actor->owner->mp == NULL) // "to be removed -signal"
    {
      actor->SetState(S_NULL);
    }
  actor->args[0]+=3;
  actor->x = actor->owner->x + orbitTableX[actor->args[0]];
  actor->y = actor->owner->y + orbitTableY[actor->args[0]];
  actor->z = actor->owner->z;
}


void A_BridgeInit(DActor *actor)
{
  byte startangle;
  DActor *ball1, *ball2, *ball3;
  fixed_t cx,cy,cz;

  //	GenerateOrbitTable();

  cx = actor->x;
  cy = actor->y;
  cz = actor->z;
  startangle = P_Random();
  actor->special1 = 0;

  // Spawn triad into world
  ball1 = actor->mp->SpawnDActor(cx, cy, cz, MT_BRIDGEBALL);
  ball1->args[0] = startangle;
  ball1->owner = actor;

  ball2 = actor->mp->SpawnDActor(cx, cy, cz, MT_BRIDGEBALL);
  ball2->args[0] = (startangle+85)&255;
  ball2->owner = actor;

  ball3 = actor->mp->SpawnDActor(cx, cy, cz, MT_BRIDGEBALL);
  ball3->args[0] = (startangle+170)&255;
  ball3->owner = actor;

  A_BridgeOrbit(ball1);
  A_BridgeOrbit(ball2);
  A_BridgeOrbit(ball3);
}
/*
void A_BridgeRemove(DActor *actor)
{
  actor->special1 = true;		// Removing the bridge
  actor->flags &= ~MF_SOLID;
  actor->SetState(S_FREE_BRIDGE1);
}
*/

//==========================================================================
//
// A_GhostOn
//
//==========================================================================

/*
void A_GhostOn(DActor *actor)
{
	actor->flags |= MF_SHADOW;
}
*/

//==========================================================================
//
// A_GhostOff
//
//==========================================================================

/*
void A_GhostOff(DActor *actor)
{
	actor->flags &= ~MF_SHADOW;
}
*/

//==========================================================================
//
// A_HideThing
//
//==========================================================================

void A_HideThing(DActor *actor)
{
  actor->flags2 |= MF2_DONTDRAW;
}

//==========================================================================
//
// A_UnHideThing
//
//==========================================================================

void A_UnHideThing(DActor *actor)
{
  actor->flags2 &= ~MF2_DONTDRAW;
}

//==========================================================================
//
// A_SetShootable
//
//==========================================================================

void A_SetShootable(DActor *actor)
{
  actor->flags2 &= ~MF2_NONSHOOTABLE;
  actor->flags |= MF_SHOOTABLE;
}

//==========================================================================
//
// A_UnSetShootable
//
//==========================================================================

void A_UnSetShootable(DActor *actor)
{
  actor->flags2 |= MF2_NONSHOOTABLE;
  actor->flags &= ~MF_SHOOTABLE;
}

//==========================================================================
//
// A_SetAltShadow
//
//==========================================================================

void A_SetAltShadow(DActor *actor)
{
  actor->flags &= ~MF_SHADOW;
  actor->flags |= MF_ALTSHADOW;
}

//==========================================================================
//
// A_UnSetAltShadow
//
//==========================================================================

/*
void A_UnSetAltShadow(DActor *actor)
{
	actor->flags &= ~MF_ALTSHADOW;
}
*/

//--------------------------------------------------------------------------
//
// Sound Action Routines
//
//--------------------------------------------------------------------------

//==========================================================================
//
// A_ContMobjSound
//
//==========================================================================

void A_ContMobjSound(DActor *actor)
{
  switch(actor->type)
    {
      // Hexen
    case MT_SERPENTFX:
      S_StartSound(actor, SFX_SERPENTFX_CONTINUOUS);
      break;
    case MT_HAMMER_MISSILE:
      S_StartSound(actor, SFX_FIGHTER_HAMMER_CONTINUOUS);
      break;
    case MT_QUAKE_FOCUS:
      S_StartSound(actor, SFX_EARTHQUAKE);
      break;
      // Heretic
    case MT_KNIGHTAXE:
      S_StartSound(actor, sfx_kgtatk);
      break;
    case MT_MUMMYFX1:
      S_StartSound(actor, sfx_mumhed);
      break;
    default:
      break;
    }
}

//==========================================================================
//
// A_ESound
//
//==========================================================================

void A_ESound(DActor *mo)
{
  int sound;

  switch(mo->type)
    {
    case MT_SOUNDWATERFALL:
      sound = sfx_waterfl;
      break;
    case MT_SOUNDWIND:
      sound = sfx_wind;
      break;
    case MT_XSOUNDWIND:
      sound = SFX_WIND;
      break;
    default:
      sound = sfx_None;
      break;
    }
  S_StartSound(mo, sound);	
}



//==========================================================================
// Summon Minotaur -- see p_enemy for variable descriptions
//==========================================================================


void A_Summon(DActor *actor)
{
  DActor *mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_XMINOTAUR);
  if (mo)
    {
      if (mo->TestLocation() == false || !actor->owner)
	{ // Didn't fit - change back to artifact
	  mo->SetState(S_NULL);
	  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_SUMMONMAULATOR);
	  if (mo) mo->flags |= MF_DROPPED;
	  return;
	}

      *((unsigned int *)mo->args) = gametic;
      Actor *master = actor->owner;
      if (master->flags & MF_CORPSE)
	mo->owner = NULL; // No master
      else
	{
	  mo->owner = master;
	  if (master->Type() == Thinker::tt_ppawn)
	    ((PlayerPawn *)master)->GivePower(pw_minotaur);
	}

      // Make smoke puff
      actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_MNTRSMOKE);
      S_StartSound(actor, SFX_MAULATOR_ACTIVE);
    }
}



//==========================================================================
// Fog Variables:
//
//		args[0]		Speed (0..10) of fog
//		args[1]		Angle of spread (0..128)
// 		args[2]		Frequency of spawn (1..10)
//		args[3]		Lifetime countdown
//		args[4]		Boolean: fog moving?
//		special1		Internal:  Counter for spawn frequency
//		special2		Internal:  Index into floatbob table
//
//==========================================================================

void A_FogSpawn(DActor *actor)
{
  Actor *mo=NULL;
  angle_t delta;

  if (actor->special1-- > 0) return;

  actor->special1 = actor->args[2];		// Reset frequency count

  switch(P_Random()%3)
    {
    case 0:
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_FOGPATCHS);
      break;
    case 1:
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_FOGPATCHM);
      break;
    case 2:
      mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_FOGPATCHL);
      break;
    }

  if (mo)
    {
      delta = actor->args[1];
      if (delta==0) delta=1;
      mo->angle = actor->angle + (((P_Random()%delta)-(delta>>1))<<24);
      mo->owner = actor;
      if (actor->args[0] < 1) actor->args[0] = 1;
      mo->args[0] = (P_Random() % (actor->args[0]))+1;	// Random speed
      mo->args[3] = actor->args[3];						// Set lifetime
      mo->args[4] = 1;									// Set to moving
      mo->special2 = P_Random()&63;
    }
}


void A_FogMove(DActor *actor)
{
  int speed = actor->args[0]<<FRACBITS;
  angle_t angle;
  int weaveindex;

  if (!(actor->args[4])) return;

  if (actor->args[3]-- <= 0)
    {
      actor->SetState(actor->info->deathstate, false);
      return;
    }

  if ((actor->args[3] % 4) == 0)
    {
      weaveindex = actor->special2;
      actor->z += FloatBobOffsets[weaveindex]>>1;
      actor->special2 = (weaveindex+1)&63;
    }

  angle = actor->angle>>ANGLETOFINESHIFT;
  actor->px = FixedMul(speed, finecosine[angle]);
  actor->py = FixedMul(speed, finesine[angle]);
}

//===========================================================================
//
// A_PoisonBagInit
//
//===========================================================================

void A_PoisonBagInit(DActor *actor)
{
  Actor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z+28*FRACUNIT, MT_POISONCLOUD);
  if(!mo)
    {
      return;
    }
  mo->px = 1; // missile objects must move to impact other objects
  mo->special1 = 24+(P_Random()&7);
  mo->special2 = 0;
  mo->target = actor->target;
  mo->radius = 20*FRACUNIT;
  mo->height = 30*FRACUNIT;
  mo->flags &= ~(MF_NOCLIPLINE | MF_NOCLIPTHING);
}

//===========================================================================
//
// A_PoisonBagCheck
//
//===========================================================================

void A_PoisonBagCheck(DActor *actor)
{
  if(!--actor->special1)
    {
      actor->SetState(S_POISONCLOUD_X1);
    }
  else
    {
      return;
    }
}

//===========================================================================
//
// A_PoisonBagDamage
//
//===========================================================================

void A_PoisonBagDamage(DActor *actor)
{
  int bobIndex;
	
  extern void A_Explode(DActor *actor);

  A_Explode(actor);	

  bobIndex = actor->special2;
  actor->z += FloatBobOffsets[bobIndex]>>4;
  actor->special2 = (bobIndex+1)&63;
}

//===========================================================================
//
// A_PoisonShroom
//
//===========================================================================

void A_PoisonShroom(DActor *actor)
{
  actor->tics = 128+(P_Random()<<1);
}

//===========================================================================
//
// A_CheckThrowBomb
//
//===========================================================================

void A_CheckThrowBomb(DActor *actor)
{
  if(abs(actor->px) < 1.5*FRACUNIT && abs(actor->py) < 1.5*FRACUNIT
     && actor->pz < 2*FRACUNIT
     && actor->state == &states[S_THROWINGBOMB6])
    {
      actor->SetState(S_THROWINGBOMB7);
      actor->z = actor->floorz;
      actor->pz = 0;
      actor->flags2 &= ~MF2_FLOORBOUNCE;
      actor->flags &= ~MF_MISSILE;
    }
  if(!--actor->health)
    {
      actor->SetState(actor->info->deathstate);
    }
}

//===========================================================================
// Quake variables
//
//		args[0]		Intensity on richter scale (2..9)
//		args[1]		Duration in tics
//		args[2]		Radius for damage
//		args[3]		Radius for tremor
//		args[4]		TID of map thing for focus of quake
//
//===========================================================================

//===========================================================================
//
// A_LocalQuake
//
//===========================================================================

bool A_LocalQuake(byte *args, Actor *actor)
{
  Actor *focus, *target;
  int lastfound = 0;
  bool success = false;

  // Find all quake foci
  do
    {
      target = actor->mp->FindFromTIDmap(args[4], &lastfound);
      if (target)
	{
	  focus = target->mp->SpawnDActor(target->x, target->y, target->z, MT_QUAKE_FOCUS);
	  if (focus)
	    {
	      focus->args[0] = args[0];
	      focus->args[1] = args[1]>>1;	// decremented every 2 tics
	      focus->args[2] = args[2];
	      focus->args[3] = args[3];
	      focus->args[4] = args[4];
	      success = true;
	    }
	}
    } while (target != NULL);

  return success;
}


//===========================================================================
//
// A_Quake
//
//===========================================================================
//int	localQuakeHappening[MAXPLAYERS];

void A_Quake(DActor *actor)
{
  angle_t an;
  Actor *victim;
  int richters = actor->args[0];
  int i;
  fixed_t dist;

  int n = actor->mp->players.size();
  if (actor->args[1]-- > 0)
    {
      for (i=0; i < n; i++)
	{
	  victim = actor->mp->players[i]->pawn;
	  dist = P_AproxDistance(actor->x - victim->x,
				 actor->y - victim->y) >> (FRACBITS+6);
	  // Tested in tile units (64 pixels)
	  if (dist < actor->args[3])		// In tremor radius
	    {
	      // FIXME do the shaking somehow  localQuakeHappening[i] = richters;
	    }
	  // Check if in damage radius
	  if ((dist < actor->args[2]) && (victim->z <= victim->floorz))
	    {
	      if (P_Random() < 50)
		{
		  victim->Damage(NULL, NULL, HITDICE(1));
		}
				// Thrust player around
	      an = victim->angle + ANGLE_1*P_Random();
	      victim->Thrust(an, richters<<(FRACBITS-1));
	    }
	}
    }
  else
    {
      for (i=0; i < n; i++)
	{
	  //localQuakeHappening[i] = false;
	}
      actor->SetState(S_NULL);
    }
}




//===========================================================================
//
// Teleport other stuff
//
//===========================================================================

#define TELEPORT_LIFE 1

void A_TeloSpawnA(DActor *actor)
{
  Actor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_TELOTHER_FX2);
  if (mo)
    {
      mo->special1 = TELEPORT_LIFE;			// Lifetime countdown
      mo->angle = actor->angle;
      mo->target = actor->target;
      mo->px = actor->px>>1;
      mo->py = actor->py>>1;
      mo->pz = actor->pz>>1;
    }
}

void A_TeloSpawnB(DActor *actor)
{
  Actor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_TELOTHER_FX3);
  if (mo)
    {
      mo->special1 = TELEPORT_LIFE;			// Lifetime countdown
      mo->angle = actor->angle;
      mo->target = actor->target;
      mo->px = actor->px>>1;
      mo->py = actor->py>>1;
      mo->pz = actor->pz>>1;
    }
}

void A_TeloSpawnC(DActor *actor)
{
  Actor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_TELOTHER_FX4);
  if (mo)
    {
      mo->special1 = TELEPORT_LIFE;			// Lifetime countdown
      mo->angle = actor->angle;
      mo->target = actor->target;
      mo->px = actor->px>>1;
      mo->py = actor->py>>1;
      mo->pz = actor->pz>>1;
    }
}

void A_TeloSpawnD(DActor *actor)
{
  Actor *mo;

  mo = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_TELOTHER_FX5);
  if (mo)
    {
      mo->special1 = TELEPORT_LIFE;			// Lifetime countdown
      mo->angle = actor->angle;
      mo->target = actor->target;
      mo->px = actor->px>>1;
      mo->py = actor->py>>1;
      mo->pz = actor->pz>>1;
    }
}

void A_CheckTeleRing(DActor *actor)
{
  if (actor->special1-- <= 0)
    {
      actor->SetState(actor->info->deathstate);
    }
}




// Dirt stuff

void P_SpawnDirt(DActor *actor, fixed_t radius)
{
  fixed_t x,y,z;
  mobjtype_t dtype = MT_DIRT1;
  Actor *mo;
  angle_t angle;

  angle = P_Random()<<5;		// <<24 >>19
  x = actor->x + FixedMul(radius,finecosine[angle]);
  y = actor->y + FixedMul(radius,finesine[angle]);
  //	x = actor->x + ((P_Random()-P_Random())%radius)<<FRACBITS;
  //	y = actor->y + ((P_Random()-P_Random()<<FRACBITS)%radius);
  z = actor->z + (P_Random()<<9) + FRACUNIT;
  switch(P_Random()%6)
    {
    case 0:
      dtype = MT_DIRT1;
      break;
    case 1:
      dtype = MT_DIRT2;
      break;
    case 2:
      dtype = MT_DIRT3;
      break;
    case 3:
      dtype = MT_DIRT4;
      break;
    case 4:
      dtype = MT_DIRT5;
      break;
    case 5:
      dtype = MT_DIRT6;
      break;
    }
  mo = actor->mp->SpawnDActor(x,y,z,dtype);
  if (mo)
    {
      mo->pz = P_Random()<<10;
    }
}




//===========================================================================
//
// Thrust floor stuff
//
// Thrust Spike Variables
//		target		pointer to dirt clump mobj
//		special2		speed of raise
//		args[0]		0 = lowered,  1 = raised
//		args[1]		0 = normal,   1 = bloody
//===========================================================================

void A_ThrustInitUp(DActor *actor)
{
  actor->special2 = 5;		// Raise speed
  actor->args[0] = 1;		// Mark as up
  actor->floorclip = 0;
  actor->flags = MF_SOLID;
  actor->flags2 = MF2_NOTELEPORT|MF2_FOOTCLIP;
  actor->target = NULL;
}

void A_ThrustInitDn(DActor *actor)
{
  actor->special2 = 5;		// Raise speed
  actor->args[0] = 0;		// Mark as down
  actor->floorclip = actor->info->height;
  actor->flags = 0;
  actor->flags2 = MF2_NOTELEPORT|MF2_FOOTCLIP|MF2_DONTDRAW;
  actor->target = actor->mp->SpawnDActor(actor->x, actor->y, actor->z, MT_DIRTCLUMP);
}


void A_ThrustRaise(DActor *actor)
{
  if (A_RaiseMobj(actor))
    {	// Reached it's target height
      actor->args[0] = 1;
      if (actor->args[1])
	actor->SetState(S_BTHRUSTINIT2_1, false);
      else
	actor->SetState(S_THRUSTINIT2_1, false);
    }

  // Lose the dirt clump
  if ((actor->floorclip < actor->height) && actor->target)
    {
      actor->target->Remove();
      actor->target = NULL;
    }

  // Spawn some dirt
  if (P_Random()<40)
    P_SpawnDirt(actor, actor->radius);
  actor->special2++;							// Increase raise speed
}

void A_ThrustLower(DActor *actor)
{
  if (A_SinkMobj(actor))
    {
      actor->args[0] = 0;
      if (actor->args[1])
	actor->SetState(S_BTHRUSTINIT1_1, false);
      else
	actor->SetState(S_THRUSTINIT1_1, false);
    }
}

void A_ThrustBlock(DActor *actor)
{
  actor->flags |= MF_SOLID;
}

void A_ThrustImpale(DActor *actor)
{
  // Impale all shootables in radius
  P_ThrustSpike(actor);
}

//===========================================================================
//
// A_SoAExplode - Suit of Armor Explode
//
//===========================================================================

void A_SoAExplode(DActor *actor)
{
  DActor *mo;
  int i;

  for(i = 0; i < 10; i++)
    {
      mo = actor->mp->SpawnDActor(actor->x+((P_Random()-128)<<12), 
		       actor->y+((P_Random()-128)<<12), 
		       actor->z+(P_Random()*actor->height/256), MT_ZARMORCHUNK);
      mo->SetState(statenum_t(mo->info->spawnstate + i));
      if(mo)
	{
	  mo->pz = ((P_Random()&7)+5)*FRACUNIT;
	  mo->px = (P_Random()-P_Random())<<(FRACBITS-6);
	  mo->py = (P_Random()-P_Random())<<(FRACBITS-6);
	}
    }
  if(actor->args[0])
    { // Spawn an item
      if(!game.nomonsters 
	 || !(mobjinfo[TranslateThingType[actor->args[0]]].flags&MF_COUNTKILL))
	{ // Only spawn monsters if not -nomonsters
	  actor->mp->SpawnDActor(actor->x, actor->y, actor->z,
		      TranslateThingType[actor->args[0]]);
	}
    }
  S_StartSound(mo, SFX_SUITOFARMOR_BREAK);
  actor->Remove();
}

//===========================================================================
//
// A_BellReset1
//
//===========================================================================

void A_BellReset1(DActor *actor)
{
  actor->flags |= MF_NOGRAVITY;
  actor->height <<= 2;	
}

//===========================================================================
//
// A_BellReset2
//
//===========================================================================

void A_BellReset2(DActor *actor)
{
  actor->flags |= MF_SHOOTABLE;
  actor->flags &= ~MF_CORPSE;
  actor->health = 5;
}


//===========================================================================
//
// A_FlameCheck
//
//===========================================================================

void A_FlameCheck(DActor *actor)
{
  if(!actor->args[0]--)		// Called every 8 tics
    {
      actor->SetState(S_NULL);
    }
}


//===========================================================================
// Bat Spawner Variables
//	special1	frequency counter
//	special2	
//	args[0]		frequency of spawn (1=fastest, 10=slowest)
//	args[1]		spread angle (0..255)
//	args[2]		
//	args[3]		duration of bats (in octics)
//	args[4]		turn amount per move (in degrees)
//
// Bat Variables
//	special2	lifetime counter
//	args[4]		turn amount per move (in degrees)
//===========================================================================

void A_BatSpawnInit(DActor *actor)
{
  actor->special1 = 0;	// Frequency count
}

void A_BatSpawn(DActor *actor)
{
  // Countdown until next spawn
  if (actor->special1-- > 0) return;
  actor->special1 = actor->args[0];		// Reset frequency count

  int delta = actor->args[1];
  if (delta==0) delta=1;
  angle_t angle = actor->angle + (((P_Random()%delta)-(delta>>1))<<24);
  Actor *mo = actor->SpawnMissileAngle(MT_BAT, angle, 0);
  if (mo)
    {
      mo->args[0] = P_Random()&63;			// floatbob index
      mo->args[4] = actor->args[4];			// turn degrees
      mo->special2 = actor->args[3]<<3;		// Set lifetime
      mo->owner = actor;
    }
}


void A_BatMove(DActor *actor)
{
  angle_t newangle;
  fixed_t speed;

  if (actor->special2 < 0)
    {
      actor->SetState(actor->info->deathstate);
    }
  actor->special2 -= 2;		// Called every 2 tics

  if (P_Random()<128)
    {
      newangle = actor->angle + ANGLE_1*actor->args[4];
    }
  else
    {
      newangle = actor->angle - ANGLE_1*actor->args[4];
    }

  // Adjust momentum vector to new direction
  newangle >>= ANGLETOFINESHIFT;
  speed = int(actor->info->speed * (P_Random()<<10));
  actor->px = FixedMul(speed, finecosine[newangle]);
  actor->py = FixedMul(speed, finesine[newangle]);

  if (P_Random()<15)
    S_StartSound(actor, SFX_BAT_SCREAM);

  // Handle Z movement
  actor->z = actor->owner->z + 2*FloatBobOffsets[actor->args[0]];
  actor->args[0] = (actor->args[0]+3)&63;	
}

//===========================================================================
//
// A_TreeDeath
//
//===========================================================================

void A_TreeDeath(DActor *actor)
{
  if(!(actor->flags2&MF2_FIREDAMAGE))
    {
      actor->height <<= 2;
      actor->flags |= MF_SHOOTABLE;
      actor->flags &= ~(MF_CORPSE+MF_DROPOFF);
      actor->health = 35;
      return;
    }
  else
    {
      actor->SetState(actor->info->meleestate);
    }
}

//===========================================================================
//
// A_NoGravity
//
//===========================================================================

void A_NoGravity(DActor *actor)
{
  actor->flags |= MF_NOGRAVITY;
}
