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
// Revision 1.7  2004/07/05 16:53:24  smite-meister
// Netcode replaced
//
// Revision 1.6  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.5  2003/12/09 01:02:00  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.4  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.3  2003/01/25 21:33:05  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.2  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.1.1.1  2002/11/16 14:17:51  hurdler
// Initial C++ version of Doom Legacy
//-----------------------------------------------------------------------------

/// \file
/// \brief Handles keyboard/mouse/joystick inputs,
/// maps inputs to game controls (forward, use, fire...).

#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "d_event.h"
#include "g_input.h"

#include "g_player.h"
#include "g_pawn.h"

#include "i_system.h"
#include "i_joy.h"
#include "i_video.h"

#include "tables.h"

#define MAXMOUSESENSITIVITY 40 // sensitivity steps

CV_PossibleValue_t onecontrolperkey_cons_t[]={{1,"One"},{2,"Several"},{0,NULL}};
CV_PossibleValue_t usemouse_cons_t[]={{0,"Off"},{1,"On"},{2,"Force"},{0,NULL}};
CV_PossibleValue_t mousesens_cons_t[]={{1,"MIN"},{MAXMOUSESENSITIVITY,"MAXCURSOR"},{MAXINT,"MAX"},{0,NULL}};

consvar_t  cv_controlperkey = {"controlperkey","1",CV_SAVE,onecontrolperkey_cons_t};

consvar_t cv_autorun     = {"autorun","0",CV_SAVE,CV_OnOff};
consvar_t cv_automlook   = {"automlook","0",CV_SAVE,CV_OnOff};
consvar_t cv_usemouse    = {"use_mouse","1", CV_SAVE | CV_CALL,usemouse_cons_t,I_StartupMouse};
consvar_t cv_invertmouse = {"invertmouse","0",CV_SAVE,CV_OnOff};
consvar_t cv_mousemove   = {"mousemove","1",CV_SAVE,CV_OnOff};
consvar_t cv_mousesensx  = {"mousesensx","10",CV_SAVE,mousesens_cons_t};
consvar_t cv_mousesensy  = {"mousesensy","10",CV_SAVE,mousesens_cons_t};

consvar_t cv_autorun2     = {"autorun2","0",CV_SAVE,CV_OnOff};
consvar_t cv_automlook2   = {"automlook2","0",CV_SAVE,CV_OnOff};
consvar_t cv_usemouse2    = {"use_mouse2","0", CV_SAVE | CV_CALL,usemouse_cons_t,I_StartupMouse2};
consvar_t cv_invertmouse2 = {"invertmouse2","0",CV_SAVE,CV_OnOff};
consvar_t cv_mousemove2   = {"mousemove2","1",CV_SAVE,CV_OnOff};
consvar_t cv_mousesensx2  = {"mousesensx2","10",CV_SAVE,mousesens_cons_t};
consvar_t cv_mousesensy2  = {"mousesensy2","10",CV_SAVE,mousesens_cons_t};

#ifdef LMOUSE2
 CV_PossibleValue_t mouse2port_cons_t[]={{0,"/dev/gpmdata"},{1,"/dev/ttyS0"},{2,"/dev/ttyS1"},{3,"/dev/ttyS2"},{4,"/dev/ttyS3"},{0,NULL}};
 consvar_t cv_mouse2port  = {"mouse2port","/dev/gpmdata", CV_SAVE, mouse2port_cons_t };
 consvar_t cv_mouse2opt = {"mouse2opt","0", CV_SAVE, NULL};
#else
 CV_PossibleValue_t mouse2port_cons_t[]={{1,"COM1"},{2,"COM2"},{3,"COM3"},{4,"COM4"},{0,NULL}};
 consvar_t cv_mouse2port  = {"mouse2port","COM2", CV_SAVE, mouse2port_cons_t };
#endif


consvar_t cv_usejoystick = {"use_joystick", "0", CV_SAVE | CV_CALL, NULL, I_InitJoystick};
consvar_t cv_joystickfreelook = {"joystickfreelook" ,"0",CV_SAVE,CV_OnOff};

#ifdef LJOYSTICK
 CV_PossibleValue_t joyport_cons_t[]={{1,"/dev/js0"},{2,"/dev/js1"},{3,"/dev/js2"},{4,"/dev/js3"},{0,NULL}};
 extern void I_JoyScale();
 consvar_t cv_joyport = {"joyport","/dev/js0", CV_SAVE, joyport_cons_t};
 consvar_t cv_joyscale = {"joyscale","0",CV_SAVE | CV_CALL,NULL,I_JoyScale};
#endif


//========================================================================

static int mousex, mousey, mouse2x, mouse2y;
static int joyxmove, joyymove;

// current state of the keys : true if down
byte gamekeydown[NUMINPUTS];

// two key (or virtual key) codes per game control
int  gamecontrol[num_gamecontrols][2];
int  gamecontrol2[num_gamecontrols][2];


//========================================================================

static char forwardspeed[2] = {50, 100};
static char sidespeed[2]    = {48, 80};

#define MAXPLMOVE (forwardspeed[1])
// Stored in a signed char, but min = -100, max = 100, mmmkay?
// There is no more turbo cheat, you should modify the pawn's max speed instead

// fixed_t MaxPlayerMove[NUMCLASSES] = { 60, 50, 45, 49 }; // implemented as pawn speeds in info_m.cpp
// Otherwise OK, but fighter should also run sideways almost as fast as forward,
// (59/60) instead of (80/100). Weird. Affects straferunning.

static fixed_t angleturn[3] = {640, 1280, 320};  // + slow turn


// clips the pitch angle to avoid ugly renderer effects
angle_t G_ClipAimingPitch(angle_t pitch)
{
  int p = pitch;
  int limitangle;

  //note: the current software mode implementation doesn't have true perspective
  if (rendermode == render_soft)
    limitangle = 732<<ANGLETOFINESHIFT;
  else
    limitangle = ANG90 - 1;

  if (p > limitangle)
    pitch = limitangle;
  else if (p < -limitangle)
    pitch = -limitangle;
  
  return pitch;
}


static byte NextWeapon(PlayerPawn *p, int step)
{
  // Kludge. TODO when netcode is redone, fix me.
  int w = p->readyweapon;
  do
    {
      w = (w + step) % NUMWEAPONS;
      if (p->weaponowned[w] && p->ammo[p->weaponinfo[w].ammo] >= p->weaponinfo[w].ammopershoot)
        return w;
    } while (w != p->readyweapon);

  return 0;
}



// Builds a ticcmd from all of the available inputs

void ticcmd_t::Build(bool primary, int realtics)
{
#define KB_LOOKSPEED    (1<<25)
#define SLOWTURNTICS    (6*NEWTICRATERATIO)

  int  i, j, k;
  int (*gc)[2]; //pointer to array[num_gamecontrols][2]
  PlayerPawn *p = NULL;

  if (primary)
    {
      gc = gamecontrol;
      i = cv_autorun.value;
      j = cv_automlook.value;
      k = 0;
      if (consoleplayer)
	p = consoleplayer->pawn;
    }
  else
    {
      gc = gamecontrol2;
      i = cv_autorun2.value;
      j = cv_automlook2.value;
      k = 1;
      if (consoleplayer2)
        p = consoleplayer2->pawn;
    }


  int  speed = (gamekeydown[gc[gc_speed][0]] || gamekeydown[gc[gc_speed][1]]) ^ i;

  bool strafe = gamekeydown[gc[gc_strafe][0]] || gamekeydown[gc[gc_strafe][1]];
  bool turnright = gamekeydown[gc[gc_turnright][0]] || gamekeydown[gc[gc_turnright][1]];
  bool turnleft  = gamekeydown[gc[gc_turnleft][0]] || gamekeydown[gc[gc_turnleft][1]];

  bool mouseaiming = (gamekeydown[gc[gc_mouseaiming][0]]
		      ||gamekeydown[gc[gc_mouseaiming][1]]) ^ j;

  bool analogjoystickmove, gamepadjoystickmove;

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


  // initialization
  //memcpy(this, I_BaseTiccmd(), sizeof(ticcmd_t)); // FIXME dangerous
  buttons = 0;
  forward = side = 0;
  yaw = pitch = 0;
  if (p)
    {
      yaw   = p->angle >> 16;
      pitch = p->aiming >> 16;
    }


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
        side += sidespeed[speed];
      if (turnleft)
        side -= sidespeed[speed];

      if (analogjoystickmove)
        {
          //faB: JOYAXISRANGE is supposed to be 1023 ( divide by 1024)
          side += ( (joyxmove * sidespeed[1]) >> 10 );
        }
    }
  else
    {
      if (turnright)
        yaw -= angleturn[tspeed];
      //else
      if (turnleft)
        yaw += angleturn[tspeed];
      if (joyxmove && analogjoystickmove) {
        //faB: JOYAXISRANGE should be 1023 ( divide by 1024)
        yaw -= ( (joyxmove * angleturn[1]) >> 10 );        // ANALOG!
        //CONS_Printf ("joyxmove %d  angleturn %d\n", joyxmove, yaw);
      }
    }

  // forwards/backwards, strafing

  if (gamekeydown[gc[gc_forward][0]] || gamekeydown[gc[gc_forward][1]] ||
      (joyymove < 0 && gamepadjoystickmove && !cv_joystickfreelook.value))
    {
      forward += forwardspeed[speed];
    }
  if (gamekeydown[gc[gc_backward][0]] || gamekeydown[gc[gc_backward][1]] ||
      (joyymove > 0 && gamepadjoystickmove && !cv_joystickfreelook.value))
    {
      forward -= forwardspeed[speed];
    }

  if (joyymove && analogjoystickmove && !cv_joystickfreelook.value)
    forward -= ( (joyymove * forwardspeed[1]) >> 10 );

  if (gamekeydown[gc[gc_straferight][0]] || gamekeydown[gc[gc_straferight][1]])
    side += sidespeed[speed];
  if (gamekeydown[gc[gc_strafeleft][0]] || gamekeydown[gc[gc_strafeleft][1]])
    side -= sidespeed[speed];


  // buttons

  if (gamekeydown[gc[gc_fire][0]] || gamekeydown[gc[gc_fire][1]])
    buttons |= BT_ATTACK;

  if (gamekeydown[gc[gc_use][0]] || gamekeydown[gc[gc_use][1]])
    buttons |= BT_USE;

  if (gamekeydown[gc[gc_jump][0]] || gamekeydown[gc[gc_jump][1]])
    buttons |= BT_JUMP;

  if (gamekeydown[gc[gc_flydown][0]] || gamekeydown[gc[gc_flydown][1]])
    buttons |= BT_FLYDOWN;

  if (p == NULL)
    return;

  if (gamekeydown[gc[gc_nextweapon][0]] || gamekeydown[gc[gc_nextweapon][1]])
    buttons |= (NextWeapon(p, 1) << WEAPONSHIFT);
  else if (gamekeydown[gc[gc_prevweapon][0]] || gamekeydown[gc[gc_prevweapon][1]])
    buttons |= (NextWeapon(p, -1) << WEAPONSHIFT);
  else for (i = gc_weapon1; i <= gc_weapon8; i++)
    if (gamekeydown[gc[i][0]] || gamekeydown[gc[i][1]])
      {
	buttons |= ((p->FindWeapon(i - gc_weapon1) + 1) << WEAPONSHIFT);
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
          pitch -= mousey << 3;
        else
          pitch += mousey << 3;
      }
    if (cv_usejoystick.value && analogjoystickmove && cv_joystickfreelook.value)
      pitch += joyymove<<16;

    // spring back if not using keyboard neither mouselookin'
    if (!keyboard_look && !cv_joystickfreelook.value && !mouseaiming)
      pitch = 0;

    if (gamekeydown[gc[gc_lookup][0]] || gamekeydown[gc[gc_lookup][1]])
      {
        pitch += KB_LOOKSPEED;
        keyboard_look[0] = true;
      }
    else if (gamekeydown[gc[gc_lookdown][0]] || gamekeydown[gc[gc_lookdown][1]])
      {
        pitch -= KB_LOOKSPEED;
        keyboard_look[0] = true;
      }
    else if (gamekeydown[gc[gc_centerview][0]] || gamekeydown[gc[gc_centerview][1]])
      pitch = 0;

    //26/02/2000: added by Hurdler: accept no mlook for network games
    if (!cv_allowmlook.value)
      pitch = 0;

    if (!mouseaiming && cv_mousemove.value)
      forward += mousey;

    if (strafe)
      side += mousex << 1;
    else
      yaw -= mousex << 3;

    mousex = mousey = 0;

  } else {

    // mouse look stuff (mouse look is not the same as mouse aim)
    if (mouseaiming)
      {
        keyboard_look[1] = false;

        // looking up/down
        if (cv_invertmouse2.value)
          pitch -= mouse2y << 3;
        else
          pitch += mouse2y << 3;
      }

    if (analogjoystickmove && cv_joystickfreelook.value)
      pitch += joyymove<<16;
    // spring back if not using keyboard neither mouselookin'
    if (!keyboard_look && !cv_joystickfreelook.value && !mouseaiming)
      pitch = 0;

    if (gamekeydown[gamecontrol2[gc_lookup][0]] ||
        gamekeydown[gamecontrol2[gc_lookup][1]])
      {
        pitch += KB_LOOKSPEED;
        keyboard_look[1] = true;
      }
    else if (gamekeydown[gamecontrol2[gc_lookdown][0]] ||
             gamekeydown[gamecontrol2[gc_lookdown][1]])
      {
        pitch -= KB_LOOKSPEED;
        keyboard_look[1] = true;
      }
    else if (gamekeydown[gamecontrol2[gc_centerview][0]] ||
             gamekeydown[gamecontrol2[gc_centerview][1]])
      pitch = 0;

    //26/02/2000: added by Hurdler: accept no mlook for network games
    if (!cv_allowmlook.value)
      pitch = 0;

    if (!mouseaiming && cv_mousemove2.value)
      forward += mouse2y;

    if (strafe)
      side += mouse2x*2;
    else
      yaw -= mouse2x*8;

    mouse2x = mouse2y = 0;
  }

  if (forward > MAXPLMOVE)
    forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
    forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
    side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
    side = -MAXPLMOVE;


  pitch = G_ClipAimingPitch(pitch << 16) >> 16;
  CONS_Printf("Move: %d, %d, %d\n", yaw, pitch, buttons);
}



struct dclick_t
{
  int time;
  int state;
  int clicks;
};
static  dclick_t  mousedclicks[MOUSEBUTTONS];
static  dclick_t  joydclicks[JOYBUTTONS];

//
//  General double-click detection routine for any kind of input.
//
static bool G_CheckDoubleClick(int state, dclick_t *dt)
{
  if (state != dt->state && dt->time > 1 )
    {
      dt->state = state;
      if (state)
	dt->clicks++;
      if (dt->clicks == 2)
        {
	  dt->clicks = 0;
	  return true;
        }
      else
	dt->time = 0;
    }
  else
    {
      dt->time ++;
      if (dt->time > 20)
        {
	  dt->clicks = 0;
	  dt->state = 0;
        }
    }
  return false;
}

//
//  Remaps the inputs to game controls.
//  A game control can be triggered by one or more keys/buttons.
//  Each key/mousebutton/joybutton triggers ONLY ONE game control.
//
void G_MapEventsToControls(event_t *ev)
{
  switch (ev->type)
    {
    case ev_keydown:
      if (ev->data1 <NUMINPUTS)
	gamekeydown[ev->data1] = 1;
      break;

    case ev_keyup:
      if (ev->data1 <NUMINPUTS)
	gamekeydown[ev->data1] = 0;
      break;

    case ev_mouse:           // buttons are virtual keys
      mousex = int(ev->data2*((cv_mousesensx.value*cv_mousesensx.value)/110.0f + 0.1));
      mousey = int(ev->data3*((cv_mousesensy.value*cv_mousesensy.value)/110.0f + 0.1));
      break;

    case ev_joystick:        // buttons are virtual keys
      joyxmove = ev->data2;
      joyymove = ev->data3;
      break;

    case ev_mouse2:           // buttons are virtual keys
      mouse2x = int(ev->data2*((cv_mousesensx2.value*cv_mousesensx2.value)/110.0f + 0.1));
      mouse2y = int(ev->data3*((cv_mousesensy2.value*cv_mousesensy2.value)/110.0f + 0.1));
      break;

    default:
      break;

    }

  int i, flag;

  // ALWAYS check for mouse & joystick double-clicks
  // even if no mouse event
  for (i=0;i<MOUSEBUTTONS;i++)
    {
      flag = G_CheckDoubleClick(gamekeydown[KEY_MOUSE1+i], &mousedclicks[i]);
      gamekeydown[KEY_DBLMOUSE1+i] = flag;
    }

  for (i=0;i<JOYBUTTONS;i++)
    {
      flag = G_CheckDoubleClick(gamekeydown[KEY_JOY1+i], &joydclicks[i]);
      gamekeydown[KEY_DBLJOY1+i] = flag;
    }
}



struct keyname_t
{
  int  keynum;
  char name[15];
};

static keyname_t keynames[] =
{
  {KEY_SPACE     ,"SPACE"},
  {KEY_CAPSLOCK  ,"CAPS LOCK"},
  {KEY_ENTER     ,"ENTER"},
  {KEY_TAB       ,"TAB"},
  {KEY_ESCAPE    ,"ESCAPE"},
  {KEY_BACKSPACE ,"BACKSPACE"},
  {KEY_CONSOLE   ,"CONSOLE"},

  {KEY_NUMLOCK   ,"NUMLOCK"},
  {KEY_SCROLLLOCK,"SCROLLLOCK"},

  // bill gates keys

  {KEY_LEFTWIN   ,"LEFTWIN"},
  {KEY_RIGHTWIN  ,"RIGHTWIN"},
  {KEY_MENU      ,"MENU"},

  // shift,ctrl,alt are not distinguished between left & right

  {KEY_SHIFT     ,"SHIFT"},
  {KEY_CTRL      ,"CTRL"},
  {KEY_ALT       ,"ALT"},

  // keypad keys

  {KEY_KPADSLASH,"KEYPAD /"},

  {KEY_KEYPAD7, "KEYPAD 7"},
  {KEY_KEYPAD8, "KEYPAD 8"},
  {KEY_KEYPAD9, "KEYPAD 9"},
  {KEY_MINUSPAD,"KEYPAD -"},
  {KEY_KEYPAD4, "KEYPAD 4"},
  {KEY_KEYPAD5, "KEYPAD 5"},
  {KEY_KEYPAD6, "KEYPAD 6"},
  {KEY_PLUSPAD, "KEYPAD +"},
  {KEY_KEYPAD1, "KEYPAD 1"},
  {KEY_KEYPAD2, "KEYPAD 2"},
  {KEY_KEYPAD3, "KEYPAD 3"},
  {KEY_KEYPAD0, "KEYPAD 0"},
  {KEY_KPADDEL, "KEYPAD ."},

  // extended keys (not keypad)

  {KEY_HOME,      "HOME"},
  {KEY_UPARROW,   "UP ARROW"},
  {KEY_PGUP,      "PGUP"},
  {KEY_LEFTARROW ,"LEFT ARROW"},
  {KEY_RIGHTARROW,"RIGHT ARROW"},
  {KEY_END,       "END"},
  {KEY_DOWNARROW, "DOWN ARROW"},
  {KEY_PGDN,      "PGDN"},
  {KEY_INS,       "INS"},
  {KEY_DELETE,    "DEL"},

  // other keys

  {KEY_F1, "F1"},
  {KEY_F2, "F2"},
  {KEY_F3, "F3"},
  {KEY_F4, "F4"},
  {KEY_F5, "F5"},
  {KEY_F6, "F6"},
  {KEY_F7, "F7"},
  {KEY_F8, "F8"},
  {KEY_F9, "F9"},
  {KEY_F10,"F10"},
  {KEY_F11,"F11"},
  {KEY_F12,"F12"},

  // virtual keys for mouse buttons and joystick buttons

  {KEY_MOUSE1,  "MOUSE1"},
  {KEY_MOUSE1+1,"MOUSE2"},
  {KEY_MOUSE1+2,"MOUSE3"},
  {KEY_MOUSEWHEELUP, "mwheel up"},
  {KEY_MOUSEWHEELDOWN,"mwheel down"},
  {KEY_MOUSE1+5,"MOUSE6"},
  {KEY_MOUSE1+6,"MOUSE7"},
  {KEY_MOUSE1+7,"MOUSE8"},
  {KEY_2MOUSE1,  "SEC_MOUSE2"},    //BP: sorry my mouse handler swap button 1 and 2
  {KEY_2MOUSE1+1,"SEC_MOUSE1"},
  {KEY_2MOUSE1+2,"SEC_MOUSE3"},
  {KEY_2MOUSEWHEELUP,"m2wheel up"},
  {KEY_2MOUSEWHEELDOWN,"m2wheel down"},
  {KEY_2MOUSE1+5,"SEC_MOUSE6"},
  {KEY_2MOUSE1+6,"SEC_MOUSE7"},
  {KEY_2MOUSE1+7,"SEC_MOUSE8"},

  {KEY_JOY1,  "JOY1"},
  {KEY_JOY1+1,"JOY2"},
  {KEY_JOY1+2,"JOY3"},
  {KEY_JOY1+3,"JOY4"},
  {KEY_JOY1+4,"JOY5"},
  {KEY_JOY1+5,"JOY6"},
  // we use up to 10 buttons in DirectInput
  {KEY_JOY1+6,"JOY7"},
  {KEY_JOY1+7,"JOY8"},
  {KEY_JOY1+8,"JOY9"},
  {KEY_JOY1+9,"JOY10"},
  // the DOS version uses Allegro's joystick support
  // the Hat is reported as extra buttons
  {KEY_JOY1+10,"HATUP"},
  {KEY_JOY1+11,"HATDOWN"},
  {KEY_JOY1+12,"HATLEFT"},
  {KEY_JOY1+13,"HATRIGHT"},

  {KEY_DBLMOUSE1,   "DBLMOUSE1"},
  {KEY_DBLMOUSE1+1, "DBLMOUSE2"},
  {KEY_DBLMOUSE1+2, "DBLMOUSE3"},
  {KEY_DBLMOUSE1+3, "DBLMOUSE4"},
  {KEY_DBLMOUSE1+4, "DBLMOUSE5"},
  {KEY_DBLMOUSE1+5, "DBLMOUSE6"},
  {KEY_DBLMOUSE1+6, "DBLMOUSE7"},
  {KEY_DBLMOUSE1+7, "DBLMOUSE8"},
  {KEY_DBL2MOUSE1,  "DBLSEC_MOUSE2"},  //BP: sorry my mouse handler swap button 1 and 2
  {KEY_DBL2MOUSE1+1,"DBLSEC_MOUSE1"},
  {KEY_DBL2MOUSE1+2,"DBLSEC_MOUSE3"},
  {KEY_DBL2MOUSE1+3,"DBLSEC_MOUSE4"},
  {KEY_DBL2MOUSE1+4,"DBLSEC_MOUSE5"},
  {KEY_DBL2MOUSE1+5,"DBLSEC_MOUSE6"},
  {KEY_DBL2MOUSE1+6,"DBLSEC_MOUSE7"},
  {KEY_DBL2MOUSE1+7,"DBLSEC_MOUSE8"},

  {KEY_DBLJOY1,  "DBLJOY1"},
  {KEY_DBLJOY1+1,"DBLJOY2"},
  {KEY_DBLJOY1+2,"DBLJOY3"},
  {KEY_DBLJOY1+3,"DBLJOY4"},
  {KEY_DBLJOY1+4,"DBLJOY5"},
  {KEY_DBLJOY1+5,"DBLJOY6"}
};

char *gamecontrolname[num_gamecontrols] =
{
  "nothing",        //a key/button mapped to gc_null has no effect
  "forward",
  "backward",
  "strafe",
  "straferight",
  "strafeleft",
  "speed",
  "turnleft",
  "turnright",
  "fire",
  "use",
  "lookup",
  "lookdown",
  "centerview",
  "mouseaiming",
  "weapon1",
  "weapon2",
  "weapon3",
  "weapon4",
  "weapon5",
  "weapon6",
  "weapon7",
  "weapon8",
  "talkkey",
  "scores",
  "jump",
  "console",
  "nextweapon",
  "prevweapon",
  "bestweapon",
  "inventorynext",
  "inventoryprev",
  "inventoryuse",
  "down"
};

static const int NUMKEYNAMES = sizeof(keynames)/sizeof(keyname_t);

//
//  Detach any keys associated to the given game control
//  - pass the pointer to the gamecontrol table for the player being edited
void  G_ClearControlKeys(int (*setupcontrols)[2], int control)
{
  setupcontrols[control][0] = KEY_NULL;
  setupcontrols[control][1] = KEY_NULL;
}

//
//  Returns the name of a key (or virtual key for mouse and joy)
//  the input value being an keynum
//
char* G_KeynumToString(int keynum)
{
  static char keynamestr[8];

  int    j;

  // return a string with the ascii char if displayable
  if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
    {
      keynamestr[0] = keynum;
      keynamestr[1] = '\0';
      return keynamestr;
    }

  // find a description for special keys
  for (j=0;j<NUMKEYNAMES;j++)
    if (keynames[j].keynum==keynum)
      return keynames[j].name;

  // create a name for Unknown key
  sprintf (keynamestr,"KEY%d",keynum);
  return keynamestr;
}


int G_KeyStringtoNum(char *keystr)
{
  int j;

  //    strupr(keystr);

  if(keystr[1]==0 && keystr[0]>' ' && keystr[0]<='z')
    return keystr[0];

  for (j=0;j<NUMKEYNAMES;j++)
    if (stricmp(keynames[j].name,keystr)==0)
      return keynames[j].keynum;

  if(strlen(keystr)>3)
    return atoi(&keystr[3]);

  return 0;
}

void G_Controldefault()
{
  gamecontrol[gc_forward    ][0]=KEY_UPARROW;
  gamecontrol[gc_forward    ][1]=KEY_MOUSE1+2;
  gamecontrol[gc_backward   ][0]=KEY_DOWNARROW;
  gamecontrol[gc_strafe     ][0]=KEY_ALT;
  gamecontrol[gc_strafe     ][1]=KEY_MOUSE1+1;
  gamecontrol[gc_straferight][0]='.';
  gamecontrol[gc_strafeleft ][0]=',';
  gamecontrol[gc_speed      ][0]=KEY_SHIFT;
  gamecontrol[gc_turnleft   ][0]=KEY_LEFTARROW;
  gamecontrol[gc_turnright  ][0]=KEY_RIGHTARROW;
  gamecontrol[gc_fire       ][0]=KEY_CTRL;
  gamecontrol[gc_fire       ][1]=KEY_MOUSE1;
  gamecontrol[gc_use        ][0]=KEY_SPACE;
  gamecontrol[gc_lookup     ][0]=KEY_PGUP;
  gamecontrol[gc_lookdown   ][0]=KEY_PGDN;
  gamecontrol[gc_centerview ][0]=KEY_END;
  gamecontrol[gc_mouseaiming][0]='s';
  gamecontrol[gc_weapon1    ][0]='1';
  gamecontrol[gc_weapon2    ][0]='2';
  gamecontrol[gc_weapon3    ][0]='3';
  gamecontrol[gc_weapon4    ][0]='4';
  gamecontrol[gc_weapon5    ][0]='5';
  gamecontrol[gc_weapon6    ][0]='6';
  gamecontrol[gc_weapon7    ][0]='7';
  gamecontrol[gc_weapon8    ][0]='8';
  gamecontrol[gc_talkkey    ][0]='t';
  gamecontrol[gc_scores     ][0]='f';
  gamecontrol[gc_jump       ][0]='/';
  gamecontrol[gc_console    ][0]=KEY_CONSOLE;
  gamecontrol[gc_nextweapon ][1]=KEY_JOY1+4;
  gamecontrol[gc_prevweapon ][1]=KEY_JOY1+5;
  
  gamecontrol[gc_invnext    ][0] = ']';
  gamecontrol[gc_invprev    ][0] = '[';
  gamecontrol[gc_invuse     ][0] = KEY_ENTER;
  gamecontrol[gc_jump       ][0] = KEY_INS;
  gamecontrol[gc_flydown    ][0] = KEY_KPADDEL;

  //gamecontrol[gc_nextweapon ][0]=']';
  //gamecontrol[gc_prevweapon ][0]='[';
}

void G_SaveKeySetting(FILE *f)
{
  int i;

  for(i=1;i<num_gamecontrols;i++)
    {
      fprintf(f,"setcontrol \"%s\" \"%s\""
	      ,gamecontrolname[i]
	      ,G_KeynumToString(gamecontrol[i][0]));

      if(gamecontrol[i][1])
	fprintf(f," \"%s\"\n"
		,G_KeynumToString(gamecontrol[i][1]));
      else
	fprintf(f,"\n");
    }

  for(i=1;i<num_gamecontrols;i++)
    {
      fprintf(f,"setcontrol2 \"%s\" \"%s\""
	      ,gamecontrolname[i]
	      ,G_KeynumToString(gamecontrol2[i][0]));

      if(gamecontrol2[i][1])
	fprintf(f," \"%s\"\n"
		,G_KeynumToString(gamecontrol2[i][1]));
      else
	fprintf(f,"\n");
    }
}

void G_CheckDoubleUsage(int keynum)
{
  if( cv_controlperkey.value==1 )
    {
      int i;
      for(i=0;i<num_gamecontrols;i++)
        {
	  if( gamecontrol[i][0]==keynum )
	    gamecontrol[i][0]= KEY_NULL;
	  if( gamecontrol[i][1]==keynum )
	    gamecontrol[i][1]= KEY_NULL;
	  if( gamecontrol2[i][0]==keynum )
	    gamecontrol2[i][0]= KEY_NULL;
	  if( gamecontrol2[i][1]==keynum )
	    gamecontrol2[i][1]= KEY_NULL;
        }
    }
}

void setcontrol(int (*gc)[2],int na)
{
  int numctrl;
  char *namectrl;
  int keynum;

  namectrl=COM_Argv(1);
  for(numctrl=0;numctrl<num_gamecontrols
	&& stricmp(namectrl,gamecontrolname[numctrl])
	;numctrl++);
  if(numctrl==num_gamecontrols)
    {
      CONS_Printf("Control '%s' unknown\n",namectrl);
      return;
    }
  keynum=G_KeyStringtoNum(COM_Argv(2));
  G_CheckDoubleUsage(keynum);
  gc[numctrl][0]=keynum;

  if(na==4)
    gc[numctrl][1]=G_KeyStringtoNum(COM_Argv(3));
  else
    gc[numctrl][1]=0;
}

void Command_Setcontrol_f()
{
  int na = COM_Argc();

  if (na!= 3 && na!=4)
    {
      CONS_Printf ("setcontrol <controlname> <keyname> [<2nd keyname>]\n");
      return;
    }

  setcontrol(gamecontrol,na);
}

void Command_Setcontrol2_f()
{
  int na = COM_Argc();

  if (na!= 3 && na!=4)
    {
      CONS_Printf ("setcontrol2 <controlname> <keyname> [<2nd keyname>]\n");
      return;
    }

  setcontrol(gamecontrol2,na);
}
