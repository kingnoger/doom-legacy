// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
// WadFile: A class that handles WAD file I/O
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <fcntl.h>

#ifndef __WIN32__
# include <unistd.h>
#endif

#include "d_netfil.h" //findfile()
#include "dehacked.h"
#include "i_system.h"
#include "md5.h"
#include "z_zone.h"


#include "wad.h"

//===========================================================================
// WadFile class implementation


void *WadFile::operator new(size_t size)
{
  return Z_Malloc(size, PU_STATIC, NULL);
}

void WadFile::operator delete(void *mem)
{
  Z_Free(mem);
}

// -----------------------------------------------------
// Allocates a wadfile, sets up the lumpinfo (directory) and
// lumpcache. return -1 in case of problem
//
// Can also load dehacked files (ext .deh)
// FIXME! unfortunately needs to know its index in the FileCache vector.

WadFile::WadFile(const char *fname, int num)
{
  char name[MAX_WADPATH];
  unsigned int i, length;

  // TODO since constructors cannot return anything, we use this for "return value"
  lumpinfo = NULL;

  // we'll want to edit the filename
  strncpy(name, fname, MAX_WADPATH);
  //fname = filenamebuf;
  // open wad file
  if ((handle = open(name, O_RDONLY|O_BINARY, 0666)) == -1)
    {
      nameonly(name); // leave full path here
      if (findfile(name, NULL, true))
	{
	  if ((handle = open(name, O_RDONLY|O_BINARY, 0666)) == -1)
	    {
	      CONS_Printf("Can't open %s\n", name);
	      return;
	    }
	}
      else
	{
	  CONS_Printf("File %s not found.\n", name);
	  return;
	}
    }
  // FIXME: WadFile class needs a way to tell the engine that a file was not found.
  //        Maybe we should use a normal method instead of a constructor?

  // file found! this is OK because filename is string, not char *
  filename = name;

  // get file system info about the file
  struct stat bufstat;
  fstat(handle, &bufstat);
  filesize = bufstat.st_size;

  // detect dehacked file with the "deh" extension
  if (stricmp(&name[strlen(name)-3],"deh") == 0)
    {
      // this code emulates a wadfile with one lump name "DEHACKED" 
      // at position 0 and size of the whole file
      // this allows deh files to be treated like wad files,
      // copied by network and loaded at the console
      numlumps = 1; 
      lumpinfo = (lumpinfo_t *)Z_Malloc(sizeof(lumpinfo_t), PU_STATIC, NULL);
      lumpinfo->position = 0;
      lumpinfo->size = bufstat.st_size;
      strcpy(lumpinfo->name, "DEHACKED");
    }
  else 
    {   // assume wad file
      wadinfo_t        header;
      lumpinfo_t*      lump_p;
      filelump_t*      fileinfo;
      
      // read the header
      read(handle, &header, sizeof(wadinfo_t));
      if (strncmp(header.identification, "IWAD", 4))
        {
	  // Homebrew levels?
	  if (strncmp(header.identification, "PWAD", 4))
            {
	      CONS_Printf("%s doesn't have IWAD or PWAD id\n", filename.c_str());
	      return;
            }
        }
      // endianness swapping
      header.numlumps = LONG(header.numlumps);
      header.infotableofs = LONG(header.infotableofs);

      // read wad file directory
      length = header.numlumps * sizeof(filelump_t);
      fileinfo = (filelump_t*)alloca(length);
      // alloca() gets its memory from the stack frame, it is automatically
      // freed when this function exits
      lseek(handle, header.infotableofs, SEEK_SET);
      // read wad directory into fileinfo
      read(handle, fileinfo, length);
      numlumps = header.numlumps;
      
      // fill in lumpinfo for this wad
      lump_p = lumpinfo = (lumpinfo_t*)Z_Malloc(numlumps * sizeof(lumpinfo_t), PU_STATIC, NULL);
      for (i=0 ; i<numlumps ; i++, lump_p++, fileinfo++)
        {
	  //lump_p->handle   = handle;
	  lump_p->position = LONG(fileinfo->filepos);
	  lump_p->size     = LONG(fileinfo->size);
	  strncpy(lump_p->name, fileinfo->name, 8);
        }
    }
  
  // generate md5sum 
  // we need a stream, not just a descriptor
  {
    FILE *stream = fopen(name, "rb");
    {
      //int t = I_GetTime();
      md5_stream(stream, md5sum);
      //if (devparm) CONS_Printf("md5 calc for %s took %f seconds\n", filename.c_str(), (float)(I_GetTime()-t)/TICRATE);
    }
    fclose(stream);
  }
  
  //  set up caching
  length = numlumps * sizeof(lumpcache_t);
  lumpcache = (lumpcache_t*)Z_Malloc(length, PU_STATIC, NULL);  
  memset(lumpcache, 0, length);

#ifdef HWRENDER
  //faB: now allocates GlidePatch info structures STATIC from the start,
  //     because these were causing a lot of fragmentation of the heap,
  //     considering they are never freed.
  length = numlumps * sizeof(GlidePatch_t);
  hwrcache = (GlidePatch_t*)Z_Malloc(length, PU_HWRPATCHINFO, NULL);    //never freed
  // set mipmap.downloaded to false
  memset(hwrcache, 0, length);
  for (i=0; i<numlumps; i++)
    {
      //store the software patch lump number for each GlidePatch
      hwrcache[i].patchlump = (num << 16) + i;
    }
#endif

  CONS_Printf("Added file %s (%i lumps)\n", filename.c_str(), numlumps);
  LoadDehackedLumps();
  return;
}

// -----------------------------------------------------
// WadFile destructor. Closes the open file.
WadFile::~WadFile()
{
  // FIXME! Doesn't free Zone memory or invalidate cache yet!
  close(handle);
}


// -----------------------------------------------------
// !!!NOT CHECKED WITH NEW WAD SYSTEM
// DOESN'T WORK! DOESN'T MAKE SENSE!
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
/*
void WadFile::Reload(void)
{
  wadinfo_t           header;
  int                 lumpcount;
  lumpinfo_t*         lump_p;
  int                 i;
  int                 handle;
  int                 length;
  filelump_t*         fileinfo;
  lumpcache_t*        lumpcache;

  // FIXME! the file is already open?? reopen() ??
  if ((handle = open(filename, O_RDONLY | O_BINARY)) == -1)
    I_Error("WadFile::Reload: couldn't open %s", filename);

  read(handle, &header, sizeof(header));
  lumpcount = LONG(header.numlumps);
  header.infotableofs = LONG(header.infotableofs);
  length = lumpcount*sizeof(filelump_t);
  fileinfo = alloca(length);
  lseek(handle, header.infotableofs, SEEK_SET);
  read(handle, fileinfo, length);

  // Fill in lumpinfo
  lump_p = wadfiles[reloadlump>>16]->lumpinfo + (reloadlump&0xFFFF);

  lumpcache = wadfiles[reloadlump>>16]->lumpcache;

  for (i=reloadlump ;
       i<reloadlump+lumpcount ;
       i++,lump_p++, fileinfo++)
    {
      if (lumpcache[i])
	Z_Free(lumpcache[i]);
      
      lump_p->position = LONG(fileinfo->filepos);
      lump_p->size = LONG(fileinfo->size);
    }
  
  close(handle);
}
*/

// -----------------------------------------------------
// FindNumForName
// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int WadFile::FindNumForName(const char *name, int startlump = 0)
{
  union {
    char s[9];
    int  x[2];
  } name8;

  unsigned int j;
  int         v1, v2;
  lumpinfo_t* lump_p;

  // make the name into two integers for easy compares
  strncpy(name8.s, name, 8);

  // in case the name was a fill 8 chars
  name8.s[8] = 0;

  // case insensitive
  strupr(name8.s);

  v1 = name8.x[0];
  v2 = name8.x[1];

  lump_p = lumpinfo + startlump;

  for (j = startlump; j<numlumps; j++, lump_p++) {
    if (*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2) return j; 
  }
  // not found
  return -1;
}


// -----------------------------------------------------
// LoadDehackedLumps
// search for DEHACKED lumps in a loaded wad and process them

void WadFile::LoadDehackedLumps(void)
{
  // just the lump number, nothing else
  int clump = 0;
    
  while (1)
    { 
      clump = FindNumForName("DEHACKED", clump);
      if (clump == -1)
	break;
      CONS_Printf("Loading DEHACKED lump %d from %s\n", clump, filename.c_str());
      //DEH_LoadDehackedLump(clump);
      {
	MYFILE f;
	lumpinfo_t* l = lumpinfo + clump;
	f.size = l->size;
	f.data = (char*)Z_Malloc(f.size + 1, PU_STATIC, 0);
	lseek(handle, l->position, SEEK_SET);
	read(handle, f.data, l->size);

	f.curpos = f.data;
	f.data[f.size] = 0;

	DEH_LoadDehackedFile(&f);
	Z_Free(f.data);
      }
      clump++;
    }
}

// --------------------------------------------------------------------------
// W_Profile
// --------------------------------------------------------------------------
//
/*     --------------------- UNUSED ------------------------
int             info[2500][10];
int             profilecount;

void W_Profile(void)
{
    int         i;
    memblock_t* block;
    void*       ptr;
    char        ch;
    FILE*       f;
    int         j;
    char        name[9];


    for (i=0 ; i<numlumps ; i++)
    {
        ptr = lumpcache[i];
        if (!ptr)
        {
            ch = ' ';
            continue;
        }
        else
        {
            block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
            if (block->tag < PU_PURGELEVEL)
                ch = 'S';
            else
                ch = 'P';
        }
        info[i][profilecount] = ch;
    }
    profilecount++;

    f = fopen ("waddump.txt","w");
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
        memcpy (name,lumpinfo[i].name,8);

        for (j=0 ; j<8 ; j++)
            if (!name[j])
                break;

        for ( ; j<8 ; j++)
            name[j] = ' ';

        fprintf (f,"%s ",name);

        for (j=0 ; j<profilecount ; j++)
            fprintf (f,"    %c",info[i][j]);

        fprintf (f,"\n");
    }
    fclose (f);
}

// --------------------------------------------------------------------------
// W_AddFile : the old code kept for reference
// --------------------------------------------------------------------------
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//

int filelen (int handle)
{
    struct stat fileinfo;

    if (fstat (handle,&fileinfo) == -1)
        I_Error ("Error fstating");

    return fileinfo.st_size;
}


int W_AddFile (char *filename)
{
    wadinfo_t           header;
    lumpinfo_t*         lump_p;
    unsigned            i;
    int                 handle;
    int                 length;
    int                 startlump;
    filelump_t*         fileinfo;
    filelump_t          singleinfo;
    int                 storehandle;

    // open the file and add to directory

    // handle reload indicator.
    if (filename[0] == '~')
    {
        filename++;
        reloadname = filename;
        reloadlump = numlumps;
    }

    if ( (handle = open (filename,O_RDONLY | O_BINARY)) == -1)
    {
        CONS_Printf (" couldn't open %s\n",filename);
        return 0;
    }

    CONS_Printf (" adding %s\n",filename);
    startlump = numlumps;

    if (stricmp (filename+strlen(filename)-3, "wad") )
    {
        // single lump file
        fileinfo = &singleinfo;
        singleinfo.filepos = 0;
        singleinfo.size = LONG(filelen(handle));
        FIL_ExtractFileBase (filename, singleinfo.name);
        numlumps++;
    }
    else
    {
        // WAD file
        read (handle, &header, sizeof(header));
        if (strncmp(header.identification,"IWAD",4))
        {
            // Homebrew levels?
            if (strncmp(header.identification,"PWAD",4))
            {
                I_Error ("Wad file %s doesn't have IWAD "
                         "or PWAD id\n", filename);
            }

            // ???modifiedgame = true;
        }
        header.numlumps = LONG(header.numlumps);
        header.infotableofs = LONG(header.infotableofs);
        length = header.numlumps*sizeof(filelump_t);
        fileinfo = alloca (length);
        lseek (handle, header.infotableofs, SEEK_SET);
        read (handle, fileinfo, length);
        numlumps += header.numlumps;
    }


    // Fill in lumpinfo
    lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
        I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? -1 : handle;

    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
    {
        lump_p->handle = storehandle;
        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
        strncpy (lump_p->name, fileinfo->name, 8);
    }

    if (reloadname)
        close (handle);

    return 1;
}
*/
