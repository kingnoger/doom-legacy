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
// $Log$
// Revision 1.13  2003/04/20 16:45:50  smite-meister
// partial SNDSEQ fix
//
// Revision 1.12  2003/04/19 17:38:47  smite-meister
// SNDSEQ support, tools, linedef system...
//
// Revision 1.11  2003/04/14 08:58:30  smite-meister
// Hexen maps load.
//
// Revision 1.10  2003/04/08 09:46:06  smite-meister
// Bugfixes
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
//
// DESCRIPTION:
//    Map class. Describes one game map and all that's in it.
//
//-----------------------------------------------------------------------------


#ifndef g_map_h
#define g_map_h 1

#include <vector>
#include <deque>
#include <string>
#include <list>
#include <map>

#include "doomdef.h"
#include "doomtype.h"
#include "g_think.h"
#include "m_fixed.h"
#include "info.h" // mobjtype_t
#include "p_spec.h"

using namespace std;

class Actor;
class DActor;
class Pawn;
class PlayerPawn;
class PlayerInfo;

struct intercept_t; 
typedef bool (*traverser_t) (intercept_t *in);


// new class for maps

class Map
{
  friend class GameInfo;
public:
  class LevelNode *level;   // the level in which this Map belongs
  string filename, mapname; // name of the map file, map lump
  int lumpnum;              // lumpnum of the separator beginning the map

  class MapInfo *info;

  tic_t starttic, maptic;   // number of tics the map has played
  int   kills, items, secrets;  // map totals

  bool hexen_format;

  //------------ Geometry ------------

  int              numvertexes;
  struct vertex_t* vertexes;

  int              numsegs;
  struct seg_t*    segs;

  int              numsectors;
  struct sector_t *sectors;

  int              numsubsectors;
  struct subsector_t *subsectors;

  int             numnodes;
  struct node_t*  nodes;

  int             numlines;
  struct line_t*  lines;

  int             numsides;
  struct side_t*  sides;

  int               NumPolyobjs;
  struct polyobj_t* polyobjs;
  struct polyblock_t **PolyBlockMap;

  //------------ BLOCKMAP ------------
  // Created from axis aligned bounding box of the map, a rectangular array of
  // blocks of size ...
  // Used to speed up collision detection by spatial subdivision in 2D.

  // mapblocks are used to check movement against lines and things
#define MAPBLOCKUNITS   128
#define MAPBLOCKSIZE    (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT   (FRACBITS+7)
#define MAPBMASK        (MAPBLOCKSIZE-1)
#define MAPBTOFRAC      (MAPBLOCKSHIFT-FRACBITS)
  // MAXRADIUS is for precalculated sector block boxes
  // the spider demon is larger, but we do not have any moving sectors nearby
#define MAXRADIUS       (32*FRACUNIT)

  // Blockmap size.
  int    bmapwidth, bmapheight;  // size in mapblocks
  short *blockmap;       // int for large maps?
  short *blockmaplump;   // offsets in blockmap are from here

  // origin of block map
  fixed_t bmaporgx, bmaporgy;

  // for thing chains
  class Actor **blocklinks;

  //------------ REJECT ------------
  // For fast sight rejection.
  // Speeds up enemy AI by skipping detailed
  //  LineOf Sight calculation.
  // Without special effect, this could be
  //  used as a PVS lookup as well.
  //
  byte *rejectmatrix;

  //------------ Scripting ------------

  // the mother of all FS scripts (in the map)
  struct script_t *levelscript;

  // currently active FS scripts
  struct runningscript_t *runningscripts; // linked list

  // ACS script data
  int   ACScriptCount;
  byte *ActionCodeBase;
  struct acsInfo_t *ACSInfo;
  int ACStringCount;
  char **ACStrings;

  //------------ Mapthings and Thinkers ------------

  int                nummapthings;
  struct mapthing_t *mapthings;
  
  // Thinkers in the map
  // Both the head and tail of the thinker list.
  Thinker thinkercap;

  deque<mapthing_t *> itemrespawnqueue;
  deque<tic_t>        itemrespawntime; // this could be combined to the previous, but...

#define BODYQUESIZE 32
  deque<Actor *> bodyqueue;

  multimap<short, Actor *> TIDmap; // Thing ID, for grouping things

  //------------ Specialized Thinkers ------------

  // currently active dynamic geometry elements, based on BOOM code
  list<class ceiling_t *> activeceilings;
  list<class plat_t *>    activeplats;

  //------------ Players ------------

  vector<PlayerInfo *> players;
  deque<PlayerInfo *>  respawnqueue;  // for players queuing to be respawned

  vector<mapthing_t *> playerstarts;
#define MAX_DM_STARTS 32
  vector<mapthing_t *> dmstarts;

  //------------ Sound sequences ------------

  map<unsigned, struct sndseq_t*> SoundSeqs;
  list<class ActiveSndSeq*> ActiveSeqs;
  vector<sndseq_t*>         AmbientSeqs;
  ActiveSndSeq             *ActiveAmbientSeq;

  //------------------------------------
  // TODO: from this line on it's badly designed stuff to be fixed someday
  int BossDeathKey;  // bit flags to see which bosses end the map when killed.

  vector<mapthing_t *> braintargets; // DoomII demonbrain spawnbox targets
  int braintargeton;

  vector<mapthing_t *> BossSpots;
  vector<mapthing_t *> MaceSpots;

public:
  // constructor and destructor
  Map(const string & mname); //(const char *mname)
  ~Map(); // also destroys all objects that belong to this map

  // not yet implemented
  void Reset(); // resets the map to starting position. Lighter than Setup().

  // in p_tick.cpp
  void Ticker();

  void InitThinkers();
  void AddThinker(Thinker *thinker);
  void DetachThinker(Thinker *thinker);
  void RemoveThinker(Thinker *thinker);
  void RunThinkers();

  // in g_map.cpp
  void AddPlayer(PlayerInfo *p); // adds a new player to the map (and respawnqueue)
  void RebornPlayer(PlayerInfo *p); // adds a player to respawnqueue, gets rid of corpse
  int  RespawnPlayers();
  bool DeathMatchRespawn(PlayerInfo *p);
  bool CoopRespawn(PlayerInfo *p);
  bool CheckRespawnSpot(PlayerInfo *p, mapthing_t *mthing);

  void BossDeath(const DActor *mo);
  int  Massacre();
  void ExitMap(int exit);

  void RespawnSpecials();
  void RespawnWeapons();

  void SpawnActor(Actor *p);
  void DetachActor(Actor *p);
  DActor *SpawnDActor(fixed_t nx, fixed_t ny, fixed_t nz, mobjtype_t t);
  void SpawnPlayer(PlayerInfo *pi, mapthing_t *mthing);
  void SpawnMapThing(mapthing_t *mthing);
  void SpawnSplash(Actor *mo, fixed_t z);
  DActor *SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage);
  void SpawnBloodSplats(fixed_t x, fixed_t y, fixed_t z, int damage, fixed_t px, fixed_t py);
  void SpawnPuff(fixed_t nx, fixed_t ny, fixed_t nz);
  void SpawnSmoke(fixed_t x, fixed_t y, fixed_t z);

  void InsertIntoTIDmap(Actor *p, int tid);
  void RemoveFromTIDmap(Actor *p);
  Actor *FindFromTIDmap(int tid, int *pos);

  // in p_map.cpp
  void CreateSecNodeList(Actor *thing, fixed_t x, fixed_t y);
  bool CheckSector(sector_t* sector, bool crunch);
  bool ChangeSector(sector_t *sector, bool crunch);
  void SlideMove(Actor* mo);

  // in p_setup.cpp
  bool Setup(tic_t start);
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


  // in p_sight.cpp
  bool CrossSubsector(int num);
  bool CrossBSPNode(int bspnum);
  bool CheckSight(Actor *t1, Actor *t2);

  // in p_maputl.cpp
  bool BlockLinesIterator(int x, int y, bool (*func)(line_t*));
  bool BlockThingsIterator(int x, int y, bool(*func)(Actor*));
  bool PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int flags, traverser_t trav);

  // in p_hpspr.cpp
  void PlaceWeapons();
  void RepositionMace(DActor *mo);

  // in p_telept.cpp
  bool EV_Teleport(line_t *line, int side, Actor *thing);
  bool EV_SilentTeleport(line_t *line, int side, Actor *thing);
  int  EV_SilentLineTeleport(line_t *line, int side, Actor *thing, bool reverse);

  // in p_spec.cpp
  side_t   *getSide(int sec, int line, int side);
  sector_t *getSector(int sec, int line, int side);
  int       twoSided(int sec, int line);
  fixed_t   FindShortestUpperAround(int secnum);
  fixed_t   FindShortestTextureAround(int secnum);

  sector_t *FindModelFloorSector(fixed_t floordestheight, sector_t *sec);
  sector_t *FindModelCeilingSector(fixed_t ceildestheight, sector_t *sec);
  int  FindSectorFromLineTag(line_t *line, int start);
  int  FindSectorFromTag(int tag, int start);
  line_t *FindLineFromTag(int tag, int *start);
  void InitTagLists();

  void AddFakeFloor(sector_t* sec, sector_t* sec2, line_t* master, int flags);

  bool ActivateLine(line_t *line, Actor *thing, int side, int activationType);
  bool ExecuteLineSpecial(unsigned special, byte *args, line_t *line, int side, Actor *mo);

  void ActivateCrossedLine(line_t *line, int side, Actor *thing);
  void ShootSpecialLine(Actor *thing, line_t *line);
  void UpdateSpecials();

  int  SpawnSectorSpecial(int sp, sector_t *sec);
  void SpawnSpecials();
  void SpawnScrollers();
  void SpawnFriction();
  void SpawnPushers();
  DActor *GetPushThing(int s);

  // in p_switch.cpp
  void ChangeSwitchTexture(line_t *line, int useAgain);
  bool UseSpecialLine(Actor *thing, line_t *line, int side);

  // in p_genlin.cpp
  int EV_DoGenFloor(line_t* line);
  int EV_DoGenCeiling(line_t* line);
  int EV_DoGenLift(line_t* line);
  int EV_DoGenStairs(line_t* line);
  int EV_DoGenCrusher(line_t* line);
  int EV_DoGenDoor(line_t* line);
  int EV_DoGenLockedDoor(line_t* line);

  // in p_lights.cpp
  void SpawnFireFlicker(sector_t *sector);  
  void SpawnLightFlash(sector_t *sector);
  void SpawnStrobeFlash(sector_t *sector, int fastOrSlow, int inSync);
  void SpawnGlowingLight(sector_t *sector);
  void FadeLight(int tag, int destvalue, int speed);
  int EV_StartLightStrobing(line_t *line);
  int EV_TurnTagLightsOff(line_t* line);
  int EV_LightTurnOn(line_t *line, int bright);

  // in p_plats.cpp
  void AddActivePlat(plat_t* plat);
  void ActivateInStasisPlat(int tag);
  void RemoveActivePlat(plat_t* plat);
  void RemoveAllActivePlats();
  int EV_DoPlat(line_t *line, plattype_e type, int amount);
  int EV_StopPlat(line_t* line);

  // in p_ceiling.cpp
  void AddActiveCeiling(ceiling_t* ceiling);
  int  ActivateInStasisCeiling(line_t *line);
  void RemoveActiveCeiling(ceiling_t* ceiling);
  void RemoveAllActiveCeilings();
  int  EV_DoCeiling(line_t *line, ceiling_e type);
  int  EV_CeilingCrushStop(line_t *line);

  // in p_doors.cpp
  void EV_OpenDoor(int sectag, int speed, int wait_time);
  void EV_CloseDoor(int sectag, int speed);
  //int  EV_DoDoor(line_t *line, byte *args, vldoor_e type);
  int  EV_DoDoor(line_t* line, byte type, fixed_t speed = VDOORSPEED, int delay = VDOORWAIT);
  int  EV_DoLockedDoor(line_t* line, PlayerPawn *p, byte type, byte lock,
		       fixed_t speed = VDOORSPEED, int delay = VDOORWAIT);
  int  EV_VerticalDoor(line_t* line, Actor *p);
  void SpawnDoorCloseIn30 (sector_t* sec);
  void SpawnDoorRaiseIn5Mins(sector_t* sec);
  
  // in p_floor.cpp
  int EV_DoFloor(line_t *line, floor_e floortype);
  int EV_DoChange(line_t *line, change_e changetype);
  int EV_BuildStairs(line_t *line, stair_e type);
  int EV_DoDonut(line_t *line);
  int EV_DoElevator(line_t* line, elevator_e elevtype);
  result_e T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest,
		       bool crush, int floorOrCeiling, int direction);

  // in s_sndseq.cpp
  void S_Read_SNDSEQ(int lump);
  bool SN_StartSequence(mappoint_t *m, unsigned seq);
  bool SN_StartSequence(Actor *a, unsigned seq);
  bool SN_StartSequenceName(Actor *m, const char *name);
  bool SN_StopSequence(void *origin);
  void UpdateSoundSequences();

  // elsewhere, usually former R_* functions
  void PrecacheMap();
  subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
  subsector_t* R_IsPointInSubsector(fixed_t x, fixed_t y);
  void HWR_SearchLightsInMobjs();

  // in p_poly.cpp
  void InitPolyobjs();
  void IterFindPolySegs(int x, int y, seg_t **segList);
  void SpawnPolyobj(int index, int tag, bool crush);
  void TranslateToStartSpot(int tag, int originX, int originY);
  void InitPolyBlockMap();

  polyobj_t *GetPolyobj(int num);
  int  GetPolyobjMirror(int num);

  void LinkPolyobj(polyobj_t *po);
  void UnLinkPolyobj(polyobj_t *po);
  bool PO_CheckBlockingActors(seg_t *seg, polyobj_t *po);
  bool PO_MovePolyobj(int num, int x, int y);
  bool PO_RotatePolyobj(int num, angle_t angle);
  bool PO_Busy(int num);

  bool EV_RotatePoly(line_t *line, byte *args, int direction, bool overRide);
  bool EV_MovePoly(line_t *line, byte *args, bool timesEight, bool overRide);
  bool EV_OpenPolyDoor(line_t *line, byte *args, podoortype_e type);

  // ACS scripting
  void StartOpenACS(int number, int infoIndex, int *address);
  bool StartACS(int number, byte *args, Actor *activator, line_t *line, int side);
  bool StartLockedACS(line_t *line, byte *args, PlayerPawn *p, int side);
  bool TerminateACS(int number);
  bool SuspendACS(int number);

  int GetACSIndex(int number);

  void TagFinished(int tag);
  bool TagBusy(int tag);
  void PolyobjFinished(int po);
  void ScriptFinished(int number);

  // FS scripting
  void T_ClearScripts();
  void T_PreprocessScripts();
  void T_RunScript(int n);
  void T_DelayedScripts();
  void T_AddRunningScript(runningscript_t *s);
  runningscript_t *T_SaveCurrentScript();
protected:
  bool T_wait_finished(runningscript_t *script);
};



#endif
