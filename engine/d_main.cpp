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
// Revision 1.25  2004/03/28 15:16:12  smite-meister
// Texture cache.
//
// Revision 1.24  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.23  2004/01/02 14:25:01  smite-meister
// cleanup
//
// Revision 1.22  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.21  2003/11/23 19:07:41  smite-meister
// New startup order
//
// Revision 1.20  2003/11/12 11:07:16  smite-meister
// Serialization done. Map progression.
//
// Revision 1.19  2003/06/10 22:39:53  smite-meister
// Bugfixes
//
// Revision 1.18  2003/05/30 13:34:42  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.17  2003/05/11 21:23:49  smite-meister
// Hexen fixes
//
// Revision 1.16  2003/05/05 00:24:48  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.15  2003/04/19 17:38:46  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.14  2003/04/05 12:20:00  smite-meister
// Makefiles fixed
//
// Revision 1.13  2003/04/04 00:01:52  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.12  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.11  2003/03/08 16:06:59  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.10  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.9  2003/02/16 16:54:50  smite-meister
// L2 sound cache done
//
// Revision 1.8  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.7  2003/01/18 20:17:41  smite-meister
// HUD fixed, levelchange crash fixed.
//
// Revision 1.6  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.5  2002/12/29 18:57:02  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.4  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.3  2002/12/16 22:10:53  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:11:39  smite-meister
// Blindness and missile clipping bugs fixed
//
// Revision 1.47  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.45  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.36  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.32  2000/11/02 17:50:06  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.23  2000/08/10 14:50:19  ydario
// OS/2 port
//
// Revision 1.15  2000/04/19 15:21:02  hurdler
// add SDL midi support
//
// Revision 1.10  2000/04/05 15:47:46  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
//
// Revision 1.9  2000/04/04 00:32:45  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
//
// DESCRIPTION:
//      DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//      plus functions to determine game mode (shareware, registered),
//      parse command line parameters, configure game parameters,
//      and call the startup functions.
//
//-----------------------------------------------------------------------------

#include <unistd.h>
#include <sys/stat.h>

#include "console.h"
#include "d_clisrv.h"

#include "am_map.h"
#include "d_net.h"
#include "dstrings.h"

#include "f_wipe.h"
#include "f_finale.h"

#include "g_game.h"
#include "g_player.h"
#include "g_pawn.h"

#include "hu_stuff.h"

#include "i_sound.h"
#include "i_system.h"
#include "i_video.h"

#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h" // configfile

#include "r_local.h"

#include "sounds.h"
#include "s_sound.h"
#include "t_script.h"
#include "v_video.h"

#include "wi_stuff.h"
#include "w_wad.h"

#include "z_zone.h"
#include "d_main.h"
#include "p_heretic.h"


#ifdef HWRENDER
# include "hardware/hw_main.h"   // 3D View Rendering
#endif



// Version information
const int  VERSION = 150;
const char VERSIONSTRING[] = "prealpha2";

// Name of local directory for config files and savegames
#ifdef LINUX 
# define DEFAULTDIR ".legacy"
#else
# define DEFAULTDIR "legacy"
#endif

// the file where all game vars and settings are saved
#define CONFIGFILENAME   "config.cfg"  


bool dedicated;
bool devparm;        // started game with -devparm
bool singletics = false; // timedemo

bool nodrawers;    // for comparative timing purposes
bool noblit;       // for comparative timing purposes



// "Mission packs". Used only for Evilution and Plutonia.
enum gamemission_t
{
  gmi_doom2 = 0,  // DOOM 2, default
  gmi_tnt,    // TNT Evilution mission pack
  gmi_plut    // Plutonia Experiment pack
};

int mission = gmi_doom2;



//======================================================================
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
// referenced from i_system.c for I_GetKey()
//======================================================================

event_t  events[MAXEVENTS];
int      eventhead;
int      eventtail;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent(const event_t* ev)
{
  events[eventhead] = *ev;
  eventhead = (++eventhead)&(MAXEVENTS-1);
}
// just for lock this function
#ifdef PC_DOS
void D_PostEvent_end() {};
#endif


//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents()
{
  event_t*    ev;

  for ( ; eventtail != eventhead ; eventtail = (++eventtail)&(MAXEVENTS-1) )
    {
      ev = &events[eventtail];
      // Menu input
      if (Menu::Responder(ev))
	continue;              // menu ate the event
      // console input
      if (CON_Responder(ev))
	continue;              // ate the event
      game.Responder(ev);
    }
}



//================
//  DEMO LOOP
//================


int   demosequence;
bool  advancedemo;
static int   pagetic;
static char *pagename = "TITLEPIC";

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo()
{
  advancedemo = true;
}


//
// This cycles through the demo sequences.
//
void D_DoAdvanceDemo()
{
  advancedemo = false;

  if (game.mode == gm_udoom)
    demosequence = (demosequence+1)%7;
  else
    demosequence = (demosequence+1)%6;

  switch (demosequence)
    {
    case 0:
      pagename = "TITLEPIC";
      switch (game.mode)
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
      game.state = GS_DEMOSCREEN;
      break;
    case 1:
      G_DeferedPlayDemo("DEMO1");
      pagetic = 9999999;
      break;
    case 2:
      pagetic = 200;
      game.state = GS_DEMOSCREEN;
      pagename = "CREDIT";
      break;
    case 3:
      G_DeferedPlayDemo("DEMO2");
      pagetic = 9999999;
      break;
    case 4:
      game.state = GS_DEMOSCREEN;
      pagetic = 200;
      switch (game.mode)
	{
	case gm_doom2:
	  pagetic = TICRATE * 11;
	  pagename = "TITLEPIC";
	  S_StartMusic(mus_dm2ttl);
	  break;
	case gm_heretic:
	  pagetic = 200;
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
      G_DeferedPlayDemo("DEMO3");
      pagetic = 9999999;
      break;
      // THE DEFINITIVE DOOM Special Edition demo
    case 6:
      G_DeferedPlayDemo("DEMO4");
      pagetic = 9999999;
      break;
    }
}



//
// D_PageTicker
// Handles timing for warped projection
//

void D_PageTicker()
{
  if (--pagetic < 0)
    D_AdvanceDemo();
}


//
// D_PageDrawer : draw a patch supposed to fill the screen,
//                fill the borders with a background pattern (a flat)
//                if the patch doesn't fit all the screen.
//
void D_PageDrawer(char *lumpname)
{
  // software mode which uses generally lower resolutions doesn't look
  // good when the pic is scaled, so it fills space aorund with a pattern,
  // and the pic is only scaled to integer multiples (x2, x3...)
  if (rendermode==render_soft)
    {
      if ((vid.width>BASEVIDWIDTH) || (vid.height>BASEVIDHEIGHT) )
        {
	  for (int y=0; y<vid.height; y += scr_borderpatch->height)
	    for (int x=0; x<vid.width; x += scr_borderpatch->width)
	      scr_borderpatch->Draw(x,y,0);
        }
    }


  Texture *t = tc.GetPtr(lumpname);
  t->Draw(0, 0, V_SCALE);

  if (game.mode >= gm_heretic && demosequence == 0 && pagetic <= 140)
    {
      t = tc.GetPtr("ADVISOR");
      t->Draw(4, 160, V_SCALE);
    }
}



//======================================================================
// D_Display
//  draw current display, possibly wiping it from the previous
//======================================================================

bool force_wipe = false;


void D_Display()
{
  extern int scaledviewwidth;

  static gamestate_t oldgamestate = GS_NULL;
  static int borderdrawcount;

  if (dedicated || nodrawers)
    return;


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


  bool screenwipe; // screen wipe in progress

  // save the current screen if about to wipe
  if (force_wipe && rendermode == render_soft)
    {
      CONS_Printf("wipe forced, state = %d\n", game.state);
      force_wipe = false;
      screenwipe = true;
      wipe_StartScreen(0, 0, vid.width, vid.height);
    }
  else
    screenwipe = false;

  // draw buffered stuff to screen
  // BP: Used only by linux GGI version
  I_UpdateNoBlit();

  bool redrawsbar = false;

  // do buffered drawing
  switch (game.state)
    {
    case GS_LEVEL:
      if (gametic)
	{
	  HU_Erase();
	  if (screenwipe
#ifdef HWRENDER
	      || rendermode != render_soft
#endif
	      )
	    redrawsbar = true;
	}

      // see if the border needs to be initially drawn
      if (oldgamestate != GS_LEVEL )
	R_FillBackScreen();    // draw the pattern into the back screen

      // draw either automap or game
      if (automap.active)
	automap.Drawer();
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
	  game.Drawer();
	}

      hud.Draw(redrawsbar); // draw hud on top anyway
      break;
      
    case GS_INTERMISSION:
      wi.Drawer();
      break;

    case GS_FINALE:
      F_Drawer();
      break;

    case GS_DEDICATEDSERVER:
    case GS_DEMOSCREEN:
      D_PageDrawer(pagename);
    case GS_WAITINGPLAYERS:
    case GS_NULL:
    default:
      break;
    }

  // change gamma if needed
  if (game.state != oldgamestate && game.state != GS_LEVEL) 
    vid.SetPalette(0);

  oldgamestate = game.state;


  // draw pause pic
  if (game.paused && !Menu::active)
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
  CON_Drawer();

  Menu::Drawer(); // menu is drawn on top of everything else
  NetUpdate();         // send out any new accumulation


  // normal update
  if (!screenwipe)
    {
      if (cv_netstat.value )
        {
	  char s[50];
	  Net_GetNetStat();
	  sprintf(s,"get %d b/s",getbps);
	  V_DrawString(BASEVIDWIDTH-V_StringWidth(s),165-40, V_WHITEMAP, s);
	  sprintf(s,"send %d b/s",sendbps);
	  V_DrawString(BASEVIDWIDTH-V_StringWidth(s),165-30, V_WHITEMAP, s);
	  sprintf(s,"GameMiss %.2f%%",gamelostpercent);
	  V_DrawString(BASEVIDWIDTH-V_StringWidth(s),165-20, V_WHITEMAP, s);
	  sprintf(s,"SysMiss %.2f%%",lostpercent);
	  V_DrawString(BASEVIDWIDTH-V_StringWidth(s),165-10, V_WHITEMAP, s);
        }

      //I_BeginProfile();
      I_FinishUpdate();              // page flip or blit buffer
      //CONS_Printf("last frame update took %d\n", I_EndProfile());
    }
  else
    {
      // wipe update
      extern consvar_t cv_screenslink;
      if (!cv_screenslink.value)
	return;

      wipe_EndScreen(0, 0, vid.width, vid.height);

      bool done;
      tic_t wipestart = I_GetTime() - 1;
      tic_t wipe_end  = wipestart + 2*TICRATE; // init a timeout
      do
	{
	  tic_t nowtime, tics;

	  do
	    {
	      //usleep(100); FIXME
	      nowtime = I_GetTime();
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
      while (!done && I_GetTime() < wipe_end);
    }
}



// =========================================================================
//   D_DoomLoop
// =========================================================================

tic_t rendergametic;


void D_DoomLoop()
{
  // timekeeping for the game
  tic_t oldtics, nowtics, elapsedtics, rendertimeout = 0;

  if (demorecording)
    game.BeginRecording();

  // user settings
  COM_BufAddText("exec autoexec.cfg\n");

  // end of loading screen: CONS_Printf() will no more call FinishUpdate()
  con_startup = false;

  oldtics = I_GetTime();

  // make sure to do a d_display to init mode _before_ load a level
  vid.SetMode();  // change video mode if needed, recalculate...

  while (1) // main game loop
    {

      // get real tics
      nowtics = I_GetTime();
      elapsedtics = nowtics - oldtics;
      oldtics = nowtics;
        
#ifdef SAVECPU_EXPERIMENTAL
      if (elapsedtics == 0)
	{
	  usleep(10000);
	  continue;
	}
#endif

      // frame syncronous IO operations
      // UNUSED for the moment (18/12/98)
      // in SDL locks screen if necessary
      I_StartFrame();


#ifdef HW3SOUND
      HW3S_BeginFrameUpdate();
#endif

      // process tics (but maybe not if elapsedtics==0), run tickers, advance game state
      TryRunTics(elapsedtics);

      if (singletics || gametic > rendergametic)
        {
	  // render if gametics have passed since last rendering
	  rendergametic = gametic;
	  rendertimeout = nowtics+TICRATE/17;

	  // move positional sounds, adjust volumes
	  S.UpdateSounds();
	  // Update display, next frame, with current state.
	  D_Display();
        }
      else if (rendertimeout < nowtics )
	{
	  // otherwise render if enough real time has elapsed since last rendering
	  // in case the server hang or netsplit
	  D_Display();
	}

	// FIXME! Doesn't look good.
#if !defined(WIN32_DIRECTX) && !defined(__OS2__) && !defined(SDL)
        //
        //Other implementations might need to update the sound here.
        //
#ifndef SNDSERV
        // Sound mixing for the buffer is snychronous.
        I_UpdateSound();
#endif
        // Synchronous sound output is explicitly called.
#ifndef SNDINTR
        // Update sound output.
        I_SubmitSound();
#endif

#endif // WIN32_DIRECTX, SDL, other civilized systems
        // check for media change, loop music..
        I_UpdateCD();

#ifdef HW3SOUND
        HW3S_EndFrameUpdate();
#endif
    }
}




//
// D_AddFile
//
static char *startupwadfiles[MAX_WADFILES];

static void D_AddFile(const char *file)
{
  static int i = 0;

  if (i >= MAX_WADFILES)
    return;

  char *newfile = (char *)malloc(strlen(file)+1);
  strcpy(newfile, file);

  startupwadfiles[i++] = newfile;
}


#ifdef __WIN32__
# define F_OK    0  // F_OK: file exists, R_OK: read permission to file                    
# define R_OK    4  //faB: win32 does not have R_OK in includes..
#elif !defined( __OS2__)
# define _MAX_PATH   MAX_WADPATH
#endif


// ==========================================================================
// Identify the Doom version, and IWAD file to use.
// Sets 'game.mode' to determine whether registered/commmercial features are
// available (notable loading PWAD files).
// ==========================================================================

// return gamemode for Doom or Ultimate Doom, use size to detect which one
static gamemode_t D_GetDoomType(const char *wadname)
{
  struct stat sbuf;
  // Fab: and if I patch my main wad and the size gets
  // bigger ? uh?
  // BP: main wad MUST not be patched !
  stat(wadname, &sbuf);
  if (sbuf.st_size<12408292)
    return gm_doom1;
  else
    return gm_udoom;      // Ultimate
}


// identifies the iwad used
void D_IdentifyVersion()
{
 char  pathtemp[_MAX_PATH];
  
#ifdef LINUX_X11 // change to the directory where 'legacy.wad' is found
  I_LocateWad();
#endif

  char *waddir = getenv("DOOMWADDIR");
  if (!waddir)
    {
      // get the current directory (possible problem on NT with "." as current dir)
      if (getcwd(pathtemp, _MAX_PATH) != NULL)
	waddir = pathtemp;
      else
	waddir = ".";
    }
  
#ifdef __MACOS__
  // cwd is always "/" when app is dbl-clicked
  if (!stricmp(waddir,"/"))
    waddir = I_GetWadDir();
#endif

  // store the wad path
  fc.SetPath(waddir);

  // will be overwritten in case of -cdrom or linux home
  sprintf(configfile, "%s/"CONFIGFILENAME, waddir);

  // external Legacy data file
  D_AddFile("legacy.wad");

  // Specify the name of an IWAD file to use.
  // Internally the game makes no difference between IWADs and PWADs.
  // Non-free files are just not offered for upload in network games.
  // The -iwad parameter just means that we MUST have this wad file
  // in order to continue. It is also loaded right after legacy.wad.

  if (M_CheckParm("-iwad"))
    {
      char *s = M_GetNextParm();

      if (access(s, F_OK))
	I_Error("IWAD %s not found!\n", s);

      D_AddFile(s);

      int i;
      // point to start of filename only
      for (i = strlen(s) - 1; i >= 0; i--)
	if (s[i]=='\\' || s[i]=='/' || s[i]==':') break;
      i++;
      
      // try to find implied gamemode
      if (!stricmp("plutonia.wad", s+i))
	{
	  game.mode = gm_doom2;
	  mission = gmi_plut;
	}
      else if (!stricmp("tnt.wad", s+i))
	{
	  game.mode = gm_doom2;
	  mission = gmi_tnt;
	}
      else if (!stricmp("heretic.wad", s+i) || !stricmp("heretic1.wad", s+i))
	game.mode = gm_heretic;
      else if (!stricmp("hexen.wad", s+i))
	game.mode = gm_hexen;
      else if (!stricmp("doom2.wad", s+i))
	game.mode = gm_doom2;
      else if (!stricmp("doomu.wad", s+i))
	game.mode = gm_udoom;
      else if (!stricmp("doom.wad", s+i))
	game.mode = D_GetDoomType(s);
      else if (!stricmp("doom1.wad", s+i))
	game.mode = gm_doom1s;
      else
	game.mode = gm_doom2;
    }
  // FIXME perhaps we should not try to find a wadfile here, rather
  // start the game without any preloaded wadfiles other than legacy.wad
  else // finally we'll try to find a wad file by ourselves
    {
      char *doom1wad = "doom1.wad";
      char *doomwad = "doom.wad";
      char *doomuwad = "doomu.wad";
      char *doom2wad = "doom2.wad";
      char *plutoniawad = "plutonia.wad";
      char *tntwad = "tnt.wad";
      char *hereticwad = "heretic.wad";
      char *heretic1wad = "heretic1.wad";
      char *hexenwad = "hexen.wad";

      if (!access(doom2wad, F_OK))
	{
	  game.mode = gm_doom2;
	  D_AddFile(doom2wad);
	}
      else if (!access(doomuwad, F_OK))
	{
	  game.mode = gm_udoom;
	  D_AddFile(doomuwad);
	}
      else if (!access(doomwad, F_OK))
	{
	  game.mode = D_GetDoomType(doomwad);
	  D_AddFile(doomwad);
	}
      else if (!access(doom1wad, F_OK))
	{
	  game.mode = gm_doom1s;
	  D_AddFile(doom1wad);
	}
      else if (!access(plutoniawad, F_OK))
	{
	  game.mode = gm_doom2;
	  mission = gmi_plut;
	  D_AddFile(plutoniawad);
	}
      else if (!access(tntwad, F_OK))
	{
	  game.mode = gm_doom2;
	  mission = gmi_tnt;
	  D_AddFile(tntwad);
	}
      else if (!access(hereticwad, F_OK))
	{
	  game.mode = gm_heretic;
	  D_AddFile(hereticwad);
	}
      else if (!access(heretic1wad, F_OK))
	{
	  game.mode = gm_heretic;
	  D_AddFile(heretic1wad);
	}
      else if (!access(hexenwad, F_OK))
	{
	  game.mode = gm_hexen;
	  D_AddFile(hexenwad);
	}
      else
	{
	  I_Error("Main IWAD file not found\n"
		  "You need either Doom.wad, Doom1.wad, Doomu.wad, Doom2.wad,\n"
		  "Tnt.wad, Plutonia.wad, Heretic.wad, Heretic1.wad or Hexen.wad\n"
	          "from any sharware or commercial version of Doom, Heretic or Hexen!\n");
	}
    }
}


// ========================================================================
// Just print the nice red titlebar like the original DOOM2 for DOS.
// ========================================================================
#ifdef PC_DOS
void D_Titlebar(char *title1, char *title2)
{
    // DOOM LEGACY banner
    clrscr();
    textattr((BLUE<<4)+WHITE);
    clreol();
    cputs(title1);

    // standard doom/doom2 banner
    textattr((RED<<4)+WHITE);
    clreol();
    gotoxy((80-strlen(title2))/2,2);
    cputs(title2);
    normvideo();
    gotoxy(1,3);

}
#endif


//
//  Center the title string, then add the date and time of compilation.
//
void D_MakeTitleString(char *s)
{
  char  temp[82];
  char *t;
  char *u;
  int   i;

  for(i=0,t=temp;i<82;i++)
    *t++=' ';

  for(t=temp+(80-strlen(s))/2,u=s;*u!='\0';)
    *t++ = *u++;

  u=__DATE__;
  for(t=temp+1,i=11;i--;)
    *t++=*u++;
  u=__TIME__;
  for(t=temp+71,i=8;i--;)
    *t++=*u++;

  temp[80]='\0';
  strcpy(s,temp);
}

void D_CheckWadVersion()
{
  // check version of legacy.wad using version lump
  int wadversion = 0;
  int lump = fc.FindNumForName("VERSION", true);
  if (lump != -1)
    {
      char s[128];
      int  l = fc.ReadLumpHeader(lump, s, 127);
      s[l]='\0';
      if (sscanf(s, "Doom Legacy WAD V%d.%d", &l, &wadversion) == 2)
	wadversion += l*100;
    }

  if (wadversion != VERSION)
    I_Error("Your legacy.wad file is version %d.%d, you need version %d.%d\n"
	    "Use the legacy.wad coming from the same zip file of this exe\n"
	    "\n"
	    "Use -nocheckwadversion to remove this check,\n"
	    "but this can cause Legacy to hang\n",
	    wadversion/100,wadversion%100,VERSION/100,VERSION%100);
}


extern char savegamename[256]; // temporary, FIXME

void SV_Init();
void CL_Init();

//
// D_DoomMain
//
void D_DoomMain()
{
  extern bool nomusic, nosound;

  // keep error messages until the final flush(stderr)
  //if (setvbuf(stderr,NULL,_IOFBF,1000)) CONS_Printf("setvbuf didnt work\n");
  if (freopen("stdout.txt", "w", stdout) == NULL) CONS_Printf("freopen didnt work\n");
  if (freopen("stderr.txt", "w", stderr) == NULL) CONS_Printf("freopen didnt work\n");
  
  // get parameters from a response file (eg: legacy @parms.txt)
  // adds parameters found within file to myargc, myargv.
  M_FindResponseFile();

  // title banner
  char legacy[82];
  // center the string, add compilation time and date.
  sprintf(legacy, "Doom Legacy %d.%d %s", VERSION/100, VERSION%100, VERSIONSTRING);
  D_MakeTitleString(legacy);

  // identify the main IWAD file to use (if any),
  // set game.mode, game.mission, devparm accordingly
  D_IdentifyVersion();

  setbuf(stdout, NULL);      // non-buffered output

  // game title
  char *title;
  switch (game.mode)
    {
    case gm_udoom    :
      title = "The Ultimate DOOM Startup"; break;
    case gm_doom1s :
      title = "DOOM Shareware Startup"; break;
    case gm_doom1:
      title = "DOOM Registered Startup"; break;
    case gm_doom2:
      switch (mission)
	{
	case gmi_plut:
	  title = "DOOM 2: Plutonia Experiment"; break;
	case gmi_tnt:
	  title = "DOOM 2: TNT - Evilution"; break;
	default:
	  title = "DOOM 2: Hell on Earth"; break;
	}
      break;
    case gm_heretic:
      title = "Heretic: Shadow of the Serpent Riders"; break;
    case gm_hexen:
      title = "Hexen: Beyond Heretic"; break;
    default:
      title = "Doom Legacy Startup"; break;
    }

#ifdef PC_DOS
  D_Titlebar(legacy,title);
#else
  CONS_Printf ("%s\n%s\n\n", legacy, title);
#endif

#ifdef __OS2__
  // FIXME do this in the OS2 interface, not here
  // set PM window title
  snprintf(pmData->title, sizeof( pmData->title), 
	   "Doom LEGACY %d.%d %s: %s",
	   VERSION/100, VERSION%100, VERSIONSTRING, title);
#endif

  // "developement parameter"
  devparm = M_CheckParm("-devparm");
  if (devparm)
    CONS_Printf(D_DEVSTR);
  
  // default savegame
  strcpy(savegamename, SAVEGAMENAME);

  {
    char *userhome, legacyhome[256];
    if (M_CheckParm("-home") && M_IsNextParm())
      userhome = M_GetNextParm();
    else
      userhome = getenv("HOME");

#ifdef LINUX // user home directory
    if (!userhome)
      I_Error("Please set $HOME to your home directory\n");
#endif  
    if (userhome)
      {
	// use user specific config file
	sprintf(legacyhome, "%s/"DEFAULTDIR, userhome);
	// little hack to allow a different config file for opengl
	// may be a problem if opengl cannot really be started
	if (M_CheckParm("-opengl"))
	  sprintf(configfile, "%s/gl"CONFIGFILENAME, legacyhome);
	else
	  sprintf(configfile, "%s/"CONFIGFILENAME, legacyhome);
      
	// can't use sprintf since there is %d in savegamename
	strcatbf(savegamename, legacyhome, "/");
	I_mkdir(legacyhome, S_IRWXU);
      }
  }

  // add any files specified on the command line with -file wadfile
  // to the wad list
  if (M_CheckParm("-file"))
    {
      // the parms after p are wadfile/lump names,
      // until end of parms or another - preceded parm
      while (M_IsNextParm())
	D_AddFile(M_GetNextParm());
    }

  int p;
  // load dehacked files
  p = M_CheckParm("-dehacked");
  if (!p)
    p = M_CheckParm("-deh");
  if (p != 0)
    {
      while (M_IsNextParm())
	D_AddFile(M_GetNextParm());
    }

  // ----------- start subsystem initializations
  // order: memory, engine patching, L1 cache,
  //  video, HUD, console (now consvars can be registered), read config file,
  //  menu, renderer, sound, scripting.

  // zone memory management
  Z_Init(); 

  // file cache
  if (!fc.InitMultipleFiles(startupwadfiles))
    I_Error("A WAD file was not found\n");

  // see that legacy.wad version matches program version
  if (!M_CheckParm("-nocheckwadversion"))
    D_CheckWadVersion();

  // the command buffer
  COM_Init();

  // system-specific stuff
  CONS_Printf("Sys_Init: Init system-specific stuff.\n");
  I_SysInit();

  // TODO init keybindings and controls, init network stuff 

  // adapt tables to legacy needs
  P_PatchInfoTables();

  switch (game.mode)
    {
    case gm_hexen:
      HexenPatchEngine();
      break;
    case gm_heretic:
      HereticPatchEngine();
      break;
    default:
      DoomPatchEngine(); // TODO temporary solution, we must be able to switch game.mode anytime!
    }

  // we need to check for dedicated before initialization of some subsystems
  dedicated = M_CheckParm("-dedicated");
  nosound = M_CheckParm("-nosound");
  nomusic = M_CheckParm("-nomusic");

  // Server init
  SV_Init();

  // Client init
  if (!dedicated) 
    CL_Init();

  // FIXME should all the consvars be registered by now?
  //------------------------------------- CONFIG.CFG
  // loads and executes config file
  M_FirstLoadConfig(); // WARNING : this does a "COM_BufExecute()"

  if (!dedicated)
    {
      // FIXME replace these with cv_fullscreen_onchange or something...
      I_PrepareVideoModeList(); // Regenerate Modelist according to cv_fullscreen
      // set user default mode or mode set at cmdline
      SCR_CheckDefaultMode();
    }

  //-------------------------------------- COMMAND LINE PARAMS

  // Initialize CD-Audio
  if (!M_CheckParm("-nocd"))
    I_InitCD();
  if (M_CheckParm("-respawn"))
    COM_BufAddText("respawnmonsters 1\n");
  if (M_CheckParm("-teamplay"))
    COM_BufAddText("teamplay 1\n");
  if (M_CheckParm("-teamskin"))
    COM_BufAddText("teamplay 2\n");
  if (M_CheckParm("-splitscreen"))
    CV_SetValue(&cv_splitscreen,1);

  if (M_CheckParm("-altdeath"))
    COM_BufAddText("deathmatch 2\n");
  else if (M_CheckParm("-deathmatch"))
    COM_BufAddText("deathmatch 1\n");

  if (M_CheckParm("-fast"))
    COM_BufAddText("fastmonsters 1\n");

  game.nomonsters = M_CheckParm("-nomonsters");

  if (M_CheckParm("-timer"))
    {
      char *s = M_GetNextParm();
      COM_BufAddText(va("timelimit %s\n",s ));
    }

  // push all "+" parameter at the command buffer
  M_PushSpecialParameters();
 

  // ------------- starting the game ----------------

  bool autostart = false;
  // init all NETWORK
  CONS_Printf("D_CheckNetGame: Checking network game status.\n");
  if (D_CheckNetGame())
    autostart = true;

  // check for a driver that wants intermission stats
  p = M_CheckParm("-statcopy");
  if (p && p<myargc-1)
    {
      I_Error("Sorry but statcopy isn't supported at this time\n");
      /*
        // for statistics driver
        extern  void*   statcopy;
        statcopy = (void*)atoi(myargv[p+1]);
      */
    }

  // get skill / episode / map from parms
  skill_t sk = sk_medium;

  p = M_CheckParm("-skill");
  if (p && p < myargc-1)
    {
      sk = (skill_t)(myargv[p+1][0]-'1');
      autostart = true;
    }

  int startepisode = 1;
  int startmap = 1;

  p = M_CheckParm("-episode");
  if (p && p < myargc-1)
    {
      startepisode = myargv[p+1][0]-'0';
      autostart = true;
    }

  p = M_CheckParm("-warp");
  if (p && p < myargc-1)
    {
      if (game.mode == gm_doom2)
	startmap = atoi(myargv[p+1]);
      else
	{
	  startepisode = myargv[p+1][0]-'0';
	  if (p < myargc-2 &&
	      myargv[p+2][0]>='0' &&
	      myargv[p+2][0]<='9')
	    startmap = myargv[p+2][0]-'0';
	  else
	    startmap = 1;
	}
      autostart = true;
    }


  // Check and print which version is executed.
  switch (game.mode)
    {
    case gm_doom1s:
      CONS_Printf(text[TXT_SHAREWARE]);
      break;
    case gm_doom1:
    case gm_udoom:
    case gm_doom2:
      CONS_Printf(text[TXT_COMMERCIAL]);
      break;
    default:
      break;
    }

  // start the apropriate game based on parms
  p = M_CheckParm("-record");
  if (p && p < myargc-1)
    {
      // sets up demo recording
      //G_RecordDemo(myargv[p+1]);
      autostart = true;
    }

  // demo doesn't need anymore to be added with D_AddFile()
  p = M_CheckParm("-playdemo");
  if (!p)
    p = M_CheckParm("-timedemo");
  if (p && M_IsNextParm())
    {
      char tmp[MAX_WADPATH];
      // add .lmp to identify the EXTERNAL demo file
      // it is NOT possible to play an internal demo using -playdemo,
      // rather push a playdemo command.. to do.

      strcpy(tmp,M_GetNextParm());
      // get spaced filename or directory
      while(M_IsNextParm()) { strcat(tmp," ");strcat(tmp,M_GetNextParm()); }
      // VB: just horrible;)
      FIL_DefaultExtension(tmp,".lmp");

      CONS_Printf("Playing demo %s.\n",tmp);

      if ((p = M_CheckParm("-playdemo")))
        {
	  singledemo = true;              // quit after one demo
	  G_DeferedPlayDemo(tmp);
        }
      //else G_TimeDemo(tmp);
	
      game.state = GS_NULL;

      return;         
    }

  p = M_CheckParm("-loadgame");
  if (p && p < myargc-1)
    {
      COM_BufAddText(va("load %d\n", atoi(myargv[p+1])));
    }
  else if (dedicated && server)
    {
      pagename = "TITLEPIC";
      game.state = GS_DEDICATEDSERVER;
    }
  else if (autostart || game.netgame || M_CheckParm("+connect") || M_CheckParm("-connect"))
    {
      if (server && !M_CheckParm("+map"))
	{
	  game.Create_classic_game(startepisode);
	  //COM_BufAddText (va("map \"%s\"\n", G_BuildMapName(startepisode, startmap)));
	  // FIXME this function nukes most of the game parameters that may have
	  // been set using cmdline args. Perhaps most cmdline args should be removed?
	  // one can always use console-args (like legacy.exe +echo "sdfad")
	  game.DeferredNewGame(sk, false);
	}
    }
  else
    game.StartIntro(); // start up intro loop 
}
