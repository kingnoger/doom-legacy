// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Portions Copyright(C) 2000 Simon Howard
// Copyright (C) 2002-2003 by Doom Legacy Team
// Thanks to Randy Heit for ZDoom ideas
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
// Revision 1.7  2003/12/06 23:57:47  smite-meister
// save-related bugfixes
//
// Revision 1.6  2003/12/03 10:49:49  smite-meister
// Save/load bugfix, text strings updated
//
// Revision 1.5  2003/11/30 00:09:43  smite-meister
// bugfixes
//
// Revision 1.4  2003/11/23 19:54:10  hurdler
// Remove warning and error at compile time
//
// Revision 1.3  2003/11/23 19:07:41  smite-meister
// New startup order
//
// Revision 1.2  2003/11/23 00:41:55  smite-meister
// bugfixes
//
// Revision 1.1  2003/11/12 11:07:17  smite-meister
// Serialization done. Map progression.
//
// Revision 1.15  2003/06/10 22:39:56  smite-meister
// Bugfixes
//
// Revision 1.14  2003/06/08 16:19:21  smite-meister
// Hexen lights.
//
// Revision 1.13  2003/06/01 18:56:30  smite-meister
// zlib compression, partial polyobj fix
//
// Revision 1.12  2003/05/11 21:23:50  smite-meister
// Hexen fixes
//
// Revision 1.11  2003/04/24 20:58:38  hurdler
// Remove lots of compiling warnings
//
// Revision 1.10  2003/04/24 20:30:07  hurdler
// Remove lots of compiling warnings
//
// Revision 1.9  2003/04/20 16:45:50  smite-meister
// partial SNDSEQ fix
//
// Revision 1.8  2003/04/04 00:01:56  smite-meister
// bugfixes, Hexen HUD
//
// Revision 1.7  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.6  2003/03/08 16:07:08  smite-meister
// Lots of stuff. Sprite cache. Movement+friction fix.
//
// Revision 1.5  2003/02/23 22:49:30  smite-meister
// FS is back! L2 cache works.
//
// Revision 1.4  2003/01/12 12:56:40  smite-meister
// Texture bug finally fixed! Pickup, chasecam and sw renderer bugs fixed.
//
// Revision 1.3  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.2  2002/12/23 23:15:41  smite-meister
// Weapon groups, MAPINFO parser added!
//
// Revision 1.1.1.1  2002/11/16 14:17:59  hurdler
// Initial C++ version of Doom Legacy
//
// DESCRIPTION:
//   Implementation of MapInfo class.
//   MapInfo lump parser. Hexen/ZDoom MAPINFO lump parser.
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#ifdef LINUX
# include <unistd.h>
#endif

#include "doomdef.h"
#include "g_mapinfo.h"
#include "g_level.h"

#include "command.h"

#include "dstrings.h"
#include "sounds.h"

#include "g_game.h"
#include "g_map.h"
#include "g_player.h"
#include "g_pawn.h"

#include "m_misc.h"
#include "m_archive.h"

#include "t_script.h"
#include "t_parse.h"

#include "w_wad.h"
#include "z_zone.h"


//=================================
// MapInfo class methods

// default constructor
MapInfo::MapInfo()
{
  state = MAP_UNLOADED;
  me = NULL;
  nicename = "Unnamed Map";

  mapnumber = 0;
  cluster = 100; // this is so that maps with no cluster are placed in a cluster of their own
  scripts = 0;
  partime = 0;
  gravity = 1.0f;

  // mapthing doomednum offsets
  doom_offs[0] = doom_offs[1] = 0;
  heretic_offs[0] = heretic_offs[1] = 0;
  hexen_offs[0] = hexen_offs[1] = 0;

  warptrans = warpnext = nextlevel = secretlevel = -1;
  //nextmaplump[0] = secretmaplump[0] = '\0';

  doublesky = lightning = false;
  sky1 = "SKY1";
  sky1sp = sky2sp = 0;

  cdtrack = 1;
  BossDeathKey = 0;
}


// destructor
MapInfo::~MapInfo()
{
  Close();
}


// ticks the map forward, 
void MapInfo::Ticker(bool hub)
{
  if (state == MAP_RUNNING || state == MAP_FINISHED)
    {
      me->Ticker();

      if (state == MAP_FINISHED)
	{
	  // check if it's time to stop
	  bool players_remaining = false;
	  int i, n = me->players.size();
	  for (i=0; i<n; i++)
	    if (me->players[i]->playerstate != PST_DONE &&
		me->players[i]->playerstate != PST_SPECTATOR)
	      players_remaining = true;

	  if (!players_remaining)
	    {
	      // TODO spectators have no destinations set, they should perhaps follow the last exiting player...
	      KickPlayers(0, 0, false);

	      if (hub)
		HubSave();
	      else
		Close();
	    }
	}
    }
}


// if this returns true, "me" must be valid
bool MapInfo::Activate()
{
  const char *temp;

  CONS_Printf("Activating map %d\n", mapnumber);

  switch (state)
    {
    case MAP_UNLOADED:
      temp = lumpname.c_str();
      if (fc.FindNumForName(temp) == -1)
	{
	  // FIXME! this entire block
	  //has the name got a dot (.) in it?
	  //if (!FIL_CheckExtension(temp))
	    // append .wad to the name
	    // try to load the file
	    

	  CONS_Printf("\2Map '%s' not found\n"
		      "(use .wad extension for external maps)\n", temp);
	  return false;
	}

      me = new Map(this);
      if (me->Setup(gametic))
	state = MAP_RUNNING;
      break;

    case MAP_RUNNING:
    case MAP_FINISHED:
      break;

    case MAP_INSTASIS:
      state = MAP_RUNNING;
      break;

    case MAP_SAVED:
      return HubLoad();

    }
  return true;
}


// throws out all the players, possibly resetting them also
int MapInfo::KickPlayers(int next, int ep, bool reset)
{
  if (next == 0)
    next = nextlevel; // zero means "normal exit"

  if (next == 100)
    next = secretlevel; // 100 means "secret exit"

  // kick out the players
  int i, n = me->players.size();
  for (i=0; i<n; i++)
    {
      me->players[i]->ExitLevel(next, ep, reset);
      me->players[i]->mp = NULL;
      if (reset)
	me->players[i]->Reset(true, true);
    }
  me->players.clear();

  return n;
}


// shuts the map down
void MapInfo::Close()
{
  state = MAP_UNLOADED;

  // delete the save file
  if (!savename.empty())
    {
      unlink(savename.c_str());
      savename.clear();
    }

  if (me)
    {
      delete me; // and then the map goes
      me = NULL;
    }
}


// for making hub saves
bool MapInfo::HubSave()
{
  if (!me)
    return false;

  CONS_Printf("hubsave...\n");

  char fname[50];
  sprintf(fname, "Legacy_hubsave_%02d.sav", mapnumber);
  savename = fname;

  LArchive a;
  a.Create("Hubsave");
  me->Serialize(a);

  byte *buffer;
  unsigned length = a.Compress(&buffer, 0);

  CONS_Printf("Simulated hubsave: %d bytes\n", length);
  FIL_WriteFile(savename.c_str(), buffer, length);
  Z_Free(buffer);

  state = MAP_SAVED;

  delete me;
  me = NULL;
  return true;
}


// well, loading the hub saves
bool MapInfo::HubLoad()
{
  if (state != MAP_SAVED)
    return false;

  byte *buffer;

  int length = FIL_ReadFile(savename.c_str(), &buffer);
  if (!length)
    {
      I_Error("Couldn't open hubsave file '%s'", savename.c_str());
      return false;
    }
  
  LArchive a;
  if (!a.Open(buffer, length))
    I_Error("Couldn't uncompress hubsave!\n");

  Z_Free(buffer);

  // dearchive the map
  me = new Map(this);
  if (me->Unserialize(a))
    I_Error("Hubsave corrupted!\n");

  state = MAP_RUNNING;
  return true;
}


//=================================
// Helper functions

// string contains only digits?
static bool IsNumeric(const char *p)
{
  for ( ; *p; p++)
    if (*p < '0' || *p > '9')
      return false;
  return true;
}

// converts 'line' to lower case
static void P_LowerCase(char *line)
{
  for ( ; *line; line++)
    *line = tolower(*line);
}

// replaces tailing spaces with NUL chars
static void P_StripSpaces(char *line)
{
  char *temp = line+strlen(line)-1;

  while (*temp == ' ')
    {
      *temp = '\0';
      temp--;
    }
}

// removes //-style comments from 'line'
static void P_RemoveComments(char *line)
{
  for ( ; *line; line++)
    {
      if (line[0] == '/' && line[1] == '/')
        {
          *line = '\0';
	  return;
        }
    }
}

// replace 'target' chars with spaces until '\0' is encountered
static void P_RemoveChars(char *p, char target)
{
  for ( ; *p; p++)
    if (*p == target)
      *p = ' ';
}

static inline char *P_PassWhitespace(char *p)
{
  while (isspace(*p))
    p++;
  return p;
}

//=================================
// Parser

typedef enum
{
  P_ITEM_NONE = 0, // bool. true if it item exists.
  P_ITEM_INT,
  P_ITEM_INT_INT, // two ints separated by whitespace
  P_ITEM_FLOAT,
  P_ITEM_STR,   // STL string
  P_ITEM_STR16, // 16 chars
  P_ITEM_STR16_FLOAT // max 16 char string and float
} parseritem_t;

struct parsercmd_t
{
  parseritem_t type;
  char   *name;
  size_t  offset;
};


// Parses one command line, stored in str. cmd contains the command name itself.
// Returns true if succesful. Modifies appropriate fields in var.
static bool P_ParseCommand(const char *cmd, char *str, const parsercmd_t *commands, char *var)
{
  for ( ; commands->name != NULL; commands++)
    if (!strcasecmp(cmd, commands->name))
      break;

  if (commands->name == NULL)
    return false; // not found

  str += strlen(cmd); // pass the command itself to get to the args
  while (isspace(*str))
    str++;

  size_t offs = commands->offset;
  int i, j;
  float f;
  char s[17];

  switch (commands->type)
    {
    case P_ITEM_NONE:
      *(bool *)(var+offs) = true;
      break;

    case P_ITEM_INT:
      i = atoi(str);
      *(int *)(var+offs) = i;
      break;

    case P_ITEM_INT_INT:
      if (sscanf(str, "%d %d", &i, &j) == 2)
	{
	  int *temp = (int *)(var + offs);
	  temp[0] = i;
	  temp[1] = j;
	}
      break;

    case P_ITEM_FLOAT:
      f = atof(str);
      *(float *)(var+offs) = f;
      break;

    case P_ITEM_STR:
      {
	string& result = *(string *)(var+offs);
	result = "";
	// get rid of surrounding quotation marks
	if (*str == '"')
	  {
	    int end = strlen(str) - 1;
	    if (str[end] == '"')
	      {
		str[end] = '\0';
		str++;
	      }
	  }

	char *temp = str;
	while (*temp)
	  {
	    // handle quotations with backslash
	    if (temp[0] == '\\')
	      {
		char r = 0;
		switch (temp[1])
		  {
		  case '\\': r = '\\'; break;
		  case 'n':  r = '\n'; break;
		  case '"':  r = '"'; break;
		  }

		if (r)
		  {
		    result.append(str, temp);
		    result += r;
		    str = temp = temp + 2;
		    continue;
		  }
	      }
	    temp++;
	  }
	result.append(str, temp);
      }
      break;

    case P_ITEM_STR16:
      sscanf(str, "%16s", var+offs);
      break;

    case P_ITEM_STR16_FLOAT:
      {
	sscanf(str, "%16s %f", s, &f);
	string & temp = *(string *)(var+offs);
	temp = s;
	*(float *)(var+offs+sizeof(string)) = f;
      }
      break;

    default:
      break;
    }

  return true;
}


//=================================
// Legacy MapInfo parser

//  Level vars: level variables in the [level info] section.
//
//  Takes the form:
//     [variable name] = [value]
//
//  '=' sign is optional: all equals signs are internally turned to spaces
//

// TODO decide the actual format of MapInfo lump...

// one command set per block
#ifdef LINUX
#define MI_offset(field) (size_t(&MapInfo::field))
#else
#define MI_offset(field) (size_t(&((MapInfo *)0)->field))
#endif
static parsercmd_t MapInfo_commands[]=
{
  {P_ITEM_STR, "levelname",  MI_offset(nicename)},
  {P_ITEM_STR, "name",       MI_offset(nicename)},
  {P_ITEM_STR, "version",    MI_offset(version)},
  {P_ITEM_STR, "creator",    MI_offset(author)},
  {P_ITEM_STR, "author",     MI_offset(author)},
  {P_ITEM_STR, "hint",       MI_offset(hint)},
  {P_ITEM_INT, "partime",    MI_offset(partime)},

  {P_ITEM_FLOAT, "gravity",  MI_offset(gravity)}, 
  {P_ITEM_STR, "skyname",    MI_offset(sky1)},
  {P_ITEM_STR, "music",      MI_offset(musiclump)},
  //{P_ITEM_STR, "nextlevel",  MI_offset(nextlevel)},
  //{P_ITEM_STR, "nextsecret", MI_offset(nextsecret)},
  /*
    {P_ITEM_STR,    "levelpic",     &info_levelpic},
    {P_ITEM_STR,    "interpic",     &info_interpic},
    {P_ITEM_STR,    "inter-backdrop",&info_backdrop},
    {P_ITEM_STR,    "defaultweapons",&info_weapons},
  */
  // {IVT_CONSOLECMD,"consolecmd",    NULL},
  {P_ITEM_NONE, NULL, 0} // terminator
};

static parsercmd_t MapFormat_commands[]=
{
  {P_ITEM_INT_INT, "doom_things", MI_offset(doom_offs)},
  {P_ITEM_INT_INT, "heretic_things", MI_offset(heretic_offs)},
  {P_ITEM_INT_INT, "hexen_things", MI_offset(hexen_offs)},
  {P_ITEM_NONE, NULL, 0} // terminator
};

static parsercmd_t MAPINFO_MAP_commands[] =
{
  {P_ITEM_INT, "cluster", MI_offset(cluster)},
  {P_ITEM_INT, "levelnum", MI_offset(mapnumber)},

  {P_ITEM_INT, "warptrans", MI_offset(warptrans)}, // Damnation!
  {P_ITEM_INT, "next", MI_offset(warpnext)}, // Hexen 'next' refers to warptrans numbers, not levelnums!
  //{P_ITEM_STR16, "next", MI_offset(nextmaplump)}, // TODO implement ZDoom lumpname-fields?
  //{P_ITEM_STR16, "secretnext", MI_offset(secretmaplump)},

  // our own solution (because the Hexen way SUCKS and ZDoom is not much better :)
  {P_ITEM_INT, "nextlevel", MI_offset(nextlevel)}, // always refer to levelnums
  {P_ITEM_INT, "secretlevel", MI_offset(secretlevel)},

  {P_ITEM_NONE, "doublesky", MI_offset(doublesky)},
  {P_ITEM_STR16_FLOAT, "sky1", MI_offset(sky1)},
  {P_ITEM_STR16_FLOAT, "sky2", MI_offset(sky2)},
  {P_ITEM_NONE, "lightning", MI_offset(lightning)},
  {P_ITEM_STR, "fadetable", MI_offset(fadetablelump)},
  {P_ITEM_INT, "cdtrack", MI_offset(cdtrack)},
  {P_ITEM_STR, "music", MI_offset(musiclump)},
  {P_ITEM_INT, "par", MI_offset(partime)},
  /*
  {P_ITEM_INT, "CD_START_TRACK", MI_offset()},
  {P_ITEM_INT, "CD_END1_TRACK", MI_offset()},
  {P_ITEM_INT, "CD_END2_TRACK", MI_offset()},
  {P_ITEM_INT, "CD_END3_TRACK", MI_offset()},
  {P_ITEM_INT, "CD_INTERMISSION_TRACK", MI_offset()},
  {P_ITEM_INT, "CD_TITLE_TRACK", MI_offset()},
  */
  {P_ITEM_NONE, NULL, 0}
};
#undef MI_offset



//----------------------------------------------------------------------------
//
// P_ParseScriptLine
//
// FraggleScript: if we are reading in script lines, we add the new lines
// into the levelscript
//

static string scriptblock;

static void P_ParseScriptLine(char *line)
{
  P_RemoveComments(line);
  scriptblock += line;  // add the new line to the current data
}


//====================================
// Reads a MapInfo lump for a map.

char *MapInfo::Read(int lump)
{
  if (lump == -1)
    return NULL;

  int length = fc.LumpLength(lump);
  char *ms = (char *)fc.CacheLumpNum(lump, PU_STATIC);
  char *me = ms + length; // past-the-end pointer

  char *s, *p;
  s = p = ms;

  scriptblock.clear();

  enum {PS_CLEAR, PS_MAPFORMAT, PS_SCRIPT, PS_INTERTEXT, PS_LEVELINFO} parsestate = PS_CLEAR;

  while (p < me)
    {
      if (*p == '\n') // line ends
	{
	  if (p > s)
	    {
	      // parse the line from s to p
	      *p = '\0';  // mark the line end
	      P_RemoveChars(s, '\r'); // damn carriage returns
	      s = P_PassWhitespace(s);

	      if (parsestate != PS_SCRIPT) // removenot for scripts
		{
		  if (s[0] == ';' || s[0] == '#' || s[0] == '\0' ||
		      (s[0] == '/' && s[1] == '/')) // comment or line end
		    s = p;
		  else
		    P_RemoveChars(s, '=');
		  // !(isprint(*p) || *p == '{' || *p == '}') // unprintable chars to whitespace?
		}

	      if (*s == '[')  // a new section seperator
		{
		  s++;
		  if (!strncasecmp(s, "level info", 10))
		    parsestate = PS_LEVELINFO;
		  else if (!strncasecmp(s, "scripts", 7))
		    {
		      parsestate = PS_SCRIPT;
		      scripts++; // has scripts
		    }
		  else if(!strncasecmp(s, "intertext", 9))
		    parsestate = PS_INTERTEXT;
		  else if(!strncasecmp(s, "map format", 10))
		    parsestate = PS_MAPFORMAT;
		}
	      else switch (parsestate)
		{
		  char buf[31]; // read in max. 30 character strings
		case PS_LEVELINFO:
		  sscanf(s, "%30s", buf);
		  if (!P_ParseCommand(buf, s, MapInfo_commands, (char *)this))
		    CONS_Printf("Unknown MapInfo command '%s' at char %d!\n", s, s - ms);
		  break;

		case PS_SCRIPT:
#ifdef FRAGGLESCRIPT
		  P_ParseScriptLine(s);
#endif
		  break;

		case PS_INTERTEXT:
		  //intertext += '\n';
		  //intertext += s;
		  break;

		case PS_MAPFORMAT:
		  sscanf(s, "%30s", buf);
		  if (!P_ParseCommand(buf, s, MapFormat_commands, (char *)this))
		    CONS_Printf("Unknown Map Format command '%s' at char %d!\n", s, s - ms);
		  break;
		  break;

		case PS_CLEAR:
		  break;
		}
	    }
	  s = p + 1;  // pass the line
	}
      p++;
    }
  
  Z_Free(ms);

  if (doom_offs[1] == 0 && heretic_offs[1] == 0 && hexen_offs[1] == 0)
    // none was given, original behavior
    if (game.mode == gm_hexen)
      hexen_offs[0] = 0, hexen_offs[1] = 65535;
    else if (game.mode == gm_heretic)
      heretic_offs[0] = 0, heretic_offs[1] = 65535;
    else
      doom_offs[0] = 0, doom_offs[1] = 65535;

  CONS_Printf("doom offsets: %d, %d\n", doom_offs[0], doom_offs[1]);
  CONS_Printf("heretic offsets: %d, %d\n", heretic_offs[0], heretic_offs[1]);
  CONS_Printf("hexen offsets: %d, %d\n", hexen_offs[0], hexen_offs[1]);
  //P_InitWeapons();

  //Set the gravity for the level!
  extern  consvar_t cv_gravity;
  fixed_t grav = int(gravity * FRACUNIT);
  if (cv_gravity.value != grav)
    COM_BufAddText(va("gravity %f\n", gravity));

  COM_BufExecute(); //Hurdler: flush the command buffer

  // FS script data TODO a bit clumsy
  s = Z_Strdup(scriptblock.c_str(), PU_LEVEL, NULL);
  scriptblock.clear();

  return s;
}



//==============================================
// Hexen/ZDoom MAPINFO parser.

#ifdef LINUX
#define CD_offset(field) (size_t(&MapCluster::field))
#else
#define CD_offset(field) (size_t(&((MapCluster *)0)->field))
#endif
// ZDoom clusterdef commands
static parsercmd_t MAPINFO_CLUSTERDEF_commands[] =
{
  {P_ITEM_STR, "entertext", CD_offset(entertext)},
  {P_ITEM_STR, "exittext", CD_offset(exittext)},
  {P_ITEM_STR, "music", CD_offset(finalemusic)},
  {P_ITEM_STR, "flat", CD_offset(finalepic)},
  {P_ITEM_NONE, "hub", CD_offset(hub)},
  {P_ITEM_NONE, NULL, 0}
};
#undef CD_offset


// Reads the MAPINFO lump, filling mapinfo and clusterdef maps with data
int GameInfo::Read_MAPINFO(int lump)
{
  int i, j;

  if (lump < 0)
    return -1;

  vector<MapInfo *> tempinfo;

  Clear_mapinfo_clusterdef();

  MapCluster *cl = NULL;

  MapInfo  def; // default map
  MapInfo *info;

  int length = fc.LumpLength(lump);
  char *ms = (char *)fc.CacheLumpNum(lump, PU_STATIC);
  char *me = ms + length; // past-the-end pointer

  char *s, *p;
  s = p = ms;

  enum {PS_CLEAR, PS_MAP, PS_CLUSTERDEF} parsestate = PS_CLEAR;

  while (p < me)
    {
      if (*p == '\n') // line ends
	{
	  if (p > s)
	    {
	      // parse the line from s to p
	      *p = '\0';  // mark the line end
	      P_RemoveChars(s, '\r'); // damn carriage returns
	      s = P_PassWhitespace(s);

	      char b1[61], b2[17]; // read in max. 60 (16) character strings

	      i = sscanf(s, "%30s", b1); // pass whitespace, read first word
	      if (i != EOF && !(b1[0] == ';' || b1[0] == '#' || (b1[0] == '/' && b1[1] == '/')))
		// not a blank line or comment
		// block starters?
		if (!strcasecmp(b1, "DEFAULTMAP"))
		  {
		    // default map definition block begins
		    parsestate = PS_MAP;
		    info = &def;
		  }
		else if (!strcasecmp(b1, "MAP"))
		  {
		    // map definition block begins
		    parsestate = PS_MAP;
		    i = sscanf(s, "%*30s %16s \"%60[^\"]\"", b2, b1);
		    info = new MapInfo(def); // copy construction
		    tempinfo.push_back(info);

		    if (IsNumeric(b2))
		      {
			// assume Hexen
			j = atoi(b2);
			char temp[17];
			if (j >= 0 && j <= 99)
			  {
			    sprintf(temp, "MAP%02d", j);
			    info->lumpname = temp;
			    info->mapnumber = j;
			  }
		      }
		    else
		      // ZDoom
		      info->lumpname = b2;

		    info->nicename = b1;
		    //CONS_Printf(" %s\n", b1);
		  }
		else if (!strcasecmp(b1, "CLUSTERDEF"))
		  {
		    // cluster definition block begins (ZDoom)
		    parsestate = PS_CLUSTERDEF;
		    i = sscanf(s, "%*30s %d", &j);
		    if (clustermap.count(j))
		      {
			CONS_Printf("MAPINFO: Cluster number %d defined more than once!\n", j);
			cl = clustermap[j]; // already there, update the existing cluster
		      }
		    else
		      clustermap[j] = cl = new MapCluster(j);
		  }
		else switch (parsestate)
		  {
		  case PS_CLEAR:
		    CONS_Printf("Unknown MAPINFO block at char %d!\n", s - ms);
		    break;

		  case PS_MAP:
		    if (!P_ParseCommand(b1, s, MAPINFO_MAP_commands, (char *)info))
		      CONS_Printf("Unknown MAPINFO MAP command '%s' at char %d!\n", s, s - ms);
		    break;

		  case PS_CLUSTERDEF:
		    if (!P_ParseCommand(b1, s, MAPINFO_CLUSTERDEF_commands, (char *)cl))
		      CONS_Printf("Unknown MAPINFO CLUSTERDEF command '%s' at char %d!\n", s, s - ms);
		    break;
		  }
	    }
	  s = p + 1;  // pass the line
	}
      else if (*p == '=') 
	*p = ' ';

      p++;      
    }

  Z_Free(ms);

  // now "remap" tempinfo into mapinfo
  int n = tempinfo.size();
  for (i=0; i<n; i++)
    {
      info = tempinfo[i];
      j = info->mapnumber;

      if (mapinfo.count(j)) // already there
	{
	  CONS_Printf("MAPINFO: Map number %d found more than once!\n", j);
	  delete mapinfo[j]; // later one takes precedence
	}

      mapinfo[j] = info;
    }

  // generate the missing clusters and fill them all with maps
  for (mapinfo_iter_t t = mapinfo.begin(); t != mapinfo.end(); t++)
    {
      info = (*t).second;
      n = info->cluster;
      if (!clustermap.count(n))
	{
	  CONS_Printf("new cluster %d (map %d)\n", n, info->mapnumber);
	  // create the cluster
	  cl = clustermap[n] = new MapCluster(n);
	  cl->hub = cl->keepstuff = true; // for original Hexen
	}
      else
	cl = clustermap[n];

      cl->maps.push_back(info);
    }

  // and then check that all clusters have at least one map
  for (cluster_iter_t t = clustermap.begin(); t != clustermap.end(); t++)
    {
      cl = (*t).second;
      if (cl->maps.empty())
	{
	  CONS_Printf("Cluster %d has no maps!\n", cl->number);
	  clustermap.erase(t);
	  delete cl;
	}
    }

  return mapinfo.size();
}



//==============================================
// GameInfo utilities related to MapInfo and MapCluster

void GameInfo::Clear_mapinfo_clusterdef()
{
  // delete old mapinfo and clusterdef
  mapinfo_iter_t s;
  for (s = mapinfo.begin(); s != mapinfo.end(); s++)
    delete (*s).second;
  mapinfo.clear(); 

  cluster_iter_t t;
  for (t = clustermap.begin(); t != clustermap.end(); t++)
    delete (*t).second;
  clustermap.clear();
}


MapCluster *GameInfo::FindCluster(int c)
{
  cluster_iter_t i = clustermap.find(c);
  if (i == clustermap.end())
    return NULL;

  return (*i).second;
}

MapInfo *GameInfo::FindMapInfo(int c)
{
  mapinfo_iter_t i = mapinfo.find(c);
  if (i == mapinfo.end())
    return NULL;

  return (*i).second;
}


//==============================================
// Console Commands

void Command_MapInfo_f()
{
  if (!consoleplayer || !consoleplayer->mp)
    return;

  Map *m = consoleplayer->mp;
  CONS_Printf("%s\n", m->info->nicename.c_str());
  CONS_Printf("%s\n", m->info->author.c_str());
}
