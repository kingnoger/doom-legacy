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
// Revision 1.11  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.10  2003/11/12 11:07:25  smite-meister
// Serialization done. Map progression.
//
// Revision 1.9  2003/06/10 22:39:58  smite-meister
// Bugfixes
//
// Revision 1.8  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.7  2003/03/15 20:07:19  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.6  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.5  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/16 22:12:07  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:15:29  smite-meister
// Older update
//
//
// DESCRIPTION:
//      Intermission screens.
//
//-----------------------------------------------------------------------------

#include <vector>

#include "doomdef.h"
#include "wi_stuff.h"
#include "d_event.h"

#include "g_game.h"
#include "g_mapinfo.h"
#include "g_level.h"
#include "g_pawn.h"
#include "g_player.h"

#include "m_random.h"
#include "w_wad.h"

#include "r_state.h" // colormap etc.
#include "d_netcmd.h" // cvars

#include "s_sound.h"
#include "sounds.h"
#include "i_video.h"
#include "v_video.h"
#include "z_zone.h"
#include "console.h"

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES     4
#define NUMMAPS         9

// # of commercial levels
//static const int              NUMCMAPS = 32;
#define NUMCMAPS 32


// GLOBAL LOCATIONS
#define WI_TITLEY               2
#define WI_SPACINGY             16 //TODO: was 33

// SINGLE-PLAYER STUFF
#define SP_STATSX               50
#define SP_STATSY               50

#define SP_TIMEX                16
#define SP_TIMEY                (BASEVIDHEIGHT-32)


// NET GAME STUFF
#define NG_STATSY               50
#define NG_STATSX               (32 + SHORT(star->width)/2 + 32*!dofrags)

#define NG_SPACINGX             64


// DEATHMATCH STUFF
#define DM_MATRIXX              16
#define DM_MATRIXY              24

#define DM_SPACINGX             32

#define DM_TOTALSX              269

#define DM_KILLERSX             0
#define DM_KILLERSY             100
#define DM_VICTIMSX             5
#define DM_VICTIMSY             50
// in sec
#define DM_WAIT                 20


int Intermission::s_count = 0;

//treat all games in a unified way

typedef enum {
  PN_BGNAME,
  PN_SPLAT,
  PN_YAH,
  PN_LAST
} patchname_e;

// three sets of names so far: Doom, Doom II, Heretic

static char **pname;

static char *patchnames[3][PN_LAST] =
{
  {"WIMAP0",   "WISPLAT", "WIURH0"}, // Doom
  {"INTERPIC", "WISPLAT", "WIURH0"}, // Doom II
  {"MAPE1",    "IN_X",    "IN_YAH"}  // Heretic
};

typedef enum {
  ANIM_ALWAYS,
  ANIM_RANDOM,
  ANIM_LEVEL
} animenum_t;

struct point_t
{
  int x;
  int y;
};


//
// Animation.
// There is another anim_t used in p_spec.
//
struct anim_t
{
  animenum_t  type;

  // period in tics between animations
  int         period;

  // number of animation frames
  int         nanims;

  // location of animation
  point_t     loc;

  // ALWAYS: n/a,
  // RANDOM: period deviation (<256),
  // LEVEL: level
  int         data1;

  // ALWAYS: n/a,
  // RANDOM: random base period,
  // LEVEL: n/a
  int         data2;

  // actual graphics for frames of animations
  patch_t*    p[3];

  // following must be initialized to zero before use!

  // next value of bcount (used in conjunction with period)
  int         nexttic;

  // last drawn animation frame
  int         lastdrawn;

  // next frame number to animate
  int         ctr;

  // used by RANDOM and LEVEL when animating
  int         state;

};

static point_t DoomMapSpots[3][NUMMAPS] =
{
  // Episode 1 World Map
  {
    { 185, 164 },   // location of level 0 (CJ)
    { 148, 143 },   // location of level 1 (CJ)
    { 69, 122 },    // location of level 2 (CJ)
    { 209, 102 },   // location of level 3 (CJ)
    { 116, 89 },    // location of level 4 (CJ)
    { 166, 55 },    // location of level 5 (CJ)
    { 71, 56 },     // location of level 6 (CJ)
    { 135, 29 },    // location of level 7 (CJ)
    { 71, 24 }      // location of level 8 (CJ)
  },

  // Episode 2 World Map should go here
  {
    { 254, 25 },    // location of level 0 (CJ)
    { 97, 50 },     // location of level 1 (CJ)
    { 188, 64 },    // location of level 2 (CJ)
    { 128, 78 },    // location of level 3 (CJ)
    { 214, 92 },    // location of level 4 (CJ)
    { 133, 130 },   // location of level 5 (CJ)
    { 208, 136 },   // location of level 6 (CJ)
    { 148, 140 },   // location of level 7 (CJ)
    { 235, 158 }    // location of level 8 (CJ)
  },

  // Episode 3 World Map should go here
  {
    { 156, 168 },   // location of level 0 (CJ)
    { 48, 154 },    // location of level 1 (CJ)
    { 174, 95 },    // location of level 2 (CJ)
    { 265, 75 },    // location of level 3 (CJ)
    { 130, 48 },    // location of level 4 (CJ)
    { 279, 23 },    // location of level 5 (CJ)
    { 198, 48 },    // location of level 6 (CJ)
    { 140, 25 },    // location of level 7 (CJ)
    { 281, 136 }    // location of level 8 (CJ)
  }
};

// same for Heretic
static point_t HereticMapSpots[3][NUMMAPS] =
{
  {
    { 172, 78 },
    { 86, 90 },
    { 73, 66 },
    { 159, 95 },
    { 148, 126 },
    { 132, 54 },
    { 131, 74 },
    { 208, 138 },
    { 52, 101 }
  },
  {
    { 218, 57 },
    { 137, 81 },
    { 155, 124 },
    { 171, 68 },
    { 250, 86 },
    { 136, 98 },
    { 203, 90 },
    { 220, 140 },
    { 279, 106 }
  },
  {
    { 86, 99 },
    { 124, 103 },
    { 154, 79 },
    { 202, 83 },
    { 178, 59 },
    { 142, 58 },
    { 219, 66 },
    { 247, 57 },
    { 107, 80 }
  }
};


//
// Animation locations for Doom episodes 1, 2 and 3.
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_t epsd0animinfo[] =
{
  { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_t epsd1animinfo[] =
{
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
  { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_t epsd2animinfo[] =
{
  { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
  { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
  { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static int NUMANIMS[NUMEPISODES] =
{
  sizeof(epsd0animinfo)/sizeof(anim_t),
  sizeof(epsd1animinfo)/sizeof(anim_t),
  sizeof(epsd2animinfo)/sizeof(anim_t)
};

static anim_t *anims[NUMEPISODES] =
{
  epsd0animinfo,
  epsd1animinfo,
  epsd2animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB 0

// States for single-player
#define SP_KILLS                0
#define SP_ITEMS                2
#define SP_SECRET               4
#define SP_FRAGS                6
#define SP_TIME                 8
#define SP_PAR                  ST_TIME

#define SP_PAUSE                1

// in seconds
#define SHOWNEXTLOCDELAY        4


// the only instance of the Intermission handler class
Intermission wi;

//
//      GRAPHICS
//

// background (map of levels).
//static patch_t*       bg;
//static char             bgname[9];

// You Are Here graphic
static patch_t*         yah[2];

// splat
static patch_t*         splat;

// %, : graphics
static patch_t*         percent;
static patch_t*         colon;

// 0-9 graphic
static patch_t*         num[10];

// minus sign
static patch_t*         wiminus;

// "Finished!" graphics
static patch_t*         finished;

// "Entering" graphic
static patch_t*         entering;

// "secret"
static patch_t*         sp_secret;

// "Kills", "Scrt", "Items", "Frags"
static patch_t*         kills;
static patch_t*         secret;
static patch_t*         items;
static patch_t*         frags;

// Time sucks.
static patch_t*         timePatch;
static patch_t*         par;
static patch_t*         sucks;

// "killers", "victims"
static patch_t*         killers;
static patch_t*         victims;

// "Total", your face, your dead face
static patch_t*         ptotal;
static patch_t*         star;
static patch_t*         bstar;

//added:08-02-98: use STPB0 for all players, but translate the colors
static patch_t*         stpb;

// Name graphics of each level (centered)
static patch_t**        lnames;

//
// CODE
//


//========================================================================
// static drawer functions

static void WI_drawAnimatedBack(int ep)
{
  int                 i;
  anim_t*             a;

  if (game.mode == gm_doom2 || game.mode == gm_heretic)
    return;

  if (ep < 1 || ep > 3)
    return;

  for (i=0 ; i<NUMANIMS[ep-1] ; i++)
    {
      a = &anims[ep-1][i];

      if (a->ctr >= 0)
	V_DrawScaledPatch(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
    }
}


// Draws "<Levelname> Finished!"
static void WI_drawLF(const char *name, int last)
{
  int y = WI_TITLEY;

  // draw <LevelName>
  if (FontBBaseLump)
    {
      V_DrawTextB(name, (BASEVIDWIDTH - V_TextBWidth(name))/2, y);
      y += (5*V_TextBHeight(name))/4;
      V_DrawTextB("Finished", (BASEVIDWIDTH - V_TextBWidth("Finished"))/2, y);
    }
  else
    {
      // no font, use levelname patches instead
      V_DrawScaledPatch ((BASEVIDWIDTH - (lnames[last]->width))/2,
			 y, FB, lnames[last]);
      y += (5*(lnames[last]->height))/4;
      // draw "Finished!"
      V_DrawScaledPatch ((BASEVIDWIDTH - (finished->width))/2,
			 y, FB, finished);
    }
}

// Draws "Entering <LevelName>"
static void WI_drawEL(const char *nextname, int next)
{
  int y = WI_TITLEY;

  // draw "Entering"
  if( FontBBaseLump )
    {
      V_DrawTextB("Entering", (BASEVIDWIDTH - V_TextBWidth("Entering"))/2, y);
      y += (5*V_TextBHeight("Entering"))/4;
      V_DrawTextB(nextname, (BASEVIDWIDTH - V_TextBWidth(nextname))/2, y);
    }
  else
    {
      V_DrawScaledPatch((BASEVIDWIDTH - (entering->width))/2, y, FB, entering);

      // draw level
      y += (5*SHORT(lnames[next]->height))/4;

      V_DrawScaledPatch((BASEVIDWIDTH - (lnames[next]->width))/2, y, FB, lnames[next]);
    }
}

// this function only exists to choose between two different doom1 YAH patches!
/*
static void WI_drawOnLnode(int n, patch_t *c[])
{
  int         i;
  int         left;
  int         top;
  int         right;
  int         bottom;
  bool     fits = false;

  point_t     *lnodes;

  lnodes = &DoomMapSpots[episode-1][n];

  i = 0;
  do
    {
      left   = lnodes->x - (c[i]->leftoffset);
      top    = lnodes->y - (c[i]->topoffset);
      right  = left + (c[i]->width);
      bottom =  top + (c[i]->height);
      
      if (left >= 0
	  && right < BASEVIDWIDTH
	  && top >= 0
	  && bottom < BASEVIDHEIGHT)
        {
	  fits = true;
        }
      else
        {
	  i++;
        }
    } while (!fits && i!=2);

  if (fits && i<2)
    V_DrawScaledPatch(lnodes->x, lnodes->y, FB, c[i]);
  else
    // DEBUG
    CONS_Printf("Could not place patch on level %d\n", n+1);
}
*/

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

static int WI_drawNum(int x, int y, int n, int digits)
{

  int         fontwidth = (num[0]->width);
  int         neg;
  int         temp;

  if (digits < 0)
    {
      if (!n)
        {
	  // make variable-length zeros 1 digit long
	  digits = 1;
        }
      else
        {
	  // figure out # of digits in #
	  digits = 0;
	  temp = n;

	  while (temp)
            {
	      temp /= 10;
	      digits++;
            }
        }
    }

  neg = n < 0;
  if (neg)
    n = -n;

  // if non-number, do not draw it
  if (n == 1994)
    return 0;

  // draw the new number
  while (digits--)
    {
      x -= fontwidth;
      V_DrawScaledPatch(x, y, FB, num[ n % 10 ]);
      n /= 10;
    }

  // draw a minus sign if necessary
  if (neg)
    V_DrawScaledPatch(x-=8, y, FB, wiminus);

  return x;
}

static void WI_drawPercent(int x, int y, int p)
{
  if (p < 0)
    return;

  V_DrawScaledPatch(x, y, FB, percent);
  WI_drawNum(x, y, p, -1);
}

//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
static void WI_drawTime(int x, int y, int t)
{

  int         div;
  int         n;

  if (t<0)
    return;

  if (t <= 61*59)
    {
      div = 1;

      do
        {
	  n = (t / div) % 60;
	  x = WI_drawNum(x, y, n, 2) - (colon->width);
	  div *= 60;

	  // draw
	  if (div==60 || t / div)
	    V_DrawScaledPatch(x, y, FB, colon);

        } while (t / div);
    }
  else
    {
      // "sucks"
      V_DrawScaledPatch(x - SHORT(sucks->width), y, FB, sucks);
    }
}


//=============================================================
// Intermission class methods


// was WI_Responder
bool Intermission::Responder(event_t* ev)
{
  // FIXME: NOTE: formerly the ticker was used to detect keys
  //  because of timing issues in netgames.
  // Now each player can speed up his intermission stages
  // but the intermission can only be ended by the server.
  if (ev->type == ev_keydown)
    {
      acceleratestage = true;
      return true;
    }
  else
    return false;
}

// was WI_slamBackground
// slam background
void Intermission::SlamBackground()
{
  if (game.mode == gm_heretic && state == StatCount)
    V_DrawFlatFill(0, 0, vid.width/vid.dupx, vid.height/vid.dupy, fc.FindNumForName("FLOOR16"));
  else if (rendermode == render_soft) 
    {
      memcpy(vid.screens[0], vid.screens[1], vid.width * vid.height);
      //V_MarkRect (0, 0, vid.width, vid.height);
    }
  else 
    V_DrawScaledPatch(0, 0, 1+V_NOSCALESTART, fc.CachePatchName(interpic, PU_CACHE));
}


// was WI_initAnimatedBack
void Intermission::InitAnimatedBack()
{
  if (game.mode == gm_doom2 || game.mode == gm_heretic)
    return;

  if (episode < 1 || episode > 3)
    return;

  int         i;
  anim_t*     a;
  for (i=0; i<NUMANIMS[episode-1]; i++)
    {
      a = &anims[episode-1][i];

      // init variables
      a->ctr = -1;

      // specify the next time to draw it
      if (a->type == ANIM_ALWAYS)
	a->nexttic = bcount + 1 + (M_Random()%a->period);
      else if (a->type == ANIM_RANDOM)
	a->nexttic = bcount + 1 + a->data2+(M_Random()%a->data1);
      else if (a->type == ANIM_LEVEL)
	a->nexttic = bcount + 1;
    }
}

// was WI_updateAnimatedBack
void Intermission::UpdateAnimatedBack()
{
  int         i;
  anim_t*     a;

  if (game.mode == gm_doom2 || game.mode == gm_heretic)
    return;

  if (episode < 1 || episode > 3)
    return;

  for (i=0;i<NUMANIMS[episode-1];i++)
    {
      a = &anims[episode-1][i];

      if (bcount >= a->nexttic)
        {
	  switch (a->type)
            {
	    case ANIM_ALWAYS:
	      if (++a->ctr >= a->nanims) a->ctr = 0;
	      a->nexttic = bcount + a->period;
	      break;

	    case ANIM_RANDOM:
	      a->ctr++;
	      if (a->ctr == a->nanims)
                {
		  a->ctr = -1;
		  a->nexttic = bcount+a->data2+(M_Random()%a->data1);
                }
	      else a->nexttic = bcount + a->period;
	      break;

	    case ANIM_LEVEL:
	      // gawd-awful hack for level anims
	      if (!(state == StatCount && i == 7)
		  && next == a->data1)
                {
		  a->ctr++;
		  if (a->ctr == a->nanims) a->ctr--;
		  a->nexttic = bcount + a->period;
                }
	      break;
            }
        }
    }
}


// was WI_DrawYAH
void Intermission::DrawYAH()
{
  int ep = (episode - 1) % 3;
  // draws splats and "you are here" marker
  int i;

  point_t (*mapspots)[NUMMAPS] = DoomMapSpots;
  if (game.mode == gm_heretic)
    mapspots = HereticMapSpots;

  // this REQUIRES that the levelnodes are in an array.
  /*
    // TODO put the intermission splats back sometime
  for (i = 0; i < 9; i++)
    if (firstlevel[i].done)
      V_DrawScaledPatch(mapspots[ep][i].x, mapspots[ep][i].y, 0, splat);
  */

  //  if(!(bcount&16) || state == ShowNextLoc) // strange Heretic thingy
  // draw flashing ptr
  if (pointeron) // draw the destination 'X' 
    V_DrawScaledPatch(mapspots[ep][next].x, mapspots[ep][next].y, 0, yah[0]);
}


// was WI_initNoState
// used for write introduce next level
void Intermission::InitNoState()
{
  state = NoState;
  acceleratestage = false;
  count = 10;
  pointeron = true;
}


// was WI_initDeathmatchStats
void Intermission::InitDMStats()
{
  // get the frag tables
  nplayers = game.GetFrags(&dm_score[0], 0);
  game.GetFrags(&dm_score[1], 1);
  game.GetFrags(&dm_score[2], 2);
  game.GetFrags(&dm_score[3], 3);

  // sort all scores
  int i, j, k, max;
  fragsort_t temp;

  for (k = 0; k < 4; k++)
    for (j=0; j<nplayers; j++)
      {
	max = j;    
	for (i=j+1; i<nplayers; i++)
	  if (dm_score[k][i].count > dm_score[k][max].count)
	    max = i;
	if (max != j)
	  {
	    temp = dm_score[k][j];
	    dm_score[k][j] = dm_score[k][max];
	    dm_score[k][max] = temp;
	  }
      }

  count = TICRATE*DM_WAIT;
}

// was WI_updateDeathmatchStats
void Intermission::UpdateDMStats()
{
  if (game.paused)
    return;

  if (--count <= 0)
    {
      S_StartAmbSound(sfx_gib);
      InitNoState();
    }
}

// Draw a column of rankings stored in fragtable
//  Quick-patch for the Cave party 19-04-1998 !!
void WI_drawRanking(const char *title, int x, int y, fragsort_t *fragtable,
                    int scorelines, bool large, int white)
{
  int   i;
  int   colornum;

  if (game.mode == gm_heretic)
    colornum = 230;
  else
    colornum = 0x78;

  if (title != NULL)
    V_DrawString(x, y-14, 0, title);

  // draw rankings
  int   plnum;
  int   frags;
  int   color;
  char  num[12];
  extern byte *translationtables;

  for (i=0; i<scorelines; i++)
    {
      frags = fragtable[i].count;
      plnum = fragtable[i].num;

      // draw color background
      color = fragtable[i].color;
      if (!color)
	color = *((byte *)colormaps + colornum);
      else
	color = *((byte *)translationtables - 256 + (color<<8) + colornum);
      V_DrawFill(x-1, y-1, large ? 40 : 26, 9, color);

      // draw frags count
      sprintf(num, "%3i", frags);
      V_DrawString (x+(large ? 32 : 24)-V_StringWidth(num), y, 0, num);

      // draw name
      V_DrawString (x+(large ? 64 : 29), y, (plnum == white) ? V_WHITEMAP : 0, fragtable[i].name);

      y += 12;
      if (y>=BASEVIDHEIGHT)
	break;            // dont draw past bottom of screen
    }
}

// was WI_drawDeathmatchStats
void Intermission::DrawDMStats()
{
#define RANKINGY 60

  char *timeleft;

  int white = -1;
  //Fab:25-04-98: when you play, you quickly see your frags because your
  //  name is displayed white, when playback demo, you quicly see who's the
  //  view.
  // TODO: splitscreen... another color?
  if (cv_teamplay.value)
    white = demoplayback ? displayplayer->team : consoleplayer->team;
  else
    white = demoplayback ? displayplayer->number : consoleplayer->number;

  // count frags for each present player
  WI_drawRanking("Frags", 5, RANKINGY, dm_score[0], nplayers, false, white);

  // count buchholz
  WI_drawRanking("Buchholz",85,RANKINGY, dm_score[1],nplayers,false, white);

  // count individual
  WI_drawRanking("Indiv.",165,RANKINGY, dm_score[2],nplayers,false, white);

  // count deads
  WI_drawRanking("Deaths",245,RANKINGY, dm_score[3],nplayers,false, white);

  timeleft = va("start in %d", count/TICRATE);
  V_DrawString (200, 30, V_WHITEMAP, timeleft);
}


/* old code
static void WI_ddrawDeathmatchStats()
{

    int         i;
    int         j;
    int         x;
    int         y;
    int         w;

    int         lh;     // line height

    byte*       colormap;       //added:08-02-98:see below

    lh = WI_SPACINGY;

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();
    WI_drawLF();

    // draw stat titles (top line)
    V_DrawScaledPatch(DM_TOTALSX-SHORT(total->width)/2,
                DM_MATRIXY-WI_SPACINGY+10,
                FB,
                total);

    V_DrawScaledPatch(DM_KILLERSX, DM_KILLERSY, FB, killers);
    V_DrawScaledPatch(DM_VICTIMSX, DM_VICTIMSY, FB, victims);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            //added:08-02-98: use V_DrawMappedPatch instead of
            //                    V_DrawScaledPatch, so that the
            // graphics are 'colormapped' to the player's colors!
            if (players[i].skincolor==0)
                colormap = colormaps;
            else
                colormap = (byte *) translationtables - 256 + (players[i].skincolor<<8);

            V_DrawMappedPatch(x-SHORT(stpb->width)/2,
                        DM_MATRIXY - WI_SPACINGY,
                        FB,
                        stpb,      //p[i], now uses a common STPB0 translated
                        colormap); //      to the right colors

            V_DrawMappedPatch(DM_MATRIXX-SHORT(stpb->width)/2,
                        y,
                        FB,
                        stpb,      //p[i]
                        colormap);

            if (i == me)
            {
                V_DrawScaledPatch(x-SHORT(stpb->width)/2,
                            DM_MATRIXY - WI_SPACINGY,
                            FB,
                            bstar);

                V_DrawScaledPatch(DM_MATRIXX-SHORT(stpb->width)/2,
                            y,
                            FB,
                            star);
            }
        }
        else
        {
            // V_DrawPatch(x-SHORT(bp[i]->width)/2,
            //   DM_MATRIXY - WI_SPACINGY, FB, bp[i]);
            // V_DrawPatch(DM_MATRIXX-SHORT(bp[i]->width)/2,
            //   y, FB, bp[i]);
        }
        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = SHORT(num[0]->width);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
            {
                if (playeringame[j])
                    WI_drawNum(x+w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }
            WI_drawNum(DM_TOTALSX+w, y, dm_totals[i], 2);
        }
        y += WI_SPACINGY;
    }
}
*/

// was WI_initStats
// was WI_initNetgameStats
void Intermission::InitCoopStats()
{
  int i, n = game.Players.size();

  ng_state = 1;
  count = TICRATE;
  cnt_time = cnt_par = -1;

  // resize the counters
  cnt.resize(n);
  plrs.resize(n);
  // TODO what about when a player joins during intermission?
  // he needs a new counter struct!
  // right now he is not shown at all.

  dofrags = false;
  map<int, PlayerInfo *>::iterator u;
  for (i = 0, u = game.Players.begin(); u != game.Players.end(); i++, u++)
    {
      cnt[i].kills = cnt[i].items = cnt[i].secrets = cnt[i].frags = 0;
      plrs[i] = (*u).second;
      dofrags = dofrags || (plrs[i]->score != 0);
    }
}


/*
static void WI_updateStats()
{

  if (acceleratestage && sp_state != 10)
    {
      acceleratestage = false;
      cnt_kills[0] = (plrs[me].kills * 100) / level->kills;
      cnt_items[0] = (plrs[me].items * 100) / level->items;
      cnt_secret[0] = (plrs[me].secrets * 100) / level->secrets;
      cnt_time = plrs[me].stime / TICRATE;
      cnt_par = level->partime / TICRATE;
      S_StartAmbSound(sfx_barexp);
      sp_state = 10;
    }

  if (sp_state == 2)
    {
      cnt_kills[0] += 2;

      if (!(bcount&3))
	S_StartAmbSound(s_count);

      if (cnt_kills[0] >= (plrs[me].kills * 100) / level->kills)
        {
	  cnt_kills[0] = (plrs[me].kills * 100) / level->kills;
	  S_StartAmbSound(sfx_barexp);
	  sp_state++;
        }
    }
  else if (sp_state == 4)
    {
      cnt_items[0] += 2;

      if (!(bcount&3))
	S_StartAmbSound(s_count);

      if (cnt_items[0] >= (plrs[me].items * 100) / level->items)
        {
	  cnt_items[0] = (plrs[me].items * 100) / level->items;
	  S_StartAmbSound(sfx_barexp);
	  sp_state++;
        }
    }
  else if (sp_state == 6)
    {
      cnt_secret[0] += 2;

      if (!(bcount&3))
	S_StartAmbSound(s_count);

      if (cnt_secret[0] >= (plrs[me].secrets * 100) / level->secrets)
        {
	  cnt_secret[0] = (plrs[me].secrets * 100) / level->secrets;
	  S_StartAmbSound(sfx_barexp);
	  sp_state++;
        }
    }

  else if (sp_state == 8)
    {
      if (!(bcount&3))
	S_StartAmbSound(s_count);

      cnt_time += 3;

      if (cnt_time >= plrs[me].stime / TICRATE)
	cnt_time = plrs[me].stime / TICRATE;

      cnt_par += 3;

      if (cnt_par >= level->partime / TICRATE)
        {
	  cnt_par = level->partime / TICRATE;

	  if (cnt_time >= plrs[me].stime / TICRATE)
            {
	      S_StartAmbSound(sfx_barexp);
	      sp_state++;
            }
        }
    }
  else if (sp_state == 10)
    {
      if (acceleratestage)
        {
	  S_StartAmbSound(sfx_sgcock);

	  if (game.mode == gm_doom2)
	    WI_initNoState();
	  else
	    WI_initShowNextLoc();
        }
    }
  else if (sp_state & 1)
    {
      if (!--cnt_pause)
        {
	  sp_state++;
	  cnt_pause = TICRATE;
        }
    }

}
*/
// was WI_updateNetgameStats
// was WI_updateStats
void Intermission::UpdateCoopStats()
{
  bool finished = true;

  int i, n = plrs.size();

  if (acceleratestage && ng_state != 12)
    {
      acceleratestage = false;

      for (i=0 ; i<n ; i++)
        {
	  //if (!playeringame[i]) continue;

	  cnt[i].kills = (plrs[i]->kills * 100) / total.kills;
	  cnt[i].items = (plrs[i]->items * 100) / total.items;
	  cnt[i].secrets = (plrs[i]->secrets * 100) / total.secrets;

	  if (dofrags) cnt[i].frags = plrs[i]->score;
        }
      cnt_time = time;
      cnt_par = partime;

      S_StartAmbSound(sfx_barexp);
      ng_state = 12;
    }

  if (ng_state == 2)
    {
      // count kills
      if (!(bcount&3))
	S_StartAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
	  //if (!playeringame[i]) continue;

	  cnt[i].kills += 2;

	  if (cnt[i].kills >= (plrs[i]->kills * 100) / total.kills)
	    cnt[i].kills = (plrs[i]->kills * 100) / total.kills;
	  else
	    finished = false;
        }

      if (finished)
        {
	  S_StartAmbSound(sfx_barexp);
	  ng_state++;
        }
    }
  else if (ng_state == 4)
    {
      // count items
      if (!(bcount&3))
	S_StartAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
	  //if (!playeringame[i]) continue;

	  cnt[i].items += 2;
	  if (cnt[i].items >= (plrs[i]->items * 100) / total.items)
	    cnt[i].items = (plrs[i]->items * 100) / total.items;
	  else
	    finished = false;
        }
      if (finished)
        {
	  S_StartAmbSound(sfx_barexp);
	  ng_state++;
        }
    }
  else if (ng_state == 6)
    {
      // count secrets
      if (!(bcount&3))
	S_StartAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
	  //if (!playeringame[i]) continue;

	  cnt[i].secrets += 2;

	  if (cnt[i].secrets >= (plrs[i]->secrets * 100) / total.secrets)
	    cnt[i].secrets = (plrs[i]->secrets * 100) / total.secrets;
	  else
	    finished = false;
        }

      if (finished)
        {
	  S_StartAmbSound(sfx_barexp);
	  ng_state += 1 + 2*!dofrags;
        }
    }
  else if (ng_state == 8)
    {
      // count frags
      if (!(bcount&3))
	S_StartAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
	  //if (!playeringame[i]) continue;
	  int  fsum;

	  cnt[i].frags += 1;

	  if (cnt[i].frags >= (fsum = plrs[i]->score))
	    cnt[i].frags = fsum;
	  else
	    finished = false;
        }

      if (finished)
        {
	  S_StartAmbSound(sfx_pldeth);
	  ng_state++;
        }
    }
  else if (ng_state == 10)
    {
      // count time and partime
      if (!(bcount&3))
	S_StartAmbSound(s_count);

      cnt_time += 3;

      if (cnt_time >= time)
	cnt_time = time;
      else
	finished = false;

      cnt_par += 3;

      if (cnt_par >= partime)
        {
	  cnt_par = partime;

	  if (finished)
            {
	      S_StartAmbSound(sfx_barexp);
	      ng_state++;
            }
        }
    }
  else if (ng_state == 12)
    {
      // wait for a keypress
      if (acceleratestage)
        {
	  S_StartAmbSound(sfx_sgcock);

	  if (game.mode == gm_doom2)
	    InitNoState();
	  else
	    {
	      state = ShowNextLoc;
	      acceleratestage = false;
	      count = SHOWNEXTLOCDELAY * TICRATE;
	    }
        }
    }
  else if (ng_state & 1)
    {
      // pause for a while between counts
      if (--count <= 0)
        {
	  ng_state++;
	  count = TICRATE;
        }
    }
}



// was WI_drawStats
// was WI_drawNetgameStats()
void Intermission::DrawCoopStats()
{
  if (!game.multiplayer)
    {
      // single player stats are a bit different
      // line height
      int lh = (3*(num[0]->height))/2;

      if (FontBBaseLump)
	{
	  // use FontB if any
	  V_DrawTextB("Kills", SP_STATSX, SP_STATSY);
	  V_DrawTextB("Items", SP_STATSX, SP_STATSY+lh);
	  V_DrawTextB("Secrets", SP_STATSX, SP_STATSY+2*lh);
	}
      else
	{
	  V_DrawScaledPatch(SP_STATSX, SP_STATSY, FB, kills);
	  V_DrawScaledPatch(SP_STATSX, SP_STATSY+lh, FB, items);
	  V_DrawScaledPatch(SP_STATSX, SP_STATSY+2*lh, FB, sp_secret);
	}
      WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY, cnt[0].kills);
      WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+lh, cnt[0].items);
      WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt[0].secrets);
    }
  else
    {
      int x, y;

      // draw stat titles (top line)
      if (FontBBaseLump)
	{
	  // use FontB if any
	  V_DrawTextB("Kills", NG_STATSX+  NG_SPACINGX-V_TextBWidth("Kills"), NG_STATSY);
	  V_DrawTextB("Items", NG_STATSX+2*NG_SPACINGX-V_TextBWidth("Items"), NG_STATSY);
	  V_DrawTextB("Scrt", NG_STATSX+3*NG_SPACINGX-V_TextBWidth("Scrt"), NG_STATSY);
	  if (dofrags)
	    V_DrawTextB("Frgs", NG_STATSX+4*NG_SPACINGX-V_TextBWidth("Frgs"), NG_STATSY);

	  y = NG_STATSY + V_TextBHeight("Kills");
	}
      else
	{
	  V_DrawScaledPatch(NG_STATSX+NG_SPACINGX-SHORT(kills->width), NG_STATSY, FB, kills);
	  V_DrawScaledPatch(NG_STATSX+2*NG_SPACINGX-SHORT(items->width), NG_STATSY, FB, items);
	  V_DrawScaledPatch(NG_STATSX+3*NG_SPACINGX-SHORT(secret->width), NG_STATSY, FB, secret);
	  if (dofrags)
	    V_DrawScaledPatch(NG_STATSX+4*NG_SPACINGX-SHORT(frags->width), NG_STATSY, FB, frags);

	  y = NG_STATSY + SHORT(kills->height);
	}
      // draw stats
      extern byte *translationtables;
      byte* colormap;
      int i, n = plrs.size();
      int pwidth = SHORT(percent->width);

      for (i=0 ; i<n ; i++)
	{
	  // TODO draw names too, use a smaller font?
	  // draw face
	  int color = plrs[i]->color;

	  x = NG_STATSX - (i & 1) ? 10 : 0;
	  if (color == 0)
	    colormap = colormaps; //no translation table for green guy
	  else
	    colormap = (byte *) translationtables - 256 + (color << 8);

	  V_DrawMappedPatch(x-SHORT(stpb->width), y, FB, stpb, colormap);

	  // TODO splitscreen
	  if (plrs[i]->number == consoleplayer->number)
	    V_DrawScaledPatch(x-SHORT(stpb->width), y, FB, star);

	  // draw stats
	  x = NG_STATSX + NG_SPACINGX;
	  WI_drawPercent(x-pwidth, y+10, cnt[i].kills);   x += NG_SPACINGX;
	  WI_drawPercent(x-pwidth, y+10, cnt[i].items);   x += NG_SPACINGX;
	  WI_drawPercent(x-pwidth, y+10, cnt[i].secrets);  x += NG_SPACINGX;

	  if (dofrags)
	    WI_drawNum(x, y+10, cnt[i].frags, -1);

	  y += WI_SPACINGY;
	}
    }
  // draw time and par
  if (FontBBaseLump)
    {
      V_DrawTextB("Time", SP_TIMEX, SP_TIMEY);
      // if (episode < 4 && game.mode!=gm_heretic)
      V_DrawTextB("Par", BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY);
    }
  else
    {
      V_DrawScaledPatch(SP_TIMEX, SP_TIMEY, FB, timePatch);
      //if (episode < 4 && game.mode!=gm_heretic)
      V_DrawScaledPatch(BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY, FB, par);
    }
  WI_drawTime(BASEVIDWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);
  // if (episode < 4 && game.mode!=gm_heretic)
  WI_drawTime(BASEVIDWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);

}


// returns true if state should be accelerated
/*
// see Intermission::Responder
static bool WI_checkForAccelerate()
{
  bool acc = false;

  // check for button presses to skip delays
  // each player does this on his own
  // n

  for (i=0; i<n ; i++)
    //(player = players ;; player++)
    {  
      if (game.players[i]->cmd.buttons & BT_ATTACK)
	{
	  if (!game.players[i]->pawn->attackdown)
	    acceleratestage = true;
	  game.players[i]->pawn->attackdown = true;
	}
      else
	game.players[i]->pawn->attackdown = false;
      if (game.players[i]->cmd.buttons & BT_USE)
	{
	  if (!game.players[i]->pawn->usedown)
	    acceleratestage = true;
	  game.players[i]->pawn->usedown = true;
	}
      else
	game.players[i]->pawn->usedown = false;
    }

  return acc;
}
*/

// was WI_loadData
void Intermission::LoadData()
{  
  switch (game.mode)
    {
    case gm_doom2:
      pname = patchnames[1]; // use Doom II patchnames
      break;
    case gm_heretic:
      pname = patchnames[2]; // use Heretic patchnames
      break;
    default:
      pname = patchnames[0]; // use Doom patchnames
    }

  if (rendermode == render_soft)
    {
      memset(vid.screens[0], 0, vid.width*vid.height*vid.BytesPerPixel);
      // clear backbuffer from status bar stuff and borders
      memset(vid.screens[1], 0, vid.width*vid.height*vid.BytesPerPixel); 
      // background stored in backbuffer        
      V_DrawScaledPatch(0, 0, 1, fc.CachePatchName(interpic, PU_CACHE));
    }

  // UNUSED unsigned char *pic = vid.screens[1];
  // if (game.mode == gm_doom2)
  // {
  // darken the background image
  // while (pic != vid.screens[1] + SCREENHEIGHT*SCREENWIDTH)
  // {
  //   *pic = colormaps[256*25 + *pic];
  //   pic++;
  // }
  //}

  int     i, j;
  char name[9];

  switch (game.mode)
    {
    case gm_doom2:
      // NUMCMAPS = 32;
      // level name patches
      lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMCMAPS, PU_STATIC, 0);
      for (i=0 ; i<NUMCMAPS ; i++)
	{
	  sprintf(name, "CWILV%2.2d", i);
	  lnames[i] = fc.CachePatchName(name, PU_STATIC);
	}
      break;
    case gm_doom1s:
    case gm_doom1:
    case gm_udoom:
      // Doom 1 intermission animations
      anim_t *a;
      if (episode >= 1)
	{
	  if (episode <= 3)
	    for (j = 0; j < NUMANIMS[episode-1]; j++)
	      {
		a = &anims[episode-1][j];
		for (i = 0; i < a->nanims; i++)
		  {
		    // MONDO HACK! There is no special picture of the
		    // Tower of Babel for the secret level of Doom 1 ep. 2.
		    if (episode == 2 && j == 8)
		      {
			// HACK ALERT!
			a->p[i] = anims[1][4].p[i];
		      }
		    else
		      {
			// animations
			sprintf(name, "WIA%d%.2d%.2d", episode-1, j, i);
			a->p[i] = fc.CachePatchName(name, PU_STATIC);
		      }
		  }
	      }

	  // level name patches
	  lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMMAPS, PU_STATIC, 0);
	  for (i=0 ; i<NUMMAPS ; i++)
	    {
	      sprintf(name, "WILV%d%d", episode-1, i);
	      lnames[i] = fc.CachePatchName(name, PU_STATIC);
	    }
	}

      // fallthru
    case gm_heretic:
      // you are here
      //yah[0] = fc.CachePatchName(game.mode == gm_heretic ? "IN_YAH" : "WIURH0", PU_STATIC);
      yah[0] = fc.CachePatchName(pname[PN_YAH], PU_STATIC);

      // you are here (alt.)
      yah[1] = fc.CachePatchName("WIURH1", PU_STATIC);

      // splat
      //splat = fc.CachePatchName(game.mode == gm_heretic ? "IN_X" : "WISPLAT", PU_STATIC);
      splat = fc.CachePatchName(pname[PN_SPLAT], PU_STATIC);
      break;
    default:
      break;
    }

  // TODO! these could use the new Hud widgets! the font is then cached just once!
  // More hacks on minus sign.
  wiminus = fc.CachePatchName(game.mode == gm_heretic ? "FONTB13" : "WIMINUS", PU_STATIC);

  for (i=0;i<10;i++)
    {
      // numbers 0-9
      if( game.mode == gm_heretic )
	sprintf(name, "FONTB%d", 16+i);
      else
	sprintf(name, "WINUM%d", i);
      num[i] = fc.CachePatchName(name, PU_STATIC);
    }

  // percent sign
  percent = fc.CachePatchName(game.mode == gm_heretic ? "FONTB05" : "WIPCNT", PU_STATIC);

  if (game.mode != gm_heretic)
    {
      // "finished"
      finished = fc.CachePatchName("WIF", PU_STATIC);
        
      // "entering"
      entering = fc.CachePatchName("WIENTER", PU_STATIC);
        
      // "kills"
      kills = fc.CachePatchName("WIOSTK", PU_STATIC);
        
      // "scrt"
      secret = fc.CachePatchName("WIOSTS", PU_STATIC);
        
      // "secret"
      sp_secret = fc.CachePatchName("WISCRT2", PU_STATIC);
        
      // "items"
      items = fc.CachePatchName("WIOSTI", PU_STATIC);
        
      // "frgs"
      frags = fc.CachePatchName("WIFRGS", PU_STATIC);
        
      // "time"
      timePatch = fc.CachePatchName("WITIME", PU_STATIC);
        
      // "sucks"
      sucks = fc.CachePatchName("WISUCKS", PU_STATIC);
        
      // "par"
      par = fc.CachePatchName("WIPAR", PU_STATIC);

      // "killers" (vertical)
      killers = fc.CachePatchName("WIKILRS", PU_STATIC);
        
      // "victims" (horiz)
      victims = fc.CachePatchName("WIVCTMS", PU_STATIC);
        
      // "total"
      ptotal = fc.CachePatchName("WIMSTT", PU_STATIC);
    }
    
  // ":"
  colon = fc.CachePatchName(game.mode == gm_heretic ? "FONTB26" : "WICOLON", PU_STATIC);

  // your face
  star = fc.CachePatchName("STFST01", PU_STATIC);

  // dead face
  bstar = fc.CachePatchName("STFDEAD0", PU_STATIC);


  //added:08-02-98: now uses a single STPB0 which is remapped to the
  //                player translation table. Whatever new colors we add
  //                since we'll have to define a translation table for
  //                it, we'll have the right colors here automatically.
  stpb = fc.CachePatchName("STPB0", PU_STATIC);
}

// was WI_unloadData
void Intermission::UnloadData()
{
  int i, j;

  //faB: never Z_ChangeTag() a pointer returned by fc.CachePatchxxx()
  //     it doesn't work and is unecessary
  if (rendermode == render_soft)
    {
      Z_ChangeTag(wiminus, PU_CACHE);

      for (i=0 ; i<10 ; i++)
        Z_ChangeTag(num[i], PU_CACHE);

      switch (game.mode)
	{
	case gm_doom2:
	  for (i=0 ; i<NUMCMAPS ; i++)
            Z_ChangeTag(lnames[i], PU_CACHE);
	  Z_Free(lnames);
	  break;

	case gm_doom1s:
	case gm_doom1:
	case gm_udoom:
	  // Doom 1
	  if (episode >= 1)
	    {
	      for (i=0 ; i<NUMMAPS ; i++)
		Z_ChangeTag(lnames[i], PU_CACHE);
	      Z_Free(lnames);
	      
	      if (episode <= 3)
		{
		  for (j=0;j<NUMANIMS[episode-1];j++)
		    {
		      if (episode != 2 || j != 8)
			for (i=0;i<anims[episode-1][j].nanims;i++)
			  Z_ChangeTag(anims[episode-1][j].p[i], PU_CACHE);
		    }
		}
	    }
	  // fallthru
	case gm_heretic:
	  Z_ChangeTag(yah[0], PU_CACHE);
	  Z_ChangeTag(yah[1], PU_CACHE);
	  Z_ChangeTag(splat, PU_CACHE);
	  break;
	default:
	  break;
	}
    }

  if (rendermode==render_soft)
    {
      Z_ChangeTag(percent, PU_CACHE);
      Z_ChangeTag(colon, PU_CACHE);

      if (game.mode != gm_heretic)
        {
	  Z_ChangeTag(finished, PU_CACHE);
	  Z_ChangeTag(entering, PU_CACHE);
	  Z_ChangeTag(kills, PU_CACHE);
	  Z_ChangeTag(secret, PU_CACHE);
	  Z_ChangeTag(sp_secret, PU_CACHE);
	  Z_ChangeTag(items, PU_CACHE);
	  Z_ChangeTag(frags, PU_CACHE);
	  Z_ChangeTag(timePatch, PU_CACHE);
	  Z_ChangeTag(sucks, PU_CACHE);
	  Z_ChangeTag(par, PU_CACHE);
            
	  Z_ChangeTag(victims, PU_CACHE);
	  Z_ChangeTag(killers, PU_CACHE);
	  Z_ChangeTag(ptotal, PU_CACHE);
        }
    }
}

// was WI_End
void Intermission::End()
{
  UnloadData();

  int i;

  if (cv_deathmatch.value)
    {
      for (i=0; i<4; i++)
	delete [] dm_score[i]; 
    }
  else
    {
      plrs.resize(0);
    }

  game.EndIntermission();
}

// was WI_Drawer
void Intermission::Drawer()
{
  SlamBackground();
  // draw animated background
  WI_drawAnimatedBack(episode);

  switch (state)
    {
    case StatCount:
      // draw "<level> finished"
      WI_drawLF(lastlevelname, last);

      if (cv_deathmatch.value)
        {
	  //if(cv_teamplay.value)
	  //WI_drawTeamsStats();
	  //else
	  DrawDMStats();
        }
      else
	DrawCoopStats();
      break;

    case ShowNextLoc:
    case NoState:
      // was WI_drawShowNextLoc
      if (count <= 0)  // all removed no draw !!!
	return;

      // FIXME! what about Ultimate Doom (gm_udoom)?
      if (game.mode != gm_doom2 && episode > 0)
	DrawYAH();

      // draws which level you are entering..
      if (game.mode != gm_doom2 || next != 30)
	WI_drawEL(nextlevelname, next);
      break;
    }
}


// was WI_Ticker
// Updates stuff each tick
void Intermission::Ticker()
{
  // counter for general background animation
  bcount++;

  if (bcount == 1)
    {
      int mus;
      // intermission music
      // FIXME make choice based on LevelInfo, not game.mode...
      switch (game.mode)
	{
	case gm_doom2:
          mus = mus_dm2int;
	  break;
        case gm_heretic:
	  mus = mus_hintr;
	  break;
	default:
          mus = mus_inter;
	  break;
	}
      S_StartMusic(mus, true);
    }

  //WI_checkForAccelerate();

  UpdateAnimatedBack();

  switch (state)
    {
    case StatCount:
      if (cv_deathmatch.value)
	UpdateDMStats();
      else
	UpdateCoopStats();
      break;

    case ShowNextLoc:
      // was WI_updateShowNextLoc
      if (!--count || acceleratestage)
	InitNoState();
      else
	pointeron = (count & 31) < 20;
      break;

    case NoState:
      // was WI_updateNoState
      // TODO here we wait until the server sends a startlevel command
      // intermission is not ended before that
      if (!--count)
	End();
      break;
    }
}

void Intermission::Start(const MapInfo *l, const MapInfo *n)
{  
  last = l->mapnumber; // number of level just completed
  next = n->mapnumber; // number of next level

  partime = l->partime;
  lastlevelname = l->nicename.c_str();
  nextlevelname = n->nicename.c_str();

  acceleratestage = false;
  count = bcount = 0;
  state = StatCount;

  MapCluster *m = game.FindCluster(l->cluster);
  episode = m->episode;
  interpic = m->interpic.c_str();

  time = m->time;

  total.kills = m->kills;
  if (total.kills == 0)
    total.kills = 1;

  total.items = m->items;
  if (total.items == 0)
    total.items = 1;

  total.secrets = m->secrets;
  if (total.secrets == 0)
    total.secrets = 1;

  LoadData();

  if (cv_deathmatch.value)
    InitDMStats();
  else
    InitCoopStats();

  InitAnimatedBack();
}
