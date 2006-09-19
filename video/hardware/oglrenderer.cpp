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

#include "oglrenderer.hpp"
#include "oglhelpers.hpp"
#include "doomdef.h"
#include "screen.h"

#include "g_map.h"
#include "g_player.h"
#include "g_actor.h"
#include "g_mapinfo.h"

#include "tables.h"
#include "r_data.h"
#include "r_main.h"
#include "r_sprite.h"
#include "am_map.h"
#include "w_wad.h" // Need file cache to get playpal.

extern int skyflatnum;

OGLRenderer::OGLRenderer() {
  screen = NULL;
  workinggl = false;
  curssec = NULL;
  palette = NULL;

  mp = NULL;
  //  l.glvertexes = NULL;

  x = y = z = 0.0;
  theta = phi = 0.0;
  viewportw = viewporth = 0;

  fov = 70.0;

  hudar = 4.0/3.0; // Basic DOOM hud takes the full screen.
  screenar = 1.0;  // Pick a default value, any default value.

  // Initially we are at 2D mode.
  consolemode = true;

  InitLumLut();

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

  // Gets rid of "black boxes" around sprites. Might also speed up
  // rendering a bit?
  //  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GEQUAL, 1.0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
}

// Clean up stuffage so we can start drawing a new frame.

void OGLRenderer::StartFrame() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ClearDrawColor();
}

// Done with drawing. Swap buffers.

void OGLRenderer::FinishFrame() {
    SDL_GL_SwapBuffers(); // Double buffered OpenGL goodness.
    palette = NULL;       // It might have gotten uncached.
}

// Writes a screen shot to the specified file. Writing is done in BMP
// format, as SDL has direct support of that. Adds proper file suffix
// when necessary. Returns true on success.

bool OGLRenderer::WriteScreenshot(const char *fname) {
  int fnamelength;
  string finalname;
  SDL_Surface *buffer;
  bool success;

  if(screen == NULL)
    return false;

  /*
  if(screen->pixels == NULL) {
    CONS_Printf("Empty SDL surface. Can not take screenshot.\n");
    return false;
  }
  */

  if(fname == NULL)
    finalname = "DOOM000.bmp";
  else {    
    fnamelength = strlen(fname);

    if(!strcmp(fname+fnamelength-4, ".bmp") ||
       !strcmp(fname+fnamelength-4, ".BMP"))
      finalname = fname;
    else {
      finalname = fname;
      finalname += ".bmp";
    }
  }

  // Now we know the file name. Time to start the magic.

  // Potential endianness bug with masks?
  buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, 
				screen->h, 32, 0xff000000, 0xff0000, \
				0xff00, 0xff);
  if(!buffer) {
    CONS_Printf("Could not create SDL surface. SDL error: %s\n", SDL_GetError());
    return false;
  }

  SDL_LockSurface(buffer);
  glReadPixels(0, 0, screen->w, screen->h, GL_RGBA, 
	       GL_UNSIGNED_INT_8_8_8_8, buffer->pixels);

  // OpenGL keeps the pixel data "upside down" for some reason. Flip
  // the surface.
  for(int i=0; i < buffer->w; i++)
    for(int j=0; j<buffer->h/2; j++) {
      Uint32 temp;
      Uint32 *p1;
      Uint32 *p2;
      p1 = static_cast<Uint32*>(buffer->pixels) + j*buffer->w + i;
      p2 = static_cast<Uint32*>(buffer->pixels) + (buffer->h-j)*buffer->w + i;
      temp = *p1;
      *p1 = *p2;
      *p2 = temp;
    }
  SDL_UnlockSurface(buffer);
  success = !SDL_SaveBMP(buffer, finalname.c_str());
  SDL_FreeSurface(buffer);
  return success;
}

bool OGLRenderer::InitVideoMode(const int w, const int h, const bool fullscreen) {
  Uint32 surfaceflags;
  int mindepth = 16;
  int temp;

  workinggl = false;

  surfaceflags = SDL_OPENGL;
  if(fullscreen)
    surfaceflags |= SDL_FULLSCREEN;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  //  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Check that we get hicolor.
  int cbpp = SDL_VideoModeOK(w, h, 24, surfaceflags);
  if(cbpp < mindepth) {
    CONS_Printf("Hicolor OpenGL mode not available.\n");
    return false;
  }

  screen = SDL_SetVideoMode(w, h, cbpp, surfaceflags);
  if(!screen) {
    CONS_Printf("Could not obtain requested resolution.\n");
    return false;
  }


  CONS_Printf("Color depth in bits: ");
  SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &temp);
  CONS_Printf("R %d, ", temp);
  SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &temp);
  CONS_Printf("G %d, ", temp);
  SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &temp);
  CONS_Printf("B %d.\n", temp);
  SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &temp);
  CONS_Printf("Alpha buffer depth %d bits.\n", temp);
  SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &temp);
  CONS_Printf("Depth buffer depth %d bits.\n", temp);

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
  CONS_Printf("OpenGL Vendor:   %s\n", glGetString(GL_VENDOR));
  CONS_Printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
  CONS_Printf("OpenGL version:  %s\n", glGetString(GL_VERSION));

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
  ClearDrawColor();

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
  ClearDrawColor();

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

void OGLRenderer::Draw2DGraphic_Doom(float x, float y, Texture *tex)
{
  /*
  if((x+width) > doomscreenw || (y+height) > doomscreenh)
    printf("Tex %d out of bounds: (%.2f, %.2f) (%.2f, %.2f).\n", tex, x, y, x+width, y+height);  
  */
  Draw2DGraphic(y/BASEVIDHEIGHT, x/BASEVIDWIDTH, 
		(y+tex->height)/BASEVIDHEIGHT, (x+tex->width)/BASEVIDWIDTH,
		tex->GLPrepare());
}

void OGLRenderer::Draw2DGraphicFill_Doom(float x, float y, float width, float height, Texture *tex)
{
  //  CONS_Printf("w: %f, h: %f, texw: %f, texh: %f.\n", width, height, texwidth, texheight);
  //  CONS_Printf("xrepeat %.2f, yrepeat %.2f.\n", width/texwidth, height/texheight);
  Draw2DGraphic(y/BASEVIDHEIGHT, x/BASEVIDWIDTH, 
		(y+height)/BASEVIDHEIGHT, (x+width)/BASEVIDWIDTH,
		tex->GLPrepare(), 1.0, 1.0-height/tex->height, 1.0, 1.0-width/tex->width);

  /*
  Draw2DGraphic(y/BASEVIDHEIGHT, x/BASEVIDWIDTH, 
		(y+height)/BASEVIDHEIGHT, (x+width)/BASEVIDWIDTH,
		tex, 0.0, 4.0, 0.0, 4.0);
  */
}

// Currently a no-op. Possibly do something in the future.

void OGLRenderer::ClearAutomap() {
}

// Draws the specified map line to screen.

void OGLRenderer::DrawAutomapLine(const fline_t *line, const int color) {
  GLfloat c[4];
  
  if(!consolemode)
    I_Error("Trying to draw level map while in 3D mode.\n");

  if(!palette)
    palette = static_cast<byte*>(fc.CacheLumpName("PLAYPAL", PU_CACHE));

  // Set color.
  c[0] = palette[3*color]/255.0;
  c[1] = palette[3*color+1]/255.0;
  c[2] = palette[3*color+2]/255.0;
  c[3] = 1.0;
  glColor4fv(c);

  // Do not use a texture.
  glBindTexture(GL_TEXTURE_2D, 0);

  glBegin(GL_LINES);
  glVertex2f(line->a.x/float(BASEVIDWIDTH), 
	     1.0-line->a.y/float(BASEVIDHEIGHT));
  glVertex2f(line->b.x/float(BASEVIDWIDTH),
	     1.0-line->b.y/float(BASEVIDHEIGHT));
  glEnd();

}

// Set up state and draw a view of the level from the given viewpoint.
// It is usually the place where the player is currently located.

void OGLRenderer::Render3DView(PlayerInfo *player)
{
  // Set up the Map to be rendered. Needs to be done separately for each viewport, since the engine
  // can run several Maps at once.
  mp = player->mp;

  x = player->pov->pos.x.Float();
  y = player->pov->pos.y.Float();
  z = player->viewz.Float();
  curssec = player->pov->subsector;

  theta = (double)(player->pov->yaw>>ANGLETOFINESHIFT)*(360.0f/(double)FINEANGLES);
  phi = (double)(player->pov->pitch>>ANGLETOFINESHIFT)*(360.0f/(double)FINEANGLES);

  // This simple sky rendering algorithm uses 2D mode. We should
  // probably do something more fancy in the future.
  DrawSimpleSky();

  Setup3DMode();
  glMatrixMode(GL_PROJECTION);

  // Set up camera to look through player's eyes.
  glRotatef(-phi,   1.0, 0.0, 0.0);
  glRotatef(-90.0,  1.0, 0.0, 0.0);
  glRotatef(90.0,   0.0, 0.0, 1.0);
  glRotatef(-theta, 0.0, 0.0, 1.0);
  glTranslatef(-x, -y, -z);

  if(mp->glvertexes == NULL)
    I_Error("Trying to render level but level geometry is not set.\n");

  RenderBSPNode(mp->numnodes-1);

  // Pretty soon we want to draw HUD graphics and stuff.
  Setup2DMode();

}


// Walk through the BSP tree and render the level in back-to-front
// order. Assumes that the OpenGL state is properly set elsewhere.

void OGLRenderer::RenderBSPNode(int nodenum)
{
  // Found a subsector (leaf node)?
  if (nodenum & NF_SUBSECTOR)
    {
      int nt = nodenum & ~NF_SUBSECTOR;
      if(curssec == NULL || CheckVis(curssec-mp->subsectors, nt))
	 RenderGLSubsector(nt);
      return;
    }

  // otherwise keep traversing the tree
  node_t *node = &mp->nodes[nodenum];

  // Decide which side the view point is on.
  int side = R_PointOnSide(float(x), float(y), node);

  // OpenGL requires back-to-front drawing for translucency effects to
  // work properly. Thus we first check the back.

  // if (R_CheckBBox(node->bbox[side^1]))
  RenderBSPNode(node->children[side^1]);

  // Now we draw the front side.
  RenderBSPNode(node->children[side]);
}

// Draws one single GL subsector polygon that is either a floor or a
// ceiling. 

void OGLRenderer::RenderGlSsecPolygon(subsector_t *ss, GLfloat height, Texture *tex, bool isFloor) {
  int curseg;
  int firstseg;
  int segcount;
  int loopstart, loopend, loopinc;
  
  firstseg = ss->first_seg;
  segcount = ss->num_segs;

  // GL subsector polygons are clockwise when viewed from above.
  // OpenGL polygons are defined counterclockwise. Thus we need to go
  // "backwards" when drawing the floor.
  if(isFloor) {
    loopstart = firstseg+segcount-1;
    loopend = firstseg-1;
    loopinc = -1;
  } else {
    loopstart = firstseg;
    loopend = firstseg+segcount;
    loopinc = 1;
  }

  glBindTexture(GL_TEXTURE_2D, tex->GLPrepare());
  
  glNormal3f(0.0, 0.0, -loopinc);
  glBegin(GL_POLYGON);
  for(curseg = loopstart; curseg != loopend; curseg += loopinc) {
    seg_t *seg = &mp->segs[curseg];
    GLfloat x, y, tx, ty;
    vertex_t *v;
    
    if(isFloor)
      v = seg->v2;
    else
      v = seg->v1;
    x = v->x.Float();
    y = v->y.Float();
    
    tx = x/tex->width;
    ty = 1.0 - y/tex->height;

    // Hi-res texture scaling.
    tx *=  tex->xscale.Float();
    ty *= tex->yscale.Float();

    glTexCoord2f(tx, ty);
    glVertex3f(x, y, height);
    
    //    printf("(%.2f, %.2f)\n", x, y);
  }
  glEnd();

}

// Draw the floor and ceiling of a single GL subsector. The rendering
// order is flats->wall segs->items, so there is no occlusion. In the
// future render also 3D floors and polyobjs.

void OGLRenderer::RenderGLSubsector(int num) {
  int curseg;
  int firstseg;
  int segcount;
  subsector_t *ss;
  sector_t *s;
  Texture *ftex;
  GLfloat c[4];
  byte light;
  if(num < 0 || num > mp->numsubsectors)
    return;


  ss = &mp->subsectors[num];
  firstseg = ss->first_seg;
  segcount = ss->num_segs;
  s = ss->sector;

  // Set up sector lighting.
  light = LightLevelToLum(s->lightlevel);
  c[0] = light/255.0;
  c[1] = light/255.0;
  c[2] = light/255.0;
  c[3] = 1.0;
  glColor4fv(c);
  
  // Draw ceiling texture, skip sky flats.
  ftex = tc[s->ceilingpic];
  if (ftex && s->ceilingpic != skyflatnum)
    RenderGlSsecPolygon(ss, s->ceilingheight.Float(), ftex, false);
 
  // Then the floor.
  ftex = tc[s->floorpic];
  if(ftex && s->floorpic != skyflatnum)
    RenderGlSsecPolygon(ss, s->floorheight.Float(), ftex, true);

  // Draw the walls of this subsector.
  for(curseg=firstseg; curseg<firstseg+segcount; curseg++)
    RenderGLSeg(curseg);

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
  GLfloat xscale, yscale;
  Texture *uppertex, *middletex, *lowertex;

  if(num < 0 || num > mp->numsegs)
    return;

  s = &(mp->segs[num]);
  ld = s->linedef;
  // Minisegs don't have wall textures so no point in drawing them.
  if(ld == NULL)
    return;

  // Mark this linedef as visited so it gets drawn on the automap.
  ld->flags |= ML_MAPPED;

  fv = s->v1;
  tv = s->v2;

  // Calculate surface normamp-> Should we account for degenerate
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

  // Texture X offsets.
  texleft = lsd->textureoffset.Float() + s->offset.Float();
  texright = texleft + s->length;

  // Ready to draw textures.
  if(rs != NULL) {
    // Upper texture.
    if(rs_ceil < ls_ceil && uppertex) {
      if(ld->flags & ML_DONTPEGTOP) {
	textop = tex_yoff;
	texbottom = textop + utexhei;
      } else {
	texbottom = tex_yoff;
	textop = texbottom - utexhei;
      }

      // If the front and back ceilings are sky, do not draw this
      // upper texture.
      if(ls->ceilingpic != skyflatnum || rs->ceilingpic != skyflatnum) {
	glBindTexture(GL_TEXTURE_2D, uppertex->GLPrepare());
	xscale = uppertex->xscale.Float();
	yscale = uppertex->yscale.Float();
	DrawSingleQuad(fv, tv, rs_ceil, ls_ceil, 
		       texleft/uppertex->width*xscale,
		       texright/uppertex->width*xscale,
		       textop/uppertex->height*yscale,
		       texbottom/uppertex->height*yscale);
      }
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
      glBindTexture(GL_TEXTURE_2D, lowertex->GLPrepare());
      xscale = lowertex->xscale.Float();
      yscale = lowertex->yscale.Float();
      DrawSingleQuad(fv, tv, ls_floor, rs_floor,
		     texleft/lowertex->width*xscale,
		     texright/lowertex->width*xscale,
		     textop/lowertex->height*yscale,
		     texbottom/lowertex->height*yscale);
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
      }
      glBindTexture(GL_TEXTURE_2D, middletex->GLPrepare());
      xscale = middletex->xscale.Float();
      yscale = middletex->yscale.Float();
      DrawSingleQuad(fv, tv, bottom, top,
		     texleft/middletex->width*xscale, 
		     texright/middletex->width*xscale,
		     0.0, 1.0*yscale);
      //, textop/middletex->height,
      //     texbottom/middletex->height);
    }
  } else if(middletex) {
    // Single sided middle texture.
    if(ld->flags & ML_DONTPEGBOTTOM) {
      texbottom = tex_yoff;
      textop = texbottom - ls_height;
    } else {
      textop = tex_yoff;
      texbottom = textop + ls_height;
    }
    glBindTexture(GL_TEXTURE_2D, middletex->GLPrepare());
    xscale = middletex->xscale.Float();
    yscale = middletex->yscale.Float();
    DrawSingleQuad(fv, tv, ls_floor, ls_ceil,
		   texleft/middletex->width*xscale, 
		   texright/middletex->width*xscale,
		   textop/middletex->height*yscale,
		   texbottom/middletex->height*yscale);
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
  GLfloat xscale, yscale;
  GLboolean isAlpha; // Alpha test enabled.

  // You can't draw the invisible.
  if(!t)
    return;

  xscale = t->xscale.Float();
  yscale = t->yscale.Float();

  // Protect against div by zero.
  if(xscale == 0.0)
    xscale = 1.0;
  if(yscale == 0.0)
    yscale = 1.0;

  // Rendering sprite items requires skipping totally transparent
  // pixels. Since alpha testing may slow down rendering we want to
  // use it as little as possible.
  isAlpha = 0; // Shut up compiler.
  glGetBooleanv(GL_ALPHA_TEST, &isAlpha);
  if(!isAlpha)
    glEnable(GL_ALPHA_TEST);

  glBindTexture(GL_TEXTURE_2D, t->GLPrepare());

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

  left = t->width/(2.0*xscale);
  right = -left;
  bottom = 0.0;
  top = t->height/yscale;
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

  // Restore alpha testing to the state it was.
  if(!isAlpha)
    glDisable(GL_ALPHA_TEST);
}

// Check for visibility between the given glsubsectors. Returns true
// if you can see from one to the other and false if not.

bool OGLRenderer::CheckVis(int fromss, int toss) {
  byte *vis;
  if(mp->glvis == NULL)
    return true;

  vis = mp->glvis + (((mp->numsubsectors + 7) / 8) * fromss);
  if (vis[toss >> 3] & (1 << (toss & 7)))
    return true;
  return false;
}

// Draws the background sky texture. Very simple, ignores looking
// up/down.

void OGLRenderer::DrawSimpleSky() {
  GLfloat left, right, top, bottom, texleft, texright, textop, texbottom;
  GLfloat fovx;
  GLboolean isdepth;

  glMatrixMode(GL_PROJECTION);

  isdepth = 0;
  glGetBooleanv(GL_DEPTH_TEST, &isdepth);
  if(isdepth)
    glDisable(GL_DEPTH_TEST);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0.0, 1.0, 0.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glBindTexture(GL_TEXTURE_2D, mp->skytexture->GLPrepare());
  textop = 0.0;   // And yet again these should be the other way around.
  texbottom = 1.0;
  top = 1.0;
  bottom = 0.0;
  left = 0.0;
  right = 1.0;

  fovx = fov*screenar;
  fovx *= 2.0; // I just love magic numbers. Don't you?

  texleft = (2.0*theta + fovx)/720.0;
  texright = (2.0*theta - fovx)/720.0;

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

  if(isdepth)
    glEnable(GL_DEPTH_TEST);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}
