// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2003-2004 by DooM Legacy Team.
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
// Revision 1.10  2004/09/23 23:21:16  smite-meister
// HUD updated
//
// Revision 1.9  2004/09/15 19:23:59  smite-meister
// bugfixes
//
// Revision 1.8  2004/09/06 19:58:02  smite-meister
// Doom linedefs done!
//
// Revision 1.7  2004/09/03 16:28:49  smite-meister
// bugfixes and ZDoom linedef types
//
// Revision 1.6  2004/08/29 20:48:48  smite-meister
// bugfixes. wow.
//
// Revision 1.5  2004/08/13 18:25:10  smite-meister
// sw renderer fix
//
// Revision 1.4  2004/07/05 16:53:25  smite-meister
// Netcode replaced
//
// Revision 1.3  2004/04/25 16:26:49  smite-meister
// Doxygen
//
// Revision 1.2  2004/04/01 09:16:16  smite-meister
// Texture system bugfixes
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Texture animation, parser for ANIMDEFS and ANIMATED lumps

#include <string>

#include "doomdef.h"
#include "parser.h"

#include "g_game.h"

#include "r_data.h"
#include "r_defs.h" // for floortypes

#include "m_random.h"
#include "w_wad.h"
#include "z_zone.h"


/// template for the Boom ANIMATED lump entries
/// used for defining texture and flat animation sequences
#pragma pack(1) // GCC needs this!
struct ANIMATED_t
{
  char        istexture;   // 0 means flat, -1 is a terminator
  char        endname[9];
  char        startname[9];
  int         speed;
};
#pragma pack()


// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.

static ANIMATED_t DoomAnims[] =
{
  // DOOM II flat animations.
  {false,     "NUKAGE3",      "NUKAGE1",      8},
  {false,     "FWATER4",      "FWATER1",      8},
  {false,     "SWATER4",      "SWATER1",      8},
  {false,     "LAVA4",        "LAVA1",        8},
  {false,     "BLOOD3",       "BLOOD1",       8},

  {false,     "RROCK08",      "RROCK05",      8},
  {false,     "SLIME04",      "SLIME01",      8},
  {false,     "SLIME08",      "SLIME05",      8},
  {false,     "SLIME12",      "SLIME09",      8},

    // animated textures
  {true,      "BLODGR4",      "BLODGR1",      8},
  {true,      "SLADRIP3",     "SLADRIP1",     8},

  {true,      "BLODRIP4",     "BLODRIP1",     8},
  {true,      "FIREWALL",     "FIREWALA",     8},
  {true,      "GSTFONT3",     "GSTFONT1",     8},
  {true,      "FIRELAVA",     "FIRELAV3",     8},
  {true,      "FIREMAG3",     "FIREMAG1",     8},
  {true,      "FIREBLU2",     "FIREBLU1",     8},
  {true,      "ROCKRED3",     "ROCKRED1",     8},

  {true,      "BFALL4",       "BFALL1",       8},
  {true,      "SFALL4",       "SFALL1",       8},
  {true,      "WFALL4",       "WFALL1",       8},
  {true,      "DBRAIN4",      "DBRAIN1",      8},
  {-1}
};

static ANIMATED_t HereticAnims[] =
{
  // heretic 
  {false,     "FLTWAWA3",     "FLTWAWA1",     8}, // Water
  {false,     "FLTSLUD3",     "FLTSLUD1",     8}, // Sludge
  {false,     "FLTTELE4",     "FLTTELE1",     6}, // Teleport
  {false,     "FLTFLWW3",     "FLTFLWW1",     9}, // River - West
  {false,     "FLTLAVA4",     "FLTLAVA1",     8}, // Lava
  {false,     "FLATHUH4",     "FLATHUH1",     8}, // Super Lava
  {true,      "LAVAFL3",      "LAVAFL1",      6}, // Texture: Lavaflow
  {true,      "WATRWAL3",     "WATRWAL1",     4}, // Texture: Waterfall
  {-1}
};




// flat floortypes
static struct
{
  char        *name;
  floortype_t  type;
}
ftypes[] =
{
  {"FWATER",   FLOOR_WATER},
  {"FLTWAWA1", FLOOR_WATER},
  {"FLTFLWW1", FLOOR_WATER},

  {"FLTLAVA1", FLOOR_LAVA},
  {"FLATHUH1", FLOOR_LAVA},

  {"FLTSLUD1", FLOOR_SLUDGE},

  {"X_005", FLOOR_WATER},
  {"X_001", FLOOR_LAVA},
  {"X_009", FLOOR_SLUDGE},
  {"F_033", FLOOR_ICE}
};


// Ohhhh... This sucks so much... FIXME
floortype_t P_GetFloorType(const char *pic)
{
  for (int i=0; i<10; i++)
    if (!strncasecmp(pic, ftypes[i].name, 8))
      return ftypes[i].type;

  return FLOOR_SOLID;
}


#define MAX_ANIM_DEFS  20
#define MAX_FRAME_DEFS 20


/// \brief Metaclass for animated Textures
class AnimatedTexture : public Texture
{
public:
  struct framedef_t
  {
    Texture *tx;
    int tics;
  };

  framedef_t *frames; ///< the animation frames
  int      numframes; ///< number of frames
  int   currentframe; ///< current frame index
  tic_t    changetic; ///< when will the current frame end?

protected:
  Texture *Update();
  virtual byte *Generate() { return NULL; }; /// not used for now
  virtual void HWR_Prepare()  { /* Update()->HWR_Prepare(); */ }

public:
  AnimatedTexture(const char *p, int n);
  ~AnimatedTexture();

  virtual bool Masked() { return Update()->Masked(); };
  virtual byte *GetColumn(int col) { return Update()->GetColumn(col); }
  virtual column_t *GetMaskedColumn(int col) { return Update()->GetMaskedColumn(col); }
  virtual byte *GetData() { return Update()->GetData(); }

  virtual void Draw(int x, int y, int scrn) { Update()->Draw(x, y, scrn); }
  virtual void HWR_Draw(int x, int y, int flags) { Update()->HWR_Draw(x, y, flags); }
};



AnimatedTexture::AnimatedTexture(const char *p, int n)
  : Texture(p)
{
  numframes = n;
  currentframe = 0;
  changetic = game.tic;
  
  frames = (framedef_t *)Z_Malloc(n*sizeof(framedef_t), PU_TEXTURE, NULL);
}


AnimatedTexture::~AnimatedTexture()
{
  if (frames)
    Z_Free(frames);
}


Texture *AnimatedTexture::Update()
{
  while (changetic <= game.tic)
    {
      // next frame
      if (++currentframe == numframes)
	currentframe = 0;

      tic_t tics = frames[currentframe].tics;
      if (tics > 255)
	tics = (tics >> 16) + P_Random() % ((tics & 0xFF00) >> 8); // Random tics     
   
      changetic += tics;
    }

  return frames[currentframe].tx;
}



/// Reads and interprets the Boom ANIMATED lump
int P_Read_ANIMATED(int lump)
{
  ANIMATED_t *anims;
  if (lump >= 0)
    {
      CONS_Printf("Reading ANIMATED...\n");
      anims = (ANIMATED_t *)fc.CacheLumpNum(lump, PU_STATIC);
    }
  else if (game.mode == gm_heretic)
    anims = HereticAnims;
  else
    anims = DoomAnims;

  int i, count = 0;

  for (ANIMATED_t *a = anims; a->istexture != -1; a++)
    {
      // TODO problem with flats
      // check different episode ?
      int base, last;
      if (a->istexture)
	{
	  base = tc.Get(a->startname, false);
	  last = tc.Get(a->endname, false);
	}
      else
	{
	  base = fc.FindNumForName(a->startname);
	  last = fc.FindNumForName(a->endname);
	}

      int n = last - base + 1; // number of frames
      if (n < 2 || n > MAX_FRAME_DEFS || base == -1 || last == -1)
	{
	  if (lump >= 0)
	    CONS_Printf("ANIMATED: Bad cycle from %s to %s", a->startname, a->endname);
	  continue;
	}

      AnimatedTexture *t = new AnimatedTexture(a->startname, n);

      int tics = ((lump >= 0) ? LONG(a->speed) : a->speed) * NEWTICRATERATIO; // duration of one frame in tics
      for (i = 0; i < n; i++)
	{
	  if (a->istexture)
	    t->frames[i].tx = tc[base + i];
	  else
	    t->frames[i].tx = tc.GetPtrNum(base + i);

	  t->frames[i].tics = tics;
	}

      tc.Insert(t);
      count++;

      /*
      // FIXME half-assed animation system
      // TODO a small-time HACK, create one instance for each frame of animation
      for (i = 1; i < n; i++)
	{
	  AnimatedTexture *r = new AnimatedTexture(*t); // copy construct
	  if (a->istexture)
	    r->name = textures[base + i]->name;
	  else
	    r->name = fc.FindNameForNum(base + i);

	  r->currentframe = i;
	  tc.Insert(r);
	}
      */
    }

  if (lump >= 0)
    {
      Z_Free(anims);
      CONS_Printf("... done. %d animations.\n", count);
    }

  return count;
}



/// Parses the Hexen ANIMDEFS lump, creates the required animated textures
int P_Read_ANIMDEFS(int lump)
{
  Parser p;

  if (!p.Open(lump))
    return -1;

  CONS_Printf("Reading ANIMDEFS...\n");

  int i, n, count = 0;
  vector<AnimatedTexture::framedef_t> frames;

  p.RemoveComments(';');
  enum p_state_e {PS_NONE, PS_FLAT, PS_TEX};
  p_state_e state = PS_NONE;
  int base = 0;
  while (p.NewLine())
    {
      // texture/flat <name>
      // pic <n> tics <t>
      // pic <n> rand <min> <max>
      char *word, *name = NULL;

      if (state != PS_NONE)
	{
	  // in record
	  char temp = p.Peek();
	  if (temp != 'p' && temp != 'P')
	    {
	      // record just ended
	      state = PS_NONE;

	      n = frames.size();
	      if (n < 2)
		I_Error("AnimDef has framecount < 2.");

	      // create the animation (it has the same name as its
	      // 1st frame, which is thus overwritten from tc map)
	      // TODO This is a problem if several animations share the frame!
	      AnimatedTexture *t = new AnimatedTexture(name, n);
	      tc.Insert(t);
	      count++;

	      for (i=0; i<n; i++)
		t->frames[i] = frames[i];

	      frames.clear();
	    }
	  else
	    {
	      // read in the frames
	      word = p.GetToken(" ");
	      if (word && !strcasecmp(word, "pic"))
		{
		  if (frames.size() >= MAX_FRAME_DEFS)
		    I_Error("Too many FrameDefs.");

		  AnimatedTexture::framedef_t fd;

		  n = base + p.GetInt() - 1;
		  if (state == PS_FLAT)
		    fd.tx = tc.GetPtrNum(n);
		  else
		    fd.tx = tc[n];

		  word = p.GetToken(" ");
		  if (!strcasecmp(word, "tics"))
		    fd.tics = p.GetInt();
		  else if (!strcasecmp(word, "rand"))
		    {
		      n = p.GetInt();          // min
		      i = p.GetInt() - n + 1;  // range
		      fd.tics = (n << 16) + (i << 8);
		    }
		  else
		    {
		      if (word)
			CONS_Printf("Unknown modifier: '%s'\n", word);
		      else
			CONS_Printf("Missing modifier!\n");
		      fd.tics = 35;
		    }

		  frames.push_back(fd);
		}
	      else
		state = PS_NONE; // end of animdef

	      continue; // next line
	    }
	}

      // no active records
      word = p.GetToken(" ");
      name = p.GetToken(" ");
      if (!strcasecmp(word, "flat"))
	{
	  // means a LumpTexture
	  base = fc.FindNumForName(name); // lump number
	  if (base >= 0)
	    state = PS_FLAT;
	}
      else if (!strcasecmp(word, "texture"))
	{
	  base = tc.Get(strupr(name)); // texture number
	  if (base > 0)
	    state = PS_TEX;
	}
      else
	CONS_Printf("Unknown command: '%s'", word);
    }


  if (count >= MAX_ANIM_DEFS)
    I_Error("Too many AnimDefs.");

  CONS_Printf("... done. %d animations.\n", count);
  return count;
}
