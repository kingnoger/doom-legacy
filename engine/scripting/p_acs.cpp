// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1996 by Raven Software, Corp.
// Copyright (C) 2003 by DooM Legacy Team.
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
// Revision 1.2  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.1  2003/03/15 20:07:20  smite-meister
// Initial Hexen compatibility!
//
//
//
// DESCRIPTION:
//   AC Script interpreter
//
//-----------------------------------------------------------------------------

#include "doomdata.h"

#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"
#include "g_player.h"
#include "g_game.h"

#include "command.h"
#include "m_random.h"
#include "r_defs.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"

#define MAX_ACS_SCRIPT_VARS 10
#define MAX_ACS_MAP_VARS 32
#define MAX_ACS_WORLD_VARS 64
#define ACS_STACK_DEPTH 32
#define MAX_ACS_STORE 20

typedef enum
{
  ACS_inactive,
  ACS_running,
  ACS_suspended,
  ACS_waitfortag,
  ACS_waitforpoly,
  ACS_waitforscript,
  ACS_terminating
} acs_state_t;


struct acsInfo_t
{
  int  number;
  int *address;
  int  argCount;
  acs_state_t state;
  int  waitValue;
};

class acs_t : public Thinker
{
public:
  Actor *activator;
  line_t *line;
  int side;
  int number;
  int infoIndex;
  int delayCount;
  int stak[ACS_STACK_DEPTH];
  int stackPtr;
  int vars[MAX_ACS_SCRIPT_VARS];
  int *ip;

public:
  virtual void Think();
  // TODO new and delete for hub/global scripts (not PU_LEVSPEC)
};

struct acsstore_t
{
  int map;		// Target map
  int script;		// Script number on target map
  byte args[4];	// Padded to 4 for alignment
};


void P_ACSInitNewGame();
void P_CheckACSStore();


extern int MapVars[MAX_ACS_MAP_VARS];
extern int WorldVars[MAX_ACS_WORLD_VARS];
extern acsstore_t ACSStore[MAX_ACS_STORE+1]; // +1 for termination marker

// static stuff

#define SCRIPT_CONTINUE 0
#define SCRIPT_STOP 1
#define SCRIPT_TERMINATE 2
#define OPEN_SCRIPTS_BASE 1000
#define PRINT_BUFFER_SIZE 256
#define GAME_SINGLE_PLAYER 0
#define GAME_NET_COOPERATIVE 1
#define GAME_NET_DEATHMATCH 2
#define TEXTURE_TOP 0
#define TEXTURE_MIDDLE 1
#define TEXTURE_BOTTOM 2
#define S_DROP ACScript->stackPtr--
#define S_POP ACScript->stak[--ACScript->stackPtr]
#define S_PUSH(x) ACScript->stak[ACScript->stackPtr++] = x



struct acsHeader_t
{
  int marker;
  int infoOffset;
  int code;
};


//static bool AddToACSStore(int map, int number, byte *args);
//static int GetACSIndex(int number);
static void Push(int value);
static int Pop();
static int Top();
static void Drop();

static int CmdNOP();
static int CmdTerminate();
static int CmdSuspend();
static int CmdPushNumber();
static int CmdLSpec1();
static int CmdLSpec2();
static int CmdLSpec3();
static int CmdLSpec4();
static int CmdLSpec5();
static int CmdLSpec1Direct();
static int CmdLSpec2Direct();
static int CmdLSpec3Direct();
static int CmdLSpec4Direct();
static int CmdLSpec5Direct();
static int CmdAdd();
static int CmdSubtract();
static int CmdMultiply();
static int CmdDivide();
static int CmdModulus();
static int CmdEQ();
static int CmdNE();
static int CmdLT();
static int CmdGT();
static int CmdLE();
static int CmdGE();
static int CmdAssignScriptVar();
static int CmdAssignMapVar();
static int CmdAssignWorldVar();
static int CmdPushScriptVar();
static int CmdPushMapVar();
static int CmdPushWorldVar();
static int CmdAddScriptVar();
static int CmdAddMapVar();
static int CmdAddWorldVar();
static int CmdSubScriptVar();
static int CmdSubMapVar();
static int CmdSubWorldVar();
static int CmdMulScriptVar();
static int CmdMulMapVar();
static int CmdMulWorldVar();
static int CmdDivScriptVar();
static int CmdDivMapVar();
static int CmdDivWorldVar();
static int CmdModScriptVar();
static int CmdModMapVar();
static int CmdModWorldVar();
static int CmdIncScriptVar();
static int CmdIncMapVar();
static int CmdIncWorldVar();
static int CmdDecScriptVar();
static int CmdDecMapVar();
static int CmdDecWorldVar();
static int CmdGoto();
static int CmdIfGoto();
static int CmdDrop();
static int CmdDelay();
static int CmdDelayDirect();
static int CmdRandom();
static int CmdRandomDirect();
static int CmdThingCount();
static int CmdThingCountDirect();
static int CmdTagWait();
static int CmdTagWaitDirect();
static int CmdPolyWait();
static int CmdPolyWaitDirect();
static int CmdChangeFloor();
static int CmdChangeFloorDirect();
static int CmdChangeCeiling();
static int CmdChangeCeilingDirect();
static int CmdRestart();
static int CmdAndLogical();
static int CmdOrLogical();
static int CmdAndBitwise();
static int CmdOrBitwise();
static int CmdEorBitwise();
static int CmdNegateLogical();
static int CmdLShift();
static int CmdRShift();
static int CmdUnaryMinus();
static int CmdIfNotGoto();
static int CmdLineSide();
static int CmdScriptWait();
static int CmdScriptWaitDirect();
static int CmdClearLineSpecial();
static int CmdCaseGoto();
static int CmdBeginPrint();
static int CmdEndPrint();
static int CmdPrintString();
static int CmdPrintNumber();
static int CmdPrintCharacter();
static int CmdPlayerCount();
static int CmdGameType();
static int CmdGameSkill();
static int CmdTimer();
static int CmdSectorSound();
static int CmdAmbientSound();
static int CmdSoundSequence();
static int CmdSetLineTexture();
static int CmdSetLineBlocking();
static int CmdSetLineSpecial();
static int CmdThingSound();
static int CmdEndPrintBold();

static void ThingCount(int type, int tid);


int MapVars[MAX_ACS_MAP_VARS];
int WorldVars[MAX_ACS_WORLD_VARS];
acsstore_t ACSStore[MAX_ACS_STORE+1]; // +1 for termination marker

static acs_t *ACScript; // script being interpreted
static Map   *ACMap;    // where it runs (shorthand)
static int *PCodePtr;
static byte SpecArgs[8];
static char PrintBuffer[PRINT_BUFFER_SIZE];
static acs_t *NewScript;

// pcode-to-function table
static int (*PCodeCmds[])() =
{
  CmdNOP,
  CmdTerminate,
  CmdSuspend,
  CmdPushNumber,
  CmdLSpec1,
  CmdLSpec2,
  CmdLSpec3,
  CmdLSpec4,
  CmdLSpec5,
  CmdLSpec1Direct,
  CmdLSpec2Direct,
  CmdLSpec3Direct,
  CmdLSpec4Direct,
  CmdLSpec5Direct,
  CmdAdd,
  CmdSubtract,
  CmdMultiply,
  CmdDivide,
  CmdModulus,
  CmdEQ,
  CmdNE,
  CmdLT,
  CmdGT,
  CmdLE,
  CmdGE,
  CmdAssignScriptVar,
  CmdAssignMapVar,
  CmdAssignWorldVar,
  CmdPushScriptVar,
  CmdPushMapVar,
  CmdPushWorldVar,
  CmdAddScriptVar,
  CmdAddMapVar,
  CmdAddWorldVar,
  CmdSubScriptVar,
  CmdSubMapVar,
  CmdSubWorldVar,
  CmdMulScriptVar,
  CmdMulMapVar,
  CmdMulWorldVar,
  CmdDivScriptVar,
  CmdDivMapVar,
  CmdDivWorldVar,
  CmdModScriptVar,
  CmdModMapVar,
  CmdModWorldVar,
  CmdIncScriptVar,
  CmdIncMapVar,
  CmdIncWorldVar,
  CmdDecScriptVar,
  CmdDecMapVar,
  CmdDecWorldVar,
  CmdGoto,
  CmdIfGoto,
  CmdDrop,
  CmdDelay,
  CmdDelayDirect,
  CmdRandom,
  CmdRandomDirect,
  CmdThingCount,
  CmdThingCountDirect,
  CmdTagWait,
  CmdTagWaitDirect,
  CmdPolyWait,
  CmdPolyWaitDirect,
  CmdChangeFloor,
  CmdChangeFloorDirect,
  CmdChangeCeiling,
  CmdChangeCeilingDirect,
  CmdRestart,
  CmdAndLogical,
  CmdOrLogical,
  CmdAndBitwise,
  CmdOrBitwise,
  CmdEorBitwise,
  CmdNegateLogical,
  CmdLShift,
  CmdRShift,
  CmdUnaryMinus,
  CmdIfNotGoto,
  CmdLineSide,
  CmdScriptWait,
  CmdScriptWaitDirect,
  CmdClearLineSpecial,
  CmdCaseGoto,
  CmdBeginPrint,
  CmdEndPrint,
  CmdPrintString,
  CmdPrintNumber,
  CmdPrintCharacter,
  CmdPlayerCount,
  CmdGameType,
  CmdGameSkill,
  CmdTimer,
  CmdSectorSound,
  CmdAmbientSound,
  CmdSoundSequence,
  CmdSetLineTexture,
  CmdSetLineBlocking,
  CmdSetLineSpecial,
  CmdThingSound,
  CmdEndPrintBold
};


//==========================================================================
//
// was P_LoadACScripts
//
//==========================================================================

void Map::LoadACScripts(int lump)
{
  if (hexen_format == false)
    return;

  int i;
  acsInfo_t *info;

  acsHeader_t *header = (acsHeader_t *)fc.CacheLumpNum(lump, PU_LEVEL);
  ActionCodeBase = (byte *)header;
  int *buffer = (int *)((byte *)header+header->infoOffset);
  ACScriptCount = *buffer++;
  if(ACScriptCount == 0)
    { // Empty behavior lump
      return;
    }
  ACSInfo = (acsInfo_t *)Z_Malloc(ACScriptCount*sizeof(acsInfo_t), PU_LEVEL, 0);
  memset(ACSInfo, 0, ACScriptCount*sizeof(acsInfo_t));
  for(i = 0, info = ACSInfo; i < ACScriptCount; i++, info++)
    {
      info->number = *buffer++;
      info->address = (int *)((byte *)ActionCodeBase+*buffer++);
      info->argCount = *buffer++;
      if(info->number >= OPEN_SCRIPTS_BASE)
	{ // Auto-activate
	  info->number -= OPEN_SCRIPTS_BASE;
	  StartOpenACS(info->number, i, info->address);
	  info->state = ACS_running;
	}
      else
	{
	  info->state = ACS_inactive;
	}
    }
  ACStringCount = *buffer++;
  ACStrings = (char **)buffer;
  for(i = 0; i < ACStringCount; i++)
    {
      ACStrings[i] += (int)ActionCodeBase;
    }
  memset(MapVars, 0, sizeof(MapVars));
}

//==========================================================================
//
// StartOpenACS
//
//==========================================================================

void Map::StartOpenACS(int number, int infoIndex, int *address)
{
  acs_t *script = new acs_t;
  memset(script, 0, sizeof(acs_t));
  script->number = number;

  // World objects are allotted 1 second for initialization
  script->delayCount = 35;

  script->infoIndex = infoIndex;
  script->ip = address;
  AddThinker(script);
}

//==========================================================================
//
// P_CheckACSStore
//
// Scans the ACS store and executes all scripts belonging to the current
// map.
//
//==========================================================================

void P_CheckACSStore()
{
  acsstore_t *store;

  /*
  for(store = ACSStore; store->map != 0; store++)
    {
      if(store->map == gamemap)
	{
	  P_StartACS(store->script, 0, store->args, NULL, NULL, 0);
	  if(NewScript)
	    {
	      NewScript->delayCount = 35;
	    }
	  store->map = -1;
	}
    }
  */
}

//==========================================================================
//
// was P_StartACS
//
//==========================================================================

bool Map::StartACS(int number, byte *args, Actor *activator, line_t *line, int side)
{
  int i;

  NewScript = NULL;
  /*
  if(map && map != gamemap)
    { // Add to the script store
      return AddToACSStore(map, number, args);
    }
  */
  int infoIndex = GetACSIndex(number);
  if(infoIndex == -1)
    { // Script not found
      I_Error("P_StartACS: Unknown script number %d\n", number);
    }
  acs_state_t *statePtr = &ACSInfo[infoIndex].state;
  if(*statePtr == ACS_suspended)
    { // Resume a suspended script
      *statePtr = ACS_running;
      return true;
    }
  if(*statePtr != ACS_inactive)
    { // Script is already executing
      return false;
    }
  acs_t *script = new acs_t;
  memset(script, 0, sizeof(acs_t));
  script->number = number;
  script->infoIndex = infoIndex;
  script->activator = activator;
  script->line = line;
  script->side = side;
  script->ip = ACSInfo[infoIndex].address;

  for(i = 0; i < ACSInfo[infoIndex].argCount; i++)
    {
      script->vars[i] = args[i];
    }
  *statePtr = ACS_running;
  AddThinker(script);
  NewScript = script;
  return true;
}

//==========================================================================
//
// AddToACSStore
//
//==========================================================================

static bool AddToACSStore(int map, int number, byte *args)
{
  int i;
  int index;

  index = -1;
  for(i = 0; ACSStore[i].map != 0; i++)
    {
      if(ACSStore[i].script == number
	 && ACSStore[i].map == map)
	{ // Don't allow duplicates
	  return false;
	}
      if(index == -1 && ACSStore[i].map == -1)
	{ // Remember first empty slot
	  index = i;
	}
    }
  if(index == -1)
    { // Append required
      if(i == MAX_ACS_STORE)
	{
	  I_Error("AddToACSStore: MAX_ACS_STORE (%d) exceeded.",
		  MAX_ACS_STORE);
	}
      index = i;
      ACSStore[index+1].map = 0;
    }
  ACSStore[index].map = map;
  ACSStore[index].script = number;
  *((int *)ACSStore[index].args) = *((int *)args);
  return true;
}


//==========================================================================
//
// was P_TerminateACS
//
//==========================================================================

bool Map::TerminateACS(int number)
{
  int infoIndex;

  infoIndex = GetACSIndex(number);
  if(infoIndex == -1)
    { // Script not found
      return false;
    }
  if(ACSInfo[infoIndex].state == ACS_inactive
     || ACSInfo[infoIndex].state == ACS_terminating)
    { // States that disallow termination
      return false;
    }
  ACSInfo[infoIndex].state = ACS_terminating;
  return true;
}

//==========================================================================
//
// was P_SuspendACS
//
//==========================================================================

bool Map::SuspendACS(int number)
{
  int infoIndex;

  infoIndex = GetACSIndex(number);
  if(infoIndex == -1)
    { // Script not found
      return false;
    }
  if(ACSInfo[infoIndex].state == ACS_inactive
     || ACSInfo[infoIndex].state == ACS_suspended
     || ACSInfo[infoIndex].state == ACS_terminating)
    { // States that disallow suspension
      return false;
    }
  ACSInfo[infoIndex].state = ACS_suspended;
  return true;
}

//==========================================================================
//
// P_Init
//
//==========================================================================

void P_ACSInitNewGame()
{
  memset(WorldVars, 0, sizeof(WorldVars));
  memset(ACSStore, 0, sizeof(ACSStore));
}

//==========================================================================
//
// was T_InterpretACS
//
//==========================================================================

void acs_t::Think()
{
  int cmd;
  int action;

  if (mp->ACSInfo[infoIndex].state == ACS_terminating)
    {
      mp->ACSInfo[infoIndex].state = ACS_inactive;
      mp->ScriptFinished(ACScript->number);
      mp->RemoveThinker(this);
      return;
    }
  if (mp->ACSInfo[infoIndex].state != ACS_running)
    {
      return;
    }
  if(delayCount)
    {
      delayCount--;
      return;
    }

  ACScript = this;
  ACMap = mp;

  PCodePtr = ACScript->ip;
  do
    {
      cmd = *PCodePtr++;
      action = PCodeCmds[cmd]();
    } while(action == SCRIPT_CONTINUE);
  ACScript->ip = PCodePtr;
  if(action == SCRIPT_TERMINATE)
    {
      mp->ACSInfo[infoIndex].state = ACS_inactive;
      mp->ScriptFinished(ACScript->number);
      mp->RemoveThinker(this);
    }
}

//==========================================================================
//
// was P_TagFinished
//
//==========================================================================

void Map::TagFinished(int tag)
{
  int i;

  if(TagBusy(tag) == true)
    {
      return;
    }
  for(i = 0; i < ACScriptCount; i++)
    {
      if(ACSInfo[i].state == ACS_waitfortag
	 && ACSInfo[i].waitValue == tag)
	{
	  ACSInfo[i].state = ACS_running;
	}
    }
}

//==========================================================================
//
// was P_PolyobjFinished
//
//==========================================================================

void Map::PolyobjFinished(int po)
{
  int i;

  if (PO_Busy(po) == true)
    return;

  for(i = 0; i < ACScriptCount; i++)
    {
      if(ACSInfo[i].state == ACS_waitforpoly
	 && ACSInfo[i].waitValue == po)
	{
	  ACSInfo[i].state = ACS_running;
	}
    }
}

//==========================================================================
//
// ScriptFinished
//
//==========================================================================

void Map::ScriptFinished(int number)
{
  int i;

  for(i = 0; i < ACScriptCount; i++)
    {
      if(ACSInfo[i].state == ACS_waitforscript
	 && ACSInfo[i].waitValue == number)
	{
	  ACSInfo[i].state = ACS_running;
	}
    }
}

//==========================================================================
//
// TagBusy
//
//==========================================================================

bool Map::TagBusy(int tag)
{
  int sectorIndex;

  sectorIndex = -1;
  while((sectorIndex = FindSectorFromTag(tag, sectorIndex)) >= 0)
    {
      if (sectors[sectorIndex].floordata ||
	  sectors[sectorIndex].ceilingdata ||
	  sectors[sectorIndex].lightingdata)
	{
	  return true;
	}
    }
  return false;
}

//==========================================================================
//
// GetACSIndex
//
// Returns the index of a script number.  Returns -1 if the script number
// is not found.
//
//==========================================================================

int Map::GetACSIndex(int number)
{
  int i;

  for(i = 0; i < ACScriptCount; i++)
    if (ACSInfo[i].number == number)
      return i;

  return -1;
}

//==========================================================================
//
// Push
//
//==========================================================================

static void Push(int value)
{
  ACScript->stak[ACScript->stackPtr++] = value;
}

//==========================================================================
//
// Pop
//
//==========================================================================

static int Pop()
{
  return ACScript->stak[--ACScript->stackPtr];
}

//==========================================================================
//
// Top
//
//==========================================================================

static int Top()
{
  return ACScript->stak[ACScript->stackPtr-1];
}

//==========================================================================
//
// Drop
//
//==========================================================================

static void Drop()
{
  ACScript->stackPtr--;
}

//==========================================================================
//
// P-Code Commands
//
//==========================================================================

static int CmdNOP()
{
  return SCRIPT_CONTINUE;
}

static int CmdTerminate()
{
  return SCRIPT_TERMINATE;
}

static int CmdSuspend()
{
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_suspended;
  return SCRIPT_STOP;
}

static int CmdPushNumber()
{
  Push(*PCodePtr++);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec1()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[0] = Pop();
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			    ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec2()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[1] = Pop();
  SpecArgs[0] = Pop();
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			    ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec3()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[2] = Pop();
  SpecArgs[1] = Pop();
  SpecArgs[0] = Pop();
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			    ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec4()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[3] = Pop();
  SpecArgs[2] = Pop();
  SpecArgs[1] = Pop();
  SpecArgs[0] = Pop();
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			    ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec5()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[4] = Pop();
  SpecArgs[3] = Pop();
  SpecArgs[2] = Pop();
  SpecArgs[1] = Pop();
  SpecArgs[0] = Pop();
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
			    ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec1Direct()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[0] = *PCodePtr++;
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
		       ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec2Direct()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[0] = *PCodePtr++;
  SpecArgs[1] = *PCodePtr++;
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
		       ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec3Direct()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[0] = *PCodePtr++;
  SpecArgs[1] = *PCodePtr++;
  SpecArgs[2] = *PCodePtr++;
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
		       ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec4Direct()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[0] = *PCodePtr++;
  SpecArgs[1] = *PCodePtr++;
  SpecArgs[2] = *PCodePtr++;
  SpecArgs[3] = *PCodePtr++;
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
		       ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdLSpec5Direct()
{
  int special;

  special = *PCodePtr++;
  SpecArgs[0] = *PCodePtr++;
  SpecArgs[1] = *PCodePtr++;
  SpecArgs[2] = *PCodePtr++;
  SpecArgs[3] = *PCodePtr++;
  SpecArgs[4] = *PCodePtr++;
  ACMap->ExecuteLineSpecial(special, SpecArgs, ACScript->line,
		       ACScript->side, ACScript->activator);
  return SCRIPT_CONTINUE;
}

static int CmdAdd()
{
  Push(Pop()+Pop());
  return SCRIPT_CONTINUE;
}

static int CmdSubtract()
{
  int operand2;

  operand2 = Pop();
  Push(Pop()-operand2);
  return SCRIPT_CONTINUE;
}

static int CmdMultiply()
{
  Push(Pop()*Pop());
  return SCRIPT_CONTINUE;
}

static int CmdDivide()
{
  int operand2;

  operand2 = Pop();
  Push(Pop()/operand2);
  return SCRIPT_CONTINUE;
}

static int CmdModulus()
{
  int operand2;

  operand2 = Pop();
  Push(Pop()%operand2);
  return SCRIPT_CONTINUE;
}

static int CmdEQ()
{
  Push(Pop() == Pop());
  return SCRIPT_CONTINUE;
}

static int CmdNE()
{
  Push(Pop() != Pop());
  return SCRIPT_CONTINUE;
}

static int CmdLT()
{
  int operand2;

  operand2 = Pop();
  Push(Pop() < operand2);
  return SCRIPT_CONTINUE;
}

static int CmdGT()
{
  int operand2;

  operand2 = Pop();
  Push(Pop() > operand2);
  return SCRIPT_CONTINUE;
}

static int CmdLE()
{
  int operand2;

  operand2 = Pop();
  Push(Pop() <= operand2);
  return SCRIPT_CONTINUE;
}

static int CmdGE()
{
  int operand2;

  operand2 = Pop();
  Push(Pop() >= operand2);
  return SCRIPT_CONTINUE;
}

static int CmdAssignScriptVar()
{
  ACScript->vars[*PCodePtr++] = Pop();
  return SCRIPT_CONTINUE;
}

static int CmdAssignMapVar()
{
  MapVars[*PCodePtr++] = Pop();
  return SCRIPT_CONTINUE;
}

static int CmdAssignWorldVar()
{
  WorldVars[*PCodePtr++] = Pop();
  return SCRIPT_CONTINUE;
}

static int CmdPushScriptVar()
{
  Push(ACScript->vars[*PCodePtr++]);
  return SCRIPT_CONTINUE;
}

static int CmdPushMapVar()
{
  Push(MapVars[*PCodePtr++]);
  return SCRIPT_CONTINUE;
}

static int CmdPushWorldVar()
{
  Push(WorldVars[*PCodePtr++]);
  return SCRIPT_CONTINUE;
}

static int CmdAddScriptVar()
{
  ACScript->vars[*PCodePtr++] += Pop();
  return SCRIPT_CONTINUE;
}

static int CmdAddMapVar()
{
  MapVars[*PCodePtr++] += Pop();
  return SCRIPT_CONTINUE;
}

static int CmdAddWorldVar()
{
  WorldVars[*PCodePtr++] += Pop();
  return SCRIPT_CONTINUE;
}

static int CmdSubScriptVar()
{
  ACScript->vars[*PCodePtr++] -= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdSubMapVar()
{
  MapVars[*PCodePtr++] -= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdSubWorldVar()
{
  WorldVars[*PCodePtr++] -= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdMulScriptVar()
{
  ACScript->vars[*PCodePtr++] *= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdMulMapVar()
{
  MapVars[*PCodePtr++] *= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdMulWorldVar()
{
  WorldVars[*PCodePtr++] *= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdDivScriptVar()
{
  ACScript->vars[*PCodePtr++] /= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdDivMapVar()
{
  MapVars[*PCodePtr++] /= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdDivWorldVar()
{
  WorldVars[*PCodePtr++] /= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdModScriptVar()
{
  ACScript->vars[*PCodePtr++] %= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdModMapVar()
{
  MapVars[*PCodePtr++] %= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdModWorldVar()
{
  WorldVars[*PCodePtr++] %= Pop();
  return SCRIPT_CONTINUE;
}

static int CmdIncScriptVar()
{
  ACScript->vars[*PCodePtr++]++;
  return SCRIPT_CONTINUE;
}

static int CmdIncMapVar()
{
  MapVars[*PCodePtr++]++;
  return SCRIPT_CONTINUE;
}

static int CmdIncWorldVar()
{
  WorldVars[*PCodePtr++]++;
  return SCRIPT_CONTINUE;
}

static int CmdDecScriptVar()
{
  ACScript->vars[*PCodePtr++]--;
  return SCRIPT_CONTINUE;
}

static int CmdDecMapVar()
{
  MapVars[*PCodePtr++]--;
  return SCRIPT_CONTINUE;
}

static int CmdDecWorldVar()
{
  WorldVars[*PCodePtr++]--;
  return SCRIPT_CONTINUE;
}

static int CmdGoto()
{
  PCodePtr = (int *)(ACMap->ActionCodeBase+*PCodePtr);
  return SCRIPT_CONTINUE;
}

static int CmdIfGoto()
{
  if(Pop())
    {
      PCodePtr = (int *)(ACMap->ActionCodeBase+*PCodePtr);
    }
  else
    {
      PCodePtr++;
    }
  return SCRIPT_CONTINUE;
}

static int CmdDrop()
{
  Drop();
  return SCRIPT_CONTINUE;
}

static int CmdDelay()
{
  ACScript->delayCount = Pop();
  return SCRIPT_STOP;
}

static int CmdDelayDirect()
{
  ACScript->delayCount = *PCodePtr++;
  return SCRIPT_STOP;
}

static int CmdRandom()
{
  int low;
  int high;

  high = Pop();
  low = Pop();
  Push(low+(P_Random()%(high-low+1)));
  return SCRIPT_CONTINUE;
}

static int CmdRandomDirect()
{
  int low;
  int high;

  low = *PCodePtr++;
  high = *PCodePtr++;
  Push(low+(P_Random()%(high-low+1)));
  return SCRIPT_CONTINUE;
}

static int CmdThingCount()
{
  int tid;

  tid = Pop();
  ThingCount(Pop(), tid);
  return SCRIPT_CONTINUE;
}

static int CmdThingCountDirect()
{
  int type;

  type = *PCodePtr++;
  ThingCount(type, *PCodePtr++);
  return SCRIPT_CONTINUE;
}

static void ThingCount(int type, int tid)
{
  int count;
  int searcher;
  Actor *mobj;
  mobjtype_t moType;

  if(!(type+tid))
    { // Nothing to count
      return;
    }
  /* FIXME when TID system works
  moType = TranslateThingType[type];
  count = 0;
  searcher = -1;
  if(tid)
    { // Count TID things
      while((mobj = P_FindMobjFromTID(tid, &searcher)) != NULL)
	{
	  if(type == 0)
	    { // Just count TIDs
	      count++;
	    }
	  else if(moType == mobj->type)
	    {
	      if(mobj->flags&MF_COUNTKILL && mobj->health <= 0)
		{ // Don't count dead monsters
		  continue;
		}
	      count++;
	    }
	}
    }
  else
    { // Count only types
      for(think = thinkercap.next; think != &thinkercap;
	  think = think->next)
	{
	  if(think->function != P_MobjThinker)
	    { // Not a mobj thinker
	      continue;
	    }
	  mobj = (Actor *)think;
	  if(mobj->type != moType)
	    { // Doesn't match
	      continue;
	    }
	  if(mobj->flags&MF_COUNTKILL && mobj->health <= 0)
	    { // Don't count dead monsters
	      continue;
	    }
	  count++;
	}
    }
  Push(count);
  */
  Push(0); // temporary
}

static int CmdTagWait()
{
  ACMap->ACSInfo[ACScript->infoIndex].waitValue = Pop();
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_waitfortag;
  return SCRIPT_STOP;
}

static int CmdTagWaitDirect()
{
  ACMap->ACSInfo[ACScript->infoIndex].waitValue = *PCodePtr++;
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_waitfortag;
  return SCRIPT_STOP;
}

static int CmdPolyWait()
{
  ACMap->ACSInfo[ACScript->infoIndex].waitValue = Pop();
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_waitforpoly;
  return SCRIPT_STOP;
}

static int CmdPolyWaitDirect()
{
  ACMap->ACSInfo[ACScript->infoIndex].waitValue = *PCodePtr++;
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_waitforpoly;
  return SCRIPT_STOP;
}

static int CmdChangeFloor()
{
  int tag;
  int flat;
  int sectorIndex;

  flat = R_FlatNumForName(ACMap->ACStrings[Pop()]);
  tag = Pop();
  sectorIndex = -1;
  while((sectorIndex = ACMap->FindSectorFromTag(tag, sectorIndex)) >= 0)
    {
      ACMap->sectors[sectorIndex].floorpic = flat;
    }
  return SCRIPT_CONTINUE;
}

static int CmdChangeFloorDirect()
{
  int tag;
  int flat;
  int sectorIndex;

  tag = *PCodePtr++;
  flat = R_FlatNumForName(ACMap->ACStrings[*PCodePtr++]);
  sectorIndex = -1;
  while((sectorIndex = ACMap->FindSectorFromTag(tag, sectorIndex)) >= 0)
    {
      ACMap->sectors[sectorIndex].floorpic = flat;
    }
  return SCRIPT_CONTINUE;
}

static int CmdChangeCeiling()
{
  int tag;
  int flat;
  int sectorIndex;

  flat = R_FlatNumForName(ACMap->ACStrings[Pop()]);
  tag = Pop();
  sectorIndex = -1;
  while((sectorIndex = ACMap->FindSectorFromTag(tag, sectorIndex)) >= 0)
    {
      ACMap->sectors[sectorIndex].ceilingpic = flat;
    }
  return SCRIPT_CONTINUE;
}

static int CmdChangeCeilingDirect()
{
  int tag;
  int flat;
  int sectorIndex;

  tag = *PCodePtr++;
  flat = R_FlatNumForName(ACMap->ACStrings[*PCodePtr++]);
  sectorIndex = -1;
  while((sectorIndex = ACMap->FindSectorFromTag(tag, sectorIndex)) >= 0)
    {
      ACMap->sectors[sectorIndex].ceilingpic = flat;
    }
  return SCRIPT_CONTINUE;
}

static int CmdRestart()
{
  PCodePtr = ACMap->ACSInfo[ACScript->infoIndex].address;
  return SCRIPT_CONTINUE;
}

static int CmdAndLogical()
{
  Push(Pop() && Pop());
  return SCRIPT_CONTINUE;
}

static int CmdOrLogical()
{
  Push(Pop() || Pop());
  return SCRIPT_CONTINUE;
}

static int CmdAndBitwise()
{
  Push(Pop()&Pop());
  return SCRIPT_CONTINUE;
}

static int CmdOrBitwise()
{
  Push(Pop()|Pop());
  return SCRIPT_CONTINUE;
}

static int CmdEorBitwise()
{
  Push(Pop()^Pop());
  return SCRIPT_CONTINUE;
}

static int CmdNegateLogical()
{
  Push(!Pop());
  return SCRIPT_CONTINUE;
}

static int CmdLShift()
{
  int operand2;

  operand2 = Pop();
  Push(Pop()<<operand2);
  return SCRIPT_CONTINUE;
}

static int CmdRShift()
{
  int operand2;

  operand2 = Pop();
  Push(Pop()>>operand2);
  return SCRIPT_CONTINUE;
}

static int CmdUnaryMinus()
{
  Push(-Pop());
  return SCRIPT_CONTINUE;
}

static int CmdIfNotGoto()
{
  if(Pop())
    {
      PCodePtr++;
    }
  else
    {
      PCodePtr = (int *)(ACMap->ActionCodeBase+*PCodePtr);
    }
  return SCRIPT_CONTINUE;
}

static int CmdLineSide()
{
  Push(ACScript->side);
  return SCRIPT_CONTINUE;
}

static int CmdScriptWait()
{
  ACMap->ACSInfo[ACScript->infoIndex].waitValue = Pop();
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_waitforscript;
  return SCRIPT_STOP;
}

static int CmdScriptWaitDirect()
{
  ACMap->ACSInfo[ACScript->infoIndex].waitValue = *PCodePtr++;
  ACMap->ACSInfo[ACScript->infoIndex].state = ACS_waitforscript;
  return SCRIPT_STOP;
}

static int CmdClearLineSpecial()
{
  if(ACScript->line)
    {
      ACScript->line->special = 0;
    }
  return SCRIPT_CONTINUE;
}

static int CmdCaseGoto()
{
  if(Top() == *PCodePtr++)
    {
      PCodePtr = (int *)(ACMap->ActionCodeBase+*PCodePtr);
      Drop();
    }
  else
    {
      PCodePtr++;
    }
  return SCRIPT_CONTINUE;
}

static int CmdBeginPrint()
{
  *PrintBuffer = 0;
  return SCRIPT_CONTINUE;
}

static int CmdEndPrint()
{
  PlayerPawn *p;

  if (ACScript->activator && ACScript->activator->Type() == Thinker::tt_ppawn)
    {
      p = (PlayerPawn *)ACScript->activator;
    }
  else
    {
      p = consoleplayer->pawn;
    }
  if (p->player)
    p->player->SetMessage(PrintBuffer, true);
  return SCRIPT_CONTINUE;
}

static int CmdEndPrintBold()
{
  int i, n = ACMap->players.size();

  for(i = 0; i < n; i++)
    {
      ACMap->players[i]->SetMessage(PrintBuffer, true);
    }
  return SCRIPT_CONTINUE;
}

static int CmdPrintString()
{
  strcat(PrintBuffer, ACMap->ACStrings[Pop()]);
  return SCRIPT_CONTINUE;
}

static int CmdPrintNumber()
{
  char tempStr[16];

  sprintf(tempStr, "%d", Pop());
  strcat(PrintBuffer, tempStr);
  return SCRIPT_CONTINUE;
}

static int CmdPrintCharacter()
{
  char *bufferEnd;

  bufferEnd = PrintBuffer+strlen(PrintBuffer);
  *bufferEnd++ = Pop();
  *bufferEnd = 0;
  return SCRIPT_CONTINUE;
}

static int CmdPlayerCount()
{
  int count = ACMap->players.size();

  Push(count);
  return SCRIPT_CONTINUE;
}

static int CmdGameType()
{
  extern consvar_t cv_deathmatch;
  int gametype;

  if(game.multiplayer == false)
    {
      gametype = GAME_SINGLE_PLAYER;
    }
  else if(cv_deathmatch.value)
    {
      gametype = GAME_NET_DEATHMATCH;
    }
  else
    {
      gametype = GAME_NET_COOPERATIVE;
    }
  Push(gametype);
  return SCRIPT_CONTINUE;
}

static int CmdGameSkill()
{
  Push(game.skill);
  return SCRIPT_CONTINUE;
}

static int CmdTimer()
{
  Push(gametic);
  return SCRIPT_CONTINUE;
}

static int CmdSectorSound()
{
  int volume;
  mappoint_t *orig = NULL;
  if(ACScript->line)
    {
      orig = &ACScript->line->frontsector->soundorg;
    }
  volume = Pop();

  // FIXME the sound functions need to be fixed someday
  //S_StartSound(orig, S_GetSoundID(ACMap->ACStrings[Pop()]), volume);
  return SCRIPT_CONTINUE;
}

static int CmdThingSound()
{
  int tid;
  int sound;
  int volume;
  Actor *mobj;
  int searcher;

  volume = Pop();
  //sound = S_GetSoundID(ACMap->ACStrings[Pop()]);
  tid = Pop();
  searcher = -1;
  /*
  while((mobj = P_FindMobjFromTID(tid, &searcher)) != NULL)
    {
      S_StartSoundAtVolume(mobj, sound, volume);
    }
  */
  return SCRIPT_CONTINUE;
}

static int CmdAmbientSound()
{
  int volume;

  volume = Pop();
  //S_StartSoundAtVolume(NULL, S_GetSoundID(ACMap->ACStrings[Pop()]), volume);
  return SCRIPT_CONTINUE;
}

static int CmdSoundSequence()
{
  mappoint_t *orig = NULL;

  if(ACScript->line)
    {
      orig = &ACScript->line->frontsector->soundorg;
    }
  //SN_StartSequenceName(mobj, ACMap->ACStrings[Pop()]);
  return SCRIPT_CONTINUE;
}

static int CmdSetLineTexture()
{
  line_t *line;
  int lineTag;
  int side;
  int position;
  int texture;
  int searcher;

  texture = R_TextureNumForName(ACMap->ACStrings[Pop()]);
  position = Pop();
  side = Pop();
  lineTag = Pop();
  searcher = -1;
  /*
    FIXME urgh. Somehow this Hexen P_FindLine should be replaced with the corresponding BOOM function used in Legacy...
  while((line = P_FindLine(lineTag, &searcher)) != NULL)
    {
      if(position == TEXTURE_MIDDLE)
	{
	  sides[line->sidenum[side]].midtexture = texture;
	}
      else if(position == TEXTURE_BOTTOM)
	{
	  sides[line->sidenum[side]].bottomtexture = texture;
	}
      else
	{ // TEXTURE_TOP
	  sides[line->sidenum[side]].toptexture = texture;
	}
    }
  */
  return SCRIPT_CONTINUE;
}

static int CmdSetLineBlocking()
{
  line_t *line;
  int lineTag;
  int blocking;
  int searcher;

  blocking = Pop() ? ML_BLOCKING : 0;
  lineTag = Pop();
  searcher = -1;
  /*
  while((line = P_FindLine(lineTag, &searcher)) != NULL)
    {
      line->flags = (line->flags&~ML_BLOCKING) | blocking;
    }
  */
  return SCRIPT_CONTINUE;
}

static int CmdSetLineSpecial()
{
  line_t *line;
  int lineTag;
  int special, arg1, arg2, arg3, arg4, arg5;
  int searcher;

  arg5 = Pop();
  arg4 = Pop();
  arg3 = Pop();
  arg2 = Pop();
  arg1 = Pop();
  special = Pop();
  lineTag = Pop();
  searcher = -1;
  /*
  while((line = P_FindLine(lineTag, &searcher)) != NULL)
    {
      line->special = special;
      line->arg1 = arg1;
      line->arg2 = arg2;
      line->arg3 = arg3;
      line->arg4 = arg4;
      line->arg5 = arg5;
    }
  */
  return SCRIPT_CONTINUE;
}
