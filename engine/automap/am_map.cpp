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
// Revision 1.18  2004/10/14 19:35:46  smite-meister
// automap, bbox_t
//
// Revision 1.17  2004/09/23 23:21:17  smite-meister
// HUD updated
//
// Revision 1.16  2004/08/12 18:30:27  smite-meister
// cleaned startup
//
// Revision 1.15  2004/07/25 20:19:21  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.14  2004/07/13 20:23:36  smite-meister
// Mod system basics
//
// Revision 1.13  2004/07/05 16:53:27  smite-meister
// Netcode replaced
//
// Revision 1.12  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.9  2003/11/12 11:07:25  smite-meister
// Serialization done. Map progression.
//
// Revision 1.5  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.4  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
//-----------------------------------------------------------------------------

/// \file
/// \brief The automap code

#include "am_map.h"

#include "doomdef.h"
#include "doomdata.h"
#include "d_event.h"

#include "g_game.h"
#include "g_player.h"
#include "g_input.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_mapinfo.h"

#include "hud.h"
#include "dstrings.h"
#include "keys.h"

#include "r_data.h"
#include "r_defs.h"
#include "r_sprite.h"
#include "r_draw.h"
#include "v_video.h"

#include "m_bbox.h"

#include "w_wad.h"
#include "z_zone.h"

#include "i_system.h"
#include "i_video.h"
#include "tables.h"

#ifdef HWRENDER
#include "hardware/hwr_render.h"
#endif


// the only AutoMap instance in the game
AutoMap automap;


struct mline_t
{
  mpoint_t a, b;
};


byte *fb; // pseudo-frame buffer

// player radius used only in am_map.c
#define PLAYERRADIUS    (16*FRACUNIT)


// For use if I do walls with outsides/insides
static byte REDS        =    (256-5*16);
static byte REDRANGE    =    16;
static byte BLUES       =    (256-4*16+8);
static byte BLUERANGE   =    8;
static byte GREENS      =    (7*16);
static byte GREENRANGE  =    16;
static byte GRAYS       =    (6*16);
static byte GRAYSRANGE  =    16;
static byte BROWNS      =    (4*16);
static byte BROWNRANGE  =    16;
static byte YELLOWS     =    (256-32+7);
static byte YELLOWRANGE =    1;
static byte DBLACK      =    0;
static byte DWHITE      =    (256-47);

static byte BLUEKEYCOLOR;
static byte YELLOWKEYCOLOR;
static byte REDKEYCOLOR;


// Automap colors
#define BACKGROUND      DBLACK
#define YOURCOLORS      DWHITE
#define YOURRANGE       0
#define WALLCOLORS      REDS
#define WALLRANGE       REDRANGE
#define TSWALLCOLORS    GRAYS
#define TSWALLRANGE     GRAYSRANGE
#define FDWALLCOLORS    BROWNS
#define FDWALLRANGE     BROWNRANGE
#define CDWALLCOLORS    YELLOWS
#define CDWALLRANGE     YELLOWRANGE
#define THINGCOLORS     GREENS
#define THINGRANGE      GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS      (GRAYS + GRAYSRANGE/2)
#define GRIDRANGE       0
#define XHAIRCOLORS     GRAYS

// keys
#define AM_PANDOWNKEY   KEY_DOWNARROW
#define AM_PANUPKEY     KEY_UPARROW
#define AM_PANRIGHTKEY  KEY_RIGHTARROW
#define AM_PANLEFTKEY   KEY_LEFTARROW
#define AM_ZOOMINKEY    '='
#define AM_ZOOMOUTKEY   '-'
#define AM_STARTKEY     KEY_TAB
#define AM_ENDKEY       KEY_TAB
#define AM_GOBIGKEY     '0'
#define AM_FOLLOWKEY    'f'
#define AM_GRIDKEY      'g'
#define AM_MARKKEY      'm'
#define AM_CLEARMARKKEY 'c'


// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC        4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (f_x + MTOF((x)-m_x))
#define CYMTOF(y)  (f_y + (f_h - MTOF((y)-m_y)))


//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] =
{
  { { -R+R/8, 0 }, { R, 0 } }, // -----
  { { R, 0 }, { R-R/2, R/4 } },  // ----->
  { { R, 0 }, { R-R/2, -R/4 } },
  { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
  { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
  { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
  { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
static const int NUMPLYRLINES = sizeof(player_arrow)/sizeof(mline_t);

mline_t cheat_player_arrow[] =
{
  { { -R+R/8, 0 }, { R, 0 } }, // -----
  { { R, 0 }, { R-R/2, R/6 } },  // ----->
  { { R, 0 }, { R-R/2, -R/6 } },
  { { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
  { { -R+R/8, 0 }, { -R-R/8, -R/6 } },
  { { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
  { { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
  { { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
  { { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
  { { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
  { { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
  { { -R/6, -R/6 }, { 0, -R/6 } },
  { { 0, -R/6 }, { 0, R/4 } },
  { { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
  { { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
  { { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
static const int NUMCHEATPLYRLINES = sizeof(cheat_player_arrow)/sizeof(mline_t);

#define R (FRACUNIT)
mline_t triangle_guy[] =
{
  { { (fixed_t)-.867*R, (fixed_t)-.5*R }, { (fixed_t) .867*R, (fixed_t)-.5*R } },
  { { (fixed_t) .867*R, (fixed_t)-.5*R }, { (fixed_t)      0, (fixed_t)    R } },
  { { (fixed_t)      0, (fixed_t)    R }, { (fixed_t)-.867*R, (fixed_t)-.5*R } }
};
#undef R
static const int NUMTRIANGLEGUYLINES = sizeof(triangle_guy)/sizeof(mline_t);

#define R (FRACUNIT)
mline_t thintriangle_guy[] = {
  { { (fixed_t)-.5*R, (fixed_t)-.7*R }, { (fixed_t)    R, (fixed_t)    0 } },
  { { (fixed_t)    R, (fixed_t)    0 }, { (fixed_t)-.5*R, (fixed_t) .7*R } },
  { { (fixed_t)-.5*R, (fixed_t) .7*R }, { (fixed_t)-.5*R, (fixed_t)-.7*R } }
};
#undef R
static const int NUMTHINTRIANGLEGUYLINES = sizeof(thintriangle_guy)/sizeof(mline_t);



// location of window on screen
static int      f_x;
static int      f_y;

// size of window on screen
static int      f_w;
static int      f_h;

static int      lightlev;               // used for funky strobing effect


static mpoint_t m_paninc; // how far the window pans each tic (map coords)
static fixed_t  mtof_zoommul; // how far the window zooms in each tic (map coords)
static fixed_t  ftom_zoommul; // how far the window zooms in each tic (fb coords)


// width/height of window on map (map coords)
static fixed_t  m_w;
static fixed_t  m_h;

static fixed_t  m_x, m_y;   // LL x,y where the window is on the map (map coords)
static fixed_t  m_x2, m_y2; // UR x,y where the window is on the map (map coords)


// zoom
static fixed_t min_scale_mtof; // used to tell when to stop zooming out
static fixed_t max_scale_mtof; // used to tell when to stop zooming in
static fixed_t scale_mtof; // used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_ftom; // used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)


// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;



// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.
/*
struct islope_t
{
  fixed_t slp, islp;
};

TODO
static void AM_getIslope(mline_t *ml, islope_t *is)
{
  int dx, dy;

  dy = ml->a.y - ml->b.y;
  dx = ml->b.x - ml->a.x;
  if (!dy) is->islp = (dx<0?-MAXINT:MAXINT);
  else is->islp = FixedDiv(dx, dy);
  if (!dx) is->slp = (dy<0?-MAXINT:MAXINT);
  else is->slp = FixedDiv(dy, dx);

}
*/

AutoMap::AutoMap()
{
  mp = NULL;
  mpawn = NULL;

  followplayer = true;
  grid = false;
  markpointnum = 0;

  mapback = NULL;

  active = false;
  translucent = false;
  am_recalc = true;
  am_cheating = 0;

}

void AutoMap::Startup()
{
  int i;
  if ((i = fc.FindNumForName("AUTOPAGE")) >= 0)
    {
      int size = fc.LumpLength(i);
      if (size % 320)
	I_Error("Size of AUTOPAGE (%d bytes) must be a multiple of 320!\n", size);

      mapback = new LumpTexture("AUTOPAGE", i, 320, size/320);
      tc.Insert(mapback);
    }
  else
    mapback = NULL;
}


// opens the automap, centered on p
void AutoMap::Open(const PlayerPawn *p)
{
  if (active)
    return;
  CONS_Printf("AM::Open, pawn = %p, map = %p\n", p, p->mp);
  mpawn = p;
  mp = p->mp;

  // just in case...
  if (mp == NULL)
    return;

  // if AutoMap is active(open), it MUST have a valid mp and mpawn
  active = true;
  bigstate = false;

  // FIXME Reset() must also be called if a new map is loaded
  if (am_recalc) // screen size changed
    {
      am_recalc = false;
      Resize();
    }

  InitVariables();

  // prepare textures
  for (int i=0;i<10;i++)
    {
      char namebuf[9];
      sprintf(namebuf, "AMMNUM%d", i);
      marknums[i] = tc.GetPtr(namebuf);
    }
}


// shuts down the automap
void AutoMap::Close()
{
  if (!active)
    return;

  mp = NULL;
  mpawn = NULL;
  active = false;
}


// should be called after a new map is started
/*
void AutoMap::Reset(const PlayerPawn *p)
{
  CONS_Printf("AM::Reset\n");
  mpawn = p;
  mp = p->mp;
  if (mp == NULL)
    return;

  clearMarks();
  Resize();
}
*/


// should be called after any vidmode change
void AutoMap::Resize()
{
  if (!active)
    {
      // delay it until opened
      am_recalc = true;
      return;
    }

  CONS_Printf("AM::Resize\n");

  fb = vid.screens[0];

  f_x = f_y = 0;
  f_w = vid.width;
  f_h = vid.height - hud.stbarheight;

  // sets global variables controlling zoom range.
  fixed_t w = FixedDiv(f_w << FRACBITS, mp->root_bbox.box[BOXRIGHT] - mp->root_bbox.box[BOXLEFT]);
  fixed_t h = FixedDiv(f_h << FRACBITS, mp->root_bbox.box[BOXTOP] - mp->root_bbox.box[BOXBOTTOM]);

  min_scale_mtof = w < h ? w : h; // min zoom
  max_scale_mtof = FixedDiv(f_h<<FRACBITS, 2*PLAYERRADIUS);

  scale_mtof = FixedDiv(min_scale_mtof, int(0.7 * FRACUNIT));
  if (scale_mtof > max_scale_mtof)
    scale_mtof = min_scale_mtof;
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}



// initializes the map window location, panning, zoom, colors etc.
void AutoMap::InitVariables()
{
  f_oldloc.x = MAXINT;
  amclock = 0;
  lightlev = 0;

  m_paninc.x = m_paninc.y = 0;
  ftom_zoommul = FRACUNIT;
  mtof_zoommul = FRACUNIT;

  // which part of the map is visible?
  m_w = FTOM(f_w);
  m_h = FTOM(f_h);

  m_x = mpawn->x - m_w/2;
  m_y = mpawn->y - m_h/2;
  changeWindowLoc();

  // for saving & restoring
  old_m_x = m_x;
  old_m_y = m_y;
  old_m_w = m_w;
  old_m_h = m_h;

  if (game.mode >= gm_heretic)
    {
      REDS       = 12*8;
      REDRANGE   = 1;
      BLUES      = (256-4*16+8);
      BLUERANGE  = 1;
      GREENS     = 224;
      GREENRANGE = 1;
      GRAYS      = (5*8);
      GRAYSRANGE = 1;
      BROWNS     = (14*8);
      BROWNRANGE = 1;
      YELLOWS    = 10*8;
      YELLOWRANGE= 1;
      DBLACK      = 0;
      DWHITE      = 4*8;

      BLUEKEYCOLOR = 197;
      YELLOWKEYCOLOR = 144;
      REDKEYCOLOR = 220; // green
    }
  else
    {
      BLUEKEYCOLOR = 200;
      YELLOWKEYCOLOR = 231;
      REDKEYCOLOR = 176;
    }

  // inform the status bar of the change
  hud.st_refresh = true;
}


// pans and clips the "map window" location on the actual Map
void AutoMap::changeWindowLoc()
{
  if (m_paninc.x || m_paninc.y)
    {
      followplayer = false;
      f_oldloc.x = MAXINT;
    }

  m_x += m_paninc.x;
  m_y += m_paninc.y;

  if (m_x + m_w/2 > mp->root_bbox.box[BOXRIGHT])
    m_x = mp->root_bbox.box[BOXRIGHT] - m_w/2;
  else if (m_x + m_w/2 < mp->root_bbox.box[BOXLEFT])
    m_x = mp->root_bbox.box[BOXLEFT] - m_w/2;

  if (m_y + m_h/2 > mp->root_bbox.box[BOXTOP])
    m_y = mp->root_bbox.box[BOXTOP] - m_h/2;
  else if (m_y + m_h/2 < mp->root_bbox.box[BOXBOTTOM])
    m_y = mp->root_bbox.box[BOXBOTTOM] - m_h/2;

  m_x2 = m_x + m_w;
  m_y2 = m_y + m_h;
}





// zooms the map window to a new scale
static void AM_activateNewScale()
{
  m_x += m_w/2;
  m_y += m_h/2;
  m_w = FTOM(f_w);
  m_h = FTOM(f_h);
  m_x -= m_w/2;
  m_y -= m_h/2;
  m_x2 = m_x + m_w;
  m_y2 = m_y + m_h;
}


void AM_saveScaleAndLoc()
{
  old_m_x = m_x;
  old_m_y = m_y;
  old_m_w = m_w;
  old_m_h = m_h;
}


void AutoMap::restoreScaleAndLoc()
{
  m_w = old_m_w;
  m_h = old_m_h;
  if (!followplayer)
    {
      m_x = old_m_x;
      m_y = old_m_y;
    } else {
      m_x = mpawn->x - m_w/2;
      m_y = mpawn->y - m_h/2;
    }
  m_x2 = m_x + m_w;
  m_y2 = m_y + m_h;

  // Change the scaling multipliers
  scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}


/// adds a marker at the current location
void AutoMap::addMark()
{
  markpoints[markpointnum].x = m_x + m_w/2;
  markpoints[markpointnum].y = m_y + m_h/2;
  markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
}



void AutoMap::clearMarks()
{
  for (int i=0; i<AM_NUMMARKPOINTS; i++)
    markpoints[i].x = -1; // means empty
  markpointnum = 0;
}


//
// set the window scale to the maximum size
//
void AM_minOutWindowScale()
{
  scale_mtof = min_scale_mtof;
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
  AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale()
{
  scale_mtof = max_scale_mtof;
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
  AM_activateNewScale();
}



// Handle events (user inputs) in automap mode
bool AutoMap::Responder(event_t *ev)
{
  static int cheatstate=0;
  static char buffer[20];

  bool rc = false;

  if (!active)
    {
      if (ev->type == ev_keydown && ev->data1 == AM_STARTKEY)
        {
          //faB: prevent alt-tab in win32 version to activate automap just before minimizing the app
          //     doesn't do any harm to the DOS version
          if (!altdown)
            {
              if (displayplayer->pawn)
                Open(displayplayer->pawn);
              rc = true;
            }
        }
    }
  else if (ev->type == ev_keydown)
    {
      rc = true;
      switch(ev->data1)
        {
        case AM_PANRIGHTKEY: // pan right
          if (!followplayer) m_paninc.x = FTOM(F_PANINC);
          else rc = false;
          break;
        case AM_PANLEFTKEY: // pan left
          if (!followplayer) m_paninc.x = -FTOM(F_PANINC);
          else rc = false;
          break;
        case AM_PANUPKEY: // pan up
          if (!followplayer) m_paninc.y = FTOM(F_PANINC);
          else rc = false;
          break;
        case AM_PANDOWNKEY: // pan down
          if (!followplayer) m_paninc.y = -FTOM(F_PANINC);
          else rc = false;
          break;
        case AM_ZOOMOUTKEY: // zoom out
          mtof_zoommul = M_ZOOMOUT;
          ftom_zoommul = M_ZOOMIN;
          break;
        case AM_ZOOMINKEY: // zoom in
          mtof_zoommul = M_ZOOMIN;
          ftom_zoommul = M_ZOOMOUT;
          break;
        case AM_ENDKEY:
          Close();
          break;
        case AM_GOBIGKEY:
          bigstate = !bigstate;
          if (bigstate)
            {
              AM_saveScaleAndLoc();
              AM_minOutWindowScale();
            }
          else
	    restoreScaleAndLoc();
          break;

          // messages are given to consoleplayer, because he's pressing the keys
        case AM_FOLLOWKEY:
          followplayer = !followplayer;
          f_oldloc.x = MAXINT;
          consoleplayer->SetMessage(followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF);
          break;
        case AM_GRIDKEY:
          grid = !grid;
          consoleplayer->SetMessage(grid ? AMSTR_GRIDON : AMSTR_GRIDOFF);
          break;
        case AM_MARKKEY:
          sprintf(buffer, "%s %d", AMSTR_MARKEDSPOT, markpointnum);
          consoleplayer->SetMessage(buffer);
          addMark();
          break;
        case AM_CLEARMARKKEY:
          clearMarks();
          consoleplayer->SetMessage(AMSTR_MARKSCLEARED);
          break;
        default:
          cheatstate=0;
          rc = false;
        }
    }
  else if (ev->type == ev_keyup)
    {
      rc = false;
      switch (ev->data1)
        {
        case AM_PANRIGHTKEY:
          if (!followplayer) m_paninc.x = 0;
          break;
        case AM_PANLEFTKEY:
          if (!followplayer) m_paninc.x = 0;
          break;
        case AM_PANUPKEY:
          if (!followplayer) m_paninc.y = 0;
          break;
        case AM_PANDOWNKEY:
          if (!followplayer) m_paninc.y = 0;
          break;
        case AM_ZOOMOUTKEY:
        case AM_ZOOMINKEY:
          mtof_zoommul = FRACUNIT;
          ftom_zoommul = FRACUNIT;
          break;
        }
    }

  return rc;
}


//
// Zooming
//
void AM_changeWindowScale()
{

  // Change the scaling multipliers
  scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

  if (scale_mtof < min_scale_mtof)
    AM_minOutWindowScale();
  else if (scale_mtof > max_scale_mtof)
    AM_maxOutWindowScale();
  else
    AM_activateNewScale();
}



// Updates on Game Tick
void AutoMap::Ticker()
{
  if (dedicated)
    return;

  if (!active)
    return;

  if (followplayer && (f_oldloc.x != mpawn->x || f_oldloc.y != mpawn->y))
    {
      m_x = FTOM(MTOF(mpawn->x)) - m_w/2;
      m_y = FTOM(MTOF(mpawn->y)) - m_h/2;
      m_x2 = m_x + m_w;
      m_y2 = m_y + m_h;
      f_oldloc.x = mpawn->x;
      f_oldloc.y = mpawn->y;

      //  m_x = FTOM(MTOF(mpawn->x - m_w/2));
      //  m_y = FTOM(MTOF(mpawn->y - m_h/2));
      //  m_x = mpawn->x - m_w/2;
      //  m_y = mpawn->y - m_h/2;
    }

  // Change the zoom if necessary
  if (ftom_zoommul != FRACUNIT)
    AM_changeWindowScale();

  // Change x,y location
  if (m_paninc.x || m_paninc.y)
    changeWindowLoc();

  // Update light level
  /*
  static int nexttic = 0;
  const int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };
  static int index = 0;

  // Change light level
  if (++amclock > nexttic)
    {
      lightlev = litelevels[index++];
      if (index == sizeof(litelevels)/sizeof(int))
	index = 0;
      nexttic = amclock + 6 - (amclock % 6);
    }
  */
}



// Clear automap frame buffer.
void AutoMap::clearFB(int color)
{
  if (!mapback)
    {
#ifdef HWRENDER
      if (rendermode != render_soft)
	HWR.ClearAutomap();
      else
#endif
	memset(fb, color, f_w*f_h*vid.BytesPerPixel);
    }
  else
    {
      /*
      static int mapxstart;
      static int mapystart;
      int MAPLUMPHEIGHT = f_h;

      if (followplayer)
        {
          static vertex_t oldplr;

          int dmapx = (MTOF(mpawn->x)-MTOF(oldplr.x)); //fixed point
          int dmapy = (MTOF(oldplr.y)-MTOF(mpawn->y));

          oldplr.x = mpawn->x;
          oldplr.y = mpawn->y;
          mapxstart += dmapx>>1;
          mapystart += dmapy>>1;

          while(mapxstart >= 320)
            mapxstart -= 320;
          while(mapxstart < 0)
            mapxstart += 320;
          while(mapystart >= MAPLUMPHEIGHT)
            mapystart -= MAPLUMPHEIGHT;
          while(mapystart < 0)
            mapystart += MAPLUMPHEIGHT;
        }
      else
        {
          mapxstart += (MTOF(m_paninc.x)>>1);
          mapystart -= (MTOF(m_paninc.y)>>1);
          if( mapxstart >= 320 )
            mapxstart -= 320;
          if( mapxstart < 0 )
            mapxstart += 320;
          if( mapystart >= MAPLUMPHEIGHT )
            mapystart -= MAPLUMPHEIGHT;
          if( mapystart < 0 )
            mapystart += MAPLUMPHEIGHT;
        }

      fixed_t colfrac = FixedDiv(FRACUNIT, vid.dupx << FRACBITS);
      fixed_t rowfrac = FixedDiv(FRACUNIT, vid.dupy << FRACBITS);

      //blit the automap background to the screen.
      byte *dest = vid.screens[0];
      int orig = mapxstart + mapystart*320;
      fixed_t row = 0;
      for (int y=0; y < f_h; y++)
        {
	  fixed_t col = 0;
	  int base = orig + (row >> FRACBITS)*320;
          for (int x=0; x < f_w; x++)
            {
	      int i = base + (col >> FRACBITS);
              *dest++ = mapback[i % (320*(200-65))];
	      col += colfrac;
            }
	  row += rowfrac;
        }
      */

      mapback->DrawFill(0, 0, f_w, f_h);
    }
}


//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
static bool AM_clipMline(mline_t* ml, fline_t* fl)
{
  enum
  {
    LEFT    =1,
    RIGHT   =2,
    BOTTOM  =4,
    TOP     =8
  };

  register    int outcode1 = 0;
  register    int outcode2 = 0;
  register    int outside;

  fpoint_t    tmp;
  int         dx;
  int         dy;


#define DOOUTCODE(oc, mx, my) \
    (oc) = 0; \
    if ((my) < 0) (oc) |= TOP; \
    else if ((my) >= f_h) (oc) |= BOTTOM; \
    if ((mx) < 0) (oc) |= LEFT; \
    else if ((mx) >= f_w) (oc) |= RIGHT;


  // do trivial rejects and outcodes
  if (ml->a.y > m_y2)
    outcode1 = TOP;
  else if (ml->a.y < m_y)
    outcode1 = BOTTOM;

  if (ml->b.y > m_y2)
    outcode2 = TOP;
  else if (ml->b.y < m_y)
    outcode2 = BOTTOM;

  if (outcode1 & outcode2)
    return false; // trivially outside

  if (ml->a.x < m_x)
    outcode1 |= LEFT;
  else if (ml->a.x > m_x2)
    outcode1 |= RIGHT;

  if (ml->b.x < m_x)
    outcode2 |= LEFT;
  else if (ml->b.x > m_x2)
    outcode2 |= RIGHT;

  if (outcode1 & outcode2)
    return false; // trivially outside

  // transform to frame-buffer coordinates.
  fl->a.x = CXMTOF(ml->a.x);
  fl->a.y = CYMTOF(ml->a.y);
  fl->b.x = CXMTOF(ml->b.x);
  fl->b.y = CYMTOF(ml->b.y);

  DOOUTCODE(outcode1, fl->a.x, fl->a.y);
  DOOUTCODE(outcode2, fl->b.x, fl->b.y);

  if (outcode1 & outcode2)
    return false;

  while (outcode1 | outcode2)
    {
      // may be partially inside box
      // find an outside point
      if (outcode1)
        outside = outcode1;
      else
        outside = outcode2;

      // clip to each side
      if (outside & TOP)
        {
          dy = fl->a.y - fl->b.y;
          dx = fl->b.x - fl->a.x;
          tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
          tmp.y = 0;
        }
      else if (outside & BOTTOM)
        {
          dy = fl->a.y - fl->b.y;
          dx = fl->b.x - fl->a.x;
          tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
          tmp.y = f_h-1;
        }
      else if (outside & RIGHT)
        {
          dy = fl->b.y - fl->a.y;
          dx = fl->b.x - fl->a.x;
          tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
          tmp.x = f_w-1;
        }
      else if (outside & LEFT)
        {
          dy = fl->b.y - fl->a.y;
          dx = fl->b.x - fl->a.x;
          tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
          tmp.x = 0;
        }

      if (outside == outcode1)
        {
          fl->a = tmp;
          DOOUTCODE(outcode1, fl->a.x, fl->a.y);
        }
      else
        {
          fl->b = tmp;
          DOOUTCODE(outcode2, fl->b.x, fl->b.y);
        }

      if (outcode1 & outcode2)
        return false; // trivially outside
    }

  return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
static void AM_drawFline_soft(fline_t* fl, int color)
{
  register int x;
  register int y;
  register int dx;
  register int dy;
  register int sx;
  register int sy;
  register int ax;
  register int ay;
  register int d;


#ifdef PARANOIA
  static int fuck = 0;

  // For debugging only
  if (      fl->a.x < 0 || fl->a.x >= f_w
            || fl->a.y < 0 || fl->a.y >= f_h
            || fl->b.x < 0 || fl->b.x >= f_w
            || fl->b.y < 0 || fl->b.y >= f_h)
    {
      CONS_Printf("line clipping problem %d \r", fuck++);
      return;
    }
#endif

#define PUTDOT(xx,yy,cc) fb[(yy)*f_w+(xx)]=(cc)

  dx = fl->b.x - fl->a.x;
  ax = 2 * (dx<0 ? -dx : dx);
  sx = dx<0 ? -1 : 1;

  dy = fl->b.y - fl->a.y;
  ay = 2 * (dy<0 ? -dy : dy);
  sy = dy<0 ? -1 : 1;

  x = fl->a.x;
  y = fl->a.y;

  if (ax > ay)
    {
      d = ay - ax/2;
      while (1)
        {
          PUTDOT(x,y,color);
          if (x == fl->b.x) return;
          if (d>=0)
            {
              y += sy;
              d -= ax;
            }
          x += sx;
          d += ay;
        }
    }
  else
    {
      d = ax - ay/2;
      while (1)
        {
          PUTDOT(x, y, color);
          if (y == fl->b.y) return;
          if (d >= 0)
            {
              x += sx;
              d -= ay;
            }
          y += sy;
          d += ax;
        }
    }
}


//
// Clip lines, draw visible parts of lines.
//
static void AM_drawMline(mline_t* ml, int color)
{
  static fline_t fl;

  if (AM_clipMline(ml, &fl))
    {
#ifdef HWRENDER
      if (rendermode != render_soft)
	HWRend::DrawAMline(&fl, color);
      else
#endif
	AM_drawFline_soft(&fl, color); // draws it on frame buffer using fb coords
    }
}




/// Draws flat (floor/ceiling tile) aligned grid lines.
void AutoMap::drawGrid(int color)
{
  fixed_t x, y;
  fixed_t start, end;
  mline_t ml;

  // Figure out start of vertical gridlines
  start = m_x;
  if ((start - mp->bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
    start += (MAPBLOCKUNITS<<FRACBITS)
      - ((start - mp->bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
  end = m_x + m_w;

  // draw vertical gridlines
  ml.a.y = m_y;
  ml.b.y = m_y+m_h;
  for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
    {
      ml.a.x = x;
      ml.b.x = x;
      AM_drawMline(&ml, color);
    }

  // Figure out start of horizontal gridlines
  start = m_y;
  if ((start - mp->bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
    start += (MAPBLOCKUNITS<<FRACBITS)
      - ((start - mp->bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
  end = m_y + m_h;

  // draw horizontal gridlines
  ml.a.x = m_x;
  ml.b.x = m_x + m_w;
  for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
    {
      ml.a.y = y;
      ml.b.y = y;
      AM_drawMline(&ml, color);
    }
}


// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
void AutoMap::drawWalls()
{
  int i;
  static mline_t l;
  // TODO polyobjs

  for (i=0; i < mp->numlines; i++)
    {
      l.a.x = mp->lines[i].v1->x;
      l.a.y = mp->lines[i].v1->y;
      l.b.x = mp->lines[i].v2->x;
      l.b.y = mp->lines[i].v2->y;
      if (am_cheating || (mp->lines[i].flags & ML_MAPPED))
        {
          if ((mp->lines[i].flags & ML_DONTDRAW) && !am_cheating)
            continue;
          if (!mp->lines[i].backsector)
            {
              AM_drawMline(&l, WALLCOLORS+lightlev);
            }
          else
            {
              switch (mp->lines[i].special)
		{
		case 70: // teleporters
		  AM_drawMline(&l, WALLCOLORS+WALLRANGE/2);
		  break;
		case 13: // locked doors
		  // mp->lines[i].args[3] = lock type
		  AM_drawMline(&l, BLUEKEYCOLOR);
		  //AM_drawMline(&l, YELLOWKEYCOLOR);
		  //AM_drawMline(&l, REDKEYCOLOR);
		  break;
		case 83: // ACS_Locked_Execute
		  AM_drawMline(&l, REDKEYCOLOR);
		  break;

		default :
		  if (mp->lines[i].flags & ML_SECRET) // secret door
		    {
		      if (am_cheating)
			AM_drawMline(&l, SECRETWALLCOLORS + lightlev);
		      else
			AM_drawMline(&l, WALLCOLORS + lightlev);
		    }
		  else if (mp->lines[i].backsector->floorheight
			   != mp->lines[i].frontsector->floorheight)
		    {
		      AM_drawMline(&l, FDWALLCOLORS + lightlev); // floor level change
		    }
		  else if (mp->lines[i].backsector->ceilingheight
			   != mp->lines[i].frontsector->ceilingheight)
		    {
		      AM_drawMline(&l, CDWALLCOLORS+lightlev); // ceiling level change
		    }
		  else if (am_cheating)
		    {
		      AM_drawMline(&l, TSWALLCOLORS+lightlev);
		    }
		}
            }
        }
      else if (mpawn->powers[pw_allmap])
        {
          if (!(mp->lines[i].flags & ML_DONTDRAW)) AM_drawMline(&l, GRAYS+3);
        }
    }
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void AM_rotate(fixed_t *x, fixed_t *y, angle_t a)
{
  fixed_t tmpx =
    FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
    - FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);

  *y   =
    FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
    + FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

  *x = tmpx;
}


static void AM_drawLineCharacter(mline_t*    lineguy,
                                 int         lineguylines,
                                 fixed_t     scale,
                                 angle_t     angle,
                                 int         color,
                                 fixed_t     x,
                                 fixed_t     y)
{
  int         i;
  mline_t     l;

  for (i=0;i<lineguylines;i++)
    {
      l.a.x = lineguy[i].a.x;
      l.a.y = lineguy[i].a.y;

      if (scale)
        {
          l.a.x = FixedMul(scale, l.a.x);
          l.a.y = FixedMul(scale, l.a.y);
        }

      if (angle)
        AM_rotate(&l.a.x, &l.a.y, angle);

      l.a.x += x;
      l.a.y += y;

      l.b.x = lineguy[i].b.x;
      l.b.y = lineguy[i].b.y;

      if (scale)
        {
          l.b.x = FixedMul(scale, l.b.x);
          l.b.y = FixedMul(scale, l.b.y);
        }

      if (angle)
        AM_rotate(&l.b.x, &l.b.y, angle);

      l.b.x += x;
      l.b.y += y;

      AM_drawMline(&l, color);
    }
}



void AutoMap::drawPlayers()
{
  int n = mp->players.size();

  for (int i=0; i<n; i++)
    {
      PlayerPawn *p = mp->players[i]->pawn;
      if (p == NULL)
        continue;

      if (p == mpawn)
        {
          if (am_cheating)
            AM_drawLineCharacter(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
                                 p->angle, DWHITE, p->x, p->y);
          else
            AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 0,
                                 p->angle, DWHITE, p->x, p->y);
        }
      else
        {
          int color;

          if (p->powers[pw_invisibility])
            color = 246; // *close* to black
          else if (p->pres->color == 0)
            color = GREENS;
          else
            color = *(translationtables + ((p->pres->color-1)<<8) +GREENS+8);

          AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 0, p->angle,
                               color, p->x, p->y);
        }
    }
}



void AutoMap::drawThings(int colors, int colorrange)
{
  int    i;
  Actor *t;

  for (i=0; i < mp->numsectors; i++)
    {
      t = mp->sectors[i].thinglist;
      while (t)
        {
          AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
                               16<<FRACBITS, t->angle, colors+lightlev, t->x, t->y);
          t = t->snext;
        }
    }
}


void AutoMap::drawMarks()
{
  int i, fx, fy, w, h;

  for (i=0;i<AM_NUMMARKPOINTS;i++)
    {
      if (markpoints[i].x != -1)
        {
          //      w = SHORT(marknums[i]->width);
          //      h = SHORT(marknums[i]->height);
          w = 5; // because something's wrong with the wad, i guess
          h = 6; // because something's wrong with the wad, i guess
          fx = CXMTOF(markpoints[i].x);
          fy = CYMTOF(markpoints[i].y);
          if (fx >= f_x && fx <= f_w - w && fy >= f_y && fy <= f_h - h)
            marknums[i]->Draw(fx, fy, 0);
        }
    }
}


void AM_drawCrosshair(int color)
{
  if( rendermode!=render_soft )
    {
      // BP: should be putpixel here
      return;
    }

  if (vid.BytesPerPixel == 1)
    fb[(f_w*(f_h+1))/2] = color; // single point for now
  else
    *( (short *)fb + (f_w*(f_h+1))/2) = color;
}


// draws the Automap
void AutoMap::Drawer()
{
  if (!active)
    return;

  //CONS_Printf("AM::Drawer\n");
  // Map *mp and PlayerPawn *mpawn must now be set!

  clearFB(BACKGROUND);
  if (grid)
    drawGrid(GRIDCOLORS);
  drawWalls();
  drawPlayers();
  if (am_cheating == 2)
    drawThings(THINGCOLORS, THINGRANGE);

  AM_drawCrosshair(XHAIRCOLORS);

  drawMarks();

  // mapname
  {
    const char *mapname = mp->info->nicename.c_str();
    int y = vid.height - hud.stbarheight - 20;
    hud_font->DrawString(20, y - hud_font->StringHeight(mapname), mapname, V_SSIZE);
  }

  //CONS_Printf("AM::Drawer n\n");
  //V_MarkRect(f_x, f_y, f_w, f_h);
}
