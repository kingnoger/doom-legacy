// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by DooM Legacy Team.
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
// Revision 1.8  2003/11/13 00:32:41  smite-meister
// Static initialization order fiasco fixed
//
// Revision 1.7  2003/11/12 11:07:27  smite-meister
// Serialization done. Map progression.
//
// Revision 1.6  2003/05/30 13:34:49  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.5  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.4  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.3  2002/12/23 23:19:37  smite-meister
// Weapon groups, MAPINFO parser, WAD2+WAD3 support added!
//
// Revision 1.2  2002/12/16 22:04:55  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:28  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   TypeInfo. Thinkers. It starts.
//
//-----------------------------------------------------------------------------


#ifndef g_think_h
#define g_think_h 1

//==========================================================
// The information here applies to the original C code.
// It is saved for learning purposes.
//==========================================================

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

// note also P_BlasterMobjThinker


#include <map>
#include <vector>

using namespace std;

class Thinker;

//==========================================================
// TypeInfo
// A RTTI helper class for Thinkers, used in serialization.

// When a Thinker class is implemented, the corresponding typeinfo object
// should be (statically) constructed (which adds its code to the map etc.)

class TypeInfo
{
private:
  typedef Thinker* (*thinker_factory_t)();
  static map<unsigned, TypeInfo*>& id_map(); // mapping from ID numbers to TypeInfo instances

public:
  unsigned           type_id; // class/type ID number
  const char        *name;    // a plaintext name for the class/type
  thinker_factory_t  factory; // well, a factory for the class
  // could contain even more info about the class

  TypeInfo(const char *n, thinker_factory_t f);
  static TypeInfo *Find(unsigned code);
};

class LArchive;

// macro used in declaring Thinker classes (defines the TypeInfo/serialization related members)
#define DECLARE_CLASS(cls) \
protected: \
  static TypeInfo _type; \
  static inline Thinker *Create() { return new (cls); }; \
public: \
  virtual TypeInfo *_Type() const { return &_type; }; \
  cls(); \
  virtual int Marshal(LArchive &a)



// this macro implements the stuff declared in DECLARE_CLASS
#define IMPLEMENT_CLASS(c,name) \
TypeInfo c::_type((name), c::Create)



//==========================================================
// Thinker: All dynamic map elements, stored in a doubly linked list.
// Base class for most game objects.
class Thinker
{
  friend class Map;

  DECLARE_CLASS(Thinker);

private:
  Thinker  *prev;
  Thinker  *next;

public:
  Map *mp; // the map where the thinker is situated

  // a way to tell the actual class of a Thinker descendant
  enum thinkertype_e
  {
    tt_none = 0,
    tt_thinker,
    tt_actor,
    tt_pawn,
    tt_ppawn,
    tt_dactor,
    tt_other
  };
  // TODO should be replaced by _Type()
  virtual thinkertype_e Type() {return tt_thinker;}; // "name-tag" function

  // constructor and destructor
  virtual ~Thinker();

  // serialization (save/load)
  static int Serialize(Thinker *p, LArchive &a);
  static Thinker *Unserialize(LArchive &a);

  // what it actually does :)
  virtual void Think() {}

  // pointer management
  virtual void CheckPointers() {}

  // memory management
  void *operator new(size_t size);
  void  operator delete(void *mem);
};


#endif
