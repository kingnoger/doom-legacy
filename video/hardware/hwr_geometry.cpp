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
// Revision 1.1  2004/06/02 21:05:55  hurdler
// change the way polygons are managed (early implementation)
//
//
// DESCRIPTION:
//      manage OpenGL geometry
//
//-----------------------------------------------------------------------------

#include "hardware/hwr_geometry.h"
#include "doomdef.h"

GLfloat *Geometry::last_vertex_array = 0;
GLfloat *Geometry::last_tex_coord_arrays[State::MAX_TEXTURE_UNITS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
GLfloat *Geometry::last_normal_array = 0;
GLuint *Geometry::last_color_array = 0;

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
#if 0 // those delete [] are not safe, but someone has to delete them (should we use a ref count or something like that?)
    delete [] vertex_array;
    delete [] tex_coord_arrays;
    delete [] normal_array;
    delete [] color_array;
    delete [] primitive_length;
    delete [] primitive_type;
    delete [] indices;
#endif
}

void Geometry::EnableArrays()
{
    if (vertex_array)
    {
        if (vertex_array != last_vertex_array)
        {
            glEnableClientState(GL_VERTEX_ARRAY);
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
            if (tex_coord_arrays[i] != last_tex_coord_arrays[i])
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
        if (color_array != last_color_array)
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
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, 0, normal_array);
        }
    }
    else if (last_normal_array)
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    last_normal_array = normal_array;
}

void Geometry::SetPrimitiveLength(int *length)
{
    //delete [] primitive_length;  // this is not safe (see destructor comment)
    primitive_length = length;
}

void Geometry::SetPrimitiveType(int *type)
{
    //delete [] primitive_type;  // this is not safe (see destructor comment)
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
        vertex_array = static_cast<GLfloat *>(array);
    }
    else if ((attr >= TEXCOORD_ARRAY) && (attr < (TEXCOORD_ARRAY + State::MAX_TEXTURE_UNITS)))
    {
        tex_coord_arrays[attr - TEXCOORD_ARRAY] = static_cast<GLfloat *>(array);
    }
    else if (attr == NORMAL_ARRAY)
    {
        normal_array = static_cast<GLfloat *>(array);
    }
    else if (attr == COLOR_ARRAY)
    {
        color_array = static_cast<GLuint *>(array);
    }
    else
    {
        CONS_Printf("Error: Unknown Geometry attribute (%d)\n", attr);
    }
}

void Geometry::SetIndices(GLushort *indices)
{
    //delete [] indices;  // this is not safe (see destructor comment)
    indices = indices;
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
