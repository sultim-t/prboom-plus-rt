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
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include "doomtype.h"
#include "w_wad.h"
#include "m_argv.h"
#include "d_event.h"
#include "v_video.h"
#include "doomstat.h"
#include "r_bsp.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_sky.h"
#include "r_plane.h"
#include "r_data.h"
#include "p_maputl.h"
#include "p_tick.h"
#include "m_bbox.h"
#include "lprintf.h"
#include "gl_intern.h"
#include "gl_struct.h"
#include "p_spec.h"
#include "e6y.h"

int imageformats[5] = {0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};

/* TEXTURES */
static GLTexture **gld_GLTextures=NULL;
/* PATCHES FLATS SPRITES */
static GLTexture **gld_GLPatchTextures=NULL;
static GLTexture **gld_GLStaticPatchTextures=NULL;

tex_filter_t tex_filter[MIP_COUNT];

const char *gl_tex_format_string;
//int gl_tex_format=GL_RGBA8;
int gl_tex_format=GL_RGB5_A1;
//int gl_tex_format=GL_RGBA4;
//int gl_tex_format=GL_RGBA2;

// e6y: development aid to see texture mip usage
int gl_color_mip_levels;

int gl_boom_colormaps = -1;
int gl_boom_colormaps_default;

GLuint* last_glTexID = NULL;

int transparent_pal_index;
unsigned char gld_palmap[256];

void gld_ResetLastTexture(void)
{
  last_glTexID = NULL;
}

void gld_InitPalettedTextures(void)
{
  const unsigned char *playpal;
  int pal[256];
  int i,j;

  playpal = V_GetPlaypal();
  for (i=0; i<256; i++) {
    pal[i] = (playpal[i*3+0] << 16) | (playpal[i*3+1] << 8) | playpal[i*3+2];
    gld_palmap[i] = i;
  }
  transparent_pal_index = -1;
  for (i=0; i<256; i++) {
    for (j=i+1; j<256; j++) {
      if (pal[i] == pal[j]) {
        transparent_pal_index = j;
        gld_palmap[j] = i;
        break;
      }
    }
    if (transparent_pal_index >= 0)
      break;
  }
}

int gld_GetTexDimension(int value)
{
  int i;

  if (value > gl_max_texture_size)
    value = gl_max_texture_size;
  
  if (gl_arb_texture_non_power_of_two)
    return value;
  
  i = 1;
  while (i < value)
    i += i;

  return i;
}

// e6y
// Creates TIntDynArray
void* NewIntDynArray(int dimCount, int *dims)
{
  int i, dim;
  int tableOffset;
  int bufferSize = 0;
  int tableSize = 1;
  void* buffer;

  for(dim = 0; dim < dimCount - 1; dim++)
  {
    tableSize *= dims[dim];
    bufferSize += sizeof(int*) * tableSize;
  }

  bufferSize += sizeof(int) * tableSize * dims[dimCount - 1];

  buffer = calloc(1, bufferSize);
  if(!buffer)
  {
    return 0;
  }

  tableOffset = 0;
  tableSize = 1;

  for(dim = 0; dim < dimCount - 1; dim++)
  {
    tableSize *= dims[dim];
    tableOffset += tableSize;

    for(i = 0; i < tableSize; i++)
    {
      if(dim < dimCount - 2)
      {
        *((int***)buffer + tableOffset - tableSize + i) =
          (((int**)buffer + tableOffset + i*dims[dim + 1]));
      }
      else
      {
        *((int**)buffer + tableOffset - tableSize + i) =
          ((int*)((int**)buffer + tableOffset) + i*dims[dim + 1]);
      }
    }
  }

  return buffer;
}

// e6y
// Get index of player->fixedcolormap for GLTexture().glTexExID array
// There are three known values for player->fixedcolormap: 0, 1 and 32
// 0 (normal) -> 0; 1 (pw_infrared) -> 1; 32 (pw_invulnerability) -> 2
void gld_GetTextureTexID(GLTexture *gltexture, int cm)
{
  static int data[NUMCOLORMAPS+1] = {
     0,  1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
     2
  };

  gltexture->cm = cm;
  gltexture->player_cm = 0;

  if (!gl_boom_colormaps)
  {
    gltexture->texflags_p = &gltexture->texflags[cm][0];
    gltexture->texid_p = &gltexture->glTexExID[cm][0][0];
    return;
  }

  if (!(gltexture->flags & GLTEXTURE_HIRES))
  {
    gltexture->player_cm = data[frame_fixedcolormap];
  }

  gltexture->texflags_p = &gltexture->texflags[cm][gltexture->player_cm];
  gltexture->texid_p = &gltexture->glTexExID[cm][gltexture->player_cm][boom_cm];
  return;
}

// e6y
// The common function for adding textures and patches
// Used by gld_AddNewGLTexture and gld_AddNewGLPatchTexture
static GLTexture *gld_AddNewGLTexItem(int num, int count, GLTexture ***items)
{
  if (num<0)
    return NULL;
  if (num>=count)
    return NULL;
  if (!(*items))
  {
    (*items)=Z_Calloc(count, sizeof(GLTexture *),PU_STATIC,0);
  }
  if (!(*items)[num])
  {
    (*items)[num]=Z_Calloc(1, sizeof(GLTexture),PU_STATIC,0);
    (*items)[num]->textype=GLDT_UNREGISTERED;

    //if (gl_boom_colormaps)
    {
      GLTexture *texture = (*items)[num];
      int dims[3] = {(CR_LIMIT+MAXPLAYERS), (PLAYERCOLORMAP_COUNT), numcolormaps};
      texture->glTexExID = NewIntDynArray(3, dims);
    }
  }
  return (*items)[num];
}

static GLTexture *gld_AddNewGLTexture(int texture_num)
{
  return gld_AddNewGLTexItem(texture_num, numtextures, &gld_GLTextures);
}

static GLTexture *gld_AddNewGLPatchTexture(int lump)
{
  if (lumpinfo[lump].flags & LUMP_STATIC)
    return gld_AddNewGLTexItem(lump, numlumps, &gld_GLStaticPatchTextures);
  else
    return gld_AddNewGLTexItem(lump, numlumps, &gld_GLPatchTextures);
}

void gld_SetTexturePalette(GLenum target)
{
  const unsigned char *playpal;
  unsigned char pal[1024];
  int i;

  playpal = V_GetPlaypal();
  for (i=0; i<256; i++) {
    pal[i*4+0] = playpal[i*3+0];
    pal[i*4+1] = playpal[i*3+1];
    pal[i*4+2] = playpal[i*3+2];
    pal[i*4+3] = 255;
  }
  pal[transparent_pal_index*4+0]=0;
  pal[transparent_pal_index*4+1]=0;
  pal[transparent_pal_index*4+2]=0;
  pal[transparent_pal_index*4+3]=0;
  GLEXT_glColorTableEXT(target, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, pal);
}

static void gld_AddPatchToTexture_UnTranslated(GLTexture *gltexture, unsigned char *buffer, const rpatch_t *patch, int originx, int originy, int paletted)
{
  int x,y,j;
  int xs,xe;
  int js,je;
  const rcolumn_t *column;
  const byte *source;
  int i, pos;
  const unsigned char *playpal;

  if (!gltexture)
    return;
  if (!patch)
    return;
  playpal = V_GetPlaypal();
  xs=0;
  xe=patch->width;
  if ((xs+originx)>=gltexture->realtexwidth)
    return;
  if ((xe+originx)<=0)
    return;
  if ((xs+originx)<0)
    xs=-originx;
  if ((xe+originx)>gltexture->realtexwidth)
    xe+=(gltexture->realtexwidth-(xe+originx));
  
  //e6y
  if (patch->flags&PATCH_HASHOLES)
    gltexture->flags |= GLTEXTURE_HASHOLES;

  for (x=xs;x<xe;x++)
  {
#ifdef RANGECHECK
    if (x>=patch->width)
    {
      lprintf(LO_ERROR,"gld_AddPatchToTexture_UnTranslated x>=patch->width (%i >= %i)\n",x,patch->width);
      return;
    }
#endif
    column = &patch->columns[x];
    for (i=0; i<column->numPosts; i++) {
      const rpost_t *post = &column->posts[i];
      y=(post->topdelta+originy);
      js=0;
      je=post->length;
      if ((js+y)>=gltexture->realtexheight)
        continue;
      if ((je+y)<=0)
        continue;
      if ((js+y)<0)
        js=-y;
      if ((je+y)>gltexture->realtexheight)
        je+=(gltexture->realtexheight-(je+y));
      source = column->pixels + post->topdelta;
      if (paletted) {
        pos=(((js+y)*gltexture->buffer_width)+x+originx);
        for (j=js;j<je;j++,pos+=(gltexture->buffer_width))
        {
#ifdef RANGECHECK
          if (pos>=gltexture->buffer_size)
          {
            lprintf(LO_ERROR,"gld_AddPatchToTexture_UnTranslated pos>=size (%i >= %i)\n",pos+3,gltexture->buffer_size);
            return;
          }
#endif
          buffer[pos]=gld_palmap[source[j]];
        }
      } else {
        pos=4*(((js+y)*gltexture->buffer_width)+x+originx);
        for (j=js;j<je;j++,pos+=(4*gltexture->buffer_width))
        {
#ifdef RANGECHECK
          if ((pos+3)>=gltexture->buffer_size)
          {
            lprintf(LO_ERROR,"gld_AddPatchToTexture_UnTranslated pos+3>=size (%i >= %i)\n",pos+3,gltexture->buffer_size);
            return;
          }
#endif
          //e6y: Boom's color maps
          if (gl_boom_colormaps && use_boom_cm && !(comp[comp_skymap] && (gltexture->flags&GLTEXTURE_SKY)))
          {
            const lighttable_t *colormap = (fixedcolormap ? fixedcolormap : fullcolormap);
            buffer[pos+0]=playpal[colormap[source[j]]*3+0];
            buffer[pos+1]=playpal[colormap[source[j]]*3+1];
            buffer[pos+2]=playpal[colormap[source[j]]*3+2];
          }
          else
          {
            buffer[pos+0]=playpal[source[j]*3+0];
            buffer[pos+1]=playpal[source[j]*3+1];
            buffer[pos+2]=playpal[source[j]*3+2];
          }
          buffer[pos+3]=255;
        }
      }
    }
  }
}

void gld_AddPatchToTexture(GLTexture *gltexture, unsigned char *buffer, const rpatch_t *patch, int originx, int originy, int cm, int paletted)
{
  int x,y,j;
  int xs,xe;
  int js,je;
  const rcolumn_t *column;
  const byte *source;
  int i, pos;
  const unsigned char *playpal;
  const unsigned char *outr;

  if (!gltexture)
    return;
  if (!patch)
    return;
  if ((cm==CR_DEFAULT) || (cm==CR_LIMIT))
  {
    gld_AddPatchToTexture_UnTranslated(gltexture,buffer,patch,originx,originy, paletted);
    return;
  }
  if (cm<CR_LIMIT)
    outr=colrngs[cm];
  else
    outr=translationtables + 256*((cm-CR_LIMIT)-1);
  playpal = V_GetPlaypal();
  xs=0;
  xe=patch->width;
  if ((xs+originx)>=gltexture->realtexwidth)
    return;
  if ((xe+originx)<=0)
    return;
  if ((xs+originx)<0)
    xs=-originx;
  if ((xe+originx)>gltexture->realtexwidth)
    xe+=(gltexture->realtexwidth-(xe+originx));

  //e6y
  if (patch->flags&PATCH_HASHOLES)
    gltexture->flags |= GLTEXTURE_HASHOLES;

  for (x=xs;x<xe;x++)
  {
#ifdef RANGECHECK
    if (x>=patch->width)
    {
      lprintf(LO_ERROR,"gld_AddPatchToTexture x>=patch->width (%i >= %i)\n",x,patch->width);
      return;
    }
#endif
    column = &patch->columns[x];
    for (i=0; i<column->numPosts; i++) {
      const rpost_t *post = &column->posts[i];
      y=(post->topdelta+originy);
      js=0;
      je=post->length;
      if ((js+y)>=gltexture->realtexheight)
        continue;
      if ((je+y)<=0)
        continue;
      if ((js+y)<0)
        js=-y;
      if ((je+y)>gltexture->realtexheight)
        je+=(gltexture->realtexheight-(je+y));
      source = column->pixels + post->topdelta;
      if (paletted) {
        pos=(((js+y)*gltexture->buffer_width)+x+originx);
        for (j=js;j<je;j++,pos+=(gltexture->buffer_width))
        {
#ifdef RANGECHECK
          if (pos>=gltexture->buffer_size)
          {
            lprintf(LO_ERROR,"gld_AddPatchToTexture_UnTranslated pos>=size (%i >= %i)\n",pos+3,gltexture->buffer_size);
            return;
          }
#endif
          buffer[pos]=gld_palmap[outr[source[j]]];
        }
      } else {
        pos=4*(((js+y)*gltexture->buffer_width)+x+originx);
        for (j=js;j<je;j++,pos+=(4*gltexture->buffer_width))
        {
#ifdef RANGECHECK
          if ((pos+3)>=gltexture->buffer_size)
          {
            lprintf(LO_ERROR,"gld_AddPatchToTexture pos+3>=size (%i >= %i)\n",pos+3,gltexture->buffer_size);
            return;
          }
#endif
          //e6y: Boom's color maps
          if (gl_boom_colormaps && use_boom_cm)
          {
            const lighttable_t *colormap = (fixedcolormap ? fixedcolormap : fullcolormap);
            buffer[pos+0]=playpal[colormap[outr[source[j]]]*3+0];
            buffer[pos+1]=playpal[colormap[outr[source[j]]]*3+1];
            buffer[pos+2]=playpal[colormap[outr[source[j]]]*3+2];
          }
          else
          {
            buffer[pos+0]=playpal[outr[source[j]]*3+0];
            buffer[pos+1]=playpal[outr[source[j]]*3+1];
            buffer[pos+2]=playpal[outr[source[j]]*3+2];
          }
          buffer[pos+3]=255;
        }
      }
    }
  }
}

static void gld_AddFlatToTexture(GLTexture *gltexture, unsigned char *buffer, const unsigned char *flat, int paletted)
{
  int x,y,pos;
  const unsigned char *playpal;

  if (!gltexture)
    return;
  if (!flat)
    return;
  if (paletted) {
    for (y=0;y<gltexture->realtexheight;y++)
    {
      pos=(y*gltexture->buffer_width);
      for (x=0;x<gltexture->realtexwidth;x++,pos++)
      {
#ifdef RANGECHECK
        if (pos>=gltexture->buffer_size)
        {
          lprintf(LO_ERROR,"gld_AddFlatToTexture pos>=size (%i >= %i)\n",pos,gltexture->buffer_size);
          return;
        }
#endif
        buffer[pos]=gld_palmap[flat[y*64+x]];
      }
    }
  } else {
    playpal = V_GetPlaypal();
    for (y=0;y<gltexture->realtexheight;y++)
    {
      pos=4*(y*gltexture->buffer_width);
      for (x=0;x<gltexture->realtexwidth;x++,pos+=4)
      {
#ifdef RANGECHECK
        if ((pos+3)>=gltexture->buffer_size)
        {
          lprintf(LO_ERROR,"gld_AddFlatToTexture pos+3>=size (%i >= %i)\n",pos+3,gltexture->buffer_size);
          return;
        }
#endif
        //e6y: Boom's color maps
        if (gl_boom_colormaps && use_boom_cm)
        {
          const lighttable_t *colormap = (fixedcolormap ? fixedcolormap : fullcolormap);
          buffer[pos+0]=playpal[colormap[flat[y*64+x]]*3+0];
          buffer[pos+1]=playpal[colormap[flat[y*64+x]]*3+1];
          buffer[pos+2]=playpal[colormap[flat[y*64+x]]*3+2];
        }
        else
        {
          buffer[pos+0]=playpal[flat[y*64+x]*3+0];
          buffer[pos+1]=playpal[flat[y*64+x]*3+1];
          buffer[pos+2]=playpal[flat[y*64+x]*3+2];
        }
        buffer[pos+3]=255;
      }
    }
  }
}

//e6y: "force" flag for loading texture with zero index
GLTexture *gld_RegisterTexture(int texture_num, dboolean mipmap, dboolean force)
{
  GLTexture *gltexture;

  //e6y: textures with zero index should be loaded sometimes
  if (texture_num==NO_TEXTURE && !force)
    return NULL;
  gltexture=gld_AddNewGLTexture(texture_num);
  if (!gltexture)
    return NULL;
  if (gltexture->textype==GLDT_UNREGISTERED)
  {
    texture_t *texture=NULL;

    if ((texture_num>=0) || (texture_num<numtextures))
      texture=textures[texture_num];
    if (!texture)
      return NULL;
    gltexture->textype=GLDT_BROKEN;
    gltexture->index=texture_num;
    
    //e6y
    gltexture->flags = 0;
    if (mipmap && tex_filter[MIP_TEXTURE].mipmap)
      gltexture->flags |= GLTEXTURE_MIPMAP;

    gltexture->realtexwidth=texture->width;
    gltexture->realtexheight=texture->height;
    gltexture->leftoffset=0;
    gltexture->topoffset=0;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=MIN(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=MIN(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=gltexture->tex_width;
    gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
    gltexture->width=gltexture->tex_width;
    gltexture->height=gltexture->tex_height;
    gltexture->buffer_width=gltexture->realtexwidth;
    gltexture->buffer_height=gltexture->realtexheight;
#endif
    if (gltexture->flags & GLTEXTURE_MIPMAP)
    {
      gltexture->width=gltexture->tex_width;
      gltexture->height=gltexture->tex_height;
      gltexture->buffer_width=gltexture->realtexwidth;
      gltexture->buffer_height=gltexture->realtexheight;
    }
    
    //e6y: right/bottom UV coordinates for texture drawing
    gltexture->scalexfac=(float)gltexture->width/(float)gltexture->tex_width;
    gltexture->scaleyfac=(float)gltexture->height/(float)gltexture->tex_height;

    gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
    if (gltexture->realtexwidth>gltexture->buffer_width)
      return gltexture;
    if (gltexture->realtexheight>gltexture->buffer_height)
      return gltexture;

    gltexture->textype=GLDT_TEXTURE;

    gld_SetTexDetail(gltexture);
  }
  return gltexture;
}

unsigned char* gld_GetTextureBuffer(GLuint texid, int miplevel, int *width, int *height)
{
  int w, h;
  static unsigned char *buf = NULL;
  static int buf_size = 512 * 256 * 4;

  if (!buf)
  {
    buf = malloc(buf_size);
  }

  if (texid)
  {
    glBindTexture(GL_TEXTURE_2D, texid);
  }

  glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &w);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &h);
  if (w * h * 4 > buf_size)
  {
    free(buf);
    buf_size = w * h * 4;
    buf = malloc(buf_size);
  }
  glGetTexImage(GL_TEXTURE_2D, miplevel, GL_RGBA, GL_UNSIGNED_BYTE, buf);

  if (width)
    *width = w;
  if (height)
    *height = h;

  return buf;
}

// e6y: from Quake3
// R_BlendOverTexture
// Apply a color blend over a set of pixels
static void gld_BlendOverTexture(byte *data, int pixelCount, byte blend[4])
{
  int i;
  int inverseAlpha;
  int premult[3];

  inverseAlpha = 255 - blend[3];
  premult[0] = blend[0] * blend[3];
  premult[1] = blend[1] * blend[3];
  premult[2] = blend[2] * blend[3];

  for(i = 0; i < pixelCount; i++, data += 4)
  {
    data[0] = (data[0] * inverseAlpha + premult[0]) >> 9;
    data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
    data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
  }
}

byte	mipBlendColors[16][4] =
{
  {0,0,0,0},
  {255,0,0,128},
  {0,255,0,128},
  {0,0,255,128},
  {255,0,0,128},
  {0,255,0,128},
  {0,0,255,128},
  {255,0,0,128},
  {0,255,0,128},
  {0,0,255,128},
  {255,0,0,128},
  {0,255,0,128},
  {0,0,255,128},
  {255,0,0,128},
  {0,255,0,128},
  {0,0,255,128},
};

static void gld_RecolorMipLevels(byte *data)
{
  //e6y: development aid to see texture mip usage
  if (gl_color_mip_levels)
  {
    int miplevel = 0;
    unsigned char *buf = NULL;

    for (miplevel = 1; miplevel < 16; miplevel++)
    {
      int w, h;

      buf = gld_GetTextureBuffer(0, miplevel, &w, &h);

      if (w <= 0 || h <= 0)
        break;

      gld_BlendOverTexture((byte *)buf, w * h, mipBlendColors[miplevel]);
      glTexImage2D( GL_TEXTURE_2D, miplevel, gl_tex_format, w, h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    }
  }
}

void gld_SetTexFilters(GLTexture *gltexture)
{
  int mip, mag_filter, min_filter;
  float aniso_filter = 0.0f;

  switch (gltexture->textype)
  {
  case GLDT_TEXTURE:
  case GLDT_FLAT:
    mip = MIP_TEXTURE;
    break;
  case GLDT_PATCH:
    mip = ((gltexture->flags & GLTEXTURE_SPRITE) ? MIP_SPRITE : MIP_PATCH);
    break;
  default:
    mip = MIP_TEXTURE;
    break;
  }

  if (render_usedetail && gltexture->detail)
    mag_filter = GL_LINEAR;
  else
    mag_filter = tex_filter[mip].mag_filter;

  if ((gltexture->flags & GLTEXTURE_MIPMAP) && tex_filter[mip].mipmap)
  {
    min_filter = tex_filter[mip].min_filter;
    if (gl_ext_texture_filter_anisotropic)
      aniso_filter = (GLfloat)(1<<gl_texture_filter_anisotropic);
  }
  else
  {
    min_filter =  tex_filter[mip].mag_filter;
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  if (aniso_filter > 0.0f)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso_filter);
}

void gld_SetTexClamp(GLTexture *gltexture, unsigned int flags)
{
  //if ((gltexture->flags & GLTEXTURE_CLAMPXY) != (flags & GLTEXTURE_CLAMPXY))
  /* sp1n0za 05/2010: simplify */
  if ((*gltexture->texflags_p ^ flags) & GLTEXTURE_CLAMPXY)
  {
    int need_clamp_x = (flags & GLTEXTURE_CLAMPX);
    int need_clamp_y = (flags & GLTEXTURE_CLAMPY);
    int has_clamp_x = (*gltexture->texflags_p & GLTEXTURE_CLAMPX);
    int has_clamp_y = (*gltexture->texflags_p & GLTEXTURE_CLAMPY);

    if (need_clamp_x)
    {
      if (!has_clamp_x)
      {
        *gltexture->texflags_p |= GLTEXTURE_CLAMPX;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLEXT_CLAMP_TO_EDGE);
      }
    }
    else
    {
      if (has_clamp_x)
      {
        *gltexture->texflags_p &= ~GLTEXTURE_CLAMPX;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      }
    }

    if (need_clamp_y)
    {
      if (!has_clamp_y)
      {
        *gltexture->texflags_p |= GLTEXTURE_CLAMPY;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLEXT_CLAMP_TO_EDGE);
      }
    }
    else
    {
      if (has_clamp_y)
      {
        *gltexture->texflags_p &= ~GLTEXTURE_CLAMPY;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      }
    }
  }
}

int gld_BuildTexture(GLTexture *gltexture, void *data, dboolean readonly, int width, int height)
{
  int result = false;

  int tex_width, tex_height, tex_buffer_size;
  unsigned char *tex_buffer = NULL;

  tex_width  = gld_GetTexDimension(width);
  tex_height = gld_GetTexDimension(height);
  tex_buffer_size = tex_width * tex_height * 4;

  //your video is modern
  if (gl_arb_texture_non_power_of_two)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP,
      ((gltexture->flags & GLTEXTURE_MIPMAP) ? GL_TRUE : GL_FALSE));

    glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
      tex_width, tex_height,
      0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    gld_RecolorMipLevels(data);

    gld_SetTexFilters(gltexture);

    result = true;
    goto l_exit;
  }

#ifdef USE_GLU_MIPMAP
  if (gltexture->flags & GLTEXTURE_MIPMAP)
  {
    gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format,
      width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    gld_RecolorMipLevels(data);

    gld_SetTexFilters(gltexture);

    result = true;
    goto l_exit;
  }
  else
#endif // USE_GLU_MIPMAP
  {
#ifdef USE_GLU_IMAGESCALE
    if ((width != tex_width) || (height != tex_height))
    {
      tex_buffer = malloc(tex_buffer_size);
      if (!tex_buffer)
      {
        goto l_exit;
      }

      gluScaleImage(GL_RGBA, width, height,
        GL_UNSIGNED_BYTE, data,
        tex_width, tex_height,
        GL_UNSIGNED_BYTE, tex_buffer);

      glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
        tex_width, tex_height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, tex_buffer);
    }
    else
#endif // USE_GLU_IMAGESCALE
    {
      if ((width != tex_width) || (height != tex_height))
      {
        if (width == tex_width)
        {
          tex_buffer = malloc(tex_buffer_size);
          memcpy(tex_buffer, data, width * height * 4);
        }
        else
        {
          int y;
          tex_buffer = calloc(1, tex_buffer_size);
          for (y = 0; y < height; y++)          
          {
            memcpy(tex_buffer + y * tex_width * 4,
              ((unsigned char*)data) + y * width * 4, width * 4);
          }
        }
      }
      else
      {
        tex_buffer = data;
      }

      if (gl_paletted_texture) {
        gld_SetTexturePalette(GL_TEXTURE_2D);
        glTexImage2D( GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT,
          tex_width, tex_height,
          0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, tex_buffer);
      } else {
        glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
          tex_width, tex_height,
          0, GL_RGBA, GL_UNSIGNED_BYTE, tex_buffer);
      }
    }

    gltexture->flags &= ~GLTEXTURE_MIPMAP;
    gld_SetTexFilters(gltexture);
    result = true;
  }

l_exit:
  if (result)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  if (tex_buffer && tex_buffer != data)
  {
    free(tex_buffer);
    tex_buffer = NULL;
  }

  if (!readonly)
  {
    free(data);
    data = NULL;
  }

  return result;
}

void gld_BindTexture(GLTexture *gltexture, unsigned int flags)
{
  const rpatch_t *patch;
  unsigned char *buffer;
  int w, h;

  if (!gltexture || gltexture->textype != GLDT_TEXTURE)
  {
    glBindTexture(GL_TEXTURE_2D, 0);
    last_glTexID = NULL;
    return;
  }

#ifdef HAVE_LIBSDL_IMAGE
  if (gld_LoadHiresTex(gltexture, CR_DEFAULT))
  {
    gld_SetTexClamp(gltexture, flags);
    last_glTexID = gltexture->texid_p;
    return;
  }
#endif

  gld_GetTextureTexID(gltexture, CR_DEFAULT);

  if (last_glTexID == gltexture->texid_p)
  {
    gld_SetTexClamp(gltexture, flags);
    return;
  }

  last_glTexID = gltexture->texid_p;

  if (*gltexture->texid_p != 0)
  {
    glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);
    gld_SetTexClamp(gltexture, flags);
    return;
  }

  buffer=(unsigned char*)Z_Malloc(gltexture->buffer_size,PU_STATIC,0);
  if (!(gltexture->flags & GLTEXTURE_MIPMAP) && gl_paletted_texture)
    memset(buffer,transparent_pal_index,gltexture->buffer_size);
  else
    memset(buffer,0,gltexture->buffer_size);
  patch=R_CacheTextureCompositePatchNum(gltexture->index);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0,
                        CR_DEFAULT, !(gltexture->flags & GLTEXTURE_MIPMAP) && gl_paletted_texture);
  R_UnlockTextureCompositePatchNum(gltexture->index);
  if (*gltexture->texid_p == 0)
    glGenTextures(1, gltexture->texid_p);
  glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);
  
  if (gltexture->flags & GLTEXTURE_HASHOLES)
  {
    SmoothEdges(buffer, gltexture->buffer_width, gltexture->buffer_height);
  }

  buffer = gld_HQResize(gltexture, buffer, gltexture->buffer_width, gltexture->buffer_height, &w, &h);

  gld_BuildTexture(gltexture, buffer, false, w, h);

  gld_SetTexClamp(gltexture, flags);
}

GLTexture *gld_RegisterPatch(int lump, int cm)
{
  const rpatch_t *patch;
  GLTexture *gltexture;

  gltexture=gld_AddNewGLPatchTexture(lump);
  if (!gltexture)
    return NULL;
  if (gltexture->textype==GLDT_UNREGISTERED)
  {
    patch=R_CachePatchNum(lump);
    if (!patch)
      return NULL;
    gltexture->textype=GLDT_BROKEN;
    gltexture->index=lump;

    //e6y
    gltexture->flags = 0;
    if (lump >= firstspritelump && lump > (firstspritelump + numsprites))
    {
      gltexture->flags |= GLTEXTURE_SPRITE;
      if (tex_filter[MIP_SPRITE].mipmap)
        gltexture->flags |= GLTEXTURE_MIPMAP;
    }
    else
    {
      if (tex_filter[MIP_PATCH].mipmap)
        gltexture->flags |= GLTEXTURE_MIPMAP;
    }
    //gltexture->wrap_mode = (patch->flags & PATCH_REPEAT ? GL_REPEAT : GLEXT_CLAMP_TO_EDGE);

    gltexture->realtexwidth=patch->width;
    gltexture->realtexheight=patch->height;
    gltexture->leftoffset=patch->leftoffset;
    gltexture->topoffset=patch->topoffset;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=MIN(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=MIN(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=gltexture->tex_width;
    gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
    gltexture->width=MIN(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=MIN(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=MAX(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->buffer_height=MAX(gltexture->realtexheight, gltexture->tex_height);
#endif
    if (gltexture->flags & GLTEXTURE_MIPMAP)
    {
      gltexture->width=gltexture->tex_width;
      gltexture->height=gltexture->tex_height;
      gltexture->buffer_width=gltexture->realtexwidth;
      gltexture->buffer_height=gltexture->realtexheight;
    }

    //e6y: right/bottom UV coordinates for patch drawing
    gltexture->scalexfac=(float)gltexture->width/(float)gltexture->tex_width;
    gltexture->scaleyfac=(float)gltexture->height/(float)gltexture->tex_height;

    gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
    R_UnlockPatchNum(lump);
    if (gltexture->realtexwidth>gltexture->buffer_width)
      return gltexture;
    if (gltexture->realtexheight>gltexture->buffer_height)
      return gltexture;
    gltexture->textype=GLDT_PATCH;
  }
  return gltexture;
}

void gld_BindPatch(GLTexture *gltexture, int cm)
{
  const rpatch_t *patch;
  unsigned char *buffer;
  int w, h;

  if (!gltexture || gltexture->textype != GLDT_PATCH)
  {
    glBindTexture(GL_TEXTURE_2D, 0);
    last_glTexID = NULL;
    return;
  }

#ifdef HAVE_LIBSDL_IMAGE
  if (gld_LoadHiresTex(gltexture, cm))
  {
    gld_SetTexClamp(gltexture, GLTEXTURE_CLAMPXY);
    last_glTexID = gltexture->texid_p;
    return;
  }
#endif

  gld_GetTextureTexID(gltexture, cm);

  if (last_glTexID == gltexture->texid_p)
  {
    gld_SetTexClamp(gltexture, GLTEXTURE_CLAMPXY);
    return;
  }

  last_glTexID = gltexture->texid_p;

  if (*gltexture->texid_p != 0)
  {
    glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);
    gld_SetTexClamp(gltexture, GLTEXTURE_CLAMPXY);
    return;
  }

  patch=R_CachePatchNum(gltexture->index);
  buffer=(unsigned char*)Z_Malloc(gltexture->buffer_size,PU_STATIC,0);
  if (gl_paletted_texture)
    memset(buffer,transparent_pal_index,gltexture->buffer_size);
  else
    memset(buffer,0,gltexture->buffer_size);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, cm, gl_paletted_texture);

  // e6y
  // Post-process the texture data after the buffer has been created.
  // Smooth the edges of transparent fields in the texture.
  //
  // It is a workaround to set the color of all transparent pixels
  // that border on a non-transparent pixel to the color
  // of one bordering non-transparent pixel.
  // It is necessary for textures that are not power of two
  // to avoid the lines (boxes) around the elements that change
  // on the intermission screens in Doom1 (E2, E3)
  
//  if ((gltexture->flags & (GLTEXTURE_HASHOLES | GLTEXTURE_SPRITE)) ==
//    (GLTEXTURE_HASHOLES | GLTEXTURE_SPRITE))
  if ((gltexture->flags & GLTEXTURE_HASHOLES))
  {
    SmoothEdges(buffer, gltexture->buffer_width, gltexture->buffer_height);
  }

  if (*gltexture->texid_p == 0)
    glGenTextures(1, gltexture->texid_p);
  glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);

  buffer = gld_HQResize(gltexture, buffer, gltexture->buffer_width, gltexture->buffer_height, &w, &h);

  gld_BuildTexture(gltexture, buffer, false, w, h);

  gld_SetTexClamp(gltexture, GLTEXTURE_CLAMPXY);

  R_UnlockPatchNum(gltexture->index);
}

GLTexture *gld_RegisterFlat(int lump, dboolean mipmap)
{
  GLTexture *gltexture;

  gltexture=gld_AddNewGLPatchTexture(firstflat+lump);
  if (!gltexture)
    return NULL;
  if (gltexture->textype==GLDT_UNREGISTERED)
  {
    gltexture->textype=GLDT_BROKEN;
    gltexture->index=firstflat+lump;

    //e6y
    gltexture->flags = 0;
    if (mipmap && tex_filter[MIP_TEXTURE].mipmap)
      gltexture->flags |= GLTEXTURE_MIPMAP;

    gltexture->realtexwidth=64;
    gltexture->realtexheight=64;
    gltexture->leftoffset=0;
    gltexture->topoffset=0;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=MIN(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=MIN(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=gltexture->tex_width;
    gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
    gltexture->width=gltexture->tex_width;
    gltexture->height=gltexture->tex_height;
    gltexture->buffer_width=gltexture->realtexwidth;
    gltexture->buffer_height=gltexture->realtexheight;
#endif
    if (gltexture->flags & GLTEXTURE_MIPMAP)
    {
      gltexture->width=gltexture->tex_width;
      gltexture->height=gltexture->tex_height;
      gltexture->buffer_width=gltexture->realtexwidth;
      gltexture->buffer_height=gltexture->realtexheight;
    }

    //e6y: right/bottom UV coordinates for flat drawing
    gltexture->scalexfac=(float)gltexture->width/(float)gltexture->tex_width;
    gltexture->scaleyfac=(float)gltexture->height/(float)gltexture->tex_height;

    gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
    if (gltexture->realtexwidth>gltexture->buffer_width)
      return gltexture;
    if (gltexture->realtexheight>gltexture->buffer_height)
      return gltexture;

    gltexture->textype=GLDT_FLAT;

    gld_SetTexDetail(gltexture);
  }
  return gltexture;
}

void gld_BindFlat(GLTexture *gltexture, unsigned int flags)
{
  const unsigned char *flat;
  unsigned char *buffer;
  int w, h;

  if (!gltexture || gltexture->textype != GLDT_FLAT)
  {
    glBindTexture(GL_TEXTURE_2D, 0);
    last_glTexID = NULL;
    return;
  }

#ifdef HAVE_LIBSDL_IMAGE
  if (gld_LoadHiresTex(gltexture, CR_DEFAULT))
  {
    gld_SetTexClamp(gltexture, flags);
    last_glTexID = gltexture->texid_p;
    return;
  }
#endif

  gld_GetTextureTexID(gltexture, CR_DEFAULT); 

  if (last_glTexID == gltexture->texid_p)
  {
    gld_SetTexClamp(gltexture, flags);
    return;
  }

  last_glTexID = gltexture->texid_p;

  if (*gltexture->texid_p != 0)
  {
    glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);
    gld_SetTexClamp(gltexture, flags);
    return;
  }

  flat=W_CacheLumpNum(gltexture->index);
  buffer=(unsigned char*)Z_Malloc(gltexture->buffer_size,PU_STATIC,0);
  if (!(gltexture->flags & GLTEXTURE_MIPMAP) && gl_paletted_texture)
    memset(buffer,transparent_pal_index,gltexture->buffer_size);
  else
    memset(buffer,0,gltexture->buffer_size);
  gld_AddFlatToTexture(gltexture, buffer, flat, !(gltexture->flags & GLTEXTURE_MIPMAP) && gl_paletted_texture);
  if (*gltexture->texid_p == 0)
    glGenTextures(1, gltexture->texid_p);
  glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);

  buffer = gld_HQResize(gltexture, buffer, gltexture->buffer_width, gltexture->buffer_height, &w, &h);

  gld_BuildTexture(gltexture, buffer, false, w, h);

  gld_SetTexClamp(gltexture, flags);

  W_UnlockLumpNum(gltexture->index);
}

// e6y
// The common function for cleaning textures and patches
// gld_CleanTextures and gld_CleanPatchTextures are replaced with that
static void gld_CleanTexItems(int count, GLTexture ***items)
{
  int i, j;

  if (!(*items))
    return;

  for (i=0; i<count; i++)
  {
    if ((*items)[i])
    {
      int cm, n;
      for (j=0; j<(CR_LIMIT+MAXPLAYERS); j++)
      {
        for (n=0; n<PLAYERCOLORMAP_COUNT; n++)
        {
          for (cm=0; cm<numcolormaps; cm++)
          {
            if ((*items) && (*items)[i]->glTexExID[j][n][cm])
            {
              glDeleteTextures(1,&((*items)[i]->glTexExID[j][n][cm]));
            }
          }
        }
      }
      
      Z_Free((*items)[i]->glTexExID);
      (*items)[i]->glTexExID = NULL;

      Z_Free((*items)[i]);
    }
  }
  memset((*items),0,count*sizeof(GLTexture *));
}

void gld_FlushTextures(void)
{
  gld_CleanTexItems(numtextures, &gld_GLTextures);
  gld_CleanTexItems(numlumps, &gld_GLPatchTextures);
  gld_CleanTexItems(numlumps, &gld_GLStaticPatchTextures);

  gl_has_hires = 0;
  
  gld_ResetLastTexture();
#ifdef HAVE_LIBSDL_IMAGE
  gld_HiRes_BuildTables();
#endif

  gld_InitSky();

  // do not draw anything in current frame after flushing
  gld_ResetDrawInfo();
}

static void CalcHitsCount(const byte *hitlist, int size, int *hit, int*hitcount)
{
  int i;

  if (!hitlist || !hit || !hitcount)
    return;

  *hit = 0;
  *hitcount = 0;
  for (i = 0; i < size; i++)
  {
    if (hitlist[i])
      (*hitcount)++;
  }
}

void gld_Precache(void)
{
  int i;
  byte *hitlist;
  int hit, hitcount = 0;
  GLTexture *gltexture;
  box_skybox_t *sb;

  unsigned int tics = SDL_GetTicks();

  int usehires = (gl_texture_external_hires) || 
    (gl_texture_internal_hires && r_have_internal_hires);

  if (doSkip || nodrawers)
    return;

  if (!usehires)
  {
    if (!precache)
      return;

    if (timingdemo)
      return;
  }

  gld_ProgressStart();

  {
    size_t size = numflats > numsprites  ? numflats : numsprites;
    hitlist = Z_Malloc((size_t)numtextures > size ? (size_t)numtextures : size,PU_LEVEL,0);
  }

  // Precache flats.

  memset(hitlist, 0, numflats);

  for (i = numsectors; --i >= 0; )
  {
    int j,k;
    
    int floorpic = sectors[i].floorpic;
    int ceilingpic = sectors[i].ceilingpic;
    
    anim_t *flatanims[2] = {
      anim_flats[floorpic].anim,
      anim_flats[ceilingpic].anim
    };

    hitlist[floorpic] = hitlist[ceilingpic] = 1;
    
    //e6y: animated flats
    for (k = 0; k < 2; k++)
    {
      if (flatanims[k] && !flatanims[k]->istexture)
      {
        for (j = flatanims[k]->basepic; j < flatanims[k]->basepic + flatanims[k]->numpics; j++)
          hitlist[j] = 1;
      }
    }
  }

  CalcHitsCount(hitlist, numflats, &hit, &hitcount);

  for (i = numflats; --i >= 0; )
    if (hitlist[i])
    {
      gld_ProgressUpdate("Loading Flats...", ++hit, hitcount);
      gltexture = gld_RegisterFlat(i,true);
      if (gltexture)
      {
        gld_BindFlat(gltexture, 0);
      }
    }

  // Precache textures.

  memset(hitlist, 0, numtextures);

  for (i = numsides; --i >= 0;)
  {
    int j, k;
    int bottomtexture, toptexture, midtexture;
    anim_t *textureanims[3];

    if (sides[i].special == 271 || sides[i].special == 272)
    {
      sb = R_GetBoxSkybox(sides[i].skybox_index);
      if (sb)
      {
        int texture;
        int face = 0;
        while (face < 6 && sb->faces[face])
        {
          texture = R_CheckTextureNumForName(sb->faces[face]);
          if (texture != -1)
          {
            hitlist[texture] = 1;
          }
          face++;
        }
      }
    }

    bottomtexture = sides[i].bottomtexture;
    toptexture = sides[i].toptexture;
    midtexture = sides[i].midtexture;
    
    textureanims[0] = anim_textures[bottomtexture].anim;
    textureanims[1] = anim_textures[toptexture].anim;
    textureanims[2] = anim_textures[midtexture].anim;

    hitlist[bottomtexture] =
      hitlist[toptexture] =
      hitlist[midtexture] = 1;

    //e6y: animated textures
    for (k = 0; k < 3; k++)
    {
      if (textureanims[k] && textureanims[k]->istexture)
      {
        for (j = textureanims[k]->basepic; j < textureanims[k]->basepic + textureanims[k]->numpics; j++)
          hitlist[j] = 1;
      }
    }

    //e6y: swithes
    {
      int GetPairForSwitchTexture(side_t *side);
      int pair = GetPairForSwitchTexture(&sides[i]);
      if (pair != -1)
        hitlist[pair] = 1;
    }
  }

  // Sky texture is always present.
  // Note that F_SKY1 is the name used to
  //  indicate a sky floor/ceiling as a flat,
  //  while the sky texture is stored like
  //  a wall texture, with an episode dependend
  //  name.

  if (hitlist)
    hitlist[skytexture] = usehires ? 1 : 0;

  sb = BoxSkybox_default;
  if (sb)
  {
    int texture;
    int face = 0;
    while (face < 6 && sb->faces[face])
    {
      texture = R_CheckTextureNumForName(sb->faces[face]);
      if (texture != -1)
      {
        hitlist[texture] = 1;
      }
      face++;
    }
  }

  CalcHitsCount(hitlist, numtextures, &hit, &hitcount);

  for (i = numtextures; --i >= 0; )
    if (hitlist[i])
    {
      gld_ProgressUpdate("Loading Textures...", ++hit, hitcount);
      gltexture = gld_RegisterTexture(i, i != skytexture, false);
      if (gltexture)
      {
        gld_BindTexture(gltexture, 0);
      }
    }

  // Precache sprites.
  memset(hitlist, 0, numsprites);

  if (thinkercap.next)
  {
    thinker_t *th;
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
      if (th->function == P_MobjThinker)
        hitlist[((mobj_t *)th)->sprite] = 1;
    }
  }

  hit = 0;
  hitcount = 0;
  for (i = 0; i < numsprites; i++)
  {
    if (hitlist[i])
      hitcount += 7 * sprites[i].numframes;
  }

  for (i=numsprites; --i >= 0;)
    if (hitlist[i])
      {
        int j = sprites[i].numframes;
        while (--j >= 0)
          {
            short *sflump = sprites[i].spriteframes[j].lump;
            int k = 7;
            do
            {
              gld_ProgressUpdate("Loading Sprites...", ++hit, hitcount);
              gltexture = gld_RegisterPatch(firstspritelump + sflump[k], CR_LIMIT);
              if (gltexture)
              {
                gld_BindPatch(gltexture, CR_LIMIT);
              }
            }
            while (--k >= 0);
          }
      }
  Z_Free(hitlist);

  if (gl_texture_external_hires)
  {
#ifdef HAVE_LIBSDL_IMAGE
    gld_PrecacheGUIPatches();
#endif
  }

  gld_ProgressEnd();

#ifdef USE_FBO_TECHNIQUE
  gld_InitFBO();
#endif

  // e6y: some statistics.  make sense for hires
  {
    char map[8];

    if (gamemode == commercial)
      sprintf(map, "MAP%02i", gamemap);
    else
      sprintf(map, "E%iM%i", gameepisode, gamemap);
    
    lprintf(LO_INFO, "gld_Precache: %s done in %d ms\n", map, SDL_GetTicks() - tics);
  }
}

void gld_CleanMemory(void)
{
  gld_CleanVertexData();
  gld_CleanTexItems(numtextures, &gld_GLTextures);
  gld_CleanTexItems(numlumps, &gld_GLPatchTextures);
  gld_CleanDisplayLists();
  gl_preprocessed = false;
}

void gld_CleanStaticMemory(void)
{
  gld_CleanTexItems(numlumps, &gld_GLStaticPatchTextures);
}
