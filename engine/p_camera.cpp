// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003-2004 by DooM Legacy Team.
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
// $Log$
// Revision 1.9  2004/10/27 17:37:06  smite-meister
// netcode update
//
// Revision 1.8  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.7  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.6  2003/11/12 11:07:19  smite-meister
// Serialization done. Map progression.
//
// Revision 1.5  2003/03/15 20:07:15  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.4  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Chasecam

#include <math.h>

#include "p_camera.h"
#include "g_map.h"
#include "command.h"
#include "cvars.h"
#include "tables.h"
#include "r_main.h"


consvar_t cv_chasecam   = {"chasecam","0",0,CV_OnOff};
consvar_t cv_cam_dist   = {"cam_dist"  ,"128"  ,CV_FLOAT,NULL};
consvar_t cv_cam_height = {"cam_height", "20"   ,CV_FLOAT,NULL};
consvar_t cv_cam_speed  = {"cam_speed" ,  "0.25",CV_FLOAT,NULL};



Camera::Camera()
{
  flags = MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY| MF_NOTRIGGER | MF_NOCLIPTHING | MF_NOTMONSTER | MF_FLOAT;
  flags2 = MF2_SLIDE | MF2_DONTDRAW;

  health = 1000;
  mass = 10*FRACUNIT;
  radius = 20*FRACUNIT;
  height = 16*FRACUNIT;

  x = y = z = 0;

  chase = false;
}


void Camera::ClearCamera()
{
  if (mp)
    Detach();
}


void Camera::Think()
{
  if (target == NULL)
    return;

  if (mp == NULL)
    I_Error("ChaseCamera: no map set\n");

  // TODO warp to target if it's too far away!
  //ResetCamera(p);
  // chasecam is reset when 1) target respawns, 2) target teleports, 3) chasecam is first turned on

  angle_t ang = target->angle;
  fixed_t tx, ty, tz;

  // sets ideal cam pos
  fixed_t dist = cv_cam_dist.value;
  tx = target->x - FixedMul( finecosine[(ang>>ANGLETOFINESHIFT) & FINEMASK], dist);
  ty = target->y - FixedMul(   finesine[(ang>>ANGLETOFINESHIFT) & FINEMASK], dist);
  tz = target->z + (cv_viewheight.value << FRACBITS) + cv_cam_height.value;

  // P_PathTraverse ( target->x, target->y, x, y, PT_ADDLINES, PTR_UseTraverse );

  // move camera down to move under lower ceilings
  subsector_t *newsubsec = mp->R_IsPointInSubsector((target->x + tx) >> 1, (target->y + ty) >> 1);
              
  if (!newsubsec)
    {
      // use player sector 
      if (target->subsector->sector->ceilingheight - height < tz)
	tz = target->subsector->sector->ceilingheight - height - 11*FRACUNIT;
      // don't be blocked by a opened door
    }
  else if (newsubsec->sector->ceilingheight - height < tz)
    // no fit
    tz = newsubsec->sector->ceilingheight - height-11*FRACUNIT;

  // is the camera fit is there own sector
  newsubsec = mp->R_PointInSubsector(tx, ty);
  if (newsubsec->sector->ceilingheight - height < tz)
    tz = newsubsec->sector->ceilingheight - height - 11*FRACUNIT;


  // point viewed by the camera
  // this point is just 64 unit forward the player
  dist = 64 << FRACBITS;
  fixed_t viewpointx, viewpointy;

  viewpointx = target->x + FixedMul( finecosine[(ang>>ANGLETOFINESHIFT) & FINEMASK], dist);
  viewpointy = target->y + FixedMul( finesine[(ang>>ANGLETOFINESHIFT) & FINEMASK], dist);

  angle = R_PointToAngle2(tx, ty, viewpointx, viewpointy);

  // follow the player
  px = FixedMul(tx - x, cv_cam_speed.value);
  py = FixedMul(ty - y, cv_cam_speed.value);
  pz = FixedMul(tz - z, cv_cam_speed.value);

  // compute aiming to look the viewed point
  float f1, f2;
  f1 = FIXED_TO_FLOAT(viewpointx - x);
  f2 = FIXED_TO_FLOAT(viewpointy - y);
  dist = int(sqrt(f1*f1+f2*f2)*FRACUNIT);
  ang = R_PointToAngle2(0, z, dist, target->z + (target->height>>1)
			+ finesine[(target->aiming>>ANGLETOFINESHIFT) & FINEMASK] * 64);

  dist = aiming - ang;
  aiming -= (dist >> 3);

  Actor::Think();
}



// make sure the camera is not outside the world
// and looks at the thing it is supposed to
void Camera::ResetCamera(Actor *p)
{
  chase = true;
  target = p;

  x = p->x;
  y = p->y;
  z = p->z + (cv_viewheight.value << FRACBITS);

  if (mp != p->mp)
    {
      if (mp != NULL)
	ClearCamera();

      // add cam to the map
      p->mp->SpawnActor(this);
      //cam = p->mp->SpawnActor(x,y,z, MT_CHASECAM);
    }

  angle = p->angle;
  aiming = 0;

  // hey we should make sure that the sounds are heard from the camera
  // instead of the marine's head : TODO
}


/*
bool PTR_FindCameraPoint (intercept_t* in)
{
  
  static fixed_t cameraz;
  int         side;
	fixed_t             slope;
	fixed_t             dist;
	line_t*             li;

	li = in->d.line;

	if ( !(li->flags & ML_TWOSIDED) )
        return false;

    // crosses a two sided line
    //added:16-02-98: Fab comments : sets opentop, openbottom, openrange
    //                lowfloor is the height of the lowest floor
    //                         (be it front or back)
    P_LineOpening (li);

    dist = FixedMul (attackrange, in->frac);

    if (li->frontsector->floorheight != li->backsector->floorheight)
    {
    //added:18-02-98: comments :
    // find the slope aiming on the border between the two floors
    slope = FixedDiv (openbottom - cameraz , dist);
    if (slope > aimslope)
    return false;
    }

    if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
    {
    slope = FixedDiv (opentop - shootz , dist);
    if (slope < aimslope)
    goto hitline;
    }

    return true;

    // hit line
    hitline:
  // stop the search
  return false;
}
*/
