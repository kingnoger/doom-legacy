// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.8  2002/08/23 18:05:38  vberghol
// idiotic segfaults fixed
//
// Revision 1.6  2002/08/17 16:02:03  vberghol
// final compile for engine!
//
// Revision 1.5  2002/08/06 13:14:21  vberghol
// ...
//
// Revision 1.4  2002/07/23 19:21:40  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:16  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:12  vberghol
// Version 133 Experimental!
//
// Revision 1.5  2000/10/21 08:43:30  bpereira
// no message
//
// Revision 1.4  2000/07/01 09:23:49  bpereira
// no message
//
// Revision 1.3  2000/04/24 20:24:38  bpereira
// no message
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      some new action routines, separated from the original doom
//      sources, so that you can include it or remove it easy.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "g_game.h"
#include "g_map.h"
#include "g_actor.h"
#include "info.h"
#include "r_state.h"
#include "p_fab.h"
#include "p_pspr.h"
#include "m_random.h"

void Translucency_OnChange();

consvar_t cv_translucency  = {"translucency" ,"1",CV_CALL|CV_SAVE,CV_OnOff, Translucency_OnChange};


// action function for running FS
// FIXME add this to a state in the states table so that DeHackEd can access it!
void A_RunFS(DActor *actor)
{
  int script = actor->tics;
  actor->tics = 0; // takes no time
  actor->mp->T_RunScript(script, actor);
}

//
// Action routine, for the ROCKET thing.
// This one adds trails of smoke to the rocket.
// The action pointer of the S_ROCKET state must point here to take effect.
// This routine is based on the Revenant Fireball Tracer code A_Tracer()
//
void A_SmokeTrailer(DActor *actor)
{
  if (gametic % (4 * NEWTICRATERATIO))
    return;

  // add the smoke behind the rocket
  DActor *th = actor->mp->SpawnDActor(actor->x - actor->px,
    actor->y - actor->py, actor->z, MT_SMOK);

  th->pz = FRACUNIT;
  th->tics -= P_Random()&3;
  if (th->tics < 1)
    th->tics = 1;
}


static bool resettrans=false;
//  Set the translucency map for each frame state of mobj
//
void R_SetTrans(statenum_t state1, statenum_t state2, transnum_t transmap)
{
  state_t*   state = &states[state1];
  int s1 = state1;
  int s2 = state2;
  do {
    state->frame &= ~FF_TRANSMASK;
    if(!resettrans)
      state->frame |= (transmap<<FF_TRANSSHIFT);
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

void BloodTime_OnChange();

CV_PossibleValue_t bloodtime_cons_t[]={{1,"MIN"},{3600,"MAX"},{0,NULL}};
// how much tics to last for the last (third) frame of blood (S_BLOODx)
consvar_t cv_bloodtime = {"bloodtime","20",CV_NETVAR|CV_CALL|CV_SAVE,bloodtime_cons_t,BloodTime_OnChange};

// Called when var. 'bloodtime' is changed : set the blood states duration
//
void BloodTime_OnChange()
{
    states[S_BLOOD1].tics = 8;
    states[S_BLOOD2].tics = 8;
    states[S_BLOOD3].tics = (cv_bloodtime.value*TICRATE) - 16;

    CONS_Printf ("blood lasts for %d seconds\n", cv_bloodtime.value);
}
