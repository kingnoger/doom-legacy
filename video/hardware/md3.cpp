// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2003 by Ville Bergholm 
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
// Revision 1.5  2003/12/18 11:57:31  smite-meister
// fixes / new bugs revealed
//
// Revision 1.4  2003/06/20 20:56:08  smite-meister
// Presentation system tweaked
//
// Revision 1.3  2003/04/24 20:30:44  hurdler
// Remove lots of compiling warnings
//
// Revision 1.2  2003/04/05 12:20:00  smite-meister
// Makefiles fixed
//
// Revision 1.1  2003/03/23 14:24:14  smite-meister
// Polyobjects, MD3 models
//
//
//
// DESCRIPTION:
//   Quake III MD3 model class for Doom Legacy
//-----------------------------------------------------------------------------

#include <iostream>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "g_actor.h"
#include "g_map.h"
#include "z_zone.h"
#include "hardware/md3.h"

// TODO malloc->Z_Malloc

modelcache_t models(PU_MODEL);


const double pi = 3.1415926536; // M_PI;

static float NormLookup[256][256][3];

void MD3_InitNormLookup()
{
  int i, j;
  float phi, theta;
  for (i=0; i<256; i++)
    for (j=0; j<256; j++)
      {
	theta = 2*i*pi/255.0;
	phi   = 2*j*pi/255.0;
	NormLookup[i][j][0] = cos(phi) * sin(theta);
	NormLookup[i][j][1] = sin(phi) * sin(theta);
	NormLookup[i][j][2] = cos(theta);
      }
}

modelcache_t::modelcache_t(memtag_t tag)
  : L2cache_t(tag)
{}

cacheitem_t *modelcache_t::Load(const char *p, cacheitem_t *r)
{
  // TODO maybe check the existence of the model first?

  MD3_player *t = (MD3_player *)r;
  if (t == NULL)
    t = new MD3_player;

  // TODO only default skin can now be used
  bool result = t->Load(p, "default"); // +tagtype
  if (!result)
    {
      delete t;
      return NULL;
    }

  return t;
}

void modelcache_t::Free(cacheitem_t *r)
{
  // TODO free model data
}


//====================================================
// texture loading
//====================================================

static int CreateTexture(int width, int height, int format, char *data)
{
  GLuint t;
  
  glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); //Texture blends with object background
  //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);  //Texture does NOT blend with object background

  //Select a filtering type. BiLinear filtering produces very good results with little performance impact
  //GL_NEAREST               - Basic texture (grainy looking texture)
  //GL_LINEAR                - BiLinear filtering
  //GL_LINEAR_MIPMAP_NEAREST - Basic mipmapped texture
  //GL_LINEAR_MIPMAP_LINEAR  - BiLinear Mipmapped texture  

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, format, GL_UNSIGNED_BYTE, data);

  return t;
}

typedef unsigned char  byte; // 8 bit
typedef unsigned short word; // 16 bit

// Loads 24 and 32bpp (alpha channel) TGA textures
// TGA format is little-endian (LSB)
struct TGAheader
{
  byte IDlength;  // image ID field lenght
  byte colormaptype; // 0 = no color maps used, 1 = color map included
  byte imagetype; // 2 = uncompressed truecolor
  byte colormapspec[5]; 
  word xorigin;
  word yorigin;
  word width;
  word height;
  byte bpp;
  byte imagedescriptor;  // bit flags
  // image ID
  // color map
  // data
  // and other fields follow...
};


static int LoadTGATexture(const char *filename)
{
  TGAheader h;

  FILE *file = fopen(filename, "rb");
  if (!file)
    return -1;

  // read header 
  fread(&h, sizeof(TGAheader), 1, file);

  //cout << "TGA: IDl = " << h.IDlength << ", cmt = " << h.colormaptype << ", it = " << h.imagetype << endl;

  // Only support uncompressed truecolor images with no color maps
  if (h.IDlength != 0 || h.imagetype != 2 || h.colormaptype != 0 || h.bpp < 24)
    {
      fclose(file);
      cout << "Unsupported TGA file type.\n";
      return -1;
    }

  // TODO: endianness correction of multi-byte fields
  // Get the width, height, and color depth
  // h.width = SHORT(h.width);
  // h.height = SHORT(h.height);
  int imagesize = h.width * h.height * (h.bpp / 8);

  char *image = new char[imagesize];
  fread(image, imagesize, 1, file);

  //cout << "TGA: w = " << h.width << ", h = " << h.height << ", bpp = " << int(h.bpp) << endl;

  int texture;
  // TGAs are stored BGR and not RGB
  // 32 bit TGA files have alpha channel and get loaded differently
  if (h.bpp == 24)
    texture = CreateTexture(h.width, h.height, GL_BGR, image);
  else
    texture = CreateTexture(h.width, h.height, GL_BGRA, image);

  delete image;
  return texture;
}

static int LoadTexture(const char *filename)
{
  cout << "Loading texture file " << filename << ".\n";
  return LoadTGATexture(filename);
}

//======================================================
// MD3 class implementation
//======================================================

// given a *.md3 file name, loads the model to memory
bool MD3_t::Load(const char *filename)
{
  int i;

  cout << "Loading md3 file " << filename << endl;

  FILE *file = fopen(filename, "rb");
  if (!file)
    {
      return false;
    }

  // read in header 
  fread(&header, sizeof(MD3_header), 1, file);

  if (header.magic[0] != 'I' || header.magic[1] != 'D' ||
      header.magic[2] != 'P' || header.magic[3] != '3'
      || header.version != 15)
    {
      fclose(file);
      return false;
    }

  // read frames
  int nf = header.numFrames;
  frames = (MD3_frame *)malloc(nf * sizeof(MD3_frame));
  // frames = new MD3_frame[nf];
  fseek(file, header.ofsFrames, SEEK_SET);
  fread(frames, sizeof(MD3_frame), nf, file);

  // read tags
  int nt = header.numTags;
  tags = (MD3_tag *)malloc(nf * nt * sizeof(MD3_tag));
  fseek(file, header.ofsTags, SEEK_SET);
  fread(tags, sizeof(MD3_tag), nf * nt, file);

  // init links
  links = (MD3_t **)malloc(nt);
  for (i=0; i<nt; i++)
    links[i] = NULL;

  // read meshes
  int nm = header.numMeshes;
  meshes = (MD3_mesh *)malloc(nm * sizeof(MD3_mesh));
  int base = header.ofsMeshes;
  for (i=0; i<nm; i++)
    {
      int n, m;
      // go to a new mesh
      fseek(file, base, SEEK_SET);
      fread(&meshes[i].header, sizeof(MD3_mesh_header), 1, file);

      // load the shaders (skins)
      // TODO how are these used?
      n = meshes[i].header.numShaders;
      meshes[i].shaders = (MD3_shader *)malloc(n * sizeof(MD3_shader));
      fseek(file, base + meshes[i].header.ofsShaders, SEEK_SET);
      fread(meshes[i].shaders, sizeof(MD3_shader), n, file);

      // triangles
      n = meshes[i].header.numTriangles;
      meshes[i].triangles = (MD3_triangle *)malloc(n * sizeof(MD3_triangle));
      fseek(file, base + meshes[i].header.ofsTriangles, SEEK_SET);
      fread(meshes[i].triangles, sizeof(MD3_triangle), n, file);

      // texture coordinates
      n = meshes[i].header.numVertices;
      meshes[i].texcoords = (MD3_texcoord *)malloc(n * sizeof(MD3_texcoord));
      fseek(file, base + meshes[i].header.ofsST, SEEK_SET);
      fread(meshes[i].texcoords, sizeof(MD3_texcoord), n, file);

      // and vertices
      m = meshes[i].header.numFrames;
      meshes[i].vertices = (MD3_vertex *)malloc(n * m * sizeof(MD3_vertex));
      fseek(file, base + meshes[i].header.ofsVertices, SEEK_SET);
      fread(meshes[i].vertices, sizeof(MD3_vertex), n * m, file);

      meshes[i].texture = 0;
      base = base + meshes[i].header.ofsEND;
    }

  fclose(file);

  //header.numFrames--; // why?
  return true;
}

void MD3_t::LoadMeshTexture(const char *meshname, const char *imagefile)
{
  int i;
  // Find the right mesh item to assign the skin to
  for (i=0; i < header.numMeshes; i++)
    {
      // check it the two names are the same
      if (strcasecmp(meshes[i].header.name, meshname) == 0)
	meshes[i].texture = LoadTexture(imagefile);
    }
}

// reads the *.skin files and loads the mesh skin textures accordingly
bool MD3_t::LoadSkinFile(const string & path, const string & filename)
{
  char meshname[50]; // enough? 
  char imagefile[50]; 

  cout << "Loading skin file " << path << filename << endl;
  FILE *file = fopen((path+filename).c_str(), "rb");
  if (!file)
    return false;

  while (fscanf(file, "%49[^,]%49s\n", meshname, imagefile) != EOF)
    {
      if (strlen(imagefile) > 1 && strlen(meshname) > 0 &&
	  strncmp(meshname, "tag_", 4) != 0) // tags dont have skins
	{
	  // we must strip the path away from imagefile and replace it with our own:
	  char *p = strrchr(imagefile, '/'); // find last slash
	  string ifile = path + string(p+1);
	  cout << " -- mesh " << meshname << " -- Texture " << ifile << endl;
	  LoadMeshTexture(meshname, ifile.c_str());
	}
    }

  fclose(file);
  return true;
}

// link another model to a tag.
void MD3_t::Link(const char *tagname, MD3_t *m)
{
  int i;

  for (i=0; i<header.numTags; i++)
    if (strcmp(tags[i].name, tagname) == 0)
      {
	links[i] = m;
	return;
      }
}


// Draw the model interpolated between frames curr and next.
// interp must be in the range [0, 1]
void MD3_t::DrawInterpolated(MD3_animstate *st)
{
  GLfloat v1[3], v2[3], v[3], n1[3], n2[3], norm[3];
  int theta1, phi1, theta2, phi2;
  float interp = st->interp;

  // draw each mesh
  MD3_mesh *mesh = meshes;
  for (int m=0; m<header.numMeshes; m++, mesh++)
    {
      // pointers to the vertex data of the frames
      MD3_vertex *cv = &mesh->vertices[st->frame * mesh->header.numVertices];
      MD3_vertex *nv = &mesh->vertices[st->nextframe * mesh->header.numVertices];

      if (mesh->texture != 0)
	glBindTexture(GL_TEXTURE_2D, mesh->texture);

      int i, j, k, n = mesh->header.numTriangles;
      GLfloat s, t;
      glBegin(GL_TRIANGLES);
      for (i=0; i<n; i++)
	{
	  // TODO display lists? for each frame?
	  for (j=0; j<3; j++)
	    {
	      // vertex index
	      int index = mesh->triangles[i].index[j];

	      theta1 = cv[index].normal[0];
	      phi1   = cv[index].normal[1];
	      theta2 = nv[index].normal[0];
	      phi2   = nv[index].normal[1];

	      // interpolation
	      for (k=0; k<3; k++)
		{
		  n1[k] = NormLookup[theta1][phi1][k];
		  n2[k] = NormLookup[theta2][phi2][k];
		  norm[k] = (1-interp)*n1[k] + interp*n2[k];

		  v1[k] = cv[index].coord[k] / 64.0;
		  v2[k] = nv[index].coord[k] / 64.0;
		  v[k] = (1-interp)*v1[k] + interp*v2[k];
		}

	      s = mesh->texcoords[index].s;
	      t = mesh->texcoords[index].t;
	      glTexCoord2f(s, 1-t);

	      glNormal3fv(norm);
	      glVertex3fv(v);
	    }

	}
      glEnd();
    }
}

// draw this model and all the models that have been linked to it
MD3_animstate *MD3_t::DrawRecursive(MD3_animstate *st)
{
  int i, j, k;
  MD3_t *mp;

  GLfloat m[16]; // 4x4 matrix
  vec3_t trans;
  float *t1, *t2;
  rotation_t rot;
  float (*r1)[3], (*r2)[3];

  DrawInterpolated(st);
  for (i=0; i<header.numTags; i++)
    {
      mp = links[i];
      if (mp != NULL)
	{
	  // TODO matrices are not stored in a good format
	  t1 = tags[st->frame * header.numTags + i].origin;
	  t2 = tags[st->nextframe * header.numTags + i].origin;

	  r1   = tags[st->frame * header.numTags + i].rot;
	  r2   = tags[st->nextframe * header.numTags + i].rot;

	  for (j = 0; j<3; j++)
	    for (k = 0; k<3; k++)
	      rot[j][k] = (1-st->interp)*r1[j][k] + st->interp*r2[j][k];

	  for (j = 0; j<3; j++)	    
	    trans[j] = (1-st->interp)*t1[j] + st->interp*t2[j];

	  m[0] = rot[0][0];
	  m[1] = rot[0][1];
	  m[2] = rot[0][2];
	  m[3] = 0;
	  m[4] = rot[1][0];
	  m[5] = rot[1][1];
	  m[6] = rot[1][2];
	  m[7] = 0;
	  m[8] = rot[2][0];
	  m[9] = rot[2][1];
	  m[10]= rot[2][2];
	  m[11]= 0;
	  m[12] = trans[0];
	  m[13] = trans[1];
	  m[14] = trans[2];
	  m[15] = 1;

	  glPushMatrix();
	  glMultMatrixf(m);
	  st++; // important! st must be an array of animstates big enough for the 
	  // entire linked submodel complex! Not a very easy system, but should work...
	  // Note the order in which the models have been linked to one another
	  st = mp->DrawRecursive(st);
	  glPopMatrix();
	}
    }
  return st;
}

//===============================================================
// MD3_player class implementation
//===============================================================

// loads a player model to memory
// path should end in a slash
bool MD3_player::Load(const string & path, const string & skin)
{
  // TODO: load a model from PK3 archive!
  // TODO: switch skins online!

  bool ok;
  ok = legs.Load((path + string("lower.md3")).c_str()) &&
    torso.Load((path + string("upper.md3")).c_str()) &&
    head.Load((path + string("head.md3")).c_str());

  if (!ok)
    {
      cout << "problem 1\n";
      return false;
    }

  ok = legs.LoadSkinFile(path, string("lower_") + skin + string(".skin")) &&
    torso.LoadSkinFile(path, string("upper_") + skin + string(".skin")) &&
    head.LoadSkinFile(path, string("head_") + skin + string(".skin"));

  // FIXME generate textures here, now the same texture file is read N times
  // and N textures are generated...

  if (!ok)
    {
      cout << "problem 2\n";
      return false;
    }

  LoadAnim((path + string("animation.cfg")).c_str());

  legs.Link("tag_torso", &torso);
  torso.Link("tag_head", &head);

  return true;
}

// load animation sequences, *.cfg
bool MD3_player::LoadAnim(const char *filename)
{
  char buf[100];
  int first, num, loop, fps;

  cout << "Loading animations file " << filename << endl;

  FILE *file = fopen(filename, "rb");
  if (!file)
    return false;

  int i = 0;
  while (fscanf(file, "%99[^\n]\n", buf) != EOF)
    {
      // TODO these probably also mean something...
      //if (strstr(buf, "sex") != NULL) {}
      //else if (strstr(buf, "headoffset") != NULL) {}
      //else if (strstr(buf, "footsteps") != NULL) {// footstep sound}
      //else
      if (strlen(buf) > 0 && sscanf(buf, "%d %d %d %d", &first, &num, &loop, &fps) == 4)
	{
	  anim[i].firstframe = first;
	  anim[i].lastframe  = first + num - 1;
	  anim[i].loopframes = loop;
	  anim[i].fps = fps;
	  i++;
	}
    }
  fclose(file);

  // legs and torso are independent models
  int skip = anim[LEGS_WALKCR].firstframe - anim[TORSO_GESTURE].firstframe;
  for (i=LEGS_WALKCR; i<MAX_ANIMATIONS; i++)
    {
      anim[i].firstframe -= skip;
      anim[i].lastframe -= skip;
    }

  return true;
}


//==================================================
//    modelpres_t implementation
//==================================================

modelpres_t::modelpres_t(const char *name, int col, const char *skin)
{
  // TODO skin is not used
  color = col;
  mdl = models.Get(name);

  SetAnim(TORSO_STAND);
  SetAnim(LEGS_IDLE);
  st[3].nextframe = 0;
  st[3].seq = MD3_animseq_e(0);

  for (int i=0; i<3; i++)
    {
      st[i].frame = st[i].nextframe;
      st[i].interp = 0.0f;
    }

  extern tic_t gametic;

  lastupdate = gametic;
}


modelpres_t::~modelpres_t()
{
  mdl->Release();
}


// set a new animation sequence for the player
void modelpres_t::SetAnim(int seq)
{
  if (seq < 0)
    return;
  else if (seq <= BOTH_DEAD3)
    {
      // common sequences
      st[0].seq = st[1].seq = MD3_animseq_e(seq);
      st[0].nextframe = st[1].nextframe = mdl->anim[seq].firstframe;
    }
  else if (seq <= TORSO_STAND2)
    {
      // torso sequences
      st[1].seq = MD3_animseq_e(seq);
      st[1].nextframe = mdl->anim[seq].firstframe;
    }
  else if (seq <= LEGS_TURN)
    {
      // leg sequences
      st[0].seq = MD3_animseq_e(seq);
      st[0].nextframe = mdl->anim[seq].firstframe;
    }
}


bool modelpres_t::Update(int nowtic)
{
  fixed_t time = int(nowtic * FRACUNIT / 35.0); // seconds

  // For some reason head is never animated? Always frame 0 for head.
  for (int i=0; i<2; i++)
    {
      MD3_anim *a = &mdl->anim[st[i].seq];

      // advance interpolation by elapsed time
      float in = st[i].interp + a->fps * (time - lastupdate) / FRACUNIT;
      
      if (in > 10)
	in = 10; // FIXME long lapses are skipped

      while (in >= 1.0)
	{
	  in--;
	  st[i].frame = st[i].nextframe;
	  if (++(st[i].nextframe) > a->lastframe)
	    st[i].nextframe -= a->loopframes;
	}

      st[i].interp = in; // remainder
    }
  lastupdate = time;
  return true;
}


// update and draw the player model
bool modelpres_t::Draw(const Actor *p)
{
  Update(p->mp->maptic);
  /*
    // TODO T&L for models
  glPushMatrix();

  float ang = (R_PointToAngle(thing->x, thing->y) >> 24);

  glTranslatef(tr->tx, ty, -tz);

  glScalef(1,1,1);
  glRotatef((p->aiming >> 24)*(360.0f/256), 1, 0, 0);
  glRotatef((p->angle >> 24)*(360.0f/256), 0, 0, 1);

  legs.DrawRecursive(st);

  glPopMatrix();
  */

  return true;
}
