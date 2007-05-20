// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

#include "r_data.h"
#include "doomdef.h"
#include "hardware/oglhelpers.hpp"

static const GLubyte *gl_extensions = NULL;

static byte lightleveltonumlut[256];

// Converts Doom sector light values to suitable background pixel
// color. extralight is for temporary brightening of the screen due to
// muzzle flashes etc.

byte LightLevelToLum(int l, int extralight) {
  /* 
  if (fixedcolormap)
    return 255;
  */
  l = lightleveltonumlut[l];
  l += (extralight << 4);
  if (l > 255)
    l = 255;
  return l;
}

// Hurdler's magical mystery mapping function initializer.

void InitLumLut() {
    int i;
    float k;
    for (i = 0; i < 256; i++)
    {
        // this polygone is the solution of equ : f(0)=0, f(1)=1
        // f(.5)=.5, f'(0)=0, f'(1)=0), f'(.5)=K
#define K   2
#define A  (-24+16*K)
#define B  ( 60-40*K)
#define C  (32*K-50)
#define D  (-8*K+15)
        float x = (float) i / 255;
        float xx, xxx;
        xx = x * x;
        xxx = x * xx;
        k = 255 * (A * xx * xxx + B * xx * xx + C * xxx + D * xx);

        lightleveltonumlut[i] = 255 < k ? 255 : int(k); //min(255, k);
    }
}

/// Tells whether the spesified extension is supported by the current
/// OpenGL implementation.

bool GLExtAvailable(char *extension)
{
    const GLubyte *start;
    GLubyte *where, *terminator;

    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;

    if(!gl_extensions)
      gl_extensions = glGetString(GL_EXTENSIONS);

    start = gl_extensions;
    for (;;)
    {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return true;
        start = terminator;
    }
    return false;
}
