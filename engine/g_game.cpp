// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// Revision 1.37  2004/10/27 17:37:06  smite-meister
// netcode update
//
// Revision 1.36  2004/10/14 19:35:30  smite-meister
// automap, bbox_t
//
// Revision 1.35  2004/09/23 23:21:16  smite-meister
// HUD updated
//
// Revision 1.34  2004/09/03 16:28:49  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.33  2004/08/15 18:08:28  smite-meister
// palette-to-palette colormaps etc.
//
// Revision 1.32  2004/08/12 18:30:23  smite-meister
// cleaned startup
//
// Revision 1.31  2004/07/25 20:19:19  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.30  2004/07/14 16:13:13  smite-meister
// cleanup, commands
//
// Revision 1.29  2004/07/13 20:23:35  smite-meister
// Mod system basics
//
// Revision 1.28  2004/07/11 14:32:00  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.27  2004/07/09 19:43:39  smite-meister
// Netcode fixes
//
// Revision 1.26  2004/07/05 16:53:24  smite-meister
// Netcode replaced
//
// Revision 1.25  2004/05/01 23:29:19  hurdler
// add dummy new renderer
//
// Revision 1.24  2004/04/25 16:26:48  smite-meister
// Doxygen
//
// Revision 1.20  2004/01/02 14:22:58  smite-meister
// items work
//
// Revision 1.19  2003/12/31 18:32:49  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.18  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.17  2003/12/09 01:02:00  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.16  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.14  2003/11/12 11:07:17  smite-meister
// Serialization done. Map progression.
//
// Revision 1.13  2003/06/01 18:56:29  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.12  2003/05/30 13:34:42  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.11  2003/05/11 21:23:49  smite-meister
// Hexen fixes
//
// Revision 1.10  2003/04/26 12:01:12  smite-meister
// Bugfixes. Hexen maps work again.
//
// Revision 1.9  2003/04/04 00:01:53  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.8  2003/03/08 16:07:00  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.7  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.6  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.5  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:11:00  smite-meister
// Actor/DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:08  hurdler
// Initial C++ version of Doom Legacy
//-----------------------------------------------------------------------------

/// \file
/// \brief Part of GameInfo class implementation.

#include "doomdef.h"
#include "command.h"
#include "console.h"
#include "cvars.h"

#include "dstrings.h"
#include "g_game.h"
#include "g_player.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_input.h"
#include "g_type.h"

#include "n_connection.h"

#include "d_event.h"
#include "d_items.h"
#include "d_main.h"

#include "p_saveg.h"

#include "i_system.h"
#include "m_random.h"

#include "r_render.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "v_video.h"

#include "s_sound.h"
#include "sounds.h"

#include "m_menu.h"
#include "am_map.h"
#include "hud.h"
#include "wi_stuff.h"
#include "f_finale.h"
#include "f_wipe.h"

#include "keys.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_video.h" // rendermode! fix!
#ifdef HWRENDER
#include "hardware/hwr_render.h"
#endif


bool nodrawers;    // for comparative timing purposes



GameInfo game; ///< The single global instance of GameInfo.


GameInfo::GameInfo()
{
  demoversion = VERSION;
  mode = gm_none;
  state = GS_NULL;
  action = ga_nothing;
  skill = sk_medium;
  maxplayers = 32;
  maxteams = 4;
  currentcluster = NULL;

  server = true;
  netgame = multiplayer = false;
  modified = paused = inventory = false;
  net = NULL;

  time = tic = 0;

  gtype = new GameType(); // TEST
};


GameInfo::~GameInfo()
{
  //ClearPlayers();
};




//================
//  INTRO LOOP
//================

static int   demosequence;
static char *pagename = "TITLEPIC";

/// starts the intro sequence
void GameInfo::StartIntro()
{
  extern int demosequence;
  action = ga_nothing;
  state = GS_INTRO;

  demosequence = -1;
  paused = false;
  pagetic = 0;
  con.Toggle(true);
}



/// This cycles through the demo sequences.
void GameInfo::AdvanceIntro()
{
  if (mode == gm_udoom)
    demosequence = (demosequence+1)%7;
  else
    demosequence = (demosequence+1)%6;

  switch (demosequence)
    {
    case 0:
      pagename = "TITLEPIC";
      switch (mode)
        {
        case gm_hexen:
          pagetic = 280;
          pagename = "TITLE";
          S.StartMusic("HEXEN", true);
          break;
        case gm_heretic:
          pagetic = 210+140;
          pagename = "TITLE";
          S_StartMusic(mus_htitl);
          break;
        case gm_doom2:
          pagetic = TICRATE * 11;
          S_StartMusic(mus_dm2ttl);
          break;
        default:
          pagetic = 170;
          S_StartMusic(mus_intro);
          break;
        }
      break;
    case 1:
      //G_DeferedPlayDemo("DEMO1");
      pagetic = 9999999;
      break;
    case 2:
      pagetic = 200;
      pagename = "CREDIT";
      break;
    case 3:
      //G_DeferedPlayDemo("DEMO2");
      pagetic = 9999999;
      break;
    case 4:
      pagetic = 200;
      switch (mode)
        {
        case gm_doom2:
          pagetic = TICRATE * 11;
          pagename = "TITLEPIC";
          S_StartMusic(mus_dm2ttl);
          break;
        case gm_heretic:
          if (fc.FindNumForName("E2M1") == -1)
            pagename = "ORDER";
          else
            pagename = "CREDIT";
          break;
        case gm_hexen:
          pagename = "CREDIT";
          break;
        case gm_udoom:
          pagename = "CREDIT";
          break;
        default:
          pagename = "HELP2";
        }
      break;
    case 5:
      //G_DeferedPlayDemo("DEMO3");
      pagetic = 9999999;
      break;
      // THE DEFINITIVE DOOM Special Edition demo
    case 6:
      //G_DeferedPlayDemo("DEMO4");
      pagetic = 9999999;
      break;
    }
}



/// draws a fullscreen picture ("page"), fills the borders with a background
/// pattern (a flat) if the page doesn't fill all the screen.
void D_PageDrawer(char *lumpname)
{
  // software mode which uses generally lower resolutions doesn't look
  // good when the pic is scaled, so it fills space aorund with a pattern,
  // and the pic is only scaled to integer multiples (x2, x3...)
  if (rendermode == render_soft)
    {
      if ((vid.width>BASEVIDWIDTH) || (vid.height>BASEVIDHEIGHT) )
        {
          for (int y=0; y<vid.height; y += window_background->height)
            for (int x=0; x<vid.width; x += window_background->width)
              window_background->Draw(x,y,0);
        }
    }


  Texture *t = tc.GetPtr(lumpname);
  t->Draw(0, 0, V_SCALE);

  if (game.mode >= gm_heretic && demosequence == 0 && game.pagetic <= 140)
    {
      t = tc.GetPtr("ADVISOR");
      t->Draw(4, 160, V_SCALE);
    }
}




//======================================================================
// D_Display
//  draw current display, possibly wiping it from the previous
//======================================================================

bool force_wipe = false; // TODO into renderer module...


void GameInfo::Display()
{
  extern int scaledviewwidth;

  static gamestate_t oldgamestate = GS_NULL;
  static int borderdrawcount;
  static int screenwipe = 0; // screen wipe progress

  if (nodrawers)
    return;

  // frame syncronous IO operations
  // in SDL locks screen if necessary
  I_StartFrame();


  // check for change of screen size (video mode)
  if (vid.setmodeneeded)
    vid.SetMode();  // change video mode, set setsizeneeded

  // change the view size if needed
  if (setsizeneeded)
    {
      R_ExecuteSetViewSize();
      force_wipe = true;
      borderdrawcount = 3;
    }

  // save the current screen if about to wipe
  if (force_wipe && cv_screenslink.value && rendermode == render_soft)
    {
      force_wipe = false;
      screenwipe = 1;
      wipe_StartScreen(0, 0, vid.width, vid.height); // "before", s0->s2
    }

  // draw buffered stuff to screen
  // BP: Used only by linux GGI version
  I_UpdateNoBlit();

  bool redrawsbar = false;

  // do buffered drawing
  switch (state)
    {
    case GS_LEVEL:
      if (game.tic)
        {
          hud.HU_Erase();
          if (screenwipe || rendermode != render_soft)
            redrawsbar = true;
        }

      // see if the border needs to be initially drawn
      if (oldgamestate != GS_LEVEL)
        R_FillBackScreen();    // draw the pattern into the back screen  ->s1

      // draw either automap or game
      if (automap.active)
	{
	  automap.Drawer();
	}
      else
        {
          // see if the border needs to be updated to the screen
          if (scaledviewwidth != vid.width)
            {
              // the menu may draw over parts out of the view window,
              // which are refreshed only when needed
              if (Menu::active)
                borderdrawcount = 3;

              if (borderdrawcount)
                {
                  R_DrawViewBorder(); // erase old menu stuff
                  borderdrawcount--;
                }
            }
          Drawer();
        }

      hud.Draw(redrawsbar); // draw hud on top anyway
      break;

    case GS_INTERMISSION:
      wi.Drawer();
      break;

    case GS_FINALE:
      F_Drawer();
      break;

    case GS_INTRO:
      D_PageDrawer(pagename);

    case GS_NULL:
    default:
      break;
    }

  // change gamma if needed
  if (state != oldgamestate && state != GS_LEVEL)
    vid.SetPalette(0);

  oldgamestate = state;


  // draw pause pic
  if (paused && !Menu::active)
    {
      int y;
      if (automap.active)
        y = 4;
      else
        y = viewwindowy + 4;
      Texture *tex = tc.GetPtr("M_PAUSE");
      tex->Draw(viewwindowx+(BASEVIDWIDTH - tex->width)/2, y, V_SCALE);
    }

  //FIXME: draw either console or menu, not the two. Menu wins.
  con.Drawer();
  Menu::Drawer(); // menu is drawn on top of everything else

  //NetUpdate();         // send out any new accumulation

  // normal update
  if (!screenwipe)
    {
      if (cv_netstat.value)
        {
          /*
          char s[50];
          sprintf(s,"get %d b/s",getbps);
          V_DrawString(BASEVIDWIDTH-V_StringWidth(s),165-40, V_WHITEMAP, s);
          */
        }

      //I_BeginProfile();
      I_FinishUpdate();              // page flip or blit buffer
      //CONS_Printf("last frame update took %d\n", I_EndProfile());
    }
  else if (screenwipe++ == 2) // we must wait until the "after" screen is rendered
    {
      // wipe update
      wipe_EndScreen(0, 0, vid.width, vid.height); // "after", s0->s3

      bool done;
      tic_t wipestart = I_GetTics() - 1;
      tic_t wipe_end  = wipestart + 2*TICRATE; // init a timeout
      do
        {
          tic_t nowtime, tics;

          do
            {
              I_Sleep(1);
              nowtime = I_GetTics();
              tics = nowtime - wipestart;
              // wait until time has passed
            }
          while (!tics);
          wipestart = nowtime;
          done = wipe_ScreenWipe(0, 0, vid.width, vid.height, tics);
          I_OsPolling();
          I_UpdateNoBlit();
          Menu::Drawer();            // menu is drawn even on top of wipes
          I_FinishUpdate();      // page flip or blit buffer

        }
      while (!done && I_GetTics() < wipe_end);
      screenwipe = 0;
    }
}




/// Renders the game view
void GameInfo::Drawer()
{
  // draw the player views

  int n = Consoleplayer.size();
  for (int i = 0; i < n; i++)
    {
      PlayerInfo *p = Consoleplayer[i];
      if (p->pov && p->mp && p->playerstate != PST_RESPAWN)
	R.R_RenderPlayerView(i, p);
    }

  //CONS_Printf("GI::Draw done\n");
}



//
// Distribute input events to the game subsystems
//
bool GameInfo::Responder(event_t* ev)
{
  // allow spy mode changes even during the demo
  if (state == GS_LEVEL && ev->type == ev_keydown
      && ev->data1 == KEY_F12 && !cv_hiddenplayers.value)
    {
      Consoleplayer[0]->connection->rpcRequestPOVchange_c2s(-1);
      return true;
    }

  // any other key pops up menu if in demos
  if (state == GS_DEMOPLAYBACK && action == ga_nothing && !singledemo)
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
      if (cht_Responder(ev))
	return true;
      if (hud.Responder(ev))
        return true;        // HUD ate the event
      if (automap.Responder(ev))
        return true;        // automap ate it
      break;

    case GS_INTERMISSION:
      if (wi.Responder(ev))
        return true;
      break;

    case GS_FINALE:
      if (F_Responder(ev))
        return true;        // finale ate the event
      break;

    default:
      break;
    }

  // update current controlkey state
  if (G_MapEventsToControls(ev))
    return true;

  // FIXME move these to Menu::Responder?
  switch (ev->type)
    {
    case ev_keydown:
      switch (ev->data1)
        {
        case KEY_PAUSE:
          COM_BufAddText("pause\n");
          return true;

        case '-':     // Screen size down
          cv_viewsize.Set(cv_viewsize.value - 1);
          S_StartLocalAmbSound(sfx_menu_adjust);
          return true;

        case '+':    // Screen size up
          cv_viewsize.Set(cv_viewsize.value + 1);
          S_StartLocalAmbSound(sfx_menu_adjust);
          return true;
        }

      return true;

    case ev_keyup:
      return false;   // always let key up events filter down

    case ev_mouse:
      return true;    // eat events

    default:
      break;
    }

  return false;
}




//==========================================================
//    Player manipulation
//==========================================================


/// returns player number 'num' if he is in the game, otherwise NULL
PlayerInfo *GameInfo::FindPlayer(int num)
{
  player_iter_t i = Players.find(num);
  if (i != Players.end())
    return (*i).second;

  return NULL;
}


/// returns player named 'name' or NULL
PlayerInfo *GameInfo::FindPlayer(const char *name)
{
  char *tail;
  int n = strtol(name, &tail, 0);

  if (tail != name)
    return FindPlayer(n); // by number

  for (player_iter_t i = Players.begin(); i != Players.end(); i++)
    if ((*i).second->name == name)
      return (*i).second;

  return NULL;
}



/// Tries to add a player into the game.
/// Returns NULL if unsuccesful.
PlayerInfo *GameInfo::AddPlayer(PlayerInfo *p)
{
  int n = Players.size();
  if (n >= maxplayers || !p)
    return NULL;  // no room in game or no player given

  // TODO what if maxplayers has recently been set to a lower-than-n value?
  // when are the extra players kicked? cv_maxplayers action func?

  // player numbers range from 1 to maxplayers
  int pnum = p->number;
  if (pnum > maxplayers)
    pnum = 0;  // pnum too high

  // check if pnum is free
  if (pnum > 0 && Players.count(pnum))
    pnum = 0; // pnum already taken

  // find first free player number, if necessary
  if (pnum <= 0)
    {
      for (int j = 1; j <= maxplayers; j++)
        if (Players.count(j) == 0)
          {
            pnum = j;
            break;
          }
    }

  // pnum is valid and free!
  p->number = pnum;
  Players[pnum] = p;

  return p;
}


/// Removes a player from game.
/// This and ClearPlayers are the ONLY ways a player should be removed.
bool GameInfo::RemovePlayer(int num)
{
  player_iter_t i = Players.find(num);

  if (i == Players.end())
    return false; // not found

  PlayerInfo *p = i->second;

  // remove avatar of player
  if (p->pawn)
    {
      p->pawn->player = NULL;
      p->pawn->Remove();
    }

  if (p->connection)
    ; // TODO send kick message
  else
    {
      // must be local
      vector<PlayerInfo *>::iterator j;
      for (j = Consoleplayer.begin(); j != Consoleplayer.end(); j++)
	if (*j == p)
	  {
	    Consoleplayer.erase(j);
	    break;
	  }
    }

  delete p;
  // NOTE! because PI's are deleted, even local PI's must be dynamically
  // allocated, using a copy constructor: new PlayerInfo(localplayer).
  Players.erase(i);
  return true;
}


/// Removes all players from a game.
void GameInfo::ClearPlayers()
{
  player_iter_t i;
  PlayerInfo *p;

  for (i = Players.begin(); i != Players.end(); i++)
    {
      p = (*i).second;
      // remove avatar of player
      if (p->pawn)
        {
          p->pawn->player = NULL;
          p->pawn->Remove();
        }
      delete p;
    }

  Players.clear();
  Consoleplayer.clear();
  hud.ST_Stop();
}
