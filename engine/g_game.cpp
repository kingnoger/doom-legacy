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
// Revision 1.1  2002/11/16 14:18:08  hurdler
// Initial revision
//
// Revision 1.24  2002/09/25 15:17:36  vberghol
// Intermission fixed?
//
// Revision 1.20  2002/09/05 14:12:13  vberghol
// network code partly bypassed
//
// Revision 1.18  2002/08/27 11:51:45  vberghol
// Menu rewritten
//
// Revision 1.17  2002/08/24 17:25:32  vberghol
// bug fixes
//
// Revision 1.16  2002/08/23 18:05:38  vberghol
// idiotic segfaults fixed
//
// Revision 1.14  2002/08/17 16:02:02  vberghol
// final compile for engine!
//
// Revision 1.13  2002/08/16 20:49:23  vberghol
// engine ALMOST done!
//
// Revision 1.12  2002/08/13 19:47:39  vberghol
// p_inter.cpp done
//
// Revision 1.11  2002/08/11 17:16:46  vberghol
// ...
//
// Revision 1.10  2002/08/08 12:01:25  vberghol
// pian engine on valmis!
//
// Revision 1.9  2002/08/06 13:14:19  vberghol
// ...
//
// Revision 1.8  2002/08/02 20:14:48  vberghol
// p_enemy.cpp done!
//
// Revision 1.7  2002/07/23 19:21:39  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.6  2002/07/12 19:21:37  vberghol
// hop
//
// Revision 1.5  2002/07/04 18:02:24  vberghol
// Pientä fiksausta, g_pawn.cpp uusi tiedosto
//
// Revision 1.4  2002/07/01 21:00:14  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:53  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.43  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.42  2001/12/15 18:41:35  hurdler
// small commit, mainly splitscreen fix
//
// Revision 1.41  2001/08/20 20:40:39  metzgermeister
// *** empty log message ***
//
// Revision 1.40  2001/08/20 18:34:18  bpereira
// glide ligthing and map30 bug
//
// Revision 1.39  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.38  2001/08/02 19:15:59  bpereira
// fix player reset in secret level of doom2
//
// Revision 1.37  2001/07/16 22:35:40  bpereira
// - fixed crash of e3m8 in heretic
// - fixed crosshair not drawed bug
//
// Revision 1.36  2001/05/16 21:21:14  bpereira
// no message
//
// Revision 1.35  2001/05/03 21:22:25  hurdler
// remove some warnings
//
// Revision 1.34  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.29  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.28  2000/11/26 20:36:14  hurdler
// Adding autorun2
//
// Revision 1.23  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.22  2000/10/21 08:43:28  bpereira
// no message
//
// Revision 1.21  2000/10/09 14:03:31  crashrl
// *** empty log message ***
//
// Revision 1.20  2000/10/08 13:30:00  bpereira
// no message
//
// Revision 1.19  2000/10/07 20:36:13  crashrl
// Added deathmatch team-start-sectors via sector/line-tag and linedef-type 1000-1031
//
// Revision 1.12  2000/04/19 10:56:51  hurdler
// commited for exe release and tag only
//
// Revision 1.11  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.10  2000/04/11 19:07:23  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.9  2000/04/07 23:11:17  metzgermeister
// added mouse move
//
// Revision 1.8  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.7  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.6  2000/03/29 19:39:48  bpereira
// no message
//
// Revision 1.5  2000/03/23 22:54:00  metzgermeister
// added support for HOME/.legacy under Linux
//
// Revision 1.4  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.3  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//  Part of GameInfo class implementation
//      game loop functions, events handling
//
//-----------------------------------------------------------------------------

#include "doomdef.h"

#include "command.h"
#include "console.h"

#include "dstrings.h"
#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_pawn.h"

#include "g_input.h"
#include "d_main.h"

#include "d_clisrv.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "i_system.h"
#include "m_random.h"

#include "r_render.h"
#include "r_state.h"
#include "r_draw.h"
#include "r_main.h"

#include "s_sound.h"
#include "sounds.h"

#include "m_misc.h" // File handling
#include "m_menu.h"
#include "m_argv.h" // remove this!

#include "am_map.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "f_finale.h"

#include "keys.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_video.h" // rendermode! fix!
#include "byteptr.h" // shouldn't be here

#include "i_joy.h" // move input processing somewhere else

#ifdef HWRENDER 
# include "hardware/hw_main.h"
#endif


byte *savebuffer;
void *statcopy;                      // for statistics driver


tic_t   gametic;


void ShowMessage_OnChange();
void AllowTurbo_OnChange();

CV_PossibleValue_t showmessages_cons_t[]={{0,"Off"},{1,"On"},{2,"Not All"},{0,NULL}};
CV_PossibleValue_t crosshair_cons_t[]   ={{0,"Off"},{1,"Cross"},{2,"Angle"},{3,"Point"},{0,NULL}};

consvar_t cv_crosshair        = {"crosshair"   ,"0",CV_SAVE,crosshair_cons_t};
consvar_t cv_autorun          = {"autorun"     ,"0",CV_SAVE,CV_OnOff};
consvar_t cv_invertmouse      = {"invertmouse" ,"0",CV_SAVE,CV_OnOff};
consvar_t cv_alwaysfreelook   = {"alwaysmlook" ,"0",CV_SAVE,CV_OnOff};
consvar_t cv_mousemove        = {"mousemove"   ,"1",CV_SAVE,CV_OnOff};
consvar_t cv_showmessages     = {"showmessages","1",CV_SAVE | CV_CALL | CV_NOINIT,showmessages_cons_t,ShowMessage_OnChange};

consvar_t cv_crosshair2       = {"crosshair2"  ,"0",CV_SAVE,crosshair_cons_t};
consvar_t cv_autorun2         = {"autorun2"    ,"0",CV_SAVE,CV_OnOff};
consvar_t cv_invertmouse2     = {"invertmouse2","0",CV_SAVE,CV_OnOff};
consvar_t cv_alwaysfreelook2  = {"alwaysmlook2","0",CV_SAVE,CV_OnOff};
consvar_t cv_mousemove2       = {"mousemove2"  ,"1",CV_SAVE,CV_OnOff};
consvar_t cv_showmessages2    = {"showmessages2","1",CV_SAVE | CV_CALL | CV_NOINIT,showmessages_cons_t,ShowMessage_OnChange};

//consvar_t cv_crosshairscale   = {"crosshairscale","0",CV_SAVE,CV_YesNo};

consvar_t cv_allowturbo       = {"allowturbo"  ,"0",CV_NETVAR | CV_CALL, CV_YesNo, AllowTurbo_OnChange};

consvar_t cv_joystickfreelook = {"joystickfreelook" ,"0",CV_SAVE,CV_OnOff};


void ShowMessage_OnChange()
{
  if (!cv_showmessages.value)
    CONS_Printf("%s\n",MSGOFF);
  else
    CONS_Printf("%s\n",MSGON);
}


static fixed_t originalforwardmove[2] = {0x19, 0x32};
static fixed_t originalsidemove[2]    = {0x18, 0x28};
static fixed_t forwardmove[2] = {25/NEWTICRATERATIO, 50/NEWTICRATERATIO};
static fixed_t sidemove[2]    = {24/NEWTICRATERATIO, 40/NEWTICRATERATIO};
static fixed_t angleturn[3]   = {640, 1280, 320};        // + slow turn


void AllowTurbo_OnChange()
{
  if(!cv_allowturbo.value && game.netgame)
    {
      // like turbo 100
      forwardmove[0] = originalforwardmove[0];
      forwardmove[1] = originalforwardmove[1];
      sidemove[0] = originalsidemove[0];
      sidemove[1] = originalsidemove[1];
    }
}

//  turbo <10-255>
//
void Command_Turbo_f()
{
  int scale = 200;

  if(!cv_allowturbo.value && game.netgame)
    {
      CONS_Printf("This server doesn't allow turbo\n");
      return;
    }

  if (COM_Argc()!=2)
    {
      CONS_Printf("turbo <10-255> : set turbo");
      return;
    }

  scale = atoi (COM_Argv(1));

  if (scale < 10)
    scale = 10;
  if (scale > 255)
    scale = 255;

  CONS_Printf ("turbo scale: %i%%\n",scale);

  forwardmove[0] = originalforwardmove[0]*scale/100;
  forwardmove[1] = originalforwardmove[1]*scale/100;
  sidemove[0] = originalsidemove[0]*scale/100;
  sidemove[1] = originalsidemove[1]*scale/100;
}



//  Clip the console player mouse aiming to the current view,
//  also returns a signed char for the player ticcmd if needed.
//  Used whenever the player view pitch is changed manually
//
//changed:3-3-98: do a angle limitation now
short G_ClipAimingPitch(int *aiming)
{
  int limitangle;

  //note: the current software mode implementation doesn't have true perspective
  if ( rendermode == render_soft)
    limitangle = 732<<ANGLETOFINESHIFT;
  else
    limitangle = ANG90 - 1;

  if (*aiming > limitangle)
    *aiming = limitangle;
  else if (*aiming < -limitangle)
    *aiming = -limitangle;

  return (*aiming)>>16;
}


// for change this table change also nextweapon func in g_game and P_PlayerThink
static char extraweapons[8] = {wp_chainsaw, -1, wp_supershotgun, -1, -1, -1, -1, -1};
static byte nextweaponorder[NUMWEAPONS] = {wp_fist, wp_chainsaw, wp_pistol,
					   wp_shotgun, wp_supershotgun, wp_chaingun,
					   wp_missile, wp_plasma, wp_bfg};

static byte NextWeapon(PlayerPawn *player, int step)
{
  byte   w;
  int    i;
  for (i=0;i<NUMWEAPONS;i++)
    if (player->readyweapon == nextweaponorder[i])
      {
	i = (i+NUMWEAPONS+step)%NUMWEAPONS;
	break;
      }
  for (;nextweaponorder[i]!=player->readyweapon; i=(i+NUMWEAPONS+step)%NUMWEAPONS)
    {
      w = nextweaponorder[i];
        
      // skip super shotgun for non-Doom2
      if (game.mode!=commercial && w==wp_supershotgun)
	continue;

      // skip plasma-bfg in sharware
      if (game.mode==shareware && (w==wp_plasma || w==wp_bfg))
	continue;

      if ( player->weaponowned[w] &&
	   player->ammo[player->weaponinfo[w].ammo] >= player->weaponinfo[w].ammopershoot)
        {
	  if(w==wp_chainsaw)
	    return (BT_CHANGE | BT_EXTRAWEAPON | (wp_fist<<BT_WEAPONSHIFT));
	  if(w==wp_supershotgun)
	    return (BT_CHANGE | BT_EXTRAWEAPON | (wp_shotgun<<BT_WEAPONSHIFT));
	  return (BT_CHANGE | (w<<BT_WEAPONSHIFT));
        }
    }
  return 0;
}


//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
// set secondaryplayer true to build player 2's ticcmd in splitscreen mode
//
int     localaiming,localaiming2;
angle_t localangle,localangle2;

//added:06-02-98: mouseaiming (looking up/down with the mouse or keyboard)
#define KB_LOOKSPEED    (1<<25)
#define MAXPLMOVE       (forwardmove[1])

#define SLOWTURNTICS    (6*NEWTICRATERATIO)

void G_BuildTiccmd(ticcmd_t* cmd, bool primary, int realtics)
{
  int         i, j, k;
  int         forward;
  int         side;
  ticcmd_t*   base;

  //added:14-02-98: these ones used for multiple conditions
  bool     turnleft,turnright,mouseaiming,analogjoystickmove,gamepadjoystickmove;

  int (*gc)[2]; //pointer to array[num_gamecontrols][2]
  PlayerPawn *p = NULL;

  if (primary)
    {
      gc = gamecontrol;
      i = cv_autorun.value;
      j = cv_alwaysfreelook.value;
      k = 0;
      if (consoleplayer)
	{
	  // FIXME this is a hack to ease debugging until the new netcode is done
	  // Here the netcode is bypassed. See also Game::Ticker() !
	  cmd = &consoleplayer->cmd;
	  p = consoleplayer->pawn;
	}
    }
  else
    {
      gc = gamecontrol2;
      i = cv_autorun2.value;
      j = cv_alwaysfreelook2.value;
      k = 1;
      if (consoleplayer2)
	p = consoleplayer2->pawn;
    }
  // FIXME no pawn nor player should be needed here. Fix doom network protocol! (BT_EXTRAWEAPON! agh!)

  base = I_BaseTiccmd();             // empty, or external driver
  memcpy(cmd, base, sizeof(*cmd));

  // a little clumsy, but then the g_input.c became a lot simpler!
  bool strafe = gamekeydown[gc[gc_strafe][0]] || gamekeydown[gc[gc_strafe][1]];

  int speed = (gamekeydown[gc[gc_speed][0]] || gamekeydown[gc[gc_speed][1]]) ^ i;

  turnright = gamekeydown[gc[gc_turnright][0]]
    ||gamekeydown[gc[gc_turnright][1]];
  turnleft  = gamekeydown[gc[gc_turnleft][0]]
    ||gamekeydown[gc[gc_turnleft][1]];

  mouseaiming = (gamekeydown[gc[gc_mouseaiming][0]]
		 ||gamekeydown[gc[gc_mouseaiming][1]]) ^ j;

  if (primary) {
    analogjoystickmove  = cv_usejoystick.value && !Joystick.bGamepadStyle && !cv_splitscreen.value;
    gamepadjoystickmove = cv_usejoystick.value &&  Joystick.bGamepadStyle && !cv_splitscreen.value;
  } else {
    analogjoystickmove  = cv_usejoystick.value && !Joystick.bGamepadStyle;
    gamepadjoystickmove = cv_usejoystick.value &&  Joystick.bGamepadStyle;
  }

  if (gamepadjoystickmove)
    {
      turnright = turnright || (joyxmove > 0);
      turnleft  = turnleft  || (joyxmove < 0);
    }
  forward = side = 0;

  // use two stage accelerative turning
  // on the keyboard and joystick
  static int turnheld[2]; // for accelerative turning

  if (turnleft || turnright)
    turnheld[k] += realtics;
  else
    turnheld[k] = 0;

  int tspeed;
  if (turnheld[k] < SLOWTURNTICS)
    tspeed = 2;             // slow turn
  else
    tspeed = speed;

  // let movement keys cancel each other out
  if (strafe)
    {
      if (turnright)
	side += sidemove[speed];
      if (turnleft)
	side -= sidemove[speed];

      if (analogjoystickmove)
        {
	  //faB: JOYAXISRANGE is supposed to be 1023 ( divide by 1024)
	  side += ( (joyxmove * sidemove[1]) >> 10 );
        }
    }
  else
    {
      if (turnright)
	cmd->angleturn -= angleturn[tspeed];
      //else
      if (turnleft)
	cmd->angleturn += angleturn[tspeed];
      if ( joyxmove && analogjoystickmove) {
	//faB: JOYAXISRANGE should be 1023 ( divide by 1024)
	cmd->angleturn -= ( (joyxmove * angleturn[1]) >> 10 );        // ANALOG!
	//CONS_Printf ("joyxmove %d  angleturn %d\n", joyxmove, cmd->angleturn);
      }
    }

  //added:07-02-98: forward with key or button
  if (gamekeydown[gc[gc_forward][0]] ||
      gamekeydown[gc[gc_forward][1]] ||
      ( joyymove < 0 && gamepadjoystickmove && !cv_joystickfreelook.value))
    {
      forward += forwardmove[speed];
    }
  if (gamekeydown[gc[gc_backward][0]] ||
      gamekeydown[gc[gc_backward][1]] ||
      (joyymove > 0 && gamepadjoystickmove && !cv_joystickfreelook.value))
    {
      forward -= forwardmove[speed];
    }
        
  if (joyymove && analogjoystickmove && !cv_joystickfreelook.value) 
    forward -= ( (joyymove * forwardmove[1]) >> 10 );               // ANALOG!

  //added:07-02-98: some people strafe left & right with mouse buttons
  if (gamekeydown[gc[gc_straferight][0]] ||
      gamekeydown[gc[gc_straferight][1]])
    side += sidemove[speed];
  if (gamekeydown[gc[gc_strafeleft][0]] ||
      gamekeydown[gc[gc_strafeleft][1]])
    side -= sidemove[speed];

  //added:07-02-98: fire with any button/key
  if (gamekeydown[gc[gc_fire][0]] ||
      gamekeydown[gc[gc_fire][1]])
    {
      cmd->buttons |= BT_ATTACK;
      CONS_Printf("attack!\n");
    }

  //added:07-02-98: use with any button/key
  if (gamekeydown[gc[gc_use][0]] ||
      gamekeydown[gc[gc_use][1]])
    cmd->buttons |= BT_USE;

  //added:22-02-98: jump button
  if (cv_allowjump.value && (gamekeydown[gc[gc_jump][0]] ||
			     gamekeydown[gc[gc_jump][1]]))
    cmd->buttons |= BT_JUMP;


  if (p == NULL) return;
  //added:07-02-98: any key / button can trigger a weapon
  // chainsaw overrides
  if (gamekeydown[gc[gc_nextweapon][0]] ||
      gamekeydown[gc[gc_nextweapon][1]])
    cmd->buttons |= NextWeapon(p, 1);
  else if (gamekeydown[gc[gc_prevweapon][0]] ||
	   gamekeydown[gc[gc_prevweapon][1]])
    cmd->buttons |= NextWeapon(p, -1);
  else for (i=gc_weapon1; i<gc_weapon1+NUMWEAPONS-1; i++)
    if (gamekeydown[gc[i][0]] ||
	gamekeydown[gc[i][1]])
      {
	cmd->buttons |= BT_CHANGE | BT_EXTRAWEAPON; // extra by default
	cmd->buttons |= (i-gc_weapon1)<<BT_WEAPONSHIFT;
	// already have extraweapon in hand switch to the normal one
	if (p->readyweapon == extraweapons[i-gc_weapon1])
	  cmd->buttons &= ~BT_EXTRAWEAPON;
	break;
      }

  static bool keyboard_look[2];      // true if lookup/down using keyboard

  if (primary) {
    // mouse look stuff (mouse look is not the same as mouse aim)
    if (mouseaiming)
      {
        keyboard_look[0] = false;

        // looking up/down
        if (cv_invertmouse.value)
	  localaiming -= mlooky<<19;
        else
	  localaiming += mlooky<<19;
      }
    if (cv_usejoystick.value && analogjoystickmove && cv_joystickfreelook.value)
      localaiming += joyymove<<16;

    // spring back if not using keyboard neither mouselookin'
    if (!keyboard_look && !cv_joystickfreelook.value && !mouseaiming)
      localaiming = 0;

    if (gamekeydown[gc[gc_lookup][0]] ||
        gamekeydown[gc[gc_lookup][1]])
      {
        localaiming += KB_LOOKSPEED;
        keyboard_look[0] = true;
      }
    else if (gamekeydown[gc[gc_lookdown][0]] ||
	     gamekeydown[gc[gc_lookdown][1]])
      {
        localaiming -= KB_LOOKSPEED;
        keyboard_look[0] = true;
      }
    else if (gamekeydown[gc[gc_centerview][0]] ||
	     gamekeydown[gc[gc_centerview][1]])
      localaiming = 0;

    //26/02/2000: added by Hurdler: accept no mlook for network games
    if (!cv_allowmlook.value)
      localaiming = 0;

    cmd->aiming = G_ClipAimingPitch (&localaiming);

    if (!mouseaiming && cv_mousemove.value)
      forward += mousey;

    if (strafe)
      side += mousex*2;
    else
      cmd->angleturn -= mousex*8;

    mousex = mousey = mlooky = 0;

  } else {

    // mouse look stuff (mouse look is not the same as mouse aim)
    if (mouseaiming)
      {
        keyboard_look[1] = false;
	
        // looking up/down
        if (cv_invertmouse2.value)
	  localaiming2 -= mlook2y<<19;
        else
	  localaiming2 += mlook2y<<19;
      }

    if (analogjoystickmove && cv_joystickfreelook.value)
      localaiming2 += joyymove<<16;
    // spring back if not using keyboard neither mouselookin'
    if (!keyboard_look && !cv_joystickfreelook.value && !mouseaiming)
      localaiming2 = 0;

    if (gamekeydown[gamecontrol2[gc_lookup][0]] ||
        gamekeydown[gamecontrol2[gc_lookup][1]])
      {
        localaiming2 += KB_LOOKSPEED;
        keyboard_look[1] = true;
      }
    else if (gamekeydown[gamecontrol2[gc_lookdown][0]] ||
	     gamekeydown[gamecontrol2[gc_lookdown][1]])
      {
        localaiming2 -= KB_LOOKSPEED;
        keyboard_look[1] = true;
      }
    else if (gamekeydown[gamecontrol2[gc_centerview][0]] ||
	     gamekeydown[gamecontrol2[gc_centerview][1]])
      localaiming2 = 0;

    //26/02/2000: added by Hurdler: accept no mlook for network games
    if (!cv_allowmlook.value)
      localaiming2 = 0;

    // look up max (viewheight/2) look down min -(viewheight/2)
    cmd->aiming = G_ClipAimingPitch (&localaiming2);;

    if (!mouseaiming && cv_mousemove2.value)
      forward += mouse2y;

    if (strafe)
      side += mouse2x*2;
    else
      cmd->angleturn -= mouse2x*8;

    mouse2x = mouse2y = mlook2y = 0;
  }

  if (forward > MAXPLMOVE)
    forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
    forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
    side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
    side = -MAXPLMOVE;

  cmd->forwardmove += forward;
  cmd->sidemove += side;

  CONS_Printf("Move: %d, %d, %d\n", cmd->buttons, cmd->forwardmove, cmd->sidemove);
   
#ifdef ABSOLUTEANGLE
  if (primary) {
    localangle += (cmd->angleturn<<16);
    cmd->angleturn = localangle >> 16;
  } else {
    localangle2 += (cmd->angleturn<<16);
    cmd->angleturn = localangle2 >> 16;
  }
#endif

 if (game.mode == heretic)
   {
     if (gamekeydown[gc[gc_flydown][0]] ||
	 gamekeydown[gc[gc_flydown][1]])
       cmd->angleturn |= BT_FLYDOWN;
     else
       cmd->angleturn &= ~BT_FLYDOWN;
   }
}


//--------------------------------------------
// was D_StartTitle
//
void GameInfo::StartIntro()
{
  extern int demosequence;
  action = ga_nothing;
  //playerdeadview = false;
  displayplayer = consoleplayer = NULL;
  demosequence = -1;
  paused = false;
  D_AdvanceDemo();
  CON_ToggleOff();
}


void GameInfo::Drawer()
{
  // draw the view directly
  CONS_Printf("GI::Draw: %p, %p\n", displayplayer,displayplayer2);
  if (displayplayer && displayplayer->pawn)
    {
      R.SetMap(displayplayer->pawn->mp);
#ifdef CLIENTPREDICTION2
      displayplayer->pawn->flags2 |= MF2_DONTDRAW;
#endif
#ifdef HWRENDER 
      if (rendermode != render_soft)
	R.HWR_RenderPlayerView(0, displayplayer);
      else //if (rendermode == render_soft)
#endif
	R.R_RenderPlayerView(displayplayer);
#ifdef CLIENTPREDICTION2
      displayplayer->pawn->flags2 &= ~MF2_DONTDRAW;
#endif
    }

  // added 16-6-98: render the second screen
  if (displayplayer2 && displayplayer2->pawn)
    {
      R.SetMap(displayplayer2->pawn->mp);
#ifdef CLIENTPREDICTION2
      displayplayer2->pawn->flags2 |= MF2_DONTDRAW;
#endif
#ifdef HWRENDER 
      if ( rendermode != render_soft)
	R.HWR_RenderPlayerView (1, displayplayer2);
      else 
#endif
	{
	  //faB: Boris hack :P !!
	  viewwindowy = vid.height/2;
	  memcpy(ylookup,ylookup2,viewheight*sizeof(ylookup[0]));
		  
	  R.R_RenderPlayerView(displayplayer2);
		  
	  viewwindowy = 0;
	  memcpy(ylookup,ylookup1,viewheight*sizeof(ylookup[0]));
	}
#ifdef CLIENTPREDICTION2
      displayplayer2->pawn->flags2 &=~MF2_DONTDRAW;
#endif
    }
}

// was G_InventoryResponder
//
bool PlayerPawn::InventoryResponder(bool primary, event_t *ev)
{
  if (!game.inventory)
    return false;

  int (*gc)[2]; //pointer to array[num_gamecontrols][2]
  if (primary)
    gc = gamecontrol;
  else
    gc = gamecontrol2;

  CONS_Printf("PP:IR  %d, %d\n", ev->type, ev->data1);

  switch (ev->type)
    {
    case ev_keydown :
      if (ev->data1 == gc[gc_invprev][0] || ev->data1 == gc[gc_invprev][1])
	{
	  if (invTics)
	    {
	      invSlot--;
	      if (invSlot < 0)
		invSlot = 0;
	      else
		{
		  st_curpos--;
		  if (st_curpos < 0)
		    st_curpos = 0;
		}
	    }
	  invTics = 5*TICRATE;
	  return true;
	}
      else if (ev->data1 == gc[gc_invnext][0] || ev->data1 == gc[gc_invnext][1])
	{
	  if (invTics)
	    {
	      invSlot++;
	      if (invSlot >= inventory.size()) //inventorySlotNum)
		{
		  invSlot = inventory.size()-1;
		  //invSlot--;
		  //if (invSlot < 0)
		  //  invSlot = 0;
		}
	      else
		{
		  st_curpos++;
		  if (st_curpos > 6)
		    st_curpos = 6;
		}
	    }
	  invTics = 5*TICRATE;
	  return true;
	}
      else if (ev->data1 == gc[gc_invuse ][0] || ev->data1 == gc[gc_invuse ][1])
	{
	  if (invTics)
	    invTics = 0;
	  else if (inventory[invSlot].count > 0)
	    {
	      //if (p == &consoleplayer)
	      if (primary)
		SendNetXCmd(XD_USEARTEFACT, &inventory[invSlot].type, 1);
	      else
		SendNetXCmd2(XD_USEARTEFACT, &inventory[invSlot].type, 1);
	    }
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



//
// G_Responder
//  Get info needed to make ticcmd_ts for the players.
//
bool GameInfo::Responder(event_t* ev)
{
  // allow spy mode changes even during the demo
  if (state == GS_LEVEL && ev->type == ev_keydown
      && ev->data1 == KEY_F12 && (singledemo || !cv_deathmatch.value))
    { // spy mode
      int i;
      int n = players.size();
      for (i = 0; i < n; i++)
	if (players[i] == displayplayer) break;
      // find next active player
      if (++i >= n) i = 0;
      displayplayer = players[i];

      //added:16-01-98:change statusbar also if playingback demo
      if (singledemo)
	hud.ST_Start(displayplayer->pawn);

      //added:11-04-98: tell who's the view
      CONS_Printf("Viewpoint : %s\n", displayplayer->name.c_str());

      return true;
    }

  // any other key pops up menu if in demos
  if (action == ga_nothing && !singledemo &&
      (demoplayback || state == GS_DEMOSCREEN))
    {
      if (ev->type == ev_keydown)
        {
	  Menu::Open();
	  return true;
        }
      return false;
    }

  switch (state)
    {
    case GS_LEVEL:
#if 1 // FIXME testing
      if (ev->type == ev_keydown && ev->data1 == 'n')
	{
	  CONS_Printf("------ n pressed\n");
	  consoleplayer->pawn->health += 50; 
	  return true;
	}
#endif
      if(!multiplayer) //FIXME! The _server_ CAN cheat in multiplayer (maybe using console only?)
	if (cht_Responder (ev))
	  return true;
      if (hud.Responder(ev))
	return true;        // HUD ate the event
      if (automap.Responder(ev))
	return true;        // automap ate it
      // FIXME player->pawn may be NULL during level,
      // make InventoryResponder a PlayerInfo method instead.
      // there, remember to check if pawn==NULL
      if (consoleplayer->pawn->InventoryResponder(true, ev))
	return true;
      CONS_Printf("G:Resp 3\n");
      if (cv_splitscreen.value && consoleplayer2->pawn->InventoryResponder(false, ev))
	return true;
      //added:07-02-98: map the event (key/mouse/joy) to a gamecontrol
      break;

    case GS_INTERMISSION:
      if (wi.Responder(ev))
	return true;
      break;

    case GS_FINALE:
      if (F_Responder (ev))
	return true;        // finale ate the event
      break;

    default:
      break;
    }

  CONS_Printf("G:Resp 5\n");

  // update keys current state
  G_MapEventsToControls(ev);

  // FIXME move these to Menu::Responder?
  switch (ev->type)
    {
    case ev_keydown:
      switch (ev->data1)
	{
	case KEY_PAUSE:
	  COM_BufAddText("pause\n");
	  return true;
	  
	case KEY_MINUS:     // Screen size down
	  CV_SetValue (&cv_viewsize, cv_viewsize.value-1);
	  S_StartAmbSound(sfx_stnmov);
	  return true;

	case KEY_EQUALS:    // Screen size up
	  CV_SetValue (&cv_viewsize, cv_viewsize.value+1);
	  S_StartAmbSound(sfx_stnmov);
	  return true;
	}

      return true;

    case ev_keyup:
      return false;   // always let key up events filter down

    case ev_mouse:
      return true;    // eat events

    case ev_joystick:
      return true;    // eat events

    default:
      break;
    }

  return false;
}


//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
void G_LoadGame(int slot)
{
  COM_BufAddText(va("load %d\n",slot));
}

#define VERSIONSIZE             16

// was G_DoLoadGame

void GameInfo::LoadGame(int slot)
{
  int         length;
  char        vcheck[VERSIONSIZE];
  char        savename[255];

  sprintf(savename, savegamename, slot);

  length = FIL_ReadFile(savename, &savebuffer);
  if (!length)
    {
      CONS_Printf ("Couldn't read file %s", savename);
      return;
    }

  // skip the description field
  save_p = savebuffer + SAVESTRINGSIZE;
    
  memset (vcheck,0,sizeof(vcheck));
  sprintf (vcheck,"version %i",VERSION);
  // VB: note conversion from byte to signed char
  if (strcmp ((char *)save_p, vcheck))
    {
      M_StartMessage ("Save game from different version\n\nPress ESC\n",NULL,MM_NOTHING);
      return;                         // bad version
    }
  save_p += VERSIONSIZE;

  if(demoplayback)  // reset game engine
    StopDemo();

  //added:27-02-98: reset the game version
  Downgrade(VERSION);

  paused = false;
  automap.Close();

  // dearchive all the modifications
  if (!P_LoadGame())
    {
      M_StartMessage ("savegame file corrupted\n\nPress ESC\n", NULL, MM_NOTHING);
      Command_ExitGame_f();
      Z_Free (savebuffer);
      return;
    }

  action = ga_nothing;
  state = GS_LEVEL;
  displayplayer = consoleplayer;

  // done
  Z_Free(savebuffer);

  multiplayer = (players.size() > 1);
  // FIXME! why can't this be saved as well?
  //if (playeringame[1] && !netgame)
  //  CV_SetValue(&cv_splitscreen,1);

  if (setsizeneeded)
    R_ExecuteSetViewSize ();

  // draw the pattern into the back screen
  R_FillBackScreen ();
  CON_ToggleOff ();
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame(int slot, char *description)
{
  if (server)
    COM_BufAddText(va("save %d \"%s\"\n",slot,description));
}

// was G_DoSaveGame

const int SAVEGAMESIZE = (128*1024);

void GameInfo::SaveGame(int savegameslot, char *savedescription)
{
  // FIXME
  // this is how it should go:
  // Here we create an instance of an Archive class.
  // GameInfo serializes itself into the Archive.
  // Players are serialized next, with info about their current Map
  // Each active (or in_stasis) Map is then serialized. Map::Serialize must _not_ re-serialize its players.
  // Archive is closed.
  // FIXME right now p_saveg.cpp is mostly commented out.

  char        name2[VERSIONSIZE];
  char        description[SAVESTRINGSIZE];
  int         length;
  char        name[256];

  action = ga_nothing;

  sprintf(name, savegamename, savegameslot);

  save_p = savebuffer = (byte *)malloc(SAVEGAMESIZE);
  if(!save_p)
    {
      CONS_Printf ("No More free memory for savegame\n");
      return;
    }

  strcpy(description,savedescription);
  description[SAVESTRINGSIZE]=0;
  WRITEMEM(save_p, description, SAVESTRINGSIZE);
  memset (name2,0,sizeof(name2));
  sprintf (name2,"version %i",VERSION);
  WRITEMEM(save_p, name2, VERSIONSIZE);

  P_SaveGame();

  length = save_p - savebuffer;
  if (length > SAVEGAMESIZE)
    I_Error ("Savegame buffer overrun");
  FIL_WriteFile (name, savebuffer, length);
  free(savebuffer);

  action = ga_nothing;

  consoleplayer->message = GGSAVED;

  // draw the pattern into the back screen
  R_FillBackScreen ();
}


//
//  Can be called by the startup code or the menu task,
//  consoleplayer, displayplayer, playeringame[] should be set.
//
// Boris comment : single player start game
void G_DeferedInitNew (skill_t skill, char* mapname, bool StartSplitScreenGame)
{
  game.Downgrade(VERSION);
  game.paused = false;

  if (demoplayback)
    COM_BufAddText ("stopdemo\n");

  // this leave the actual game if needed
  SV_StartSinglePlayerServer();
    
  COM_BufAddText (va("splitscreen %d;deathmatch 0;fastmonsters 0;"
		     "respawnmonsters 0;timelimit 0;fraglimit 0\n",
		     StartSplitScreenGame));

  COM_BufAddText (va("map \"%s\" -skill %d -monsters 1\n",mapname,skill+1));
}

