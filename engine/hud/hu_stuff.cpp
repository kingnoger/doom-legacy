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
// Revision 1.14  2004/07/05 16:53:27  smite-meister
// Netcode replaced
//
// Revision 1.13  2004/04/25 16:26:50  smite-meister
// Doxygen
//
// Revision 1.11  2003/11/23 19:07:41  smite-meister
// New startup order
//
// Revision 1.10  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.9  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.8  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.7  2003/03/08 16:07:11  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.6  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.5  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.4  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.3  2002/12/16 22:12:10  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:20:08  smite-meister
// HUD rationalized
//
// Revision 1.1.1.1  2002/11/16 14:18:15  hurdler
// Initial C++ version of Doom Legacy
//-----------------------------------------------------------------------------

/// \file
/// \brief Heads Up Displays, cleaned up (hasta la vista hu_lib)

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "hu_stuff.h"

#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"
#include "g_input.h"

#include "m_random.h"
#include "i_video.h"

// Data.
#include "dstrings.h"
#include "r_local.h"
#include "wi_stuff.h"  // for drawrankings

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"


#ifdef HWRENDER
# include "hardware/hw_main.h"
#endif


HUD hud;



void ShowMessage_OnChange()
{
  if (!cv_showmessages.value)
    CONS_Printf("%s\n",MSGOFF);
  else
    CONS_Printf("%s\n",MSGON);
}

void ST_Overlay_OnChange()
{
  hud.CreateOverlayWidgets();
}


CV_PossibleValue_t crosshair_cons_t[]   ={{0,"Off"},{1,"Cross"},{2,"Angle"},{3,"Point"},{0,NULL}};
consvar_t cv_crosshair        = {"crosshair"   ,"0",CV_SAVE,crosshair_cons_t};
consvar_t cv_crosshair2       = {"crosshair2"  ,"0",CV_SAVE,crosshair_cons_t};
consvar_t cv_stbaroverlay     = {"overlay", "kahmf", CV_SAVE|CV_CALL, NULL, ST_Overlay_OnChange};
CV_PossibleValue_t showmessages_cons_t[]={{0,"Off"},{1,"On"},{2,"Not All"},{0,NULL}};
consvar_t cv_showmessages     = {"showmessages","1",CV_SAVE | CV_CALL | CV_NOINIT,showmessages_cons_t,ShowMessage_OnChange};
consvar_t cv_showmessages2    = {"showmessages2","1",CV_SAVE | CV_CALL | CV_NOINIT,showmessages_cons_t,ShowMessage_OnChange};



// coords are scaled
#define HU_INPUTX       0
#define HU_INPUTY       0


//-------------------------------------------
//              heads up font
//-------------------------------------------
bool                 chat_on;

static char             w_chat[HU_MAXMSGLEN];

static char             hu_tick;

//-------------------------------------------
//              misc vars
//-------------------------------------------

consvar_t*   chat_macros[10];

//added:16-02-98: crosshair 0=off, 1=cross, 2=angle, 3=point
static Texture* crosshair[HU_CROSSHAIRS]; // precached crosshair graphics

static Texture* PatchRankings;

// -------
// protos.
// -------
void   HU_drawDeathmatchRankings();
void   HU_drawCrosshair();
static void HU_DrawTip();


//======================================================================
//                          HEADS UP DISPLAY
//======================================================================

HUD::HUD()
{
  st_x = 0;
  st_y = BASEVIDHEIGHT - ST_HEIGHT_DOOM;
  stbarheight = ST_HEIGHT_DOOM;
  st_palette = 0;
  st_active = false;
  sbpawn = NULL;
};


void HUD::Startup()
{
  // client hud
  CV_RegisterVar(&cv_crosshair);
  CV_RegisterVar(&cv_crosshair2);
  CV_RegisterVar(&cv_showmessages);
  CV_RegisterVar(&cv_showmessages2);  
  CV_RegisterVar(&cv_stbaroverlay);

  // first initialization
  Init();
}


//-------------------------------------------------------------------
// was ST_Init, HU_Init
//  Initializes the HUD
//  sets the defaults border patch for the window borders.
void ST_LoadHexenData();
void ST_LoadHereticData();
void ST_LoadDoomData();

void HUD::Init()
{
  int startlump, endlump;
  int  i;

  // cache the HUD font for entire game execution
  // TODO add legacy default font (in legacy.wad)
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

  // NOTE! Heretic FONTAxx ends with FONTA59, HU_FONTSIZE and STCFNxx are longer!
  for (i=0; i <= endlump-startlump; i++) // endlump - startlump < HU_FONTSIZE
    font[i] = tc.GetPtrNum(i + startlump);
  // fill the rest with the first character
  for ( ; i < HU_FONTSIZE; i++)
    font[i] = tc.GetPtrNum(startlump);

  //----------- cache all legacy.wad stuff here

  startlump = fc.GetNumForName("CROSHAI1");
  for (i=0; i<HU_CROSSHAIRS; i++)
    crosshair[i] = tc.GetPtrNum(startlump + i);

  PatchRankings = tc.GetPtr("RANKINGS");

  //----------- legacy.wad stuff ends

  // Damn! sbo* icons are in pic_t format, not patch_t!
  // drawn using V_DrawScalePic()
  // using doom.wad or heretic.wad sprites instead...

  switch (game.mode)
    {
    case gm_hexen:
      ST_LoadHexenData();
      break;
    case gm_heretic:
      ST_LoadHereticData();
      break;
    case gm_doom1s:
    case gm_doom1:
    case gm_udoom:
    case gm_doom2:
      ST_LoadDoomData();
      break;
    default:
      break;
    }

  st_refresh = true;
}


char HU_dequeueChatChar();
void HU_queueChatChar(char c);
bool HU_keyInChatString(char *s, char ch);

//--------------------------------------
//  Returns true if key eaten
//
bool HUD::Responder(event_t *ev)
{
  bool  eatkey = false;

  if (ev->type != ev_keydown)
    return false;

  // only KeyDown events now...

  if (!chat_on)
    {
      // enter chat mode
      if (ev->data1 == gamecontrol[gc_talkkey][0]
	  || ev->data1 == gamecontrol[gc_talkkey][1])
        {
	  eatkey = chat_on = true;
	  w_chat[0] = 0;
	  HU_queueChatChar(HU_BROADCAST);
        }
    }
  else
    {
      char c = ev->data1;

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

	  char *macromessage = chat_macros[c]->str;

	  // kill last message with a '\n'
	  HU_queueChatChar(KEY_ENTER); // DEBUG!!!

	  // send the macro message
	  while (*macromessage)
	    HU_queueChatChar(*macromessage++);
	  HU_queueChatChar(KEY_ENTER);

	  // leave chat mode and notify that it was sent
	  chat_on = false;
	  eatkey = true;
        }
      else
        {
	  eatkey = HU_keyInChatString(w_chat,c);
	  if (eatkey)
            {
	      // static unsigned char buf[20]; // DEBUG
	      HU_queueChatChar(c);
	      
	      // sprintf(buf, "KEY: %d => %d", ev->data1, c);
	      //      plr->message = buf;
            }
	  if (c == KEY_ENTER)
            {
	      chat_on = false;
            }
	  else if (c == KEY_ESCAPE)
	    chat_on = false;
        }
    }

  if (eatkey)
    return true;

  // ST_ part
  if (ev->type == ev_keyup)
    {
      // Filter automap on/off : activates the statusbar while automap is active
      if( (ev->data1 & 0xffff0000) == AM_MSGHEADER )
	{
	  switch(ev->data1)
	    {
	    case AM_MSGENTERED:
	      st_refresh = true;        // force refresh of status bar
	      break;
	      
	    case AM_MSGEXITED:
	      break;
	    }
	}
    }
  return false;
}




//======================================================================
//                            EXECUTION
//======================================================================

//  Handles key input and string input
//
bool HU_keyInChatString(char *s, char ch)
{
  int l;

  if (ch >= ' ' && ch <= '_')
    {
      l = strlen(s);
      if (l<HU_MAXMSGLEN-1)
        {
	  s[l++]=ch;
	  s[l]=0;
	  return true;
        }
      return false;
    }
  else if (ch == KEY_BACKSPACE)
    {
      l = strlen(s);
      if (l)
	s[--l]=0;
      else
	return false;
    }
  else if (ch != KEY_ENTER)
    return false; // did not eat key

  return true; // ate the key
}



void HUD::Ticker()
{
  if (dedicated)
    return;

  //if (!st_active) return;

  hu_tick++;
  hu_tick &= 7; // currently only to blink chat input cursor

  if (damagecount > 100)
    damagecount = 100;  // teleport stomp does 10k points...
  else if (damagecount)
    damagecount--;

  if (bonuscount)
    bonuscount--;

  /*
  if ((game.mode == gm_heretic) && (gametic & 1))
    ChainWiggle = M_Random()&1;
  */  

  st_randomnumber = M_Random();

  // update widget data
  UpdateWidgets();

  // display message if necessary
  PlayerInfo *pl = consoleplayer;

  if (cv_showmessages.value && pl->message)
    {
      CONS_Printf ("%s\n", pl->message);
      pl->message = NULL;
    }

  // In splitscreen, display second player's messages
  if (cv_splitscreen.value)
    {
      pl = consoleplayer2;
      if (cv_showmessages.value && pl && pl->message)
	{
	  CONS_Printf ("\4%s\n", pl->message);
	  pl->message = NULL;
	}
    }

  // deathmatch rankings overlay if press key or while in death view
  if (cv_deathmatch.value)
    {
      if (gamekeydown[gamecontrol[gc_scores][0]] ||
	  gamekeydown[gamecontrol[gc_scores][1]] )
	drawscore = !chat_on;
      else
	drawscore = (pl->playerstate == PST_DEAD); // dead players see the frag roster
    }
  else
    drawscore = false;
}


#define QUEUESIZE               128

static char     chatchars[QUEUESIZE];
static int      head = 0;
static int      tail = 0;

//
//
char HU_dequeueChatChar()
{
    char c;

    if (head != tail)
    {
        c = chatchars[tail];
        tail = (tail + 1) & (QUEUESIZE-1);
    }
    else
    {
        c = 0;
    }

    return c;
}

//
//
void HU_queueChatChar(char c)
{
  if (((head + 1) & (QUEUESIZE-1)) == tail)
    {
      consoleplayer->message = HUSTR_MSGU;      //message not send
    }
    else
    {
        if (c == KEY_BACKSPACE)
        {
            if(tail!=head)
                head = (head - 1) & (QUEUESIZE-1);
        }
        else
        {
            chatchars[head] = c;
            head = (head + 1) & (QUEUESIZE-1);
        }
    }

    // send automaticly the message (no more chat char)
    if(c==KEY_ENTER)
    {
        char buf[255],c;
        int i=0;

        do {
            c=HU_dequeueChatChar();
            buf[i++]=c;
        } while(c);
        if(i>3)
            COM_BufInsertText (va("say %s",buf));
    }
}



//======================================================================
//                         HEADS UP DRAWING
//======================================================================

//  Draw chat input
//
static void HU_DrawChat()
{
  int c = 0;
  int i = 0;
  int y = HU_INPUTY;
  while (w_chat[i])
    {
      //Hurdler: isn't it better like that?
      V_DrawCharacter(HU_INPUTX + (c<<3), y, w_chat[i++] | 0x80);

      c++;
      if (c>=(vid.width>>3))
        {
	  c = 0;
	  y+=8;
        }
    }

  if (hu_tick<4)
    V_DrawCharacter(HU_INPUTX + (c<<3), y, '_' | 0x80);
}


//
//  Heads up displays drawer, call each frame
//
void HUD::Draw(bool redrawsbar)
{
  // draw chat string plus cursor
  if (chat_on)
    HU_DrawChat();

  // draw deathmatch rankings
  if (drawscore)
    HU_drawDeathmatchRankings();

  // draw the crosshair, not with chasecam
  if (!automap.active && cv_crosshair.value && !cv_chasecam.value)
    HU_drawCrosshair ();

  HU_DrawTip();
  HU_DrawFSPics();

  ST_Drawer(redrawsbar);
}

//======================================================================
//                          PLAYER TIPS
//======================================================================
#define MAXTIPLINES 20
char    *tiplines[MAXTIPLINES];
int     numtiplines = 0;
int     tiptime = 0;
int     largestline = 0;



void HU_SetTip(char *tip, int displaytics)
{
  int    i;
  char   *rover, *ctipline, *ctipline_p;


  for(i = 0; i < numtiplines; i++)
    Z_Free(tiplines[i]);


  numtiplines = 0;

  rover = tip;
  ctipline = ctipline_p = (char *)Z_Malloc(128, PU_STATIC, NULL);
  *ctipline = 0;
  largestline = 0;

  while(*rover)
  {
    if(*rover == '\n' || strlen(ctipline) + 2 >= 128 || V_StringWidth(ctipline) + 16 >= BASEVIDWIDTH)
    {
      if(numtiplines > MAXTIPLINES)
        break;
      if(V_StringWidth(ctipline) > largestline)
        largestline = V_StringWidth(ctipline);

      tiplines[numtiplines] = ctipline;
      ctipline = ctipline_p = (char *)Z_Malloc(128, PU_STATIC, NULL);
      *ctipline = 0;
      numtiplines ++;
    }
    else
    {
      *ctipline_p = *rover;
      ctipline_p++;
      *ctipline_p = 0;
    }
    rover++;

    if(!*rover)
    {
      if(V_StringWidth(ctipline) > largestline)
        largestline = V_StringWidth(ctipline);
      tiplines[numtiplines] = ctipline;
      numtiplines ++;
    }
  }

  tiptime = displaytics;
}




static void HU_DrawTip()
{
  int    i;
  if(!numtiplines) return;
  if(!tiptime)
  {
    for(i = 0; i < numtiplines; i++)
      Z_Free(tiplines[i]);
    numtiplines = 0;
    return;
  }
  tiptime--;


  for(i = 0; i < numtiplines; i++)
  {
    V_DrawString((BASEVIDWIDTH - largestline) / 2,
                 ((BASEVIDHEIGHT - (numtiplines * 8)) / 2) + ((i + 1) * 8),
                 0,
                 tiplines[i]);
  }
}


void HU_ClearTips()
{
  int    i;

  for(i = 0; i < numtiplines; i++)
    Z_Free(tiplines[i]);
  numtiplines = 0;

  tiptime = 0;
}


//======================================================================
//                           FS HUD Grapics!
//======================================================================
typedef struct
{
  int       lumpnum;
  int       xpos;
  int       ypos;
  Texture   *data;
  bool   draw;
} fspic_t;

fspic_t*   piclist = NULL;
int        maxpicsize = 0;


//
// HU_InitFSPics
// This function is called when Doom starts and every time the piclist needs
// to be expanded.
void HU_InitFSPics()
{
  int  newstart, newend, i;

  if(!maxpicsize)
  {
    newstart = 0;
    newend = maxpicsize = 128;
  }
  else
  {
    newstart = maxpicsize;
    newend = maxpicsize = (maxpicsize * 2);
  }

  piclist = (fspic_t *)realloc(piclist, sizeof(fspic_t) * maxpicsize);
  for(i = newstart; i < newend; i++)
  {
    piclist[i].lumpnum = -1;
    piclist[i].data = NULL;
  }
}


int  HU_GetFSPic(int lumpnum, int xpos, int ypos)
{
  int      i;

  if(!maxpicsize)
    HU_InitFSPics();

  getpic:
  for(i = 0; i < maxpicsize; i++)
  {
    if(piclist[i].lumpnum != -1)
      continue;

    piclist[i].lumpnum = lumpnum;
    piclist[i].xpos = xpos;
    piclist[i].ypos = ypos;
    piclist[i].draw = false;
    return i;
  }

  HU_InitFSPics();
  goto getpic;
}


int   HU_DeleteFSPic(int handle)
{
  if(handle < 0 || handle > maxpicsize)
    return -1;

  piclist[handle].lumpnum = -1;
  piclist[handle].data = NULL;
  return 0;
}


int   HU_ModifyFSPic(int handle, int lumpnum, int xpos, int ypos)
{
  if(handle < 0 || handle > maxpicsize)
    return -1;

  if(piclist[handle].lumpnum == -1)
    return -1;

  piclist[handle].lumpnum = lumpnum;
  piclist[handle].xpos = xpos;
  piclist[handle].ypos = ypos;
  piclist[handle].data = NULL;
  return 0;
}


int   HU_FSDisplay(int handle, bool newval)
{
  if(handle < 0 || handle > maxpicsize)
    return -1;
  if(piclist[handle].lumpnum == -1)
    return -1;

  piclist[handle].draw = newval;
  return 0;
}


void HU_DrawFSPics()
{
  int       i;

  for(i = 0; i < maxpicsize; i++)
  {
    if(piclist[i].lumpnum == -1 || piclist[i].draw == false)
      continue;
    if(piclist[i].xpos >= vid.width || piclist[i].ypos >= vid.height)
      continue;

    if(!piclist[i].data)
      piclist[i].data = tc.GetPtrNum(piclist[i].lumpnum);

    if((piclist[i].xpos + piclist[i].data->width) < 0 || (piclist[i].ypos + piclist[i].data->height) < 0)
      continue;

    piclist[i].data->Draw(piclist[i].xpos, piclist[i].ypos, V_SCALE);
  }
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
static int     oldclearlines;

void HU_Erase()
{
    int topline;
    int bottomline;
    int y,yoffset;

    //faB: clear hud msgs on double buffer (Glide mode)
    bool secondframe;
    static  int     secondframelines;

    if (con_clearlines==oldclearlines && !con_hudupdate && !chat_on)
        return;

    // clear the other frame in double-buffer modes
    secondframe = (con_clearlines!=oldclearlines);
    if (secondframe)
        secondframelines = oldclearlines;

    // clear the message lines that go away, so use _oldclearlines_
    bottomline = oldclearlines;
    oldclearlines = con_clearlines;
    if( chat_on )
        if( bottomline < 8 )
            bottomline=8;

    if (automap.active || viewwindowx==0)   // hud msgs don't need to be cleared
        return;

    // software mode copies view border pattern & beveled edges from the backbuffer
    if (rendermode==render_soft)
    {
        topline = 0;
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
#ifdef HWRENDER 
    else {
        // refresh just what is needed from the view borders
        HWR_DrawViewBorder (secondframelines);
        con_hudupdate = secondframe;
    }
#endif
}



//======================================================================
//                   IN-LEVEL DEATHMATCH RANKINGS
//======================================================================

// count frags for each team
/*
int HU_CreateTeamFragTbl(fragsort_t *fragtab,int dmtotals[],int fragtbl[MAXPLAYERS][MAXPLAYERS])
{
  int i,j,k,scorelines,team;

  scorelines = 0;
  for (i=0; i<MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
	  if(cv_teamplay.value==1)
	    team=players[i].skincolor;
	  else
	    team=players[i].skin;

	  for(j=0;j<scorelines;j++)
	    if (fragtab[j].num == team)
	      { // found there team
		if(fragtbl)
		  {
		    for(k=0;k<MAXPLAYERS;k++)
		      if(playeringame[k])
			{
			  if(cv_teamplay.value==1)
			    fragtbl[team][players[k].skincolor] +=
			      players[i].frags[k];
			  else
			    fragtbl[team][players[k].skin] +=
			      players[i].frags[k];
			}
		  }

		fragtab[j].count += ST_PlayerFrags(i);
		if(dmtotals)
		  dmtotals[team]=fragtab[j].count;
		break;
	      }
	  if (j==scorelines)
            {   // team not found add it

	      if(fragtbl)
		for(k=0;k<MAXPLAYERS;k++)
		  fragtbl[team][k] = 0;

	      fragtab[scorelines].count = ST_PlayerFrags(i);
	      fragtab[scorelines].num   = team;
	      fragtab[scorelines].color = players[i].skincolor;
	      fragtab[scorelines].name  = team_names[team];

	      if(fragtbl)
                {
		  for(k=0;k<MAXPLAYERS;k++)
		    if(playeringame[k])
		      {
			if(cv_teamplay.value==1)
			  fragtbl[team][players[k].skincolor] +=
			    players[i].frags[k];
			else
			  fragtbl[team][players[k].skin] +=
			    players[i].frags[k];
		      }
                }

	      if(dmtotals)
		dmtotals[team]=fragtab[scorelines].count;

	      scorelines++;
            }
        }
    }
  return scorelines;
}
*/


//
//  draw Deathmatch Rankings
//
void HU_drawDeathmatchRankings()
{
  fragsort_t  *fragtab;
  int          scorelines;

  // draw the ranking title panel
  PatchRankings->Draw((BASEVIDWIDTH - PatchRankings->width)/2, 5, V_SCALE);

  scorelines = game.GetFrags(&fragtab, 0);

  //Fab:25-04-98: when you play, you quickly see your frags because your
  //  name is displayed white, when playback demo, you quicly see who's the
  //  view.
  PlayerInfo *whiteplayer = (game.state == GameInfo::GS_DEMOPLAYBACK) ? displayplayer : consoleplayer;
  
  if (scorelines>9)
    scorelines = 9; //dont draw past bottom of screen, show the best only

  if (cv_teamplay.value == 0)
    WI_drawRanking(NULL,80,70,fragtab,scorelines,true, whiteplayer->number);
  else
    {
      // draw the frag to the right
      //        WI_drawRanking("Individual",170,70,fragtab,scorelines,true,whiteplayer);

      // and the team frag to the left
      WI_drawRanking("Teams",80,70,fragtab,scorelines,true, whiteplayer->team);
    }
  delete [] fragtab;
}


// draw the Crosshair, at the exact center of the view.

#ifdef HWRENDER
extern float gr_basewindowcentery;
extern float gr_viewheight;
#endif

void HU_drawCrosshair()
{
  int i, y;

  i = cv_crosshair.value & 3;
  if (!i)
    return;

#ifdef HWRENDER
  if (rendermode != render_soft) 
    y = int(gr_basewindowcentery);
  else
#endif
    y = viewwindowy+(viewheight>>1);

  crosshair[i-1]->Draw(vid.width >> 1, y, V_TL | V_SSIZE);

  if (cv_splitscreen.value)
    {
#ifdef HWRENDER
      if ( rendermode != render_soft )
	y += int(gr_viewheight);
      else
#endif
	y += viewheight;

      crosshair[i-1]->Draw(vid.width >> 1, y, V_TL | V_SSIZE);
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
    CV_RegisterVar(chat_macros[i]);
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
  CV_Set(chat_macros[i], COM_Argv(2));
}
