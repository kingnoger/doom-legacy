// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2002 by DooM Legacy Team.
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
// Revision 1.8  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.7  2003/03/08 16:07:10  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.6  2003/02/16 16:54:51  smite-meister
// L2 sound cache done
//
// Revision 1.5  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/16 22:12:06  smite-meister
// Actor/DActor separation done!
//
// Revision 1.2  2002/12/03 10:15:29  smite-meister
// Older update
//
//
// DESCRIPTION:
//      Game completion, final screen animation.
//
//-----------------------------------------------------------------------------



#include "dstrings.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_stuff.h"

#include "r_state.h" // sprites array
#include "r_sprite.h"
#include "info.h"
#include "p_pspr.h"

#include "s_sound.h"
#include "sounds.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "g_level.h"
#include "p_info.h"


// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int             finalestage;
int             finalecount;

#define TEXTSPEED       3
#define TEXTWAIT        250

void    F_StartCast();
void    F_CastTicker();
bool F_CastResponder(event_t *ev);
void    F_CastDrawer();


static bool endgame;
static int gameepisode;
static const char *finaleflat;
static const char *finaletext;

static bool keypressed = false;

//
// F_StartFinale
//
void F_StartFinale(const clusterdef_t *cd, bool enter, bool end)
{
  endgame = end;
  gameepisode = cd->episode;
  finaleflat = cd->flatlump.c_str();
  if (enter)
    finaletext = cd->entertext.c_str();
  else
    finaletext = cd->exittext.c_str();

  S.StartMusic(cd->musiclump.c_str(), true);

  finalestage = 0;
  finalecount = 0;
}


bool F_Responder (event_t *event)
{
  if (finalestage == 2)
    return F_CastResponder (event);
  if( finalestage == 0 )
    {
      if (event->type != ev_keydown)
	return false;

      if( keypressed )
	return false;

      keypressed = true;
      return true;
    }
  return false;
}


//
// F_Ticker
//
void F_Ticker()
{
  // advance animation
  finalecount++;

  switch (finalestage)
    {
    case 0 :
      // check for skipping
      if (keypressed)
	{
	  keypressed = false;
	  if ((unsigned)finalecount < strlen(finaletext) * TEXTSPEED)
	    // force text to be write 
	    finalecount = strlen(finaletext) * TEXTSPEED;
	  else
	    // skip waiting time
	    finalecount += TEXTWAIT; 
	}

      if (finalecount > strlen(finaletext)*TEXTSPEED + TEXTWAIT)
	{
	  if (endgame)
	    { // skip to next stage
	      if (game.mode == gm_doom2)
		F_StartCast();
	      else {
		finalecount = 0;
		finalestage = 1;
		game.wipestate = GS_WIPE;             // force a wipe
		if (!game.raven && gameepisode == 3)
		  S.StartMusic("D_BUNNY");
	      }
	    }
	  else
	    { // return to game
	      game.EndFinale();
	      finalecount = MININT;    // wait until map is lunched
	    }
	}
      break;

    case 2 : 
      F_CastTicker();
      break;

    default:
      break;
    }
}



//
// F_TextWrite
//
void F_TextWrite()
{
  int         w;
  int         count;
  const char *ch;
  int         c;
  int         cx;
  int         cy;

  // erase the entire screen to a tiled background
  V_DrawFlatFill(0,0,vid.width,vid.height,fc.GetNumForName(finaleflat));

  //V_MarkRect (0, 0, vid.width, vid.height);

  // draw some of the text onto the screen
  if( game.raven )
    {
      cx = 20;
      cy = 5;
    }
  else
    {
      cx = 10;
      cy = 10;
    }
  ch = finaletext;

  count = (finalecount - 10)/TEXTSPEED;
  if (count < 0)
    count = 0;
  for ( ; count ; count-- )
    {
      c = *ch++;
      if (!c)
	break;
      if (c == '\n')
        {
	  cx = game.raven ? 20 : 10;
	  cy += game.raven ? 9 : 11;
	  continue;
        }

      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c> HU_FONTSIZE)
        {
	  cx += 4;
	  continue;
        }

      w =  (hud.font[c]->width);
      if (cx+w > vid.width)
	break;
      V_DrawScaledPatch(cx, cy, 0, hud.font[c]);
      cx+=w;
    }

}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
struct castinfo_t
{
  char       *name;
  mobjtype_t  type;
};

castinfo_t castorder[] = {
  {NULL, MT_POSSESSED},
  {NULL, MT_SHOTGUY},
  {NULL, MT_CHAINGUY},
  {NULL, MT_TROOP},
  {NULL, MT_SERGEANT},
  {NULL, MT_SKULL},
  {NULL, MT_HEAD},
  {NULL, MT_KNIGHT},
  {NULL, MT_BRUISER},
  {NULL, MT_BABY},
  {NULL, MT_PAIN},
  {NULL, MT_UNDEAD},
  {NULL, MT_FATSO},
  {NULL, MT_VILE},
  {NULL, MT_SPIDER},
  {NULL, MT_CYBORG},
  {NULL, MT_PLAYER},
  {NULL, (mobjtype_t)0}
};

int             castnum;
int             casttics;
state_t*        caststate;
bool         castdeath;
int             castframes;
int             castonmelee;
bool         castattacking;


//
// F_StartCast
//


void F_StartCast()
{
  int i;

  for(i=0;i<17;i++)
    castorder[i].name = text[CC_ZOMBIE_NUM+i];

  game.wipestate = GS_WIPE;         // force a screen wipe
  castnum = 0;
  caststate = &states[mobjinfo[castorder[castnum].type].seestate];
  casttics = caststate->tics;
  castdeath = false;
  finalestage = 2;
  castframes = 0;
  castonmelee = 0;
  castattacking = false;
  S.StartMusic("D_EVIL", true);
}


//
// F_CastTicker
//
void F_CastTicker()
{
  int         st;
  int         sfx;

  if (--casttics > 0)
    return;                 // not time to change state yet

  if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
      // switch from deathstate to next monster
      castnum++;
      castdeath = false;
      if (castorder[castnum].name == NULL)
	castnum = 0;
      if (mobjinfo[castorder[castnum].type].seesound)
	S_StartAmbSound(mobjinfo[castorder[castnum].type].seesound);
      caststate = &states[mobjinfo[castorder[castnum].type].seestate];
      castframes = 0;
    }
  else
    {
      // just advance to next state in animation
      if (caststate == &states[S_PLAY_ATK1])
	goto stopattack;    // Oh, gross hack!
      st = caststate->nextstate;
      caststate = &states[st];
      castframes++;

      // sound hacks....
      switch (st)
        {
	case S_PLAY_ATK1:     sfx = sfx_dshtgn; break;
	case S_POSS_ATK2:     sfx = sfx_pistol; break;
	case S_SPOS_ATK2:     sfx = sfx_shotgn; break;
	case S_VILE_ATK2:     sfx = sfx_vilatk; break;
	case S_SKEL_FIST2:    sfx = sfx_skeswg; break;
	case S_SKEL_FIST4:    sfx = sfx_skepch; break;
	case S_SKEL_MISS2:    sfx = sfx_skeatk; break;
	case S_FATT_ATK8:
	case S_FATT_ATK5:
	case S_FATT_ATK2:     sfx = sfx_firsht; break;
	case S_CPOS_ATK2:
	case S_CPOS_ATK3:
	case S_CPOS_ATK4:     sfx = sfx_shotgn; break;
	case S_TROO_ATK3:     sfx = sfx_claw; break;
	case S_SARG_ATK2:     sfx = sfx_sgtatk; break;
	case S_BOSS_ATK2:
	case S_BOS2_ATK2:
	case S_HEAD_ATK2:     sfx = sfx_firsht; break;
	case S_SKULL_ATK2:    sfx = sfx_sklatk; break;
	case S_SPID_ATK2:
	case S_SPID_ATK3:     sfx = sfx_shotgn; break;
	case S_BSPI_ATK2:     sfx = sfx_plasma; break;
	case S_CYBER_ATK2:
	case S_CYBER_ATK4:
	case S_CYBER_ATK6:    sfx = sfx_rlaunc; break;
	case S_PAIN_ATK3:     sfx = sfx_sklatk; break;
	default: sfx = 0; break;
        }

      if (sfx)
	S_StartAmbSound(sfx);
    }

  if (castframes == 12)
    {
      // go into attack frame
      castattacking = true;
      if (castonmelee)
	caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
      else
	caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
      castonmelee ^= 1;
      if (caststate == &states[S_NULL])
        {
	  if (castonmelee)
	    caststate=
	      &states[mobjinfo[castorder[castnum].type].meleestate];
	  else
	    caststate=
	      &states[mobjinfo[castorder[castnum].type].missilestate];
        }
    }

  if (castattacking)
    {
      if (castframes == 24
	  ||  caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
        {
	stopattack:
	  castattacking = false;
	  castframes = 0;
	  caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
    }

  casttics = caststate->tics;
  if (casttics == -1)
    casttics = 15;
}


//
// F_CastResponder
//

bool F_CastResponder (event_t* ev)
{
  if (ev->type != ev_keydown)
    return false;

  if (castdeath)
    return true;                    // already in dying frames

  // go into death frame
  castdeath = true;
  caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
  casttics = caststate->tics;
  castframes = 0;
  castattacking = false;
  if (mobjinfo[castorder[castnum].type].deathsound)
    S_StartAmbSound(mobjinfo[castorder[castnum].type].deathsound);

  return true;
}


#define CASTNAME_Y   180                // where the name of actor is drawn
void F_CastPrint (char* text)
{
  V_DrawString ((BASEVIDWIDTH-V_StringWidth (text))/2, CASTNAME_Y, 0, text);
}


//
// F_CastDrawer
//
void F_CastDrawer()
{
  sprite_t*        sprdef;
  spriteframe_t*      sprframe;
  int                 lump;
  bool             flip;
  patch_t*            patch;

  // erase the entire screen to a background
  //V_DrawPatch (0,0,0, fc.CachePatchName ("BOSSBACK", PU_CACHE));
  D_PageDrawer("BOSSBACK");

  F_CastPrint(castorder[castnum].name);

  // draw the current frame in the middle of the screen
  sprdef = sprites.Get(sprnames[caststate->sprite]);
  sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
  lump = sprframe->lumppat[0];      //Fab: see R_InitSprites for more
  flip = (bool)sprframe->flip[0];

  patch = fc.CachePatchNum (lump, PU_CACHE);

  V_DrawScaledPatch (BASEVIDWIDTH>>1,170,flip ? V_FLIPPEDPATCH : 0,patch);
}


//
// F_DrawPatchCol
//
static void F_DrawPatchCol (int           x,
			    patch_t*      patch,
			    int           col )
{
  column_t*   column;
  byte*       source;
  byte*       dest;
  byte*       desttop;
  int         count;

  column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
  desttop = vid.screens[0]+x*vid.dupx;

  // step through the posts in a column
  while (column->topdelta != 0xff )
    {
      source = (byte *)column + 3;
      dest = desttop + column->topdelta*vid.width;
      count = column->length;

      while (count--)
        {
	  int dupycount=vid.dupy;

	  while(dupycount--)
            {
	      int dupxcount=vid.dupx;
	      while(dupxcount--)
		*dest++ = *source;

	      dest += (vid.width-vid.dupx);
            }
	  source++;
        }
      column = (column_t *)(  (byte *)column + column->length + 4 );
    }
}


//
// F_BunnyScroll
//
void F_BunnyScroll()
{
  int         scrolled;
  int         x;
  patch_t*    p1;
  patch_t*    p2;
  char        name[10];
  int         stage;
  static int  laststage;

  p1 = fc.CachePatchName ("PFUB2", PU_LEVEL);
  p2 = fc.CachePatchName ("PFUB1", PU_LEVEL);

  //V_MarkRect (0, 0, vid.width, vid.height);

  scrolled = 320 - (finalecount-230)/2;
  if (scrolled > 320)
    scrolled = 320;
  if (scrolled < 0)
    scrolled = 0;
  //faB:do equivalent for hw mode ?
  if (rendermode==render_soft)
    {
      for ( x=0 ; x<320 ; x++)
        {
	  if (x+scrolled < 320)
	    F_DrawPatchCol (x, p1, x+scrolled);
	  else
	    F_DrawPatchCol (x, p2, x+scrolled - 320);
        }
    }
  else
    {
      if( scrolled>0 )
	V_DrawScaledPatch(320-scrolled,0, 0, p2 );
      if( scrolled<320 )
	V_DrawScaledPatch(-scrolled,0, 0, p1 );
    }

  if (finalecount < 1130)
    return;
  if (finalecount < 1180)
    {
      V_DrawScaledPatch ((320-13*8)/2,
			 (200-8*8)/2,0, fc.CachePatchName ("END0",PU_CACHE));
      laststage = 0;
      return;
    }

  stage = (finalecount-1180) / 5;
  if (stage > 6)
    stage = 6;
  if (stage > laststage)
    {
      S_StartAmbSound(sfx_pistol);
      laststage = stage;
    }

  sprintf (name,"END%i",stage);
  V_DrawScaledPatch ((320-13*8)/2, (200-8*8)/2,0, fc.CachePatchName (name,PU_CACHE));
}


/*
==================
=
= F_DemonScroll
=
==================
*/

void F_DemonScroll()
{
    
  int scrolled;

  scrolled = (finalecount-70)/3;
  if (scrolled > 200)
    scrolled = 200;
  if (scrolled < 0)
    scrolled = 0;

  V_DrawRawScreen(0, scrolled*vid.dupy, fc.FindNumForName("FINAL1"),320,200);
  if( scrolled>0 )
    V_DrawRawScreen(0, (scrolled-200)*vid.dupy, fc.FindNumForName("FINAL2"),320,200);
}


/*
==================
=
= F_DrawUnderwater
=
==================
*/

void F_DrawUnderwater()
{
  static bool underwawa = false;

  switch(finalestage)
    {
    case 1:
      if(!underwawa)
	{
	  underwawa = true;
	  vid.SetPaletteLump("E2PAL");
	}
      V_DrawRawScreen(0, 0, fc.FindNumForName("E2END"),320,200);
      break;
    case 2:
      V_DrawRawScreen(0, 0, fc.FindNumForName("TITLE"),320,200);
      underwawa = false;
      //D_StartTitle(); // go to intro/demo mode.
    }
}


//
// F_Drawer
//
void F_Drawer()
{
  switch (finalestage)
    {
    case 0:
      F_TextWrite();
      break;
    case 2:
      F_CastDrawer();
      break;
    default:
      if (game.mode == gm_heretic)
	{
	  switch (gameepisode)
	    {
	    case 1:
	      if(fc.FindNumForName("e2m1")==-1)
		V_DrawRawScreen(0, 0, fc.FindNumForName("ORDER"),320,200);
	      else
		// FIXME!  search only in the first pwad since legacy define a patch with same name
		V_DrawRawScreen(0, 0, fc.FindNumForNamePwad("CREDIT",0,0),320,200);
	      break;
	    case 2:
	      F_DrawUnderwater();
	      break;
	    case 3:
	      F_DemonScroll();
	      break;
	    case 4: // Just show credits screen for extended episodes
	    case 5:
	      // FIXME! search only in the first pwad since legacy define a patch with same name
	      V_DrawRawScreen(0, 0, fc.FindNumForNamePwad("CREDIT",0,0),320,200);
	      break;
	    }
	} else switch (gameepisode)
	  {
	  case 1:
	    if ( game.mode == gm_udoom )
	      V_DrawScaledPatch (0,0,0, fc.CachePatchName(text[CREDIT_NUM],PU_CACHE));
	    else
	      V_DrawScaledPatch (0,0,0, fc.CachePatchName(text[HELP2_NUM],PU_CACHE));
	    break;
	  case 2:
	    V_DrawScaledPatch(0,0,0, fc.CachePatchName(text[VICTORY2_NUM],PU_CACHE));
	    break;
	  case 3:
	    F_BunnyScroll();
	    break;
	  case 4:
	    V_DrawScaledPatch (0,0,0, fc.CachePatchName(text[ENDPIC_NUM],PU_CACHE));
	    break;
	  }
    }

  return;
}
