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
// Revision 1.4  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.3  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.2  2002/12/03 10:15:29  smite-meister
// Older update
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "command.h"
#include "m_random.h"
#include "f_wipe.h"
#include "i_video.h"
#include "v_video.h"
#include "r_defs.h" // tr_transxxx
#include "screen.h"
#include "z_zone.h"

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------


CV_PossibleValue_t screenslink_cons_t[] = {{0,"none"},{1,"color"},{2,"melt"},{0,NULL}};
consvar_t cv_screenslink = {"screenlink","2", CV_SAVE,screenslink_cons_t};

struct wipe_t
{
  void (*init)(int width, int height, int ticks);
  bool (*perform)(int width, int height, int ticks);
  void (*exit)(int width, int height, int ticks);
};



static byte *wipe_scr_start;
static byte *wipe_scr_end;
static byte *wipe_scr;


// transposes an array
void wipe_shittyColMajorXform(short *array, int width, int height)
{
  int x, y;
  short *dest = (short*)Z_Malloc(width*height*sizeof(short), PU_STATIC, 0);

  for (y=0;y<height;y++)
    for (x=0;x<width;x++)
      dest[x*height+y] = array[y*width+x];

  memcpy(array, dest, width*height*sizeof(short));

  Z_Free(dest);
}


void wipe_initColorXForm(int width, int height, int ticks)
{
  memcpy(wipe_scr, wipe_scr_start, width*height*vid.BytesPerPixel);
}


// BP:the original one, work only in hicolor
/*
int wipe_doColorXForm(int width, int height, int ticks)
{
  int newval;
  
  bool changed = false;
  byte *w = wipe_scr;
  byte *e = wipe_scr_end;

  while (w != wipe_scr + width*height)
    {
      if (*w != *e)
	{
	  if (*w > *e)
	    {
	      newval = *w - ticks;
	      if (newval < *e)
		*w = *e;
	      else
		*w = newval;
	      changed = true;
	    }
	  else if (*w < *e)
	    {
	      newval = *w + ticks;
	      if (newval > *e)
		*w = *e;
	      else
		*w = newval;
	      changed = true;
	    }
	}
      w++;
      e++;
    }

  return !changed;
}
*/



bool wipe_doColorXForm(int width, int height, int ticks)
{
  extern byte *transtables;
  static int slowdown = 0;

  byte newval;
  bool changed = false;

  while (ticks--)
    {
      // slowdown
      if (slowdown++)
	{
	  slowdown = 0;
	  return false;
	}
 
      byte *w = wipe_scr;
      byte *e = wipe_scr_end;
 
 
      while (w != wipe_scr + width*height)
	{
	  if (*w != *e)
	    {
	      if ((newval = transtables[(*e << 8) + *w + ((tr_transmor-1) << FF_TRANSSHIFT)]) == *w)
		if ((newval = transtables[(*e << 8) + *w + ((tr_transmed-1) << FF_TRANSSHIFT)]) == *w)
		  if ((newval = transtables[(*w << 8) + *e + ((tr_transmor-1) << FF_TRANSSHIFT)]) == *w)
		    newval = *e;
	      *w = newval;
	      changed = true;
	    }
	  w++;
	  e++;
	}
    }

  return !changed;
}


void wipe_exitColorXForm(int width, int height, int ticks) {}





static int *column_y; // the y coordinate of the melt for each column


void wipe_initMelt(int width, int height, int ticks)
{
  int i, r;

  // copy start screen to main screen
  memcpy(wipe_scr, wipe_scr_start, width*height*vid.BytesPerPixel);

  // makes this wipe faster (in theory)
  // to have stuff in column-major format
  wipe_shittyColMajorXform((short*)wipe_scr_start, width*vid.BytesPerPixel/sizeof(short), height);
  wipe_shittyColMajorXform((short*)wipe_scr_end, width*vid.BytesPerPixel/sizeof(short), height);

  // setup initial column positions
  // (y<0 => not ready to scroll yet)
  column_y = (int *)Z_Malloc(width*sizeof(int), PU_STATIC, 0);
  column_y[0] = -(M_Random()%16);
  for (i=1;i<width;i++)
    {
      r = (M_Random()%3) - 1; 
      column_y[i] = column_y[i-1] + r;
      if (column_y[i] > 0)
	column_y[i] = 0;
      else if (column_y[i] == -16)
	column_y[i] = -15;
    }
  // dup for normal speed in high res
  for (i=0;i<width;i++)
    column_y[i] *= vid.dupy;
}



bool wipe_doMelt(int width, int height, int ticks)
{
  int  i, j;
  bool done = true;

  width = (width * vid.BytesPerPixel) / sizeof(short);

  while (ticks--)
    {
      for (i=0;i<width;i++)
	{
	  if (column_y[i] < 0)
	    {
	      column_y[i]++;
	      done = false;
	    }
	  else if (column_y[i] < height)
	    {
	      int dy = (column_y[i] < 16) ? column_y[i]+1 : 8;
	      dy *= vid.dupy;
	      if (column_y[i] + dy >= height)
		dy = height - column_y[i];

	      short *s = &((short *)wipe_scr_end)[i*height+column_y[i]];
	      short *d = &((short *)wipe_scr)[column_y[i]*width+i];

	      int idx = 0;
	      for (j=dy;j;j--)
		{
		  d[idx] = *(s++);
		  idx += width;
		}
	      column_y[i] += dy;
	      s = &((short *)wipe_scr_start)[i*height];
	      d = &((short *)wipe_scr)[column_y[i]*width+i];
	      idx = 0;
	      for (j=height-column_y[i];j;j--)
		{
		  d[idx] = *(s++);
		  idx += width;
		}
	      done = false;
	    }
	}
    }

  return done;
}


void wipe_exitMelt(int width, int height, int ticks)
{
  Z_Free(column_y);
}





// save the 'before' screen of the wipe (the one that melts down)
void wipe_StartScreen(int x, int y, int width, int height)
{
  wipe_scr_start = vid.screens[2];
  I_ReadScreen(wipe_scr_start);
}


// save the 'after' screen of the wipe (the one that show behind the melt)
void wipe_EndScreen(int x, int y, int width, int height)
{
  wipe_scr_end = vid.screens[3];
  I_ReadScreen(wipe_scr_end);
  V_DrawBlock(x, y, 0, width, height, wipe_scr_start); // restore start scr.
}



bool wipe_ScreenWipe(int x, int y, int width, int height, int ticks)
{
  static wipe_t wipes[] =
    {
      {wipe_initColorXForm, wipe_doColorXForm, wipe_exitColorXForm},
      {wipe_initMelt, wipe_doMelt, wipe_exitMelt}
    };

  int i = cv_screenslink.value - 1;

  // when false, stop the wipe
  static bool go = false;

  // initial stuff
  if (!go)
    {
      go = true;
      // wipe_scr = (byte *) Z_Malloc(width*height*vid.BytesPerPixel, PU_STATIC, 0); // DEBUG
      wipe_scr = vid.screens[0];
      wipes[i].init(width, height, ticks);
    }

  // do a piece of wipe-in
  bool rc = wipes[i].perform(width, height, ticks);

  // final stuff
  if (rc)
    {
      go = false;
      wipes[i].exit(width, height, ticks);
    }

  return !go;
}
