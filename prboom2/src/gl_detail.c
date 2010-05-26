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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gl_opengl.h"

#include "z_zone.h"
#include <SDL.h>
#include <SDL_opengl.h>

#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>
#endif

#include <math.h>

#include "v_video.h"
#include "r_main.h"
#include "gl_intern.h"
#include "w_wad.h"
#include "lprintf.h"
#include "p_spec.h"
#include "m_misc.h"
#include "sc_man.h"
#include "e6y.h"

int render_usedetail;
int gl_allow_detail_textures;
int gl_detail_maxdist;
float gl_detail_maxdist_sqrt;

detail_t *details;
int details_count;
int details_size;
int level_has_details;

typedef enum
{
  TAG_DETAIL_WALL,
  TAG_DETAIL_FLAT,
  TAG_DETAIL_MAX,
} tag_detail_e;

static const char *DetailItem_Keywords[TAG_DETAIL_MAX + 1] =
{
  "walls",
  "flats",
  NULL
};

static GLuint last_detail_texid = -1;

float xCamera,yCamera,zCamera;
TAnimItemParam *anim_flats = NULL;
TAnimItemParam *anim_textures = NULL;

void gld_ShutdownDetail(void);

void M_ChangeUseDetail(void)
{
  render_usedetail = false;

  if (V_GetMode() == VID_MODEGL)
  {
    render_usedetail = gl_allow_detail_textures;
    gld_EnableDetail(true);
    gld_EnableDetail(false);
  }
}

float distance2piece(float x0, float y0, float x1, float y1, float x2, float y2)
{
  float t, w;
  float x01 = x0 - x1;
  float x02 = x0 - x2;
  float x21 = x2 - x1;
  float y01 = y0 - y1;
  float y02 = y0 - y2;
  float y21 = y2 - y1;

  if((x01 * x21 + y01 * y21) * (x02 * x21 + y02 * y21) > 0.0001f)
  {
    t = x01 * x01 + y01 * y01;
    w = x02 * x02 + y02 * y02;
    if (w < t) t = w;
  }
  else
  {
    float i1 = x01 * y21 - y01 * x21;
    float i2 = x21 * x21 + y21 * y21;
    t = (i1 * i1) / i2;
  }

  return t;
}

int gld_IsDetailVisible(float x0, float y0, float x1, float y1, float x2, float y2)
{
  if (gl_detail_maxdist_sqrt == 0)
  {
    return true;
  }
  else
  {
    return (distance2piece(x0, y0, x1, y1, x2, y2) < gl_detail_maxdist_sqrt);
  }
}

void gld_InitDetail(void)
{
  gl_detail_maxdist_sqrt = (float)sqrt((float)gl_detail_maxdist);

  atexit(gld_ShutdownDetail);
  M_ChangeUseDetail();
}

void gld_ShutdownDetail(void)
{
  int i;

  if (details)
  {
    for (i = 0; i < details_count; i++)
    {
      glDeleteTextures(1, &details[i].texid);
    }

    free(details);
    details = NULL;
    details_count = 0;
    details_size = 0;
  }
}

void gld_DrawTriangleStripARB(GLWall *wall, gl_strip_coords_t *c1, gl_strip_coords_t *c2)
{
  glBegin(GL_TRIANGLE_STRIP);

  // lower left corner
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB,(const GLfloat*)&c1->t[0]); 
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB,(const GLfloat*)&c2->t[0]);
  glVertex3fv((const GLfloat*)&c1->v[0]);

  // split left edge of wall
  //if (gl_seamless && !wall->glseg->fracleft)
  //  gld_SplitLeftEdge(wall, true, w, h);

  // upper left corner
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB,(const GLfloat*)&c1->t[1]);
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB,(const GLfloat*)&c2->t[1]);
  glVertex3fv((const GLfloat*)&c1->v[1]);

  // upper right corner
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB,(const GLfloat*)&c1->t[2]); 
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB,(const GLfloat*)&c2->t[2]);
  glVertex3fv((const GLfloat*)&c1->v[2]);

  // split right edge of wall
  //if (gl_seamless && !wall->glseg->fracright)
  //  gld_SplitRightEdge(wall, true, w, h);

  // lower right corner
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB,(const GLfloat*)&c1->t[3]); 
  GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB,(const GLfloat*)&c2->t[3]);
  glVertex3fv((const GLfloat*)&c1->v[3]);

  glEnd();
}

void gld_PreprocessDetail(void)
{
  if (gl_arb_multitexture)
  {
    GLEXT_glClientActiveTextureARB(GL_TEXTURE0_ARB);
#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
    glTexCoordPointer(2, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_u);
#endif

    GLEXT_glClientActiveTextureARB(GL_TEXTURE1_ARB);
#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
    glTexCoordPointer(2, GL_FLOAT, sizeof(flats_vbo[0]), flats_vbo_u);
#endif
    GLEXT_glClientActiveTextureARB(GL_TEXTURE0_ARB);

    GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
    GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
  }
}


void gld_EnableDetail(int enable)
{
  if (!gl_arb_multitexture || !render_usedetail)
    return;

  gld_EnableTexture2D(GL_TEXTURE1_ARB, enable);
  gld_EnableClientCoordArray(GL_TEXTURE1_ARB, enable);
}

void gld_DrawWallWithDetail(GLWall *wall)
{
  float w, h, s;
  TAnimItemParam *animitem;
  dboolean fake = (wall->flag == GLDWF_TOPFLUD) || (wall->flag == GLDWF_BOTFLUD);
  detail_t *detail = &details[wall->gltexture->detail_id];
  
  if (fake)
  {
    int i;
    gl_strip_coords_t c1;
    gl_strip_coords_t c2;

    gld_BindFlat(wall->gltexture, 0);

    animitem = &anim_flats[wall->gltexture->index - firstflat];
    if (!animitem->anim)
    {
      s = 0.0f;
    }
    else
    {
      s = 1.0f / animitem->anim->numpics * animitem->index;
      if (s < 0.001) s = 0.0f;
    }

    gld_SetupFloodStencil(wall);

    w = s + wall->gltexture->realtexwidth  / detail->kx;
    h = s + wall->gltexture->realtexheight / detail->ky;

    gld_SetupFloodedPlaneLight(wall);
    gld_SetupFloodedPlaneCoords(wall, &c1);
    for (i = 0; i < 4; i++)
    {
      c2.t[i][0] = c1.t[i][0] * w;
      c2.t[i][1] = c1.t[i][1] * h;
    }

    gld_EnableTexture2D(GL_TEXTURE1_ARB, true);
    gld_DrawTriangleStripARB(wall, &c1, &c2);
    gld_EnableTexture2D(GL_TEXTURE1_ARB, false);

    gld_ClearFloodStencil(wall);

    return;
  }

  animitem = &anim_textures[wall->gltexture->index];
  if (!animitem->anim)
  {
    s = 0.0f;
  }
  else
  {
    s = 1.0f / animitem->anim->numpics * animitem->index;
    if (s < 0.001f) s = 0.0f;
  }
  w = s + (float)wall->gltexture->realtexwidth  / detail->kx;
  h = s + (float)wall->gltexture->realtexheight / detail->ky;
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
}

void gld_DrawWallDetail_NoARB(GLWall *wall, int from_index, int to_index)
{
  int k;
  for (k=from_index; k<to_index; k++)
  {
    if (wall->flag==k)
    {
      if (wall->gltexture->detail_id != -1 &&
          gld_IsDetailVisible(xCamera, yCamera, 
            wall->glseg->x1, wall->glseg->z1,
            wall->glseg->x2, wall->glseg->z2))
      {
        detail_t *detail;
        float w, h;

        detail = &details[wall->gltexture->detail_id];
        gld_BindDetail(wall->gltexture, detail->texid);

        w = wall->gltexture->realtexwidth  / detail->kx;
        h = wall->gltexture->realtexheight / detail->ky;
        gld_StaticLightAlpha(wall->light, wall->alpha);
        glBegin(GL_TRIANGLE_FAN);

        // lower left corner
        glTexCoord2f(wall->ul*w,wall->vb*h); glVertex3f(wall->glseg->x1,wall->ybottom,wall->glseg->z1);

        // split left edge of wall
        if (gl_seamless && !wall->glseg->fracleft)
          gld_SplitLeftEdge(wall, true, w, h);

        // upper left corner
        glTexCoord2f(wall->ul*w,wall->vt*h); glVertex3f(wall->glseg->x1,wall->ytop,wall->glseg->z1);

        // upper right corner
        glTexCoord2f(wall->ur*w,wall->vt*h); glVertex3f(wall->glseg->x2,wall->ytop,wall->glseg->z2);

        // split right edge of wall
        if (gl_seamless && !wall->glseg->fracright)
          gld_SplitRightEdge(wall, true, w, h);

        // lower right corner
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
  detail_t *detail;

  if (flat->gltexture->detail_id == -1)
    return;

  detail = &details[flat->gltexture->detail_id];
  gld_BindDetail(flat->gltexture, detail->texid);

  gld_StaticLightAlpha(flat->light, flat->alpha);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0.0f,flat->z,0.0f);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  if (flat->flags & GLFLAT_HAVE_OFFSET)
    glTranslatef(flat->uoffs * 4.0f, flat->voffs * 4.0f, 0.0f);
  glScalef(4.0f, 4.0f, 1.0f);

  if (flat->sectornum>=0)
  {
    // go through all loops of this sector
#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
    for (loopnum=0; loopnum<sectorloops[flat->sectornum].loopcount; loopnum++)
    {
      currentloop=&sectorloops[flat->sectornum].loops[loopnum];
      glDrawArrays(currentloop->mode,currentloop->vertexindex,currentloop->vertexcount);
    }
#else
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
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, (GLfloat*)&flats_vbo[vertexnum].u);
          GLEXT_glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, (GLfloat*)&flats_vbo[vertexnum].u);
        }
        else
        {
          glTexCoord2fv((GLfloat*)&flats_vbo[vertexnum].u);
        }
        // set vertex coordinate
        glVertex3fv((GLfloat*)&flats_vbo[vertexnum].x);
      }
      // end of loop
      glEnd();
    }
#endif
  }
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

static int C_DECL dicmp_wall_detail(const void *a, const void *b)
{
  GLuint tx1 = ((const GLDrawItem *)a)->item.wall->gltexture->detail_id;
  GLuint tx2 = ((const GLDrawItem *)b)->item.wall->gltexture->detail_id;
  return tx1 - tx2;
}

static int C_DECL dicmp_flat_detail(const void *a, const void *b)
{
  GLuint tx1 = ((const GLDrawItem *)a)->item.flat->gltexture->detail_id;
  GLuint tx2 = ((const GLDrawItem *)b)->item.flat->gltexture->detail_id;
  return tx1 - tx2;
}

static void gld_DrawItemsSortByDetail(GLDrawItemType itemtype)
{
  typedef int(C_DECL *DICMP_ITEM)(const void *a, const void *b);

  static DICMP_ITEM itemfuncs[GLDIT_TYPES] = {
    0,
    dicmp_wall_detail, dicmp_wall_detail, dicmp_wall_detail, dicmp_wall_detail, dicmp_wall_detail,
    dicmp_wall_detail, dicmp_wall_detail,
    dicmp_flat_detail, dicmp_flat_detail,
    dicmp_flat_detail, dicmp_flat_detail,
    0, 0, 0,
    0,
  };

  if (itemfuncs[itemtype] && gld_drawinfo.num_items[itemtype] > 1)
  {
    qsort(gld_drawinfo.items[itemtype], gld_drawinfo.num_items[itemtype],
      sizeof(gld_drawinfo.items[itemtype]), itemfuncs[itemtype]);
  }
}

void gld_DrawDetail_NoARB(void)
{
  int i;

  if (!level_has_details)
    return;

  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);

  last_detail_texid = -1;

  // detail flats

  // enable backside removing
  glEnable(GL_CULL_FACE);

  // floors
  glCullFace(GL_FRONT);
  gld_DrawItemsSortByDetail(GLDIT_FLOOR);
  for (i = gld_drawinfo.num_items[GLDIT_FLOOR] - 1; i >= 0; i--)
  {
    gld_DrawFlatDetail_NoARB(gld_drawinfo.items[GLDIT_FLOOR][i].item.flat);
  }
  // ceilings
  glCullFace(GL_BACK);
  gld_DrawItemsSortByDetail(GLDIT_CEILING);
  for (i = gld_drawinfo.num_items[GLDIT_CEILING] - 1; i >= 0; i--)
  {
    gld_DrawFlatDetail_NoARB(gld_drawinfo.items[GLDIT_CEILING][i].item.flat);
  }
  glDisable(GL_CULL_FACE);

  // detail walls
  gld_DrawItemsSortByDetail(GLDIT_WALL);
  for (i = gld_drawinfo.num_items[GLDIT_WALL] - 1; i >= 0; i--)
    gld_DrawWallDetail_NoARB(gld_drawinfo.items[GLDIT_WALL][i].item.wall, GLDWF_TOP, GLDWF_SKY-1);

  gld_DrawItemsSortByDetail(GLDIT_MWALL);
  for (i = gld_drawinfo.num_items[GLDIT_MWALL] - 1; i >= 0; i--)
    gld_DrawWallDetail_NoARB(gld_drawinfo.items[GLDIT_MWALL][i].item.wall, GLDWF_TOP, GLDWF_SKY-1);

  gld_DrawItemsSortByDetail(GLDIT_TWALL);
  for (i = gld_drawinfo.num_items[GLDIT_TWALL] - 1; i >= 0; i--)
    gld_DrawWallDetail_NoARB(gld_drawinfo.items[GLDIT_TWALL][i].item.wall, GLDWF_TOP, GLDWF_SKY-1);

  // restore
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gld_BindDetail(GLTexture *gltexture, int enable)
{
  if (render_usedetail && gltexture->detail_id != -1)
  {
    if (gl_arb_multitexture)
    {
      gld_EnableTexture2D(GL_TEXTURE1_ARB, enable);
      gld_EnableClientCoordArray(GL_TEXTURE1_ARB, enable);

      if (enable && details[gltexture->detail_id].texid != last_detail_texid)
      {
        last_detail_texid = details[gltexture->detail_id].texid;

        GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
        glBindTexture(GL_TEXTURE_2D, details[gltexture->detail_id].texid);
        GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);
      }
    }
    else
    {
      if (enable && details[gltexture->detail_id].texid != last_detail_texid)
      {
        last_detail_texid = details[gltexture->detail_id].texid;
        glBindTexture(GL_TEXTURE_2D, details[gltexture->detail_id].texid);
      }
    }
  }
}

void gld_SetTexDetail(GLTexture *gltexture)
{
  int i;

  gltexture->detail_id = -1;

  if (details_count > 0)
  {
    // linear search
    for (i = 0; i < details_count; i++)
    {
      if (gltexture->index == details[i].texture_num)
      {
        gltexture->detail_id = i;
        break;
      }
    }

    if (gltexture->detail_id == -1)
    {
      switch (gltexture->textype)
      {
      case GLDT_TEXTURE:
        if (details[TAG_DETAIL_WALL].texid > 0)
          gltexture->detail_id = TAG_DETAIL_WALL;
        break;
      case GLDT_FLAT:
        if (details[TAG_DETAIL_FLAT].texid > 0)
          gltexture->detail_id = TAG_DETAIL_FLAT;
        break;
      }
    }

    if (gltexture->detail_id != -1)
      level_has_details = true;
  }
}

GLuint gld_LoadDetailName(const char *name)
{
  GLuint texid = 0;
  int lump;

  lump = (W_CheckNumForName)(name, ns_hires);

  if (lump != -1)
  {
    SDL_PixelFormat fmt;
    SDL_Surface *surf = NULL;
    SDL_Surface *surf_raw;
    const unsigned char *memDetail = W_CacheLumpNum(lump);
    
#ifdef HAVE_LIBSDL_IMAGE
    surf_raw = IMG_Load_RW(SDL_RWFromMem((unsigned char *)memDetail, W_LumpLength(lump)), 1);
#else
    surf_raw = SDL_LoadBMP_RW(SDL_RWFromMem((unsigned char *)memDetail, W_LumpLength(lump)), 1);
#endif
    if (surf_raw)
    {
      W_UnlockLumpNum(lump);

      fmt = *surf_raw->format;
      fmt.BitsPerPixel = 24;
      fmt.BytesPerPixel = 3;
      surf = SDL_ConvertSurface(surf_raw, &fmt, surf_raw->flags);
      SDL_FreeSurface(surf_raw);
      if (surf)
      {
        if (gl_arb_multitexture)
          GLEXT_glActiveTextureARB(GL_TEXTURE1_ARB);
        glGenTextures(1, &texid);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, texid);

        gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format,
          surf->w, surf->h, 
          imageformats[surf->format->BytesPerPixel], 
          GL_UNSIGNED_BYTE, surf->pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        if (gl_ext_texture_filter_anisotropic)
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)(1<<gl_texture_filter_anisotropic));

        if (gl_arb_multitexture)
          GLEXT_glActiveTextureARB(GL_TEXTURE0_ARB);

        SDL_FreeSurface(surf);
      }
    }
  }

  return texid;
}

int gld_ReadDetailParams(tag_detail_e item, detail_t *detail)
{
  int result = false;
  if (SC_Check())
  {
    // get detail texture name
    SC_GetString();

    if (strlen(sc_String) < 9)
    {
      detail->texid = gld_LoadDetailName(sc_String);

      if (detail->texid > 0)
      {
        float f;

        if (SC_Check() && SC_GetString() && M_StrToFloat(sc_String, &f))
          detail->kx = f;
        if (SC_Check() && SC_GetString() && M_StrToFloat(sc_String, &f))
          detail->ky = f;

        result = true;
      }
    }
    // skip the rest of unknown params
    while (SC_Check())
      SC_GetString();
  }

  return result;
}

void gld_ParseDetailItem(tag_detail_e item)
{
  // item's params
  if (SC_Check() && !SC_Compare("{"))
  {
    details[item].kx = 16.0f;
    details[item].ky = 16.0f;
    gld_ReadDetailParams(item, &details[item]);
  }

  if (SC_GetString() && SC_Compare("{"))
  {
    while (SC_GetString() && !SC_Compare("}"))
    {
      int result;
      detail_t detail;

      if (strlen(sc_String) < 9)
      {
        switch (item)
        {
        case TAG_DETAIL_WALL:
          detail.texture_num = R_CheckTextureNumForName(sc_String);
          break;
        case TAG_DETAIL_FLAT:
          detail.texture_num = (W_CheckNumForName)(sc_String, ns_flats);
          break;
        }

        result = gld_ReadDetailParams(item, &detail);

        if (result || details[item].texid > 0)
        {
          if (details_count + 1 > details_size)
          {
            details_size = (details_size == 0 ? 128 : details_size * 2);
            details = realloc(details, details_size * sizeof(details[0]));
          }
          details[details_count] = detail;
          details_count++;
        }
      }
    }
  }
}

void gld_ParseDetail(void)
{
  free(details);
  details_count = 2; // reserved for default wall and flat
  details_size = 128;
  details = calloc(details_size, sizeof(details[0]));

  level_has_details = false;

  // skip "Detail" params
  while (SC_Check() && !SC_Compare("{"))
    SC_GetString();

  if (SC_GetString() && SC_Compare("{"))
  {
    while (SC_GetString())
    {
      switch (SC_MatchString(DetailItem_Keywords))
      {
      case TAG_DETAIL_WALL:
        gld_ParseDetailItem(TAG_DETAIL_WALL);
        break;
      case TAG_DETAIL_FLAT:
        gld_ParseDetailItem(TAG_DETAIL_FLAT);
        break;
      }
    }
  }
}
