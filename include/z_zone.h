// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2003 by DooM Legacy Team.
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
// Revision 1.2  2003/02/08 21:43:50  smite-meister
// New Memzone system. Choose your pawntype! Cyberdemon OK.
//
// Revision 1.1.1.1  2002/11/16 14:18:23  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//      Remark: this was the only stuff that, according
//       to John Carmack, might have been useful for
//       Quake.
//
//---------------------------------------------------------------------

#ifndef z_zone_h
#define z_zone_h 1

#include <stdio.h>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < PU_PURGELEVEL are not overwritten until freed.
#define PU_STATIC               1      // static entire execution time
#define PU_SOUND                2      // static while playing
#define PU_MUSIC                3      // static while playing
#define PU_DAVE                 4      // anything else Dave wants static

#define PU_HWRPATCHINFO         5      // Hardware GlidePatch_t struct for OpenGl/Glide texture cache
#define PU_HWRPATCHCOLMIPMAP    6      // Hardware GlideMipmap_t struct colromap variation of patch

#define PU_LEVEL               50      // static until level exited
#define PU_LEVSPEC             51      // a special thinker in a level
#define PU_HWRPLANE            52
// Tags >= PU_PURGELEVEL are automatically purgable whenever needed.
#define PU_PURGELEVEL         100
#define PU_CACHE              101
#define PU_HWRCACHE           102      // 'second-level' cache for graphics
                                       // stored in hardware format and downloaded as needed

void    Z_Init();
void    Z_FreeTags (int lowtag, int hightag);
void    Z_DumpHeap (int lowtag, int hightag);
void    Z_FileDumpHeap (FILE *f);
void    Z_CheckHeap (int i);
void    Z_ChangeTag2 (void *ptr, int tag);

// returns number of bytes allocated for one tag type
int     Z_TagUsage (int tagnum);
void    Z_FreeMemory (int *realfree,int *cachemem,int *usedmem,int *largefreeblock);

//#define ZDEBUG

#ifdef ZDEBUG
#define Z_Free(p) Z_Free2(p,__FILE__,__LINE__)
void    Z_Free2 (void *ptr,char *file,int line);
#define Z_Malloc(s,t,p) Z_Malloc2(s,t,p,0,__FILE__,__LINE__)
#define Z_MallocAlign(s,t,p,a) Z_Malloc2(s,t,p,a,__FILE__,__LINE__)
void*   Z_Malloc2 (int size, int tag, void *ptr, int alignbits, char *file,int line);
#else
void    Z_Free (void *ptr);
void*   Z_MallocAlign(int size,int tag,void* user,int alignbits);
#define Z_Malloc(s,t,p) Z_MallocAlign(s,t,p,0)
#endif

void Z_ChangeTag(void *ptr, int tag);

char *Z_Strdup(const char *s, int tag, void **user);

#endif
