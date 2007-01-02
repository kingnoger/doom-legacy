// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by DooM Legacy Team.
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
/// \brief ActorInfo class definition.

#include <string>
#include <map>
#include "dictionary.h"
#include "info.h"

using namespace std;

/// \brief DActor type definition. Defines a DECORATE class. Replaces mobjinfo_t.
/// \ingroup g_thing
/*!
  The only reason we still have mobjinfo_t is the mobjinfo table, which is converted into
  a pair of std::maps at program start. Later it could (along with the state table) be replaced
  using DECORATE files, but it is a huge effort and I have better things to do.
 */
class ActorInfo
{
protected:
  mobjtype_t   mobjtype; ///< Old mobjtype_t number.
  char    classname[64]; ///< Name of the DECORATE class, identical to the ZDoom equivalents if possible.

public:
  string       obituary; ///< Obituary message for players killed by "instances" of this DECORATE class.
  string      modelname; ///< Name of MD3 model to be used to represent this class.
  bool     spawn_always; ///< Do not care about mapthing "when-to-spawn" flags.

public:
  // Old mobjinfo_t members, so that this class is semantically identical to mobjinfo_t and can replace it.
  int     doomednum;     ///< Editor number for this thing type or -1 if none.
  int     spawnhealth;   ///< Initial health.
  int     reactiontime;  ///< How soon (in tics) will the thing do something again.
  int     painchance;    ///< Probability of going into painstate when hurt.
  float   speed;         ///< Max. movement speed.
  fixed_t radius;        ///< Thing radius.
  fixed_t height;        ///< Thing height.
  float   mass;          ///< Just take a guess.
  Uint32  damage;        ///< Low 16 bits: damage for missiles, high 16 bits: see damage_t.

  Uint32  flags;         ///< mobjflag_t flags.
  Uint32  flags2;        ///< mobjflag2_t flags.

  int seesound;          ///< Played when thing sees an enemy.
  int attacksound;       ///< Played when attacking.
  int painsound;         ///< Played when hurt.
  int deathsound;        ///< Played when thing dies.
  int activesound;       ///< Occasionally played when active.

  state_t *spawnstate;   ///< Initial state.
  state_t *seestate;     ///< Used when thing sees an enemy. Active.
  state_t *meleestate;   ///< Used when thing executes a melee attack.
  state_t *missilestate; ///< Same, but for missile attacks.
  state_t *painstate;    ///< Used when thing is hurt.
  state_t *deathstate;   ///< Used when thing is killed.
  state_t *xdeathstate;  ///< Used when thing is killed in a messy way, e.g. exploded.
  state_t *crashstate;   ///< Used when a flying thing has died and crashed on the ground.
  state_t *raisestate;   ///< Used when thing is being raised from dead.

  touchfunc_t touchf;    ///< If MF_TOUCHFUNC is set, this function is to be called when touching another Actor.

public:
  /// constructors
  ActorInfo(const string& n);
  ActorInfo(const mobjinfo_t& m);

  inline const char *GetName() const { return classname; }
  inline mobjtype_t  GetMobjType() const { return mobjtype; }
  void SetName(const char *n);
};


/// \brief Stores all known DActor type definitions.
/// \ingroup g_thing
/*!
  The stored ActorInfo records are stored in three maps according to three different keys:
  - DECORATE class name (primary mapping)
  - mobjtype_t number
  - DoomEd number (not every class has one)
 */
class ActorInfoDictionary : public HashDictionary<ActorInfo>
{
protected:
  typedef HashDictionary<ActorInfo> parent;

  typedef map<mobjtype_t, ActorInfo*> mt_map_t;
  typedef mt_map_t::iterator mt_iter_t;
  mt_map_t mt_map;

  typedef map<int, ActorInfo*> doomed_map_t;
  typedef doomed_map_t::iterator doomed_iter_t;
  doomed_map_t doomed_map;

public:
  /// The safe way of inserting stuff into the dictionary.
  /// The main point is that 'classname' is stored within the ActorInfo structure itself.
  inline bool Insert(ActorInfo *p)
  {
    if (parent::Insert(p))
      {
	// do not replace possible original in mt_map (should not make any difference)
	mt_map.insert(mt_map_t::value_type(p->GetMobjType(), p));
	return true;
      }
    else
      return false;
  }


  /// Find the ActorInfo corresponding to mobjtype mt.
  ActorInfo *operator[](mobjtype_t mt)
  {
    mt_iter_t t = mt_map.find(mt);
    if (t == mt_map.end())
      return NULL;
    else
      return t->second;
  }


  inline bool InsertDoomEd(ActorInfo *p, bool replace)
  {
    if (p->doomednum >= 0)
      {
	if (replace)
	  doomed_map.erase(p->doomednum);

	return doomed_map.insert(doomed_map_t::value_type(p->doomednum, p)).second;
      }
    return false;
  }


  /// Find the ActorInfo corresponding to doomednum n
  ActorInfo *FindDoomEdNum(int n)
  {
    doomed_iter_t t = doomed_map.find(n);
    if (t == doomed_map.end())
      return NULL;
    else
      return t->second;
  }


  int  Clear();
};


extern ActorInfoDictionary aid;
