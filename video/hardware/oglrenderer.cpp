// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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

#include"oglrenderer.hpp"
#include"doomdef.h"
#include"screen.h"

#include"g_map.h"
#include"g_player.h"
#include"g_actor.h"

#include"tables.h"
#include"r_data.h"
#include"r_sprite.h"

#include"r_render.h" // for spritepres_t::Draw()
#include"i_video.h" // for spritepres_t::Draw()


OGLRenderer::OGLRenderer() {
  screen = NULL;
  workinggl = false;

  mp = NULL;
  l.glvertexes = NULL;

  x = y = z = 0.0;
  theta = phi = 0.0;
  viewportw = viewporth = 0;

  fov = 45.0;

  hudar = 4.0/3.0; // Basic DOOM hud takes the full screen.
  screenar = 1.0;  // Pick a default value, any default value.

  // Initially we are at 2D mode.
  consolemode = true;

  CONS_Printf("New OpenGL renderer created.\n");
}

OGLRenderer::~OGLRenderer() {
  CONS_Printf("Closing OpenGL renderer.\n");

  // No need to release screen. SDL does it automatically.
}

// Sets up those GL states that never change during rendering.

void OGLRenderer::InitGLState() {
  glShadeModel(GL_SMOOTH);
  glEnable(GL_TEXTURE_2D);

  /*
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GEQUAL, 1.0);
  */
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
}

// Clean up stuffage so we can start drawing a new frame.

void OGLRenderer::StartFrame() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Done with drawing. Swap buffers.

void OGLRenderer::FinishFrame() {
    SDL_GL_SwapBuffers(); // Double buffered OpenGL goodness.
}

bool OGLRenderer::InitVideoMode(const int w, const int h, const bool fullscreen) {
  Uint32 surfaceflags;
  int mindepth = 16;
  int temp;

  workinggl = false;

  surfaceflags = SDL_OPENGL;
  if(fullscreen)
    surfaceflags |= SDL_FULLSCREEN;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Check that we get hicolor.
  int cbpp = SDL_VideoModeOK(w, h, mindepth, surfaceflags);
  if(cbpp < 16) {
    CONS_Printf("Hicolor OpenGL mode not available.\n");
    return false;
  }

  screen = SDL_SetVideoMode(w, h, cbpp, surfaceflags);
  if(!screen) {
    CONS_Printf("Could not obtain requested resolution.\n");
    return false;
  }

  SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &temp);
  if(temp)
    CONS_Printf("OpenGL mode is double buffered.\n");
  else
    CONS_Printf("OpenGL mode is NOT double buffered.\n");

  // Print state and debug info.
  CONS_Printf("Set OpenGL video mode %dx%dx%d.", screen->w, screen->h, cbpp);
  if(fullscreen)
    CONS_Printf(" (fullscreen)\n");
  else
    CONS_Printf(" (windowed)\n");
  CONS_Printf("OpenGL Vendor:   %s.\n", glGetString(GL_VENDOR));
  CONS_Printf("OpenGL renderer: %s.\n", glGetString(GL_RENDERER));
  CONS_Printf("OpenGL version:  %s.\n", glGetString(GL_VERSION));

  // Calculate the screen's aspect ratio. Assumes square pixels.
  if(w == 1280 && h == 1024 &&     // Check a couple of exceptions.
     surfaceflags & SDL_FULLSCREEN)
    screenar = 4.0/3.0;
  else if(w == 320 && h == 200 &&
	  surfaceflags & SDL_FULLSCREEN)
    screenar = 4.0/3.0;
  else
    screenar = double(w)/h;

  CONS_Printf("Screen aspect ratio %.2f.\n", screenar);
  CONS_Printf("HUD aspect ratio %.2f.\n", hudar);

  workinggl = true;
  //  vid.setmodeneeded = 0;

  // Let the software portions of the renderer think that we are
  // running the base resolution. It uses weird offsets otherwise.
  vid.width = BASEVIDWIDTH;
  vid.height = BASEVIDHEIGHT;
  InitGLState();

  // ADD: currently we only use one viewport, so we set it here. When
  // multiple subwindows are added this should be set somewhere else.
  viewportw = screen->w;
  viewporth = screen->h;
  glViewport(0, 0, viewportw, viewporth);

  // Reset matrix transformations.
  if(consolemode)
    Setup2DMode();
  else 
    Setup3DMode();


  return true;
}


// Set up the GL matrices so that we can draw 2D stuff like menus.

void OGLRenderer::Setup2DMode() {
  GLfloat extraoffx, extraoffy, extrascalex, extrascaley;
  consolemode = true;

  //glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, 1.0, 0.0, 1.0);
  
  if(screenar > hudar) {
    extraoffx = (screenar - hudar)/(screenar*2.0);
    extraoffy = 0.0;
    extrascalex = hudar/screenar;
    extrascaley = 1.0;
  } else if(screenar < hudar) {
     extraoffx = 0.0;
     extraoffy = (hudar - screenar)/(hudar*2.0);
     extrascalex = 1.0;
     extrascaley = screenar/hudar;
  } else {
     extrascalex = extrascaley = 1.0;
     extraoffx = extraoffy = 0.0;
  }

  glTranslatef(extraoffx, extraoffy, 0.0);
  glScalef(extrascalex, extrascaley, 1.0);
   
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}


// Setup GL matrices to render level graphics.

void OGLRenderer::Setup3DMode() {
  consolemode = false;

  // glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(fov, double(viewportw)/viewporth, 1, 10000);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Now we can render items using their Doom coordinates directly.

}

// Draws a square defined by the parameters. The square is filled by
// the specified texture.
//
// (0, 0) is the top left corner and (1, 1) is the bottom right.

void OGLRenderer::Draw2DGraphic(GLfloat top, GLfloat left, GLfloat bottom, GLfloat right, GLuint tex, GLfloat textop, GLfloat texbottom, GLfloat texleft, GLfloat texright) {
  //  GLfloat scalex, scaley, offsetx, offsety;
  //  GLfloat texleft, texright, textop, texbottom;
  GLfloat temp;

  if(!workinggl)
    return;
  
  if(!consolemode)
    I_Error("Attempting to draw a 2D HUD graphic while in 3D mode.\n");

  //  printf("Drawing tex %d at (%.2f, %.2f) (%.2f, %.2f).\n", tex, top, left, bottom, right);

  // OpenGL origo is at the bottom left corner. Doom is at top left.
  // Fix that.
  bottom = 1.0-bottom;
  top = 1.0-top;

  glBindTexture(GL_TEXTURE_2D, tex);

  // FIXME. This should not be done. But unless we do this the
  // textures get drawn upside down. However I just spent 3 hours
  // trying to debug this thing and failing spectacularly, so fuck it.
  temp = textop;
  textop = texbottom;
  texbottom = temp;

  glBegin(GL_QUADS);
  glTexCoord2f(texleft, texbottom);
  glVertex2f(left, bottom);
  glTexCoord2f(texright, texbottom);
  glVertex2f(right, bottom);
  glTexCoord2f(texright, textop);
  glVertex2f(right, top);
  glTexCoord2f(texleft, textop);
  glVertex2f(left, top);
  glEnd();
}

// Just like the earlier one, except the coordinates are given in Doom
// units. (screen is 320 wide and 200 high.)

void OGLRenderer::Draw2DGraphic_Doom(float x, float y, float width, float height, GLuint tex) {
  /*
  if((x+width) > doomscreenw || (y+height) > doomscreenh)
    printf("Tex %d out of bounds: (%.2f, %.2f) (%.2f, %.2f).\n", tex, x, y, x+width, y+height);  
  */
  Draw2DGraphic(y/BASEVIDHEIGHT, x/BASEVIDWIDTH, 
		(y+height)/BASEVIDHEIGHT, (x+width)/BASEVIDWIDTH,
		tex);
}

void OGLRenderer::Draw2DGraphicFill_Doom(float x, float y, float width, float height, float texwidth, float texheight, GLuint tex) {
  //  CONS_Printf("w: %f, h: %f, texw: %f, texh: %f.\n", width, height, texwidth, texheight);
  //  CONS_Printf("xrepeat %.2f, yrepeat %.2f.\n", width/texwidth, height/texheight);
  Draw2DGraphic(y/BASEVIDHEIGHT, x/BASEVIDWIDTH, 
		(y+height)/BASEVIDHEIGHT, (x+width)/BASEVIDWIDTH,
		tex, 1.0, 1.0-height/texheight, 1.0, 1.0-width/texwidth);

  /*
  Draw2DGraphic(y/BASEVIDHEIGHT, x/BASEVIDWIDTH, 
		(y+height)/BASEVIDHEIGHT, (x+width)/BASEVIDWIDTH,
		tex, 0.0, 4.0, 0.0, 4.0);
  */
}

// Set up state and draw a view of the level from the given viewpoint.
// It is usually the place where the player is currently located.

void OGLRenderer::Render3DView(PlayerInfo *player)
{
  // Set up the Map to be rendered. Needs to be done separately for each viewport, since the engine
  // can run several Maps at once.
  mp = player->mp;

  // shortcuts...
  l.vertexes = mp->vertexes;
  l.numvertexes = mp->numvertexes;

  l.lines = mp->lines;
  l.numlines = mp->numlines;

  l.sides = mp->sides;
  l.numsides = mp->numsides;

  l.sectors = mp->sectors;
  l.numsectors = mp->numsectors;

  l.glvertexes = mp->glvertexes;
  l.numglvertexes = mp->numglvertexes;

  l.glsegs = mp->segs;
  l.numglsegs = mp->numsegs;

  l.glsubsectors = mp->subsectors;
  l.numglsubsectors = mp->numsubsectors;

  l.glnodes = mp->nodes;
  l.numglnodes = mp->numnodes;

  l.polyobjs = mp->polyobjs;
  l.numpolyobjs = mp->NumPolyobjs;


  Setup3DMode();
  x = player->pov->pos.x.Float();
  y = player->pov->pos.y.Float();
  z = player->viewz.Float();

  theta = (double)(player->pov->yaw>>ANGLETOFINESHIFT)*(360.0f/(double)FINEANGLES);
  phi = (double)(player->pov->pitch>>ANGLETOFINESHIFT)*(360.0f/(double)FINEANGLES);

  //  printf("Rendering at (%.2f, %.2f, %.2f), theta %.2f, phi %.2f.\n", x, y, z, theta, phi);

  glMatrixMode(GL_PROJECTION);

  // Set up camera to look through player's eyes.
  glRotatef(-phi,   1.0, 0.0, 0.0);
  glRotatef(-90.0,  1.0, 0.0, 0.0);
  glRotatef(90.0,   0.0, 0.0, 1.0);
  glRotatef(-theta, 0.0, 0.0, 1.0);
  glTranslatef(-x, -y, -z);

  RenderBSP();

  // Pretty soon we want to draw HUD graphics and stuff.
  Setup2DMode();

}


// Walk through the BSP tree and render walls and items. Assumes that
// the OpenGL state is properly set elsewhere.

void OGLRenderer::RenderBSP() {
  if(l.glvertexes == NULL)
    I_Error("Trying to render level but level geometry is not set.\n");

  // FIXME We don't use no fancy schmanzy BSP trees. Real men just
  // dump their vertex data to the card and let it handle everything.
  // if (l.numglnodes == 0) fuck_you();
  // RenderBSPNode(l.numglnodes-1);

  for(int i=0; i<l.numglsubsectors; i++)
    RenderGLSubsector(i);

  for(int i=0; i<l.numglsegs; i++)
    RenderGLSeg(i);
}


void OGLRenderer::RenderBSPNode(int nodenum)
{
  // Found a subsector (leaf node)?
  if (nodenum & NF_SUBSECTOR)
    {
      RenderGLSubsector(nodenum & ~NF_SUBSECTOR);
      return;
    }

  // otherwise keep traversing the tree
  node_t *node = &l.glnodes[nodenum];

  // Decide which side the view point is on.
  //int side = R_PointOnSide(x, y, node);

  /*
  // Recursively divide front space.
  RenderBSPNode(node->children[side]);

  // Possibly divide back space.
  // TODO check view frustum overlap with backnode bbox
  if (R_CheckBBox(node->bbox[side^1]))
    RenderBSPNode(node->children[side^1]);
  */
}




// Draw the floor and ceiling of a single GL subsector. In the future
// render also 3D floors and possibly things inside the subsector.

void OGLRenderer::RenderGLSubsector(int num) {
  int curseg;
  int firstseg;
  int segcount;
  subsector_t *ss;
  sector_t *s;
  Texture *ftex;
  if(num < 0 || num > l.numglsubsectors)
    return;
  
  ss = &l.glsubsectors[num];
  firstseg = ss->first_seg;
  segcount = ss->num_segs;
  s = ss->sector;

  // Bind ceiling texture.
  ftex = tc[s->ceilingpic];
  if(ftex) {
    if(ftex->glid == ftex->NOTEXTURE) 
      ftex->GetData(); // Creates the texture.
    glBindTexture(GL_TEXTURE_2D, ftex->glid);

    // First draw the ceiling.
    glNormal3f(0.0, 0.0, -1.0);
    glBegin(GL_POLYGON);
    for(curseg = firstseg; curseg < firstseg + segcount; curseg++) {
      seg_t seg = l.glsegs[curseg];
      GLfloat x, y, z, tx, ty;
      vertex_t *v;

      v = seg.v1;
      x = v->x.Float();
      y = v->y.Float();
      z = s->ceilingheight.Float(); 

      tx = x/ftex->width;
      ty = 1.0 - y/ftex->height;

      glTexCoord2f(tx, ty);
      glVertex3f(x, y, z);

      //    printf("(%.2f, %.2f)\n", x, y);
    }
    glEnd();
  }
 
  // Then the floor. First bind texture.
  ftex = tc[s->floorpic];
  if(ftex) {
    if(ftex->glid == ftex->NOTEXTURE) 
      ftex->GetData(); // Creates the texture.
    glBindTexture(GL_TEXTURE_2D, ftex->glid);

    glNormal3f(0.0, 0.0, 1.0);
    glBegin(GL_POLYGON);
    for(curseg = firstseg+segcount-1; curseg >= firstseg; curseg--) {
      seg_t seg = l.glsegs[curseg];
      GLfloat x, y, z, tx, ty;
      vertex_t *v;

      v = seg.v2;
      x = v->x.Float();
      y = v->y.Float();
      z = s->floorheight.Float(); 

      tx = x/ftex->width;
      ty = 1.0 - y/ftex->height;

      glTexCoord2f(tx, ty);
      glVertex3f(x, y, z);

      //    printf("(%.2f, %.2f)\n", x, y);
    }
    glEnd();
  }

  // TODO render segs here

  // finally the actors
  RenderActors(s);
}



void OGLRenderer::RenderActors(sector_t *sec)
{
  // BSP is traversed by subsector.
  // A sector might have been split into several subsectors during BSP building.
  // Thus we check whether its already drawn.
  if (sec->validcount == validcount)
    return;

  // Well, now it will be done.
  sec->validcount = validcount;

  /*
  if (!sec->numlights)
    {
      if (sec->heightsec == -1)
        lightlevel = sec->lightlevel;

      int lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;

      if (lightnum < 0)
        spritelights = scalelight[0];
      else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
      else
        spritelights = scalelight[lightnum];
    }
  */

  // Handle all things in sector.
  for (Actor *thing = sec->thinglist; thing; thing = thing->snext)
    if (!(thing->flags2 & MF2_DONTDRAW))
      {
	if (!thing->pres)
	  continue;
        thing->pres->Draw(thing); // does both sprites and 3d models
      }
}



// Renders one single GL seg. Minisegs and invalid parameters are
// silently ignored.

void OGLRenderer::RenderGLSeg(int num) {
  // TODO segs should be rendered together with their associated subsec (with backface culling...)
  seg_t *s;
  line_t *ld;
  vertex_t *fv;
  vertex_t *tv;
  fixed_t nx,ny;
  side_t *lsd; // Local sidedef.
  side_t *rsd; // Remote sidedef.
  sector_t *ls; // Local sector.
  sector_t *rs; // Remote sector.

  GLfloat rs_floor, rs_ceil, ls_floor, ls_ceil; // Floor and ceiling heights.
  GLfloat textop, texbottom, texleft, texright;
  GLfloat utexhei, ltexhei;
  GLfloat ls_height, tex_yoff;
  Texture *uppertex, *middletex, *lowertex;

  if(num < 0 || num > l.numglsegs)
    return;

  s = &(l.glsegs[num]);
  ld = s->linedef;
  // Don't draw minisegs.
  if(ld == NULL)
    return;

  fv = s->v1;
  tv = s->v2;

  // Calculate surface normal. Should we account for degenerate
  // linedefs?
  nx = tv->y - fv->y;
  ny = fv->x - tv->x;
  
  glNormal3f(nx.Float(), ny.Float(), 0.0);

  if(s->side == 0) {
    lsd = ld->sideptr[0];
    rsd = ld->sideptr[1];
  } else {
    lsd = ld->sideptr[1];
    rsd = ld->sideptr[0];
  }

  // This should never happen, but we are paranoid.
  if(lsd == NULL)
    return;

  ls = lsd->sector;
  ls_floor = ls->floorheight.Float();
  ls_ceil = ls->ceilingheight.Float();
  ls_height = ls_ceil - ls_floor;

  if(rsd) {
    rs = rsd->sector;
    rs_floor = rs->floorheight.Float();
    rs_ceil = rs->ceilingheight.Float();
    utexhei = ls_ceil - rs_ceil;
    ltexhei = rs_floor - ls_floor;
  } else {
    rs = NULL;
    rs_floor = 0.0;
    rs_ceil = 0.0;
  }

  tex_yoff = lsd->rowoffset.Float();
  uppertex = tc[lsd->toptexture];
  middletex = tc[lsd->midtexture];
  lowertex = tc[lsd->bottomtexture];

  // Generate GL textures.
  if(uppertex && uppertex->glid == uppertex->NOTEXTURE)
    uppertex->GetData();
  if(middletex && middletex->glid == middletex->NOTEXTURE)
    middletex->GetData();
  if(lowertex && lowertex->glid == lowertex->NOTEXTURE)
    lowertex->GetData();

  // Texture X offsets.
  texleft = lsd->textureoffset.Float() + s->offset.Float();
  texright = texleft + s->length;

  // Ready to draw textures.
  if(rs != NULL) {
    if(rs_ceil < ls_ceil && uppertex) {
      if(ld->flags & ML_DONTPEGTOP) {
	textop = tex_yoff;
	texbottom = textop + utexhei;
      } else {
	texbottom = tex_yoff;
	textop = texbottom - utexhei;
      }

      glBindTexture(GL_TEXTURE_2D, uppertex->glid);
      DrawSingleQuad(fv, tv, rs_ceil, ls_ceil, texleft/uppertex->width,
		     texright/uppertex->width, textop/uppertex->height,
		     texbottom/uppertex->height);
    }
    
    if(rs_floor > ls_floor && lowertex) {
      // Lower textures are a major PITA.
      if(ld->flags & ML_DONTPEGBOTTOM) {
	texbottom = tex_yoff + ls_height - lowertex->height;
	textop = texbottom - ltexhei;
      } else {
	textop = tex_yoff;
	texbottom = textop + ltexhei;
      }
      glBindTexture(GL_TEXTURE_2D, lowertex->glid);
      DrawSingleQuad(fv, tv, ls_floor, rs_floor, texleft/lowertex->width,
		     texright/lowertex->width, textop/lowertex->height,
		     texbottom/lowertex->height);
    }

    // Double sided middle textures do not repeat, so we need some
    // trickery.
    if(middletex != 0 && ls_floor < rs_ceil && ls_ceil > rs_floor) {
      GLfloat top, bottom;
      if(ld->flags & ML_DONTPEGBOTTOM) {
	if(ls_floor < rs_floor)
	  bottom = rs_floor;
	else
	  bottom = ls_floor;
	top = bottom + middletex->height; // FIXME, should be at most
					  // the height of the remote
					  // sector?
      } else {
	if(ls_ceil < rs_ceil)
	  top = ls_ceil;
	else
	  top = rs_ceil;
	bottom = top - middletex->height;
      glBindTexture(GL_TEXTURE_2D, middletex->glid);
      DrawSingleQuad(fv, tv, bottom, top, texleft/middletex->width, 
		     texright/middletex->width);
      //, textop/middletex->height,
      //     texbottom/middletex->height);
      }
    }
  } else if(middletex) {
    if(ld->flags & ML_DONTPEGTOP) {
      texbottom = tex_yoff;
      textop = texbottom - ls_height;
    } else {
      textop = tex_yoff;
      texbottom = textop + ls_height;
    }
    glBindTexture(GL_TEXTURE_2D, middletex->glid);
    DrawSingleQuad(fv, tv, ls_floor, ls_ceil, texleft/middletex->width, 
		   texright/middletex->width, textop/middletex->height,
		   texbottom/middletex->height);
  }
}

// Draw a single textured wall segment.

void OGLRenderer::DrawSingleQuad(vertex_t *fv, vertex_t *tv, GLfloat lower, GLfloat upper, GLfloat texleft, GLfloat texright, GLfloat textop, GLfloat texbottom) {
  glBegin(GL_QUADS);
  
  glTexCoord2f(texleft, texbottom);
  glVertex3f(fv->x.Float(), fv->y.Float(), lower);
  glTexCoord2f(texright, texbottom);
  glVertex3f(tv->x.Float(), tv->y.Float(), lower);
  glTexCoord2f(texright, textop);
  glVertex3f(tv->x.Float(), tv->y.Float(), upper);
  glTexCoord2f(texleft, textop);
  glVertex3f(fv->x.Float(), fv->y.Float(), upper);

  glEnd();
}

// Draws the specified item (weapon, monster, flying rocket etc) in
// the 3D view. Translations and rotations are done with OpenGL
// matrices.

void OGLRenderer::DrawSpriteItem(const vec_t<fixed_t>& pos, Texture *t, bool flip)
{
  GLfloat top, bottom, left, right;
  GLfloat texleft, texright, textop, texbottom;

  // You can't draw the invisible.
  if(!t)
    return;

  if(t->glid == t->NOTEXTURE)
    t->GetData();
  glBindTexture(GL_TEXTURE_2D, t->glid);

  if(flip) {
    texleft = 1.0;
    texright = 0.0;
  } else {
    texleft = 0.0;
    texright = 1.0;
  }

  // Shouldn't these be the other way around?
  texbottom = 1.0;
  textop = 0.0;

  left = t->width/2;
  right = -left;
  bottom = 0.0;
  top = t->height;
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glTranslatef(pos.x.Float(), pos.y.Float(), pos.z.Float());
  glRotatef(theta, 0.0, 0.0, 1.0);
  glNormal3f(-1.0, 0.0, 0.0);

  glBegin(GL_QUADS);
  glTexCoord2f(texleft, texbottom);
  glVertex3f(0.0, left, bottom);
  glTexCoord2f(texright, texbottom);
  glVertex3f(0.0, right, bottom);
  glTexCoord2f(texright, textop);
  glVertex3f(0.0, right, top);
  glTexCoord2f(texleft, textop);
  glVertex3f(0.0, left, top);
  glEnd();

  /* The world famous black triangles look.
  glBegin(GL_TRIANGLES);
  glVertex3f(0.0, 0.0, 128.0);
  glVertex3f(0.0, 32.0, 0.0);
  glVertex3f(0.0, -32.0, 0.0);
  glEnd();
  */

  // Leave the matrix stack as it was.
  glPopMatrix();
}


bool spritepres_t::Draw(const Actor *p)
{
  int frame = state->frame & TFF_FRAMEMASK;

  if (frame >= spr->numframes)
    {
      frame = spr->numframes - 1; // Thing may require more frames than the defaultsprite has...
      //I_Error("spritepres_t::Project: invalid sprite frame %d (%d)\n", frame, spr->numframes);
    }
    
  spriteframe_t *sprframe = &spr->spriteframes[frame];

  Texture  *t;
  bool      flip;

  // decide which patch to use for sprite relative to player
  if (sprframe->rotate)
    {
      // choose a different rotation based on player view
      angle_t ang = R.R_PointToAngle(p->pos.x, p->pos.y); // uses viewx,viewy
      unsigned rot = (ang - p->yaw + unsigned(ANG45/2) * 9) >> 29;

      t = sprframe->tex[rot];
      flip = sprframe->flip[rot];
    }
  else
    {
      // use single rotation for all views
      t = sprframe->tex[0];
      flip = sprframe->flip[0];
    }

  // TODO MF_SHADOW, translucency, colormapping etc.

  // hardware renderer part
  oglrenderer->DrawSpriteItem(p->pos, t, flip);


  //CONS_Printf("spritepres_t::Draw: Not yet implemented\n");
  return true;
}
