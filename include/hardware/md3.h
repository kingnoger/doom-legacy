// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by Ville Bergholm 
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
// Revision 1.2  2004/03/28 15:16:14  smite-meister
// Texture cache.
//
// Revision 1.1  2003/03/23 14:24:14  smite-meister
// Polyobjects, MD3 models
//
//
//
// DESCRIPTION:
//   Quake III MD3 model class for Doom Legacy
//-----------------------------------------------------------------------------


#ifndef md3_h
#define md3_h 1

#include <string>
#include "r_sprite.h"
#include "z_cache.h"
#include "m_fixed.h"

using namespace std;

// MD3 file format is little-endian. Char is 8 bits, Short 16 bits, int 32 bits.
// Floats are 32 bit, vec3_t is three consecutive floats.

typedef float rotation_t[3][3];

typedef float vec3_t[3];

struct MD3_header
{
  char magic[4]; // "IDP3"
  int  version;  // 15 is the latest one?
  char filename[64]; // NUL-terminated string
  int  flags;
  int  numFrames;
  int  numTags;
  int  numMeshes;
  int  numSkins;  // unused?
  int  ofsFrames;
  int  ofsTags;
  int  ofsMeshes;
  int  ofsEOF;
};

struct MD3_frame
{
  vec3_t minBBox;  // bounding box corners
  vec3_t maxBBox;
  vec3_t origin;   // local origin
  float  radius;   // bounding sphere
  char   name[16]; // frame name. NUL-terminated string
};

struct MD3_tag
{
  char name[64]; // tag name. NUL-terminated string
  vec3_t origin; // translation vector
  rotation_t rot; // rotation matrix (3x3)
};

struct MD3_mesh_header
{
  char magic[4]; // "IDP3" again
  char name[64];
  int  flags;
  int  numFrames;   // same as in MD3_header
  int  numShaders;  // shader != skin ?
  int  numVertices;
  int  numTriangles;
  int  ofsTriangles;  // relative offset from MD3_surface start
  int  ofsShaders;
  int  ofsST; // texture coordinates
  int  ofsVertices;
  int  ofsEND;
};

struct MD3_shader
{
  char name[64];
  int  index;
};

struct MD3_triangle
{
  int index[3]; // indices to vertex array
};

struct MD3_texcoord
{
  float s, t; // 0..1
};

struct MD3_vertex
{
  short coord[3];  // 10.6 signed fixed point. multiply by 1.0/64 to make float.
  unsigned char normal[2]; // compressed normal vector, phi and theta polar angles
};

struct MD3_mesh
{
  MD3_mesh_header header;
  MD3_shader   *shaders;
  MD3_triangle *triangles;
  MD3_texcoord *texcoords;
  MD3_vertex   *vertices;
  unsigned int texture; // OpenGL texture number
};


struct MD3_animstate;

class MD3_t
{
protected:
  MD3_header header;
  MD3_frame *frames;
  MD3_tag   *tags;
  MD3_mesh  *meshes;

  //vector<MD3 *> links;
  MD3_t **links; // tag links to other models

public:

  // *.md3
  bool Load(const char *filename);
  // *.skin
  bool LoadSkinFile(const string & path, const string & filename);
  void LoadMeshTexture(const char *meshname, const char *imagefile);

  // attach another model to a tag
  void Link(const char *tagname, MD3_t *m);

  // draw one frame
  void DrawFrame(int frame);

  // draw this model in an arbitrary state
  void DrawInterpolated(MD3_animstate *st);

  // draw this model and all the models that have been linked to it
  MD3_animstate *DrawRecursive(MD3_animstate *st);
};

struct MD3_anim
{
  int firstframe;
  int lastframe;
  int loopframes;
  float fps;
};


//================================
//           Models
//================================


// Player model. Consists of three submodels.
class MD3_player : public cacheitem_t
{
  friend class modelcache_t;
  friend class modelpres_t;
protected:
  MD3_anim anim[MAX_ANIMATIONS]; // FIXME check the max number of sequences...
  MD3_t legs, torso, head;

public:
  virtual ~MD3_player();

  bool Load(const string & path, const string & skin);
  bool LoadAnim(const char *file);
};


class modelcache_t : public L2cache_t
{
protected:
  cacheitem_t *Load(const char *p);

public:
  modelcache_t(memtag_t tag);
  inline MD3_player *Get(const char *p) { return (MD3_player *)Cache(p); };
};


extern modelcache_t models;


#endif
