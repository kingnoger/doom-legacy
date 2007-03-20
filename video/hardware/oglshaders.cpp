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

#if defined(TEST_SHADERS) && !defined(GL_VERSION_2_0)
#error Shaders require OpenGL 2.0!
#endif

#ifdef GL_VERSION_2_0  // GLSL is introduced in OpenGL 2.0

#include <GL/glu.h>
#include "doomdef.h"
#include "w_wad.h"
#include "z_zone.h"

ShaderProg *sprog = NULL;

// simple test shaders
const char *vshader = 
"void main(){\
gl_TexCoord[0] = gl_MultiTexCoord0;\
gl_Position = ftransform();}";

const char *fshader =
"uniform sampler2D tex0; uniform float t;\
void main(){\
vec4 texel;\
vec2 tc;\
tc.s = gl_TexCoord[0].s+0.1*sin(t+gl_TexCoord[0].t);\
tc.t = gl_TexCoord[0].t+0.1*(1-cos(t+gl_TexCoord[0].s));\
texel = texture2D(tex0, tc);\
gl_FragColor = vec4(texel.rgb, texel.a*abs(sin(t)));}";


void TestShaders()
{
  static bool shaders_done = false;
  if (!shaders_done)
    {
      sprog = new ShaderProg("xxxx");
      Shader *v = new Shader("aaa", GL_VERTEX_SHADER);
      Shader *f = new Shader("bbb", GL_FRAGMENT_SHADER);
      v->Create(vshader);
      f->Create(fshader);

      sprog->AttachShader(v);
      sprog->AttachShader(f);
      sprog->Link();
      sprog->SetUniforms(0);
      shaders_done = true;
    }
}



static void PrintError()
{
  GLenum err = glGetError();
  while (err != GL_NO_ERROR)
    {
      CONS_Printf("OpenGL error: %s\n", gluErrorString(err));
      err = glGetError();
    }
}




Shader::Shader(const char *n, GLenum t)
  : cacheitem_t(n), shader_id(NOSHADER), ready(false)
{
  type = (t == GL_VERTEX_SHADER) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
}

Shader::~Shader()
{
  if (shader_id != NOSHADER)
    glDeleteShader(shader_id);
}

bool Shader::Create()
{
  int lump = fc.FindNumForName(name);
  if (lump < 0)
    return false;

  int len = fc.LumpLength(lump);
  char *code = static_cast<char*>(fc.CacheLumpNum(lump, PU_STATIC));
  Compile(code, len);
  Z_Free(code);
  return true;
}

bool Shader::Create(const char *code)
{
  Compile(code, strlen(code));
  return true;
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
      char *log = static_cast<char*>(Z_Malloc(len, PU_STATIC, NULL));
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

  if ((loc.t = glGetUniformLocation(prog_id, "t")) == -1)
    CONS_Printf("var not found!\n");
}

void ShaderProg::Use()
{
  glUseProgram(prog_id);
}

void ShaderProg::SetUniforms(float t)
{
  // only call after linking!
  glUniform1i(loc.tex0, 0);
  glUniform1f(loc.t, t);

  /*
  // then vertex attribute vars (TODO vertex attribute arrays) (can be changed anywhere)
  GLint loc = glGetAttribLocation(prog_id, const GLchar *varname);
  if (loc != -1)
  glVertexAttrib2f(loc, f1, f2);
  */  
}

void ShaderProg::PrintInfoLog()
{
  int len = 0;
  glGetProgramiv(prog_id, GL_INFO_LOG_LENGTH, &len);

  if (len > 0)
    {
      char *log = static_cast<char*>(Z_Malloc(len, PU_STATIC, NULL));
      int chars = 0;
      glGetProgramInfoLog(prog_id, len, &chars, log);
      CONS_Printf("Program %s InfoLog:\n%s\n", name, log);
      Z_Free(log);
    }
}

#endif // GL_VERSION_2_0
