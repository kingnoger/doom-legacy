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
// Revision 1.2  2004/06/27 10:50:35  hurdler
// new renderer things which will not break everyting else
//
// Revision 1.1  2004/06/02 21:05:55  hurdler
// change the way polygons are managed (early implementation)
//
//
//-----------------------------------------------------------------------------

#ifndef hwr_geometry_h
#define hwr_geometry_h 1

#include "hwr_states.h"

/**
  \brief Geometry class
*/
class Geometry
{
public:
    enum GeometryAttributes
    {
        VERTEX_ARRAY = 0,
        TEXCOORD_ARRAY,
        TEXCOORD_ARRAY0 = TEXCOORD_ARRAY,
        TEXCOORD_ARRAY1,
        TEXCOORD_ARRAY2,
        TEXCOORD_ARRAY3,
        TEXCOORD_ARRAY4,
        TEXCOORD_ARRAY5,
        TEXCOORD_ARRAY6,
        TEXCOORD_ARRAY7,
        COLOR_ARRAY,
        NORMAL_ARRAY,
        NUM_GEOMETRY_ATTRIBUTES
    };

private:

    GLfloat *vertex_array;
    GLfloat *tex_coord_arrays[State::MAX_TEXTURE_UNITS];
    GLfloat *normal_array;
    GLuint *color_array;
    GLushort *indices;
    int *primitive_length;
    int *primitive_type;
    GLushort num_primitives;

    static GLfloat *last_vertex_array;
    static GLfloat *last_tex_coord_arrays[State::MAX_TEXTURE_UNITS];
    static GLfloat *last_normal_array;
    static GLuint *last_color_array;

    void EnableArrays();

public:

    /// Create a Geometry.
    Geometry();
    /// Destroy a Geometry.
    ~Geometry();
    /// Set the length of each primitive of the Geometry.
    void SetPrimitiveLength(int *length);
    /// Set the primitive type (GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN) of each Geometry's primitives.
    void SetPrimitiveType(int *type);
    /// Set the number of primitive in the Geometry.
    void SetNumPrimitives(int num);
    /// Change one of the attributes (VERTEX_ARRAY, COLOR_ARRAY, NORMAL_ARRAY, TEXCOORD_ARRAY, TEXCOORD_ARRAYi) of the Geometry.
    void SetAttributes(GeometryAttributes attr, void *array);
    /// Set the indices array. If null, we will assume indices are 0, 1, 2, ...
    void SetIndices(GLushort *indices);
    /// Create a basic textured rectangle
    void CreateTexturedRectangle(bool overwrite = false, float x1 = 0.0f, float y1 = 0.0f, float x2 = 1.0f, float y2 = 1.0f, float z = 0.0f);
    /// Change a vertex in the arrays
    inline void SetTexturedVertex(int which, float x, float y, float z, float s, float t);
    /// Draw the Geometry
    void Draw();
    /// Disbale all arrays
    static void DisableArrays();
};

inline void Geometry::SetTexturedVertex(int which, float x, float y, float z, float s, float t)
{
    GLfloat *vertex = &vertex_array[which * 3];
    vertex[0] = x;
    vertex[1] = y;
    vertex[2] = z;
    GLfloat *tex_coords = &tex_coord_arrays[0][which * 2];
    tex_coords[0] = s;
    tex_coords[1] = t;
}

#endif
