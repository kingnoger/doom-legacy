// WadFile class definition
// Ville Bergholm

#ifndef wad_h
#define wad_h 1

#include <string>

#include "doomtype.h"


typedef void*  lumpcache_t;

// ========================================================================
// class WadFile
// This class handles all WAD i/o and keeps a cache for the data lumps.
// (almost) no knowledge of a game type is assumed.
class WadFile
{
  // who needs a full encapsulation;)
  friend class FileCache;
private:
  string         filename;
  int            handle;       // file descriptor
  ULONG          filesize;     // for network
  unsigned int   numlumps;     // this wad's number of resources (was just int)
  lumpinfo_t    *lumpinfo;     // lump headers
  lumpcache_t   *lumpcache;    // memory cache for wad data
  GlidePatch_t  *hwrcache;     // patches are cached in renderer's native format
  unsigned char  md5sum[16];

public:

  // open a new wadfile
  WadFile(const char *fname, int num);
  
  // close a wadfile
  ~WadFile();

  void *operator new(size_t size);
  void  operator delete(void *mem);

  // flushes certain caches (used when a new wad is added)
  //int Replace(void);

  // not used at the moment
  //void Reload(void);

  // process any DeHackEd lumps in this wad
  void LoadDehackedLumps();

  int FindNumForName(const char* name, int startlump = 0);

  // TODO: Maybe sort the wad directory to speed up searches,
  // remove the 16:16 lump numbering system?
  // move functionality from FileCache to here?
};

#endif
