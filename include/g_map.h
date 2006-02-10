// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2005 by DooM Legacy Team.
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
// Revision 1.48  2006/02/10 18:01:08  smite-meister
// glnodes fixed
//
// Revision 1.47  2006/02/08 19:09:27  jussip
// Added beginnings of a new OpenGL renderer.
//
// Revision 1.46  2006/01/04 23:15:08  jussip
// Read and convert GL nodes if they exist.
//
// Revision 1.45  2005/12/16 18:18:35  smite-meister
// Deus Vult BLOCKMAP fix
//
// Revision 1.44  2005/09/29 15:35:27  smite-meister
// JDS texture standard
//
// Revision 1.43  2005/09/11 16:23:25  smite-meister
// template classes
//
// Revision 1.42  2005/07/11 16:58:56  smite-meister
// msecnode_t bug fixed
//
// Revision 1.41  2005/06/28 17:05:00  smite-meister
// item respawning cleaned up
//
// Revision 1.40  2005/06/05 19:32:26  smite-meister
// unsigned map structures
//
// Revision 1.39  2004/11/18 20:30:14  smite-meister
// tnt, plutonia
//
// Revision 1.38  2004/10/17 02:01:39  smite-meister
// bots!
//
// Revision 1.37  2004/10/14 19:35:50  smite-meister
// automap, bbox_t
//
// Revision 1.36  2004/10/11 11:13:42  smite-meister
// map utils
//
// Revision 1.35  2004/09/06 19:58:03  smite-meister
// Doom linedefs done!
//
// Revision 1.30  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.26  2003/12/23 18:06:06  smite-meister
// Hexen stairbuilders. Moving geometry done!
//
// Revision 1.20  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
// Revision 1.19  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.17  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.16  2003/05/30 13:34:48  smite-meister
// Cleanup, HUD improved, serialization
//
// Revision 1.14  2003/05/05 00:24:50  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.13  2003/04/20 16:45:50  smite-meister
// partial SNDSEQ fix
//
// Revision 1.12  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.11  2003/04/14 08:58:30  smite-meister
// Hexen maps load.
//
// Revision 1.9  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.8  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.7  2003/03/15 20:07:20  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.6  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.5  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Map class definition.

#ifndef g_map_h
#define g_map_h 1

#include <vector>
#include <deque>
#include <string>
#include <list>
#include <map>

#include "doomdef.h"
#include "r_defs.h"
#include "g_think.h"
#include "vect.h"
#include "m_fixed.h"
#include "m_bbox.h"
#include "info.h" // mobjtype_t

using namespace std;

typedef bool (*traverser_t)(struct intercept_t *in);
typedef bool (*line_iterator_t)(struct line_t *l);
typedef bool (*thing_iterator_t)(class Actor *a);
typedef bool (*thinker_iterator_t)(Thinker *t);

/// \brief A single game map and all the stuff it contains.
///
/// \nosubgrouping
/// This class stores all gameplay-related information about one map,
/// including geometry, BSP, blockmap, reject, mapthings, Thinker list,
/// player list, ambient sounds, physics...

class Map
{
  friend class GameInfo;
public:
  /// \name Essentials
  //@{
  class MapInfo *info; ///< the controlling MapInfo struct for this map

  string lumpname;   ///< map lumpname
  int    lumpnum;    ///< lumpnum of the separator beginning the map

  tic_t starttic, maptic;      ///< number of tics the map has played
  int   kills, items, secrets; ///< map totals

  bool hexen_format; ///< is this map stored in the Hexen format?
  //@}

  /// \name Geometry
  //@{
  int                 numvertexes;
  struct vertex_t    *vertexes;
  bbox_t              root_bbox; ///< bounding box for all the vertices in the map

  int                 numlines;
  struct line_t      *lines;

  int                 numsides;
  struct side_t      *sides;

  int                 numsectors;
  struct sector_t    *sectors;
  line_t            **linebuffer; ///< combining sectors and lines

  int                 numsubsectors;
  struct subsector_t *subsectors;

  int                 numnodes;
  struct node_t      *nodes;

  int                 numsegs;
  struct seg_t       *segs;

  /// additional vertices from GL-nodes
  int                 numglvertexes;
  vertex_t           *glvertexes;

  int                 NumPolyobjs;
  struct polyobj_t   *polyobjs;

  //@}

  /// \name Rendering
  /// Map-specific stuff only the renderer uses.
  //@{
  class fadetable_t *fadetable; ///< colormaps the software renderer uses to simulate light levels
  bool R_SetFadetable(const char *name);

  class Texture *skytexture; ///< current sky texture
  //@}

  /// \name Blockmap
  /// Created from axis aligned bounding box of the map, a rectangular array of
  /// blocks of size 128 map units square. See blockmapheader_t.
  /// Used to speed up collision detection agains lines and things by a spatial subdivision in 2D.
  //@{
#define MAPBLOCKUNITS   128
#define MAPBLOCKBITS    7
  //#define MAPBLOCKSIZE    (MAPBLOCKUNITS*FRACUNIT)
  //#define MAPBLOCKSHIFT   (FRACBITS+7)
  //#define MAPBMASK        (MAPBLOCKSIZE-1)
  //#define MAPBTOFRAC      (MAPBLOCKSHIFT-FRACBITS)
  // MAXRADIUS is for precalculated sector block boxes
  // the spider demon is larger, but we do not have any moving sectors nearby
#define MAXRADIUS       32
#define MAPBLOCK_END    0xFFFF ///< terminator for a blocklist

  // I'm too lazy to move these into the struct...
  fixed_t bmaporgx, bmaporgy;    ///< origin (lower left corner) of block map in map coordinates
  int     bmapwidth, bmapheight; ///< size of the blockmap in mapblocks

  struct blockmap_t
  {
    Uint16 **index;  ///< width*height array of pointers to the blocklists
    Uint16  *lists;  ///< packed array of -1 terminated blocklists
  } bmap;

  class  Actor       **blocklinks;   ///< width*height array of thing chains
  struct polyblock_t **PolyBlockMap; ///< width*height array of polyblock chains
  //@}


  /// \name Reject
  /// For fast sight rejection.
  /// Speeds up enemy AI by skipping detailed LineOf Sight calculation.
  /// Without the "Reject special effects" hacks in some PWADs, this could be used as a PVS lookup as well.
  //@{
  byte *rejectmatrix;
  //@}


  /// \name Scripting
  //@{
  // FS
  struct script_t        *levelscript;    ///< the mother of all FS scripts in this map
  struct runningscript_t *runningscripts; ///< linked list of currently active FS scripts

  // ACS
#define MAX_ACS_MAP_VARS 32
  byte   *ActionCodeBase;    ///< the raw BEHAVIOR lump, base for offsets
  int     ACScriptCount;     ///< number of different AC scripts in this map
  struct acsInfo_t *ACSInfo; ///< properties of the scripts
  int     ACStringCount;     ///< number of ACS strings in this map
  char  **ACStrings;         ///< array of the ACS strings
  int     ACMapVars[MAX_ACS_MAP_VARS]; ///< ACS map variables
  //@}


  /// \name Mapthings and Thinkers
  //@{
  int                nummapthings;
  struct mapthing_t *mapthings;

  Thinker thinkercap; ///< Linked list of Thinkers in the map. The head and tail of the thinker list.

  bool        force_pointercheck; ///< force a Thinker pointer cleanup
  vector<Thinker *> DeletionList; ///< Thinkers to be deleted are stored here

  deque<mapthing_t *> itemrespawnqueue; ///< for respawning items
  deque<tic_t>        itemrespawntime;  ///< this could be combined to the previous one, but...

  static const unsigned BODYQUESIZE = 32;
  deque<Actor *> bodyqueue; ///< queue for player corpses

  multimap<short, Actor *> TIDmap; ///< Thing ID, a system for grouping things.

  /// Currently active dynamic geometry elements, based on BOOM code.
  list<class ceiling_t *> activeceilings;
  list<class plat_t *>    activeplats;
  //@}


  /// \name Players
  //@{
  vector<class PlayerInfo *> players; ///< players currently in the map
  deque<PlayerInfo *>   respawnqueue; ///< players queuing to be respawned

  multimap<int, mapthing_t *> playerstarts; ///< Mapping from player number to playerstart. args[0] holds the entrypoint number.
  static const unsigned MAX_DM_STARTS = 32;
  vector<mapthing_t *> dmstarts; ///< DeathMatch starts
  //@}


  /// \name Sound sequences
  //@{
  list<class ActiveSndSeq*> ActiveSeqs;
  vector<int>   AmbientSeqs;
  ActiveSndSeq *ActiveAmbientSeq;
  //@}


  //------------ Misc. ambiance -------------

  int NextLightningFlash; // tics until next flash
  int Flashcount;         // ongoing flash?
  int *LightningLightLevels; // storage for original light levels


  //-----------------------------------
  class BotNodes *botnodes; // TEST

  //------------------------------------
  // TODO: from this line on it's badly designed stuff to be fixed someday
  vector<mapthing_t *> braintargets; // DoomII demonbrain spawnbox targets
  int braintargeton;

  vector<mapthing_t *> BossSpots;
  vector<mapthing_t *> MaceSpots;

public:
  // constructor and destructor
  Map(MapInfo *i);
  ~Map(); // also destroys all objects that belong to this map

  // not yet implemented
  void Reset(); // resets the map to starting position. Lighter than Setup().

  int  Serialize(class LArchive &a);
  int  Unserialize(LArchive &a);

  // in p_tick.cpp
  void Ticker();

  void InitThinkers();
  void AddThinker(Thinker *thinker);
  void DetachThinker(Thinker *thinker);
  void RemoveThinker(Thinker *thinker);
  void RunThinkers();

  // in g_map.cpp
  void AddPlayer(PlayerInfo *p); // adds a new player to the map (and respawnqueue)
  bool RemovePlayer(PlayerInfo *p);
  void RebornPlayer(PlayerInfo *p); // adds a player to respawnqueue, gets rid of corpse
  PlayerInfo *FindPlayer(int number); // returns player 'number' if he is in the map, otherwise NULL
  int  RespawnPlayers();
  int  HandlePlayers();

  bool DeathMatchRespawn(PlayerInfo *p);
  bool CoopRespawn(PlayerInfo *p);
  bool CheckRespawnSpot(PlayerInfo *p, mapthing_t *mthing);
  void QueueBody(Actor *p);

  void BossDeath(const class DActor *mo);
  int  Massacre();
  void ExitMap(Actor *activator, int exit, int entrypoint = 0);

  void RespawnSpecials();
  void RespawnWeapons();

  void SpawnActor(Actor *p);
  DActor *SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t);
  inline DActor *SpawnDActor(const vec_t<fixed_t>& r, mobjtype_t t) { return SpawnDActor(r.x, r.y, r.z, t); }
  void SpawnPlayer(PlayerInfo *pi, mapthing_t *mthing);
  DActor *SpawnMapThing(mapthing_t *mthing, bool initial = true);
  DActor *SpawnSplash(const vec_t<fixed_t>& pos, fixed_t z, int sound, mobjtype_t base,
		      mobjtype_t chunk = MT_NONE, bool randtics = true); 
  DActor *SpawnBlood(const vec_t<fixed_t>& r, int damage);
  void SpawnBloodSplats(const vec_t<fixed_t>& r, int damage, fixed_t px, fixed_t py);
  void SpawnPuff(fixed_t nx, fixed_t ny, fixed_t nz);
  inline void SpawnPuff(const vec_t<fixed_t>& r) { SpawnPuff(r.x, r.y, r.z); }
  void SpawnSmoke(fixed_t x, fixed_t y, fixed_t z);

  void InsertIntoTIDmap(Actor *p, int tid);
  void RemoveFromTIDmap(Actor *p);
  Actor *FindFromTIDmap(int tid, int *pos);

  // in p_map.cpp
  bool RadiusLinesCheck(fixed_t x, fixed_t y, fixed_t radius, line_iterator_t func);
  bool CheckSector(sector_t* sector, int crunch);
  bool ChangeSector(sector_t *sector, int crunch);
  void SlideMove(Actor* mo);

  // in p_setup.cpp
  bool Setup(tic_t start, bool spawnthings = true);
  void LoadVertexes(int lump);
  void LoadSegs(int lump);
  void LoadSubsectors(int lump);
  void LoadSectors1(int lump);
  void LoadSectors2(int lump);
  void LoadNodes(int lump);
  void LoadThings(int lump);
  void LoadLineDefs(int lump);
  void LoadLineDefs2();
  void ConvertLineDefs();
  void LoadSideDefs(int lump);
  void LoadSideDefs2(int lump);
  void LoadBlockMap(int lump);
  void LoadACScripts(int lump);
  void GroupLines();
  void SetupSky();

  int LoadGLVertexes(const int lump);
  void LoadGLSegs(const int lump, const int glversion);
  void LoadGLSubsectors(const int lump, const int glversion);
  void LoadGLNodes(const int lump, const int glversion);

  // in p_sight.cpp
  bool CrossSubsector(int num);
  bool CrossBSPNode(int bspnum);
  bool CheckSight(Actor *t1, Actor *t2);
  bool CheckSight2(Actor *t1, Actor *t2, fixed_t nx, fixed_t ny, fixed_t nz);

  // in p_maputl.cpp
  bool BlockLinesIterator(int x, int y, line_iterator_t func);
  bool BlockThingsIterator(int x, int y, thing_iterator_t func);
  bool IterateThinkers(thinker_iterator_t func);
  bool PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int flags, traverser_t trav);
  Actor *RoughBlockSearch(Actor *center, Actor *master, int distance, int flags);
  Actor *RoughBlockCheck(Actor *center, Actor *master, int index, int flags);
  void CreateSecNodeList(Actor *thing, fixed_t x, fixed_t y);

  // in p_hpspr.cpp
  void PlaceWeapons();
  void RepositionMace(DActor *mo);

  // in p_telept.cpp
  bool EV_Teleport(int tag, line_t *line, Actor *thing, int type, int flags);
  bool EV_SilentLineTeleport(int tag, line_t *line, Actor *thing, bool reverse);

  // in p_spec.cpp
  side_t   *getSide(int sec, int line, int side);
  sector_t *getSector(int sec, int line, int side);
  bool      twoSided(int sec, int line);
  fixed_t   FindShortestUpperAround(sector_t *sec);
  fixed_t   FindShortestLowerAround(sector_t *sec);

  sector_t *FindModelFloorSector(fixed_t floordestheight, sector_t *sec);
  sector_t *FindModelCeilingSector(fixed_t ceildestheight, sector_t *sec);
  int  FindSectorFromLineTag(line_t *line, int start);
  int  FindSectorFromTag(int tag, int start);
  line_t *FindLineFromTag(int tag, int *start);
  void InitTagLists();

  void AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags);

  bool ActivateLine(line_t *line, Actor *thing, int side, int activationType);
  bool ExecuteLineSpecial(unsigned special, byte *args, line_t *line, int side, Actor *mo);

  void UpdateSpecials();

  int  SpawnSectorSpecial(int sp, sector_t *sec);
  void SpawnLineSpecials();
  void SpawnScroller(line_t *l, int tag, int type, int control);
  void SpawnFriction(line_t *l, int tag);
  void SpawnPusher(line_t *l, int tag, int type);

  // some event functions that fit nowhere else
  int  EV_SectorSoundChange(int tag, int seq);

  // in a_action.cpp
  bool EV_LocalQuake(byte *args);

  // in p_switch.cpp
  void ChangeSwitchTexture(line_t *line, int useAgain);

  // in p_genlin.cpp
  int EV_DoGenFloor(line_t* line);
  int EV_DoGenCeiling(line_t* line);
  int EV_DoGenLift(line_t* line);
  int EV_DoGenStairs(line_t* line);
  int EV_DoGenCrusher(line_t* line);
  int EV_DoGenDoor(line_t* line);
  int EV_DoGenLockedDoor(line_t* line);

  // in p_lights.cpp
  void SpawnPhasedLightSequence(sector_t *sec, int indexStep);
  void SpawnStrobeLight(sector_t *sec, short brighttime, short darktime, bool inSync);
  int  EV_StartLightStrobing(int tag, short brighttime, short darktime);
  int  EV_FadeLight(int tag, int destvalue, int speed);
  int  EV_SpawnLight(int tag, int type, short maxl, short minl = 0, short maxt = 0, short mint = 0);
  int  EV_TurnTagLightsOff(int tag);
  int  EV_LightTurnOn(int tag, int bright);

  // in p_plats.cpp
  void AddActivePlat(plat_t* plat);
  int  ActivateInStasisPlat(int tag);
  void RemoveActivePlat(plat_t* plat);
  void RemoveAllActivePlats();
  int  EV_DoPlat(int tag, line_t *line, int type, fixed_t speed, int wait, fixed_t height);
  int  EV_StopPlat(int tag);

  // in p_ceiling.cpp
  void AddActiveCeiling(ceiling_t* ceiling);
  int  ActivateInStasisCeiling(int tag);
  void RemoveActiveCeiling(ceiling_t* ceiling);
  void RemoveAllActiveCeilings();
  int  EV_DoCeiling(int tag, line_t *line, int type, fixed_t speed, int crush, fixed_t height);
  int  EV_DoCrusher(int tag, int type, fixed_t upspeed, fixed_t downspeed, int crush, fixed_t height);
  int  EV_StopCeiling(int tag);

  // in p_doors.cpp
  void EV_OpenDoor(int sectag, int speed, int wait_time);
  void EV_CloseDoor(int sectag, int speed);
  int  EV_DoDoor(int tag, line_t* line, Actor *mo, byte type, fixed_t speed, int delay);
  void SpawnDoorCloseIn30(sector_t* sec);
  void SpawnDoorRaiseIn5Mins(sector_t* sec);

  // in p_floor.cpp
  int EV_DoFloor(int tag, line_t *line, int type, fixed_t speed, int crush, fixed_t height);
  int EV_DoChange(line_t *line, int changetype);
  int EV_BuildStairs(int tag, int type, fixed_t speed, fixed_t stepsize, int crush);
  int EV_BuildHexenStairs(int tag, int type, fixed_t speed, fixed_t stepdelta, int resetdelay, int stepdelay = 0);
  int EV_DoDonut(int tag, fixed_t pspeed, fixed_t sspeed);
  int EV_DoElevator(int tag, int type, fixed_t speed, fixed_t height_f, fixed_t height_c = 0, int crush = 0);
  int EV_DoFloorWaggle(int tag, fixed_t amp, angle_t freq, angle_t offset, int wait);
  int T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, int crush, int floorOrCeiling);

  // in s_sndseq.cpp
  bool SN_StartSequence(struct mappoint_t *m, int seq);
  bool SN_StartSequence(Actor *a, int seq);
  bool SN_StartSequenceName(mappoint_t *m, const char *name);
  bool SN_StopSequence(void *origin, bool quiet = false);
  void UpdateSoundSequences();

  // elsewhere, usually former R_* functions
  void PrecacheMap();
  subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
  subsector_t* R_IsPointInSubsector(fixed_t x, fixed_t y);

  // in p_poly.cpp
  void InitPolyobjs();
  void IterFindPolySegs(fixed_t x, fixed_t y, seg_t **segList);
  void SpawnPolyobj(int index, int tag, bool crush);
  void TranslateToStartSpot(int tag, fixed_t originX, fixed_t originY);
  void InitPolyBlockMap();

  polyobj_t *GetPolyobj(int num);
  int  GetPolyobjMirror(int num);

  void LinkPolyobj(polyobj_t *po);
  void UnLinkPolyobj(polyobj_t *po);
  bool PO_CheckBlockingActors(seg_t *seg, polyobj_t *po);
  bool PO_MovePolyobj(int num, fixed_t x, fixed_t y);
  bool PO_RotatePolyobj(int num, angle_t angle);
  bool PO_Busy(int num);

  bool EV_RotatePoly(byte *args, int direction, bool overRide);
  bool EV_MovePoly(byte *args, bool timesEight, bool overRide);
  bool EV_OpenPolyDoor(byte *args, int type);

  // in p_things.cpp
  bool EV_ThingProjectile(byte *args, bool gravity);
  bool EV_ThingSpawn(byte *args, bool fog);
  bool EV_ThingActivate(int tid);
  bool EV_ThingDeactivate(int tid);
  bool EV_ThingRemove(int tid);
  bool EV_ThingDestroy(int tid);

  // p_anim.cpp
  void InitLightning();
  void ForceLightning();
  void LightningFlash();

  // ACS scripting
  void CheckACSStore();
  void StartOpenACS(int number, int infoIndex, int *address);
  bool StartACS(int number, byte *args, Actor *activator, line_t *line, int side);
  bool StartLockedACS(line_t *line, byte *args, class PlayerPawn *p, int side);
  bool TerminateACS(int number);
  bool SuspendACS(int number);

  int GetACSIndex(int number);

  void TagFinished(int tag);
  bool TagBusy(int tag);
  void PolyobjFinished(int po);
  void ScriptFinished(int number);

  // FS scripting
  void FS_ClearScripts();
  void FS_ClearRunningScripts();
  void FS_PreprocessScripts();
  bool FS_RunScript(int n, Actor *trig);
  void FS_DelayedScripts();
  void FS_AddRunningScript(runningscript_t *s);
protected:
  bool FS_wait_finished(runningscript_t *script);

public:
  void R_AddWallSplat(line_t *line, int side, char *name, fixed_t top, fixed_t wallfrac, int flags);
};



#endif
