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
// Revision 1.15  2004/01/10 16:02:59  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.14  2004/01/05 11:48:08  smite-meister
// 7 bugfixes
//
// Revision 1.13  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.12  2003/12/13 23:51:03  smite-meister
// Hexen update
//
// Revision 1.11  2003/11/30 00:09:47  smite-meister
// bugfixes
//
// Revision 1.10  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.9  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
// Revision 1.8  2003/05/11 21:23:51  smite-meister
// Hexen fixes
//
// Revision 1.7  2003/05/05 00:24:49  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.6  2003/04/24 20:30:26  hurdler
// Remove lots of compiling warnings
//
// Revision 1.5  2003/04/20 17:35:38  smite-meister
// Hexen volume fix
//
// Revision 1.4  2003/04/14 08:58:29  smite-meister
// Hexen maps load.
//
// Revision 1.3  2003/04/04 00:01:57  smite-meister
// bugfixes, Hexen HUD
//
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
#include "g_mapinfo.h"
#include "g_map.h"
#include "g_player.h"
#include "g_game.h"

#include "p_acs.h"
#include "command.h"
#include "m_random.h"
#include "r_defs.h"
#include "r_data.h"
#include "sounds.h"

#include "w_wad.h"
#include "z_zone.h"


//=================================
//  global ACS stuff
//=================================

int WorldVars[MAX_ACS_WORLD_VARS];
multimap<int, acsstore_t> ACS_store;


void P_ACSInitNewGame()
{
  memset(WorldVars, 0, sizeof(WorldVars));
  ACS_store.clear();
}


bool P_AddToACSStore(int tmap, int number, byte *args)
{
  multimap<int, acsstore_t>::iterator i, j;
  i = ACS_store.lower_bound(tmap);
  j = ACS_store.upper_bound(tmap);
  while (i != j)
    {
      if ((*i).second.script == number)
	return false; // no duplicates
      i++;
    }

  acsstore_t temp;
  temp.tmap = tmap;
  temp.script = number;
  temp.args[0] = args[0];
  temp.args[1] = args[1];
  temp.args[2] = args[2];
  temp.args[3] = 0;

  ACS_store.insert(pair<const int, acsstore_t>(tmap, temp));
  return true;
}



//=================================
//  static stuff and prototypes
//=================================

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


static acs_t *ACScript; // script being interpreted
static Map   *ACMap;    // where it runs (shorthand)
static int   *PCodePtr;
static byte   SpecArgs[8];
static char   PrintBuffer[PRINT_BUFFER_SIZE];
static acs_t *NewScript; // was a new script just started by Map::StartACS ? TODO should be a return value...



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



//=================================================
// Console command for running ACS scripts
//=================================================

void Command_RunACS_f()
{
  if (COM_Argc() < 2)
    {
      CONS_Printf("Usage: run-acs <script>\n");
      return;
    }

  if (!consoleplayer || !consoleplayer->pawn)
    return;
  
  byte args[5] = {0,0,0,0,0};
  int num = atoi(COM_Argv(1));
  int i, n = COM_Argc() - 2;
  for (i=0; i<n; i++)
    args[i] = atoi(COM_Argv(i+2));

  // line_t* is always NULL...
  consoleplayer->mp->StartACS(num, args, consoleplayer->pawn, NULL, 0);
}


//=================================================
//  Map class methods
//=================================================

// Initialization, called during Map setup.
void Map::LoadACScripts(int lump)
{
  if (hexen_format == false)
    return;

  int i;
  acsInfo_t *info;

  struct acsHeader_t
  {
    int marker;
    int infoOffset;
    int code;
  };

  acsHeader_t *header = (acsHeader_t *)fc.CacheLumpNum(lump, PU_LEVEL);
  ActionCodeBase = (byte *)header;
  int *buffer = (int *)((byte *)header+header->infoOffset);
  ACScriptCount = *buffer++;
  if (ACScriptCount == 0)
    return; // Empty behavior lump

  ACSInfo = (acsInfo_t *)Z_Malloc(ACScriptCount*sizeof(acsInfo_t), PU_LEVEL, 0);
  memset(ACSInfo, 0, ACScriptCount*sizeof(acsInfo_t));
  for (i = 0, info = ACSInfo; i < ACScriptCount; i++, info++)
    {
      info->number = *buffer++;
      info->address = (int *)(ActionCodeBase+*buffer++);
      info->argCount = *buffer++;
      if (info->number >= OPEN_SCRIPTS_BASE)
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
  for (i = 0; i < ACStringCount; i++)
    ACStrings[i] += (int)ActionCodeBase;

  memset(ACMapVars, 0, sizeof(ACMapVars));
}


// Scripts that always start immediately at level load.
void Map::StartOpenACS(int number, int infoIndex, int *address)
{
  CONS_Printf("Starting an opening ACS (script %d)\n", number);
  acs_t *script = new acs_t(number, infoIndex, address);

  // World objects are allotted 1 second for initialization
  script->delayCount = 35;

  AddThinker(script);
}


// Scans the ACS store and executes all scripts belonging to the current map.
// Called at Map setup.
void Map::CheckACSStore()
{
  int m = info->mapnumber;

  multimap<int, acsstore_t>::iterator i, j, k;
  i = j = ACS_store.lower_bound(m);
  k = ACS_store.upper_bound(m);
  while (i != k)
    {
      acsstore_t &s = (*i).second;
      StartACS(s.script, s.args, NULL, NULL, 0);
      if (NewScript)
	NewScript->delayCount = 35;
      i++;
    }

  ACS_store.erase(j, k);
}


// Starts a new script running.
bool Map::StartACS(int number, byte *args, Actor *activator, line_t *line, int side)
{
  NewScript = NULL;

  int infoIndex = GetACSIndex(number);
  if (infoIndex == -1)
    { // Script not found
      CONS_Printf("Map::StartACS: Unknown script number %d\n", number);
      return false;
    }

  CONS_Printf("Starting ACS script %d\n", number);

  acs_state_t *statePtr = &ACSInfo[infoIndex].state;
  if (*statePtr == ACS_suspended)
    { // Resume a suspended script
      *statePtr = ACS_running;
      return true;
    }
  if (*statePtr != ACS_inactive)
    return false; // Script is already executing

  acs_t *script = new acs_t(number, infoIndex, ACSInfo[infoIndex].address);

  script->activator = activator;
  script->line = line;
  script->side = side;

  for (int i = 0; i < ACSInfo[infoIndex].argCount; i++)
    script->vars[i] = args[i];

  *statePtr = ACS_running;
  AddThinker(script);
  NewScript = script;
  return true;
}


// Stops a running script
bool Map::TerminateACS(int number)
{
  int infoIndex = GetACSIndex(number);
  if (infoIndex == -1)
    return false; // Script not found

  if (ACSInfo[infoIndex].state == ACS_inactive
      || ACSInfo[infoIndex].state == ACS_terminating)
    { // States that disallow termination
      return false;
    }
  ACSInfo[infoIndex].state = ACS_terminating;
  return true;
}


// Pauses a running script
bool Map::SuspendACS(int number)
{
  int infoIndex = GetACSIndex(number);
  if (infoIndex == -1)
    return false; // Script not found

  if (ACSInfo[infoIndex].state == ACS_inactive
     || ACSInfo[infoIndex].state == ACS_suspended
     || ACSInfo[infoIndex].state == ACS_terminating)
    { // States that disallow suspension
      return false;
    }
  ACSInfo[infoIndex].state = ACS_suspended;
  return true;
}


// Unpauses scripts that are waiting for a particular sector tag to finish
void Map::TagFinished(int tag)
{
  if (TagBusy(tag) == true)
    return;

  for (int i = 0; i < ACScriptCount; i++)
    if (ACSInfo[i].state == ACS_waitfortag && ACSInfo[i].waitValue == tag)
      ACSInfo[i].state = ACS_running;
}


// Unpauses scripts that are waiting for a particular polyobj to finish
void Map::PolyobjFinished(int po)
{
  if (PO_Busy(po) == true)
    return;

  for (int i = 0; i < ACScriptCount; i++)
    if(ACSInfo[i].state == ACS_waitforpoly && ACSInfo[i].waitValue == po)
      ACSInfo[i].state = ACS_running;
}


// Unpauses scripts that are waiting for a particular script to finish
void Map::ScriptFinished(int number)
{
  for (int i = 0; i < ACScriptCount; i++)
    if (ACSInfo[i].state == ACS_waitforscript && ACSInfo[i].waitValue == number)
      ACSInfo[i].state = ACS_running;
}


// Checks if the sector(s) with a given tag are still active
bool Map::TagBusy(int tag)
{
  int sectorIndex = -1;
  while ((sectorIndex = FindSectorFromTag(tag, sectorIndex)) >= 0)
    {
      if (sectors[sectorIndex].floordata ||
	  sectors[sectorIndex].ceilingdata ||
	  sectors[sectorIndex].lightingdata)
	return true;
    }
  return false;
}


// Returns the index of a script number.
// Returns -1 if the script number is not found.
int Map::GetACSIndex(int number)
{
  for (int i = 0; i < ACScriptCount; i++)
    if (ACSInfo[i].number == number)
      return i;

  return -1;
}


//=================================================
//  Thinker class for running AC scripts
//=================================================

IMPLEMENT_CLASS(acs_t, "AC script");
acs_t::acs_t() {}


acs_t::acs_t(int num, int ii, int *addr)
{
  number = num;
  infoIndex = ii;
  ip = addr;

  activator = NULL;
  line = NULL;
  side = 0;
  delayCount = 0;
  stackPtr = 0;
  memset(stak, 0, sizeof(stak));
  memset(vars, 0, sizeof(vars));
}


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
    return;

  if (delayCount)
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
    } while (action == SCRIPT_CONTINUE);
  ACScript->ip = PCodePtr;
  if (action == SCRIPT_TERMINATE)
    {
      mp->ACSInfo[infoIndex].state = ACS_inactive;
      mp->ScriptFinished(ACScript->number);
      mp->RemoveThinker(this);
    }
}


//=================================================
//  The ACS interpreter functions
//=================================================


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
  ACMap->ACMapVars[*PCodePtr++] = Pop();
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
  Push(ACMap->ACMapVars[*PCodePtr++]);
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
  ACMap->ACMapVars[*PCodePtr++] += Pop();
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
  ACMap->ACMapVars[*PCodePtr++] -= Pop();
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
  ACMap->ACMapVars[*PCodePtr++] *= Pop();
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
  ACMap->ACMapVars[*PCodePtr++] /= Pop();
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
  ACMap->ACMapVars[*PCodePtr++] %= Pop();
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
  ACMap->ACMapVars[*PCodePtr++]++;
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
  ACMap->ACMapVars[*PCodePtr++]--;
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


static mobjtype_t moType;
static int thingCount;
static bool IT_TypeCount(Thinker *th)
{
  if (th->Type() == Thinker::tt_dactor)
    {
      DActor *m = (DActor *)th;

      if (m->type == moType)
	{
	  if (m->flags & MF_COUNTKILL && m->flags & MF_CORPSE)
	    return true; // Don't count dead monsters
	  thingCount++;
	}
    }
  return true;
}


static void ThingCount(int type, int tid)
{
  if (!(type + tid))
    return; // Nothing to count TODO messes stack up, need to Push something

  extern mobjtype_t TranslateThingType[];
  moType = TranslateThingType[type];
  thingCount = 0;

  CONS_Printf("looking for tid %d, type %d...", tid, type);
  int searcher = -1;
  if (tid)
    {
      // Count TID things
      Actor *mobj;
      while ((mobj = ACMap->FindFromTIDmap(tid, &searcher)) != NULL)
	{
	  if (type == 0)	    
	    thingCount++; // Just count TIDs
	  else if (mobj->Type() == Thinker::tt_dactor && moType == ((DActor *)mobj)->type)
	    {
	      if (mobj->flags & MF_COUNTKILL && mobj->flags & MF_CORPSE)
		continue; // Don't count dead monsters
	      thingCount++;
	    }
	}
    }
  else
    ACMap->IterateThinkers(IT_TypeCount); // Count only types

  CONS_Printf("found %d objects.\n", thingCount);
  Push(thingCount);
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

  for (i = 0; i < n; i++)
    ACMap->players[i]->SetMessage(PrintBuffer, true);

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

  S_StartSound(orig, S_GetSoundID(ACMap->ACStrings[Pop()]), volume/127.0);
  return SCRIPT_CONTINUE;
}

static int CmdThingSound()
{
  Actor *mobj;

  int volume = Pop();
  int sound = S_GetSoundID(ACMap->ACStrings[Pop()]);
  int tid = Pop();
  int searcher = -1;

  while((mobj = ACMap->FindFromTIDmap(tid, &searcher)) != NULL)
    {
      S_StartSound(mobj, sound, volume/127.0);
    }

  return SCRIPT_CONTINUE;
}

static int CmdAmbientSound()
{
  int volume = Pop();
  S_StartAmbSound(S_GetSoundID(ACMap->ACStrings[Pop()]), volume/127.0);
  return SCRIPT_CONTINUE;
}

static int CmdSoundSequence()
{
  mappoint_t *orig = NULL;

  if(ACScript->line)
    {
      orig = &ACScript->line->frontsector->soundorg;
    }
  ACMap->SN_StartSequenceName(orig, ACMap->ACStrings[Pop()]);
  return SCRIPT_CONTINUE;
}

static int CmdSetLineTexture()
{
  line_t *line;
  int lineTag;
  int side;
  int position;
  int texture;
  int s;

  texture = R_TextureNumForName(ACMap->ACStrings[Pop()]);
  position = Pop();
  side = Pop();
  lineTag = Pop();

  for (s = -1; (line = ACMap->FindLineFromTag(lineTag, &s)) != NULL; )
    {
      if (position == TEXTURE_MIDDLE)
	{
	  ACMap->sides[line->sidenum[side]].midtexture = texture;
	}
      else if (position == TEXTURE_BOTTOM)
	{
	  ACMap->sides[line->sidenum[side]].bottomtexture = texture;
	}
      else
	{ // TEXTURE_TOP
	  ACMap->sides[line->sidenum[side]].toptexture = texture;
	}
    }

  return SCRIPT_CONTINUE;
}

static int CmdSetLineBlocking()
{
  line_t *line;
  int lineTag;
  int blocking;
  int s;

  blocking = Pop() ? ML_BLOCKING : 0;
  lineTag = Pop();

  for (s = -1; (line = ACMap->FindLineFromTag(lineTag, &s)) != NULL; )
    {
      line->flags = (line->flags&~ML_BLOCKING) | blocking;
    }

  return SCRIPT_CONTINUE;
}

static int CmdSetLineSpecial()
{
  line_t *line;
  int lineTag;
  int special, arg1, arg2, arg3, arg4, arg5;
  int s;

  arg5 = Pop();
  arg4 = Pop();
  arg3 = Pop();
  arg2 = Pop();
  arg1 = Pop();
  special = Pop();
  lineTag = Pop();

  for (s = -1; (line = ACMap->FindLineFromTag(lineTag, &s)) != NULL; )
    {
      line->special = special;
      line->args[0] = arg1;
      line->args[1] = arg2;
      line->args[2] = arg3;
      line->args[3] = arg4;
      line->args[4] = arg5;
    }

  return SCRIPT_CONTINUE;
}
