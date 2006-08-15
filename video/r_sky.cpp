// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
//
// DESCRIPTION:
//      Sky rendering. The DOOM sky is a texture map like any
//      wall, wrapping around. A 1024 columns equal 360 degrees.
//      The default sky map is 256 columns and repeats 4 times
//      on a 320 screen?
//  
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "r_sky.h"
#include "r_data.h"

//
// sky mapping
//
// the "sky flat" is not used in rendering, just to let the engine know
// that a particular sector ceiling is in fact the sky.

int          skyflatnum; 
int          skytexturemid;


//  Setup sky draw for old or new skies (new skies = freelook 256x240)
//
//  Call at loadlevel after skytexture is set
//
//  NOTE: skycolfunc should be set at R_ExecuteSetViewSize ()
//        I dont bother because we don't use low detail no more
//
void R_SetupSkyDraw(Texture *skytex)
{
  // parse the patches composing sky texture for the tallest one
  // patches are usually RSKY1,RSKY2... and unique

  // note: the TEXTURES lump doesn't have the taller size of Legacy
  //       skies, but the patches it use will give the right size

  if (skytex->height > 128)
    {
      // horizon line on 256x240 freelook textures of Legacy or heretic
      skytexturemid = 200;
      // normal aspect ratio corrected scale
    }
  else
    {
      // the horizon line in a 256x128 sky texture
      skytexturemid = 100;
      // double the texture vertically, bleeergh!!
      skytex->yscale >>= 1;
    }


  // skytexturemid = (100*skytex->height) / 128;
}
