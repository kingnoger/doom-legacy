// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003-2005 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Chasecam

#include <math.h>

#include "command.h"
#include "cvars.h"

#include "g_map.h"
#include "p_camera.h"
#include "r_defs.h"

#include "m_archive.h"

#include "tables.h"


consvar_t cv_chasecam   = {"chasecam","0",0,CV_OnOff};
consvar_t cv_cam_dist   = {"cam_dist"  ,"128"  ,CV_FLOAT,NULL};
consvar_t cv_cam_height = {"cam_height", "20"   ,CV_FLOAT,NULL};
consvar_t cv_cam_speed  = {"cam_speed" ,  "0.25",CV_FLOAT,NULL};


IMPLEMENT_CLASS(Camera, Actor);

Camera::Camera()
  : Actor()
{
  flags = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY | MF_NOCLIPTHING | MF_FLOAT;
  flags2 = MF2_SLIDE | MF2_DONTDRAW;

  health = 1000;
  mass = 10;
  radius = 20;
  height = 16;

  //x = y = z = 0;

  fixedcolormap = 0;
  chase = false;
}


Camera::Camera(mapthing_t *mt)
  : Actor()
{
  flags = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY | MF_NOCLIPTHING | MF_FLOAT;
  flags2 = MF2_SLIDE | MF2_DONTDRAW;

  health = 1000;
  mass = 10;
  radius = 20;
  height = 16;

  pos.x = mt->x;
  pos.y = mt->y;
  yaw = ANG45 * (mt->angle/45);

  spawnpoint = mt;
  mt->mobj = this;

  fixedcolormap = 0;
  chase = false;
}


int Camera::Marshal(LArchive &a)
{
  Actor::Marshal(a);
  a << fixedcolormap << chase;

  return 0;
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

  angle_t ang = target->yaw;

  // sets ideal cam pos
  fixed_t dist = cv_cam_dist.value;
  vec_t<fixed_t> t = target->pos;
  t.x -= Cos(ang) * dist;
  t.y -= Sin(ang) * dist;
  t.z += cv_viewheight.value + cv_cam_height.value;

  // P_PathTraverse ( target->x, target->y, x, y, PT_ADDLINES, PTR_UseTraverse );

  // move camera down to move under lower ceilings
  subsector_t *newsubsec = mp->R_IsPointInSubsector((target->pos.x + t.x) >> 1, (target->pos.y + t.y) >> 1);
              
  if (!newsubsec)
    {
      // use player sector 
      if (target->subsector->sector->ceilingheight - height < t.z)
	t.z = target->subsector->sector->ceilingheight - height - 11;
      // don't be blocked by a opened door
    }
  else if (newsubsec->sector->ceilingheight - height < t.z)
    // no fit
    t.z = newsubsec->sector->ceilingheight - height - 11;

  // is the camera fit is there own sector
  newsubsec = mp->R_PointInSubsector(t.x, t.y);
  if (newsubsec->sector->ceilingheight - height < t.z)
    t.z = newsubsec->sector->ceilingheight - height - 11;


  // point viewed by the camera
  // this point is just 64 unit forward the player
  dist = 64;
  fixed_t viewpointx, viewpointy;

  viewpointx = target->pos.x + Cos(ang) * dist;
  viewpointy = target->pos.y + Sin(ang) * dist;

  yaw = R_PointToAngle2(t.x, t.y, viewpointx, viewpointy);

  // follow the player
  vel = (t-pos) * cv_cam_speed.value;

  // compute aiming to look the viewed point
  float f1 = (viewpointx - pos.x).Float();
  float f2 = (viewpointy - pos.y).Float();
  dist = sqrtf(f1*f1 + f2*f2);

  angle_t temp = pitch - R_PointToAngle2(0, pos.z, dist, target->Center() + Sin(target->pitch) * 64);
  pitch -= (temp >> 3);

  Actor::Think();
}



// make sure the camera is not outside the world
// and looks at the thing it is supposed to
void Camera::ResetCamera(Actor *p)
{
  chase = true;
  target = p;

  pos = p->pos;
  pos.z += cv_viewheight.value;

  if (mp != p->mp)
    {
      if (mp != NULL)
	ClearCamera();

      // add cam to the map
      p->mp->SpawnActor(this);
      //cam = p->mp->SpawnActor(x,y,z, MT_CHASECAM);
    }

  yaw = p->yaw;
  pitch = 0;

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
