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
// Revision 1.4  2003/03/15 20:07:17  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.3  2003/03/08 16:07:09  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.2  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.1.1.1  2002/11/16 14:18:04  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.11  2002/09/25 15:17:38  vberghol
// Intermission fixed?
//
// Revision 1.7  2002/08/21 16:58:34  vberghol
// Version 1.41 Experimental compiles and links!
//
// Revision 1.6  2002/08/08 18:36:25  vberghol
// p_spec.cpp fixed
//
// Revision 1.5  2002/08/08 12:01:29  vberghol
// pian engine on valmis!
//
// Revision 1.4  2002/07/23 19:21:44  vberghol
// fixed up to p_enemy.cpp
//
// Revision 1.3  2002/07/01 21:00:21  jpakkane
// Fixed cr+lf to UNIX form.
//
// Revision 1.2  2002/06/28 10:57:16  vberghol
// Version 133 Experimental!
//
// Revision 1.10  2001/04/18 21:00:22  metzgermeister
// fix crash bug
//
// Revision 1.9  2001/02/24 13:35:21  bpereira
// no message
//
// Revision 1.8  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.7  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.6  2000/09/28 20:57:17  bpereira
// no message
//
// Revision 1.5  2000/04/16 18:38:07  bpereira
// no message
//
// Revision 1.4  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// fix CR+LF problem
//
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------

#include <vector>

#include "doomdef.h"
#include "doomdata.h"
#include "command.h"
#include "g_game.h"
#include "g_actor.h"
#include "g_pawn.h"
#include "g_map.h"

#include "p_spec.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_main.h"
#include "r_data.h"
#include "r_state.h"

#include "t_script.h"

#include "w_wad.h"
#include "z_zone.h"

//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//
switchlist_t DoomSwitchList[] =
{
  // Doom shareware episode 1 switches
  {"SW1BRCOM","SW2BRCOM",     1},
  {"SW1BRN1", "SW2BRN1",      1},
  {"SW1BRN2", "SW2BRN2",      1},
  {"SW1BRNGN","SW2BRNGN",     1},
  {"SW1BROWN","SW2BROWN",     1},
  {"SW1COMM", "SW2COMM",      1},
  {"SW1COMP", "SW2COMP",      1},
  {"SW1DIRT", "SW2DIRT",      1},
  {"SW1EXIT", "SW2EXIT",      1},
  {"SW1GRAY", "SW2GRAY",      1},
  {"SW1GRAY1","SW2GRAY1",     1},
  {"SW1METAL","SW2METAL",     1},
  {"SW1PIPE", "SW2PIPE",      1},
  {"SW1SLAD", "SW2SLAD",      1},
  {"SW1STARG","SW2STARG",     1},
  {"SW1STON1","SW2STON1",     1},
  {"SW1STON2","SW2STON2",     1},
  {"SW1STONE","SW2STONE",     1},
  {"SW1STRTN","SW2STRTN",     1},

  // Doom registered episodes 2&3 switches
  {"SW1BLUE", "SW2BLUE",      2},
  {"SW1CMT",  "SW2CMT",       2},
  {"SW1GARG", "SW2GARG",      2},
  {"SW1GSTON","SW2GSTON",     2},
  {"SW1HOT",  "SW2HOT",       2},
  {"SW1LION", "SW2LION",      2},
  {"SW1SATYR","SW2SATYR",     2},
  {"SW1SKIN", "SW2SKIN",      2},
  {"SW1VINE", "SW2VINE",      2},
  {"SW1WOOD", "SW2WOOD",      2},

    // Doom II switches
  {"SW1PANEL","SW2PANEL",     3},
  {"SW1ROCK", "SW2ROCK",      3},
  {"SW1MET2", "SW2MET2",      3},
  {"SW1WDMET","SW2WDMET",     3},
  {"SW1BRIK", "SW2BRIK",      3},
  {"SW1MOD1", "SW2MOD1",      3},
  {"SW1ZIM",  "SW2ZIM",       3},
  {"SW1STON6","SW2STON6",     3},
  {"SW1TEK",  "SW2TEK",       3},
  {"SW1MARB", "SW2MARB",      3},
  {"SW1SKULL","SW2SKULL",     3},

  {"\0",      "\0",           0}
};

switchlist_t HereticSwitchList[] =
{
  // heretic
  {"SW1OFF",  "SW1ON",        4},
  {"SW2OFF",  "SW2ON",        4},

  {"\0",      "\0",           0}
};

// TODO Hexen switch sounds
switchlist_t HexenSwitchList[] =
{
  {"SW_1_UP",  "SW_1_DN", 1}, // SFX_SWITCH1},
  {"SW_2_UP",  "SW_2_DN", 1}, // SFX_SWITCH1},
  {"VALVE1",   "VALVE2",  1}, // SFX_VALVE_TURN},
  {"SW51_OFF", "SW51_ON", 1}, // SFX_SWITCH2},
  {"SW52_OFF", "SW52_ON", 1}, // SFX_SWITCH2},
  {"SW53_UP",  "SW53_DN", 1}, // SFX_ROPE_PULL},
  {"PUZZLE5",  "PUZZLE9", 1}, // SFX_SWITCH1},
  {"PUZZLE6",  "PUZZLE10", 1}, // SFX_SWITCH1},
  {"PUZZLE7",  "PUZZLE11", 1}, // SFX_SWITCH1},
  {"PUZZLE8",  "PUZZLE12", 1}, // SFX_SWITCH1},
  {"\0",       "\0", 0}
};

// list of switch texture numbers (on/off states), common to all maps
static vector<int> switchlist;

//
// P_InitSwitchList
// this is now called at GI::Startlevel()
//
void P_InitSwitchList()
{
  int i, nameset = 0;
  switchlist_t *sl = DoomSwitchList;

  switch (game.mode)
    {
    case gm_doom1:
    case gm_udoom:
      nameset = 2;
      break;
    case gm_doom2:
      nameset = 3;
      break;
    case gm_heretic:
      sl = HereticSwitchList;
      nameset = 4;
      break;
    case gm_hexen:
      sl = HexenSwitchList;
      nameset = 1;
      break;
    default:
      nameset = 1;
    }

  // Boom SWITCHES lump present?
  if (fc.FindNumForName("SWITCHES") != -1)
    {
      // sl is not needed anymore after this function, therefore not PU_STATIC
      sl = (switchlist_t *)fc.CacheLumpName("SWITCHES", PU_CACHE);
      // endian conversion only when loading from extra lump
      for (i=0; sl[i].episode != 0; i++)
        sl[i].episode = SHORT(sl[i].episode);
    }

  // initialization for artificial levels without switches (yes, they exist!)
  //if (NULL == switchlist) switchlist = (int *)malloc(sizeof(*switchlist));

  switchlist.clear();
  for (i=0; sl[i].episode != 0; i++)
    {
      if (sl[i].episode <= nameset)
	{
	  switchlist.push_back(R_TextureNumForName(sl[i].name1));
	  switchlist.push_back(R_TextureNumForName(sl[i].name2));
	}
    }

  //SoM: 3/22/2000: Don't change tag if not from lump
  //if (sl != oldalphSwitchList) Z_ChangeTag(alphSwitchList,PU_CACHE);
}

// button_t statics
int button_t::buttonsound = 0;

// was P_StartButton
// Start a button counting down till it turns off.
//
button_t::button_t(line_t *l, button_e w, int tex, int time)
{
  l->thinker = this;

  line = l;
  where = w;
  texture = tex;
  timer = time;
  soundorg = &line->frontsector->soundorg;
}

// timed switches
void button_t::Think()
{
  if (timer > 0)
    timer--;

  if (!timer)
    {
      switch (where)
	{
	case button_top:
	  mp->sides[line->sidenum[0]].toptexture = texture;
	  break;

	case button_middle:
	  mp->sides[line->sidenum[0]].midtexture = texture;
	  break;

	case button_bottom:
	  mp->sides[line->sidenum[0]].bottomtexture = texture;
	  break;
	default:
	  break;
	}
      S_StartSound(soundorg, buttonsound);
      line->thinker = NULL;
      mp->RemoveThinker(this);  // unlink and free
    }
}

// was P_ChangeSwitchTexture
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//
void Map::ChangeSwitchTexture(line_t *line, int useAgain)
{
  int     texTop, texMid, texBot;
  int     i, n = switchlist.size();
  button_e loc = button_none;

  if (!useAgain)
    line->special = 0;

  texTop = sides[line->sidenum[0]].toptexture;
  texMid = sides[line->sidenum[0]].midtexture;
  texBot = sides[line->sidenum[0]].bottomtexture;

  for (i = 0; i < n; i++)
    {
      if (switchlist[i] == texTop)
        {
	  sides[line->sidenum[0]].toptexture = switchlist[i^1];

	  loc = button_top;
	  break;
        }
      else if (switchlist[i] == texMid)
	{
	  sides[line->sidenum[0]].midtexture = switchlist[i^1];

	  loc = button_middle;
	  break;
	}
      else if (switchlist[i] == texBot)
	{
	  sides[line->sidenum[0]].bottomtexture = switchlist[i^1];

	  loc = button_bottom;
	  break;
	}
    }

  if (loc != button_none)
    {
      // FIXME sounds/gametype, see HereticPatchEngine
      int sound = sfx_swtchn;

      // EXIT SWITCH?
      if (line->special == 11)
	sound = sfx_swtchx;

      S_StartSound(&line->frontsector->soundorg, sound);

      // See if it is a button and not already pressed
      if (useAgain && line->thinker == NULL)
	{
	  button_t *but = new button_t(line, loc, switchlist[i], BUTTONTIME);
	  AddThinker(but);
	}
    }
}


//
// was P_UseSpecialLine
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
//
bool Map::UseSpecialLine(Actor *thing, line_t *line, int side)
{
  extern consvar_t   cv_allowexitlevel;

  // Err...
  // Use the back sides of VERY SPECIAL lines...
  if (side)
    return false;

  // is thing a PlayerPawn?
  bool p = (thing->Type() == Thinker::tt_ppawn);


  //SoM: 3/18/2000: Add check for Generalized linedefs.
  if (boomsupport)
    {
      // pointer to line function is NULL by default, set non-null if
      // line special is push or switch generalized linedef type
      int (Map::*linefunc)(line_t *line) = NULL;

      // check each range of generalized linedefs
      if ((unsigned)line->special >= GenFloorBase)
	{
	  if (!p)
	    if ((line->special & FloorChange) || !(line->special & FloorModel))
	      return false; // FloorModel is "Allow Monsters" if FloorChange is 0
	  if (!line->tag && ((line->special&6)!=6)) //all non-manual
	    return false;                           //generalized types require tag
	  linefunc = &Map::EV_DoGenFloor;
	}
      else if ((unsigned)line->special >= GenCeilingBase)
	{
	  if (!p)
	    if ((line->special & CeilingChange) || !(line->special & CeilingModel))
	      return false;   // CeilingModel is "Allow Monsters" if CeilingChange is 0
	  if (!line->tag && ((line->special&6)!=6)) //all non-manual
	    return false;                           //generalized types require tag
	  linefunc = &Map::EV_DoGenCeiling;
	}
      else if ((unsigned)line->special >= GenDoorBase)
	{
	  if (!p)
	    {
	      if (!(line->special & DoorMonster))
		return false;   // monsters disallowed from this door
	      if (line->flags & ML_SECRET) // they can't open secret doors either
		return false;
	    }
	  if (!line->tag && ((line->special & 6) != 6)) //all non-manual
	    return false;                           //generalized types require tag
	  linefunc = &Map::EV_DoGenDoor;
	}
      else if ((unsigned)line->special >= GenLockedBase)
	{
	  if (!p)
	    return false;   // monsters disallowed from unlocking doors
	  if (!((PlayerPawn *)thing)->CanUnlockGenDoor(line))
	    return false;
	  if (!line->tag && ((line->special&6)!=6)) //all non-manual
	    return false;                           //generalized types require tag

	  linefunc = &Map::EV_DoGenLockedDoor;
	}
      else if ((unsigned)line->special >= GenLiftBase)
	{
	  if (!p)
	    if (!(line->special & LiftMonster))
	      return false; // monsters disallowed
	  if (!line->tag && ((line->special&6)!=6)) //all non-manual
	    return false;                           //generalized types require tag
	  linefunc = &Map::EV_DoGenLift;
	}
      else if ((unsigned)line->special >= GenStairsBase)
	{
	  if (!p)
	    if (!(line->special & StairMonster))
	      return false; // monsters disallowed
	  if (!line->tag && ((line->special&6)!=6)) //all non-manual
	    return false;                           //generalized types require tag
	  linefunc = &Map::EV_DoGenStairs;
	}
      else if ((unsigned)line->special >= GenCrusherBase)
	{
	  if (!p)
	    if (!(line->special & CrusherMonster))
	      return false; // monsters disallowed
	  if (!line->tag && ((line->special&6)!=6)) //all non-manual
	    return false;                           //generalized types require tag
	  linefunc = &Map::EV_DoGenCrusher;
	}

      if (linefunc)
	switch ((line->special & TriggerType) >> TriggerTypeShift)
	  {
	  case PushOnce:
	    if ((this->*linefunc)(line))
	      line->special = 0;
	    return true;
	  case PushMany:
	    (this->*linefunc)(line);
	    return true;
	  case SwitchOnce:
	    if ((this->*linefunc)(line))
	      ChangeSwitchTexture(line,0);
	    return true;
	  case SwitchMany:
	    if ((this->*linefunc)(line))
	      ChangeSwitchTexture(line,1);
	    return true;
	  default:  // if not a switch/push type, do nothing here
	    return false;
	  }
    }

  // "standard" Doom linedefs

  // Switches that other things than players can activate.
  if (!p)
    {
      // never open secret doors
      if (line->flags & ML_SECRET)
	return false;

      switch (line->special)
        {
	case 1:       // MANUAL DOOR RAISE
	  // FIXME what's this? Monsters have no keys!
	  //case 32:      // MANUAL BLUE
	  //case 33:      // MANUAL RED
	  //case 34:      // MANUAL YELLOW
          //SoM: 3/18/2000: add ability to use teleporters for monsters
	case 195:       // switch teleporters
	case 174:
	case 210:       // silent switch teleporters
	case 209:
	  break;

	default:
	  return false;
	  break;
        }
    }

  if (!P_CheckTag(line) && boomsupport)  //disallow zero tag on some types
    return false;

  // do something
  switch (line->special)
    {
      // MANUALS
    case 1:           // Vertical Door
    case 26:          // Blue Door/Locked
    case 27:          // Yellow Door /Locked
    case 28:          // Red Door /Locked

    case 31:          // Manual door open
    case 32:          // Blue locked door open
    case 33:          // Red locked door open
    case 34:          // Yellow locked door open

    case 117:         // Blazing door raise
    case 118:         // Blazing door open
      EV_VerticalDoor(line, thing);
      break;

      //UNUSED - Door Slide Open&Close
      // case 124:
      // EV_SlidingDoor (line, thing);
      // break;

      // SWITCHES
    case 7:
      // Build Stairs
      if (EV_BuildStairs(line, stair_e(game.mode == gm_heretic ? 8*FRACUNIT : build8)))
	ChangeSwitchTexture(line,0);
      break;

    case 107:
      if( game.mode == gm_heretic )
        {
	  if (EV_BuildStairs (line, stair_e(16 * FRACUNIT)))
	    ChangeSwitchTexture (line, 0);
        }
      break;

    case 9:
      // Change Donut
      if (EV_DoDonut(line))
	ChangeSwitchTexture(line,0);
      break;

    case 11:
      // Exit level
      if(cv_allowexitlevel.value)
        {
	  ChangeSwitchTexture(line,0);
	  ExitMap(0);
        }
      break;

    case 14:
      // Raise Floor 32 and change texture
      if (EV_DoPlat(line,raiseAndChange,32))
	ChangeSwitchTexture(line,0);
      break;

    case 15:
      // Raise Floor 24 and change texture
      if (EV_DoPlat(line,raiseAndChange,24))
	ChangeSwitchTexture(line,0);
      break;

    case 18:
      // Raise Floor to next highest floor
      if (EV_DoFloor(line, raiseFloorToNearest))
	ChangeSwitchTexture(line,0);
      break;

    case 20:
      // Raise Plat next highest floor and change texture
      if (EV_DoPlat(line,raiseToNearestAndChange,0))
	ChangeSwitchTexture(line,0);
      break;

    case 21:
      // PlatDownWaitUpStay
      if (EV_DoPlat(line,downWaitUpStay,0))
	ChangeSwitchTexture(line,0);
      break;

    case 23:
      // Lower Floor to Lowest
      if (EV_DoFloor(line,lowerFloorToLowest))
	ChangeSwitchTexture(line,0);
      break;

    case 29:
      // Raise Door
      if (EV_DoDoor(line,normalDoor,VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 41:
      // Lower Ceiling to Floor
      if (EV_DoCeiling(line,lowerToFloor))
	ChangeSwitchTexture(line,0);
      break;

    case 71:
      // Turbo Lower Floor
      if (EV_DoFloor(line,turboLower))
	ChangeSwitchTexture(line,0);
      break;

    case 49:
      // Ceiling Crush And Raise
      if (EV_DoCeiling(line,game.mode==gm_heretic ? lowerAndCrush : crushAndRaise))
	ChangeSwitchTexture(line,0);
      break;

    case 50:
      // Close Door
      if (EV_DoDoor(line,doorclose,VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 51:
      // Secret EXIT
      if(cv_allowexitlevel.value)
        {
	  ChangeSwitchTexture(line,0);
	  ExitMap(1);
	}
      break;

    case 55:
      // Raise Floor Crush
      if (EV_DoFloor(line,raiseFloorCrush))
	ChangeSwitchTexture(line,0);
      break;

    case 101:
      // Raise Floor
      if (EV_DoFloor(line,raiseFloor))
	ChangeSwitchTexture(line,0);
      break;

    case 102:
      // Lower Floor to Surrounding floor height
      if (EV_DoFloor(line,lowerFloor))
	ChangeSwitchTexture(line,0);
      break;

    case 103:
      // Open Door
      if (EV_DoDoor(line,dooropen,VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 111:
      // Blazing Door Raise (faster than TURBO!)
      if (EV_DoDoor (line,blazeRaise,4*VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 112:
      // Blazing Door Open (faster than TURBO!)
      if (EV_DoDoor (line,blazeOpen,4*VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 113:
      // Blazing Door Close (faster than TURBO!)
      if (EV_DoDoor (line,blazeClose,4*VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 122:
      // Blazing PlatDownWaitUpStay
      if (EV_DoPlat(line,blazeDWUS,0))
	ChangeSwitchTexture(line,0);
      break;

    case 127:
      // Build Stairs Turbo 16
      if (EV_BuildStairs(line,turbo16))
	ChangeSwitchTexture(line,0);
      break;

    case 131:
      // Raise Floor Turbo
      if (EV_DoFloor(line,raiseFloorTurbo))
	ChangeSwitchTexture(line,0);
      break;

    case 133:
      // BlzOpenDoor BLUE
    case 135:
      // BlzOpenDoor RED
    case 137:
      // BlzOpenDoor YELLOW
      if (EV_DoLockedDoor (line,blazeOpen, (PlayerPawn *)thing,4*VDOORSPEED))
	ChangeSwitchTexture(line,0);
      break;

    case 140:
      // Raise Floor 512
      if (EV_DoFloor(line,raiseFloor512))
	ChangeSwitchTexture(line,0);
      break;

      //SoM: FraggleScript!
    case 276:
    case 277:
#ifdef FRAGGLESCRIPT
      t_trigger = thing;
      T_RunScript(line->tag);
#endif
      if(line->special == 277)
        {
          line->special = 0;         // clear tag
          ChangeSwitchTexture(line,0);
        }
      else
	ChangeSwitchTexture(line,1);
      break;

    default:
      if (boomsupport)
	switch (line->special)
          {
            // added linedef types to fill all functions out so that
            // all possess SR, S1, WR, W1 types

	  case 158:
	    // Raise Floor to shortest lower texture
	    if (EV_DoFloor(line,raiseToTexture))
	      ChangeSwitchTexture(line,0);
	    break;
  
	  case 159:
	    // Raise Floor to shortest lower texture
	    if (EV_DoFloor(line,lowerAndChange))
	      ChangeSwitchTexture(line,0);
	    break;
        
	  case 160:
	    // Raise Floor 24 and change
	    if (EV_DoFloor(line,raiseFloor24AndChange))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 161:
	    // Raise Floor 24
	    if (EV_DoFloor(line,raiseFloor24))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 162:
	    // Moving floor min n to max n
	    if (EV_DoPlat(line,perpetualRaise,0))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 163:
	    // Stop Moving floor
	    EV_StopPlat(line);
	    ChangeSwitchTexture(line,0);
	    break;

	  case 164:
	    // Start fast crusher
	    if (EV_DoCeiling(line,fastCrushAndRaise))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 165:
	    // Start slow silent crusher
	    if (EV_DoCeiling(line,silentCrushAndRaise))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 166:
	    // Raise ceiling, Lower floor
	    if (EV_DoCeiling(line, raiseToHighest) ||
		EV_DoFloor(line, lowerFloorToLowest))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 167:
	    // Lower floor and Crush
	    if (EV_DoCeiling(line, lowerAndCrush))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 168:
	    // Stop crusher
	    if (EV_CeilingCrushStop(line))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 169:
	    // Lights to brightest neighbor sector
	    EV_LightTurnOn(line,0);
	    ChangeSwitchTexture(line,0);
	    break;

	  case 170:
	    // Lights to near dark
	    EV_LightTurnOn(line,35);
	    ChangeSwitchTexture(line,0);
	    break;

	  case 171:
	    // Lights on full
	    EV_LightTurnOn(line,255);
	    ChangeSwitchTexture(line,0);
	    break;

	  case 172:
	    // Start Lights Strobing
	    EV_StartLightStrobing(line);
	    ChangeSwitchTexture(line,0);
	    break;

	  case 173:
	    // Lights to Dimmest Near
	    EV_TurnTagLightsOff(line);
	    ChangeSwitchTexture(line,0);
	    break;

	  case 174:
	    // Teleport
	    if (EV_Teleport(line,side,thing))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 175:
	    // Close Door, Open in 30 secs
	    if (EV_DoDoor(line,close30ThenOpen,VDOORSPEED))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 189: //create texture change no motion type
	    // Texture Change Only (Trigger)
	    if (EV_DoChange(line,trigChangeOnly))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 203:
	    // Lower ceiling to lowest surrounding ceiling
	    if (EV_DoCeiling(line,lowerToLowest))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 204:
	    // Lower ceiling to highest surrounding floor
	    if (EV_DoCeiling(line,lowerToMaxFloor))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 209:
	    // killough 1/31/98: silent teleporter
	    if (EV_SilentTeleport(line, side, thing))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 241: //jff 3/15/98 create texture change no motion type
	    // Texture Change Only (Numeric)
	    if (EV_DoChange(line,numChangeOnly))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 221:
	    // Lower floor to next lowest floor
	    if (EV_DoFloor(line,lowerFloorToNearest))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 229:
	    // Raise elevator next floor
	    if (EV_DoElevator(line,elevateUp))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 233:
	    // Lower elevator next floor
	    if (EV_DoElevator(line,elevateDown))
	      ChangeSwitchTexture(line,0);
	    break;

	  case 237:
	    // Elevator to current floor
	    if (EV_DoElevator(line,elevateCurrent))
	      ChangeSwitchTexture(line,0);
	    break;


            //end of added S1 linedef types

            //added linedef types to fill all functions out so that
            //all possess SR, S1, WR, W1 types
            
	  case 78:
	    // Texture/type Change Only (Numeric)
	    if (EV_DoChange(line,numChangeOnly))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 176:
	    // Raise Floor to shortest lower texture
	    if (EV_DoFloor(line,raiseToTexture))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 177:
	    // Raise Floor to shortest lower texture
	    if (EV_DoFloor(line,lowerAndChange))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 178:
	    // Raise Floor 512
	    if (EV_DoFloor(line,raiseFloor512))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 179:
	    // Raise Floor 24 and change
	    if (EV_DoFloor(line,raiseFloor24AndChange))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 180:
	    // Raise Floor 24
	    if (EV_DoFloor(line,raiseFloor24))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 181:
	    // Moving floor min n to max n
	    EV_DoPlat(line,perpetualRaise,0);
	    ChangeSwitchTexture(line,1);
	    break;

	  case 182:
	    // Stop Moving floor
	    EV_StopPlat(line);
	    ChangeSwitchTexture(line,1);
	    break;

	  case 183:
	    // Start fast crusher
	    if (EV_DoCeiling(line,fastCrushAndRaise))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 184:
	    // Start slow crusher
	    if (EV_DoCeiling(line,crushAndRaise))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 185:
	    // Start slow silent crusher
	    if (EV_DoCeiling(line,silentCrushAndRaise))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 186:
	    // Raise ceiling, Lower floor
	    if (EV_DoCeiling(line, raiseToHighest) ||
		EV_DoFloor(line, lowerFloorToLowest))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 187:
	    // Lower floor and Crush
	    if (EV_DoCeiling(line, lowerAndCrush))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 188:
	    // Stop crusher
	    if (EV_CeilingCrushStop(line))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 190: //jff 3/15/98 create texture change no motion type
	    // Texture Change Only (Trigger)
	    if (EV_DoChange(line,trigChangeOnly))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 191:
	    // Lower Pillar, Raise Donut
	    if (EV_DoDonut(line))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 192:
	    // Lights to brightest neighbor sector
	    EV_LightTurnOn(line,0);
	    ChangeSwitchTexture(line,1);
	    break;

	  case 193:
	    // Start Lights Strobing
	    EV_StartLightStrobing(line);
	    ChangeSwitchTexture(line,1);
	    break;

	  case 194:
	    // Lights to Dimmest Near
	    EV_TurnTagLightsOff(line);
	    ChangeSwitchTexture(line,1);
	    break;

	  case 195:
	    // Teleport
	    if (EV_Teleport(line,side,thing))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 196:
	    // Close Door, Open in 30 secs
	    if (EV_DoDoor(line,close30ThenOpen,VDOORSPEED))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 205:
	    // Lower ceiling to lowest surrounding ceiling
	    if (EV_DoCeiling(line,lowerToLowest))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 206:
	    // Lower ceiling to highest surrounding floor
	    if (EV_DoCeiling(line,lowerToMaxFloor))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 210:
	    // Silent teleporter
	    if (EV_SilentTeleport(line, side, thing))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 211:
	    // Toggle Floor Between C and F Instantly
	    if (EV_DoPlat(line,toggleUpDn,0))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 222:
	    // Lower floor to next lowest floor
	    if (EV_DoFloor(line,lowerFloorToNearest))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 230:
	    // Raise elevator next floor
	    if (EV_DoElevator(line,elevateUp))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 234:
	    // Lower elevator next floor
	    if (EV_DoElevator(line,elevateDown))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 238:
	    // Elevator to current floor
	    if (EV_DoElevator(line,elevateCurrent))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 258:
	    // Build stairs, step 8
	    if (EV_BuildStairs(line,build8))
	      ChangeSwitchTexture(line,1);
	    break;

	  case 259:
	    // Build stairs, step 16
	    if (EV_BuildStairs(line,turbo16))
	      ChangeSwitchTexture(line,1);
	    break;

            // end of added SR linedef types

          }
      break;


      // BUTTONS
    case 42:
      // Close Door
      if (EV_DoDoor(line,doorclose,VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 43:
      // Lower Ceiling to Floor
      if (EV_DoCeiling(line,lowerToFloor))
	ChangeSwitchTexture(line,1);
      break;

    case 45:
      // Lower Floor to Surrounding floor height
      if (EV_DoFloor(line,lowerFloor))
	ChangeSwitchTexture(line,1);
      break;

    case 60:
      // Lower Floor to Lowest
      if (EV_DoFloor(line,lowerFloorToLowest))
	ChangeSwitchTexture(line,1);
      break;

    case 61:
      // Open Door
      if (EV_DoDoor(line,dooropen,VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 62:
      // PlatDownWaitUpStay
      if (EV_DoPlat(line,downWaitUpStay,1))
	ChangeSwitchTexture(line,1);
      break;

    case 63:
      // Raise Door
      if (EV_DoDoor(line,normalDoor,VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 64:
      // Raise Floor to ceiling
      if (EV_DoFloor(line,raiseFloor))
	ChangeSwitchTexture(line,1);
      break;

    case 66:
      // Raise Floor 24 and change texture
      if (EV_DoPlat(line,raiseAndChange,24))
	ChangeSwitchTexture(line,1);
      break;

    case 67:
      // Raise Floor 32 and change texture
      if (EV_DoPlat(line,raiseAndChange,32))
	ChangeSwitchTexture(line,1);
      break;

    case 65:
      // Raise Floor Crush
      if (EV_DoFloor(line,raiseFloorCrush))
	ChangeSwitchTexture(line,1);
      break;

    case 68:
      // Raise Plat to next highest floor and change texture
      if (EV_DoPlat(line,raiseToNearestAndChange,0))
	ChangeSwitchTexture(line,1);
      break;

    case 69:
      // Raise Floor to next highest floor
      if (EV_DoFloor(line, raiseFloorToNearest))
	ChangeSwitchTexture(line,1);
      break;

    case 70:
      // Turbo Lower Floor
      if (EV_DoFloor(line,turboLower))
	ChangeSwitchTexture(line,1);
      break;

    case 114:
      // Blazing Door Raise (faster than TURBO!)
      if (EV_DoDoor (line,blazeRaise,4*VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 115:
      // Blazing Door Open (faster than TURBO!)
      if (EV_DoDoor (line,blazeOpen,4*VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 116:
      // Blazing Door Close (faster than TURBO!)
      if (EV_DoDoor (line,blazeClose,4*VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 123:
      // Blazing PlatDownWaitUpStay
      if (EV_DoPlat(line,blazeDWUS,0))
	ChangeSwitchTexture(line,1);
      break;

    case 132:
      // Raise Floor Turbo
      if (EV_DoFloor(line,raiseFloorTurbo))
	ChangeSwitchTexture(line,1);
      break;

    case 99:
      if( game.mode == gm_heretic ) // used for right scrolling texture
	break;
      // BlzOpenDoor BLUE
    case 134:
      // BlzOpenDoor RED
    case 136:
      // BlzOpenDoor YELLOW
      if (EV_DoLockedDoor(line,blazeOpen,(PlayerPawn *)thing,4*VDOORSPEED))
	ChangeSwitchTexture(line,1);
      break;

    case 138:
      // Light Turn On
      EV_LightTurnOn(line,255);
      ChangeSwitchTexture(line,1);
      break;

    case 139:
      // Light Turn Off
      EV_LightTurnOn(line,35);
      ChangeSwitchTexture(line,1);
      break;

    }

  return true;
}
