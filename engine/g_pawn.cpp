// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//  $Id$
//
// Copyright (C) 1998-2002 by DooM Legacy Team.
//
// $Log$
// Revision 1.7  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.6  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.5  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
//
// DESCRIPTION:
//   Pawn / PlayerPawn class implementation

#include "g_pawn.h"
#include "g_player.h"
#include "g_game.h"
#include "g_map.h"

#include "d_netcmd.h" // consvars
#include "dstrings.h"

#include "p_camera.h" // camera
#include "p_spec.h" // boom stuff

#include "s_sound.h"
#include "sounds.h"
#include "hardware/hw3sound.h" // ugh.
#include "hu_stuff.h" // HUD
#include "tables.h" // angle
#include "r_main.h" // PointToAngle functions FIXME which do not belong there
#include "m_random.h"


// Pawn methods

// virtuals
void Pawn::Think() {}
bool Pawn::Morph() { return false; }
bool Pawn::Damage(Actor *inflictor, Actor *source, int damage, int dtype)
{ return false; }

// PlayerPawn methods

int PlayerPawn::Serialize(LArchive & a)
{
  return 0;
}

void P_UpdateBeak(PlayerPawn *p, pspdef_t *psp);
void P_PostChickenWeapon(PlayerPawn *player, weapontype_t weapon);

// added 2-2-98 for hacking with dehacked patch
int initial_health=100; //MAXHEALTH;
int initial_bullets=50;

// creates a pawn based on a Doom/Heretic mobj
Pawn::Pawn(fixed_t x, fixed_t y, fixed_t z, const pawn_info_t *t)
  : Actor(x, y, z)
{
  pinfo = t;
  const mobjinfo_t *info = &mobjinfo[t->mt];
  mass   = info->mass;
  radius = info->radius;
  height = info->height;
  flags  = info->flags;
  flags2 = info->flags2;
  health = info->spawnhealth;
  maxhealth = 2*health;
  reactiontime = info->reactiontime;

  //FIXME remove this and just set the correct sprite/model here...  
  /*
  state = &states[info->spawnstate];
  tics = state->tics;
  */
  state_t *state = &states[info->spawnstate];
  sprite = state->sprite;
  frame = state->frame; // FF_FRAMEMASK for frame, and other bits..
}

PlayerPawn::PlayerPawn(fixed_t nx, fixed_t ny, fixed_t nz, const pawn_info_t *t)
  : Pawn(nx, ny, nz, t)
{
  // note! here Map *mp is not yet set! This means you can't call functions such as
  // SetPosition that have something to do with a map.
  refire = 0;
  message = NULL;
  morphTics = 0;
  flamecount = 0;
  flyheight = 0;
  rain1 = NULL;
  rain2 = NULL;
  extralight = 0;
  fixedcolormap = 0;
  invSlot = 0;
  inventory.resize(2, inventory_t(3,2)); // at least 1 empty slot
  flags |= (MF_NOTMONSTER | MF_PICKUP | MF_SHOOTABLE | MF_DROPOFF);
  flags &= ~MF_COUNTKILL;
  // the playerpawn is not a monster. MT_PLAYER might be.
  flags2 |= (MF2_WINDTHRUST | MF2_SLIDE | MF2_PASSMOBJ | MF2_TELESTOMP);

  usedown = attackdown = true;  // don't do anything immediately

  weaponinfo = wpnlev1info;
  maxammo = maxammo1;

  if (game.mode == gm_heretic)
    {
      weaponowned[wp_staff] = true;
      /*
      readyweapon = pendingweapon = wp_goldwand;
      weaponowned[wp_goldwand] = true;
      ammo[am_goldwand] = initial_bullets;
      */
    }
  else
    {
      weaponowned[wp_fist] = true;
      /*
      readyweapon = pendingweapon = wp_pistol;
      weaponowned[wp_pistol] = true;
      ammo[am_clip] = initial_bullets;
      */
    }
  weapontype_t w = pinfo->bweapon;
  if (w != wp_nochange)
    {
      weaponowned[w] = true;
      readyweapon = pendingweapon = w;
      ammotype_t a = wpnlev1info[w].ammo;
      if (a != am_noammo)
	ammo[a] = pinfo->bammo;
    }
}


//--------------------------------------------------------
// was P_PlayerThink

#define BLINKTHRESHOLD  (4*32) // for powers

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP  32

void PlayerPawn::Think()
{
  int                 waterz;

  ticcmd_t* cmd = &player->cmd;

  // a corpse, for example. Thinks just like an actor.
  if (!player)
    goto actor_think;

  // inventory
  if (invTics)
    invTics--;

  // Turn on required cheats
  if (cheats & CF_NOCLIP)
    flags |= MF_NOCLIP;
  else
    flags &= ~MF_NOCLIP;

  // chain saw run forward
  if (flags & MF_JUSTATTACKED)
    {
// added : now angle turn is a absolute value not relative
#ifndef ABSOLUTEANGLE
      cmd->angleturn = 0;
#endif
      cmd->forwardmove = 0xc800/512;
      cmd->sidemove = 0;
      flags &= ~MF_JUSTATTACKED;
    }

  //if (player->playerstate == PST_REBORN) I_Error("player %d is in PST_REBORN\n"); // debugging...

  if (player->playerstate == PST_DEAD)
    {
      DeathThink();
      goto actor_think;
    }

  // morphed state?
  if (morphTics)
    MorphThink();

  // CheckWater is also called at Actor::Think later...
  CheckWater();

  // Move around.
  // Reactiontime is used to prevent movement
  //  for a bit after a teleport.
  if (reactiontime > 0)
    reactiontime--;
  else
    Move();

  //added:22-02-98: bob view only if looking by the marine's eyes
#ifndef CLIENTPREDICTION2
  if (!camera.chase)
    CalcHeight(z <= floorz);
#endif

  // check special sectors : damage & secrets
  PlayerInSpecialSector();

  // water splashes
  if (specialsector >= 887 && specialsector <= 888)
    {
      if ((px > (2*FRACUNIT) || px < (-2*FRACUNIT) ||
	   py > (2*FRACUNIT) || py < (-2*FRACUNIT) ||
	   pz >  (2*FRACUNIT)) &&  // jump out of water
	  !(gametic % (32 * NEWTICRATERATIO)))
        {
	  //
	  // make sure we disturb the surface of water (we touch it)
	  //
	  if (specialsector == 887)
	    //FLAT TEXTURE 'FWATER'
	    waterz = subsector->sector->floorheight + (FRACUNIT/4);
	  else
	    //FIXME: no hacks! faB's current water hack using negative sector tags
	    waterz = - (subsector->sector->tag << FRACBITS);

	  // half in the water
	  if (eflags & MF_TOUCHWATER)
            {
	      if (z <= floorz) // onground
                {
		  fixed_t whater_height = waterz-subsector->sector->floorheight;

		  if( whater_height<(height>>2 ))
		    S_StartSound(this, sfx_splash);
		  else
		    S_StartSound(this, sfx_floush);
                }
	      else
		S_StartSound(this, sfx_floush);
            }                   
        }
    }

  // Check for weapon change.
  if (cmd->buttons & BT_CHANGE)
    {
      // The actual changing of the weapon is done
      //  when the weapon psprite can do it
      //  (read: not in the middle of an attack).
      int wg = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;
      weapontype_t newweapon;
      int i, j;
      if (weapondata[readyweapon].group == wg)
	{
	  for (i=0; i<4; i++) // find next weapon in the group
	    if (wgroups[wg][i] == readyweapon)
	      break;
	  i++;
	}
      else
	i = 0; // choose first weapon in group

      for (j=0; j<4; j++, i++)
	{
	  newweapon = wgroups[wg][i%4];
	  if ((newweapon < NUMWEAPONS) && weaponowned[newweapon])
	    {
	      pendingweapon = newweapon;
	      break;
	    }
	}

      /*
	//if (cmd->buttons & BT_EXTRAWEAPON)
	switch (newweapon)
	{
	case wp_fist:
	  if (weaponowned[wp_chainsaw] && (readyweapon == wp_fist))
	    newweapon = wp_chainsaw;
	  break;
	case wp_shotgun: 
	  if (game.mode == commercial && weaponowned[wp_supershotgun] && (readyweapon == wp_shotgun))
	    newweapon = wp_supershotgun;
	  break;
	default:
	  break;
	  }
      */

      // Do not go to plasma or BFG in shareware, even if cheated.
      if ((game.mode == gm_doom1s) &&
	  (pendingweapon == wp_plasma || pendingweapon == wp_bfg))
	pendingweapon = wp_nochange;
    }

  // check for use
  if (cmd->buttons & BT_USE)
    {
      if (!usedown)
        {
	  UseLines();
	  usedown = true;
        }
    }
  else
    usedown = false;

  // morph counter
  if (morphTics)
    {
      // Chicken attack counter
      if (chickenPeck)
	chickenPeck -= 3;
      // Attempt to undo the chicken
      if (--morphTics == 0)
	UndoMorph();
    }

  // cycle psprites
  MovePsprites();

  // Counters, time dependend power ups.
  // Strength counts up to diminish fade.
  if (powers[pw_strength])
    powers[pw_strength]++;

  if (powers[pw_invulnerability])
    powers[pw_invulnerability]--;

  // the MF_SHADOW activates the tr_transhi translucency while it is set
  // (it doesnt use a preset value through FF_TRANSMASK)
  if (powers[pw_invisibility])
    if (--powers[pw_invisibility] == 0)
      flags &= ~MF_SHADOW;

  if (powers[pw_infrared])
    powers[pw_infrared]--;

  if (powers[pw_ironfeet])
    powers[pw_ironfeet]--;

  if (powers[pw_flight])
    {
      if(--powers[pw_flight] == 0)
	{
	  flags2 &= ~MF2_FLY;
	  flags &= ~MF_NOGRAVITY;
	}
    }

  if(powers[pw_weaponlevel2])
    {
      if(--powers[pw_weaponlevel2] == 0)
	{
	  weaponinfo = wpnlev1info;
	  // end of weaponlevel2 power
	  if (readyweapon == wp_phoenixrod
	      && (psprites[ps_weapon].state != &states[S_PHOENIXREADY])
	      && (psprites[ps_weapon].state != &states[S_PHOENIXUP]))
	    {
	      SetPsprite(ps_weapon, S_PHOENIXREADY);
	      ammo[am_phoenixrod] -= USE_PHRD_AMMO_2;
	      refire = 0;
	    }
	  else if ((readyweapon == wp_gauntlets) || (readyweapon == wp_staff))
	    pendingweapon = readyweapon;
	}
    }

  // damage and bonus counts are now HUD properties and are ticked there

  // Handling colormaps.
  if (powers[pw_invulnerability])
    {
      if (powers[pw_invulnerability] > BLINKTHRESHOLD
	  || (powers[pw_invulnerability] & 8))
	fixedcolormap = INVERSECOLORMAP;
      else
	fixedcolormap = 0;
    }
  else if (powers[pw_infrared])
    {
      if (powers[pw_infrared] > BLINKTHRESHOLD
	  || (powers[pw_infrared] & 8))
        {
	  // almost full bright
	  fixedcolormap = 1;
        }
      else
	fixedcolormap = 0;
    }
  else
    fixedcolormap = 0;

  // this is where the "actor part" of the thinking begins
 actor_think:

#ifdef CLIENTPREDICTION2
  compiler_error_sentinel; // is clientprediction2 used at all
  // move player mobj (not the spirit) to spirit position (sent by ticcmd)
  if ((player->cmd.angleturn & (TICCMD_XY|TICCMD_RECEIVED) == TICCMD_XY|TICCMD_RECEIVED) && 
      (player->playerstate == PST_LIVE))
    {
      int oldx = x, oldy = y;

      if (oldx != player->cmd.x || oldy != player->cmd.y)
        {
	  eflags |= MF_NOZCHECKING;
	  // cross special lines and pick up things
	  if (!TryMove(player->cmd.x, player->cmd.y, true))
            {
	      // P_TryMove fail mean cannot change mobj position to requestied position
	      // the mobj is blocked by something
	      if (player == consoleplayer)
                {
		  // reset spirit possition
		  CL_ResetSpiritPosition(mobj);

		  //if(devparm)
		  CONS_Printf("\2MissPrediction\n");
                }
            }
	  eflags &= ~MF_NOZCHECKING;
        }
      XYFriction(oldx, oldy, false);
    }
  else
#endif

    // we call Actor::Think(), because a playerpawn is an actor too
    Actor::Think();
}


//--------------------------------------------------------
// was P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
#define ANG5    (ANG90/18)

//static bool onground;

void PlayerPawn::DeathThink()
{
  MovePsprites();

  // fall to the ground
  if (player->viewheight > 6*FRACUNIT)
    player->viewheight -= FRACUNIT;

  if (player->viewheight < 6*FRACUNIT)
    player->viewheight = 6*FRACUNIT;

  player->deltaviewheight = 0;

  CalcHeight(z <= floorz);

  // watch my killer (if there is one)
  if (attacker != NULL && attacker != this)
    {
      angle_t ang = R_PointToAngle2(x, y, attacker->x, attacker->y);
      angle_t delta = ang - angle;

      if (delta < ANG5 || delta > (unsigned)-ANG5)
        {
	  // Looking at killer,
	  //  so fade damage flash down.
	  angle = ang;
        }
      else if (delta < ANG180)
	angle += ANG5;
      else
	angle -= ANG5;

      // change aiming to look up or down at the attacker (DOESNT WORK)
      // FIXME : the aiming returned seems to be too up or down...
      //fixed_t dist = P_AproxDistance(attacker->x - x, attacker->y - y);
      //if (dist)
      //  pitch = FixedMul ((160<<FRACBITS), FixedDiv (attacker->z + (attacker->height>>1), dist)) >>FRACBITS;
      //else pitch = 0;
      int pitch = (attacker->z - z) >> 17;
      aiming = G_ClipAimingPitch(&pitch);
    }
  
  if (player->cmd.buttons & BT_USE)
    {
      mp->RebornPlayer(player);
      special2 = 666;
    }
}



//----------------------------------------------
// was P_ChickenPlayerThink
//

void PlayerPawn::MorphThink()
{
  if (health > 0)
    { // Handle beak movement
      P_UpdateBeak(this, &psprites[ps_weapon]);
    }

  if (morphTics & 15)
    return;

  if (!(px+py) && P_Random() < 160)
    { // Twitch view angle
      angle += P_SignedRandom() << 19;
    }
  if ((z <= floorz) && (P_Random() < 32))
    { // Jump and noise
      pz += FRACUNIT;
      // FIXME set pain state here...
      // SetState(S_CHICPLAY_PAIN);
      return;
    }
  if(P_Random() < 48)
    { // Just noise
      S_StartScreamSound(this, sfx_chicact);
    }
}


//-----------------------------------------

void PlayerPawn::XYMovement()
{
  fixed_t oldx, oldy;
  oldx = x;
  oldy = y;

  Actor::XYMovement();

  // slow down
  if (cheats & CF_NOMOMENTUM)
    {
      // debug option for no sliding at all
      px = py = 0;
      return;
    }
  else if ((cheats & CF_FLYAROUND) || (flags2 & MF2_FLY))
    {
      //XYFriction(oldx, oldy, true);
      return;
    }
  //        if (z <= subsector->sector->floorheight)
  //          XYFriction(oldx, oldy, false);

}

void PlayerPawn::ZMovement()
{
  // check for smooth step up
#ifdef CLIENTPREDICTION2
  if (player && z < floorz && type != MT_PLAYER)
#else
  if (player && (z < floorz)) //  && type != MT_SPIRIT)
#endif
    {
      player->viewheight -= floorz - z;

      player->deltaviewheight
	= ((cv_viewheight.value<<FRACBITS) - player->viewheight)>>3;
    }

  fixed_t oldz, oldpz;
  oldz = z;
  oldpz = pz;

  Actor::ZMovement();

  if (oldz + oldpz <= floorz && (oldpz < 0)) // falling
    {
      if ((oldpz < -8*FRACUNIT) && !(flags2 & MF2_FLY))
	{
	  // Squat down.
	  // Decrease viewheight for a moment
	  // after hitting the ground (hard),
	  // and utter appropriate sound.
	  player->deltaviewheight = oldpz>>3;
	  S_StartSound(this, sfx_oof);
	}
    }

  if (oldz+oldpz + height > ceilingz)
    {
      // player avatar hits his head on the ceiling, ouch!
      if (!(cheats & CF_FLYAROUND) && !(flags2 & MF2_FLY) && pz > 8*FRACUNIT)
	S_StartSound(this, sfx_ouch);
    }
}

#define STOPSPEED               (0x1000/NEWTICRATERATIO)

void PlayerPawn::XYFriction(fixed_t oldx, fixed_t oldy, bool oldfriction)
{
  if (player == NULL)
    return;

  if (px > -STOPSPEED && px < STOPSPEED && py > -STOPSPEED && py < STOPSPEED && 
      (player->cmd.forwardmove == 0 && player->cmd.sidemove == 0 ))
    {
      // if in a walking frame, stop moving
      // FIXME set states accordingly
      if (morphTics)
	{
	  //if((unsigned)((state - states) -S_CHICPLAY_RUN1) < 4) SetState(S_CHICPLAY);
	}
      else
	{
	  //if((unsigned)((state - states) -S_PLAY_RUN1) < 4) SetState(S_PLAY);
	}
    
      px = 0;
      py = 0;
    }
  else Actor::XYFriction(oldx, oldy);
}



//-----------------------------------------------------
//
// was G_PlayerFinishLevel
//  Called when a player completes a level alive.
//  throws away extra items, removes powers, keys, curses

void PlayerPawn::FinishLevel()
{
  int i, n;

  n = inventory.size();
  for (i=0; i<n; i++)
    if (inventory[i].count > 1) 
      inventory[i].count = 1; // why? not fair

  if (!cv_deathmatch.value)
    for(i = 0; i < MAXARTECONT; i++)
      UseArtifact(arti_fly);
  memset(powers, 0, sizeof (powers));

  weaponinfo = wpnlev1info;    // cancel power weapons
  cards = 0;
  flags &= ~MF_SHADOW;         // cancel invisibility
  extralight = 0;                  // cancel gun flashes
  fixedcolormap = 0;               // cancel ir gogles

  if (morphTics)
    {
      readyweapon = (weapontype_t)special1; // Restore weapon
      morphTics = 0;
    }
  rain1 = NULL; //FIXME! instead rain actor should have an owner playerpawn!
  rain2 = NULL;

  // save pawn for next level
  mp->DetachActor(this);
}


bool P_UseArtifact(PlayerPawn *player, artitype_t arti);

//----------------------------------------------------------------------------
// was partly P_PlayerNextArtifact
// was partly P_PlayerRemoveArtifact
// was P_PlayerUseArtifact

void PlayerPawn::UseArtifact(artitype_t arti)
{
  extern int st_curpos;
  int n;
  vector<inventory_t>::iterator i;

  CONS_Printf("USING arti %d\n", arti);
  for(i = inventory.begin(); i < inventory.end(); i++) 
    if (i->type == arti)
      { // Found match - try to use
	CONS_Printf("USING 2\n");
	if (P_UseArtifact(this, arti))
	  { // Artifact was used - remove it from inventory
	    CONS_Printf("USING 3\n");
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

	    if (this == displayplayer->pawn
		|| this == displayplayer2->pawn)
	      {
		S_StartSound(this, sfx_artiuse);
		hud.itemuse = 4;
	      }
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

#define WPNLEV2TICS     (40*TICRATE)
#define FLIGHTTICS      (60*TICRATE)




// was P_GivePower
//
bool PlayerPawn::GivePower(int /*powertype_t*/ power)
{
  if (power == pw_invulnerability)
    {
      // Already have it
      if (game.inventory && powers[power] > BLINKTHRESHOLD)
	return false;

      powers[power] = INVULNTICS;
      return true;
    }
  if(power == pw_weaponlevel2)
    {
      // Already have it
      if (game.inventory && powers[power] > BLINKTHRESHOLD)
	return false;

      powers[power] = WPNLEV2TICS;
      weaponinfo = wpnlev2info;
      return true;
    }
  if (power == pw_invisibility)
    {
      // Already have it
      if (game.inventory && powers[power] > BLINKTHRESHOLD)
	return false;

      powers[power] = INVISTICS;
      flags |= MF_SHADOW;
      return true;
    }
  if (power == pw_flight)
    {
      // Already have it
      if (powers[power] > BLINKTHRESHOLD)
	return(false);
      powers[power] = FLIGHTTICS;
      flags2 |= MF2_FLY;
      flags |= MF_NOGRAVITY;
      if(z <= floorz)
	flyheight = 10; // thrust the player in the air a bit
      return(true);
    }
  if (power == pw_infrared)
    {
      // Already have it
      if(powers[power] > BLINKTHRESHOLD)
	return(false);

      powers[power] = INFRATICS;
      return true;
    }

  if (power == pw_ironfeet)
    {
      powers[power] = IRONTICS;
      return true;
    }

  if (power == pw_strength)
    {
      GiveBody(100);
      powers[power] = 1;
      return true;
    }

  if (powers[power] != 0)
    return false;   // already got it

  powers[power] = 1;
  return true;
}


// was P_GiveBody
// Returns false if the body isn't needed at all
//
bool Pawn::GiveBody(int num)
{
#define MAXCHICKENHEALTH 30

  //if (morphTics) max = MAXCHICKENHEALTH; // FIXME maxhealth must be updated

  if (health >= maxhealth)
    return false;

  health += num;
  if (health > maxhealth)
    health = maxhealth;

  return true;
}


//----------------------------------------------------------------------------
// was P_UndoPlayerChicken

bool PlayerPawn::UndoMorph()
{
  DActor *mo = mp->SpawnDActor(x, y, z, mobjtype_t(player->pawntype));

  if (mo->TestLocation() == false)
    { // Didn't fit
      mo->Remove();
      morphTics = 2*35;
      // some sound to indicate unsuccesful morph?
      return false;
    }

  maxhealth = mo->health;
  mo->Remove(); // it is used just to check if a player fits here

  reactiontime = 18;
  morphTics = 0;
  powers[pw_weaponlevel2] = 0;
  //weapon = weapontype_t(special1);
  weaponinfo = wpnlev1info;
  health = maxhealth;
  angle_t ang = angle >> ANGLETOFINESHIFT;
  DActor *fog = mp->SpawnDActor(x+20*finecosine[ang], y+20*finesine[ang],
				z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_telept);
  P_PostChickenWeapon(this, weapontype_t(special1));

  /*
  Actor *mo, *pmo;
  fixed_t x, y, z;
  angle_t angle;
  int playerNum;
  weapontype_t weapon;
  int oldFlags;
  int oldFlags2;

  pmo = this;
  x = pmo->x;
  y = pmo->y;
  z = pmo->z;
  angle = pmo->angle;
  weapon = weapontype_t(pmo->special1);
  oldFlags = pmo->flags;
  oldFlags2 = pmo->flags2;

  pmo->SetState(S_FREETARGMOBJ); //FIXME! what's a freetargmobj?
  // can't i just change the chicken into a marine?
  mo = P_SpawnMobj(x, y, z, MT_PLAYER);

  if (P_TestMobjLocation(mo) == false)
    { // Didn't fit
      mo->Remove();
      mo = mp->SpawnActor(x, y, z, MT_CHICPLAYER);
      mo->angle = angle;
      mo->health = player->health;
      mo->special1 = weapon;
      mo->player = player;
      mo->flags = oldFlags;
      mo->flags2 = oldFlags2;
      player->mo = mo;
      player->morphTics = 2*35;
      return(false);
    }

  playerNum = player-players;
  if(playerNum != 0)
    { // Set color translation
      mo->flags |= playerNum<<MF_TRANSSHIFT;
    }
  mo->angle = angle;
  mo->player = player;
  mo->reactiontime = 18;
  if(oldFlags2 & MF2_FLY)
    {
      mo->flags2 |= MF2_FLY;
      mo->flags |= MF_NOGRAVITY;
    }
  player->morphTics = 0;
  player->powers[pw_weaponlevel2] = 0;
  player->weaponinfo = wpnlev1info;
  player->health = mo->health = max_health;
  player->mo = mo;
  angle >>= ANGLETOFINESHIFT;
  Actor *fog = P_SpawnMobj(x+20*finecosine[angle], y+20*finesine[angle],
			   z+TELEFOGHEIGHT, MT_TFOG);
  S_StartSound(fog, sfx_telept);
  P_PostChickenWeapon(player, weapon);
  */
  return true;
}






// 16 pixels of bob
#define MAXBOB  0x100000

//
// was P_CalcHeight
// Calculate the walking / running height adjustment
//
void PlayerPawn::CalcHeight(bool onground)
{
  // Regular movement bobbing
  // (needs to be calculated for gun swing
  // even if not on ground)
  // OPTIMIZE: tablify angle
  // Note: a LUT allows for effects
  //  like a ramp with low health.

  PlayerInfo *pl = player;

#ifdef CLIENTPREDICTION2
  if( p->spirit )
    mo = p->spirit;
#endif

  if ((flags2 & MF2_FLY) && !onground)
    pl->bob = FRACUNIT/2;
  else
    {
      pl->bob = ((FixedMul(px,px) + FixedMul (py,py))*NEWTICRATERATIO) >> 2;

      if (pl->bob > MAXBOB)
	pl->bob = MAXBOB;
    }

  if ((cheats & CF_NOMOMENTUM) || z > floorz)
    {
      //added:15-02-98: it seems to be useless code!
      //pl->viewz = p->mo->z + (cv_viewheight.value<<FRACBITS);

      //if (p->viewz > p->mo->ceilingz-4*FRACUNIT)
      //    p->viewz = p->mo->ceilingz-4*FRACUNIT;
      pl->viewz = z + pl->viewheight;
      return;
    }

  int ang = (FINEANGLES/20*gametic/NEWTICRATERATIO) & FINEMASK;
  fixed_t bob = FixedMul(pl->bob/2, finesine[ang]);

  // move viewheight
  fixed_t viewheight = cv_viewheight.value << FRACBITS; // default eye view height

  if (pl->playerstate == PST_LIVE)
    {
      pl->viewheight += pl->deltaviewheight;

      if (pl->viewheight > viewheight)
        {
	  pl->viewheight = viewheight;
	  pl->deltaviewheight = 0;
        }

      if (pl->viewheight < viewheight/2)
        {
	  pl->viewheight = viewheight/2;
	  if (pl->deltaviewheight <= 0)
	    pl->deltaviewheight = 1;
        }

      if (pl->deltaviewheight)
        {
	  pl->deltaviewheight += FRACUNIT/4;
	  if (!pl->deltaviewheight)
	    pl->deltaviewheight = 1;
        }
    }   

  if (morphTics)
    pl->viewz = z + pl->viewheight-(20*FRACUNIT);
  else
    pl->viewz = z + pl->viewheight + bob;

  if (flags2 & MF2_FEETARECLIPPED && pl->playerstate != PST_DEAD && z <= floorz)
    pl->viewz -= FOOTCLIPSIZE;

  if (pl->viewz > ceilingz-4*FRACUNIT)
    pl->viewz = ceilingz-4*FRACUNIT;
  if (pl->viewz < floorz+4*FRACUNIT)
    pl->viewz = floorz+4*FRACUNIT;
}



//
// was P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
DActor *PlayerPawn::SPMAngle(mobjtype_t type, angle_t ang)
{
  extern consvar_t cv_allowautoaim;
  extern Actor *linetarget;

  fixed_t  mx, my, mz;
  fixed_t  slope = 0;

  if (player->autoaim && cv_allowautoaim.value)
    {
      // see which target is to be aimed at
      slope = AimLineAttack (ang, 16*64*FRACUNIT);

      if (!linetarget)
        {
	  ang += 1<<26;
	  slope = AimLineAttack (ang, 16*64*FRACUNIT);

	  if (!linetarget)
            {
	      ang -= 2<<26;
	      slope = AimLineAttack (ang, 16*64*FRACUNIT);
            }

	  if (!linetarget)
	    slope = 0;
        }
    }

  //added:18-02-98: if not autoaim, or if the autoaim didnt aim something,
  //                use the mouseaiming
  if (!(player->autoaim && cv_allowautoaim.value) || !linetarget)
    {
      slope = AIMINGTOSLOPE(aiming);
    }

  mx = x;
  my = y;
  mz = z + 4*8*FRACUNIT;
  if (flags2 & MF2_FEETARECLIPPED)
    z -= FOOTCLIPSIZE;

  DActor *th = mp->SpawnDActor(mx, my, mz, type);

  if (th->info->seesound)
    S_StartSound(th, th->info->seesound);

  th->owner = this;

  th->angle = ang;
  th->px = FixedMul(th->info->speed, finecosine[ang>>ANGLETOFINESHIFT]);
  th->py = FixedMul(th->info->speed, finesine[ang>>ANGLETOFINESHIFT]);
    
  th->px = FixedMul(th->px, finecosine[aiming>>ANGLETOFINESHIFT]);
  th->py = FixedMul(th->py, finecosine[aiming>>ANGLETOFINESHIFT]);

  th->pz = FixedMul(th->info->speed, slope);

  if (th->type == MT_BLASTERFX1)
    { // Ultra-fast ripper spawning missile
      th->x += (th->px>>3)-(th->px>>1);
      th->y += (th->py>>3)-(th->py>>1);
      th->z += (th->pz>>3)-(th->pz>>1);
    }

  slope = th->CheckMissileSpawn();

  if( game.demoversion<131 )
    return th;
  else
    return slope ? th : NULL;
}


//SoM: 3/7/2000
//
// was P_CanUnlockGenDoor()
//
// Passed a generalized locked door linedef and a player, returns whether
// the player has the keys necessary to unlock that door.
//
// Note: The linedef passed MUST be a generalized locked door type
//       or results are undefined.
//
//
bool PlayerPawn::CanUnlockGenDoor(line_t *line)
{
  // does this line special distinguish between skulls and keys?
  int skulliscard = (line->special & LockedNKeys)>>LockedNKeysShift;
  bool ok = true;

  // determine for each case of lock type if player's keys are adequate
  switch ((line->special & LockedKey)>>LockedKeyShift)
    {
    case AnyKey_:
      if (!(cards & it_redcard) &&
	  !(cards & it_redskull) &&
	  !(cards & it_bluecard) &&
	  !(cards & it_blueskull) &&
	  !(cards & it_yellowcard) &&
	  !(cards & it_yellowskull))
	{
	  message = PD_ANY;
	  ok = false;
	}
      break;
    case RCard:
      if
	(
	 !(cards & it_redcard) &&
	 (!skulliscard || !(cards & it_redskull))
	 )
	{
	  message = skulliscard? PD_REDK : PD_REDC;
	  ok = false;
	}
      break;
    case BCard:
      if
	(
	 !(cards & it_bluecard) &&
	 (!skulliscard || !(cards & it_blueskull))
	 )
	{
	  message = skulliscard? PD_BLUEK : PD_BLUEC;
	  ok = false;
	}
      break;
    case YCard:
      if
	(
	 !(cards & it_yellowcard) &&
	 (!skulliscard || !(cards & it_yellowskull))
	 )
	{
	  message = skulliscard? PD_YELLOWK : PD_YELLOWC;
	  ok = false;
	}
      break;
    case RSkull:
      if
	(
	 !(cards & it_redskull) &&
	 (!skulliscard || !(cards & it_redcard))
	 )
	{
	  message = skulliscard? PD_REDK : PD_REDS;
	  ok = false;
	}
      break;
    case BSkull:
      if
	(
	 !(cards & it_blueskull) &&
	 (!skulliscard || !(cards & it_bluecard))
	 )
	{
	  message = skulliscard? PD_BLUEK : PD_BLUES;
	  ok = false;
	}
      break;
    case YSkull:
      if
	(
	 !(cards & it_yellowskull) &&
	 (!skulliscard || !(cards & it_yellowcard))
	 )
	{
	  message = skulliscard? PD_YELLOWK : PD_YELLOWS;
	  ok = false;
	}
      break;
    case AllKeys:
      if
	(
	 !skulliscard &&
	 (
          !(cards & it_redcard) ||
          !(cards & it_redskull) ||
          !(cards & it_bluecard) ||
          !(cards & it_blueskull) ||
          !(cards & it_yellowcard) ||
          !(cards & it_yellowskull)
	  )
	 )
	{
	  message = PD_ALL6;
	  ok = false;
	}
      if
	(
	 skulliscard &&
	 (
          (!(cards & it_redcard) &&
	   !(cards & it_redskull)) ||
          (!(cards & it_bluecard) &&
	   !(cards & it_blueskull)) ||
          (!(cards & it_yellowcard) &&
	   !(cards & it_yellowskull))
	  )
	 )
	{
	  message = PD_ALL3;
	  ok = false;
	}
      break;
    }
  if (ok == false) S_StartScreamSound(this, sfx_oof);

  return ok;
}


//
// was P_ProcessSpecialSector
// Function that actually applies the sector special to the player.
void PlayerPawn::ProcessSpecialSector(sector_t *sector, bool instantdamage)
{
  if (sector->special < 32)
    {
      // Has hitten ground.
      switch (sector->special)
	{
        case 5:
          // HELLSLIME DAMAGE
          if (!powers[pw_ironfeet])
	    if (instantdamage)
              {
		Damage(NULL, NULL, 10);

		// spawn a puff of smoke
		//CONS_Printf ("damage!\n"); //debug
		mp->SpawnSmoke(x, y, z);
              }
          break;

        case 7:
          // NUKAGE DAMAGE
          if (!powers[pw_ironfeet])
	    if (instantdamage)
	      Damage(NULL, NULL, 5);
          break;

        case 16:
          // SUPER HELLSLIME DAMAGE
        case 4:
          // STROBE HURT
          if (!powers[pw_ironfeet]
              || (P_Random()<5) )
	    {
              if (instantdamage)
		Damage(NULL, NULL, 20);
	    }
          break;

        case 9:
          // SECRET SECTOR
          player->secrets++;
          sector->special = 0;

          //faB: useful only in single & coop.
          if (!cv_deathmatch.value && this == displayplayer->pawn)
	    CONS_Printf ("\2You found a secret area!\n");

          break;

        case 11:
          // EXIT SUPER DAMAGE! (for E1M8 finale)
          cheats &= ~CF_GODMODE;

          if (instantdamage)
	    Damage(NULL, NULL, 20);

          if ((health <= 10) && cv_allowexitlevel.value)
	    mp->ExitMap(0);
          break;

        default:
          //SoM: 3/8/2000: Just ignore.
          //CONS_Printf ("P_PlayerInSpecialSector: unknown special %i",
          //             sector->special);
          break;
	};
    }
  else //SoM: Extended sector types for secrets and damage
    {
      switch ((sector->special & DAMAGE_MASK) >> DAMAGE_SHIFT)
	{
	case 0: // no damage
	  break;
	case 1: // 2/5 damage per 31 ticks
	  if (!powers[pw_ironfeet] && instantdamage)
	    Damage(NULL, NULL, 5);
	  break;
	case 2: // 5/10 damage per 31 ticks
	  if (!powers[pw_ironfeet] && instantdamage)
	    Damage(NULL, NULL, 10);
	  break;
	case 3: // 10/20 damage per 31 ticks
	  if ((!powers[pw_ironfeet]
	       || P_Random()<5) && instantdamage)  // take damage even with suit
	    {
	      Damage(NULL, NULL, 20);
	    }
	  break;
	}
      if (sector->special & SECRET_MASK)
	{
	  player->secrets++;
	  sector->special &= ~SECRET_MASK;

	  if (!cv_deathmatch.value && this == displayplayer->pawn)
	    CONS_Printf ("\2You found a secret area!\n");

	  if (sector->special < 32)
	    sector->special=0;
	}
    }
}


//
// was P_PlayerOnSpecial3DFloor
// Checks to see if a player is standing on or is inside a 3D floor (water)
// and applies any speicials..
void PlayerPawn::PlayerOnSpecial3DFloor()
{
  sector_t* sec;
  bool   instantdamage = false;
  ffloor_t* rover;

  sec = subsector->sector;
  if(!sec->ffloors)
    return;

  for (rover = sec->ffloors; rover; rover = rover->next)
    {
      if (!rover->master->frontsector->special)
	continue;

      // Check the 3D floor's type...
      if (rover->flags & FF_SOLID)
	{
	  // Player must be on top of the floor to be affected...
	  if (z != *rover->topheight)
	    continue;

	  if (game.demoversion >= 125 &&
	      (eflags & MF_JUSTHITFLOOR) &&
	      sec->heightsec == -1 && (mp->maptic % (2*NEWTICRATERATIO))) //SoM: penalize jumping less.
	    instantdamage = true;
	  else
	    instantdamage = !(mp->maptic % (32*NEWTICRATERATIO));
	}
      else
	{
	  //Water and DEATH FOG!!! heh
	  if(z > *rover->topheight || (z + height) < *rover->bottomheight)
	    continue;
	  instantdamage = !(mp->maptic % (32*NEWTICRATERATIO));
	}

      ProcessSpecialSector(rover->master->frontsector, instantdamage);
    }
}


//
// was P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void PlayerPawn::PlayerInSpecialSector()
{
  bool instantdamage = false;

  // SoM: Check 3D floors...
  PlayerOnSpecial3DFloor();

  sector_t *sec = subsector->sector;

  //Fab: keep track of what sector type the player's currently in
  specialsector = sec->special;

#ifdef OLDWATER
  //Fab: VERY NASTY hack for water QUICK TEST !!!!!!!!!!!!!!!!!!!!!!!
  if (sec->tag < 0)
    {
      specialsector = 888;    // no particular value
      return;
    }
  else if (levelflats[sec->floorpic].iswater)
    // old water (flat texture)
    {
      specialsector = 887;
      return;
    }
#endif

  if (!specialsector)     // nothing special, exit
    return;

  if (game.mode == gm_heretic)
    {
      HerePlayerInSpecialSector();
      return;
    }

  // Falling, not all the way down yet?
  //SoM: 3/17/2000: Damage if in slimey water!
  if (sec->heightsec != -1)
    {
      if(z > mp->sectors[sec->heightsec].floorheight)
	return;
    }
  else if (z != sec->floorheight)
    return;

  //Fab: jumping in lava/slime does instant damage (no jump cheat)
  if ((eflags & MF_JUSTHITFLOOR) &&
      (sec->heightsec == -1) && (mp->maptic % (2*NEWTICRATERATIO))) //SoM: penalize jumping less.
    instantdamage = true;
  else
    instantdamage = !(mp->maptic % (32*NEWTICRATERATIO));

  ProcessSpecialSector(sec, instantdamage);
}

