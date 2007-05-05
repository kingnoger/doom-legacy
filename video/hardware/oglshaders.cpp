// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2007 by DooM Legacy Team.
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
/// \brief GLSL shader implementation.

#include "hardware/oglshaders.h"

#ifdef GL_VERSION_2_0  // GLSL is introduced in OpenGL 2.0

#include <GL/glu.h>
#include "doomdef.h"
#include "g_map.h"
#include "w_wad.h"
#include "z_zone.h"
#include "hardware/oglrenderer.hpp"

shader_cache_t shaders;
shaderprog_cache_t shaderprogs;


static void PrintError()
{
  GLenum err = glGetError();
  while (err != GL_NO_ERROR)
    {
      CONS_Printf("OpenGL error: %s\n", gluErrorString(err));
      err = glGetError();
    }
}




Shader::Shader(const char *n, bool vertex_shader)
  : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
  type = vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

  int lump = fc.FindNumForName(name);
  if (lump < 0)
    return;

  int len = fc.LumpLength(lump);
  char *code = static_cast<char*>(fc.CacheLumpNum(lump, PU_DAVE));
  Compile(code, len);
  Z_Free(code);
}

Shader::Shader(const char *n, const char *code, bool vertex_shader)
  : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
  type = vertex_shader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
  Compile(code, strlen(code));
}


Shader::~Shader()
{
  if (shader_id != NOSHADER)
    glDeleteShader(shader_id);
}


void Shader::Compile(const char *code, int len)
{
  shader_id = glCreateShader(type);
  PrintError();
  glShaderSource(shader_id, 1, &code, &len);
  PrintError();
  glCompileShader(shader_id);
  PrintError();

  PrintInfoLog();

  GLint status = 0;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
  ready = (status == GL_TRUE);
  if (!ready)
    CONS_Printf("Shader %s would not compile.\n", name);
}

void Shader::PrintInfoLog()
{
  int len = 0;
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &len);

  if (len > 0)
    {
      char *log = static_cast<char*>(Z_Malloc(len, PU_DAVE, NULL));
      int chars = 0;
      glGetShaderInfoLog(shader_id, len, &chars, log);
      CONS_Printf("Shader %s InfoLog:\n%s\n", name, log);
      Z_Free(log);
    }
}




ShaderProg::ShaderProg(const char *n)
  : cacheitem_t(n)
{
  prog_id = glCreateProgram();
  PrintError();
}

ShaderProg::~ShaderProg()
{
  glDeleteProgram(prog_id);
}

void ShaderProg::DisableShaders()
{
  glUseProgram(0);
}

void ShaderProg::AttachShader(Shader *s)
{
  glAttachShader(prog_id, s->GetID());
  PrintError();
}

void ShaderProg::Link()
{
  glLinkProgram(prog_id);
  PrintError();
  if (true)
    {
      // for debugging shaders
      glValidateProgram(prog_id);
      PrintError();
      PrintInfoLog();
      PrintError();
    }

  GLint status = 0;
  glGetProgramiv(prog_id, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
    CONS_Printf("Shader program %s could not be linked.\n", name);

  // find locations for uniform vars (per-primitive vars, ie. only changed outside glBegin()..glEnd())
  // TODO the var names need to be defined (they define the Legacy-shader interface!)
  if ((loc.tex0 = glGetUniformLocation(prog_id, "tex0")) == -1)
    CONS_Printf("var not found!\n");

  if ((loc.tex1 = glGetUniformLocation(prog_id, "tex1")) == -1)
    CONS_Printf("var not found!\n");

  if ((loc.eye_pos = glGetUniformLocation(prog_id, "eye_pos")) == -1)
    CONS_Printf("var not found!\n");

  if ((loc.t = glGetUniformLocation(prog_id, "t")) == -1)
    CONS_Printf("var not found!\n");

  if ((loc.eye_pos = glGetAttribLocation(prog_id, "tangent")) == -1)
    CONS_Printf("var not found!\n");
}

void ShaderProg::Use()
{
  glUseProgram(prog_id);
}

void ShaderProg::SetUniforms()
{
  // only call after linking!
  glUniform1i(loc.tex0, 0);
  glUniform1i(loc.tex1, 1);
  glUniform3f(loc.eye_pos, oglrenderer->x, oglrenderer->y, oglrenderer->z);
  glUniform1f(loc.t, oglrenderer->mp->maptic/60.0);
}

void ShaderProg::SetAttributes(shader_attribs_t *a)
{
  // then vertex attribute vars (TODO vertex attribute arrays) (can be changed anywhere)
  glVertexAttrib3fv(loc.tangent, a->tangent); // FIXME TEST
}

void ShaderProg::PrintInfoLog()
{
  int len = 0;
  glGetProgramiv(prog_id, GL_INFO_LOG_LENGTH, &len);

  if (len > 0)
    {
      char *log = static_cast<char*>(Z_Malloc(len, PU_DAVE, NULL));
      int chars = 0;
      glGetProgramInfoLog(prog_id, len, &chars, log);
      CONS_Printf("Program %s InfoLog:\n%s\n", name, log);
      Z_Free(log);
    }
}

#else
#warning Shaders require OpenGL 2.0!
#endif // GL_VERSION_2_0

