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
// Revision 1.1  2004/05/29 15:30:57  hurdler
// change the way states are managed (early implementation)
//
//
//-----------------------------------------------------------------------------

#ifndef hwr_states_h
#define hwr_states_h 1

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>

// undef CG_SHADER if you don't want to compile Legacy with Cg
#define CG_SHADER
#ifdef CG_SHADER
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#endif

#include <string>

/**
  \brief TextureModifier class
*/
class TextureModifier
{
private:

    GLint gen_s;
    GLint gen_t;
    GLint env_mode;
    GLint wrap_s;
    GLint wrap_t;
    GLint min_filter;
    GLint mag_filter;

    static TextureModifier tm;

public:

    /// Create a TextureModifier using OpenGL defaults for TexGen, TexEnv and TexParameters
    TextureModifier();
    /// Destroy the TextureModifier
    ~TextureModifier();
    /// Change the texture coordinate generation mode
    void SetTexGenMode(GLint s, GLint t);
    /// Change the texture environment mode
    void SetTexEnvMode(GLint env);
    /// Change the texture wrap mode
    void SetWrapMode(GLint s, GLint t);
    /// Change the texture filter mode
    void SetFilterMode(GLint min, GLint mag);
    /// Apply the TextureModifier as the current OpenGL TexGen, TexEnv and TexParameters mode
    void Apply();
    /// Apply the default OpenGL TexGen, TexEnv and TexParameters mode
    static void ApplyBasic();
};

/**
  \brief Fog class

  TODO: having an interpolator to switch between two fogs
*/
class Fog
{
private:

    GLfloat color[4];
    GLint mode;
    GLfloat density;
    GLfloat start;
    GLfloat end;

    static Fog fog;

public:

    /// Create a Fog using OpenGL defaults
    Fog();
    /// Destroy the Fog
    ~Fog();
    /// Change the fog color
    void SetFogColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
    /// Change the fog mode
    void SetFogMode(GLint mode);
    /// Change the fog density
    void SetFogDensity(GLfloat density);
    /// Change the fog start/end
    void SetFogStartEnd(GLfloat start, GLfloat end);
    /// Apply the Fog as the current OpenGL fog
    void Apply();
    /// Apply the default OpenGL fog (no fog)
    static void ApplyBasic();
};

#ifdef CG_SHADER
/**
  \brief Shader class
*/
class Shader
{
private:

    static CGcontext cg_context;
    static CGprofile vertex_profile;
    static CGprofile fragment_profile;

    std::string shader_name;
    CGprogram vertex_program;
    CGprogram fragment_program;

    static Shader *shader;

public:

    /// Create a default shader
    Shader(std::string shader_name);
    /// Destroy the Shader
    ~Shader();
    /// Apply the Shader. All polygons drawn after this call will be affected by it
    void Apply();
    /// Apply the default OpenGL Shader, which actually means no shader
    static void ApplyBasic();
};
#endif

/**
  \brief OpenGL state class
*/
class State
{
public:
    static const int MAX_TEXTURE_UNITS = 8;

private:

    GLfloat color[4];
    GLenum blend_func_src;
    GLenum blend_func_dst;
    GLenum alpha_func;
    GLclampf alpha_ref;
    GLenum shade_model;
    GLenum cull_face_mode;
    Fog *fog;
    TextureModifier *texture_modifier[MAX_TEXTURE_UNITS];
#ifdef CG_SHADER
    Shader *shader;
#endif

    static State state;

public:

    static GLfloat last_color[4];
    static GLenum last_blend_func_src;
    static GLenum last_blend_func_dst;
    static GLenum last_alpha_func;
    static GLclampf last_alpha_ref;
    static GLenum last_shade_model;
    static GLenum last_cull_face_mode;
    static Fog *last_fog;
    static TextureModifier *last_texture_modifier[MAX_TEXTURE_UNITS];
    static Shader *last_shader;

    /// Create a graphics state based on OpenGL defaults
    State();
    /// Destroy this graphics state
    ~State();
    /// Set the global color
    void SetColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
    /// Set the blend src and dst functions
    void SetBlendFunc(GLenum src, GLenum dst);
    /// Set the alpha func
    void SetAlphaFunc(GLenum func, GLclampf ref);
    /// Set the shading model
    void SetShadeModel(GLenum);
    /// Set the face culling mode
    void SetCullFace(GLenum mode);
    /// Set the fog
    void SetFog(Fog *fog);
    /// Change the texture modifier (TexGen, TexEnv and TexParameter) of a texture (it depends on the texture unit)
    void SetTextureModifier(int tex_unit, TextureModifier *texture_modifier);
    /// Change the shader (vertex and/or fragment program) associated to this state
    void SetShader(Shader *shader);
    /// Force this state as the new OpenGL states
    void Apply();
    /// Apply the default OpenGL states
    static void ApplyBasic();
};

#endif
