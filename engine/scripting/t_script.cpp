// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id$
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2001-2003 Doom Legacy Team
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
//
//--------------------------------------------------------------------------
//
// scripting.
//
// delayed scripts, running scripts, console cmds etc in here
// the interface between FraggleScript and the rest of the game
//
// By Simon Howard
//
//----------------------------------------------------------------------------


#include "doomdata.h"
#include "command.h"
#include "g_game.h"
#include "g_map.h"
#include "g_mapinfo.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_player.h"

#include "r_state.h"
#include "p_spec.h"
#include "p_setup.h"
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
//           hubscript                 thingscript
//          /         \                  /     \.
//    levelscript    [levelscript]    ... scripts ...
//     /      \          /      \.
//  ... scripts...   ... scripts ...
//

// the level script is just the stuff put in the wad,
// which the other scripts are derivatives of

// the thing script
//script_t thingscript;


Actor *t_trigger;

runningscript_t *freelist = NULL; // maintain a freelist for speed

runningscript_t *new_runningscript()
{
  // check the freelist
  if(freelist)
    {
      runningscript_t *returnv=freelist;
      freelist = freelist->next;
      return returnv;
    }
  
  // alloc static: can be used in other levels too
  return (runningscript_t *)Z_Malloc(sizeof(runningscript_t), PU_STATIC, 0);
}



static void free_runningscript(runningscript_t *runscr)
{
  // add to freelist
  runscr->next = freelist;
  freelist = runscr;
}


void init_functions();

//     T_Init()
//
//    called at program start

void T_Init()
{
  init_variables();
  init_functions();
}

//
// T_ClearScripts()
//
// called at level start, clears all scripts
//

void Map::T_ClearScripts()
{
  int i;

  current_map = this; // just in case
  
  // stop runningscripts
  {
    runningscript_t *runscr, *next;
    runscr = runningscripts;
  
    // free the whole chain
    while (runscr)
      {
	next = runscr->next;
	free_runningscript(runscr);
	runscr = next;
      }
    runningscripts = NULL;
  }

  if (!levelscript)
    levelscript = (script_t *)Z_Malloc(sizeof(script_t), PU_LEVEL, NULL);
  else if (levelscript->data)
    Z_Free(levelscript->data);

  // clear the levelscript
  levelscript->mp = this;
  levelscript->data = NULL;
  
  levelscript->scriptnum = -1;
  levelscript->parent = &hub_script;

  // clear levelscript variables
  for (i=0; i<VARIABLESLOTS; i++)
    levelscript->variables[i] = NULL;
}

void T_LoadThingScript()
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



void Map::T_PreprocessScripts()
{
  // run the levelscript first
  // get the other scripts

  // levelscript started by first player (not necessarily player 0) 'superplayer'
  levelscript->player = NULL;  // FIXME who is superplayer?
  levelscript->trigger = NULL; // superplayer->pawn;

  preprocess(levelscript);
  run_script(levelscript);

  // load and run the thing script
  T_LoadThingScript();
}



void Map::T_RunScript(int n)
{
  if (n < 0 || n >= MAXSCRIPTS)
    return;

  // use the level's child script script n
  script_t *script = levelscript->children[n];
  if(!script) return;
 
  script->trigger = t_trigger;    // save trigger in script
  
  run_script(script);
}



// T_RunThingScript:
// identical to T_RunScript but runs a script
// from the thingscript list rather than the
// levelscript list

void T_RunThingScript(int n)
{
/*  script_t *script;
  
  if(n<0 || n>=MAXSCRIPTS) return;

  // use the level's child script script n
  script = thingscript.children[n];
  if(!script) return;
 
  script->trigger = t_trigger;    // save trigger in script
  
  run_script(script);*/
}



// console scripting debugging commands

void COM_T_DumpScript_f()
{
  script_t *script;
  
  if (COM_Argc() < 2)
    {
      CONS_Printf("usage: T_DumpScript <scriptnum>\n");
      return;
    }

  if (!strcmp(COM_Argv(1), "global"))
    script = current_map->levelscript;
  else
    script = current_map->levelscript->children[atoi(COM_Argv(1))];
  
  if (!script)
    {
      CONS_Printf("script '%s' not defined.\n", COM_Argv(1));
      return;
    }
  
  CONS_Printf("%s\n", script->data);
}



void COM_T_RunScript_f()
{
  int sn;
  
  if (COM_Argc() < 2)
    {
      CONS_Printf("Usage: T_RunScript <script>\n");
      return;
    }
  
  sn = atoi(COM_Argv(1));
  
  if (!current_map->levelscript->children[sn])
    {
      CONS_Printf("script not defined\n");
      return;
    }
  t_trigger = consoleplayer->pawn;
  
  current_map->T_RunScript(sn);
}



/************************
         PAUSING SCRIPTS
 ************************/


bool Map::T_wait_finished(runningscript_t *script)
{
  switch(script->wait_type)
    {
    case wt_none: return true;        // uh? hehe
    case wt_scriptwait:               // waiting for script to finish
      {
	runningscript_t *current;
	for(current = runningscripts; current; current = current->next)
	  {
	    if(current == script) continue;  // ignore this script
	    if(current->script->scriptnum == script->wait_data)
	      return false;        // script still running
	  }
	return true;        // can continue now
      }

    case wt_delay:                          // just count down
      {
	return --script->wait_data <= 0;
      }
    
    case wt_tagwait:
      {
	int secnum = -1;

	while ((secnum = FindSectorFromTag(script->wait_data, secnum)) >= 0)
	  {
	    sector_t *sec = &sectors[secnum];
	    if(sec->floordata || sec->ceilingdata || sec->lightingdata)
	      return false;        // not finished
	  }
	return true;
      }

    default: return true;
    }

  return false;
}




void Map::T_DelayedScripts()
{
  runningscript_t *current, *next;
  int i;

  if (!info->scripts)
    return; // no level scripts
  
  current = runningscripts;
  
  while (current)
    {
      if (T_wait_finished(current))
	{
	  // copy out the script variables from the
	  // runningscript_t

	  for(i=0; i<VARIABLESLOTS; i++)
	    current->script->variables[i] = current->variables[i];
	  current->script->trigger = current->trigger; // copy trigger
	  
	  // continue the script
	  continue_script(current->script, current->savepoint);
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
	  free_runningscript(current);
	}
      else
	next = current->next;


      current = next;   // continue to next in chain
    }
                
}

void Map::T_AddRunningScript(runningscript_t *s)
{
  // hook into chain at start
  s->next = runningscripts;
  s->prev = NULL;
  if (s->next)
    s->next->prev = s;
}

runningscript_t *Map::T_SaveCurrentScript()
{
  int i;

  runningscript_t *runscr = new_runningscript();
  runscr->script = current_script;
  runscr->savepoint = rover;

  // leave to other functions to set wait_type: default to wt_none
  runscr->wait_type = wt_none;

  // hook into chain at start
  T_AddRunningScript(runscr);
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = current_script->variables[i];
      
      // remove all the variables from the script variable list
      // to prevent them being removed when the script stops

      while(current_script->variables[i] &&
	    current_script->variables[i]->type != svt_label)
	current_script->variables[i] =
	  current_script->variables[i]->next;
    }
  runscr->trigger = current_script->trigger;      // save trigger
  
  killscript = true;      // stop the script

  return runscr;
}




// script function
void SF_Wait()
{
  runningscript_t *runscr;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = current_map->T_SaveCurrentScript();

  runscr->wait_type = wt_delay;
  runscr->wait_data = (intvalue(t_argv[0]) * 35) / 100;
}



// wait for sector with particular tag to stop moving
void SF_TagWait()
{
  runningscript_t *runscr;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = current_map->T_SaveCurrentScript();

  runscr->wait_type = wt_tagwait;
  runscr->wait_data = intvalue(t_argv[0]);
}




// wait for a script to finish
void SF_ScriptWait()
{
  runningscript_t *runscr;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = current_map->T_SaveCurrentScript();

  runscr->wait_type = wt_scriptwait;
  runscr->wait_data = intvalue(t_argv[0]);
}




extern Actor *trigger_obj;           // in t_func.c

void SF_StartScript()
{
  runningscript_t *runscr;
  int i, snum;
  
  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  snum = intvalue(t_argv[0]);
  
  script_t *script = current_map->levelscript->children[snum];
  
  if (!script)
    {
      script_error("script %i not defined\n", snum);
    }
  
  runscr = new_runningscript();
  runscr->script = script;
  runscr->savepoint = script->data; // start at beginning
  runscr->wait_type = wt_none;      // start straight away

  // hook into chain at start
  current_map->T_AddRunningScript(runscr);
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = script->variables[i];
      
      // in case we are starting another current_script:
      // remove all the variables from the script variable list
      // we only start with the basic labels
      while(runscr->variables[i] &&
	    runscr->variables[i]->type != svt_label)
	runscr->variables[i] =
	  runscr->variables[i]->next;
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




// running scripts

void COM_T_Running_f()
{
  runningscript_t *current = current_map->runningscripts;
  
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



/*********************
            ADD SCRIPT
 *********************/

// when the level is first loaded, all the
// scripts are simply stored in the levelscript.
// before the level starts, this script is
// preprocessed and run like any other. This allows
// the individual scripts to be derived from the
// levelscript. When the interpreter detects the
// 'script' keyword this function is called

void spec_script()
{
  int scriptnum;
  int datasize;
  script_t *script;

  if(!current_section)
    {
      script_error("need seperators for script\n");
      return;
    }
  
  // presume that the first token is "script"
  
  if(num_tokens < 2)
    {
      script_error("need script number\n");
      return;
    }

  scriptnum = intvalue(evaluate_expression(1, num_tokens-1));
  
  if(scriptnum < 0)
    {
      script_error("invalid script number\n");
      return;
    }

  script = (script_t *)Z_Malloc(sizeof(script_t), PU_LEVEL, 0);

  // add to scripts list of parent
  current_script->children[scriptnum] = script;
  
  // copy script data
  // workout script size: -2 to ignore { and }
  datasize = current_section->end - current_section->start - 2;

  // alloc extra 10 for safety
  script->data = (char *)Z_Malloc(datasize+10, PU_LEVEL, 0);
 
  // copy from parent script (levelscript) 
  // ignore first char which is {
  memcpy(script->data, current_section->start+1, datasize);

  // tack on a 0 to end the string
  script->data[datasize] = '\0';
  
  script->scriptnum = scriptnum;
  script->parent = current_script; // remember parent
  
  // preprocess the script now
  preprocess(script);
    
  // restore current_script: usefully stored in new script
  current_script = script->parent;

  // rover may also be changed, but is changed below anyway
  
  // we dont want to run the script, only add it
  // jump past the script in parsing
  
  rover = current_section->end + 1;
}


// console commands
void T_AddCommands()
{
  COM_AddCommand("t_dumpscript",  COM_T_DumpScript_f);
  COM_AddCommand("t_runscript",   COM_T_RunScript_f);
  COM_AddCommand("t_running",     COM_T_Running_f);
}
