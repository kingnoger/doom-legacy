// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004 by DooM Legacy Team.
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
// Revision 1.7  2004/08/18 14:35:22  smite-meister
// PNG support!
//
// Revision 1.6  2004/08/06 18:54:39  smite-meister
// netcode update
//
// Revision 1.5  2004/07/25 20:17:26  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.4  2004/07/11 14:32:01  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.3  2004/07/07 17:27:20  smite-meister
// bugfixes
//
// Revision 1.2  2004/07/05 16:53:30  smite-meister
// Netcode replaced
//
// Revision 1.1  2004/06/25 19:50:59  smite-meister
// Netcode
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
#include "hu_stuff.h"
#include "m_menu.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_main.h"
#include "hardware/hwr_render.h"
#include "m_misc.h"
#include "w_wad.h"

#include "i_system.h"
#include "i_video.h"


void  Command_Setcontrol_f();
void  Command_Setcontrol2_f();

void Command_Water_f();

void SendWeaponPref();
void SendNameAndColor();
void SendNameAndColor2();



//=========================================================================
//        Client consvars
//=========================================================================


CV_PossibleValue_t Color_cons_t[] =
{
  {0,"green"}, {1,"gray"}, {2,"brown"}, {3,"red"}, {4,"light gray"},
  {5,"light brown"}, {6,"light red"}, {7,"light blue"}, {8,"blue"}, {9,"yellow"},
  {10,"beige"}, {0,NULL}
};
#define DEFAULTSKIN "marine"   // name of the standard doom marine as skin

// player info and preferences
consvar_t cv_playername  = {"name"      ,"gi joe"   ,CV_SAVE | CV_CALL | CV_NOINIT,NULL,SendNameAndColor};
consvar_t cv_playercolor = {"color"     ,"0"        ,CV_SAVE | CV_CALL | CV_NOINIT,Color_cons_t,SendNameAndColor};
consvar_t cv_skin        = {"skin"      ,DEFAULTSKIN,CV_SAVE | CV_CALL | CV_NOINIT,NULL,SendNameAndColor};
consvar_t cv_autoaim     = {"autoaim"   ,"1"        ,CV_SAVE | CV_CALL | CV_NOINIT,CV_OnOff,SendWeaponPref};
consvar_t cv_originalweaponswitch = {"originalweaponswitch","0",CV_SAVE | CV_CALL | CV_NOINIT,CV_OnOff,SendWeaponPref};
consvar_t cv_weaponpref  = {"weaponpref","014576328",CV_SAVE | CV_CALL | CV_NOINIT,NULL,SendWeaponPref};

// secondary player for splitscreen mode
consvar_t cv_playername2  = {"name2"    ,"big b"    ,CV_SAVE | CV_CALL | CV_NOINIT,NULL,SendNameAndColor2};
consvar_t cv_playercolor2 = {"color2"   ,"1"        ,CV_SAVE | CV_CALL | CV_NOINIT,Color_cons_t,SendNameAndColor2};
consvar_t cv_skin2        = {"skin2"    ,DEFAULTSKIN,CV_SAVE | CV_CALL | CV_NOINIT,NULL /*skin_cons_t*/,SendNameAndColor2};
consvar_t cv_autoaim2     = {"autoaim2" ,"1"        ,CV_SAVE | CV_CALL | CV_NOINIT,CV_OnOff,SendWeaponPref};


void SplitScreen_OnChange()
{
  // recompute screen size
  R_ExecuteSetViewSize();

  // change the menu
  M_SwitchSplitscreen();

  if (game.state < GameInfo::GS_LEVEL)
    return;

  // FIXME splitscreen
  if (cv_splitscreen.value)
    {
      displayplayer2 = consoleplayer2 = game.AddPlayer(new PlayerInfo(localplayer2));
      // TODO as server first
    }
  else
    {
      consoleplayer2->playerstate = PST_REMOVE;
    }

  // TODO in a demo, just open another viewport...
  //displayplayer2 = game.FindPlayer(1);
}

consvar_t cv_splitscreen = {"splitscreen","0",CV_CALL,CV_OnOff,SplitScreen_OnChange};




// =========================================================================
//                           CLIENT INIT
// =========================================================================

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

  // init renderer
  CONS_Printf("R_Init: Init DOOM refresh daemon.\n");
  R_Init();

  // set up sound and music
  CONS_Printf("S_Init: Init sound module.\n");
  S.Startup();

  // read the basic legacy.wad sound script lumps
  S_Read_SNDINFO(fc.FindNumForNameFile("SNDINFO", 0));
  S_Read_SNDSEQ(fc.FindNumForNameFile("SNDSEQ", 0));


  COM_AddCommand("setcontrol", Command_Setcontrol_f);
  COM_AddCommand("setcontrol2", Command_Setcontrol2_f);
  COM_AddCommand("screenshot",M_ScreenShot);

  // FIXME WATER HACK TEST UNTIL FULLY FINISHED
  COM_AddCommand("dev_water", Command_Water_f);


  // client info
  char *temp = I_GetUserName();
  if (temp)
    cv_playername.defaultvalue = temp;

  cv_playername.Reg();
  cv_playercolor.Reg();
  cv_skin.Reg();
  cv_autoaim.Reg();
  cv_originalweaponswitch.Reg();
  cv_weaponpref.Reg();

  // secondary player
  cv_playername2.Reg();
  cv_playercolor2.Reg();
  cv_skin2.Reg();
  cv_autoaim2.Reg();
  //cv_originalweaponswitch2.Reg();
  //cv_weaponpref2.Reg();


  // client input
  cv_controlperkey.Reg();

  cv_autorun.Reg();
  cv_automlook.Reg();
  cv_usemouse.Reg();
  cv_invertmouse.Reg();
  cv_mousemove.Reg();
  cv_mousesensx.Reg();
  cv_mousesensy.Reg();

  cv_autorun2.Reg();
  cv_automlook2.Reg();
  cv_usemouse2.Reg();
  cv_invertmouse2.Reg();
  cv_mousemove2.Reg();
  cv_mousesensx2.Reg();
  cv_mousesensy2.Reg();

  // WARNING : the order is important when initing mouse2, we need the mouse2port
  cv_mouse2port.Reg();
#ifdef LMOUSE2
  cv_mouse2opt.Reg();
#endif

  cv_usejoystick.Reg();
  cv_joystickfreelook.Reg();
#ifdef LJOYSTICK
  cv_joyport.Reg();
  cv_joyscale.Reg();
#endif


  // client renderer
  cv_viewheight.Reg();
  cv_chasecam.Reg();
  cv_cam_dist.Reg();
  cv_cam_height.Reg();
  cv_cam_speed.Reg();

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


void SendWeaponPref()
{
  // TODO remove not needed
}

void Got_WeaponPref(char **cp,int playernum)
{
  // TODO remove not needed
}


void D_SendPlayerConfig()
{
  SendNameAndColor();
  if(cv_splitscreen.value)
    SendNameAndColor2();
  SendWeaponPref();
}











/*

void SL_InsertServer( serverinfo_pak *info, int node)
{
    int i;
    bool moved;

    // search if not allready on it
    i = SL_SearchServer( node );
    if( i==-1 )
    {
        // not found add it
        if( serverlistcount >= MAXSERVERLIST )
            return; // list full
        i=serverlistcount++;
    }

    serverlist[i].info = *info;
    serverlist[i].node = node;

    // list is sorted by time (ping)
    // so move the entry until it is sorted
    do {
        moved = false;
        if( i>0 && serverlist[i].info.time < serverlist[i-1].info.time )
        {
            serverelem_t s;
            s = serverlist[i];
            serverlist[i] =  serverlist[i-1];
            serverlist[i-1] = s;
            i--;
            moved = true;
        }
        else
        if( i<serverlistcount-1 && serverlist[i].info.time > serverlist[i+1].info.time )
        {
            serverelem_t s;
            s = serverlist[i];
            serverlist[i] =  serverlist[i+1];
            serverlist[i+1] = s;
            i++;
            moved = true;
        }
    } while(moved);
}

void CL_UpdateServerList( bool internetsearch )
{
    SL_ClearServerList(0);

    if( !game.netgame )
    {
        I_NetOpenSocket();
        game.netgame = true;
        game.multiplayer = true;
    }
    // search for local servers
    SendAskInfo( BROADCASTADDR );

    if( internetsearch )
    {
        msg_server_t *server_list;
        int          i;

        if( (server_list = GetShortServersList()) )
        {
            for (i=0; server_list[i].header[0]; i++)
            {
                int  node;
                char addr_str[24];

                // insert ip (and optionaly port) in node list
                sprintf(addr_str, "%s:%s", server_list[i].ip, server_list[i].port);
                node = I_NetMakeNode(addr_str);
                if( node == -1 )
                    break; // no more node free
                SendAskInfo( node );
            }
        }
    }
}
*/




/*
void CL_RemovePlayer(int playernum)
{
  // FIXME netnode information should be stored in PlayerInfo!
  if( server && !demoplayback )
    {
      int node = playernode[playernum];
      playerpernode[node]--;
      if( playerpernode[node]<=0 )
        {
          nodeingame[playernode[playernum]] = false;
          Net_CloseConnection(playernode[playernum]);
          ResetNode(node);
        }
    }

  // player is scheduled for removal
  PlayerInfo *p = game.FindPlayer(playernum);
  if (p)
    p->playerstate = PST_REMOVE;

  playernode[playernum] = 255;
  //while (playeringame[doomcom->numplayers-1]==0 && doomcom->numplayers>1) doomcom->numplayers--;
  while (playernode[doomcom->numplayers-1] == 255 && doomcom->numplayers>1) doomcom->numplayers--;
}

*/



void GameInfo::CL_Reset()
{
  CONS_Printf("Client reset\n");

  net->CL_Reset();
}
