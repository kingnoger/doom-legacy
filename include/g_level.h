// $Id$
// LevelNode class.
// These are used to build a graph describing connections
// between maps, e.g. which map follows which. A levelnode
// also holds all the necessary information for the intermission/finale
// following the level.

#ifndef g_level_h
#define g_level_h 1

#include <vector>
#include <string>

// (implemented as sparse graphs). These generalize map ordering.
// idea: each map has n "exit methods". These could be
// normal exits, secret exits, portals etc. Each exit method
// corresponds to a destination in the list.
// note! several exits in the Map can "point" to the same exit method,
// exit methods can be remapped to point to new destinations.

// for now it's simple: exit[0] is the normal exit, exit[1] secret exit
// exits are pointers to new levelnodes. NULL means episode ends here.

class LevelNode
{
  friend class GameInfo;
  friend class Map;
public:
  string levelname;
  
  vector<LevelNode *> exit; // destinations for exit methods
  int exittype; // exit type used
  bool done; // has it been finished yet?
  string mapname; // will be a vector of map lumpnames

  int BossDeathKey; // What will boss deaths accomplish? Deprecated.
  // Scripting is a better solution for use-made levels.

public:
  int kills, items, secrets;  // level totals
  int time, partime; // the time it took to complete level, partime (in s)

  // old Doom relics
  int number; // MAP01 == 0, E3M2 == 1 etc.
  string skyname;
  string musicname;

  // intermission / finale data for the level
  int episode; // Episode. Only used to decide which finale/intermission to show.
  string interpic; // intermission background picture lumpname
  string finaletext, finaleflat; // if finaletext is not empty, show finale after level
  // entertext? finale/intermission music lumpnames?

  LevelNode()
    {
      done = false;
      exittype = -1;
    };

  LevelNode(const char *lname, const char *mname, LevelNode *e = NULL, LevelNode *se = NULL)
    {
      levelname = lname;
      mapname = mname;
      done = false;
      exittype = -1;
      exit.push_back(e);
      exit.push_back(se);
    };
  
};

#endif
