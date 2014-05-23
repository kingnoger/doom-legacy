// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: wadtool.cpp 739 2010-09-05 11:22:12Z smite-meister $
//
// Copyright (C) 2004-2014 by DooM Legacy Team.
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

/// \file
/// \brief Simple CLI tool for WAD manipulation

#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <malloc.h>

#include "md5.h"

#define VERSION "0.7.0"

// lump list format
#define STR_LUMPLIST_HEADER "    #  lumpname     size (B)\n----------------------------\n"
#define STR_LUMPLIST " %4d  %-8s %12d\n"

// WAD files are always little-endian.
static inline int16_t SWAP_INT16( uint16_t x)
{
    return (int16_t)
     (  (( x & (uint16_t)0x00ffU) << 8)
      | (( x & (uint16_t)0xff00U) >> 8)
     );
}

static inline int32_t SWAP_INT32( uint32_t x)
{
    return (int32_t)
     (  (( x & (uint32_t)0x000000ffUL) << 24)
      | (( x & (uint32_t)0x0000ff00UL) <<  8)
      | (( x & (uint32_t)0x00ff0000UL) >>  8)
      | (( x & (uint32_t)0xff000000UL) >> 24)
     );
}

#ifdef __BIG_ENDIAN__
# define LE_SWAP16(x)  SWAP_INT16(x)
# define LE_SWAP32(x)  SWAP_INT32(x)
# define BE_SWAP16(x)  (x)
# define BE_SWAP32(x)  (x)
#else // little-endian machine
# define LE_SWAP16(x)  (x)
# define LE_SWAP32(x)  (x)
# define BE_SWAP16(x)  SWAP_INT16(x)
# define BE_SWAP32(x)  SWAP_INT32(x)
#endif


using namespace std;

// work directory for wad creation and lump extraction
string workdir = "";


// wad header
struct wadheader_t 
{
  union
  {
    char magic[4];   // "IWAD", "PWAD"
    int  imagic;
  };
  int  numentries; // number of entries in WAD
  int  diroffset;  // offset to WAD directory
};


// a WAD directory entry
struct waddir_t
{
  int  offset;  // file offset of the resource
  int  size;    // size of the resource
  union
  {
    char name[8]; // name of the resource (NUL-padded)
    int  iname[2];
  };
};


static bool TestPadding(char *name, int len)
{
  // TEST padding of lumpnames
  bool warn = false;
  for (int j=0; j<len; j++)
    if (name[j] == 0)
      {
	for (j++; j<len; j++)
	  if (name[j] != 0)
	    {
	      name[j] = 0; // fix it
	      warn = true;
	    }
	break;
      }

  if (warn)
    printf("Warning: Lumpname %s not padded with NULs!\n", name);

  return warn;
}


/// \brief Simplified WAD class for wadtool
class Wad
{
protected:
  string filename; ///< the name of the associated physical file
  FILE *stream;    ///< associated stream
  int   diroffset; ///< offset to file directory
  int   numitems;  ///< number of data items (lumps)

  struct waddir_t *directory;  ///< wad directory

public:
  unsigned char md5sum[16];    ///< checksum for data integrity checks

  // constructor and destructor
  Wad();
  ~Wad();

  /// open a new wadfile
  bool Open(const char *fname);

  // query data item properties
  int GetNumItems() { return numitems; }
  const char *GetItemName(int i) { return directory[i].name; }
  int GetItemSize(int i) { return directory[i].size; }
  void ListItems(bool lumps);

  /// retrieval
  int ReadItemHeader(int item, void *dest, int size = 0);
};


// constructor
Wad::Wad()
{
  stream = NULL;
  directory = NULL;
  diroffset = numitems = 0;
}

Wad::~Wad()
{
  if (directory)
    free(directory);

  if (stream)
    fclose(stream);
}


void Wad::ListItems(bool lumps)
{
  int n = GetNumItems();
  printf(" %d lumps, MD5: ", n);
  for (int i=0; i<16; i++)
    printf("%02x:", md5sum[i]);
  printf("\n\n");

  if (!lumps)
    return;

  printf(STR_LUMPLIST_HEADER);
  char name8[9];
  name8[8] = '\0';

  waddir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
    {
      strncpy(name8, p->name, 8);
      printf(STR_LUMPLIST, i, name8, p->size);
    }
}


int Wad::ReadItemHeader(int lump, void *dest, int size)
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


// read a WAD file from disk
bool Wad::Open(const char *fname)
{
  stream = fopen(fname, "rb");
  if (!stream)
    return false;

  filename = fname;

  // read header
  wadheader_t h;
  fread(&h, sizeof(wadheader_t), 1, stream);

  if (h.imagic != *reinterpret_cast<const int *>("IWAD") &&
      h.imagic != *reinterpret_cast<const int *>("PWAD"))
    {
      printf("Bad WAD magic number!\n");
      fclose(stream);
      stream = NULL;
      return false;
    }

  // endianness swapping
  numitems = LE_SWAP32(h.numentries);
  diroffset = LE_SWAP32(h.diroffset);

  // read wad file directory
  fseek(stream, diroffset, SEEK_SET);
  waddir_t *p = directory = (waddir_t *)malloc(numitems * sizeof(waddir_t)); 
  fread(directory, sizeof(waddir_t), numitems, stream);  

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
    {
      p->offset = LE_SWAP32(p->offset);
      p->size   = LE_SWAP32(p->size);
      TestPadding(p->name, 8);
    }

  // generate md5sum 
  rewind(stream);
  md5_stream(stream, md5sum);

  return true;
}



//=============================================================================

// prints wad contents
int ListWad(const char *wadname)
{
  Wad w;

  if (!w.Open(wadname))
    {
      printf("WAD file '%s' could not be opened!\n", wadname);
      return -1;
    }

  printf("WAD file '%s':\n", wadname);
  w.ListItems(true);

  return 0;
}



// creates a new wad file from the lumps listed in a special "inventory" file
int CreateWad(const char *wadname, const char *inv_name)
{
  // read the inventory file
  FILE *invfile = fopen(inv_name, "rb");
  if (!invfile)
    {
      fprintf(stderr, "Could not open the inventory file %s.\n", inv_name);
      return -1;
    }

  int   len, i;
  vector<waddir_t>  dir;
  vector<string> fnames;

  //char *p = NULL;
  //while ((len = getline(&p, &dummy, invfile)) > 0) {}
  //free(p);

  // read the inventory file
  char p[256];
  while (fgets(p, 256, invfile))
    {
      len = strlen(p);

      for (i=0; i<len && !isspace(p[i]); i++)
	; // pass the lump filename
      if (i == 0)
	{
	  printf("warning: you must give a filename for each lump.\n");
	  continue;
	}
      p[i++] = '\0'; // pass the first ws char

      for ( ; i<len && isspace(p[i]); i++)
	; // pass the ws

      char *lumpname = &p[i];
      for ( ; i<len && !isspace(p[i]); i++) // we're looking for a newline, but windows users will have crap like \r before it
	; // pass the lumpname
      p[i] = '\0';

      int n = strlen(lumpname);
      if (n < 1 || n > 8)
	{
	  printf("warning: lumpname '%s' is not acceptable.\n", lumpname);
	  continue;
	}

      if (p[0] == '-')
	{
	  printf("empty lump: %s\n", lumpname);
	}
      else
	{
	  string lump_filename = workdir + p;
	  if (access(lump_filename.c_str(), R_OK))
	    {
	      printf("warning: lump file '%s' cannot be accessed.\n", p);
	      continue;
	    }
	}

      waddir_t temp;
      strncpy(temp.name, lumpname, 8);
      dir.push_back(temp);
      fnames.push_back(p);
    }
  fclose(invfile);

  len = dir.size(); // number of lumps
  if (len < 1)
    return 0;

  // construct the WAD
  FILE *outfile = fopen(wadname, "wb");
  if (!outfile)
    {
      printf("Could not create file '%s'.\n", wadname);
      return -1;
    }

  // file layout: header, lumps, directory
  wadheader_t h;
  h.imagic = *reinterpret_cast<const int *>("PWAD");
  h.numentries = LE_SWAP32(len);
  h.diroffset = 0; // temporary, will be fixed later

  // write header
  fwrite(&h, sizeof(wadheader_t), 1, outfile);

  // write the lumps
  for (i=0; i<len; i++)
    {
      dir[i].offset = ftell(outfile);
      dir[i].size = 0;

      if (fnames[i] == "-")
	{
	  // separator lump
	  continue;
	}

      string lump_filename = workdir + fnames[i];
      FILE *lumpfile = fopen(lump_filename.c_str(), "rb");
      if (lumpfile)
	{
	  // get file system info about the lumpfile
	  struct stat tempstat;
	  fstat(fileno(lumpfile), &tempstat);
	  int size = dir[i].size = tempstat.st_size;
 
	  // insert the lump
	  void *buf = malloc(size);
	  fread(buf, size, 1, lumpfile);
	  fclose(lumpfile);
	  fwrite(buf, size, 1, outfile);
	  free(buf);
	}
      else
	fprintf(stderr, "Lumpfile %s could not be opened.\n", lump_filename.c_str());
   }

  h.diroffset = LE_SWAP32(ftell(outfile)); // actual directory offset

  // write the directory
  for (i=0; i<len; i++)
    {
      dir[i].offset = LE_SWAP32(dir[i].offset);
      dir[i].size   = LE_SWAP32(dir[i].size);
      fwrite(&dir[i], sizeof(waddir_t), 1, outfile);
    }

  // re-write the header with the correct diroffset
  rewind(outfile);
  fwrite(&h, sizeof(wadheader_t), 1, outfile);

  fclose(outfile);
  return ListWad(wadname); // see if it opens OK
}




int ExtractWad(const char *wadname, int num, char *lumpnames[])
{
  Wad w;

  // open the WAD file
  if (!w.Open(wadname))
    {
      fprintf(stderr, "WAD file '%s' could not be opened!\n", wadname);
      return -1;
    }

  int n = w.GetNumItems();
  printf("Extracting the lumps from WAD file %s\n", wadname);
  w.ListItems(false);

  // wad inventory/log
  string logname = workdir + basename(wadname) + ".log";
  FILE *log = NULL;
  if (!num)
    {
      // create a log file (which can be used as an inventory file when recreating the wad!)
      log = fopen(logname.c_str(), "wb");
      if (!log)
	fprintf(stderr, "Could not create logfile '%s'!\n", logname.c_str());
    }

  char name8[9];
  name8[8] = '\0';    
  int count = 0;
  int ln = 0;

  printf(STR_LUMPLIST_HEADER);

  do {

  int i;
  // extract the lumps into files
  for (i = 0; i < n; i++)
    {
      const char *name = w.GetItemName(i);
      strncpy(name8, name, 8);

      if (num && strcasecmp(name8, lumpnames[ln]))
	continue; // not the correct one

      string lfilename = name8;
      lfilename += ".lmp";

      int size = w.GetItemSize(i);

      printf(STR_LUMPLIST, i, name8, size);
      if (log)
	{
	  if (size == 0)
	    {
	      fprintf(log, "-\t\t%s\n", name8);
	      continue; // do not extract separator lumps...
	    }
	  else
	    fprintf(log, "%-12s\t\t%s\n", lfilename.c_str(), name8);
	}

      // write the lump into a file in the work directory
      string lump_filename = workdir + lfilename;
      FILE *lumpfile = fopen(lump_filename.c_str(), "wb");
      if (lumpfile)
	{
	  void *dest = malloc(size);
	  w.ReadItemHeader(i, dest, 0);
	  fwrite(dest, size, 1, lumpfile);
	  fclose(lumpfile);
	  free(dest);
	  count++;
	}
      else
	fprintf(stderr, "Lumpfile %s could not be created.\n", lump_filename.c_str());

      if (num)
	break; // extract only first instance
    }

  if (num && i == n)
    printf("Lump '%s' not found.\n", lumpnames[ln]);

  } while (++ln < num);

  if (log)
    fclose(log);

  printf("\nDone. Wrote %d lumps.\n", count);
  return 0;
}





int main(int argc, char *argv[])
{
  if (argc < 3 || argv[1][0] != '-')
    {
      printf("\nWADtool: Simple commandline tool for manipulating WAD files.\n"
	     "Version " VERSION "\n"
	     "Copyright 2004-2014 Doom Legacy Team.\n\n"
	     "Usage:\n"
	     "  wadtool -l <wadfile>\t\t\t\t\tLists the contents of the WAD.\n"
	     "  wadtool -c <wadfile> [-d <dir>] <inventoryfile>\tConstructs a new WAD using the given inventory file.\n"
	     "  wadtool -x <wadfile> [-d <dir>] [<lumpname> ...]\tExtracts the given lumps.\n"
	     "    If no lumpnames are given, extracts the entire contents of the WAD.\n\n"
	     "  Lump extraction and wad construction use the working directory for storing the lumps.\n"
	     "  Alternatively another directory can be given using the -d switch.\n");
      return -1;
    }

  int c, ret = -1;
  const char *wadfile = NULL;
  char func = ' ';
  workdir = getcwd(NULL, 0); // default: current working directory

  // commandline parameter handling using getopt
  opterr = 0;
  while ((c = getopt(argc, argv, "l:c:x:d:")) != -1)
    switch (c)
      {
      case 'l':
      case 'c':
      case 'x':
	if (func != ' ')
	  fprintf(stderr, "Only one main option (lcx) per invocation.\n");
	else
	  {
	    wadfile = optarg;
	    func = c;
	  }
	break;

      case 'd': // set work directory
	workdir = optarg;
	break;

      case '?':  // unknown option or missing parameter
	switch (optopt)
	  {
	  case 'l':
	    fprintf(stderr, "Usage: wadtool -l <wadfile>\n");
	    break;

	  case 'c':
	    fprintf(stderr, "Usage: wadtool -c <wadfile> [-d <dir>] <inventoryfile>\n");
	    break;

	  case 'x':
	    fprintf(stderr, "Usage: wadtool -x <wadfile> [-d <dir>] [<lumpname> ...]\n");
	    break;

	  default:
	    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
	  }
	return 1;

      default:
	abort();
      }

  // add a slash to the end of the working directory name to be sure
  workdir += '/';
  //printf("working dir: %s\n", workdir.c_str());

  switch (func)
    {
    case 'l':
      ret = ListWad(wadfile);
      break;

    case 'c':
      ret = CreateWad(wadfile, argv[optind]);
      break;

    case 'x':
      ret = ExtractWad(wadfile, argc-optind, &argv[optind]);
      break;

    default:
      fprintf(stderr, "Nothing to do (%c).\n", func);
      return -1;
    }

  return ret;
}
