// $Id$
// temporary? Renderer class implementation
// Ville Bergholm

#include "doomdef.h"
#include "r_render.h"
#include "g_map.h"


void Rend::SetMap(Map *m)
{
  CONS_Printf("Rend::SetMap, m == %p\n", m);
  numvertexes = m->numvertexes;
  vertexes = m->vertexes;

  segs = m->segs;

  numsectors = m->numsectors;
  sectors = m->sectors;

  numsubsectors = m->numsubsectors;
  subsectors = m->subsectors;

  numnodes = m->numnodes;
  nodes = m->nodes;

  numlines = m->numlines;
  lines = m->lines;

  sides = m->sides;
}
