// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// $Log$
// Revision 1.2  2002/12/16 22:07:59  smite-meister
// cr/lf fix
//
// Revision 1.1  2002/12/16 22:06:24  smite-meister
// Added damage system
//
//
// DESCRIPTION:
//   Damage types for Doom Legacy
//
//-----------------------------------------------------------------------------

#ifndef g_damage_h
#define g_damage_h 1

// Damagetypes are not implemented yet
// damagetype bit field
typedef enum {
  // what kind of damage
  dt_kinetic    = 0x0001, // bullets
  dt_crushing   = 0x0002, // fist
  dt_concussion = 0x0004, // shock wave
  dt_cutting    = 0x0008, // claws, shrapnel
  dt_heat       = 0x0010, // imp fireball
  dt_cold       = 0x0020,
  dt_radiation  = 0x0040, // nukage
  dt_biological = 0x0080, // biowarfare agents
  dt_corrosive  = 0x0100, // chemicals, acid
  dt_poison     = 0x0200,
  dt_shock      = 0x0400, // electric shock
  dt_magic      = 0x0800, // ethereal or otherwise strange
  dt_oxygen     = 0x1000, // lack of oxygen
  dt_telefrag   = 0x2000, // telefrag
  dt_typemask   = 0xFFFF,

  // other effects (bit of a hack)
  dt_oldrecoil  = 0x10000, // target is pushed by an amount based on the damage given
  dt_always     = 0x20000, // cannot be avoided, even if in god mode
  dt_othermask  = 0xF0000,
  /*
  // how intense? (armor piercing bullets etc.)
  dt_normal     = 0x000000,
  dt_intense    = 0x010000,
  dt_heavy      = 0x100000,
  dt_devastate  = 0x110000,

  // admission methods?
  dt_local,
  dt_enveloping,
  dt_breathed,
  */

  dt_normal = dt_kinetic + dt_oldrecoil

} damage_t;

#endif
