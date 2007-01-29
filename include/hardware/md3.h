// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2007 by Doom Legacy Team
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
/// \brief Quake III MD3 model class for Doom Legacy.

#ifndef md3_h
#define md3_h 1

#include <string>
#include "r_sprite.h"


using namespace std;

// MD3 file format is little-endian. Char is 8 bits, Short 16 bits, int 32 bits.
// Floats are 32 bit, vec3_t is three consecutive floats.

typedef float rotation_t[3][3];

typedef float vec3_t[3];

/// \brief Header struct for an MD3 model.
struct MD3_header
{
  char magic[4]; // "IDP3"
  Sint32  version;  // 15 is the latest one?
  char filename[64]; // NUL-terminated string
  Uint32  flags;
  Sint32  numFrames;
  Sint32  numTags;
  Sint32  numMeshes;
  Sint32  numSkins;  // unused?
  Sint32  ofsFrames;
  Sint32  ofsTags;
  Sint32  ofsMeshes;
  Sint32  ofsEOF;
} __attribute__((packed));


/// \brief Single animation frame for an MD3 model.
struct MD3_frame
{
  vec3_t minBBox;
  vec3_t maxBBox;  ///< bounding box corners
  vec3_t origin;   ///< local origin
  float  radius;   ///< bounding sphere radius
  Uint8  name[16]; ///< Frame name. NUL-terminated string.
} __attribute__((packed));


/// \brief Mesh attachment point and rotation for an MD3 model.
struct MD3_tag
{
  char name[64];  ///< Tag name. NUL-terminated string.
  vec3_t origin;  ///< translation vector
  rotation_t rot; ///< rotation matrix (3x3)
} __attribute__((packed));


/// \brief Mesh header for an MD3 model.
struct MD3_mesh_header
{
  char magic[4]; // "IDP3" again
  char name[64];
  Sint32  flags;
  Sint32  numFrames;   // same as in MD3_header
  Sint32  numShaders;  // shader != skin ?
  Sint32  numVertices;
  Sint32  numTriangles;
  Sint32  ofsTriangles;  // relative offset from MD3_surface start
  Sint32  ofsShaders;
  Sint32  ofsST; // texture coordinates
  Sint32  ofsVertices;
  Sint32  ofsEND;
} __attribute__((packed));


/// \brief Mesh shader for an MD3 model.
struct MD3_mesh_shader
{
  char   name[64];
  Sint32 index;
} __attribute__((packed));


/// \brief Mesh triangle for an MD3 model.
struct MD3_mesh_triangle
{
  Sint32 index[3]; ///< indices to vertex array
} __attribute__((packed));


/// \brief Mesh vertex texture coordinates for an MD3 model.
struct MD3_mesh_texcoord
{
  float s, t; ///< 0..1
} __attribute__((packed));


/// \brief Mesh vertex for an MD3 model.
struct MD3_mesh_vertex
{
  Sint16 coord[3];  ///< 10.6 signed fixed point. multiply by 1.0/64 to make float.
  Uint8  normal[2]; ///< compressed normal vector, phi and theta polar angles
} __attribute__((packed));


/// \brief Mesh, a rigid part of an MD3 model.
struct MD3_mesh
{
  MD3_mesh_header    header;
  MD3_mesh_shader   *shaders;
  MD3_mesh_triangle *triangles;
  MD3_mesh_texcoord *texcoords;
  MD3_mesh_vertex   *vertices;
  class Texture *texture; ///< Texture object to be used with this mesh
};



/// \brief MD3 model.
class MD3_t
{
  friend class modelpres_t;
protected:
  byte       *data; ///< all model data in a linear block

  MD3_header  header;
  MD3_frame  *frames;
  MD3_tag    *tags;
  MD3_mesh   *meshes;

  vector<MD3_t *> links; ///< tag links to other models

public:
  MD3_t();
  ~MD3_t();

  /// Read the model from a *.md3 file.
  bool Load(const string& filename);

  /// Read the model *.skin files.
  bool LoadSkinFile(const string& path, const string& filename);

  /// Attach another MD3 model to a tag.
  void Link(const char *tagname, MD3_t *m);

  /// draw one frame
  void DrawFrame(int frame);

  /// draw this model in an arbitrary state
  void DrawInterpolated(struct MD3_animstate *st);

  /// draw this model and all the models that have been linked to it
  MD3_animstate *DrawRecursive(MD3_animstate *st, float pitch);
};



//================================
//           Models
//================================

/// \brief Single animation sequence definition for MD3 models.
struct MD3_anim
{
  int firstframe;
  int lastframe;
  int loopframes;
  float fps;
};

/// \brief MD3 player model. Consists of three MD3 submodels.
class MD3_player : public cacheitem_t
{
  friend class modelcache_t;
  friend class modelpres_t;

protected:
  MD3_anim anim[MD3_animstate::MAX_ANIMATIONS]; // FIXME check the max number of sequences...
  MD3_t legs, torso, head; ///< the submodels

public:
  MD3_player(const char *n);
  virtual ~MD3_player();

  bool Load(const string& path, const string& skin);
  bool LoadAnim(const string& filename);
};


class modelcache_t : public cache_t
{
protected:
  cacheitem_t *Load(const char *p);

public:
  modelcache_t(memtag_t tag);
  inline MD3_player *Get(const char *p) { return (MD3_player *)Cache(p); };
};


extern modelcache_t models;


#endif
