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
// Revision 1.5  2004/05/01 23:29:19  hurdler
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

#ifndef r_render_h
#define r_render_h 1

#include "m_fixed.h"
#include "hardware/hw_defs.h"

class PlayerInfo;
class PlayerPawn;
class Actor;
struct vertex_t;
struct seg_t;
struct sector_t;
struct subsector_t;
struct node_t;
struct side_t;
struct line_t;

struct poly_t;
struct drawseg_t;
struct ffloor_t;
struct vissprite_t;
struct polyvertex_t;
struct extrasubsector_t;
struct gr_vissprite_t;
struct pspdef_t;
struct visplane_t;
struct extracolormap_t;

class Rend
{
  friend class spritepres_t; // this is a HACK too, for software renderer
private:
  const class Map *m; // currently rendered Map

  // FIXME temporary this is the dirtiest HACK ever
  // temporarily we have here COPIES of, or pointers to certain Map data members (geometry).
  // these MUST be set every time before rendering begins

  // when the renderer is rewritten, this can be done in a better way (the Map* should be enough)

  int             numvertexes;
  vertex_t*       vertexes;

  seg_t*          segs;

  int             numsectors;
  sector_t       *sectors;

  int             numsubsectors;
  subsector_t*    subsectors;

  int             numnodes;
  node_t*         nodes;

  int             numlines;
  line_t*         lines;

  side_t*         sides;

  // viewpoint
  fixed_t viewx, viewy, viewz;
  float   fviewx, fviewy, fviewz; // same with floats

  angle_t viewangle, aimingangle; // yaw, pitch, roll?

  PlayerPawn *viewplayer; // may be NULL
  Actor      *viewactor;  // may be NULL

public:

  // setting the geometry data pointers
  void SetMap(Map *m);

  // software renderer

  // r_bsp.cpp
  void R_AddLine(seg_t *line);
  bool R_CheckBBox(fixed_t *bspcoord);
  void R_ClipSolidWallSegment(int first, int last);
  void R_ClipPassWallSegment(int first, int last);

  // r_main.cpp
  angle_t R_PointToAngle(fixed_t x, fixed_t y);
  fixed_t R_PointToDist(fixed_t x, fixed_t y);
  fixed_t R_ScaleFromGlobalAngle (angle_t visangle);
  void R_SetupFrame(PlayerInfo *player);

  // r_plane.cpp
  void R_ClearPlanes(int tag);
  visplane_t *R_FindPlane(fixed_t height, int picnum, int lightlevel, fixed_t xoff, fixed_t yoff, extracolormap_t* planecolormap, ffloor_t* ffloor);
  void R_DrawPlanes();
  void R_DrawSinglePlane(visplane_t* pl, bool handlesource);

  // r_segs.cpp
  void R_DrawWallSplats();
  void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2);
  void R_RenderSegLoop();
  void R_StoreWallRange(int start, int stop);

  // r_things.cpp
  void R_SplitSprite(vissprite_t* sprite, Actor* thing);
  void R_DrawPSprite(pspdef_t *psp);
  void R_DrawPlayerSprites();
  void R_CreateDrawNodes();
  void R_DrawMasked();

  sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
               int *floorlightlevel, int *ceilinglightlevel,
               bool back);
  void R_Prep3DFloors(sector_t*  sector);
  void R_Subsector(int num);
  void R_RenderBSPNode (int bspnum);

  void R_RenderPlayerView(PlayerInfo *player);

  void R_RenderThickSideRange(drawseg_t *ds, int x1, int x2, ffloor_t *ffloor);

  void R_AddWallSplat(line_t  *wallline,
              int      sectorside,
              char    *patchname,
              fixed_t  top,
              fixed_t  wallfrac,
              int      flags);

  void R_AddSprites(sector_t *sec, int lightlevel);
  //void R_ProjectSprite (Actor* thing);
  void R_DrawSprite (vissprite_t* spr);

  // hardware renderer
  // hw_main.cpp
  void transform(float *cx, float *cy, float *cz);

  void HWR_AddLine(seg_t *line);
  void HWR_DrawPlayerSprites();
  bool HWR_CheckBBox(fixed_t *bspcoord);
  void HWR_DrawSkyBackground(PlayerPawn *player);
  //void HWR_ProjectSprite(Actor *thing);
  void HWR_AddSprites(sector_t *sec);
  float HWR_ClipViewSegment(int x, polyvertex_t* v1, polyvertex_t* v2);
  void HWR_ClipPassWallSegment(int first, int last);
  void HWR_ClipSolidWallSegment(int first, int last);
  void HWR_Render3DWater();

  void HWR_SubsecPoly(int num, poly_t* poly);
  void WalkBSPNode (int bspnum, poly_t* poly, unsigned short* leafnode, fixed_t *bbox);

  void AdjustSegs();
  void SearchSegInBSP(int bspnum,polyvertex_t *p,poly_t *poly);
  int  SolveTProblem();
  void HWR_CreatePlanePolygons(int bspnum);

  void HWR_CheckSubsector(int num, fixed_t *bbox);
  void HWR_ComputeLightMapsInBSPNode(int bspnum, fixed_t *bbox);
  void HWR_CreateStaticLightmaps(int bspnum);

  void HWR_RenderPlane(extrasubsector_t *xsub, fixed_t fixedheight,
               FBITFIELD PolyFlags, int lightlevel);
  void HWR_SplitWall(sector_t *sector, wallVert3D *wallVerts, int texnum,
               FSurfaceInfo *Surf, int cutflag);
  void HWR_StoreWallRange(int startfrac, int endfrac);
  void HWR_Subsector(int num);
  void HWR_RenderBSPNode(int bspnum);
  void HWR_DrawSprite(gr_vissprite_t *spr);
  void HWR_DrawPSprite(pspdef_t *psp, int lightlevel);
  void HWR_RenderPlayerView(int viewnumber, PlayerInfo *player);

  void releaseLineChains();
  void generateStacklist(sector_t *thisSector);
  void freeStacklists();
  bool areToptexturesMissing(sector_t *thisSector);
  bool areBottomtexturesMissing(sector_t *thisSector);
  void HWR_CorrectSWTricks();
};

extern Rend R;

#endif
