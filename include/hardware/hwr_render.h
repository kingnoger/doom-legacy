// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// A renderer "class" to give rendering functions access to
// private Map class data (geometry!)
// Renderer is a friend class of Map
//
// $Log$
// Revision 1.1  2004/05/01 23:29:19  hurdler
// add dummy new renderer
//
// Revision 1.4  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.3  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.2  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
//-----------------------------------------------------------------------------

#ifndef hwr_render_h
#define hwr_render_h 1

#include "../r_render.h"

class HWRend
{
public:
  void RenderPlayerView(int viewnumber, PlayerInfo *player);
  void Setup(int bspnum);
};

extern HWRend HWR;

#endif
