// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// Revision 1.12  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.11  2004/08/19 19:42:40  smite-meister
// bugfixes
//
// Revision 1.10  2004/08/12 18:30:23  smite-meister
// cleaned startup
//
// Revision 1.9  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.8  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.7  2003/11/27 11:28:25  smite-meister
// Doom/Heretic startup bug fixed
//
// Revision 1.6  2003/11/23 19:07:41  smite-meister
// New startup order
//
// Revision 1.5  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.4  2003/03/15 20:07:15  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.3  2002/12/16 22:11:29  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:11:39  smite-meister
// Blindness and missile clipping bugs fixed
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
  if (game.tic % (4 * NEWTICRATERATIO))
    return;

  // add the smoke behind the rocket
  DActor *th = actor->mp->SpawnDActor(actor->x - actor->px,
    actor->y - actor->py, actor->z, MT_SMOK);

  th->pz = FRACUNIT;
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
