// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
// Revision 1.6  2004/11/21 12:51:07  hurdler
// might fix "glClientActiveTexture" compilation error on some systems
//
// Revision 1.5  2004/10/31 22:22:13  smite-meister
// Hasta la vista, pic_t!
//
// Revision 1.4  2004/07/23 22:18:42  hurdler
// respect indent style and temporary (static, unoptimized and not correct) support for wall/floor/ceiling so I can actually work on texture support
//
// Revision 1.3  2004/07/07 19:24:43  hurdler
// manage buffers properly
//
// Revision 1.2  2004/06/27 10:50:35  hurdler
// new renderer things which will not break everyting else
//
// Revision 1.1  2004/06/02 21:05:55  hurdler
// change the way polygons are managed (early implementation)
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Manage OpenGL geometry

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "hardware/hwr_geometry.h"
#include "doomdef.h"

#ifdef __WIN32__
# define glClientActiveTexture(x)
# warning glClientActiveTexture must be fixed!
#endif

GLfloat *Geometry::last_vertex_array = 0;
GLfloat *Geometry::last_tex_coord_arrays[State::MAX_TEXTURE_UNITS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
GLfloat *Geometry::last_normal_array = 0;
GLuint *Geometry::last_color_array = 0;
std::map<GLfloat *, int> Geometry::float_refcount;
std::map<GLuint *, int> Geometry::uint_refcount;
std::map<GLushort *, int> Geometry::ushort_refcount;

Geometry::Geometry():
  vertex_array(0),
  normal_array(0),
  color_array(0),
  indices(0),
  primitive_length(0),
  primitive_type(0),
  num_primitives(0)
{
  memset(tex_coord_arrays, 0, sizeof(tex_coord_arrays));
}

Geometry::~Geometry()
{
  UnrefDelete(vertex_array);
  for (int i = 0; i < State::MAX_TEXTURE_UNITS; i++)
    {
      UnrefDelete(tex_coord_arrays[i]);
    }
  UnrefDelete(normal_array);
  UnrefDelete(color_array);
  UnrefDelete(primitive_length);
  UnrefDelete(primitive_type);
  UnrefDelete(indices);
}

void Geometry::EnableArrays()
{
  if (vertex_array)
    {
      if (vertex_array != last_vertex_array)
        {
          glEnableClientState(GL_VERTEX_ARRAY); // TODO: do that only if last_vertex_array was null
          glVertexPointer(3, GL_FLOAT, 0, vertex_array);
        }
    }
  else
    {
      glDisableClientState(GL_VERTEX_ARRAY);
      CONS_Printf("Warning: no vertex array defined.\n");
      return;
    }
  last_vertex_array = vertex_array;

  for (int i = 0; i < State::MAX_TEXTURE_UNITS; i++)
    {
      if (tex_coord_arrays[i])
        {
          if (tex_coord_arrays[i] != last_tex_coord_arrays[i])  // TODO: do that only if last_tex_coord_arrays[i] was null
            {
              glClientActiveTexture(GL_TEXTURE0 + i);
              glEnableClientState(GL_TEXTURE_COORD_ARRAY);
              glTexCoordPointer(2, GL_FLOAT, 0, tex_coord_arrays[i]);
            }
        }
      else if (last_tex_coord_arrays[i])
        {
          glClientActiveTexture(GL_TEXTURE0 + i);
          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
      last_tex_coord_arrays[i] = tex_coord_arrays[i];
    }

  if (color_array)
    {
      if (color_array != last_color_array)  // TODO: do that only if last_color_array was null
        {
          glEnableClientState(GL_COLOR_ARRAY);
          glColorPointer(4, GL_UNSIGNED_BYTE, 0, color_array);
        }
    }
  else if (last_color_array)
    {
      glDisableClientState(GL_COLOR_ARRAY);
    }
  last_color_array = color_array;

  if (normal_array)
    {
      if (normal_array != last_normal_array)
        {
          glEnableClientState(GL_NORMAL_ARRAY);  // TODO: do that only if last_normal_array was null
          glNormalPointer(GL_FLOAT, 0, normal_array);
        }
    }
  else if (last_normal_array)
    {
      glDisableClientState(GL_NORMAL_ARRAY);
    }
  last_normal_array = normal_array;
}

void Geometry::SetPrimitiveLength(GLuint *length)
{
  Ref(length);
  UnrefDelete(primitive_length);
  primitive_length = length;
}

void Geometry::SetPrimitiveType(GLuint *type)
{
  Ref(type);
  UnrefDelete(primitive_type);
  primitive_type = type;
}

void Geometry::SetNumPrimitives(int num)
{
  num_primitives = num;
}

void Geometry::SetAttributes(GeometryAttributes attr, void *array)
{
  if (attr == VERTEX_ARRAY)
    {
      Ref(static_cast<GLfloat *>(array));
      UnrefDelete(vertex_array);
      vertex_array = static_cast<GLfloat *>(array);
    }
  else if ((attr >= TEXCOORD_ARRAY) && (attr < (TEXCOORD_ARRAY + State::MAX_TEXTURE_UNITS)))
    {
      Ref(static_cast<GLfloat *>(array));
      UnrefDelete(tex_coord_arrays[attr - TEXCOORD_ARRAY]);
      tex_coord_arrays[attr - TEXCOORD_ARRAY] = static_cast<GLfloat *>(array);
    }
  else if (attr == NORMAL_ARRAY)
  {
      Ref(static_cast<GLfloat *>(array));
      UnrefDelete(normal_array);
      normal_array = static_cast<GLfloat *>(array);
    }
  else if (attr == COLOR_ARRAY)
    {
      Ref(static_cast<GLuint *>(array));
      UnrefDelete(color_array);
      color_array = static_cast<GLuint *>(array);
    }
  else
    {
      CONS_Printf("Error: Unknown Geometry attribute (%d)\n", attr);
    }
}

void Geometry::SetIndices(GLushort *indices)
{
  Ref(indices);
  UnrefDelete(this->indices);
  this->indices = indices;
}

void Geometry::CreateTexturedRectangle(bool overwrite, float x1, float y1, float x2, float y2, float z)
{
  if (!overwrite)
    {
      num_primitives = 1;

      UnrefDelete(primitive_length);
      primitive_length = new GLuint(4);
      Ref(primitive_length);

      UnrefDelete(primitive_type);
      primitive_type = new GLuint(GL_TRIANGLE_STRIP);
      Ref(primitive_type);

      UnrefDelete(vertex_array);
      vertex_array = new GLfloat[3 * 4];
      Ref(vertex_array);

      UnrefDelete(tex_coord_arrays[0]);
      GLfloat *tex_coord_array = tex_coord_arrays[0] = new GLfloat[2 * 4];
      tex_coord_array[0] = 0.0f; tex_coord_array[1] = 0.0f;
      tex_coord_array[2] = 0.0f; tex_coord_array[3] = 1.0f;
      tex_coord_array[4] = 1.0f; tex_coord_array[5] = 0.0f;
      tex_coord_array[6] = 1.0f; tex_coord_array[7] = 1.0f;
      Ref(tex_coord_arrays[0]);

      UnrefDelete(indices);
      indices = new GLushort[4];
      indices[0] = 0; indices[1] = 1; indices[2] = 2; indices[3] = 3;
      Ref(indices);
    }
  vertex_array[ 0] = x1; vertex_array[ 1] = y1; vertex_array[ 2] = z;
  vertex_array[ 3] = x1; vertex_array[ 4] = y2; vertex_array[ 5] = z;
  vertex_array[ 6] = x2; vertex_array[ 7] = y1; vertex_array[ 8] = z;
  vertex_array[ 9] = x2; vertex_array[10] = y2; vertex_array[11] = z;
}

void Geometry::DisableArrays()
{
  if (last_vertex_array)
    {
      glDisableClientState(GL_VERTEX_ARRAY);
    }
  if (last_color_array)
    {
      glDisableClientState(GL_COLOR_ARRAY);
    }
  if (last_normal_array)
    {
      glDisableClientState(GL_NORMAL_ARRAY);
    }
  for (int i = 0; i < State::MAX_TEXTURE_UNITS; i++)
    {
      if (last_tex_coord_arrays[i])
        {
          glClientActiveTexture(GL_TEXTURE0 + i);
          glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }
}

void Geometry::FreeAllBuffers()
{
  for (std::map<GLfloat *, int>::iterator float_it = float_refcount.begin(); float_it != float_refcount.end(); ++float_it)
    {
      if ((float_it->second <=0) && float_it->first)
        {
          delete [] float_it->first;
        }
    }
  for (std::map<GLuint *, int>::iterator uint_it = uint_refcount.begin(); uint_it != uint_refcount.end(); ++uint_it)
    {
      if ((uint_it->second <= 0) && uint_it->first)
        {
          delete [] uint_it->first;
        }
    }
  for (std::map<GLushort *, int>::iterator ushort_it = ushort_refcount.begin(); ushort_it != ushort_refcount.end(); ++ushort_it)
    {
      if ((ushort_it->second <= 0) && ushort_it->first)
        {
          delete [] ushort_it->first;
        }
    }
}

void Geometry::Draw()
{
  // draw using vertex array
  // TODO: it's not the better solution, try also immediate mode and VBO.
  //       then choose the faster method for low end cards
  EnableArrays();

  GLushort *indices = this->indices;
  for (int i = 0; i < num_primitives; i++)
    {
      glDrawElements(primitive_type ? primitive_type[i] : GL_TRIANGLE_STRIP, primitive_length[i], GL_UNSIGNED_SHORT, indices);
      indices += primitive_length[i];
    }

  //DisableArrays();
}
