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
// Revision 1.9  2005/05/31 18:04:21  smite-meister
// screenslink crash fixed
//
// Revision 1.7  2004/11/09 20:38:51  smite-meister
// added packing to I/O structs
//
// Revision 1.6  2004/09/23 23:21:17  smite-meister
// HUD updated
//
// Revision 1.4  2004/03/28 15:16:13  smite-meister
// Texture cache.
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Mission begin melt/wipe screen special effect.

#include <string.h>

#include "command.h"
#include "m_random.h"
#include "f_wipe.h"
#include "i_video.h"
#include "v_video.h"
#include "r_draw.h" // tr_transxxx
#include "screen.h"
#include "z_zone.h"

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------


CV_PossibleValue_t screenslink_cons_t[] = {{0,"none"},{1,"color"},{2,"melt"},{0,NULL}};
consvar_t cv_screenslink = {"screenlink","2", CV_SAVE,screenslink_cons_t};

struct wipe_t
{
  void (*init)(int width, int height);
  bool (*perform)(int width, int height, int ticks);
  void (*exit)();
};



static byte *wipe_scr_start = NULL;
static byte *wipe_scr_end = NULL;
static byte *wipe_scr = NULL;


// transposes an array
static void wipe_shittyColMajorXform(short *array, int width, int height)
{
  int x, y;
  short *dest = (short*)Z_Malloc(width*height*sizeof(short), PU_STATIC, 0);

  for (y=0;y<height;y++)
    for (x=0;x<width;x++)
      dest[x*height+y] = array[y*width+x];

  memcpy(array, dest, width*height*sizeof(short));

  Z_Free(dest);
}


static void wipe_initColorXForm(int width, int height)
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



static bool wipe_doColorXForm(int width, int height, int ticks)
{
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
	      if ((newval = transtables[tr_transmor-1][(*e << 8) + *w]) == *w)
		if ((newval = transtables[tr_transmed-1][(*e << 8) + *w]) == *w)
		  if ((newval = transtables[tr_transmor-1][(*w << 8) + *e]) == *w)
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


static void wipe_exitColorXForm()
{
}




static int *column_y; // the y coordinate of the melt for each column


static void wipe_initMelt(int width, int height)
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



static bool wipe_doMelt(int width, int height, int ticks)
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


static void wipe_exitMelt()
{
  Z_Free(column_y);
}



static wipe_t wipes[] =
{
  {wipe_initColorXForm, wipe_doColorXForm, wipe_exitColorXForm},
  {wipe_initMelt, wipe_doMelt, wipe_exitMelt}
};



// save the 'before' screen of the wipe (the one that melts down)
bool wipe_StartScreen()
{
  if (cv_screenslink.value == 0)
    return false; // no wipes required

  wipe_scr_start = (byte *)calloc(vid.height * vid.rowbytes, 1);
  if (wipe_scr_start)
    {
      I_ReadScreen(wipe_scr_start);
      return true;
    }

  return false; // no memory
}


// save the 'after' screen of the wipe (the one that show behind the melt)
bool wipe_EndScreen()
{
  wipe_scr_end = (byte *)calloc(vid.height * vid.rowbytes, 1);
  if (wipe_scr_end)
    {
      I_ReadScreen(wipe_scr_end);

      // initialize the wipe algorithm
      wipe_scr = vid.screens[0];
      wipes[cv_screenslink.value - 1].init(vid.width, vid.height);
      return true;
    }

  free(wipe_scr_start); // no memory, no wipe
  return false;
}



bool wipe_ScreenWipe(int ticks)
{
  int i = cv_screenslink.value - 1;

  // do a piece of wipe-in
  if (ticks < 0 || wipes[i].perform(vid.width, vid.height, ticks))
    {
      // finish the wipe
      wipes[i].exit();
      free(wipe_scr_start);
      free(wipe_scr_end);
      wipe_scr_start = wipe_scr_end = wipe_scr = NULL;
      return true;
    }

  return false;
}
