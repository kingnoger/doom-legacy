// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2004 Doom Legacy Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log$
// Revision 1.10  2004/08/12 18:30:28  smite-meister
// cleaned startup
//
// Revision 1.9  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.8  2004/01/05 11:48:08  smite-meister
// 7 bugfixes
//
// Revision 1.7  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.6  2003/11/23 19:07:42  smite-meister
// New startup order
//
// Revision 1.5  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.4  2003/11/12 11:07:26  smite-meister
// Serialization done. Map progression.
//
// Revision 1.3  2003/05/11 21:23:52  smite-meister
// Hexen fixes
//
// Revision 1.2  2003/02/23 22:49:31  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.1.1.1  2002/11/16 14:18:19  hurdler
// Initial C++ version of Doom Legacy
//
//--------------------------------------------------------------------------

/// \file
/// \brief Delayed scripts, running scripts, console cmds etc.
/// The interface between FraggleScript and the rest of the game.

#include "doomdata.h"
#include "command.h"
#include "g_game.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h"

#include "p_spec.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_script.h"
#include "t_parse.h"
#include "t_vari.h"
#include "t_func.h"

//                  script tree:
//
//                     global_script
//                  /                 \.
//           hub_script               thingscript
//          /         \                  /     \.
//    levelscript    [levelscript]    ... scripts ...
//     /      \          /      \.
//  ... scripts...   ... scripts ...
//

// the global script just holds all the global variables and functions
script_t global_script;

// the hub script holds all the variables shared between levels in a hub
script_t hub_script;

// the level script is just the stuff put in the wad,
// which the other scripts are derivatives of

// the thing script
//script_t thingscript;

void FS_init_functions();



//==========================================================
//                Running (paused) scripts
//==========================================================

runningscript_t *runningscript_t::freelist = NULL;


void *runningscript_t::operator new(size_t size)
{
  // check the freelist first
  if (freelist)
    {
      runningscript_t *r = freelist;
      freelist = freelist->next;
      return r;
    }
  
  // static allocation: can be used in other levels too
  return Z_Malloc(size, PU_STATIC, NULL);
}


void runningscript_t::operator delete(void *mem)
{
  // add to freelist
  runningscript_t *r = (runningscript_t *)mem;
  r->next = freelist;
  freelist = r;
}



// stop and free runningscripts
void Map::FS_ClearRunningScripts()
{
  runningscript_t *r = runningscripts;
  
  // free the whole chain
  while (r)
    {
      runningscript_t *next = r->next;
      delete r;
      r = next;
    }
  runningscripts = NULL;
}


void Map::FS_AddRunningScript(runningscript_t *r)
{
  // hook into chain at start
  r->next = runningscripts;
  r->prev = NULL;
  runningscripts = r;
  if (r->next)
    r->next->prev = r;
}



bool Map::FS_wait_finished(runningscript_t *s)
{
  switch (s->wait_type)
    {
    case wt_delay:  // just count down
      return --s->wait_data <= 0;

    case wt_scriptwait:  // waiting for another script to finish
      {
	runningscript_t *current;
	for (current = runningscripts; current; current = current->next)
	  {
	    if (current == s)
	      continue;      // ignore this script
	    if (current->script->scriptnum == s->wait_data)
	      return false;  // script still running
	  }
	return true;         // can continue now
      }
    
    case wt_tagwait:
      {
	int secnum = -1;
	while ((secnum = FindSectorFromTag(s->wait_data, secnum)) >= 0)
	  {
	    sector_t *sec = &sectors[secnum];
	    if (sec->floordata || sec->ceilingdata || sec->lightingdata)
	      return false;  // not finished
	  }
	return true;
      }

    default:
      return true;
    }

  return false;
}



void Map::FS_DelayedScripts()
{
  if (!info->scripts)
    return; // no level scripts

  current_map = this; // this must always be set before we start processing FS scripts...

  runningscript_t *current = runningscripts;
  
  while (current)
    {
      runningscript_t *next;
      if (FS_wait_finished(current))
	{
	  // copy out the script variables from the
	  // runningscript_t

	  for(int i=0; i<VARIABLESLOTS; i++)
	    current->script->variables[i] = current->variables[i];
	  current->script->trigger = current->trigger; // copy trigger
	  
	  // continue the script
	  current->script->continue_script(current->savepoint);
	  // TODO? too bad we can't just keep this runningscript instance
	  // if the script continues, but have to free this and create another

	  // unhook from chain and free
	  if (current->prev)
	    current->prev->next = current->next;
	  else
	    runningscripts = current->next;  // this was the first script in list

	  if (current->next)
	    current->next->prev = current->prev;

	  next = current->next;   // save before freeing
	  delete current;
	}
      else
	next = current->next;

      current = next;   // continue to next in chain
    }
}


// halts the script for a while, saving its data into a "runningscript" struct
void script_t::save(char *r, int wt, int wdata)
{
  runningscript_t *runscr = new runningscript_t;

  runscr->script = this;
  runscr->savepoint = r;
  runscr->wait_type = fs_wait_e(wt);
  runscr->wait_data = wdata;

  // save the script variables 
  for(int i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = variables[i];
      
      // remove all the variables from the script variable list
      // to prevent them being removed when the script stops

      while (variables[i] && variables[i]->type != svt_label)
	variables[i] = variables[i]->next;
    }
  runscr->trigger = trigger;      // save trigger

  // hook into chain at start
  current_map->FS_AddRunningScript(runscr);
  
  killscript = true;      // stop the script
}




//==========================================================


/*
void FS_ClearHubScript()
{
  for (int i=0; i<VARIABLESLOTS; i++)
    {
      while (hub_script.variables[i])
	{
	  svariable_t *next = hub_script.variables[i]->next;
	  if (hub_script.variables[i]->type == svt_string)
	    Z_Free(hub_script.variables[i]->value.s);
	  Z_Free(hub_script.variables[i]);
	  hub_script.variables[i] = next;
	}
    }
}
*/



// called at program start
void FS_Init()
{
  // initialise the global script and hubscript: clear all the variables
  global_script.parent = NULL;        // globalscript is the root script
  hub_script.parent = &global_script; // hub_script is the next level down
  
  for (int i=0; i<VARIABLESLOTS; i++)
    global_script.variables[i] = hub_script.variables[i] = NULL;
  
  // any hardcoded global variables can be added here

  FS_init_functions();
}



// called at level start, clears all scripts
void Map::FS_ClearScripts()
{
  script_camera_on = false;
  FS_ClearRunningScripts(); // TODO better to do this when map ends!

  if (!levelscript)
    levelscript = (script_t *)Z_Malloc(sizeof(script_t), PU_LEVEL, NULL);
  else if (levelscript->data)
    Z_Free(levelscript->data);

  levelscript->data = NULL;
}

void FS_LoadThingScript()
{
/*  char *scriptlump;
  int lumpnum, lumplen;
  
  if(thingscript.data)
    Z_Free(thingscript.data);

  // load lump into thingscript.data

  // get lumpnum, lumplen
  
  lumpnum = W_CheckNumForName("THINGSCR");
  if(lumpnum == -1)
    return;
  
  lumplen = W_LumpLength(lumpnum);

  // alloc space for lump and copy lump data into it
  
  thingscript.data = Z_Malloc(lumplen+10, PU_STATIC, 0);
  scriptlump = W_CacheLumpNum(lumpnum, PU_CACHE);

  memcpy(thingscript.data, scriptlump, lumplen);

  // add '\0' to end of string

  thingscript.data[lumplen] = '\0';

  // preprocess script

  preprocess(&thingscript);

  // run script

  thingscript.trigger = game.players[0]->pawn;
  run_script(&thingscript);  */
}



void Map::FS_PreprocessScripts()
{
  current_map = this; // this must always be set before we start processing FS scripts...

  // clear the levelscript
  // levelscript started by first player (not necessarily player 0) 'superplayer'
  //  levelscript->player = NULL;  // FIXME who is superplayer?
  levelscript->trigger = NULL; // superplayer->pawn;

  levelscript->scriptnum = -1;
  levelscript->parent = &hub_script;

  // run the levelscript first
  // get the other scripts
  if (levelscript->data)
    {
      levelscript->preprocess();
      levelscript->run();
    }

  // load and run the thing script
  //FS_LoadThingScript();
}



bool Map::FS_RunScript(int n, Actor *trig)
{
  if (n < 0 || n >= MAXSCRIPTS)
    return false;

  // use the level's child script script n
  script_t *script = levelscript->children[n];
  if (!script)
    return false;
 
  current_map = this; // this must always be set before we start processing FS scripts...

  script->trigger = trig;    // save trigger in script
  
  script->run();
  return true;
}



// identical to FS_RunScript but runs a script
// from the thingscript list rather than the
// levelscript list
void FS_RunThingScript(int n)
{
/*  script_t *script;
  
  if(n<0 || n>=MAXSCRIPTS) return;

  // use the level's child script script n
  script = thingscript.children[n];
  if(!script) return;
 
  script->trigger = ;    // save trigger in script
  
  script->run();
*/
}


//===================================================================
// console scripting debugging commands
//===================================================================

void COM_FS_DumpScript_f()
{
  script_t *script;
  
  if (COM_Argc() < 2)
    {
      CONS_Printf("usage: FS_DumpScript <scriptnum>\n");
      return;
    }

  if (!consoleplayer || !consoleplayer->mp)
    return;

  if (!strcmp(COM_Argv(1), "level"))
    script = consoleplayer->mp->levelscript;
  else
    script = consoleplayer->mp->levelscript->children[atoi(COM_Argv(1))];
  
  if (!script)
    {
      CONS_Printf("script '%s' not defined.\n", COM_Argv(1));
      return;
    }
  
  CONS_Printf("%s\n", script->data);
}



void COM_FS_RunScript_f()
{
  if (COM_Argc() < 2)
    {
      CONS_Printf("Usage: FS_RunScript <script>\n");
      return;
    }

  if (!consoleplayer || !consoleplayer->mp)
    return;

  int sn = atoi(COM_Argv(1));
  
  if (!consoleplayer->mp->levelscript->children[sn])
    {
      CONS_Printf("script not defined\n");
      return;
    }

  consoleplayer->mp->FS_RunScript(sn, consoleplayer->pawn);
}



// running scripts
void COM_FS_Running_f()
{
  if (!consoleplayer || !consoleplayer->mp)
    return;

  runningscript_t *current = consoleplayer->mp->runningscripts;
  
  CONS_Printf("Running scripts:\n");
  
  if (!current)
    CONS_Printf("none\n");
  
  while (current)
    {
      CONS_Printf("%i:", current->script->scriptnum);
      switch (current->wait_type)
	{
	case wt_none:
	  CONS_Printf("waiting for nothing?\n");
	  break;
	case wt_delay:
	  CONS_Printf("delay %i tics\n", current->wait_data);
	  break;
	case wt_tagwait:
	  CONS_Printf("waiting for tag %i\n", current->wait_data);
	  break;
	case wt_scriptwait:
	  CONS_Printf("waiting for script %i\n", current->wait_data);
	  break;
	default:
	  CONS_Printf("unknown wait type \n");
	  break;
	}
      current = current->next;
    }
}




void SF_StartScript()
{
  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  int snum = intvalue(t_argv[0]);
  
  script_t *script = current_map->levelscript->children[snum];
  
  if (!script)
    {
      script_error("script %i not defined\n", snum);
    }
  
  runningscript_t *runscr = new runningscript_t;
  runscr->script = script;
  runscr->savepoint = script->data; // start at beginning
  runscr->wait_type = wt_none;      // start straight away

  // hook into chain at start
  current_map->FS_AddRunningScript(runscr);
  
  // save the script variables 
  for(int i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = script->variables[i];
      
      // in case we are starting another current_script (another instance of the same script):
      // remove all the variables from the script variable list
      // we only start with the basic labels
      while(runscr->variables[i] && runscr->variables[i]->type != svt_label)
	runscr->variables[i] = runscr->variables[i]->next;
    }
  // copy trigger
  runscr->trigger = current_script->trigger;
}




void SF_ScriptRunning()
{
  runningscript_t *current;

  if (t_argc < 1)
    {
      script_error("not enough arguments to function\n");
      return;
    }

  int snum = intvalue(t_argv[0]);
  
  for (current = current_map->runningscripts; current; current = current->next)
    {
      if (current->script->scriptnum == snum)
	{
	  // script found so return
	  t_return.type = svt_int;
	  t_return.value.i = 1;
	  return;
	}
    }

  // script not found
  t_return.type = svt_int;
  t_return.value.i = 0;
}


// this is here just for convenience
Actor *MobjForSvalue(svalue_t svalue)
{
  if (svalue.type == svt_actor)
    return svalue.value.mobj;
  
  // this requires some creativity. We use the intvalue
  // as the thing number of a thing in the level.
  
  int intval = intvalue(svalue);        
  
  if (intval < 0 || intval >= current_map->nummapthings)
    {
      script_error("no mapthing %i\n", intval);
      return NULL;
    }

  Actor *p = current_map->mapthings[intval].mobj;

  if (!p)
    script_error("mapthing %i has no Actor\n", intval);

  return p;
}
