// $Id$
//
// chasecam (and maybe other cameras) class implementation

#include "doomdef.h"
#include "p_camera.h"
#include "g_actor.h"
#include "g_map.h"
#include "command.h"
#include "tables.h"
#include "r_main.h"

camera_t camera;

consvar_t cv_cam_dist   = {"cam_dist"  ,"128"  ,CV_FLOAT,NULL};
consvar_t cv_cam_height = {"cam_height", "20"   ,CV_FLOAT,NULL};
consvar_t cv_cam_speed  = {"cam_speed" ,  "0.25",CV_FLOAT,NULL};

short G_ClipAimingPitch(int *aiming);

void camera_t::ClearCamera()
{
  if (cam != NULL)
    {
      cam->Remove();
      cam = NULL;
    }
}

void camera_t::MoveChaseCamera(Actor *p)
{
  fixed_t      x,y,z;
  float        f1,f2;

  if (p == NULL)
    I_Error("MoveChaseCamera: no target\n");

  if (cam == NULL)
    ResetCamera(p);

  angle_t angle = p->angle;

  // sets ideal cam pos
  fixed_t dist = cv_cam_dist.value;
  x = p->x - FixedMul( finecosine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);
  y = p->y - FixedMul(   finesine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);
  z = p->z + (cv_viewheight.value << FRACBITS) + cv_cam_height.value;

/*    P_PathTraverse ( p->x, p->y, x, y, PT_ADDLINES, PTR_UseTraverse );*/

  // move camera down to move under lower ceilings
  subsector_t *newsubsec = p->mp->R_IsPointInSubsector((p->x + cam->x)>>1,(p->y + cam->y)>>1);
              
  if (!newsubsec)
    {
      // use player sector 
      if (p->subsector->sector->ceilingheight - cam->height < z)
	z = p->subsector->sector->ceilingheight - cam->height-11*FRACUNIT;
      // don't be blocked by a opened door
    }
  else if (newsubsec->sector->ceilingheight - cam->height < z)
    // no fit
    z = newsubsec->sector->ceilingheight - cam->height-11*FRACUNIT;

  // is the camera fit is there own sector
  newsubsec = p->mp->R_PointInSubsector(cam->x, cam->y);
  if (newsubsec->sector->ceilingheight - cam->height < z)
    z = newsubsec->sector->ceilingheight - cam->height-11*FRACUNIT;


  // point viewed by the camera
  // this point is just 64 unit forward the player
  dist = 64 << FRACBITS;
  fixed_t viewpointx, viewpointy;

  viewpointx = p->x + FixedMul( finecosine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);
  viewpointy = p->y + FixedMul( finesine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);

  cam->angle = R_PointToAngle2(cam->x, cam->y, viewpointx, viewpointy);

  // follow the player
  cam->px = FixedMul(x - cam->x, cv_cam_speed.value);
  cam->py = FixedMul(y - cam->y, cv_cam_speed.value);
  cam->pz = FixedMul(z - cam->z, cv_cam_speed.value);

  // compute aiming to look the viewed point
  f1 = FIXED_TO_FLOAT(viewpointx - cam->x);
  f2 = FIXED_TO_FLOAT(viewpointy - cam->y);
  dist = sqrt(f1*f1+f2*f2)*FRACUNIT;
  angle = R_PointToAngle2(0, cam->z, dist, p->z + (p->height>>1)
			  + finesine[(p->aiming>>ANGLETOFINESHIFT) & FINEMASK] * 64);

  G_ClipAimingPitch((int *)&angle);
  dist = aiming - angle;
  aiming -= (dist >> 3);
}



//
// was P_MoveCamera :
// make sure the camera is not outside the world
// and looks at the thing it is supposed to
//

void camera_t::ResetCamera(Actor *p)
{
  fixed_t x, y, z;

  chase = true;
  x = p->x;
  y = p->y;
  z = p->z + (cv_viewheight.value << FRACBITS);

  // hey we should make sure that the sounds are heard from the camera
  // instead of the marine's head : TODO

  // set bits for the camera
  if (cam == NULL)
    cam = p->mp->SpawnActor(x,y,z, MT_CHASECAM);
  else
    {
      cam->x = x;
      cam->y = y;
      cam->z = z;
    }

  cam->angle = p->angle;
  aiming = 0;
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
