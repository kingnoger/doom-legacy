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
// Revision 1.17  2004/07/11 14:32:02  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.16  2004/07/05 16:53:31  smite-meister
// Netcode replaced
//
// Revision 1.15  2004/05/01 23:29:19  hurdler
// add dummy new renderer
//
// Revision 1.14  2004/03/28 15:16:15  smite-meister
// Texture cache.
//
// Revision 1.13  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.12  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.11  2003/05/30 13:34:49  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.10  2003/05/11 21:23:53  smite-meister
// Hexen fixes
//
// Revision 1.9  2003/04/24 20:30:36  hurdler
// Remove lots of compiling warnings
//
// Revision 1.8  2003/04/19 17:38:48  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.7  2003/03/23 14:24:14  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.6  2003/03/08 16:07:19  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.5  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/23 23:22:47  smite-meister
// WAD2+WAD3 support, MAPINFO parser added!
//
// Revision 1.2  2002/12/16 22:22:01  smite-meister
// Actor/DActor separation
//
// Revision 1.1.1.1  2002/11/16 14:18:46  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.24  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.23  2001/05/16 21:21:14  bpereira
// no message
//
// Revision 1.22  2001/03/30 17:12:51  bpereira
// no message
//
// Revision 1.21  2001/03/21 18:24:39  stroggonmeth
// Misc changes and fixes. Code cleanup
//
// Revision 1.20  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.19  2001/02/10 12:27:14  bpereira
// no message
//
// Revision 1.18  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.17  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.16  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.15  2000/09/28 20:57:17  bpereira
// no message
//
// Revision 1.14  2000/09/21 16:45:06  bpereira
// no message
//
// Revision 1.13  2000/08/31 14:30:56  bpereira
// no message
//
// Revision 1.12  2000/07/01 09:23:49  bpereira
// no message
//
// Revision 1.11  2000/04/30 10:30:10  bpereira
// no message
//
// Revision 1.10  2000/04/24 20:24:38  bpereira
// no message
//
// Revision 1.9  2000/04/18 17:39:39  stroggonmeth
// Bug fixes and performance tuning.
//
// Revision 1.8  2000/04/08 17:29:25  stroggonmeth
// no message
//
// Revision 1.7  2000/04/06 21:06:19  stroggonmeth
// Optimized extra_colormap code...
// Added #ifdefs for older water code.
//
// Revision 1.6  2000/04/04 19:28:43  stroggonmeth
// Global colormaps working. Added a new linedef type 272.
//
// Revision 1.5  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.4  2000/03/06 15:15:54  hurdler
// compiler warning removed
//
// Revision 1.3  2000/02/27 16:30:28  hurdler
// dead player bug fix + add allowmlook <yes|no>
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Rendering main loop and setup functions,
//       utility functions (BSP, geometry, trigonometry).
//      See tables.c, too.
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "cvars.h"

#include "g_game.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "p_camera.h"

#include "r_render.h"
#include "hu_stuff.h"
#include "am_map.h"

#include "r_local.h"
#include "r_state.h"
#include "i_video.h"
#include "m_menu.h"
#include "t_func.h"
#include "d_main.h"

#ifdef HWRENDER
# include "hardware/hw_main.h"
# include "hardware/hwr_render.h"
#endif


Rend R;
#ifdef HWRENDER
HWRend HWR;
#endif

angle_t G_ClipAimingPitch(angle_t pitch);


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
long long mycount;
long long mytotal = 0;
//unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW             2048



int                     viewangleoffset;

// increment every time a check is made
int                     validcount = 1;


lighttable_t*           fixedcolormap;

int                     centerx;
int                     centery;
int                     centerypsp;     //added:06-02-98:cf R_DrawPSprite

fixed_t                 centerxfrac;
fixed_t                 centeryfrac;
fixed_t                 projection;
//added:02-02-98:fixing the aspect ration stuff...
fixed_t                 projectiony;

// just for profiling purposes
int                     framecount;

int                     sscount;
int                     linecount;
int                     loopcount;


fixed_t                 viewcos;
fixed_t                 viewsin;

// 0 = high, 1 = low
int                     detailshift;

//
// precalculated math tables
//
angle_t                 clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int                     viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t                 xtoviewangle[MAXVIDWIDTH+1];


lighttable_t*           scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*           scalelightfixed[MAXLIGHTSCALE];
lighttable_t*           zlight[LIGHTLEVELS][MAXLIGHTZ];

//SoM: 3/30/2000: Hack to support extra boom colormaps.
int                     num_extra_colormaps;
extracolormap_t         extra_colormaps[MAXCOLORMAPS];

// bumped light from gun blasts
int                     extralight;


//===========================================
//  client consvars
//===========================================

CV_PossibleValue_t viewsize_cons_t[]={{3,"MIN"},{12,"MAX"},{0,NULL}};
CV_PossibleValue_t detaillevel_cons_t[]={{0,"High"},{1,"Low"},{0,NULL}};

consvar_t cv_viewsize       = {"viewsize","10",CV_SAVE|CV_CALL,viewsize_cons_t,R_SetViewSize};      //3-12
consvar_t cv_detaillevel    = {"detaillevel","0",CV_SAVE|CV_CALL,detaillevel_cons_t,R_SetViewSize}; // UNUSED
consvar_t cv_scalestatusbar = {"scalestatusbar","0",CV_SAVE|CV_CALL,CV_YesNo,R_SetViewSize};
// consvar_t cv_fov = {"fov","2048", CV_CALL | CV_NOINIT, NULL, R_ExecuteSetViewSize};

void Translucency_OnChange();
void BloodTime_OnChange();
CV_PossibleValue_t bloodtime_cons_t[]={{1,"MIN"},{3600,"MAX"},{0,NULL}};

consvar_t cv_translucency  = {"translucency","1",CV_CALL|CV_SAVE,CV_OnOff, Translucency_OnChange};
// how much tics to last for the last (third) frame of blood (S_BLOODx)
consvar_t cv_splats    = {"splats","1",CV_SAVE,CV_OnOff};
consvar_t cv_bloodtime = {"bloodtime","20",CV_CALL|CV_SAVE,bloodtime_cons_t,BloodTime_OnChange};
consvar_t cv_psprites  = {"playersprites","1",0,CV_OnOff};

CV_PossibleValue_t viewheight_cons_t[]={{16,"MIN"},{56,"MAX"},{0,NULL}};
consvar_t cv_viewheight = {"viewheight", "41",0,viewheight_cons_t,NULL};


//===========================================

//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int R_PointOnSide(fixed_t x, fixed_t y, node_t *node)
{
  fixed_t     dx;
  fixed_t     dy;
  fixed_t     left;
  fixed_t     right;

  if (!node->dx)
    {
      if (x <= node->x)
    return node->dy > 0;

      return node->dy < 0;
    }
  if (!node->dy)
    {
      if (y <= node->y)
    return node->dx < 0;

      return node->dx > 0;
    }

  dx = (x - node->x);
  dy = (y - node->y);

  // Try to quickly decide by looking at sign bits.
  if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
      if ( (node->dy ^ dx) & 0x80000000 )
        {
      // (left is negative)
      return 1;
        }
      return 0;
    }

  left = FixedMul ( node->dy>>FRACBITS , dx );
  right = FixedMul ( dy , node->dx>>FRACBITS );

  if (right < left)
    {
      // front side
      return 0;
    }
  // back side
  return 1;
}


int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
    fixed_t     lx;
    fixed_t     ly;
    fixed_t     ldx;
    fixed_t     ldy;
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     left;
    fixed_t     right;

    lx = line->v1->x;
    ly = line->v1->y;

    ldx = line->v2->x - lx;
    ldy = line->v2->y - ly;

    if (!ldx)
    {
        if (x <= lx)
            return ldy > 0;

        return ldy < 0;
    }
    if (!ldy)
    {
        if (y <= ly)
            return ldx < 0;

        return ldx > 0;
    }

    dx = (x - lx);
    dy = (y - ly);

    // Try to quickly decide by looking at sign bits.
    if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
    {
        if  ( (ldy ^ dx) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul ( ldy>>FRACBITS , dx );
    right = FixedMul ( dy , ldx>>FRACBITS );

    if (right < left)
    {
        // front side
        return 0;
    }
    // back side
    return 1;
}


//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

//
angle_t R_PointToAngle2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1)
{
    x1 -= x2;
    y1 -= y2;

    if ( (!x1) && (!y1) )
        return 0;

    if (x1>= 0)
    {
        // x >=0
        if (y1>= 0)
        {
            // y>= 0

            if (x1>y1)
            {
                // octant 0
                return tantoangle[ SlopeDiv(y1,x1)];
            }
            else
            {
                // octant 1
                return ANG90-1-tantoangle[ SlopeDiv(x1,y1)];
            }
        }
        else
        {
            // y<0
            y1 = -y1;

            if (x1>y1)
            {
                // octant 8
                return -tantoangle[SlopeDiv(y1,x1)];
            }
            else
            {
                // octant 7
                return ANG270+tantoangle[ SlopeDiv(x1,y1)];
            }
        }
    }
    else
    {
        // x<0
        x1 = -x1;

        if (y1>= 0)
        {
            // y>= 0
            if (x1>y1)
            {
                // octant 3
                return ANG180-1-tantoangle[ SlopeDiv(y1,x1)];
            }
            else
            {
                // octant 2
                return ANG90+ tantoangle[ SlopeDiv(x1,y1)];
            }
        }
        else
        {
            // y<0
            y1 = -y1;

            if (x1>y1)
            {
                // octant 4
                return ANG180+tantoangle[ SlopeDiv(y1,x1)];
            }
            else
            {
                 // octant 5
                return ANG270-1-tantoangle[ SlopeDiv(x1,y1)];
            }
        }
    }
    return 0;
}


angle_t Rend::R_PointToAngle(fixed_t x, fixed_t y)
{
  return R_PointToAngle2 (viewx, viewy, x, y);
}


fixed_t R_PointToDist2(fixed_t x2, fixed_t y2, fixed_t x1, fixed_t y1)
{
    int         angle;
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     dist;

    dx = abs(x1 - x2);
    dy = abs(y1 - y2);

    if (dy>dx)
    {
        fixed_t     temp;

        temp = dx;
        dx = dy;
        dy = temp;
    }
    if(dy==0)
       return dx;

    angle = (tantoangle[ FixedDiv(dy,dx)>>DBITS ]+ANG90) >> ANGLETOFINESHIFT;

    // use as cosine
    dist = FixedDiv (dx, finesine[angle] );

    return dist;
}


//SoM: 3/27/2000: Little extra utility. Works in the same way as
//R_PointToAngle2
fixed_t Rend::R_PointToDist(fixed_t x, fixed_t y)
{
  return R_PointToDist2(viewx, viewy, x, y);
}


//
// R_InitPointToAngle
//
void R_InitPointToAngle (void)
{
    // UNUSED - now getting from tables.c
#if 0
    int i;
    long        t;
    float       f;
//
// slope (tangent) to angle lookup
//
    for (i=0 ; i<=SLOPERANGE ; i++)
    {
        f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
        t = 0xffffffff*f;
        tantoangle[i] = t;
    }
#endif
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
//added:02-02-98:note: THIS IS USED ONLY FOR WALLS!
fixed_t Rend::R_ScaleFromGlobalAngle (angle_t visangle)
{
    // UNUSED
#if 0
    //added:02-02-98:note: I've tried this and it displays weird...
    fixed_t             scale;
    fixed_t             dist;
    fixed_t             z;
    fixed_t             sinv;
    fixed_t             cosv;

    sinv = finesine[(visangle-rw_normalangle)>>ANGLETOFINESHIFT];
    dist = FixedDiv (rw_distance, sinv);
    cosv = finecosine[(viewangle-visangle)>>ANGLETOFINESHIFT];
    z = abs(FixedMul (dist, cosv));
    scale = FixedDiv(projection, z);
    return scale;

#else
    fixed_t             scale;
    int                 anglea;
    int                 angleb;
    int                 sinea;
    int                 sineb;
    fixed_t             num;
    int                 den;


    anglea = ANG90 + (visangle-viewangle);
    angleb = ANG90 + (visangle-rw_normalangle);

    // both sines are allways positive
    sinea = finesine[anglea>>ANGLETOFINESHIFT];
    sineb = finesine[angleb>>ANGLETOFINESHIFT];
    //added:02-02-98:now uses projectiony instead of projection for
    //               correct aspect ratio!
    num = FixedMul(projectiony,sineb)<<detailshift;
    den = FixedMul(rw_distance,sinea);

    if (den > num>>16)
    {
        scale = FixedDiv (num, den);

        if (scale > 64*FRACUNIT)
            scale = 64*FRACUNIT;
        else if (scale < 256)
            scale = 256;
    }
    else
        scale = 64*FRACUNIT;

    return scale;
#endif
}



//
// R_InitTables
//
void R_InitTables (void)
{
    // UNUSED: now getting from tables.c
#if 0
    int         i;
    float       a;
    float       fv;
    int         t;

    // viewangle tangent table
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
        fv = FRACUNIT*tan (a);
        t = fv;
        finetangent[i] = t;
    }

    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
        // OPTIMIZE: mirror...
        a = (i+0.5)*PI*2/FINEANGLES;
        t = FRACUNIT*sin (a);
        finesine[i] = t;
    }
#endif

}



//
// R_InitTextureMapping
//
void R_InitTextureMapping()
{
    int                 i;
    int                 x;
    int                 t;
    fixed_t             focallength;

    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv (centerxfrac,
                            finetangent[FINEANGLES/4+/*cv_fov.value*/ FIELDOFVIEW/2] );

    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        if (finetangent[i] > FRACUNIT*2)
            t = -1;
        else if (finetangent[i] < -FRACUNIT*2)
            t = viewwidth+1;
        else
        {
            t = FixedMul (finetangent[i], focallength);
            t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

            if (t < -1)
                t = -1;
            else if (t>viewwidth+1)
                t = viewwidth+1;
        }
        viewangletox[i] = t;
    }

    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.
    for (x=0;x<=viewwidth;x++)
    {
        i = 0;
        while (viewangletox[i]>x)
            i++;
        xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

    // Take out the fencepost cases from viewangletox.
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        t = FixedMul (finetangent[i], focallength);
        t = centerx - t;

        if (viewangletox[i] == -1)
            viewangletox[i] = 0;
        else if (viewangletox[i] == viewwidth+1)
            viewangletox[i]  = viewwidth;
    }

    clipangle = xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP         2

void R_InitLightTables (void)
{
    int         i;
    int         j;
    int         level;
    int         startmap;
    int         scale;

    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTZ ; j++)
        {
            //added:02-02-98:use BASEVIDWIDTH, vid.width is not set already,
            // and it seems it needs to be calculated only once.
            scale = FixedDiv ((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
            scale >>= LIGHTSCALESHIFT;
            level = startmap - scale/DISTMAP;

            if (level < 0)
                level = 0;

            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;

            zlight[i][j] = colormaps + level*256;
        }
    }
}


//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
bool         setsizeneeded;

void R_SetViewSize()
{
  setsizeneeded = true;
}


//
// R_ExecuteSetViewSize
//
// now uses screen variables cv_viewsize, cv_detaillevel
//
void R_ExecuteSetViewSize()
{
  int i, j;

  setsizeneeded = false;

  // no reduced view in splitscreen mode
  if (cv_splitscreen.value && cv_viewsize.value < 11)
    cv_viewsize.Set(11);

#ifdef HWRENDER
  if ((rendermode != render_soft) && (cv_viewsize.value < 6))
    cv_viewsize.Set(6);
#endif

  switch (game.mode)
    {
    case gm_hexen:
      hud.stbarheight = ST_HEIGHT_HEXEN;
      break;
    case gm_heretic:
      hud.stbarheight = ST_HEIGHT_HERETIC;
      break;
    default:
      hud.stbarheight = ST_HEIGHT_DOOM;
      break;
    }

  if (cv_scalestatusbar.value || cv_viewsize.value > 10)
    hud.stbarheight = (int)(hud.stbarheight * (rendermode==render_soft) ? vid.dupy : vid.fdupy);

  //added 01-01-98: full screen view, without statusbar
  if (cv_viewsize.value > 10)
    {
      scaledviewwidth = vid.width;
      viewheight = vid.height;
    }
  else
    {
      //added 01-01-98: always a multiple of eight
      scaledviewwidth = (cv_viewsize.value * vid.width/10) & ~7;
      //added:05-02-98: make viewheight multiple of 2 because sometimes
      //                a line is not refreshed by R_DrawViewBorder()
      viewheight = (cv_viewsize.value*(vid.height-hud.stbarheight)/10) & ~1;
    }

  // added 16-6-98:splitscreen
  if (cv_splitscreen.value)
    viewheight >>= 1;

  int setdetail = cv_detaillevel.value;
  // clamp detail level (actually ignore it, keep it for later who knows)
  if (setdetail)
    {
      setdetail = 0;
      CONS_Printf("lower detail mode n.a.\n");
      cv_detaillevel.Set(setdetail);
    }

  detailshift = setdetail;
  viewwidth = scaledviewwidth>>detailshift;

  centery = viewheight/2;
  centerx = viewwidth/2;
  centerxfrac = centerx<<FRACBITS;
  centeryfrac = centery<<FRACBITS;

  //added:01-02-98:aspect ratio is now correct, added an 'projectiony'
  //      since the scale is not always the same between horiz. & vert.
  projection  = centerxfrac;
  projectiony = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width)<<FRACBITS;

  //
  // no more low detail mode, it used to setup the right drawer routines
  // for either detail mode here
  //
  // if (!detailshift) ... else ...

  R_InitViewBuffer(scaledviewwidth, viewheight);

  R_InitTextureMapping();

#ifdef HWRENDER
  if (rendermode != render_soft)
    HWR_InitTextureMapping();
#endif

  // psprite scales
  centerypsp = viewheight/2;  //added:06-02-98:psprite pos for freelook

  pspritescale  = (viewwidth<<FRACBITS)/BASEVIDWIDTH;
  pspriteiscale = (BASEVIDWIDTH<<FRACBITS)/viewwidth;   // x axis scale
  //added:02-02-98:now aspect ratio correct for psprites
  pspriteyscale = (((vid.height*viewwidth)/vid.width)<<FRACBITS)/BASEVIDHEIGHT;

  // thing clipping
  for (i=0 ; i<viewwidth ; i++)
    screenheightarray[i] = viewheight;

  // setup sky scaling for old/new skies (uses pspriteyscale)
  R_SetSkyScale();

  // planes
  //added:02-02-98:now correct aspect ratio!
  int aspectx = (((vid.height*centerx*BASEVIDWIDTH)/BASEVIDHEIGHT)/vid.width);

  if (rendermode == render_soft)
    {
      // this is only used for planes rendering in software mode
      j = viewheight*4;
      for (i=0 ; i<j ; i++)
    {
      //added:10-02-98:(i-centery) became (i-centery*2) and centery*2=viewheight
      fixed_t dy = ((i-viewheight*2)<<FRACBITS)+FRACUNIT/2;
      dy = abs(dy);
      yslopetab[i] = FixedDiv (aspectx*FRACUNIT, dy);
    }
    }

  for (i=0 ; i<viewwidth ; i++)
    {
      fixed_t cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
      distscale[i] = FixedDiv (FRACUNIT,cosadj);
    }

  // Calculate the light levels to use
  //  for each level / scale combination.
  for (i=0 ; i< LIGHTLEVELS ; i++)
    {
      int startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
      for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
      int level = startmap - j*vid.width/(viewwidth<<detailshift)/DISTMAP;

      if (level < 0)
        level = 0;

      if (level >= NUMCOLORMAPS)
        level = NUMCOLORMAPS-1;

      scalelight[i][j] = colormaps + level*256;
        }
    }

  //faB: continue to do the software setviewsize as long as we use
  //     the reference software view
#ifdef HWRENDER // not win32 only 19990829 by Kin
  if (rendermode!=render_soft)
    HWR_SetViewSize(cv_viewsize.value);
#endif

  hud.ST_Recalc();
  automap.Resize();
}


//
// R_Init
//

static void TestAnims()
{
  // This function can be used to test the integrity of the mobj states (sort of)

  int i, j, k, l, spr;
  mobjinfo_t *info;
  state_t *s, *n;

  const statenum_t mobjinfo_t::*seqptr[9] =
  {
    &mobjinfo_t::spawnstate,
    &mobjinfo_t::seestate,
    &mobjinfo_t::painstate,
    &mobjinfo_t::meleestate,
    &mobjinfo_t::missilestate,
    &mobjinfo_t::deathstate,
    &mobjinfo_t::xdeathstate,
    &mobjinfo_t::crashstate,
    &mobjinfo_t::raisestate
  };
  const char *snames[9] =
  {
    "spawn  ",
    "see    ",
    "pain   ",
    "melee  ",
    "missile",
    "death  ",
    "xdeath ",
    "crash  ",
    "raise  "
  };

  state_t *seq[9];

  for (i=0; i<NUMMOBJTYPES; i++)
    {
      info = &mobjinfo[i];

      s = &states[info->spawnstate];
      spr = s->sprite;
      printf("\n%d: %s\n", i, sprnames[spr]);

      for (j = 0; j<9; j++)
    seq[j] = &states[info->*seqptr[j]];;

      for (j = 0; j<9; j++)
    {
      s = n = seq[j];
      printf(" %s: ", snames[j]);
      if (n == &states[S_NULL])
        {
          printf("(none)\n");
          continue;
        }

      for (k = 0; k < 40; k++)
        {
          if (n == &states[S_NULL])
        {
          printf("S_NULL, %d\n", k);
          break;
        }

          if (n->sprite != spr)
        {
          printf("! name: %s: ", sprnames[n->sprite]);
          spr = n->sprite;
        }

          if (n->tics < 0)
        {
          printf("hold, %d\n", k+1);
          break;
        }

          n = &states[n->nextstate];
          if (n == s)
        {
          printf("loop, %d\n", k+1);
          break;
        }
          else for (l=0; l<9; l++)
        if (n == seq[l] && n != &states[S_NULL])
          break;

          if (l < 9)
        {
          printf("6-loop to %s, %d+\n", snames[l], k+1);
          break;
        }
        }
      if (k == 40)
        printf("l >= 40 !!!\n");
    }
    }

  I_Error("\n ... done.\n");
}


void R_Init()
{
  //TestAnims();

  //added:24-01-98: screensize independent

    if(devparm)
        CONS_Printf ("\nR_InitPointToAngle");
    R_InitPointToAngle ();

    if(devparm)
        CONS_Printf ("\nR_InitTables");
    R_InitTables ();

    R_InitViewBorder ();

    R_SetViewSize ();   // setsizeneeded is set true

    if(devparm)
        CONS_Printf ("\nR_InitPlanes");
    R_InitPlanes ();

    //added:02-02-98: this is now done by SCR_Recalc() at the first mode set
    if(devparm)
        CONS_Printf ("\nR_InitLightTables");
    R_InitLightTables ();

    if(devparm)
        CONS_Printf ("\nR_InitSkyMap");
    R_InitSkyMap ();

    if(devparm)
        CONS_Printf ("\nR_InitTranslationsTables");
    R_InitTranslationTables ();

    R_InitDrawNodes();

    framecount = 0;
}


//
// was R_PointInSubsector
//
subsector_t *Map::R_PointInSubsector(fixed_t x, fixed_t y)
{
    node_t*     node;
    int         side;
    int         nodenum;

    // single subsector is a special case
    if (!numnodes)
        return subsectors;

    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// was R_IsPointInSubsector, same of above but return 0 if not in subsector
//
subsector_t* Map::R_IsPointInSubsector(fixed_t x, fixed_t y)
{
    node_t*     node;
    int         side;
    int         nodenum,i;
    subsector_t *ret;

    // single subsector is a special case
    if (!numnodes)
        return subsectors;

    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
    }

    ret=&subsectors[nodenum & ~NF_SUBSECTOR];
    for(i=0;i<ret->numlines;i++)
    {
        if(R_PointOnSegSide(x,y,&segs[ret->firstline+i]))
            return 0;
    }

    return ret;
}


//
// R_SetupFrame
//

// WARNING : a should be unsigned but to add with 2048, it isn't !
#define AIMINGTODY(a) ((finetangent[(2048+(((int)a)>>ANGLETOFINESHIFT)) & FINEMASK]*160)>>FRACBITS)

void Rend::R_SetupFrame(PlayerInfo *player)
{
  int         i;
  int         fixedcolormap_setup;
  int         dy=0; //added:10-02-98:

  extralight = player->pawn->extralight;

  if (cv_chasecam.value && !camera.chase)
    {
      camera.ResetCamera(player->pawn);
      camera.chase = true;
    }
  else if (!cv_chasecam.value)
    camera.chase = false;

#ifdef FRAGGLESCRIPT
  if (script_camera_on)
    {
      viewactor = script_camera;

      viewz = viewactor->z;
      viewangle = viewactor->angle;
      aimingangle = viewactor->aiming;
      fixedcolormap_setup = script_camera->fixedcolormap;
    }
  else
#endif
    if (camera.chase)
      {
	// use outside cam view
        viewactor = &camera;

        viewz = viewactor->z + (viewactor->height>>1);
        fixedcolormap_setup = camera.fixedcolormap;
        aimingangle = camera.aiming;
        viewangle = viewactor->angle;
      }
    else
      {
	// use the player's eyes view
        viewactor = player->pawn;
        viewz = player->viewz;

        fixedcolormap_setup = player->pawn->fixedcolormap;
        aimingangle = viewactor->aiming;
        viewangle = viewactor->angle+viewangleoffset;
      }

#ifdef PARANOIA
  if (!viewactor)
    I_Error("R_Setupframe : viewactor null (player %d)",player->number);
#endif
  viewplayer = player->pawn;
  viewx = viewactor->x;
  viewy = viewactor->y;

  viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

  sscount = 0;

  if (fixedcolormap_setup)
    {
      fixedcolormap = colormaps + fixedcolormap_setup*256*sizeof(lighttable_t);

      walllights = scalelightfixed;

      for (i=0 ; i<MAXLIGHTSCALE ; i++)
    scalelightfixed[i] = fixedcolormap;
    }
  else
    fixedcolormap = 0;

  //added:06-02-98:recalc necessary stuff for mouseaiming
  //               slopes are already calculated for the full
  //               possible view (which is 4*viewheight).

  if ( rendermode == render_soft )
    {
      // clip it in the case we are looking a hardware 90� full aiming
      // (lmps, nework and use F12...)
      aimingangle = G_ClipAimingPitch(aimingangle);

      if(!cv_splitscreen.value)
	dy = AIMINGTODY(aimingangle)* viewheight/BASEVIDHEIGHT ;
      else
	dy = AIMINGTODY(aimingangle)* viewheight*2/BASEVIDHEIGHT ;

      yslope = &yslopetab[(3*viewheight/2) - dy];
    }
  centery = (viewheight/2) + dy;
  centeryfrac = centery<<FRACBITS;

  framecount++;
  validcount++;
}

#ifdef HORIZONTALDRAW

#define CACHEW 32      // bytes in cache line
#define CACHELINES 32  // cache lines to use
void R_RotateBuffere (void)
{
    byte    bh,bw;
//    int     modulo;
    byte*   src,*srca,*srcr;
    byte*   dest,*destr;
    int     i,dl;


#define modulo 200  //= viewheight;

    srcr  = yhlookup[0];
    destr = ylookup[0] + columnofs[0];

    bh = viewwidth / CACHELINES;
    while (bh--)
    {
        srca = srcr;
        dest = destr;

        bw = viewheight;
        while (bw--)
        {
             src  = srca++;
             for (i=0;i<CACHELINES/4;i++)  // fill 32 cache lines
             {
                 *dest++ = *src;
                 *dest++ = *(src-modulo);
                 *dest++ = *(src-2*modulo);
                 *dest++ = *(src-3*modulo);
                 src -= 4*modulo;
             }
             dest = (dest - CACHELINES) + vid.width;
        }
        srcr  -= (CACHELINES*viewheight);
        destr += CACHELINES;
    }
}
#endif



// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void Rend::R_RenderPlayerView(PlayerInfo *player)
{
    R_SetupFrame (player);

    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes (player->pawn->subsector->sector->tag); //needs OLDwaterheight in occupied sector
    //R_ClearPortals ();
    R_ClearSprites ();

#ifdef FLOORSPLATS
    R_ClearVisibleFloorSplats ();
#endif

    // check for new console commands.
    //NetUpdate ();

    // The head node is the last node output.

//profile stuff ---------------------------------------------------------
#ifdef TIMING
    mytotal=0;
    ProfZeroTimer();
#endif
    R_RenderBSPNode (numnodes-1);
#ifdef TIMING
    RDMSR(0x10,&mycount);
    mytotal += mycount;   //64bit add

    CONS_Printf("RenderBSPNode: 0x%d %d\n", *((int*)&mytotal+1),
                                             (int)mytotal );
#endif
//profile stuff ---------------------------------------------------------

// horizontal column draw optimisation test.. deceiving.
#ifdef HORIZONTALDRAW
//    R_RotateBuffere ();
    dc_source   = yhlookup[0];
    dc_colormap = ylookup[0] + columnofs[0];
    R_RotateBufferasm ();
#endif

    // Check for new console commands.
    //NetUpdate ();

    //R_DrawPortals ();
    R_DrawPlanes ();

    // Check for new console commands.
    //NetUpdate ();

#ifdef FLOORSPLATS
    //faB(21jan): testing
    R_DrawVisibleFloorSplats ();
#endif

    // draw mid texture and sprite
    // SoM: And now 3D floors/sides!
    R_DrawMasked ();

    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!viewangleoffset && !camera.chase && cv_psprites.value
#ifdef FRAGGLESCRIPT
    && !script_camera_on
#endif
    )
      R_DrawPlayerSprites ();

    // Check for new console commands.
    //NetUpdate ();
    player->pawn->flags &= ~MF_NOSECTOR; // don't show self (uninit) clientprediction code
}
