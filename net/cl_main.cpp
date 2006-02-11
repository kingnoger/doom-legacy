// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2005 by DooM Legacy Team.
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
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Main client code
///
/// Uses OpenTNL


#include "doomdef.h"
#include "command.h"
#include "cvars.h"

#include "n_interface.h"

#include "g_game.h"
#include "g_player.h"
#include "g_input.h"

#include "screen.h"
#include "console.h"
#include "hud.h"
#include "m_menu.h"
#include "am_map.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_main.h"
#include "hardware/hwr_render.h"
#include "m_misc.h"
#include "w_wad.h"

#include "i_system.h"
#include "i_video.h"



//=========================================================================
//        Client consvars
//=========================================================================

void SplitScreen_OnChange()
{
  // recompute screen size
  R_ExecuteSetViewSize();

  // change the menu
  M_SwitchSplitscreen();

  if (game.state < GameInfo::GS_LEVEL)
    return;

  // FIXME splitscreen
  /*
  if (cv_splitscreen.value)
    {
      displayplayer2 = consoleplayer2 = game.AddPlayer(new PlayerInfo(localplayer2));
      // TODO as server first
    }
  else
    {
      consoleplayer2->playerstate = PST_REMOVE;
    }
  */
  // TODO in a demo, just open another viewport...
  //displayplayer2 = game.FindPlayer(1);
}

consvar_t cv_splitscreen = {"splitscreen", "0", CV_CALL, CV_OnOff, SplitScreen_OnChange};



//=========================================================================
//        Client commands
//=========================================================================

void Command_Setcontrol_f();
void Command_BindJoyaxis_f();
void Command_UnbindJoyaxis_f();

void Command_Water_f();

/// Used for setting player properties.
void Command_Player_f()
{
  if (COM_Argc() != 4)
    {
      CONS_Printf("player <number> <attribute> <value>\n");
      return;
    }

  int n = atoi(COM_Argv(1));
  if (n >= NUM_LOCALHUMANS)
    {
      CONS_Printf("Only %d local players supported.\n", NUM_LOCALHUMANS);
      return;
    }
  LocalPlayerInfo *p = &LocalPlayers[n];

  const char *attr = COM_Argv(2);
  const char *val  = COM_Argv(3);

  if (!strcasecmp(attr, "name"))
    p->name = val;
  else if (!strcasecmp(attr, "color"))
    p->color = atoi(val);
  else if (!strcasecmp(attr, "autoaim"))
    p->autoaim = atoi(val);
  else if (!strcasecmp(attr, "messages"))
    p->messagefilter = atoi(val);
  else if (!strcasecmp(attr, "autorun"))
    p->autorun = atoi(val);
  else if (!strcasecmp(attr, "crosshair"))
    p->crosshair = atoi(val);
  else
    CONS_Printf("Unknown player attribute '%s'.\n", attr);
}


//=========================================================================
//                           CLIENT INIT
//=========================================================================

void CL_Init()
{
  // contains only stuff that a _dedicated_ server does _not_ need
  CONS_Printf("CL_Init\n");

  // set the video mode, graphics scaling properties, load palette
  CONS_Printf("V_Init: Init the video module.\n");
  vid.Startup();

  // we need the HUD font for the console
  // HUD font, crosshairs, say commands
  CONS_Printf("HU_Init: Init the Heads Up Display\n");
  hud.Startup();

  // startup console
  con.Init(); //-------------------------------------- CONSOLE is on

  // setup menu
  CONS_Printf("M_Init: Init menu.\n");
  Menu::Startup();

  automap.Startup();

  // init renderer
  CONS_Printf("R_Init: Init DOOM refresh daemon.\n");
  R_Init();

  // set up sound and music
  CONS_Printf("S_Init: Init sound module.\n");
  S.Startup();

  // read the basic legacy.wad sound script lumps
  S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", 0));
  S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", 0));

  COM_AddCommand("player", Command_Player_f);
  COM_AddCommand("setcontrol", Command_Setcontrol_f);
  COM_AddCommand("bindjoyaxis", Command_BindJoyaxis_f);
  COM_AddCommand("unbindjoyaxis", Command_UnbindJoyaxis_f);

  COM_AddCommand("screenshot",M_ScreenShot);

  // FIXME WATER HACK TEST UNTIL FULLY FINISHED
  COM_AddCommand("dev_water", Command_Water_f);

  // client info
  char *temp = I_GetUserName();
  if (temp)
    LocalPlayers[0].name = temp;

  // client input
  cv_controlperkey.Reg();

  // WARNING : the order is important when initing mouse2, we need the mouse2port
  cv_mouse2port.Reg();
#ifdef LMOUSE2
  cv_mouse2opt.Reg();
#endif

  // we currently support two mice
  for (int i=0; i<2; i++)
    {
      cv_usemouse[i].Reg();
      cv_mousesensx[i].Reg();
      cv_mousesensy[i].Reg();
      cv_automlook[i].Reg();
      cv_mousemove[i].Reg();
      cv_invertmouse[i].Reg();
    }

  // client renderer
  cv_viewheight.Reg();

  cv_scr_width.Reg();
  cv_scr_height.Reg();
  cv_scr_depth.Reg();
  cv_fullscreen.Reg();
  cv_usegamma.Reg();

  cv_viewsize.Reg();
  cv_detaillevel.Reg();
  cv_scalestatusbar.Reg();
  //cv_fov.Reg();
  cv_splitscreen.Reg();

  cv_screenslink.Reg();
  cv_translucency.Reg();
  cv_fuzzymode.Reg();
  cv_splats.Reg();
  cv_bloodtime.Reg();
  cv_psprites.Reg();

#ifdef HWRENDER
  if (rendermode != render_soft)
  {
    HWR.AddCommands();
  }
#endif
}



// =========================================================================
//                            CLIENT STUFF
// =========================================================================

//  name, color, or skin has changed
//
void SendNameAndColor()
{
  // game.net->server_con->c2sSendClientInfo(number, name, color, skin)
  // better yet, clientinfo struct...
  /*
  struct playerinfo_t
  {
    name, color, skin, autoaim, weaponpref (what if ammo runs out on current weapon...)
  }
  */
}

// splitscreen
void SendNameAndColor2()
{
  // game.net->server_con->c2sSendClientInfo(2)
}


void Got_NameAndcolor(char **cp,int playernum)
{
  /*
  PlayerInfo *p = game.FindPlayer(playernum);

  // color
  p->color=READBYTE(*cp) % MAXSKINCOLORS;

  // a copy of color
  if(p->pawn)
    p->pawn->color = p->color;

  const char *oldname = p->name.c_str();
  char newname[50];
  // name
  if (strcasecmp(oldname, *cp))
    CONS_Printf("%s renamed to %s\n", oldname, *cp);
  READSTRING(*cp, newname);
  p->name = newname;

  // skin
  SetPlayerSkin(playernum,*cp);
  SKIPSTRING(*cp);


  PlayerInfo *p = game.FindPlayer(playernum);
  p->originalweaponswitch = *(*cp)++;
  memcpy(p->favoriteweapon,*cp,NUMWEAPONS);
  *cp+=NUMWEAPONS;
  p->autoaim = *(*cp)++;
  */
}



void GameInfo::CL_Reset()
{
  CONS_Printf("Client reset\n");

  net->CL_Reset();
}
