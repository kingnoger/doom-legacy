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
// Revision 1.1  2004/05/29 15:30:58  hurdler
// change the way states are managed (early implementation)
//
//
// DESCRIPTION:
//      manage OpenGL graphics states
//
//-----------------------------------------------------------------------------

#include "hardware/hwr_states.h"

// Basic states
TextureModifier TextureModifier::tm;
Fog Fog::fog;
Shader *Shader::shader = 0;  // must be a pointer for the reason explained in the constructor
State State::state;

//-----------------------------------------------------------------------------

TextureModifier::TextureModifier():
    gen_s(GL_FALSE),
    gen_t(GL_FALSE),
    env_mode(GL_MODULATE),
    wrap_s(GL_REPEAT),
    wrap_t(GL_REPEAT),
    min_filter(GL_NEAREST),
    mag_filter(GL_NEAREST)
{
}

TextureModifier::~TextureModifier()
{
}

void TextureModifier::SetTexGenMode(GLint s, GLint t)
{
    gen_s = s;
    gen_t = t;
}

void TextureModifier::SetTexEnvMode(GLint env)
{
    env_mode = env;
}

void TextureModifier::SetWrapMode(GLint s, GLint t)
{
    wrap_s = s;
    wrap_t = t;
}

void TextureModifier::SetFilterMode(GLint min, GLint mag)
{
    min_filter = min;
    mag_filter = mag;
}

void TextureModifier::Apply()
{
    // TODO: TexGen and TexEnv are global to all textures while TexParameter is local to the bound
    //       texture. That means TexParameter should only be called when creating the texture, while
    //       TexGen and TexEnv must be called if TexGen and TexEnv mode are different than the one
    //       used for the previously applied texture.
    if (gen_s != GL_FALSE)
    {
        glEnable(GL_TEXTURE_GEN_S);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, gen_s);
    }
    else
    {
        glDisable(GL_TEXTURE_GEN_S);
    }
    if (gen_t != GL_FALSE)
    {
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, gen_t);
    }
    else
    {
        glDisable(GL_TEXTURE_GEN_T);
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env_mode);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void TextureModifier::ApplyBasic()
{
    tm.Apply();
}

//-----------------------------------------------------------------------------

Fog::Fog():
    mode(GL_FALSE),
    density(1.0f),
    start(0.0f),
    end(1.0f)
{
    memset(color, 0 , sizeof(color));
}

Fog::~Fog()
{
}

void Fog::SetFogColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
}

void Fog::SetFogMode(GLint mode)
{
    this->mode = mode;
}

void Fog::SetFogDensity(GLfloat density)
{
    this->density = density;
}

void Fog::SetFogStartEnd(GLfloat start, GLfloat end)
{
    this->start = start;
    this->end = end;
}

void Fog::Apply()
{
    if (mode != GL_FALSE)
    {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, mode);
        glFogf(GL_FOG_DENSITY, density);
        if (mode == GL_LINEAR)
        {
            glFogf(GL_FOG_START, start);
            glFogf(GL_FOG_END, end);
        }
        glFogfv(GL_FOG_COLOR, color);
    }
    else
    {
        glDisable(GL_FOG);
    }
}

void Fog::ApplyBasic()
{
    fog.Apply();
}

//-----------------------------------------------------------------------------

#ifdef CG_SHADER

CGcontext Shader::cg_context = 0;
CGprofile Shader::vertex_profile;
CGprofile Shader::fragment_profile;

Shader::Shader(std::string shader_name)
{
    // Be carefull: Cg needs a valid OpenGL context (be sure no shader is created before we have an OpenGL window)
    if (!cg_context)
    {
        cg_context = cgCreateContext();
        vertex_profile = cgGLGetLatestProfile(CG_GL_VERTEX);      // auto-detect best vertex profile
        cgGLSetOptimalOptions(vertex_profile);
        fragment_profile = cgGLGetLatestProfile(CG_GL_FRAGMENT);  // auto-detect best fragment profile
        cgGLSetOptimalOptions(fragment_profile);
    }
    if (shader_name.length())
    {
        vertex_program = cgCreateProgramFromFile(cg_context, CG_SOURCE, shader_name.c_str(), vertex_profile, "VertexProgram", 0);
        CGerror vertex_error = cgGetError();
        if (vertex_error != CG_NO_ERROR)
        {
            printf("Warning: No valid vertex program found in %s: %s\n", shader_name.c_str(), cgGetErrorString(vertex_error));
            vertex_program = 0;
        }
        else
        {
            cgGLLoadProgram(vertex_program);
        }
        fragment_program = cgCreateProgramFromFile(cg_context, CG_SOURCE, shader_name.c_str(), fragment_profile, "FragmentProgram", 0);
        CGerror fragment_error = cgGetError();
        if (fragment_error != CG_NO_ERROR)
        {
            printf("Warning: No valid fragment program found in %s: %s\n", shader_name.c_str(), cgGetErrorString(fragment_error));
            fragment_program = 0;
        }
        else
        {
            cgGLLoadProgram(fragment_program);
        }
    }
}

Shader::~Shader()
{
}

void Shader::Apply()
{
    if (vertex_program)
    {
        cgGLSetStateMatrixParameter(cgGetNamedParameter(vertex_program, "ModelViewProj"), CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
        cgGLBindProgram(vertex_program);
        cgGLEnableProfile(vertex_profile);
    }
    else
    {
        cgGLDisableProfile(vertex_profile);
    }
    if (fragment_program)
    {
        GLint handle;
        CGparameter param;

        glGetIntegerv(GL_TEXTURE_BINDING_2D, &handle);
        param = cgGetNamedParameter(fragment_program, "Decal");
        cgGLSetTextureParameter(param, handle);
        cgGLEnableTextureParameter(param);

        cgGLBindProgram(fragment_program);
        cgGLEnableProfile(fragment_profile);
    }
    else
    {
        cgGLDisableProfile(fragment_profile);
    }
}

void Shader::ApplyBasic()
{
    if (!shader)
    {
        shader = new Shader("");
    }
    shader->Apply();
}
#endif

//-----------------------------------------------------------------------------

GLfloat State::last_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
GLenum State::last_blend_func_src = GL_ONE;
GLenum State::last_blend_func_dst = GL_ZERO;
GLenum State::last_alpha_func = GL_ALWAYS;
GLclampf State::last_alpha_ref = 0.0f;
GLenum State::last_shade_model = GL_SMOOTH;
GLenum State::last_cull_face_mode = GL_FALSE;
Fog *State::last_fog = 0;
TextureModifier *State::last_texture_modifier[MAX_TEXTURE_UNITS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
Shader *State::last_shader = 0;

State::State():
    blend_func_src(GL_ONE),
    blend_func_dst(GL_ZERO),
    alpha_func(GL_ALWAYS),
    alpha_ref(0.0f),
    shade_model(GL_SMOOTH),
    cull_face_mode(GL_FALSE),
    fog(0),
    shader(0)
{
    memset(color, 0, sizeof(color));
    memset(texture_modifier, 0, sizeof(texture_modifier));
}

State::~State()
{
}

void State::SetColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
}

void State::SetBlendFunc(GLenum src, GLenum dst)
{
    blend_func_src = src;
    blend_func_dst = dst;
}

void State::SetAlphaFunc(GLenum func, GLclampf ref)
{
    alpha_func = func;
    alpha_ref = ref;
}

void State::SetShadeModel(GLenum model)
{
    shade_model = model;
}

void State::SetCullFace(GLenum mode)
{
    cull_face_mode = mode;
}

void State::SetFog(Fog *fog)
{
    this->fog = fog;
}

void State::SetTextureModifier(int tex_unit, TextureModifier *texture_modifier)
{
    this->texture_modifier[tex_unit] = texture_modifier;
}

void State::SetShader(Shader *shader)
{
    this->shader = shader;
}

void State::Apply()
{
    glColor4fv(color);
    if ((blend_func_src != last_blend_func_src) || (blend_func_dst != last_blend_func_dst))
    {
        last_blend_func_src = blend_func_src;
        last_blend_func_dst = blend_func_dst;
        if ((blend_func_src != GL_ONE) || (blend_func_src != GL_ZERO))
        {
            glEnable(GL_BLEND);
            glBlendFunc(blend_func_src, blend_func_dst);
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }
    if ((alpha_func != last_alpha_func) || (alpha_ref != last_alpha_ref))
    {
        last_alpha_func = alpha_func;
        last_alpha_ref = alpha_ref;
        if (alpha_func != GL_ALWAYS)
        {
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(alpha_func, alpha_ref);
        }
        else
        {
            glDisable(GL_ALPHA_TEST);
        }
    }
    if (shade_model != last_shade_model)
    {
        last_shade_model = shade_model;
        glShadeModel(shade_model);
    }
    if (cull_face_mode != last_cull_face_mode)
    {
        last_cull_face_mode = cull_face_mode;
        if (cull_face_mode != GL_FALSE)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(cull_face_mode);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
    }
    if (fog != last_fog)
    {
        last_fog = fog;
        if (fog)
        {
            fog->Apply();
        }
        else
        {
            Fog::ApplyBasic();
        }
    }
    for (int i=0; i<MAX_TEXTURE_UNITS; i++)
    {
      if (texture_modifier[i] != last_texture_modifier[i])
      {
          last_texture_modifier[i] = texture_modifier[i];
          if (texture_modifier[i])
          {
              texture_modifier[i]->Apply();
          }
          else
          {
              TextureModifier::ApplyBasic();
          }
      }
    }
    if (shader != last_shader)
    {
        last_shader = shader;
        if (shader)
        {
            shader->Apply();
        }
        else
        {
            Shader::ApplyBasic();
        }
    }
}

void State::ApplyBasic()
{
    state.Apply();
}
