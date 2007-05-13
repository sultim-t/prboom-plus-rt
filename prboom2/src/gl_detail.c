/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

#include "z_zone.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "v_video.h"
#include "r_main.h"
#include "gl_intern.h"
#include "w_wad.h"
#include "lprintf.h"
#include "e6y.h"

int glversion;

unsigned int idDetail;
boolean gl_arb_multitexture;
int render_canusedetail;

PFNGLACTIVETEXTUREARBPROC        GLEXT_glActiveTextureARB       = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC  GLEXT_glClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC      GLEXT_glMultiTexCoord2fARB     = NULL;
PFNGLMULTITEXCOORD2FVARBPROC     GLEXT_glMultiTexCoord2fvARB    = NULL;

float xCamera,yCamera;
TAnimItemParam *anim_flats = NULL;
TAnimItemParam *anim_textures = NULL;

float distance2piece(float x0, float y0, float x1, float y1, float x2, float y2)
{
  float t, w;
  
  float x01 = x0-x1;
  float x02 = x0-x2;
  float x21 = x2-x1;
  float y01 = y0-y1;
  float y02 = y0-y2;
  float y21 = y2-y1;

  if((x01*x21+y01*y21)*(x02*x21+y02*y21)>0.0001f)
  {
    t = x01*x01 + y01*y01;
    w = x02*x02 + y02*y02;
    if (w < t) t = w;
  }
  else
  {
    float i1 = x01*y21-y01*x21;
    float i2 = x21*x21+y21*y21;
    t = (i1*i1)/i2;
  }
  return t;
}

void gld_InitExtensionsEx(void)
{
#define isExtensionSupported(ext) strstr(extensions, ext)
  static int imageformats[5] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};

  extern int gl_tex_filter;
  extern int gl_mipmap_filter;
  extern int gl_texture_filter_anisotropic;
  extern int gl_tex_format;

  const GLubyte *extensions = glGetString(GL_EXTENSIONS);

  gl_arb_multitexture = isExtensionSupported("GL_ARB_multitexture") != NULL;

  if (gl_arb_multitexture)
  {
    GLEXT_glActiveTextureARB       = SDL_GL_GetProcAddress("glActiveTextureARB");
    GLEXT_glClientActiveTextureARB = SDL_GL_GetProcAddress("glClientActiveTextureARB");
    GLEXT_glMultiTexCoord2fARB     = SDL_GL_GetProcAddress("glMultiTexCoord2fARB");
    GLEXT_glMultiTexCoord2fvARB    = SDL_GL_GetProcAddress("glMultiTexCoord2fvARB");

    if (!GLEXT_glActiveTextureARB   || !GLEXT_glClientActiveTextureARB ||
        !GLEXT_glMultiTexCoord2fARB || !GLEXT_glMultiTexCoord2fvARB)
      gl_arb_multitexture = false;
  }
  //gl_arb_multitexture = false;

  render_canusedetail = false;
  //if (gl_arb_multitexture)
  {
    int gldetail_lumpnum = (W_CheckNumForName)("GLDETAIL", ns_prboom);
    if (gldetail_lumpnum != -1)
    {
      const unsigned char *memDetail=W_CacheLumpNum(gldetail_lumpnum);
      SDL_PixelFormat fmt;
      SDL_Surface *surf = NULL;
      
      surf = SDL_LoadBMP_RW(SDL_RWFromMem((unsigned char *)memDetail, W_LumpLength(gldetail_lumpnum)), 1);
      W_UnlockLumpName("PLAYPAL");
      
      fmt = *surf->format;
      fmt.BitsPerPixel = 24;
      fmt.BytesPerPixel = 3;
      surf = SDL_ConvertSurface(surf, &fmt, surf->flags);
      if (surf)
      {
        if (gl_arb_multitexture)
          GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
        glGenTextures(1, &idDetail);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, idDetail);
        
        gluBuild2DMipmaps(GL_TEXTURE_2D, 
          surf->format->BytesPerPixel, 
          surf->w, surf->h, 
          imageformats[surf->format->BytesPerPixel], 
          GL_UNSIGNED_BYTE, surf->pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        if (gl_texture_filter_anisotropic)
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0);
        
        if (gl_arb_multitexture)
          GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
        
        SDL_FreeSurface(surf);
        render_canusedetail = true;
      }
    }
  }
  M_ChangeUseDetail();
  if (gl_arb_multitexture)
    lprintf(LO_INFO,"e6y: using GL_ARB_multitexture\n");
}

void gld_PreprocessDetail(void)
{
  if (gl_arb_multitexture)
  {
    GLEXT_glClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
    GLEXT_glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2,GL_FLOAT,0,gld_texcoords);
    GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
    GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
  }
}

void gld_InitGLVersion(void)
{
  int MajorVersion, MinorVersion;
  glversion = OPENGL_VERSION_1_0;
  if (sscanf(glGetString(GL_VERSION), "%d.%d", &MajorVersion, &MinorVersion) == 2)
  {
    if (MajorVersion > 1)
    {
      glversion = OPENGL_VERSION_2_0;
      if (MinorVersion > 0) glversion = OPENGL_VERSION_2_1;
    }
    else
    {
      glversion = OPENGL_VERSION_1_0;
      if (MinorVersion > 0) glversion = OPENGL_VERSION_1_1;
      if (MinorVersion > 1) glversion = OPENGL_VERSION_1_2;
      if (MinorVersion > 2) glversion = OPENGL_VERSION_1_3;
      if (MinorVersion > 3) glversion = OPENGL_VERSION_1_4;
      if (MinorVersion > 4) glversion = OPENGL_VERSION_1_5;
    }
  }
}

void gld_DrawWallWithDetail(GLWall *wall)
{
  float w, h, s;
  GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
  glEnable(GL_TEXTURE_2D);
  if (anim_textures[wall->gltexture->index].count==0)
  {
    s = 0.0f;
  }
  else
  {
    s = 1.0f/anim_textures[wall->gltexture->index].count*
      (anim_textures[wall->gltexture->index].index);
    if (s < 0.001) s = 0.0f;
  }
  w = s + wall->gltexture->realtexwidth / 18.0f;
  h = s + wall->gltexture->realtexheight / 18.0f;
  gld_StaticLightAlpha(wall->light, wall->alpha);

  glBegin(GL_TRIANGLE_FAN);

  // lower left corner
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE0_ARB,wall->ul,wall->vb); 
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE1_ARB,wall->ul*w,wall->vb*h);
  glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);

  // split left edge of wall
  if (gl_seamless && !wall->glseg->fracleft)
    gld_SplitLeftEdge(wall, true, w, h);

  // upper left corner
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE0_ARB,wall->ul,wall->vt);
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE1_ARB,wall->ul*w,wall->vt*h);
  glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);

  // upper right corner
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE0_ARB,wall->ur,wall->vt); 
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE1_ARB,wall->ur*w,wall->vt*h);
  glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);

  // split right edge of wall
  if (gl_seamless && !wall->glseg->fracright)
    gld_SplitRightEdge(wall, true, w, h);

  // lower right corner
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE0_ARB,wall->ur,wall->vb); 
  GLEXT_glMultiTexCoord2fARB(GL_TEXTURE1_ARB,wall->ur*w,wall->vb*h);
  glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);

  glEnd();

  glDisable(GL_TEXTURE_2D);
  GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
}

void gld_DrawWallDetail_NoARB(GLWall *wall, int from_index, int to_index)
{
  int k;
  for (k=from_index; k<to_index; k++)
  {
    if (wall->flag==k)
    {
      if (distance2piece(xCamera, yCamera, 
        wall->glseg->x1, wall->glseg->z1,
        wall->glseg->x2, wall->glseg->z2) < DETAIL_DISTANCE)
      {
        float w, h;
        w = wall->gltexture->realtexwidth / 18.0f;
        h = wall->gltexture->realtexheight / 18.0f;
        gld_StaticLightAlpha(wall->light, wall->alpha);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(wall->ul*w,wall->vt*h); glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);
        glTexCoord2f(wall->ul*w,wall->vb*h); glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);
        glTexCoord2f(wall->ur*w,wall->vt*h); glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);
        glTexCoord2f(wall->ur*w,wall->vb*h); glVertex3f(wall->glseg->x2,wall->ybottom,wall->glseg->z2);
        glEnd();
      }
    }
  }
}

void gld_DrawFlatDetail_NoARB(GLFlat *flat)
{
  int loopnum;
  GLLoopDef *currentloop;

  gld_StaticLight(flat->light);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,flat->z,0.0f);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslatef(flat->uoffs/16.0f,flat->voffs/16.0f,0.0f);
  glScalef(4.0f, 4.0f, 1.0f);

  if (flat->sectornum>=0)
  {
    // go through all loops of this sector
#ifndef USE_VERTEX_ARRAYS
    for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
    {
      int vertexnum;
      // set the current loop
      currentloop=&sectorloops[flat->sectornum].loops[loopnum];
      if (!currentloop)
        continue;
      // set the mode (GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN)
      glBegin(currentloop->mode);
      // go through all vertexes of this loop
      for (vertexnum=currentloop->vertexindex; vertexnum<(currentloop->vertexindex+currentloop->vertexcount); vertexnum++)
      {
        // set texture coordinate of this vertex
        if (gl_arb_multitexture && render_detailedflats)
        {
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, (GLfloat*)&gld_texcoords[vertexnum]);
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, (GLfloat*)&gld_texcoords[vertexnum]);
        }
        else
        {
          glTexCoord2fv((GLfloat*)&gld_texcoords[vertexnum]);
        }
        // set vertex coordinate
        glVertex3fv((GLfloat*)&gld_vertexes[vertexnum]);
      }
      // end of loop
      glEnd();
    }
#else
    for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
    {
      currentloop=&sectorloops[flat->sectornum].loops[loopnum];
      glDrawArrays(currentloop->mode,currentloop->vertexindex,currentloop->vertexcount);
    }
#endif
  }
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void gld_DrawDetail_NoARB(void)
{
  int i;

  if (render_usedetail)
  {
    glBindTexture(GL_TEXTURE_2D, idDetail);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);
    
    if (render_detailedflats)
    {
      glEnable(GL_CULL_FACE);
      // floors
      glCullFace(GL_FRONT);
      for (i=gld_drawinfo.num_flats-1; i>=0; i--)
      {
        if (!gld_drawinfo.flats[i].ceiling)
          gld_DrawFlatDetail_NoARB(&gld_drawinfo.flats[i]);
      }
      // ceilings
      glCullFace(GL_BACK);
      for (i=gld_drawinfo.num_flats-1; i>=0; i--)
      {
        if (gld_drawinfo.flats[i].ceiling)
          gld_DrawFlatDetail_NoARB(&gld_drawinfo.flats[i]);
      }
      glDisable(GL_CULL_FACE);
    }

    if (render_detailedwalls)
    {
      for (i=gld_drawinfo.num_walls-1; i>=0; i--)
        gld_DrawWallDetail_NoARB(&gld_drawinfo.walls[i], GLDWF_TOP, GLDWF_SKY-1);

      for (i=gld_drawinfo.num_twalls-1; i>=0; i--)
        gld_DrawWallDetail_NoARB(&gld_drawinfo.twalls[i], GLDWF_TOP, GLDWF_SKY-1);
    }
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
}
