// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2002 by DooM Legacy Team.
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
// WadFile, Wad, Wad3 classes: WAD file I/O
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <sys/stat.h>

#ifndef __WIN32__
//# include <unistd.h>
#endif

#include "doomdef.h"
#include "wad.h"

#include "dehacked.h"
#include "i_system.h"
#include "md5.h"
#include "z_zone.h"


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
// constructor
WadFile::WadFile()
{
  filename = "";
  stream = NULL;
  cache = NULL;
}


// -----------------------------------------------------
// WadFile destructor. Closes the open file.
WadFile::~WadFile()
{
  fclose(stream);
  // TODO: free cache memory, invalidate cache
}


bool WadFile::Load(const char *fname, FILE *str)
{
  // at this point we have already checked the magic number and opened the file
  stream = str;
  filename = fname;

  // get file system info about the file
  struct stat bufstat;
  fstat(fileno(stream), &bufstat);
  filesize = bufstat.st_size;

  // read header
  rewind(str);
  fread(&header, sizeof(wadheader_t), 1, stream);
  // endianness swapping
  header.numentries = LONG(header.numentries);
  header.diroffset = LONG(header.diroffset);

  // set up caching
  cache = (lumpcache_t *)Z_Malloc(header.numentries * sizeof(lumpcache_t), PU_STATIC, NULL);
  memset(cache, 0, header.numentries * sizeof(lumpcache_t));

  // generate md5sum 
  rewind(str);
  md5_stream(stream, md5sum);

  return true;
}


//===========================================================================
// Wad class implementation

// constructor
Wad::Wad()
  : WadFile()
{
  directory = NULL;
}

Wad::~Wad()
{
  fclose(stream);
  // TODO free hwrcache
  Z_Free(directory);
}

// -----------------------------------------------------
// Loads a WAD file, sets up the directory and cache.
// Returns false in case of problem
//
// FIXME! unfortunately needs to know its index in the FileCache vector.

bool Wad::Load(const char *fname, FILE *str, int num)
{
  // common to all WAD files
  if (!WadFile::Load(fname, str))
    return false;

  int i, n;
  n = header.numentries;
  
  waddir_t *p;

  // read wad file directory
  fseek(stream, header.diroffset, SEEK_SET);
  p = directory = (waddir_t *)Z_Malloc(n * sizeof(waddir_t), PU_STATIC, NULL); 
  fread(directory, sizeof(waddir_t), n, stream);  
  // endianness conversion for directory
  for (i=0; i<n; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->size   = LONG(p->size);
    }
    
#ifdef HWRENDER
  //faB: now allocates GlidePatch info structures STATIC from the start,
  //     because these were causing a lot of fragmentation of the heap,
  //     considering they are never freed.
  int length = n * sizeof(GlidePatch_t);
  hwrcache = (GlidePatch_t*)Z_Malloc(length, PU_HWRPATCHINFO, NULL);    //never freed
  // set mipmap.downloaded to false
  memset(hwrcache, 0, length);
  for (i=0; i<n; i++)
    {
      //store the software patch lump number for each GlidePatch
      hwrcache[i].patchlump = (num << 16) + i;
    }
#endif

  char buf[5];
  strncpy(buf, header.magic, 4);
  buf[4] = '\0';

  CONS_Printf("Added %s file %s (%i lumps)\n", buf, filename.c_str(), n);
  LoadDehackedLumps();
  return true;
}


int Wad::GetLumpSize(int i)
{
  return directory[i].size;
}

char *Wad::GetLumpName(int i)
{
  return directory[i].name;
}


int Wad::ReadLumpHeader(int lump, void *dest, int size)
{
  waddir_t *l = directory + lump;

  // empty resource (usually markers like S_START, F_END ..)
  if (l->size == 0)
    return 0;
  
  // 0 size means read all the lump
  if (size == 0 || size > l->size)
    size = l->size;
    
  fseek(stream, l->offset, SEEK_SET);

  return fread(dest, 1, size, stream); 
}


void Wad::ListDir()
{
  int i;
  waddir_t *p = directory;
  for (i=0; i<header.numentries; i++, p++)
    printf("%-8s\n", p->name);
}

// -----------------------------------------------------
// FindNumForName
// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int Wad::FindNumForName(const char *name, int startlump)
{
  union {
    char s[9];
    int  x[2];
  } name8;

  int j;
  int v1, v2;

  // make the name into two integers for easy compares
  strncpy(name8.s, name, 8);

  // in case the name was a fill 8 chars
  name8.s[8] = 0;

  // case insensitive
  strupr(name8.s);

  v1 = name8.x[0];
  v2 = name8.x[1];

  waddir_t *p = directory + startlump;

  for (j = startlump; j < header.numentries; j++, p++)
    if (*(int *)p->name == v1 && *(int *)&p->name[4] == v2)
      return j; 

  // not found
  return -1;
}


// -----------------------------------------------------
// LoadDehackedLumps
// search for DEHACKED lumps in a loaded wad and process them

void Wad::LoadDehackedLumps()
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
	waddir_t *l = directory + clump;
	f.size = l->size;
	f.data = (char*)Z_Malloc(f.size + 1, PU_STATIC, 0);
	fseek(stream, l->offset, SEEK_SET);
	fread(f.data, f.size, 1, stream);

	f.curpos = f.data;
	f.data[f.size] = 0;

	DEH_LoadDehackedFile(&f);
	Z_Free(f.data);
      }
      clump++;
    }
}


//===========================================================================
// Wad3 class implementation


// constructor
Wad3::Wad3()
  : WadFile()
{
  directory = NULL;
}

// destructor
Wad3::~Wad3()
{
  fclose(stream);
  Z_Free(directory);
}


// -----------------------------------------------------
// Loads a WAD2 or WAD3 file, sets up the directory and cache.
// Returns false in case of problem

bool Wad3::Load(const char *fname, FILE *str)
{
  // common to all WAD files
  if (!WadFile::Load(fname, str))
    return false;

  int i, n;
  n = header.numentries;

  wad3dir_t *p;

  // read in directory
  fseek(stream, header.diroffset, SEEK_SET);
  p = directory = (wad3dir_t *)Z_Malloc(n * sizeof(wad3dir_t), PU_STATIC, NULL);
  fread(directory, sizeof(wad3dir_t), n, stream);  
  // endianness conversion for directory
  for (i=0; i<n; i++, p++)
    {
      p->offset = LONG(p->offset);
      p->dsize  = LONG(p->dsize);
      p->size   = LONG(p->size);
    }
    
  // TODO hwrcache?? maybe not. All graphics in WAD2/WAD2
  // files should be in sensible formats.

  char buf[5];
  strncpy(buf, header.magic, 4);
  buf[4] = '\0';

  CONS_Printf("Added %s file %s (%i lumps)\n", buf, filename.c_str(), n);
  return true;
}


int Wad3::GetLumpSize(int i)
{
  return directory[i].size;
}

char *Wad3::GetLumpName(int i)
{
  return directory[i].name;
}


int Wad3::ReadLumpHeader(int lump, void *dest, int size)
{
  wad3dir_t *l = directory + lump;

  if (l->compression)
    return -1;
  
  if (l->size == 0)
    return 0;
  
  // 0 size means read all the lump
  if (size == 0 || size > l->size)
    size = l->size;
    
  fseek(stream, l->offset, SEEK_SET);

  return fread(dest, 1, size, stream);
}


void Wad3::ListDir()
{
  int i;
  wad3dir_t *p = directory;
  for (i=0; i<header.numentries; i++, p++)
    printf("%-16s\n", p->name);
}

// -----------------------------------------------------
// FindNumForName
// Searches the wadfile for lump named 'name', returns the lump number
// if not found, returns -1
int Wad3::FindNumForName(const char *name, int startlump)
{
  union
  {
    char s[16];
    int  x[4];
  }; // name16;

  // make the name into 4 integers for easy compares
  // strncpy pads the target string with zeros if needed
  strncpy(s, name, 16);

  // comparison is case sensitive

  wad3dir_t *p = directory + startlump;

  int j;
  for (j = startlump; j<header.numentries; j++, p++)
    {
      if (*(int *)p->name == x[0] && *(int *)(p->name + 4) == x[1] &&
	  *(int *)(p->name + 8) == x[2] && *(int *)(p->name + 12) == x[3])
	return j; 
    }
  // not found
  return -1;
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
