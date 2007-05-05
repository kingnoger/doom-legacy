// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Heads Up Display (everything that is rendered on the 3D view)

#include <ctype.h>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "d_event.h"
#include "hud.h"

#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_input.h"

#include "m_random.h"
#include "keys.h"

#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "i_video.h"
#include "v_video.h"
#include "screen.h"

#include "dstrings.h"
#include "w_wad.h"

#include "am_map.h"
#include "d_main.h"

#ifndef NO_OPENGL
# include "hardware/hwr_render.h"
#include "hardware/oglrenderer.hpp"
#endif


//======================================================================
//        HUD-related consvars
//======================================================================

void ST_Overlay_OnChange()
{
  hud.CreateOverlayWidgets();
}


consvar_t cv_stbaroverlay     = {"hud_overlay", "kahmf", CV_SAVE|CV_CALL, NULL, ST_Overlay_OnChange};
consvar_t *chat_macros[10];


//======================================================================
//                          HEADS UP DISPLAY
//======================================================================

HUD hud;

class HudTip
{
  string tip;
public:
  int lines;
  int time;

public:
  HudTip(string &tip, int time);
  void Draw(int x, int y);
};

static char hu_tick;


#define HU_CROSSHAIRS 3
Material* crosshair[HU_CROSSHAIRS]; // precached crosshair graphics
static Material* PatchRankings;


HUD::HUD()
{
  st_x = 0;
  st_y = BASEVIDHEIGHT;
  stbarheight = 0;
  st_palette = 0;
  st_active = false;
  st_player = NULL;
};


void HUD::Startup()
{
  // client hud
  cv_stbaroverlay.Reg();

  // first initialization
  Init();
}


void ST_LoadHexenData();
void ST_LoadHereticData();
void ST_LoadDoomData();

//  Initializes the HUD
//  sets the defaults border patch for the window borders.
void HUD::Init()
{
  int startlump, endlump;
  // TODO add legacy default font (in legacy.wad)

  // "hud font"
  switch (game.mode)
    {
    case gm_heretic:
    case gm_hexen:
      startlump = fc.GetNumForName("FONTA01");
      endlump  =  fc.GetNumForName("FONTA59");
      break;
    default:
      startlump = fc.GetNumForName("STCFN033");
      endlump =   fc.GetNumForName("STCFN095");
    }

  hud_font = new font_t(startlump, endlump);

  // "big font"
  if (fc.FindNumForName("FONTB_S") < 0)
    big_font = NULL;
  else
    {
      startlump = fc.FindNumForName("FONTB01");
      endlump   = fc.GetNumForName("FONTB58");
      big_font = new font_t(startlump, endlump);
    }

  //----------- cache all legacy.wad stuff here

  startlump = fc.GetNumForName("CROSHAI1");
  for (int i=0; i<HU_CROSSHAIRS; i++)
    crosshair[i] = materials.GetLumpnum(startlump + i);

  PatchRankings = materials.Get("RANKINGS");

  //----------- legacy.wad stuff ends

  switch (game.mode)
    {
    case gm_hexen:
      ST_LoadHexenData();
      break;
    case gm_heretic:
      ST_LoadHereticData();
      break;
    case gm_doom1:
    case gm_doom2:
      ST_LoadDoomData();
      break;
    default:
      break;
    }

  st_refresh = true;
}



bool HUD::Responder(event_t *ev)
{
  bool eatkey = false;

  if (ev->type != ev_keydown)
    return false;

  // only KeyDown events now...

  if (!chat_on)
    {
      // enter chat mode
      if (ev->data1 == gk_talk)
        {
          chat_on = true;
	  return true;
        }
    }
  else
    {
      int c = ev->data1;

      // use console translations for chat
      if (shiftdown)
        c = shiftxform[c];
      else if (con_keymap != la_english)
        c = KeyTranslation(c);

      // send a macro
      if (altdown)
        {
          c = c - '0';
          if (c > 9)
            return false;

          // kill (and send) the last message
	  SendChat();

          // send the macro message
	  chat_msg = chat_macros[c]->str;
	  SendChat();
	  return true;
        }
      else
        {
	  if (c >= ' ' && c <= '_')
	    {
	      if (chat_msg.length() < 80)
		chat_msg.push_back(c);
	      return true;
	    }
	  else if (c == KEY_BACKSPACE)
	    {
	      if (!chat_msg.empty())
		chat_msg.erase(--chat_msg.end());
	      return true;
	    }
          else if (c == KEY_ENTER)
            {
	      SendChat();
	      return true;
            }
          else if (c == KEY_ESCAPE)
	    {
	      chat_on = false;
	      return true;
	    }
        }
    }

  return eatkey;
}



void HUD::Ticker()
{
  hu_tick++;
  hu_tick &= 7; // currently only to blink chat input cursor

  /*
  if ((game.mode == gm_heretic) && (gametic & 1))
    ChainWiggle = M_Random()&1;
  */

  int n = ViewPlayers.size();
  for (int i=0; i<n; i++)
    {
      PlayerInfo *pl = ViewPlayers[i];

      if (pl->palette >= 0)
	{
	  vid.SetPalette(pl->palette);
	  pl->palette = -1;
	  damagecount = bonuscount = 0;
	}
      else
	{
	  damagecount += pl->damagecount;
	  bonuscount += pl->bonuscount;
	  pl->damagecount = pl->bonuscount = 0;
	}
      
      if (pl->itemuse)
	{
	  itemuse = 4; // number of animation frames
	  pl->itemuse = false;
	}

      // display player messages
      while (!pl->messages.empty())
	{
	  PlayerInfo::message_t &m = pl->messages.front();
	  // TODO message priorities: a message blocks lower-priority messages for n seconds

	  switch (m.type)
	    {
	    case PlayerInfo::M_CONSOLE:
	      CONS_Printf("%s\n", m.msg.c_str());
	      break;
	    case PlayerInfo::M_HUD:
	      tips.push_back(new HudTip(m.msg, 150));
	      break;
	    }

	  pl->messages.pop_front();
	}

      // dead players see the frag roster
      if (cv_deathmatch.value)
	drawscore = (!pl->pawn || (pl->pawn->flags & MF_CORPSE));
    }

  // update widget data
  UpdateWidgets();

  if (damagecount > 100)
    damagecount = 100;  // teleport stomp does 10k points...
  else if (damagecount > 0)
    damagecount--;

  if (bonuscount > 0)
    bonuscount--;

  // deathmatch rankings overlay if press key or while in death view
  if (cv_deathmatch.value)
    {
      extern bool gamekeydown[NUMINPUTS];
      if (gamekeydown[commoncontrols[gk_scores][0]] ||
	  gamekeydown[commoncontrols[gk_scores][1]])
	drawscore = !chat_on;
    }
  else
    drawscore = false;

}


//======================================================================
//                             CHAT
//======================================================================

void HUD::SendChat()
{
  // send the message, get out of chat mode
  if (chat_msg.length() > 3)
    COM_BufInsertText(va("say %s", chat_msg.c_str()));

  chat_msg.clear();
  chat_on = false;
}


//======================================================================
//                         HEADS UP DRAWING
//======================================================================


// Draw a column of rankings stored in fragtable
//  Quick-patch for the Cave party 19-04-1998 !!
void HU_DrawRanking(const char *title, int x, int y, fragsort_t *fragtable, int scorelines, bool large, int white)
{
  int colornum;

  if (game.mode == gm_heretic)
    colornum = 230;
  else
    colornum = 0x78;

  if (title)
    hud_font->DrawString(x, y-14, title, V_SCALE);

  // draw rankings
  for (int i=0; i<scorelines; i++)
    {
      // draw color background
      int color = translationtables[fragtable[i].color][colornum];
      V_DrawFill(x-1, y-1, large ? 40 : 26, 9, color);

      // draw frags count
      char num[12];
      sprintf(num, "%3i", fragtable[i].count);
      hud_font->DrawString(x+(large ? 32 : 24) - hud_font->StringWidth(num), y, num, V_SCALE);

      // draw name
      hud_font->DrawString(x+(large ? 64 : 29), y, fragtable[i].name,
			   ((fragtable[i].num == white) ? V_WHITEMAP : 0) | V_SCALE);

      y += 12;
      if (y >= BASEVIDHEIGHT)
        break;            // dont draw past bottom of screen
    }
}


//  draw Deathmatch Rankings
void HU_drawDeathmatchRankings()
{
  fragsort_t  *fragtab;

  // draw the ranking title panel
  PatchRankings->Draw((BASEVIDWIDTH - PatchRankings->worldwidth)/2, 5, V_SCALE);

  // TODO alloc fragtab here...
  int scorelines = game.GetFrags(&fragtab, 0);

  //Fab:25-04-98: when you play, you quickly see your frags because your
  //  name is displayed white, when playback demo, you quicly see who's the
  //  view.
  PlayerInfo *whiteplayer = ViewPlayers[0];

  if (scorelines > 9)
    scorelines = 9; //dont draw past bottom of screen, show the best only

  if (cv_teamplay.value == 0)
    HU_DrawRanking(NULL, 80, 70, fragtab, scorelines, true, whiteplayer->number);
  else
    {
      // draw the frag to the right
      //        HU_drawRanking("Individual",170,70,fragtab,scorelines,true,whiteplayer);

      // and the team frag to the left
      HU_DrawRanking("Teams", 80, 70, fragtab, scorelines, true, whiteplayer->team);
    }
  delete [] fragtab;
}


//
//  Heads up display drawer, called each frame
//
void HUD::Draw(bool redrawsbar)
{
#define HU_INPUTX 0
#define HU_INPUTY 0
  // draw chat string plus cursor
  if (chat_on)
    {
      hud_font->DrawString(HU_INPUTX, HU_INPUTY, chat_msg.c_str(), V_SCALE | V_WHITEMAP);
      int cx = HU_INPUTX + hud_font->StringWidth(chat_msg.c_str());
      if (hu_tick < 4)
	hud_font->DrawCharacter(cx, HU_INPUTY, '_' | 0x80, V_SCALE);
    }

  // draw deathmatch rankings
  if (drawscore)
    HU_drawDeathmatchRankings();

  // draw the crosshair, not with chasecam
  if (rendermode == render_soft &&
      !automap.active)
    {
      // (cv_splitscreen.value) FIXME viewports!
      int y = viewwindowy + (viewheight>>1);
      for (int i=0; i<2; i++)
	{
	  if (LocalPlayers[i].crosshair) // && !LocalPlayers[i].chasecam)
	    {
	      int c = LocalPlayers[i].crosshair & 3;
	      crosshair[c-1]->Draw(vid.width >> 1, y, V_TL | V_SSIZE);
	    }
	  y += viewheight;
	}
    }

  DrawTips();
  DrawPics();

  ST_Drawer(redrawsbar);
}



//======================================================================
//                          PLAYER TIPS
//======================================================================


HudTip::HudTip(string &text, int tiptime)
{
  //x = (BASEVIDWIDTH - largestline) / 2
  //y = ((BASEVIDHEIGHT - (numlines * 8)) / 2

  tip = text;
  const char *t = tip.c_str();
  time = tiptime;
  lines = 1;
  int n = tip.length();
  int last_space = -1;
  int cline = 0;

  // TODO write a better word wrap algorithm (using newlines)
  // crappy word wrap
  for (int i=0; i<n; i++)
    {
      if (tip[i] == '\n')
	{
	  lines++;
	  cline = i + 1;
	  last_space = -1;
	  continue;
	}

      if (isspace(tip[i]))
	last_space = i;

      int clen = i - cline;
      if (clen >= 128 || hud_font->StringWidth(&t[cline], clen) + 16 >= BASEVIDWIDTH)
	{
	  if (last_space == -1 || i - last_space > 32)
	    {
	      tip[i] = '\n'; // mangle really long words...
	      i--;
	    }
	  else
	    {
	      tip[last_space] = '\n';
	      i = last_space - 1;
	    }
	  continue;
	}
    }
}



void HudTip::Draw(int x, int y)
{
  time--;
  hud_font->DrawString(x, y, tip.c_str(), V_SCALE);
}



void HUD::DrawTips()
{
  int cy = 32; // cursor y
  list<HudTip *>::iterator i, j;
  for (i = tips.begin(); i != tips.end(); )
    {
      j = i; // because erase invalidates the iterator, we must use a copy
      HudTip *h = *i;
      i++;

      if (h->time > 0)
	{
	  h->Draw(8, cy);
	  cy += h->lines * 8;
	}
      else
	{
	  delete h;
	  tips.erase(j);
	}
    }
}



//======================================================================
//                           FS HUD Grapics!
//======================================================================

class HudPic
{
protected:
  int      xpos, ypos;
  Material *data;

public:
  bool     draw;

public:
  void Set(int lump, int x, int y)
  {
    xpos = x;
    ypos = y;
    data = materials.GetLumpnum(lump);
    draw = true;
  };

  void Draw()
  {
    if (!draw)
      return;
    if (xpos >= vid.width || ypos >= vid.height)
      return;
    if ((xpos + data->worldwidth) < 0 || (ypos + data->worldheight) < 0)
      return;

    data->Draw(xpos, ypos, V_SCALE);
  };
};


// creates a new HUD picture, returns a handle
int HUD::GetFSPic(int lumpnum, int x, int y)
{
  int n = pics.size();

  // find the first free slot
  for (int i = 0; i < n; i++)
  {
    if (pics[i])
      continue;

    pics[i] = new HudPic;
    pics[i]->Set(lumpnum, x, y);
    return i;
  }

  // no free slots, add a new one
  pics.push_back(new HudPic);
  pics[n]->Set(lumpnum, x, y);
  return n;
}


// deletes a HUD picture
// returns true if there was some problem
bool HUD::DeleteFSPic(int handle)
{
  if (handle < 0 || handle >= int(pics.size()) || pics[handle] == NULL)
    return true;

  delete pics[handle];
  pics[handle] = NULL;
  return false;
}


// modifies an existing HUD picture
bool HUD::ModifyFSPic(int handle, int lump, int x, int y)
{
  if (handle < 0 || handle >= int(pics.size()) || pics[handle] == NULL)
    return true;

  pics[handle]->Set(lump, x, y);
  return false;
}


bool HUD::DisplayFSPic(int handle, bool newval)
{
  if (handle < 0 || handle >= int(pics.size()) || pics[handle] == NULL)
    return true;

  pics[handle]->draw = newval;
  return false;
}


void HUD::DrawPics()
{
  int n = pics.size();
  for (int i = 0; i < n; i++)
    if (pics[i])
      pics[i]->Draw();
      
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

//  Clear old messages from the borders around the view window
//  (only for reduced view, refresh the borders when needed)
//
//  startline  : y coord to start clear,
//  clearlines : how many lines to clear.
//

void HUD::HU_Erase()
{
  static int oldclearlines;
  int y,yoffset;

  if (con_clearlines==oldclearlines && !con_hudupdate && !chat_on)
    return;

  // clear the other frame in double-buffer modes
  bool secondframe = (con_clearlines != oldclearlines);

  // clear the message lines that go away, so use _oldclearlines_
  int bottomline = oldclearlines;
  oldclearlines = con_clearlines;
  if (chat_on && bottomline < 8)
    bottomline = 8;

  if (automap.active || viewwindowx==0)   // hud msgs don't need to be cleared
    return;

#ifndef NO_OPENGL
  if (rendermode!=render_soft)
    {
      // refresh just what is needed from the view borders
      HWR.DrawViewBorder();
      con_hudupdate = secondframe;
    }
  else
#endif
    { // software mode copies view border pattern & beveled edges from the backbuffer
      int topline = 0;
      for (y=topline,yoffset=y*vid.width; y<bottomline ; y++,yoffset+=vid.width)
        {
	  if (y < viewwindowy || y >= viewwindowy + viewheight)
	    R_VideoErase(yoffset, vid.width); // erase entire line
	  else
            {
	      R_VideoErase(yoffset, viewwindowx); // erase left border
	      // erase right border
	      R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
            }
        }
      con_hudupdate = false;      // if it was set..
    }
}


//======================================================================
//                    CHAT MACROS COMMAND & VARS
//======================================================================

// better do HackChatmacros() because the strings are NULL !!

consvar_t cv_chatmacro1 = {"_chatmacro1", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro2 = {"_chatmacro2", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro3 = {"_chatmacro3", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro4 = {"_chatmacro4", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro5 = {"_chatmacro5", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro6 = {"_chatmacro6", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro7 = {"_chatmacro7", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro8 = {"_chatmacro8", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro9 = {"_chatmacro9", NULL, CV_SAVE,NULL};
consvar_t cv_chatmacro0 = {"_chatmacro0", NULL, CV_SAVE,NULL};


// set the chatmacros original text, before config is executed
// if a dehacked patch was loaded, it will set the hacked texts,
// but the config.cfg will override it.
//
void HU_HackChatmacros()
{
  // this is either the original text, or dehacked ones
  cv_chatmacro0.defaultvalue = HUSTR_CHATMACRO0;
  cv_chatmacro1.defaultvalue = HUSTR_CHATMACRO1;
  cv_chatmacro2.defaultvalue = HUSTR_CHATMACRO2;
  cv_chatmacro3.defaultvalue = HUSTR_CHATMACRO3;
  cv_chatmacro4.defaultvalue = HUSTR_CHATMACRO4;
  cv_chatmacro5.defaultvalue = HUSTR_CHATMACRO5;
  cv_chatmacro6.defaultvalue = HUSTR_CHATMACRO6;
  cv_chatmacro7.defaultvalue = HUSTR_CHATMACRO7;
  cv_chatmacro8.defaultvalue = HUSTR_CHATMACRO8;
  cv_chatmacro9.defaultvalue = HUSTR_CHATMACRO9;

  // link chatmacros to cvars
  chat_macros[0] = &cv_chatmacro0;
  chat_macros[1] = &cv_chatmacro1;
  chat_macros[2] = &cv_chatmacro2;
  chat_macros[3] = &cv_chatmacro3;
  chat_macros[4] = &cv_chatmacro4;
  chat_macros[5] = &cv_chatmacro5;
  chat_macros[6] = &cv_chatmacro6;
  chat_macros[7] = &cv_chatmacro7;
  chat_macros[8] = &cv_chatmacro8;
  chat_macros[9] = &cv_chatmacro9;

  // register chatmacro vars ready for config.cfg
  for (int i=0;i<10;i++)
    chat_macros[i]->Reg();
}


//  chatmacro <0-9> "chat message"
//
void Command_Chatmacro_f()
{
  if (COM_Argc()<2)
    {
      CONS_Printf("chatmacro <0-9> : view chatmacro\n"
                  "chatmacro <0-9> \"chat message\" : change chatmacro\n");
      return;
    }

  int i = atoi(COM_Argv(1)) % 10;

  if (COM_Argc() == 2)
    {
      CONS_Printf("chatmacro %d is \"%s\"\n",i,chat_macros[i]->str);
      return;
    }

  // change a chatmacro
  chat_macros[i]->Set(COM_Argv(2));
}
