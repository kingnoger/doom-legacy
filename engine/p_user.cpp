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
// Revision 1.1  2002/11/16 14:18:04  hurdler
// Initial revision
//
// Revision 1.17  2002/09/20 22:41:33  vberghol
// Sound system rewritten! And it workscvs update
//
// Revision 1.16  2002/09/05 14:12:15  vberghol
// network code partly bypassed
//
// Revision 1.14  2002/08/25 18:22:00  vberghol
// little fixes
//
// Revision 1.13  2002/08/20 13:56:59  vberghol
// sdfgsd
//
// Revision 1.12  2002/08/17 21:21:53  vberghol
// Only scripting to be fixed in engine!
//
// Revision 1.11  2002/08/11 17:16:50  vberghol
// ...
//
// Revision 1.10  2002/08/08 12:01:29  vberghol
// pian engine on valmis!
//
// Revision 1.9  2002/08/06 13:14:25  vberghol
// ...
//
// Revision 1.8  2002/07/23 19:21:44  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.7  2002/07/10 19:57:02  vberghol
// g_pawn.cpp tehty
//
// Revision 1.6  2002/07/08 20:46:33  vberghol
// More files compile!
//
// Revision 1.5  2002/07/04 18:02:26  vberghol
// Pientä fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.4  2002/07/01 21:00:22  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:54  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.15  2001/05/27 13:42:48  bpereira
// no message
//
// Revision 1.14  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.11  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.10  2000/11/04 16:23:43  bpereira
// no message
//
// Revision 1.9  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   part of PlayerPawn class implementation
//      Bobbing POV/weapon, movement.
//      Pending weapon.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"

#include "g_game.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"

#include "command.h"
#include "p_camera.h"

#include "d_event.h"

#include "r_main.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_setup.h"
#include "m_random.h"



#include "hardware/hw3sound.h"


//
// Movement.
//


bool PlayerPawn::Teleport(fixed_t nx, fixed_t ny, angle_t nangle)
{
  bool ret = Actor::Teleport(nx,ny,nangle);

  // don't move for a bit
  if (!powers[pw_weaponlevel2])
    reactiontime = 18;
  // added : absolute angle position
  if (this == consoleplayer->pawn)
    localangle = nangle;
  if (this == displayplayer2->pawn)
    localangle2 = nangle;
#ifdef CLIENTPREDICTION2
  if (this == consoleplayer->pawn)
    {
      consoleplayer->spirit->reactiontime = reactiontime;
      CL_ResetSpiritPosition(this);
    }
#endif
  // move chasecam at new player location
  if (camera.chase && displayplayer == player)
    camera.ResetCamera(this);
    
  // FIXME code below is useless, right?
  // Adjust player's view, in case there has been a height change
  
  // Save the current deltaviewheight, used in stepping
  //  fixed_t deltaviewheight = player->deltaviewheight;
  // Clear deltaviewheight, since we don't want any changes
  //  player->deltaviewheight = 0;
  // Set player's view according to the newly set parameters
  //  CalcHeight(player);
  // Reset the delta to have the same dynamics as before
  // player->deltaviewheight = deltaviewheight;

  return ret;
}


//
// was P_Thrust
// Moves the given origin along a given angle.
//
void PlayerPawn::Thrust(angle_t angle, fixed_t move)
{
  angle >>= ANGLETOFINESHIFT;
  if (subsector->sector->special == 15
      && !(powers[pw_flight] && !(z <= floorz))) // Friction_Low
    {
      px += FixedMul(move>>2, finecosine[angle]);
      py += FixedMul(move>>2, finesine[angle]);
    }
  else
    {
      px += FixedMul(move, finecosine[angle]);
      py += FixedMul(move, finesine[angle]);
    }
}

#ifdef CLIENTPREDICTION2
//
// P_ThrustSpirit
// Moves the given origin along a given angle.
//
void P_ThrustSpirit(player_t *player, angle_t angle, fixed_t move)
{
  angle >>= ANGLETOFINESHIFT;
  if(player->spirit->subsector->sector->special == 15
     && !(player->powers[pw_flight] && !(player->spirit->z <= player->spirit->floorz))) // Friction_Low
    {
      player->spirit->px += FixedMul(move>>2, finecosine[angle]);
      player->spirit->py += FixedMul(move>>2, finesine[angle]);
    }
  else
    {
      player->spirit->px += FixedMul(move, finecosine[angle]);
      player->spirit->py += FixedMul(move, finesine[angle]);
    }
}
#endif



extern int ticruned,ticmiss;

#define JUMPGRAVITY     (6*FRACUNIT/NEWTICRATERATIO)

//
// was P_MovePlayer
//
void PlayerPawn::Move()
{
  extern int variable_friction;
  ticcmd_t *cmd;
  int       movefactor = 2048; //For Boom friction

  cmd = &player->cmd;

#ifndef ABSOLUTEANGLE
  angle += (cmd->angleturn<<16);
#else
  if (game.demoversion < 125)
    angle += (cmd->angleturn<<16);
  else
    angle = (cmd->angleturn<<16);
#endif

  ticruned++;

  if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
    ticmiss++;
  // Do not let the player control movement
  //  if not onground.
  bool onground = (z <= floorz) || (cheats & CF_FLYAROUND)
    || (flags2 & (MF2_ONMOBJ|MF2_FLY));

  if (game.demoversion < 128)
    {
      bool jumpover = cheats & CF_JUMPOVER;
      if (cmd->forwardmove && (onground || jumpover))
        {
	  // dirty hack to let the player avatar walk over a small wall
	  // while in the air
	  if (jumpover && pz > 0)
	    Thrust(angle, 5*2048);
	  else if (!jumpover)
	    Thrust(angle, cmd->forwardmove*2048);
        }
    
      if (cmd->sidemove && onground)
	Thrust(angle-ANG90, cmd->sidemove*2048);

      aiming = (signed char)cmd->aiming;
    }
  else
    {
      fixed_t  movepushforward=0, movepushside=0;
      aiming = cmd->aiming<<16;
      if (morphTics)
	movefactor = 2500;
      if (boomsupport && variable_friction)
        {
          //SoM: This seems to be buggy! Can anyone figure out why??
          movefactor = GetMoveFactor();
          //CONS_Printf("movefactor: %i\n", movefactor);
        }

      if (cmd->forwardmove)
        {
	  movepushforward = cmd->forwardmove * movefactor;
        
	  if (eflags & MF_UNDERWATER)
            {
	      // half forward speed when waist under water
	      // a little better grip if feets touch the ground
	      if (!onground)
		movepushforward >>= 1;
	      else
		movepushforward = movepushforward * 3/4;
            }
	  else
            {
	      // allow very small movement while in air for gameplay
	      if (!onground)
		movepushforward >>= 3;
            }

	  Thrust(angle, movepushforward);
        }

      if (cmd->sidemove)
        {
	  movepushside = cmd->sidemove * movefactor;
	  if (eflags & MF_UNDERWATER)
            {
	      if (!onground)
		movepushside >>= 1;
	      else
		movepushside = movepushside *3/4;
            }
	  else 
	    if (!onground)
	      movepushside >>= 3;

	  Thrust(angle-ANG90, movepushside);
        }

      // mouselook swim when waist underwater
      eflags &= ~MF_SWIMMING;
      if (eflags & MF_UNDERWATER)
        {
	  fixed_t a;
	  // swim up/down full move when forward full speed
	  a = FixedMul(movepushforward*50, finesine[aiming >> ANGLETOFINESHIFT] >>5 );
            
	  if ( a != 0 ) {
	    eflags |= MF_SWIMMING;
	    pz += a;
	  }
        }
    }

  //added:22-02-98: jumping
  if (cmd->buttons & BT_JUMP)
    {
      if (flags2 & MF2_FLY)
	flyheight = 10;
      else if (eflags & MF_UNDERWATER)
	//TODO: goub gloub when push up in water
	pz = JUMPGRAVITY/2;
      else if (onground && !(jumpdown & 1)) 
	// can't jump while in air, can't jump while jumping
	{
	  pz = JUMPGRAVITY;
	  if (!(cheats & CF_FLYAROUND))
	    {
	      S_StartScreamSound(this, sfx_jump);
	      // keep jumping ok if FLY mode.
	      jumpdown |= 1;
	    }
	}
    }
  else
    jumpdown &= ~1;


  if (cmd->forwardmove || cmd->sidemove)
    {
      if (morphTics)
        {
	  if (state == &states[S_CHICPLAY])
	    SetState(S_CHICPLAY_RUN1);
        }
      else if (state == &states[S_PLAY])
	SetState(S_PLAY_RUN1);
    }

  if (game.mode == heretic && (cmd->angleturn & BT_FLYDOWN))
    {
      flyheight = -10;
    }
  /* HERETODO
     fly = cmd->lookfly>>4;
     if(fly > 7)
     fly -= 16;
     if(fly && player->powers[pw_flight])
     {
     if(fly != TOCENTER)
     {
     player->flyheight = fly*2;
     if(!(flags2&MF2_FLY))
     {
     flags2 |= MF2_FLY;
     flags |= MF_NOGRAVITY;
     }
     }
     else
     {
     flags2 &= ~MF2_FLY;
     flags &= ~MF_NOGRAVITY;
     }
     }
     else if(fly > 0)
     {
     P_PlayerUseArtifact(player, arti_fly);
     }*/
  if (flags2 & MF2_FLY)
    {
      pz = flyheight*FRACUNIT;
      if (flyheight)
	flyheight /= 2;
    }
}


byte weapontobutton[NUMWEAPONS]={wp_fist    <<BT_WEAPONSHIFT,
                                 wp_pistol  <<BT_WEAPONSHIFT,
                                 wp_shotgun <<BT_WEAPONSHIFT,
                                 wp_chaingun<<BT_WEAPONSHIFT,
                                 wp_missile <<BT_WEAPONSHIFT,
                                 wp_plasma  <<BT_WEAPONSHIFT,
                                 wp_bfg     <<BT_WEAPONSHIFT,
				 (wp_fist    <<BT_WEAPONSHIFT) | BT_EXTRAWEAPON,// wp_chainsaw
				 (wp_shotgun <<BT_WEAPONSHIFT) | BT_EXTRAWEAPON};//wp_supershotgun

#ifdef CLIENTPREDICTION2

void CL_ResetSpiritPosition(Actor *mobj)
{
  P_UnsetThingPosition(mobj->player->spirit);
  mobj->player->spirit->x=mobj->x;
  mobj->player->spirit->y=mobj->y;
  mobj->player->spirit->z=mobj->z;
  mobj->player->spirit->px=0;
  mobj->player->spirit->py=0;
  mobj->player->spirit->pz=0;
  mobj->player->spirit->angle=mobj->angle;
  P_SetThingPosition(mobj->player->spirit);
}

void P_ProcessCmdSpirit (player_t* player,ticcmd_t *cmd)
{
  fixed_t   movepushforward=0,movepushside=0;
#ifdef PARANOIA
  if(!player)
    I_Error("P_MoveSpirit : player null");
  if(!player->spirit)
    I_Error("P_MoveSpirit : player->spirit null");
  if(!cmd)
    I_Error("P_MoveSpirit : cmd null");
#endif

  // don't move if dead
  if( player->playerstate != PST_LIVE )
    {
      cmd->angleturn &= ~TICCMD_XY;
      return;
    }
  onground = (player->spirit->z <= player->spirit->floorz) ||
    (player->cheats & CF_FLYAROUND);

  if (player->spirit->reactiontime)
    {
      player->spirit->reactiontime--;
      return;
    }

  player->spirit->angle = cmd->angleturn<<16;
  cmd->angleturn |= TICCMD_XY;
  /*
    // now weapon is allways send change is detected at receiver side
    if(cmd->buttons & BT_CHANGE) 
    {
    player->spirit->movedir = cmd->buttons & (BT_WEAPONMASK | BT_EXTRAWEAPON);
    cmd->buttons &=~BT_CHANGE;
    }
    else
    {
    if( player->pendingweapon!=wp_nochange )
    player->spirit->movedir=weapontobutton[player->pendingweapon];
    cmd->buttons&=~(BT_WEAPONMASK | BT_EXTRAWEAPON);
    cmd->buttons|=player->spirit->movedir;
    }
  */
  if (cmd->forwardmove)
    {
      movepushforward = cmd->forwardmove * movefactor;
        
      if (player->spirit->eflags & MF_UNDERWATER)
        {
	  // half forward speed when waist under water
	  // a little better grip if feets touch the ground
	  if (!onground)
	    movepushforward >>= 1;
	  else
	    movepushforward = movepushforward *3/4;
        }
      else
        {
	  // allow very small movement while in air for gameplay
	  if (!onground)
	    movepushforward >>= 3;
        }
        
      P_ThrustSpirit (player->spirit, player->spirit->angle, movepushforward);
    }
    
  if (cmd->sidemove)
    {
      movepushside = cmd->sidemove * movefactor;
      if (player->spirit->eflags & MF_UNDERWATER)
        {
	  if (!onground)
	    movepushside >>= 1;
	  else
	    movepushside = movepushside *3/4;
        }
      else 
	if (!onground)
	  movepushside >>= 3;
            
      P_ThrustSpirit (player->spirit, player->spirit->angle-ANG90, movepushside);
    }
    
  // mouselook swim when waist underwater
  player->spirit->eflags &= ~MF_SWIMMING;
  if (player->spirit->eflags & MF_UNDERWATER)
    {
      fixed_t a;
      // swim up/down full move when forward full speed
      a = FixedMul( movepushforward*50, finesine[ (cmd->aiming>>(ANGLETOFINESHIFT-16)) ] >>5 );
        
      if ( a != 0 ) {
	player->spirit->eflags |= MF_SWIMMING;
	player->spirit->pz += a;
      }
    }

  //added:22-02-98: jumping
  if (cmd->buttons & BT_JUMP)
    {
      // can't jump while in air, can't jump while jumping
      if (!(player->jumpdown & 2) &&
	  (onground || (player->spirit->eflags & MF_UNDERWATER)) )
        {
	  if (onground)
	    player->spirit->pz = JUMPGRAVITY;
	  else //water content
	    player->spirit->pz = JUMPGRAVITY/2;

	  //TODO: goub gloub when push up in water
            
	  if ( !(player->cheats & CF_FLYAROUND) && onground && !(player->spirit->eflags & MF_UNDERWATER))
            {
	      S_StartScreamSound(player->spirit, sfx_jump);

	      // keep jumping ok if FLY mode.
	      player->jumpdown |= 2;
            }
        }
    }
  else
    player->jumpdown &= ~2;

}

void P_MoveSpirit (player_t* p,ticcmd_t *cmd, int realtics)
{
  if( gamestate != GS_LEVEL )
    return;
  if(p->spirit)
    {
      extern bool supdate;
      int    i;

      p->spirit->flags|=MF_SOLID;
      for(i=0;i<realtics;i++)
        {
	  P_ProcessCmdSpirit(p,cmd);
	  P_MobjThinker(p->spirit);
        }                 
      p->spirit->flags&=~MF_SOLID;
      P_CalcHeight (p);                 // z-bobing of player
      A_TicWeapon(p, &p->psprites[0]);  // bobing of weapon
      cmd->x=p->spirit->x;
      cmd->y=p->spirit->y;
      supdate=true;
    }
  else
    if(p->mo)
      {
        cmd->x=p->mo->x;
        cmd->y=p->mo->y;
      }
}

#endif



//----------------------------------------------------------------------------
//
// PROC P_ArtiTele
//
//----------------------------------------------------------------------------

void P_ArtiTele(PlayerPawn *p)
{
  int i, n = p->mp->dmstarts.size();
  fixed_t destX;
  fixed_t destY;
  angle_t destAngle;
  extern  consvar_t  cv_deathmatch;


  if (cv_deathmatch.value)
    {
      i = P_Random() % n;
      destX = p->mp->dmstarts[i]->x << FRACBITS;
      destY = p->mp->dmstarts[i]->y << FRACBITS;
      destAngle = ANG45*(p->mp->dmstarts[i]->angle/45);
    }
  else
    {
      destX = p->mp->playerstarts[0]->x<<FRACBITS;
      destY = p->mp->playerstarts[0]->y<<FRACBITS;
      destAngle = ANG45*(p->mp->playerstarts[0]->angle/45);
    }
  p->Teleport(destX, destY, destAngle);
  S_StartAmbSound(sfx_wpnup); // Full volume laugh
}


//----------------------------------------------------------------------------
//
// FUNC P_UseArtifact
//
// Returns true if artifact was used.
//
//----------------------------------------------------------------------------

bool P_UseArtifact(PlayerPawn *p, artitype_t arti)
{
  Actor *mo;
    
  switch(arti)
    {
    case arti_invulnerability:
      if(!p->GivePower(pw_invulnerability))
        {
	  return false;
        }
      break;
    case arti_invisibility:
      if(!p->GivePower(pw_invisibility))
        {
	  return false;
        }
      break;
    case arti_health:
      if(!p->GiveBody(25))
        {
	  return false;
        }
      break;
    case arti_superhealth:
      if(!p->GiveBody(100))
        {
	  return false;
        }
      break;
    case arti_tomeofpower:
      if (p->morphTics)
        { // Attempt to undo chicken
	  if (p->UndoMorph() == false)
            { // Failed
	      p->Damage(NULL, NULL, 10000);
            }
	  else
            { // Succeeded
	      p->morphTics = 0;
	      S_StartScreamSound(p, sfx_wpnup);
            }
        }
      else
        {
	  if (!p->GivePower(pw_weaponlevel2))
            {
	      return false;
            }
	  if (p->readyweapon == wp_staff)
            {
	      p->SetPsprite(ps_weapon, S_STAFFREADY2_1);
            }
	  else if (p->readyweapon == wp_gauntlets)
            {
	      p->SetPsprite(ps_weapon, S_GAUNTLETREADY2_1);
            }
        }
      break;
    case arti_torch:
      if(!p->GivePower(pw_infrared))
        {
	  return false;
        }
      break;
    case arti_firebomb:
      {
	angle_t ang = p->angle >> ANGLETOFINESHIFT;
	mo = p->mp->SpawnActor(p->x+24*finecosine[ang], p->y+24*finesine[ang],
			 p->z - 15*FRACUNIT*((p->flags2&MF2_FEETARECLIPPED) != 0), MT_FIREBOMB);
	mo->target = p;
      }
      break;
    case arti_egg:
      p->SpawnPlayerMissile(MT_EGGFX);
      p->SPMAngle(MT_EGGFX, p->angle-(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->angle+(ANG45/6));
      p->SPMAngle(MT_EGGFX, p->angle-(ANG45/3));
      p->SPMAngle(MT_EGGFX, p->angle+(ANG45/3));
      break;
    case arti_fly:
      if(!p->GivePower(pw_flight))
        {
	  return false;
        }
      break;
    case arti_teleport:
      P_ArtiTele(p);
      break;
    default:
      return false;
    }
  return true;
}
