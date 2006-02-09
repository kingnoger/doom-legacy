// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006 by DooM Legacy Team.
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


#ifndef oglrenderer_hpp_
#define oglrenderer_hpp_

#include<SDL/SDL.h>
#include<GL/gl.h>
#include<GL/glu.h>

#include"r_defs.h"

class PlayerInfo;

// This is an attempt at creating a new OpenGL renderer for Legacy.
// The basic idea is that ALL rendering related stuff is kept inside
// this one class. Avoid intertwingling at all costs! Otherwise we
// just get a huge mess.

// There are two basic rendering modes. The first is the 3D mode for
// rendering level graphics. The second is 2D mode for drawing HUD
// graphics, menus, the console etc.

// The renderer should work regardless of screen aspect ratio. This
// should make people with widescreen monitors happy.

class OGLRenderer {

private:

  double x, y, z; // Location of camera.
  double theta;   // Rotation angle of camera in degrees.
  double phi;     // Up-down rotation angle of camera in degrees.

  double fov;     // Field of view in degrees.

  bool consolemode; // Are we drawing 3D level graphics or 2D console
		    // graphics.

  double hudar;     // HUD aspect ratio.
  double screenar;  // Aspect ratio of the physical screen (monitor).

  int viewportw; // Width of current viewport in pixels.
  int viewporth; // Height of current viewport in pixels.

  SDL_Surface *screen; // Main screen turn on.

  bool workinggl;  // Do we have a working OpenGL context?

  gllevel_t l; // All we need to render the level geometry.

  void RenderBSP(); // Walk through the BSP and render walls and things.
  void RenderGLSubsector(int num);
  void RenderGLSeg(int num);
  void DrawSingleQuad(vertex_t *fv, vertex_t *tv, GLfloat lower, GLfloat upper, GLfloat texleft=0.0, GLfloat texright=1.0, GLfloat textop=0.0, GLfloat texbottom=1.0);

public:

  OGLRenderer();
  ~OGLRenderer();
  bool InitVideoMode(const int w, const int h, const bool fullscreen);

  void InitGLState();
  void StartFrame();
  void FinishFrame();

  bool ReadyToDraw() const { return workinggl; } // Console tries to draw to screen before video is initialized.

  bool In2DMode() const {return consolemode;}

  void Setup2DMode();
  void Draw2DGraphic(GLfloat top, GLfloat left, GLfloat bottom, GLfloat right, GLuint tex, GLfloat textop=1.0, GLfloat texbottom=0.0, GLfloat texleft=0.0, GLfloat texright=1.0);
  void Draw2DGraphic_Doom(float x, float y, float width, float height, GLuint tex);
  void Draw2DGraphicFill_Doom(float x, float y, float width, float height, float texwidth, float texheight, GLuint tex);

  void Setup3DMode();

  void SetLevelData(gllevel_t &in) {l = in;}

  void Render3DView(PlayerInfo *player);

};


#endif
