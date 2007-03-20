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
/// \brief GLSL shaders.

#ifndef oglshaders_h
#define oglshaders_h 1

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#ifdef GL_VERSION_2_0  // GLSL is introduced in OpenGL 2.0

#include "z_cache.h"


/// GLSL shader object.
class Shader : public cacheitem_t
{
protected:
  static const GLuint NOSHADER = 0;
  GLuint shader_id; ///< OpenGL handle for the shader.
  GLenum type;      ///< Vertex or fragment shader?
  bool   ready;     ///< Compiled and functional?

public:
  Shader(const char *name, GLenum type);
  ~Shader();

  GLuint GetID() const { return shader_id; }
  bool IsReady() const { return ready; }

  bool Create();
  bool Create(const char *code);

  void Compile(const char *code, int len);

  void PrintInfoLog();
};


/// GLSL Program object.
class ShaderProg : public cacheitem_t
{
protected:
  GLuint prog_id; ///< OpenGL handle for the program.
  /// Uniform variable locations in the linked program.
  struct
  {
    GLint tex0; ///< Texture sampler 0
    GLint t;
  } loc;

public:
  ShaderProg(const char *name);
  ~ShaderProg();

  static void DisableShaders();

  void AttachShader(Shader *s);
  void Link();
  void Use();
  void SetUniforms(float t);

  void PrintInfoLog();
};

#endif // GL_VERSION_2_0
#endif
