// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003-2007 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

/// \file
/// \brief Polyobjects.

#include <vector>

#include "g_actor.h"
#include "g_map.h"

#include "p_spec.h"
#include "p_maputl.h"
#include "r_poly.h"
#include "r_defs.h"
#include "m_bbox.h"
#include "tables.h"
#include "sounds.h"
#include "z_zone.h"

#define PO_MAXPOLYSEGS 64


static void ThrustMobj(Actor *mobj, seg_t *seg, polyobj_t *po);
static void UpdateSegBBox(seg_t *seg);


//==========================================================================
//   Poly rotator
//==========================================================================

IMPLEMENT_CLASS(polyobject_t, Thinker);
polyobject_t::polyobject_t() {}

IMPLEMENT_CLASS(polyrotator_t, polyobject_t);
polyrotator_t::polyrotator_t() {}


polyrotator_t::polyrotator_t(int num, byte *args, int dir)
  : polyobject_t(num)
{
  if (args[2])
    {
      if (args[2] == 255)
	dist = -1; // special case, perpetual rotator
      else
	dist = args[2]*((ANG90/64) >> ANGLETOFINESHIFT); // Angle
    }
  else
    dist = ANGLE_MAX >> ANGLETOFINESHIFT;

  speed = dir * (args[1] * ((ANG90/64) >> 3) >> ANGLETOFINESHIFT);
  // max. +-pi/4 per tic, min 4 fineangleunits/tic!
}


fixed_t polyrotator_t::PushForce()
{
  fixed_t res;
  res.setvalue(abs(speed) << 11);
  return res;
}


void polyrotator_t::Think()
{
  polyobj_t *poly = mp->GetPolyobj(polyobj);

  if (dist == -1) // perpetual rotator
    {
      mp->PO_RotatePolyobj(poly, speed << ANGLETOFINESHIFT);
      return;
    }

  // not a perpetual polyobj
  int absSpeed = abs(speed);
  if (dist < absSpeed)
    {
      speed = dist * (speed < 0 ? -1 : 1);
      absSpeed = dist;
    }

  if (mp->PO_RotatePolyobj(poly, speed << ANGLETOFINESHIFT))
    {
      dist -= absSpeed;
      if (dist <= 0)
	{
	  if (poly->specialdata == this)
	    poly->specialdata = NULL;

	  mp->SN_StopSequence(&poly->spawnspot);
	  mp->PolyobjFinished(poly->tag);
	  mp->RemoveThinker(this);
	}
    }
}


bool Map::EV_RotatePoly(byte *args, int direction, bool overRide)
{
  int polynum = args[0];
  polyobj_t *poly = GetPolyobj(polynum);

  if (poly)
    {
      if (poly->specialdata && !overRide)
	// poly is already moving
	return false;
    }
  else
    I_Error("EV_RotatePoly:  Invalid polyobj num: %d\n", polynum);

  polyobject_t *p = new polyrotator_t(polynum, args, direction);
  AddThinker(p);
  poly->specialdata = p;

  SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);

  int mirror;
  while ((mirror = GetPolyobjMirror(polynum)))
    {
      poly = GetPolyobj(mirror);
      if (poly && poly->specialdata && !overRide)
	// mirroring poly is already in motion
	break;

      direction = -direction;

      p = new polyrotator_t(mirror, args, direction);
      AddThinker(p);
      poly->specialdata = p;

      // FIXME WTF?
      /*
      if ((poly = GetPolyobj(polynum)))
	poly->specialdata = p;
      else
	I_Error("EV_RotatePoly:  Invalid polyobj num: %d\n", polynum);
      */

      SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);
      polynum = mirror;
    }
  return true;
}


//==========================================================================
//   Poly mover
//==========================================================================

IMPLEMENT_CLASS(polymover_t, polyobject_t);
polymover_t::polymover_t() {}

polymover_t::polymover_t(int num, byte *args, bool timesEight, bool mirror)
  : polyobject_t(num)
{
  speed = args[1]*0.125f;

  ang = args[2]*(ANG90/64);
  if (mirror)
    ang += ANG180; // reverse the angle

  dist = args[3];
  if (timesEight)
    dist *= 8;

  int angle = ang >> ANGLETOFINESHIFT;
  xs = speed * finecosine[angle];
  ys = speed * finesine[angle];
}


fixed_t polymover_t::PushForce() { return speed >> 3; }


void polymover_t::Think()
{
  polyobj_t *poly = mp->GetPolyobj(polyobj);

  if (dist < speed) // speed is always nonnegative
    {
      speed = dist;
      int angle = ang >> ANGLETOFINESHIFT;
      xs = speed * finecosine[angle];
      ys = speed * finesine[angle];
    }

  if (mp->PO_MovePolyobj(poly, xs, ys))
    {
      dist -= speed;
      if (dist <= 0)
	{
	  if (poly->specialdata == this)
	    {
	      poly->specialdata = NULL;
	    }
	  mp->SN_StopSequence(&poly->spawnspot);
	  mp->PolyobjFinished(poly->tag);
	  mp->RemoveThinker(this);
	}
    }
}


bool Map::EV_MovePoly(byte *args, bool timesEight, bool overRide)
{
  int mirror;

  polyobj_t *poly;

  int polynum = args[0];
  if ((poly = GetPolyobj(polynum)))
    {
      if (poly->specialdata && !overRide)
	{ // poly is already moving
	  return false;
	}
    }
  else
    I_Error("EV_MovePoly:  Invalid polyobj num: %d\n", polynum);

  bool mirrored = false;
  polymover_t *p = new polymover_t(polynum, args, timesEight, mirrored);
  AddThinker(p);
  poly->specialdata = p;
  SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);

  while ((mirror = GetPolyobjMirror(polynum)))
    {
      poly = GetPolyobj(mirror);
      if (poly && poly->specialdata && !overRide)
	{ // mirroring poly is already in motion
	  break;
	}

      mirrored = !mirrored;
      p = new polymover_t(mirror, args, timesEight, mirrored);
      AddThinker(p);
      poly->specialdata = p;

      polynum = mirror;
      SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);
    }
  return true;
}


//==========================================================================
//   Poly doors
//==========================================================================

IMPLEMENT_CLASS(polydoor_rot_t, polyrotator_t);
polydoor_rot_t::polydoor_rot_t() {}

IMPLEMENT_CLASS(polydoor_slide_t, polymover_t);
polydoor_slide_t::polydoor_slide_t() {}


polydoor_rot_t::polydoor_rot_t(int num, byte *args, bool mirror)
  : polyrotator_t(num, args, mirror ? -1 : 1)
{
  closing = false;
  tics = 0;
  waitTics = args[3];

  if (dist == -1)
    dist = 8160; // no perpetual doors...

  totalDist = dist;
}


void polydoor_rot_t::Think()
{
  polyobj_t *poly = mp->GetPolyobj(polyobj);

  if (tics)
    {
      if (!--tics)
	{
	  mp->SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);
	}
      return;
    }

  int absSpeed = abs(speed);
  if (dist < absSpeed)
    {
      speed = dist * (speed < 0 ? -1 : 1);
      absSpeed = dist;
    }

  if (mp->PO_RotatePolyobj(poly, speed))
    {
      dist -= absSpeed;
      if (dist <= 0)
	{
	  mp->SN_StopSequence(&poly->spawnspot);
	  if (!closing)
	    {
	      closing = true;
	      dist = totalDist;
	      tics = waitTics;
	      speed = -speed;
	    }
	  else
	    {
	      if (poly->specialdata == this)
		poly->specialdata = NULL;
	      mp->PolyobjFinished(poly->tag);
		  mp->RemoveThinker(this);
	    }
	}
    }
  else
    {
      if (poly->crush || !closing)
	{ // continue moving if the poly is a crusher, or is opening
	  return;
	}
      else
	{ // open back up and rewait
	  closing = false;
	  dist = totalDist-dist;
	  speed = -speed;
	  mp->SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);
	}
    }
}



polydoor_slide_t::polydoor_slide_t(int num, byte *args, bool mirror)
  : polymover_t(num, args, false, mirror)
{
  closing = false;
  tics = 0;
  waitTics = args[4];
  totalDist = dist;
}


void polydoor_slide_t::Think()
{
  polyobj_t *poly = mp->GetPolyobj(polyobj);

  if (tics)
    {
      if (!--tics)
	{
	  mp->SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);
	}
      return;
    }

  if (dist < speed) // speed is always nonnegative
    {
      speed = dist;
      int angle = ang >> ANGLETOFINESHIFT;
      xs = speed * finecosine[angle];
      ys = speed * finesine[angle];
    }

  if (mp->PO_MovePolyobj(poly, xs, ys))
    {
      dist -= speed;
      if (dist <= 0)
	{
	  mp->SN_StopSequence(&poly->spawnspot);
	  if (!closing)
	    {
	      closing = true;
	      dist = totalDist;
	      tics = waitTics;
	      ang += ANG180; // reverse direction
	      xs = -xs;
	      ys = -ys;
	    }
	  else
	    {
	      if (poly->specialdata == this)
		poly->specialdata = NULL;
	      mp->PolyobjFinished(poly->tag);
	      mp->RemoveThinker(this);
	    }
	}
    }
  else
    {
      if (poly->crush || !closing)
	{ // continue moving if the poly is a crusher, or is opening
	  return;
	}
      else
	{ // open back up
	  closing = false;
	  dist = totalDist-dist;
	  ang += ANG180; // reverse direction
	  xs = -xs;
	  ys = -ys;
	  mp->SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);
	}
    }
}



bool Map::EV_OpenPolyDoor(byte *args, int type)
{
  polyobj_t *poly;

  int polynum = args[0];
  if ((poly = GetPolyobj(polynum)))
    {
      if (poly->specialdata)
	{ // poly is already moving
	  return false;
	}
    }
  else
    I_Error("EV_OpenPolyDoor:  Invalid polyobj num: %d\n", polynum);

  bool mirrored = false;

  polyobject_t *pd;
  if (type == polyobject_t::pd_slide)
    pd = new polydoor_slide_t(polynum, args, mirrored);
  else
    pd = new polydoor_rot_t(polynum, args, mirrored);

  AddThinker(pd);
  poly->specialdata = pd;

  SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);

  int mirror;
  while ((mirror = GetPolyobjMirror(polynum)))
    {
      poly = GetPolyobj(mirror);
      if (poly && poly->specialdata)
	{ // mirroring poly is already in motion
	  break;
	}
      mirrored = !mirrored; // flip

      if (type == polyobject_t::pd_slide)
	pd = new polydoor_slide_t(mirror, args, mirrored);
      else
	pd = new polydoor_rot_t(mirror, args, mirrored);

      AddThinker(pd);
      poly->specialdata = pd;

      SN_StartSequence(&poly->spawnspot, SEQ_DOOR + poly->seqType);

      polynum = mirror;
    }
  return true;
}


//==========================================================================
//   Polyobject utilities
//==========================================================================

polyobj_t *Map::GetPolyobj(int num)
{
  for (int i = 0; i < NumPolyobjs; i++)
    if (polyobjs[i].tag == num)
      return &polyobjs[i];

  return NULL;
}


int Map::GetPolyobjMirror(int num)
{
  for (int i = 0; i < NumPolyobjs; i++)
    if (polyobjs[i].tag == num)
      return ((*polyobjs[i].segs)->linedef->args[1]);

  return 0;
}


bool Map::PO_Busy(int polyobj)
{
  polyobj_t *poly = GetPolyobj(polyobj);
  if (!poly->specialdata)
    return false;
  else
    return true;
}


static void ThrustMobj(Actor *mobj, seg_t *seg, polyobj_t *po)
{
  if (!(mobj->flags & MF_SHOOTABLE))
    return;

  polyobject_t *pe = po->specialdata;
  fixed_t force;
  if (pe)
    {
      force = pe->PushForce();

      if (force < 1)
	force = 1;
      else if (force > 4)
	force = 4;
    }
  else
    force = 1;

  int thrustAngle = (seg->angle-ANG90) >> ANGLETOFINESHIFT;
  fixed_t thrustX = force * finecosine[thrustAngle];
  fixed_t thrustY = force * finesine[thrustAngle];
  mobj->vel.x += thrustX;
  mobj->vel.y += thrustY;

  if (po->crush)
    if (!mobj->TestLocation(mobj->pos.x + thrustX, mobj->pos.y + thrustY)) // was checkposition
      mobj->Damage(NULL, NULL, 3);
}


static void UpdateSegBBox(seg_t *seg)
{
  line_t *line = seg->linedef;

  if (seg->v1->x < seg->v2->x)
    {
      line->bbox.box[BOXLEFT] = seg->v1->x;
      line->bbox.box[BOXRIGHT] = seg->v2->x;
    }
  else
    {
      line->bbox.box[BOXLEFT] = seg->v2->x;
      line->bbox.box[BOXRIGHT] = seg->v1->x;
    }
  if (seg->v1->y < seg->v2->y)
    {
      line->bbox.box[BOXBOTTOM] = seg->v1->y;
      line->bbox.box[BOXTOP] = seg->v2->y;
    }
  else
    {
      line->bbox.box[BOXBOTTOM] = seg->v2->y;
      line->bbox.box[BOXTOP] = seg->v1->y;
    }

  // Update the line's slopetype
  line->dx = line->v2->x - line->v1->x;
  line->dy = line->v2->y - line->v1->y;
  if (!line->dx)
    {
      line->slopetype = ST_VERTICAL;
    }
  else if (!line->dy)
    {
      line->slopetype = ST_HORIZONTAL;
    }
  else
    {
      if (line->dy / line->dx > 0) // agh. could as well multiply.
	{
	  line->slopetype = ST_POSITIVE;
	}
      else
	{
	  line->slopetype = ST_NEGATIVE;
	}
    }
}


//==========================================================================


bool Map::PO_MovePolyobj(polyobj_t *po, fixed_t x, fixed_t y)
{
  int count;

  seg_t **veryTempSeg;

  if (!po)
    I_Error("PO_MovePolyobj:  Invalid polyobj.\n");

  UnLinkPolyobj(po);

  seg_t **segList = po->segs;
  bool blocked = false;

  validcount++;
  for (count = po->numsegs; count; count--, segList++)
    {
      if ((*segList)->linedef->validcount != validcount)
	{
	  (*segList)->linedef->bbox.Move(x, y);
	  (*segList)->linedef->validcount = validcount;
	}
      for (veryTempSeg = po->segs; veryTempSeg != segList;
	  veryTempSeg++)
	{
	  if ((*veryTempSeg)->v1 == (*segList)->v1)
	    {
	      break;
	    }
	}
      if (veryTempSeg == segList)
	{
	  (*segList)->v1->x += x;
	  (*segList)->v1->y += y;
	}
    }
  segList = po->segs;
  for (count = po->numsegs; count; count--, segList++)
    {
      if (PO_CheckBlockingActors(*segList, po))
	{
	  blocked = true;
	}
    }
  if (blocked)
    {
      count = po->numsegs;
      segList = po->segs;
      validcount++;
      while(count--)
	{
	  if ((*segList)->linedef->validcount != validcount)
	    {
	      (*segList)->linedef->bbox.Move(-x, -y);
	      (*segList)->linedef->validcount = validcount;
	    }
	  for (veryTempSeg = po->segs; veryTempSeg != segList;
	      veryTempSeg++)
	    {
	      if ((*veryTempSeg)->v1 == (*segList)->v1)
		break;
	    }
	  if (veryTempSeg == segList)
	    {
	      (*segList)->v1->x -= x;
	      (*segList)->v1->y -= y;
	    }
	  segList++;
	}
      LinkPolyobj(po);
      return false;
    }
  po->spawnspot.x += x;
  po->spawnspot.y += y;
  LinkPolyobj(po);
  return true;
}


//==========================================================================

static void RotatePt(angle_t an, fixed_t& x, fixed_t& y, fixed_t startSpotX, fixed_t startSpotY)
{
  an >>= ANGLETOFINESHIFT;

  fixed_t tx, ty;
  fixed_t gxt, gyt;

  tx = x;
  ty = y;

  gxt = tx * finecosine[an];
  gyt = ty * finesine[an];
  x = (gxt-gyt)+startSpotX;

  gxt = tx * finesine[an];
  gyt = ty * finecosine[an];
  y = (gyt+gxt)+startSpotY;
}



bool Map::PO_RotatePolyobj(polyobj_t *po, angle_t angle)
{
  int count;

  if (!po)
    I_Error("PO_RotatePolyobj:  Invalid polyobj.\n");

  UnLinkPolyobj(po);

  seg_t **segList = po->segs;
  vertex_t *originalPts = po->originalPts;

  vertex_t  prev_points[po->numsegs];
  vertex_t *prevPts = prev_points;

  for (count = po->numsegs; count; count--, segList++, originalPts++, prevPts++)
    {
      prevPts->x = (*segList)->v1->x;
      prevPts->y = (*segList)->v1->y;
      (*segList)->v1->x = originalPts->x;
      (*segList)->v1->y = originalPts->y;
      RotatePt(po->angle + angle, (*segList)->v1->x, (*segList)->v1->y,
	       po->spawnspot.x, po->spawnspot.y);
    }
  segList = po->segs;
  bool blocked = false;
  validcount++;
  for (count = po->numsegs; count; count--, segList++)
    {
      if (PO_CheckBlockingActors(*segList, po))
	{
	  blocked = true;
	}
      if ((*segList)->linedef->validcount != validcount)
	{
	  UpdateSegBBox(*segList);
	  (*segList)->linedef->validcount = validcount;
	}
      (*segList)->angle += angle;
    }
  if (blocked)
    {
      segList = po->segs;
      prevPts = prev_points;
      for (count = po->numsegs; count; count--, segList++, prevPts++)
	{
	  (*segList)->v1->x = prevPts->x;
	  (*segList)->v1->y = prevPts->y;
	}
      segList = po->segs;
      validcount++;
      for (count = po->numsegs; count; count--, segList++)
	{
	  if ((*segList)->linedef->validcount != validcount)
	    {
	      UpdateSegBBox(*segList);
	      (*segList)->linedef->validcount = validcount;
	    }
	  (*segList)->angle -= angle;
	}
      LinkPolyobj(po);
      return false;
    }
  po->angle += angle;
  LinkPolyobj(po);
  return true;
}


//==========================================================================

void Map::UnLinkPolyobj(polyobj_t *po)
{
  // remove the polyobj from each blockmap section
  for (int j = po->bbox[BOXBOTTOM]; j <= po->bbox[BOXTOP]; j++)
    {
      int index = j*bmap.width;
      for (int i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
	{
	  if (i >= 0 && i < bmap.width && j >= 0 && j < bmap.height)
	    {
	      polyblock_t *link = PolyBlockMap[index+i];
	      while(link != NULL && link->polyobj != po)
		link = link->next;

	      if (link == NULL)
		continue; // polyobj not located in the link cell

	      link->polyobj = NULL;
	    }
	}
    }
}


void Map::LinkPolyobj(polyobj_t *po)
{
  fixed_t leftX, rightX, topY, bottomY;
  int i, j;

  // calculate the polyobj bbox
  seg_t **tempSeg = po->segs;
  rightX = leftX = (*tempSeg)->v1->x;
  topY = bottomY = (*tempSeg)->v1->y;

  for (i = 0; i < po->numsegs; i++, tempSeg++)
    {
      if ((*tempSeg)->v1->x > rightX)
	rightX = (*tempSeg)->v1->x;

      if ((*tempSeg)->v1->x < leftX)
	leftX = (*tempSeg)->v1->x;

      if ((*tempSeg)->v1->y > topY)
	topY = (*tempSeg)->v1->y;

      if ((*tempSeg)->v1->y < bottomY)
	bottomY = (*tempSeg)->v1->y;
    }
  po->bbox[BOXRIGHT]  = bmap.BlockX(rightX);
  po->bbox[BOXLEFT]   = bmap.BlockX(leftX);
  po->bbox[BOXTOP]    = bmap.BlockY(topY);
  po->bbox[BOXBOTTOM] = bmap.BlockY(bottomY);
  // add the polyobj to each blockmap section
  for (j = po->bbox[BOXBOTTOM]*bmap.width; j <= po->bbox[BOXTOP]*bmap.width; j += bmap.width)
    {
      for (i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
	{
	  if (i >= 0 && i < bmap.width && j >= 0 && j < bmap.height*bmap.width)
	    {
	      polyblock_t **link = &PolyBlockMap[j+i];
	      polyblock_t *tempLink;

	      if (!(*link))
		{ // Create a new link at the current block cell
		  *link = (polyblock_t *)Z_Malloc(sizeof(polyblock_t), PU_LEVEL, 0);
		  (*link)->next = NULL;
		  (*link)->prev = NULL;
		  (*link)->polyobj = po;
		  continue;
		}
	      else
		{
		  tempLink = *link;
		  while (tempLink->next != NULL && tempLink->polyobj != NULL)
		    tempLink = tempLink->next;
		}
	      if (tempLink->polyobj == NULL)
		{
		  tempLink->polyobj = po;
		  continue;
		}
	      else
		{
		  tempLink->next = (polyblock_t *)Z_Malloc(sizeof(polyblock_t), PU_LEVEL, 0);
		  tempLink->next->next = NULL;
		  tempLink->next->prev = tempLink;
		  tempLink->next->polyobj = po;
		}
	    }
	  // else, don't link the polyobj, since it's off the map
	}
    }
}



bool Map::PO_CheckBlockingActors(seg_t *seg, polyobj_t *po)
{
  Actor *mobj;
  int i, j;

  line_t *ld = seg->linedef;

  int top =    bmap.BlockY(ld->bbox.box[BOXTOP] + MAXRADIUS);
  int bottom = bmap.BlockY(ld->bbox.box[BOXBOTTOM] - MAXRADIUS);
  int left =   bmap.BlockX(ld->bbox.box[BOXLEFT] - MAXRADIUS);
  int right =  bmap.BlockX(ld->bbox.box[BOXRIGHT] + MAXRADIUS);

  bool blocked = false;

  bottom = bottom < 0 ? 0 : bottom;
  bottom = bottom >= bmap.height ? bmap.height-1 : bottom;
  top = top < 0 ? 0 : top;
  top = top >= bmap.height  ? bmap.height-1 : top;
  left = left < 0 ? 0 : left;
  left = left >= bmap.width ? bmap.width-1 : left;
  right = right < 0 ? 0 : right;
  right = right >= bmap.width ?  bmap.width-1 : right;

  for (j = bottom*bmap.width; j <= top*bmap.width; j += bmap.width)
    {
      for (i = left; i <= right; i++)
	{
	  for (mobj = blocklinks[j+i]; mobj; mobj = mobj->bnext)
	    {
	      if (mobj->flags & MF_SOLID)
		{
		  bbox_t box;
		  box.Set(mobj->pos.x, mobj->pos.y, mobj->radius);

		  if (!box.BoxTouchBox(ld->bbox))
		    continue;

		  if (box.BoxOnLineSide(ld) != -1)
		    continue;

		  ThrustMobj(mobj, seg, po);
		  blocked = true;
		}
	    }
	}
    }
  return blocked;
}



void Map::InitPolyBlockMap()
{
  fixed_t leftX, rightX, topY, bottomY;

  for (int i = 0; i < NumPolyobjs; i++)
    {
      LinkPolyobj(&polyobjs[i]);

      // calculate a rough area
      // right now, working like shit...gotta fix this...
      seg_t **segList = polyobjs[i].segs;
      leftX = rightX = (*segList)->v1->x;
      topY = bottomY = (*segList)->v1->y;
      for (int j = 0; j < polyobjs[i].numsegs; j++, segList++)
	{
	  if ((*segList)->v1->x < leftX)
	    {
	      leftX = (*segList)->v1->x;
	    }
	  if ((*segList)->v1->x > rightX)
	    {
	      rightX = (*segList)->v1->x;
	    }
	  if ((*segList)->v1->y < bottomY)
	    {
	      bottomY = (*segList)->v1->y;
	    }
	  if ((*segList)->v1->y > topY)
	    {
	      topY = (*segList)->v1->y;
	    }
	}
      //int area = ((rightX - leftX)*(topY - bottomY)).floor();

      //    fprintf(stdaux, "Area of Polyobj[%d]: %d\n", polyobjs[i].tag, area);
      //    fprintf(stdaux, "\t[%d]\n[%d]\t\t[%d]\n\t[%d]\n", topY>>FRACBITS,
      //    		leftX>>FRACBITS,
      //    	rightX>>FRACBITS, bottomY>>FRACBITS);
    }
}


//==========================================================================

static vector<seg_t *> zzz_polysegs;

// find segs in PO (assume it is a simple cycle, or else...)
int Map::FindPolySegs(seg_t *seg)
{
  vertex_t start = *seg->v1;
  vertex_t end = *seg->v2;

  zzz_polysegs.clear();
  zzz_polysegs.push_back(seg);

  if (end == start)
    return zzz_polysegs.size();

  for (int i = 0; i < numsegs; i++)
    {
      if (segs[i].linedef && // skip minisegs
	  *segs[i].v1 == end)
	{
	  // if PO segs do not form a simple cycle, we may get stuck in infinite loop...
	  if (zzz_polysegs.size() >= PO_MAXPOLYSEGS)
	    I_Error("FindPolySegs:  Polyobj with more than %d segs (or a subcycle) found.\n", PO_MAXPOLYSEGS);

	  zzz_polysegs.push_back(&segs[i]);
	  end = *segs[i].v2;

	  if (end == start)
	    return zzz_polysegs.size();

	  i = -1; //re-start search
	}
    }

  I_Error("FindPolySegs:  Non-closed Polyobj located.\n");
  return 0;
}



bool Map::SpawnPolyobj(polyobj_t *po, int tag, bool crush)
{
  seg_t *seg;

  for (int i = 0; i < numsegs; i++)
    {
      seg = &segs[i];
      // find PO_LINE_START(tag)
      if (seg->linedef && // not miniseg
	  seg->linedef->special == PO_LINE_START &&
	  seg->linedef->args[0] == tag)
	{
	  // mark as used
	  seg->linedef->special = 0;
	  seg->linedef->args[0] = 0;

	  // find segs in PO(tag)
	  po->numsegs = FindPolySegs(seg);
	  po->segs = (seg_t **)Z_Malloc(po->numsegs * sizeof(seg_t *), PU_LEVEL, 0);
	  for (int j=0; j < po->numsegs; j++)
	    po->segs[j] = zzz_polysegs[j]; // store the segs

	  po->crush = crush;
	  po->tag = tag;
	  po->seqType = seg->linedef->args[2];
	  return true;
	}
    }


  // didn't find polyobj(tag) through PO_LINE_START, try PO_LINE_EXPLICIT

  seg_t *polySegList[PO_MAXPOLYSEGS];
  int psIndex = 0;
  for (int j = 1; j < PO_MAXPOLYSEGS; j++) // iterate linedef order numbers
    {
      int psIndexOld = psIndex;
      bool tagfound = false;

      for (int i = 0; i < numsegs; i++)
	{
	  seg = &segs[i];
	  if (seg->linedef && // not miniseg
	      seg->linedef->special == PO_LINE_EXPLICIT &&
	      seg->linedef->args[0] == tag)
	    {
	      tagfound = true;

	      if (!seg->linedef->args[1])
		I_Error("SpawnPolyobj:  Explicit line missing order number (probably %d) in poly %d.\n", j+1, tag);

	      if (seg->linedef->args[1] == j)
		{
		  polySegList[psIndex++] = seg;
		  po->numsegs++;

		  if (psIndex > PO_MAXPOLYSEGS)
		    I_Error("SpawnPolyobj:  psIndex > PO_MAXPOLYSEGS\n");
		}
	    }
	}

      if (psIndex != psIndexOld) // found segs with current order number + tag
	for (int i = 0; i < numsegs; i++)
	  {
	    // Clear out any specials for these segs...we cannot clear them out
	    // in the above loop, since we aren't guaranteed one seg per linedef.

	    seg = &segs[i];
	    if (seg->linedef && // not miniseg
		seg->linedef->special == PO_LINE_EXPLICIT &&
		seg->linedef->args[0] == tag && seg->linedef->args[1] == j)
	      {
		seg->linedef->special = 0;
		seg->linedef->args[0] = 0;
	      }
	  }
      else // no segs found with current order number + tag
	// Check if an explicit line order has been skipped
	// A line has been skipped if there are any more explicit
	// lines with the current tag value
	if (tagfound)
	  I_Error("SpawnPolyobj:  Missing explicit line %d for poly %d\n", j, tag);
    }

  if (po->numsegs)
    {
      po->crush = crush;
      po->tag = tag;
      po->segs = (seg_t **)Z_Malloc(po->numsegs * sizeof(seg_t *), PU_LEVEL, 0);
      for (int i = 0; i < po->numsegs; i++)
	po->segs[i] = polySegList[i];

      po->seqType = po->segs[0]->linedef->args[3];

      // Next, change the polyobjs first line to point to a mirror if it exists
      po->segs[0]->linedef->args[1] = po->segs[0]->linedef->args[2];
      return true;
    }

  return false; // no segs found
}




void Map::TranslateToStartSpot(polyobj_t *po, fixed_t anchorX, fixed_t anchorY)
{
  po->originalPts = (vertex_t *)Z_Malloc(po->numsegs * sizeof(vertex_t), PU_LEVEL, 0);

  fixed_t deltaX = anchorX - po->spawnspot.x;
  fixed_t deltaY = anchorY - po->spawnspot.y;

  vertex_t avg = {0, 0}; // used to find a polyobj's center, and hence subsector

  validcount++;
  for (int i = 0; i < po->numsegs; i++)
    {
      // the linedef bboxes are moved just once
      if (po->segs[i]->linedef->validcount != validcount)
	{
	  po->segs[i]->linedef->bbox.Move(-deltaX, -deltaY);
	  po->segs[i]->linedef->validcount = validcount;
	}

      // multiply used vertices are moved just once
      seg_t **temp;
      for (temp = po->segs; temp != &po->segs[i]; temp++)
	{
	  if ((*temp)->v1 == po->segs[i]->v1)
	    break;
	}
      if (temp == &po->segs[i])
	{ // the point hasn't been translated, yet
	  po->segs[i]->v1->x -= deltaX;
	  po->segs[i]->v1->y -= deltaY;
	}

      // sacrifice some precision
      avg.x += po->segs[i]->v1->x >> fixed_t::FBITS;
      avg.y += po->segs[i]->v1->y >> fixed_t::FBITS;

      // origin is the anchor spot
      po->originalPts[i].x = po->segs[i]->v1->x - po->spawnspot.x;
      po->originalPts[i].y = po->segs[i]->v1->y - po->spawnspot.y;
    }

  avg.x /= po->numsegs;
  avg.y /= po->numsegs;
  subsector_t *sub = R_PointInSubsector(avg.x << fixed_t::FBITS, avg.y << fixed_t::FBITS);

  if (sub->poly != NULL)
    //I_Error("PO_TranslateToStartSpot:  Multiple polyobjs in a single subsector.\n");
    CONS_Printf("Multiple polyobjs (%d) in a single subsector %d (%d)\n", po->tag, sub-subsectors, numsubsectors);

  sub->poly = po;
}




//==========================================================================

vector<mapthing_t *> polyspawn; // temporary list of PO mapthings used during map loading


void Map::InitPolyobjs()
{
  // NOTE: mapthing_t::z stores the polyobj thing type

  int i;
  mapthing_t *mt;

  CONS_Printf("%d Polyobjs\n", NumPolyobjs);

  // allocate the polyobjects
  polyobjs = (polyobj_t *)Z_Malloc(NumPolyobjs * sizeof(polyobj_t), PU_LEVEL, 0);
  memset(polyobjs, 0, NumPolyobjs * sizeof(polyobj_t));

  int index = 0; // index polyobj number
  int n = polyspawn.size();

  // Find the spawn spots, and spawn each polyobj.
  // For a EN_PO_SPAWN* thing, the "angle" field contains the PO tag.
  for (i=0; i<n; i++)
    {
      mt = polyspawn[i];
      if (mt->z == EN_PO_SPAWN || mt->z == EN_PO_SPAWNCRUSH)
	{ // Polyobj StartSpot Pt.
	  polyobjs[index].spawnspot.x = mt->x;
	  polyobjs[index].spawnspot.y = mt->y;
	  if (!SpawnPolyobj(&polyobjs[index], mt->angle, mt->z == EN_PO_SPAWNCRUSH))
	    I_Error("InitPolyobjs:  No lines found for PO %d!\n", mt->angle);
	  //CONS_Printf("Polyobj %d: tag = %d\n", index, mt->angle);
	  index++;
	}
    }

  // then find anchor spots, and translate the polyobjs
  for (i=0; i<n; i++)
    {
      mt = polyspawn[i];
      if (mt->z == EN_PO_ANCHOR)
	{
	  int tag = mt->angle;
	  polyobj_t *po = GetPolyobj(tag);

	  if (po)
	    TranslateToStartSpot(po, mt->x, mt->y);
	  else
	    CONS_Printf("InitPolyobjs:  Unused anchor point (tag: %d)\n", tag);
	}
    }

  // check for a startspot without an anchor point
  for (i = 0; i < NumPolyobjs; i++)
    if (!polyobjs[i].originalPts)
      {
	I_Error("InitPolyobjs:  PO without an anchor point: %d\n",
		polyobjs[i].tag);
      }

  // clean up for next level
  polyspawn.clear();

  InitPolyBlockMap();
}


