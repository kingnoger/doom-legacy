// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1995 by Sebastien Bacquet.
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
// Revision 1.2  2004/08/18 14:35:20  smite-meister
// PNG support!
//
// Revision 1.1.1.1  2002/11/16 14:18:25  hurdler
// Initial C++ version of Doom Legacy
//
// Revision 1.4  2000/09/10 10:46:15  metzgermeister
// merged with SDL version
//
// Revision 1.3  2000/04/21 08:23:47  emanne
// To have SDL working.
// Makefile: made the hiding by "@" optional. See the CC variable at
// the begining. Sorry, but I like to see what's going on while building
//
// qmus2mid.h: force include of qmus2mid_sdl.h when needed.
// s_sound.c: ??!
// s_sound.h: with it.
// (sorry for s_sound.* : I had problems with cvs...)
//
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief MUS to MIDI conversion

#if !defined(QMUS2MID_H)
#define QMUS2MID_H

#define NOTMUSFILE      1       /* Not a MUS file */
#define COMUSFILE       2       /* Can't open MUS file */
#define COTMPFILE       3       /* Can't open TMP file */
#define CWMIDFILE       4       /* Can't write MID file */
#define MUSFILECOR      5       /* MUS file corrupted */
#define TOOMCHAN        6       /* Too many channels */
#define MEMALLOC        7       /* Memory allocation error */
#define MIDTOLARGE      8       /* If the mid don't fit in the buffer */

/* some (old) compilers mistake the "MUS\x1A" construct (interpreting
   it as "MUSx1A")      */

#define MUSMAGIC     "MUS\032"                    /* this seems to work */
#define MIDIMAGIC    "MThd\000\000\000\006\000\001"
#define TRACKMAGIC1  "\000\377\003\035"
#define TRACKMAGIC2  "\000\377\057\000"           /* end of track */
#define TRACKMAGIC3  "\000\377\002\026"
#define TRACKMAGIC4  "\000\377\131\002\000\000"
#define TRACKMAGIC5  "\000\377\121\003\011\243\032"
#define TRACKMAGIC6  "\000\377\057\000"


struct MUSheader
{
  char          ID[4];            // identifier "MUS" 0x1A
  Uint16        scoreLength;      // length of score in bytes
  Uint16        scoreStart;       // absolute file pos of the score
  Uint16        channels;         // count of primary channels
  Uint16        sec_channels;      // count of secondary channels
  Uint16        instrCnt;
  Uint16        dummy;
  // variable-length part starts here
  Uint16        instruments[0];
};


struct Track
{
    unsigned long  current;
    char           vel;
    long           DeltaTime;
    unsigned char  LastEvent;
    char           *data;            /* Primary data */
};

int qmus2mid (byte  *mus, byte *mid,     // buffers in memory
              Uint16 division, int BufferSize, int nocomp,
              int    length, int midbuffersize,
              Uint32 *midilength);    //faB: returns midi file length in here

#endif
