// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Revision 1.1  2002/11/16 14:18:28  hurdler
// Initial revision
//
// Revision 1.6  2002/08/13 19:47:45  vberghol
// p_inter.cpp done
//
// Revision 1.5  2002/08/11 17:16:51  vberghol
// ...
//
// Revision 1.4  2002/08/08 12:01:32  vberghol
// pian engine on valmis!
//
// Revision 1.3  2002/07/26 19:23:06  vberghol
// a little something
//
// Revision 1.2  2002/07/23 19:21:45  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.1  2002/07/18 19:16:40  vberghol
// renamed a few files
//
// Revision 1.6  2002/07/14 19:10:33  vberghol
// Illumination!
//
// Revision 1.5  2002/07/12 19:21:40  vberghol
// hop
//
// Revision 1.4  2002/07/01 21:00:44  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.3  2002/07/01 15:01:56  vberghol
// HUD alkaa olla kunnossa
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Thinkers. Now you are in trouble.
//
//-----------------------------------------------------------------------------


#ifndef g_think_h
#define g_think_h 1

#include <stddef.h>

//#ifdef __GNUG__
//#pragma interface
//#endif

// VB: general comments about thinkers

// Thinkers are used to implement all dynamic properties of a Doom map,
// eg. monsters, moving doors etc.

// Thinkers are a sort of C++ -like class construct created in plain C.
// thinker_t can be thought of as an abstract parent class with one virtual method,
// "function" (the doubly-linked list is just some simple extra stuff).
// thinker_t has lots of child "classes" ("classes" that "inherit" it), such as
// mobj_t, ceiling_t, vldoor_t etc.
// Any struct with thinker_t as its first member can be considered a child class of thinker_t.

// The normal way to invoke a thinker descendant is to call its "function" (acp1) with one parameter,
// a pointer to the class itself. This is just like a C++ method call passing the implicit
// 'this' pointer to a method.

// acv is not really used at all (only dummy use)
// acp2 is used to update player weapon state, through the
// player mobj_t state variable, but not by thinkers.



// mobj_t : one function pointer in thinker_t (always P_MobjThinker), another available through state,
//     called _by_ P_MobjThinker.
// What makes mobjs different from one another is their initial attributes and their state cycle ("AI").
// in short, in the case of player mobjs, mobj_t.state is used to implement
// weapon state control and simple sequences where no weapon is required (like dying).
// Otherwise it is used to implement the mobj AI.

// Geometry-related thinker_t children:
// vldoor_t: T_VerticalDoor
// ceiling_t: T_MoveCeiling
// floormove_t: T_MoveFloor
// elevator_t: T_MoveElevator
// plat_t: T_PlatRaise

// fireflicker_t: T_FireFlicker
// lightflash_t: T_LightFlash
// strobe_t: T_StrobeFlash
// glow_t: T_Glow
// lightlevel_t: T_LightFade

// scroll_t: T_Scroll
// friction_t: T_Friction
// pusher_t: T_Pusher

// discrepancies towards a new-class -style mobj creation: P_BlasterMobjThinker (mobj_t)

//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//

typedef void (*actionf_v)(void);
typedef void (*actionf_p1)(void*);
typedef void (*actionf_p2)(void*, void*);

union actionf_t
{
  actionf_v     acv;
  actionf_p1    acp1;
  actionf_p2    acp2;
};

// Historically, "think_t" is yet another
//  function pointer to a routine to handle
//  an actor.
//typedef actionf_t  think_t;

class Map;
class LArchive;

// Doubly linked list of action functions
class Thinker
{
  friend class Map;
private:
  Thinker  *prev;
  Thinker  *next;
public:
  Map *mp; // the map where the thinker is situated

  // a way to tell the actual class of a Thinker descendant
  enum thinkertype_e {
    tt_none  = 0,
    tt_thinker = 1,
    tt_actor = 2,
    tt_pawn  = 3,
    tt_ppawn = 4
  };
  virtual thinkertype_e Type() {return tt_thinker;}; // "name-tag" function

  // constructor and destructor
  Thinker();
  virtual ~Thinker();

  // serialization (save/load)
  virtual int Serialize(LArchive & a);

  // what it actually does;)
  virtual void Think() {}
  //think_t    function;
  //actionf_t  function;

  // memory management
  void *operator new (size_t size);

  // Deallocation is lazy -- it will not actually be freed
  // until its thinking turn comes up.
  void operator delete (void *mem);
};


#endif
