// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
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
// $Log$
// Revision 1.21  2005/09/11 16:22:54  smite-meister
// template classes
//
// Revision 1.20  2005/06/23 19:42:15  smite-meister
// obscure Hexen bugs fixed
//
// Revision 1.18  2005/03/10 22:28:43  smite-meister
// poly renderer
//
// Revision 1.17  2004/11/09 20:38:50  smite-meister
// added packing to I/O structs
//
// Revision 1.16  2004/11/04 21:12:52  smite-meister
// save/load fixed
//
// Revision 1.15  2004/10/14 19:35:30  smite-meister
// automap, bbox_t
//
// Revision 1.14  2004/04/25 16:26:49  smite-meister
// Doxygen
//
// Revision 1.12  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.10  2003/11/12 11:07:23  smite-meister
// Serialization done. Map progression.
//
// Revision 1.9  2003/06/20 20:56:07  smite-meister
// Presentation system tweaked
//
// Revision 1.7  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.6  2003/06/01 18:56:30  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.5  2003/05/30 13:34:46  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.4  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.3  2003/04/24 20:30:15  hurdler
// Remove lots of compiling warnings
//
// Revision 1.2  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.1  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Polyobjects

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

IMPLEMENT_CLASS(polyrotator_t, Thinker);
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

  speed = dir * args[1] * (ANG90 >> 9) >> ANGLETOFINESHIFT;
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
  if (mp->PO_RotatePolyobj(polyobj, speed << ANGLETOFINESHIFT))
    {
      int absSpeed = abs(speed);

      if (dist == -1)
	{ // perpetual polyobj
	  return;
	}
      dist -= absSpeed;
      if (dist <= 0)
	{
	  polyobj_t *poly = mp->GetPolyobj(polyobj);
	  if (poly->specialdata == this)
	    poly->specialdata = NULL;

	  mp->SN_StopSequence(&poly->startSpot);
	  mp->PolyobjFinished(poly->tag);
	  mp->RemoveThinker(this);
	}

      if (dist < absSpeed)
	speed = dist * (speed < 0 ? -1 : 1);
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

  SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
	
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

      if ((poly = GetPolyobj(polynum)))
	poly->specialdata = p;
      else
	I_Error("EV_RotatePoly:  Invalid polyobj num: %d\n", polynum);

      polynum = mirror;
      SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
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
  polyobj_t *poly;

  if (mp->PO_MovePolyobj(polyobj, xs, ys))
    {
      fixed_t absSpeed = abs(speed);
      dist -= absSpeed;
      if (dist <= 0)
	{
	  poly = mp->GetPolyobj(polyobj);
	  if (poly->specialdata == this)
	    {
	      poly->specialdata = NULL;
	    }
	  mp->SN_StopSequence(&poly->startSpot);
	  mp->PolyobjFinished(poly->tag);
	  mp->RemoveThinker(this);
	}
      if (dist < absSpeed)
	{
	  speed = dist * (speed < 0 ? -1 : 1);
	  int angle = ang >> ANGLETOFINESHIFT;
	  xs = speed * finecosine[angle];
	  ys = speed * finesine[angle];
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
  SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);

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
      SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
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
  polyobj_t *poly;

  if (tics)
    {
      if (!--tics)
	{
	  poly = mp->GetPolyobj(polyobj);
	  mp->SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
	}
      return;
    }

  if (mp->PO_RotatePolyobj(polyobj, speed))
    {
      int absSpeed = abs(speed);
      dist -= absSpeed;
      if (dist <= 0)
	{
	  poly = mp->GetPolyobj(polyobj);
	  mp->SN_StopSequence(&poly->startSpot);
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
      poly = mp->GetPolyobj(polyobj);
      if (poly->crush || !closing)
	{ // continue moving if the poly is a crusher, or is opening
	  return;
	}
      else
	{ // open back up and rewait
	  closing = false;
	  dist = totalDist-dist;
	  speed = -speed;
	  mp->SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
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
  polyobj_t *poly;

  if (tics)
    {
      if (!--tics)
	{
	  poly = mp->GetPolyobj(polyobj);
	  mp->SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
	}
      return;
    }

  if (mp->PO_MovePolyobj(polyobj, xs, ys))
    {
      fixed_t absSpeed = abs(speed);
      dist -= absSpeed;
      if (dist <= 0)
	{
	  poly = mp->GetPolyobj(polyobj);
	  mp->SN_StopSequence(&poly->startSpot);
	  if (!closing)
	    {
	      closing = true;
	      dist = totalDist;
	      tics = waitTics;
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
      poly = mp->GetPolyobj(polyobj);
      if (poly->crush || !closing)
	{ // continue moving if the poly is a crusher, or is opening
	  return;
	}
      else
	{ // open back up
	  closing = false;
	  dist = totalDist-dist;
	  xs = -xs;
	  ys = -ys;
	  mp->SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);
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

  SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);

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

      SN_StartSequence(&poly->startSpot, SEQ_DOOR + poly->seqType);

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
    if (!mobj->CheckPosition(mobj->pos.x + thrustX, mobj->pos.y + thrustY))
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


bool Map::PO_MovePolyobj(int num, fixed_t x, fixed_t y)
{
  int count;

  seg_t **veryTempSeg;
  polyobj_t *po;

  if (!(po = GetPolyobj(num)))
    I_Error("PO_MovePolyobj:  Invalid polyobj number: %d\n", num);

  UnLinkPolyobj(po);

  seg_t **segList = po->segs;
  vertex_t *prevPts = po->prevPts;
  bool blocked = false;

  validcount++;
  for (count = po->numsegs; count; count--, segList++, prevPts++)
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
      (*prevPts).x += x; // previous points are unique for each seg
      (*prevPts).y += y;
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
      prevPts = po->prevPts;
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
	  (*prevPts).x -= x;
	  (*prevPts).y -= y;
	  segList++;
	  prevPts++;
	}
      LinkPolyobj(po);
      return false;
    }
  po->startSpot.x += x;
  po->startSpot.y += y;
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



bool Map::PO_RotatePolyobj(int num, angle_t angle)
{
  int count;
  polyobj_t *po;

  if (!(po = GetPolyobj(num)))
    I_Error("PO_RotatePolyobj:  Invalid polyobj number: %d\n", num);

  UnLinkPolyobj(po);

  seg_t **segList = po->segs;
  vertex_t *originalPts = po->originalPts;
  vertex_t *prevPts = po->prevPts;

  for (count = po->numsegs; count; count--, segList++, originalPts++, prevPts++)
    {
      prevPts->x = (*segList)->v1->x;
      prevPts->y = (*segList)->v1->y;
      (*segList)->v1->x = originalPts->x;
      (*segList)->v1->y = originalPts->y;
      RotatePt(po->angle + angle, (*segList)->v1->x, (*segList)->v1->y,
	       po->startSpot.x, po->startSpot.y);
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
      prevPts = po->prevPts;
      for (count = po->numsegs; count; count--, segList++, prevPts++)
	{
	  (*segList)->v1->x = prevPts->x;
	  (*segList)->v1->y = prevPts->y;
	}
      segList = po->segs;
      validcount++;
      for (count = po->numsegs; count; count--, segList++, prevPts++)
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
      int index = j*bmapwidth;
      for (int i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
	{
	  if (i >= 0 && i < bmapwidth && j >= 0 && j < bmapheight)
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
  po->bbox[BOXRIGHT]  = (rightX-bmaporgx).floor()  >> MAPBLOCKBITS;
  po->bbox[BOXLEFT]   = (leftX-bmaporgx).floor()   >> MAPBLOCKBITS;
  po->bbox[BOXTOP]    = (topY-bmaporgy).floor()    >> MAPBLOCKBITS;
  po->bbox[BOXBOTTOM] = (bottomY-bmaporgy).floor() >> MAPBLOCKBITS;
  // add the polyobj to each blockmap section
  for (j = po->bbox[BOXBOTTOM]*bmapwidth; j <= po->bbox[BOXTOP]*bmapwidth; j += bmapwidth)
    {
      for (i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
	{
	  if (i >= 0 && i < bmapwidth && j >= 0 && j < bmapheight*bmapwidth)
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

  int top =    ((ld->bbox.box[BOXTOP]-bmaporgy).floor() +MAXRADIUS) >> MAPBLOCKBITS;
  int bottom = ((ld->bbox.box[BOXBOTTOM]-bmaporgy).floor() -MAXRADIUS) >> MAPBLOCKBITS;
  int left =   ((ld->bbox.box[BOXLEFT]-bmaporgx).floor() -MAXRADIUS) >> MAPBLOCKBITS;
  int right =  ((ld->bbox.box[BOXRIGHT]-bmaporgx).floor() +MAXRADIUS) >> MAPBLOCKBITS;

  bool blocked = false;

  bottom = bottom < 0 ? 0 : bottom;
  bottom = bottom >= bmapheight ? bmapheight-1 : bottom;
  top = top < 0 ? 0 : top;
  top = top >= bmapheight  ? bmapheight-1 : top;
  left = left < 0 ? 0 : left;
  left = left >= bmapwidth ? bmapwidth-1 : left;
  right = right < 0 ? 0 : right;
  right = right >= bmapwidth ?  bmapwidth-1 : right;

  for (j = bottom*bmapwidth; j <= top*bmapwidth; j += bmapwidth)
    {
      for (i = left; i <= right; i++)
	{
	  for (mobj = blocklinks[j+i]; mobj; mobj = mobj->bnext)
	    {
	      if (mobj->flags & MF_SOLID)
		{
		  tmb.Set(mobj->pos.x, mobj->pos.y, mobj->radius); 

		  if (!tmb.BoxTouchBox(ld->bbox))
		    continue;

		  if (tmb.BoxOnLineSide(ld) != -1)
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
      int area = ((rightX - leftX)*(topY - bottomY)).floor();

      //    fprintf(stdaux, "Area of Polyobj[%d]: %d\n", polyobjs[i].tag, area);
      //    fprintf(stdaux, "\t[%d]\n[%d]\t\t[%d]\n\t[%d]\n", topY>>FRACBITS, 
      //    		leftX>>FRACBITS,
      //    	rightX>>FRACBITS, bottomY>>FRACBITS);
    }
}

//==========================================================================
//
// IterFindPolySegs
//
//              Passing NULL for segList will cause IterFindPolySegs to
//      count the number of segs in the polyobj
//==========================================================================

static int PolySegCount;
static fixed_t PolyStartX;
static fixed_t PolyStartY;

void Map::IterFindPolySegs(fixed_t x, fixed_t y, seg_t **segList)
{
  if (x == PolyStartX && y == PolyStartY)
    return;

  for (int i = 0; i < numsegs; i++)
    {
      if (segs[i].v1->x == x && segs[i].v1->y == y)
	{
	  if (!segList)
	    PolySegCount++;
	  else
	    *segList++ = &segs[i];

	  IterFindPolySegs(segs[i].v2->x, segs[i].v2->y, segList);
	  return;
	}
    }
  I_Error("IterFindPolySegs:  Non-closed Polyobj located.\n");
}



void Map::SpawnPolyobj(int index, int tag, bool crush)
{
  int i, j;
  seg_t *polySegList[PO_MAXPOLYSEGS];

  for (i = 0; i < numsegs; i++)
    {
      if (segs[i].linedef->special == PO_LINE_START &&
	 segs[i].linedef->args[0] == tag)
	{
	  if (polyobjs[index].segs)
	    I_Error("SpawnPolyobj:  Polyobj %d already spawned.\n", tag);

	  segs[i].linedef->special = 0;
	  segs[i].linedef->args[0] = 0;
	  PolySegCount = 1;
	  PolyStartX = segs[i].v1->x;
	  PolyStartY = segs[i].v1->y;
	  //CONS_Printf(" xxx seg(%d) v1 = %d, line(%d) v1 = %d\n", i, segs[i].v1 - vertexes, segs[i].linedef - lines, segs[i].linedef->v1 - vertexes);
	  IterFindPolySegs(segs[i].v2->x, segs[i].v2->y, NULL);

	  polyobjs[index].numsegs = PolySegCount;
	  polyobjs[index].segs = (seg_t **)Z_Malloc(PolySegCount*sizeof(seg_t *), PU_LEVEL, 0);
	  *(polyobjs[index].segs) = &segs[i]; // insert the first seg
	  IterFindPolySegs(segs[i].v2->x, segs[i].v2->y, polyobjs[index].segs+1);
	  polyobjs[index].crush = crush;
	  polyobjs[index].tag = tag;
	  polyobjs[index].seqType = segs[i].linedef->args[2];
	  //CONS_Printf("--- %d\n", PolySegCount);
	  /*
	    // not necessary
	  if (polyobjs[index].seqType >= SEQTYPE_NUMSEQ)
	    polyobjs[index].seqType = 0;
	  */

	  break;
	}
    }

  if (!polyobjs[index].segs)
    { // didn't find a polyobj through PO_LINE_START
      int psIndex = 0;
      polyobjs[index].numsegs = 0;
      for (j = 1; j < PO_MAXPOLYSEGS; j++)
	{
	  int psIndexOld = psIndex;
	  for (i = 0; i < numsegs; i++)
	    {
	      if (segs[i].linedef->special == PO_LINE_EXPLICIT &&
		  segs[i].linedef->args[0] == tag)
		{
		  if (!segs[i].linedef->args[1])
		    I_Error("SpawnPolyobj:  Explicit line missing order number (probably %d) in poly %d.\n",
			    j+1, tag);

		  if (segs[i].linedef->args[1] == j)
		    {
		      polySegList[psIndex] = &segs[i];
		      polyobjs[index].numsegs++;
		      psIndex++;
		      if (psIndex > PO_MAXPOLYSEGS)
			I_Error("SpawnPolyobj:  psIndex > PO_MAXPOLYSEGS\n");
		    }
		}
	    }
	  // Clear out any specials for these segs...we cannot clear them out
	  // 	in the above loop, since we aren't guaranteed one seg per
	  //		linedef.
	  for (i = 0; i < numsegs; i++)
	    {
	      if (segs[i].linedef->special == PO_LINE_EXPLICIT &&
		 segs[i].linedef->args[0] == tag && segs[i].linedef->args[1] == j)
		{
		  segs[i].linedef->special = 0;
		  segs[i].linedef->args[0] = 0;
		}
	    }
	  if (psIndex == psIndexOld)
	    { // Check if an explicit line order has been skipped
				// A line has been skipped if there are any more explicit
				// lines with the current tag value
	      for (i = 0; i < numsegs; i++)
		{
		  if (segs[i].linedef->special == PO_LINE_EXPLICIT &&
		     segs[i].linedef->args[0] == tag)
		    {
		      I_Error("SpawnPolyobj:  Missing explicit line %d for poly %d\n",
			      j, tag);
		    }
		}
	    }
	}
      if (polyobjs[index].numsegs)
	{
	  PolySegCount = polyobjs[index].numsegs; // PolySegCount used globally
	  polyobjs[index].crush = crush;
	  polyobjs[index].tag = tag;
	  polyobjs[index].segs = (seg_t **)Z_Malloc(polyobjs[index].numsegs*sizeof(seg_t *), PU_LEVEL, 0);
	  for (i = 0; i < polyobjs[index].numsegs; i++)
	    polyobjs[index].segs[i] = polySegList[i];

	  polyobjs[index].seqType = (*polyobjs[index].segs)->linedef->args[3];
	}
      // Next, change the polyobjs first line to point to a mirror
      //		if it exists
      (*polyobjs[index].segs)->linedef->args[1] =
	(*polyobjs[index].segs)->linedef->args[2];
    }
}



void Map::TranslateToStartSpot(int tag, fixed_t originX, fixed_t originY)
{
  int i;

  polyobj_t *po = NULL;
  for (i = 0; i < NumPolyobjs; i++)
    {
      if (polyobjs[i].tag == tag)
	{
	  po = &polyobjs[i];
	  break;
	}
    }
  if (!po)
    // didn't match the tag with a polyobj tag
    I_Error("TranslateToStartSpot:  Unable to match polyobj tag: %d\n", tag);

  if (po->segs == NULL)
    I_Error("TranslateToStartSpot:  Anchor point located without a StartSpot point: %d\n", tag);

  po->originalPts = (vertex_t *)Z_Malloc(po->numsegs*sizeof(vertex_t), PU_LEVEL, 0);
  po->prevPts = (vertex_t *)Z_Malloc(po->numsegs*sizeof(vertex_t), PU_LEVEL, 0);
  fixed_t deltaX = originX-po->startSpot.x;
  fixed_t deltaY = originY-po->startSpot.y;
  //CONS_Printf("origin x,y = %d,%d \n", originX >> FRACBITS, originY >> FRACBITS);
  //CONS_Printf("delta x,y = %d,%d \n", deltaX >> FRACBITS, deltaY >> FRACBITS);
  seg_t **tempSeg = po->segs;
  seg_t **veryTempSeg;
  vertex_t *tempPt = po->originalPts;
  vertex_t avg; // used to find a polyobj's center, and hence subsector
  avg.x = 0;
  avg.y = 0;

  validcount++;
  for (i = 0; i < po->numsegs; i++, tempSeg++, tempPt++)
    {
      if ((*tempSeg)->linedef->validcount != validcount)
	{
	  (*tempSeg)->linedef->bbox.Move(-deltaX, -deltaY);
	  (*tempSeg)->linedef->validcount = validcount;
	}
      for (veryTempSeg = po->segs; veryTempSeg != tempSeg; veryTempSeg++)
	{
	  if ((*veryTempSeg)->v1 == (*tempSeg)->v1)
	    break;
	}
      if (veryTempSeg == tempSeg)
	{ // the point hasn't been translated, yet
	  (*tempSeg)->v1->x -= deltaX;
	  (*tempSeg)->v1->y -= deltaY;
	}
      //CONS_Printf("tempseg x = %d, ", (*tempSeg)->v1->x>>FRACBITS);
      //CONS_Printf("tempseg y = %d\n", (*tempSeg)->v1->y>>FRACBITS);
      // sacrifice some precision
      avg.x += (*tempSeg)->v1->x>>fixed_t::FBITS;
      avg.y += (*tempSeg)->v1->y>>fixed_t::FBITS;
      // the original Pts are based off the startSpot Pt, and are
      // unique to each seg, not each linedef
      tempPt->x = (*tempSeg)->v1->x-po->startSpot.x;
      tempPt->y = (*tempSeg)->v1->y-po->startSpot.y;
    }
  avg.x /= po->numsegs;
  avg.y /= po->numsegs;
  //CONS_Printf("avg x,y = %d,%d\n", avg.x, avg.y);
  subsector_t *sub = R_PointInSubsector(avg.x<<fixed_t::FBITS, avg.y<<fixed_t::FBITS);

  // FIXME errors in polyobj spawning
  if (sub->poly != NULL)
    //I_Error("PO_TranslateToStartSpot:  Multiple polyobjs in a single subsector.\n");
    CONS_Printf("Multiple polyobjs (%d) in a single subsector %d (%d)\n", tag, sub-subsectors, numsubsectors);

  sub->poly = po;
}




//==========================================================================

vector<mapthing_t *> polyspawn; // temporary list of PO mapthings used during map loading

void Map::InitPolyobjs()
{
  int i, n;
  mapthing_t  *mt;

  // allocate the polyobjects
  polyobjs = (polyobj_t *)Z_Malloc(NumPolyobjs * sizeof(polyobj_t), PU_LEVEL, 0);
  memset(polyobjs, 0, NumPolyobjs * sizeof(polyobj_t));

  int index = 0; // index polyobj number
  CONS_Printf("%d Polyobjs\n", NumPolyobjs);
  // Find the startSpot points, and spawn each polyobj
  n = polyspawn.size();
  for (i=0; i<n; i++)
    {
      mt = polyspawn[i];
      if (mt->type == PO_SPAWN_TYPE || mt->type == PO_SPAWNCRUSH_TYPE)
	{ // Polyobj StartSpot Pt.
	  polyobjs[index].startSpot.x = mt->x;
	  polyobjs[index].startSpot.y = mt->y;
	  SpawnPolyobj(index, mt->angle, (mt->type == PO_SPAWNCRUSH_TYPE));
	  //CONS_Printf("Polyobj %d: tag = %d\n", index, mt->angle);
	  index++;
	}
      //CONS_Printf(" xyz = (%d %d %d), angle = %d, tid = %d\n", mt->x, mt->y, mt->z, mt->angle, mt->tid);
    }

  for (i=0; i<n; i++)
    {
      mt = polyspawn[i];
      if (mt->type == PO_ANCHOR_TYPE)
	TranslateToStartSpot(mt->angle, mt->x, mt->y);
      mt->type = 0; // so that it won't interfere with the spawning of the real THINGS
    }

  // check for a startspot without an anchor point
  for (i = 0; i < NumPolyobjs; i++)
    if (!polyobjs[i].originalPts)
      {
	I_Error("InitPolyobjs:  StartSpot located without an Anchor point: %d\n",
		polyobjs[i].tag);
      }

  // clean up for next level
  polyspawn.clear();

  InitPolyBlockMap();
}


