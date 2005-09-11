// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2005 by DooM Legacy Team.
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
// Revision 1.28  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.27  2005/07/18 12:31:23  smite-meister
// cross-cluster mapchanges
//
// Revision 1.26  2005/06/12 16:26:27  smite-meister
// alpha2 bugfixes
//
// Revision 1.25  2005/04/17 18:36:34  smite-meister
// netcode
//
// Revision 1.24  2005/03/19 13:51:29  smite-meister
// sound samplerate fix
//
// Revision 1.23  2004/11/19 16:51:04  smite-meister
// cleanup
//
// Revision 1.22  2004/11/18 20:30:12  smite-meister
// tnt, plutonia
//
// Revision 1.21  2004/11/13 22:38:43  smite-meister
// intermission works
//
// Revision 1.20  2004/11/09 20:38:51  smite-meister
// added packing to I/O structs
//
// Revision 1.19  2004/10/27 17:37:07  smite-meister
// netcode update
//
// Revision 1.18  2004/09/23 23:21:17  smite-meister
// HUD updated
//
// Revision 1.17  2004/08/13 18:25:10  smite-meister
// sw renderer fix
//
// Revision 1.16  2004/08/12 18:30:26  smite-meister
// cleaned startup
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Intermission screens.

#include <vector>

#include "doomdef.h"
#include "command.h"
#include "cvars.h"
#include "console.h"

#include "d_event.h"
#include "wi_stuff.h"
#include "hud.h"

#include "g_game.h"
#include "g_level.h"
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_pawn.h"
#include "g_player.h"

#include "m_random.h"
#include "r_main.h" // colormap etc.
#include "r_data.h"

#include "sounds.h"
#include "s_sound.h"
#include "i_video.h"
#include "v_video.h"
#include "screen.h"

#include "w_wad.h"
#include "z_zone.h"



#define FB  (0 | V_SCALE)



#define NUMEPISODES     4
#define NUMMAPS         9


// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.

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
#define NG_STATSX               (32 + star->width/2 + 32 * !dofrags)

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



// the only instance of the Intermission handler class
Intermission wi;


//================================================================
//    Animations and map locations
//================================================================

enum animenum_t
{
  ANIM_ALWAYS,
  ANIM_RANDOM,
  ANIM_LEVEL
};

struct point_t
{
  int x;
  int y;
};


/// Intermission background animation.
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
  Texture*    p[3];

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


//================================================================
//       GENERAL DATA
//================================================================


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


//
//      GRAPHICS
//

// background (map of levels).
static Texture*         intermission_bg;

// You Are Here graphic
static Texture*         yah[2];

// splat
static Texture*         splat;

// %, : graphics
static Texture  *percent,  *colon;


// 0-9 graphics, minus sign
static Texture*         num[11];

// "Finished!" graphics
static Texture*         finished;

// "Entering" graphic
static Texture*         entering;

// "secret"
static Texture*         sp_secret;

// "Kills", "Scrt", "Items", "Frags"
static Texture*         kills;
static Texture*         secret;
static Texture*         items;
static Texture*         frags;

// Time sucks.
static Texture*         timePatch;
static Texture*         par;
static Texture*         sucks;

// "killers", "victims"
static Texture*         killers;
static Texture*         victims;

// "Total"
static Texture*         ptotal;

// your face, your dead face, face background
static Texture*         star;
static Texture*         bstar;
static Texture*         stpb;

// Name graphics of each level (centered)
static Texture         *lastname_tex, *nextname_tex;


/// local class for drawing numbers
class WI_Number
{
protected:
  Texture **tex;      ///< graphics for numbers 0-9, tex[10] is the minus sign
  Texture  *pcent;
public:
  void Set(Texture **t, Texture *p)
  {
    tex = t;
    pcent = p;
  };
  
  int Draw(int x, int y, int n, int digits = 2);
  int DrawPercent(int x, int y, int n);
};


int WI_Number::DrawPercent(int x, int y, int n)
{
  if (n < 0)
    return x;

  pcent->Draw(x, y, FB);
  return Draw(x, y, n, -1);
}


int WI_Number::Draw(int x, int y, int n, int digits)
{
  // how many digits are needed?
  if (digits < 0)
    {
      if (n == 0)
	digits = 1;
      else
        {
	  digits = 0;
	  int temp = n;

	  while (temp)
            {
	      temp /= 10;
	      digits++;
            }
        }
    }

  bool neg = n < 0;

  if (neg)
    n = -n;

  // if non-number, do not draw it
  if (n == 1994)
    return 0;

  int w = tex[0]->width;

  // draw the new number
  while (digits--)
    {
      x -= w;
      tex[n % 10]->Draw(x, y, FB);
      n /= 10;
    }

  // draw a minus sign if necessary
  if (neg)
    tex[10]->Draw(x -= 8, y, FB);

  return x;
}



/// the single instance of WI_Number
WI_Number Num;


//========================================================================




// Draws "<Levelname> Finished!"
static void WI_drawLF(const char *name)
{
  int y = WI_TITLEY;

  // draw <LevelName>
  if (big_font)
    {
      big_font->DrawString((BASEVIDWIDTH - big_font->StringWidth(name))/2, y, name, FB);
      y += (5*big_font->StringHeight(name))/4;
      big_font->DrawString((BASEVIDWIDTH - big_font->StringWidth("Finished"))/2, y, "Finished", FB);
    }
  else
    {
      // no font, use levelname patches instead
      lastname_tex->Draw((BASEVIDWIDTH - (lastname_tex->width))/2, y, FB);
      y += (5 * lastname_tex->height)/4;
      // draw "Finished!"
      finished->Draw((BASEVIDWIDTH - (finished->width))/2,y, FB);
    }
}


// Draws "Entering <LevelName>"
static void WI_drawEL(const char *nextname)
{
  int y = WI_TITLEY;

  // draw "Entering"
  if (big_font)
    {
      big_font->DrawString((BASEVIDWIDTH - big_font->StringWidth("Entering"))/2, y, "Entering", FB);
      y += (5*big_font->StringHeight("Entering"))/4;
      big_font->DrawString((BASEVIDWIDTH - big_font->StringWidth(nextname))/2, y, nextname, FB);
    }
  else
    {
      entering->Draw((BASEVIDWIDTH - entering->width)/2, y, FB);

      // draw level
      y += (5 * nextname_tex->height)/4;
      nextname_tex->Draw((BASEVIDWIDTH - nextname_tex->width)/2, y, FB);
    }
}



// Display level completion time and par,
//  or "sucks" message if overflow.
static void WI_drawTime(int x, int y, int t)
{
  if (t < 0)
    return;

  if (t <= 61*59)
    {
      int div = 1;

      do
        {
          int n = (t / div) % 60;
          x = Num.Draw(x, y, n, 2) - colon->width;
          div *= 60;

          // draw
          if (div == 60 || t / div)
            colon->Draw(x, y, FB);

        } while (t / div);
    }
  else
    {
      // "sucks"
      sucks->Draw(x - sucks->width, y, FB);
    }
}


//=============================================================
//              Intermission class methods
//=============================================================

Intermission::Intermission()
{
  state = Inactive;
}


bool Intermission::Responder(event_t* ev)
{
  if (ev->type == ev_keydown)
    {
      acceleratestage = true;
      return true;
    }
  else
    return false;
}


// draw background
void Intermission::SlamBackground()
{
  // Heretic has a different bg during statcount
  if (game.mode == gm_heretic && state == StatCount)
    tc.GetPtr("FLOOR16")->DrawFill(0, 0, vid.width/vid.dupx, vid.height/vid.dupy);
  else if (rendermode == render_soft)
    memcpy(vid.screens[0], vid.screens[1], vid.width * vid.height);
  else
    intermission_bg->Draw(0, 0, FB);
}


static void InitAnimatedBack(int episode)
{
  if (episode < 1 || episode > 3)
    return;

  for (int i=0; i<NUMANIMS[episode-1]; i++)
    {
      anim_t *a = &anims[episode-1][i];

      // init variables
      a->ctr = -1;

      // specify the next time to draw it
      if (a->type == ANIM_ALWAYS)
        a->nexttic = 1 + (M_Random() % a->period);
      else if (a->type == ANIM_RANDOM)
        a->nexttic = 1 + a->data2 + (M_Random() % a->data1);
      else if (a->type == ANIM_LEVEL)
        a->nexttic = 1;
    }
}


void Intermission::UpdateAnimatedBack()
{
  if (episode < 1 || episode > 3)
    return;

  for (int i=0; i<NUMANIMS[episode-1]; i++)
    {
      anim_t *a = &anims[episode-1][i];

      if (bcount >= a->nexttic)
        {
          switch (a->type)
            {
            case ANIM_ALWAYS:
              if (++a->ctr >= a->nanims)
		a->ctr = 0;
              a->nexttic = bcount + a->period;
              break;

            case ANIM_RANDOM:
              a->ctr++;
              if (a->ctr == a->nanims)
                {
                  a->ctr = -1;
                  a->nexttic = bcount + a->data2 + (M_Random() % a->data1);
                }
              else
		a->nexttic = bcount + a->period;
              break;

            case ANIM_LEVEL:
              // gawd-awful hack for level anims
              if (!(state == StatCount && i == 7) && (next % 10) == a->data1)
                {
                  a->ctr++;
                  if (a->ctr == a->nanims)
		    a->ctr--;
                  a->nexttic = bcount + a->period;
                }
              break;
            }
        }
    }
}


static void DrawAnimatedBack(int ep)
{
  if (ep < 1 || ep > 3)
    return;

  for (int i=0 ; i<NUMANIMS[ep-1] ; i++)
    {
      anim_t *a = &anims[ep-1][i];
      if (a->ctr >= 0)
        a->p[a->ctr]->Draw(a->loc.x, a->loc.y, FB);
    }
}


// draws splats and "you are here" marker
void Intermission::DrawYAH()
{
  int ep = (episode - 1) % 3;

  point_t (*mapspots)[NUMMAPS] = DoomMapSpots;
  if (game.mode == gm_heretic)
    mapspots = HereticMapSpots;

  // not QUITE correct, but...
  int n = last % 10;
  for (int i = 0; i <= n; i++)
    splat->Draw(mapspots[ep][i].x, mapspots[ep][i].y, FB);

  // draw flashing ptr
  if (pointeron) // draw the destination 'X'
    yah[0]->Draw(mapspots[ep][next % 10].x, mapspots[ep][next % 10].y, FB);
}



// wait until the next level starts
void Intermission::InitWait()
{
  state = Wait;
  acceleratestage = false;
  count = 10;
  pointeron = true;
}



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

  count = TICRATE * DM_WAIT;
}


void Intermission::UpdateDMStats()
{
  if (--count <= 0)
    {
      S_StartLocalAmbSound(sfx_gib);
      InitWait();
    }
}



void Intermission::DrawDMStats()
{
#define RANKINGY 60

  char *timeleft;

  int white = -1;
  //Fab:25-04-98: when you play, you quickly see your frags because your
  //  name is displayed white, when playback demo, you quicly see who's the view.
  // TODO: splitscreen... another color?

  if (ViewPlayers.size())
    white = cv_teamplay.value ? ViewPlayers[0]->team : ViewPlayers[0]->number;

  // count frags for each present player
  HU_DrawRanking("Frags", 5, RANKINGY, dm_score[0], nplayers, false, white);

  // count buchholz
  HU_DrawRanking("Buchholz",85,RANKINGY, dm_score[1],nplayers,false, white);

  // count individual
  HU_DrawRanking("Indiv.",165,RANKINGY, dm_score[2],nplayers,false, white);

  // count deads
  HU_DrawRanking("Deaths",245,RANKINGY, dm_score[3],nplayers,false, white);

  timeleft = va("start in %d", count/TICRATE);
  hud_font->DrawString(200, 30, timeleft, V_WHITEMAP | V_SCALE);
}


/* old code
static void WI_ddrawDeathmatchStats()
{
    lh = WI_SPACINGY;

    // draw stat titles (top line)
    total->Draw(DM_TOTALSX-total->width/2,DM_MATRIXY-WI_SPACINGY+10,FB);

     killers->Draw(DM_KILLERSX, DM_KILLERSY, FB);
     victims->Draw(DM_VICTIMSX, DM_VICTIMSY, FB);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
	  colormap = translationtables[players[i].skincolor];

		V_DrawMappedPatch(x-stpb->width/2,
                        DM_MATRIXY - WI_SPACINGY,
                        FB,
                        stpb,      //p[i], now uses a common STPB0 translated
                        colormap); //      to the right colors

            V_DrawMappedPatch(DM_MATRIXX-stpb->width/2,
                        y,
                        FB,
                        stpb,      //p[i]
                        colormap);

            if (i == me)
            {
             bstar->Draw(x-stpb->width/2, DM_MATRIXY - WI_SPACINGY, FB);

             star->Draw(DM_MATRIXX-stpb->width/2, y, FB);
            }
        }
        else
        {
            // V_DrawPatch(x-bp[i]->width/2,
            //   DM_MATRIXY - WI_SPACINGY, FB, bp[i]);
            // V_DrawPatch(DM_MATRIXX-bp[i]->width/2,
            //   y, FB, bp[i]);
        }
        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = num[0]->width;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
            {
                if (playeringame[j])
                    Num.Draw(x+w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }
            Num.Draw(DM_TOTALSX+w, y, dm_totals[i], 2);
        }
        y += WI_SPACINGY;
    }
}
*/


void Intermission::InitCoopStats()
{
  int i, n = game.Players.size();

  count_stage = 1;
  count = TICRATE;
  cnt_time = cnt_par = -1;

  // resize the counters
  cnt.resize(n);
  plrs.resize(n);

  dofrags = false;
  map<int, PlayerInfo *>::iterator u;
  for (i = 0, u = game.Players.begin(); u != game.Players.end(); i++, u++)
    {
      cnt[i].kills = cnt[i].items = cnt[i].secrets = cnt[i].frags = 0;
      plrs[i] = u->second;
      dofrags = dofrags || (plrs[i]->score != 0);
    }
}




void Intermission::UpdateCoopStats()
{
  int temp;
  int i, n = plrs.size();

  if (acceleratestage && count_stage < 12)
    {
      acceleratestage = false;

      for (i=0 ; i<n ; i++)
        {
          cnt[i].kills = (plrs[i]->kills * 100) / total.kills;
          cnt[i].items = (plrs[i]->items * 100) / total.items;
          cnt[i].secrets = (plrs[i]->secrets * 100) / total.secrets;

          if (dofrags)
	    cnt[i].frags = plrs[i]->score;
        }
      cnt_time = time;
      cnt_par = partime;

      S_StartLocalAmbSound(sfx_barexp);
      count_stage = 12;
      return;
    }

  bool finished = true;

  if (count_stage == 2)
    {
      // count kills
      if (!(bcount&3))
        S_StartLocalAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
          cnt[i].kills += 2;
	  temp = (plrs[i]->kills * 100) / total.kills;

          if (cnt[i].kills >= temp)
            cnt[i].kills = temp;
          else
            finished = false;
        }
    }
  else if (count_stage == 4)
    {
      // count items
      if (!(bcount&3))
        S_StartLocalAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
          cnt[i].items += 2;
	  temp = (plrs[i]->items * 100) / total.items;

          if (cnt[i].items >= temp)
            cnt[i].items = temp;
          else
            finished = false;
        }
    }
  else if (count_stage == 6)
    {
      // count secrets
      if (!(bcount&3))
        S_StartLocalAmbSound(s_count);

      for (i=0 ; i<n ; i++)
        {
          cnt[i].secrets += 2;
	  temp = (plrs[i]->secrets * 100) / total.secrets;

          if (cnt[i].secrets >= temp)
            cnt[i].secrets = temp;
          else
            finished = false;
        }
    }
  else if (count_stage == 8)
    {
      // count frags
      if (!(bcount&3))
        S_StartLocalAmbSound(s_count);

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
    }
  else if (count_stage == 10)
    {
      // count time and partime
      if (!(bcount&3))
        S_StartLocalAmbSound(s_count);

      cnt_time += 3;

      if (cnt_time >= time)
        cnt_time = time;
      else
        finished = false;

      cnt_par += 3;

      if (cnt_par >= partime)
	cnt_par = partime;
      else
	finished = false;
    }
  else if (count_stage >= 12)
    {
      // wait for a keypress
      if (acceleratestage)
        {
          S_StartLocalAmbSound(sfx_sgcock);

          if (!episode)
            InitWait();
          else
            {
              state = ShowNextLoc;
              acceleratestage = false;
              count = SHOWNEXTLOCDELAY * TICRATE;
            }
        }
      return;
    }

  if (count_stage & 1)
    {
      // pause for a while between counts
      if (--count <= 0)
        {
          count_stage++;
          count = TICRATE;
        }
      return;
    }

  if (finished)
    {
      switch (count_stage)
	{
	case 6:
          S_StartLocalAmbSound(sfx_barexp);
          count_stage += 2 * !dofrags;
	  break;

	case 8:
          S_StartLocalAmbSound(sfx_pldeth);
	  break;

	default:
	  S_StartLocalAmbSound(sfx_barexp);
	}

      count_stage++;
    }
}


void Intermission::DrawCoopStats()
{
  if (!game.multiplayer)
    {
      // single player stats are a bit different
      // line height
      int lh = (3*(num[0]->height))/2;

      if (big_font)
        {
          // use FontB if any
          big_font->DrawString(SP_STATSX, SP_STATSY, "Kills", FB);
          big_font->DrawString(SP_STATSX, SP_STATSY+lh, "Items", FB);
          big_font->DrawString(SP_STATSX, SP_STATSY+2*lh, "Secrets", FB);
        }
      else
        {
          kills->Draw(SP_STATSX, SP_STATSY, FB);
          items->Draw(SP_STATSX, SP_STATSY+lh, FB);
          sp_secret->Draw(SP_STATSX, SP_STATSY+2*lh, FB);
        }
      Num.DrawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY, cnt[0].kills);
      Num.DrawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+lh, cnt[0].items);
      Num.DrawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt[0].secrets);
    }
  else
    {
      int x, y;

      // draw stat titles (top line)
      if (big_font)
        {
          // use FontB if any
          big_font->DrawString(NG_STATSX+  NG_SPACINGX-big_font->StringWidth("Kills"), NG_STATSY, "Kills", FB);
          big_font->DrawString(NG_STATSX+2*NG_SPACINGX-big_font->StringWidth("Items"), NG_STATSY, "Items", FB);
          big_font->DrawString(NG_STATSX+3*NG_SPACINGX-big_font->StringWidth("Scrt"), NG_STATSY, "Scrt", FB);
          if (dofrags)
            big_font->DrawString(NG_STATSX+4*NG_SPACINGX-big_font->StringWidth("Frgs"), NG_STATSY, "Frgs", FB);

          y = NG_STATSY + big_font->StringHeight("Kills");
        }
      else
        {
          kills->Draw(NG_STATSX+NG_SPACINGX-kills->width, NG_STATSY, FB);
          items->Draw(NG_STATSX+2*NG_SPACINGX-items->width, NG_STATSY, FB);
          secret->Draw(NG_STATSX+3*NG_SPACINGX-secret->width, NG_STATSY, FB);
          if (dofrags)
            frags->Draw(NG_STATSX+4*NG_SPACINGX-frags->width, NG_STATSY, FB);

          y = NG_STATSY + kills->height;
        }
      // draw stats
      int i, n = plrs.size();
      int pwidth = percent->width;
      int you = ViewPlayers.size() ? ViewPlayers[0]->number : 0;

      for (i=0 ; i<n ; i++)
        {
          // TODO draw names too, use a smaller font?
          // draw face
          int color = plrs[i]->options.color;

          x = NG_STATSX - (i & 1) ? 10 : 0;
	  current_colormap = translationtables[color];

          stpb->Draw(x - stpb->width, y, FB | V_MAP);

          // TODO splitscreen
          if (plrs[i]->number == you)
            star->Draw(x-stpb->width, y, FB);

          // draw stats
          x = NG_STATSX + NG_SPACINGX;
          Num.DrawPercent(x-pwidth, y+10, cnt[i].kills);   x += NG_SPACINGX;
          Num.DrawPercent(x-pwidth, y+10, cnt[i].items);   x += NG_SPACINGX;
          Num.DrawPercent(x-pwidth, y+10, cnt[i].secrets);  x += NG_SPACINGX;

          if (dofrags)
            Num.Draw(x, y+10, cnt[i].frags, -1);

          y += WI_SPACINGY;
        }
    }

  // draw time and par
  if (big_font)
    {
      big_font->DrawString(SP_TIMEX, SP_TIMEY, "Time", FB);
      big_font->DrawString(BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY, "Par", FB);
    }
  else
    {
      timePatch->Draw(SP_TIMEX, SP_TIMEY, FB);
      par->Draw(BASEVIDWIDTH/2 + SP_TIMEX, SP_TIMEY, FB);
    }

  WI_drawTime(BASEVIDWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);
  WI_drawTime(BASEVIDWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
}



void Intermission::LoadData()
{
  intermission_bg = tc.GetPtr(interpic);
  // TODO in doom2, darken the background image with colormap[25]? Nah.

  if (rendermode == render_soft)
    {
      memset(vid.screens[0], 0, vid.width*vid.height*vid.BytesPerPixel);
      memset(vid.screens[1], 0, vid.width*vid.height*vid.BytesPerPixel);

      // background stored in backbuffer
      intermission_bg->Draw(0, 0, 1 | V_SCALE);
    }

  int     i, j;
  char name[9];

  switch (game.mode)
    {
    case gm_doom2:
      // level name patches
      sprintf(name, "CWILV%2.2d", last);
      lastname_tex = tc.GetPtr(name);

      sprintf(name, "CWILV%2.2d", next);
      nextname_tex = tc.GetPtr(name);
      break;

    case gm_doom1s:
    case gm_doom1:
    case gm_udoom:
      // Doom 1 intermission animations
      anim_t *a;
      if (episode >= 1 && episode <= 3)
        {
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
		      a->p[i] = tc.GetPtr(name);
		    }
		}
	    }
	}
          // level name patches
      sprintf(name, "WILV%d%d", last / 10, last % 10); // our current map number encoding
      lastname_tex = tc.GetPtr(name);

      sprintf(name, "WILV%d%d", next / 10, next % 10);
      nextname_tex = tc.GetPtr(name);

      yah[0] = tc.GetPtr("WIURH0");
      yah[1] = tc.GetPtr("WIURH1");
      splat = tc.GetPtr("WISPLAT");
      break;

    case gm_heretic:
      yah[0] = yah[1] = tc.GetPtr("IN_YAH");
      splat = tc.GetPtr("IN_X");
      break;

    default:
      break;
    }


  if (game.mode == gm_heretic || game.mode == gm_hexen)
    {
      // numbers 0-9
      for (i=0;i<10;i++)
	{
	  sprintf(name, "FONTB%d", 16+i);
	  num[i] = tc.GetPtr(name);
	}

      num[10] = tc.GetPtr("FONTB13"); // minus sign
      percent = tc.GetPtr("FONTB05");
      colon   = tc.GetPtr("FONTB26");
    }
  else
    {
      // Doom
      num[10] = tc.GetPtr("WIMINUS");
      percent = tc.GetPtr("WIPCNT");
      colon   = tc.GetPtr("WICOLON");

      // numbers 0-9
      for (i=0;i<10;i++)
	{
	  sprintf(name, "WINUM%d", i);
	  num[i] = tc.GetPtr(name);
	}

      // "finished"
      finished = tc.GetPtr("WIF");

      // "entering"
      entering = tc.GetPtr("WIENTER");

      // "kills"
      kills = tc.GetPtr("WIOSTK");

      // "scrt"
      secret = tc.GetPtr("WIOSTS");

      // "secret"
      sp_secret = tc.GetPtr("WISCRT2");

      // "items"
      items = tc.GetPtr("WIOSTI");

      // "frgs"
      frags = tc.GetPtr("WIFRGS");

      // "time"
      timePatch = tc.GetPtr("WITIME");

      // "sucks"
      sucks = tc.GetPtr("WISUCKS");

      // "par"
      par = tc.GetPtr("WIPAR");

      // "killers" (vertical)
      killers = tc.GetPtr("WIKILRS");

      // "victims" (horiz)
      victims = tc.GetPtr("WIVCTMS");

      // "total"
      ptotal = tc.GetPtr("WIMSTT");
    }


  // your face
  star = tc.GetPtr("STFST01");

  // dead face
  bstar = tc.GetPtr("STFDEAD0");


  //added:08-02-98: now uses a single STPB0 which is remapped to the
  //                player translation table. Whatever new colors we add
  //                since we'll have to define a translation table for
  //                it, we'll have the right colors here automatically.
  stpb = tc.GetPtr("STPB0");

  Num.Set(num, percent); // setup number "widget"
}


void Intermission::UnloadData()
{
  int i, j;

  switch (game.mode)
    {
    case gm_doom2:
      lastname_tex->Release();
      nextname_tex->Release();
      break;

    case gm_doom1s:
    case gm_doom1:
    case gm_udoom:
      // Doom 1
      lastname_tex->Release();
      nextname_tex->Release();

      if (episode >= 1 && episode <= 3)
	{
	  for (j=0;j<NUMANIMS[episode-1];j++)
	    {
	      if (episode != 2 || j != 8)
		for (i=0;i<anims[episode-1][j].nanims;i++)
		  anims[episode-1][j].p[i]->Release();
	    }
	}

      yah[1]->Release();
      // fallthru
    case gm_heretic:
      yah[0]->Release();
      splat->Release();
      break;

    default:
      break;
    }

  // numbers
  for (i=0;i<11;i++)
    num[i]->Release();

  percent->Release();
  colon->Release();

  if (game.mode != gm_heretic && game.mode != gm_hexen)
    {
      finished->Release();
      entering->Release();
      kills->Release();
      secret->Release();
      sp_secret->Release();
      items->Release();
      frags->Release();
      timePatch->Release();
      sucks->Release();
      par->Release();

      victims->Release();
      killers->Release();
      ptotal->Release();
    }
}



// draws intermission screen
void Intermission::Drawer()
{
  SlamBackground();

  // draw animated background (doom1 only)
  if (game.mode >= gm_doom1s && game.mode <= gm_udoom)
    DrawAnimatedBack(episode);

  switch (state)
    {
    case StatCount:
      // draw "<level> finished"
      WI_drawLF(lastlevelname);

      if (cv_deathmatch.value)
	DrawDMStats();
      else
        DrawCoopStats();
      break;

    case ShowNextLoc:
    case Wait:
      if (count <= 0)  // all removed no draw !!!
        return;

      if (episode >= 1 && episode <= 3 && show_yah)
        DrawYAH();

      // draws which level you are entering..
      WI_drawEL(nextlevelname);
      break;

    default:
      break;
    }
}


// Updates stuff each tick
void Intermission::Ticker()
{
  // counter for general background animation
  bcount++;

  if (game.mode >= gm_doom1s && game.mode <= gm_udoom)
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
      if (!--count || acceleratestage)
        InitWait();
      else
        pointeron = (count & 31) < 20;
      break;

    case Wait:
      // TODO here we wait until the server sends a startlevel command
      // intermission is not ended before that
      if (!--count)
        End();
      break;

    default:
      break;
    }
}



// store data from the last completed map
void Intermission::Start(const Map *m, const MapInfo *n)
{
  if (!n)
    I_Error("Intermission: Nextmap does not exist!\n");

  if (state != Inactive)
    return; // already running

  time = m->maptic / 35;

  total.kills = m->kills;
  if (total.kills == 0)
    total.kills = 1;

  total.items = m->items;
  if (total.items == 0)
    total.items = 1;

  total.secrets = m->secrets;
  if (total.secrets == 0)
    total.secrets = 1;

  last = m->info->mapnumber - 1; // number of level just completed, zero-based
  partime = m->info->partime;
  lastlevelname = m->info->nicename.c_str();

  next = n->mapnumber - 1; // number of next level
  nextlevelname = n->nicename.c_str();

  // current and next clusters: show yah only if clusters belong to same episode
  episode = game.FindCluster(m->info->cluster)->episode;
  show_yah = (game.FindCluster(n->cluster)->episode == episode);

  interpic = m->info->interpic.c_str();
  intermusic = m->info->intermusic.c_str();

  acceleratestage = false;
  count = bcount = 0;
  state = StatCount;

  s_count = sfx_menu_choose;

  LoadData();

  if (cv_deathmatch.value)
    InitDMStats();
  else
    InitCoopStats();

  if (game.mode >= gm_doom1s && game.mode <= gm_udoom)
    InitAnimatedBack(episode);

  S.StartMusic(intermusic, true);
}



void Intermission::End()
{
  UnloadData();

  if (cv_deathmatch.value)
    {
      for (int i=0; i<4; i++)
        delete [] dm_score[i];
    }
  else
    {
      cnt.clear();
      plrs.clear();
    }

  state = Inactive;

  game.EndIntermission();
}
